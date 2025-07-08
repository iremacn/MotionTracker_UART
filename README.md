# MotionTracker UART Project

STM32F3 Discovery Board ile L3GD20 gyroscope sensörü kullanarak motor kontrolü yapan proje.

## 🎯 Proje Özellikleri

- **L3GD20 Gyroscope**: X, Y, Z eksenlerinde açısal hız ölçümü
- **Motor Kontrolü**: Gyroscope verilerine göre motor hızı kontrolü  
- **UART İletişimi**: Real-time veri görüntüleme (115200 baud)
- **SPI İletişimi**: Gyroscope ile haberleşme
- **Python GUI**: Real-time grafikler ve veri görselleştirme
- **Veri Kaydetme**: JSON formatında veri export

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

### STM32 Firmware:
1. Projeyi STM32CubeIDE'de açın
2. Donanım bağlantılarını yapın
3. Kodu derleyip STM32F3 Discovery'ye yükleyin

### GUI Arayüzü:
4. `python setup_gui.py` komutunu çalıştırın (otomatik kurulum)
5. GUI'de doğru COM portunu seçin ve "Bağlan" butonuna tıklayın
6. Real-time gyroscope grafikleri ve motor kontrolü görüntüleyin

### Manuel Terminal:
- UART Terminal (YAT Terminal) ile 115200 baud'da bağlanın
- Board'u hareket ettirin ve motor tepkisini gözlemleyin

## 📁 Proje Yapısı

```
MotionTracker_UART/
├── Core/
│   ├── Inc/           # Header dosyaları
│   └── Src/           # Source dosyaları (main.c, L3GD20.c vb.)
├── Drivers/           # STM32 HAL drivers
├── gui_interface.py   # Python GUI arayüzü
├── setup_gui.py       # GUI otomatik kurulum scripti
├── requirements.txt   # Python kütüphane gereksinimleri
├── MotionTracker_UART.ioc  # STM32CubeIDE yapılandırması
└── README.md          # Proje dökümantasyonu
```

## 🖥️ GUI Arayüzü Özellikleri

- **Real-time Grafikler**: X, Y, Z gyroscope verileri ve motor hızı
- **Anlık Veri Gösterimi**: Sayısal değerler ve progress bar
- **COM Port Yönetimi**: Otomatik port tespiti ve bağlantı kontrolü
- **Veri Kaydetme**: JSON formatında export
- **Ham Veri Görüntüsü**: Terminal benzeri veri akışı
- **Kolay Kurulum**: Otomatik dependency kurulumu

## 🛠️ Geliştirme Ortamı

- **IDE**: STM32CubeIDE
- **MCU**: STM32F303VCT6
- **HAL Library**: STM32F3xx
- **GUI**: Python 3.7+ (Tkinter, Matplotlib, PySerial)
- **Terminal**: YAT Terminal (115200 baud)

## 📝 Notlar

- Motor kontrolü için `HW153_SetMotor()` fonksiyonu kullanılmaktadır
- Gyroscope kalibrasyonu için board'u düz bir yüzeyde tutun
- Terminal bağlantısı için doğru COM portunu seçtiğinizden emin olun

## 🤝 Katkıda Bulunma

Bu proje eğitim amaçlı geliştirilmiştir. Önerileriniz ve geliştirmeleriniz için issue açabilir veya pull request gönderebilirsiniz.

## 📄 Lisans

Bu proje MIT lisansı altında lisanslanmıştır. 