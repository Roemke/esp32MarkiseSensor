#include <Arduino.h>
#include <ArduinoJson.h>
#include <stdarg.h>
#include "logging.h"
#include "main.h"

extern AsyncWebSocket ws;

char logBuffer[LOG_BUFFER_SIZE][LOG_LINE_LENGTH]; // 50 Zeilen à 128 Zeichen
uint8_t logIndex = 0; // Index der nächsten Zeile
uint8_t logCount = 0; // Wie viele Zeilen aktuell befüllt sind

void logBufferAdd(const char *line) {
    strncpy(logBuffer[logIndex], line, LOG_LINE_LENGTH - 1);
    logBuffer[logIndex][LOG_LINE_LENGTH - 1] = '\0';

    logIndex = (logIndex + 1) % LOG_BUFFER_SIZE;
    if (logCount < LOG_BUFFER_SIZE) logCount++;

    JsonDocument doc;
    doc["action"] = "log";
    doc["line"] = line;

    String msg;
    serializeJson(doc, msg);

    ws.textAll(msg);
}

void logPrintln(const String &text) {
    Serial.println(text);
    logBufferAdd(text.c_str());
}

void logPrintln(const char* text) {
    logPrintln(String(text));
}

void logPrintf(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    Serial.print(buf);

    String s = String(buf);
    int start = 0;
    int index;
    while ((index = s.indexOf('\n', start)) >= 0) {
        logBufferAdd(s.substring(start, index).c_str());
        start = index + 1;
    }
    if (start < s.length()) {
        logBufferAdd(s.substring(start).c_str());
    }
}