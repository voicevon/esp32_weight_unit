#ifndef DISPLAY_COMPONENT_H
#define DISPLAY_COMPONENT_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "SystemTypes.h"

class DisplayComponent {
public:
    DisplayComponent(int width, int height, int sda, int scl);
    void begin();
    
    // 渲染主函数
    void update(UIMode mode, SlaveState state, float weight, int32_t rawADC, 
                int currentId, bool commPulse, int displayParam, bool stable);
    
    void showMessage(const char* msg, int duration = 500);
    void showLargeMessage(const char* msg, int duration = 500);
    
    void updateViewCalib(float factor, long offset);

private:
    Adafruit_SSD1306 display;
    int _width, _height, _sda, _scl;

    // 内部绘制子模块
    void drawHeader(int id, bool commActive);
    void drawPageRun(float weight, SlaveState state, int32_t rawADC, bool stable);
    void drawPageConfig(int id);
    void drawPageConfigZTR(int ztr);
    void drawPageCalibrate(SlaveState state, int calWeight, int32_t rawADC);
    void drawPageVersion();
    void drawPageRS485Diag(int tx, int rx);
};

#endif
