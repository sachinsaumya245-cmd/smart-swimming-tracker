#include "heart_sensor.h"

#include <Arduino.h>
#include <Wire.h>
#include <MAX30105.h>
#include "spo2_algorithm.h"

static MAX30105 max30102;

bool HeartSensor::begin() {
    if (!max30102.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("[MAX30102] Sensor not found.");
        return false;
    }

    // powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange
    // ledMode 2 = Red + IR, suitable for MAX30102.
    max30102.setup(60, 4, 2, 100, 411, 4096);
    max30102.setPulseAmplitudeRed(0x3F);
    max30102.setPulseAmplitudeIR(0x3F);
    max30102.setPulseAmplitudeGreen(0);

    Serial.println("[MAX30102] Ready.");
    return true;
}

void HeartSensor::update() {
    max30102.check();

    while (max30102.available()) {
        uint32_t red = max30102.getRed();
        uint32_t ir  = max30102.getIR();
        max30102.nextSample();

        fingerPresent_ = (ir >= FINGER_IR_THRESHOLD);

        if (!fingerPresent_) {
            bpmValid_ = false;
            spo2Valid_ = false;
            bpm_ = 0;
            spo2_ = 0;
            fill_ = 0;
            continue;
        }

        irBuffer_[fill_] = ir;
        redBuffer_[fill_] = red;
        ++fill_;

        if (fill_ >= BUFFER_SIZE) {
            int32_t calcSpO2 = 0;
            int8_t  validSpO2 = 0;
            int32_t calcHR = 0;
            int8_t  validHR = 0;

            maxim_heart_rate_and_oxygen_saturation(
                irBuffer_, BUFFER_SIZE, redBuffer_,
                &calcSpO2, &validSpO2, &calcHR, &validHR
            );

            bpmValid_ = validHR && calcHR > 30 && calcHR < 220;
            spo2Valid_ = validSpO2 && calcSpO2 >= 70 && calcSpO2 <= 100;

            if (bpmValid_) bpm_ = (int)calcHR;
            if (spo2Valid_) spo2_ = (int)calcSpO2;

            // Keep 75 samples and collect 25 new samples before recalculating.
            const int keep = BUFFER_SIZE - RECALC_STEP;
            for (int i = 0; i < keep; ++i) {
                irBuffer_[i] = irBuffer_[i + RECALC_STEP];
                redBuffer_[i] = redBuffer_[i + RECALC_STEP];
            }
            fill_ = keep;
        }
    }
}
