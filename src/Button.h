#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

enum ButtonEvent {
    BTN_NONE,
    BTN_CLICK,
    BTN_DOUBLE_CLICK,
    BTN_LONG_PRESS
};

class Button {
public:
    Button(int pin);
    void begin();
    ButtonEvent scan();

private:
    int _pin;
    bool _lastState;
    bool _isPressed;
    unsigned long _pressStartTime;
    unsigned long _lastReleaseTime;
    bool _longPressHandled;
    bool _potentialDoubleClick;
    
    static const unsigned long DEBOUNCE_MS = 20;
    static const unsigned long LONG_PRESS_MS = 1500;
    static const unsigned long DOUBLE_CLICK_TIMEOUT_MS = 200;
};

#endif
