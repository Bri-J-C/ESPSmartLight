#include "ota.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "status_led.h"
#include "logger.h"
#include "config.h"
#include <ArduinoOTA.h>

static const char* TAG = "OTA";

OtaManager otaManager;

void OtaManager::begin() {
    _hostname = configManager.getHostname();
    ArduinoOTA.setHostname(_hostname.c_str());
    ArduinoOTA.setPort(OTA_PORT);

    ArduinoOTA.onStart([]() {
        logger.info(TAG, "Update starting...");
        statusLed.setPattern(LedPattern::RAPID_FLASH);
    });

    ArduinoOTA.onEnd([]() {
        logger.info(TAG, "Update complete");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static int lastPercent = -1;
        int percent = total > 0 ? (progress * 100) / total : 0;
        if (percent != lastPercent && percent % 25 == 0) {
            logger.info(TAG, "Progress: %u%%", percent);
            lastPercent = percent;
        }
    });

    ArduinoOTA.onError([](ota_error_t error) {
        const char* msg;
        switch (error) {
            case OTA_AUTH_ERROR:    msg = "Auth Failed"; break;
            case OTA_BEGIN_ERROR:   msg = "Begin Failed"; break;
            case OTA_CONNECT_ERROR: msg = "Connect Failed"; break;
            case OTA_RECEIVE_ERROR: msg = "Receive Failed"; break;
            case OTA_END_ERROR:     msg = "End Failed"; break;
            default:                msg = "Unknown"; break;
        }
        logger.error(TAG, "Error: %s", msg);
        statusLed.setPattern(LedPattern::SOS);
    });

    logger.info(TAG, "Initialized (port %d)", OTA_PORT);
}

void OtaManager::update() {
    if (!wifiManager.isConnected()) {
        _initialized = false;
        return;
    }

    if (!_initialized) {
        ArduinoOTA.begin();
        _initialized = true;
        logger.info(TAG, "Ready on %s:%d", _hostname.c_str(), OTA_PORT);
    }

    ArduinoOTA.handle();
}
