#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

class ConfigManager {
public:
    void begin();

    // WiFi
    String getWifiSSID();
    String getWifiPass();
    void setWifiSSID(const String& ssid);
    void setWifiPass(const String& pass);
    bool hasWifiConfig();
    void clearWifi();

    // Hostname (derived from device name)
    String getHostname();

    // MQTT
    String getMqttHost();
    uint16_t getMqttPort();
    String getMqttUser();
    String getMqttPass();
    String getMqttRoot();
    void setMqttHost(const String& host);
    void setMqttPort(uint16_t port);
    void setMqttUser(const String& user);
    void setMqttPass(const String& pass);
    void setMqttRoot(const String& root);

    // Device
    String getDeviceName();
    uint8_t getRelayPin();
    uint8_t getButtonPin();
    uint8_t getBootState();
    bool getRelayLast();
    void setDeviceName(const String& name);
    void setRelayPin(uint8_t pin);
    void setButtonPin(uint8_t pin);
    void setBootState(uint8_t state);
    void setRelayLast(bool state);

    // Utility
    String getDefaultHostname();
    void factoryReset();

private:
    Preferences _prefs;
    String safeGetString(const char* key, const String& def);
    String slugify(const String& input);
};

extern ConfigManager configManager;
