#include "ModbusSlave.h"

static const uint16_t crcTable[] = {
    0X0000, 0XC0C1, 0XC181, 0X0140, 0X0301, 0XC3C1, 0XC281, 0X0240, 0X0601, 0XC6C1, 0XC781, 0X0740, 0X0501, 0XC5C1, 0XC481, 0X0440,
    0X0C01, 0XCCC1, 0XCD81, 0X0D40, 0X0F01, 0XCFC1, 0XCE81, 0X0E40, 0X0A01, 0XCAC1, 0XCB81, 0X0B40, 0X0901, 0XC9C1, 0XC881, 0X0840,
    0X1801, 0XD8C1, 0XD9C1, 0X1940, 0X1B01, 0XDBC1, 0XDA81, 0X1A40, 0X1E01, 0XDEC1, 0XDFC1, 0X1F40, 0X1D01, 0XDDC1, 0XDC81, 0X1C40,
    0X1401, 0XD4C1, 0XD5C1, 0X1540, 0X1701, 0XD7C1, 0XD681, 0X1640, 0X1201, 0XD2C1, 0XD3C1, 0X1340, 0X1101, 0XD1C1, 0XD081, 0X1040,
    0X3001, 0XF0C1, 0XF1C1, 0X3140, 0X3301, 0XF3C1, 0XF281, 0X3240, 0X3601, 0XF6C1, 0XF781, 0X3740, 0X3501, 0XF5C1, 0XF481, 0X3440,
    0X3C01, 0XFCC1, 0XFDC1, 0X3D40, 0X3F01, 0XFFC1, 0XFE81, 0X3E40, 0X3A01, 0XFAC1, 0XFB81, 0X3B40, 0X3901, 0XF9C1, 0XF881, 0X3840,
    0X2801, 0XE8C1, 0XE9C1, 0X2940, 0X2B01, 0XEBC1, 0XEA81, 0X2A40, 0X2E01, 0XEEC1, 0XEFC1, 0X2F40, 0X2D01, 0XEDC1, 0XEC81, 0X2C40,
    0X2401, 0XE4C1, 0XE5C1, 0X2540, 0X2701, 0XE7C1, 0XE681, 0X2640, 0X2201, 0XE2C1, 0XE3C1, 0X2340, 0X2101, 0XE1C1, 0XE081, 0X2040,
    0X8140, 0X4101, 0X4001, 0X80C1, 0X4201, 0X82C1, 0X83C1, 0X4340, 0X4601, 0X86C1, 0X87C1, 0X4740, 0X4501, 0X85C1, 0X8481, 0X4440,
    0X4C01, 0X8CC1, 0X8DC1, 0X4D40, 0X4F01, 0X8FC1, 0X8E81, 0X4E40, 0X4A01, 0X8AC1, 0X8B81, 0X4B40, 0X4901, 0X89C1, 0X8881, 0X4840,
    0X9801, 0X58C1, 0X59C1, 0X9940, 0X5B01, 0X9BC1, 0X9A81, 0X5A40, 0X5E01, 0X9EC1, 0X9FC1, 0X5F40, 0X5D01, 0X9DC1, 0X9C81, 0X5C40,
    0X5401, 0X94C1, 0X95C1, 0X5540, 0X5701, 0X97C1, 0X9681, 0X5640, 0X5201, 0X92C1, 0X93C1, 0X5340, 0X5101, 0X91C1, 0X9081, 0X5040,
    0X7001, 0XB0C1, 0XB1C1, 0X7140, 0X7301, 0XB3C1, 0XB281, 0X7240, 0X7601, 0XB6C1, 0XB781, 0X7740, 0X7501, 0XB5C1, 0XB481, 0X7440,
    0X7C01, 0XBCC1, 0XBDC1, 0X7D40, 0X7F01, 0XBFC1, 0XBE81, 0X7E40, 0X7A01, 0XBAC1, 0XBB81, 0X7B40, 0X7901, 0XB9C1, 0XB881, 0X7840,
    0X6801, 0XA8C1, 0XA9C1, 0X6940, 0X6B01, 0XABC1, 0XAA81, 0X6A40, 0X6E01, 0XAEC1, 0XAFC1, 0X6F40, 0X6D01, 0XADC1, 0XAC81, 0X6C40,
    0X6401, 0XA4C1, 0XA5C1, 0X6540, 0X6701, 0XA7C1, 0XA681, 0X6640, 0X6201, 0XA2C1, 0XA3C1, 0X6340, 0X6101, 0XA1C1, 0XA081, 0X6040
};

ModbusSlave::ModbusSlave(int rxPin, int txPin, int enPin, long baud) 
    : _rxPin(rxPin), _txPin(txPin), _enPin(enPin), _baud(baud), _dataId(0) {
    memset(_regBank, 0, sizeof(_regBank));
}

void ModbusSlave::begin(uint8_t slaveId) {
    _slaveId = slaveId;
    _charTimeUs = 1000000 * 11 / _baud; // 计算单个字符(11位)传输所需的微秒数
    Serial2.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    
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
        crc = (crc >> 8) ^ pgm_read_word(&crcTable[(crc ^ buf[i]) & 0xFF]);
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
