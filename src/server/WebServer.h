#pragma once
#include <WebServer.h>
#include "../sensor/DHTSensor.h"
#include "../storage/HistoricalData.h"

class ANNAServer {
public:
    ANNAServer(DHTSensor &sensor, HistoricalData &history);

    void begin();
    void handle();   // call in loop()

private:
    WebServer      _server;
    DHTSensor&     _sensor;
    HistoricalData& _history;

    void _registerRoutes();
};
