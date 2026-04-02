#include "ModbusSlave.h"

ModbusSlave::ModbusSlave(int rxPin, int txPin, int enPin, long baud) 
    : _rxPin(rxPin), _txPin(txPin), _enPin(enPin), _baud(baud), _dataId(0) {}

void ModbusSlave::begin(uint8_t slaveId) {
    _slaveId = slaveId;
    Serial2.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    _mb.begin(&Serial2, _enPin);
    _mb.slave(_slaveId);
    
    _mb.addHreg(REG_WEIGHT_H, 0, 2);
    _mb.addHreg(REG_STATUS, 0);
    _mb.addHreg(REG_RAW_ADC_H, 0, 2);
    _mb.addHreg(REG_CTRL_CMD, 0);
    _mb.addHreg(REG_ZTR_THRESH, 2);
    _mb.addHreg(REG_DOOR_TIME, 1000);
    _mb.addHreg(REG_DATA_ID, 0);
}

void ModbusSlave::task() {
    static unsigned long lastLog = 0;
    static unsigned long lastBusActivity = 0;
    static uint8_t rawBuf[16];
    static int rawIdx = 0;
    unsigned long now = millis();

    // 1. 流量宏观监控 (修正版)：不读取数据，仅探测是否有数据进入，让协议栈由 _mb.task() 处理
    if (Serial2.available()) {
        lastBusActivity = now;
    }

    // 2. 协议栈处理
    _mb.task();

    // 3. 周期性运行报告 (5s 一次)
    if (now - lastLog > 5000) {
        lastLog = now;
        bool activeRecently = (now - lastBusActivity < 5000);
        Serial.printf("[SLAVE %d] Heartbeat. Bus Latency: %ld ms, RS485 Pulse: %s\n", 
                      _slaveId, (now - lastBusActivity), activeRecently ? "ACTIVE" : "IDLE");
    }
}

void ModbusSlave::updateWeight(float weight) {
    uint16_t regs[2];
    floatToRegs(weight, regs);
    _mb.Hreg(REG_WEIGHT_H, regs[0]);
    _mb.Hreg(REG_WEIGHT_L, regs[1]);
}

void ModbusSlave::updateStatus(SlaveState sysState, bool stable) {
    uint16_t val = (uint8_t)sysState;
    if (stable) val |= (1 << 8); 
    _mb.Hreg(REG_STATUS, val);
}

void ModbusSlave::updateRawADC(int32_t raw) {
    _mb.Hreg(REG_RAW_ADC_H, (uint16_t)(raw >> 16));
    _mb.Hreg(REG_RAW_ADC_L, (uint16_t)(raw & 0xFFFF));
}

void ModbusSlave::incrementDataId() {
    _dataId++;
    _mb.Hreg(REG_DATA_ID, _dataId);
}

uint16_t ModbusSlave::getControlCommand() {
    return _mb.Hreg(REG_CTRL_CMD);
}

void ModbusSlave::clearControlCommand() {
    _mb.Hreg(REG_CTRL_CMD, 0);
}

uint16_t ModbusSlave::getZtrThreshold() {
    return _mb.Hreg(REG_ZTR_THRESH);
}

void ModbusSlave::setZtrThreshold(uint16_t ztr) {
    _mb.Hreg(REG_ZTR_THRESH, ztr);
}

uint16_t ModbusSlave::getDoorTime() {
    return _mb.Hreg(REG_DOOR_TIME);
}

void ModbusSlave::setDoorTime(uint16_t ms) {
    _mb.Hreg(REG_DOOR_TIME, ms);
}

void ModbusSlave::floatToRegs(float val, uint16_t* regs) {
    union {
        float f;
        uint16_t w[2];
    } converter;
    converter.f = val;
    regs[0] = converter.w[1];
    regs[1] = converter.w[0];
}

bool ModbusSlave::availableRaw() {
    return Serial2.available() > 0;
}

uint8_t ModbusSlave::readRawByte() {
    return Serial2.read();
}

void ModbusSlave::sendRawByte(uint8_t data) {
    digitalWrite(_enPin, HIGH);
    delayMicroseconds(50); 
    Serial2.write(data);
    Serial2.flush();       
    delayMicroseconds(50);
    digitalWrite(_enPin, LOW);
}
