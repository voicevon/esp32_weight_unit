#ifndef DIAG_HANDLER_H
#define DIAG_HANDLER_H

#include "IModeHandler.h"
#include "SlaveApp.h"

/**
 * @class DiagnosticHandler
 * @brief 诊断模式处理器：负责 RS485 链路的原始字节收发诊断。
 */
class DiagnosticHandler : public IModeHandler {
public:
    DiagnosticHandler(SlaveApp* app);
    virtual void enter() override;
    virtual void update() override;
    virtual void exit() override;

private:
    SlaveApp* _app;
    uint8_t _txCounter;
    uint8_t _rxByte;
    uint16_t _rxCount;
    unsigned long _lastSendTime;
    
    void handleRawComm();
    void handleUI();
};

#endif
