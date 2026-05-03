#include "matrix.h"
#include "config_manager.h"

const uint8_t rowPins[NUM_ROWS] = {16, 17, 18, 19};
const uint8_t colPins[NUM_COLS] = {32, 33, 25, 26};

bool currentState[NUM_ROWS][NUM_COLS] = {false};
bool previousState[NUM_ROWS][NUM_COLS] = {false};

// Raw reading before debounce
static bool rawState[NUM_ROWS][NUM_COLS] = {false};
static unsigned long lastDebounceTime[NUM_ROWS][NUM_COLS] = {0};

void initMatrix() {
    for (int r = 0; r < NUM_ROWS; r++) {
        pinMode(rowPins[r], OUTPUT);
        digitalWrite(rowPins[r], HIGH);
    }
    for (int c = 0; c < NUM_COLS; c++) {
        pinMode(colPins[c], INPUT_PULLUP);
    }
}

bool scanMatrix() {
    bool stateChanged = false;
    unsigned long now = millis();

    // Snapshot entire previous state BEFORE scanning.
    // This ensures main.cpp's diff (currentState != previousState) is always
    // a clean before/after for THIS scan cycle — no phantom re-fires from
    // keys that changed in an earlier cycle.
    for (int r = 0; r < NUM_ROWS; r++)
        for (int c = 0; c < NUM_COLS; c++)
            previousState[r][c] = currentState[r][c];

    for (int r = 0; r < NUM_ROWS; r++) {
        digitalWrite(rowPins[r], LOW);
        delayMicroseconds(10);

        for (int c = 0; c < NUM_COLS; c++) {
            bool reading = (digitalRead(colPins[c]) == LOW);

            // Restart debounce timer only when raw reading changes
            if (reading != rawState[r][c]) {
                rawState[r][c] = reading;
                lastDebounceTime[r][c] = now;
            }

            // Commit stable reading after debounce window
            if ((now - lastDebounceTime[r][c]) > DEBOUNCE_DELAY) {
                if (reading != currentState[r][c]) {
                    currentState[r][c] = reading;
                    stateChanged = true;
                }
            }
        }

        digitalWrite(rowPins[r], HIGH);
    }

    return stateChanged;
}

bool isKeyPressed(uint8_t row, uint8_t col) {
    if (row >= NUM_ROWS || col >= NUM_COLS) return false;
    return currentState[row][col];
}