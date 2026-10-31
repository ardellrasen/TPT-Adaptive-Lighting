# Task 01 — Pindahkan sensor IR dari pin strapping ESP32

- **Prioritas:** Tinggi (mempengaruhi boot dan upload)
- **Status:** Belum dikerjakan
- **Area kode:** `sketch_oct28c.ino` → `IR_PIN[]` (baris 28), `setup()` (`pinMode`)
- **Referensi:** `SPESIFIKASI.md` §2.3, §12 poin 1

## Latar Belakang

Empat sensor IR terhubung ke pin strapping ESP32:

| Zona | GPIO | Fungsi strapping | Risiko |
|---|---|---|---|
| 0 | 15 | MTDO, mengatur log boot / timing SDIO | Log boot bisa hilang, timing salah |
| 1 | 2 | Harus LOW/floating untuk masuk download | Upload bisa gagal |
| 2 | 0 | LOW saat reset = masuk mode download | **ESP32 tidak menjalankan program** jika IR mendeteksi saat boot |
| 6 | 5 | Timing SDIO slave | Risiko kecil, tapi sebaiknya dihindari |

Output HW-201 aktif LOW (terdeteksi = LOW), jadi jika ada benda di depan sensor zona 2 saat ESP32 di-reset, board akan masuk mode bootloader.

## Tujuan

ESP32 selalu boot normal dan upload lancar, apa pun kondisi sensor IR.

## Langkah

1. Daftar GPIO yang masih bebas di DevKit. Yang sudah dipakai: 22, 23 (PCA9685), 32, 33, 25, 26 (bus sensor), dan IR (15, 2, 0, 4, 16, 17, 5, 18, 19, 21).
2. Kandidat pin pengganti: **GPIO 34, 35, 36 (VP), 39 (VN)** (input-only, aman, **tidak punya pull-up internal**), GPIO 13, 14, 27.
   - Hindari GPIO 6–11 (flash), 12 (strapping tegangan flash), 1/3 (UART0).
3. Jika memakai GPIO 34–39, pasang **resistor pull-up eksternal 10 kΩ ke 3,3 V** (atau pastikan output HW-201 push-pull/sudah punya pull-up di modul).
4. Rewiring perangkat keras untuk zona 0, 1, 2, 6.
5. Update `IR_PIN[]` dan tabel di `SPESIFIKASI.md` §2.3.
6. `pinMode(..., INPUT_PULLUP)` tetap boleh untuk pin yang punya pull-up internal; untuk 34–39 cukup `INPUT`.

## Kriteria Selesai

- [ ] Tidak ada IR yang memakai GPIO 0, 2, 5, 12, 15.
- [ ] ESP32 boot normal saat semua sensor IR dalam kondisi terdeteksi (ditutup tangan) ketika reset.
- [ ] Upload via `arduino-cli upload` berhasil tanpa menekan tombol BOOT dengan sensor terpasang.
- [ ] Perintah `ir` menampilkan status yang benar untuk semua zona.
- [ ] Compile sukses, `SPESIFIKASI.md` diperbarui.
