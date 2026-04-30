#ifndef INMP441_H
#define INMP441_H

#include "Arduino.h"
#include "HardwareSerial.h"
#include "wav_header.h"
#include <driver/gpio.h>
#include <driver/i2s.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// Ring buffer chunk structure
typedef struct {
  uint8_t *data;
  uint32_t size;
  uint32_t timestamp;
} AudioChunk;

class INMP441 {
private:
  int pin_sck, pin_ws, pin_din, i2s_num;
  uint8_t *wav_buffer;
  uint32_t wav_size;
  bool is_initialized;

  const int BITS_PER_SAMPLE = 16;
  uint32_t sample_rate;
  // Continuous recording members
  QueueHandle_t audio_queue;
  TaskHandle_t recording_task_handle;
  bool recording_active;

  // Static task wrapper (required for FreeRTOS)
  static void recordingTaskStatic(void *arg) {
    ((INMP441 *)arg)->recordingTask();
  }

  // Main recording task - runs continuously in background
  void recordingTask();

public:
  INMP441(int pin_sck, int pin_ws, int pin_din);
  ~INMP441();

  bool init(uint32_t sample_rate);
  void end();

  // Continuous recording - starts background task
  bool startContinuousRecording(uint32_t chunk_size_ms = 500);
  bool stopContinuousRecording();

  // Get next chunk from queue (blocking)
  bool getAudioChunk(AudioChunk &chunk, uint32_t timeout_ms = 1000);

  // Legacy method - still available
  bool recordPCMOnly(uint32_t duration_ms);

  void applyGain(float gain_factor);
  void clearBuffer();

  uint8_t *getPCMBuffer() const { return wav_buffer; }
  uint32_t getWavSize() const { return wav_size; }
  bool isRecording() const { return recording_active; }
};

#endif
