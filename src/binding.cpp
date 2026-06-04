#include "binding.h"
#include "macro_handler.h"
#include "keyboard_sender.h"
#include "serial_protocol.h"
#include "config_manager.h"

extern BleKeyboard bleKeyboard;

void executeBinding(const KeyBinding* binding, bool isPressed) {
    if (!binding || binding->type == BIND_NONE) return;

    switch (binding->type) {
        case BIND_KEY: {
            if (!bleKeyboard.isConnected()) return;
            uint8_t mods = binding->mods;
            uint8_t key = binding->key;
            if (isPressed) {
                if (mods & 1) bleKeyboard.press(KEY_LEFT_CTRL);
                if (mods & 2) bleKeyboard.press(KEY_LEFT_SHIFT);
                if (mods & 4) bleKeyboard.press(KEY_LEFT_ALT);
                if (mods & 8) bleKeyboard.press(KEY_LEFT_GUI);
                if (key > 0)  bleKeyboard.press(key);
            } else {
                if (key > 0)  bleKeyboard.release(key);
                if (mods & 1) bleKeyboard.release(KEY_LEFT_CTRL);
                if (mods & 2) bleKeyboard.release(KEY_LEFT_SHIFT);
                if (mods & 4) bleKeyboard.release(KEY_LEFT_ALT);
                if (mods & 8) bleKeyboard.release(KEY_LEFT_GUI);
                if (key == 0 && mods == 0) bleKeyboard.releaseAll();
            }
            break;
        }
        case BIND_MEDIA: {
            if (!isPressed || !bleKeyboard.isConnected()) return;
            switch (binding->key) {
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
            break;
        }
        case BIND_MACRO: {
            if (!isPressed) return;
            if (binding->key < macroCount && macros[binding->key].stepCount > 0) {
                char buf[96];
                snprintf(buf, sizeof(buf),
                    "{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"Trigger macro #%d: %s\"}",
                    binding->key, macros[binding->key].name);
                Serial.println(buf);
                runMacro(binding->key);
            }
            break;
        }
        case BIND_LAYER: {
            if (!isPressed) return;
            if (binding->key < NUM_LAYERS) {
                bleKeyboard.releaseAll();
                currentLayer = binding->key;
                sendLayerChange(currentLayer);
            }
            break;
        }
        case BIND_STRING: {
            if (!isPressed || !bleKeyboard.isConnected()) return;
            if (binding->text[0] != '\0') {
                bleKeyboard.print(binding->text);
            }
            break;
        }
    }
}
