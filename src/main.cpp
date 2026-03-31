#include <Arduino.h>
#include "WeighingScale.h"
#include "TinyScreen.h"
#include "ModbusSlave.h"
#include "MainController.h"

// ---------------------------
// Pin & Configuration
// ---------------------------
#define HX711_DT_PIN 19
#define HX711_SCK_PIN 18

#define I2C_SDA_PIN 27
#define I2C_SCL_PIN 26
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define RS485_RX_PIN 22
#define RS485_TX_PIN 23
#define RS485_EN_PIN 21
#define RS485_BAUD 9600

#define SERVO_PIN 16
#define BUTTON_PIN 25

// ---------------------------
// Global Objects
// ---------------------------
WeighingScale scale(HX711_DT_PIN, HX711_SCK_PIN);
TinyScreen oled(SCREEN_WIDTH, SCREEN_HEIGHT, I2C_SDA_PIN, I2C_SCL_PIN);
ModbusSlave modbus(RS485_RX_PIN, RS485_TX_PIN, RS485_EN_PIN, RS485_BAUD);
MainController controller(scale, oled, modbus, BUTTON_PIN, SERVO_PIN);

// ---------------------------
// Setup & Loop
// ---------------------------
void setup() {
    Serial.begin(115200);
    controller.begin();
}

void loop() {
    controller.loop();
}
