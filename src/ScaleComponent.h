#ifndef SCALE_COMPONENT_H
#define SCALE_COMPONENT_H

#include <HX711.h>
#include <Preferences.h>

class ScaleComponent {
public:
    ScaleComponent(int dtPin, int sckPin);
    void begin();
    float getWeight(int samples = 5);
    float calculateWeight(long raw);
    void updateHistory(float w);
    bool isStable(float threshold = 0.5f); // Check if the last readings are within threshold
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
    
    // Stability tracking
    static const int HISTORY_SIZE = 10;
    float weightHistory[HISTORY_SIZE];
    int historyIndex;
    bool historyFull;

    void saveCalibration();
    void loadCalibration();
};

#endif
