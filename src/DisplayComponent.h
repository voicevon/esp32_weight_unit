#ifndef DISPLAY_COMPONENT_H
#define DISPLAY_COMPONENT_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "SystemTypes.h"

class DisplayComponent {
public:
    DisplayComponent(int width, int height, int sda, int scl);
    void begin();
    
    // 增强版更新函数：增加 rawADC 支持与最近字节 HUD
    void update(SystemState state, float weight, int32_t rawADC, int currentId, 
                bool commPulse, int displayParam, uint32_t rxCount, uint8_t lastByte,
                bool stable = false, uint8_t doorPhase = 0);
    
    void showMessage(const char* msg, int duration = 500);
    void showLargeMessage(const char* msg, int duration = 500);
    
    void updateViewCalib(float factor, long offset);

private:
    Adafruit_SSD1306 display;
    int _width, _height, _sda, _scl;

    // 内部私有绘图模块
    void drawHeader(int id, bool commActive, uint32_t rxCount, uint8_t lastByte);
    void drawPageRun(float weight, SystemState state, int32_t rawADC, bool stable, uint8_t doorPhase);
    void drawPageConfig(int id);
    void drawPageConfigZTR(int ztr);
    void drawPageCalibrate(SystemState state, int calWeight, int32_t rawADC);
    void drawPageCommTest(int txCount, String rxData);
    void drawPageRS485Diag(uint8_t tx, uint8_t rx);
    void drawPageVersion();
};

#endif
