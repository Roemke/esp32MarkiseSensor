#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='de'>
<head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width'>
<title>MarkisenSensoren</title>
<style>
  body { font-family: Arial, sans-serif; margin: 0; padding: 0; }
  .tab { overflow: hidden; background-color: #333; }
  .tab button {
    background-color: inherit; border: none; outline: none;
    cursor: pointer; padding: 14px 16px; color: white;
  }
  .tab button:hover { background-color: #555; }
  .tab button.active { background-color: #111; }
  .tabcontent { display: none; padding: 20px; }
  .btn { padding: 10px 20px; margin: 10px; font-size: 16px; }
  .slider-container { margin: 15px 0; }
  .slider-container label { display: block; margin-bottom: 5px; }
  input[type=range], input[type=number] { width: 100%%; }
  .btn.active {
    background-color: green;
    color: white;
  }
  .invisible {
    display: none;
  }
  .infoField {
    background-color: #008000; 
    padding: 10px; 
    border: 10px inset #004000;
    margin-top: 10px;
  }  
  div {
    margin-bottom: 10px;
  }
  .kv { 
    display:flex; gap:10px; align-items:baseline; 
  }
  .kv label 
    { 
      min-width: 230px; 
      color:#444; 
    }

  .badge { display:inline-block; padding:2px 8px; border-radius:10px; background:#eee; color:#000; }
  .badge.ok      { background:#2e7d32; color:#fff; }  /* grün */
  .badge.warn    { background:#f9a825; color:#000; }  /* gelb */
  .badge.err     { background:#c62828; color:#fff; }  /* rot */
  .badge.neutral { background:#9e9e9e; color:#fff; }  /* grau */    
</style>

<script>
let gateway = `ws://${window.location.hostname}/ws`;
let websocket = null;


function addLogLine(line) {
    const logContainer = document.getElementById("logContainer");
    if (!logContainer) return;

    const div = document.createElement("div");
    div.textContent = line;
    logContainer.appendChild(div);
    logContainer.scrollTop = logContainer.scrollHeight; // scrollt automatisch nach unten
}

function initWebSocket() {
    if (websocket && (websocket.readyState === WebSocket.OPEN || websocket.readyState === WebSocket.CONNECTING)) {
        return;
    }
    websocket = new WebSocket(gateway);
    websocket.onopen = () => console.log("WebSocket verbunden");
    websocket.onclose = () => {websocket = null; setTimeout(initWebSocket, 3000);};

    websocket.onmessage = (e) => {
        console.log("Nachricht:", e.data);
        let data = JSON.parse(e.data);
        if (data.action === "init") 
        {
          // Initialdaten vom ESP: alle Werte auf einmal
          // → in die UI eintragen, ist hier nicht mehr viel

          if (Array.isArray(data.logs)) data.logs.forEach(line => addLogLine(line));
          if (data.sensors) updateStatusUI(data.sensors);
        }
        else if (data.action === "sensor") 
        {
          const v = data.value || {};
          updateStatusUI(v);
        }
        else if (data.action === "log") 
        {
          addLogLine(data.line);
        }

    };
}


function setText(id, text) {
  const el = document.getElementById(id);
  if (el) el.textContent = text;
}

function magnetText(v) { return v ? "da" : "weg"; }

function tempClass(t) 
{
  if (t < -10 || t > 50) return "err";
  if (t < 0 || t > 40)   return "warn";
  return "ok";
}
function humClass(h) 
{
  if (h < 10 || h > 95) return "err";
  if (h < 20 || h > 80) return "warn";
  return "ok";
}

function setBadge(id, text, cls) 
{
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = text;
  el.className = "badge " + (cls || "neutral");
}

function updateStatusUI(v) {
  if (typeof v.t === "number")  setBadge("valTemp", v.t.toFixed(1) + " °C", tempClass(v.t));
  if (typeof v.h === "number")  setBadge("valHum",  v.h.toFixed(1) + " %",  humClass(v.h));
  if (typeof v.m !== "undefined")  setBadge("valM",  magnetText(!!v.m),  !!v.m ? "ok" : "err");
  if (typeof v.s1 !== "undefined") setBadge("valS1", magnetText(!!v.s1), !!v.s1 ? "ok" : "err");
  if (typeof v.s2 !== "undefined") setBadge("valS2", magnetText(!!v.s2), !!v.s2 ? "ok" : "err");
}

function sendAction(action, value="") 
{
    if (websocket && websocket.readyState === WebSocket.OPEN)
        websocket.send(JSON.stringify({ action: action, value: value }));    
}

//noch nicht angeschaut, von chatgpt generiert
function openTab(evt, tabName) {
    let i, tabcontent, tablinks;
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) { tabcontent[i].style.display = "none"; }
    tablinks = document.getElementsByClassName("tablink");
    for (i = 0; i < tablinks.length; i++) { tablinks[i].className = tablinks[i].className.replace(" active", ""); }
    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";
}

function initUI() {
    initWebSocket();
    document.getElementById("defaultTab").click();


    // WLAN speichern
    const btnWifi = document.getElementById("btnWifiSave");
    if (btnWifi) btnWifi.addEventListener("click", () => {
        sendAction("wifiSetCredentials", {
            ssid: document.getElementById("wifiSsid").value,
            password: document.getElementById("wifiPass").value
        });
        document.getElementById("wifiInfo").classList.remove("invisible");
    });
    // MQTT speichern
    const btnMqtt = document.getElementById("btnMqttSave");
    if (btnMqtt) btnMqtt.addEventListener("click", () => {
        sendAction("mqttSet", {
            broker: document.getElementById("mqttBroker").value,
            port: document.getElementById("mqttPort").value
        });
    });
    
    // GPIO-Pins speichern
    const btnReed = document.getElementById("btnReedSave");
    if (btnReed) btnReed.addEventListener("click", () => {
        sendAction("reedSetPins", {
            markise: parseInt(document.getElementById("gpioMarkise").value, 10),
            s1:      parseInt(document.getElementById("gpioS1").value, 10),
            s2:      parseInt(document.getElementById("gpioS2").value, 10),
            t:      parseInt(document.getElementById("gpioT").value, 10) //kein Reed, aber speichern
        });
    });
    // Reboot Button
    const btnReboot = document.getElementById("btnReboot");
    if (btnReboot) btnReboot.addEventListener("click", () => {
        sendAction("reboot");
    });
}
window.addEventListener("load", initUI);
</script>
</head>

<body>
<div class="tab">
  <button class="tablink" id="defaultTab" onclick="openTab(event, 'status')">Status</button>
  <button class="tablink" onclick="openTab(event, 'wifi')">Setup</button>
  <button class="tablink" onclick="openTab(event, 'help')">Help</button>  
</div>
<div id="status" class="tabcontent">
  <h2>Status</h2>
  <div class="kv"><label>Temperatur:</label> <span class="badge" id="valTemp">--.- °C</span></div>
  <div class="kv"><label>Feuchtigkeit:</label> <span class="badge" id="valHum">--.- %%</span></div>
  <hr>
  <div class="kv"><label>Magnet Markise:</label> <span class="badge" id="valM">unbekannt</span></div>
  <div class="kv"><label>Magnet Sensor 1:</label> <span class="badge" id="valS1">unbekannt</span></div>
  <div class="kv"><label>Magnet Sensor 2:</label> <span class="badge" id="valS2">unbekannt</span></div>
</div>
<div id="wifi" class="tabcontent">
  <h2>WLAN Konfiguration</h2>
  <p>Mac-Adresse im AP-Modus: %WIFI_MAC_AP%</p>
  <p>Mac-Adresse im Client-Modus (STA): %WIFI_MAC_STA%</p>
  <p>aktueller Modus: %WIFI_MAC_MODE%</p>
  <div class="slider-container">
    <label for="wifiSsid">SSID</label>
    <input type="text" id="wifiSsid" placeholder="Netzwerkname">
  </div>
  <div class="slider-container">
    <label for="wifiPass">Passwort</label>
    <input type="password" id="wifiPass" placeholder="Passwort">
  </div>
  <button class="btn" id="btnWifiSave">Wifi Speichern</button>
  <div class="infoField invisible" id="wifiInfo">
    <strong>Daten übermittelt.</strong><br>  
    <strong>Hinweis:</strong> Nach dem Speichern der WLAN-Daten muss der ESP neu gestartet werden, 
    in der Regel geschieht dies automatisch.  Bitte ansonsten den ESP manuell neu starten.  
  </div>
  <h2>MQTT Konfiguration</h2>
  <div>
    <label for="mqttBroker">MQTT-Broker</label>
    <input type="text" id="mqttBroker" value="%MQTT_BROKER%" placeholder="MQTT-Broker Adresse">
  </div>
  <div>
    <label for="mqttPort">MQTT-Port</label>
    <input type="text" id="mqttPort" value="%MQTT_PORT%" placeholder="1883">
  </div>
  <button class="btn" id="btnMqttSave">MQTT Speichern</button>
  <div>ein Neustart ist nicht notwendig, die Verbindung wird automatisch neu hergestellt.</div>
<h2>Reedkontakte (GPIOs)</h2>
  <div>
    <label for="gpioMarkise">Markise Eingefahren Status (GPIO)</label>
    <input type="number" id="gpioMarkise" min="0" max="39" value="%GPIO_MARKISE%" placeholder="z.B. 26">
  </div>
  <div>
    <label for="gpioS1">Sensor 1 Status (GPIO)</label>
    <input type="number" id="gpioS1" min="0" max="39" value="%GPIO_S1%" placeholder="z.B. 25">
  </div>
  <div>
    <label for="gpioS2">Sensor 2 Status (GPIO)</label>
    <input type="number" id="gpioS2" min="0" max="39" value="%GPIO_S2%" placeholder="z.B. 27">
  </div>
  <div>
    <label for="gpioT">Temperatursensor (GPIO)</label>
    <input type="number" id="gpioT" min="0" max="39" value="%GPIO_T%" placeholder="z.B. 23">
  </div>
  <button class="btn" id="btnReedSave">GPIOs Speichern</button>
</div>
<div id="help" class="tabcontent">
  <h2>Help</h2>
  <p>Folgende Funktionen existiereen:</p>
  <ul>
    <li>Steuerung der Markise</li>
    <li>Setup der Servo-Endpunkte</li>
    <li>WLAN / MQTT Konfiguration</li>
    <li>OTA-Update über <a href="/update">diesen Link</a></li>
  </ul>

  <!-- Logs ans Ende des Help-Tabs -->
  <h3 style="display:inline;">Log-Nachrichten</h3> <button class="btn" id="btnReboot"> Reboot ESP</button>
  <div id="logContainer" style="height:300px; overflow:auto; background:#f0f0f0; padding:10px; font-family:monospace;"></div>
</div>

</body>
</html>
)rawliteral";
