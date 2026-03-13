#include "wifi_manager.h"
#include "config_manager.h"
#include "status_led.h"
#include "logger.h"
#include <ArduinoJson.h>

static const char* TAG = "WIFI";

WiFiManager wifiManager;

void WiFiManager::begin() {
    _hostname = configManager.getHostname();
    _ssid = configManager.getWifiSSID();
    _pass = configManager.getWifiPass();

    WiFi.mode(WIFI_STA);

    uint8_t mac[6];
    WiFi.macAddress(mac);
    char suffix[5];
    snprintf(suffix, sizeof(suffix), "%02X%02X", mac[4], mac[5]);
    _apSSID = String(AP_SSID_PREFIX) + suffix;

    logger.info(TAG, "MAC: %s, AP SSID: %s", WiFi.macAddress().c_str(), _apSSID.c_str());

    if (configManager.hasWifiConfig()) {
        logger.info(TAG, "WiFi configured, connecting to %s", _ssid.c_str());
        startSTA();
    } else {
        logger.warn(TAG, "No WiFi config, starting AP");
        startAP();
    }
}

void WiFiManager::update() {
    if (_restartPending && (millis() - _restartScheduled >= 500)) {
        ESP.restart();
    }

    if (_apActive) {
        _dnsServer.processNextRequest();
    }

    switch (_state) {
        case WiFiState::CONNECTING:
            if (WiFi.status() == WL_CONNECTED) {
                _state = WiFiState::CONNECTED;
                _reconnectInterval = WIFI_RECONNECT_MIN_MS;
                logger.info(TAG, "Connected! IP: %s", WiFi.localIP().toString().c_str());
                if (_apActive) stopAP();
                setDhcpHostname();
                setupMdns();
            } else if (millis() - _connectStartTime > STA_CONNECT_TIMEOUT_MS) {
                logger.warn(TAG, "Connection timeout after %dms, starting AP", STA_CONNECT_TIMEOUT_MS);
                startAP();
            }
            break;

        case WiFiState::CONNECTED:
            if (WiFi.status() != WL_CONNECTED) {
                logger.warn(TAG, "Disconnected, starting reconnect");
                _state = WiFiState::RECONNECTING;
                _disconnectTime = millis();
                _lastReconnectAttempt = millis();
                _reconnectInterval = WIFI_RECONNECT_MIN_MS;
                statusLed.setPattern(LedPattern::SLOW_BLINK);
            }
            break;

        case WiFiState::RECONNECTING: {
            unsigned long now = millis();
            if (WiFi.status() == WL_CONNECTED) {
                _state = WiFiState::CONNECTED;
                _reconnectInterval = WIFI_RECONNECT_MIN_MS;
                logger.info(TAG, "Reconnected! IP: %s", WiFi.localIP().toString().c_str());
                setDhcpHostname();
                MDNS.end();
                setupMdns();
                break;
            }
            if (now - _disconnectTime > WIFI_RECONNECT_FAIL_MS) {
                logger.error(TAG, "Reconnect timeout after %dms — restarting", WIFI_RECONNECT_FAIL_MS);
                _restartPending = true;
                _restartScheduled = millis();
                break;
            }
            if (now - _lastReconnectAttempt > _reconnectInterval) {
                logger.debug(TAG, "Reconnect attempt (backoff: %lums)", _reconnectInterval);
                WiFi.disconnect();
                WiFi.setHostname(_hostname.c_str());
                WiFi.begin(_ssid.c_str(), _pass.c_str());
                _lastReconnectAttempt = now;
                _reconnectInterval = min(_reconnectInterval * 2, (unsigned long)WIFI_RECONNECT_MAX_MS);
            }
            break;
        }

        case WiFiState::AP_MODE:
        case WiFiState::IDLE:
            break;
    }
}

void WiFiManager::startSTA() {
    _state = WiFiState::CONNECTING;
    _connectStartTime = millis();
    statusLed.setPattern(LedPattern::SLOW_BLINK);

    WiFi.mode(WIFI_STA);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(_hostname.c_str());
    WiFi.begin(_ssid.c_str(), _pass.c_str());
    logger.info(TAG, "Connecting to %s (hostname: %s)", _ssid.c_str(), _hostname.c_str());
}

void WiFiManager::startAP() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(_apSSID.c_str(), strlen(AP_PASSWORD) > 0 ? AP_PASSWORD : nullptr);

    _dnsServer.start(53, "*", AP_IP);
    _apActive = true;
    _state = WiFiState::AP_MODE;
    statusLed.setPattern(LedPattern::FAST_BLINK);
    logger.info(TAG, "AP started: %s at %s", _apSSID.c_str(), WiFi.softAPIP().toString().c_str());
}

void WiFiManager::stopAP() {
    _dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    _apActive = false;
    logger.info(TAG, "AP stopped");
}

void WiFiManager::setDhcpHostname() {
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_set_hostname(netif, _hostname.c_str());
        logger.info(TAG, "DHCP hostname set: %s", _hostname.c_str());
    } else {
        logger.error(TAG, "Failed to get netif for DHCP hostname");
    }
}

void WiFiManager::setupMdns() {
    if (!MDNS.begin(_hostname.c_str())) {
        logger.error(TAG, "mDNS init failed");
        return;
    }
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("arduino", "tcp", 3232);
    logger.info(TAG, "mDNS started: %s.local", _hostname.c_str());
}

WiFiState WiFiManager::getState() const { return _state; }
bool WiFiManager::isConnected() const { return _state == WiFiState::CONNECTED; }
bool WiFiManager::isAPMode() const { return _state == WiFiState::AP_MODE; }

String WiFiManager::getIP() const {
    if (isConnected()) return WiFi.localIP().toString();
    if (isAPMode()) return WiFi.softAPIP().toString();
    return "N/A";
}

String WiFiManager::getAPSSID() { return _apSSID; }

String WiFiManager::scanNetworks() {
    int n = WiFi.scanComplete();

    if (n == WIFI_SCAN_FAILED) {
        // No scan running, start one
        logger.info(TAG, "Starting async WiFi scan...");
        WiFi.scanNetworks(true);
        return "{\"scanning\":true}";
    }

    if (n == WIFI_SCAN_RUNNING) {
        return "{\"scanning\":true}";
    }

    // Scan complete, build results
    logger.info(TAG, "Scan found %d networks", n);
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.isEmpty()) continue;

        bool found = false;
        for (JsonObject obj : arr) {
            if (obj["ssid"].as<String>() == ssid) {
                if (WiFi.RSSI(i) > obj["rssi"].as<int>()) {
                    obj["rssi"] = WiFi.RSSI(i);
                    obj["enc"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
                }
                found = true;
                break;
            }
        }
        if (!found) {
            JsonObject obj = arr.add<JsonObject>();
            obj["ssid"] = ssid;
            obj["rssi"] = WiFi.RSSI(i);
            obj["enc"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        }
    }

    WiFi.scanDelete();
    String json;
    serializeJson(doc, json);
    return json;
}
