#include "encoder.h"
#include "config_manager.h"
#include "keyboard_sender.h"
#include <esp_timer.h>

// --- Rotary encoder (ISR with temporal debounce + quadrature state machine) ---
volatile int16_t encoder_delta = 0;

// State machine: tracks the last valid (DT, CLK) pair to reject bounce transitions
static uint8_t  encoder_lastState = 0;
static uint64_t encoder_lastISRTime = 0;

void IRAM_ATTR encoderRotationISR() {
    // Temporal debounce: reject interrupts <300µs after previous.
    // Mechanical switch bounce is typically 5-20µs per event; this suppresses
    // rapid-fire bounce edges while allowing legitimate high-speed rotation.
    uint64_t now = esp_timer_get_time();
    if ((now - encoder_lastISRTime) < 300) return;
    encoder_lastISRTime = now;

    uint8_t clk = digitalRead(ENCODER_CLK_PIN);
    uint8_t dt  = digitalRead(ENCODER_DT_PIN);
    uint8_t state = (dt << 1) | clk;       // encode (DT, CLK) as 2-bit value

    // Quadrature state transition table.
    // Index = (oldState << 2) | newState
    //  +1 = valid CW step,  -1 = valid CCW step,  0 = invalid (bounce)
    static const int8_t enc_table[16] = {
         0,  1, -1,  0,   // 00→00, 00→01, 00→10, 00→11
        -1,  0,  0,  1,   // 01→00, 01→01, 01→10, 01→11
         1,  0,  0, -1,   // 10→00, 10→01, 10→10, 10→11
         0, -1,  1,  0    // 11→00, 11→01, 11→10, 11→11
    };

    int8_t step = enc_table[(encoder_lastState << 2) | state];
    encoder_lastState = state;               // always re-sync after every ISR

    if (step != 0) {
        encoder_delta += step;
    }
}

// --- Button (polled, no ISR — millis() is not safe in ESP32 ISR) ---
static bool   lastStableBtnState = HIGH;     // debounced state
static unsigned long btnDebounceStart = 0;   // timer for debounce window
static bool   btnDebouncing = false;         // are we inside a debounce window?
static bool   btnRawPrev = HIGH;             // previous raw reading for edge log

void initEncoder() {
    pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
    pinMode(ENCODER_DT_PIN,  INPUT_PULLUP);
    pinMode(ENCODER_SW_PIN,  INPUT_PULLUP);

    // Read initial pin state so the quadrature state machine starts in sync
    uint8_t clk = digitalRead(ENCODER_CLK_PIN);
    uint8_t dt  = digitalRead(ENCODER_DT_PIN);
    encoder_lastState = (dt << 1) | clk;

    // CHANGE on both pins → full quadrature decoding (4 edges per detent)
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), encoderRotationISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_DT_PIN),  encoderRotationISR, CHANGE);
}

void processEncoder() {
    // ---- 1. Rotation -------------------------------------------------------
    int16_t delta;
    noInterrupts();
    delta = encoder_delta;
    encoder_delta = 0;
    interrupts();

    if (delta != 0) {
        // State machine accumulates ±1 per valid quadrature transition.
        // With CHANGE on both pins, one full detent produces ±4.
        // Divide by 4 (integer truncation toward zero) to recover ±1/detent.
        int16_t steps = delta / 4;
        int16_t rem   = delta % 4;

        // Always carry forward the remainder so partial counts accumulate
        // across multiple processEncoder() calls until they make a full step.
        noInterrupts();
        encoder_delta += rem;
        interrupts();

        if (steps != 0) {
            encoder_action_t action = (steps > 0) ? encoder_right[currentLayer]
                                                  : encoder_left[currentLayer];

            char buf[96];
            snprintf(buf, sizeof(buf),
                "{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Encoder step: dir=%d, code=0x%04X\"}",
                steps > 0 ? 1 : -1, action);
            Serial.println(buf);

            if (action != 0) {
                sendKeycode(action);
            } else {
                Serial.println("{\"cmd\":\"log\",\"level\":\"warn\",\"msg\":\"Encoder step action is 0, nothing sent\"}");
            }
        }
    }

    // ---- 2. Button (edge‑triggered + lockout, no blocking) ----------------
    bool reading = digitalRead(ENCODER_SW_PIN);

    // Log on every raw edge (immediate feedback for debugging)
    if (reading != btnRawPrev) {
        btnRawPrev = reading;
        char buf[96];
        snprintf(buf, sizeof(buf),
            "{\"cmd\":\"log\",\"level\":\"debug\",\"msg\":\"Encoder btn raw edge: %s\"}",
            reading == LOW ? "PRESS" : "RELEASE");
        Serial.println(buf);
    }

    if (btnDebouncing) {
        // Lockout window — ignore all edges until it expires
        if ((millis() - btnDebounceStart) >= 20) {
            btnDebouncing = false;
            // After lockout, sync state with physical reading so that
            // a quick release+press during lockout doesn't desync the FSM.
            if (reading != lastStableBtnState) {
                lastStableBtnState = reading;
            }
        }
    } else {
        // Detect press (HIGH → LOW)
        if (reading == LOW && lastStableBtnState == HIGH) {
            lastStableBtnState = LOW;
            btnDebounceStart = millis();
            btnDebouncing = true;

            encoder_action_t action = encoder_button[currentLayer];

            // Log BEFORE sendKeycode so it always appears, even if BLE blocks.
            char buf[96];
            snprintf(buf, sizeof(buf),
                "{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Encoder btn PRESS (code=0x%04X)\"}",
                action);
            Serial.println(buf);

            if (action != 0) {
                sendKeycode(action);
            } else {
                Serial.println("{\"cmd\":\"log\",\"level\":\"warn\",\"msg\":\"Encoder btn action is 0, nothing sent\"}");
            }
        }
        // Detect release (LOW → HIGH)
        else if (reading == HIGH && lastStableBtnState == LOW) {
            // Log BEFORE state change for safety
            Serial.println("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Encoder btn RELEASE\"}");
            lastStableBtnState = HIGH;
            btnDebounceStart = millis();
            btnDebouncing = true;
        }
    }
}
