#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

class OLEDDisplay {
public:
    void begin();
    void showReadings(float temp, float humidity, bool ok);

private:
    Adafruit_SH1106G display{128, 64, &Wire, -1};};