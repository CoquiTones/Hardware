#ifndef INMP441_H
#define INMP441_H

// ====== INMP441 ======
#define I2S_MIC_LR_PIN_GND true
#define I2S_PIN_WS 13
#define I2S_PIN_BCK 12
#define I2S_PIN_SDIN 11
#define I2S_SAMPLE_RATE 64000
#define I2S_BUF_COUNT 6
#define I2S_BUF_LEN (I2S_SAMPLE_RATE / 200)
#define I2S_SAMPLE_SIZE sizeof(uint32_t)
#define I2S_BUFFER_SAMPLES ((I2S_BUF_COUNT - 1) * I2S_BUF_LEN)
#define I2S_BUFFER_BYTES (I2S_BUFFER_SAMPLES * I2S_SAMPLE_SIZE)

#include "esp_heap_caps.h"
#include <Arduino.h>
#include <driver/i2s_std.h>
// ===== INMP441 =====
class INMP441 {
public:
  INMP441();
  ~INMP441();

  bool begin(int bck = I2S_PIN_BCK, int ws = I2S_PIN_WS, int din = I2S_PIN_SDIN,
             int sample_rate = I2S_SAMPLE_RATE, int buf_count = I2S_BUF_COUNT,
             int buf_len = I2S_BUF_LEN, bool channelLeft = I2S_MIC_LR_PIN_GND);

  bool read(uint8_t *destination, size_t bytes_to_read, size_t *read);

private:
  i2s_chan_handle_t rx_handle;
  uint8_t *aux_buffer;
  size_t aux_buffer_size;
  static constexpr size_t word_size = 4;
  bool channelLeft;
};

#endif