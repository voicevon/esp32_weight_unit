#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H

#include <Arduino.h>
#include <ModbusRTU.h>
#include "SystemTypes.h"

// Modbus 寄存器地址定义
#define REG_WEIGHT_H    0x0000
#define REG_WEIGHT_L    0x0001
#define REG_STATUS      0x0002
#define REG_RAW_ADC_H   0x0003
#define REG_RAW_ADC_L   0x0004
#define REG_CTRL_CMD    0x0100
#define REG_ZTR_THRESH  0x0101 // 零漂追踪死区阈值
#define REG_DOOR_TIME   0x0102 // 开门等待时间 (ms)
#define REG_DATA_ID     0x0005 // 数据新鲜度 ID (紧贴 RawADC)

class ModbusSlave {
public:
    ModbusSlave(int rxPin, int txPin, int enPin, long baud);
    void begin(uint8_t slaveId);
    void task(); 

    void updateWeight(float weight);
    void updateStatus(SlaveState sysState, bool stable);
    void updateRawADC(int32_t raw);
    void incrementDataId(); 
    
    uint16_t getControlCommand();
    void clearControlCommand();
    
    uint16_t getZtrThreshold();
    void setZtrThreshold(uint16_t ztr);
    uint16_t getDoorTime();
    void setDoorTime(uint16_t ms);

    // 链路层直读接口 (用于诊断模式)
    bool availableRaw();
    uint8_t readRawByte();
    void sendRawByte(uint8_t data);

private:
    int _rxPin, _txPin, _enPin;
    long _baud;
    uint8_t _slaveId;
    ModbusRTU _mb;
    uint16_t _dataId;

    void floatToRegs(float val, uint16_t* regs);
};

#endif
