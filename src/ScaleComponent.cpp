#include "ScaleComponent.h"
#include <Arduino.h>

ScaleComponent::ScaleComponent(int dtPin, int sckPin) 
    : _dtPin(dtPin), _sckPin(sckPin), scaleFactor(1.0), offset(0), 
      historyIndex(0), historyFull(false) {
    for (int i = 0; i < HISTORY_SIZE; i++) weightHistory[i] = 0.0f;
}

void ScaleComponent::begin() {
    scale.begin(_dtPin, _sckPin);
    loadCalibration();
    
    unsigned long st = millis();
    while(!scale.is_ready() && millis() - st < 2000) {
        delay(10);
    }
    if (scale.is_ready()) {
        scale.tare();
    } else {
        Serial.println("Scale: HX711 not ready, skip tare() on boot");
    }
}

float ScaleComponent::getWeight(int samples) {
    if (scale.is_ready()) {
        float w = scale.get_units(samples);
        
        // Update history
        weightHistory[historyIndex] = w;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;
        if (historyIndex == 0) historyFull = true;
        
        return w;
    }
    return 0.0;
}

bool ScaleComponent::isStable(float threshold) {
    if (!historyFull && historyIndex < 5) return false; // Not enough data
    
    int count = historyFull ? HISTORY_SIZE : historyIndex;
    float minW = weightHistory[0];
    float maxW = weightHistory[0];
    
    for (int i = 1; i < count; i++) {
        if (weightHistory[i] < minW) minW = weightHistory[i];
        if (weightHistory[i] > maxW) maxW = weightHistory[i];
    }
    
    return (maxW - minW) <= threshold;
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
