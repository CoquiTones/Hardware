#ifndef RECORDER_H
#define RECORDER_H

#include "INMP441.h"
#include "SDCard.h"

class Recorder {
private:
  SDCARD &sd;
  INMP441 &mic;
  bool sd_initialized;

  // Helper function to create WAV header
  void createWavHeader(uint8_t *header, uint32_t pcm_data_size,
                       uint32_t sample_rate);

public:
  // Constructor and Destructor
  Recorder(SDCARD &sd, INMP441 &mic);
  ~Recorder();

  // Setup SD card and microphone
  void initialize();

  // Continuous recording method - seamless audio capture
  const char *recordFiveMinutesToFile(const char *fname);

  // Optional: Legacy single-chunk recording
  const char *recordDurationToFile(const char *fname, uint32_t duration_ms);
};

#endif
