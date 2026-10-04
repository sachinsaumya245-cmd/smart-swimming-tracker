#define BLYNK_TEMPLATE_ID "TMPL6Q8RWEJLS"
#define BLYNK_TEMPLATE_NAME "Swimmer Tracker"

#include "secrets.h"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

#include "config.h"
#include "types.h"
#include "imu.h"
#include "led.h"
#include "storage.h"
#include "battery.h"
#include "tracker.h"
#include "calibration.h"
#include "heart_sensor.h"

#include <cmath>

// ─────────────────────────────────────────────────────────────
// Core project objects
// ─────────────────────────────────────────────────────────────
static IMU         imu;
static Storage     store;
static Battery     batt;
static SwimTracker tracker;
static HeartSensor heart;

static TrackerState appState = TrackerState::IDLE;
static TaskHandle_t imuTaskHandle = nullptr;

static BlynkTimer blynkTimer;
static unsigned long lastWiFiAttemptMs = 0;
static unsigned long lastBlynkAttemptMs = 0;

// 0–100 dashboard score calculated from MPU6050 activity.
static volatile float movementIntensity = 0.0f;

// ─────────────────────────────────────────────────────────────
// Session timer + physiological summary accumulators
// ─────────────────────────────────────────────────────────────
static unsigned long sessionStartMs = 0;
static unsigned long stoppedSessionSeconds = 0;

static uint32_t hrSum = 0;
static uint32_t hrSamples = 0;
static int maxHeartRate = 0;

static uint32_t spo2Sum = 0;
static uint32_t spo2Samples = 0;


// ─────────────────────────────────────────────────────────────
// Blynk-visible device/session status (V15)
// ─────────────────────────────────────────────────────────────
static void setStatus(const char* text) {
    Serial.printf("[STATUS] %s\n", text);
    if (Blynk.connected()) {
        Blynk.virtualWrite(V15, text);
    }
}

// ─────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────
static unsigned long currentSessionSeconds() {
    if (appState == TrackerState::TRACKING && sessionStartMs != 0) {
        return (millis() - sessionStartMs) / 1000UL;
    }
    return stoppedSessionSeconds;
}

static void formatSessionTime(unsigned long totalSeconds,
                              char* out,
                              size_t outSize) {
    unsigned long hours = totalSeconds / 3600UL;
    unsigned long minutes = (totalSeconds % 3600UL) / 60UL;
    unsigned long seconds = totalSeconds % 60UL;

    if (hours > 0) {
        snprintf(out, outSize, "%02lu:%02lu:%02lu",
                 hours, minutes, seconds);
    } else {
        snprintf(out, outSize, "%02lu:%02lu",
                 minutes, seconds);
    }
}

static void resetSessionAccumulators() {
    sessionStartMs = 0;
    stoppedSessionSeconds = 0;

    hrSum = 0;
    hrSamples = 0;
    maxHeartRate = 0;

    spo2Sum = 0;
    spo2Samples = 0;
}

static void samplePhysiologyForSummary() {
    if (appState != TrackerState::TRACKING) return;
    if (!heart.fingerPresent()) return;

    if (heart.bpmValid()) {
        const int bpm = heart.bpm();
        hrSum += bpm;
        hrSamples++;
        if (bpm > maxHeartRate) {
            maxHeartRate = bpm;
        }
    }

    if (heart.spo2Valid()) {
        spo2Sum += heart.spo2();
        spo2Samples++;
    }
}

// ─────────────────────────────────────────────────────────────
// Wi-Fi / Blynk
// ─────────────────────────────────────────────────────────────
static void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    lastWiFiAttemptMs = millis();
}

static void serviceConnectivity() {
    const unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastWiFiAttemptMs >= 10000) {
            connectWiFi();
        }
        return;
    }

    if (!Blynk.connected() && now - lastBlynkAttemptMs >= 5000) {
        lastBlynkAttemptMs = now;
        Serial.println("[Blynk] Connecting...");

        if (Blynk.connect(1500)) {
            Serial.println("[Blynk] Connected.");
            if (appState == TrackerState::TRACKING) {
                Blynk.virtualWrite(V15, "TRACKING");
            } else {
                Blynk.virtualWrite(V15, "READY");
            }
        }
    }

    if (Blynk.connected()) {
        Blynk.run();
    }
}

// Live dashboard values V0–V5.
static void sendLiveDataToBlynk() {
    if (!Blynk.connected()) return;

    const auto& s = tracker.stats();
    const auto& c = tracker.cadence();

    if (heart.fingerPresent()) {
        if (heart.bpmValid()) {
            Blynk.virtualWrite(V0, heart.bpm());
        }
        if (heart.spo2Valid()) {
            Blynk.virtualWrite(V1, heart.spo2());
        }
    }

    Blynk.virtualWrite(V2, s.strokes);
    Blynk.virtualWrite(V3, c.rate);
    Blynk.virtualWrite(V4, s.laps);          // Lap Count
    Blynk.virtualWrite(V5, batt.percent());
    Blynk.virtualWrite(V16, s.speedMs);       // Current speed (m/s)

    // Collect one physiological sample per live-upload cycle.
    samplePhysiologyForSummary();
}

// V14 live session timer.
static void sendTimerToBlynk() {
    if (!Blynk.connected()) return;

    char timerText[16];
    formatSessionTime(currentSessionSeconds(), timerText, sizeof(timerText));
    Blynk.virtualWrite(V14, timerText);
}

// Final summary V8–V13. Called when the session stops.
static void sendSessionSummaryToBlynk() {
    if (!Blynk.connected()) {
        Serial.println("[SUMMARY] Blynk offline; summary not uploaded.");
        return;
    }

    const auto& s = tracker.stats();

    const float avgHeartRate =
        (hrSamples > 0) ? (float)hrSum / (float)hrSamples : 0.0f;

    const float avgSpO2 =
        (spo2Samples > 0) ? (float)spo2Sum / (float)spo2Samples : 0.0f;

    const float averageSpeed =
        (stoppedSessionSeconds > 0)
            ? (s.distance / (float)stoppedSessionSeconds)
            : 0.0f;

    Blynk.virtualWrite(V8, stoppedSessionSeconds);
    Blynk.virtualWrite(V9, avgHeartRate);
    Blynk.virtualWrite(V10, maxHeartRate);
    Blynk.virtualWrite(V11, avgSpO2);
    Blynk.virtualWrite(V12, s.distance);
    Blynk.virtualWrite(V13, s.avgRate);
    Blynk.virtualWrite(V17, averageSpeed);   // Average speed (m/s)

    Serial.println("[SUMMARY] Uploaded to Blynk.");
    Serial.printf("  Duration: %lu s\n", stoppedSessionSeconds);
    Serial.printf("  Avg HR: %.1f BPM\n", avgHeartRate);
    Serial.printf("  Max HR: %d BPM\n", maxHeartRate);
    Serial.printf("  Avg SpO2: %.1f %%\n", avgSpO2);
    Serial.printf("  Distance: %.1f m\n", s.distance);
    Serial.printf("  Avg Stroke Rate: %.1f /min\n", s.avgRate);
    Serial.printf("  Avg Speed: %.2f m/s\n", averageSpeed);
}

// ─────────────────────────────────────────────────────────────
// Session control
// ─────────────────────────────────────────────────────────────
static void startTracking() {
    if (appState == TrackerState::TRACKING) return;

    setStatus("CALIBRATING");
    LED::blink(2);

    if (imuTaskHandle) {
        vTaskSuspend(imuTaskHandle);
    }

    const bool ok = runCalibration(imu, tracker, store);

    if (ok) {
        tracker.reset();
        resetSessionAccumulators();

        const unsigned long now = millis();
        tracker.startSession(now);
        sessionStartMs = now;
        appState = TrackerState::TRACKING;

        if (Blynk.connected()) {
            Blynk.virtualWrite(V14, "00:00");
        }

        setStatus("TRACKING");
        LED::blink(3);
    } else {
        appState = TrackerState::IDLE;

        if (Blynk.connected()) {
            Blynk.virtualWrite(V6, 0);
        }

        setStatus("CALIBRATION FAILED");
        LED::blink(6);
    }

    if (imuTaskHandle) {
        vTaskResume(imuTaskHandle);
    }
}

static void stopTracking() {
    if (appState != TrackerState::TRACKING) {
        setStatus("STOPPED");
        return;
    }

    stoppedSessionSeconds = currentSessionSeconds();

    appState = TrackerState::IDLE;
    movementIntensity = 0.0f;

    tracker.computeFinals();
    sendTimerToBlynk();
    sendSessionSummaryToBlynk();

    setStatus("STOPPED");
    LED::blink(3);

    Serial.println("[SESSION] Tracking stopped.");
}

static void resetTracking() {
    tracker.reset();
    resetSessionAccumulators();
    movementIntensity = 0.0f;
    appState = TrackerState::IDLE;

    if (Blynk.connected()) {
        Blynk.virtualWrite(V2, 0);
        Blynk.virtualWrite(V3, 0);
        Blynk.virtualWrite(V4, 0);   // Lap Count reset
        Blynk.virtualWrite(V14, "00:00");
        Blynk.virtualWrite(V6, 0);
    }

    setStatus("READY");
    LED::blink(2);

    Serial.println("[SESSION] Session reset. Previous summary kept.");
}

// V6 = Start / Stop switch
BLYNK_WRITE(V6) {
    if (param.asInt()) {
        startTracking();
    } else {
        stopTracking();
    }
}

// V7 = momentary Reset button
BLYNK_WRITE(V7) {
    if (param.asInt() == 1) {
        resetTracking();
        Blynk.virtualWrite(V7, 0);
    }
}

// ─────────────────────────────────────────────────────────────
// MPU6050 task - 50 Hz on Core 1
// ─────────────────────────────────────────────────────────────
static void imuTask(void* /*pv*/) {
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(Config::SAMPLE_RATE_MS));

        if (appState != TrackerState::TRACKING) {
            movementIntensity = 0.0f;
            continue;
        }

        SensorData d;
        if (!imu.readAll(d)) continue;

        const float dynamicAccel = fabsf(d.accelMag - 1.0f);
        const float accelPart =
            constrain(dynamicAccel / 1.5f * 60.0f, 0.0f, 60.0f);
        const float gyroPart =
            constrain(d.gyroMag / 300.0f * 40.0f, 0.0f, 40.0f);

        movementIntensity = accelPart + gyroPart;

        const unsigned long now = millis();
        tracker.update(d, now);

        if (tracker.restDurationMs(now) > Config::DEEP_SLEEP_MS) {
            Serial.println("[POWER] Long rest detected. Entering deep sleep.");
            delay(100);

            esp_sleep_enable_ext0_wakeup(Config::WAKEUP_PIN, 0);
            esp_deep_sleep_start();
        }

        imu.checkHealth();
    }
}

// ─────────────────────────────────────────────────────────────
// setup / loop
// ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    LED::begin();
    delay(300);

    const auto reason = esp_sleep_get_wakeup_cause();
    if (reason == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("[BOOT] Woke from deep sleep.");
    }

    if (!imu.begin()) {
        Serial.println("[BOOT] MPU6050 FAILED - restarting in 5s");
        LED::blink(10);
        delay(5000);
        ESP.restart();
    }

    if (!heart.begin()) {
        Serial.println("[BOOT] MAX30102 not detected. MPU tracking will still run.");
    }

    store.begin();
    batt.begin();

    tracker.spl   = store.getSPL();
    tracker.poolM = store.getPool();
    tracker.cal() = store.loadCal();

    Serial.printf("[BOOT] Ready. Pool: %.0fm  SPL: %d\n",
                  tracker.poolM, tracker.spl);

    Blynk.config(BLYNK_AUTH_TOKEN);
    connectWiFi();

    // Live values every 2 seconds.
    blynkTimer.setInterval(2000L, sendLiveDataToBlynk);

    // Session timer every 1 second.
    blynkTimer.setInterval(1000L, sendTimerToBlynk);

    Serial.println("[STATUS] READY");

    xTaskCreatePinnedToCore(
        imuTask,
        "IMU",
        4096,
        nullptr,
        5,
        &imuTaskHandle,
        1
    );

    LED::blink(3);
}

void loop() {
    serviceConnectivity();

    // Keep the MAX30102 FIFO drained frequently.
    heart.update();

    blynkTimer.run();

    delay(10);
}
