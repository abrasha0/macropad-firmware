#ifndef MACRO_HANDLER_H
#define MACRO_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define MAX_MACROS 16
#define MAX_MACRO_STEPS 32
#define MAX_MACRO_TEXT 64

struct MacroStep {
    uint16_t keycode;
    uint16_t delayMs;
    char text[MAX_MACRO_TEXT];
};

struct Macro {
    char name[32];
    uint8_t stepCount;
    MacroStep steps[MAX_MACRO_STEPS];
};

extern Macro macros[MAX_MACROS];
extern uint8_t macroCount;

void initMacros();
bool loadMacros(JsonDocument& doc);
void saveMacros(JsonDocument& doc);
void runMacro(uint8_t index);
void stopMacro();
bool isMacroRunning();
void processMacro();

#endif
