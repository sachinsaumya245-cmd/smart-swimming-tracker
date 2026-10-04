#pragma once
#include <cstdint>

namespace LED {
    void begin();
    void blink(int count);   // non-blocking, fires a FreeRTOS task
    void set(bool on);
}
