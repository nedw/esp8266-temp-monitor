#include "Arduino.h"
#include <ESP8266WiFi.h>
#include "WebServer.h"
#include "Temperature.h"
#include "Logger.h"
#include "temp-monitor.h"
#include "WiFi.h"

//#define DEBUG

extern Temperature  temperature;
extern Logger       *logger;

ProvisionInfo provisionInfo;

namespace WebServer {

ESP8266WebServer webServer(kServerPort);

int formatLog(String& reply, const char *arg)
{
    if (!logger) {
        reply = F("<html>Log inactive</html>");
        return -1;
    } else
    if (logger->size() == 0) {
        reply = F("<html>Log empty</html>");
        return -1;
    }

    int16_t startIndex;
    int16_t count;

    int n = sscanf(arg, "%hd,%hd", &startIndex, &count);

    if (n < 1)
        startIndex = 0;

    if (n < 2)
        count = logger->size() > kDefaultLogEntryCount ? logger->size() : kDefaultLogEntryCount;
    else
    if (count > logger->size())
        count = logger->size();

    if (startIndex < 0) {
        startIndex += logger->size();
        if (startIndex < 0)
            startIndex = 0;
    } else 
    if (startIndex >= logger->size()) {
        reply = F("<html>Start index too large</html>");
        return -1;
    }

    const int16_t endIndex = startIndex + count;

#ifdef DEBUG
    Serial.printf_P("formatLog(): startIndex %d, endIndex %d\n", (int)startIndex, (int)endIndex);
#endif

    reply.reserve(512);
    reply = F("<html>");
    for (int i = startIndex ; i < endIndex ; ++i) {
        const LogEntry* p = logger->getEntry(i);
        if (p) {
            const int temp = p->v();
            // Note: time is recorded in 10s units to reduce storage requirements
            const time_t time = bootEpochTime + (p->t() * 10);
            char line[128];
            snprintf_P(line, sizeof(line), PSTR("%d: %.24s: %d.%d<br>"), i, ctime(&time), temp / 10, temp % 10);
            reply += line;
        }
    }
    reply += F("</html>");
    return 0;
}

void logHandler()
{
    Serial.printf_P(PSTR("Log Handler: uri %s\n"), webServer.uri().c_str());

    String reply;
    formatLog(reply, webServer.arg("log").c_str());
    webServer.send(200, "text/html", reply);
}

void handler()
{
    if (webServer.hasArg("force"))
        temperature.readTemperature(true);

    if (webServer.hasArg("log")) {
        logHandler();
        return;
    }
    Serial.printf_P(PSTR("Handler: uri %s\n"), webServer.uri().c_str());
    const time_t t = bootEpochTime + temperature.getLastUpdate();

    char reply[128];
    snprintf_P(reply, sizeof(reply), PSTR("<html>%.24s: %d.%d</html>"), ctime(&t), temperature.getLastTemperature() / 10, temperature.getLastTemperature() % 10);
    webServer.send(200, "text/html", reply);
}

void notFound()
{
    Serial.printf_P(PSTR("Not found: uri %s\n"), webServer.uri().c_str());
    if (webServer.uri().equals("/favicon.ico")) {
        webServer.send_P(404, PSTR("text/html"), PSTR(""));
    }
}

void info()
{
    Serial.printf_P(PSTR("Info: uri %s\n"), webServer.uri().c_str());

    char reply[512];
    const IPAddress addr = WiFi.localIP();
    snprintf_P(reply, sizeof(reply), PSTR("<html>Time = %s<br>Boot time = %.24s<br>Last time update = %.24s<br>"
                                          "IP address = %d.%d.%d.%d<br>Log entries = %d<br>"
                                          "Temp update interval = %d<br>"
                                          "WiFi disconnect count = %d, last disconnect = %.24s<br>"
                                          "WiFi connect count = %d, last connect = %.24s<br>"
                                          "RSSI %ddbm<br>Free heap = %u<br></html>"),
        ntpClient.getFormattedTime().c_str(), ctime(&bootEpochTime), ctime(&lastTimeUpdate),
        addr[0], addr[1], addr[2], addr[3], logger->size(),
        temperature.getUpdateInterval(),
        wifiDisconnectCount, lastWifiDisconnect ? ctime(&lastWifiDisconnect) : "never",
        wifiConnectCount, lastWifiConnect ? ctime(&lastWifiConnect) : "never",
        WiFi.RSSI(), ESP.getFreeHeap());
    webServer.send(200, "text/html", reply);
}

//
// Set configuration parameters
//

void config()
{
    Serial.printf_P(PSTR("Config: uri %s\n"), webServer.uri().c_str());

    String reply(F("<html>"));
    bool found = false;

    if (webServer.hasArg(F("update"))) {
        found = true;
        const int32_t updateInterval = atoi(webServer.arg(F("update")).c_str());
        if (updateInterval < 1 || updateInterval > kOneWeekSecs) {
            reply += F("Update argument out of range<br>");
        } else {
            temperature.setUpdateInterval(updateInterval);
            char buf[128];
            snprintf_P(buf, sizeof(buf), PSTR("Update interval set to %d<br>"), temperature.getUpdateInterval());
            reply += buf;
        }
    }
    if (webServer.hasArg(F("delay"))) {
        found = true;
        const int32_t n = atoi(webServer.arg(F("delay")).c_str());
        if (n < 0 || n > 1000)  {
            reply += F("Delay argument out of range<br>");
        } else {
            char buf[128];
            snprintf_P(buf, sizeof(buf), PSTR("Delay period set to %d<br>"), n);
            delayPeriod = n;
            reply += buf;
        }
    }

    if (!found)
        reply += F("No argument specified<br>");
    reply += "</html>";
    webServer.send_P(200, PSTR("text/html"), reply.c_str());
}

void init()
{
    webServer.begin();
    webServer.on(Uri(F("/sensor/temp")), HTTPMethod::HTTP_GET, handler);
    webServer.on(Uri(F("/sensor/info")), HTTPMethod::HTTP_GET, info);
    webServer.on(Uri(F("/sensor/config")), HTTPMethod::HTTP_GET, config);
    webServer.onNotFound(notFound);
}

//
// Soft AP mode provisioning routines
//

void provisionWiFi(const char *ssid, const char *psk)
{
    webServer.close();
    strncpy(provisionInfo.ssid, ssid, sizeof(provisionInfo.ssid));
    strncpy(provisionInfo.pass, psk,  sizeof(provisionInfo.pass));
    state = STATE_PROVISION_WIFI;
}

void softHandler()
{
    Serial.printf_P(PSTR("softHandler: uri %s\n"), webServer.uri().c_str());

    String reply(F("<html>"));
    bool found = false;

    const bool hasId = webServer.hasArg(F("id"));
    const bool hasPsk = webServer.hasArg(F("psk"));
    if (hasId ^ hasPsk) {
        webServer.send_P(200, PSTR("text/html"), PSTR("<html>id and psk arguments must be supplied together</html>"));
        return;
    } else
    if (hasId) {
        found = true;
        const String& id(webServer.arg(F("id")));
        const String& psk(webServer.arg(F("psk")));
        char buf[128];
        snprintf_P(buf, sizeof(buf), PSTR("id='%s', psk='%s'<br>\n"), id.c_str(), psk.c_str());
        reply += buf;
        provisionWiFi(id.c_str(), psk.c_str());
    }

    if (!found) {
        webServer.send_P(200, PSTR("text/html"), PSTR("<html>Invalid argument</html>"));
    } else {
        reply += F("</html>");
        webServer.send_P(200, PSTR("text/html"), reply.c_str());
    }
}

void initAP()
{
    webServer.begin();
    webServer.on(Uri(F("/sensor/setup")), HTTPMethod::HTTP_GET, softHandler);
}

} // namespace WebServer
