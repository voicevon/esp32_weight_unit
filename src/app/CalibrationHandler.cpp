#include "CalibrationHandler.h"

CalibrationHandler::CalibrationHandler(SlaveApp* app) 
    : _app(app), _timer(0), _calWeightIndex(0) {}

void CalibrationHandler::enter() {
    _app->getOled().showMessage("CALIBRATION ON", 1000);
    _timer = millis();
}

void CalibrationHandler::update(ButtonEvent event) {
    handleUI(event);
    handleCalib();
}

void CalibrationHandler::exit() {
    _app->getOled().showMessage("SAVING...", 500);
    _app->getScale().saveCalibration();
}

void CalibrationHandler::handleUI(ButtonEvent event) {
    // 监听按键切换标定砝码
    if (event == BTN_CLICK) {
        _calWeightIndex = (_calWeightIndex + 1) % 5;
    }
    
    // 定时更新显示
    unsigned long now = millis();
    _app->getOled().update(UI_MENU_CALIB, SLAVE_CALIBRATING, 
                          _app->getScale().getFilteredWeight(), 
                          _app->getScale().getLastRaw(), _app->getNodeId(), false, 
                          _calWeights[_calWeightIndex], false, 0, 0);
}

void CalibrationHandler::handleCalib() {
    // 标定具体逻辑集成
    // (例如：长按确认标定)
}
