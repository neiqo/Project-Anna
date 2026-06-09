#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "../comfort/ComfortScore.h"
#include "../config.h"

struct HourlyEntry {
    time_t hour;    // epoch of hour start
    float  avgT;
    float  avgH;
};

struct DailyEntry {
    time_t       day;   // epoch of day start (UTC midnight)
    float        avgT;
    float        avgH;
    ComfortResult comfort;
};

class HistoricalData {
public:
    void begin();

    // Call once per sensor read with the current epoch + values
    void tick(time_t epoch, float t, float h);

    // Rolling averages over n most-recent completed hours
    bool rollingHourAvg(int n, float &outT, float &outH) const;

    // Rolling average over all stored days
    bool rollingDayAvg(float &outT, float &outH) const;

    // Direct read access for HTTP handlers
    const HourlyEntry* hourlyBuffer() const { return _hourly; }
    int                hourlyHead()   const { return _hourlyHead; }

    const DailyEntry*  dailyBuffer()  const { return _daily; }
    int                dailyHead()    const { return _dailyHead; }

private:
    // Circular buffers
    HourlyEntry _hourly[HIST_HOURS] = {};
    int         _hourlyHead = -1;

    DailyEntry  _daily[HIST_DAYS] = {};
    int         _dailyHead = -1;

    // Accumulators
    double   _sumT = 0, _sumH = 0;
    uint32_t _sampleCount = 0;
    time_t   _currentHour = 0;

    double   _daySumT = 0, _daySumH = 0;
    uint32_t _daySampleCount = 0;
    time_t   _currentDay = 0;

    Preferences _prefs;

    void _finalizeHour(time_t hour);
    void _finalizeDay(time_t day);
    void _pushHourly(time_t hour, float aT, float aH);
    void _pushDaily(time_t day, float aT, float aH);

    void _saveHourly();
    void _loadHourly();
    void _saveDaily();
    void _loadDaily();

    static inline time_t _hourStart(time_t t) { return t - (t % 3600); }
    static inline time_t _dayStart(time_t t)  { return t - (t % 86400); }
};
