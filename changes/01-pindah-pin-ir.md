# 01 — Sensor IR pindah dari pin strapping

- **Task:** [01](../tasks/01-pin-strapping-ir.md)
- **Perlu ubah wiring:** **Ya**
- **Status:** Compile OK, belum diuji di hardware

## Apa yang berubah

Empat sensor IR sebelumnya terhubung ke pin strapping ESP32, yaitu pin yang ikut menentukan mode saat boot. Output HW-201 bernilai LOW saat mendeteksi benda, jadi benda di depan sensor zona 2 (GPIO 0) saat reset bisa membuat ESP32 masuk mode download dan program tidak berjalan.

| Zona | GPIO lama | GPIO baru | `pinMode` |
|---|---|---|---|
| 0 | 15 | **13** | `INPUT_PULLUP` |
| 1 | 2 | **14** | `INPUT_PULLUP` |
| 2 | 0 | **27** | `INPUT_PULLUP` |
| 6 | 5 | **34** | `INPUT` |

Zona 3, 4, 5, 7, 8, dan 9 tidak berubah.

Di `sketch_oct28c.ino`:

- `IR_PIN[]` memakai pin baru.
- `setup()` memasang `INPUT` untuk GPIO 34 ke atas, karena pin tersebut tidak punya pull-up internal, dan `INPUT_PULLUP` untuk pin lainnya.

## Yang perlu dilakukan pada wiring

Lakukan dengan **daya dimatikan** (cabut USB dan catu daya).

1. Pindahkan kabel OUT sensor IR:

   | Sensor | Cabut dari | Pasang ke |
   |---|---|---|
   | IR zona 0 | GPIO 15 | **GPIO 13** |
   | IR zona 1 | GPIO 2 | **GPIO 14** |
   | IR zona 2 | GPIO 0 | **GPIO 27** |
   | IR zona 6 | GPIO 5 | **GPIO 34** |

2. Pasang **resistor pull-up 10 kΩ** antara GPIO 34 dan 3,3 V:

   ```
   3,3 V ----[ 10 kΩ ]----+---- GPIO 34
                          |
                          +---- OUT sensor IR zona 6
   ```

3. Kabel VCC dan GND semua sensor tidak perlu diubah.
4. Pastikan tidak ada kabel yang masih menempel di GPIO 0, 2, 5, 12, dan 15.

## Yang perlu diperhatikan

- **Tegangan output modul:** GPIO ESP32 hanya tahan 3,3 V. Kalau HW-201 diberi 5 V, outputnya bisa ikut 5 V. Ukur dulu tegangan pin OUT dengan multimeter. Kalau di atas 3,3 V, beri modul daya 3,3 V atau pasang pembagi tegangan. Ini berlaku untuk semua zona, bukan hanya yang dipindah.
- **Pull-up di GPIO 34 jangan dilewatkan:** tanpa resistor, pin mengambang saat kabel OUT lepas dan zona 6 bisa salah mendeteksi. Modul yang sudah punya pull-up tidak menolong kalau kabelnya sendiri yang lepas.
- **GPIO 34 hanya bisa input:** pin ini tidak bisa dipakai untuk output kalau nanti wiring diubah lagi.
- **GPIO 14 saat boot:** referensi pinout ESP32 yang umum menyebut pin ini bisa mengeluarkan sinyal sesaat saat boot. Pin ini tidak menentukan mode boot, dan sinyalnya tidak dipakai program. Kalau perilaku aneh muncul pada IR zona 1 sesaat setelah reset, mulailah memeriksa dari sini.
- **GPIO 13 dan 14 adalah pin JTAG:** tidak masalah kecuali Anda memakai debugger JTAG.

## Cara menguji

1. Tutup semua sensor IR dengan tangan, lalu tekan reset. ESP32 harus boot normal dan banner `=== Adaptive Lighting ...` muncul di Serial.
2. Upload dengan sensor tetap terpasang. Upload harus berhasil tanpa menekan tombol BOOT.
3. Ketik `ir`, lalu tutup sensor satu per satu. Pastikan setiap zona berubah menjadi `TERDETEKSI` dan menampilkan GPIO yang benar.
4. Lepas kabel OUT zona 6. Zona 6 harus terbaca `kosong`, bukan `TERDETEKSI`.
