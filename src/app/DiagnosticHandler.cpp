#include "DiagnosticHandler.h"

DiagnosticHandler::DiagnosticHandler(SlaveApp* app) 
    : _app(app), _txCounter(0), _rxByte(0), _rxCount(0), _lastSendTime(0) {}

void DiagnosticHandler::enter() {
    _app->getOled().showMessage("DIAG MODE START", 1000);
}

void DiagnosticHandler::update(ButtonEvent event) {
    handleRawComm();
    handleUI();
}

void DiagnosticHandler::exit() {
    _app->getOled().showMessage("DIAG MODE END", 800);
    // 退出前物理清空缓冲区
    while(_app->getModbus().availableRaw()) {
        _app->getModbus().readRawByte();
    }
}

void DiagnosticHandler::handleRawComm() {
    unsigned long now = millis();
    
    // 定时发送自增字节作为心跳
    if (now - _lastSendTime >= 1000) {
        _lastSendTime = now;
        _txCounter++;
        _app->getModbus().sendRawByte(_txCounter);
    }
    
    // 读取原始字节并统计
    while (_app->getModbus().availableRaw()) {
        _rxByte = _app->getModbus().readRawByte();
        _rxCount = (_rxCount + 1) % 10000;
    }
}

void DiagnosticHandler::handleUI() {
    // 诊断页面专用渲染
    // displayParam 传 txCounter, rxByte 和 rxCount 由 update 协议默认处理
    _app->getOled().update(UI_RS485_DIAG, SLAVE_INIT, 0, 0, _app->getNodeId(), false, 
                          _txCounter, false, _rxByte, _rxCount);
}
