#include "config_manager.h"

uint16_t keymap[NUM_LAYERS][NUM_ROWS][NUM_COLS] = {0};
uint8_t currentLayer = 0;

// Encoder actions per layer
uint16_t encoder_left[NUM_LAYERS] = {0};
uint16_t encoder_right[NUM_LAYERS] = {0};
uint16_t encoder_button[NUM_LAYERS] = {0};

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
        
        // Set default encoder actions for every layer:
        //   Left turn  (CCW): VOLUME_UP    (0x1001)
        //   Right turn (CW):  VOLUME_DOWN  (0x1002)
        //   Button press:      PLAY_PAUSE  (0x1004)
        for (int l = 0; l < NUM_LAYERS; l++) {
            encoder_left[l]   = 0x1001; // VOLUME_UP
            encoder_right[l]  = 0x1002; // VOLUME_DOWN
            encoder_button[l] = 0x1004; // PLAY_PAUSE
        }
        
        // Note: We are setting the encoder arrays directly. We could also set them via the keymap? No, they are separate.
        // We'll save the config, which will now include the encoder array.
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
        configFile.close();
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
    
    // Handle encoder array
    JsonArray encoderArray = doc["encoder"];
    bool needsSave = false;

    // Default encoder actions for every layer:
    //   Left turn  (CCW): VOLUME_UP    (0x1001)
    //   Right turn (CW):  VOLUME_DOWN  (0x1002)
    //   Button press:      PLAY_PAUSE  (0x1004)
    const uint16_t LEFT_DEF   = 0x1001;
    const uint16_t RIGHT_DEF  = 0x1002;
    const uint16_t BUTTON_DEF = 0x1004;

    if (encoderArray.isNull()) {
        // No encoder array in the config, set defaults for all layers and save
        for (int l = 0; l < NUM_LAYERS; l++) {
            encoder_left[l]   = LEFT_DEF;
            encoder_right[l]  = RIGHT_DEF;
            encoder_button[l] = BUTTON_DEF;
        }
        needsSave = true;
    } else {
        // Load the encoder array for each layer that exists
        for (int l = 0; l < NUM_LAYERS; l++) {
            if (l < encoderArray.size()) {
                JsonObject encoderObj = encoderArray[l];
                if (encoderObj.isNull()) {
                    encoder_left[l]   = LEFT_DEF;
                    encoder_right[l]  = RIGHT_DEF;
                    encoder_button[l] = BUTTON_DEF;
                } else {
                    encoder_left[l]   = encoderObj["left"] | 0;
                    encoder_right[l]  = encoderObj["right"] | 0;
                    encoder_button[l] = encoderObj["button"] | 0;
                }
            } else {
                // This layer doesn't have an entry in the encoder array, use defaults
                encoder_left[l]   = LEFT_DEF;
                encoder_right[l]  = RIGHT_DEF;
                encoder_button[l] = BUTTON_DEF;
            }
        }
        // If the encoderArray has more than NUM_LAYERS, we ignore the extra.
    }
    
    // Sanity check: if all encoder actions are 0 (e.g. from a previous buggy version),
    // re-initialize with defaults and save.
    bool allZero = true;
    for (int l = 0; l < NUM_LAYERS && allZero; l++) {
        if (encoder_left[l] != 0 || encoder_right[l] != 0 || encoder_button[l] != 0) {
            allZero = false;
        }
    }
    if (allZero) {
        for (int l = 0; l < NUM_LAYERS; l++) {
            encoder_left[l]   = LEFT_DEF;
            encoder_right[l]  = RIGHT_DEF;
            encoder_button[l] = BUTTON_DEF;
        }
        needsSave = true;
    }
    
    configFile.close();
    
    // If we had to set any defaults (because the encoder part was missing or incomplete), save the config to add the encoder part.
    if (needsSave) {
        // We don't want to call saveKeymap() recursively? Actually, we are not in saveKeymap() yet.
        // But note: saveKeymap() will write the entire config, including the encoder part we just set.
        return saveKeymap();
    }
    
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
    
    // Add encoder array
    JsonArray encoderArray = doc["encoder"].to<JsonArray>();
    for(int l = 0; l < NUM_LAYERS; l++) {
        JsonObject encoderObj = encoderArray.add<JsonObject>();
        encoderObj["left"]   = encoder_left[l];
        encoderObj["right"]  = encoder_right[l];
        encoderObj["button"] = encoder_button[l];
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