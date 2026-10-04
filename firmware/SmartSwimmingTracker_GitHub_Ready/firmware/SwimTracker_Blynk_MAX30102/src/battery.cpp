#include "battery.h"
#include "config.h"
#include <Arduino.h>

void Battery::begin() {
    analogSetAttenuation(ADC_11db);
    pinMode(Config::BATTERY_PIN, INPUT);
}

float Battery::voltage() {
    int raw = analogRead(Config::BATTERY_PIN);
    lastV_  = (raw / 4095.0f) * Config::ADC_REF_V * Config::VDIV_RATIO;
    return lastV_;
}

int Battery::percent() {
    float v = voltage();
    float pct = (v - Config::BATT_EMPTY_V) /
                (Config::BATT_FULL_V - Config::BATT_EMPTY_V) * 100.0f;
    return constrain((int)pct, 0, 100);
}

bool Battery::isLow() { return lastV_ < Config::BATT_LOW_V; }

bool Battery::periodicCheck() {
    uint32_t now = millis();
    if (now - lastMs_ < Config::BATT_CHECK_MS) return false;
    lastMs_ = now;
    return percent() < 10;          // critical
}
