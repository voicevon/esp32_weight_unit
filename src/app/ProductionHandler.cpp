#include "ProductionHandler.h"

ProductionHandler::ProductionHandler(SlaveApp* app) 
    : _app(app), _state(SLAVE_STANDBY), _stateTimer(0), _pendingTare(false) {}

void ProductionHandler::enter() {
    _app->getOled().showMessage("NORMAL MODE", 800);
}

bool ProductionHandler::update(ButtonEvent event) {
    bool consumed = false;
    if (event == BTN_CLICK) {
        _app->getScale().tare();
        _app->getOled().showLargeMessage("TARED", 2000);
        consumed = true;
    }

    handleSampling();
    handleLogic();
    handleComm();
    handleUI();
    return consumed;
}

void ProductionHandler::exit() {
    // 退出前确保舵机关闭
    // (逻辑安全检查)
}

void ProductionHandler::handleSampling() {
    WeighingScale& scale = _app->getScale();
    if (scale.isReady()) {
        long raw = scale.getRawValue();
        scale.update(raw);
    }
}

void ProductionHandler::handleLogic() {
    unsigned long now = millis();
    switch (_state) {
        case SLAVE_DISCHARGING:
            // 出料逻辑
            break;
        default: break;
    }
}

void ProductionHandler::handleComm() {
    ModbusSlave& modbus = _app->getModbus();
    WeighingScale& scale = _app->getScale();

    // 协议栈正常运行
    modbus.task();

    // 同步指令
    uint16_t cmd = modbus.getControlCommand();
    if (cmd != 0) {
        // 分发指令逻辑
        if (cmd == 1) { // CMD_SERVO_OPEN
            _app->setServo(true);
            Serial.println("[CMD] Modbus Servo OPEN");
        } else if (cmd == 2) { // CMD_SERVO_CLOSE
            _app->setServo(false);
            Serial.println("[CMD] Modbus Servo CLOSE");
        } else if (cmd == 3) { // CMD_TARE
            scale.tare();
            Serial.println("[CMD] Modbus TARE Executed");
            _app->getOled().showLargeMessage("TARED", 2000);
        }

        modbus.clearControlCommand();
    }

    // 更新寄存器
    float weight = scale.getFilteredWeight();
    modbus.updateWeight(weight);
    modbus.updateStatus(_state, scale.isStable(2.0f));
}

void ProductionHandler::handleUI() {
    unsigned long now = millis();
    if (now - _lastUIUpdateMillis >= 50) {
        _lastUIUpdateMillis = now;
        WeighingScale& scale = _app->getScale();
        _app->getOled().update(UI_RUN, _state, scale.getFilteredWeight(), 
                              scale.getLastRaw(), _app->getNodeId(), 
                              false, 0, scale.isStable(2.0f), 0, 0);
    }
}
