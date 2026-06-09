#include "ComfortScore.h"
#include "../config.h"

ComfortResult ComfortScore::compute(float t, float h) {
    if (isnan(t) || isnan(h)) {
        return { -1, COMFORT_IDEAL };
    }

    float tPenalty = abs(t - COMFORT_IDEAL_TEMP) * 4.0f;
    float hPenalty = abs(h - COMFORT_IDEAL_HUM)  * 0.6f;
    float combo    = (t > 28.0f && h > 70.0f)
                       ? (t - 28.0f) * (h - 70.0f) * 0.15f
                       : 0.0f;

    int score = (int)constrain(100.0f - tPenalty - hPenalty - combo, 0.0f, 100.0f);

    ComfortLabel label;
    if      (score >= 80) label = COMFORT_IDEAL;
    else if (score >= 60) label = COMFORT_COMFORTABLE;
    else if (h > 75.0f)   label = COMFORT_HUMID;
    else if (t > 30.0f)   label = COMFORT_HOT;
    else                  label = COMFORT_WARM;

    return { score, label };
}
