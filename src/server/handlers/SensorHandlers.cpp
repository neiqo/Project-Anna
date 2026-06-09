#include "SensorHandlers.h"

// Pointers set at registration time — handlers are plain functions (WebServer limitation)
static DHTSensor*     _sensor  = nullptr;
static HistoricalData* _history = nullptr;

static void handleDiag() {
    String json = "{";
    json += "\"ok\":"        + String(_sensor->ok ? "true" : "false");
    json += ",\"temp\":"     + (isnan(_sensor->temp)     ? String("null") : String(_sensor->temp, 2));
    json += ",\"humidity\":" + (isnan(_sensor->humidity) ? String("null") : String(_sensor->humidity, 2));
    json += ",\"tries\":"    + String(_sensor->tries);
    json += ",\"ms\":"       + String(millis() - _sensor->lastReadMs);
    json += "}";
    WebServer* s = &(*(WebServer*)nullptr); // handled via capture below
    // Note: response sent via the server reference captured at registration
    // See registerSensorHandlers for the actual server.send call pattern
}

static WebServer* _server = nullptr;

static void handleDiagRoute() {
    String json = "{";
    json += "\"ok\":"        + String(_sensor->ok ? "true" : "false");
    json += ",\"temp\":"     + (isnan(_sensor->temp)     ? String("null") : String(_sensor->temp, 2));
    json += ",\"humidity\":" + (isnan(_sensor->humidity) ? String("null") : String(_sensor->humidity, 2));
    json += ",\"tries\":"    + String(_sensor->tries);
    json += ",\"ms\":"       + String(millis() - _sensor->lastReadMs);
    json += "}";
    _server->sendHeader("Cache-Control", "no-cache");
    _server->send(200, "application/json", json);
}

static void handleStatsRoute() {
    ComfortResult cr = ComfortScore::compute(_sensor->temp, _sensor->humidity);

    float r3T, r3H, r12T, r12H, r24T, r24H, r7dT, r7dH;
    bool has3  = _history->rollingHourAvg(3,  r3T,  r3H);
    bool has12 = _history->rollingHourAvg(12, r12T, r12H);
    bool has24 = _history->rollingHourAvg(24, r24T, r24H);
    bool has7d = _history->rollingDayAvg(r7dT, r7dH);

    auto fv = [](bool has, float v) -> String {
        return (!has || isnan(v)) ? "null" : String(v, 2);
    };

    String json = "{\"ok\":true";
    json += ",\"score\":"    + String(cr.score);
    json += ",\"labelIdx\":" + String((int)cr.label);
    json += ",\"r3T\":"      + fv(has3,  r3T);
    json += ",\"r3H\":"      + fv(has3,  r3H);
    json += ",\"r12T\":"     + fv(has12, r12T);
    json += ",\"r12H\":"     + fv(has12, r12H);
    json += ",\"r24T\":"     + fv(has24, r24T);
    json += ",\"r24H\":"     + fv(has24, r24H);
    json += ",\"r7dT\":"     + fv(has7d, r7dT);
    json += ",\"r7dH\":"     + fv(has7d, r7dH);
    json += "}";
    _server->sendHeader("Cache-Control", "no-cache");
    _server->send(200, "application/json", json);
}

static void handleTimeRoute() {
    time_t t = time(nullptr);
    if (t < 100000) { _server->send(200, "application/json", "{\"ok\":false}"); return; }
    String json = "{\"ok\":true,\"epoch_ms\":";
    json += String((unsigned long long)t * 1000ULL);
    json += "}";
    _server->sendHeader("Cache-Control", "no-cache");
    _server->send(200, "application/json", json);
}

void registerSensorHandlers(WebServer &server, DHTSensor &sensor, HistoricalData &history) {
    _server  = &server;
    _sensor  = &sensor;
    _history = &history;

    server.on("/diag",  HTTP_GET, handleDiagRoute);
    server.on("/stats", HTTP_GET, handleStatsRoute);
    server.on("/time",  HTTP_GET, handleTimeRoute);
}
