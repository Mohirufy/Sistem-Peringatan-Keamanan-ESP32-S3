# 🔐 Sistem Keamanan Otomatis IoT

Sistem keamanan berbasis ESP32-S3 dengan deteksi gerakan PIR, alarm buzzer, dan kontrol volume menggunakan rotary encoder. Proyek ini menggunakan FreeRTOS untuk multitasking pada dual-core processor.

## ✨ Fitur

- **Deteksi Gerakan Otomatis** - Sensor PIR mendeteksi pergerakan dan mengaktifkan alarm
- **Alarm Buzzer** - Buzzer dengan pola beep yang dapat dikontrol volumenya
- **Kontrol Volume** - Rotary encoder untuk mengatur volume alarm (1-10)
- **Display OLED** - Tampilan status sistem real-time pada layar SSD1306
- **LED Indikator** - LED merah menunjukkan status sistem aktif/nonaktif
- **Dual Core Processing** - Memanfaatkan kedua core ESP32-S3 
- **FreeRTOS Multitasking** - Queue, Mutex, dan Task untuk komunikasi antar core

## 🛠️ Komponen Hardware

| Komponen | Jumlah | Keterangan |
|----------|--------|------------|
| ESP32-S3 DevKit-C | 1 | Microcontroller utama |
| OLED SSD1306 128x64 | 1 | Display I2C |
| PIR Motion Sensor | 1 | Sensor deteksi gerakan |
| Buzzer | 1 | Alarm suara |
| LED Merah | 1 | Indikator status |
| Rotary Encoder KY-040 | 1 | Kontrol volume |
| Push Button Merah | 1 | Tombol aktivasi sistem |

## 📋 Pin Configuration

```
ESP32-S3 Pin Mapping:
├── OLED Display
│   ├── SDA → GPIO 8
│   └── SCL → GPIO 9
├── Input Sensors
│   ├── PIR Sensor → GPIO 3
│   ├── Button 1 → GPIO 5
│   └── Rotary Encoder
│       ├── CLK → GPIO 10
│       ├── DT → GPIO 11
│       └── SW → GPIO 4
└── Output Actuators
    ├── LED → GPIO 7
    └── Buzzer → GPIO 6
```

## 📚 Library Dependencies

Install library berikut melalui Arduino Library Manager:

```
- Adafruit GFX Library
- Adafruit SSD1306
- Wire (built-in)
```

## 🚀 Operasional Sistem

**Mengaktifkan Sistem:**
- Tekan **Button Merah (BTN1)** untuk mengaktifkan/menonaktifkan sistem
- LED merah akan menyala ketika sistem aktif
- Display OLED menampilkan status "Sistem Aktif"

**Mengatur Volume Alarm:**
- Putar **Rotary Encoder** untuk mengatur volume buzzer (1-10)
- Volume ditampilkan pada OLED display
- Volume yang lebih tinggi = pola beep lebih cepat dan lebih banyak

**Reset Sistem:**
- Tekan **Tombol Encoder (SW)** untuk mereset deteksi gerakan
- Display akan menampilkan "Reset"

**Deteksi Gerakan:**
- Ketika sistem aktif, sensor PIR akan mendeteksi gerakan
- Buzzer akan berbunyi dengan volume sesuai setting
- Display menampilkan peringatan "Gerakan Terdeteksi"

## 🏗️ Arsitektur Sistem

### FreeRTOS Task Architecture

```
Core 0 (Task Priority 1):
└── core0Task
    ├── Membaca input (Button, Encoder, PIR)
    ├── Update OLED Display
    └── Kirim data ke Core 1 via Queue

Core 1:
├── core1Task (Priority 1)
│   ├── Terima data dari Core 0
│   ├── Kontrol LED
│   └── Set flag buzzer
└── buzzerTask (Priority 2)
    └── Generate tone alarm dengan volume control
```

### Synchronization Mechanisms

- **Queue** - Komunikasi data antar core (Core 0 → Core 1)
- **Mutex** - Proteksi akses resource:
  - `ledMutex` - Kontrol LED
  - `buzzerMutex` - Kontrol buzzer
  - `displayMutex` - Akses OLED display
- **ISR** - Interrupt untuk button dan encoder

## 🎛️ Volume Control Logic

Sistem volume menggunakan pola beep yang berbeda:

| Volume | Karakteristik |
|--------|---------------|
| 1-3 | Beep lambat dengan jeda panjang |
| 4-7 | Beep sedang |
| 8-10 | Beep cepat dengan jeda minimal |

## 🔍 Serial Monitor Output

Monitor debugging via serial (115200 baud):
```
Sistem Keamanan IoT Siap!
Sistem AKTIF
Encoder Value: 5 | Buzzer Volume: 5
PIR State: MOTION DETECTED!
>>> GERAKAN TERDETEKSI - ALARM AKTIF <<<
=== CORE 1 DATA ===
System Active: YES
Motion Detected: YES
Buzzer Volume: 5
==================
>>> BUZZER ENABLED - Volume: 5
```

## 🌐 Simulasi Online

Proyek ini dapat disimulasikan di [Wokwi](https://wokwi.com/projects/449126163689876481):
```bash
https://wokwi.com/projects/449126163689876481
```
