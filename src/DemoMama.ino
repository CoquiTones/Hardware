/**
 * @file MamaDuck.ino
 * @brief Uses the built in Mama Duck.
 */

#include <string>
#include <arduino-timer.h>
#include <CDP.h>
#include <MemoryFree.h>
#include <../lib/Weather/WeatherSensors.h>

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
DuckDisplay *display;
WeatherSensors *sens;
// create a timer with default settings
auto timer = timer_create_default();

// for sending the counter message
const int INTERVAL_MS = 10000; // 10 seconds
int counter = 1;
bool setupOK = false;
void setup()
{
    Serial.begin(115200);
    // We are using a hardcoded device id here, but it should be retrieved or
    // given during the device provisioning then converted to a byte vector to
    // setup the duck NOTE: The Device ID must be exactly 8 bytes otherwise it
    // will get rejected
    // has to be 8 chars

    sens = new WeatherSensors(bmeSDA, bmeSCL, rainPin);
    Serial.print("\nTemp Data: ");
    Serial.println(sens->getTemperature());

    Serial.println("Weather Data Object Created");

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

    bool result;
    const byte *buffer;
    String temp = String(sens->getTemperature());
    String pres = String(sens->getPressure());
    String humid = String(sens->getHumidity());
    String israining = sens->isRaining() ? "Yes" : "No";

    // String message = "Temperature: " + temp + "*C\n" + "Pressure : " + pres + "hPa\n" + "Humidity: " + humid + "RH%\n" + "Raining: " + israining + "\n";
    String message = "Temp: " + temp + "*F" + "Pres: " + pres + "hPa" + "Humid: " + humid + "RH%\n" + "Raining: " + israining ;
    int length = message.length();

#ifdef DEBUG_MAMA
    display->clear();
    display->drawString(0, 10, "---MAMA DATA---");
    display->drawString(0, 20, (String("Temp: ") + temp + "*F").c_str());
    display->drawString(0, 30, (String("Pres: ") + pres + "hPa").c_str());
    display->drawString(0, 40, (String("Humidity: ") + humid + "RH%").c_str());
    display->drawString(0, 50, (String("Raining: ") + israining).c_str());
    display->sendBuffer();
#endif
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
