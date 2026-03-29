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
    SemaphoreHandle_t _mutexComm;
    
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
    
    // Door sequence management
    uint8_t _doorPhase; // 0: IDLE, 1: OPENING, 2: WAITING, 3: DONE
    unsigned long _doorTimer;
    uint16_t _doorWaitTime;
    
    // Calibration management (Async)
    unsigned long _calTimer;
    int _calTargetWeight;


    // Communication Test / RS485 Diag
    unsigned long _lastUpdateMillis;
    unsigned long _lastDiagMillis;
    uint8_t _diagTxValue;
    uint8_t _diagRxValue;
    int _txCount;
    String _rxData;

    TaskHandle_t _commTaskHandle;
    static void commTask(void* pvParameters);

    void handleButton();
    void handleComm();
    void handleCommTest();
    void performCalibration(int weight);
};

#endif
