#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include <Arduino.h>

void initSerialProtocol();
void processSerialProtocol();
void sendKeyState(uint8_t row, uint8_t col, bool isPressed);
void sendLayerChange(uint8_t layer);

#endif // SERIAL_PROTOCOL_H