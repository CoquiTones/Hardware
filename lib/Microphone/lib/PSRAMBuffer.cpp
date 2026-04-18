#include "PSRAMBuffer.h"

// --- PSRAMBuffer ---
PSRAMBuffer::PSRAMBuffer() : buffer(nullptr), size_bytes(0) {}
bool PSRAMBuffer::init(size_t size) {
  Serial.println("Iniciando PSRAM");
  if (size == 0)
    size = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  if (size == 0) {
    Serial.println("No PSRAM");
    return false;
  }
  buffer = (uint8_t *)ps_malloc(size);
  if (!buffer) {
    size_bytes = 0;
    return false;
  }
  size_bytes = size;
  Serial.printf("PSRAM %d bytes\n", size_bytes);
  Serial.println("PSRAM OK");
  return true;
}
uint8_t *PSRAMBuffer::get() { return buffer; }
size_t PSRAMBuffer::getSize() { return size_bytes; }
void PSRAMBuffer::freeBuffer() {
  if (buffer)
    free(buffer);
  buffer = nullptr;
  size_bytes = 0;
}