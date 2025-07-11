/*
 * ESP32 MQTT MotionTracker Gateway
 * STM32 → ESP32 → MQTT Broker → Multiple Clients
 * Features: Real-time data streaming, Remote motor control, Cloud ready
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>

// WiFi Ayarları
const char* ssid = "ARGE-MERKEZI";
const char* password = "D3ms4yArge-2025!";

// MQTT Broker Ayarları
const char* mqtt_server = "broker.hivemq.com";  // Public test broker
// const char* mqtt_server = "192.168.1.100";   // Local broker
const int mqtt_port = 1883;
const char* mqtt_user = "";                     // Boş = anonymous
const char* mqtt_password = "";
const char* device_id = "STM32_MotorTracker_001";

// MQTT Topics
const char* topic_motor_speed = "motortracker/data/motor_speed";
const char* topic_gyro_x = "motortracker/data/gyro/x";
const char* topic_gyro_y = "motortracker/data/gyro/y";
const char* topic_gyro_z = "motortracker/data/gyro/z";
const char* topic_magnitude = "motortracker/data/magnitude";
const char* topic_motor_control = "motortracker/control/motor_set";
const char* topic_system_status = "motortracker/status/system";
const char* topic_all_data = "motortracker/data/all";

// Clients
WiFiClient espClient;
PubSubClient mqttClient(espClient);
WebServer webServer(80);

// STM32 UART
#define STM32_SERIAL_RX 16
#define STM32_SERIAL_TX 17
HardwareSerial STM32Serial(2);

// Data Storage
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
unsigned long lastMqttPublish = 0;
const unsigned long mqttPublishInterval = 500; // 500ms = 2Hz

void setup() {
  Serial.begin(115200);
  Serial.println("🚀 ESP32 MQTT MotionTracker Starting...");
  
  // Initialize data
  lastData.device_id = device_id;
  lastData.motor_speed = 0;
  lastData.gyro_x = 0.0;
  lastData.gyro_y = 0.0;
  lastData.gyro_z = 0.0;
  lastData.magnitude = 0.0;
  
  // STM32 UART
  STM32Serial.begin(9600, SERIAL_8N1, STM32_SERIAL_RX, STM32_SERIAL_TX);
  Serial.println("✅ STM32 UART initialized");
  
  // WiFi Setup
  setupWiFi();
  
  // MQTT Setup
  setupMQTT();
  
  // Web Server (for local dashboard)
  setupWebServer();
  
  Serial.println("🎯 System Ready! Publishing to MQTT...");
}

void loop() {
  // Handle connections
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
  webServer.handleClient();
  
  // Read STM32 data
  readSTM32Data();
  
  // Publish to MQTT (2Hz rate)
  if (millis() - lastMqttPublish > mqttPublishInterval) {
    publishMQTTData();
    lastMqttPublish = millis();
  }
  
  delay(50);
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("📡 Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi Connected!");
    Serial.println("🌐 IP Address: " + WiFi.localIP().toString());
    Serial.println("🔗 Web Dashboard: http://" + WiFi.localIP().toString());
  } else {
    Serial.println();
    Serial.println("❌ WiFi Failed! Starting AP Mode...");
    WiFi.softAP("ESP32_MotorTracker", "12345678");
    Serial.println("📡 AP Mode: " + WiFi.softAPIP().toString());
  }
}

void setupMQTT() {
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(1024); // Larger buffer for JSON
  
  Serial.println("🔗 MQTT Broker: " + String(mqtt_server) + ":" + String(mqtt_port));
  
  // Connect to MQTT
  reconnectMQTT();
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("🔄 Connecting to MQTT...");
    
    // Create unique client ID
    String clientId = "ESP32_" + String(device_id) + "_" + String(random(0xffff), HEX);
    
    if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println(" ✅ MQTT Connected!");
      
      // Subscribe to control topics
      mqttClient.subscribe(topic_motor_control);
      mqttClient.subscribe("motortracker/control/+");
      
      // Publish connection status
      mqttClient.publish(topic_system_status, "{\"device\":\"" + String(device_id) + "\",\"status\":\"ONLINE\",\"ip\":\"" + WiFi.localIP().toString() + "\"}");
      
      Serial.println("📢 Subscribed to: " + String(topic_motor_control));
      
    } else {
      Serial.print(" ❌ Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("📥 MQTT Received: " + String(topic) + " = " + message);
  
  // Motor control
  if (String(topic) == topic_motor_control) {
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, message) == DeserializationError::Ok) {
      int speed = doc["speed"];
      
      // Send to STM32
      String command = "MOTOR_SET:" + String(speed);
      STM32Serial.println(command);
      STM32Serial.flush();
      
      Serial.println("🎮 Motor Command Sent to STM32: " + command);
      
      // Update local data
      lastData.motor_speed = speed;
      
      // Publish ACK
      mqttClient.publish("motortracker/status/motor_ack", ("{\"speed\":" + String(speed) + ",\"status\":\"OK\"}").c_str());
    }
  }
}

void readSTM32Data() {
  if (STM32Serial.available()) {
    String data = STM32Serial.readStringUntil('\n');
    data.trim();
    
    if (data.length() > 0) {
      processSTM32Data(data);
    }
  }
}

void processSTM32Data(String data) {
  // Parse: MOTOR:25,X:1.2,Y:-0.8,Z:2.5
  if (data.startsWith("MOTOR:")) {
    int motorIndex = data.indexOf(":");
    int xIndex = data.indexOf(",X:");
    int yIndex = data.indexOf(",Y:");
    int zIndex = data.indexOf(",Z:");
    
    if (motorIndex > 0 && xIndex > motorIndex) {
      // Motor speed
      String motorStr = data.substring(motorIndex + 1, xIndex);
      lastData.motor_speed = motorStr.toInt();
      
      // Gyro X
      if (yIndex > xIndex) {
        String xStr = data.substring(xIndex + 3, yIndex);
        lastData.gyro_x = xStr.toFloat();
      }
      
      // Gyro Y
      if (zIndex > yIndex) {
        String yStr = data.substring(yIndex + 3, zIndex);
        lastData.gyro_y = yStr.toFloat();
      }
      
      // Gyro Z
      if (zIndex > 0) {
        String zStr = data.substring(zIndex + 3);
        zStr.trim();
        lastData.gyro_z = zStr.toFloat();
      }
      
      // Calculate magnitude
      lastData.magnitude = sqrt(lastData.gyro_x * lastData.gyro_x + 
                               lastData.gyro_y * lastData.gyro_y + 
                               lastData.gyro_z * lastData.gyro_z);
      
      lastData.timestamp = millis();
    }
  }
}

void publishMQTTData() {
  // Individual topics (for specific subscribers)
  mqttClient.publish(topic_motor_speed, String(lastData.motor_speed).c_str());
  mqttClient.publish(topic_gyro_x, String(lastData.gyro_x, 2).c_str());
  mqttClient.publish(topic_gyro_y, String(lastData.gyro_y, 2).c_str());
  mqttClient.publish(topic_gyro_z, String(lastData.gyro_z, 2).c_str());
  mqttClient.publish(topic_magnitude, String(lastData.magnitude, 2).c_str());
  
  // Combined JSON (for dashboard apps)
  DynamicJsonDocument doc(512);
  doc["device_id"] = lastData.device_id;
  doc["motor_speed"] = lastData.motor_speed;
  doc["gyro_x"] = lastData.gyro_x;
  doc["gyro_y"] = lastData.gyro_y;
  doc["gyro_z"] = lastData.gyro_z;
  doc["magnitude"] = lastData.magnitude;
  doc["timestamp"] = lastData.timestamp;
  doc["uptime"] = millis() / 1000;
  
  String jsonString;
  serializeJson(doc, jsonString);
  mqttClient.publish(topic_all_data, jsonString.c_str());
  
  // Debug (every 10th publish)
  static int counter = 0;
  if (++counter % 10 == 0) {
    Serial.println("📤 MQTT Published: " + jsonString);
  }
}

void setupWebServer() {
  webServer.on("/", handleWebRoot);
  webServer.on("/data", handleWebData);
  webServer.on("/motor", HTTP_POST, handleWebMotor);
  
  webServer.begin();
  Serial.println("🌐 Web Server Started");
}

void handleWebRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>MQTT MotionTracker</title>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "</head><body>";
  html += "<h1>🚀 MQTT MotionTracker</h1>";
  html += "<h2>📊 Real-Time Data</h2>";
  html += "<p>⚡ Motor Speed: <b>" + String(lastData.motor_speed) + "%</b></p>";
  html += "<p>↔️ Gyro X: <b>" + String(lastData.gyro_x, 1) + " dps</b></p>";
  html += "<p>↕️ Gyro Y: <b>" + String(lastData.gyro_y, 1) + " dps</b></p>";
  html += "<p>🔄 Gyro Z: <b>" + String(lastData.gyro_z, 1) + " dps</b></p>";
  html += "<p>📏 Magnitude: <b>" + String(lastData.magnitude, 1) + " dps</b></p>";
  html += "<h2>🎮 Motor Control</h2>";
  html += "<button onclick='setMotor(0)'>🛑 STOP</button> ";
  html += "<button onclick='setMotor(25)'>🐌 25%</button> ";
  html += "<button onclick='setMotor(50)'>🚶 50%</button> ";
  html += "<button onclick='setMotor(75)'>🏃 75%</button> ";
  html += "<button onclick='setMotor(100)'>🚀 100%</button>";
  html += "<h2>📡 MQTT Info</h2>";
  html += "<p>📍 Broker: " + String(mqtt_server) + ":" + String(mqtt_port) + "</p>";
  html += "<p>🔗 Status: " + (mqttClient.connected() ? "✅ Connected" : "❌ Disconnected") + "</p>";
  html += "<p>🕒 Uptime: " + String(millis()/1000) + "s</p>";
  html += "<script>";
  html += "function setMotor(speed){";
  html += "fetch('/motor',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({speed:speed})});";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

void handleWebData() {
  DynamicJsonDocument doc(512);
  doc["motor_speed"] = lastData.motor_speed;
  doc["gyro_x"] = lastData.gyro_x;
  doc["gyro_y"] = lastData.gyro_y;
  doc["gyro_z"] = lastData.gyro_z;
  doc["magnitude"] = lastData.magnitude;
  doc["timestamp"] = lastData.timestamp;
  doc["mqtt_connected"] = mqttClient.connected();
  
  String json;
  serializeJson(doc, json);
  webServer.send(200, "application/json", json);
}

void handleWebMotor() {
  if (webServer.hasArg("plain")) {
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, webServer.arg("plain")) == DeserializationError::Ok) {
      int speed = doc["speed"];
      
      // Publish to MQTT
      String mqttMsg = "{\"speed\":" + String(speed) + "}";
      mqttClient.publish(topic_motor_control, mqttMsg.c_str());
      
      webServer.send(200, "text/plain", "MQTT Motor Command Sent: " + String(speed) + "%");
    } else {
      webServer.send(400, "text/plain", "Invalid JSON");
    }
  } else {
    webServer.send(400, "text/plain", "No data");
  }
} 