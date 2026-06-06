#include "config_manager.h"

KeyBinding keymap[NUM_LAYERS][NUM_ROWS][NUM_COLS] = {};
uint8_t currentLayer = 0;

KeyBinding encoder_left[NUM_LAYERS] = {};
KeyBinding encoder_right[NUM_LAYERS] = {};
KeyBinding encoder_button[NUM_LAYERS] = {};

static void configLog(const char* level, const char* msg) {
    Serial.print("{\"cmd\":\"log\",\"level\":\"");
    Serial.print(level);
    Serial.print("\",\"msg\":\"[CONFIG] ");
    Serial.print(msg);
    Serial.println("\"}");
}

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
    configLog("info", "Mounting LittleFS...");

    if (!LittleFS.begin(true)) {
        configLog("error", "ERROR: Failed to mount LittleFS (may have been auto-formatted)");
        return false;
    }
    configLog("info", "LittleFS mounted successfully");

    configLog("info", "Checking for config files...");

    // Try loading primary config
    if (LittleFS.exists(CONFIG_PATH)) {
        File f = LittleFS.open(CONFIG_PATH, "r");
        if (f) {
            size_t sz = f.size();
            f.close();
            Serial.printf("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"[CONFIG] /config.json FOUND, size: %d bytes — loading...\"}\n", sz);
        }

        if (loadKeymap(CONFIG_PATH)) {
            configLog("success", "Config loaded successfully from /config.json");
            return true;
        }

        configLog("warn", "/config.json invalid, attempting backup /config.bak...");
    } else {
        configLog("warn", "/config.json NOT FOUND");
    }

    // Try backup
    if (LittleFS.exists(CONFIG_BAK_PATH)) {
        configLog("info", "/config.bak FOUND — attempting to restore...");

        if (loadKeymap(CONFIG_BAK_PATH)) {
            configLog("success", "Config restored from /config.bak, re-saving to /config.json...");
            saveKeymap();
            return true;
        }

        configLog("error", "/config.bak also invalid");
    } else {
        configLog("warn", "/config.bak NOT FOUND");
    }

    // No valid config found, load defaults
    configLog("info", "No valid config found, loading defaults...");

    setDefaultProfiles();
    for (int l = 0; l < NUM_LAYERS; l++) {
        encoder_left[l]   = {BIND_MEDIA, 0, 1, ""};
        encoder_right[l]  = {BIND_MEDIA, 0, 2, ""};
        encoder_button[l] = {BIND_MEDIA, 0, 4, ""};
    }

    configLog("info", "Default config created, saving to flash...");
    saveKeymap();
    return true;
}

bool loadKeymap(const char* path) {
    Serial.printf("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"[CONFIG] Opening %s for reading...\"}\n", path);

    File configFile = LittleFS.open(path, "r");
    if (!configFile) {
        Serial.printf("{\"cmd\":\"log\",\"level\":\"error\",\"msg\":\"[CONFIG] ERROR: Failed to open %s\"}\n", path);
        return false;
    }

    size_t size = configFile.size();
    Serial.printf("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"[CONFIG] Reading %d bytes from %s\"}\n", size, path);

    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    configFile.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buf.get());
    if (error) {
        Serial.printf("{\"cmd\":\"log\",\"level\":\"error\",\"msg\":\"[CONFIG] ERROR: JSON parse error: %s\"}\n",
                      error.c_str());
        return false;
    }

    configLog("info", "JSON parsed successfully");

    JsonArray layers = doc["layers"];
    int layersLoaded = 0;
    for (int l = 0; l < NUM_LAYERS && l < layers.size(); l++) {
        JsonArray rows = layers[l];
        for (int r = 0; r < NUM_ROWS && r < rows.size(); r++) {
            JsonArray cols = rows[r];
            for (int c = 0; c < NUM_COLS && c < cols.size(); c++) {
                parseKeyBinding(cols[c], keymap[l][r][c]);
            }
        }
        layersLoaded++;
    }
    Serial.printf("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"[CONFIG] Parsed %d/%d layers\"}\n", layersLoaded, NUM_LAYERS);

    JsonArray encoderArray = doc["encoder"];
    bool needsSave = false;

    static const KeyBinding LEFT_DEF   = {BIND_MEDIA, 0, 1, ""};
    static const KeyBinding RIGHT_DEF  = {BIND_MEDIA, 0, 2, ""};
    static const KeyBinding BUTTON_DEF = {BIND_MEDIA, 0, 4, ""};

    if (encoderArray.isNull()) {
        configLog("warn", "Encoder config missing in JSON, applying defaults");
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
        configLog("warn", "All encoder bindings are empty, applying defaults");
        for (int l = 0; l < NUM_LAYERS; l++) {
            encoder_left[l]   = LEFT_DEF;
            encoder_right[l]  = RIGHT_DEF;
            encoder_button[l] = BUTTON_DEF;
        }
        needsSave = true;
    }

    configLog("info", "Encoder config loaded");

    loadMacros(doc);

    if (needsSave) {
        configLog("info", "Config self-healed — re-saving to flash...");
        return saveKeymap();
    }

    configLog("success", "Config loaded successfully");
    return true;
}

bool saveKeymap() {
    configLog("info", "Serializing config to JSON...");

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

    // Atomic write to temp file
    configLog("info", "Writing to /config.tmp...");

    File tempFile = LittleFS.open(CONFIG_TEMP_PATH, "w");
    if (!tempFile) {
        configLog("error", "ERROR: Failed to open /config.tmp for writing");
        return false;
    }

    size_t jsonSize = serializeJson(doc, tempFile);
    Serial.printf("{\"cmd\":\"log\",\"level\":\"info\",\"msg\":\"[CONFIG] JSON size: %d bytes written to /config.tmp\"}\n", jsonSize);

    tempFile.flush();
    configLog("info", "Data flushed to flash");
    tempFile.close();
    configLog("info", "Temp file closed");

    // Remove old backup
    if (LittleFS.exists(CONFIG_BAK_PATH)) {
        LittleFS.remove(CONFIG_BAK_PATH);
        configLog("info", "Old backup removed");
    }

    // Create backup from current config
    if (LittleFS.exists(CONFIG_PATH)) {
        if (!LittleFS.rename(CONFIG_PATH, CONFIG_BAK_PATH)) {
            configLog("warn", "WARN: Failed to create backup (non-critical)");
        } else {
            configLog("info", "Backup created: /config.json -> /config.bak");
        }
    }

    // Atomic swap: temp -> live
    configLog("info", "Renaming /config.tmp -> /config.json...");
    if (!LittleFS.rename(CONFIG_TEMP_PATH, CONFIG_PATH)) {
        configLog("error", "ERROR: Failed to rename /config.tmp -> /config.json");

        // Try to restore from backup
        if (LittleFS.exists(CONFIG_BAK_PATH)) {
            configLog("warn", "Attempting to restore config from backup...");
            if (!LittleFS.rename(CONFIG_BAK_PATH, CONFIG_PATH)) {
                configLog("error", "CRITICAL: Failed to restore from backup!");
            } else {
                configLog("success", "Config restored from backup");
            }
        }
        return false;
    }

    configLog("success", "Config saved successfully to flash (persistent)");
    return true;
}

bool processConfigJson(const String& jsonString) {
    configLog("info", "Received set_config via serial — parsing...");

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        Serial.printf("{\"cmd\":\"log\",\"level\":\"error\",\"msg\":\"[CONFIG] ERROR: set_config parse error: %s\"}\n", error.c_str());
        return false;
    }

    JsonArray layers = doc["layers"];
    if (layers.isNull() || layers.size() == 0) {
        configLog("error", "ERROR: set_config: missing or empty 'layers' array");
        return false;
    }

    configLog("info", "Applying config to RAM...");

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

    configLog("info", "Saving to flash via saveKeymap()...");
    return saveKeymap();
}

String getConfigJson() {
    File configFile = LittleFS.open(CONFIG_PATH, "r");
    if (!configFile) {
        configLog("warn", "getConfigJson: /config.json not found, trying backup...");
        configFile = LittleFS.open(CONFIG_BAK_PATH, "r");
        if (!configFile) {
            configLog("warn", "getConfigJson: no config file found, returning empty");
            return "{}";
        }
        configLog("info", "getConfigJson: reading from /config.bak (primary missing)");
    }
    String res = configFile.readString();
    configFile.close();
    return res;
}
