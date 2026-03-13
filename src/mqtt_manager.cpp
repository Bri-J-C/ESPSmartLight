#include "mqtt_manager.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "relay.h"
#include "status_led.h"
#include "logger.h"
#include "config.h"
#include <ArduinoJson.h>

static const char* TAG = "MQTT";

MqttManager mqttManager;

void MqttManager::begin() {
    _mqttClient.setClient(_wifiClient);
    _mqttClient.setBufferSize(1024);
    _mqttClient.setSocketTimeout(3);
    _mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        onMessage(topic, payload, length);
    });

    // Cache all config to avoid dangling pointers and repeated NVS reads
    _host = configManager.getMqttHost();
    _port = configManager.getMqttPort();
    _user = configManager.getMqttUser();
    _pass = configManager.getMqttPass();
    _clientId = configManager.getHostname();

    _mqttConfigured = !_host.isEmpty();

    if (_mqttConfigured) {
        _baseTopic = configManager.getMqttRoot() + "/" + _clientId;
        _stateTopic = _baseTopic + "/state";
        _commandTopic = _baseTopic + "/set";
        _availabilityTopic = _baseTopic + "/available";
        _discoveryTopic = "homeassistant/light/" + _clientId + "/config";

        // Set server once with persistent member string
        _mqttClient.setServer(_host.c_str(), _port);

        logger.info(TAG, "Broker: %s:%d", _host.c_str(), _port);
        logger.info(TAG, "Base topic: %s", _baseTopic.c_str());
        logger.info(TAG, "Discovery: %s", _discoveryTopic.c_str());
        logger.info(TAG, "State: %s", _stateTopic.c_str());
        logger.info(TAG, "Command: %s", _commandTopic.c_str());
        logger.info(TAG, "Availability: %s", _availabilityTopic.c_str());
    } else {
        logger.warn(TAG, "No MQTT broker configured");
    }
}

void MqttManager::update() {
    if (!_mqttConfigured) return;
    if (!wifiManager.isConnected()) return;

    if (!_mqttClient.connected()) {
        if (statusLed.getPattern() != LedPattern::DOUBLE_BLINK &&
            statusLed.getPattern() != LedPattern::SLOW_BLINK &&
            statusLed.getPattern() != LedPattern::FAST_BLINK) {
            statusLed.setPattern(LedPattern::DOUBLE_BLINK);
        }

        unsigned long now = millis();
        if (now - _lastReconnectAttempt > MQTT_RECONNECT_INTERVAL_MS) {
            _lastReconnectAttempt = now;
            connect();
        }
    } else {
        _mqttClient.loop();
    }
}

void MqttManager::connect() {
    logger.info(TAG, "Connecting to %s:%d as '%s'", _host.c_str(), _port, _clientId.c_str());

    bool connected;
    if (_user.length() > 0) {
        logger.debug(TAG, "Using auth user: %s", _user.c_str());
        connected = _mqttClient.connect(
            _clientId.c_str(),
            _user.c_str(),
            _pass.c_str(),
            _availabilityTopic.c_str(), 1, true, "offline"
        );
    } else {
        connected = _mqttClient.connect(
            _clientId.c_str(),
            _availabilityTopic.c_str(), 1, true, "offline"
        );
    }

    if (connected) {
        logger.info(TAG, "Connected!");

        bool subOk = _mqttClient.subscribe(_commandTopic.c_str());
        logger.info(TAG, "Subscribe %s: %s", _commandTopic.c_str(), subOk ? "OK" : "FAILED");

        bool availOk = _mqttClient.publish(_availabilityTopic.c_str(), "online", true);
        logger.info(TAG, "Availability 'online': %s", availOk ? "OK" : "FAILED");

        clearOldDiscovery();
        publishDiscovery();
        publishState();

        statusLed.setPattern(relayControl.isOn() ? LedPattern::ON : LedPattern::OFF);
    } else {
        int state = _mqttClient.state();
        const char* reason;
        switch (state) {
            case -4: reason = "TIMEOUT"; break;
            case -3: reason = "CONNECTION_LOST"; break;
            case -2: reason = "CONNECT_FAILED"; break;
            case -1: reason = "DISCONNECTED"; break;
            case  1: reason = "BAD_PROTOCOL"; break;
            case  2: reason = "BAD_CLIENT_ID"; break;
            case  3: reason = "UNAVAILABLE"; break;
            case  4: reason = "BAD_CREDENTIALS"; break;
            case  5: reason = "UNAUTHORIZED"; break;
            default: reason = "UNKNOWN"; break;
        }
        logger.error(TAG, "Connect failed: %s (rc=%d)", reason, state);
    }
}

void MqttManager::publishDiscovery() {
    JsonDocument doc;

    doc["name"] = (const char*)nullptr;
    doc["unique_id"] = "light_" + _clientId;
    doc["object_id"] = _clientId;
    doc["command_topic"] = _commandTopic;
    doc["state_topic"] = _stateTopic;
    doc["availability_topic"] = _availabilityTopic;
    doc["payload_on"] = "ON";
    doc["payload_off"] = "OFF";
    doc["payload_available"] = "online";
    doc["payload_not_available"] = "offline";
    doc["qos"] = 1;
    doc["retain"] = true;

    JsonObject device = doc["device"].to<JsonObject>();
    device["identifiers"][0] = "light_" + WiFi.macAddress();
    device["name"] = configManager.getDeviceName();
    device["manufacturer"] = "DIY";
    device["model"] = "ESP32-C3 Smart Light";
    device["sw_version"] = FW_VERSION;
    device["configuration_url"] = "http://" + wifiManager.getIP();

    String payload;
    serializeJson(doc, payload);

    logger.info(TAG, "Discovery (%d bytes): %s", payload.length(), payload.c_str());

    bool ok = _mqttClient.publish(_discoveryTopic.c_str(), payload.c_str(), true);
    logger.info(TAG, "Discovery publish: %s", ok ? "OK" : "FAILED");

    if (!ok) {
        logger.error(TAG, "Buffer size: %d, payload: %d", _mqttClient.getBufferSize(), payload.length());
    }
}

void MqttManager::clearOldDiscovery() {
    String oldTopic = "homeassistant/switch/" + _clientId + "/config";
    bool ok = _mqttClient.publish(oldTopic.c_str(), "", true);
    logger.debug(TAG, "Clear old switch discovery: %s", ok ? "OK" : "FAILED");
}

void MqttManager::publishState() {
    if (!_mqttClient.connected()) return;
    const char* state = relayControl.isOn() ? "ON" : "OFF";
    bool ok = _mqttClient.publish(_stateTopic.c_str(), state, true);
    logger.info(TAG, "State '%s': %s", state, ok ? "OK" : "FAILED");

    if (wifiManager.isConnected()) {
        statusLed.setPattern(relayControl.isOn() ? LedPattern::ON : LedPattern::OFF);
    }
}

void MqttManager::onMessage(char* topic, byte* payload, unsigned int length) {
    char msg[length + 1];
    memcpy(msg, payload, length);
    msg[length] = '\0';

    logger.info(TAG, "Received [%s]: %s", topic, msg);

    if (strcmp(topic, _commandTopic.c_str()) == 0) {
        if (strcmp(msg, "ON") == 0) {
            relayControl.setState(true);
            logger.info(TAG, "Relay ON (via MQTT)");
        } else if (strcmp(msg, "OFF") == 0) {
            relayControl.setState(false);
            logger.info(TAG, "Relay OFF (via MQTT)");
        } else {
            logger.warn(TAG, "Unknown command: %s", msg);
        }
        publishState();
    }
}

bool MqttManager::isConnected() {
    return _mqttClient.connected();
}
