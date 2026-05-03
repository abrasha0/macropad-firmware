#include <Arduino.h>
#include <BleKeyboard.h>

#include "config_manager.h"
#include "matrix.h"
#include "serial_protocol.h"

BleKeyboard bleKeyboard("Makropad ESP32", "Custom", 100);

// External references — previousState is now correctly updated inside scanMatrix()
extern bool currentState[NUM_ROWS][NUM_COLS];
extern bool previousState[NUM_ROWS][NUM_COLS];

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Initializing macropad...\"}");

    if (!initConfig()) {
        Serial.println("{\"cmd\":\"log\",\"level\":\"error\",\"msg\":\"Config init failed! Using defaults.\"}");
    }

    initMatrix();

    Serial.println("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Starting BLE Keyboard...\"}");
    bleKeyboard.begin();

    initSerialProtocol();

    // Send initial layer state to WebUI
    sendLayerChange(currentLayer);

    Serial.println("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Setup complete.\"}");
}

void loop() {
    if (scanMatrix()) {
        // --- 1. Sprzętowy skrót zmiany profilu (Górny lewy + Górny prawy) ---
        static bool layerComboProcessed = false;
        if (currentState[0][0] && currentState[0][3]) {
            if (!layerComboProcessed) {
                currentLayer = (currentLayer + 1) % NUM_LAYERS;
                sendLayerChange(currentLayer);
                layerComboProcessed = true;
            }
            previousState[0][0] = currentState[0][0];
            previousState[0][3] = currentState[0][3];
        } else {
            layerComboProcessed = false;
        }

        // --- 2. Standardowe przetwarzanie matrycy ---
        for (int r = 0; r < NUM_ROWS; r++) {
            for (int c = 0; c < NUM_COLS; c++) {
                if (currentState[r][c] != previousState[r][c]) {
                    uint16_t keycode = keymap[currentLayer][r][c];
                    bool isPressed = currentState[r][c];

                    // Zawsze wysyłaj stan klawisza do WebUI, nawet jeśli pusty
                    sendKeyState(r, c, isPressed);

                    if (keycode == 0) continue;

                    uint8_t type = (keycode & 0xF000) >> 12;
                    uint8_t mods = (keycode & 0x0F00) >> 8;
                    uint8_t key  = (keycode & 0x00FF);

                    // A. Zmiana warstw przypisana do konkretnego klawisza (np. L1)
                    if (type == 3) {
                        if (isPressed && key < NUM_LAYERS) {
                            currentLayer = key;
                            sendLayerChange(currentLayer);
                        }
                    } 
                    // B. Rozszerzone Klawisze Multimedialne
                    else if (type == 1 && bleKeyboard.isConnected()) {
                        if (isPressed) {
                            switch(key) {
                                case 1: bleKeyboard.write(KEY_MEDIA_VOLUME_UP); break;
                                case 2: bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN); break;
                                case 3: bleKeyboard.write(KEY_MEDIA_MUTE); break;
                                case 4: bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE); break;
                                case 5: bleKeyboard.write(KEY_MEDIA_NEXT_TRACK); break;
                                case 6: bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK); break;
                                case 7: bleKeyboard.write(KEY_MEDIA_STOP); break;
                                case 8: bleKeyboard.write(KEY_MEDIA_CALCULATOR); break;
                                case 9: bleKeyboard.write(KEY_MEDIA_WWW_HOME); break;
                            }
                        }
                    } 
                    // C. Standardowe klawisze i akordy
                    else if (type == 0 && bleKeyboard.isConnected()) {
                        if (isPressed) {
                            // Naciskanie
                            if (mods & 1) bleKeyboard.press(KEY_LEFT_CTRL);
                            if (mods & 2) bleKeyboard.press(KEY_LEFT_SHIFT);
                            if (mods & 4) bleKeyboard.press(KEY_LEFT_ALT);
                            if (mods & 8) bleKeyboard.press(KEY_LEFT_GUI);
                            if (key > 0)  bleKeyboard.press(key);
                        } else {
                            // Zwalnianie (KRYTYCZNA POPRAWKA: Puszczamy modyfikatory ZAWSZE)
                            if (key > 0)  bleKeyboard.release(key);
                            if (mods & 1) bleKeyboard.release(KEY_LEFT_CTRL);
                            if (mods & 2) bleKeyboard.release(KEY_LEFT_SHIFT);
                            if (mods & 4) bleKeyboard.release(KEY_LEFT_ALT);
                            if (mods & 8) bleKeyboard.release(KEY_LEFT_GUI);
                            
                            // Awaryjne zrzucenie wszystkich stanów, jeśli coś ugrzęzło
                            if (key == 0 && mods == 0) bleKeyboard.releaseAll(); 
                        }
                    }
                }
            }
        }
    }
    processSerialProtocol();
}