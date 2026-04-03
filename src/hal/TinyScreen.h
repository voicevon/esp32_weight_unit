#ifndef TINY_SCREEN_H
#define TINY_SCREEN_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../common/SystemTypes.h"

class TinyScreen {
public:
    TinyScreen(int width, int height, int sda, int scl);
    void begin();
    
    void update(UIMode mode, SlaveState state, float weight, int32_t rawADC, 
                int currentId, bool commPulse, int displayParam, bool stable, 
                uint8_t rxByte = 0, uint16_t rxCount = 0);
    
    void showMessage(const char* msg, int duration = 500);
    void showLargeMessage(const char* msg, int duration = 500);
    void updateViewCalib(float factor, long offset);

private:
     Adafruit_SSD1306 display;
    int _width, _height, _sda, _scl;

    void drawHeader(int id, bool commActive);
    void drawPageRun(float weight, SlaveState state, int32_t rawADC, bool stable,
                     uint8_t rxByte = 0, uint16_t rxCount = 0);
    void drawPageConfig(int id);
    void drawPageConfigZTR(int ztr, float currentWeight);
    void drawPageConfigServo(bool enabled);
    void drawPageCalibrate(SlaveState state, int calWeight, int32_t rawADC);
    void drawPageCalibResult(float k, long b);
    void drawPageVersion();
    void drawPageRS485Diag(int tx, uint8_t rx, uint16_t count);
};

#endif
