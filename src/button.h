#pragma once

#include <Arduino.h>

class ButtonHandler {
public:
    void begin();
    void update();

private:
    uint8_t _pin;
    bool _lastReading = HIGH;
    bool _buttonState = HIGH;
    bool _pressed = false;
    unsigned long _lastDebounceTime = 0;
    unsigned long _pressStartTime = 0;
    bool _actionTaken = false;
    unsigned long _restartScheduled = 0;
    bool _restartPending = false;
    bool _factoryResetPending = false;
};

extern ButtonHandler buttonHandler;
