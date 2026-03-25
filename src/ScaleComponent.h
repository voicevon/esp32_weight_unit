#ifndef SCALE_COMPONENT_H
#define SCALE_COMPONENT_H

#include <HX711.h>
#include <Preferences.h>

class ScaleComponent {
public:
    ScaleComponent(int dtPin, int sckPin);
    void begin();
    float getWeight(int samples = 5);
    void tare();
    void calibrate(int knownWeight);
    void setScale(float factor);
    bool isReady() { return scale.is_ready(); }
    
    float getScaleFactor() { return scaleFactor; }
    long getOffset() { return offset; }
    long getRawValue() { return scale.read(); }

private:
    HX711 scale;
    Preferences preferences;
    int _dtPin, _sckPin;
    float scaleFactor;
    long offset;

    void saveCalibration();
    void loadCalibration();
};

#endif
