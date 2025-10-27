// mqtt.h
#pragma once
#include <Arduino.h>
#include <functional>
#include "credentials.h"
#include <WiFi.h>
#include <PubSubClient.h>


class AwningMQTT{
    public:
        String broker = mqttBROKER;            // aus credentials.h
        uint16_t port = mqttPORT;              // aus credentials.h


        void saveConfig();
        void loadConfig();
        // MQTT-Lebenszyklus (nicht-blockierend)
        void begin(const char* userArg = nullptr, const char* passArg = nullptr, const char* clientIdArg = nullptr);
        void poll();                // in loop() aufrufen
        void disconnect();
        bool isConnected() { return mqtt.connected(); }

        // Kommunikation
        bool publishAwningStatus(const String& json);   // no retain
        bool publishClimateStatus(const String& json);
        void setOnCmd(void (*handler)(const String&)); // Callback für empfangene Kommandos    

    private:
        //awning = markise
        
        String topicAwning= "awning/status"; 
        String topicClimate= "climate/status";
        //hier ist const char * besser, da feste Strings
        const char * willTopic= "device/status";        
        const char * willPayload = R"({"closed":null,"p":null,"t":null,"h":null,"availability":"offline"})";
        bool enable = true;       // false => MQTT komplett aus
        String user;            
        String pass;            

        // Interna        
        String lastAppliedBroker; //falls neuverbindung auftaucht, sollte unnoetig sein
        uint16_t lastAppliedPort = 0; //wie oben
        unsigned long serverChangedAt = 0;
        const unsigned long connectCooldownMs = 1500; // 1.5s Ruhe nach Wechsel
        WiFiClient   net;
        PubSubClient mqtt{net}; //gleichbedeutend mit PubSubClient mqtt(net);
        String       clientIdAuto;
        static String makeAutoClientId();

        void (*cmdHandler)(const String&) = nullptr;

        unsigned long lastAttemptMs = 0;
        uint32_t      reconnectIntervalMs = 5000; // 5s

        void attemptConnect();
        // Callback-Brücke (eine Instanz), Technik um ein Member als C-Style Callback zu verwenden
        // auch Trampolin genannt, nur eine Instanz erlaubt
        //wird hier nicht benötgt, callback, der sensor empfängt aber keine Daten vom Broker
        /*
        static AwningMQTT* instance ;
        static void onMqttStatic(char* topic, byte* payload, unsigned int len); //statische signatur, also ohne this
        // tatsächliche Callback-Methode, kann ich mir doch eigenltich sparen, wenn ich beim initialisieren eine 
        // Funktion übergebe - so ist nur eine Instanz erlaubt
        void onMqtt(char* topic, byte* payload, unsigned int len);
        */
};

extern AwningMQTT mqtt;





