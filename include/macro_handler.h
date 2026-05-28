#ifndef MACRO_HANDLER_H
#define MACRO_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define MAX_MACROS 32
#define MAX_MACRO_STEPS 64
#define MAX_STRINGS 16
#define MAX_STRING_LEN 128

struct MacroStep {
    uint16_t keycode;
    uint16_t delayMs;
};

struct Macro {
    char name[32];
    uint8_t stepCount;
    MacroStep steps[MAX_MACRO_STEPS];
};

extern Macro macros[MAX_MACROS];
extern uint8_t macroCount;
extern char strings[MAX_STRINGS][MAX_STRING_LEN];
extern uint8_t stringCount;

void initMacros();
bool loadMacros(JsonDocument& doc);
void saveMacros(JsonDocument& doc);
void runMacro(uint8_t index);
void stopMacro();
bool isMacroRunning();
void processMacro();

#endif
