/*
 * ESP32 WROOM-32 WiFi Gateway for STM32 MotionTracker
 * STM32 → ESP32 WROOM-32 → WiFi → Web Dashboard
 * FIXED VERSION - MOTOR Format Support Added
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// WiFi Ayarları
const char* ssid = "ARGE-MERKEZI";
const char* password = "D3ms4yArge-2025!";

// Access Point ayarları (yedek)
const char* ap_ssid = "ESP32_MotorControl";
const char* ap_password = "12345678";

// Web Server
WebServer server(80);

// STM32 UART Ayarları
#define STM32_SERIAL_RX 16  // GPIO16 (RX2) ← STM32 TX
#define STM32_SERIAL_TX 17  // GPIO17 (TX2) → STM32 RX
HardwareSerial STM32Serial(2);

// Veri Depolama
struct MotorData {
  String device_id;
  int motor_speed;
  float gyro_x;
  float gyro_y; 
  float gyro_z;
  float magnitude;
  unsigned long timestamp;
};

MotorData lastData;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 MotorTracker Gateway Starting...");
  
  // UART2 başlat
  STM32Serial.begin(9600, SERIAL_8N1, STM32_SERIAL_RX, STM32_SERIAL_TX);
  
  // WiFi bağlantısı
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20) {
    delay(500);
    Serial.print(".");
    wifi_attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWeb Dashboard: http://" + WiFi.localIP().toString());
  } else {
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("\nWeb Dashboard: http://" + WiFi.softAPIP().toString());
  }
  
  // Web Server Route'ları
  server.on("/", handleRoot);
  server.on("/motor", HTTP_POST, handleMotorControl);
  server.on("/data", HTTP_GET, handleDataAPI);
  
  server.begin();
  
  // İlk değerler
  lastData.device_id = "STM32_F3";
  lastData.motor_speed = 0;
  lastData.gyro_x = 0.0;
  lastData.gyro_y = 0.0;
  lastData.gyro_z = 0.0;
  lastData.magnitude = 0.0;
  lastData.timestamp = millis();
  
  Serial.println("System Ready! Open web dashboard to see motor graph.");
}

void loop() {
  server.handleClient();
  
  // STM32'den veri oku
  if (STM32Serial.available()) {
    String stm32_data = STM32Serial.readStringUntil('\n');
    stm32_data.trim();
    
    if (stm32_data.length() > 0) {
      processSTM32Data(stm32_data);
    }
  }
  
  delay(50);
}

// STM32 veri işleme - MOTOR format desteği
void processSTM32Data(String data) {
  // MOTOR:25,X:1.2,Y:-0.8,Z:2.5 formatını parse et
  if (data.startsWith("MOTOR:")) {
    int motorIndex = data.indexOf(":");
    int xIndex = data.indexOf(",X:");
    int yIndex = data.indexOf(",Y:");
    int zIndex = data.indexOf(",Z:");
    
    if (motorIndex > 0 && xIndex > motorIndex) {
      // Motor hızını parse et
      String motorStr = data.substring(motorIndex + 1, xIndex);
      lastData.motor_speed = motorStr.toInt();
      
      // X değerini parse et
      if (yIndex > xIndex) {
        String xStr = data.substring(xIndex + 3, yIndex);
        lastData.gyro_x = xStr.toFloat();
      }
      
      // Y değerini parse et
      if (zIndex > yIndex) {
        String yStr = data.substring(yIndex + 3, zIndex);
        lastData.gyro_y = yStr.toFloat();
      }
      
      // Z değerini parse et
      if (zIndex > 0) {
        String zStr = data.substring(zIndex + 3);
        zStr.trim();
        lastData.gyro_z = zStr.toFloat();
      }
      
      // Magnitude hesapla
      lastData.magnitude = sqrt(lastData.gyro_x * lastData.gyro_x + 
                               lastData.gyro_y * lastData.gyro_y + 
                               lastData.gyro_z * lastData.gyro_z);
      
      lastData.device_id = "STM32F3_LIVE";
      lastData.timestamp = millis();
    }
  }
  
  // MOTOR_SET ACK mesajlarını işle
  else if (data.startsWith("ACK:MOTOR_SET:")) {
    // ACK alındı - sessizce işle
  }
}

// Motor kontrolü
void handleMotorControl() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    
    int speed = doc["speed"];
    
    // STM32'ye komut gönder
    String command = "MOTOR_SET:" + String(speed);
    STM32Serial.println(command);
    STM32Serial.flush();
    
    // Başarılı yanıt gönder
    server.send(200, "text/plain", "Motor: " + String(speed) + "%");
  } else {
    server.send(400, "text/plain", "Invalid request");
  }
}

// Ana sayfa HTML
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<title>STM32 MotionTracker</title>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
  html += "<style>";
  html += "body{font-family:Arial;background:linear-gradient(135deg, #0c0c0c 0%, #1a1a2e 50%, #16213e 100%);color:white;padding:20px;min-height:100vh;}";
  html += ".container{max-width:800px;margin:0 auto;background:rgba(255,255,255,0.1);padding:30px;border-radius:20px;}";
  html += "h1{text-align:center;color:#FFD700;}";
  html += ".data-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px;margin:20px 0;}";
  html += ".data-item{background:rgba(255,255,255,0.2);padding:15px;border-radius:10px;text-align:center;}";
  html += ".value{font-size:1.5em;color:#FFD700;font-weight:bold;}";
  html += ".progress-bar{width:100%;height:30px;background:rgba(255,255,255,0.3);border-radius:15px;overflow:hidden;margin:10px 0;}";
  html += ".progress-fill{height:100%;background:linear-gradient(90deg,#32CD32,#FFD700,#FF6347);transition:width 0.5s;}";
  html += ".control-buttons{display:grid;grid-template-columns:repeat(auto-fit,minmax(120px,1fr));gap:10px;margin:20px 0;}";
  html += ".btn{padding:12px;border:none;border-radius:25px;font-weight:bold;cursor:pointer;color:white;transition:transform 0.2s;}";
  html += ".btn:hover{transform:scale(1.05);}";
  html += ".btn-stop{background:#FF4444;} .btn-slow{background:#FFAA00;} .btn-medium{background:#00AAFF;} .btn-fast{background:#00FF00;} .btn-full{background:#FF00FF;}";
  html += ".chart-container{background:rgba(255,255,255,0.1);padding:20px;border-radius:15px;margin:20px 0;height:300px;}";
  html += ".chart-title{text-align:center;color:#FFD700;font-size:1.2em;margin-bottom:15px;}";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>🚀 STM32 MotionTracker Dashboard</h1>";
  
  html += "<div class='data-grid'>";
  html += "<div class='data-item'><div>⚡ Motor Hızı</div><div class='value'>" + String(lastData.motor_speed) + "%</div></div>";
  html += "<div class='data-item'><div>↔️ Gyro X</div><div class='value'>" + String(lastData.gyro_x, 1) + " dps</div></div>";
  html += "<div class='data-item'><div>↕️ Gyro Y</div><div class='value'>" + String(lastData.gyro_y, 1) + " dps</div></div>";
  html += "<div class='data-item'><div>🔄 Gyro Z</div><div class='value'>" + String(lastData.gyro_z, 1) + " dps</div></div>";
  html += "<div class='data-item'><div>📏 Magnitude</div><div class='value'>" + String(lastData.magnitude, 1) + " dps</div></div>";
  html += "</div>";
  
  html += "<div class='progress-bar'>";
  html += "<div id='progressBar' class='progress-fill' style='width:" + String(lastData.motor_speed) + "%'></div>";
  html += "</div>";
  html += "<div id='progressText' style='text-align:center;font-size:1.5em;color:#FFD700;'>" + String(lastData.motor_speed) + "%</div>";
  
  html += "<div class='control-buttons'>";
  html += "<button onclick='setMotor(0)' class='btn btn-stop'>⏹️ STOP</button>";
  html += "<button onclick='setMotor(25)' class='btn btn-slow'>🐌 25%</button>";
  html += "<button onclick='setMotor(50)' class='btn btn-medium'>🚶 50%</button>";
  html += "<button onclick='setMotor(75)' class='btn btn-fast'>🏃 75%</button>";
  html += "<button onclick='setMotor(100)' class='btn btn-full'>🚀 100%</button>";
  html += "</div>";
  
  html += "<div class='chart-container'>";
  html += "<div class='chart-title'>📊 Motor Hızı Grafiği (Real-Time)</div>";
  html += "<canvas id='motorChart'></canvas>";
  html += "</div>";
  
  html += "<div style='text-align:center;margin-top:20px;'>";
  html += "<p>🔧 Device: " + lastData.device_id + "</p>";
  html += "<p>📡 IP: " + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "</p>";
  html += "<p>🕒 Uptime: " + String(millis()/1000) + "s</p>";
  html += "</div>";
  
  html += "</div>";
  
  html += "<script>";
  
  // Chart.js Motor Speed Graph
  html += "let motorChart;";
  html += "let chartData = [];";
  html += "let chartLabels = [];";
  html += "let maxDataPoints = 30;";
  
  html += "function initChart(){";
  html += "const ctx = document.getElementById('motorChart').getContext('2d');";
  html += "motorChart = new Chart(ctx, {";
  html += "type: 'line',";
  html += "data: {";
  html += "labels: chartLabels,";
  html += "datasets: [{";
  html += "label: 'Motor Hızı (%)',";
  html += "data: chartData,";
  html += "borderColor: '#FFD700',";
  html += "backgroundColor: 'rgba(255, 215, 0, 0.2)',";
  html += "borderWidth: 3,";
  html += "fill: true,";
  html += "tension: 0.4";
  html += "}]";
  html += "},";
  html += "options: {";
  html += "responsive: true,";
  html += "maintainAspectRatio: false,";
  html += "plugins: {legend: {labels: {color: '#FFD700'}}},";
  html += "scales: {";
  html += "y: {beginAtZero: true, max: 100, ticks: {color: '#FFD700'}, grid: {color: 'rgba(255,255,255,0.1)'}},";
  html += "x: {ticks: {color: '#FFD700'}, grid: {color: 'rgba(255,255,255,0.1)'}}";
  html += "}";
  html += "}";
  html += "});";
  html += "}";
  
  html += "function updateChart(speed){";
  html += "let now = new Date().toLocaleTimeString();";
  html += "chartData.push(speed);";
  html += "chartLabels.push(now);";
  html += "if(chartData.length > maxDataPoints){";
  html += "chartData.shift();";
  html += "chartLabels.shift();";
  html += "}";
  html += "motorChart.update('none');";
  html += "}";
  
  html += "function setMotor(speed){";
  html += "document.getElementById('progressBar').style.width=speed+'%';";
  html += "document.getElementById('progressText').innerText=speed+'%';";
  html += "document.querySelectorAll('.value')[0].innerText=speed+'%';";
  html += "updateChart(speed);";
  html += "fetch('/motor',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({speed:speed})});";
  html += "}";
  
  // Auto-refresh data
  html += "function refreshData(){";
  html += "fetch('/data').then(r=>r.json()).then(data=>{";
  html += "if(data.motor_speed !== undefined){";
  html += "document.getElementById('progressBar').style.width=data.motor_speed+'%';";
  html += "document.getElementById('progressText').innerText=data.motor_speed+'%';";
  html += "document.querySelectorAll('.value')[0].innerText=data.motor_speed+'%';";
  html += "document.querySelectorAll('.value')[1].innerText=data.gyro_x.toFixed(1)+' dps';";
  html += "document.querySelectorAll('.value')[2].innerText=data.gyro_y.toFixed(1)+' dps';";
  html += "document.querySelectorAll('.value')[3].innerText=data.gyro_z.toFixed(1)+' dps';";
  html += "document.querySelectorAll('.value')[4].innerText=data.magnitude.toFixed(1)+' dps';";
  html += "updateChart(data.motor_speed);";
  html += "}";
  html += "}).catch(e=>console.log('Data fetch error:',e));";
  html += "}";
  
  html += "window.onload = function(){";
  html += "initChart();";
  html += "setInterval(refreshData, 1000);";
  html += "};";
  
  html += "</script>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// JSON veri API'si - grafik için real-time data
void handleDataAPI() {
  String json = "{";
  json += "\"motor_speed\":" + String(lastData.motor_speed) + ",";
  json += "\"gyro_x\":" + String(lastData.gyro_x, 2) + ",";
  json += "\"gyro_y\":" + String(lastData.gyro_y, 2) + ",";
  json += "\"gyro_z\":" + String(lastData.gyro_z, 2) + ",";
  json += "\"magnitude\":" + String(lastData.magnitude, 2) + ",";
  json += "\"device_id\":\"" + lastData.device_id + "\",";
  json += "\"timestamp\":" + String(lastData.timestamp);
  json += "}";
  
  server.send(200, "application/json", json);
} 