#include "ConfigZtrHandler.h"
#include <Preferences.h>

ConfigZtrHandler::ConfigZtrHandler(SlaveApp* app) : _app(app) {
    _tempZtrValue = _app->getZtrThreshold();
}

void ConfigZtrHandler::enter() {
    _app->getOled().showMessage("ZERO TRACK", 800);
}

bool ConfigZtrHandler::update(ButtonEvent event) {
    bool consumed = false;
    if (event == BTN_CLICK) {
        if (_tempZtrValue == 2) _tempZtrValue = 5;
        else if (_tempZtrValue == 5) _tempZtrValue = 10;
        else _tempZtrValue = 2; // Cycle back to 2
        Serial.printf("[CONFIG] Temp ZTR: %d g\n", _tempZtrValue);
        consumed = true;
    }

    unsigned long now = millis();
    if (now - _lastUIUpdate >= 50) {
        _lastUIUpdate = now;
        // 使用 TinyScreen::update 并传入 _tempZtrValue 作为 displayParam
        // 使用 getTrueFilteredWeight() 让用户在设置时能看到真实的零飘情况
        _app->getOled().update(UI_CONFIG_ZTR, SLAVE_INIT, _app->getScale().getTrueFilteredWeight(), 
                               _app->getScale().getLastRaw(), _app->getNodeId(), 
                               false, _tempZtrValue, false);
    }
    return consumed;
}

void ConfigZtrHandler::exit() {
    if (_tempZtrValue != _app->getZtrThreshold()) {
        // 更新并保存配置
        Preferences prefs;
        prefs.begin("weight_unit", false);
        prefs.putInt("ztr_thresh", (int)_tempZtrValue);
        prefs.end();
        
        // 更新 runtime 状态
        _app->setZtrThreshold(_tempZtrValue);
        
        Serial.printf("[CONFIG] ZTR Saved & Applied: %d g\n", _tempZtrValue);
    }
}
