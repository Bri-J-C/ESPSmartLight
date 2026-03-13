#pragma once

#include <Arduino.h>
#include <functional>

#define LOG_ENTRIES 50
#define LOG_ENTRY_SIZE 128

enum class LogLevel : char {
    ERROR = 'E',
    WARN  = 'W',
    INFO  = 'I',
    DEBUG = 'D'
};

struct LogEntry {
    unsigned long timestamp;
    char level;
    char message[LOG_ENTRY_SIZE];
};

typedef std::function<void(const String&)> ChunkSender;

class Logger {
public:
    void begin();
    void log(LogLevel level, const char* tag, const char* fmt, ...);
    void error(const char* tag, const char* fmt, ...);
    void warn(const char* tag, const char* fmt, ...);
    void info(const char* tag, const char* fmt, ...);
    void debug(const char* tag, const char* fmt, ...);
    void sendLogsHtml(ChunkSender send);

private:
    void vlog(LogLevel level, const char* tag, const char* fmt, va_list args);
    void addEntry(LogLevel level, const char* tag, const char* formatted);
    LogEntry _buffer[LOG_ENTRIES];
    int _head = 0;
    int _count = 0;
};

extern Logger logger;
