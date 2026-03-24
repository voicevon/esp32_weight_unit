#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include <Preferences.h>

#include "ScaleComponent.h"
#include "DisplayComponent.h"
#include "CommComponent.h"
#include "SystemTypes.h"
#include "config.h"
#include <ESP32Servo.h>

class MainController {
public:
    MainController(ScaleComponent& scale, DisplayComponent& display, 
                   CommComponent& comm, int buttonPin, int servoPin);
    void begin();
    void loop();

private:
    ScaleComponent& _scale;
    DisplayComponent& _display;
    CommComponent& _comm;
    Servo _servo;
    int _buttonPin, _servoPin;
    
    SystemState _currentState;
    int _calWeightIndex;
    int _currentId;
    int _ztrThreshold;
    int _ztrIndex;
    const int _ztrOptions[4] = {0, 2, 5, 10};
    Preferences _prefs;
    const int _calWeights[5] = {0, 100, 200, 500, 1000}; // added 0 = EXIT
    
    unsigned long _btnPressStart;
    unsigned long _lastBtnRelease;
    bool _btnPressed;
    bool _longPressHandled;
    bool _servoIsOpen;
    
    // Communication Test variables
    unsigned long _lastSendTime;
    int _txCount;
    String _rxData;

    void handleButton();
    void handleComm();
    void handleCommTest();
    void performCalibration(int weight);
};

#endif
