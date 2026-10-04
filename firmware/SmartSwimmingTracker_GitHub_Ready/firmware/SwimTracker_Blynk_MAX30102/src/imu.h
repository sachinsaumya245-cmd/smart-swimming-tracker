#pragma once
#include "types.h"

class IMU {
public:
    bool begin();                       // init I2C + verify WHO_AM_I
    bool readAll(SensorData& out);      // single 14-byte burst (feature 4)
    bool checkHealth();                 // periodic frozen-sensor test (feature 16)

private:
    bool verify();
    bool configure();
    void resetBus();

    float    lastAccel_  = 0;
    float    lastGyro_   = 0;
    uint8_t  frozenA_    = 0;
    uint8_t  frozenG_    = 0;
    uint32_t lastHealth_ = 0;
    uint8_t  retries_    = 0;
};
