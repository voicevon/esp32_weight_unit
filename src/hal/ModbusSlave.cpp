#include "ModbusSlave.h"

static const uint16_t crcTable[] = {
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

ModbusSlave::ModbusSlave(int rxPin, int txPin, int enPin, long baud) 
    : _rxPin(rxPin), _txPin(txPin), _enPin(enPin), _baud(baud), _dataId(0) {
    memset(_regBank, 0, sizeof(_regBank));
}

void ModbusSlave::begin(uint8_t slaveId) {
    _slaveId = slaveId;
    _charTimeUs = 1000000 * 11 / _baud; // 计算单个字符(11位)传输所需的微秒数
    Serial2.begin(_baud, SERIAL_8N2, _rxPin, _txPin);
    
    if (_enPin >= 0) {
        pinMode(_enPin, OUTPUT);
        digitalWrite(_enPin, LOW); // 默认接收模式
    }

    // 初始化寄存器默认值
    _regBank[REG_DOOR_TIME] = 1000;
    _regBank[REG_ZTR_THRESH] = 2;
}

void ModbusSlave::task() {
    // 1. 接收数据与 3.5T 判定
    while (Serial2.available()) {
        if (_rxLen < sizeof(_rxBuf)) {
            if (_rxLen == 0) _firstByteTime = millis(); // 记录首字节到达时间 (证据：时效基准)
            _rxBuf[_rxLen++] = Serial2.read();
            _lastByteTime = micros();
        } else {
            Serial2.read(); // 缓冲区满，丢弃
        }
    }

    // 2. 检查静默时间 (Modbus 标准 3.5T 判定一帧结束)
    // 我们取 4 个字符时间作为阈值，确保稳定，最小不低于 2ms
    uint32_t silenceThreshold = (_charTimeUs * 4 > 2000) ? (_charTimeUs * 4) : 2000;
    if (_rxLen > 0 && (micros() - _lastByteTime > silenceThreshold)) {
        processFrame();
        _rxLen = 0; // 处理完清空
    }
}

void ModbusSlave::processFrame() {
    if (_rxLen < 4) return; // 最小有效帧 [ID, FN, CRC16_L, CRC16_H]

    // 时效性校验：放宽到 1500ms。
    if (millis() - _firstByteTime > 1500) {
        Serial.printf("[SLAVE_DIAG] DISCARD: Request too old (%lu ms)\n", millis() - _firstByteTime);
        _rxLen = 0;
        return;
    }


    // 1. 检查 ID (0 为广播)
    if (_rxBuf[0] != _slaveId && _rxBuf[0] != 0x00) return;

    // 2. 校验 CRC
    uint16_t calcCrc = calculateCRC(_rxBuf, _rxLen - 2);
    uint16_t rxCrc = _rxBuf[_rxLen - 2] | (_rxBuf[_rxLen - 1] << 8);
    if (calcCrc != rxCrc) return;

    uint8_t fn = _rxBuf[1];
    if (fn == 0x03) { // 读保持寄存器
        uint16_t startAddr = (_rxBuf[2] << 8) | _rxBuf[3];
        uint16_t quantity = (_rxBuf[4] << 8) | _rxBuf[5];

        if (startAddr + quantity > MAX_REGS) {
            sendException(fn, 0x02); // 非法地址
            return;
        }

        uint8_t resp[128];
        resp[0] = _slaveId;
        resp[1] = 0x03;
        resp[2] = quantity * 2;
        for (int i = 0; i < quantity; i++) {
            uint16_t val = _regBank[startAddr + i];
            resp[3 + i * 2] = val >> 8;
            resp[4 + i * 2] = val & 0xFF;
        }
        
        unsigned long procTime = millis() - _firstByteTime;
        // Serial.printf("[SLAVE_DIAG] RX 0x03 ID:%d, Addr:0x%04X, Qty:%d, Proc:%lu ms\n", 
        //               _rxBuf[0], startAddr, quantity, procTime);
                      
        sendResponse(resp, 3 + quantity * 2);

        
    } else if (fn == 0x06) { // 写单个寄存器
        uint16_t addr = (_rxBuf[2] << 8) | _rxBuf[3];
        uint16_t val = (_rxBuf[4] << 8) | _rxBuf[5];

        if (addr >= MAX_REGS) {
            sendException(fn, 0x02);
            return;
        }

        _regBank[addr] = val;
        
        unsigned long procTime = millis() - _firstByteTime;
        // Serial.printf("[SLAVE_DIAG] RX 0x06 ID:%d, Addr:0x%04X, Value:%d, Proc:%lu ms\n", 
        //               _rxBuf[0], addr, val, procTime);

        // 广播不回复
        if (_rxBuf[0] != 0x00) {
            sendResponse(_rxBuf, 6); // 0x06 回复原帧即可
        }
    } else {
        sendException(fn, 0x01); // 非法功能码
    }
}

uint16_t ModbusSlave::calculateCRC(uint8_t* buf, int len) {
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crcTable[(crc ^ buf[i]) & 0xFF];
    }
    return crc;
}

void ModbusSlave::sendResponse(uint8_t* buf, int len) {
    uint16_t crc = calculateCRC(buf, len);
    buf[len++] = crc & 0xFF;
    buf[len++] = (crc >> 8) & 0xFF;

    if (_enPin >= 0) digitalWrite(_enPin, HIGH);
    delayMicroseconds(_charTimeUs / 2); // 硬件切换稳定时间：半个字符
    Serial2.write(buf, len);
    Serial2.flush();       
    delayMicroseconds(_charTimeUs);     // 确保最后一比特移出移位寄存器：1个字符
    if (_enPin >= 0) digitalWrite(_enPin, LOW);
}

void ModbusSlave::sendException(uint8_t fn, uint8_t code) {
    uint8_t resp[5];
    resp[0] = _slaveId;
    resp[1] = fn | 0x80;
    resp[2] = code;
    sendResponse(resp, 3);
}

// -------------------------
// 业务接口适配
// -------------------------

void ModbusSlave::updateWeight(float weight) {
    uint16_t regs[2];
    floatToRegs(weight, regs);
    _regBank[REG_WEIGHT_H] = regs[0];
    _regBank[REG_WEIGHT_L] = regs[1];
}

void ModbusSlave::updateStatus(SlaveState sysState, bool stable) {
    uint16_t val = (uint8_t)sysState;
    if (stable) val |= (1 << 8); 
    _regBank[REG_STATUS] = val;
}

void ModbusSlave::updateRawADC(int32_t raw) {
    _regBank[REG_RAW_ADC_H] = (uint16_t)(raw >> 16);
    _regBank[REG_RAW_ADC_L] = (uint16_t)(raw & 0xFFFF);
}

void ModbusSlave::incrementDataId() {
    _dataId++;
    _regBank[REG_DATA_ID] = _dataId;
}

uint16_t ModbusSlave::getControlCommand() {
    return _regBank[REG_CTRL_CMD];
}

void ModbusSlave::clearControlCommand() {
    _regBank[REG_CTRL_CMD] = 0;
}

uint16_t ModbusSlave::getZtrThreshold() {
    return _regBank[REG_ZTR_THRESH];
}

void ModbusSlave::setZtrThreshold(uint16_t ztr) {
    _regBank[REG_ZTR_THRESH] = ztr;
}

uint16_t ModbusSlave::getDoorTime() {
    return _regBank[REG_DOOR_TIME];
}

void ModbusSlave::setDoorTime(uint16_t ms) {
    _regBank[REG_DOOR_TIME] = ms;
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

bool ModbusSlave::availableRaw() { return Serial2.available() > 0; }
uint8_t ModbusSlave::readRawByte() { return Serial2.read(); }
void ModbusSlave::sendRawByte(uint8_t data) {
    if (_enPin >= 0) digitalWrite(_enPin, HIGH);
    delayMicroseconds(_charTimeUs / 2); 
    Serial2.write(data);
    Serial2.flush();
    delayMicroseconds(_charTimeUs); 
    if (_enPin >= 0) digitalWrite(_enPin, LOW);
}
