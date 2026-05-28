#include "config_manager.h"

uint16_t keymap[NUM_LAYERS][NUM_ROWS][NUM_COLS] = {0};
uint8_t currentLayer = 0;

// Encoder actions per layer
uint16_t encoder_left[NUM_LAYERS] = {0};
uint16_t encoder_right[NUM_LAYERS] = {0};
uint16_t encoder_button[NUM_LAYERS] = {0};

const char* configFilePath = "/config.json";

static void setDefaultProfiles() {
    // --- Layer 0: Numpad ---
    // KP_7(0x5F)  KP_8(0x60)  KP_9(0x61)  KP_DIVIDE(0x54)
    // KP_4(0x5C)  KP_5(0x5D)  KP_6(0x5E)  KP_MULTIPLY(0x55)
    // KP_1(0x59)  KP_2(0x5A)  KP_3(0x5B)  KP_SUBTRACT(0x56)
    // KP_0(0x62)  KP_DOT(0x63) KP_ENTER(0x58) KP_ADD(0x57)
    keymap[0][0][0] = 0x005F; keymap[0][0][1] = 0x0060; keymap[0][0][2] = 0x0061; keymap[0][0][3] = 0x0054;
    keymap[0][1][0] = 0x005C; keymap[0][1][1] = 0x005D; keymap[0][1][2] = 0x005E; keymap[0][1][3] = 0x0055;
    keymap[0][2][0] = 0x0059; keymap[0][2][1] = 0x005A; keymap[0][2][2] = 0x005B; keymap[0][2][3] = 0x0056;
    keymap[0][3][0] = 0x0062; keymap[0][3][1] = 0x0063; keymap[0][3][2] = 0x0058; keymap[0][3][3] = 0x0057;

    // --- Layer 1: NFS Underground 2 ---
    // Up(0x52)  Down(0x51)  Left(0x50)  Right(0x4F)
    // Shift(0x0200)  Ctrl(0x0100)  Space(0x2C)  Enter(0x28)
    // W(0x1A)  S(0x16)  A(0x04)  D(0x07)
    // 1(0x1E)  2(0x1F)  3(0x20)  Esc(0x29)
    keymap[1][0][0] = 0x0052; keymap[1][0][1] = 0x0051; keymap[1][0][2] = 0x0050; keymap[1][0][3] = 0x004F;
    keymap[1][1][0] = 0x0200; keymap[1][1][1] = 0x0100; keymap[1][1][2] = 0x002C; keymap[1][1][3] = 0x0028;
    keymap[1][2][0] = 0x001A; keymap[1][2][1] = 0x0016; keymap[1][2][2] = 0x0004; keymap[1][2][3] = 0x0007;
    keymap[1][3][0] = 0x001E; keymap[1][3][1] = 0x001F; keymap[1][3][2] = 0x0020; keymap[1][3][3] = 0x0029;

    // --- Layer 2: CS:GO / CS2 ---
    // 1(0x1E)  2(0x1F)  3(0x20)  4(0x21)
    // 5(0x22)  B(0x05)  G(0x0A)  R(0x15)
    // Z(0x1D)  X(0x1B)  C(0x06)  V(0x19)
    // Q(0x14)  E(0x08)  F(0x09)  Ctrl walk(0x0100)
    keymap[2][0][0] = 0x001E; keymap[2][0][1] = 0x001F; keymap[2][0][2] = 0x0020; keymap[2][0][3] = 0x0021;
    keymap[2][1][0] = 0x0022; keymap[2][1][1] = 0x0005; keymap[2][1][2] = 0x000A; keymap[2][1][3] = 0x0015;
    keymap[2][2][0] = 0x001D; keymap[2][2][1] = 0x001B; keymap[2][2][2] = 0x0006; keymap[2][2][3] = 0x0019;
    keymap[2][3][0] = 0x0014; keymap[2][3][1] = 0x0008; keymap[2][3][2] = 0x0009; keymap[2][3][3] = 0x0100;

    // --- Layer 3: Custom 1 (empty) ---
    for (int r = 0; r < NUM_ROWS; r++)
        for (int c = 0; c < NUM_COLS; c++)
            keymap[3][r][c] = 0;

    // --- Layer 4: Custom 2 (empty) ---
    for (int r = 0; r < NUM_ROWS; r++)
        for (int c = 0; c < NUM_COLS; c++)
            keymap[4][r][c] = 0;
}

bool initConfig() {
    if (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return false;
    }

    if (!LittleFS.exists(configFilePath) || !loadKeymap()) {
        Serial.println("Config file missing or invalid, creating default 5-layer config...");

        setDefaultProfiles();

        // Set default encoder actions for every layer:
        //   Left turn  (CCW): VOLUME_UP    (0x1001)
        //   Right turn (CW):  VOLUME_DOWN  (0x1002)
        //   Button press:      PLAY_PAUSE  (0x1004)
        for (int l = 0; l < NUM_LAYERS; l++) {
            encoder_left[l]   = 0x1001;
            encoder_right[l]  = 0x1002;
            encoder_button[l] = 0x1004;
        }

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
    
    // Load macros and strings
    loadMacros(doc);

    configFile.close();

    if (needsSave) {
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

    // Save macros and strings
    saveMacros(doc);

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

    // Load encoder actions
    JsonArray encoderArray = doc["encoder"];
    if (!encoderArray.isNull()) {
        for (int l = 0; l < NUM_LAYERS && l < encoderArray.size(); l++) {
            JsonObject encoderObj = encoderArray[l];
            if (!encoderObj.isNull()) {
                encoder_left[l]   = encoderObj["left"] | encoder_left[l];
                encoder_right[l]  = encoderObj["right"] | encoder_right[l];
                encoder_button[l] = encoderObj["button"] | encoder_button[l];
            }
        }
    }

    // Load macros and strings
    loadMacros(doc);

    return saveKeymap();
}

String getConfigJson() {
    File configFile = LittleFS.open(configFilePath, "r");
    if (!configFile) return "{}";
    String res = configFile.readString();
    configFile.close();
    return res;
}