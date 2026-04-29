#include "Arduino.h"
#include "INMP441.h"
#include "Microphone.h"
#include "SDCard.h"
#include "WeatherSensors.h"

#define SD_PIN_CS 4
#define SD_PIN_SCLK 5
#define SD_PIN_MOSI 6
#define SD_PIN_MISO 7

#define MIC_PIN_SCK 19 // green
#define MIC_PIN_WS 20 // blue
#define MIC_PIN_DIN 47 // grey

#define BME280_PIN_SDA 1
#define BME280_PIN_SCL 2
// Create instances
SDCARD sdCard(SD_PIN_CS, SD_PIN_SCLK, SD_PIN_MOSI, SD_PIN_MISO);
INMP441 microphoneHW(MIC_PIN_SCK, MIC_PIN_WS, MIC_PIN_DIN);
Microphone microphone(sdCard, microphoneHW);
WeatherSensors sensors(BME280_PIN_SDA, BME280_PIN_SCL, 10);


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  Serial.println("\n\n=== INMP441 Microphone Recording Test ===\n");
  Serial.println("Initializing microphone and SD card...");

  microphone.initialize();
  sensors.initialize();
  delay(2000);

  Serial.println("\nStarting 5-minute recording...");
  Serial.println("Do not disconnect power or remove SD card!\n");

  unsigned long start_time = millis();
  const char *result = microphone.recordFiveMinutesToFile("/recording-with-sensors-no-cuts-maybee.wav");
  unsigned long elapsed_time = millis() - start_time;

  Serial.println("\n=== Recording Complete ===\n");
  Serial.println(result);
  Serial.printf("Total time: %lu seconds\n", elapsed_time / 1000);
  Serial.println("\nTest finished!");
}

void loop() {
  delay(1000);
  Serial.printf("\nTemperature: %f", sensors.getTemperature());
  Serial.printf("Humidity: %f", sensors.getHumidity());
}
