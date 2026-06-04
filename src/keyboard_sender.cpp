#include "keyboard_sender.h"
#include "macro_handler.h"
#include "serial_protocol.h"

extern BleKeyboard bleKeyboard;

void sendKeycode(uint16_t keycode) {
    if (keycode == 0) return;

    uint8_t type = (keycode & 0xF000) >> 12;
    uint8_t mods = (keycode & 0x0F00) >> 8;
    uint8_t key  = (keycode & 0x00FF);

    if (type == 0) {
        if (!bleKeyboard.isConnected()) return;

        if (mods & 1) bleKeyboard.press(KEY_LEFT_CTRL);
        if (mods & 2) bleKeyboard.press(KEY_LEFT_SHIFT);
        if (mods & 4) bleKeyboard.press(KEY_LEFT_ALT);
        if (mods & 8) bleKeyboard.press(KEY_LEFT_GUI);
        if (key > 0)  bleKeyboard.press(key);
        bleKeyboard.releaseAll();

    } else if (type == 1) {
        if (!bleKeyboard.isConnected()) return;

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

    } else if (type == 2) {
        if (key < macroCount) {
            char buf[96];
            snprintf(buf, sizeof(buf),
                "{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Trigger macro #%d: %s\"}",
                key, macros[key].name);
            Serial.println(buf);
            runMacro(key);
        }

    } else if (type == 3) {
        if (key < NUM_LAYERS) {
            bleKeyboard.releaseAll();
            currentLayer = key;
            sendLayerChange(currentLayer);
        }
    }
}
