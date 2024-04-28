#include <../lib/shared/sd_card/src/SDCard.h>
#include <../lib/shared/config.h>
#include <Arduino.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>


void setup () {

  new SDCard("/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
}

void loop() {

}