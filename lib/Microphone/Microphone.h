#ifndef MICROPHONE_H
#define MICROPHONE_H

#include "../SD/SDCard.h"
#include "config.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

// Forward declarations
class INMP441;
class PSRAMBuffer;

/**
 * @class Microphone
 * @brief Wrapper class for INMP441 microphone with SD card recording
 *
 * This class provides high-level audio recording functionality using an
 * INMP441 I2S microphone. Records audio to PSRAM and saves to SD card
 * as binary files.
 *
 * **Requirements:**
 * - INMP441 microphone connected to I2S pins (see config.h)
 * - SD card reader connected to SPI pins (see config.h)
 * - PSRAM available on ESP32
 * - config.h with pin definitions and I2S parameters
 */
class Microphone {
public:
  /**
   * @brief Construct a new Microphone object
   * Initializes SD card and microphone resources
   */
  Microphone();

  /**
   * @brief Destructor
   * Cleanup resources (private, use static/stack allocation)
   */
  ~Microphone();

  /**
   * @brief Record 5 minutes of audio to a binary file
   *
   * Records 5 minutes (300 seconds) of audio from the INMP441 microphone
   * into PSRAM buffer, then writes the complete buffer to an SD card
   * binary file. The recording runs to completion before returning.
   *
   * @param fname Filename for output (e.g., "/audio_001.bin")
   *              Path is relative to SD card root
   *
   * @return const char* Pointer to filename if successful, nullptr on error
   *
   * **Example:**
   * ```cpp
   * Microphone recorder;
   * const char *result = recorder.recordToFile("/recording.bin");
   * if (result) {
   *   Serial.printf("Saved to: %s\n", result);
   * } else {
   *   Serial.println("Recording failed!");
   * }
   * ```
   *
   * **Timing:** Approximately 5 minutes + SD write time (varies by card speed)
   * **Memory:** Uses nearly all available PSRAM (calculate size based on sample
   * rate)
   */
  const char *recordToFile(const char *fname);

  /**
   * @brief Take a single audio measurement from the microphone
   *
   * Captures one audio sample (smallest possible read from I2S).
   * Useful for continuous monitoring or transmitting raw data between nodes.
   * The microphone is initialized on first call and persists across calls.
   *
   * @return int32_t Single 32-bit audio sample value
   *         Returns 0 on read error
   *
   * **Example:**
   * ```cpp
   * Microphone mic;
   * for (int i = 0; i < 100; i++) {
   *   int32\_t sample = mic.takeMeasurement();
   *   Serial.println(sample);
   * }
   * ```
   *
   * **Note:** First call initializes I2S microphone; subsequent calls reuse
   * the same microphone instance
   */
  int32_t takeMeasurement();

  void setup();

private:
  SDCARD *sd;
  PSRAMBuffer *buffer;
  INMP441 *mic;

  // Member variables
  bool sd_initialized; ///< Track SD card initialization status
};

#endif // MICROPHONE_H
