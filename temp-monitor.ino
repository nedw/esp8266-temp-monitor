#include <Arduino.h>
#include <ESP8266WebServer.h>

#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>
#include "i2c_scan.h"
#include "Temperature.h"
#include "Logger.h"
#include <time.h>
#include "temp-monitor.h"
#include "WiFi.h"
#include "WebServer.h"

#define ARRAYSIZE(x)    (sizeof(x) / sizeof(x[0]))
#define RX0_PIN         3

int           user_input  = -1;
State         state       = STATE_NORMAL;
uint16_t      delayPeriod = 200;

extern ProvisionInfo provisionInfo;

//
// NTP definitions
//

#define NTP_SERVER_NAME     "pool.ntp.org"
#define NTP_TIME_OFFSET     0
#define NTP_UPDATE_INTERVAL_MS (86400 * 3 * 1000)

time_t      bootEpochTime  = 0;
time_t      lastTimeUpdate = 0;
WiFiUDP     ntpUdp;
NTPClient   ntpClient(ntpUdp, NTP_SERVER_NAME, NTP_TIME_OFFSET, NTP_UPDATE_INTERVAL_MS);

//
// I2C Definitions
//
#define SDA_PIN         0
#define SCL_PIN         2

//
// Logger and Temperature definitions
//

Logger *logger = nullptr;
Temperature temperature;


void initLogger()
{
    logger = new Logger();
}

//
// Initialise I2C and detect any devices on the bus
//

void initI2C()
{
    Wire.begin(SDA_PIN, SCL_PIN);
    i2c_addr_t devices[10];

    int nDevices = i2c_scan(devices, ARRAYSIZE(devices));
    if (nDevices) {
        Serial.printf_P(PSTR("%d I2C device(s) found: "), nDevices);
        for (int i = 0 ; i < nDevices ; ++i) {
            Serial.printf_P(PSTR("0x%x "), devices[i]);
        }
        Serial.printf_P(PSTR("\n"));
    } else {
        Serial.printf_P(PSTR("No I2C devices found\n"));
    }
}

//
// Detect user input to move into softAP WiFi provisioning mode
//
// NOTE: serial not initialised at this point, as we temporarily use
// the RX pin for user input (ESP-01S).

void read_user_input()
{
    pinMode(RX0_PIN, INPUT_PULLUP);
    delay(1);
    user_input = digitalRead(RX0_PIN);
    delay(10);
    pinMode(RX0_PIN, INPUT);
}

//
// Main setup() routines
//

void setupNormal()
{
        initI2C();
        temperature.init();         // requires initI2C() to have been called
        initWiFi();
        WebServer::init();
        ntpClient.begin();
}

void setup()
{
    read_user_input();

    Serial.begin(74880, SERIAL_8N1, SERIAL_TX_ONLY);
    delay(500);

    Serial.printf_P(PSTR("\nUser input = %d\n"), (int)user_input);

    if (user_input == 0) {
        state = STATE_SOFTAP;
        initWiFiAP();
        WebServer::initAP();
    } else {
        state = STATE_NORMAL;
        setupNormal();
    }
}                                                                                                                                

//
// Main loop() routines for various modes
//

void softApLoop()
{
    WebServer::webServer.handleClient();
    delay(200);
}

void provisionWiFiLoopOnce()
{
    WiFi.persistent(false);             // prevent disconnects from writing to flash
    WiFi.softAPdisconnect();
    delay(10);
    WiFi.disconnect();
    delay(10);

    Serial.printf_P(PSTR("Provisioning, ssid='%s', psk='%s'\n"), provisionInfo.ssid, provisionInfo.pass);
    WiFi.persistent(true);              // ensure that new provisioned credentials are written to flash
    WiFi.begin(provisionInfo.ssid, provisionInfo.pass);
    delay(10);
    setupNormal();          // we will wait for WiFi connect later in initWiFi()
    state = STATE_NORMAL;
}

void normalLoop()
{
    if (ntpClient.update()) {
        lastTimeUpdate = (time_t) ntpClient.getEpochTime();
        if (bootEpochTime == 0) {
            bootEpochTime = lastTimeUpdate;
            Serial.printf_P(PSTR("Boot time %s\n"), ctime(&bootEpochTime));
            initLogger();
        }
    }

    WebServer::webServer.handleClient();
    int temp = temperature.readTemperature();
    if (temp >= 0 && logger) {
        // Note: time is recorded in 10s units to reduce storage requirements
        logger->AddEntry((ntpClient.getEpochTime() - bootEpochTime) / 10, temp);
    }

    delay(delayPeriod);
}

void loop()
{
    switch (state) {
    case STATE_SOFTAP:
        softApLoop();
        break;
    case STATE_PROVISION_WIFI:
        provisionWiFiLoopOnce();
        break;
    case STATE_NORMAL:
        normalLoop();
        break;
    }
}


