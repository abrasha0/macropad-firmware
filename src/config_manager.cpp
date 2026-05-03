#include "config_manager.h"

uint16_t keymap[NUM_LAYERS][NUM_ROWS][NUM_COLS] = {0};
uint8_t currentLayer = 0;

const char* configFilePath = "/config.json";

bool initConfig() {
    if (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return false;
    }
    
    if (!LittleFS.exists(configFilePath) || !loadKeymap()) {
        Serial.println("Config file missing or invalid, creating default 5-layer config...");
        for(int l=0; l<NUM_LAYERS; l++) {
            for(int r=0; r<NUM_ROWS; r++) {
                for(int c=0; c<NUM_COLS; c++) {
                    keymap[l][r][c] = 'a' + (r * NUM_COLS + c) + (l * 16);
                }
            }
        }
        
        keymap[0][0][0] = 0x3000; 
        saveKeymap();
    }
    return true;
}

bool loadKeymap() {
    File configFile = LittleFS.open(configFilePath, "r");
    if (!configFile) {
        Serial.println("Failed to open config file for reading");
        return false;
    }

    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buf.get());
    if (error) {
        Serial.println("Failed to parse config file");
        return false;
    }
    
    JsonArray layers = doc["layers"];
    for(int l = 0; l < NUM_LAYERS && l < layers.size(); l++) {
        JsonArray rows = layers[l];
        for(int r = 0; r < NUM_ROWS && r < rows.size(); r++) {
            JsonArray cols = rows[r];
            for(int c = 0; c < NUM_COLS && c < cols.size(); c++) {
                keymap[l][r][c] = cols[c].as<uint16_t>();
            }
        }
    }
    
    configFile.close();
    return true;
}

bool saveKeymap() {
    JsonDocument doc;
    JsonArray layers = doc["layers"].to<JsonArray>();
    
    for(int l = 0; l < NUM_LAYERS; l++) {
        JsonArray rowArray = layers.add<JsonArray>();
        for(int r = 0; r < NUM_ROWS; r++) {
            JsonArray colArray = rowArray.add<JsonArray>();
            for(int c = 0; c < NUM_COLS; c++) {
                colArray.add(keymap[l][r][c]);
            }
        }
    }
    
    File configFile = LittleFS.open(configFilePath, "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return false;
    }
    
    serializeJson(doc, configFile);
    configFile.close();
    return true;
}

bool processConfigJson(const String& jsonString) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    if(error){
        return false;
    }
    
    JsonArray layers = doc["layers"];
    for(int l = 0; l < NUM_LAYERS && l < layers.size(); l++) {
        JsonArray rows = layers[l];
        for(int r = 0; r < NUM_ROWS && r < rows.size(); r++) {
            JsonArray cols = rows[r];
            for(int c = 0; c < NUM_COLS && c < cols.size(); c++) {
                keymap[l][r][c] = cols[c].as<uint16_t>();
            }
        }
    }
    return saveKeymap();
}

String getConfigJson() {
    File configFile = LittleFS.open(configFilePath, "r");
    if (!configFile) return "{}";
    String res = configFile.readString();
    configFile.close();
    return res;
}