#include "config_manager.h"
#include <WiFi.h>

ConfigManager configManager;

void ConfigManager::begin() {
    _prefs.begin("smartlight", false);
}

String ConfigManager::safeGetString(const char* key, const String& def) {
    if (_prefs.isKey(key)) return _prefs.getString(key, def);
    return def;
}

// WiFi
String ConfigManager::getWifiSSID() { return safeGetString("wifi_ssid", ""); }
String ConfigManager::getWifiPass() { return safeGetString("wifi_pass", ""); }
void ConfigManager::setWifiSSID(const String& ssid) { _prefs.putString("wifi_ssid", ssid); }
void ConfigManager::setWifiPass(const String& pass) { _prefs.putString("wifi_pass", pass); }
bool ConfigManager::hasWifiConfig() { return _prefs.isKey("wifi_ssid") && getWifiSSID().length() > 0; }
void ConfigManager::clearWifi() {
    _prefs.remove("wifi_ssid");
    _prefs.remove("wifi_pass");
}

// Hostname - derived from device name
String ConfigManager::getHostname() {
    String name = getDeviceName();
    if (name.isEmpty()) return getDefaultHostname();
    return slugify(name);
}

String ConfigManager::slugify(const String& input) {
    String result;
    result.reserve(input.length());
    for (unsigned int i = 0; i < input.length(); i++) {
        char c = input.charAt(i);
        if (isalnum(c)) {
            result += (char)tolower(c);
        } else if (c == ' ' || c == '_') {
            if (result.length() > 0 && result.charAt(result.length() - 1) != '-') {
                result += '-';
            }
        }
    }
    // Trim trailing hyphens
    while (result.length() > 0 && result.charAt(result.length() - 1) == '-') {
        result.remove(result.length() - 1);
    }
    if (result.isEmpty()) return getDefaultHostname();
    return result;
}

// MQTT
String ConfigManager::getMqttHost() { return safeGetString("mqtt_host", ""); }
uint16_t ConfigManager::getMqttPort() { return _prefs.isKey("mqtt_port") ? _prefs.getUShort("mqtt_port", DEFAULT_MQTT_PORT) : DEFAULT_MQTT_PORT; }
String ConfigManager::getMqttUser() { return safeGetString("mqtt_user", ""); }
String ConfigManager::getMqttPass() { return safeGetString("mqtt_pass", ""); }
String ConfigManager::getMqttRoot() { return safeGetString("mqtt_root", DEFAULT_MQTT_ROOT); }
void ConfigManager::setMqttHost(const String& host) { _prefs.putString("mqtt_host", host); }
void ConfigManager::setMqttPort(uint16_t port) { _prefs.putUShort("mqtt_port", port); }
void ConfigManager::setMqttUser(const String& user) { _prefs.putString("mqtt_user", user); }
void ConfigManager::setMqttPass(const String& pass) { _prefs.putString("mqtt_pass", pass); }
void ConfigManager::setMqttRoot(const String& root) { _prefs.putString("mqtt_root", root); }

// Device
String ConfigManager::getDeviceName() { return safeGetString("device_name", "Smart Light"); }
uint8_t ConfigManager::getRelayPin() { return _prefs.isKey("relay_pin") ? _prefs.getUChar("relay_pin", DEFAULT_RELAY_PIN) : DEFAULT_RELAY_PIN; }
uint8_t ConfigManager::getButtonPin() { return _prefs.isKey("button_pin") ? _prefs.getUChar("button_pin", DEFAULT_BUTTON_PIN) : DEFAULT_BUTTON_PIN; }
uint8_t ConfigManager::getBootState() { return _prefs.isKey("boot_state") ? _prefs.getUChar("boot_state", BOOT_STATE_OFF) : BOOT_STATE_OFF; }
bool ConfigManager::getRelayLast() { return _prefs.isKey("relay_last") ? _prefs.getBool("relay_last", false) : false; }
void ConfigManager::setDeviceName(const String& name) { _prefs.putString("device_name", name); }
void ConfigManager::setRelayPin(uint8_t pin) { _prefs.putUChar("relay_pin", pin); }
void ConfigManager::setButtonPin(uint8_t pin) { _prefs.putUChar("button_pin", pin); }
void ConfigManager::setBootState(uint8_t state) { _prefs.putUChar("boot_state", state); }
void ConfigManager::setRelayLast(bool state) { _prefs.putBool("relay_last", state); }

// Utility
String ConfigManager::getDefaultHostname() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char buf[20];
    snprintf(buf, sizeof(buf), "smartlight-%02X%02X", mac[4], mac[5]);
    return String(buf);
}

void ConfigManager::factoryReset() {
    _prefs.clear();
}
