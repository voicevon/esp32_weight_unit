#ifndef WEIGHING_SCALE_H
#define WEIGHING_SCALE_H

#include <HX711.h>
#include <Preferences.h>

class WeighingScale {
public:
    WeighingScale(int dtPin, int sckPin);
    void begin();
    
    void update(long raw); 
    float getWeight() const { 
        return (abs(_currentWeight) <= _ztrThreshold) ? 0.0f : _currentWeight; 
    }
    float getFilteredWeight() const { 
        return (abs(_filteredWeight) <= _ztrThreshold) ? 0.0f : _filteredWeight; 
    }
    
    // 原始测量值（无死区），用于调试或配置记录
    float getTrueWeight() const { return _currentWeight; }
    float getTrueFilteredWeight() const { return _filteredWeight; }
    
    bool isStable(float threshold); 
    void tare();                    
    void calibrate(int knownWeight);
    
    bool isReady() { return _scale.is_ready(); }
    float getScaleFactor() const { return _scaleFactor; }
    long getOffset() const { return _offset; }
    long getRawValue() { return _scale.read(); }
    long getLastRaw() const { return _lastRaw; }

    void setScale(float factor);
    void saveCalibration();
    void loadCalibration();
    void setZtrThreshold(uint16_t threshold) { _ztrThreshold = threshold; }

private:
    HX711 _scale;
    Preferences _prefs;
    int _dtPin, _sckPin;
    
    float _scaleFactor;
    long _offset;
    long _lastRaw;
    
    float _currentWeight;
    float _filteredWeight;
    float _emaAlpha; 
    
    static const int HISTORY_SIZE = 15;
    float _weightHistory[HISTORY_SIZE];
    int _historyIndex;
    bool _historyFull;
    
    uint16_t _ztrThreshold;
    unsigned long _lastZeroStableTime;

};

#endif
