#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include "temp-monitor.h"

//
// WiFi/HTTP Server Definitions
//

#define WIFI_AUTOCONNECT_TIMEOUT_MS             (60 * 1000)

WiFiEventHandler wifiHandlerDisconnected;
WiFiEventHandler wifiHandlerConnected;

uint32_t wifiDisconnectCount = 0;
uint32_t wifiConnectCount    = 0;
time_t   lastWifiDisconnect = 0;
time_t   lastWifiConnect = 0;

#if 0
extern const char *getSSID();
extern const char *getPass();
#endif

extern const char *softSSID;
extern const char *softPASS;

void onWiFiDisconnected(const WiFiEventStationModeDisconnected& event)
{
    Serial.printf_P(PSTR("WiFi disconnected, reason = %d\n"), (int)event.reason);
    wifiDisconnectCount++;
    lastWifiDisconnect = ntpClient.getEpochTime();
}

void onWiFiConnected(const WiFiEventStationModeConnected& /*event*/)
{
    Serial.printf_P(PSTR("WiFi Connected\n"));
    wifiConnectCount++;
    lastWifiConnect = ntpClient.getEpochTime();
}

#ifdef DO_SCAN
bool findSSID(const char *ssid)
{
    bool found = false;
    int n = WiFi.scanNetworks(false, true, 0, nullptr);
    for (int i = 0 ; i < n ; ++i) {
        String ssidStr;
        uint8_t enc;
        int32_t rssi;
        uint8_t *bssid;
        int32_t channel;
        bool hidden;
        WiFi.getNetworkInfo(i, ssidStr, enc, rssi, bssid, channel, hidden);
        if (!strcmp(ssidStr.c_str(), ssid)) {
            Serial.printf_P(PSTR("%s, RSSI %ddbm\n"), ssidStr.c_str(), rssi);
            return true;
        }
    }

    Serial.printf_P(PSTR("Target %s not found\n"), ssid);
    return false;
}
#endif

//
// Soft Access Point mode
//

void initWiFiAP()
{
    Serial.printf_P(PSTR("Starting soft access point\n"));
    WiFi.persistent(false);
    if (!WiFi.softAP(softSSID, softPASS)) {
        Serial.printf_P(PSTR("Failed to start soft access point\n"));
    } else {
        Serial.printf_P(PSTR("Started, IP = %s\n"), WiFi.softAPIP().toString().c_str());
        WiFi.setOutputPower(0);
    }
}

//
// Station mode
//

void initWiFi()
{
    WiFi.persistent(false);
    WiFi.setAutoConnect(true);
    WiFi.setAutoReconnect(true);

    wl_status_t status = WL_IDLE_STATUS;
    for (int i = 0 ; i < 3 ; ++i) {
        Serial.printf_P(PSTR("Auto connecting to WiFi, status %d (%d secs timeout, attempt %d)\n"), WiFi.status(), WIFI_AUTOCONNECT_TIMEOUT_MS / 1000, i + 1);
        status = (wl_status_t) WiFi.waitForConnectResult(WIFI_AUTOCONNECT_TIMEOUT_MS);
        if (status != WL_CONNECTED) {
            Serial.printf_P(PSTR("Could not autoconnect, status %d / %d\n"), (int)status, WiFi.status());
        } else {
            break;
        }
    }

#if 0
    if (status != WL_CONNECTED) {
        Serial.printf_P(PSTR("Connecting to WiFi\n"));
        WiFi.begin(getSSID(), getPass());
        
        while ((status = WiFi.status()) != WL_CONNECTED) {
            Serial.printf_P(PSTR("."));
            delay(1000);
        }
    }
#endif
    if (status == WL_CONNECTED) {
        Serial.printf_P(PSTR("\nConnected, IP = %s\n"), WiFi.localIP().toString().c_str());
        wifiHandlerDisconnected = WiFi.onStationModeDisconnected(onWiFiDisconnected);
        wifiHandlerConnected = WiFi.onStationModeConnected(onWiFiConnected);
    }
}

