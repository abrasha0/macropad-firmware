#include "encoder.h"
#include "config_manager.h"
#include "binding.h"
#include <esp_timer.h>

volatile int16_t encoder_delta = 0;

static uint8_t  encoder_lastState = 0;
static uint64_t encoder_lastISRTime = 0;

void IRAM_ATTR encoderRotationISR() {
    uint64_t now = esp_timer_get_time();
    if ((now - encoder_lastISRTime) < 300) return;
    encoder_lastISRTime = now;

    uint8_t clk = digitalRead(ENCODER_CLK_PIN);
    uint8_t dt  = digitalRead(ENCODER_DT_PIN);
    uint8_t state = (dt << 1) | clk;

    static const int8_t enc_table[16] = {
          0,  1, -1,  0,
         -1,  0,  0,  1,
          1,  0,  0, -1,
          0, -1,  1,  0
    };

    int8_t step = enc_table[(encoder_lastState << 2) | state];
    encoder_lastState = state;

    if (step != 0) {
        encoder_delta += step;
    }
}

static bool   lastStableBtnState = HIGH;
static unsigned long btnDebounceStart = 0;
static bool   btnDebouncing = false;
static bool   btnRawPrev = HIGH;
static bool   btnReleaseDuringLockout = false;

void initEncoder() {
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN,  INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN,  INPUT_PULLUP);

    uint8_t clk = digitalRead(ENCODER_CLK_PIN);
    uint8_t dt  = digitalRead(ENCODER_DT_PIN);
    encoder_lastState = (dt << 1) | clk;

    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), encoderRotationISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN),  encoderRotationISR, CHANGE);
}

void processEncoder() {
    int16_t delta;
    noInterrupts();
    delta = encoder_delta;
    encoder_delta = 0;
    interrupts();

    if (delta != 0) {
        int16_t steps = delta / 4;
        int16_t rem   = delta % 4;

        noInterrupts();
        encoder_delta += rem;
        interrupts();

        if (steps != 0) {
            const KeyBinding* action = (steps > 0) ? &encoder_right[currentLayer]
                                                   : &encoder_left[currentLayer];

            char buf[96];
            snprintf(buf, sizeof(buf),
                "{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Encoder step: dir=%d, type=%d\"}",
                steps > 0 ? 1 : -1, action->type);
            Serial.println(buf);

            if (action->type != BIND_NONE) {
                executeBinding(action, true);
            } else {
                Serial.println("{\"cmd\":\"log\",\"level\":\"warn\",\"msg\":\"Encoder step action is empty, nothing sent\"}");
            }
        }
    }

    bool reading = digitalRead(ENCODER_SW_PIN);

    if (reading != btnRawPrev) {
        btnRawPrev = reading;
        char buf[96];
        snprintf(buf, sizeof(buf),
            "{\"cmd\":\"log\",\"level\":\"debug\",\"msg\":\"Encoder btn raw edge: %s\"}",
            reading == LOW ? "PRESS" : "RELEASE");
        Serial.println(buf);
    }

    if (btnDebouncing) {
        if (reading == HIGH && btnRawPrev == LOW) {
            btnReleaseDuringLockout = true;
        }

        if ((millis() - btnDebounceStart) >= 20) {
            btnDebouncing = false;

            if (btnReleaseDuringLockout && reading == LOW && lastStableBtnState == LOW) {
                lastStableBtnState = HIGH;
            } else if (reading != lastStableBtnState) {
                lastStableBtnState = reading;
            }
            btnReleaseDuringLockout = false;
        }
    } else {
        if (reading == LOW && lastStableBtnState == HIGH) {
            lastStableBtnState = LOW;
            btnDebounceStart = millis();
            btnDebouncing = true;

            const KeyBinding* action = &encoder_button[currentLayer];

            char buf[96];
            snprintf(buf, sizeof(buf),
                "{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Encoder btn PRESS (type=%d)\"}",
                action->type);
            Serial.println(buf);

            if (action->type != BIND_NONE) {
                executeBinding(action, true);
            } else {
                Serial.println("{\"cmd\":\"log\",\"level\":\"warn\",\"msg\":\"Encoder btn action is empty, nothing sent\"}");
            }
        }
        else if (reading == HIGH && lastStableBtnState == LOW) {
            Serial.println("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Encoder btn RELEASE\"}");
            lastStableBtnState = HIGH;
            btnDebounceStart = millis();
            btnDebouncing = true;
        }
    }
}
