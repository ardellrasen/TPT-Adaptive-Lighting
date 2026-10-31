# Task 06 — Bersihkan sisa komentar "test edit"

- **Prioritas:** Rendah (kerapian kode)
- **Status:** Belum dikerjakan
- **Area kode:** `sketch_oct28c.ino` baris 7, 25, 26

## Latar Belakang

Ada sisa komentar uji coba di header bagian kode:

```cpp
//  BUS I2C PCA9685 (Wire, SDA 22 / SCL 23) -- test edit
//  SENSOR IR HW-201 (OUT LOW = ada benda / terdeteksi) -- test edit lagi ah
//  IR zona z mengatur lampu CH z -- test edit lagi ah lagi
```

## Langkah

1. Hapus akhiran `-- test edit`, `-- test edit lagi ah`, dan `-- test edit lagi ah lagi`.
2. Cari sisa lain: `grep -n "test edit" sketch_oct28c.ino`.

## Kriteria Selesai

- [ ] Tidak ada lagi teks `test edit` di kode.
- [ ] Compile sukses.
