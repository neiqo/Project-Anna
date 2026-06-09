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
    display.println("VoidDeck Starting...");
    display.display();
}

void OLEDDisplay::showReadings(float temp, float humidity, bool ok) {
    display.clearDisplay();

    display.setCursor(0, 0);
    display.println("DHT Sensor");

    if (ok) {
        display.setCursor(0, 20);
        display.printf("Temp: %.1f C\n", temp);

        display.setCursor(0, 35);
        display.printf("Hum : %.1f %%\n", humidity);
    } else {
        display.setCursor(0, 25);
        display.println("Sensor Error");
    }

    display.display();
}