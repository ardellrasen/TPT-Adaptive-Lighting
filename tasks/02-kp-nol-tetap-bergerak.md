# Task 02 — `kp 0` seharusnya tidak menggerakkan PWM

- **Prioritas:** Sedang
- **Status:** Kode selesai (opsi A), perlu uji hardware
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

## Asal Nilai `KP = 0.05`

**Sumber resminya tidak tercatat.** Nilai ini sudah ada sejak commit pertama (`c70b68c init`, 2026-10-31) dan tidak pernah berubah. Di repo tidak ada perhitungan, data pengukuran, atau catatan tuning yang menjelaskannya, dan `SPESIFIKASI.md` §7 hanya menyebutnya sebagai nilai default. Jadi `0.05` belum bisa dianggap hasil tuning pada lampu dan ruangan yang sebenarnya.

Yang bisa dilakukan hanya menafsirkan nilainya. Ini **analisis, bukan catatan asal-usul**:

```
KP = STEP_MAX / lebar band = 5 PWM / (400 - 300) lux = 0.05 PWM/lux
```

Dengan kata lain, langkah mencapai maksimum saat selisih lux dari threshold sama dengan lebar band (100 lux). Angkanya cocok, tetapi tidak ada bukti bahwa nilainya memang diturunkan dengan cara ini. Kalau `th` atau `step` diubah lewat Serial, `KP` tidak ikut menyesuaikan.

| Selisih dari threshold | `(int)(selisih * KP)` | Delta PWM setelah `constrain` 1–5 |
|---|---|---|
| < 20 lux | 0 | 1 (kena `STEP_MIN`) |
| 20–39 lux | 1 | 1 |
| 40–59 lux | 2 | 2 |
| 60–79 lux | 3 | 3 |
| 80–99 lux | 4 | 4 |
| ≥ 100 lux | 5 atau lebih | 5 (kena `STEP_MAX`) |

### Cara memvalidasi dengan hardware

Nilai `3 lux/PWM` di `docs/learning/05-sistem-kontrol.md` §5.4 hanya contoh simulasi, bukan hasil ukur. Untuk memastikan `KP`:

1. Di mode MANUAL, ukur lux pada dua nilai PWM dengan cahaya luar yang tetap, lalu hitung `G = (lux2 - lux1) / (PWM2 - PWM1)` dalam lux/PWM. Ulangi di beberapa zona.
2. Pastikan `KP * G < 1` supaya satu langkah proporsional tidak melewati targetnya. Dengan `KP = 0.05`, syarat ini berarti `G < 20 lux/PWM`.
3. Coba di mode KONTROL. Kalau lux bolak-balik melewati band, turunkan `KP` atau `STEP_MAX`. Kalau terlalu lambat mencapai band, naikkan.
4. Catat hasil ukur dan nilai akhirnya di `SPESIFIKASI.md` §7.

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
- [x] Compile sukses, spesifikasi diperbarui.
