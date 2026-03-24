#include "CommComponent.h"

CommComponent::CommComponent(int rxPin, int txPin, int enPin, long baud)
    : _rxPin(rxPin), _txPin(txPin), _enPin(enPin), _baud(baud) {}

void CommComponent::begin(uint8_t slaveId) {
    _slaveId = slaveId;
    
    // 初始化串口用于 Modbus RTU
    Serial2.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    
    // 初始化 Modbus 库
    _mb.begin(&Serial2, _enPin);
    _mb.slave(_slaveId);
    
    // 增加设计文档中定义的寄存器
    _mb.addHreg(REG_WEIGHT_H, 0x0000, 2); // 连续 2 个寄存器存放 Float
    _mb.addHreg(REG_STATUS, 0x0000);
    _mb.addHreg(REG_RAW_ADC_H, 0x0000, 2); // 连续 2 个寄存器存放 Int32 ADC
    _mb.addHreg(REG_CTRL_CMD, 0x0000);     // 命令控制寄存器
    _mb.addHreg(REG_ZTR_THRESH, 0x0002);   // 默认零点追踪死区为2g
}

void CommComponent::task() {
    _mb.task(); // 处理 Modbus 请求协议栈
}

void CommComponent::updateWeight(float weight) {
    uint16_t regs[2];
    floatToRegs(weight, regs);
    _mb.Hreg(REG_WEIGHT_H, regs[0]);
    _mb.Hreg(REG_WEIGHT_L, regs[1]);
}

void CommComponent::updateStatus(uint16_t status) {
    _mb.Hreg(REG_STATUS, status);
}

void CommComponent::updateRawADC(int32_t raw) {
    _mb.Hreg(REG_RAW_ADC_H, (uint16_t)(raw >> 16));
    _mb.Hreg(REG_RAW_ADC_L, (uint16_t)(raw & 0xFFFF));
}

uint16_t CommComponent::getControlCommand() {
    return _mb.Hreg(REG_CTRL_CMD);
}

void CommComponent::clearControlCommand() {
    _mb.Hreg(REG_CTRL_CMD, 0); // 指令执行后清零，防止重复执行
}

uint16_t CommComponent::getZtrThreshold() {
    return _mb.Hreg(REG_ZTR_THRESH);
}

void CommComponent::setZtrThreshold(uint16_t ztr) {
    _mb.Hreg(REG_ZTR_THRESH, ztr);
}

// 辅助函数实现
void CommComponent::floatToRegs(float val, uint16_t* regs) {
    union {
        float f;
        uint16_t r[2];
    } converter;
    converter.f = val;
    regs[0] = converter.r[0];
    regs[1] = converter.r[1];
}
