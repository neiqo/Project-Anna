#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <time.h>

#include "config.h"
#include "secrets.h"
#include "sensor/DHTSensor.h"
#include "storage/HistoricalData.h"
#include "server/WebServer.h"
#include "display/OLEDDisplay.h"
#include "prayertiming/prayTime.h"
#include "buzzer/Buzzer.h"

Buzzer buzzer;

prayTime prayTimes;
OLEDDisplay oled(prayTimes);

// ─── Globals ───────────────────────────────────────────────────────────────
DHTSensor      sensor(DHTPIN, DHTTYPE);
HistoricalData history;
ANNAServer server(sensor, history);

unsigned long lastReadMs = 0;
bool          warmedUp   = false;

// ─── Helpers ───────────────────────────────────────────────────────────────
static void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("[wifi] connecting to %s", WIFI_SSID);
    for (int i = 0; i < 60 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500); Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\n[wifi] failed — rebooting");
        delay(1500); ESP.restart();
    }
    Serial.printf("\n[wifi] connected — %s\n", WiFi.localIP().toString().c_str());
}

static void setupMDNS() {
    if (MDNS.begin(HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[mdns] http://%s.local\n", HOSTNAME);
    }
}

static void syncTime() {
    setenv("TZ", "SGT-8", 1); tzset();
    configTime(8 * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
    Serial.print("[time] syncing");
    for (int i = 0; i < 40; i++) {
        if (time(nullptr) > 100000) { Serial.println(" ok"); return; }
        Serial.print("."); delay(250);
    }
    Serial.println(" timed out");
}

void getCurrentDate(char* buf, size_t len) {
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    strftime(buf, len, "%d-%m-%Y", &timeinfo); // formats to "15-06-2025"
}

void getCurrentTime(char* buf, size_t len) {
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    strftime(buf, len, "%H:%M", &timeinfo); // formats to "14:32"
}

// ─── Arduino entry points ──────────────────────────────────────────────────
void setup() {
    Serial.begin(115200); delay(200);
    Serial.println("\n[boot] anna starting");

    pinMode(DHTPIN, INPUT_PULLUP);
    sensor.begin();
    history.begin();
    oled.begin();
    connectWiFi();
    setupMDNS();
    syncTime();
    buzzer.begin();


    server.begin();

    buzzer.songOfStorms();
}

void loop() {
    server.handle();

    unsigned long nowMs = millis();
    if (nowMs - lastReadMs >= READ_INTERVAL_MS) {
        if (!warmedUp) { delay(1500); warmedUp = true; }

        sensor.read();
        
        char date[12];
        char timeStr[6];
        getCurrentDate(date, sizeof(date));
        getCurrentTime(timeStr, sizeof(timeStr));

        prayTimes.pullPrayTime(String(date));
        prayTimes.updateCurrentPrayTimeBlock(prayTimes.prayTimes, String(timeStr), String(date));

        oled.showReadings(
            sensor.temp,
            sensor.humidity,
            sensor.ok
        );
        history.tick(time(nullptr), sensor.temp, sensor.humidity);

        lastReadMs = nowMs;
    }
    delay(1);
}
