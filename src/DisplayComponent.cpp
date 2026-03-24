#include "DisplayComponent.h"
#include <Wire.h>

static String formatWithCommas(int32_t value) {
    String numStr = String(value);
    int insertPosition = numStr.length() - 3;
    while (insertPosition > (value < 0 ? 1 : 0)) {
        numStr = numStr.substring(0, insertPosition) + "," + numStr.substring(insertPosition);
        insertPosition -= 3;
    }
    return numStr;
}

DisplayComponent::DisplayComponent(int width, int height, int sda, int scl)
    : _width(width), _height(height), _sda(sda), _scl(scl), 
      display(width, height, &Wire, -1) {}

void DisplayComponent::begin() {
    Wire.begin(_sda, _scl);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Display: SSD1306 Init Failed"));
    } else {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        display.setCursor(0,0);
        display.println("Feng's Scale OS");
        display.println("Initializing...");
        display.display();
    }
}

void DisplayComponent::update(SystemState state, float weight, int32_t rawADC, int currentId, bool commPulse, int calWeight) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // 1. 始终绘制顶部状态栏
    drawHeader(currentId, commPulse);

    // 2. 根据状态分发绘制页面内容
    switch (state) {
        case STATE_RUN:
            drawPageRun(weight, state, rawADC);
            break;
        case STATE_MENU_SELECT_WEIGHT:
            drawPageCalibrate(state, calWeight, rawADC);
            break;
        case STATE_CALIBRATING:
            drawPageCalibrate(state, calWeight, rawADC);
            break;
        case STATE_CONFIG_ID:
            drawPageConfig(currentId);
            break;
        case STATE_CONFIG_ZTR:
            drawPageConfigZTR(calWeight); // 利用 calWeight 参数传递 ztr
            break;
        case STATE_COMM_TEST:
            drawPageCommTest(0, commPulse ? "Active" : "Idle"); // 临时兼容
            break;
        default:
            break;
    }

    display.display();
}

void DisplayComponent::drawHeader(int id, bool commActive) {
    // 将 ID 移到右上角，改为 # 符号，恢复为 TextSize(1)
    display.setTextSize(1);
    display.setCursor(104, 0);
    display.print("#");
    if (id < 10) display.print("0");
    display.print(id);

    if (commActive) {
        display.print("*");
    }
}

void DisplayComponent::drawPageRun(float weight, SystemState state, int32_t rawADC) {
    // 格式化重量，只保留整数部分，向右对齐至 X=76
    char weightStr[16];
    sprintf(weightStr, "%4.0f", weight);
    int wWidth = strlen(weightStr) * 12; // Size 2
    int wStartX = 76 - wWidth;
    if (wStartX < 0) wStartX = 0;

    display.setTextSize(2);
    display.setCursor(wStartX, 2); // 整体向上移顶至 Y=2
    display.print(weightStr);

    // 左下角显示原始采样 AD 值（去掉 AD: 前缀，右对齐到 X=76，恢复小号字体）
    display.setTextSize(1);
    String adStr = formatWithCommas(rawADC);
    int textWidth = adStr.length() * 6; // Size 1 font width is 6px per char
    int startX = 76 - textWidth;
    if (startX < 0) startX = 0;
    display.setCursor(startX, 24);
    display.print(adStr);

    // 已根据要求移除右下方的 HLD:TARE 提示，保持界面极简
}

void DisplayComponent::drawPageConfig(int id) {
    display.setCursor(0, 12);
    display.print("CHANGE ID > ");
    display.setTextSize(2);
    display.print(id);

    // 已按照要求移除底部的操作提示，保持清爽
}

void DisplayComponent::drawPageConfigZTR(int ztr) {
    display.setCursor(0, 12);
    display.print("ZERO TRACK BAND>");
    display.setTextSize(2);
    if (ztr == 0) {
        display.print("OFF");
    } else {
        display.print(ztr);
        display.print("g");
    }
}

void DisplayComponent::drawPageCalibrate(SystemState state, int calWeight, int32_t rawADC) {
    if (state == STATE_MENU_SELECT_WEIGHT) {
        display.setCursor(0, 0); // 往上移以便塞下AD值
        display.print("CALIBRATE WT:");
        display.setCursor(20, 12);
        display.print("-> ");
        display.setTextSize(1); 
        if (calWeight == 0) {
            display.print("EXIT");
        } else {
            display.print(calWeight);
            display.print(" g");
        }
        
        // 左下角显示 AD 值
        String adStr = formatWithCommas(rawADC);
        int textWidth = adStr.length() * 6;
        int startX = 76 - textWidth;
        if (startX < 0) startX = 0;
        display.setCursor(startX, 24);
        display.print(adStr);
        
    } else if (state == STATE_CALIBRATING) {
        display.setCursor(10, 10);
        display.print("CALIBRATING...");
        
        // 同时保留显示底部的 AD
        String adStr = formatWithCommas(rawADC);
        int textWidth = adStr.length() * 6;
        int startX = 76 - textWidth;
        if (startX < 0) startX = 0;
        display.setCursor(startX, 24);
        display.print(adStr);
    }
}

void DisplayComponent::updateViewCalib(float factor, long offset) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    // 顶部标题
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("CALIB PARAMS [HOLD>]");
    
    // 显示斜率 K (Scale Factor)
    display.setCursor(0, 12);
    display.print("Slope:");
    display.print(factor, 4);
    
    // 显示偏移量 B (Offset)
    display.setCursor(0, 24);
    display.print("Offset:");
    display.print(offset);
    
    display.display();
}

void DisplayComponent::drawPageCommTest(int txCount, String rxData) {
    display.setCursor(0, 12);
    display.print("RS485 DIAG: OK");
    display.setCursor(0, 22);
    display.print("DATA: ");
    display.print(rxData);
}

void DisplayComponent::showMessage(const char* msg, int duration) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(30, 12);
    display.print(msg);
    display.display();
    if (duration > 0) delay(duration);
}

void DisplayComponent::showLargeMessage(const char* msg, int duration) {
    display.clearDisplay();
    display.setTextSize(3); // 每个大写字母宽度大约是 5*3=15，算上间距总计18
    int textWidth = strlen(msg) * 18;
    int startX = (128 - textWidth) / 2;
    if (startX < 0) startX = 0;
    
    display.setCursor(startX, 6);
    display.print(msg);
    display.display();
    if (duration > 0) delay(duration);
}
