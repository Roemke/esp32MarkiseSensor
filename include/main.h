#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
#include <LittleFS.h>
#include <DHTesp.h>
#include <vector>


// Function declarations
String processor(const String& var);
template <typename T>
void informClients(const String& action, T value);
void initialInformClient(AsyncWebSocketClient *client);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void setupOTA();
void setupWebsocket();
struct PendingWSMsg {
    uint32_t clientId; // 0 = broadcast, sonst Zielclient
    String msg;
};
extern std::vector<PendingWSMsg> wsOutbox;
