#include "CommComponent.h"

class CountingStream : public Stream {
public:
    CountingStream(Stream& s) : _s(s), _count(0), _lastByte(0) {}
    int available() override { return _s.available(); }
    int read() override {
        int r = _s.read();
        if (r != -1) {
            _count++;
            _lastByte = (uint8_t)r;
        }
        return r;
    }
    int peek() override { return _s.peek(); }
    size_t write(uint8_t b) override { return _s.write(b); }
    size_t write(const uint8_t *buffer, size_t size) override { return _s.write(buffer, size); }
    void flush() override { _s.flush(); }
    uint32_t getCount() const { return _count; }
    uint8_t getLastByte() const { return _lastByte; }
    void clearCount() { _count = 0; }
    uint32_t baudRate() { return 9600; } // Satisfy ModbusRTU template requirement
private:
    Stream& _s;
    uint32_t _count;
    uint8_t _lastByte;
};

static CountingStream _countingSerial(Serial2);

CommComponent::CommComponent(int rxPin, int txPin, int enPin, long baud)
    : _rxPin(rxPin), _txPin(txPin), _enPin(enPin), _baud(baud) {}

void CommComponent::begin(uint8_t slaveId) {
    _slaveId = slaveId;
    
    // 初始化串口用于 Modbus RTU
    Serial2.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    
    // 初始化 Modbus 库
    _mb.begin(&_countingSerial, _enPin);
    _mb.slave(_slaveId);
    
    // 增加设计文档中定义的寄存器
    _mb.addHreg(REG_WEIGHT_H, 0x0000, 2); // 连续 2 个寄存器存放 Float
    _mb.addHreg(REG_STATUS, 0x0000);
    _mb.addHreg(REG_RAW_ADC_H, 0x0000, 2); // 连续 2 个寄存器存放 Int32 ADC
    _mb.addHreg(REG_CTRL_CMD, 0x0000);     // 命令控制寄存器
    _mb.addHreg(REG_ZTR_THRESH, 0x0002);   // 默认零点追踪死区为2g
    _mb.addHreg(REG_DOOR_TIME, 1000);      // 默认开门等待 1000ms
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

void CommComponent::updateStatus(uint8_t sysState, bool stable, uint8_t doorPhase) {
    // REG_STATUS (0x0002):
    // Bits 0-7: SystemState
    // Bit 8: Stable Flag
    // Bits 9-11: Door Phase (0-7)
    uint16_t status = (uint16_t)sysState;
    if (stable) status |= (1 << 8);
    status |= ((doorPhase & 0x07) << 9);
    
    _mb.Hreg(REG_STATUS, status);
}

void CommComponent::updateRawADC(int32_t raw) {
    _mb.Hreg(REG_RAW_ADC_H, (uint16_t)(raw >> 16));
    _mb.Hreg(REG_RAW_ADC_L, (uint16_t)(raw & 0xFFFF));
}

uint32_t CommComponent::getRxByteCount() {
    return _countingSerial.getCount();
}

uint8_t CommComponent::getLastRxByte() {
    return _countingSerial.getLastByte();
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

uint16_t CommComponent::getDoorTime() {
    return _mb.Hreg(REG_DOOR_TIME);
}

void CommComponent::setDoorTime(uint16_t ms) {
    _mb.Hreg(REG_DOOR_TIME, ms);
}

void CommComponent::sendRawByte(uint8_t b) {
    digitalWrite(_enPin, HIGH); // 强制发送模式
    delayMicroseconds(50);      // 稳定时间
    Serial2.write(b);
    Serial2.flush();            // 确保发送完毕
    delayMicroseconds(50);
    digitalWrite(_enPin, LOW);  // 回到接收模式
}

int CommComponent::readRawByte() {
    if (Serial2.available()) {
        return Serial2.read();
    }
    return -1;
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
