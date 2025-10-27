#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
#include <LittleFS.h>
#include <DHTesp.h>


// Function declarations
String processor(const String& var);
template <typename T>
void informClients(const String& action, T value);
void initialInformClient(AsyncWebSocketClient *client);
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void setupOTA();
void setupWebsocket();

#endif // MAIN_H