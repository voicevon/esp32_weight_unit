#ifndef PRODUCTION_HANDLER_H
#define PRODUCTION_HANDLER_H

#include "IModeHandler.h"
#include "SlaveApp.h"

/**
 * @class ProductionHandler
 * @brief 生产模式处理器：负责正常的称重轮询、喂料/出料逻辑及重量同步。
 */
class ProductionHandler : public IModeHandler {
public:
    ProductionHandler(SlaveApp* app);
    virtual void enter() override;
    virtual bool update(ButtonEvent event) override;
    virtual void exit() override;

private:
    SlaveApp* _app;
    SlaveState _state;
    unsigned long _stateTimer;
    bool _pendingTare;
    unsigned long _lastUIUpdateMillis = 0;

    void handleSampling();
    void handleLogic();
    void handleComm();
    void handleUI();
    void updateState(SlaveState next);
};

#endif
