#include "ScaleComponent.h"
#include <Arduino.h>

ScaleComponent::ScaleComponent(int dtPin, int sckPin) 
    : _dtPin(dtPin), _sckPin(sckPin), scaleFactor(1.0), offset(0) {}

void ScaleComponent::begin() {
    scale.begin(_dtPin, _sckPin);
    loadCalibration();
    scale.tare();
}

float ScaleComponent::getWeight(int samples) {
    if (scale.is_ready()) {
        return scale.get_units(samples);
    }
    return 0.0;
}

void ScaleComponent::tare() {
    scale.tare();
    offset = scale.get_offset();
    saveCalibration();
}

void ScaleComponent::calibrate(int knownWeight) {
    if (knownWeight <= 0) return;
    long raw = scale.read_average(10);
    offset = scale.get_offset();
    
    long diff = raw - offset;
    // 安全防御：防止用户在放上砝码后不小心去皮，导致差值为 0，进而算出 0 或 NaN 系数
    if (abs(diff) < 100) {
        diff = 100; // 给定一个非0保护值
    }
    
    scaleFactor = (float)diff / (float)knownWeight;
    if (abs(scaleFactor) < 0.001) scaleFactor = 1.0; // 最终防御
    
    scale.set_scale(scaleFactor);
    saveCalibration();
}

void ScaleComponent::setScale(float factor) {
    if (factor == 0) return;
    scaleFactor = factor;
    scale.set_scale(scaleFactor);
    saveCalibration();
}

void ScaleComponent::saveCalibration() {
    preferences.begin("scale", false);
    preferences.putFloat("factor", scaleFactor);
    preferences.putLong("offset", offset);
    preferences.end();
    Serial.println("Scale: Calibration saved.");
}

void ScaleComponent::loadCalibration() {
    preferences.begin("scale", true);
    scaleFactor = preferences.getFloat("factor", 1.0);
    offset = preferences.getLong("offset", 0);
    preferences.end();

    scale.set_scale(scaleFactor);
    scale.set_offset(offset);
    Serial.printf("Scale: Loaded Factor: %.2f, Offset: %ld\n", scaleFactor, offset);
}
