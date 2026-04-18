#ifndef CLASES_H
#define CLASES_H

#include "esp_heap_caps.h"
#include <Arduino.h>
#include <driver/i2s_std.h>

// ====== Defines ======
#define RGB_BUILTIN 48 // Pin LED interno
#define NUMPIXELS 1
#define CHANNEL_LEFT true

// ==== PSRAMBuffer ====
class PSRAMBuffer {
public:
  PSRAMBuffer();
  bool init(size_t size = 0);
  uint8_t *get();
  size_t getSize();
  void freeBuffer();

private:
  uint8_t *buffer;
  size_t size_bytes;
};
// =====================
#endif