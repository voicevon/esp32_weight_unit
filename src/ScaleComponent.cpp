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
        updateHistory(w);
        return w;
    }
    return 0.0;
}

float ScaleComponent::calculateWeight(long raw) {
    float factor = scaleFactor;
    if (factor == 0) factor = 1.0; // avoid div by zero
    return (float)(raw - offset) / factor;
}

void ScaleComponent::updateHistory(float w) {
    weightHistory[historyIndex] = w;
    historyIndex = (historyIndex + 1) % HISTORY_SIZE;
    if (historyIndex == 0) historyFull = true;
}

bool ScaleComponent::isStable(float threshold) {
    // 移除实时 is_ready() 判断，仅仅根据历史数组进行极差运算
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 1000) {
        lastLog = millis();
        if (!historyFull && historyIndex < 5) {
            Serial.printf("[STB_DIAG] Waiting for history... (%d/%d)\n", historyIndex, 5);
        }
    }

    if (!historyFull && historyIndex < 5) return false; 
    
    int count = historyFull ? HISTORY_SIZE : historyIndex;
    float minW = weightHistory[0];
    float maxW = weightHistory[0];
    
    for (int i = 1; i < count; i++) {
        if (weightHistory[i] < minW) minW = weightHistory[i];
        if (weightHistory[i] > maxW) maxW = weightHistory[i];
    }
    
    float diff = maxW - minW;
    bool stable = diff <= threshold;

    if (millis() - lastLog <= 10) { // 如果上面刚好一秒，这里复用
        Serial.printf("[STB_DIAG] Min:%.2f Max:%.2f Diff:%.2f (Thresh:%.2f) %s\n", 
                      minW, maxW, diff, threshold, stable ? "STABLE" : "UNSTABLE");
    }
    
    return stable;
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
