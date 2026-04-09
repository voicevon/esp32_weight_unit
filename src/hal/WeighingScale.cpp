#include "WeighingScale.h"
#include <Arduino.h>

WeighingScale::WeighingScale(int dtPin, int sckPin) 
    : _dtPin(dtPin), _sckPin(sckPin), _scaleFactor(1.0), _offset(0), _lastRaw(0),
      _currentWeight(0.0), _filteredWeight(0.0), _emaAlpha(0.3),
      _historyIndex(0), _historyFull(false), _ztrThreshold(2), _lastZeroStableTime(0) {
    for (int i = 0; i < HISTORY_SIZE; i++) _weightHistory[i] = 0.0f;
}

void WeighingScale::begin() {
    pinMode(_dtPin, INPUT);
    pinMode(_sckPin, OUTPUT);
    _scale.begin(_dtPin, _sckPin);
    
    loadCalibration();
    
    unsigned long st = millis();
    while(!_scale.is_ready() && millis() - st < 1000) {
        delay(5);
    }
}

void WeighingScale::update(long raw) {
    _lastRaw = raw; // 缓存最新读数
    float factor = _scaleFactor;
    if (abs(factor) < 0.001) factor = 1.0;
    _currentWeight = (float)(raw - _offset) / factor;
    _filteredWeight = (_emaAlpha * _currentWeight) + ((1.0f - _emaAlpha) * _filteredWeight);
    _weightHistory[_historyIndex] = _filteredWeight;
    _historyIndex = (_historyIndex + 1) % HISTORY_SIZE;
    if (_historyIndex == 0) _historyFull = true;

    // --- 零点自动追踪核心逻辑 ---
    if (_ztrThreshold > 0 && abs(_filteredWeight) <= _ztrThreshold) {
        if (isStable(1.0f)) { // 零点稳定阈值固定为 1.0g
            if (millis() - _lastZeroStableTime > 2000) { // 持续稳定 2 秒
                long rawShift = (long)(_filteredWeight * factor); 
                _offset += rawShift;
                _filteredWeight = 0; // 瞬间归零
                _lastZeroStableTime = millis();
                Serial.printf("[ZTR] Auto tracking zero. Shift: %ld, New offset: %ld\n", rawShift, _offset);
            }
        } else {
            _lastZeroStableTime = millis();
        }
    } else {
        _lastZeroStableTime = millis();
    }
}

bool WeighingScale::isStable(float threshold) {
    int count = _historyFull ? HISTORY_SIZE : _historyIndex;
    if (count < 5) return false; 
    
    float minW = _weightHistory[0];
    float maxW = _weightHistory[0];
    
    for (int i = 1; i < count; i++) {
        if (_weightHistory[i] < minW) minW = _weightHistory[i];
        if (_weightHistory[i] > maxW) maxW = _weightHistory[i];
    }
    
    float diff = maxW - minW;
    return diff <= threshold;
}

void WeighingScale::tare() {
    _scale.tare();
    _offset = _scale.get_offset();
    _filteredWeight = 0;
    saveCalibration();
}

void WeighingScale::calibrate(int knownWeight) {
    if (knownWeight <= 0) return;
    long raw = _scale.read_average(10);
    _offset = _scale.get_offset();
    long diff = raw - _offset;
    if (abs(diff) < 10) diff = 10;
    _scaleFactor = (float)diff / (float)knownWeight;
    _scale.set_scale(_scaleFactor);
    Serial.printf("[SCALE] Calibrate raw=%ld, offset=%ld, diff=%ld, known=%d -> K=%.4f\n", 
                  raw, _offset, diff, knownWeight, _scaleFactor);
    saveCalibration();
}

void WeighingScale::setScale(float factor) {
    if (abs(factor) < 0.001) return;
    _scaleFactor = factor;
    _scale.set_scale(_scaleFactor);
    saveCalibration();
}

void WeighingScale::saveCalibration() {
    _prefs.begin("scale", false);
    _prefs.putFloat("factor", _scaleFactor);
    _prefs.putLong("offset", _offset);
    _prefs.end();
}

void WeighingScale::loadCalibration() {
    _prefs.begin("scale", true);
    _scaleFactor = _prefs.getFloat("factor", 1.0);
    _offset = _prefs.getLong("offset", 0);
    _prefs.end();
    _scale.set_scale(_scaleFactor);
    _scale.set_offset(_offset);
}
