# Task 02 — `kp 0` seharusnya tidak menggerakkan PWM

- **Prioritas:** Sedang
- **Status:** Belum dikerjakan
- **Area kode:** `langkahKontrol()` (baris 348–366), perintah `kp` dan `step` di `prosesPerintah()`
- **Referensi:** `SPESIFIKASI.md` §7, §12 poin 2

## Latar Belakang

Di `langkahKontrol()`:

```cpp
delta = constrain((int)((thBawah - lux) * KP), STEP_MIN, STEP_MAX);
```

Karena batas bawah `constrain` adalah `STEP_MIN` (≥ 1), hasilnya **minimal 1** walaupun `KP = 0`. Akibatnya:

- `kp 0` tidak bisa dipakai untuk "membekukan" kontrol.
- Untuk selisih lux kecil (mis. `KP * selisih < 1`), PWM tetap naik/turun `STEP_MIN`, sehingga di kasus tertentu PWM bisa berosilasi di sekitar tepi band.

## Tujuan

Perilaku `KP` jelas dan bisa diprediksi.

## Opsi Solusi (pilih salah satu)

**A. `KP = 0` menonaktifkan koreksi** (perubahan minimal)
```cpp
if (KP <= 0) continue;   // atau delta = 0
```

**B. `STEP_MIN` hanya diterapkan bila hasil proporsional > 0**
```cpp
int p = (int)((thBawah - lux) * KP);
delta = (p <= 0) ? 0 : constrain(p, STEP_MIN, STEP_MAX);
```
Catatan: dengan opsi B, selisih kecil tidak dikoreksi sama sekali, sehingga band efektif melebar. Perlu diuji.

**C. Tetap seperti sekarang** tetapi dokumentasikan di `help` bahwa `KP` hanya mempengaruhi besar langkah di atas `STEP_MIN`.

Rekomendasi: **A** (paling sederhana dan sesuai ekspektasi pengguna).

## Langkah

1. Terapkan opsi yang dipilih di kedua cabang (`lux < thBawah` dan `lux > thAtas`).
2. Sesuaikan teks `cetakBantuan()` bila perlu.
3. Update `SPESIFIKASI.md` §7.

## Kriteria Selesai

- [ ] Dengan `kp 0` di mode KONTROL, PWM semua zona tidak berubah (cek output monitor beberapa detik).
- [ ] Dengan `kp 0.05` perilaku kontrol sama seperti sebelumnya (lux masuk ke band 300–400 lx).
- [ ] Compile sukses, spesifikasi diperbarui.
