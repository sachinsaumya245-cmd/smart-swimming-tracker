#pragma once
#include <cstdint>
#include "driver/gpio.h"

// ═══════════════════════════════════════════════════════════════
//  All compile-time constants in one place.
//  constexpr gives type safety, debugger visibility, and lets
//  the compiler optimise just as aggressively as #define.
// ═══════════════════════════════════════════════════════════════

namespace Config {

// ── Pin assignments ──────────────────────────────────────────
constexpr uint8_t  SDA_PIN      = 21;
constexpr uint8_t  SCL_PIN      = 22;
constexpr uint8_t  LED_PIN      = 2;
constexpr gpio_num_t WAKEUP_PIN = GPIO_NUM_0;
constexpr uint8_t  BATTERY_PIN  = 34;           // ADC1_CH6

// ── MPU-6050 ─────────────────────────────────────────────────
constexpr uint8_t  MPU_ADDR           = 0x68;
constexpr uint8_t  MPU_WHO_AM_I_VAL   = 0x68;
constexpr float    ACCEL_SENSITIVITY  = 8192.0f; // ±4 g
constexpr float    GYRO_SENSITIVITY   = 65.5f;   // ±500 dps

// ── Sampling ─────────────────────────────────────────────────
constexpr uint16_t SAMPLE_RATE_MS     = 20;      // 50 Hz
constexpr uint8_t  FILTER_WINDOW      = 5;
constexpr float    EMA_ALPHA          = 0.3f;

// ── Stroke detection thresholds ──────────────────────────────
constexpr float    PEAK_THRESHOLD_RATIO = 0.45f;
constexpr float    PEAK_EXIT_RATIO      = 0.5f;
constexpr uint16_t MIN_STROKE_GAP_MS    = 350;
constexpr uint16_t PEAK_REFRACTORY_MS   = 200;
constexpr uint16_t CAL_PEAK_GAP_MS      = 800;
constexpr float    NOISE_GATE_SIGMA     = 3.0f;

// ── Turn detection ───────────────────────────────────────────
// These are deliberately strict: both accel AND gyro must
// exceed their threshold simultaneously (AND logic).
// A strong freestyle stroke peaks ~1.5-2.2g / ~120 dps.
// A wall push peaks ~3-5g / ~250+ dps.  Plenty of gap.
constexpr float    TURN_ACCEL_G         = 2.5f;   // g above gravity
constexpr float    TURN_GYRO_DPS        = 220.0f;  // degrees/sec
constexpr uint32_t MIN_LAP_TIME_MS      = 20000;   // 20s min between laps
constexpr uint8_t  MIN_STROKES_FOR_TURN = 8;       // need ≥8 strokes first

// ── Calibration ──────────────────────────────────────────────
constexpr uint8_t  CAL_STROKES          = 3;
constexpr uint16_t CAL_TIMEOUT_MS       = 15000;
constexpr float    CAL_CONSISTENCY      = 0.35f;
constexpr uint16_t CAL_STILL_MS         = 3000;
constexpr uint16_t CAL_MIN_SAMPLES      = 10;

// ── Duration constraints ─────────────────────────────────────
constexpr uint16_t DUR_MIN_FLOOR  = 100;
constexpr uint16_t DUR_MIN_CEIL   = 400;
constexpr uint16_t DUR_MAX_FLOOR  = 800;
constexpr uint16_t DUR_MAX_CEIL   = 2000;
constexpr float    DUR_LOW_RATIO  = 0.4f;
constexpr float    DUR_HIGH_RATIO = 2.5f;

// ── Rest & power ─────────────────────────────────────────────
constexpr uint32_t REST_THRESHOLD_MS  = 4000;
constexpr uint32_t DEEP_SLEEP_MS      = 900000;  // 15 min

// ── Battery ──────────────────────────────────────────────────
constexpr float    BATT_FULL_V       = 4.2f;
constexpr float    BATT_EMPTY_V      = 3.0f;
constexpr float    BATT_LOW_V        = 3.2f;
constexpr float    ADC_REF_V         = 3.3f;
constexpr float    VDIV_RATIO        = 2.0f;
constexpr uint32_t BATT_CHECK_MS     = 30000;

// ── Swim defaults ────────────────────────────────────────────
constexpr int      DEFAULT_SPL       = 36;
constexpr float    DEFAULT_POOL_M    = 50.0f;
constexpr float    CAL_PER_STROKE    = 0.15f;

// ── Stroke-rate window ───────────────────────────────────────
constexpr uint8_t  RATE_WINDOW       = 10;


// ── Sensor health ────────────────────────────────────────────
constexpr uint32_t HEALTH_CHECK_MS   = 5000;
constexpr uint8_t  I2C_MAX_RETRIES   = 3;
constexpr float    FROZEN_THRESH     = 0.001f;
constexpr uint8_t  FROZEN_LIMIT      = 50;

// ── Auto-SPL learning ────────────────────────────────────────
constexpr uint8_t  SPL_MIN_LAPS      = 3;
constexpr float    SPL_LEARN_RATE    = 0.3f;

// ── Style detection heuristics ───────────────────────────────
constexpr float    STYLE_BREAST_GYRO_CEIL  = 0.4f;
constexpr float    STYLE_FLY_ACCEL_FLOOR   = 1.8f;
constexpr float    STYLE_BACK_AZ_SIGN      = -0.3f;
constexpr uint8_t  STYLE_VOTE_WINDOW       = 8;

} // namespace Config
