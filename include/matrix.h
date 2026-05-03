#ifndef MATRIX_H
#define MATRIX_H

#include <Arduino.h>

#define DEBOUNCE_DELAY 10

void initMatrix();
bool scanMatrix(); // Returns true if state changed
bool isKeyPressed(uint8_t row, uint8_t col);

#endif // MATRIX_H