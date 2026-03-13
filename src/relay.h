#pragma once

#include <Arduino.h>

class RelayControl {
public:
    void begin();
    void setState(bool on);
    void toggle();
    bool isOn() const;
    void update();  // Call in loop() to flush dirty NVS writes

private:
    bool _state = false;
    uint8_t _pin;
    bool _nvsDirty = false;
    unsigned long _lastStateChange = 0;
};

extern RelayControl relayControl;
