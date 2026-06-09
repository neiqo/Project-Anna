#pragma once
#include <WebServer.h>
#include "../../sensor/DHTSensor.h"
#include "../../storage/HistoricalData.h"
#include "../../comfort/ComfortScore.h"

// Registers all sensor-related routes onto the server
void registerSensorHandlers(WebServer &server, DHTSensor &sensor, HistoricalData &history);
