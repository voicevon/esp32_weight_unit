#include "SlaveApp.h"
#include "ProductionHandler.h"
#include "CalibrationHandler.h"
#include "DiagnosticHandler.h"

SlaveApp::SlaveApp(WeighingScale& scale, TinyScreen& oled, ModbusSlave& modbus, int buttonPin)
    : _scale(scale), _oled(oled), _modbus(modbus), _btn(buttonPin) {}

void SlaveApp::begin() {
    _btn.begin();
    _oled.begin();
    _scale.begin();
    
    // 默认进入生产工作模式
    switchMode(UI_RUN);
}

void SlaveApp::loop() {
    // 监听按键事件进行模式切换 (全局逻辑层)
    ButtonEvent event = _btn.scan();
    if (event == BTN_LONG_PRESS) {
        if (_curMode == UI_RUN) switchMode(UI_MENU_CALIB);
        else if (_curMode == UI_MENU_CALIB) switchMode(UI_RS485_DIAG);
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
