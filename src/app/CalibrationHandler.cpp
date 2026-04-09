#include "CalibrationHandler.h"

CalibrationHandler::CalibrationHandler(SlaveApp* app) 
    : _app(app), _timer(0), _calWeightIndex(0) {}

void CalibrationHandler::enter() {
    _app->getOled().showLargeMessage("CALIBRATE", 800);
    _timer = millis();
}

bool CalibrationHandler::update(ButtonEvent event) {
    handleSampling(); 
    bool consumed = handleCalib(event);
    if (handleUI(event)) consumed = true;
    return consumed;
}

void CalibrationHandler::exit() {
    _app->getOled().showMessage("SAVING...", 500);
    _app->getScale().saveCalibration();
}

void CalibrationHandler::handleSampling() {
    WeighingScale& scale = _app->getScale();
    if (scale.isReady()) {
        scale.update(scale.getRawValue());
    }
}

bool CalibrationHandler::handleUI(ButtonEvent event) {
    bool consumed = false;
    // 单击：循环切换标定重量 (EXIT, 0, 100, 200, 500, 1000)
    if (event == BTN_CLICK) {
        _calWeightIndex = (_calWeightIndex + 1) % 6; 
        consumed = true;
    }
    
    // 定时更新显示
    unsigned long now = millis();
    if (now - _timer >= 50) {
        _timer = now;
        int displayWeight = (_calWeightIndex == 0) ? -1 : _calWeights[_calWeightIndex - 1];
        
        _app->getOled().update(UI_MENU_CALIB, SLAVE_CALIBRATING, 
                              _app->getScale().getFilteredWeight(), 
                              _app->getScale().getLastRaw(), _app->getNodeId(), false, 
                              displayWeight, false, 0, 0);
    }
    return consumed;
}

bool CalibrationHandler::handleCalib(ButtonEvent event) {
    bool consumed = false;
    // 长按：确认当前选项
    if (event == BTN_LONG_PRESS) {
        consumed = true;
        if (_calWeightIndex == 0) {
            // 选择 EXIT 后长按，跳转到下一个配置菜单
            _app->switchMode(UI_CONFIG_ID);
        } else {
            // 执行标定权重逻辑
            int targetWeight = _calWeights[_calWeightIndex - 1];
            if (targetWeight == 0) {
                _app->getScale().tare();
            } else {
                _app->getScale().calibrate(targetWeight);
            }
        }
    }
    return consumed;
}
