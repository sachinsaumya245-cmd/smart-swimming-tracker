#include "imu.h"
#include "config.h"
#include <Wire.h>
#include <Arduino.h>
#include <cmath>

// ── Public ───────────────────────────────────────────────────

bool IMU::begin() {
    Wire.begin(Config::SDA_PIN, Config::SCL_PIN);
    Wire.setClock(400000);

    // Try up to MAX_RETRIES if WHO_AM_I fails (feature 11)
    for (uint8_t i = 0; i <= Config::I2C_MAX_RETRIES; ++i) {
        if (verify()) return configure();
        Serial.printf("[IMU] retry %d/%d\n", i + 1, Config::I2C_MAX_RETRIES);
        delay(100);
        resetBus();
    }
    return false;
}

// Feature 4 – read accel + temp + gyro in one 14-byte I²C burst
bool IMU::readAll(SensorData& d) {
    Wire.beginTransmission(Config::MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    if (Wire.requestFrom((uint16_t)Config::MPU_ADDR,
                         (uint8_t)14, (bool)true) != 14) return false;

    auto rd16 = [&]() -> int16_t {
        uint8_t hi = Wire.read(), lo = Wire.read();
        return (int16_t)((hi << 8) | lo);
    };

    d.ax   = rd16() / Config::ACCEL_SENSITIVITY;
    d.ay   = rd16() / Config::ACCEL_SENSITIVITY;
    d.az   = rd16() / Config::ACCEL_SENSITIVITY;
    d.temp = rd16() / 340.0f + 36.53f;
    d.gx   = rd16() / Config::GYRO_SENSITIVITY;
    d.gy   = rd16() / Config::GYRO_SENSITIVITY;
    d.gz   = rd16() / Config::GYRO_SENSITIVITY;

    d.accelMag = sqrtf(d.ax*d.ax + d.ay*d.ay + d.az*d.az);
    d.gyroMag  = sqrtf(d.gx*d.gx + d.gy*d.gy + d.gz*d.gz);
    return true;
}

// Feature 16 – detect frozen / stuck sensor
bool IMU::checkHealth() {
    uint32_t now = millis();
    if (now - lastHealth_ < Config::HEALTH_CHECK_MS) return true;
    lastHealth_ = now;

    SensorData d;
    if (!readAll(d)) {
        if (++retries_ >= Config::I2C_MAX_RETRIES) {
            resetBus(); configure(); retries_ = 0;
        }
        return false;
    }
    retries_ = 0;

    frozenA_ = (fabsf(d.accelMag - lastAccel_) < Config::FROZEN_THRESH)
               ? frozenA_ + 1 : 0;
    frozenG_ = (fabsf(d.gyroMag  - lastGyro_)  < Config::FROZEN_THRESH)
               ? frozenG_ + 1 : 0;
    lastAccel_ = d.accelMag;
    lastGyro_  = d.gyroMag;

    if (frozenA_ > Config::FROZEN_LIMIT || frozenG_ > Config::FROZEN_LIMIT) {
        Serial.println("[IMU] sensor frozen – resetting");
        resetBus(); configure();
        frozenA_ = frozenG_ = 0;
        return false;
    }
    return true;
}

// ── Private ──────────────────────────────────────────────────

bool IMU::verify() {
    Wire.beginTransmission(Config::MPU_ADDR);
    Wire.write(0x75);
    Wire.endTransmission(false);
    if (Wire.requestFrom((uint16_t)Config::MPU_ADDR, (uint8_t)1, (bool)true) != 1)
        return false;
    return Wire.read() == Config::MPU_WHO_AM_I_VAL;
}

bool IMU::configure() {
    Wire.beginTransmission(Config::MPU_ADDR);
    Wire.write(0x6B); Wire.write(0x00);              // wake
    if (Wire.endTransmission() != 0) return false;

    Wire.beginTransmission(Config::MPU_ADDR);
    Wire.write(0x1C); Wire.write(0x08);              // accel ±4 g
    Wire.endTransmission();

    Wire.beginTransmission(Config::MPU_ADDR);
    Wire.write(0x1B); Wire.write(0x08);              // gyro ±500 dps
    Wire.endTransmission();

    Wire.beginTransmission(Config::MPU_ADDR);
    Wire.write(0x1A); Wire.write(0x03);              // DLPF ~44 Hz
    Wire.endTransmission();
    return true;
}

void IMU::resetBus() {
    Wire.end();
    delay(10);
    Wire.begin(Config::SDA_PIN, Config::SCL_PIN);
    Wire.setClock(400000);
}
