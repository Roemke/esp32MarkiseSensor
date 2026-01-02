#pragma once
#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='de'>
<head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width'>
<title>MarkisenSensoren</title>
<style>
  body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #fafafa;}
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
  
  .kv {
    display:flex;
    gap:10px;
    align-items:baseline;
    margin-bottom: 10px;   /* schöner Abstand */
  }

  
  .kv label 
    { 
      min-width: 160px;
      flex-shrink: 0; 
      color:#444; 
    }

  .badge { display:inline-block; padding:2px 8px; border-radius:10px; background:#eee; color:#000; }
  .badge.ok      { background:#2e7d32; color:#fff; }  /* grün */
  .badge.warn    { background:#f9a825; color:#000; }  /* gelb */
  .badge.err     { background:#c62828; color:#fff; }  /* rot */
  .badge.neutral { background:#9e9e9e; color:#fff; }  /* grau */  
  
  
  .status-flex {
    display: flex;
    flex-flow: row nowrap;
    margin: 0 auto;       /* zentriert */
    
    
    align-items: stretch;
    justify-content: space-evenly;
    padding: 15px 50px;
    grid-template-columns: auto 1fr auto;
    column-gap: 0px;
    row-gap: 5px;
  }

  .status-left, .status-right {
    background: #eaeaea;
    padding: 10px 50px;
    border-radius: 8px;
    box-shadow: 0 1px 3px rgba(0,0,0,0.1);
    min-width: 300px;
  }
  
  .status-left hr {
    margin: 16px 0;  
  }
  .status-right div {
    width: fit-content;
    margin: 0px auto;
    margin-top: 5px;
  }


  /* Der große History-Chart darunter */
.history-wide {
    width: 80vw;
    margin: 0 auto;   /* garantiert perfekt symmetrisch! */
}

.history-wide canvas {
    width: 100%% ;
    height: 300px;
    display: block;
}
  
#windGauge {
    display: block;
    max-width: 100%;
    width: auto;              /* NICHT 100% → sonst oval */
    height: auto;
    aspect-ratio: 2 / 1;      /* garantiert Rundheit */
    margin: 0 auto;           /* zentriert */
    margin-bottom: 20px;
}

  
  .wind-chart-wrapper {
    margin-top: 10px;
  }

  .wind-chart-wrapper canvas {
    width: 100%%;
    max-width: 800px;
  }


/* Mobile */
  @media (max-width: 600px) {
    .history-wide canvas {
        height: 180px;
    }
    .status-flex {
      display: flex;
      flex-flow: column;
    }
    .status-left, .status-right {
      padding: 10px 10px;
    }
  }

</style>


<script>
let gateway = `ws://${window.location.hostname}/ws`;
let websocket = null;

// ---- Wind UI / Charts ----
let maxWind = 20;  // hier kannst du später auf 30 ändern
let latestWindValue = 0.0;

let windGaugeChart = null;
let windHistoryChart = null;
const windHistory = [];
// Gemeinsame Beaufort-Tabelle für beide Funktionen
const BEAUFORT_MAP = [
    { max: 0.2,  bft: 0,  label: "Flaute",          color: "#4caf50" },
    { max: 1.5,  bft: 1,  label: "Leiser Zug",       color: "#8bc34a" },
    { max: 3.3,  bft: 2,  label: "Leichte Brise",    color: "#cddc39" },
    { max: 5.4,  bft: 3,  label: "Schwache Brise",   color: "#ffeb3b" },
    { max: 7.9,  bft: 4,  label: "Mäßige Brise",     color: "#ffc107" },
    { max: 10.7, bft: 5,  label: "Frische Brise",    color: "#ff9800" },
    { max: 13.8, bft: 6,  label: "Starker Wind",     color: "#ff5722" },
    { max: 17.1, bft: 7,  label: "Steifer Wind",     color: "#f44336" },
    { max: 20.7, bft: 8,  label: "Stürmischer Wind", color: "#e53935" },
    { max: 24.4, bft: 9,  label: "Sturm",            color: "#c62828" },
    { max: 28.4, bft: 10, label: "Schwerer Sturm",   color: "#b71c1c" },
    { max: 33.0, bft: 11, label: "Orkan",            color: "#880e4f" },
    { max: 40.0, bft: 12, label: "Orkan+",           color: "#4a148c" },
    { max: 50.0, bft: 13, label: "Extrem",           color: "#1a237e" },
    { max: 80.0, bft: 14, label: "Mega-Orkan",       color: "#0d47a1" }
];

function windToBeaufort(v) {

  for (const e of BEAUFORT_MAP) {
    if (v <= e.max) return { bft: e.bft, label: e.label };
  }
  // Sollte prinzipiell nie erreicht werden
    const last = BEAUFORT_MAP[BEAUFORT_MAP.length - 1];
    return { bft: last.bft, label: last.label };
}

// ------------------------------------------------------


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

          if (v.w > maxWind)
          {
            while(v.w > maxWind) maxWind +=5;
            console.log("MaxWind automatisch auf", maxWind, "erhöht");
            document.getElementById("maxWindSlider").value = maxWind; //müsste send ausloesen
            document.getElementById("maxWindValue").textContent = maxWind;
            windHistoryChart.options.scales.y.max = maxWind*3.6;
            windHistoryChart.update();
          }          
          updateStatusUI(v);
        }
        else if (data.action === "log") 
        {
          addLogLine(data.line);
        }
        else if (data.action === "setMaxWind") 
        {
          maxWind = Number(data.value);
          console.log("Neuer MaxWind:", maxWind);
          if (windHistoryChart) {
              windHistoryChart.options.scales.y.max = maxWind*3.6;
              windHistoryChart.update();
          }
          if (windGaugeChart) updateWindUI(latestWindValue);
        }  
        else
        {
          addLogLine("sonstiges"+JSON.stringify(data));
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
function windClass(w) {
    if (w >= 15) return "err";     // Sturm
    if (w >= 8)  return "warn";    // starker Wind
    return "ok";                   // normal
}

function setBadge(id, text, cls) 
{
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = text;
  el.className = "badge " + (cls || "neutral");
}

//helper fuer fließenden übergang
function hexToRgb(hex) {
    const n = parseInt(hex.replace("#",""), 16);
    return { r: (n>>16)&255, g: (n>>8)&255, b: n&255 };
}

function linInterpolationColor(c1, c2, t) {
    return {
        r: c1.r + (c2.r - c1.r) * t,
        g: c1.g + (c2.g - c1.g) * t,
        b: c1.b + (c2.b - c1.b) * t
    };
}
function drawBeaufortGradientScale(ctx, cx, cy, r, lw, maxWind) {
    
    // Steps passend zu maxWind aufbauen — aber mit RGB-Farben
    const steps = [];
    let prev = 0;
    let prevColor = BEAUFORT_MAP[0].color;

    for (const s of BEAUFORT_MAP) {
        const end = Math.min(s.max, maxWind);
        steps.push({
            min: prev,
            max: end,
            c1: hexToRgb(prevColor),
            c2: hexToRgb(s.color)
        });
        prev = end;
        prevColor = s.color;
        if (end >= maxWind) break;
    }

    ctx.lineWidth = lw;

    const totalWind  = maxWind;
    const angleStart = Math.PI;
    const totalAngle = Math.PI;

    let SUB = 10; // Anzahl Untersegmente pro Beaufort-Stufe
    let delta = 0.1;
    for (const s of steps) {
        const span = s.max - s.min;
        if (span <= 0) continue;

        for (let j = 0; j < SUB; j++) {
            const t0 = j / SUB;
            const t1 = (j + 1) / SUB;

            const w0 = s.min + span * t0;
            const w1 = s.min + span * t1;

            // Farbe interpolieren
            const col = linInterpolationColor(s.c1, s.c2, t0);
            ctx.strokeStyle = `rgb(${col.r|0},${col.g|0},${col.b|0})`;

            let a1 = angleStart + (w0 / totalWind) * totalAngle - delta;
            let a2 = angleStart + (w1 / totalWind) * totalAngle +delta; //kleiner Puffer gegen Lücken
            ctx.beginPath();
            //pi ist links, bis 2pi ist der bogen obenrum, arc ist im uhrzeigersinn counterclockwise=false
            //eigentlich der standard. 
            ctx.arc(cx, cy, r, a1, a2, false);  // clockwise zeichnen 
            ctx.stroke();
        }
    }
}

function drawBeaufortTicks(ctx, cx, cy, rPointer, maxWind) {
    ctx.lineWidth = 2;
    ctx.strokeStyle = "rgba(0,0,0,0.7)";

    const angleStart = Math.PI;
    const totalAngle = Math.PI;

    for (const s of BEAUFORT_MAP) {

        const wind = Math.min(s.max, maxWind);
        if (wind <= 0 || wind > maxWind) continue;

        const frac  = wind / maxWind;
        const angle = angleStart + frac * totalAngle;

        const inner = rPointer - 12;
        const outer = rPointer + 4;

        const x1 = cx + Math.cos(angle) * inner;
        const y1 = cy + Math.sin(angle) * inner;
        const x2 = cx + Math.cos(angle) * outer;
        const y2 = cy + Math.sin(angle) * outer;

        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.lineTo(x2, y2);
        ctx.stroke();
    }
}


//----------------- Helper End ----------------
function drawWindGauge(value, maxWind) {
    const canvas = document.getElementById("windGauge");
    if (!canvas) return;
    const ctx = canvas.getContext("2d");

    const w = canvas.width;
    const h = canvas.height;

    // Mittelpunkt unten in der Mitte
    const margin = 10;       // Abstand zum Rand
    const lw = 80;           // gewünschte Strichstärke    

    const cx = w / 2;
    const cy = h-margin/2;
    const rGauge = Math.min(w / 2, h) - margin - lw/2-1; 
    const rPointer = rGauge + lw/2-margin/2;


    ctx.clearRect(0, 0, w, h);

    // Schutz: maxWind darf nicht 0 oder negativ sein
    if (!(maxWind > 0)) maxWind = 1;

    // Wert v ins gültige Intervall klemmen
    const v = Math.max(0, Math.min(value, maxWind));

    // Wir laufen von links (π) nach rechts (0) über den oberen Halbkreis
    //let prevFrac = 0;  // 0 = maxWind 0, 1 = maxWind erreicht
    //geaendert, jedes segment wird durch folgende routine in 0.5 Grad unterteilt
    drawBeaufortGradientScale(ctx, cx, cy, rGauge, lw, maxWind);
    drawBeaufortTicks(ctx, cx, cy, rPointer, maxWind);


    // ------ Zeiger ------
    const fracVal = v / maxWind;    
    const valAngle = Math.PI + fracVal * Math.PI;
    const px = cx + Math.cos(valAngle) * rPointer;
    const py = cy + Math.sin(valAngle) * rPointer;

    ctx.beginPath();
    ctx.strokeStyle = "#000";
    ctx.lineWidth = 3;
    ctx.moveTo(cx, cy);
    ctx.lineTo(px, py);
    ctx.stroke();

    // kleiner Punkt am Zentrum
    ctx.beginPath();
    ctx.fillStyle = "#000";
    ctx.arc(cx, cy, 5, 0, Math.PI * 2);
    ctx.fill();
}

function updateWindUI(w) {
    const v = Math.min(w, maxWind);  // sinnvoll & einfach
    drawWindGauge(v, maxWind);

    document.getElementById("windValueKMH").textContent = (v*3.6).toFixed(1);
    document.getElementById("windValue").textContent = v.toFixed(1);
    const bf = windToBeaufort(v);
    document.getElementById("windBft").textContent =
        `Beaufort ${bf.bft}: ${bf.label}`;

    // Verlauf weiter aktualisieren wie bisher …
    const now = new Date().toLocaleTimeString();
    windHistory.push({ t: now, v: (v*3.6) });
    if (windHistory.length > 1000) windHistory.shift();
    windHistoryChart.data.labels = windHistory.map(e => e.t);
    windHistoryChart.data.datasets[0].data = windHistory.map(e => e.v);//v ist bereits in km/h in der history
    windHistoryChart.update();
}

function initWindHistoryChart() {
    const chartCanvas = document.getElementById("windChart");
    if (!chartCanvas || !window.Chart) return;

    const ctx = chartCanvas.getContext("2d");

    windHistoryChart = new Chart(ctx, {
        type: "line",
        data: {
            labels: [],
            datasets: [{
                label: "Wind (History)",
                data: [],
                borderColor: "#1976d2",
                backgroundColor: "rgba(25,118,210,0.15)",
                lineTension: 0.35,
                borderWidth: 2,
                pointRadius: 0,
                fill: true
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            scales: {
                x: { display: false },
                y: {
                    min: 0,
                    max: maxWind*3.6,
                    grid: { color: "rgba(0,0,0,0.1)" }
                }
            },
            plugins: {
                legend: { display: true }
            }
        }
    });
}

function initWindUI() {
  initWindHistoryChart();   // ✔ Verlaufsgrafik initialisieren
  drawWindGauge(0, maxWind); // ✔ einmal initialer Gauge-Draw  
}

//----------------------
function updateStatusUI(v) {
  if (typeof v.t === "number")  setBadge("valTemp", v.t.toFixed(1) + " °C", tempClass(v.t));
  if (typeof v.h === "number")  setBadge("valHum",  v.h.toFixed(1) + " %%",  humClass(v.h));
  if (typeof v.m !== "undefined")  setBadge("valM",  magnetText(!!v.m),  !!v.m ? "ok" : "err");
  if (typeof v.s1 !== "undefined") setBadge("valS1", magnetText(!!v.s1), !!v.s1 ? "ok" : "err");
  if (typeof v.s2 !== "undefined") setBadge("valS2", magnetText(!!v.s2), !!v.s2 ? "ok" : "err");
  if (typeof v.w === "number") 
  { 
    setBadge("valWind", (v.w*3.6).toFixed(1) + " km/h", windClass(v.w));
    latestWindValue = v.w;
    updateWindUI(v.w);
  }

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
    initWindUI();//wind UI initialisieren


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
            t:       parseInt(document.getElementById("gpioT").value, 10), //kein Reed, aber speichern
            wind:    parseInt(document.getElementById("gpioWind").value, 10)
        });
    });
    // Reboot Button
    const btnReboot = document.getElementById("btnReboot");
    if (btnReboot) btnReboot.addEventListener("click", () => {
        sendAction("reboot");
    });
    const  btnCheck = document.getElementById("btnCheck");
    if (btnCheck) btnCheck.addEventListener("click", () => {
        sendAction("check", "Testnachricht vom Client");
    });
    
    // MaxWind Slider live anzeigen    
    const maxWindSlider = document.getElementById("maxWindSlider");
    const maxWindValue  = document.getElementById("maxWindValue");    
    maxWindSlider.addEventListener("input", () => {
      maxWindValue.textContent = maxWindSlider.value;
    });
    const btnMaxWindApply = document.getElementById("btnMaxWindApply");
    btnMaxWindApply.addEventListener("click", () => {
        sendAction("setMaxWind", Number(maxWindSlider.value));
    });        
}


window.addEventListener("load", initUI);
</script>
<!-- und ein wenig für die Grafik des Windsensors -->
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

</head>

<body>
<div class="tab">
  <button class="tablink" id="defaultTab" onclick="openTab(event, 'status')">Status</button>
  <button class="tablink" onclick="openTab(event, 'wifi')">Setup</button>
  <button class="tablink" onclick="openTab(event, 'help')">Help</button>  
</div>
<div id="status" class="tabcontent">

  <div class="status-flex">
    <!-- Linke Spalte: Temperatur / Feuchte / Reed -->
    <div class="status-left">
      <h2>Status</h2>
      <div class="kv"><label>Temperatur:</label> <span class="badge" id="valTemp">--.- °C</span></div>
      <div class="kv"><label>Feuchtigkeit:</label> <span class="badge" id="valHum">--.- %%</span></div>
      <hr>
      <div class="kv"><label>Magnet Markise:</label> <span class="badge" id="valM">unbekannt</span></div>
      <div class="kv"><label>Magnet Sensor 1:</label> <span class="badge" id="valS1">unbekannt</span></div>
      <div class="kv"><label>Magnet Sensor 2:</label> <span class="badge" id="valS2">unbekannt</span></div>
      <div class="kv"><label>Wind:</label> <span class="badge" id="valWind">--.- km/h</span></div>
    </div>
    <!-- puffer spalte layout -->
    <div class="status-puffer"></div>
    <!-- Rechte Spalte: Wind-Gauge  -->
    <div class="status-right">
	    <h2>Wind</h2>
	    <canvas id="windGauge"></canvas>
	    <div class="wind-value">
	      <span id="windValueKMH">--.-</span> km/h =
	      <span id="windValue">--.-</span> m/s 
	    </div>
	    <div class="wind-bft" id="windBft">Beaufort: --</div>
    </div>
  </div>
  <div class="history-wide">
    <canvas id="windChart"></canvas>
  </div>

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
  <div>
    <label for="gpioWind">Windsensor (GPIO)</label>
    <input type="number" id="gpioWind" min="0" max="39" value="%GPIO_WIND%" placeholder="z.B. 33">
  </div>
  <button class="btn" id="btnReedSave">GPIOs Speichern</button>
  <div>
    <label for="maxWindSlider">Maximaler Windbereich (m/s)</label>
    <input type="range" id="maxWindSlider" min="1" max="50" value="20" step="1">
    <span id="maxWindValue">20</span> m/s
  </div>
  <button class="btn" id="btnMaxWindApply">MaxWind anwenden</button>
</div>
<div id="help" class="tabcontent">
  <h2>Help</h2>
  <p>Folgende Funktionen existieren:</p>
  <ul>    
    <li>WLAN / MQTT Konfiguration</li>
    <li>Festlegen der GPIOs für die Reedkontakte</li>
    <li>Anzeige der Sensorwerte (Temperatur, Feuchte, Reedkontakte, Wind)</li>
    <li>Anzeige des Windwertes in einem analogen Windmesser (Gauge) und als Verlaufsgrafik</li>
    <li>Einstellen des maximalen Windbereiches für die Anzeige</li>
    <li>Anzeige von Log-Nachrichten vom ESP</li>
    <li>Neustart des ESP</li>
    <li>OTA-Update über <a href="/update">diesen Link</a></li>    
  </ul>

  <!-- Logs ans Ende des Help-Tabs -->
  <h3 style="display:inline;">Log-Nachrichten</h3> 
  <button class="btn" id="btnCheck"> Send Testmessage to ESP</button>
  <button class="btn" id="btnReboot"> Reboot ESP</button>
  <div id="logContainer" style="height:300px; overflow:auto; background:#f0f0f0; padding:10px; font-family:monospace;"></div>
</div>

</body>
</html>
)rawliteral";
