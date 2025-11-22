#pragma once
#include <Arduino.h>
#include <vector>

void logBufferAdd(const char *line);
void logPrintln(const String &text);
void logPrintln(const char* text);
void logPrintf(const char* format, ...);

extern char logBuffer[50][128]; // 50 Zeilen à 128 Zeichen
extern uint8_t logIndex; // Index der nächsten Zeile
extern uint8_t logCount; // Wie viele Zeilen aktuell befüllt sind   

extern bool insideWebSocketEvent;
extern std::vector<String> pendingMessages;

#define LOG_BUFFER_SIZE 50
#define LOG_LINE_LENGTH 128
