#ifndef COMM_COMPONENT_H
#define COMM_COMPONENT_H

#include <Arduino.h>
#include <ModbusRTU.h>

// Modbus 寄存器地址定义 (基于设计文档)
#define REG_WEIGHT_H    0x0000
#define REG_WEIGHT_L    0x0001
#define REG_STATUS      0x0002
#define REG_RAW_ADC_H   0x0003
#define REG_RAW_ADC_L   0x0004
#define REG_CTRL_CMD    0x0100
#define REG_ZTR_THRESH  0x0101 // 零飘追踪死区阈值

class CommComponent {
public:
    CommComponent(int rxPin, int txPin, int enPin, long baud);
    void begin(uint8_t slaveId);
    void task(); // Modbus 协议栈任务句柄

    // 数据同步接口
    void updateWeight(float weight);
    void updateStatus(uint16_t status);
    void updateRawADC(int32_t raw);
    
    // 命令读取接口
    uint16_t getControlCommand();
    void clearControlCommand();
    
    // 零飘阈值接口
    uint16_t getZtrThreshold();
    void setZtrThreshold(uint16_t ztr);

private:
    int _rxPin, _txPin, _enPin;
    long _baud;
    uint8_t _slaveId;
    ModbusRTU _mb;

    // 辅助函数：Float 与 Uint16 互转（Modbus 常用）
    void floatToRegs(float val, uint16_t* regs);
};

#endif
