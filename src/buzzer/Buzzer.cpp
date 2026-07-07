#include "Buzzer.h"

void Buzzer::begin() {
    ledcSetup(0, 1000, 8);        // channel 0, 1kHz, 8-bit
    ledcAttachPin(BUZZER_PIN, 0); // pin, channel 0
}

void Buzzer::startupSequence() {
    Serial.println("[buzzer] startup sequence");
    beep(1000, 150);
    delay(80);
    beep(1200, 150);
    delay(80);
    beep(1500, 200);
}

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST 0

void Buzzer::zelda() {
    int tempo = 88;
    int wholenote = (60000 * 4) / tempo;

    int melody[] = {
        NOTE_AS4,-2, NOTE_F4,8, NOTE_F4,8, NOTE_AS4,8,
        NOTE_GS4,16, NOTE_FS4,16, NOTE_GS4,-2,
        NOTE_AS4,-2, NOTE_FS4,8, NOTE_FS4,8, NOTE_AS4,8,
        NOTE_A4,16, NOTE_G4,16, NOTE_A4,-2,
        REST,1,

        NOTE_AS4,4, NOTE_F4,-4, NOTE_AS4,8, NOTE_AS4,16, NOTE_C5,16, NOTE_D5,16, NOTE_DS5,16,
        NOTE_F5,2, NOTE_F5,8, NOTE_F5,8, NOTE_F5,8, NOTE_FS5,16, NOTE_GS5,16,
        NOTE_AS5,-2, NOTE_AS5,8, NOTE_AS5,8, NOTE_GS5,8, NOTE_FS5,16,
        NOTE_GS5,-8, NOTE_FS5,16, NOTE_F5,2, NOTE_F5,4,

        NOTE_DS5,-8, NOTE_F5,16, NOTE_FS5,2, NOTE_F5,8, NOTE_DS5,8,
        NOTE_CS5,-8, NOTE_DS5,16, NOTE_F5,2, NOTE_DS5,8, NOTE_CS5,8,
        NOTE_C5,-8, NOTE_D5,16, NOTE_E5,2, NOTE_G5,8,
        NOTE_F5,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,8, NOTE_F4,16, NOTE_F4,8,

        NOTE_AS4,4, NOTE_F4,-4, NOTE_AS4,8, NOTE_AS4,16, NOTE_C5,16, NOTE_D5,16, NOTE_DS5,16,
        NOTE_F5,2, NOTE_F5,8, NOTE_F5,8, NOTE_F5,8, NOTE_FS5,16, NOTE_GS5,16,
        NOTE_AS5,-2, NOTE_CS6,4,
        NOTE_C6,4, NOTE_A5,2, NOTE_F5,4,
        NOTE_FS5,-2, NOTE_AS5,4,
        NOTE_A5,4, NOTE_F5,2, NOTE_F5,4,

        NOTE_FS5,-2, NOTE_AS5,4,
        NOTE_A5,4, NOTE_F5,2, NOTE_D5,4,
        NOTE_DS5,-2, NOTE_FS5,4,
        NOTE_F5,4, NOTE_CS5,2, NOTE_AS4,4,
        NOTE_C5,-8, NOTE_D5,16, NOTE_E5,2, NOTE_G5,8,
        NOTE_F5,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,16, NOTE_F4,8, NOTE_F4,16, NOTE_F4,8
    };

    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    for (int i = 0; i < notes * 2; i += 2) {
        int divider = melody[i + 1];
        int noteDuration;

        if (divider > 0) {
            noteDuration = wholenote / divider;
        } else {
            noteDuration = wholenote / abs(divider);
            noteDuration *= 1.5;
        }

        if (melody[i] == REST) {
            ::delay(noteDuration);
        } else {
            ledcWriteTone(0, melody[i]);
            ::delay(noteDuration * 0.9);
            ledcWriteTone(0, 0);
            ::delay(noteDuration * 0.1);
        }
    }
}

void Buzzer::beep(uint32_t freq, uint32_t duration) {
    ledcWriteTone(0, freq);   // ← channel 0, not BUZZER_PIN
    delay(duration);
    ledcWriteTone(0, 0);
}

/* 
  Zelda's Lullaby 
  Connect a piezo buzzer or speaker to pin 11 or select a new pin.
  More songs available at https://github.com/robsoncouto/arduino-songs                                            
                                              
                                              Robson Couto, 2019
*/

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0
void Buzzer::zeldasLullaby() {

    int tempo = 108;
    int wholenote = (60000 * 4) / tempo;

    int melody[] = {

    NOTE_E4,2, NOTE_G4,4,
    NOTE_D4,2, NOTE_C4,8, NOTE_D4,8,
    NOTE_E4,2, NOTE_G4,4,
    NOTE_D4,-2,
    NOTE_E4,2, NOTE_G4,4,

    NOTE_D5,2, NOTE_C5,4,
    NOTE_G4,2, NOTE_F4,8, NOTE_E4,8,
    NOTE_D4,-2,

    NOTE_E4,2, NOTE_G4,4,
    NOTE_D4,2, NOTE_C4,8, NOTE_D4,8,
    NOTE_E4,2, NOTE_G4,4,
    NOTE_D4,-2,
    NOTE_E4,2, NOTE_G4,4,

    NOTE_D5,2, NOTE_C5,4,
    NOTE_G4,2, NOTE_F4,8, NOTE_E4,8,
    NOTE_F4,8, NOTE_E4,8, NOTE_C4,2,
    NOTE_F4,2, NOTE_E4,8, NOTE_D4,8,
    NOTE_E4,8, NOTE_D4,8, NOTE_A3,2,
    NOTE_G4,2, NOTE_F4,8, NOTE_E4,8,
    NOTE_F4,8, NOTE_E4,8, NOTE_C4,4, NOTE_F4,4,
    NOTE_C5,-2

    };

    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    for (int i = 0; i < notes * 2; i += 2) {

        int divider = melody[i + 1];
        int noteDuration;

        if (divider > 0) {
            noteDuration = wholenote / divider;
        } else {
            noteDuration = (wholenote / abs(divider)) * 1.5;
        }

        if (melody[i] == REST) {
            delay(noteDuration);
        } else {
            // 🔥 FIX: USE LEDC ONLY (NO tone/noTone)
            ledcWriteTone(0, melody[i]);

            delay((int)(noteDuration * 0.9));

            ledcWriteTone(0, 0);
            delay((int)(noteDuration * 0.1));
        }
    }
}


#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

void Buzzer::songOfStorms() {

    int tempo = 108;
    int wholenote = (60000 * 4) / tempo;

    int melody[] = {
        NOTE_D4,8, NOTE_F4,8, NOTE_D5,2,
        NOTE_E5,-4, NOTE_F5,8, NOTE_E5,8, NOTE_E5,8,
        NOTE_E5,8, NOTE_C5,8, NOTE_A4,2,
        NOTE_A4,4, NOTE_D4,4, NOTE_F4,8, NOTE_G4,8,
        NOTE_A4,-2,
        NOTE_A4,4, NOTE_D4,4, NOTE_F4,8, NOTE_G4,8,
        NOTE_E4,-2,
        // NOTE_D4,8, NOTE_F4,8, NOTE_D5,2,
        // NOTE_D4,8, NOTE_F4,8, NOTE_D5,2,

    };

    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    for (int i = 0; i < notes * 2; i += 2) {

        int divider = melody[i + 1];
        int noteDuration;

        if (divider > 0) {
            noteDuration = wholenote / divider;
        } else {
            noteDuration = (wholenote / abs(divider)) * 1.5;
        }

        if (melody[i] == REST) {
            delay(noteDuration);
        } else {
            // ESP32 LEDC output ONLY (no tone/noTone)
            ledcWriteTone(0, melody[i]);

            delay((int)(noteDuration * 0.9));

            ledcWriteTone(0, 0);
            delay((int)(noteDuration * 0.1));
        }
    }
}

void Buzzer::softTone(int freq, int durationMs) {

    int steps = 12;  // more steps = smoother sound

    int attack = durationMs * 0.25;
    int sustain = durationMs * 0.5;
    int release = durationMs * 0.25;

    int attackStep = attack / steps;
    int releaseStep = release / steps;

    // ---------- ATTACK (rising feel) ----------
    for (int i = 1; i <= steps; i++) {
        ledcWriteTone(0, freq);
        delay(attackStep);
    }

    // ---------- SUSTAIN ----------
    ledcWriteTone(0, freq);
    delay(sustain);

    // ---------- RELEASE (soft fade effect via pulsing) ----------
    for (int i = steps; i > 0; i--) {
        ledcWriteTone(0, freq);
        delay(releaseStep);
    }

    ledcWriteTone(0, 0);
}

void Buzzer::softChime() {

    int melody[] = {
        NOTE_C5, 4,
        NOTE_E5, 4,
        NOTE_G5, 2,
        REST, 4,
        NOTE_G5, 8,
        NOTE_E5, 8,
        NOTE_C5, 2
    };

    int tempo = 90;
    int wholenote = (60000 * 4) / tempo;

    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    for (int i = 0; i < notes * 2; i += 2) {

        int note = melody[i];
        int divider = melody[i + 1];

        int duration = (divider > 0)
            ? wholenote / divider
            : (wholenote / abs(divider)) * 1.5;

        if (note == REST) {
            delay(duration);
        } else {
            softTone(note, duration);
            delay(10);
        }
    }
}

void Buzzer::notifyChime() {
    int melody[]     = { NOTE_C5, NOTE_D5, NOTE_E5, NOTE_G5, NOTE_A5, NOTE_C6, NOTE_C6, NOTE_A5, NOTE_G5, NOTE_E5, NOTE_D5, NOTE_C5 };
    int durations[]  = { 600,     600,     600,     600,     600,    1400, 600,    600,     600,     600,     600,     1400 };
    int gap = 130;

    for (int i = 0; i < 12; i++) {
        ledcWriteTone(0, melody[i]);
        delay(durations[i]);
        ledcWriteTone(0, 0);
        delay(gap);
    }
}

void Buzzer::softClassical() {

    int melody[] = {
        NOTE_C5, 4,
        NOTE_E5, 4,
        NOTE_G5, 4,
        NOTE_E5, 4,

        NOTE_D5, 4,
        NOTE_F5, 4,
        NOTE_A5, 2,

        REST, 4,

        NOTE_G5, 4,
        NOTE_E5, 4,
        NOTE_C5, 2
    };

    int tempo = 80;
    int wholenote = (60000 * 4) / tempo;

    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    for (int i = 0; i < notes * 2; i += 2) {

        int note = melody[i];
        int divider = melody[i + 1];

        int duration = (divider > 0)
            ? wholenote / divider
            : (wholenote / abs(divider)) * 1.5;

        if (note == REST) {
            delay(duration);
        } else {
            softTone(note, duration);
            delay(8);
        }
    }
}

void Buzzer::softAmbient() {

    int melody[] = {
        NOTE_C5, 2,
        REST, 8,
        NOTE_E5, 2,
        REST, 8,
        NOTE_G5, 2,
        REST, 4,
        NOTE_E5, 2,
        NOTE_C5, 1
    };

    int tempo = 60;
    int wholenote = (60000 * 4) / tempo;

    int notes = sizeof(melody) / sizeof(melody[0]) / 2;

    for (int i = 0; i < notes * 2; i += 2) {

        int note = melody[i];
        int divider = melody[i + 1];

        int duration = (divider > 0)
            ? wholenote / divider
            : (wholenote / abs(divider)) * 1.5;

        if (note == REST) {
            delay(duration);
        } else {
            softTone(note, duration);
            delay(12);
        }
    }
}