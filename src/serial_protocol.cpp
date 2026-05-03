#include "serial_protocol.h"
#include "config_manager.h"
#include <ArduinoJson.h>

void initSerialProtocol() {
    // Serial.begin() called in setup()
}

void processSerialProtocol() {
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    // Ignoruj śmieci wysyłane przy nawiązywaniu połączenia przez terminal
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

    } else if (strcmp(cmd, "get_layer") == 0) {
        // Respond with current active layer
        sendLayerChange(currentLayer);

    } else if (strcmp(cmd, "set_layer") == 0) {
        uint8_t newLayer = doc["layer"] | 0;
        if (newLayer < NUM_LAYERS) {
            currentLayer = newLayer;
            sendLayerChange(currentLayer);
            Serial.println("{\"cmd\":\"status\",\"status\":\"success\",\"msg\":\"Zmieniono warstwę z WebUI.\"}");
        }

    } else {
        Serial.println("{\"cmd\":\"log\",\"level\":\"warn\",\"msg\":\"Unknown command\"}");
    }
}

// Sends both press and release events — WebUI can precisely track state
void sendKeyState(uint8_t row, uint8_t col, bool isPressed) {
    Serial.printf("{\"cmd\":\"key_state\",\"row\":%d,\"col\":%d,\"isPressed\":%s}\n",
                  row, col, isPressed ? "true" : "false");
}

void sendLayerChange(uint8_t layer) {
    Serial.printf("{\"cmd\":\"layer_change\",\"layer\":%d}\n", layer);
}