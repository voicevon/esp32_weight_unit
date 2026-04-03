#ifndef SLAVE_APP_H
#define SLAVE_APP_H

#include <Arduino.h>
#include "../common/SystemTypes.h"
#include "IModeHandler.h"
#include "../hal/WeighingScale.h"
#include "../hal/TinyScreen.h"
#include "../hal/ModbusSlave.h"
#include "../hal/Button.h"

/**
 * @class SlaveApp
 * @brief 顶层应用管理器：负责模式切换与资源分发。
 */
class SlaveApp {
public:
    SlaveApp(WeighingScale& scale, TinyScreen& oled, ModbusSlave& modbus, int buttonPin);
    void begin();
    void loop();
    void switchMode(UIMode next);

    // 共享服务获取接口
    WeighingScale& getScale() { return _scale; }
    TinyScreen& getOled() { return _oled; }
    ModbusSlave& getModbus() { return _modbus; }
    Button& getButton() { return _btn; }

private:
    WeighingScale& _scale;
    TinyScreen& _oled;
    ModbusSlave& _modbus;
    Button _btn;

    IModeHandler* _activeHandler = nullptr;
    UIMode _curMode;
};

#endif
