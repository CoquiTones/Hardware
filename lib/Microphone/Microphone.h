// Microphone.hpp
#ifndef MICROPHONE_H
#define MICROPHONE_H

#include "INMP441.h"
#include "SDCard.h"

class Microphone {
private:
  SDCARD &sd;
  INMP441 &mic;
  bool sd_initialized;

  // Helper function to create WAV header
  void createWavHeader(uint8_t *header, uint32_t pcm_data_size,
                       uint32_t sample_rate);

public:
  // Constructor and Destructor
  Microphone(SDCARD &sd, INMP441 &mic);
  ~Microphone();

  // Setup SD card and microphone
  void initialize();

  // Continuous recording method - seamless audio capture
  const char *recordFiveMinutesToFile(const char *fname);

  // Optional: Legacy single-chunk recording
  const char *recordDurationToFile(const char *fname, uint32_t duration_ms);
};

#endif
