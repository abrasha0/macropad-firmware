#ifndef BINDING_H
#define BINDING_H

#include <Arduino.h>

#define MAX_KEY_TEXT 64

enum BindingType : uint8_t {
    BIND_NONE = 0,
    BIND_KEY,
    BIND_MEDIA,
    BIND_MACRO,
    BIND_LAYER,
    BIND_STRING,
};

struct KeyBinding {
    uint8_t type;
    uint8_t mods;
    uint8_t key;
    char text[MAX_KEY_TEXT];

    KeyBinding() : type(BIND_NONE), mods(0), key(0) {
        text[0] = '\0';
    }

    KeyBinding(uint8_t t, uint8_t m, uint8_t k, const char* s) : type(t), mods(m), key(k) {
        strncpy(text, s, MAX_KEY_TEXT - 1);
        text[MAX_KEY_TEXT - 1] = '\0';
    }
};

void executeBinding(const KeyBinding* binding, bool isPressed);

#endif
