#pragma once
#include <Arduino.h>
#include <DHT.h>

class DHTSensor {
public:
    DHTSensor(uint8_t pin, uint8_t type);

    void  begin();
    bool  read();       // returns true on success

    // Last successful reading — read-only from outside
    float    temp        = NAN;
    float    humidity    = NAN;
    bool     ok          = false;
    uint8_t  tries       = 0;
    uint32_t lastReadMs  = 0;

private:
    DHT _dht;
};
