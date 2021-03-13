#include "Adafruit_BMP280.h"

#define MEAN_SEA_LEVEL_PRESSURE 1013.25

class Temperature {
public:
    void init();
    int readTemperature(bool force);
    int getLastTemperature() const       { return mLastTemperature; }
    int getLastUpdate() const            { return mLastUpdate; }
    int getUpdateInterval() const        { return mUpdateInterval; }
    void setUpdateInterval(int32_t secs) { mUpdateInterval = secs; }

private:
    enum { UPDATE_INTERVAL_SECS = 900 };

    void initBMP280();
    int read_BMP280(float* temp_p, float* pres_p, float* alt_p);

    Adafruit_BMP280 mBmp280;
    int32_t  mUpdateInterval     = UPDATE_INTERVAL_SECS;
    uint32_t mLastUpdate         = 0;
    int16_t  mLastTemperature    = -1;
};