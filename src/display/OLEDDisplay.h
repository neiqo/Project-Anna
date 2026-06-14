#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <prayertiming/prayTime.h>


class OLEDDisplay {
public:
    OLEDDisplay(prayTime& pt) : _prayTime(pt) {}  // take reference in constructor
    void begin();
    void showReadings(float temp, float humidity, bool ok);

private:
    void drawThermometer(int x, int y);
    void drawDroplet(int x, int y);
    prayTime& _prayTime;

    Adafruit_SH1106G display{128, 64, &Wire, -1};};