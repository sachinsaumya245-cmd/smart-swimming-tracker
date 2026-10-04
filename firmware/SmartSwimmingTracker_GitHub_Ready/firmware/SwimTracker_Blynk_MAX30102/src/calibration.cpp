#include "calibration.h"
#include "imu.h"
#include "tracker.h"
#include "storage.h"
#include "led.h"
#include "filter.h"
#include "config.h"
#include <Arduino.h>
#include <cmath>

static float med3f(float a, float b, float c) {
    return fmaxf(fminf(a,b), fminf(fmaxf(a,b), c));
}

static unsigned long med3ul(unsigned long a, unsigned long b, unsigned long c) {
    return max(min(a,b), min(max(a,b), c));
}

static void pushStatus(CalibrationStatusCallback cb, const char* text) {
    if (cb) cb(text);
}

static void service(CalibrationServiceCallback cb) {
    if (cb) cb();
}

// Delay while still allowing Blynk/Wi-Fi servicing.
static void serviceDelay(unsigned long ms, CalibrationServiceCallback cb) {
    const unsigned long start = millis();
    while (millis() - start < ms) {
        service(cb);
        delay(10);
    }
}

bool runCalibration(IMU& imu,
                    SwimTracker& trk,
                    Storage& store,
                    CalibrationStatusCallback statusCb,
                    CalibrationServiceCallback serviceCb) {
    trk.reset();
    CalibrationData& cal = trk.cal();

    // ── Phase 1: Still baseline ─────────────────────────────
    Serial.print("[CAL 1/3] Hold STILL for 3 sec...\n");
    pushStatus(statusCb, "CAL 1/3 - HOLD STILL");
    LED::blink(2);
    serviceDelay(300, serviceCb);

    float sumM = 0, sumM2 = 0, sumG = 0, sumG2 = 0;
    int n = 0;

    const unsigned long end = millis() + Config::CAL_STILL_MS;
    int lastShownSecond = -1;

    while (millis() < end) {
        service(serviceCb);

        const unsigned long now = millis();
        const unsigned long remainMs = (end > now) ? (end - now) : 0;
        const int remainSec = (int)((remainMs + 999UL) / 1000UL);

        if (remainSec != lastShownSecond && remainSec > 0) {
            lastShownSecond = remainSec;

            char msg[24];
            snprintf(msg, sizeof(msg), "HOLD STILL: %d", remainSec);

            Serial.printf("[CAL 1/3] %s\n", msg);
            pushStatus(statusCb, msg);
        }

        SensorData d;
        if (imu.readAll(d)) {
            sumM  += d.accelMag;
            sumM2 += d.accelMag * d.accelMag;
            sumG  += d.gyroMag;
            sumG2 += d.gyroMag * d.gyroMag;
            ++n;
        }

        serviceDelay(Config::SAMPLE_RATE_MS, serviceCb);
    }

    if (n < Config::CAL_MIN_SAMPLES) {
        Serial.print("[CAL] FAILED: sensor error.\n");
        pushStatus(statusCb, "CAL FAILED - SENSOR");
        service(serviceCb);
        return false;
    }

    cal.gravityBaseline = sumM / n;
    cal.noiseFloor =
        sqrtf(fabsf(sumM2/n - cal.gravityBaseline*cal.gravityBaseline));
    cal.noiseGate =
        cal.gravityBaseline + cal.noiseFloor * Config::NOISE_GATE_SIGMA;

    const float gBase = sumG / n;
    cal.gyroNoiseFloor =
        sqrtf(fabsf(sumG2/n - gBase*gBase));

    // ── Phase 2: Capture 3 strokes ─────────────────────────
    Serial.print("[CAL 2/3] Do 3 NORMAL freestyle strokes.\n");
    pushStatus(statusCb, "CAL 2/3 - DO 3 STROKES");
    LED::blink(3);
    serviceDelay(500, serviceCb);

    constexpr int NEED = Config::CAL_STROKES;
    float peakA[NEED], peakG[NEED];
    unsigned long dur[NEED];
    int got = 0;

    MovingAverageFilter fA, fG;
    fA.reset();
    fG.reset();

    bool inPk = false;
    float curA = 0, curG = 0;
    unsigned long pkStart = 0;
    unsigned long lastEnd = millis();
    const unsigned long t0 = millis();

    while (got < NEED) {
        service(serviceCb);

        if (millis() - t0 > Config::CAL_TIMEOUT_MS) {
            Serial.print("[CAL] Timeout! Send START again.\n");
            pushStatus(statusCb, "CAL FAILED - TIMEOUT");
            service(serviceCb);
            return false;
        }

        SensorData d;
        if (!imu.readAll(d)) {
            serviceDelay(Config::SAMPLE_RATE_MS, serviceCb);
            continue;
        }

        const float sA = fA.update(d.accelMag);
        const float sG = fG.update(d.gyroMag);
        const float dev = sA - cal.gravityBaseline;
        const float thr = cal.noiseGate - cal.gravityBaseline;

        if (!inPk &&
            dev > thr &&
            (millis() - lastEnd) > Config::CAL_PEAK_GAP_MS) {
            inPk = true;
            curA = sA;
            curG = sG;
            pkStart = millis();
            LED::set(true);
        }
        else if (inPk) {
            if (sA > curA) curA = sA;
            if (sG > curG) curG = sG;

            if (dev < thr * Config::PEAK_EXIT_RATIO ||
                (millis() - pkStart) > 2000) {

                const float pd = curA - cal.gravityBaseline;

                if (pd > thr) {
                    peakA[got] = pd;
                    peakG[got] = curG;
                    dur[got] = millis() - pkStart;
                    ++got;

                    char msg[24];
                    snprintf(msg, sizeof(msg), "STROKES: %d/3", got);

                    Serial.printf("[CAL] Stroke %d/3 Logged.\n", got);
                    pushStatus(statusCb, msg);
                    service(serviceCb);
                    LED::blink(1);
                }

                inPk = false;
                lastEnd = millis();
                LED::set(false);
            }
        }

        serviceDelay(Config::SAMPLE_RATE_MS, serviceCb);
    }

    // ── Phase 3: Validate ──────────────────────────────────
    Serial.print("[CAL 3/3] Validating...\n");
    pushStatus(statusCb, "CAL 3/3 - VALIDATING");
    service(serviceCb);

    const float mA = med3f(peakA[0], peakA[1], peakA[2]);
    const float mG = med3f(peakG[0], peakG[1], peakG[2]);
    const unsigned long mD = med3ul(dur[0], dur[1], dur[2]);

    bool inconsistent = false;

    for (int i = 0; i < NEED; ++i) {
        if (fabsf(peakA[i] - mA) / mA > Config::CAL_CONSISTENCY) {
            inconsistent = true;
        }
    }

    if (inconsistent) {
        Serial.print("[CAL] WARNING: strokes inconsistent; using median values.\n");
        pushStatus(statusCb, "CAL WARNING - USING MEDIAN");
        service(serviceCb);
        delay(500);
    }

    cal.strokeThreshold =
        cal.gravityBaseline + mA * Config::PEAK_THRESHOLD_RATIO;

    cal.gyroThreshold =
        fmaxf(cal.gyroNoiseFloor * Config::NOISE_GATE_SIGMA,
              mG * Config::PEAK_THRESHOLD_RATIO);

    cal.minDurMs =
        constrain((long)(mD * Config::DUR_LOW_RATIO),
                  Config::DUR_MIN_FLOOR,
                  Config::DUR_MIN_CEIL);

    cal.maxDurMs =
        constrain((long)(mD * Config::DUR_HIGH_RATIO),
                  Config::DUR_MAX_FLOOR,
                  Config::DUR_MAX_CEIL);

    store.saveCal(cal);

    Serial.print("[CAL] SUCCESS! Tracking started. Swim!\n");
    pushStatus(statusCb, "CAL SUCCESS");
    service(serviceCb);

    return true;
}
