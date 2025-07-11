/*
 * ESP32 WROOM-32 WiFi Gateway for STM32 MotionTracker
 * STM32 → ESP32 WROOM-32 → WiFi → Web Dashboard
 * 
 * ESP32 WROOM-32 Pin Bağlantıları UART4 için:
 * ESP32 Pin 16 (RX2) ← STM32 PC10 (UART4 TX) 
 * ESP32 Pin 17 (TX2) → STM32 PC11 (UART4 RX)
 * ESP32 3V3          ← STM32 3V3
 * ESP32 GND          ← STM32 GND
 * 
 * Not: STM32F3 Discovery UART4 konfigürasyonu PC10/PC11 pinlerini kullanır
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// WiFi Ayarları - KENDI WiFi BİLGİLERİNİZİ GİRİN!
const char* ssid = "ARGE-MERKEZI";         // WiFi ağ adı
const char* password = "D3ms4yArge-2025!";        // WiFi şifresi

// Access Point ayarları (yedek)
const char* ap_ssid = "ESP32_MotorControl";
const char* ap_password = "12345678";

// Web Server
WebServer server(80);

// STM32 UART4 Ayarları - ESP32 WROOM-32 için
#define STM32_SERIAL_RX 16  // GPIO16 (RX2) ← STM32 PC10 (UART4 TX)
#define STM32_SERIAL_TX 17  // GPIO17 (TX2) → STM32 PC11 (UART4 RX)
HardwareSerial STM32Serial(2);  // ESP32 UART2 kullanarak STM32 UART4 ile iletişim

// DEBUG UART Test Variables
unsigned long last_test_time = 0;
uint8_t test_counter = 0;
bool uart_debug_mode = true;

// Veri Depolama
struct MotorData {
  String device_id;
  int motor_speed;
  float gyro_x;
  float gyro_y; 
  float gyro_z;
  float magnitude;
  unsigned long timestamp;
  String received_time;
};

MotorData lastData;
bool newDataReceived = false;

void setup() {
  Serial.begin(115200);
  
  // UART2 konfigürasyonunu debug et
  Serial.println("=== ESP32 UART4 FULL DEBUG MODE ===");
  Serial.println("Configuring ESP32 UART2 for STM32 UART4...");
  Serial.print("ESP32 RX Pin (GPIO16): STM32 UART4 TX (PC10)");
  Serial.println();
  Serial.print("ESP32 TX Pin (GPIO17): STM32 UART4 RX (PC11)");
  Serial.println();
  
  // UART2 başlat - parametreleri açıkça belirt (FIXED - NO BOOL ASSIGNMENT)
  STM32Serial.begin(9600, SERIAL_8N1, STM32_SERIAL_RX, STM32_SERIAL_TX);
  Serial.println("✅ ESP32 UART2 started for STM32 UART4 communication!");
  
  // Pin modlarını kontrol et
  Serial.println("Pin configurations:");
  Serial.printf("GPIO16 (RX): %d\n", digitalRead(STM32_SERIAL_RX));
  Serial.printf("GPIO17 (TX): %d\n", digitalRead(STM32_SERIAL_TX));
  
  Serial.println("ESP32 WiFi Gateway Starting...");
  Serial.println("UART4 FULL DEBUG MODE - Baud Rate: 9600");
  
  // WiFi bağlantısı dene
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_attempts < 20) {
    delay(500);
    Serial.print(".");
    wifi_attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi connection failed! Starting Access Point...");
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
  }
  
  // Web Server Route'ları
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/motor", HTTP_POST, handleMotorControl);
  server.on("/style.css", handleCSS);
  
  server.begin();
  Serial.println("Web Server Started!");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Open this address in browser: http://" + WiFi.localIP().toString());
  } else {
    Serial.println("Open this address in browser: http://" + WiFi.softAPIP().toString());
  }
  
  // STM32 UART4'e hazır sinyali gönder  
  Serial.println("Sending ready signal to STM32 UART4...");
  STM32Serial.println("ESP32_READY_UART4");
  STM32Serial.flush();  // Verinin gönderilmesini bekle
  
  Serial.println("=== ESP32 UART4 FULL DEBUG MODE ACTIVE ===");
  Serial.println("Serial Monitor Commands:");
  Serial.println("- Send single chars: A, B, C");
  Serial.println("- Send simple strings: HELLO, WORLD");
  Serial.println("- Send TEST format: TEST:Motor=50,X=1.2");
  Serial.println("- Example JSON: {\"device_id\":\"TEST\",\"motor_speed\":50,\"gyro_x\":1.2,\"gyro_y\":-0.8,\"gyro_z\":2.1,\"magnitude\":2.5,\"timestamp\":12345}");
  
  Serial.println("=== WAITING FOR STM32 UART4 DATA ===");
  Serial.println("Expecting A, B, C characters from STM32 UART4_Init()...");
  
  // İlk veri değerleri
  lastData.device_id = "STM32_UART4_WAITING";
  lastData.motor_speed = 0;
  lastData.gyro_x = 0.0;
  lastData.gyro_y = 0.0;
  lastData.gyro_z = 0.0;
  lastData.magnitude = 0.0;
  lastData.timestamp = millis();
  lastData.received_time = "BOOT_TIME";
}

void loop() {
  server.handleClient();
  
  // STM32 UART4'den veri oku ve RAW bayt analizi yap
  if (STM32Serial.available()) {
    // Tüm available byte'ları oku
    uint8_t raw_bytes[128];
    int bytes_read = 0;
    
    // Byte-by-byte okuma ile timing problemlerini önle
    while (STM32Serial.available() && bytes_read < 127) {
      raw_bytes[bytes_read] = STM32Serial.read();
      bytes_read++;
      delay(1);  // Byte'lar arasında küçük delay
    }
    
    raw_bytes[bytes_read] = 0;  // NULL terminate
    
    // RAW bytes tam analizi
    Serial.println("=== RAW BYTES RECEIVED FROM STM32 UART4 ===");
    Serial.print("Bytes count: ");
    Serial.println(bytes_read);
    
    Serial.print("HEX: ");
    for (int i = 0; i < bytes_read; i++) {
      Serial.printf("0x%02X ", raw_bytes[i]);
    }
    Serial.println();
    
    Serial.print("ASCII: ");
    for (int i = 0; i < bytes_read; i++) {
      if (raw_bytes[i] >= 32 && raw_bytes[i] <= 126) {
        Serial.printf("'%c' ", raw_bytes[i]);
      } else {
        Serial.printf("[%d] ", raw_bytes[i]);
      }
    }
    Serial.println();
    
    // Decimal değerler
    Serial.print("DEC: ");
    for (int i = 0; i < bytes_read; i++) {
      Serial.printf("%d ", raw_bytes[i]);
    }
    Serial.println();
    
    // String olarak işle
    String stm32_data = String((char*)raw_bytes);
    stm32_data.trim();
    
    if (stm32_data.length() > 0) {
      Serial.println("STM32 UART4'den veri: " + stm32_data);
      processSTM32Data(stm32_data);
    } else {
      Serial.println("⚠️  Empty string after trim");
    }
    
    Serial.println("========================");
  }
  
  // Serial Monitor'dan test verisi oku (TEST İÇİN)
  if (Serial.available()) {
    String test_data = Serial.readStringUntil('\n');
    test_data.trim();
    
    if (test_data.length() > 0) {
      Serial.println("✏️  TEST Data from Serial Monitor: " + test_data);
      processSTM32Data(test_data);
    }
  }
  
  // Periyodik STM32 UART4'e test mesajı gönder
  if (millis() - last_test_time > 15000) {  // Her 15 saniyede
    last_test_time = millis();
    test_counter++;
    
    String test_msg = "ESP32_TEST_" + String(test_counter);
    Serial.println("📤 Sending periodic test to STM32 UART4: " + test_msg);
    STM32Serial.println(test_msg);
    STM32Serial.flush();
  }
  
  delay(50);  // CPU yükünü azalt
}

// STM32 UART4'den gelen verisini işle (FULL DEBUG + DUAL FORMAT SUPPORT)
void processSTM32Data(String data) {
  Serial.println("=== PROCESSING STM32 UART4 DATA ===");
  Serial.println("Raw data received: '" + data + "'");
  Serial.print("Data length: ");
  Serial.println(data.length());
  
  // Boş veri kontrolü
  if (data.length() == 0) {
    Serial.println("❌ ERROR: EmptyInput");
    return;
  }
  
  // HEX formatında veriyi göster (ek debug)
  if (uart_debug_mode) {
    Serial.print("String HEX: ");
    for (int i = 0; i < data.length() && i < 50; i++) {
      Serial.printf("0x%02X ", (uint8_t)data[i]);
    }
    Serial.println();
  }
  
  // *** TEK KARAKTER KONTROLÜ - STM32 UART4_Init() test ***
  if (data.equals("A") || data.equals("B") || data.equals("C")) {
    Serial.println("✅ SINGLE CHARACTER TEST PASSED: " + data);
    Serial.println("🎯 STM32 UART4 is working correctly!");
    return;
  }
  
  // *** BASİT STRING KONTROLÜ ***
  if (data.equals("HELLO") || data.equals("WORLD")) {
    Serial.println("✅ SIMPLE STRING RECEIVED: " + data);
    Serial.println("🎯 Basic string communication working!");
    return;
  }
  
  // *** ESP32 TEST MESSAGE ECHO ***
  if (data.startsWith("ESP32_TEST_")) {
    Serial.println("🔄 ESP32 test message echoed back: " + data);
    Serial.println("🎯 Bidirectional communication confirmed!");
    return;
  }
  
  // *** BASİT FORMAT KONTROLÜ: MOTOR:75,X:8.5,Y:3.2,Z:6.1 ***
  if (data.startsWith("MOTOR:")) {
    Serial.println("✅ SIMPLE FORMAT DETECTED!");
    parseSimpleFormat(data);
    return;
  }
  
  // *** JSON FORMAT KONTROLÜ ***
  // JSON karakteri ile başlamıyorsa geç
  if (!data.startsWith("{")) {
    Serial.println("❌ ERROR: InvalidInput - Does not start with { and not MOTOR format");
    Serial.println("First character is: '" + String(data[0]) + "' (ASCII: " + String((int)data[0]) + ")");
    
    // JSON başlangıcını ara
    int json_start = data.indexOf('{');
    if (json_start > 0) {
      Serial.println("🔍 Found { at position: " + String(json_start));
      data = data.substring(json_start);
      Serial.println("✂️  Trimmed data: " + data);
    } else {
      Serial.println("🚫 No { found in data!");
      return;
    }
  }
  
  // JSON sonunu kontrol et
  if (!data.endsWith("}")) {
    Serial.println("⚠️  WARNING: JSON doesn't end with }");
    int json_end = data.lastIndexOf('}');
    if (json_end > 0) {
      data = data.substring(0, json_end + 1);
      Serial.println("✂️  Trimmed to: " + data);
    }
  }
  
  DynamicJsonDocument doc(1024);  // Buffer boyutunu artırdım
  DeserializationError error = deserializeJson(doc, data);
  
  if (error) {
    Serial.println("❌ JSON PARSE ERROR: " + String(error.c_str()));
    Serial.println("📝 Error code: " + String(error.code()));
    Serial.println("💾 Problematic data: " + data);
    
    // JSON hata türlerini detaylandır
    switch (error.code()) {
      case DeserializationError::InvalidInput:
        Serial.println("-> Invalid JSON format");
        break;
      case DeserializationError::IncompleteInput:
        Serial.println("-> Incomplete JSON data");
        break;
      case DeserializationError::NoMemory:
        Serial.println("-> Not enough memory");
        break;
      case DeserializationError::TooDeep:
        Serial.println("-> JSON too deep");
        break;
      default:
        Serial.println("-> Unknown error");
    }
    return;
  }
  
  Serial.println("✅ JSON PARSED SUCCESSFULLY!");
  
  // JSON alanlarının varlığını kontrol et
  if (!doc.containsKey("device_id") || !doc.containsKey("motor_speed") || 
      !doc.containsKey("gyro_x") || !doc.containsKey("gyro_y") || 
      !doc.containsKey("gyro_z") || !doc.containsKey("magnitude")) {
    Serial.println("❌ ERROR: Missing required JSON fields");
    
    // Hangi alanların eksik olduğunu göster
    if (!doc.containsKey("device_id")) Serial.println("-> Missing: device_id");
    if (!doc.containsKey("motor_speed")) Serial.println("-> Missing: motor_speed");
    if (!doc.containsKey("gyro_x")) Serial.println("-> Missing: gyro_x");
    if (!doc.containsKey("gyro_y")) Serial.println("-> Missing: gyro_y");
    if (!doc.containsKey("gyro_z")) Serial.println("-> Missing: gyro_z");
    if (!doc.containsKey("magnitude")) Serial.println("-> Missing: magnitude");
    
    return;
  }
  
  // Veriyi struct'a kaydet
  lastData.device_id = doc["device_id"].as<String>();
  lastData.motor_speed = doc["motor_speed"];
  lastData.gyro_x = doc["gyro_x"];
  lastData.gyro_y = doc["gyro_y"];
  lastData.gyro_z = doc["gyro_z"];
  lastData.magnitude = doc["magnitude"];
  lastData.timestamp = doc["timestamp"];
  lastData.received_time = String(millis());
  
  newDataReceived = true;
  
  Serial.println("🎉 JSON DATA PROCESSED SUCCESSFULLY!");
  Serial.println("🏷️  Device: " + lastData.device_id);
  Serial.println("⚡ Motor: " + String(lastData.motor_speed) + "%");
  Serial.println("📈 Gyro X: " + String(lastData.gyro_x, 2));
  Serial.println("📊 Gyro Y: " + String(lastData.gyro_y, 2));
  Serial.println("🔄 Gyro Z: " + String(lastData.gyro_z, 2));
  Serial.println("📏 Magnitude: " + String(lastData.magnitude, 2));
  Serial.println("========================");
}

// BASİT STRING formatını parse et: "MOTOR:75,X:8.5,Y:3.2,Z:6.1"
void parseSimpleFormat(String data) {
  Serial.println("Parsing simple format: " + data);
  
  // MOTOR: kısmını parse et
  int motorIndex = data.indexOf("MOTOR:");
  int xIndex = data.indexOf(",X:");
  int yIndex = data.indexOf(",Y:");
  int zIndex = data.indexOf(",Z:");
  
  if (motorIndex == -1 || xIndex == -1 || yIndex == -1 || zIndex == -1) {
    Serial.println("Simple format parse error - missing fields");
    return;
  }
  
  // Değerleri çıkar
  int motor_speed = data.substring(motorIndex + 6, xIndex).toInt();
  float gyro_x = data.substring(xIndex + 3, yIndex).toFloat();
  float gyro_y = data.substring(yIndex + 3, zIndex).toFloat();
  float gyro_z = data.substring(zIndex + 3).toFloat();
  
  // Magnitude hesapla
  float magnitude = sqrt(gyro_x*gyro_x + gyro_y*gyro_y + gyro_z*gyro_z);
  
  // Global struct'a kaydet
  lastData.device_id = "STM32F3_UART4";
  lastData.motor_speed = motor_speed;
  lastData.gyro_x = gyro_x;
  lastData.gyro_y = gyro_y;
  lastData.gyro_z = gyro_z;
  lastData.magnitude = magnitude;
  lastData.timestamp = millis();
  lastData.received_time = String(millis());
  
  newDataReceived = true;
  Serial.println("UART4 SIMPLE DATA PARSED - Motor: " + String(motor_speed) + "% X:" + String(gyro_x) + " Y:" + String(gyro_y) + " Z:" + String(gyro_z));
}

// Ana sayfa
void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<title>STM32 Motor Tracker</title>";
  html += "<link rel='stylesheet' href='/style.css'>";
  html += "<meta http-equiv='refresh' content='2'>";
  html += "</head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>🚀 STM32 MotionTracker Dashboard (UART4)</h1>";
  
  html += "<div class='status'>";
  html += "<h2>📊 Canli Veriler</h2>";
  html += "<div class='data-grid'>";
  
  html += "<div class='data-item'>";
  html += "<span class='label'>⚡ Motor Hizi:</span>";
  html += "<span class='value'>" + String(lastData.motor_speed) + "%</span>";
  html += "</div>";
  
  html += "<div class='data-item'>";
  html += "<span class='label'>↔️ Gyro X:</span>";
  html += "<span class='value'>" + String(lastData.gyro_x, 2) + " dps</span>";
  html += "</div>";
  
  html += "<div class='data-item'>";
  html += "<span class='label'>↕️ Gyro Y:</span>";
  html += "<span class='value'>" + String(lastData.gyro_y, 2) + " dps</span>";
  html += "</div>";
  
  html += "<div class='data-item'>";
  html += "<span class='label'>🔄 Gyro Z:</span>";
  html += "<span class='value'>" + String(lastData.gyro_z, 2) + " dps</span>";
  html += "</div>";
  
  html += "<div class='data-item'>";
  html += "<span class='label'>📏 Magnitude:</span>";
  html += "<span class='value'>" + String(lastData.magnitude, 2) + " dps</span>";
  html += "</div>";
  
  html += "</div>";
  html += "</div>";
  
  html += "<div class='motor-control'>";
  html += "<h2>🎮 Motor Kontrolu</h2>";
  html += "<div class='motor-progress'>";
  html += "<div class='progress-bar'>";
  html += "<div class='progress-fill' style='width: " + String(lastData.motor_speed) + "%'></div>";
  html += "</div>";
  html += "<span class='progress-text'>" + String(lastData.motor_speed) + "%</span>";
  html += "</div>";
  
  html += "<div class='control-buttons'>";
  html += "<button onclick='setMotor(0)' class='btn btn-stop'>⏹️ DURDUR</button>";
  html += "<button onclick='setMotor(25)' class='btn btn-slow'>🐌 YAVAS (25%)</button>";
  html += "<button onclick='setMotor(50)' class='btn btn-medium'>🚶 ORTA (50%)</button>";
  html += "<button onclick='setMotor(75)' class='btn btn-fast'>🏃 HIZLI (75%)</button>";
  html += "<button onclick='setMotor(100)' class='btn btn-full'>🚀 FULL (100%)</button>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='info'>";
  html += "<p>🕒 Son guncelleme: " + String(millis()/1000) + " saniye once</p>";
  if (WiFi.status() == WL_CONNECTED) {
    html += "<p>📶 WiFi: " + String(WiFi.RSSI()) + " dBm</p>";
  } else {
    html += "<p>📶 Mode: Access Point</p>";
  }
  html += "<p>🔧 Device ID: " + lastData.device_id + "</p>";
  html += "<p>📡 ESP32 IP: " + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) + "</p>";
  html += "<p>🔧 UART: STM32 UART4 (PC10/PC11) - 9600 baud</p>";
  html += "<p>📊 ESP32 Pin: GPIO16(RX), GPIO17(TX)</p>";
  html += "<p>🧪 Test Counter: " + String(test_counter) + "</p>";
  html += "</div>";
  
  html += "</div>";
  
  html += "<script>";
  html += "function setMotor(speed) {";
  html += "console.log('🎮 Button clicked: ' + speed + '%');";
  html += "fetch('/motor', {";
  html += "method: 'POST',";
  html += "headers: {'Content-Type': 'application/json'},";
  html += "body: JSON.stringify({speed: speed})";
  html += "})";
  html += ".then(response => {";
  html += "console.log('📡 Response status:', response.status);";
  html += "return response.text();";
  html += "})";
  html += ".then(data => {";
  html += "console.log('✅ Response data:', data);";
  html += "alert('Motor hizi ' + speed + '% olarak ayarlandi!\\nResponse: ' + data);";
  html += "setTimeout(() => location.reload(), 1000);";
  html += "})";
  html += ".catch(error => {";
  html += "console.error('❌ Error:', error);";
  html += "alert('HATA: ' + error);";
  html += "});";
  html += "}";
  html += "</script>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

// JSON veri endpoint'i
void handleData() {
  DynamicJsonDocument doc(512);
  doc["motor_speed"] = lastData.motor_speed;
  doc["gyro_x"] = lastData.gyro_x;
  doc["gyro_y"] = lastData.gyro_y;
  doc["gyro_z"] = lastData.gyro_z;
  doc["magnitude"] = lastData.magnitude;
  doc["timestamp"] = lastData.timestamp;
  doc["wifi_rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  doc["connection_mode"] = WiFi.status() == WL_CONNECTED ? "WiFi" : "AP";
  doc["ip_address"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  doc["uart_baud"] = 9600;
  doc["uart_status"] = "UART4_FULL_DEBUG_MODE";
  doc["rx_pin"] = STM32_SERIAL_RX;
  doc["tx_pin"] = STM32_SERIAL_TX;
  doc["test_counter"] = test_counter;
  doc["debug_mode"] = uart_debug_mode;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

// Motor kontrol endpoint'i
void handleMotorControl() {
  Serial.println("=== MOTOR CONTROL REQUEST RECEIVED ===");
  Serial.print("Request method: ");
  Serial.println(server.method());
  Serial.print("Has 'plain' arg: ");
  Serial.println(server.hasArg("plain") ? "YES" : "NO");
  
  if (server.hasArg("plain")) {
    String payload = server.arg("plain");
    Serial.println("Raw payload: " + payload);
    
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.println("❌ JSON Parse Error: " + String(error.c_str()));
      server.send(400, "text/plain", "JSON parse error");
      return;
    }
    
    int speed = doc["speed"];
    Serial.println("🎮 MOTOR CONTROL COMMAND: " + String(speed) + "%");
    Serial.println("🎯 Sending to STM32 UART4...");
    
    // STM32 UART4'e motor komutu gönder
    String command = "MOTOR_SET:" + String(speed);
    STM32Serial.println(command);
    STM32Serial.flush();  // Verinin gönderilmesini garantile
    Serial.println("📤 SENT TO STM32 UART4: " + command);
    Serial.println("========================================");
    
    server.send(200, "text/plain", "Motor hizi ayarlandi: " + String(speed) + "%");
  } else {
    Serial.println("❌ No 'plain' argument in request");
    Serial.print("Available args: ");
    for (int i = 0; i < server.args(); i++) {
      Serial.print(server.argName(i) + "=" + server.arg(i) + " ");
    }
    Serial.println();
    server.send(400, "text/plain", "Gecersiz istek - no plain arg");
  }
}

// CSS stil dosyası
void handleCSS() {
  String css = "body {";
  css += "font-family: 'Segoe UI', Arial, sans-serif;";
  css += "margin: 0;";
  css += "padding: 20px;";
  css += "background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);";
  css += "color: white;";
  css += "min-height: 100vh;";
  css += "}";
  
  css += ".container {";
  css += "max-width: 900px;";
  css += "margin: 0 auto;";
  css += "background: rgba(255, 255, 255, 0.1);";
  css += "padding: 30px;";
  css += "border-radius: 20px;";
  css += "backdrop-filter: blur(10px);";
  css += "box-shadow: 0 8px 32px rgba(31, 38, 135, 0.37);";
  css += "border: 1px solid rgba(255, 255, 255, 0.18);";
  css += "}";
  
  css += "h1 {";
  css += "text-align: center;";
  css += "font-size: 2.8em;";
  css += "margin-bottom: 30px;";
  css += "text-shadow: 2px 2px 4px rgba(0,0,0,0.3);";
  css += "background: linear-gradient(45deg, #FFD700, #FFA500);";
  css += "-webkit-background-clip: text;";
  css += "-webkit-text-fill-color: transparent;";
  css += "}";
  
  css += "h2 {";
  css += "color: #FFD700;";
  css += "border-bottom: 2px solid #FFD700;";
  css += "padding-bottom: 10px;";
  css += "font-size: 1.8em;";
  css += "}";
  
  css += ".data-grid {";
  css += "display: grid;";
  css += "grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));";
  css += "gap: 20px;";
  css += "margin-bottom: 30px;";
  css += "}";
  
  css += ".data-item {";
  css += "background: rgba(255, 255, 255, 0.2);";
  css += "padding: 20px;";
  css += "border-radius: 15px;";
  css += "display: flex;";
  css += "justify-content: space-between;";
  css += "align-items: center;";
  css += "transition: transform 0.3s ease;";
  css += "border: 1px solid rgba(255, 255, 255, 0.3);";
  css += "}";
  
  css += ".data-item:hover {";
  css += "transform: translateY(-5px);";
  css += "box-shadow: 0 5px 20px rgba(0,0,0,0.3);";
  css += "}";
  
  css += ".label {";
  css += "font-weight: bold;";
  css += "font-size: 1.1em;";
  css += "}";
  
  css += ".value {";
  css += "font-size: 1.4em;";
  css += "color: #FFD700;";
  css += "font-weight: bold;";
  css += "}";
  
  css += ".motor-control {";
  css += "background: rgba(255, 255, 255, 0.1);";
  css += "padding: 25px;";
  css += "border-radius: 15px;";
  css += "margin-bottom: 20px;";
  css += "}";
  
  css += ".motor-progress {";
  css += "margin-bottom: 25px;";
  css += "}";
  
  css += ".progress-bar {";
  css += "width: 100%;";
  css += "height: 40px;";
  css += "background: rgba(255, 255, 255, 0.3);";
  css += "border-radius: 20px;";
  css += "overflow: hidden;";
  css += "position: relative;";
  css += "margin-bottom: 15px;";
  css += "border: 2px solid rgba(255, 255, 255, 0.5);";
  css += "}";
  
  css += ".progress-fill {";
  css += "height: 100%;";
  css += "background: linear-gradient(90deg, #32CD32 0%, #FFD700 50%, #FF6347 100%);";
  css += "border-radius: 18px;";
  css += "transition: width 0.8s ease;";
  css += "box-shadow: 0 0 20px rgba(255, 215, 0, 0.6);";
  css += "}";
  
  css += ".progress-text {";
  css += "font-size: 1.8em;";
  css += "font-weight: bold;";
  css += "text-align: center;";
  css += "display: block;";
  css += "color: #FFD700;";
  css += "text-shadow: 2px 2px 4px rgba(0,0,0,0.5);";
  css += "}";
  
  css += ".control-buttons {";
  css += "display: grid;";
  css += "grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));";
  css += "gap: 15px;";
  css += "justify-content: center;";
  css += "}";
  
  css += ".btn {";
  css += "padding: 15px 25px;";
  css += "border: none;";
  css += "border-radius: 30px;";
  css += "font-size: 1.1em;";
  css += "font-weight: bold;";
  css += "cursor: pointer;";
  css += "transition: all 0.3s ease;";
  css += "backdrop-filter: blur(10px);";
  css += "border: 2px solid rgba(255, 255, 255, 0.3);";
  css += "color: white;";
  css += "text-shadow: 1px 1px 2px rgba(0,0,0,0.5);";
  css += "}";
  
  css += ".btn-stop { background: linear-gradient(45deg, #FF4444, #CC0000); }";
  css += ".btn-slow { background: linear-gradient(45deg, #FFFF44, #CCCC00); }";
  css += ".btn-medium { background: linear-gradient(45deg, #44FFFF, #00CCCC); }";
  css += ".btn-fast { background: linear-gradient(45deg, #44FF44, #00CC00); }";
  css += ".btn-full { background: linear-gradient(45deg, #FF44FF, #CC00CC); }";
  
  css += ".btn:hover {";
  css += "transform: translateY(-3px);";
  css += "box-shadow: 0 8px 25px rgba(0,0,0,0.4);";
  css += "filter: brightness(1.2);";
  css += "}";
  
  css += ".btn:active {";
  css += "transform: translateY(0);";
  css += "box-shadow: 0 4px 10px rgba(0,0,0,0.3);";
  css += "}";
  
  css += ".info {";
  css += "background: rgba(255, 255, 255, 0.15);";
  css += "padding: 20px;";
  css += "border-radius: 15px;";
  css += "margin-top: 20px;";
  css += "text-align: center;";
  css += "border: 1px solid rgba(255, 255, 255, 0.3);";
  css += "}";
  
  css += ".info p {";
  css += "margin: 8px 0;";
  css += "font-size: 1.1em;";
  css += "}";
  
  css += "@media (max-width: 768px) {";
  css += ".data-grid { grid-template-columns: 1fr; }";
  css += ".control-buttons { grid-template-columns: 1fr; }";
  css += "h1 { font-size: 2.2em; }";
  css += ".container { padding: 20px; margin: 10px; }";
  css += "}";
  
  server.send(200, "text/css", css);
} 