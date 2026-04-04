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
        case UI_CONFIG_SERVO:
            drawPageConfigServo(displayParam != 0);
            break;
        case UI_MENU_CALIB:
            drawPageCalibrate(state, displayParam, rawADC);
            break;
        case UI_CALIB_RESULT:
            drawPageCalibResult(weight, (long)rawADC);
            break;
        case UI_VERSION:
            drawPageVersion();
            break;
        case UI_RS485_DIAG:
            drawPageRS485Diag(displayParam, rxByte, rxCount);
            break;
    }

    display.display();
}

void TinyScreen::drawHeader(int id, bool commActive) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("#");
    if (id < 10) display.print("0");
    display.print(id);
    if (commActive) display.print("*");
}

void TinyScreen::drawPageRun(float weight, SlaveState state, int32_t rawADC, bool stable,
                             uint8_t rxByte, uint16_t rxCount) {
    // 1. (已移除) 顶部状态文字

    // 2. 核心重量显示 (继续向上平移，并改为整数显示)
    display.setTextSize(2);
    display.setCursor(10, 3);
    display.printf("%6d g", (int)round(weight));

    // 3. 底部调试信息 (AD - 向下微移)
    display.setTextSize(1);
    
    // ADC 格式化 (千分位)
    char rawStr[16];
    char adBuf[32];
    snprintf(rawStr, sizeof(rawStr), "%ld", rawADC);
    
    int len = strlen(rawStr);
    int startIdx = (rawADC < 0) ? 1 : 0;
    int destIdx = 0;
    if (startIdx == 1) adBuf[destIdx++] = '-';
    
    int digits = len - startIdx;
    for (int i = 0; i < digits; i++) {
        adBuf[destIdx++] = rawStr[startIdx + i];
        int remaining = digits - 1 - i;
        if (remaining > 0 && remaining % 3 == 0) {
            adBuf[destIdx++] = ',';
        }
    }
    adBuf[destIdx] = '\0';
    
    // 显示 AD (底部紧贴)
    display.setCursor(0, 25);
    display.printf("A:%s", adBuf);

    // 4. 稳定标志 (移至右下角，高度对齐)
    if (stable) {
        display.fillRect(110, 24, 18, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(112, 25);
        display.print("ST");
        display.setTextColor(SSD1306_WHITE);
    }
}

void TinyScreen::drawPageConfig(int id) {
    display.setTextSize(1);
    display.setCursor(30, 0);
    display.print("SET SLAVE ID:");
    
    display.setTextSize(2);
    display.setCursor(45, 12);
    if (id < 10) display.print("0");
    display.print(id);
}

void TinyScreen::drawPageConfigZTR(int ztr, float currentWeight) {
    display.setTextSize(1);
    display.setCursor(30, 0);
    display.print("ZERO RANGE");
    
    // 右上角实时重量参考
    display.setCursor(100, 0);
    display.printf("%.1f", currentWeight);
    
    // 主数值居中显示
    display.setTextSize(2);
    display.setCursor(30, 14);
    if (ztr == 0) display.print("OFF");
    else display.printf("%d g", ztr);
}

void TinyScreen::drawPageConfigServo(bool isOpen) {
    display.setTextSize(1);
    display.setCursor(30, 0);
    display.print("SERVO TEST");
    
    display.setTextSize(2);
    display.setCursor(30, 14);
    if (isOpen) display.print("OPEN");
    else display.print("CLOSE");
}

void TinyScreen::drawPageCalibrate(SlaveState state, int calWeight, int32_t rawADC) {
    display.setCursor(0, 0);
    display.print("CALIBRATION");
    display.setCursor(0, 15);
    if (calWeight == -1) display.print("-> EXIT");
    else display.printf("-> %d g", calWeight);
    
    // Fixed coordinate: y=24 instead of 40 (screen is only 32px high)
    display.setCursor(0, 24);
    display.printf("AD: %ld", rawADC);
}

void TinyScreen::drawPageCalibResult(float k, long b) {
    display.setCursor(0, 0);
    display.print("CALIB DONE! [OK]");
    display.setCursor(0, 15);
    display.printf("K: %.4f", k);
    display.setCursor(0, 24);
    display.printf("B: %ld", b);
}

void TinyScreen::drawPageVersion() {
    display.setTextSize(2);
    display.setCursor(20, 24);
    display.print("VER 2.5.REF");
}

void TinyScreen::drawPageRS485Diag(int tx, uint8_t rx, uint16_t count) {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("485 DIAGNOSTIC");
    
    display.setTextSize(2);
    display.setCursor(0, 14);
    display.printf("T:%02X R:%02X", tx, rx);
    
    display.setTextSize(1);
    display.setCursor(85, 24);
    display.printf("#%04d", count);
}

void TinyScreen::showMessage(const char* msg, int duration) {
    display.clearDisplay();
    display.setTextSize(1);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((128 - w) / 2, (32 - h) / 2);
    display.print(msg);
    display.display();
    if (duration > 0) delay(duration);
}

void TinyScreen::showLargeMessage(const char* msg, int duration) {
    display.clearDisplay();
    display.setTextSize(2);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
    
    // 如果大号字体太宽，自动降级为小号字体
    if (w > 124) {
        display.setTextSize(1);
        display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
    }
    
    display.setCursor((128 - w) / 2, (32 - h) / 2);
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
