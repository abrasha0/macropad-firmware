#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#define NUM_LAYERS 5
#define NUM_ROWS 4
#define NUM_COLS 4

extern uint16_t keymap[NUM_LAYERS][NUM_ROWS][NUM_COLS];
extern uint8_t currentLayer;

bool initConfig();
bool loadKeymap();
bool saveKeymap();
bool processConfigJson(const String& json);
String getConfigJson();

#endif // CONFIG_MANAGER_H