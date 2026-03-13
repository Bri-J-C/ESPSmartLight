#include "web_portal.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "relay.h"
#include "mqtt_manager.h"
#include "logger.h"
#include "html_pages.h"
#include <ArduinoJson.h>

WebPortal webPortal;

void WebPortal::begin() {
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/config", HTTP_GET, [this]() { handleConfig(); });
    _server.on("/save", HTTP_POST, [this]() { handleSave(); });
    _server.on("/scan", HTTP_GET, [this]() { handleScan(); });
    _server.on("/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/toggle", HTTP_POST, [this]() { handleToggle(); });
    _server.on("/restart", HTTP_POST, [this]() { handleRestart(); });
    _server.on("/reset", HTTP_POST, [this]() { handleReset(); });
    _server.on("/logs", HTTP_GET, [this]() { handleLogs(); });

    // Captive portal detection endpoints
    _server.on("/generate_204", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/fwlink", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/hotspot-detect.html", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/canonical.html", HTTP_GET, [this]() { handleCaptivePortal(); });
    _server.on("/connecttest.txt", HTTP_GET, [this]() { handleCaptivePortal(); });

    _server.onNotFound([this]() { handleCaptivePortal(); });
    _server.begin();
    Serial.println("Web server started");
}

void WebPortal::update() {
    _server.handleClient();

    if (_restartPending && (millis() - _restartScheduled >= _restartDelay)) {
        if (_factoryResetPending) {
            configManager.factoryReset();
        }
        ESP.restart();
    }
}

void WebPortal::scheduleRestart(unsigned long delayMs) {
    _restartPending = true;
    _restartScheduled = millis();
    _restartDelay = delayMs;
}

void WebPortal::handleRoot() {
    if (wifiManager.isAPMode() || !wifiManager.isConnected()) {
        handleConfig();
    } else {
        sendStatusPage();
    }
}

void WebPortal::handleConfig() {
    sendConfigPage();
}

void WebPortal::handleSave() {
    // Validate GPIO pins before saving
    if (_server.hasArg("relay_pin") || _server.hasArg("button_pin")) {
        int rp = _server.hasArg("relay_pin") ? _server.arg("relay_pin").toInt() : configManager.getRelayPin();
        int bp = _server.hasArg("button_pin") ? _server.arg("button_pin").toInt() : configManager.getButtonPin();

        auto pinValid = [](int pin) -> bool {
            if (pin < 0 || pin > 21) return false;
            if (pin >= 6 && pin <= 7) return false;   // SPI flash
            if (pin >= 11 && pin <= 17) return false;  // Internal flash/PSRAM
            if (pin == STATUS_LED_PIN) return false;
            return true;
        };

        if (!pinValid(rp) || !pinValid(bp) || rp == bp) {
            _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
            _server.send(400, "text/html", "");
            _server.sendContent(String(FPSTR(HTML_HEADER)));
            _server.sendContent("<h1>Error</h1><div class='card'><p>Invalid GPIO pins. Avoid 6-7, 8, 11-17, and ensure relay and button pins differ.</p>");
            _server.sendContent("<a class='btn btn-primary' href='/config'>Back</a></div>");
            _server.sendContent(String(FPSTR(HTML_FOOTER)));
            _server.sendContent("");
            return;
        }
    }

    // Validate MQTT port
    if (_server.hasArg("mqtt_port")) {
        long port = _server.arg("mqtt_port").toInt();
        if (port < 1 || port > 65535) port = DEFAULT_MQTT_PORT;
        configManager.setMqttPort((uint16_t)port);
    }

    // Validate boot state
    if (_server.hasArg("boot_state")) {
        int bs = _server.arg("boot_state").toInt();
        if (bs >= 0 && bs <= 2) configManager.setBootState(bs);
    }

    if (_server.hasArg("wifi_ssid") && _server.arg("wifi_ssid").length() > 0)
        configManager.setWifiSSID(_server.arg("wifi_ssid"));
    if (_server.hasArg("wifi_pass") && _server.arg("wifi_pass").length() > 0)
        configManager.setWifiPass(_server.arg("wifi_pass"));
    if (_server.hasArg("mqtt_host")) configManager.setMqttHost(_server.arg("mqtt_host"));
    if (_server.hasArg("mqtt_user")) configManager.setMqttUser(_server.arg("mqtt_user"));
    if (_server.hasArg("mqtt_pass") && _server.arg("mqtt_pass").length() > 0)
        configManager.setMqttPass(_server.arg("mqtt_pass"));
    if (_server.hasArg("mqtt_root")) configManager.setMqttRoot(_server.arg("mqtt_root"));
    if (_server.hasArg("device_name") && _server.arg("device_name").length() > 0)
        configManager.setDeviceName(_server.arg("device_name"));
    if (_server.hasArg("relay_pin")) configManager.setRelayPin(_server.arg("relay_pin").toInt());
    if (_server.hasArg("button_pin")) configManager.setButtonPin(_server.arg("button_pin").toInt());

    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/html", "");
    _server.sendContent(String(FPSTR(HTML_HEADER)));
    _server.sendContent("<h1>Saved!</h1><div class='card'><p>Settings saved. Rebooting...</p></div>");
    _server.sendContent(String(FPSTR(HTML_FOOTER)));
    _server.sendContent("");

    scheduleRestart(1000);
}

void WebPortal::handleScan() {
    _server.send(200, "application/json", wifiManager.scanNetworks());
}

void WebPortal::handleStatus() {
    JsonDocument doc;
    doc["WiFi"] = wifiManager.isConnected() ? "Connected" : "Disconnected";
    doc["IP Address"] = wifiManager.getIP();
    doc["MQTT"] = mqttManager.isConnected() ? "Connected" : "Disconnected";
    doc["Relay"] = relayControl.isOn() ? "ON" : "OFF";

    unsigned long s = millis() / 1000;
    char uptime[32];
    snprintf(uptime, sizeof(uptime), "%lud %luh %lum %lus", s / 86400, (s % 86400) / 3600, (s % 3600) / 60, s % 60);
    doc["Uptime"] = uptime;
    doc["Firmware"] = FW_VERSION;
    doc["Free Heap"] = String(ESP.getFreeHeap() / 1024) + " KB";

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebPortal::handleToggle() {
    relayControl.toggle();
    mqttManager.publishState();
    _server.send(200, "text/plain", relayControl.isOn() ? "ON" : "OFF");
}

void WebPortal::handleRestart() {
    _server.send(200, "text/plain", "Restarting...");
    scheduleRestart();
}

void WebPortal::handleReset() {
    _server.send(200, "text/plain", "Factory reset...");
    _factoryResetPending = true;
    scheduleRestart();
}

void WebPortal::handleLogs() {
    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/html", "");
    _server.sendContent(String(FPSTR(HTML_HEADER)));
    _server.sendContent("<meta http-equiv='refresh' content='5'>");
    _server.sendContent("<h1>Device Logs</h1>");
    _server.sendContent("<div class='card'>");
    logger.sendLogsHtml([this](const String& chunk) { _server.sendContent(chunk); });
    _server.sendContent("</div>");
    _server.sendContent("<div class='actions'><a class='btn btn-secondary' href='/'>Back</a></div>");
    _server.sendContent("<p style='color:rgba(255,255,255,0.3);font-size:12px;text-align:center;margin-top:12px;'>Auto-refresh every 5 seconds</p>");
    _server.sendContent("<script>window.onload=function(){var l=document.getElementById('logbox');if(l)l.scrollTop=l.scrollHeight;}</script>");
    _server.sendContent(String(FPSTR(HTML_FOOTER)));
    _server.sendContent("");
}

String WebPortal::htmlEscape(const String& s) {
    String out;
    out.reserve(s.length());
    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s.charAt(i);
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;        break;
        }
    }
    return out;
}

void WebPortal::handleCaptivePortal() {
    _server.sendHeader("Location", "http://192.168.4.1/", true);
    _server.send(302, "text/plain", "");
}

void WebPortal::sendConfigPage() {
    String page = String(FPSTR(HTML_CONFIG_PAGE));

    page.replace("%WIFI_SSID%", htmlEscape(configManager.getWifiSSID()));
    page.replace("%WIFI_PASS_PH%", configManager.getWifiPass().isEmpty() ? "No password set" : "Password set (leave blank to keep)");
    page.replace("%MQTT_HOST%", htmlEscape(configManager.getMqttHost()));
    page.replace("%MQTT_PORT%", String(configManager.getMqttPort()));
    page.replace("%MQTT_USER%", htmlEscape(configManager.getMqttUser()));
    page.replace("%MQTT_PASS_PH%", configManager.getMqttPass().isEmpty() ? "No password set" : "Password set (leave blank to keep)");
    page.replace("%MQTT_ROOT%", htmlEscape(configManager.getMqttRoot()));
    page.replace("%DEVICE_NAME%", htmlEscape(configManager.getDeviceName()));
    page.replace("%RELAY_PIN%", String(configManager.getRelayPin()));
    page.replace("%BUTTON_PIN%", String(configManager.getButtonPin()));

    uint8_t bs = configManager.getBootState();
    page.replace("%BOOT_OFF%", bs == 0 ? "selected" : "");
    page.replace("%BOOT_ON%", bs == 1 ? "selected" : "");
    page.replace("%BOOT_LAST%", bs == 2 ? "selected" : "");

    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/html", "");
    _server.sendContent(String(FPSTR(HTML_HEADER)));
    _server.sendContent(page);
    _server.sendContent(String(FPSTR(HTML_FOOTER)));
    _server.sendContent("");
}

void WebPortal::sendStatusPage() {
    String page = String(FPSTR(HTML_STATUS_PAGE));

    page.replace("%DEVICE_NAME%", htmlEscape(configManager.getDeviceName()));
    page.replace("%WIFI_STATUS%", wifiManager.isConnected() ? "Connected" : "Disconnected");
    page.replace("%WIFI_CLASS%", wifiManager.isConnected() ? "online" : "offline");
    page.replace("%IP_ADDR%", wifiManager.getIP());
    page.replace("%MQTT_STATUS%", mqttManager.isConnected() ? "Connected" : "Disconnected");
    page.replace("%MQTT_CLASS%", mqttManager.isConnected() ? "online" : "offline");
    page.replace("%RELAY_STATE%", relayControl.isOn() ? "ON" : "OFF");

    unsigned long s = millis() / 1000;
    char uptime[32];
    snprintf(uptime, sizeof(uptime), "%lud %luh %lum %lus", s / 86400, (s % 86400) / 3600, (s % 3600) / 60, s % 60);
    page.replace("%UPTIME%", String(uptime));
    page.replace("%FW_VERSION%", FW_VERSION);
    page.replace("%FREE_HEAP%", String(ESP.getFreeHeap() / 1024) + " KB");

    _server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    _server.send(200, "text/html", "");
    _server.sendContent(String(FPSTR(HTML_HEADER)));
    _server.sendContent(page);
    _server.sendContent(String(FPSTR(HTML_FOOTER)));
    _server.sendContent("");
}
