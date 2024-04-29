#ifndef MQTT
#define MQTT

#include <BotleticsSIM7000.h>
#include <HardwareSerial.h>
#include <../SD/SDCard.h>
/************************* MQTT PARAMETERS *********************************/
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME "PAPADUCK"
#define MQTT_PASSWORD "password"

// Set topic names to publish and subscribe to
#define GPS_TOPIC "location"
#define WEATHER_TOPIC "weather"
#define AUDIO_TOPIC "audio"
#define END "end"
// For botletics SIM70X0 shield
#define PWRKEY 6
#define RST 7
// #define DTR 8 // Connect with solder jumper
// #define RI 9 // Need to enable via AT commands
#define TX 10 // Microcontroller RX
#define RX 11 // Microcontroller TX
#define APN "fast.t-mobile.com"

HardwareSerial modemSS(1);

class LTE_Wrapper
{
public:
    /**
     * @brief Create a new wrapper object for the Botletics 7000 LTE shield for handling MQTT uplink of duck
    */
    LTE_Wrapper();
    /**
     * @brief Publishes String content to topic
     * @param topic [GPS_TOPIC, WEATHER_TOPIC, AUDIO_TOPIC]: topic to publish to 
     * @param content String with content; for AUDIO encoded string of audio file 
    */
    bool publish(const char *topic, const char *content);
    Botletics_modem_LTE modem = Botletics_modem_LTE();

private:
    /**
     * @brief Handles Audio Publishing
     * @param filepath: filepath of recorded wav file 
    */
    bool handleAudioPublish(const char *filepath);
    /**
     * @brief Returns network status of sim. Example output: "Registered roaming"
    */
    bool netStatus();
    /**
     * @brief setup all components for LTE wrapper including modem object, and SD card
    */
    bool setup();
    SDCARD *sd;
};
#endif