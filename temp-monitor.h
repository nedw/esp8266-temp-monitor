#include <NTPClient.h>
#include <time.h>

enum State {
    STATE_SOFTAP         = 1,
    STATE_PROVISION_WIFI = 2,
    STATE_NORMAL         = 3
};

extern State state;

extern NTPClient    ntpClient;
extern time_t       bootEpochTime;
extern time_t       lastTimeUpdate;
extern uint16_t      delayPeriod;
