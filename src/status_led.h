#pragma once

#include <Arduino.h>
#include "config.h"

enum class LedPattern {
    OFF,
    ON,
    FAST_BLINK,      // 200ms - AP mode
    SLOW_BLINK,      // 1s - connecting
    DOUBLE_BLINK,    // MQTT disconnected
    SOS,             // Error
    TRIPLE_BLINK,    // WiFi clear in progress
    RAPID_FLASH      // Factory reset in progress
};

class StatusLed {
public:
    void begin();
    void update();
    void setPattern(LedPattern pattern);
    LedPattern getPattern() const;

private:
    void setLed(bool on);
    LedPattern _pattern = LedPattern::OFF;
    unsigned long _lastToggle = 0;
    uint8_t _step = 0;
    bool _ledState = false;
};

extern StatusLed statusLed;
