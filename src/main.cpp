#include <Arduino.h>
#include "config_manager.h"
#include "logger.h"
#include "status_led.h"
#include "wifi_manager.h"
#include "web_portal.h"
#include "relay.h"
#include "button.h"
#include "mqtt_manager.h"
#include "ota.h"
#include "config.h"

static const char* TAG = "MAIN";

void setup() {
    Serial.begin(115200);
    delay(500);

    logger.begin();
    logger.info(TAG, "ESP Smart Light v%s starting", FW_VERSION);

    configManager.begin();
    logger.info(TAG, "Config loaded");

    statusLed.begin();
    statusLed.setPattern(LedPattern::SLOW_BLINK);

    relayControl.begin();
    buttonHandler.begin();

    wifiManager.begin();
    webPortal.begin();
    mqttManager.begin();
    otaManager.begin();

    logger.info(TAG, "Hostname: %s", configManager.getHostname().c_str());
    logger.info(TAG, "Device: %s", configManager.getDeviceName().c_str());
    logger.info(TAG, "MQTT broker: %s:%d", configManager.getMqttHost().c_str(), configManager.getMqttPort());
    logger.info(TAG, "MQTT root: %s", configManager.getMqttRoot().c_str());
    logger.info(TAG, "Relay pin: %d, Button pin: %d", configManager.getRelayPin(), configManager.getButtonPin());
    logger.info(TAG, "Free heap: %d bytes", ESP.getFreeHeap());
    logger.info(TAG, "Setup complete");
}

void loop() {
    statusLed.update();
    wifiManager.update();
    webPortal.update();
    buttonHandler.update();
    relayControl.update();
    mqttManager.update();
    otaManager.update();
}
