#include "WeatherSensors.h"

WeatherSensors::WeatherSensors(int bmeSDA, int bmeSCL, int rainPin) {
  this->bmeSDA = bmeSDA;
  this->bmeSCL = bmeSCL;
  this->rainPin = rainPin;
  this->initialized = false;
  // Don't initialize I2C here - do it in initialize()
};

bool WeatherSensors::initialize() {
  Serial.println("[WeatherSensors] Initializing I2C on pins SDA=" +
                 String(this->bmeSDA) + " SCL=" + String(this->bmeSCL));

  // Add a small delay before I2C init
  delay(100);

  // Initialize I2C with explicit error handling
  if (!this->I2CBME.begin(this->bmeSDA, this->bmeSCL, 400000)) {
    Serial.println("[WeatherSensors] ERROR: I2C initialization failed!");
    return false;
  }

  Serial.println("[WeatherSensors] I2C initialized successfully");
  delay(100);

  // Try to initialize BME280
  Serial.println(
      "[WeatherSensors] Attempting BME280 initialization at address 0x77...");
  bool status = this->bme.begin(0x77, &this->I2CBME);

  if (!status) {
    Serial.println("[WeatherSensors] ERROR: BME280 not found at 0x77!");
    return false;
  }

  Serial.println("[WeatherSensors] BME280 initialized successfully");

  this->initialized = true;
  return true;
}

float WeatherSensors::getTemperature() {
  if (!this->initialized)
    return 0.0;
  return (this->bme.readTemperature() * 1.8) + 32;
}

float WeatherSensors::getPressure() {
  if (!this->initialized)
    return 0.0;
  return this->bme.readPressure();
}

float WeatherSensors::getHumidity() {
  if (!this->initialized)
    return 0.0;
  return this->bme.readHumidity();
}

float WeatherSensors::getAltitude() {
  if (!this->initialized)
    return 0.0;
  return this->bme.readAltitude(SEALEVELPRESSURE_HPA);
}

bool WeatherSensors::isRaining() {
  int rain_state = digitalRead(this->rainPin);
  Serial.println("Rain sensor state: " + String(rain_state));
  return !(rain_state == HIGH);
}

void WeatherSensors::printAllValues() {
  if (!this->initialized) {
    Serial.println("[WeatherSensors] Sensors not initialized!");
    return;
  }

  float temperature = this->bme.readTemperature();
  float humidity = this->bme.readHumidity();
  float altitude = this->bme.readAltitude(SEALEVELPRESSURE_HPA);
  float pressure = this->bme.readPressure();
  bool isRaining = this->isRaining();

  Serial.println("Temperature: " + String(temperature));
  Serial.println("Humidity: " + String(humidity));
  Serial.println("Altitude: " + String(altitude));
  Serial.println("Pressure: " + String(pressure));
  Serial.println("Is Raining: " + String(isRaining));
}

WeatherSensors::~WeatherSensors() {
  // Don't delete this!
}
