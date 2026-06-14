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

    Serial.println("[oled] pulling prayer times");
    if (_prayTime.currentPrayTimes.valid) {
        Serial.println("[oled] showing prayer times");
        Serial.println("Fajr: " + _prayTime.currentPrayTimes.fajr);
    } else {
        Serial.println("[oled] no valid prayer times");
    }

    if (!ok)
    {
        display.setCursor(20, 25);
        display.println("Sensor Error");
        display.display();
        return;
    }

    // Border
    display.drawRect(0, 0, 128, 64, SH110X_WHITE);

    // Header
    display.setCursor(30, 3);
    display.print("ANNA SENSOR");

    // Separator lines
    display.drawLine(0, 14, 128, 14, SH110X_WHITE);
    display.drawLine(64, 14, 64, 63, SH110X_WHITE);

    // Icons
    drawThermometer(32, 20);
    drawDroplet(96, 25);

    // Temperature
    display.setTextSize(1.5);
    display.setCursor(21, 50);
    display.printf("%.1f", temp);

    // Degree symbol
    display.drawCircle(52, 43, 2, SH110X_WHITE);
    display.setTextSize(1);
    display.setCursor(56, 45);
    display.print("C");

    // Humidity
    display.setTextSize(1.5);
    display.setCursor(91, 50);
    display.printf("%.0f", humidity);

    display.setTextSize(1);
    display.setCursor(112, 45);
    display.print("%");

    display.display();

    
}

void OLEDDisplay::drawThermometer(int x, int y)
{
    display.drawRoundRect(x - 3, y, 6, 16, 2, SH110X_WHITE);
    display.fillRect(x - 1, y + 5, 2, 9, SH110X_WHITE);

    display.drawCircle(x, y + 18, 5, SH110X_WHITE);
    display.fillCircle(x, y + 18, 4, SH110X_WHITE);
}

void OLEDDisplay::drawDroplet(int x, int y)
{
    display.drawTriangle(
        x, y,
        x - 5, y + 10,
        x + 5, y + 10,
        SH110X_WHITE);

    display.drawCircle(x, y + 12, 5, SH110X_WHITE);
}

