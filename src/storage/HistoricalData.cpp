#include "HistoricalData.h"

// ─── Public ────────────────────────────────────────────────────────────────

void HistoricalData::begin() {
    _loadHourly();
    _loadDaily();
}

void HistoricalData::tick(time_t epoch, float t, float h) {
    if (epoch == 0) return;

    time_t hour = _hourStart(epoch);
    time_t day  = _dayStart(epoch);

    if (_currentHour == 0) _currentHour = hour;
    if (_currentDay  == 0) _currentDay  = day;

    if (hour != _currentHour) {
        _finalizeHour(_currentHour);
        _currentHour = hour;
    }
    if (day != _currentDay) {
        _finalizeDay(_currentDay);
        _currentDay = day;
    }

    if (!isnan(t) && !isnan(h)) {
        _sumT += t; _sumH += h; _sampleCount++;
    }
}

bool HistoricalData::rollingHourAvg(int n, float &outT, float &outH) const {
    if (_hourlyHead < 0) return false;
    double st = 0, sh = 0; int cnt = 0;
    for (int i = 0; i < HIST_HOURS && cnt < n; i++) {
        int idx = (_hourlyHead - i + HIST_HOURS) % HIST_HOURS;
        if (_hourly[idx].hour == 0) break;
        st += _hourly[idx].avgT;
        sh += _hourly[idx].avgH;
        cnt++;
    }
    if (cnt == 0) return false;
    outT = (float)(st / cnt);
    outH = (float)(sh / cnt);
    return true;
}

bool HistoricalData::rollingDayAvg(float &outT, float &outH) const {
    if (_dailyHead < 0) return false;
    double st = 0, sh = 0; int cnt = 0;
    for (int i = 0; i < HIST_DAYS; i++) {
        int idx = (_dailyHead - i + HIST_DAYS) % HIST_DAYS;
        if (_daily[idx].day == 0) break;
        st += _daily[idx].avgT;
        sh += _daily[idx].avgH;
        cnt++;
    }
    if (cnt == 0) return false;
    outT = (float)(st / cnt);
    outH = (float)(sh / cnt);
    return true;
}

// ─── Private: finalize ─────────────────────────────────────────────────────

void HistoricalData::_finalizeHour(time_t hour) {
    if (_sampleCount == 0) return;
    float aT = (float)(_sumT / _sampleCount);
    float aH = (float)(_sumH / _sampleCount);
    _pushHourly(hour, aT, aH);
    _daySumT += aT; _daySumH += aH; _daySampleCount++;
    _sumT = _sumH = 0.0; _sampleCount = 0;
}

void HistoricalData::_finalizeDay(time_t day) {
    if (_daySampleCount == 0) return;
    _pushDaily(day,
        (float)(_daySumT / _daySampleCount),
        (float)(_daySumH / _daySampleCount)
    );
    _daySumT = _daySumH = 0.0; _daySampleCount = 0;
}

// ─── Private: push ─────────────────────────────────────────────────────────

void HistoricalData::_pushHourly(time_t hour, float aT, float aH) {
    _hourlyHead = (_hourlyHead + 1) % HIST_HOURS;
    _hourly[_hourlyHead] = { hour, aT, aH };
    _saveHourly();
}

void HistoricalData::_pushDaily(time_t day, float aT, float aH) {
    _dailyHead = (_dailyHead + 1) % HIST_DAYS;
    _daily[_dailyHead] = { day, aT, aH, ComfortScore::compute(aT, aH) };
    _saveDaily();
}

// ─── Private: NVS ──────────────────────────────────────────────────────────

void HistoricalData::_saveHourly() {
    _prefs.begin("vd_hourly", false);
    _prefs.putInt("head", _hourlyHead);
    _prefs.putBytes("buf", _hourly, sizeof(_hourly));
    _prefs.end();
}

void HistoricalData::_loadHourly() {
    _prefs.begin("vd_hourly", true);
    _hourlyHead = _prefs.getInt("head", -1);
    _prefs.getBytes("buf", _hourly, sizeof(_hourly));
    _prefs.end();
    Serial.printf("[history] loaded hourly, head=%d\n", _hourlyHead);
}

void HistoricalData::_saveDaily() {
    _prefs.begin("vd_daily", false);
    _prefs.putInt("head", _dailyHead);
    _prefs.putBytes("buf", _daily, sizeof(_daily));
    _prefs.end();
}

void HistoricalData::_loadDaily() {
    _prefs.begin("vd_daily", true);
    _dailyHead = _prefs.getInt("head", -1);
    _prefs.getBytes("buf", _daily, sizeof(_daily));
    _prefs.end();
    Serial.printf("[history] loaded daily, head=%d\n", _dailyHead);
}
