# 🚀 STM32 IoT MotionTracker System

**Industrial IoT Motion Control System** with professional web dashboard and MQTT connectivity.

![IoT Architecture](https://img.shields.io/badge/Architecture-Multi--MCU%20IoT-blue) ![STM32](https://img.shields.io/badge/STM32-F3%20Discovery-green) ![ESP32](https://img.shields.io/badge/ESP32-WROOM--32-red) ![MQTT](https://img.shields.io/badge/Protocol-MQTT-orange) ![WebApp](https://img.shields.io/badge/Interface-Professional%20Web-purple)

## 🎯 **System Overview**

Advanced IoT system combining **STM32F3 Discovery board** with **L3GD20 gyroscope** and **ESP32 WiFi gateway** to create a professional motion tracking and control platform with real-time web visualization.

### **🏗️ System Architecture**
```
STM32F3 Discovery ←→ ESP32 WROOM-32 ←→ WiFi Network
     ↓                      ↓                ↓
L3GD20 Gyroscope      Web Dashboard    MQTT Broker
HW-153 Motor Driver   Real-time Charts  Remote Control
```

## ✨ **Key Features**

### **🎛️ Professional Web Dashboard**
- **Corporate dark theme** design (#1a1a2e, #16213e)
- **Real-time analytics** with Chart.js integration
- **Responsive grid layout** with live sensor data
- **Motor control interface** with instant feedback
- **Status monitoring** (WiFi, MQTT, uptime)

### **📡 IoT Connectivity**
- **MQTT Protocol** integration (broker.hivemq.com)
- **Dual control modes**: Web Dashboard + MQTT commands
- **Real-time data streaming** (2Hz sensor publishing)
- **Remote monitoring** from any MQTT client
- **JSON API endpoints** for third-party integration

### **🔧 Hardware Integration**
- **STM32F3 Discovery** (STM32F303VCT6)
- **L3GD20 3-axis gyroscope** with SPI communication
- **HW-153 motor driver** with PWM control
- **ESP32 WROOM-32** WiFi gateway
- **UART communication** between microcontrollers

## 📊 **Technical Specifications**

| Component | Technology | Protocol |
|-----------|------------|----------|
| **Main MCU** | STM32F3 Discovery | UART, SPI, I2C |
| **WiFi Gateway** | ESP32 WROOM-32 | WiFi 802.11, MQTT |
| **Sensor** | L3GD20 Gyroscope | SPI (CS: PE3) |
| **Motor Control** | HW-153 Driver | PWM (PA6) |
| **Communication** | STM32 ↔ ESP32 | UART4 (9600 baud) |
| **Web Server** | ESP32 Embedded | HTTP REST API |

## 🚀 **Getting Started**

### **📦 Hardware Setup**

#### STM32F3 Discovery Connections:
```
PE3  → CS (Chip Select)
PA5  → SCK (SPI Clock)  
PA6  → MISO + Motor PWM
PA7  → MOSI + Motor Direction
PC10 → UART4 TX (to ESP32)
PC11 → UART4 RX (from ESP32)
```

#### ESP32 WROOM-32 Connections:
```
GPIO16 → STM32 UART4 TX
GPIO17 → STM32 UART4 RX
```

### **💻 Software Installation**

#### STM32 Firmware:
1. Open `MotionTracker_UART/` in **STM32CubeIDE**
2. Build and flash to STM32F3 Discovery
3. Monitor via UART2 (115200 baud)

#### ESP32 Gateway:
1. Open `ESP32_Unified_WebMQTT_Version.ino` in **Arduino IDE**
2. Install required libraries:
   - `WiFi` (built-in)
   - `WebServer` (built-in)
   - `ArduinoJson` (v6.21.0+)
   - `PubSubClient` (v2.8.0+)
3. Configure WiFi credentials in code
4. Upload to ESP32 WROOM-32

## 🌐 **Web Dashboard Features**

### **📱 Professional UI Components**
- **Status Bar**: WiFi IP, MQTT status, system uptime
- **Data Grid**: Live sensor values with hover effects
- **Progress Bar**: Visual motor speed indicator
- **Control Buttons**: STOP, 25%, 50%, 75%, 100% motor speeds
- **Real-time Chart**: Motor speed analytics with Chart.js

### **🎨 Design Specifications**
- **Typography**: Segoe UI, clean sans-serif
- **Color Palette**: Professional dark navy theme
- **Interactions**: Subtle hover animations, smooth transitions
- **Responsive**: Optimized for desktop and mobile
- **Accessibility**: High contrast, readable fonts

## 📡 **MQTT Integration**

### **🔗 Connection Details**
- **Broker**: `broker.hivemq.com:1883`
- **Client ID**: `ESP32_MotionTracker_Unique_2025`
- **Authentication**: Anonymous (public broker)

### **📋 Topic Structure**
```
motortracker/data/sensor     → Real-time sensor JSON data
motortracker/control/motor   → Motor control commands (0-100)
motortracker/status/online   → Device online status
```

### **📨 Message Formats**

#### Sensor Data (Published):
```json
{
  "device_id": "STM32F3_LIVE",
  "motor_speed": 50,
  "gyro": {"x": 1.2, "y": -0.8, "z": 2.5},
  "magnitude": 2.9,
  "timestamp": 1640995200,
  "ip": "192.168.1.100"
}
```

#### Motor Control (Subscribed):
```json
50  // Simple integer (0-100%)
```

## 🛠️ **Development Environment**

### **🔧 Required Tools**
- **STM32CubeIDE** (v1.18.1+)
- **Arduino IDE** (v1.8.0+) or PlatformIO
- **MQTTX Client** (for testing)
- **Git** (for version control)

### **📚 Libraries & Dependencies**
```cpp
// ESP32 Libraries
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

// STM32 HAL Libraries
#include "stm32f3xx_hal.h"
#include "L3GD20.h"
```

## 🎯 **Usage Examples**

### **🌐 Web Dashboard Access**
```
http://192.168.1.100  // ESP32 IP address
```

### **📱 MQTT Control Examples**

#### Using MQTTX Client:
```
Topic: motortracker/control/motor
Payload: 75
```

#### Using Python:
```python
import paho.mqtt.client as mqtt

client = mqtt.Client()
client.connect("broker.hivemq.com", 1883, 60)
client.publish("motortracker/control/motor", "50")
```

## 📊 **System Performance**

- **Data Rate**: 2Hz sensor publishing
- **Response Time**: <100ms motor control
- **Web Dashboard**: Real-time updates every 1 second
- **MQTT Latency**: <50ms command processing
- **Memory Usage**: STM32 <32KB, ESP32 <512KB

## 🔒 **Production Considerations**

### **🛡️ Security Features**
- Input validation on all interfaces
- MQTT message size limits
- WiFi WPA2 security
- Rate limiting on web endpoints

### **📈 Scalability**
- Multiple MQTT clients support
- RESTful API for integration
- JSON data format standardization
- Modular code architecture

## 🤝 **Contributing**

This project demonstrates **industrial IoT development** best practices:

1. **Multi-microcontroller communication**
2. **Professional web interface design**
3. **IoT protocol implementation** 
4. **Real-time data visualization**
5. **Production-ready architecture**

Feel free to fork, enhance, and submit pull requests!

## 📄 **License**

This project is licensed under the **MIT License** - see the LICENSE file for details.

## 🏆 **Achievements**

- ✅ **Multi-MCU Architecture**: STM32 + ESP32 integration
- ✅ **Professional UI/UX**: Corporate-grade web design
- ✅ **IoT Protocol Stack**: MQTT + HTTP REST API
- ✅ **Real-time Visualization**: Chart.js integration
- ✅ **Production Ready**: Error handling, validation, documentation

---

**⭐ If this project helped you, please give it a star!**

**🔗 Connect**: [LinkedIn](https://linkedin.com/in/yourusername) | [GitHub](https://github.com/iremacn) 