#include "INMP441.h"

INMP441::INMP441(int pin_sck, int pin_ws, int pin_din)
    : pin_sck(pin_sck), pin_ws(pin_ws), pin_din(pin_din), wav_buffer(nullptr),
      wav_size(0), sample_rate(SAMPLE_RATE), is_initialized(false) {}

INMP441::~INMP441() {
  clearBuffer();
  if (is_initialized) {
    i2s_driver_uninstall(I2S_NUM);
  }
}

bool INMP441::begin(uint32_t sr) {
  if (is_initialized)
    return true;

  sample_rate = sr;

  // OPTIMIZED I2S CONFIG
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = sample_rate,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4, // Reduced from 8
      .dma_buf_len = 512, // Reduced from 1024
      .use_apll = true,   // CRITICAL: Use APLL for better clock accuracy
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  if (i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL) != ESP_OK) {
    Serial.println("I2S driver install failed!");
    return false;
  }

  i2s_pin_config_t pin_config = {.mck_io_num = -1,
                                 .bck_io_num = pin_sck,
                                 .ws_io_num = pin_ws,
                                 .data_out_num = -1,
                                 .data_in_num = pin_din};

  if (i2s_set_pin(I2S_NUM, &pin_config) != ESP_OK) {
    Serial.println("I2S set pins failed!");
    return false;
  }

  is_initialized = true;
  Serial.println("INMP441 initialized successfully");
  return true;
}

bool INMP441::recordPCMOnly(uint32_t duration_ms) {
  if (!is_initialized) {
    Serial.println("INMP441 not initialized!");
    return false;
  }

  clearBuffer();

  // Calculate sample counts and sizes
  uint32_t num_samples = (sample_rate / 1000) * duration_ms;
  uint32_t pcm_data_size = num_samples * (BITS_PER_SAMPLE / 8) * NUM_CHANNELS;

  // Check available heap memory before allocation
  uint32_t free_heap = esp_get_free_heap_size();
  uint32_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

  Serial.printf("Free heap: %d bytes, Largest contiguous block: %d bytes\n",
                free_heap, largest_block);
  Serial.printf("Recording PCM %d ms at %d Hz (%d bytes requested)\n",
                duration_ms, sample_rate, pcm_data_size);

  // Validate sufficient memory
  if (largest_block < pcm_data_size) {
    Serial.printf(
        "Error: Insufficient contiguous memory! Need %d bytes, have %d bytes\n",
        pcm_data_size, largest_block);
    return false;
  }

  // Allocate buffer for PCM data ONLY (no header)
  wav_buffer = (uint8_t *)malloc(pcm_data_size);
  if (!wav_buffer) {
    Serial.println("Failed to allocate PCM buffer!");
    return false;
  }

  size_t bytes_read = 0;
  if (i2s_read(I2S_NUM, wav_buffer, pcm_data_size, &bytes_read,
               portMAX_DELAY) != ESP_OK) {
    Serial.println("I2S read failed!");
    free(wav_buffer);
    wav_buffer = nullptr;
    return false;
  }

  if (bytes_read != pcm_data_size) {
    Serial.printf("Warning: Only read %d of %d bytes\n", bytes_read,
                  pcm_data_size);
  }

  wav_size = bytes_read;
  Serial.printf("PCM data recorded: %d bytes\n", wav_size);
  return true;
}

void INMP441::applyGain(float gain_factor) {
  if (!wav_buffer || wav_size == 0) {
    Serial.println("Error: No audio buffer to apply gain to!");
    return;
  }

  int32_t *samples = (int32_t *)wav_buffer;
  uint32_t num_samples = wav_size / sizeof(int32_t);

  Serial.printf("Applying gain factor: %.2f with soft clipping\n", gain_factor);

  for (uint32_t i = 0; i < num_samples; i++) {
    // Apply gain
    float amplified = (float)samples[i] * gain_factor;

    // SOFT CLIPPING using tanh (smooth, preserves quality)
    // Normalize to roughly [-1, 1] range before tanh
    float normalized = amplified / 2147483647.0f;
    float clipped = tanhf(normalized); // Smooth clipping

    // Scale back to int32 range
    samples[i] = (int32_t)(clipped * 2147483647.0f);
  }

  Serial.println("Gain applied with soft clipping");
}

void INMP441::clearBuffer() {
  if (wav_buffer) {
    free(wav_buffer);
    wav_buffer = nullptr;
    wav_size = 0;
  }
}
