#include "ConfigServoHandler.h"
 
ConfigServoHandler::ConfigServoHandler(SlaveApp* app) : _app(app) {}
 
void ConfigServoHandler::enter() {
    _app->getOled().showMessage("SERVO TEST ON", 800);
}
 
bool ConfigServoHandler::update(ButtonEvent event) {
    bool consumed = false;
    if (event == BTN_CLICK) {
        _app->toggleServo();
        consumed = true;
    }
 
    unsigned long now = millis();
    if (now - _lastUIUpdate >= 50) {
        _lastUIUpdate = now;
        // 使用 displayParam 传输 1 或 0 代表 Open/Close
        _app->getOled().update(UI_CONFIG_SERVO, SLAVE_INIT, 0, 0, 
                              _app->getNodeId(), false, 
                              _app->isServoOpen() ? 1 : 0, false);
    }
    return consumed;
}
 
void ConfigServoHandler::exit() {
    // 退出菜单时自动关闭舵机，确保电机安全
    _app->setServo(false);
    Serial.println("[SERVO] Auto Close on Exit");
}
