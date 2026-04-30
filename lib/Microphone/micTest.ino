#include "Recorder.h"

// Create Recorder instance
Recorder recorder;

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("\n\n=== INMP441 Recorder Recording Test ===\n");

  // Initialize Recorder and SD card
  Serial.println("Initializing Recorder and SD card...");
  recorder.setup();

  delay(2000); // Allow time for initialization

  // Start 5-minute recording
  Serial.println("\nStarting 5-minute recording...");
  Serial.println("Do not disconnect power or remove SD card!\n");

  unsigned long start_time = millis();
  const char *result = recorder.recordFiveMinutesToFile("/recording.wav");
  unsigned long elapsed_time = millis() - start_time;

  // Display results
  Serial.println("\n=== Recording Complete ===\n");
  Serial.println(result);
  Serial.printf("Total time: %lu seconds\n", elapsed_time / 1000);
  Serial.println("\nTest finished!");
}

void loop() {
  // Nothing to do after recording completes
  delay(1000);
}
