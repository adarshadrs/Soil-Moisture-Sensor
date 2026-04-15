#include <WiFi.h>
#include <WebServer.h>

// ==========================================
// ⚙️ WiFi Settings
// ==========================================
const char* ssid = "wifi name";             // ⚠️ Replace with your WiFi Network Name
const char* password = "password of wifi";       // ⚠️ Replace with your WiFi Password

WebServer server(80); // Creates an HTTP web server on port 80

// ==========================================
// 🔌 Sensor & Calibration Settings
// ==========================================
#define SOIL_MOISTURE_PIN 34 

const int DRY_VAL = 4095; // Typical completely dry value 
const int WET_VAL = 1500; // Typical completely wet value 

// Variables to store current readings
int currentRaw = 0;
int currentPct = 0;

// ==========================================
// 🎨 HTML, CSS & JavaScript (The Web UI)
// ==========================================
// We use PROGMEM to save RAM, storing the giant HTML string in Flash memory
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-width=1.0">
<title>Live Plant Dashboard</title>
<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;500;700&display=swap" rel="stylesheet">
<style>
  :root {
    --bg-gradient: linear-gradient(135deg, #09121b, #152433, #0a1f1c);
    --card-bg: rgba(255, 255, 255, 0.05);
    --card-border: rgba(255, 255, 255, 0.1);
  }
  body, html {
    margin: 0; padding: 0;
    height: 100%;
    font-family: 'Outfit', sans-serif;
    color: #ffffff;
    background: var(--bg-gradient);
    background-size: 400% 400%;
    animation: gradientBG 15s ease infinite;
    display: flex; justify-content: center; align-items: center;
  }
  @keyframes gradientBG {
    0% { background-position: 0% 50%; }
    50% { background-position: 100% 50%; }
    100% { background-position: 0% 50%; }
  }
  .dashboard {
    background: var(--card-bg);
    backdrop-filter: blur(20px);
    -webkit-backdrop-filter: blur(20px);
    border: 1px solid var(--card-border);
    border-radius: 28px;
    padding: 40px;
    text-align: center;
    box-shadow: 0 30px 60px rgba(0,0,0,0.6);
    width: 320px;
    display: flex; flex-direction: column; align-items: center;
  }
  h1 {
    font-size: 24px; font-weight: 500; margin-bottom: 30px; letter-spacing: 1px;
    text-transform: uppercase; opacity: 0.9;
  }
  
  .circle {
    position: relative;
    width: 220px; height: 220px;
    border-radius: 50%;
    background: rgba(0, 0, 0, 0.4);
    display: flex; justify-content: center; align-items: center;
    box-shadow: inset 0 0 25px rgba(0,0,0,0.6), 0 0 30px rgba(0, 210, 255, 0.15);
    overflow: hidden;
    border: 4px solid rgba(255,255,255,0.05);
  }
  
  .wave {
    position: absolute;
    width: 450px; height: 450px;
    background: rgba(0, 210, 255, 0.4);
    border-radius: 40%;
    bottom: -450px; 
    animation: waveAnim 6s linear infinite;
    transition: bottom 1.5s cubic-bezier(0.4, 0, 0.2, 1), background-color 1.5s ease;
  }
  .wave:nth-child(2) {
    background: rgba(0, 210, 255, 0.7);
    animation: waveAnim 8s linear infinite;
  }
  @keyframes waveAnim {
    0% { transform: rotate(0deg); }
    100% { transform: rotate(360deg); }
  }
  
  .moisture-value {
    position: relative; z-index: 10;
    font-size: 64px; font-weight: 700;
    text-shadow: 0 4px 15px rgba(0,0,0,0.6);
  }
  .moisture-value span { font-size: 26px; opacity: 0.8; }
  
  .status {
    margin-top: 25px;
    font-weight: 500; font-size: 18px;
    padding: 10px 20px;
    border-radius: 50px;
    background: rgba(255,255,255,0.08);
    transition: color 0.4s ease;
    letter-spacing: 0.5px;
  }
  .raw-value {
    margin-top: 20px; font-size: 13px; color: #8b9bb4; letter-spacing: 0.5px;
  }
  
  #error-banner {
    display: none;
    margin-top: 20px;
    color: #ff6b6b;
    font-size: 14px;
    background: rgba(255, 107, 107, 0.1);
    padding: 8px 15px;
    border-radius: 12px;
  }
</style>
</head>
<body>

<div class="dashboard">
  <h1>Local WiFi Dashboard</h1>
  <div class="circle">
    <div class="wave" id="wave1"></div>
    <div class="wave" id="wave2"></div>
    <div class="moisture-value"><span id="pct">--</span><span>%</span></div>
  </div>
  <div class="status" id="status-text">Connecting...</div>
  <div class="raw-value">Raw ADC: <span id="raw">----</span></div>
  <div id="error-banner">Connecting to ESP32...</div>
</div>

<script>
  let isError = false;

  function updateData() {
    // Poll the ESP32 for new data
    fetch('/data', { cache: "no-store" })
      .then(response => {
        if (!response.ok) throw new Error("Network error");
        return response.json();
      })
      .then(data => {
        if (isError) {
          isError = false;
          document.getElementById('error-banner').style.display = "none";
        }

        const pct = data.moisture;
        document.getElementById('pct').innerText = pct;
        document.getElementById('raw').innerText = data.raw;
        
        // Dynamic fluid wave calculation
        const waveBottom = -450 + (pct * 2.2);
        const wave1 = document.getElementById('wave1');
        const wave2 = document.getElementById('wave2');
        
        wave1.style.bottom = waveBottom + 'px';
        wave2.style.bottom = (waveBottom - 15) + 'px';
        
        const statusEl = document.getElementById('status-text');
        
        // Dynamic theme updates based on moisture levels
        if(pct < 30) {
          statusEl.innerText = "Dry (Needs Water)";
          statusEl.style.color = "#ff6b6b"; 
          wave1.style.background = "rgba(255, 107, 107, 0.4)";
          wave2.style.background = "rgba(255, 107, 107, 0.7)";
        } else if (pct > 75) {
          statusEl.innerText = "Over-watered";
          statusEl.style.color = "#4dabf7"; 
          wave1.style.background = "rgba(77, 171, 247, 0.4)";
          wave2.style.background = "rgba(77, 171, 247, 0.7)";
        } else {
          statusEl.innerText = "Optimal Moisture";
          statusEl.style.color = "#51cf66"; 
          wave1.style.background = "rgba(81, 207, 102, 0.4)";
          wave2.style.background = "rgba(81, 207, 102, 0.7)";
        }
      })
      .catch(error => {
        isError = true;
        document.getElementById('error-banner').style.display = "block";
        document.getElementById('error-banner').innerText = "Disconnected from ESP32. Retrying...";
      });
  }
  
  // Update UI every 1.5 seconds automatically
  setInterval(updateData, 1500);
  updateData(); 
</script>
</body>
</html>
)rawliteral";

// ==========================================
// 📡 Web Server Endpoints
// ==========================================

// When someone types the IP in their browser, send them the HTML Website
void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "text/html", index_html);
}

// When the Javascript calls /data, send the JSON sensor data
void handleData() {
  String json = "{\"moisture\":";
  json += String(currentPct);
  json += ",\"raw\":";
  json += String(currentRaw);
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", json);
}

// ==========================================
// 🚀 Main Setup & Loop
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n--- ESP32 Local Web Host Starting ---");

  pinMode(SOIL_MOISTURE_PIN, INPUT);

  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  // Give WiFi up to 10 seconds to connect
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("🌐 Open Chrome/Safari and enter EXACTLY this IP address: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi Failed to Connect! Please check your SSID/Password.");
  }

  // Bind server endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.begin();
  
  Serial.println("📡 HTTP Web Server started.");
}

void loop() {
  // Check if a client is trying to connect or fetching data
  server.handleClient();
  
  // We periodically update the sensor data exactly twice a second
  // This prevents loop blocking and keeps the WebServer extremely fast!
  static unsigned long lastReading = 0;
  if(millis() - lastReading > 500) { 
    lastReading = millis();
    
    currentRaw = analogRead(SOIL_MOISTURE_PIN);
    currentPct = map(currentRaw, DRY_VAL, WET_VAL, 0, 100);
    currentPct = constrain(currentPct, 0, 100);
  }
}