#include "HistoryHandlers.h"
#include "../../config.h"

static WebServer*      _server  = nullptr;
static HistoricalData* _history = nullptr;

static void handleHistoryRoute() {
    const HourlyEntry* buf  = _history->hourlyBuffer();
    int                head = _history->hourlyHead();

    String out = "{\"ok\":true,\"items\":[";
    bool first = true;
    if (head >= 0) {
        for (int i = 0; i < HIST_HOURS; i++) {
            int idx = (head - i + HIST_HOURS) % HIST_HOURS;
            if (buf[idx].hour == 0) break;
            if (!first) out += ",";
            first = false;
            out += "{\"hour_start_ms\":"  + String((unsigned long long)buf[idx].hour * 1000ULL);
            out += ",\"avgT\":"           + String(buf[idx].avgT, 3);
            out += ",\"avgH\":"           + String(buf[idx].avgH, 3);
            out += "}";
        }
    }
    out += "]}";
    _server->sendHeader("Cache-Control", "no-cache");
    _server->send(200, "application/json", out);
}

static void handleDailyRoute() {
    const DailyEntry* buf  = _history->dailyBuffer();
    int               head = _history->dailyHead();

    String out = "{\"ok\":true,\"items\":[";
    bool first = true;
    if (head >= 0) {
        for (int i = 0; i < HIST_DAYS; i++) {
            int idx = (head - i + HIST_DAYS) % HIST_DAYS;
            if (buf[idx].day == 0) break;
            if (!first) out += ",";
            first = false;
            out += "{\"day_start_ms\":"   + String((unsigned long long)buf[idx].day * 1000ULL);
            out += ",\"avgT\":"           + String(buf[idx].avgT, 3);
            out += ",\"avgH\":"           + String(buf[idx].avgH, 3);
            out += ",\"score\":"          + String(buf[idx].comfort.score);
            out += ",\"labelIdx\":"       + String((int)buf[idx].comfort.label);
            out += "}";
        }
    }
    out += "]}";
    _server->sendHeader("Cache-Control", "no-cache");
    _server->send(200, "application/json", out);
}

void registerHistoryHandlers(WebServer &server, HistoricalData &history) {
    _server  = &server;
    _history = &history;

    server.on("/history", HTTP_GET, handleHistoryRoute);
    server.on("/daily",   HTTP_GET, handleDailyRoute);
}
