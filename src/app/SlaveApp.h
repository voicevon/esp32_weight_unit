#ifndef SLAVE_APP_H
#define SLAVE_APP_H
 
#include <Arduino.h>
#include <ESP32Servo.h>
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
    SlaveApp(WeighingScale& scale, TinyScreen& oled, ModbusSlave& modbus, 
             int buttonPin, int servoPin);
    void begin();
    void loop();
    void switchMode(UIMode next);

    // 共享服务获取接口
    WeighingScale& getScale() { return _scale; }
    TinyScreen& getOled() { return _oled; }
    ModbusSlave& getModbus() { return _modbus; }
    Button& getButton() { return _btn; }

    uint8_t getNodeId() const { return _nodeId; }
    void setNodeId(uint8_t id) { _nodeId = id; }
    
    bool isServoOpen() const { return _servoOpen; }
    void toggleServo();
    void setServo(bool open);

    uint16_t getZtrThreshold() const { return _ztrThreshold; }
    void setZtrThreshold(uint16_t threshold);
    uint16_t getDoorTime() const { return _doorTime; }

    static const uint8_t SERVO_OPEN_ANGLE = 72;
    static const uint8_t SERVO_CLOSE_ANGLE = 0;

private:
    WeighingScale& _scale;
    TinyScreen& _oled;
    ModbusSlave& _modbus;
    Button _btn;
    Servo _servo;

    IModeHandler* _activeHandler = nullptr;
    UIMode _curMode;
    int _servoPin;
    uint8_t _nodeId = 1;
    uint16_t _ztrThreshold = 2;
    uint16_t _doorTime = 1000;
    bool _servoOpen = false;
};

#endif
