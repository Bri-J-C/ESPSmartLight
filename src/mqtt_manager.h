#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

class MqttManager {
public:
    void begin();
    void update();
    bool isConnected();
    void publishState();

private:
    void connect();
    void publishDiscovery();
    void clearOldDiscovery();
    void onMessage(char* topic, byte* payload, unsigned int length);

    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    unsigned long _lastReconnectAttempt = 0;
    bool _mqttConfigured = false;

    // Cached strings to avoid dangling pointers and repeated NVS reads
    String _host;
    uint16_t _port = 0;
    String _user;
    String _pass;
    String _clientId;
    String _baseTopic;
    String _stateTopic;
    String _commandTopic;
    String _availabilityTopic;
    String _discoveryTopic;
};

extern MqttManager mqttManager;
