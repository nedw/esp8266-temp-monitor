#include <ESP8266WebServer.h>

struct ProvisionInfo {
    char ssid[32];
    char pass[32];
};

extern ProvisionInfo provisionInfo;

namespace WebServer {
constexpr int16_t kDefaultLogEntryCount = 20;
constexpr int32_t kOneWeekSecs          = 86400 * 7;
constexpr int     kServerPort           = 47163;

extern ESP8266WebServer webServer;
extern void init();
extern void initAP();
};

