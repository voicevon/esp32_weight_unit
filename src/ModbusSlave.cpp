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
    _mb.task();
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
