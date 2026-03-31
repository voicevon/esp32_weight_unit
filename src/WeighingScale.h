#ifndef WEIGHING_SCALE_H
#define WEIGHING_SCALE_H

#include <HX711.h>
#include <Preferences.h>

class WeighingScale {
public:
    WeighingScale(int dtPin, int sckPin);
    void begin();
    
    void update(long raw); 
    float getWeight() const { return _currentWeight; }
    float getFilteredWeight() const { return _filteredWeight; }
    
    bool isStable(float threshold); 
    void tare();                    
    void calibrate(int knownWeight);
    
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
    
    float _currentWeight;
    float _filteredWeight;
    float _emaAlpha; 
    
    static const int HISTORY_SIZE = 15;
    float _weightHistory[HISTORY_SIZE];
    int _historyIndex;
    bool _historyFull;

    void saveCalibration();
    void loadCalibration();
};

#endif
