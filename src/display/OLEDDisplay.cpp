#include "OLEDDisplay.h"


void OLEDDisplay::begin() {
    Wire.begin(21, 22);

    if (!display.begin(0x3C, true)) {
        return;
    }

    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("ANNA Starting...");
    display.display();
}

void OLEDDisplay::showReadings(float temp, float humidity, bool ok)
{
    display.clearDisplay();

    if (_prayTime.currentPrayTime.valid) {
        Serial.println("[oled] showing current prayer time block");
        Serial.println("Current: " + _prayTime.currentPrayTime.prayName + " at " + _prayTime.currentPrayTime.prayTime);
        Serial.println("Next: " + _prayTime.currentPrayTime.nextPrayName + " at " + _prayTime.currentPrayTime.nextPrayTime);
    } else {
        Serial.println("[oled] no valid current prayer time block");
    }

    if (!ok)
    {
        display.setCursor(20, 25);
        display.println("Sensor Error");
        display.display();
        return;
    }

    // Outer border
    display.drawRect(0, 0, 128, 64, SH110X_WHITE);

    // ─── Header row: time (top) + date (below), stacked ──────────────────
    display.setTextSize(1);
    drawTimeLeftAligned(3, 2, _prayTime.currentPrayTime.currTime, SH110X_WHITE);

    display.setTextColor(SH110X_WHITE);
    display.setCursor(3, 10);
    display.print(_prayTime.currentPrayTime.date);

    display.setCursor(100, 2);
    display.print("ANNA");
    display.setCursor(88, 10);
    display.print("SENSOR");

    // Separator under header — header band is now shorter (0-19)
    display.drawLine(0, 19, 128, 19, SH110X_WHITE);

    // Vertical divider through the temp/humidity band only
    display.drawLine(64, 19, 64, 40, SH110X_WHITE);

    // Separator above prayer section
    display.drawLine(0, 40, 128, 40, SH110X_WHITE);

    // ─── Middle band: temp (left) + humidity (right), compact ────────────
    drawThermometer(14, 22);   // moved up so it clears the y=40 line
    display.setCursor(24, 26);
    display.printf("%.1f", temp);
    display.drawCircle(48, 23, 1, SH110X_WHITE); // degree dot
    display.setCursor(51, 26);
    display.print("C");

    drawDroplet(78, 24);       // moved up to match
    display.setCursor(88, 26);
    display.printf("%.0f", humidity);
    display.setCursor(108, 26);
    display.print("%");

    // ─── Bottom band: now (highlighted) + next, times right-aligned ──────
    if (_prayTime.currentPrayTime.valid) {
        // Highlighted "now" bar — inverted colors
        display.fillRect(1, 42, 126, 10, SH110X_WHITE);
        display.setTextColor(SH110X_BLACK);
        display.setCursor(4, 44);
        display.print(_prayTime.currentPrayTime.prayName);
        drawTimeRightAligned(_prayTime.currentPrayTime.prayTime, 122, 44, SH110X_BLACK);


        // "next" row — label + arrow + name (left), time (right)
        display.setTextColor(SH110X_WHITE);
        display.setCursor(4, 54);
        display.print("Next:");
        drawArrow(30, 56);
        display.setCursor(40, 54);
        display.print(_prayTime.currentPrayTime.nextPrayName);
        drawTimeRightAligned(_prayTime.currentPrayTime.nextPrayTime, 124, 55, SH110X_WHITE);
    } else {
        display.setTextColor(SH110X_WHITE);
        display.setCursor(3, 50);
        display.print("Prayer times unavailable");
    }

    display.display();
}


// tiny 3x5 bitmap font, just A / M / P — each row is 3 bits wide
static const uint8_t TINY_A[5] = {0b010, 0b101, 0b111, 0b101, 0b101};
static const uint8_t TINY_M[5] = {0b101, 0b111, 0b111, 0b101, 0b101};
static const uint8_t TINY_P[5] = {0b110, 0b101, 0b110, 0b100, 0b100};

String OLEDDisplay::to12Hour(const String& time24, bool& isPM)
{
    if (time24.length() < 5) { isPM = false; return time24; }

    int hour = time24.substring(0, 2).toInt();
    String minute = time24.substring(3, 5);

    isPM = hour >= 12;
    int hour12 = hour % 12;
    if (hour12 == 0) hour12 = 12;

    return String(hour12) + ":" + minute;
}

void OLEDDisplay::drawTinyChar(int x, int y, const uint8_t* pattern, uint16_t color)
{
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (pattern[row] & (1 << (2 - col))) {
                display.drawPixel(x + col, y + row, color);
            }
        }
    }
}

void OLEDDisplay::drawTinyAmPm(int x, int y, bool isPM, uint16_t color)
{
    drawTinyChar(x, y, isPM ? TINY_P : TINY_A, color);
    drawTinyChar(x + 4, y, TINY_M, color); // 3px char + 1px gap
}

void OLEDDisplay::drawTimeLeftAligned(int x, int y, const String& time24, uint16_t color)
{
    bool isPM;
    String hm = to12Hour(time24, isPM);

    display.setTextColor(color);
    display.setCursor(x, y);
    display.print(hm);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(hm, x, y, &x1, &y1, &w, &h);

    drawTinyAmPm(x + w + 3, y + 2, isPM, color); // vertically nudged to sit mid-height of the time text
}

void OLEDDisplay::drawTimeRightAligned(const String& time24, int rightEdgeX, int y, uint16_t color)
{
    bool isPM;
    String hm = to12Hour(time24, isPM);

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(hm, 0, y, &x1, &y1, &w, &h);

    const int ampmWidth = 7; // 3 + 1 + 3
    const int gap = 3;
    int startX = rightEdgeX - (w + gap + ampmWidth);

    display.setTextColor(color);
    display.setCursor(startX, y);
    display.print(hm);

    drawTinyAmPm(startX + w + gap, y + 2, isPM, color);
}

void OLEDDisplay::drawThermometer(int x, int y)
{
    display.drawRoundRect(x - 2, y, 4, 10, 1, SH110X_WHITE);
    display.fillRect(x - 1, y + 3, 2, 6, SH110X_WHITE);
    display.drawCircle(x, y + 12, 3, SH110X_WHITE);
    display.fillCircle(x, y + 12, 2, SH110X_WHITE);
}

void OLEDDisplay::drawDroplet(int x, int y)
{
    display.drawTriangle(
        x, y,
        x - 3, y + 6,
        x + 3, y + 6,
        SH110X_WHITE);

    display.drawCircle(x, y + 7, 3, SH110X_WHITE);
}

void OLEDDisplay::drawArrow(int x, int y)
{
    display.fillTriangle(
        x, y,
        x, y + 5,
        x + 5, y + 2,
        SH110X_WHITE);
}

void OLEDDisplay::drawRightAligned(const String& text, int rightEdgeX, int y, bool black)
{
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
    display.setCursor(rightEdgeX - w, y);
    display.print(text);
}