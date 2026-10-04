#pragma once
#include "config.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════════
//  O(1) Moving-Average Filter  (feature 5)
//  Maintains a running sum so update() is constant time
//  instead of re-summing the whole window every sample.
// ═══════════════════════════════════════════════════════════════
class MovingAverageFilter {
public:
    void reset() {
        memset(buf_, 0, sizeof(buf_));
        sum_ = 0.0f;  idx_ = 0;  count_ = 0;
    }

    float update(float v) {
        if (count_ == Config::FILTER_WINDOW) sum_ -= buf_[idx_];
        buf_[idx_] = v;
        sum_ += v;
        idx_ = (idx_ + 1) % Config::FILTER_WINDOW;
        if (count_ < Config::FILTER_WINDOW) ++count_;
        return sum_ / count_;
    }

private:
    float    buf_[Config::FILTER_WINDOW] = {};
    float    sum_   = 0.0f;
    uint8_t  idx_   = 0;
    uint8_t  count_ = 0;
};

// ═══════════════════════════════════════════════════════════════
//  Exponential Moving Average  (feature 18)
//  filtered = α·new + (1-α)·old
//  Only one stored value, smoother transient response.
// ═══════════════════════════════════════════════════════════════
class EMAFilter {
public:
    explicit EMAFilter(float alpha = Config::EMA_ALPHA) : a_(alpha) {}

    void reset() { val_ = 0; init_ = false; }

    float update(float v) {
        if (!init_) { val_ = v; init_ = true; }
        else        { val_ = a_ * v + (1.0f - a_) * val_; }
        return val_;
    }

    float value() const { return val_; }

private:
    float a_;
    float val_  = 0;
    bool  init_ = false;
};
