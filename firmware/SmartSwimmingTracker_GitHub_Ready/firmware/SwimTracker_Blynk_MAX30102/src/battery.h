#pragma once
#include <cstdint>

class Battery {
public:
    void begin();
    float voltage();
    int   percent();
    bool  isLow();
    bool  periodicCheck();   // returns true when battery is critically low

private:
    uint32_t lastMs_ = 0;
    float    lastV_  = 4.2f;
};
