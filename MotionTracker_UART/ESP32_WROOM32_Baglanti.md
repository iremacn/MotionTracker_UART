# 🔌 ESP32 WROOM-32 Bağlantı Rehberi

## 📋 ESP32 WROOM-32 Özellikleri
- **WiFi**: 802.11 b/g/n (2.4 GHz)
- **Bluetooth**: v4.2 BR/EDR ve BLE
- **Flash**: 4MB
- **RAM**: 520KB
- **GPIO**: 30 pin (bazıları dahili kullanımda)
- **UART**: 3 adet (UART0, UART1, UART2)

## 🔗 STM32F3 ↔ ESP32 WROOM-32 Bağlantıları

### 📍 Fiziksel Pin Bağlantıları
```
STM32F3 Discovery          ESP32 WROOM-32
━━━━━━━━━━━━━━━━━━          ━━━━━━━━━━━━━━━━
PA9  (UART3 TX)     ──────► GPIO16 (Pin 16)
PA10 (UART3 RX)     ◄────── GPIO17 (Pin 17)  
3V3                 ──────► 3V3 (Pin 2)
GND                 ──────► GND (Pin 1)
```

### 🎯 ESP32 WROOM-32 Pinout Diyagramı
```
                    ESP32 WROOM-32
                   ┌─────────────────┐
                   │  1 GND      3V3 │ 2  ← STM32 3V3
                   │  3 EN        IO2│ 4
                   │  5 IO4      IO15│ 6
                   │  7 IO16     IO13│ 8  ← STM32 PA9 (TX)
STM32 PA10 (RX) →  │  9 IO17     IO12│10
                   │ 11 IO5      IO14│12
                   │ 13 IO18     IO27│14
                   │ 15 IO19     IO26│16
                   │ 17 IO21     IO25│18
                   │ 19 IO3      IO33│20
                   │ 21 IO1      IO32│22
                   │ 23 IO22     IO35│24
                   │ 25 IO23     IO34│26
                   │ 27 IO6-11   VN │28
                   │ 29 Flash    VP │30
                   └─────────────────┘
```

## ⚠️ WROOM-32 Özel Notları

### ✅ Güvenli GPIO Pinleri (kullanılabilir)
- **GPIO16, GPIO17**: UART2 için ideal
- **GPIO18, GPIO19**: SPI için
- **GPIO21, GPIO22**: I2C için
- **GPIO23, GPIO25-27**: Genel amaçlı

### ❌ Kaçınılması Gereken Pinler
- **GPIO0**: Boot mode kontrolü
- **GPIO2**: Boot sırasında HIGH olmalı
- **GPIO12**: Boot sırasında LOW olmalı  
- **GPIO15**: Boot sırasında LOW olmalı
- **GPIO6-11**: Flash memory için (kullanmayın!)

## 🛠️ Arduino IDE Kurulumu

### 1️⃣ ESP32 Board Package
```
File → Preferences → Additional Board Manager URLs:
https://dl.espressif.com/dl/package_esp32_index.json

Tools → Board → Boards Manager → "ESP32" ara ve kur
```

### 2️⃣ Board Ayarları
```
Board: "ESP32 Dev Module"
Upload Speed: "921600"
CPU Frequency: "240MHz (WiFi/BT)"
Flash Frequency: "80MHz"
Flash Mode: "QIO"
Flash Size: "4MB (32Mb)"
Partition Scheme: "Default 4MB..."
Core Debug Level: "None"
PSRAM: "Disabled"
```

### 3️⃣ Gerekli Kütüphaneler
```
Tools → Manage Libraries → Şunları kurun:
✅ ArduinoJson (v6.19.4 veya üzeri)
✅ ESP32 Arduino Core (2.0.0 veya üzeri)
```

## 🔌 Breadboard Bağlantı Şeması

```
Breadboard Layout:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

     STM32F3                               ESP32 WROOM-32
   ┌─────────────┐                       ┌─────────────────┐
   │             │                       │                 │
   │ PA9  (TX3) ●├─────── Kırmızı ──────►│● GPIO16 (RX2)   │
   │ PA10 (RX3) ●├─────── Mavi ─────────►│● GPIO17 (TX2)   │
   │ 3V3        ●├─────── Turuncu ──────►│● 3V3            │
   │ GND        ●├─────── Siyah ────────►│● GND            │
   │             │                       │                 │
   └─────────────┘                       └─────────────────┘

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## 💻 Programlama Adımları

### 1️⃣ ESP32 Kodu Yüklemesi
```bash
1. ESP32 WROOM-32'yi USB ile bilgisayara bağlayın
2. Arduino IDE'de board ayarlarını yapın
3. esp32_wifi_gateway.ino dosyasını açın
4. WiFi ayarlarını düzenleyin:
   const char* ssid = "WIFI_AGINIZ";
   const char* password = "WIFI_SIFRENIZ";
5. Upload butonuna tıklayın
```

### 2️⃣ Serial Monitor Kontrolü
```
Baud Rate: 115200

Beklenen Çıktı:
🚀 ESP32 WiFi Gateway Başlatılıyor...
WiFi'ye bağlanıyor......
✅ WiFi Bağlandı!
🌐 IP Adresi: 192.168.1.105
🖥️ Web Server Başlatıldı!
📱 Tarayıcıda şu adresi açın: http://192.168.1.105
```

## 🧪 Test ve Doğrulama

### ✅ Başarılı Bağlantı Göstergeleri
1. **ESP32 Serial Monitor**: IP adresi gösteriyor
2. **STM32 Terminal**: "ESP32 WiFi Modülü Başlatıldı!" mesajı
3. **Web Browser**: Dashboard açılıyor
4. **LED'ler**: STM32'de LED efektleri çalışıyor

### 🔧 Sorun Giderme - WROOM-32 Özel

#### ❌ Upload hatası
```bash
# EN butonunu basılı tutup BOOT butonuna basın
# Upload başlatın, ardından butonları bırakın
```

#### ❌ Sık sık resetleniyor
```bash
# GPIO2'ye pull-up direnci (10kΩ) ekleyin
# Veya GPIO2'yi kullanmayın
```

#### ❌ WiFi bağlantı sorunu
```bash
# 2.4GHz ağ kullandığınızdan emin olun
# SSID ve password'de özel karakter olmasın
# Router yakınında test edin
```

## 📊 WROOM-32 Performans Özellikleri

### 📈 WiFi Performansı
- **Menzil**: 50-100 metre (açık alan)
- **Bağlantı süresi**: 2-5 saniye
- **Veri transfer**: ~1 Mbps
- **Güç tüketimi**: 80mA (aktif), 10μA (deep sleep)

### ⚡ UART Performansı
- **Baud rate**: 115200 bps
- **Buffer size**: 256 byte
- **Latency**: <10ms

## 🎯 Gelişmiş Özellikler

### 🔮 Opsiyonel Geliştirmeler
1. **LED Status**: GPIO2'ye status LED'i ekleyin
2. **Reset Button**: GPIO0'a reset butonu ekleyin
3. **External Antenna**: WiFi menzili artırmak için
4. **Power Management**: Deep sleep modu ekleyin

### 📱 Mobil Optimizasyon
Web dashboard otomatik olarak mobil cihazlara uyum sağlar:
- **Responsive design**
- **Touch-friendly** butonlar
- **Auto-refresh** özelliği

---

**🚀 ESP32 WROOM-32 artık hazır! STM32 projesi ile mükemmel uyum sağlayacak.** 