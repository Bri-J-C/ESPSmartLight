#include "button.h"
#include "config_manager.h"
#include "relay.h"
#include "status_led.h"
#include "mqtt_manager.h"
#include "logger.h"
#include "config.h"

static const char* TAG = "BTN";

ButtonHandler buttonHandler;

void ButtonHandler::begin() {
    _pin = configManager.getButtonPin();
    pinMode(_pin, INPUT_PULLUP);
    logger.info(TAG, "Button on pin %d", _pin);
}

void ButtonHandler::update() {
    if (_restartPending && (millis() - _restartScheduled >= 1000)) {
        if (_factoryResetPending) {
            configManager.factoryReset();
        }
        ESP.restart();
    }

    bool reading = digitalRead(_pin);

    if (reading != _lastReading) {
        _lastDebounceTime = millis();
    }
    _lastReading = reading;

    if (millis() - _lastDebounceTime < BUTTON_DEBOUNCE_MS) return;

    if (reading != _buttonState) {
        _buttonState = reading;

        if (_buttonState == LOW) {
            _pressed = true;
            _pressStartTime = millis();
            _actionTaken = false;
            logger.debug(TAG, "Pressed");
        } else {
            if (_pressed && !_actionTaken) {
                unsigned long holdTime = millis() - _pressStartTime;
                if (holdTime >= BUTTON_WIFI_CLEAR_MIN_MS && holdTime < BUTTON_WIFI_CLEAR_MAX_MS) {
                    logger.warn(TAG, "WiFi clear (held %lums)", holdTime);
                    statusLed.setPattern(LedPattern::TRIPLE_BLINK);
                    configManager.clearWifi();
                    _restartPending = true;
                    _restartScheduled = millis();
                } else {
                    logger.info(TAG, "Short press (%lums) — toggle", holdTime);
                    relayControl.toggle();
                    mqttManager.publishState();
                }
            }
            _pressed = false;
        }
    }

    if (_pressed) {
        unsigned long holdTime = millis() - _pressStartTime;

        if (holdTime >= BUTTON_FACTORY_RESET_MS && !_actionTaken) {
            _actionTaken = true;
            logger.error(TAG, "Factory reset! (held %lums)", holdTime);
            statusLed.setPattern(LedPattern::RAPID_FLASH);
            _factoryResetPending = true;
            _restartPending = true;
            _restartScheduled = millis();
        } else if (holdTime >= BUTTON_WIFI_CLEAR_MIN_MS && holdTime < BUTTON_WIFI_CLEAR_MAX_MS) {
            statusLed.setPattern(LedPattern::TRIPLE_BLINK);
        } else if (holdTime >= BUTTON_WIFI_CLEAR_MAX_MS && holdTime < BUTTON_FACTORY_RESET_MS) {
            statusLed.setPattern(LedPattern::RAPID_FLASH);
        }
    }
}
