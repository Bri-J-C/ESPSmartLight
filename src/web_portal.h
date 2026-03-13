#pragma once

#include <Arduino.h>
#include <WebServer.h>

class WebPortal {
public:
    void begin();
    void update();

private:
    WebServer _server{80};
    unsigned long _restartScheduled = 0;
    unsigned long _restartDelay = 0;
    bool _restartPending = false;
    bool _factoryResetPending = false;

    void handleRoot();
    void handleConfig();
    void handleSave();
    void handleScan();
    void handleStatus();
    void handleToggle();
    void handleRestart();
    void handleReset();
    void handleCaptivePortal();
    void handleLogs();

    void scheduleRestart(unsigned long delayMs = 500);
    void sendConfigPage();
    void sendStatusPage();
    static String htmlEscape(const String& s);
};

extern WebPortal webPortal;
