#include "wifi.h"
#include "logging.h"


WifiData wifiData;

String wifiMode;
String wifiMacAp;
String wifiMacSta;

static void saveWifiData() {
    File file = LittleFS.open("/wifiData.json", "w");
    if (!file) {
        logPrintln("Fehler beim Öffnen der Datei zum Speichern der WiFi-Daten.");
        return;
    }
    file.print("{\"ssid\":\"");
    file.print(wifiData.ssid);
    file.print("\",\"password\":\"");
    file.print(wifiData.password);
    file.print("\",\"magic\":");
    file.print(wifiData.magic);
    file.println("}");
    file.close();
}
bool loadWifiData() {
    File file = LittleFS.open("/wifiData.json", "r");
    if (!file) {
        logPrintln("WiFi-Daten nicht gefunden, starte AP...");
        return false;
    }
    String content = file.readString();
    file.close();

    // JSON parsen (hier könntest du auch ArduinoJson verwenden)
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, content);
    if (error) {
        logPrintln("Fehler beim Parsen der WiFi-Daten.");
        return false;
    }

    // Daten in wifiData speichern
    strncpy(wifiData.ssid, doc["ssid"], sizeof(wifiData.ssid) - 1);
    strncpy(wifiData.password, doc["password"], sizeof(wifiData.password) - 1);
    wifiData.magic = doc["magic"];

    if (wifiData.magic != 0x43) {
        logPrintln("WiFi-Daten ungültig, starte AP...");
        return false;
    }
    
    logPrintln("WiFi-Daten geladen.");
    return true;
}

void wifiSetCredentials(const char* ssid, const char* password) {
    strncpy(wifiData.ssid, ssid, sizeof(wifiData.ssid) - 1);
    strncpy(wifiData.password, password, sizeof(wifiData.password) - 1);
    wifiData.ssid[sizeof(wifiData.ssid)-1] = 0;
    wifiData.password[sizeof(wifiData.password)-1] = 0;
    wifiData.magic = 0x43;
    saveWifiData();
}

void wifiSetup() {
    // Alle Verbindungen sauber trennen
    WiFi.mode(WIFI_AP_STA); // beide aktiv, damit wir auf beide MACs zugreifen können
    wifiMacAp = WiFi.softAPmacAddress();
    wifiMacSta = WiFi.macAddress();
    delay(1000);
    logPrintln("=== WiFi Reset ===")    ;
    
    // Kompletter Hardware-Reset des WiFi
    WiFi.disconnect(true);  // disconnect + disable STA
    WiFi.softAPdisconnect(true); // disconnect + disable AP
    delay(1000);
    
    // WiFi komplett ausschalten
    WiFi.mode(WIFI_OFF);
    delay(2000);
    
    // Speicher bereinigen
    yield();

    
    logPrintf("Heap nach Reset: %d\n", ESP.getFreeHeap());   
    delay(1000);
    if (loadWifiData()) {
        logPrintf("Verbinde mit WLAN %s ...\n", wifiData.ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiData.ssid, wifiData.password);

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
            logPrintf("\nVerbunden, IP: %s\n", WiFi.localIP().toString().c_str());
            wifiMode = "client-Modus, IP " + WiFi.localIP().toString();
            WiFi.setSleep(false);
            WiFi.setAutoReconnect(true);
            return;
        }
    }

    // AP-Modus mit sauberem Start
    logPrintln("Starte AP...");
    WiFi.mode(WIFI_OFF);
    delay(1000);
    logPrintln("Vor dem setzen des AP-Modus");
    WiFi.mode(WIFI_AP);
    logPrintln("AP-Modus gesetzt");
    delay(1000);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    // Vereinfachte AP-Konfiguration
    bool success = WiFi.softAP("MarkisenSensor");
    logPrintf("AP Erfolg: %d\n", success);
    delay(2000);
    logPrintf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    wifiMode = "AP-Modus, IP " + WiFi.softAPIP().toString();
}

bool wifiIsConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String wifiGetIP() {
    if (wifiIsConnected())
        return WiFi.localIP().toString();
    else
        return WiFi.softAPIP().toString();
}
