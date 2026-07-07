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
    void drawArrow(int x, int y);
    void drawRightAligned(const String& text, int rightEdgeX, int y, bool black = false);
    prayTime& _prayTime;

    String to12Hour(const String& time24, bool& isPM);
    void drawTinyChar(int x, int y, const uint8_t* pattern, uint16_t color);
    void drawTinyAmPm(int x, int y, bool isPM, uint16_t color);
    void drawTimeLeftAligned(int x, int y, const String& time24, uint16_t color);
    void drawTimeRightAligned(const String& time24, int rightEdgeX, int y, uint16_t color);

    Adafruit_SH1106G display{128, 64, &Wire, -1};};