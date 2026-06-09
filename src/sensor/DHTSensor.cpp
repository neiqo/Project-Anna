#include "DHTSensor.h"

DHTSensor::DHTSensor(uint8_t pin, uint8_t type) : _dht(pin, type) {}

void DHTSensor::begin() {
    _dht.begin();
}

bool DHTSensor::read() {
    ok = false; tries = 0;

    for (int i = 0; i < 4 && !ok; i++) {
        tries = i + 1;
        float h = _dht.readHumidity();
        float t = _dht.readTemperature();
        if (!isnan(h) && !isnan(t)) {
            humidity = h;
            temp     = t;
            ok       = true;
        } else {
            delay(120);
        }
    }

    lastReadMs = millis();
    return ok;
}
