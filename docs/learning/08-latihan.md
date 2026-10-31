# Bab 8 — Latihan

Coba jawab dulu sebelum membuka kunci jawaban. Soal bertanda 🔧 butuh hardware.

## A. Pemahaman Dasar

**A1.** Zona 9 dibaca lewat TCA yang mana, dan pintu (channel) nomor berapa?

<details><summary>Jawaban</summary>

`SENSOR_TCA[9] = 1` → **TCA2** (GPIO 25/26), `SENSOR_CH[9] = 1` → **pintu 1**.
</details>

**A2.** Kenapa sistem butuh TCA9548A? Kenapa tidak semua BH1750 langsung disambung ke satu bus I2C?

<details><summary>Jawaban</summary>

Karena semua BH1750 punya alamat yang sama (`0x23`). Kalau disambung langsung, semuanya menjawab bersamaan. TCA9548A membuka satu "pintu" saja sehingga hanya satu sensor yang terdengar.
</details>

**A3.** `pwmMaks = 100`. Kamu mengetik `all 3000` di mode MANUAL. Berapa PWM setiap zona, dan berapa persen terangnya?

<details><summary>Jawaban</summary>

**100**, karena dipotong limiter. 100 / 4095 × 100% ≈ **2,4%**. Serial juga mencetak "Nilai melebihi PWM maksimum, dibatasi ke 100".
</details>

**A4.** Apa yang terjadi kalau `loop()` memakai `delay(500)` alih-alih `millis()` di mode KONTROL?

<details><summary>Jawaban</summary>

Selama `delay`, ESP32 tidak membaca Serial, sehingga perintah terasa lambat direspons (bisa sampai 0,5 detik). Kalau nanti ada tugas lain di `loop()` (misalnya IR), tugas itu juga ikut tertunda.
</details>

## B. Sistem Kontrol

**B1.** Target 300–400 lux, `KP = 0.05`, step 1–5. Zona 2 terbaca **150 lux** dengan PWM 40. Berapa PWM setelah satu langkah kontrol?

<details><summary>Jawaban</summary>

Selisih = 300 − 150 = 150. 150 × 0,05 = 7,5 → dibulatkan 7 → dijepit 1–5 → **5**. PWM baru = 40 + 5 = **45**.
</details>

**B2.** Zona 5 terbaca **410 lux**, PWM 60. Berapa PWM setelahnya?

<details><summary>Jawaban</summary>

Selisih = 410 − 400 = 10. 10 × 0,05 = 0,5 → 0 → dijepit minimal 1 → delta **−1**. PWM baru = **59**.
</details>

**B3.** Zona 1 terbaca **350 lux**. Apa yang terjadi pada PWM-nya?

<details><summary>Jawaban</summary>

Tidak berubah, karena 350 berada di dalam rentang 300–400 (deadband).
</details>

**B4.** Target 300–400, `pwmMaks = 50`, dan dengan PWM 50 zona 7 hanya mencapai 220 lux. Apa yang terjadi? Bagaimana memperbaikinya?

<details><summary>Jawaban</summary>

Kontrol terus ingin menaikkan PWM, tapi `constrain(..., 0, pwmMaks)` membuatnya mentok di 50. Lux tetap 220. Solusi: naikkan `maks` (misalnya `maks 200`), atau turunkan target (`th 200 300`).
</details>

**B5.** Kenapa target dibuat rentang 300–400, bukan tepat 350?

<details><summary>Jawaban</summary>

Agar ada **deadband**. Sensor selalu sedikit bergoyang. Dengan satu angka, lampu akan naik-turun terus. Dengan rentang, lampu diam selama lux masih di dalam rentang.
</details>

## C. Kalibrasi

**C1.** Hanya ada 1 titik kalibrasi: mentah 80, luxmeter 100. Berapa GAIN dan OFFSET? Kalau sensor membaca 200 mentah, berapa lux terkalibrasinya?

<details><summary>Jawaban</summary>

1 titik → perbandingan. GAIN = 100 / 80 = **1,25**, OFFSET = **0**. Lux = 1,25 × 200 = **250**.
</details>

**C2.** GAIN = 1,2, OFFSET = −10. Sensor membaca mentah 5. Berapa lux yang dipakai sistem?

<details><summary>Jawaban</summary>

1,2 × 5 − 10 = −4. Karena negatif, `bacaSensor()` mengembalikan **0** (`(lux < 0) ? 0 : lux`).
</details>

**C3.** Kenapa saat kalibrasi sebaiknya titik diambil dari gelap sampai terang, bukan semuanya di sekitar 300 lux?

<details><summary>Jawaban</summary>

Titik yang berdekatan membuat garis "goyang": sedikit kesalahan pengukuran membuat kemiringan (GAIN) berubah jauh. Titik yang tersebar membuat garis lebih stabil dan akurat di seluruh rentang. Kalau titik-titiknya terlalu berdekatan, kode bahkan jatuh ke mode "perbandingan" (seperti 1 titik).
</details>

**C4.** ESP32 dicabut listriknya lalu dinyalakan lagi. Mana yang masih ada: titik kalibrasi, `pwmMaks`, threshold, timer IR?

<details><summary>Jawaban</summary>

Hanya **titik kalibrasi** (disimpan di flash lewat Preferences). Lainnya kembali ke default: `pwmMaks` 100, threshold 300–400, timer 10 detik.
</details>

## D. Mode IR

**D1.** Timer 10 detik. Orang terdeteksi di detik 0, 3, dan 6, lalu pergi. Kapan lampu mati?

<details><summary>Jawaban</summary>

Di detik **16** (deteksi terakhir detik 6 + 10 detik). Tepatnya sedikit lebih dari 16, karena sensor biasanya masih terdeteksi beberapa saat setelah detik 6.
</details>

**D2.** Di mode MANUAL_IR kamu mengetik `4 80`, tapi lampu zona 4 tidak menyala. Kenapa?

<details><summary>Jawaban</summary>

Di mode MANUAL_IR, `4 80` hanya mengatur **PWM yang dipakai saat terdeteksi**. Lampu baru menyala ketika sensor IR zona 4 mendeteksi sesuatu.
</details>

**D3.** Kenapa `prosesIR()` butuh variabel `irNyala[z]`? Apa yang terjadi tanpa variabel itu?

<details><summary>Jawaban</summary>

Tanpa itu, selama orang berdiri di depan sensor, kode akan memanggil `setChannel()` dan mencetak "[IR] ... lampu ON" **ribuan kali per detik**, sehingga Serial banjir pesan dan bus I2C sibuk tanpa guna.
</details>

## E. 🔧 Praktik dengan Hardware

1. **Lihat boot log.** Pastikan kedua TCA "terdeteksi" dan semua zona "BH1750 OK".
2. **Uji limiter.** `maks 50`, lalu `all 200`. Cek dengan `status` bahwa semua zona PWM 50.
3. **Uji kontrol.** `maks 500`, `th 200 300`, `kontrol`. Tutup sensor zona 0 dengan tangan → lihat PWM zona 0 di monitor naik. Lepaskan → PWM turun lagi.
4. **Uji IR.** `manual_ir`, `timer 3`. Lambaikan tangan di depan sensor zona 0 → lampu menyala, mati 3 detik setelah tangan pergi.
5. **Kalibrasi kecil.** Dengan luxmeter, ambil 3 titik di zona 0, lalu lihat `kalinfo`. Apakah R² di atas 0,95?

## F. Tantangan Coding

Coba ubah kode (di branch terpisah!) untuk:

1. **Mudah:** Tambah perintah `versi` yang mencetak versi firmware.
2. **Sedang:** Simpan `pwmMaks` ke Preferences supaya tidak hilang saat reboot. *Petunjuk:* lihat cara `simpanKalibrasi()` memakai `prefs.putUChar()`; untuk `uint16_t` gunakan `prefs.putUShort()`.
3. **Sedang:** Di `cetakMonitor()`, tampilkan tanda `*` di samping zona yang lux-nya sudah di dalam target.
4. **Menantang:** Kerjakan salah satu task di folder [`tasks/`](../../tasks/README.md).

Jangan lupa compile setelah mengubah kode:

```
arduino-cli compile --fqbn esp32:esp32:esp32 .
```

---

Selamat, kamu sudah menyelesaikan semua bab! 🎉 Untuk referensi cepat, gunakan [SPESIFIKASI.md](../../SPESIFIKASI.md).
