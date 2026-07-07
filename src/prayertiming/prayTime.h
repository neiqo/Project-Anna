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

struct currentPrayTimeBlock {
    String prayTime;
    String prayName;
    String nextPrayTime;
    String nextPrayName;
    String timeUntilNextPray;
    String currTime;
    String date;
    bool valid = false;
    
};

class prayTime {
    public:
        void begin();
        void pullPrayTime(String curr_date);
        void updateCurrentPrayTimeBlock(const PrayTimes& prayTimes, const String& curr_time, const String& curr_date);
        PrayTimes prayTimes;
        currentPrayTimeBlock currentPrayTime;
        

    private:
        String _lastFetchedDate = "";
        String _lastPrayName = "";
};      