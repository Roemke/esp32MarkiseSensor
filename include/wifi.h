#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "logging.h"
#include <ArduinoJson.h>

//wifi.h
//struct fuer WLAN Daten im EEPROM
struct WifiData
{
    char ssid[32];
    char password[64];
    uint8_t magic = 0; //0x43 fuer valide Daten
} ;
extern WifiData wifiData;
#define EEPROM_WIFI_ADDR 0
#define EEPROM_WIFI_SIZE sizeof(WifiData)

extern String wifiMode;
extern String wifiMacAp;
extern String wifiMacSta;

// öffentliche API
void wifiSetup();
bool wifiIsConnected();
String wifiGetIP();
void wifiSetCredentials(const char* ssid, const char* password);
