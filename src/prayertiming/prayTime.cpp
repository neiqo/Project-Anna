#include "prayTime.h"
#include "HTTPClient.h"
#include <ArduinoJson.h>
#include <time.h>

void prayTime::begin() {

}

void prayTime::pullPrayTime(String curr_date) {
    // Only fetch if date has changed (i.e. once per day)
    if (curr_date == _lastFetchedDate) return;

    HTTPClient client;
    client.begin("https://api.aladhan.com/v1/timings/" + curr_date + "?latitude=1.290270&longitude=103.851959&method=11");
    client.addHeader("Accept", "application/json");

    int statusCode = client.GET();
    if (statusCode == HTTP_CODE_OK) {
        String body = client.getString();
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);

        if (!err) {
            currentPrayTimes.fajr    = doc["data"]["timings"]["Fajr"].as<String>();
            currentPrayTimes.dhuhr   = doc["data"]["timings"]["Dhuhr"].as<String>();
            currentPrayTimes.asr     = doc["data"]["timings"]["Asr"].as<String>();
            currentPrayTimes.maghrib = doc["data"]["timings"]["Maghrib"].as<String>();
            currentPrayTimes.isha    = doc["data"]["timings"]["Isha"].as<String>();
            currentPrayTimes.date    = doc["data"]["date"]["readable"].as<String>();
            currentPrayTimes.method  = doc["data"]["meta"]["method"]["name"].as<String>();
            currentPrayTimes.valid   = true;

            _lastFetchedDate = curr_date; 
            Serial.println("[pray] fetched times for " + curr_date);
        }
    }
    client.end();
}

