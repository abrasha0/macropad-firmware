#include "serial_protocol.h"
#include "config_manager.h"
#include "macro_handler.h"
#include <ArduinoJson.h>

void initSerialProtocol() {
}

#define SERIAL_MAX_LINE 4096

void processSerialProtocol() {
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    if (line.length() >= SERIAL_MAX_LINE) {
        Serial.println("{\"cmd\":\"log\",\"level\":\"error\",\"msg\":\"Line too long, discarded.\"}");
        return;
    }
    line.trim();
    if (line.length() == 0) return;

    if (!line.startsWith("{")) return;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, line);

    if (error) {
        Serial.println("{\"cmd\":\"log\",\"level\":\"error\",\"msg\":\"Invalid JSON received\"}");
        return;
    }

    const char* cmd = doc["cmd"];
    if (!cmd) return;

    if (strcmp(cmd, "get_config") == 0) {
        String configJson = getConfigJson();
        Serial.print("{\"cmd\":\"config\",\"data\":");
        Serial.print(configJson.length() > 0 ? configJson : "{}");
        Serial.println("}");

    } else if (strcmp(cmd, "set_config") == 0) {
        String dataPayload;
        serializeJson(doc["data"], dataPayload);
        if (processConfigJson(dataPayload)) {
            Serial.println("{\"cmd\":\"status\",\"status\":\"success\",\"msg\":\"Config saved.\"}");
        } else {
            Serial.println("{\"cmd\":\"status\",\"status\":\"error\",\"msg\":\"Failed to save config.\"}");
        }

    } else if (strcmp(cmd, "get_macros") == 0) {
        JsonDocument resp;
        resp["cmd"] = "macros";
        JsonArray arr = resp["data"].to<JsonArray>();
        for (uint8_t i = 0; i < macroCount; i++) {
            JsonObject m = arr.add<JsonObject>();
            m["name"] = macros[i].name;
            JsonArray steps = m["steps"].to<JsonArray>();
            for (uint8_t s = 0; s < macros[i].stepCount; s++) {
                JsonObject step = steps.add<JsonObject>();
                if (macros[i].steps[s].text[0] != '\0') {
                    step["text"] = macros[i].steps[s].text;
                    step["delay"] = macros[i].steps[s].delayMs;
                } else {
                    step["keycode"] = macros[i].steps[s].keycode;
                    step["delay"] = macros[i].steps[s].delayMs;
                }
            }
        }
        serializeJson(resp, Serial);
        Serial.println();

    } else if (strcmp(cmd, "set_macros") == 0) {
        JsonVariant data = doc["data"];
        if (!data.is<JsonArray>() && !data.is<JsonObject>()) {
            Serial.println("{\"cmd\":\"status\",\"status\":\"error\",\"msg\":\"Invalid data: expected array or object.\"}");
            return;
        }
        JsonDocument dataDoc;
        if (data.is<JsonArray>()) {
            dataDoc["macros"] = data;
        } else {
            if (data["macros"].isNull()) {
                Serial.println("{\"cmd\":\"status\",\"status\":\"error\",\"msg\":\"Missing 'macros' field in data object.\"}");
                return;
            }
            dataDoc["macros"] = data["macros"];
        }
        loadMacros(dataDoc);
        if (saveKeymap()) {
            Serial.println("{\"cmd\":\"status\",\"status\":\"success\",\"msg\":\"Macros saved.\"}");
        } else {
            Serial.println("{\"cmd\":\"status\",\"status\":\"error\",\"msg\":\"Failed to save macros.\"}");
        }

    } else if (strcmp(cmd, "run_macro") == 0) {
        uint8_t idx = doc["index"] | 0;
        if (idx < macroCount && macros[idx].stepCount > 0) {
            runMacro(idx);
            Serial.println("{\"cmd\":\"status\",\"status\":\"success\",\"msg\":\"Macro started.\"}");
        } else {
            Serial.println("{\"cmd\":\"status\",\"status\":\"error\",\"msg\":\"Invalid macro index.\"}");
        }

    } else if (strcmp(cmd, "get_layer") == 0) {
        sendLayerChange(currentLayer);

    } else if (strcmp(cmd, "set_layer") == 0) {
        uint8_t newLayer = doc["layer"] | 0;
        if (newLayer < NUM_LAYERS) {
            currentLayer = newLayer;
            sendLayerChange(currentLayer);
            Serial.println("{\"cmd\":\"status\",\"status\":\"success\",\"msg\":\"Zmieniono warstwę z WebUI.\"}");
        } else {
            Serial.println("{\"cmd\":\"status\",\"status\":\"error\",\"msg\":\"Invalid layer index.\"}");
        }

    } else {
        Serial.println("{\"cmd\":\"log\",\"level\":\"warn\",\"msg\":\"Unknown command\"}");
    }
}

void sendKeyState(uint8_t row, uint8_t col, bool isPressed) {
    Serial.printf("{\"cmd\":\"key_state\",\"row\":%d,\"col\":%d,\"isPressed\":%s}\n",
                  row, col, isPressed ? "true" : "false");
}

void sendLayerChange(uint8_t layer) {
    Serial.printf("{\"cmd\":\"layer_change\",\"layer\":%d}\n", layer);
}

void sendMacroProgress(uint8_t index, uint8_t step, uint8_t total) {
    Serial.printf("{\"cmd\":\"macro_progress\",\"index\":%d,\"step\":%d,\"total\":%d}\n",
                  index, step, total);
}
