/*
 * ESP32 MQTT Debug - Simple Test
 * Test basic MQTT connectivity
 */

#include <WiFi.h>
#include <PubSubClient.h>

// WiFi
const char* ssid = "ARGE-MERKEZI";
const char* password = "D3ms4yArge-2025!";

// MQTT
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
int counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 MQTT Debug Test");
  
  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    counter++;
    
    String msg = "ESP32 Test Message #" + String(counter);
    Serial.print("Publishing: ");
    Serial.println(msg);
    
    client.publish("motortracker/test/esp32", msg.c_str());
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println(messageTemp);
  
  // Test motor command
  if (String(topic) == "motortracker/control/motor_set") {
    Serial.println("✅ MOTOR COMMAND RECEIVED!");
    Serial.println("Message: " + messageTemp);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println(" connected!");
      
      // Subscribe
      client.subscribe("motortracker/control/motor_set");
      client.subscribe("motortracker/test/+");
      
      Serial.println("Subscribed to motor_set topic");
      
      // Publish initial message
      client.publish("motortracker/status/esp32", "ESP32 Connected and Ready");
      
    } else {
      Serial.print(" failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
} 