#include "config_manager.h"

KeyBinding keymap[NUM_LAYERS][NUM_ROWS][NUM_COLS] = {};
uint8_t currentLayer = 0;

KeyBinding encoder_left[NUM_LAYERS] = {};
KeyBinding encoder_right[NUM_LAYERS] = {};
KeyBinding encoder_button[NUM_LAYERS] = {};

const char* configFilePath = "/config.json";

static void parseKeyBinding(JsonVariant src, KeyBinding& dst) {
    dst = {};
    if (!src.is<JsonObject>()) return;

    JsonObject obj = src.as<JsonObject>();
    uint8_t t = obj["t"] | BIND_NONE;
    dst.type = t;

    switch (t) {
        case BIND_KEY:
            dst.mods = obj["m"] | 0;
            dst.key  = obj["k"] | 0;
            break;
        case BIND_MEDIA:
        case BIND_MACRO:
        case BIND_LAYER:
            dst.key = obj["k"] | 0;
            break;
        case BIND_STRING: {
            const char* s = obj["s"] | "";
            strncpy(dst.text, s, MAX_KEY_TEXT - 1);
            dst.text[MAX_KEY_TEXT - 1] = '\0';
            break;
        }
    }
}

static void serializeKeyBinding(JsonVariant dst, const KeyBinding& src) {
    if (src.type == BIND_NONE && src.text[0] == '\0' && src.mods == 0 && src.key == 0) {
        dst.set(0);
        return;
    }
    JsonObject obj = dst.to<JsonObject>();
    obj["t"] = src.type;

    switch (src.type) {
        case BIND_KEY:
            if (src.mods) obj["m"] = src.mods;
            if (src.key)  obj["k"] = src.key;
            break;
        case BIND_STRING:
            obj["s"] = src.text;
            break;
        default:
            if (src.key) obj["k"] = src.key;
            break;
    }
}

static void setDefaultProfiles() {
    // --- Layer 0: Numpad ---
    keymap[0][0][0] = {BIND_KEY, 0, 0x5F, ""};
    keymap[0][0][1] = {BIND_KEY, 0, 0x60, ""};
    keymap[0][0][2] = {BIND_KEY, 0, 0x61, ""};
    keymap[0][0][3] = {BIND_KEY, 0, 0x54, ""};
    keymap[0][1][0] = {BIND_KEY, 0, 0x5C, ""};
    keymap[0][1][1] = {BIND_KEY, 0, 0x5D, ""};
    keymap[0][1][2] = {BIND_KEY, 0, 0x5E, ""};
    keymap[0][1][3] = {BIND_KEY, 0, 0x55, ""};
    keymap[0][2][0] = {BIND_KEY, 0, 0x59, ""};
    keymap[0][2][1] = {BIND_KEY, 0, 0x5A, ""};
    keymap[0][2][2] = {BIND_KEY, 0, 0x5B, ""};
    keymap[0][2][3] = {BIND_KEY, 0, 0x56, ""};
    keymap[0][3][0] = {BIND_KEY, 0, 0x62, ""};
    keymap[0][3][1] = {BIND_KEY, 0, 0x63, ""};
    keymap[0][3][2] = {BIND_KEY, 0, 0x58, ""};
    keymap[0][3][3] = {BIND_KEY, 0, 0x57, ""};

    // --- Layer 1: NFS Underground 2 ---
    keymap[1][0][0] = {BIND_KEY, 0, 0x52, ""};
    keymap[1][0][1] = {BIND_KEY, 0, 0x51, ""};
    keymap[1][0][2] = {BIND_KEY, 0, 0x50, ""};
    keymap[1][0][3] = {BIND_KEY, 0, 0x4F, ""};
    keymap[1][1][0] = {BIND_KEY, 2, 0x00, ""};
    keymap[1][1][1] = {BIND_KEY, 1, 0x00, ""};
    keymap[1][1][2] = {BIND_KEY, 0, 0x2C, ""};
    keymap[1][1][3] = {BIND_KEY, 0, 0x28, ""};
    keymap[1][2][0] = {BIND_KEY, 0, 0x1A, ""};
    keymap[1][2][1] = {BIND_KEY, 0, 0x16, ""};
    keymap[1][2][2] = {BIND_KEY, 0, 0x04, ""};
    keymap[1][2][3] = {BIND_KEY, 0, 0x07, ""};
    keymap[1][3][0] = {BIND_KEY, 0, 0x1E, ""};
    keymap[1][3][1] = {BIND_KEY, 0, 0x1F, ""};
    keymap[1][3][2] = {BIND_KEY, 0, 0x20, ""};
    keymap[1][3][3] = {BIND_KEY, 0, 0x29, ""};

    // --- Layer 2: CS:GO / CS2 ---
    keymap[2][0][0] = {BIND_KEY, 0, 0x1E, ""};
    keymap[2][0][1] = {BIND_KEY, 0, 0x1F, ""};
    keymap[2][0][2] = {BIND_KEY, 0, 0x20, ""};
    keymap[2][0][3] = {BIND_KEY, 0, 0x21, ""};
    keymap[2][1][0] = {BIND_KEY, 0, 0x22, ""};
    keymap[2][1][1] = {BIND_KEY, 0, 0x05, ""};
    keymap[2][1][2] = {BIND_KEY, 0, 0x0A, ""};
    keymap[2][1][3] = {BIND_KEY, 0, 0x15, ""};
    keymap[2][2][0] = {BIND_KEY, 0, 0x1D, ""};
    keymap[2][2][1] = {BIND_KEY, 0, 0x1B, ""};
    keymap[2][2][2] = {BIND_KEY, 0, 0x06, ""};
    keymap[2][2][3] = {BIND_KEY, 0, 0x19, ""};
    keymap[2][3][0] = {BIND_KEY, 0, 0x14, ""};
    keymap[2][3][1] = {BIND_KEY, 0, 0x08, ""};
    keymap[2][3][2] = {BIND_KEY, 0, 0x09, ""};
    keymap[2][3][3] = {BIND_KEY, 1, 0x00, ""};

    // --- Layer 3: Custom 1 (empty) ---
    for (int r = 0; r < NUM_ROWS; r++)
        for (int c = 0; c < NUM_COLS; c++)
            keymap[3][r][c] = {};

    // --- Layer 4: Custom 2 (empty) ---
    for (int r = 0; r < NUM_ROWS; r++)
        for (int c = 0; c < NUM_COLS; c++)
            keymap[4][r][c] = {};
}

bool initConfig() {
    if (!LittleFS.begin(true)) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return false;
    }

    if (!LittleFS.exists(configFilePath) || !loadKeymap()) {
        Serial.println("Config file missing or invalid, creating default 5-layer config...");

        setDefaultProfiles();

        for (int l = 0; l < NUM_LAYERS; l++) {
            encoder_left[l]   = {BIND_MEDIA, 0, 1, ""};
            encoder_right[l]  = {BIND_MEDIA, 0, 2, ""};
            encoder_button[l] = {BIND_MEDIA, 0, 4, ""};
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
    for (int l = 0; l < NUM_LAYERS && l < layers.size(); l++) {
        JsonArray rows = layers[l];
        for (int r = 0; r < NUM_ROWS && r < rows.size(); r++) {
            JsonArray cols = rows[r];
            for (int c = 0; c < NUM_COLS && c < cols.size(); c++) {
                parseKeyBinding(cols[c], keymap[l][r][c]);
            }
        }
    }

    JsonArray encoderArray = doc["encoder"];
    bool needsSave = false;

    static const KeyBinding LEFT_DEF   = {BIND_MEDIA, 0, 1, ""};
    static const KeyBinding RIGHT_DEF  = {BIND_MEDIA, 0, 2, ""};
    static const KeyBinding BUTTON_DEF = {BIND_MEDIA, 0, 4, ""};

    if (encoderArray.isNull()) {
        for (int l = 0; l < NUM_LAYERS; l++) {
            encoder_left[l]   = LEFT_DEF;
            encoder_right[l]  = RIGHT_DEF;
            encoder_button[l] = BUTTON_DEF;
        }
        needsSave = true;
    } else {
        for (int l = 0; l < NUM_LAYERS; l++) {
            if (l < encoderArray.size()) {
                JsonObject encoderObj = encoderArray[l];
                if (encoderObj.isNull()) {
                    encoder_left[l]   = LEFT_DEF;
                    encoder_right[l]  = RIGHT_DEF;
                    encoder_button[l] = BUTTON_DEF;
                } else {
                    parseKeyBinding(encoderObj["left"],   encoder_left[l]);
                    parseKeyBinding(encoderObj["right"],  encoder_right[l]);
                    parseKeyBinding(encoderObj["button"], encoder_button[l]);
                }
            } else {
                encoder_left[l]   = LEFT_DEF;
                encoder_right[l]  = RIGHT_DEF;
                encoder_button[l] = BUTTON_DEF;
            }
        }
    }

    bool allZero = true;
    for (int l = 0; l < NUM_LAYERS && allZero; l++) {
        if (encoder_left[l].type != BIND_NONE ||
            encoder_right[l].type != BIND_NONE ||
            encoder_button[l].type != BIND_NONE) {
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

    for (int l = 0; l < NUM_LAYERS; l++) {
        JsonArray rowArray = layers.add<JsonArray>();
        for (int r = 0; r < NUM_ROWS; r++) {
            JsonArray colArray = rowArray.add<JsonArray>();
            for (int c = 0; c < NUM_COLS; c++) {
                serializeKeyBinding(colArray.add<JsonVariant>(), keymap[l][r][c]);
            }
        }
    }

    JsonArray encoderArray = doc["encoder"].to<JsonArray>();
    for (int l = 0; l < NUM_LAYERS; l++) {
        JsonObject encoderObj = encoderArray.add<JsonObject>();
        serializeKeyBinding(encoderObj["left"].to<JsonVariant>(),   encoder_left[l]);
        serializeKeyBinding(encoderObj["right"].to<JsonVariant>(),  encoder_right[l]);
        serializeKeyBinding(encoderObj["button"].to<JsonVariant>(), encoder_button[l]);
    }

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
    if (error) {
        return false;
    }

    JsonArray layers = doc["layers"];
    if (layers.isNull() || layers.size() == 0) {
        Serial.println("{\"cmd\":\"log\",\"level\":\"error\",\"msg\":\"set_config: missing or empty 'layers' array\"}");
        return false;
    }
    for (int l = 0; l < NUM_LAYERS && l < layers.size(); l++) {
        JsonArray rows = layers[l];
        for (int r = 0; r < NUM_ROWS && r < rows.size(); r++) {
            JsonArray cols = rows[r];
            for (int c = 0; c < NUM_COLS && c < cols.size(); c++) {
                parseKeyBinding(cols[c], keymap[l][r][c]);
            }
        }
    }

    JsonArray encoderArray = doc["encoder"];
    if (!encoderArray.isNull()) {
        for (int l = 0; l < NUM_LAYERS && l < encoderArray.size(); l++) {
            JsonObject encoderObj = encoderArray[l];
            if (!encoderObj.isNull()) {
                parseKeyBinding(encoderObj["left"],   encoder_left[l]);
                parseKeyBinding(encoderObj["right"],  encoder_right[l]);
                parseKeyBinding(encoderObj["button"], encoder_button[l]);
            }
        }
    }

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
