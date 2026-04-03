#ifndef CONFIG_ID_HANDLER_H
#define CONFIG_ID_HANDLER_H
 
#include "IModeHandler.h"
#include "SlaveApp.h"
 
class ConfigIdHandler : public IModeHandler {
public:
    ConfigIdHandler(SlaveApp* app);
    virtual void enter() override;
    virtual void update() override;
    virtual void exit() override;
 
private:
    SlaveApp* _app;
    uint8_t _tempId;
    unsigned long _lastUIUpdate = 0;
};
 
#endif
