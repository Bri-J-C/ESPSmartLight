#include "relay.h"
#include "config_manager.h"
#include "logger.h"
#include "config.h"

static const char* TAG = "RELAY";
static const unsigned long NVS_WRITE_DEBOUNCE_MS = 60000;

RelayControl relayControl;

void RelayControl::begin() {
    _pin = configManager.getRelayPin();
    digitalWrite(_pin, HIGH);  // De-energize relay before setting as output
    pinMode(_pin, OUTPUT);

    uint8_t bootState = configManager.getBootState();
    logger.info(TAG, "Pin: %d, Boot state: %d", _pin, bootState);

    switch (bootState) {
        case BOOT_STATE_ON:
            setState(true);
            break;
        case BOOT_STATE_LAST:
            setState(configManager.getRelayLast());
            break;
        case BOOT_STATE_OFF:
        default:
            setState(false);
            break;
    }
    _nvsDirty = false;  // Don't count initial state as dirty
}

void RelayControl::setState(bool on) {
    if (_state != on) {
        _state = on;
        digitalWrite(_pin, on ? LOW : HIGH);  // Active LOW relay module
        _nvsDirty = true;
        _lastStateChange = millis();
        logger.info(TAG, "State: %s (pin %d = %s)", on ? "ON" : "OFF", _pin, on ? "LOW" : "HIGH");
    }
}

void RelayControl::toggle() {
    logger.info(TAG, "Toggle");
    setState(!_state);
}

bool RelayControl::isOn() const {
    return _state;
}

void RelayControl::update() {
    if (_nvsDirty && (millis() - _lastStateChange > NVS_WRITE_DEBOUNCE_MS)) {
        configManager.setRelayLast(_state);
        _nvsDirty = false;
        logger.debug(TAG, "NVS flushed: relay_last=%s", _state ? "true" : "false");
    }
}
