#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <esp_netif.h>
#include <ESPmDNS.h>
#include "config.h"

enum class WiFiState {
    IDLE,
    CONNECTING,
    CONNECTED,
    AP_MODE,
    RECONNECTING
};

class WiFiManager {
public:
    void begin();
    void update();
    WiFiState getState() const;
    bool isConnected() const;
    bool isAPMode() const;
    String getIP() const;
    String getAPSSID();
    String scanNetworks();

private:
    void startAP();
    void startSTA();
    void stopAP();
    void setupMdns();
    void setDhcpHostname();

    WiFiState _state = WiFiState::IDLE;
    DNSServer _dnsServer;
    bool _apActive = false;
    unsigned long _connectStartTime = 0;
    unsigned long _lastReconnectAttempt = 0;
    unsigned long _reconnectInterval = WIFI_RECONNECT_MIN_MS;
    unsigned long _disconnectTime = 0;
    unsigned long _restartScheduled = 0;
    bool _restartPending = false;
    String _apSSID;
    String _hostname;
    String _ssid;
    String _pass;
};

extern WiFiManager wifiManager;
