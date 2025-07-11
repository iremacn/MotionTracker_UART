# 🔌 ESP32 WiFi Gateway Bağlantı Rehberi

## 📋 Gerekli Malzemeler
- 1x ESP32 DevKit (NodeMCU-32S veya benzeri)
- 4x Jumper Kablo (erkek-erkek)
- 1x Breadboard (opsiyonel)
- USB Kablosu (ESP32 programlama için)

## 🔗 Pin Bağlantıları

### STM32F3 Discovery ↔ ESP32
```
STM32F3 Discovery          ESP32 DevKit
━━━━━━━━━━━━━━━━━━          ━━━━━━━━━━━━
PA9  (UART3 TX)     ──────► GPIO16 (RX2)
PA10 (UART3 RX)     ◄────── GPIO17 (TX2)  
3V3                 ──────► 3V3
GND                 ──────► GND
```

## ⚠️ Önemli Notlar
- **ESP32'yi 5V'a bağlamayın!** Sadece 3.3V kullanın
- **GND bağlantısını unutmayın** - ortak referans gerekli
- **STM32'nin PA9/PA10 pinleri** UART3 için kullanılıyor

## 🛠️ Kurulum Adımları

### 1️⃣ Arduino IDE Kurulumu
```bash
# Arduino IDE'yi indirin: https://www.arduino.cc/en/software
# ESP32 Board Package ekleyin:
# File > Preferences > Additional Board Manager URLs:
https://dl.espressif.com/dl/package_esp32_index.json

# Tools > Board > Boards Manager > "ESP32" ara ve kur
```

### 2️⃣ Gerekli Kütüphaneler
```cpp
// Arduino IDE > Tools > Manage Libraries > Şunları kurun:
- ArduinoJson (by Benoit Blanchon)
- ESP32 Arduino Core (otomatik gelir)
```

### 3️⃣ WiFi Ayarları
```cpp
// esp32_wifi_gateway.ino dosyasında bu satırları düzenleyin:
const char* ssid = "KENDI_WIFI_AGINIZ";
const char* password = "KENDI_WIFI_SIFRENIZ";
```

### 4️⃣ ESP32 Programlama
1. ESP32'yi USB ile bilgisayara bağlayın
2. Arduino IDE'de Board: "ESP32 Dev Module" seçin
3. Port: COM portunu seçin
4. `esp32_wifi_gateway.ino` dosyasını açın
5. Upload butonuna tıklayın

## 🧪 Test Adımları

### 1️⃣ Serial Monitor Kontrolü
```
ESP32 Arduino IDE Serial Monitor (115200 baud):
🚀 ESP32 WiFi Gateway Başlatılıyor...
✅ WiFi Bağlandı!
🌐 IP Adresi: 192.168.1.xxx
🖥️ Web Server Başlatıldı!
📱 Tarayıcıda şu adresi açın: http://192.168.1.xxx
```

### 2️⃣ STM32 Terminal Kontrolü
```
STM32 Serial Terminal (115200 baud):
📡 ESP32 WiFi Modülü Başlatıldı!
📡 ESP32'ye veri gönderildi (Motor:45%)
🎮 ESP32 Komutu: MOTOR_SET:75
✅ Motor hızı uzaktan ayarlandı: 75%
```

### 3️⃣ Web Dashboard Testi
1. Tarayıcıda `http://ESP32_IP_ADRESI` açın
2. Gyroscope verilerinin güncellendiğini kontrol edin
3. Motor kontrol butonlarını test edin

## 🔧 Sorun Giderme

### ❌ WiFi bağlanamıyor
- WiFi adı ve şifresini kontrol edin
- 2.4GHz ağ kullandığınızdan emin olun (5GHz desteklenmez)
- Router'ın ESP32'ye yakın olduğundan emin olun

### ❌ STM32 ile iletişim yok
- Pin bağlantılarını kontrol edin (RX-TX çaprazlama)
- Her iki cihazın GND'lerinin bağlı olduğundan emin olun
- Baud rate'in her iki tarafta da 115200 olduğunu kontrol edin

### ❌ Web sayfası açılmıyor
- ESP32'nin IP adresini Serial Monitor'den kontrol edin
- Firewall/antivirus yazılımını geçici olarak kapatın
- Tarayıcı cache'ini temizleyin

### ❌ JSON parse hatası
- STM32'den gelen veri formatını kontrol edin
- UART iletişiminde veri kaybı olup olmadığını kontrol edin

## 📊 Beklenen Performans
- **Veri güncelleme**: Her 2.5 saniyede bir (STM32 500ms delay × 5)
- **Web sayfası yenileme**: Her 2 saniyede bir otomatik
- **Motor komut gecikmesi**: <100ms
- **WiFi menzil**: 10-50 metre (ortama göre)

## 🎯 Sonraki Adımlar
1. **Mobil uyumlu** arayüz geliştirin
2. **Veri loglama** özelliği ekleyin  
3. **Alarm sistemi** entegre edin
4. **Multiple device** desteği ekleyin
5. **MQTT** protokolü ile cloud bağlantısı yapın 