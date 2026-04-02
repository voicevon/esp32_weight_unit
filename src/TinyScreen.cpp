#include "TinyScreen.h"
#include <Wire.h>

TinyScreen::TinyScreen(int width, int height, int sda, int scl)
    : _width(width), _height(height), _sda(sda), _scl(scl), 
      display(width, height, &Wire, -1) {}

void TinyScreen::begin() {
    Wire.begin(_sda, _scl);
    Wire.setClock(400000);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("TinyScreen: Init Failed"));
    } else {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.display();
    }
}

void TinyScreen::update(UIMode mode, SlaveState state, float weight, int32_t rawADC, 
                        int currentId, bool commPulse, int displayParam, bool stable,
                        uint8_t rxByte, uint16_t rxCount) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    drawHeader(currentId, commPulse);

    switch (mode) {
        case UI_RUN:
            drawPageRun(weight, state, rawADC, stable, rxByte, rxCount);
            break;
        case UI_CONFIG_ID:
            drawPageConfig(currentId);
            break;
        case UI_CONFIG_ZTR:
            drawPageConfigZTR(displayParam, weight);
            break;
        case UI_MENU_CALIB:
            drawPageCalibrate(state, displayParam, rawADC);
            break;
        case UI_VERSION:
            drawPageVersion();
            break;
        case UI_RS485_DIAG:
            drawPageRS485Diag(displayParam >> 8, displayParam & 0xFF);
            break;
    }

    display.display();
}

void TinyScreen::drawHeader(int id, bool commActive) {
    display.setTextSize(1);
    display.setCursor(104, 0);
    display.print("#");
    if (id < 10) display.print("0");
    display.print(id);
    if (commActive) display.print("*");
}

void TinyScreen::drawPageRun(float weight, SlaveState state, int32_t rawADC, bool stable,
                             uint8_t rxByte, uint16_t rxCount) {
    // 1. 状态文字移至左上角 (0, 0)
    display.setTextSize(1);
    display.setCursor(0, 0);
    switch(state) {
        case SLAVE_STANDBY: display.print("IDLE"); break;
        case SLAVE_LOCKED: display.print("LOCKED"); break;
        case SLAVE_DISCHARGING: display.print("OPENING"); break;
        case SLAVE_RECOVERY: display.print("WAIT.."); break;
        case SLAVE_CALIBRATING: display.print("CALIB"); break;
        default: display.print("BUSY"); break;
    }

    // 2. 稳定标志
    if (stable) {
        display.fillRect(80, 0, 20, 9, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(81, 1);
        display.print("ST");
        display.setTextColor(SSD1306_WHITE);
    }

    // 3. 核心重量显示 (居中)
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.printf("%6.1f g", weight);

    // 4. 底部调试信息 (AD + RX)
    display.setTextSize(1);
    
    // ADC 格式化 (千分位)
    char adBuf[24];
    String s = String(rawADC);
    int len = s.length();
    int start = (rawADC < 0) ? 1 : 0;
    String formattedAd = "";
    int count = 0;
    for (int i = len - 1; i >= start; i--) {
        formattedAd = s[i] + formattedAd;
        count++;
        if (count % 3 == 0 && i > start) {
            formattedAd = "," + formattedAd;
        }
    }
    if (start == 1) formattedAd = "-" + formattedAd;
    
    // 显示 AD
    display.setCursor(0, 24);
    display.printf("A:%s", formattedAd.c_str());

    // 5. 链路层接收信息 RX [Byte] [Count]
    display.setCursor(85, 24);
    display.printf("R:%02X %03d", rxByte, rxCount % 1000);
}

void TinyScreen::drawPageConfig(int id) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("SET SLAVE ID:");
    
    display.setTextSize(2);
    display.setCursor(45, 12);
    if (id < 10) display.print("0");
    display.print(id);
}

void TinyScreen::drawPageConfigZTR(int ztr, float currentWeight) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("ZERO TRACK");
    
    // 中间显示实时重量，方便观察
    display.setCursor(70, 0);
    display.printf("W:%4.1fg", currentWeight);
    
    display.setTextSize(2);
    display.setCursor(35, 14);
    if (ztr == 0) display.print("OFF");
    else display.printf("%d g", ztr);
}

void TinyScreen::drawPageCalibrate(SlaveState state, int calWeight, int32_t rawADC) {
    display.setCursor(0, 0);
    display.print("CALIBRATION");
    display.setCursor(0, 16);
    if (calWeight == 0) display.print("-> EXIT");
    else display.printf("-> %d g", calWeight);
    
    display.setCursor(0, 40);
    display.printf("AD: %ld", rawADC);
}

void TinyScreen::drawPageVersion() {
    display.setTextSize(2);
    display.setCursor(20, 24);
    display.print("VER 2.5.REF");
}

void TinyScreen::drawPageRS485Diag(int tx, int rx) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("485 DIAGNOSTIC");
    
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.printf("T:%02X R:%02X", tx, rx);
}

void TinyScreen::showMessage(const char* msg, int duration) {
    display.clearDisplay();
    display.setCursor(20, 24);
    display.print(msg);
    display.display();
    if (duration > 0) delay(duration);
}

void TinyScreen::showLargeMessage(const char* msg, int duration) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 24);
    display.print(msg);
    display.display();
    if (duration > 0) delay(duration);
}

void TinyScreen::updateViewCalib(float factor, long offset) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("K: %.4f\nB: %ld", factor, offset);
    display.display();
}
