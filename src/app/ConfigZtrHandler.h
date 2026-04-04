#ifndef CONFIG_ZTR_HANDLER_H
#define CONFIG_ZTR_HANDLER_H

#include "IModeHandler.h"
#include "SlaveApp.h"

class ConfigZtrHandler : public IModeHandler {
public:
    ConfigZtrHandler(SlaveApp* app);
    virtual void enter() override;
    virtual void update(ButtonEvent event) override;
    virtual void exit() override;

private:
    SlaveApp* _app;
    uint16_t _tempZtrValue;
    unsigned long _lastUIUpdate = 0;
};

#endif
