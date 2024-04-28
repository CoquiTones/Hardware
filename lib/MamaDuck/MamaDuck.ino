/**
 * @file MamaDuck.ino
 * @brief Uses the built in Mama Duck.
 */

#include <string>
#include <arduino-timer.h>
#include <CDP.h>
#include <MemoryFree.h>
#include <../lib/Microphone/MicWrapper.h>
#include <../lib/Weather/WeatherSensors.h>
#include <../lib/Microphone/lib/sd_card/src/SDCard.h>


#ifdef SERIAL_PORT_USBVIRTUAL
#define Serial SERIAL_PORT_USBVIRTUAL
#endif
#define bmeSDA 41
#define bmeSCL 42
#define rainPin 45

#define DEBUG_MAMA

bool sendData(std::vector<byte> message);
bool runSensor(void *);

// INTEGRATE VTASKS FROM https://github.com/atomic14/esp32_sdcard_audio/tree/main MAIN

// create a built-in mama duck
MamaDuck duck;
DuckDisplay* display;
Microphone *mic;
WeatherSensors *sens;

// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 5000; // 5 seconds
int counter = 1;
bool setupOK = false;
void setup()
{
	// We are using a hardcoded device id here, but it should be retrieved or
	// given during the device provisioning then converted to a byte vector to
	// setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
	// will get rejected
	Serial.begin(115200);
	Serial.println("Wait ");
	delay(3000);
	sens = new WeatherSensors(bmeSDA, bmeSCL, rainPin);
	Serial.print("\nTemp Data: ");
	Serial.println(sens->getTemperature());

	mic = new Microphone();
	Serial.println("Microphone Created");
	Serial.println("Weather Data Object Created");

	// has to be 8 chars
	std::string deviceId("MAMA0001");
	std::vector<byte> devId;
	devId.insert(devId.end(), deviceId.begin(), deviceId.end());
	if (duck.setupWithDefaults(devId) != DUCK_ERR_NONE)
	{
		Serial.println("[MAMA] Failed to setup MamaDuck");
		return;
	}

	#ifdef DEBUG_MAMA

	display = DuckDisplay::getInstance();
	display->setupDisplay(duck.getType(), devId);
	display->showDefaultScreen();

	#endif
	setupOK = true;
	// Initialize the timer. The timer thread runs separately from the main loop
	// and will trigger sending a counter message.
	timer.every(INTERVAL_MS, runSensor);
	Serial.println("[MAMA] Setup OK!");
}

std::vector<byte> stringToByteVector(const String &str)
{
	std::vector<byte> byteVec;
	byteVec.reserve(str.length());

	for (unsigned int i = 0; i < str.length(); ++i)
	{
		byteVec.push_back(static_cast<byte>(str[i]));
	}

	return byteVec;
}

void loop()
{
	if (!setupOK)
	{
		return;
	}
	timer.tick();
	// Use the default run(). The Mama duck is designed to also forward data it receives
	// from other ducks, across the network. It has a basic routing mechanism built-in
	// to prevent messages from hoping endlessly.
	duck.run();
}

bool runSensor(void *)
{

	// Record Audio for AUDIO_DURATION seconds (this value is defined in ../lib/shared/config.h)
	#ifdef DEBUG_MAMA
	display->drawString(10, 3, "Start Singing, Sexy");
	#endif
	mic->recordToFile("test.wav");
	char *read_data = mic->readFile("test.wav");
	#ifdef DEBUG_MAMA
	display->clearLine(10, 3);
	display->drawString(10, 3, "Finished!");
	#endif
	Serial.println(read_data);
	delay(3000);
	bool result;
	const byte *buffer;
	String temp = String(sens->getTemperature());
	String pres = String(sens->getPressure());
	String humid = String(sens->getHumidity());
	String israining = sens->isRaining() ? "Yes" : "No";

	// String message = "Temperature: " + temp + "*C\n" + "Pressure : " + pres + "hPa\n" + "Humidity: " + humid + "RH%\n" + "Raining: " + israining + "\n";
	String message = "Temperature: " + temp + "*C\n" + "Pressure : " + pres + "hPa\n" + "Humidity: " + humid + "RH%\n" + "Raining: " + israining + "\n";
	int length = message.length();

	Serial.print("[MAMA] sensor data: ");
	Serial.println(message);

	result = sendData(stringToByteVector(message));
	if (result)
	{
		Serial.println("[MAMA] runSensor ok.");
	}
	else
	{
		Serial.println("[MAMA] runSensor failed.");
	}
	return result;
}

bool sendData(std::vector<byte> message)
{
	bool sentOk = false;

	int err = duck.sendData(topics::status, message);
	if (err == DUCK_ERR_NONE)
	{
		counter++;
		sentOk = true;
	}
	if (!sentOk)
	{
		Serial.println("[MAMA] Failed to send data. error = " + String(err));
	}
	return sentOk;
}
