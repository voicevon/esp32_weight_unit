#include "Button.h"
#include <Arduino.h>

Button::Button(int pin) 
    : _pin(pin), _lastState(HIGH), _isPressed(false), _pressStartTime(0),
      _lastReleaseTime(0), _longPressHandled(false), _potentialDoubleClick(false) {}

void Button::begin() {
    pinMode(_pin, INPUT_PULLUP);
}

ButtonEvent Button::scan() {
    bool currentState = (digitalRead(_pin) == LOW);
    unsigned long now = millis();
    ButtonEvent event = BTN_NONE;

    if (currentState && !_isPressed) {
        if (now - _lastReleaseTime > DEBOUNCE_MS) {
            _isPressed = true;
            _pressStartTime = now;
            _longPressHandled = false;
        }
    } else if (currentState && _isPressed) {
        if (!_longPressHandled && (now - _pressStartTime >= LONG_PRESS_MS)) {
            _longPressHandled = true;
            _potentialDoubleClick = false;
            event = BTN_LONG_PRESS;
        }
    } else if (!currentState && _isPressed) {
        _isPressed = false;
        unsigned long duration = now - _pressStartTime;
        _lastReleaseTime = now;

        if (!_longPressHandled && duration > DEBOUNCE_MS) {
            if (_potentialDoubleClick) {
                _potentialDoubleClick = false;
                event = BTN_DOUBLE_CLICK;
            } else {
                _potentialDoubleClick = true;
            }
        }
    } else if (!currentState && !_isPressed) {
        if (_potentialDoubleClick && (now - _lastReleaseTime >= DOUBLE_CLICK_TIMEOUT_MS)) {
            _potentialDoubleClick = false;
            event = BTN_CLICK;
        }
    }

    return event;
}
