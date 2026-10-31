# Task 04 — Kalibrasi BH1750 per zona

- **Prioritas:** Sedang (mempengaruhi akurasi kontrol)
- **Status:** Belum dikerjakan
- **Area kode:** bagian KALIBRASI (baris 62–76, 204–343), `bacaSensor()`, perintah mode KALIBRASI di `prosesPerintah()`
- **Referensi:** `SPESIFIKASI.md` §8, §12 poin 4

## Latar Belakang

Saat ini kalibrasi hanya diambil dari zona 0 (`KAL_ZONA = 0`), lalu `kalGain`/`kalOffset` yang sama dipakai untuk **semua** zona. Perbedaan antar sensor (toleransi BH1750, posisi pemasangan, difuser/penutup, pantulan) tidak terkoreksi, sehingga zona lain bisa dikontrol ke lux yang sebenarnya berbeda dari target.

## Tujuan

Setiap zona punya koefisien kalibrasi sendiri, dengan fallback ke kalibrasi global bila zona belum dikalibrasi.

## Desain Usulan

- Data per zona: `kalN[NUM_CH]`, `kalRaw[NUM_CH][KAL_MAKS]`, `kalRef[NUM_CH][KAL_MAKS]`, `kalGain[NUM_CH]`, `kalOffset[NUM_CH]`, `kalR2[NUM_CH]`.
- Variabel `kalZona` (zona yang sedang dikalibrasi), menggantikan konstanta `KAL_ZONA`.
- Perintah baru di mode KALIBRASI: `zona <z>` untuk memilih zona acuan; `kalinfo` menampilkan semua zona atau `kalinfo <z>`.
- `bacaSensor(z)` memakai koefisien zona `z`; jika `kalN[z] == 0`, pakai koefisien zona 0 (perilaku lama).
- Opsional: perintah `salin <dari> <ke>` / `salin all` untuk menyalin kalibrasi.

### Penyimpanan NVS

- Key per zona: `n0`..`n9`, `raw0`..`raw9`, `ref0`..`ref9` (nama key NVS maks 15 karakter).
- **Migrasi:** jika key lama `n`/`raw`/`ref` ada, pindahkan ke zona 0 lalu hapus key lama.

## Langkah

1. Refactor fungsi kalibrasi agar menerima parameter zona (`hitungKalibrasi(z)`, `simpanKalibrasi(z)`, dst.).
2. Implementasi migrasi data lama.
3. Tambah perintah pemilihan zona dan perbarui `cetakBantuanKalibrasi()`.
4. Perbarui `cetakStatus()` untuk menampilkan GAIN/OFFSET per zona (atau ringkas).
5. Perhatikan RAM: 10 × 10 × 2 float = 800 byte, aman.

## Kriteria Selesai

- [ ] Kalibrasi tersimpan sebelum update tetap terbaca sebagai kalibrasi zona 0 setelah update.
- [ ] Bisa mengkalibrasi zona 3 tanpa mengubah koefisien zona lain.
- [ ] Zona yang belum dikalibrasi memakai koefisien zona 0.
- [ ] Koefisien per zona tetap ada setelah reboot.
- [ ] Compile sukses, `SPESIFIKASI.md` §8 diperbarui.
