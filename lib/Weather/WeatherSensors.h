
#ifndef bmeSensor
#define bmeSensor

#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#define SEALEVELPRESSURE_HPA 1013.25

class WeatherSensors {

public:
  /**
   * @brief Construct a new Weather Data object; This object encapsulates both
   * the bme280 sensor and the rain sensor
   *
   *
   * @param bmeSDA:  pin for SDA of bme280 sensor
   * @param bmeSCL: pin for SCL of bme280 sensor
   * @param rainPin: pin for rain sensor
   */
  WeatherSensors(int bmeSDA, int bmeSCL, int rainPin);

  /**
   * @brief Prints output of all sensor data to serial console
   *
   */
  void printAllValues();

  /**
   * @brief returns float of temperature in Farenheit
   */
  float getTemperature();
  /**
   * @brief returns float of pressure in hPa
   */
  float getPressure();
  /**
   * @brief float of relative humidity in RH%
   */
  float getHumidity();
  /**
   * @brief calculates altitude (in meters) based on sea level pressure of PR
   * (1013.25hpa) and measured pressure
   */
  float getAltitude();
  /**
   * @brief returns true if rain sensor measures water
   */
  bool isRaining();

  bool initialize();
  /**
   * @brief close all pin connections
   */
  ~WeatherSensors();

private:
  TwoWire I2CBME = TwoWire(0);
  Adafruit_BME280 bme;
  int rainPin;
  int bmeSDA;
  int bmeSCL;
  bool initialized;
};

#endif