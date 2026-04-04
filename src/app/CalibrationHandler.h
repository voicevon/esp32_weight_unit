#ifndef CALIB_HANDLER_H
#define CALIB_HANDLER_H

#include "IModeHandler.h"
#include "SlaveApp.h"

/**
 * @class CalibrationHandler
 * @brief 标定模式处理器：负责称重传感器的零位/量程标定流程。
 */
class CalibrationHandler : public IModeHandler {
public:
    CalibrationHandler(SlaveApp* app);
    virtual void enter() override;
    virtual void update(ButtonEvent event) override;
    virtual void exit() override;

private:
    SlaveApp* _app;
    unsigned long _timer;
    int _calWeightIndex = 0;
    const int _calWeights[5] = {0, 100, 200, 500, 1000};
    
    void handleUI(ButtonEvent event);
    void handleCalib(ButtonEvent event);
    void handleSampling();
};

#endif
