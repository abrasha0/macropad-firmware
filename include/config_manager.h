#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "binding.h"
#include "macro_handler.h"

#define NUM_LAYERS 5
#define NUM_ROWS 4
#define NUM_COLS 4

#define CONFIG_PATH "/config.json"
#define CONFIG_TEMP_PATH "/config.tmp"
#define CONFIG_BAK_PATH "/config.bak"

extern KeyBinding keymap[NUM_LAYERS][NUM_ROWS][NUM_COLS];
extern uint8_t currentLayer;

extern KeyBinding encoder_left[NUM_LAYERS];
extern KeyBinding encoder_right[NUM_LAYERS];
extern KeyBinding encoder_button[NUM_LAYERS];

bool initConfig();
bool loadKeymap(const char* path = CONFIG_PATH);
bool saveKeymap();
bool processConfigJson(const String& json);
String getConfigJson();

#endif // CONFIG_MANAGER_H