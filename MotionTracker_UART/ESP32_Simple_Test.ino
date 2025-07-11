/*
 * ESP32 Simple Test - Web Server Only
 * Test if basic web server works
 */

#include <WiFi.h>
#include <WebServer.h>

// WiFi Ayarları
const char* ssid = "ARGE-MERKEZI";
const char* password = "D3ms4yArge-2025!";

// Web Server
WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Simple Test Starting...");
  
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
    WiFi.softAP("ESP32_Test", "12345678");
    Serial.println("\nAP Mode: http://" + WiFi.softAPIP().toString());
  }
  
  // Basit web server
  server.on("/", handleRoot);
  server.on("/test", handleTest);
  
  server.begin();
  Serial.println("Simple Web Server Started!");
}

void loop() {
  server.handleClient();
  delay(100);
}

// Basit ana sayfa
void handleRoot() {
  String html = "<html><head><title>ESP32 Test</title></head><body>";
  html += "<h1>ESP32 Test Page</h1>";
  html += "<p>Time: " + String(millis()/1000) + " seconds</p>";
  html += "<p><a href='/test'>Test Link</a></p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  Serial.println("Root page served");
}

// Test sayfası
void handleTest() {
  server.send(200, "text/plain", "ESP32 Test OK - " + String(millis()));
  Serial.println("Test page served");
} 