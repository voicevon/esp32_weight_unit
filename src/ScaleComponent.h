#ifndef SCALE_COMPONENT_H
#define SCALE_COMPONENT_H

#include <HX711.h>
#include <Preferences.h>

class ScaleComponent {
public:
    ScaleComponent(int dtPin, int sckPin);
    void begin();
    
    // 核心采样与逻辑
    void update(long raw);              // 输入原始值
    float getWeight() const { return _currentWeight; }
    float getFilteredWeight() const { return _filteredWeight; }
    
    bool isStable(float threshold);     // 稳定性判定
    void tare();                        // 去皮
    void calibrate(int knownWeight);    // 标定
    
    // 基础属性
    bool isReady() { return _scale.is_ready(); }
    float getScaleFactor() const { return _scaleFactor; }
    long getOffset() const { return _offset; }
    long getRawValue() { return _scale.read(); }

    void setScale(float factor);

private:
    HX711 _scale;
    Preferences _prefs;
    int _dtPin, _sckPin;
    
    float _scaleFactor;
    long _offset;
    
    // 滤波与数据流
    float _currentWeight;
    float _filteredWeight;
    float _emaAlpha; // EMA 滤波系数 (0.0 - 1.0)
    
    // 稳定性追踪 (极差法)
    static const int HISTORY_SIZE = 15;
    float _weightHistory[HISTORY_SIZE];
    int _historyIndex;
    bool _historyFull;

    void saveCalibration();
    void loadCalibration();
};

#endif
