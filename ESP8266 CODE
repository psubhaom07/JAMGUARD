#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// ==================================================
// JAMGUARD - NODEMCU ESP8266
// Live RPM dashboard + session performance graph
// ==================================================

// Arduino Uno D10 TX -> voltage divider -> NodeMCU D5 RX
// NodeMCU D6 TX -> Arduino Uno D9 RX (optional)
SoftwareSerial arduinoSerial(D5, D6);

ESP8266WebServer server(80);

const char* ssid = "JamGuard";
const char* password = "jamguard123";

const int RPM_THRESHOLD = 2100;
const int MAX_DISPLAY_RPM = 3000;

int rpm = 0;
String motorStatus = "WAITING";
String ledStatus = "UNKNOWN";
String buzzerStatus = "UNKNOWN";
String warningText = "Waiting for Arduino data";

unsigned long lastDataTime = 0;

// ==================================================
// RECEIVE ARDUINO JSON
// Example:
// {"rpm":1850,"status":"RUNNING","led":"BLINKING",
//  "buzzer":"OFF","warning":"LOW RPM: BELOW 2100"}
// ==================================================

void receiveArduinoData() {
  if (!arduinoSerial.available()) return;

  String line = arduinoSerial.readStringUntil('\n');
  line.trim();

  if (line.length() == 0) return;

  StaticJsonDocument<384> doc;
  DeserializationError error = deserializeJson(doc, line);

  if (error) {
    Serial.print("JSON error: ");
    Serial.println(error.c_str());
    return;
  }

  rpm = doc["rpm"] | 0;
  motorStatus = doc["status"] | "UNKNOWN";

  // Actual states are preferred when supplied by Arduino.
  if (doc["led"].is<const char*>()) {
    ledStatus = doc["led"].as<String>();
  } else if (motorStatus == "FAULT") {
    ledStatus = "BLINKING";
  } else if (motorStatus == "RUNNING" && rpm < RPM_THRESHOLD) {
    ledStatus = "BLINKING";
  } else {
    ledStatus = "OFF";
  }

  if (doc["buzzer"].is<const char*>()) {
    buzzerStatus = doc["buzzer"].as<String>();
  } else {
    buzzerStatus = motorStatus == "FAULT" ? "BLINKING" : "OFF";
  }

  if (doc["warning"].is<const char*>()) {
    warningText = doc["warning"].as<String>();
  } else if (motorStatus == "FAULT") {
    warningText = "MOTOR FAULT";
  } else if (motorStatus == "RUNNING" && rpm < RPM_THRESHOLD) {
    warningText = "LOW RPM: BELOW 2100";
  } else if (motorStatus == "STOPPED") {
    warningText = "Motor stopped";
  } else {
    warningText = "No warning";
  }

  lastDataTime = millis();

  Serial.print("Received: ");
  Serial.println(line);
}

// ==================================================
// JSON API FOR BROWSER POLLING
// ==================================================

void handleData() {
  bool connected = lastDataTime != 0 &&
                   millis() - lastDataTime < 5000;

  // This is a speed-based indicator, NOT true efficiency.
  int indicator = 0;
  if (connected && rpm > 0) {
    indicator = constrain(
      (long)rpm * 100 / RPM_THRESHOLD, 0, 100
    );
  }

  StaticJsonDocument<512> doc;
  doc["connected"] = connected;
  doc["rpm"] = rpm;
  doc["status"] = motorStatus;
  doc["led"] = ledStatus;
  doc["buzzer"] = buzzerStatus;
  doc["warning"] = warningText;
  doc["indicator"] = indicator;
  doc["threshold"] = RPM_THRESHOLD;

  String output;
  serializeJson(doc, output);

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", output);
}

// ==================================================
// DASHBOARD PAGE
// ==================================================

void handleDashboard() {
  String page = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>JamGuard | Motor Monitor</title>
<style>
:root {
  --bg:#080e1b;
  --panel:#111b2e;
  --border:#25334c;
  --text:#eef4ff;
  --muted:#91a1bb;
  --cyan:#38bdf8;
  --green:#34d399;
  --yellow:#fbbf24;
  --red:#fb7185;
}
* {box-sizing:border-box}
body {
  margin:0;
  background:radial-gradient(ellipse at top,#172744 0%,var(--bg) 65%);
  color:var(--text);
  font-family:Arial,Helvetica,sans-serif;
  min-height:100vh;
}
.container {max-width:960px;margin:auto;padding:22px 16px 35px}
header {
  display:flex;align-items:center;justify-content:space-between;
  gap:12px;flex-wrap:wrap;margin-bottom:24px;
}
.brand {display:flex;align-items:center;gap:12px}
.logo {
  width:48px;height:48px;border-radius:15px;
  display:grid;place-items:center;font-size:25px;
  background:linear-gradient(135deg,#0ea5e9,#6366f1);
}
h1 {font-size:23px;margin:0;letter-spacing:1px}
.subtitle {color:var(--muted);font-size:12px;margin-top:5px}
.pill {
  padding:9px 12px;border:1px solid var(--border);
  border-radius:30px;font-size:12px;background:#101a2c;
}
.dot {
  display:inline-block;width:8px;height:8px;
  border-radius:50%;margin-right:7px;background:var(--yellow);
}
.section-title {
  font-size:12px;letter-spacing:2px;color:var(--muted);
  text-transform:uppercase;margin:24px 0 12px;
}
.grid {
  display:grid;grid-template-columns:repeat(2,minmax(0,1fr));
  gap:13px;
}
.card {
  background:linear-gradient(145deg,#141f33,#0f1829);
  border:1px solid var(--border);border-radius:19px;
  padding:19px;min-width:0;
  box-shadow:0 10px 30px #00000018;
}
.card-label {
  color:var(--muted);font-size:12px;
  text-transform:uppercase;letter-spacing:1px;
}
.value {font-size:18px;font-weight:700;margin-top:13px;overflow-wrap:anywhere}
.mini {font-size:12px;color:var(--muted);margin-top:8px;line-height:1.6}
.gauge-wrap {display:flex;justify-content:center;padding:22px 0 5px}
.gauge {
  width:190px;height:190px;border-radius:50%;
  display:grid;place-items:center;position:relative;
  background:conic-gradient(from 225deg,#38bdf8 0deg,#6366f1 var(--angle),#26344b var(--angle),#26344b 270deg,#111b2e 270deg);
}
.gauge:before {
  content:"";position:absolute;inset:10px;border-radius:50%;
  background:#101a2b;border:1px solid #2a3a54;
}
.gauge-inner {position:relative;text-align:center}
.gauge-number {font-size:37px;font-weight:800;letter-spacing:-1px}
.gauge-label {font-size:11px;letter-spacing:2px;color:var(--muted);margin-top:5px}
.bar {height:7px;border-radius:10px;background:#26344b;margin-top:16px;overflow:hidden}
.bar-fill {height:100%;border-radius:10px;background:linear-gradient(90deg,#38bdf8,#818cf8);width:0}
.alert {
  border-radius:17px;padding:16px;margin-top:14px;
  background:#17253a;border:1px solid #344762;
  display:flex;gap:12px;align-items:flex-start;
}
.alert-icon {font-size:23px}
.alert-title {font-weight:750;font-size:14px}
.alert-text {font-size:12px;color:#bdc9dc;margin-top:6px;line-height:1.5}
.chart-card {margin-top:14px}
.chart-head {display:flex;justify-content:space-between;gap:10px;flex-wrap:wrap}
.legend {font-size:12px;color:var(--muted);margin-top:12px}
.legend span {color:var(--cyan);font-weight:bold}
canvas {display:block;width:100%;height:260px;margin-top:15px}
.tag {
  display:inline-block;padding:6px 9px;border-radius:8px;
  background:#1b2b43;color:#b8c9e4;font-size:11px;
}
footer {text-align:center;color:#71819b;font-size:11px;margin-top:25px}
@media(min-width:650px) {
  .main-grid {grid-template-columns:1.15fr .85fr}
}
@media(max-width:480px) {
  .container {padding:15px 10px}
  .card {padding:14px}
  .gauge {width:160px;height:160px}
  .gauge-number {font-size:31px}
  canvas {height:230px}
}
</style>
</head>
<body>
<div class="container">
<header>
 <div class="brand">
  <div class="logo">&#9889;</div>
  <div>
   <h1>JamGuard</h1>
   <div class="subtitle">SMART MOTOR MONITORING SYSTEM</div>
  </div>
 </div>
 <div class="pill"><span id="dot" class="dot"></span><span id="connection">WAITING FOR ARDUINO</span></div>
</header>

<div class="section-title">Live performance</div>
<div class="grid main-grid">
 <div class="card">
  <div class="card-label">&#9889; Motor speed</div>
  <div class="gauge-wrap">
   <div id="gauge" class="gauge" style="--angle:0deg">
    <div class="gauge-inner">
     <div id="rpm" class="gauge-number">--</div>
     <div class="gauge-label">RPM</div>
    </div>
   </div>
  </div>
  <div class="mini" style="text-align:center">Low-RPM threshold: 2100 RPM</div>
  <div class="bar"><div id="barFill" class="bar-fill"></div></div>
 </div>
 <div class="card">
  <div class="card-label">Motor condition</div>
  <div id="status" class="value">WAITING</div>
  <div class="mini">Operating state reported by Arduino.</div>
  <div class="section-title">System alert</div>
  <div id="alertTitle" class="value">Waiting for data</div>
  <div id="alertDetail" class="mini">Start the Arduino motor monitoring sketch.</div>
 </div>
</div>

<div class="section-title">Protection indicators</div>
<div class="grid">
 <div class="card">
  <div class="card-label">&#128161; Warning LED</div>
  <div id="led" class="value">UNKNOWN</div>
  <div class="mini">Reported or inferred LED state.</div>
 </div>
 <div class="card">
  <div class="card-label">&#128276; Alarm buzzer</div>
  <div id="buzzer" class="value">UNKNOWN</div>
  <div class="mini">Reported or inferred buzzer state.</div>
 </div>
</div>

<div class="section-title">Performance history</div>
<div class="card chart-card">
 <div class="chart-head">
  <div>
   <div class="card-label">Speed-based performance indicator</div>
   <div id="indicator" class="value">--%</div>
  </div>
  <div class="tag">SESSION HISTORY</div>
 </div>
 <div class="mini">
  Indicator = RPM / 2100 x 100, capped at 100%.
  This is a speed reference, NOT true motor efficiency.
 </div>
 <div class="legend"><span>&#9632;</span> Performance indicator
   &nbsp; <span style="color:#fbbf24">&#9473;</span> 100% threshold
 </div>
 <canvas id="chart"></canvas>
 <div class="mini">
  Readings are retained in this page session. Reloading or closing the page resets the graph.
 </div>
</div>

<div class="alert">
 <div class="alert-icon">&#9888;</div>
 <div>
  <div class="alert-title">System message</div>
  <div id="warning" class="alert-text">Waiting for Arduino data.</div>
 </div>
</div>

<footer>
 JAMGUARD &bull; MOTOR SAFETY MONITORING<br><br>
 Live updates every 2 seconds
</footer>
</div>

<script>
const historyData = [];
const MAX_POINTS = 1800; // Up to 60 minutes at 2-second intervals
let lastPointTime = 0;
let latestConnected = false;

function setText(id, value) {
  document.getElementById(id).textContent = value;
}

function setColor(id, color) {
  document.getElementById(id).style.color = color;
}

function drawGraph() {
  const canvas = document.getElementById("chart");
  const rect = canvas.getBoundingClientRect();
  if (!rect.width) return;

  const dpr = window.devicePixelRatio || 1;
  canvas.width = Math.round(rect.width * dpr);
  canvas.height = Math.round(rect.height * dpr);

  const ctx = canvas.getContext("2d");
  ctx.scale(dpr, dpr);

  const W = rect.width;
  const H = rect.height;
  const left = 42, right = 12, top = 15, bottom = 32;
  const plotW = W - left - right;
  const plotH = H - top - bottom;

  ctx.clearRect(0, 0, W, H);
  ctx.font = "11px Arial";
  ctx.lineWidth = 1;

  // Horizontal grid and y-axis labels
  for (let v = 0; v <= 100; v += 25) {
    const y = top + plotH * (1 - v / 100);
    ctx.strokeStyle = "#293750";
    ctx.beginPath();
    ctx.moveTo(left, y);
    ctx.lineTo(W - right, y);
    ctx.stroke();

    ctx.fillStyle = "#91a1bb";
    ctx.textAlign = "right";
    ctx.fillText(v + "%", left - 8, y + 4);
  }

  if (historyData.length === 0) {
    ctx.fillStyle = "#91a1bb";
    ctx.textAlign = "center";
    ctx.fillText("Waiting for readings...", W / 2, H / 2);
    return;
  }

  // X-axis uses elapsed session time.
  const start = historyData[0].t;
  const end = historyData[historyData.length - 1].t;
  const span = Math.max(1000, end - start);

  ctx.fillStyle = "#91a1bb";
  ctx.textAlign = "center";
  for (let i = 0; i <= 4; i++) {
    const x = left + plotW * i / 4;
    const seconds = Math.round(span * i / 4 / 1000);
    ctx.fillText(seconds + "s", x, H - 9);
  }

  // 100% reference line
  const thresholdY = top;
  ctx.save();
  ctx.setLineDash([5, 4]);
  ctx.strokeStyle = "#fbbf24";
  ctx.beginPath();
  ctx.moveTo(left, thresholdY);
  ctx.lineTo(W - right, thresholdY);
  ctx.stroke();
  ctx.restore();

  if (historyData.length < 2) {
    const p = historyData[0];
    const x = left + plotW / 2;
    const y = top + plotH * (1 - p.v / 100);
    ctx.fillStyle = "#38bdf8";
    ctx.beginPath();
    ctx.arc(x, y, 4, 0, Math.PI * 2);
    ctx.fill();
    return;
  }

  // Performance line
  ctx.beginPath();
  historyData.forEach((p, i) => {
    const x = left + plotW * (p.t - start) / span;
    const y = top + plotH * (1 - p.v / 100);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.strokeStyle = "#38bdf8";
  ctx.lineWidth = 2.5;
  ctx.lineJoin = "round";
  ctx.stroke();

  // Highlight most recent point
  const last = historyData[historyData.length - 1];
  const lx = left + plotW * (last.t - start) / span;
  const ly = top + plotH * (1 - last.v / 100);
  ctx.fillStyle = "#38bdf8";
  ctx.beginPath();
  ctx.arc(lx, ly, 4, 0, Math.PI * 2);
  ctx.fill();
}

async function updateDashboard() {
  try {
    const response = await fetch("/data?t=" + Date.now(), {
      cache: "no-store"
    });
    if (!response.ok) throw new Error("HTTP " + response.status);

    const d = await response.json();
    latestConnected = d.connected;

    setText("connection", d.connected ? "ARDUINO CONNECTED" : "NO RECENT DATA");
    document.getElementById("dot").style.background =
      d.connected ? "#34d399" : "#fb7185";

    setText("rpm", d.connected ? d.rpm : "--");

    const gaugeRPM = Math.max(0, Math.min(3000, d.rpm));
    const angle = gaugeRPM / 3000 * 270;
    document.getElementById("gauge").style.setProperty("--angle", angle + "deg");
    document.getElementById("barFill").style.width =
      (gaugeRPM / 3000 * 100) + "%";

    setText("status", d.connected ? d.status : "NO DATA");
    setColor("status",
      !d.connected ? "#fb7185" :
      d.status === "FAULT" ? "#fb7185" :
      d.status === "STOPPED" ? "#94a3b8" : "#34d399");

    const lowRPM = d.connected &&
                   d.status === "RUNNING" &&
                   d.rpm < d.threshold;
    const fault = d.connected && d.status === "FAULT";
    const stopped = d.connected && d.status === "STOPPED";

    setText("alertTitle", !d.connected ? "CONNECTION LOST" :
      fault ? "FAULT DETECTED" :
      lowRPM ? "LOW RPM WARNING" :
      stopped ? "MOTOR STOPPED" : "NO ACTIVE WARNING");

    setColor("alertTitle", !d.connected || fault ? "#fb7185" :
      lowRPM ? "#fbbf24" : "#34d399");

    setText("alertDetail", !d.connected ?
      "No recent serial data received from Arduino." :
      fault ? "Arduino reports a motor fault." :
      lowRPM ? "RPM is below 2100. Check motor operation." :
      stopped ? "The motor is currently stopped." :
      "Monitoring motor data from Arduino.");

    setText("led", d.connected ? d.led : "UNKNOWN");
    setText("buzzer", d.connected ? d.buzzer : "UNKNOWN");
    setText("warning", d.connected ? d.warning : "Waiting for fresh Arduino data.");

    setColor("led", d.led === "OFF" ? "#94a3b8" : "#fbbf24");
    setColor("buzzer", d.buzzer === "OFF" ? "#94a3b8" : "#fb7185");

    const value = d.connected ?
      Math.min(100, Math.max(0, d.rpm / d.threshold * 100)) : null;

    setText("indicator", value === null ? "--%" :
      Math.round(value) + "%");

    // Record a point every polling interval, including connection gaps.
    const now = Date.now();
    if (now - lastPointTime >= 1800) {
      historyData.push({
        t: now,
        v: value === null ? 0 : value,
        connected: d.connected
      });
      lastPointTime = now;

      if (historyData.length > MAX_POINTS) historyData.shift();
      drawGraph();
    }
  } catch (e) {
    latestConnected = false;
    setText("connection", "ESP8266 CONNECTION ERROR");
    document.getElementById("dot").style.background = "#fb7185";
    setText("rpm", "--");
    setText("status", "NO DATA");
    setText("warning", "Cannot contact the ESP8266 dashboard API.");
  }
}

window.addEventListener("resize", drawGraph);
drawGraph();
updateDashboard();
setInterval(updateDashboard, 2000);
</script>
</body>
</html>
)HTML";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", page);
}

// ==================================================
// SETUP
// ==================================================

void setup() {
  Serial.begin(115200);

  arduinoSerial.begin(9600);
  arduinoSerial.setTimeout(50);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.on("/", handleDashboard);
  server.on("/data", handleData);
  server.begin();

  Serial.println();
  Serial.println("JamGuard ESP8266 started");
  Serial.print("Wi-Fi: ");
  Serial.println(ssid);
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.softAPIP());
}

// ==================================================
// MAIN LOOP
// ==================================================

void loop() {
  receiveArduinoData();
  server.handleClient();
}
