#include "SlaveApp.h"
#include "ProductionHandler.h"
#include "CalibrationHandler.h"
#include "DiagnosticHandler.h"
#include "ConfigIdHandler.h"
#include "ConfigServoHandler.h"
#include "ConfigZtrHandler.h"
#include <Preferences.h>

SlaveApp::SlaveApp(WeighingScale& scale, TinyScreen& oled, ModbusSlave& modbus, 
                   int buttonPin, int servoPin)
    : _scale(scale), _oled(oled), _modbus(modbus), _btn(buttonPin), _servoPin(servoPin) {}

void SlaveApp::begin() {
    _btn.begin();
    _oled.begin();
    _scale.begin();

    // 显式分配 PWM 定时器，防止与其他外设冲突
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    _servo.setPeriodHertz(50);    // MG995 标准 50Hz
    // 显式指定脉冲宽度范围 (500us - 2400us)
    _servo.attach(_servoPin, 500, 2400);
    if (_servo.attached()) {
        Serial.printf("[SYSTEM] Servo attached to pin %d\n", _servoPin);
    } else {
        Serial.printf("[ERROR] Servo ATTACH FAILED on pin %d\n", _servoPin);
    }

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
    
    // 同步给称重单元
    _scale.setZtrThreshold(_ztrThreshold);
    
    Serial.printf("[SYSTEM] NodeID=%d, ZTR=%d, Door=%dms\n", _nodeId, _ztrThreshold, _doorTime);
    
    // 默认进入生产工作模式
    switchMode(UI_RUN);
}

void SlaveApp::loop() {
    // 监听按键事件进行模式切换 (全局逻辑层)
    ButtonEvent event = _btn.scan();

    // 优先让当前 Handler 处理事件 (事件冒泡)
    bool consumed = false;
    if (_activeHandler) {
        consumed = _activeHandler->update(event);
    }

    // 如果事件未被消费，且是长按，则执行全局模式切换
    if (!consumed && event == BTN_LONG_PRESS) {
        if (_curMode == UI_RUN) switchMode(UI_CONFIG_ZTR);
        else if (_curMode == UI_CONFIG_ZTR) switchMode(UI_MENU_CALIB);
        else if (_curMode == UI_MENU_CALIB) switchMode(UI_CONFIG_ID);
        else if (_curMode == UI_CONFIG_ID) switchMode(UI_CONFIG_SERVO);
        else if (_curMode == UI_CONFIG_SERVO) switchMode(UI_RS485_DIAG);
        else if (_curMode == UI_RS485_DIAG) switchMode(UI_RUN);
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
        case UI_CONFIG_ZTR:
            _activeHandler = new ConfigZtrHandler(this);
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
    
    const char* modeNames[] = {
        "RUN", "CONFIG_ID", "CONFIG_ZTR", "CONFIG_SERVO", 
        "MENU_CALIB", "CALIB_RESULT", "RS485_DIAG", "VERSION"
    };
    Serial.printf("[SYSTEM] Mode Switched to %s\n", modeNames[next]);
}

void SlaveApp::toggleServo() {
    setServo(!_servoOpen);
}

void SlaveApp::setZtrThreshold(uint16_t threshold) {
    _ztrThreshold = threshold;
    _modbus.setZtrThreshold(threshold);
    _scale.setZtrThreshold(threshold);
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
