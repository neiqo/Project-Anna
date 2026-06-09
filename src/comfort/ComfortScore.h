#pragma once
#include <Arduino.h>

// Label indices — used by both C++ and passed to the frontend as integers
enum ComfortLabel {
    COMFORT_IDEAL       = 0,
    COMFORT_COMFORTABLE = 1,
    COMFORT_WARM        = 2,
    COMFORT_HUMID       = 3,
    COMFORT_HOT         = 4
};

struct ComfortResult {
    int          score;       // 0–100, or -1 if no data
    ComfortLabel label;
};

class ComfortScore {
public:
    // Returns a ComfortResult from a temp + humidity reading
    static ComfortResult compute(float t, float h);
};
