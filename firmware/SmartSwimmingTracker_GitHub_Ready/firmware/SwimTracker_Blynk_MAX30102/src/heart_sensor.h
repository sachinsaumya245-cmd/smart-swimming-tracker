#pragma once

#include <cstdint>

class HeartSensor {
public:
    bool begin();
    void update();

    int  bpm() const { return bpm_; }
    int  spo2() const { return spo2_; }
    bool bpmValid() const { return bpmValid_; }
    bool spo2Valid() const { return spo2Valid_; }
    bool fingerPresent() const { return fingerPresent_; }

private:
    static constexpr int BUFFER_SIZE = 100;
    static constexpr int RECALC_STEP = 25;
    static constexpr uint32_t FINGER_IR_THRESHOLD = 50000;

    uint32_t irBuffer_[BUFFER_SIZE] = {};
    uint32_t redBuffer_[BUFFER_SIZE] = {};
    int fill_ = 0;

    int bpm_ = 0;
    int spo2_ = 0;
    bool bpmValid_ = false;
    bool spo2Valid_ = false;
    bool fingerPresent_ = false;
};
