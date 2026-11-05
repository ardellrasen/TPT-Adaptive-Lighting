# Task 03 — PWM IR tidak ikut berubah saat `maks` diubah

- **Prioritas:** Sedang
- **Status:** Kode selesai (opsi A), perlu uji hardware
- **Area kode:** `setup()` (baris 888), perintah `maks` di `prosesPerintah()` (baris 639–665), `cetakStatus()`, `cetakIR()`
- **Referensi:** `SPESIFIKASI.md` §9, §12 poin 3

## Latar Belakang

`irPwmOn[z]` diisi **sekali** di `setup()`:

```cpp
irPwmOn[z] = pwmMaks;   // = 100
```

- Jika pengguna `maks 2000`, lampu di mode MANUAL_IR tetap menyala dengan PWM 100. Pengguna harus menebak bahwa `all <nilai>` perlu diketik di mode MANUAL_IR.
- Jika `maks` diturunkan, nilai di `irPwmOn[]` tetap besar; saat menyala dipotong `setChannel()`, jadi aman, tetapi tampilan bisa membingungkan.

## Tujuan

Hubungan antara `maks` dan PWM IR jelas bagi pengguna.

## Opsi Solusi

**A. Tandai "ikut maks"** — pakai nilai khusus (mis. `irPwmOn[z] = 0xFFFF` / flag `irIkutMaks[z]`) yang berarti "pakai `pwmMaks`". Saat pengguna set nilai eksplisit, flag dimatikan. Saat menyalakan: `setChannel(z, ikutMaks ? pwmMaks : irPwmOn[z])`.

**B. Saat `maks` diubah, set ulang `irPwmOn[]`** yang nilainya sama dengan `pwmMaks` lama ke `pwmMaks` baru.

**C. Hanya informasi** — setelah `maks` diubah, cetak peringatan bila `irPwmOn[z] != pwmMaks`.

Rekomendasi: **A**, karena perilakunya jelas dan nilai yang di-set manual oleh pengguna tidak tertimpa.

## Langkah

1. Implementasi opsi terpilih.
2. Jika zona sedang menyala di MANUAL_IR saat `maks` diubah, pastikan PWM aktifnya ikut diperbarui.
3. Tampilkan di `status` / `ir` apakah PWM IR mengikuti maks atau nilai manual (mis. `PWM IR 100 (maks)`).
4. Pertimbangkan digabung dengan Task 05/penyimpanan parameter ke NVS bila dikerjakan bersamaan.

## Kriteria Selesai

- [ ] Boot → `maks 500` → `manual_ir` → IR terdeteksi → lampu menyala PWM 500.
- [ ] `3 80` di MANUAL_IR → `maks 500` → zona 3 tetap PWM 80, zona lain 500.
- [ ] `maks 50` → semua zona menyala maksimal 50.
- [x] Compile sukses, spesifikasi diperbarui.
