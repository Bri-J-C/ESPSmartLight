#include "logger.h"
#include <stdarg.h>

Logger logger;

void Logger::begin() {
    _head = 0;
    _count = 0;
}

void Logger::addEntry(LogLevel level, const char* tag, const char* formatted) {
    LogEntry& entry = _buffer[_head];
    entry.timestamp = millis();
    entry.level = (char)level;
    snprintf(entry.message, LOG_ENTRY_SIZE, "[%s] %s", tag, formatted);
    _head = (_head + 1) % LOG_ENTRIES;
    if (_count < LOG_ENTRIES) _count++;

    // Also print to serial
    unsigned long s = entry.timestamp / 1000;
    unsigned long ms = entry.timestamp % 1000;
    Serial.printf("[%3lu.%03lu] %c %s\n", s, ms, entry.level, entry.message);
}

void Logger::vlog(LogLevel level, const char* tag, const char* fmt, va_list args) {
    char buf[LOG_ENTRY_SIZE];
    vsnprintf(buf, sizeof(buf), fmt, args);
    addEntry(level, tag, buf);
}

void Logger::log(LogLevel level, const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog(level, tag, fmt, args);
    va_end(args);
}

void Logger::error(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog(LogLevel::ERROR, tag, fmt, args);
    va_end(args);
}

void Logger::warn(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog(LogLevel::WARN, tag, fmt, args);
    va_end(args);
}

void Logger::info(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog(LogLevel::INFO, tag, fmt, args);
    va_end(args);
}

void Logger::debug(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vlog(LogLevel::DEBUG, tag, fmt, args);
    va_end(args);
}

void Logger::sendLogsHtml(ChunkSender send) {
    send("<div id='logbox' style='font-family:monospace;font-size:12px;background:#1a1a1a;"
         "color:#eee;padding:10px;border-radius:5px;max-height:500px;overflow-y:auto;'>"
         "<style>"
         ".log-E{color:#ff6b6b}.log-W{color:#feca57}.log-I{color:#5cd85c}.log-D{color:#48dbfb}"
         ".log-time{color:#888;margin-right:10px}"
         "</style>");

    int start = (_count < LOG_ENTRIES) ? 0 : _head;
    char line[256];
    for (int i = 0; i < _count; i++) {
        int idx = (start + i) % LOG_ENTRIES;
        LogEntry& entry = _buffer[idx];
        unsigned long s = entry.timestamp / 1000;
        unsigned long ms = entry.timestamp % 1000;

        // Build escaped message in-place
        String escaped;
        escaped.reserve(LOG_ENTRY_SIZE);
        for (int j = 0; entry.message[j] && j < LOG_ENTRY_SIZE; j++) {
            char c = entry.message[j];
            if (c == '<') escaped += "&lt;";
            else if (c == '>') escaped += "&gt;";
            else if (c == '&') escaped += "&amp;";
            else escaped += c;
        }

        snprintf(line, sizeof(line),
            "<div class='log-%c'><span class='log-time'>[%3lu.%03lu]</span>",
            entry.level, s, ms);

        send(String(line) + escaped + "</div>");
    }

    send("</div>");
}
