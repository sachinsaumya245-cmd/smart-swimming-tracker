#include "led.h"
#include "config.h"
#include <Arduino.h>

namespace LED {

static void blinkTask(void* pv) {
    int n = (int)(intptr_t)pv;
    for (int i = 0; i < n; ++i) {
        digitalWrite(Config::LED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(Config::LED_PIN, LOW);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

void begin() {
    pinMode(Config::LED_PIN, OUTPUT);
    digitalWrite(Config::LED_PIN, LOW);
}

void blink(int count) {
    xTaskCreate(blinkTask, "led", 1024, (void*)(intptr_t)count, 1, NULL);
}

void set(bool on) {
    digitalWrite(Config::LED_PIN, on ? HIGH : LOW);
}

} // namespace LED
