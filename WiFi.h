
#include <time.h>
#include <stdint.h>

extern uint32_t     wifiDisconnectCount;
extern uint32_t     wifiConnectCount;
extern time_t       lastWifiDisconnect;
extern time_t       lastWifiConnect;

extern void initWiFi();
extern void initWiFiAP();
