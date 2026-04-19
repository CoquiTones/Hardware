#include "Arduino.h"
#include "INMP441.h"
#include "Microphone.h"
#include "SDCard.h"

// Create instances
SDCARD sdCard;
INMP441 microphoneHW;
Microphone microphone(sdCard, microphoneHW);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("\n\n=== INMP441 Microphone Recording Test ===\n");
  Serial.println("Initializing microphone and SD card...");

  microphone.initialize();
  delay(2000);

  Serial.println("\nStarting 5-minute recording...");
  Serial.println("Do not disconnect power or remove SD card!\n");

  unsigned long start_time = millis();
  const char *result = microphone.recordFiveMinutesToFile("/recording.wav");
  unsigned long elapsed_time = millis() - start_time;

  Serial.println("\n=== Recording Complete ===\n");
  Serial.println(result);
  Serial.printf("Total time: %lu seconds\n", elapsed_time / 1000);
  Serial.println("\nTest finished!");
}

void loop() { delay(1000); }
