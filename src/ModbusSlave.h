#ifndef MODBUS_SLAVE_H
#define MODBUS_SLAVE_H

#include <Arduino.h>
#include "SystemTypes.h"

// Modbus 寄存器地址定义 (与原定义保持一致)
#define REG_WEIGHT_H    0x0000
#define REG_WEIGHT_L    0x0001
#define REG_STATUS      0x0002
#define REG_RAW_ADC_H   0x0003
#define REG_RAW_ADC_L   0x0004
#define REG_DATA_ID     0x0005
#define REG_CTRL_CMD    0x0100
#define REG_ZTR_THRESH  0x0101 
#define REG_DOOR_TIME   0x0102 

#define MAX_REGS        0x0110 // 覆盖到 0x010F 的空间

class ModbusSlave {
public:
    ModbusSlave(int rxPin, int txPin, int enPin, long baud);
    void begin(uint8_t slaveId);
    void task(); 

    // 业务 API (保持兼容)
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
    uint32_t _charTimeUs;
    uint8_t _slaveId;
    uint16_t _dataId;

    // 静态寄存器镜像
    uint16_t _regBank[MAX_REGS];

    // 通讯缓冲区与状态
    uint8_t _rxBuf[128];
    int     _rxLen = 0;
    unsigned long _lastByteTime = 0;

    // 内部处理函数
    uint16_t calculateCRC(uint8_t* buf, int len);
    void processFrame();
    void sendResponse(uint8_t* buf, int len);
    void sendException(uint8_t fn, uint8_t code);
    
    void floatToRegs(float val, uint16_t* regs);
};

#endif
