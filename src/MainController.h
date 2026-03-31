#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include <Preferences.h>
#include <ESP32Servo.h>
#include "ScaleComponent.h"
#include "DisplayComponent.h"
#include "CommComponent.h"
#include "SystemTypes.h"
#include "config.h"

class MainController {
public:
    MainController(ScaleComponent& scale, DisplayComponent& display, 
                   CommComponent& comm, int buttonPin, int servoPin);
    void begin();
    void loop();

private:
    // 组件引用
    ScaleComponent& _scale;
    DisplayComponent& _display;
    CommComponent& _comm;
    Servo _servo;
    
    // 硬件定义
    int _buttonPin, _servoPin;
    SemaphoreHandle_t _mutexComm;
    Preferences _prefs;

    // --- 核心状态变量 ---
    SlaveState _state;       // 逻辑状态
    UIMode _uiMode;         // UI 菜单模式
    
    // --- 业务参数 ---
    int _currentId;
    int _ztrThreshold;      // 零漂阈值 (g)
    int _doorWaitTime;      // 开门时间 (ms)
    
    // --- 状态机控制 ---
    unsigned long _stateTimer;
    bool _pendingTare;
    int _calTargetWeight;
    
    // --- UI/交互变量 ---
    unsigned long _lastUpdateMillis;
    bool _btnPressed;
    unsigned long _btnPressStart;
    unsigned long _lastBtnRelease;
    bool _longPressHandled;
    int _ztrIndex;
    int _calWeightIndex;

    const int _ztrOptions[4] = {0, 2, 5, 10};
    const int _calWeights[5] = {0, 100, 200, 500, 1000};

    // --- 任务句柄 ---
    TaskHandle_t _commTaskHandle;
    static void commTask(void* pvParameters);

    // --- 内部逻辑模块 ---
    void handleSampling();   // 数据采样与滤波
    void handleLogic();      // 核心业务状态机
    void handleUI();         // 界面更新与按钮
    void handleComm();       // Modbus 交互处理
    
    void updateState(SlaveState newState, const char* reason = "");
    void performCalibration(int weight);
};

#endif
