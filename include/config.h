#pragma once

#define FW_VERSION "1.0.0"

// Pin assignments (defaults, configurable via web portal)
#define DEFAULT_RELAY_PIN 3
#define DEFAULT_BUTTON_PIN 9
#define STATUS_LED_PIN 8
#define STATUS_LED_ACTIVE_LOW true

// WiFi
#define AP_SSID_PREFIX "SmartLight-"
#define AP_PASSWORD ""  // Open AP
#define AP_IP IPAddress(192, 168, 4, 1)
#define STA_CONNECT_TIMEOUT_MS 15000
#define WIFI_RECONNECT_MIN_MS 1000
#define WIFI_RECONNECT_MAX_MS 30000
#define WIFI_RECONNECT_FAIL_MS 60000

// MQTT
#define DEFAULT_MQTT_PORT 1883
#define MQTT_RECONNECT_INTERVAL_MS 5000
#define DEFAULT_MQTT_ROOT "smartlight"

// Button timing
#define BUTTON_DEBOUNCE_MS 50
#define BUTTON_WIFI_CLEAR_MIN_MS 3000
#define BUTTON_WIFI_CLEAR_MAX_MS 5000
#define BUTTON_FACTORY_RESET_MS 10000

// Boot state options
#define BOOT_STATE_OFF 0
#define BOOT_STATE_ON 1
#define BOOT_STATE_LAST 2

// OTA
#define OTA_PORT 3232
