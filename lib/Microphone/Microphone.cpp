#include "Microphone.h"

// Constructor
Microphone::Microphone(SDCARD &sd, INMP441 &mic)
    : sd(sd), mic(mic), sd_initialized(false) {}

// Destructor
Microphone::~Microphone() {
  // Resources managed by singleton instances
}

// Setup SD card and microphone
void Microphone::initialize() {
  sd_initialized = sd.setup();

  if (!sd_initialized) {
    Serial.println("SD card initialization failed!");
    return;
  }

  // Initialize microphone with 16kHz sample rate
  if (!mic.begin(16000)) {
    Serial.println("Microphone initialization failed!");
    return;
  }

  Serial.println("Microphone setup complete");
}

// Create WAV header for PCM data
void Microphone::createWavHeader(uint8_t *header, uint32_t pcm_data_size,
                                 uint32_t sample_rate) {
  pcm_wav_header_t wav_header =
      PCM_WAV_HEADER_DEFAULT(pcm_data_size,   // wav_sample_size
                             BITS_PER_SAMPLE, // wav_sample_bits (32-bit)
                             sample_rate,     // wav_sample_rate
                             NUM_CHANNELS     // wav_channel_num (mono)
      );

  memcpy(header, &wav_header, sizeof(pcm_wav_header_t));
}

const char *Microphone::recordFiveMinutesToFile(const char *fname) {
  static char result_msg[128];

  if (!fname || strlen(fname) == 0) {
    snprintf(result_msg, sizeof(result_msg), "Error: Invalid filename");
    return result_msg;
  }

  if (!sd_initialized) {
    snprintf(result_msg, sizeof(result_msg), "Error: SD card not initialized");
    return result_msg;
  }

  if (!mic.begin(16000)) {
    snprintf(result_msg, sizeof(result_msg),
             "Error: Microphone not initialized");
    return result_msg;
  }

  // Recording parameters - REDUCED TO 1-SECOND CHUNKS
  const uint32_t CHUNK_DURATION_ms = 1000;   // 1 second (64 KB)
  const uint32_t TOTAL_DURATION_ms = 300000; // 5 minutes (300,000 ms) - FIXED!
  const uint32_t NUM_CHUNKS =
      TOTAL_DURATION_ms / CHUNK_DURATION_ms; // 300 chunks
  const uint32_t AUDIO_SAMPLE_RATE = 16000;

  // GAIN ADJUSTMENT - Change this value to adjust volume
  // 1.0 = no change, 2.0 = 2x louder, 4.0 = 4x louder, etc.
  const float GAIN_FACTOR = 4.0f;

  Serial.printf("Starting 5-minute recording to file: %s\n", fname);
  Serial.printf("Total chunks: %d, Chunk duration: %d ms\n", NUM_CHUNKS,
                CHUNK_DURATION_ms);
  Serial.printf("Gain factor: %.2f\n", GAIN_FACTOR);

  // Calculate total PCM data size for the WAV header
  uint32_t num_samples = (AUDIO_SAMPLE_RATE / 1000) * TOTAL_DURATION_ms;
  uint32_t total_pcm_size = num_samples * (BITS_PER_SAMPLE / 8) * NUM_CHANNELS;

  Serial.printf("Total PCM size for header: %d bytes\n", total_pcm_size);

  // Create WAV header once for the entire 5-minute recording
  uint8_t wav_header[PCM_WAV_HEADER_SIZE];
  createWavHeader(wav_header, total_pcm_size, AUDIO_SAMPLE_RATE);

  // Write WAV header to file (only once at the beginning)
  if (!sd.appendToFile(SD, fname, wav_header, PCM_WAV_HEADER_SIZE)) {
    snprintf(result_msg, sizeof(result_msg),
             "Error: Failed to write WAV header to SD card");
    Serial.println(result_msg);
    return result_msg;
  }

  Serial.printf("WAV header written (%d bytes)\n", PCM_WAV_HEADER_SIZE);

  // Record and append each chunk
  for (uint32_t i = 0; i < NUM_CHUNKS; i++) {
    Serial.printf("\n--- Recording chunk %d of %d ---\n", i + 1, NUM_CHUNKS);

    // Record raw PCM data for this chunk (no header)
    if (!mic.recordPCMOnly(CHUNK_DURATION_ms)) {
      snprintf(result_msg, sizeof(result_msg),
               "Error: Recording failed at chunk %d", i + 1);
      Serial.println(result_msg);
      return result_msg;
    }

    // Apply gain to amplify the audio
    mic.applyGain(GAIN_FACTOR);

    // Get the PCM buffer and size
    uint8_t *buffer = mic.getPCMBuffer();
    uint32_t buffer_size = mic.getWavSize();

    // Validate buffer
    if (!buffer || buffer_size == 0) {
      snprintf(result_msg, sizeof(result_msg),
               "Error: Empty buffer at chunk %d", i + 1);
      Serial.println(result_msg);
      return result_msg;
    }

    Serial.printf("Chunk %d recorded: %d bytes\n", i + 1, buffer_size);

    // Append raw PCM data to file
    if (!sd.appendToFile(SD, fname, buffer, buffer_size)) {
      snprintf(result_msg, sizeof(result_msg),
               "Error: Failed to write chunk %d to SD card", i + 1);
      Serial.println(result_msg);
      return result_msg;
    }

    Serial.printf("Chunk %d written to file\n", i + 1);
  }

  // Success message
  snprintf(result_msg, sizeof(result_msg),
           "Success: 5-minute recording saved to %s", fname);
  Serial.println("\n=== Recording Complete ===");
  Serial.println(result_msg);

  return result_msg;
}
