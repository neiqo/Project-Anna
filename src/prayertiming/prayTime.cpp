#include "prayTime.h"
#include "HTTPClient.h"
#include <ArduinoJson.h>
#include <time.h>
#include "../buzzer/buzzer.h"

extern Buzzer buzzer;

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
            prayTimes.fajr    = doc["data"]["timings"]["Fajr"].as<String>();
            prayTimes.dhuhr   = doc["data"]["timings"]["Dhuhr"].as<String>();
            prayTimes.asr     = doc["data"]["timings"]["Asr"].as<String>();
            prayTimes.maghrib = doc["data"]["timings"]["Maghrib"].as<String>();
            prayTimes.isha    = doc["data"]["timings"]["Isha"].as<String>();
            prayTimes.date    = doc["data"]["date"]["readable"].as<String>();
            prayTimes.method  = doc["data"]["meta"]["method"]["name"].as<String>();
            prayTimes.valid   = true;

            _lastFetchedDate = curr_date; 
            Serial.println("[pray] fetched times for " + curr_date);
        }
    }
    client.end();
}

void  prayTime::updateCurrentPrayTimeBlock(const PrayTimes& prayTimes, const String& curr_time, const String& curr_date) {
    if (!prayTimes.valid) {
        currentPrayTime.valid = false;
        return;
    }

    currentPrayTime.currTime = curr_time;
    currentPrayTime.date = curr_date;

        // curr_date comes in as "07-07-2026" — trim to "07-07-26"
    if (curr_date.length() == 10) {
        currentPrayTime.date = curr_date.substring(0, 6) + curr_date.substring(8, 10);
    } else {
        currentPrayTime.date = curr_date; // fallback, shouldn't normally hit
    }

    currentPrayTime.valid = true;

    if (curr_time < prayTimes.dhuhr) { // fajr block
        currentPrayTime.prayTime = prayTimes.fajr;
        currentPrayTime.prayName = "Fajr";
        currentPrayTime.nextPrayTime = prayTimes.dhuhr;
        currentPrayTime.nextPrayName = "Dhuhr";
    } else if (curr_time < prayTimes.asr) { // dhuhr block
        currentPrayTime.prayTime = prayTimes.dhuhr;
        currentPrayTime.prayName = "Dhuhr";
        currentPrayTime.nextPrayTime = prayTimes.asr;
        currentPrayTime.nextPrayName = "Asr";
    } else if (curr_time < prayTimes.maghrib) { // asr block
        currentPrayTime.prayTime = prayTimes.asr;
        currentPrayTime.prayName = "Asr";
        currentPrayTime.nextPrayTime = prayTimes.maghrib;
        currentPrayTime.nextPrayName = "Maghrib";
    } else if (curr_time < prayTimes.isha) { // maghrib block
        currentPrayTime.prayTime = prayTimes.maghrib;
        currentPrayTime.prayName = "Maghrib";
        currentPrayTime.nextPrayTime = prayTimes.isha;
        currentPrayTime.nextPrayName = "Isha";
    } else if (curr_time < prayTimes.fajr) { // isha block
        currentPrayTime.prayTime = prayTimes.isha;
        currentPrayTime.prayName = "Isha";
        currentPrayTime.nextPrayTime = prayTimes.fajr; // Next day
        currentPrayTime.nextPrayName = "Fajr";
    } else {
        // After Isha, next prayer is Fajr of the next day
        currentPrayTime.prayTime = prayTimes.fajr; // Next day
        currentPrayTime.prayName = "Fajr";
        currentPrayTime.nextPrayTime = prayTimes.dhuhr; // Next day
        currentPrayTime.nextPrayName = "Dhuhr";
    }


        // ─── Detect block transition and chime ────────────────────────────────
    if (_lastPrayName != currentPrayTime.prayName) {
        Serial.println("[pray] block changed: " + _lastPrayName + " -> " + currentPrayTime.prayName);
        buzzer.notifyChime();


    }
    _lastPrayName = currentPrayTime.prayName;
}
