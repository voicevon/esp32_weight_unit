#include "CommComponent.h"

CommComponent::CommComponent(int rxPin, int txPin, int enPin, long baud) 
    : _rxPin(rxPin), _txPin(txPin), _enPin(enPin), _baud(baud), _dataId(0) {}

void CommComponent::begin(uint8_t slaveId) {
    _slaveId = slaveId;
    Serial2.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    _mb.begin(&Serial2, _enPin);
    _mb.slave(_slaveId);
    
    // 初始化 Hregs (Hold Registers)
    _mb.addHreg(REG_WEIGHT_H, 0, 2);
    _mb.addHreg(REG_STATUS, 0);
    _mb.addHreg(REG_RAW_ADC_H, 0, 2);
    _mb.addHreg(REG_CTRL_CMD, 0);
    _mb.addHreg(REG_ZTR_THRESH, 2);
    _mb.addHreg(REG_DOOR_TIME, 1000);
    _mb.addHreg(REG_DATA_ID, 0);
}

void CommComponent::task() {
    _mb.task();
}

void CommComponent::updateWeight(float weight) {
    uint16_t regs[2];
    floatToRegs(weight, regs);
    _mb.Hreg(REG_WEIGHT_H, regs[0]);
    _mb.Hreg(REG_WEIGHT_L, regs[1]);
}

void CommComponent::updateStatus(SlaveState sysState, bool stable) {
    uint16_t val = (uint8_t)sysState; // 低 8 位系统状态
    if (stable) val |= (1 << 8);      // 第 8 位稳定性
    _mb.Hreg(REG_STATUS, val);
}

void CommComponent::updateRawADC(int32_t raw) {
    _mb.Hreg(REG_RAW_ADC_H, (uint16_t)(raw >> 16));
    _mb.Hreg(REG_RAW_ADC_L, (uint16_t)(raw & 0xFFFF));
}

void CommComponent::incrementDataId() {
    _dataId++;
    _mb.Hreg(REG_DATA_ID, _dataId);
}

uint16_t CommComponent::getControlCommand() {
    return _mb.Hreg(REG_CTRL_CMD);
}

void CommComponent::clearControlCommand() {
    _mb.Hreg(REG_CTRL_CMD, 0);
}

uint16_t CommComponent::getZtrThreshold() {
    return _mb.Hreg(REG_ZTR_THRESH);
}

void CommComponent::setZtrThreshold(uint16_t ztr) {
    _mb.Hreg(REG_ZTR_THRESH, ztr);
}

uint16_t CommComponent::getDoorTime() {
    return _mb.Hreg(REG_DOOR_TIME);
}

void CommComponent::setDoorTime(uint16_t ms) {
    _mb.Hreg(REG_DOOR_TIME, ms);
}

void CommComponent::floatToRegs(float val, uint16_t* regs) {
    union {
        float f;
        uint16_t w[2];
    } converter;
    converter.f = val;
    regs[0] = converter.w[1]; // High word first
    regs[1] = converter.w[0];
}

// 诊断 API
uint32_t CommComponent::getRxByteCount() { return 0; } // 暂时不统计
uint8_t CommComponent::getLastRxByte() { return 0; }
void CommComponent::sendRawByte(uint8_t b) {}
int CommComponent::readRawByte() { return -1; }
