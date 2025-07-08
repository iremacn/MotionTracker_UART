# MotionTracker UART Project

STM32F3 Discovery Board ile L3GD20 gyroscope sensörü kullanarak motor kontrolü yapan proje.

## 🎯 Proje Özellikleri

- **L3GD20 Gyroscope**: X, Y, Z eksenlerinde açısal hız ölçümü
- **Motor Kontrolü**: Gyroscope verilerine göre motor hızı kontrolü  
- **UART İletişimi**: Real-time veri görüntüleme (115200 baud)
- **SPI İletişimi**: Gyroscope ile haberleşme

## 🔧 Donanım Bağlantıları

### STM32F3 Discovery Board:
- **PE3**: CS (Chip Select)
- **PA5**: SCK (SPI Clock)
- **PA6**: MISO (Master In Slave Out) + Motor PWM
- **PA7**: MOSI (Master Out Slave In) + Motor Direction
- **PA2**: UART TX
- **PA3**: UART RX

### Kullanılan Bileşenler:
- STM32F3 Discovery Board
- L3GD20 Gyroscope Sensor
- HW-153 V1 Motor Driver
- DC Motor

## 📊 Çalışma Prensibi

1. **Gyroscope Okuma**: L3GD20 sensöründen SPI ile X, Y, Z açısal hız değerleri okunur
2. **Magnitude Hesaplama**: `magnitude = √(x² + y² + z²)`
3. **Motor Hızı**: Magnitude değeri 0.5-10 dps aralığından 0-100% motor hızına dönüştürülür
4. **UART Çıktısı**: `Gyro[X:1.2 Y:0.8 Z:-2.5] |2.9| -> Motor:26%` formatında terminal çıktısı

## 🚀 Kullanım

1. Projeyi STM32CubeIDE'de açın
2. Donanım bağlantılarını yapın
3. Kodu derleyip STM32F3 Discovery'ye yükleyin
4. UART Terminal (YAT Terminal önerilen) ile 115200 baud'da bağlanın
5. Board'u hareket ettirin ve motor tepkisini gözlemleyin

## 📁 Proje Yapısı

```
MotionTracker_UART/
├── Core/
│   ├── Inc/           # Header dosyaları
│   └── Src/           # Source dosyaları (main.c, L3GD20.c vb.)
├── Drivers/           # STM32 HAL drivers
└── MotionTracker_UART.ioc  # STM32CubeIDE yapılandırması
```

## 🛠️ Geliştirme Ortamı

- **IDE**: STM32CubeIDE
- **MCU**: STM32F303VCT6
- **HAL Library**: STM32F3xx
- **Terminal**: YAT Terminal (115200 baud)

## 📝 Notlar

- Motor kontrolü için `HW153_SetMotor()` fonksiyonu kullanılmaktadır
- Gyroscope kalibrasyonu için board'u düz bir yüzeyde tutun
- Terminal bağlantısı için doğru COM portunu seçtiğinizden emin olun

## 🤝 Katkıda Bulunma

Bu proje eğitim amaçlı geliştirilmiştir. Önerileriniz ve geliştirmeleriniz için issue açabilir veya pull request gönderebilirsiniz.

## 📄 Lisans

Bu proje MIT lisansı altında lisanslanmıştır. 