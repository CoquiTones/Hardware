#ifndef MICROPHONE_H
#define MICROPHONE_H

#include "INMP441.h"
#include "SDCard.h"

class Microphone {
public:
  Microphone(SDCARD &sd, INMP441 &mic);
  ~Microphone();

  void initialize();
  const char *recordFiveMinutesToFile(const char *fname);

private:
  void createWavHeader(uint8_t *header, uint32_t pcm_data_size,
                       uint32_t sample_rate);

  SDCARD &sd;
  INMP441 &mic;
  bool sd_initialized;
};

#endif // MICROPHONE_H
