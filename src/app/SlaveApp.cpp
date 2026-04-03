#include "SlaveApp.h"
#include "ProductionHandler.h"
#include "CalibrationHandler.h"
#include "DiagnosticHandler.h"
#include "ConfigIdHandler.h"
#include "ConfigServoHandler.h"
#include <Preferences.h>

SlaveApp::SlaveApp(WeighingScale& scale, TinyScreen& oled, ModbusSlave& modbus, 
                   int buttonPin, int servoPin)
    : _scale(scale), _oled(oled), _modbus(modbus), _btn(buttonPin), _servoPin(servoPin) {}

void SlaveApp::begin() {
    _btn.begin();
    _oled.begin();
    _scale.begin();
    _servo.attach(_servoPin);
    _servo.write(0); // 初始关闭
    _servoOpen = false;

    // 从 EEPROM (Preferences) 读取配置
    Preferences prefs;
    prefs.begin("weight_unit", true); // 只读模式
    _nodeId = prefs.getUChar("slave_id", 1);
    _ztrThreshold = (uint16_t)prefs.getInt("ztr_thresh", 2);
    _doorTime = (uint16_t)prefs.getInt("door_time", 1000);
    prefs.end();

    // 初始化 Modbus 从站
    _modbus.begin(_nodeId);
    _modbus.setZtrThreshold(_ztrThreshold);
    _modbus.setDoorTime(_doorTime);
    
    Serial.printf("[SYSTEM] NodeID=%d, ZTR=%d, Door=%dms\n", _nodeId, _ztrThreshold, _doorTime);
    
    // 默认进入生产工作模式
    switchMode(UI_RUN);
}

void SlaveApp::loop() {
    // 监听按键事件进行模式切换 (全局逻辑层)
    ButtonEvent event = _btn.scan();
    if (event == BTN_LONG_PRESS) {
        if (_curMode == UI_RUN) switchMode(UI_MENU_CALIB);
        else if (_curMode == UI_MENU_CALIB) switchMode(UI_CONFIG_ID);
        else if (_curMode == UI_CONFIG_ID) switchMode(UI_CONFIG_SERVO);
        else if (_curMode == UI_CONFIG_SERVO) switchMode(UI_RS485_DIAG);
        else if (_curMode == UI_RS485_DIAG) switchMode(UI_RUN);
    }

    if (_activeHandler) {
        _activeHandler->update();
    }
}

void SlaveApp::switchMode(UIMode next) {
    if (_curMode == next && _activeHandler != nullptr) return;

    if (_activeHandler) {
        _activeHandler->exit();
        delete _activeHandler;
        _activeHandler = nullptr;
    }

    _curMode = next;
    
    switch (next) {
        case UI_RUN:
            _activeHandler = new ProductionHandler(this);
            break;
        case UI_MENU_CALIB:
            _activeHandler = new CalibrationHandler(this);
            break;
        case UI_CONFIG_ID:
            _activeHandler = new ConfigIdHandler(this);
            break;
        case UI_CONFIG_SERVO:
            _activeHandler = new ConfigServoHandler(this);
            break;
        case UI_RS485_DIAG:
            _activeHandler = new DiagnosticHandler(this);
            break;
        default:
            break;
    }

    if (_activeHandler) {
        _activeHandler->enter();
    }
    
    Serial.printf("[SYSTEM] Mode Switched to %d\n", next);
}

void SlaveApp::toggleServo() {
    setServo(!_servoOpen);
}

void SlaveApp::setServo(bool open) {
    _servoOpen = open;
    if (_servoOpen) {
        _servo.write(90);
        Serial.println("[SERVO] Manual OPEN");
    } else {
        _servo.write(0);
        Serial.println("[SERVO] Manual CLOSE");
    }
}
