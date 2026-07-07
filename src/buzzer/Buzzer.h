#pragma once
#include <Arduino.h>

#define BUZZER_PIN 25

class Buzzer {
public:
    void begin();
    void startupSequence();
    void zelda();
    void zeldasLullaby();
    void songOfStorms();
    void softChime();
    void softClassical();
    void softAmbient();
    void notifyChime();
private:
    void beep(uint32_t freq, uint32_t duration);
    void softTone(int freq, int durationMs);
};