

# Markisensensoren
Ein wenig was zur schaltung

# Schaltung Windsensor
den habe ich noch nicht:
Dieser liefert 20 Pulse pro s bei 1,75m/s
wind, viel mit chatgpt gespielt, er liefert teilweise widersprüchliche



Schaltpläne, daher hier nochmal die mir sinnvoll erscheinende Variante
Windsensor: https://www.amazon.de/dp/B0872XPWFC?ref=ppx_yo2ov_dt_b_fed_asin_title
Mumusuki 0-70 m/s Umgebungssignal Ausgang Impulstyp Kohlenstoff 
Windgeschwindigkeitssensor Anemometer DREI Tassen Sender Windgeschwindigkeitssensor 

Hier noch einmal die klare, richtige Anordnung (ASCII-Schematisch):

```text
3.3V
     |
    4.7k  <-- Pull-up (zieht Signal auf 3.3V)
     |
     +------ Node_Signal ----+------> [1k] ----> GPIO (ESP32)
     |                      |
     |                     (optional)
     |                      | 
   Sensor OUT (open-col.)   --- C = 10 nF
     |                      --- 
    GND                     |
                            GND
``


Erläuterungen / wichtige Punkte

    Node_Signal ist der gemeinsame Punkt, an dem Pull-up, Sensor-OUT und Kondensator zusammentreffen.

    Sensor OUT ist open-collector / NPN: wenn aktiv → Node wird auf GND gezogen (LOW). 
    Wenn inaktiv → Pull-up zieht Node auf 3.3 V (HIGH).

    1 kΩ sitzt zwischen Node und dem ESP-GPIO (physisch nahe am ESP-Pin angebracht). 
    Er schützt den GPIO bei Fehlverdrahtung oder wenn der Pin versehentlich als Ausgang konfiguriert ist.

    C = 10 nF (optional) zwischen Node_Signal und GND dicht am ESP-Eingang: entstört sehr kurzzeitige Störungen, 
    aber klein genug, um keine 800 Hz-Impulse zu verschlucken (mit 4.7k → τ ≈ 47 µs).
    (Keramik-Kondensator)

    Gemeinsame Masse: Sensor-GND und ESP-GND unbedingt verbinden.

Strom-/Pegelrechnung (zur Sicherheit)

    Wenn Sensor LOW zieht: Strom durch Pull-up ≈ 3.3 V / 4.7 k ≈ 0.7 mA → unkritisch.

    Wenn aufgrund Boot/Fehlkonfig ein Pegelkonflikt entsteht (ESP Pin OUTPUT HIGH = 3.3 V, Sensor zieht auf GND), 
    begrenzt 1 k den Strom grob auf ~3.3 mA → schützt den Pin. (vorher war
    auch von Blitz und elektromagnetschen Störungen die Rede


## Reedschalter Markise 
Die Reed-Schalter für die Markise (GPIO 26 m, 25 s1 und 27 s2)

Sie sind folgender Typ: 
https://www.amazon.de/dp/B07Z4NCWDD?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1
normally closed, d.h. der magnet schließt das system (chat gpt sieht die
Bezeichnung genau anders herum) 

Schaltung ist vom GPIO 100nF Keramik auf GND (Entstörung) direkt am ESP pin
und 470Ohm in Serie im Signalleiter, als Schutz.
(bei der Windgeschichte ist es anders, da kommt erst der 1k Widerstand dann
der kondensator (vom esp aus gesehen), eine analyse durch chat gpt sagt aber
das auch dies ok ist, wobei bei der Windschaltung ja auch nicht der interne
pullup genutzt wird.
 
Hier wird kein externer pullup eingesetzt, da der Interne reicht. Dieser ist
relativ hochohmig 40-50k und zieht daher den GPIO nur langsam auf high, das
macht bei den Reeds für die markise aber nichts 

# Temperatur Luftfeuchtigkeit

Und dann noch ein dht22 engebaut in am2302 (hat nur 3 Beinchen) für
Temperatur und Luftfeuchtigkeit, ist direkt an GPIO angeschlossen (GPIO23)
und an 3.3V vom ESP

Stromversorgung ESP über USB, ansonsten alles auf einer Lochrasterplatine. 
Es würde sich aber ein PCB lohnen.
