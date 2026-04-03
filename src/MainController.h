#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include <Preferences.h>
#include <ESP32Servo.h>
#include "WeighingScale.h"
#include "TinyScreen.h"
#include "ModbusSlave.h"
#include "Button.h"
#include "SystemTypes.h"
#include "config.h"

class MainController {
public:
    MainController(WeighingScale& scale, TinyScreen& oled, 
                   ModbusSlave& modbus, int buttonPin, int servoPin);
    void begin();
    void loop();

private:
    WeighingScale& _scale;
    TinyScreen& _oled;
    ModbusSlave& _modbus;
    Button _btn; 
    Servo _servo;
    
    int _buttonPin, _servoPin;
    SemaphoreHandle_t _mutexComm;
    Preferences _prefs;

    SlaveState _state;       
    UIMode _uiMode;         
    
    int _currentId;
    int _ztrThreshold;      
    int _doorWaitTime;      
    
    unsigned long _stateTimer;
    bool _pendingTare;
    int _calTargetWeight;
    
    unsigned long _lastUIUpdateMillis = 0;
    int _ztrIndex;
    int _calWeightIndex;

    // 链路诊断变量
    uint8_t _diagTxCounter = 0;
    uint8_t _diagRxByte = 0;
    uint16_t _diagRxCount = 0;
    unsigned long _lastDiagTime = 0;

    const int _ztrOptions[4] = {0, 2, 5, 10};
    const int _calWeights[5] = {0, 100, 200, 500, 1000};

    TaskHandle_t _commTaskHandle;
    static void commTask(void* pvParameters);

    void handleSampling();   
    void handleLogic();      
    void handleUI();         
    void handleComm();       
    
    void updateState(SlaveState newState, const char* reason = "");
    void performCalibration(int weight);
};

#endif
