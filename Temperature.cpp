#include "Temperature.h"

#define DEBUG

void Temperature::init()
{
    // initI2C() must have been called beforehand
    initBMP280();
}

void Temperature::initBMP280()
{
    if (!mBmp280.begin(BMP280_ADDRESS_ALT)) {
        Serial.printf_P(PSTR("Error initialising BMP280\n"));
        return;
    }

    mBmp280.setSampling(Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
                Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                Adafruit_BMP280::STANDBY_MS_1000); /* Standby time. */
}

int Temperature::read_BMP280(float* temp_p, float* pres_p, float* alt_p)
{
    mBmp280.takeForcedMeasurement();
    if (temp_p)
        *temp_p = mBmp280.readTemperature();
    if (pres_p)
        *pres_p = mBmp280.readPressure();
    if (alt_p)
        *alt_p = mBmp280.readAltitude(MEAN_SEA_LEVEL_PRESSURE);
    return 0;
}

int Temperature::readTemperature(bool force)
{
    const uint32_t now = millis() / 1000;
    if (!force && (now - mLastUpdate) < mUpdateInterval) {
        return -1;
    }

    mLastUpdate = now;

    float temp;
    read_BMP280(&temp, nullptr, nullptr);
    mLastTemperature = ((temp * 100) + 5) / 10;
#ifdef DEBUG
    Serial.printf_P(PSTR("Temperature %d\n"), (int)mLastTemperature);
#endif

    return mLastTemperature;
}

