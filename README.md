# TPT Adaptive Lighting

Firmware ESP32 untuk sistem pencahayaan adaptif **10 zona**. Setiap zona memiliki lampu LED COB yang diatur PWM, sensor cahaya BH1750, dan sensor IR HW-201. Lampu dapat diatur manual, dikontrol otomatis agar lux tetap di rentang target, atau dinyalakan saat ada deteksi IR.

## Fitur

- **4 mode operasi**
  - `MANUAL`: atur PWM tiap zona secara langsung.
  - `KONTROL`: kontrol otomatis per zona agar lux berada di antara threshold bawah dan atas.
  - `KALIBRASI`: kalibrasi BH1750 terhadap luxmeter (regresi linear, tersimpan di flash).
  - `MANUAL_IR`: lampu zona menyala saat IR mendeteksi, mati otomatis setelah timer habis.
- Batas maksimum PWM global untuk semua mode.
- Semua pengaturan lewat perintah teks di Serial Monitor.

## Perangkat Keras

| Komponen | Jumlah | Keterangan |
|---|---|---|
| ESP32 DevKit | 1 | Mikrokontroler |
| PCA9685 | 1 | Driver PWM 16 channel, alamat `0x40`, PWM 1000 Hz |
| TCA9548A | 2 | Multiplexer I2C, alamat `0x70` (bus terpisah) |
| BH1750 | 10 | Sensor cahaya, alamat `0x23` |
| HW-201 | 10 | Sensor IR, output LOW = terdeteksi |
| LED COB + driver | 10 | Lampu per zona |

Ringkasan wiring:

| Bus | SDA | SCL | Perangkat |
|---|---|---|---|
| `Wire` | GPIO 22 | GPIO 23 | PCA9685 |
| `Wire1` (TCA1) | GPIO 32 | GPIO 33 | BH1750 zona 0–7 |
| `Wire1` (TCA2) | GPIO 25 | GPIO 26 | BH1750 zona 8–9 |

Sensor IR zona 0–9: GPIO `13, 14, 27, 4, 16, 17, 34, 18, 19, 21`. GPIO 34 perlu resistor pull-up eksternal 10 kΩ ke 3,3 V.

Pemetaan lengkap ada di [SPESIFIKASI.md](SPESIFIKASI.md#2-perangkat-keras).

## Build & Upload

Kebutuhan:

- [arduino-cli](https://arduino.github.io/arduino-cli/) dengan core `esp32:esp32` (diuji di 3.3.12)
- Library:

```bash
arduino-cli lib install "Adafruit PWM Servo Driver Library" "BH1750"
```

Compile, upload, dan buka Serial Monitor (sesuaikan port):

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32 .
arduino-cli monitor -p COM5 -c baudrate=115200
```

## Penggunaan Singkat

Setelah boot, sistem berada di mode `MANUAL`. Contoh perintah:

```
help              # daftar perintah
maks 500          # batas maksimum PWM (0-4095)
3 200             # set PWM zona 3 ke 200
all 100           # set PWM semua zona
kontrol           # masuk mode kontrol otomatis
th 300 400        # target lux 300-400
manual_ir         # mode lampu berbasis sensor IR
timer 10          # lampu IR menyala 10 detik
kalibrasi         # masuk mode kalibrasi BH1750
status            # tampilkan semua status
off               # matikan semua lampu
```

Format monitor di mode `KONTROL`: `Z<zona>:<lux>/<pwm>`.

Daftar perintah lengkap ada di [SPESIFIKASI.md](SPESIFIKASI.md#10-antarmuka-serial).

## Dokumentasi

- [SPESIFIKASI.md](SPESIFIKASI.md): spesifikasi lengkap firmware (arsitektur, algoritma kontrol, kalibrasi, perintah serial).
- [tasks/](tasks/README.md): daftar task pengembangan berikutnya.
- [changes/](changes/README.md): catatan perubahan, kebutuhan wiring, dan hal yang perlu diperhatikan.

## Struktur Repo

```
sketch_oct28c.ino   firmware utama
SPESIFIKASI.md      dokumen spesifikasi
tasks/              dokumen task pengembangan
changes/            catatan perubahan per task
```
