#include "INMP441.h"
#include "hal/i2s_types.h"

INMP441::INMP441(int pin_sck, int pin_ws, int pin_din)
    : pin_sck(pin_sck), pin_ws(pin_ws), pin_din(pin_din), wav_buffer(nullptr),
      wav_size(0), is_initialized(false), audio_queue(nullptr),
      recording_task_handle(nullptr), recording_active(false) {}

INMP441::~INMP441() {
  stopContinuousRecording();
  clearBuffer();
  if (is_initialized) {
    i2s_driver_uninstall(I2S_NUM);
  }
}

bool INMP441::init(uint32_t sample_rate) {
  if (is_initialized)
    return true;

  this->sample_rate = sample_rate;

  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = sample_rate,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8, // Increased for continuous recording
      .dma_buf_len = 512,
      .use_apll = true,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0};

  if (i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) != ESP_OK) {
    Serial.println("I2S driver install failed!");
    return false;
  }

  i2s_pin_config_t pin_config = {.mck_io_num = -1,
                                 .bck_io_num = pin_sck,
                                 .ws_io_num = pin_ws,
                                 .data_out_num = -1,
                                 .data_in_num = pin_din};

  if (i2s_set_pin(I2S_NUM_0, &pin_config) != ESP_OK) {
    Serial.println("I2S set pins failed!");
    return false;
  }

  is_initialized = true;
  Serial.println("INMP441 initialized successfully");
  return true;
}

void INMP441::end() {
  stopContinuousRecording();
  if (is_initialized) {
    i2s_driver_uninstall(I2S_NUM_0);
    is_initialized = false;
  }
}

bool INMP441::startContinuousRecording(uint32_t chunk_size_ms) {
  if (!is_initialized) {
    Serial.println("INMP441 not initialized!");
    return false;
  }

  if (recording_active) {
    Serial.println("Recording already in progress!");
    return true;
  }

  // Create queue to hold 3 chunks at a time (3x buffering)
  audio_queue = xQueueCreate(3, sizeof(AudioChunk));
  if (!audio_queue) {
    Serial.println("Failed to create audio queue!");
    return false;
  }

  recording_active = true;

  // Create high-priority task pinned to core 0
  BaseType_t result =
      xTaskCreatePinnedToCore(recordingTaskStatic, "AudioRecorder",
                              8192, // Stack size
                              this, // Pass 'this' pointer
                              3,    // High priority (0-24, 24 is max)
                              &recording_task_handle,
                              0 // Core 0
      );

  if (result != pdPASS) {
    Serial.println("Failed to create recording task!");
    vQueueDelete(audio_queue);
    audio_queue = nullptr;
    recording_active = false;
    return false;
  }

  Serial.printf("Continuous recording started (chunk size: %d ms)\n",
                chunk_size_ms);
  return true;
}

bool INMP441::stopContinuousRecording() {
  if (!recording_active)
    return true;

  recording_active = false;

  // Wait for task to complete
  if (recording_task_handle) {
    vTaskDelete(recording_task_handle);
    recording_task_handle = nullptr;
  }

  // Clean up queue
  if (audio_queue) {
    vQueueDelete(audio_queue);
    audio_queue = nullptr;
  }

  Serial.println("Continuous recording stopped");
  return true;
}

void INMP441::recordingTask() {
  const uint32_t CHUNK_SIZE_MS = 500; // 500ms chunks = minimal latency
  uint32_t chunk_samples = (sample_rate / 1000) * CHUNK_SIZE_MS;
  uint32_t chunk_bytes = chunk_samples * (BITS_PER_SAMPLE / 8) * NUM_CHANNELS;

  Serial.printf("Recording task started: %d bytes per chunk\n", chunk_bytes);

  while (recording_active) {
    // Allocate buffer for this chunk
    uint8_t *chunk_buffer = (uint8_t *)malloc(chunk_bytes);
    if (!chunk_buffer) {
      Serial.println("Failed to allocate chunk buffer!");
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    // Read from I2S (blocking call, I2S DMA handles the heavy lifting)
    size_t bytes_read = 0;
    esp_err_t err = i2s_read(I2S_NUM, chunk_buffer, chunk_bytes, &bytes_read,
                             portMAX_DELAY);

    if (err != ESP_OK) {
      Serial.println("I2S read error!");
      free(chunk_buffer);
      continue;
    }

    if (bytes_read > 0) {
      // Create chunk structure
      AudioChunk chunk = {.data = chunk_buffer,
                          .size = bytes_read,
                          .timestamp = xTaskGetTickCount()};

      // Send to queue (non-blocking with 100ms timeout)
      if (xQueueSend(audio_queue, &chunk, pdMS_TO_TICKS(100)) != pdPASS) {
        Serial.println("Warning: Audio queue full, dropping chunk!");
        free(chunk_buffer);
      }
    } else {
      free(chunk_buffer);
    }
  }

  vTaskDelete(NULL); // Delete self
}

bool INMP441::getAudioChunk(AudioChunk &chunk, uint32_t timeout_ms) {
  if (!audio_queue) {
    return false;
  }

  if (xQueueReceive(audio_queue, &chunk, pdMS_TO_TICKS(timeout_ms)) == pdPASS) {
    return true;
  }

  return false;
}

bool INMP441::recordPCMOnly(uint32_t duration_ms) {
  if (!is_initialized) {
    Serial.println("INMP441 not initialized!");
    return false;
  }

  clearBuffer();

  uint32_t num_samples = (sample_rate / 1000) * duration_ms;
  uint32_t pcm_data_size = num_samples * (BITS_PER_SAMPLE / 8) * NUM_CHANNELS;

  uint32_t free_heap = esp_get_free_heap_size();
  uint32_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

  Serial.printf("Free heap: %d bytes, Largest block: %d bytes\n", free_heap,
                largest_block);

  if (largest_block < pcm_data_size) {
    Serial.printf("Insufficient memory: need %d, have %d\n", pcm_data_size,
                  largest_block);
    return false;
  }

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

  wav_size = bytes_read;
  Serial.printf("PCM data recorded: %d bytes\n", wav_size);
  return true;
}

void INMP441::applyGain(float gain_factor) {
  if (!wav_buffer || wav_size == 0) {
    Serial.println("Error: No audio buffer!");
    return;
  }

  int32_t *samples = (int32_t *)wav_buffer;
  uint32_t num_samples = wav_size / sizeof(int32_t);

  for (uint32_t i = 0; i < num_samples; i++) {
    float amplified = (float)samples[i] * gain_factor;
    float normalized = amplified / 2147483647.0f;
    float clipped = tanhf(normalized);
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
