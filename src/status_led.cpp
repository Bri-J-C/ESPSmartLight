#include "status_led.h"

StatusLed statusLed;

void StatusLed::begin() {
    pinMode(STATUS_LED_PIN, OUTPUT);
    setLed(false);
}

void StatusLed::setLed(bool on) {
    _ledState = on;
    digitalWrite(STATUS_LED_PIN, STATUS_LED_ACTIVE_LOW ? !on : on);
}

void StatusLed::setPattern(LedPattern pattern) {
    if (_pattern != pattern) {
        _pattern = pattern;
        _step = 0;
        _lastToggle = millis();
    }
}

LedPattern StatusLed::getPattern() const {
    return _pattern;
}

void StatusLed::update() {
    unsigned long now = millis();
    unsigned long elapsed = now - _lastToggle;

    switch (_pattern) {
        case LedPattern::OFF:
            if (_ledState) setLed(false);
            break;

        case LedPattern::ON:
            if (!_ledState) setLed(true);
            break;

        case LedPattern::FAST_BLINK:
            if (elapsed >= 200) {
                setLed(!_ledState);
                _lastToggle = now;
            }
            break;

        case LedPattern::SLOW_BLINK:
            if (elapsed >= 1000) {
                setLed(!_ledState);
                _lastToggle = now;
            }
            break;

        case LedPattern::DOUBLE_BLINK: {
            // ON 150, OFF 150, ON 150, OFF 600
            static const uint16_t timing[] = {150, 150, 150, 600};
            if (elapsed >= timing[_step % 4]) {
                _step = (_step + 1) % 4;
                setLed(_step % 2 == 0);
                _lastToggle = now;
            }
            break;
        }

        case LedPattern::TRIPLE_BLINK: {
            // ON 150, OFF 150, ON 150, OFF 150, ON 150, OFF 600
            static const uint16_t timing[] = {150, 150, 150, 150, 150, 600};
            if (elapsed >= timing[_step % 6]) {
                _step = (_step + 1) % 6;
                setLed(_step % 2 == 0);
                _lastToggle = now;
            }
            break;
        }

        case LedPattern::SOS: {
            // S: ...  O: ---  S: ...
            // Short=100, Long=300, gap=100, letter_gap=300, word_gap=700
            static const uint16_t sos_timing[] = {
                100,100, 100,100, 100,300,  // S
                300,100, 300,100, 300,300,  // O
                100,100, 100,100, 100,700   // S
            };
            static const bool sos_state[] = {
                true,false, true,false, true,false,
                true,false, true,false, true,false,
                true,false, true,false, true,false
            };
            if (elapsed >= sos_timing[_step % 18]) {
                _step = (_step + 1) % 18;
                setLed(sos_state[_step]);
                _lastToggle = now;
            }
            break;
        }

        case LedPattern::RAPID_FLASH:
            if (elapsed >= 80) {
                setLed(!_ledState);
                _lastToggle = now;
            }
            break;
    }
}
