#include "ConfigIdHandler.h"
#include <Preferences.h>
 
ConfigIdHandler::ConfigIdHandler(SlaveApp* app) : _app(app) {
    _tempId = _app->getNodeId();
}
 
void ConfigIdHandler::enter() {
    _app->getOled().showMessage("SET ID MODE", 800);
}
 
bool ConfigIdHandler::update(ButtonEvent event) {
    bool consumed = false;
    if (event == BTN_CLICK) {
        _tempId = (_tempId % 20) + 1;
        Serial.printf("[CONFIG] Temp ID: %d\n", _tempId);
        consumed = true;
    }
 
    unsigned long now = millis();
    if (now - _lastUIUpdate >= 50) {
        _lastUIUpdate = now;
        // 使用 TinyScreen::update 并传入 _tempId
        _app->getOled().update(UI_CONFIG_ID, SLAVE_INIT, 0, 0, _tempId, false, 0, false);
    }
    return consumed;
}
 
void ConfigIdHandler::exit() {
    if (_tempId != _app->getNodeId()) {
        _app->setNodeId(_tempId);
        
        Preferences prefs;
        prefs.begin("weight_unit", false);
        prefs.putUChar("slave_id", _tempId);
        prefs.end();
        
        // 立即生效 Modbus 地址
        _app->getModbus().begin(_tempId);
        
        Serial.printf("[CONFIG] Slave ID Saved: %d\n", _tempId);
    }
}
