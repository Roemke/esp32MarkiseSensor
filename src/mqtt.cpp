// mqtt.cpp
#include <WiFi.h>
#include <PubSubClient.h>
#include "mqtt.h"
#include "credentials.h"
#include "logging.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

AwningMQTT mqtt;
//AwningMQTT* AwningMQTT::instance = nullptr; //call back ist raus
/*
    Hinweis: hier ist viel im zusammenhang mit Chat GPT entstanden, mir ist die notwendigkeit der
    callBackBrücke nicht ganz klar, das ist doch ein ziemlicher Aufwand geworden.
    Meines Erachtens sollte es auch gehen, indem ich beim Begin einen funktionszeiger übergebe,
    der dann direkt als callback fungiert, ohne die statische Methode und die Instanzvariable
    Außerdem LWT (last will testament): wenn die Verbindung abbricht, wird auf dem availability topic "offline" 
    veröffentlicht (retained). Beim Verbinden wird "online" veröffentlicht.
*/
// ---------- MQTT-Konfiguration speichern/laden ----------
void AwningMQTT::saveConfig(){
  // Hier könnten die MQTT-Daten in LittleFS oder EEPROM gespeichert werden
  // Für dieses Beispiel wird nur eine Log-Ausgabe gemacht
  logPrintf("MQTT Konfiguration speichern: BROKER=%s, PORT=%d\n", this->broker.c_str(), this->port);
  File f = LittleFS.open("/mqttdata.json", "w");
    if (!f) {
        logPrintln("Fehler beim Öffnen der Datei zum Speichern der MQTT-Daten");
        return;
    }
    f.printf("{\"broker\":\"%s\",\"port\":%d}", this->broker.c_str(), this->port);
    f.close();
}

void AwningMQTT::loadConfig(){
    if (!LittleFS.exists("/mqttdata.json")) {
        logPrintln("⚠️ Keine mqttdata.json gefunden – Standardwerte verwenden");
        this->saveConfig();
        return;
      }    
    File f = LittleFS.open("/mqttdata.json", "r");
    if (!f) {
        logPrintln("Fehler beim Öffnen der Datei zum Laden der MQTT-Daten, Standardwerte verwenden");
        this->saveConfig();
        return;    
    }
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        logPrintf("❌ JSON Fehler: %s\n", err.c_str());
        this->saveConfig();
        return;
    }

    // Werte übernehmen (mit Defaults)
    this->broker = doc["broker"] | mqttBROKER; //aus credentials.h
    this->port   = doc["port"]   | mqttPORT;   //aus credentials.h

    logPrintln("✅ MqttConfig geladen");
}

// ---------- MQTT-Betrieb ----------
void AwningMQTT::begin(const char * userArg, const char* passArg, const char* clientIdArg) {
    user = userArg;
    pass = passArg;
    clientIdAuto = (clientIdArg && strlen(clientIdArg) > 0) ? String(clientIdArg) : makeAutoClientId();

    lastAppliedBroker = broker;
    lastAppliedPort = port;

    mqtt.setServer(broker.c_str(), port);
    //mqtt.setCallback(onMqttStatic /* oder dein rawCb */);
    mqtt.setKeepAlive(15);     // Sekunden – weniger Ping-Druck
    mqtt.setSocketTimeout(2);  // Sekunden – Connect/IO blockiert nur kurz

    //instance = this;  
    lastAttemptMs = 0; // erster Versuch im nächsten poll()
}
void AwningMQTT::poll() {
    if (!enable) return;
    if (WiFi.status() != WL_CONNECTED) return;
  
    // Server zur Laufzeit geändert? Dann neu setzen
    if (broker != lastAppliedBroker || port != lastAppliedPort) {
      if (mqtt.connected()) mqtt.disconnect();
      lastAppliedBroker = broker; 
      lastAppliedPort = port;      
      mqtt.setServer(broker.c_str(), port);           
      serverChangedAt = millis();      // Cooldown starten
      lastAttemptMs   = 0;             // ersten Versuch nach Cooldown erlauben
      return;                          // hier bewusst raus, damit WS Luft hat
    }
    // Nach Wechsel kurze Ruhephase, bevor wir connecten
    if (serverChangedAt && millis() - serverChangedAt < connectCooldownMs) {
        yield();                         // ESP8266: Scheduler bedienen
        return;
    }

    if (mqtt.connected()) {
      mqtt.loop();
      return;
    }
  
    unsigned long now = millis();
    if (now - lastAttemptMs < reconnectIntervalMs) return;
    lastAttemptMs = now;
  
    attemptConnect();
  }
  
  void AwningMQTT::disconnect() {
    mqtt.publish(willTopic, R"({"availability":"offline"})", true);
    if (mqtt.connected()) mqtt.disconnect();
  }
  
  
  bool AwningMQTT::publishClimateStatus(const String& json) {
    if (!mqtt.connected()) return false;
    logPrintf("MQTT TX topic=%s payload=%s\n", topicClimate.c_str(), json.c_str());
    return mqtt.publish(topicClimate.c_str(), json.c_str(), false);//not retained
  }

  bool AwningMQTT::publishAwningStatus(const String& json) {
    if (!mqtt.connected()) return false;
    logPrintf("MQTT TX topic=%s payload=%s\n", topicAwning.c_str(), json.c_str());
    return mqtt.publish(topicAwning.c_str(), json.c_str(), false);//not retained
  }
  
  
  void AwningMQTT::attemptConnect() {
    if (broker.length() == 0 || port == 0) return;
  
    bool ok = false;
    // LWT: availability → "offline" (retained)
    if (user && pass) {
      ok = mqtt.connect(clientIdAuto.c_str(),
                        user.c_str(), pass.c_str(),
                        willTopic,1,true,willPayload); //true = clean session
      //topicAvai als LWT(lastg will testament), QoS 1, retained, "offline" als payload                  
    } else {
      ok = mqtt.connect(clientIdAuto.c_str(),"","", // 
                      willTopic,1,true,willPayload);
      logPrintf("MQTT: Verbinde zu %s:%d als %s ohne Auth\n", broker.c_str(), port, clientIdAuto.c_str());
    }
  
    if (ok) {
      //logPrintf("MQTT: subscribed to %s\n", topicCmd.c_str());//alt
      // Online signalisieren (retained)
      //mqtt.publish(topicAvail.c_str(), "online", true);
      mqtt.publish(willTopic, R"({"availability":"online"})", true);
    }
  }
  
  // ---------- Callback-Brücke ---------- nicht mehr genutzt
  /*
  void AwningMQTT::setOnCmd(void (*handler)(const String&)) {
    cmdHandler = handler;
  }
  void AwningMQTT::onMqttStatic(char* topic, byte* payload, unsigned int len) {
    if (instance) instance->onMqtt(topic, payload, len);
  }
  
  void AwningMQTT::onMqtt(char* topic, byte* payload, unsigned int len) {
    logPrintf("MQTT RX topic=%s payload=%.*s\n", topic, len, (const char*)payload);
    String t(topic);
    String msg; 
    msg.reserve(len);
    for (unsigned int i = 0; i < len; i++) msg += (char)payload[i];
    msg.trim(); msg.toLowerCase();
  
    if ( cmdHandler) {
      cmdHandler(msg);
    }
  }
  */
  // ---------- Interne Helfer als Klassenmethode ----------
  String AwningMQTT::makeAutoClientId() {
    String mac = WiFi.macAddress(); // "AA:BB:CC:DD:EE:FF"
    mac.replace(":", "");
    return "awning-" + mac; // awning-AABBCCDDEEFF 
  }