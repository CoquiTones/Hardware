#include "Microphone.h"
#include "config.h"
#include "lib/INMP441.h"
#include "lib/PSRAMBuffer.h"

// Constructor
Microphone::Microphone() : sd_initialized(false) { setupSDCard(); }

// Destructor
Microphone::~Microphone() {
  // SD library cleanup (automatic on end scope)
  SD->end();
}

// Record 5 minutes of audio to file
const char *Microphone::recordToFile(const char *fname) {
  // Verify SD card is ready
  if (!sd_initialized) {
    Serial.println("SD card not initialized!");
    return nullptr;
  }

  // Calculate required PSRAM size for 5 minutes
  // Formula: sample_rate (Hz) × duration (sec) × sample_size (bytes)
  const size_t recording_duration_seconds = 300; // 5 minutes
  size_t required_psram_bytes =
      I2S_SAMPLE_RATE * recording_duration_seconds * I2S_SAMPLE_SIZE;

  Serial.printf("Allocating PSRAM: %u bytes for 5-minute recording\n",
                (unsigned)required_psram_bytes);

  if (!buffer->init(required_psram_bytes)) {
    Serial.println("Failed to allocate PSRAM - not enough memory!");
    return nullptr;
  }

  // Initialize I2S microphone
  Serial.println("Initializing INMP441 microphone...");
  if (!mic.begin(I2S_PIN_BCK, I2S_PIN_WS, I2S_PIN_SDIN, I2S_SAMPLE_RATE,
                 I2S_BUF_COUNT, I2S_BUF_LEN, I2S_MIC_LR_PIN_GND)) {
    Serial.println("Failed to initialize INMP441 microphone!");
    buffer->freeBuffer();
    return nullptr;
  }

  Serial.println("========================================");
  Serial.println("Starting 5-minute recording...");
  Serial.println("Do not interrupt power or SD card!");
  Serial.println("========================================");

  // Record audio into PSRAM
  size_t offset = 0;
  size_t psram_size = buffer->getSize();
  size_t read_count = 0;
  uint32_t start_time = millis();

  while (offset < psram_size) {
    // Progress indicator
    if (read_count % 25 == 0) {
      float elapsed = (float)(millis() - start_time) / 1000.0f;
      Serial.printf("[%.1fs] ", elapsed);
      Serial.print(".");
    }
    read_count++;

    // Calculate remaining bytes
    size_t buffer_bytes = I2S_BUFFER_BYTES;
    size_t remaining = psram_size - offset;
    if (buffer_bytes > remaining) {
      buffer_bytes = remaining;
    }

    // Read from microphone
    size_t bytesRead = 0;
    if (mic.read(buffer->get() + offset, buffer_bytes, &bytesRead) &&
        bytesRead > 0) {
      offset += bytesRead;
    } else {
      Serial.println("\nError reading from I2S microphone!");
      buffer->freeBuffer();
      return nullptr;
    }
  }

  uint32_t recording_time = millis() - start_time;

  // Calculate statistics
  size_t total_samples = offset / I2S_SAMPLE_SIZE;
  float duration_sec = (float)total_samples / I2S_SAMPLE_RATE;

  Serial.printf("\n========================================\n");
  Serial.printf("Recording Complete:\n");
  Serial.printf("  Time elapsed: %u ms\n", recording_time);
  Serial.printf("  Bytes recorded: %u\n", (unsigned)offset);
  Serial.printf("  Total samples: %u\n", (unsigned)total_samples);
  Serial.printf("  Duration: %.2f seconds\n", duration_sec);
  Serial.printf("========================================\n");

  // Write PSRAM data to SD card
  Serial.printf("Writing to SD card: %s\n", fname);

  File file = SD.open(fname, FILE_WRITE);
  if (!file) {
    Serial.printf("Failed to open file %s for writing!\n", fname);
    buffer->freeBuffer();
    return nullptr;
  }

  // Write in blocks
  size_t pos = 0;
  const size_t BLOCK_SIZE = 4096; // 4KB blocks for optimal SD performance
  uint32_t sd_write_start = millis();

  while (pos < offset) {
    size_t toWrite = (offset - pos) > BLOCK_SIZE ? BLOCK_SIZE : (offset - pos);
    size_t written = file.write((uint8_t *)buffer->get() + pos, toWrite);

    if (written != toWrite) {
      Serial.printf("Error writing block at position %u\n", (unsigned)pos);
      file.close();
      buffer->freeBuffer();
      return nullptr;
    }

    pos += written;

    // Progress indicator every 40KB
    if (pos % (BLOCK_SIZE * 10) == 0) {
      float progress = (float)pos / (float)offset * 100.0f;
      Serial.printf("  %.1f%% written (%u/%u bytes)\n", progress, (unsigned)pos,
                    (unsigned)offset);
    }
  }

  // Finalize file
  file.flush();
  file.close();

  uint32_t sd_write_time = millis() - sd_write_start;

  Serial.printf("========================================\n");
  Serial.printf("SD Card Write Complete:\n");
  Serial.printf("  Time: %u ms\n", sd_write_time);
  Serial.printf("  Speed: %.2f KB/s\n",
                (float)offset / 1024.0f / ((float)sd_write_time / 1000.0f));
  Serial.printf("  File: %s\n", fname);
  Serial.printf("  Size: %u bytes\n", (unsigned)offset);
  Serial.printf("========================================\n");

  // Cleanup
  buffer->freeBuffer();

  return fname;
}

// Take single measurement from microphone
int32_t Microphone::takeMeasurement() {
  static INMP441 *mic = nullptr;
  static bool initialized = false;

  // Lazy initialization
  if (!initialized) {
    mic = new INMP441();
    if (!mic->begin(I2S_PIN_BCK, I2S_PIN_WS, I2S_PIN_SDIN, I2S_SAMPLE_RATE,
                    I2S_BUF_COUNT, I2S_BUF_LEN, I2S_MIC_LR_PIN_GND)) {
      Serial.println("Failed to initialize microphone for measurement!");
      delete mic;
      mic = nullptr;
      return 0;
    }
    initialized = true;
    Serial.println("Microphone initialized for measurements");
  }

  if (!mic)
    return 0;

  // Read single sample
  uint8_t sample_buffer[I2S_SAMPLE_SIZE];
  size_t bytesRead = 0;

  if (mic->read(sample_buffer, I2S_SAMPLE_SIZE, &bytesRead) && bytesRead > 0) {
    int32_t *sample = (int32_t *)sample_buffer;
    return sample[0];
  }

  return 0;
}
