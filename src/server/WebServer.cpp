#include "WebServer.h"
#include "handlers/SensorHandlers.h"
#include "handlers/HistoryHandlers.h"
#include <ESPmDNS.h>
#include "../config.h"
#include "WebHtml.h"

// ─── Inline HTML ───────────────────────────────────────────────────────────
// Keep the frontend here so it stays co-located with the server,
// not buried in main.cpp
static const char HTML[] PROGMEM = HTML_CONTENT;

// ─── VoiddeckServer ────────────────────────────────────────────────────────

VoiddeckServer::VoiddeckServer(DHTSensor &sensor, HistoricalData &history)
    : _server(80), _sensor(sensor), _history(history) {}

void VoiddeckServer::begin() {
    _registerRoutes();
    _server.begin();
    Serial.println("[server] HTTP started on port 80");
}

void VoiddeckServer::handle() {
    _server.handleClient();
}

void VoiddeckServer::_registerRoutes() {
    // Root — serve the dashboard
    _server.on("/", HTTP_GET, [this]() {
        _server.send_P(200, "text/html; charset=utf-8", HTML);
    });

    // Feature-grouped handlers
    registerSensorHandlers(_server, _sensor, _history);
    registerHistoryHandlers(_server, _history);

    _server.on("/favicon.ico", HTTP_GET, [this]() { _server.send(204); });
    _server.onNotFound([this]() { _server.send(404, "text/plain", "Not found"); });
}
