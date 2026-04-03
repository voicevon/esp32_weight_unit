#include "ProductionHandler.h"

ProductionHandler::ProductionHandler(SlaveApp* app) 
    : _app(app), _state(SLAVE_STANDBY), _stateTimer(0), _pendingTare(false) {}

void ProductionHandler::enter() {
    _app->getOled().showMessage("NORMAL MODE", 800);
}

void ProductionHandler::update() {
    handleSampling();
    handleLogic();
    handleComm();
    handleUI();
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
                              scale.getLastRaw(), 0, false, 0, 
                              scale.isStable(2.0f), 0, 0);
    }
}
