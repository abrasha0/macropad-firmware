#ifndef KEYBOARD_SENDER_H
#define KEYBOARD_SENDER_H

#include <Arduino.h>
#include <BleKeyboard.h>
#include "config_manager.h"   // for currentLayer and NUM_LAYERS

extern BleKeyboard bleKeyboard;

void sendKeycode(uint16_t keycode);

#endif // KEYBOARD_SENDER_H