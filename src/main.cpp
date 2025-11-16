//main.cpp

//#include <EEPROM.h> umgestellt auf littlefs
// Eigene Header
//#include "credentials.h" //fuer zuhause, eigentlich überflüssig mit wifi.h
#include "main.h"
#include "indexHtmlJS.h"
#include "wifi.h"
#include "logging.h"
#include "mqtt.h"

// ---- WebServer + WebSocket ----
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
// DHT22 (AM2302)
DHTesp dht;
bool dhtReady = false;
float curTemp = NAN, curHum = NAN;
uint32_t lastDhtMs = 0;
static const char* REED_CFG_PATH = "/reed.json";


//fuer die reedPins
struct ReedPins {
  int markise = 26; // Defaults
  int s1 = 25;
  int s2 = 27; 
  int t = 23;   // Temperatur, kein Reed, aber speichern
} reedPins;



// Entprellung für Reedkontakte (aktiv LOW)
struct Debounce {
  int pin = -1;
  bool stable = false;   // entprellt (true = Reed geschlossen = LOW)
  bool lastRaw = false;
  uint32_t lastChange = 0;
  uint16_t debounceMs = 20;
} dbM, dbS1, dbS2;

static inline bool rawReed(int pin) { return digitalRead(pin) == LOW; }

bool updateDebounce(Debounce& d, uint32_t nowMs) {
  bool raw = rawReed(d.pin);
  bool changed = false;
  if (raw != d.lastRaw) { d.lastRaw = raw; d.lastChange = nowMs; }
  if ((nowMs - d.lastChange) >= d.debounceMs && d.stable != d.lastRaw) {
    d.stable = d.lastRaw;
    changed = true;
  }
  return changed;
}

// Sensor-Snapshot als JSON-String bauen
String makeSensorMsg() {
  JsonDocument doc;
  doc["action"] = "sensor";
  JsonObject v = doc["value"].to<JsonObject>();
  if (!isnan(curTemp)) v["t"] = curTemp;
  if (!isnan(curHum))  v["h"] = curHum;
  v["m"]  = (int)dbM.stable;
  v["s1"] = (int)dbS1.stable;
  v["s2"] = (int)dbS2.stable;
  String msg;
  serializeJson(doc, msg);
  return msg;
}


static bool isSafeInputPullupPin(int gpio) {
  // verbotene Gruppen
  if (gpio >= 6 && gpio <= 11) return false;  // Flash
  if (gpio >= 34 && gpio <= 39) return false;  // keine internen Pullups
  if (gpio == 0 || gpio == 2 || gpio == 12 || gpio == 15) return false; // Boot-Straps
  // ansonsten ok (0..39)
  return gpio >= 0 && gpio <= 39;
} 
bool reedSaveConfig() {
  File f = LittleFS.open(REED_CFG_PATH, "w");
  if (!f) return false;
  JsonDocument doc;
  doc["markise"] = reedPins.markise;
  doc["s1"] = reedPins.s1;
  doc["s2"] = reedPins.s2;
  doc["t"] = reedPins.t;
  bool ok = (serializeJson(doc, f) > 0);
  f.close();
  return ok;
}

bool reedLoadConfig() {
  if (!LittleFS.exists(REED_CFG_PATH)) {
    // Defaults erstmal speichern
    return reedSaveConfig();
  }
  File f = LittleFS.open(REED_CFG_PATH, "r");
  if (!f) return false;
  JsonDocument doc;
  auto err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  int m = doc["markise"] | reedPins.markise;
  int s1 = doc["s1"] | reedPins.s1;
  int s2 = doc["s2"] | reedPins.s2;
  int t = doc["t"] | reedPins.t;

  // Validieren
  if (isSafeInputPullupPin(m)) reedPins.markise = m;
  if (isSafeInputPullupPin(s1)) reedPins.s1 = s1;
  if (isSafeInputPullupPin(s2)) reedPins.s2 = s2;
  if (isSafeInputPullupPin(t)) reedPins.t = t;
  return true;
}

void reedApplyPins() {
  pinMode(reedPins.markise, INPUT_PULLUP);
  pinMode(reedPins.s1, INPUT_PULLUP);
  pinMode(reedPins.s2, INPUT_PULLUP);
  pinMode(reedPins.t, INPUT);
  
}

static void mqttPublishAwning() {
  JsonDocument doc;
  bool closed = dbM.stable;                 // dein Reed M als „geschlossen/offen“
  doc["closed"] = closed;
  // optional: alte Namenskompatibilität
  doc["m"] = closed ? "geschlossen" : "offen";
  // optional: S1/S2 als binäre Readings
  doc["s1"] = dbS1.stable ? 1 : 0;
  doc["s2"] = dbS2.stable ? 1 : 0;
  // optional: Prozent p, falls Du sie berechnen kannst
  // doc["p"] = isnan(percent) ? nullptr : (int)lroundf(constrain(percent,0,100));

  String payload; 
  payload.reserve(96);
  serializeJson(doc, payload);
  mqtt.publishAwningStatus(payload);        // -> awning/status
}

static void mqttPublishClimate() {
  JsonDocument doc;
  if (isnan(curTemp)) doc["t"] = nullptr;
  else                doc["t"] = roundf(curTemp * 10) / 10.0f;

  if (isnan(curHum))  doc["h"] = nullptr;
  else                doc["h"] = roundf(curHum * 10) / 10.0f;

  String payload; 
  payload.reserve(96);
  serializeJson(doc, payload);
  mqtt.publishClimateStatus(payload);       // -> climate/status
}


//Prozessor um ggf. Werte zu setzen (webseite)
String processor(const String& var)
{
  String result = "";
 
  if (var == "COPYRIGHT")
    result = "2025 by Roemke";
  else if (var == "WIFI_MAC_AP")
    result = wifiMacAp;
  else if (var == "WIFI_MAC_STA")
    result = wifiMacSta;
  else if (var == "WIFI_MAC_MODE")
    result = wifiMode;
  else if (var == "MQTT_BROKER")
    result = mqtt.broker;
  else if (var == "MQTT_PORT")
    result = String(mqtt.port);
  else if (var == "GPIO_MARKISE")
    result = String(reedPins.markise);
  else if (var == "GPIO_S1")
    result = String(reedPins.s1);
  else if (var == "GPIO_S2")
    result = String(reedPins.s2);
  else if (var == "GPIO_T")
    result = String(reedPins.t);
  return result;
}



// ---- WebSocket Event Handler ----
// Nachricht an alle Clients senden
template <typename T>
void informClients(const String& action, T value) 
{
  JsonDocument doc;
  doc["action"] = action;
  doc["value"] = value;   // JsonVariant nimmt String oder int

  String msg;
  serializeJson(doc, msg);
  ws.textAll(msg);
}


void initialInformClient(AsyncWebSocketClient *client)
{  
  JsonDocument doc;
  doc["action"] = "init";
  // Log-Array hinzufügen
  JsonArray logArr = doc["logs"].to<JsonArray>();
  for (uint8_t i = 0; i < logCount; i++) {
    uint8_t idx = (logIndex + LOG_BUFFER_SIZE - logCount + i) % LOG_BUFFER_SIZE;
    logArr.add(logBuffer[idx]);
  }
  String msg;
  serializeJson(doc, msg);
  client->text(msg);
  client->text(makeSensorMsg());
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
  AwsEventType type, void *arg, uint8_t *data, size_t len)
{  
  if (type == WS_EVT_CONNECT) 
  {
    logPrintf("📡 Client #%u verbunden\n", client->id());
    // Aktuellen Status senden
    initialInformClient(client);
  }
  else if(type != WS_EVT_DATA) 
    return;

  logPrintf("Event-Type: %d, Heap: %d\n", type, ESP.getFreeHeap());
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

// Debug aller Bedingungen
if(!info->final) {
    logPrintf("❌ WS: info->final == false\n");
    return;
}

if(info->index != 0) {
    logPrintf("❌ WS: info->index = %u (erwartet 0)\n", info->index);
    return;
}

if(info->len != len) {
    logPrintf("❌ WS: info->len (%u) != len (%u) -> frame fragmentiert?\n",
              info->len, len);
    return;
}

if(info->opcode != WS_TEXT) {
    logPrintf("❌ WS: opcode = %u (erwartet WS_TEXT=1)\n", info->opcode);
    return;
}

logPrintf("✔️ WS: Frame OK – Verarbeitung laeuft weiter\n");

  
  if(!info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) return;

  
  

  // StaticJsonDocument mit ausreichendem Puffer, deprecated sollte reichen ein JsonDocument zu nehmen
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, data,len);
  if(error) 
  {
    Serial.print("JSON Fehler: ");
    logPrintln(error.c_str());
    return;
  }

  String action = doc["action"] | "";
  JsonVariant value = doc["value"];
  String valueStr = value.isNull() ? "" : value.as<String>();
  logPrintf("Aktion: %s, Wert: %s\n", action.c_str(), valueStr.c_str());
   
  if (action == "wifiSetCredentials")
  {
      String ssid = value["ssid"] | "";
      String pass = value["password"] | "";
      logPrintf("Neue WiFi Daten: SSID=%s, PASS=%s\n", ssid.c_str(), pass.c_str());
      if (ssid.length() > 0)
      {
          wifiSetCredentials(ssid.c_str(), pass.c_str());
          informClients("wifiState", "saved");
          ESP.restart();  // Neustart mit neuen Daten
      }
  }
  else if (action == "mqttSet")
  {
      String broker = value["broker"] | "";
      int port = value["port"] | 1883;
      logPrintf("Neue MQTT Daten: BROKER=%s, PORT=%d\n", broker.c_str(), port);
      if (broker.length() > 0)
      {
          mqtt.broker = broker;
          mqtt.port = port;
          mqtt.saveConfig();          
          // Kein Neustart notwendig, MQTT-Verbindung wird neu initialisiert
      }
  }
  else if (action == "reedSetPins") 
  {
    int m  = value["markise"] | reedPins.markise;
    int s1 = value["s1"]      | reedPins.s1;
    int s2 = value["s2"]      | reedPins.s2;
    int t = value["t"]      | reedPins.t;
    // Validieren
    if (!isSafeInputPullupPin(m) || !isSafeInputPullupPin(s1) || !isSafeInputPullupPin(s2)) {
      informClients("error", "Ungueltige GPIOs (keine Pullups/Flash/Bootstraps)");
      return;
    }
    // Vorherige Pins sind INPUT_PULLUP – keine Deinit nötig,
    // optional könnte man hier prüfen, ob Pins sich geändert haben.
    reedPins.markise = m;
    reedPins.s1 = s1;
    reedPins.s2 = s2;
    reedPins.t = t;
    reedApplyPins();
    reedSaveConfig();
    informClients("reedPinsSaved", "ok");
    logPrintf("GPIO Pins aktualisiert: M=%d, S1=%d, S2=%d, T=%d\n", m, s1, s2,t);
  }
  else if (action == "reboot") 
  {
    logPrintln("Reboot Befehl empfangen, Neustart...");
    informClients("rebooting", "ESP wird neu gestartet");
    delay(1000); // kurze Pause, damit Nachricht gesendet wird
    ESP.restart();
  }

}

void setupDHTandSensor()
{
    // Debouncer initialisieren
    dbM.pin = reedPins.markise;
    dbS1.pin = reedPins.s1;
    dbS2.pin = reedPins.s2;
    uint32_t now = millis();
    dbM.lastRaw = dbM.stable = rawReed(dbM.pin);
    dbS1.lastRaw = dbS1.stable = rawReed(dbS1.pin);
    dbS2.lastRaw = dbS2.stable = rawReed(dbS2.pin);
    dbM.lastChange = dbS1.lastChange = dbS2.lastChange = now;
  
    // DHT22 (AM2302) starten
    dht.setup(reedPins.t, DHTesp::DHT22);
    dhtReady = true;    
}

void setupOTA() {
  ElegantOTA.begin(&server); // Startet ElegantOTA  
  
  ElegantOTA.onStart([]() {
    logPrintln("OTA Start");
  });

  ElegantOTA.onEnd([](bool success) {
    success ? logPrintln("OTA Ende, Neustart...") : logPrintln("OTA Ende mit Fehler");
    success ? ESP.restart() : logPrintln("Kein Neustart");   
  });

  ElegantOTA.onProgress([](unsigned int progress, unsigned int total) {
    char buf[64];
    snprintf(buf, sizeof(buf), "OTA Fortschritt: %u/%u", progress, total);
    logPrintln(buf);
    informClients("ota", buf);
  });
  
  logBufferAdd("🌐 OTA Update verfügbar unter /update");
}

void setupWebsocket()
{
  // WebSocket einbinden
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // lustig, ich bin alt,  [] leitet einen Lambda Ausdruck ein, also eine anonyme Funktion
  // die gabs frueher nicht :-), dafür mehr Lametta
  server.on("/favicon.ico", [](AsyncWebServerRequest *request)
            {
              request->send(204); // no content
            });

  // Hauptseite
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", index_html, processor); });
  //jsonDaten abfragen - liefert hier zunächst letzter Stand, später evtl. reedKontakt abfragen
  server.on("/getData", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              JsonDocument doc;
              doc["status"] = "ok";
              String response;
              serializeJson(doc, response);
              request->send(200, "application/json", response);
            });
}

//folgendes nicht mehr benötigt, da kein Befehl empfangen wird
/*
void onMqttCommand(const String& cmd)
{
    logPrintf("MQTT Befehl empfangen: %s\n", cmd.c_str());
}
*/
void statusPoll() {
  uint32_t now = millis();

  // Awning: Änderungen separat erfassen
  bool awnChanged = false;
  awnChanged |= updateDebounce(dbM,  now);
  awnChanged |= updateDebounce(dbS1, now);
  awnChanged |= updateDebounce(dbS2, now);

  // Klima: Änderungen mit Hysterese erfassen
  bool cliChanged = false;
  if (dhtReady && (now - lastDhtMs) >= 2000) {
    lastDhtMs = now;
    TempAndHumidity th = dht.getTempAndHumidity();
    if (!isnan(th.temperature) && !isnan(th.humidity)) {
      bool tChanged = isnan(curTemp) || fabsf(th.temperature - curTemp) >= 0.1f;
      bool hChanged = isnan(curHum)  || fabsf(th.humidity    - curHum)  >= 0.1f;
      if (tChanged) curTemp = th.temperature;
      if (hChanged) curHum  = th.humidity;
      cliChanged = (tChanged || hChanged);
    }
  }

  // WebSocket wie gehabt
  if (awnChanged || cliChanged) {
    ws.textAll(makeSensorMsg());
  }

  // MQTT: getrennt publizieren
  if (awnChanged) mqttPublishAwning();   // neu
  if (cliChanged) mqttPublishClimate();  // neu
}

 
 


// ---- Setup und Loop ----     
void setup()
{
  Serial.begin(115200);
  if (!LittleFS.begin(true)) { // true = bei Bedarf formatieren
    Serial.println("LittleFS mount failed");
    while (true) { delay(1000); }
  }
  wifiSetup();

  // OTA einrichten
   setupOTA();

   setupWebsocket();

   server.begin();
   logPrintln("HTTP-Server gestartet");

   // MQTT initialisieren
    mqtt.loadConfig();
    mqtt.begin();    
   //mqtt.setOnCmd(onMqttCommand); // keine Befehle vom MQTT-Server
   // Reed-Pins laden und anwenden
    reedLoadConfig();
    reedApplyPins();  
    setupDHTandSensor();
}

void loop() 
{
  
  ws.cleanupClients();
  //ElegantOTA.loop(); nur wenn blockierend, hier nicht nötig da Async

  statusPoll();
  mqtt.poll();
}