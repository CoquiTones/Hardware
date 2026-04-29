#ifndef INMP441_H
#define INMP441_H

#include "Arduino.h"
#include "wav_header.h"
#include <driver/gpio.h>
#include <driver/i2s.h>
#include <esp_heap_caps.h>

#define I2S_NUM I2S_NUM_0

#define SAMPLE_RATE 16000
#define BUFFER_SIZE 1024
#define BITS_PER_SAMPLE 32
#define NUM_CHANNELS 1

class INMP441 {
public:
  INMP441(int pin_sck, int pin_ws, int pin_din);
  ~INMP441();

  bool begin(uint32_t sample_rate = SAMPLE_RATE);

  // Record audio and return WAV-formatted buffer (header + PCM data)
  bool recordWAV(uint32_t duration_ms);

  // Record only PCM data without WAV header (for chunk-based recording)
  bool recordPCMOnly(uint32_t duration_ms);

  void clearBuffer();

  // Apply gain to PCM buffer
  void applyGain(float gain_factor);

  uint8_t *getWavBuffer() const { return wav_buffer; }
  uint32_t getWavSize() const { return wav_size; }
  uint8_t *getPCMBuffer() const { return wav_buffer; }

private:
  int pin_sck;
  int pin_ws;
  int pin_din;
  uint8_t *wav_buffer;
  size_t wav_size;
  uint32_t sample_rate;
  bool is_initialized;
};

#endif
