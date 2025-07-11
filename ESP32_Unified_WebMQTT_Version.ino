/*
 * ESP32 WROOM-32 Unified WiFi Gateway for STM32 MotionTracker
 * Features: Web Dashboard + MQTT + Real-time Charts
 * STM32 → ESP32 WROOM-32 → WiFi → Web Dashboard + MQTT Broker
 * UNIFIED VERSION - Both Web Control & MQTT Control
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// WiFi Ayarları
const char* ssid = "ARGE-MERKEZI";
const char* password = "D3ms4yArge-2025!";

// Access Point ayarları (yedek)
const char* ap_ssid = "ESP32_MotorControl";
const char* ap_password = "12345678";

// MQTT Ayarları
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32_MotionTracker_Unique_2025";

// MQTT Topics
const char* topic_data = "motortracker/data/sensor";
const char* topic_control = "motortracker/control/motor";
const char* topic_status = "motortracker/status/online";

// Web Server ve MQTT Client
WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

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

// Timing Variables
unsigned long lastMqttPublish = 0;
const unsigned long mqttPublishInterval = 500;  // 2Hz (500ms)
unsigned long lastMqttReconnect = 0;
const unsigned long mqttReconnectInterval = 5000;  // 5 seconds

void setup() {
  Serial.begin(115200);
  Serial.println("🚀 ESP32 MQTT MotionTracker Starting...");
  
  // UART2 başlat
  STM32Serial.begin(9600, SERIAL_8N1, STM32_SERIAL_RX, STM32_SERIAL_TX);
  
  // WiFi bağlantısı
  setupWiFi();
  
  // MQTT setup
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);
  
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
  
  Serial.println("✅ System Ready!");
  Serial.println("📱 Web Dashboard: http://" + 
    (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString()));
  Serial.println("📡 MQTT: " + String(mqtt_server) + ":" + String(mqtt_port));
  Serial.println("🎯 Control Topic: " + String(topic_control));
}

void loop() {
  // Web Server işlemleri
  server.handleClient();
  
  // MQTT bağlantı kontrolü ve işlemleri
  if (!mqttClient.connected()) {
    unsigned long now = millis();
    if (now - lastMqttReconnect > mqttReconnectInterval) {
      lastMqttReconnect = now;
      reconnectMQTT();
    }
  } else {
    mqttClient.loop();
    
    // Callback'i sürekli register et (FIX)
    mqttClient.setCallback(mqttCallback);
    
    // MQTT veri gönderimi (2Hz)
    unsigned long now = millis();
    if (now - lastMqttPublish > mqttPublishInterval) {
      lastMqttPublish = now;
      publishSensorData();
    }
  }
  
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

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20) {
    delay(500);
    Serial.print(".");
    wifi_attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.println("📍 IP Address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n⚠️ WiFi Failed - Starting AP Mode");
    WiFi.softAP(ap_ssid, ap_password);
    Serial.println("📍 AP IP: " + WiFi.softAPIP().toString());
  }
}

void reconnectMQTT() {
  if (mqttClient.connect(mqtt_client_id)) {
    Serial.println("✅ MQTT Connected!");
    
    // Subscribe to control topic
    bool subResult = mqttClient.subscribe(topic_control);
    Serial.println("📡 Subscribed to: " + String(topic_control) + " Result: " + String(subResult));
    
    // Publish online status
    mqttClient.publish(topic_status, "online", true);
    Serial.println("📤 Published online status");
  } else {
    Serial.print("❌ MQTT Failed, rc=");
    Serial.println(mqttClient.state());
  }
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  
  // Convert payload to string
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("📥 MQTT Received [" + String(topic) + "]: " + message);
  
  // Motor control commands
  if (String(topic) == topic_control) {
    int speed = message.toInt();
    if (speed >= 0 && speed <= 100) {
      setMotorSpeed(speed);
      Serial.println("🎯 MQTT Motor Command: " + String(speed) + "%");
    }
  }
}

void publishSensorData() {
  if (mqttClient.connected()) {
    DynamicJsonDocument doc(512);
    
    doc["device_id"] = lastData.device_id;
    doc["motor_speed"] = lastData.motor_speed;
    doc["gyro"]["x"] = round(lastData.gyro_x * 100) / 100.0;
    doc["gyro"]["y"] = round(lastData.gyro_y * 100) / 100.0;
    doc["gyro"]["z"] = round(lastData.gyro_z * 100) / 100.0;
    doc["magnitude"] = round(lastData.magnitude * 100) / 100.0;
    doc["timestamp"] = lastData.timestamp;
    doc["ip"] = WiFi.localIP().toString();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    if (mqttClient.publish(topic_data, jsonString.c_str())) {
      // Başarılı gönderim (sessiz)
    } else {
      Serial.println("❌ MQTT Publish Failed");
    }
  }
}

void setMotorSpeed(int speed) {
  // STM32'ye komut gönder
  String command = "MOTOR_SET:" + String(speed);
  STM32Serial.println(command);
  STM32Serial.flush();
  
  // Local data update
  lastData.motor_speed = speed;
  lastData.timestamp = millis();
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
      // ⚠️ STM32'den gelen motor hızını IGNORE et (ESP32 kontrolü korunur)
      // lastData.motor_speed = motorStr.toInt();  // BU SATIR KAPATILDI
      
      // Sadece gyro verilerini al
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
    Serial.println("✅ STM32 ACK: " + data);
  }
}

// Motor kontrolü (Web API)
void handleMotorControl() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));
    
    int speed = doc["speed"];
    setMotorSpeed(speed);
    
    // Başarılı yanıt gönder
    server.send(200, "text/plain", "Motor: " + String(speed) + "%");
    Serial.println("🌐 Web Motor Command: " + String(speed) + "%");
  } else {
    server.send(400, "text/plain", "Invalid request");
  }
}

// Ana sayfa HTML
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<title>STM32 MotionTracker - Unified</title>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
  html += "<style>";
  html += "body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:20px;margin:0;}";
  html += ".container{max-width:1000px;margin:0 auto;background:#16213e;padding:30px;border-radius:12px;box-shadow:0 10px 30px rgba(0,0,0,0.3);border:1px solid #2d3748;}";
  html += "h1{text-align:center;color:#ffffff;margin-bottom:8px;font-weight:600;font-size:2.2em;}";
  html += ".subtitle{text-align:center;color:#a0a0a0;font-size:1em;margin-bottom:30px;font-weight:400;}";
  html += ".status-bar{display:flex;justify-content:space-between;background:#0f3460;padding:12px 20px;border-radius:8px;margin-bottom:25px;font-size:0.9em;border:1px solid #2d3748;}";
  html += ".status-item{display:flex;align-items:center;gap:8px;color:#b0c4de;}";
  html += ".data-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:16px;margin:25px 0;}";
  html += ".data-item{background:#0f3460;padding:20px;border-radius:8px;text-align:center;transition:all 0.2s;border:1px solid #2d3748;}";
  html += ".data-item:hover{transform:translateY(-2px);box-shadow:0 5px 15px rgba(0,0,0,0.2);}";
  html += ".data-item h4{margin:0 0 8px 0;color:#b0c4de;font-size:0.9em;font-weight:500;}";
  html += ".value{font-size:1.8em;color:#4fc3f7;font-weight:600;margin:0;}";
  html += ".progress-bar{width:100%;height:24px;background:#2d3748;border-radius:12px;overflow:hidden;margin:20px 0;border:1px solid #3a4a5c;}";
  html += ".progress-fill{height:100%;background:linear-gradient(90deg,#2196f3,#4fc3f7);transition:width 0.5s;}";
  html += ".control-buttons{display:grid;grid-template-columns:repeat(auto-fit,minmax(110px,1fr));gap:12px;margin:25px 0;}";
  html += ".btn{padding:12px 16px;border:none;border-radius:6px;font-weight:600;cursor:pointer;color:white;transition:all 0.2s;font-size:0.9em;}";
  html += ".btn:hover{transform:translateY(-1px);box-shadow:0 4px 12px rgba(0,0,0,0.2);}";
  html += ".btn-stop{background:#d32f2f;} .btn-stop:hover{background:#b71c1c;}";
  html += ".btn-slow{background:#f57c00;} .btn-slow:hover{background:#e65100;}";
  html += ".btn-medium{background:#1976d2;} .btn-medium:hover{background:#0d47a1;}";
  html += ".btn-fast{background:#388e3c;} .btn-fast:hover{background:#1b5e20;}";
  html += ".btn-full{background:#7b1fa2;} .btn-full:hover{background:#4a148c;}";
  html += ".chart-container{background:#0f3460;padding:25px;border-radius:8px;margin:25px 0;height:350px;border:1px solid #2d3748;}";
  html += ".chart-title{text-align:center;color:#ffffff;font-size:1.1em;margin-bottom:20px;font-weight:500;}";
  html += ".footer{text-align:center;margin-top:25px;font-size:0.85em;color:#a0a0a0;}";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>STM32 MotionTracker</h1>";
  html += "<div class='subtitle'>Industrial IoT Motion Control System</div>";
  
  // Status Bar
  html += "<div class='status-bar'>";
  html += "<div class='status-item'>WiFi: " + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "</div>";
  html += "<div class='status-item'>MQTT: " + String(mqttClient.connected() ? "Connected" : "Disconnected") + "</div>";
  html += "<div class='status-item'>Uptime: " + String(millis()/1000) + "s</div>";
  html += "</div>";
  
  html += "<div class='data-grid'>";
  html += "<div class='data-item'><h4>Motor Speed</h4><div class='value'>" + String(lastData.motor_speed) + "%</div></div>";
  html += "<div class='data-item'><h4>Gyro X-Axis</h4><div class='value'>" + String(lastData.gyro_x, 1) + " dps</div></div>";
  html += "<div class='data-item'><h4>Gyro Y-Axis</h4><div class='value'>" + String(lastData.gyro_y, 1) + " dps</div></div>";
  html += "<div class='data-item'><h4>Gyro Z-Axis</h4><div class='value'>" + String(lastData.gyro_z, 1) + " dps</div></div>";
  html += "<div class='data-item'><h4>Total Magnitude</h4><div class='value'>" + String(lastData.magnitude, 1) + " dps</div></div>";
  html += "</div>";
  
  html += "<div class='progress-bar'>";
  html += "<div id='progressBar' class='progress-fill' style='width:" + String(lastData.motor_speed) + "%'></div>";
  html += "</div>";
  html += "<div id='progressText' style='text-align:center;font-size:1.5em;color:#FFD700;margin-bottom:20px;'>" + String(lastData.motor_speed) + "%</div>";
  
  html += "<div class='control-buttons'>";
  html += "<button onclick='setMotor(0)' class='btn btn-stop'>STOP</button>";
  html += "<button onclick='setMotor(25)' class='btn btn-slow'>25%</button>";
  html += "<button onclick='setMotor(50)' class='btn btn-medium'>50%</button>";
  html += "<button onclick='setMotor(75)' class='btn btn-fast'>75%</button>";
  html += "<button onclick='setMotor(100)' class='btn btn-full'>100%</button>";
  html += "</div>";
  
  html += "<div class='chart-container'>";
  html += "<div class='chart-title'>Real-Time Motor Speed Analytics</div>";
  html += "<canvas id='motorChart'></canvas>";
  html += "</div>";
  
  html += "<div class='footer'>";
  html += "<p>Device: " + lastData.device_id + " | MQTT Topics: motortracker/*</p>";
  html += "<p>Industrial IoT Gateway - STM32F3 Discovery + ESP32 WROOM-32</p>";
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
  html += "label: 'Motor Speed (%)',";
  html += "data: chartData,";
  html += "borderColor: '#4fc3f7',";
  html += "backgroundColor: 'rgba(79, 195, 247, 0.1)',";
  html += "borderWidth: 2,";
  html += "fill: true,";
  html += "tension: 0.4";
  html += "}]";
  html += "},";
  html += "options: {";
  html += "responsive: true,";
  html += "maintainAspectRatio: false,";
  html += "plugins: {legend: {labels: {color: '#e0e0e0'}}},";
  html += "scales: {";
  html += "y: {beginAtZero: true, max: 100, ticks: {color: '#b0c4de'}, grid: {color: 'rgba(176,196,222,0.1)'}},";
  html += "x: {ticks: {color: '#b0c4de'}, grid: {color: 'rgba(176,196,222,0.1)'}}";
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
  json += "\"timestamp\":" + String(lastData.timestamp) + ",";
  json += "\"mqtt_connected\":" + String(mqttClient.connected() ? "true" : "false");
  json += "}";
  
  server.send(200, "application/json", json);
} 