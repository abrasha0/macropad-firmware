#include "keyboard_sender.h"

extern BleKeyboard bleKeyboard;

void sendKeycode(uint16_t keycode) {
    if (keycode == 0) {
        // Already filtered by callers, but guard here too.
        return;
    }

    if (!bleKeyboard.isConnected()) {
        Serial.println("{\"cmd\":\"log\",\"level\":\"warning\",\"msg\":\"Encoder action ignored - BLE not connected\"}");
        return;
    }

    uint8_t type = (keycode & 0xF000) >> 12;
    uint8_t mods = (keycode & 0x0F00) >> 8;
    uint8_t key  = (keycode & 0x00FF);

    if (type == 0) {
        // Standard key with modifiers
        // Press modifiers and key
        if (mods & 1) bleKeyboard.press(KEY_LEFT_CTRL);
        if (mods & 2) bleKeyboard.press(KEY_LEFT_SHIFT);
        if (mods & 4) bleKeyboard.press(KEY_LEFT_ALT);
        if (mods & 8) bleKeyboard.press(KEY_LEFT_GUI);
        if (key > 0)  bleKeyboard.press(key);
        // Release all
        bleKeyboard.releaseAll();
    } else if (type == 1) {
        // Media key: send as a write (which presses and releases)
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
    // Note: type 3 (layer change) is not sent as a keyboard key, so we ignore it.
    // If we ever get a type 3 in the encoder actions, we do nothing.
}