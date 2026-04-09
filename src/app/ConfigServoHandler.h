#ifndef CONFIG_SERVO_HANDLER_H
#define CONFIG_SERVO_HANDLER_H
 
#include "IModeHandler.h"
#include "SlaveApp.h"
 
class ConfigServoHandler : public IModeHandler {
public:
    ConfigServoHandler(SlaveApp* app);
    virtual void enter() override;
    virtual bool update(ButtonEvent event) override;
    virtual void exit() override;
 
private:
    SlaveApp* _app;
    unsigned long _lastUIUpdate = 0;
};
 
#endif
