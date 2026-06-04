#include <Arduino.h>
#include <BleKeyboard.h>

#include "config_manager.h"
#include "matrix.h"
#include "serial_protocol.h"
#include "encoder.h"
#include "keyboard_sender.h"
#include "macro_handler.h"
#include "binding.h"

BleKeyboard bleKeyboard("Makropad ESP32", "Custom", 100);

extern bool currentState[NUM_ROWS][NUM_COLS];
extern bool previousState[NUM_ROWS][NUM_COLS];

static KeyBinding pressedBinding[NUM_ROWS][NUM_COLS] = {};

static void clearPressedKeys() {
    for (int r = 0; r < NUM_ROWS; r++)
        for (int c = 0; c < NUM_COLS; c++)
            pressedBinding[r][c] = {};
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Initializing macropad...\"}");

    if (!initConfig()) {
        Serial.println("{\"cmd\":\"log\",\"level\":\"error\",\"msg\":\"Config init failed! Using defaults.\"}");
    }

    initMacros();
    initMatrix();

    initEncoder();

    Serial.println("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Starting BLE Keyboard...\"}");
    bleKeyboard.begin();

    initSerialProtocol();

    sendLayerChange(currentLayer);

    Serial.println("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Setup complete.\"}");
}

void loop() {
    bool matrixChanged = scanMatrix();

    if (matrixChanged) {
        static bool layerComboProcessed = false;
        bool comboActive = currentState[0][0] && currentState[0][3];
        if (comboActive) {
            if (!layerComboProcessed) {
                bleKeyboard.releaseAll();
                clearPressedKeys();
                currentLayer = (currentLayer + 1) % NUM_LAYERS;
                sendLayerChange(currentLayer);
                layerComboProcessed = true;
            }
        } else {
            layerComboProcessed = false;
        }

        for (int r = 0; r < NUM_ROWS; r++) {
            for (int c = 0; c < NUM_COLS; c++) {
                if (currentState[r][c] != previousState[r][c]) {
                    bool isPressed = currentState[r][c];
                    KeyBinding binding;

                    if (isPressed) {
                        binding = keymap[currentLayer][r][c];
                    } else {
                        binding = pressedBinding[r][c];
                        pressedBinding[r][c] = {};
                    }

                    sendKeyState(r, c, isPressed);

                    if (binding.type == BIND_NONE &&
                        binding.text[0] == '\0' &&
                        binding.mods == 0 &&
                        binding.key == 0) continue;

                    if (comboActive && r == 0 && (c == 0 || c == 3)) continue;

                    if (isPressed) {
                        pressedBinding[r][c] = binding;
                    }

                    executeBinding(&binding, isPressed);
                }
            }
        }
    }

    processEncoder();

    processMacro();

    processSerialProtocol();
}
