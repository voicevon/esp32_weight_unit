# ESP32 Weighing Scale - Implementation Walkthrough

## Overview
This project implements a smart weighing scale using ESP32, capable of local weight calculation, display, and RS485 communication.

## Features Implemented
1.  **Core Components**:
    - **HX711**: Reads load cell data.
    - **SSD1306 OLED**: Displays weight and status.
    - **Servo**: Acts as an actuator (Open/Close).
    - **RS485**: Modbus-like ASCII communication.


2.  **Distributed Architecture**:
    - Weight calculation happens locally on the ESP32 via manual calibration logic (using raw ADC values).
    - Zero (Offset) and Scale Factor are stored in NVS (`Preferences`), persisting across reboots.

3.  **Physical Interface**:
    - **Button (GPIO 15)**:
        - **Short Press**: Tare.
        - **Long Press**: Enter Calibration Menu (Select 100g/200g/500g/1000g).

## Verification Results

### Compilation
The firmware compiles successfully using PlatformIO.
`pio run` -> SUCCESS

### Usage Guide
1.  **Power On**: System initializes, loads calibration from flash.
2.  **Tare**: Short press button or send `TARE` via RS485.
3.  **Calibrate**:
    - Long press button to enter menu.
    - Short press to cycle target weight.
    - Place weight.
    - Long press to confirm.
4.  **Remote Control**:
    - Send `OPEN`/`CLOSE` to control servo.
    - Send `GET_WEIGHT` to read weight.

## Artifacts
- [Requirement Description](doc/需求描述.md)
- [Wiring Diagram](doc/硬件接线图.md)
- [Calibration Strategy](doc/标定方案对比分析.md)
