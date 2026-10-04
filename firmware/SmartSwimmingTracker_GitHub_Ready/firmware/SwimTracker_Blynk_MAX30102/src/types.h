#pragma once
#include <cstdint>

// ═══════════════════════════════════════════════════════════════
//  Data structures shared across modules.
// ═══════════════════════════════════════════════════════════════

// ── Single IMU sample (feature 6) ────────────────────────────
struct SensorData {
    float ax, ay, az;   // accelerometer  (g)
    float gx, gy, gz;   // gyroscope      (dps)
    float temp;         // die temperature (°C)
    float accelMag;     // sqrt(ax²+ay²+az²)
    float gyroMag;      // sqrt(gx²+gy²+gz²)
};

// ── Tracker state machine ────────────────────────────────────
enum class TrackerState : uint8_t {
    IDLE,
    CALIBRATING_STILL,
    CALIBRATING_STROKE,
    TRACKING
};

// ── Stroke-peak state machine (feature 7) ────────────────────
enum class PeakState : uint8_t {
    WAITING,       // below noise gate, looking for next peak
    PEAK_RISING,   // inside a peak, tracking maximum
    CONFIRMED      // valid stroke accepted (transient)
};

// ── Swimming style (feature 13) ──────────────────────────────
enum class SwimStyle : uint8_t {
    UNKNOWN    = 0,
    FREESTYLE  = 1,
    BACKSTROKE = 2,
    BREASTSTROKE = 3,
    BUTTERFLY  = 4
};

inline const char* styleName(SwimStyle s) {
    switch (s) {
        case SwimStyle::FREESTYLE:    return "free";
        case SwimStyle::BACKSTROKE:   return "back";
        case SwimStyle::BREASTSTROKE: return "breast";
        case SwimStyle::BUTTERFLY:    return "fly";
        default:                      return "unknown";
    }
}

// ── Calibration results ──────────────────────────────────────
struct CalibrationData {
    float gravityBaseline    = 1.0f;
    float noiseFloor         = 0.02f;
    float noiseGate          = 1.05f;
    float gyroNoiseFloor     = 3.0f;
    float strokeThreshold    = 1.5f;
    float gyroThreshold      = 60.0f;
    unsigned long minDurMs   = 150;
    unsigned long maxDurMs   = 1200;
};

// ── Session statistics (feature 8) ───────────────────────────
struct SessionStats {
    int      strokes        = 0;
    int      laps           = 0;
    int      strokesThisLap = 0;
    int      rests          = 0;
    unsigned long activeMs  = 0;
    unsigned long startMs   = 0;

    // Computed at export / on demand
    float    distance       = 0.0f;   // metres
    float    calories       = 0.0f;
    float    avgRate        = 0.0f;   // strokes/min
    float    pace100m       = 0.0f;   // seconds per 100 m
    float    speedMs        = 0.0f;   // m/s
    SwimStyle style         = SwimStyle::UNKNOWN;
};

// ── Peak tracker state ───────────────────────────────────────
struct PeakData {
    PeakState     state     = PeakState::WAITING;
    float         accelMax  = 0.0f;
    float         gyroMax   = 0.0f;
    unsigned long startMs   = 0;
    unsigned long endMs     = 0;
};

// ── Cadence analytics (feature 19) ───────────────────────────
struct CadenceStats {
    float meanMs        = 0.0f;   // mean interval (ms)
    float stdDevMs      = 0.0f;   // standard deviation (ms)
    float consistency   = 0.0f;   // 0-100 %
    float rate          = 0.0f;   // strokes/min
};

// ── Per-peak style accumulator (internal) ────────────────────
struct StyleAccum {
    float sumAbsGx = 0, sumAbsGy = 0, sumAbsGz = 0;
    float sumAz    = 0;
    float peakAccel = 0;
    int   samples   = 0;

    void reset() { sumAbsGx = sumAbsGy = sumAbsGz = sumAz = peakAccel = 0; samples = 0; }
};
