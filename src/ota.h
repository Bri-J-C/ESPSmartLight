#pragma once

#include <Arduino.h>

class OtaManager {
public:
    void begin();
    void update();

private:
    bool _initialized = false;
    String _hostname;
};

extern OtaManager otaManager;
