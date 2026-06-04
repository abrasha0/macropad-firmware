#include "macro_handler.h"
#include "keyboard_sender.h"
#include "serial_protocol.h"
#include <ArduinoJson.h>

Macro macros[MAX_MACROS];
uint8_t macroCount = 0;

static bool macroRunning = false;
static uint8_t activeMacroIndex = 0;
static uint8_t activeMacroStep = 0;
static uint16_t currentDelay = 0;
static unsigned long stepStartTime = 0;

void initMacros() {
    macroRunning = false;
    activeMacroStep = 0;
    currentDelay = 0;
}

bool loadMacros(JsonDocument& doc) {
    JsonArray macrosArray = doc["macros"];
    macroCount = 0;

    if (!macrosArray.isNull()) {
        for (uint8_t i = 0; i < MAX_MACROS && i < macrosArray.size(); i++) {
            JsonObject macroObj = macrosArray[i];
            if (macroObj.isNull()) continue;

            const char* name = macroObj["name"] | "";
            strncpy(macros[macroCount].name, name, sizeof(macros[macroCount].name) - 1);
            macros[macroCount].name[sizeof(macros[macroCount].name) - 1] = '\0';

            JsonArray steps = macroObj["steps"];
            uint8_t stepIdx = 0;
            for (uint8_t s = 0; s < MAX_MACRO_STEPS && s < steps.size(); s++) {
                JsonObject stepObj = steps[s];
                if (stepObj.isNull()) continue;

                const char* text = stepObj["text"] | "";
                if (text[0] != '\0') {
                    macros[macroCount].steps[stepIdx].keycode = 0;
                    macros[macroCount].steps[stepIdx].delayMs = stepObj["delay"] | 0;
                    strncpy(macros[macroCount].steps[stepIdx].text, text, MAX_MACRO_TEXT - 1);
                    macros[macroCount].steps[stepIdx].text[MAX_MACRO_TEXT - 1] = '\0';
                } else {
                    macros[macroCount].steps[stepIdx].keycode = stepObj["keycode"] | 0;
                    macros[macroCount].steps[stepIdx].delayMs = stepObj["delay"] | 0;
                    macros[macroCount].steps[stepIdx].text[0] = '\0';
                }
                stepIdx++;
            }
            macros[macroCount].stepCount = stepIdx;
            macroCount++;
        }
    }

    return true;
}

void saveMacros(JsonDocument& doc) {
    JsonArray macrosArray = doc["macros"].to<JsonArray>();

    for (uint8_t i = 0; i < macroCount; i++) {
        JsonObject macroObj = macrosArray.add<JsonObject>();
        macroObj["name"] = macros[i].name;
        JsonArray steps = macroObj["steps"].to<JsonArray>();
        for (uint8_t s = 0; s < macros[i].stepCount; s++) {
            JsonObject stepObj = steps.add<JsonObject>();
            if (macros[i].steps[s].text[0] != '\0') {
                stepObj["text"] = macros[i].steps[s].text;
                stepObj["delay"] = macros[i].steps[s].delayMs;
            } else {
                stepObj["keycode"] = macros[i].steps[s].keycode;
                stepObj["delay"] = macros[i].steps[s].delayMs;
            }
        }
    }
}

void runMacro(uint8_t index) {
    if (index >= macroCount) {
        Serial.println("{\"cmd\":\"log\",\"level\":\"warn\",\"msg\":\"runMacro: invalid index\"}");
        return;
    }

    if (macros[index].stepCount == 0) {
        Serial.println("{\"cmd\":\"log\",\"level\":\"warn\",\"msg\":\"runMacro: empty macro\"}");
        return;
    }

    if (macroRunning) {
        Serial.println("{\"cmd\":\"log\",\"level\":\"warn\",\"msg\":\"runMacro: preempting running macro\"}");
        bleKeyboard.releaseAll();
    }

    activeMacroIndex = index;
    activeMacroStep = 0;
    currentDelay = 0;
    stepStartTime = millis();
    macroRunning = true;

    sendMacroProgress(activeMacroIndex, 0, macros[activeMacroIndex].stepCount);
}

void stopMacro() {
    macroRunning = false;
}

bool isMacroRunning() {
    return macroRunning;
}

void processMacro() {
    if (!macroRunning) return;

    Macro& m = macros[activeMacroIndex];

    unsigned long now = millis();
    if ((now - stepStartTime >= currentDelay) || (now < stepStartTime)) {
        if (activeMacroStep >= m.stepCount) {
            macroRunning = false;
            sendMacroProgress(activeMacroIndex, m.stepCount, m.stepCount);
            return;
        }

        MacroStep& step = m.steps[activeMacroStep];

        if (step.text[0] != '\0') {
            if (bleKeyboard.isConnected()) {
                bleKeyboard.print(step.text);
            }
        } else if (step.keycode != 0) {
            sendKeycode(step.keycode);
        }

        currentDelay = step.delayMs;
        activeMacroStep++;
        stepStartTime = millis();

        sendMacroProgress(activeMacroIndex, activeMacroStep, m.stepCount);

        if (activeMacroStep >= m.stepCount) {
            macroRunning = false;
            sendMacroProgress(activeMacroIndex, m.stepCount, m.stepCount);
        }
    }
}
