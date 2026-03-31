#ifndef COMM_COMPONENT_H
#define COMM_COMPONENT_H

#include <Arduino.h>
#include <ModbusRTU.h>
#include "SystemTypes.h"

// Modbus 寄存器地址定义 (基于设计文档)
#define REG_WEIGHT_H    0x0000
#define REG_WEIGHT_L    0x0001
#define REG_STATUS      0x0002
#define REG_RAW_ADC_H   0x0003
#define REG_RAW_ADC_L   0x0004
#define REG_CTRL_CMD    0x0100
#define REG_ZTR_THRESH  0x0101 // 零飘追踪死区阈值
#define REG_DOOR_TIME   0x0102 // 开门等待时间 (ms)
#define REG_DATA_ID     0x0103 // 数据新鲜度 ID (每次下料更新)

class CommComponent {
public:
    CommComponent(int rxPin, int txPin, int enPin, long baud);
    void begin(uint8_t slaveId);
    void task(); // Modbus 协议栈任务句柄

    // 数据同步接口
    void updateWeight(float weight);
    void updateStatus(SlaveState sysState, bool stable);
    void updateRawADC(int32_t raw);
    void incrementDataId(); // 更新数据新鲜度标签
    
    uint32_t getRxByteCount();
    uint8_t getLastRxByte();
    
    // 原始 485 诊断接口 (绕过 Modbus)
    void sendRawByte(uint8_t b);
    int readRawByte();
    
    // 命令读取接口
    uint16_t getControlCommand();
    void clearControlCommand();
    
    // 功能设置接口
    uint16_t getZtrThreshold();
    void setZtrThreshold(uint16_t ztr);
    uint16_t getDoorTime();
    void setDoorTime(uint16_t ms);

private:
    int _rxPin, _txPin, _enPin;
    long _baud;
    uint8_t _slaveId;
    ModbusRTU _mb;
    uint16_t _dataId;

    void floatToRegs(float val, uint16_t* regs);
};

#endif
