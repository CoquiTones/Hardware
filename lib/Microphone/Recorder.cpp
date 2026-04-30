// Microphone.cpp
#include "Recorder.h"

Recorder::Recorder(SDCARD &sd, INMP441 &mic)
    : sd(sd), mic(mic), sd_initialized(false) {}

Recorder::~Recorder() {
  // Resources managed by singleton instances
}

void Recorder::initialize() {
  sd_initialized = sd.setup();

  if (!sd_initialized) {
    Serial.println("SD card initialization failed!");
    return;
  }

  if (!mic.begin(16000)) {
    Serial.println("Microphone initialization failed!");
    return;
  }

  Serial.println("Microphone setup complete");
}

void Recorder::createWavHeader(uint8_t *header, uint32_t pcm_data_size,
                               uint32_t sample_rate) {
  pcm_wav_header_t wav_header =
      PCM_WAV_HEADER_DEFAULT(pcm_data_size,   // wav_sample_size
                             BITS_PER_SAMPLE, // wav_sample_bits (32-bit)
                             sample_rate,     // wav_sample_rate
                             NUM_CHANNELS     // wav_channel_num (mono)
      );

  memcpy(header, &wav_header, sizeof(pcm_wav_header_t));
}

const char *Recorder::recordFiveMinutesToFile(const char *fname) {
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

  // Recording parameters
  const uint32_t TOTAL_DURATION_ms = 300000; // 5 minutes
  const uint32_t AUDIO_SAMPLE_RATE = 16000;
  const float GAIN_FACTOR = 32.0f;

  Serial.printf("Starting 5-minute continuous recording to: %s\n", fname);
  Serial.printf("Gain factor: %.2f\n", GAIN_FACTOR);

  // Calculate total PCM data size for WAV header
  uint32_t num_samples = (AUDIO_SAMPLE_RATE / 1000) * TOTAL_DURATION_ms;
  uint32_t total_pcm_size = num_samples * (BITS_PER_SAMPLE / 8) * NUM_CHANNELS;

  Serial.printf("Total PCM size for header: %d bytes\n", total_pcm_size);

  // Create and write WAV header
  uint8_t wav_header[PCM_WAV_HEADER_SIZE];
  createWavHeader(wav_header, total_pcm_size, AUDIO_SAMPLE_RATE);

  if (!sd.appendToFile(SD, fname, wav_header, PCM_WAV_HEADER_SIZE)) {
    snprintf(result_msg, sizeof(result_msg),
             "Error: Failed to write WAV header to SD card");
    Serial.println(result_msg);
    return result_msg;
  }

  Serial.printf("WAV header written (%d bytes)\n", PCM_WAV_HEADER_SIZE);

  // START CONTINUOUS RECORDING IN BACKGROUND TASK
  if (!mic.startContinuousRecording(500)) { // 500ms chunks
    snprintf(result_msg, sizeof(result_msg),
             "Error: Failed to start continuous recording");
    Serial.println(result_msg);
    return result_msg;
  }

  Serial.println("Continuous recording started - capturing audio...");

  uint32_t total_bytes_written = 0;
  uint32_t chunk_count = 0;
  uint32_t start_time = millis();
  uint32_t last_print = start_time;

  // MAIN WRITE LOOP - continuously receive chunks and write to SD
  // No gaps between chunks because recording task runs on separate core
  while (millis() - start_time < TOTAL_DURATION_ms) {
    AudioChunk chunk;

    // Get next audio chunk from queue (blocks until available)
    // Timeout of 2 seconds - if no data arrives, timeout and check timer
    if (mic.getAudioChunk(chunk, 2000)) {
      // Apply gain to the chunk data
      int32_t *samples = (int32_t *)chunk.data;
      uint32_t num_chunk_samples = chunk.size / sizeof(int32_t);

      for (uint32_t i = 0; i < num_chunk_samples; i++) {
        float amplified = (float)samples[i] * GAIN_FACTOR;
        float normalized = amplified / 2147483647.0f;
        float clipped = tanhf(normalized); // Soft clipping
        samples[i] = (int32_t)(clipped * 2147483647.0f);
      }

      // Write processed chunk to SD card
      if (!sd.appendToFile(SD, fname, chunk.data, chunk.size)) {
        mic.stopContinuousRecording();
        snprintf(result_msg, sizeof(result_msg),
                 "Error: Failed to write chunk %d to SD card", chunk_count + 1);
        Serial.println(result_msg);
        free(chunk.data);
        return result_msg;
      }

      total_bytes_written += chunk.size;
      chunk_count++;

      // Free the chunk buffer after writing
      free(chunk.data);

      // Print progress every 10 seconds
      uint32_t elapsed = millis() - start_time;
      if (elapsed - last_print >= 10000) {
        uint32_t elapsed_sec = elapsed / 1000;
        uint32_t remaining_sec = (TOTAL_DURATION_ms - elapsed) / 1000;
        float mb_written = total_bytes_written / (1024.0f * 1024.0f);
        Serial.printf("[%d/%d sec] Chunk %d written | %.2f MB total | "
                      "Queue healthy\n",
                      elapsed_sec, TOTAL_DURATION_ms / 1000, chunk_count,
                      mb_written);
        last_print = elapsed;
      }
    } else {
      // Timeout waiting for chunk - could indicate recording issue
      uint32_t elapsed = millis() - start_time;
      Serial.printf("Warning: Timeout waiting for chunk at %d ms\n", elapsed);
    }
  }

  // STOP CONTINUOUS RECORDING
  mic.stopContinuousRecording();

  // Final summary
  uint32_t elapsed = millis() - start_time;
  float mb_total = total_bytes_written / (1024.0f * 1024.0f);

  snprintf(result_msg, sizeof(result_msg),
           "Success: 5-min recording complete | %d chunks | %.2f MB | %d ms",
           chunk_count, mb_total, elapsed);

  Serial.println("\n=== Recording Complete ===");
  Serial.println(result_msg);
  Serial.printf("Average chunk write time: %.2f ms\n",
                (float)elapsed / chunk_count);

  return result_msg;
}

const char *Recorder::recordDurationToFile(const char *fname,
                                           uint32_t duration_ms) {
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

  const uint32_t AUDIO_SAMPLE_RATE = 16000;
  const float GAIN_FACTOR = 32.0f;

  Serial.printf("Starting %d ms continuous recording to: %s\n", duration_ms,
                fname);

  // Calculate total PCM data size
  uint32_t num_samples = (AUDIO_SAMPLE_RATE / 1000) * duration_ms;
  uint32_t total_pcm_size = num_samples * (BITS_PER_SAMPLE / 8) * NUM_CHANNELS;

  // Create and write WAV header
  uint8_t wav_header[PCM_WAV_HEADER_SIZE];
  createWavHeader(wav_header, total_pcm_size, AUDIO_SAMPLE_RATE);

  if (!sd.appendToFile(SD, fname, wav_header, PCM_WAV_HEADER_SIZE)) {
    snprintf(result_msg, sizeof(result_msg),
             "Error: Failed to write WAV header");
    return result_msg;
  }

  // Start continuous recording
  if (!mic.startContinuousRecording(500)) {
    snprintf(result_msg, sizeof(result_msg),
             "Error: Failed to start recording");
    return result_msg;
  }

  uint32_t total_bytes_written = 0;
  uint32_t chunk_count = 0;
  uint32_t start_time = millis();

  // Record for specified duration
  while (millis() - start_time < duration_ms) {
    AudioChunk chunk;

    if (mic.getAudioChunk(chunk, 2000)) {
      // Apply gain
      int32_t *samples = (int32_t *)chunk.data;
      uint32_t num_chunk_samples = chunk.size / sizeof(int32_t);

      for (uint32_t i = 0; i < num_chunk_samples; i++) {
        float amplified = (float)samples[i] * GAIN_FACTOR;
        float normalized = amplified / 2147483647.0f;
        float clipped = tanhf(normalized);
        samples[i] = (int32_t)(clipped * 2147483647.0f);
      }

      // Write to SD
      if (!sd.appendToFile(SD, fname, chunk.data, chunk.size)) {
        mic.stopContinuousRecording();
        snprintf(result_msg, sizeof(result_msg),
                 "Error: Failed to write to SD at chunk %d", chunk_count + 1);
        free(chunk.data);
        return result_msg;
      }

      total_bytes_written += chunk.size;
      chunk_count++;
      free(chunk.data);
    }
  }

  mic.stopContinuousRecording();

  float mb_total = total_bytes_written / (1024.0f * 1024.0f);
  snprintf(result_msg, sizeof(result_msg),
           "Success: Recording complete | %d chunks | %.2f MB", chunk_count,
           mb_total);

  Serial.println(result_msg);
  return result_msg;
}
