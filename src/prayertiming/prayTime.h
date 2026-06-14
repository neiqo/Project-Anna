#pragma once

#include <Wire.h>

struct PrayTimes {
    String fajr;
    String dhuhr;
    String asr;
    String maghrib;
    String isha;
    String date;
    String method;
    bool valid = false;

};

class prayTime {
    public:
        void begin();
        void pullPrayTime(String curr_date);
        PrayTimes currentPrayTimes;

    private:
        String _lastFetchedDate = "";
};