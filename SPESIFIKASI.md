# Spesifikasi Sistem — Adaptive Lighting 10 Zona

Dokumen ini menjelaskan perilaku firmware `sketch_oct28c.ino` saat ini, sebagai acuan untuk pengembangan berikutnya.

> Kondisi kode: setelah task 01, 02, 03, 05, dan 06 diterapkan, compile OK di `esp32:esp32` 3.3.12 (flash 25%, RAM 7%). Perilaku baru belum diuji di hardware.

---

## 1. Ringkasan

Firmware ESP32 untuk mengatur **10 zona lampu LED COB** melalui driver PWM **PCA9685**. Setiap zona punya:

- 1 lampu (1 channel PCA9685),
- 1 sensor cahaya **BH1750** (lewat multiplexer I2C **TCA9548A**),
- 1 sensor halangan/kehadiran **IR HW-201**.

Sistem dioperasikan lewat **Serial Monitor (115200 baud)** dengan perintah teks, dan punya 4 mode:

| Mode | Fungsi singkat |
|---|---|
| `MANUAL` | PWM tiap zona diatur langsung oleh pengguna (mode awal). |
| `KONTROL` | Kontrol otomatis: PWM disesuaikan agar lux tiap zona berada di rentang threshold. |
| `KALIBRASI` | Mengambil titik kalibrasi BH1750 zona 0 terhadap luxmeter. |
| `MANUAL_IR` | Lampu zona menyala saat sensor IR zona tersebut mendeteksi, mati setelah timer habis. |

---

## 2. Perangkat Keras

### 2.1 Komponen

| Komponen | Jumlah | Keterangan |
|---|---|---|
| ESP32 DevKit (`esp32:esp32:esp32`) | 1 | Mikrokontroler utama |
| PCA9685 | 1 | Driver PWM 16 channel 12-bit, alamat `0x40`, hanya CH 0–9 yang dipakai |
| TCA9548A | 2 | Multiplexer I2C, keduanya alamat `0x70` (A0–A2 = GND), berada di bus fisik berbeda |
| BH1750 | 10 | Sensor cahaya, alamat `0x23` (ADDR = GND) |
| HW-201 (IR) | 10 | Output digital, **LOW = terdeteksi** |
| LED COB + driver | 10 | Dikendalikan PWM PCA9685 @ 1000 Hz |

### 2.2 Bus I2C

| Bus | Objek | SDA | SCL | Frekuensi | Perangkat |
|---|---|---|---|---|---|
| Bus PWM | `Wire` | GPIO 22 | GPIO 23 | default | PCA9685 |
| Bus sensor (TCA1) | `Wire1` | GPIO 32 | GPIO 33 | 100 kHz | TCA9548A #1 → BH1750 zona 0–7 |
| Bus sensor (TCA2) | `Wire1` | GPIO 25 | GPIO 26 | 100 kHz | TCA9548A #2 → BH1750 zona 8–9 |

Karena kedua TCA9548A memakai alamat yang sama (`0x70`), keduanya diletakkan di pin berbeda dan **`Wire1` dipindah-pindah pinnya** (`Wire1.end()` → `Wire1.begin(...)`) oleh fungsi `pilihBus()`. Variabel `busAktif` menyimpan bus yang sedang aktif agar perpindahan hanya dilakukan bila perlu.

### 2.3 Pemetaan Zona

| Zona | PCA9685 CH | TCA | Channel TCA | GPIO IR |
|---|---|---|---|---|
| 0 | 0 | TCA1 | 0 | 13 |
| 1 | 1 | TCA1 | 1 | 14 |
| 2 | 2 | TCA1 | 2 | 27 |
| 3 | 3 | TCA1 | 3 | 4 |
| 4 | 4 | TCA1 | 4 | 16 |
| 5 | 5 | TCA1 | 5 | 17 |
| 6 | 6 | TCA1 | 6 | 34 |
| 7 | 7 | TCA1 | 7 | 18 |
| 8 | 8 | TCA2 | 0 | 19 |
| 9 | 9 | TCA2 | 1 | 21 |

Sumber pemetaan di kode: `PIN_MAP`, `SENSOR_TCA`, `SENSOR_CH`, `IR_PIN`.

Sensor IR tidak memakai pin strapping (GPIO 0, 2, 5, 12, 15). GPIO 34 (zona 6) adalah pin input-only tanpa pull-up internal, jadi perlu **resistor pull-up eksternal 10 kΩ ke 3,3 V**. Pin IR lain memakai `INPUT_PULLUP`.

---

## 3. Dependensi

| Library | Versi terpasang | Kegunaan |
|---|---|---|
| Adafruit PWM Servo Driver Library | 3.0.3 | PCA9685 |
| BH1750 (claws) | 1.3.0 | Sensor cahaya |
| Adafruit BusIO | 1.17.4 | Dependensi Adafruit |
| Preferences | bawaan core ESP32 | Simpan kalibrasi di NVS |
| Wire | bawaan core ESP32 | I2C |

Core: `esp32:esp32` 3.3.12.

Perintah build (lihat `CLAUDE.md`):

```
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32 .
arduino-cli monitor -p COM5 -c baudrate=115200
```

---

## 4. Arsitektur Firmware

Semua kode ada di satu file `sketch_oct28c.ino`, terbagi dalam bagian:

| Bagian | Fungsi utama |
|---|---|
| PCA9685 | `setChannel()`, `semuaOff()` |
| Bus sensor / TCA / BH1750 | `pilihBus()`, `tcaSelect()`, `cekTCA()`, `bacaSensorMentah()`, `bacaSensor()`, `bacaMentahRata()`, `initSensor()`, `pulihkanSensor()` |
| Kalibrasi | `hitungKalibrasi()`, `simpanKalibrasi()`, `muatKalibrasi()`, `cetakKalibrasi()`, `tambahTitikKalibrasi()`, `cekKalibrasi()` |
| Sistem kontrol | `langkahKontrol()`, `cetakMonitor()` |
| Mode IR | `pwmIrZona()`, `cetakPwmIr()`, `prosesIR()`, `cetakIR()` |
| Tampilan serial | `cetakBantuan()`, `cetakBantuanKalibrasi()`, `cetakBantuanIR()`, `cetakStatus()`, `bacaSemua()` |
| Parsing | `isAngka()`, `isDesimal()`, `pisah()`, `bacaBaris()`, `prosesPerintah()` |
| Entry point | `setup()`, `loop()` |

### 4.1 Urutan `setup()`

1. `Serial.begin(115200)`, jeda 1 detik, cetak banner.
2. Pin IR → `INPUT_PULLUP` (GPIO ≥ 34 → `INPUT`); `irIkutMaks[z] = true`.
3. `Wire.begin(22, 23)`, `pwm.begin()`, frekuensi 1000 Hz, semua 16 channel PCA9685 = 0.
4. `initSensor()`: cek kedua TCA, lalu `begin()` tiap BH1750 dalam `CONTINUOUS_HIGH_RES_MODE`. Status per zona disimpan di `sensorOk[]`.
5. `prefs.begin("kalbh")` dan `muatKalibrasi()`.
6. Cetak bantuan, mode awal **MANUAL**.

### 4.2 `loop()`

Non-blocking (kecuali saat ambil titik kalibrasi):

```
baca satu baris serial -> prosesPerintah()
semua mode     : pulihkanSensor(now) di setiap iterasi
jika KONTROL   : tiap 500 ms langkahKontrol(), tiap 1000 ms cetakMonitor()
jika MANUAL_IR : prosesIR(now) di setiap iterasi
MANUAL / KALIBRASI : tidak ada proses periodik lain
```

Perbandingan waktu memakai `now - t >= interval` sehingga aman terhadap overflow `millis()`.

---

## 5. Output PWM

- Resolusi 12-bit: `0` = full OFF, `4095` = full ON (`pwm.setPin()`).
- **Semua** penulisan PWM lewat `setChannel()`, yang memotong nilai ke `pwmMaks`.
- `pwmMaks` default **100** (~2,4%), bisa diubah dengan `maks <0–4095>`. Saat diturunkan, zona yang melebihi batas baru langsung diturunkan.
- `nilaiCh[]` menyimpan nilai PWM terakhir per zona (sumber data status/monitor).

---

## 6. Pembacaan Sensor Cahaya

- `bacaSensorMentah(z)`: pilih bus & channel TCA, baca BH1750. Mengembalikan **nilai negatif** bila sensor sedang ditandai gagal, TCA gagal di-select, pembacaan gagal, atau init baru berjalan < 200 ms (pengukuran pertama belum siap).
- `bacaSensor(z)`: nilai terkalibrasi `lux = kalGain * mentah + kalOffset`, dipotong minimal 0. Nilai negatif = error.
- `luxCh[]` menyimpan bacaan terakhir (`-1` = error / belum dibaca). Hanya diperbarui oleh `langkahKontrol()` dan perintah `baca`.

### 6.1 Pemulihan sensor

- 3 kali gagal baca beruntun (`SENSOR_GAGAL_MAKS`) → `sensorOk[z] = false` dan log `[SENSOR] Zona <z> hilang`. Satu kali baca berhasil mereset hitungan ini.
- Di semua mode, `pulihkanSensor()` mencoba init ulang zona yang `sensorOk[z] == false`, maksimal sekali per 5 detik per zona (`SENSOR_RETRY_MS`). Alamat sensor dicek dulu supaya `begin()` tidak mencetak error berulang. Saat berhasil, tercetak `[SENSOR] Zona <z> pulih`.
- Perintah `init` (alias `scan`) menjalankan `initSensor()` secara manual dan mencetak hasil per zona. Perintah ini blocking sekitar 400 ms bila semua sensor merespons.
- **Keputusan:** selama sensor error di mode KONTROL, PWM zona tersebut tetap **ditahan** di nilai terakhir, bukan diturunkan ke nilai aman, supaya lampu tidak tiba-tiba mati atau berubah saat kabel sensor bermasalah.
- Keterbatasan: kegagalan dideteksi hanya saat sensor dibaca, jadi di mode MANUAL dan MANUAL_IR sensor yang tercabut baru terdeteksi lewat perintah `baca`. Bila sensor kehilangan daya sebentar tanpa 3 kali gagal baca beruntun, sensor tidak di-init ulang otomatis. Pakai `init` bila bacaannya terlihat salah.

---

## 7. Mode KONTROL

Masuk dengan `kontrol` (alias `auto`). Ditolak jika `thBawah >= thAtas`.

Setiap `CONTROL_INTERVAL` (500 ms), untuk setiap zona:

```
lux = bacaSensor(z)
jika error              -> PWM zona ditahan (tidak diubah)
jika KP <= 0            -> PWM zona ditahan (lux tetap diperbarui)
jika lux < thBawah      -> delta = +clamp((thBawah - lux) * KP, STEP_MIN, STEP_MAX)
jika lux > thAtas       -> delta = -clamp((lux - thAtas) * KP, STEP_MIN, STEP_MAX)
selain itu (dalam band) -> delta = 0
PWM baru = clamp(PWM lama + delta, 0, pwmMaks)
```

Ini adalah kontroler **proporsional bertahap dengan deadband**: tiap zona bekerja independen berdasarkan sensornya sendiri.

| Parameter | Default | Perintah |
|---|---|---|
| `thBawah` | 300 lx | `th <bawah> <atas>` |
| `thAtas` | 400 lx | `th <bawah> <atas>` |
| `KP` | 0.05 PWM/lx | `kp <nilai>` (`kp 0` membekukan PWM) |
| `STEP_MIN` | 1 | `step <min> <maks>` |
| `STEP_MAX` | 5 | `step <min> <maks>` |

Monitor dicetak tiap 1 detik dengan format:

```
[KONTROL 300-400 lx, maks 100] Z0:312.5/45  Z1:ERR/0  ...
```

`Z<zona>:<lux>/<pwm>`.

Selama mode KONTROL, perintah set PWM per zona (`<zona> <nilai>`, `all <nilai>`) ditolak.

---

## 8. Mode KALIBRASI

Tujuan: memetakan bacaan mentah BH1750 ke nilai luxmeter referensi. Kalibrasi diambil **hanya dari zona 0** (`KAL_ZONA`) lalu **dipakai untuk semua zona**.

### 8.1 Alur pengguna

1. `kalibrasi` → masuk mode, tampil bantuan dan bacaan zona 0 saat ini.
2. Atur cahaya (mis. `0 50` atau `all 50`), ketik nilai luxmeter (mis. `152`).
3. Firmware merata-rata 8 sampel (`KAL_SAMPEL`, jeda 130 ms → ~1 detik, **blocking**), menyimpan pasangan (mentah, referensi).
4. Ulangi 3–5 titik dari gelap ke terang (maksimal 10 titik, `KAL_MAKS`).
5. `selesai` atau `manual` → kembali ke MANUAL.

### 8.2 Perhitungan (`hitungKalibrasi()`)

| Kondisi | Hasil |
|---|---|
| 0 titik | `GAIN = 1`, `OFFSET = 0` |
| 1 titik, atau variansi mentah kecil (`n·Σx² − (Σx)² < n`) | `GAIN = Σref / Σmentah`, `OFFSET = 0` |
| ≥ 2 titik | Regresi linear least squares → `GAIN`, `OFFSET`, `R²` |
| Hasil regresi `GAIN ≤ 0` | Fallback ke rasio seperti di atas |

`R²` hanya dihitung pada jalur regresi; selain itu ditampilkan `-`.

### 8.3 Penyimpanan (NVS / Preferences)

Namespace `kalbh`:

| Key | Tipe | Isi |
|---|---|---|
| `n` | `uchar` | jumlah titik |
| `raw` | bytes (`float[10]`) | bacaan mentah |
| `ref` | bytes (`float[10]`) | bacaan luxmeter |

`GAIN`/`OFFSET` tidak disimpan, melainkan **dihitung ulang** saat boot dari titik yang tersimpan. Data dianggap rusak (dan diabaikan) bila `n > 10` atau ukuran blob tidak cocok.

Titik disimpan otomatis setiap kali ada perubahan (tambah, `batal`, `reset`).

---

## 9. Mode MANUAL_IR

Masuk dengan `manual_ir`: semua zona dimatikan (`semuaOff()`).

Untuk setiap zona, di setiap iterasi `loop()`:

```
jika IR zona z LOW (terdeteksi):
    irTerakhir[z] = now                 # timer di-reset
    jika lampu belum nyala: nyalakan dengan pwmIrZona(z), log "[IR] ... ON"
jika tidak terdeteksi dan lampu nyala dan (now - irTerakhir[z] >= irTimerMs):
    matikan lampu, log "[IR] ... OFF"
```

| Parameter | Default | Perintah |
|---|---|---|
| `irTimerMs` | 10000 ms | `timer <detik>` (boleh desimal, > 0) |
| PWM saat terdeteksi | mengikuti `pwmMaks` | `<zona> <nilai>` / `all <nilai>` saat di mode MANUAL_IR |

Di mode ini, `<zona> <nilai>` **tidak langsung menyalakan lampu**, melainkan mengubah PWM yang dipakai saat terdeteksi. Zona yang sedang menyala langsung ikut berubah. Perubahan `timer` langsung berlaku juga untuk lampu yang sedang menyala.

PWM saat terdeteksi ditentukan oleh `pwmIrZona(z)`:

- `irIkutMaks[z] == true` (default): PWM mengikuti `pwmMaks`.
- `irIkutMaks[z] == false` (setelah `<zona> <nilai>` / `all <nilai>`): PWM memakai `irPwmOn[z]`, dibatasi `pwmMaks`. Nilainya tidak tertimpa saat `maks` diubah.

Saat `maks` diubah, zona yang sedang menyala langsung ikut memakai nilai barunya. `status` dan `ir` menampilkan sumber nilainya, mis. `PWM IR 100 (maks)` atau `PWM IR 80 (manual)`. Zona yang sudah diset manual kembali mengikuti `pwmMaks` setelah reboot.

Sensor IR **tidak diproses** di mode lain (perintah `ir` tetap bisa menampilkan status pin).

---

## 10. Antarmuka Serial

### 10.1 Format input

- Baud 115200, baris diakhiri `\n` atau `\r`.
- Maksimal 64 karakter per baris (sisa dibuang).
- Tidak peka huruf besar/kecil; koma dianggap spasi; maksimal 3 token.
- `isAngka`: hanya digit (tanpa tanda minus). `isDesimal`: digit dengan maksimal satu titik.

### 10.2 Daftar perintah

**Perintah umum (berlaku di semua mode):**

| Perintah | Fungsi |
|---|---|
| `help` | Bantuan sesuai mode aktif |
| `status` | Mode, PWM maks, timer IR, threshold, KP, step, kalibrasi, PWM & lux per zona |
| `baca` | Baca semua BH1750 (terkalibrasi) |
| `init` / `scan` | Inisialisasi ulang semua BH1750 dan tampilkan hasilnya |
| `kalinfo` | Tampilkan GAIN, OFFSET, R², daftar titik kalibrasi |
| `ir` | Status semua sensor IR dan sisa timer |
| `maks <0–4095>` | Batas maksimum PWM (nilai > 4095 dipotong) |
| `timer <detik>` | Lama lampu menyala di MANUAL_IR |
| `th <bawah> <atas>` | Threshold kontrol (bawah < atas) |
| `kp <nilai>` | Gain kontrol (`kp 0` membekukan PWM) |
| `step <min> <maks>` | Langkah PWM kontrol (1 ≤ min ≤ maks) |
| `manual` / `selesai` | Ke mode MANUAL, PWM terakhir dipertahankan |
| `manual_ir` | Ke mode MANUAL_IR (semua lampu OFF dulu) |
| `kontrol` / `auto` | Ke mode KONTROL |
| `kalibrasi` | Ke mode KALIBRASI |
| `off` | Semua zona OFF dan pindah ke MANUAL |

**Perintah PWM (MANUAL, KALIBRASI, MANUAL_IR; ditolak di KONTROL):**

| Perintah | Fungsi |
|---|---|
| `<zona> <nilai>` | Zona 0–9, nilai 0–4095 (dipotong ke `pwmMaks`) |
| `all <nilai>` | Semua zona |

**Khusus mode KALIBRASI:**

| Perintah | Fungsi |
|---|---|
| `<angka>` (satu token) | Nilai luxmeter → tambah titik kalibrasi |
| `cek` | Bacaan zona 0 mentah dan terkalibrasi |
| `batal` | Hapus titik terakhir |
| `reset` | Hapus semua titik (GAIN=1, OFFSET=0) |

### 10.3 Transisi mode

```
            manual / selesai / off
   +-------------------------------------------+
   v                                           |
MANUAL --kontrol/auto--> KONTROL               |
   |  --kalibrasi------> KALIBRASI ------------+
   |  --manual_ir------> MANUAL_IR ------------+
   (setiap mode dapat langsung berpindah ke mode lain dengan perintahnya)
```

Saat berpindah ke MANUAL, KONTROL, atau KALIBRASI, flag `irNyala[]` direset tetapi PWM **tidak** diubah (kecuali `off` dan `manual_ir`, yang mematikan semua zona).

---

## 11. Data yang Persisten vs Tidak

| Data | Persisten? |
|---|---|
| Titik kalibrasi (→ GAIN/OFFSET) | Ya (NVS `kalbh`) |
| `pwmMaks`, threshold, `KP`, step | **Tidak**, kembali ke default saat reboot |
| `irTimerMs`, `irPwmOn[]`, `irIkutMaks[]` | **Tidak** |
| Mode | **Tidak**, selalu mulai dari MANUAL |
| PWM zona | **Tidak**, semua 0 saat boot |

---

## 12. Catatan Teknis & Potensi Masalah

Hal-hal yang ditemukan saat membaca kode, perlu diperhatikan untuk pengembangan berikutnya:

1. *(Sudah ditangani di kode, Task 01; wiring perlu dipindahkan.)* IR dipindah dari pin strapping, lihat §2.3.
2. *(Sudah ditangani, Task 02.)* `kp 0` kini membekukan PWM, lihat §7. Selama `KP > 0`, selisih kecil tetap dikoreksi minimal `STEP_MIN`.
3. *(Sudah ditangani, Task 03.)* PWM IR mengikuti `maks` kecuali diset manual, lihat §9.
4. **Kalibrasi tunggal untuk semua zona.** Variasi antar sensor BH1750 (posisi, toleransi, difuser) tidak terkompensasi. Kalibrasi per zona bisa menjadi pengembangan berikutnya.
5. **Ambil titik kalibrasi bersifat blocking (~1 detik).** Selama itu serial dan loop lain tidak diproses (tidak masalah di mode KALIBRASI, tetapi perlu diingat bila logika lain ditambahkan).
6. **Pergantian bus `Wire1`** (`end()`/`begin()`) terjadi minimal 2× per siklus kontrol (zona 0–7 di TCA1, 8–9 di TCA2). Waktu siklus baca 10 sensor perlu dijaga tetap < 500 ms.
7. **Channel TCA tidak pernah di-deselect.** Aman untuk konfigurasi sekarang (satu sensor per channel, alamat sama), tetapi bila ada perangkat lain di `Wire1` perlu diperhatikan.
8. *(Sudah ditangani, Task 05.)* Sensor yang gagal dicoba ulang otomatis, lihat §6.1.
9. **Parsing angka sederhana.** Nilai negatif dan notasi ilmiah ditolak; baris > 64 karakter dipotong diam-diam.
10. *(Sudah ditangani, Task 06.)* Sisa komentar uji sudah dibersihkan.

---

## 13. Ide Pengembangan Berikutnya

- Simpan parameter (`maks`, threshold, `KP`, step, timer, `irPwmOn`) ke NVS.
- Kalibrasi per zona (array GAIN/OFFSET per sensor).
- Gabungkan mode KONTROL dan IR (kontrol lux hanya aktif saat ada orang).
- Antarmuka web/MQTT (library ESPAsyncWebServer sudah terpasang di sistem).
- Memecah `sketch_oct28c.ino` menjadi beberapa file (`pwm`, `sensor`, `kalibrasi`, `kontrol`, `ir`, `serial`) agar lebih mudah dirawat.
