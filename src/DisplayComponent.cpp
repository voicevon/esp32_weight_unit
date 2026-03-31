#include "DisplayComponent.h"
#include <Wire.h>

DisplayComponent::DisplayComponent(int width, int height, int sda, int scl)
    : _width(width), _height(height), _sda(sda), _scl(scl), 
      display(width, height, &Wire, -1) {}

void DisplayComponent::begin() {
    Wire.begin(_sda, _scl);
    Wire.setClock(400000);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Display: SSD1306 Init Failed"));
    } else {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.display();
    }
}

void DisplayComponent::update(UIMode mode, SlaveState state, float weight, int32_t rawADC, 
                              int currentId, bool commPulse, int displayParam, bool stable) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    drawHeader(currentId, commPulse);

    switch (mode) {
        case UI_RUN:
            drawPageRun(weight, state, rawADC, stable);
            break;
        case UI_CONFIG_ID:
            drawPageConfig(currentId);
            break;
        case UI_CONFIG_ZTR:
            drawPageConfigZTR(displayParam);
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

void DisplayComponent::drawHeader(int id, bool commActive) {
    display.setTextSize(1);
    display.setCursor(104, 0);
    display.print("#");
    if (id < 10) display.print("0");
    display.print(id);
    if (commActive) display.print("*");
}

void DisplayComponent::drawPageRun(float weight, SlaveState state, int32_t rawADC, bool stable) {
    // 稳定性指示
    if (stable) {
        display.fillRect(92, 23, 36, 9, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(93, 24);
        display.print("STABLE");
        display.setTextColor(SSD1306_WHITE);
    }

    // 重量显示
    display.setTextSize(2);
    display.setCursor(0, 4);
    display.printf("%6.1f g", weight);

    // 状态显示
    display.setTextSize(1);
    display.setCursor(0, 24);
    switch(state) {
        case SLAVE_STANDBY: display.print("IDLE"); break;
        case SLAVE_LOCKED: display.print("LOCKED"); break;
        case SLAVE_DISCHARGING: display.print("OPENING"); break;
        case SLAVE_RECOVERY: display.print("WAIT.."); break;
        default: display.print("BUSY"); break;
    }

    // AD 原值
    display.setCursor(0, 48);
    display.printf("AD: %ld", rawADC);
}

void DisplayComponent::drawPageConfig(int id) {
    display.setCursor(0, 16);
    display.print("SET SLAVE ID:");
    display.setTextSize(2);
    display.setCursor(40, 32);
    display.print(id);
}

void DisplayComponent::drawPageConfigZTR(int ztr) {
    display.setCursor(0, 16);
    display.print("ZERO TRACKING:");
    display.setTextSize(2);
    display.setCursor(40, 32);
    if (ztr == 0) display.print("OFF");
    else display.printf("%d g", ztr);
}

void DisplayComponent::drawPageCalibrate(SlaveState state, int calWeight, int32_t rawADC) {
    display.setCursor(0, 0);
    display.print("CALIBRATION");
    display.setCursor(0, 16);
    if (calWeight == 0) display.print("-> EXIT");
    else display.printf("-> %d g", calWeight);
    
    display.setCursor(0, 40);
    display.printf("AD: %ld", rawADC);
}

void DisplayComponent::drawPageVersion() {
    display.setTextSize(2);
    display.setCursor(20, 24);
    display.print("VER 2.5.REF");
}

void DisplayComponent::drawPageRS485Diag(int tx, int rx) {
    display.setCursor(0, 16);
    display.print("485 DIAG");
    display.printf("\nTX: %02X  RX: %02X", tx, rx);
}

void DisplayComponent::showMessage(const char* msg, int duration) {
    display.clearDisplay();
    display.setCursor(20, 24);
    display.print(msg);
    display.display();
    if (duration > 0) delay(duration);
}

void DisplayComponent::showLargeMessage(const char* msg, int duration) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 24);
    display.print(msg);
    display.display();
    if (duration > 0) delay(duration);
}

void DisplayComponent::updateViewCalib(float factor, long offset) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("K: %.4f\nB: %ld", factor, offset);
    display.display();
}
