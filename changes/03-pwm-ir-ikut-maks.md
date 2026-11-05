# 03 — PWM IR mengikuti `maks`

- **Task:** [03](../tasks/03-pwm-ir-tidak-ikut-maks.md) (opsi A)
- **Perlu ubah wiring:** Tidak
- **Status:** Compile OK, belum diuji di hardware

## Apa yang berubah

Sebelumnya PWM mode MANUAL_IR hanya diisi sekali saat boot (100), jadi `maks 500` tidak membuat lampu IR lebih terang.

Sekarang setiap zona punya flag `irIkutMaks[z]`:

| Kondisi zona | PWM saat terdeteksi | Tampilan di `status` / `ir` |
|---|---|---|
| Belum diset (default) | Mengikuti `pwmMaks` | `100 (maks)` |
| Sudah diset dengan `<zona> <nilai>` atau `all <nilai>` | `irPwmOn[z]`, dibatasi `pwmMaks` | `80 (manual)` |

Di `sketch_oct28c.ino`:

- Fungsi baru `pwmIrZona(z)` menghitung PWM saat terdeteksi, dan `cetakPwmIr(z)` menampilkannya.
- Perintah `maks` langsung memperbarui zona yang sedang menyala di MANUAL_IR.
- `<zona> <nilai>` dan `all <nilai>` di MANUAL_IR mematikan `irIkutMaks[z]`.

## Yang perlu dilakukan pada wiring

Tidak ada.

## Yang perlu diperhatikan

- **Lampu IR bisa jadi jauh lebih terang:** menaikkan `maks` kini langsung menaikkan zona default, termasuk yang sedang menyala. Contohnya, `maks 4095` membuat lampu menyala penuh. Pastikan driver dan LED COB aman di nilai itu sebelum menaikkannya.
- **Belum ada perintah untuk kembali ke `(maks)`:** zona yang sudah diset manual baru mengikuti `maks` lagi setelah reboot.
- **`all <nilai>` mengubah semua zona menjadi manual:** setelah itu, tidak ada zona yang mengikuti `maks` lagi.
- **Nilai manual dipotong saat diketik:** dengan `maks 100`, perintah `3 500` menyimpan **100**. Kalau `maks` dinaikkan ke 1000, zona 3 tetap 100. Naikkan `maks` lebih dulu, baru set nilainya.
- **Batas `maks` tidak menimpa nilai manual:** `3 80` lalu `maks 50` menampilkan `50 (manual)`. Setelah `maks 500`, zona 3 kembali ke 80.
- **Pengaturan ini tidak tersimpan:** `irPwmOn[]` dan `irIkutMaks[]` kembali ke default setelah reboot.

## Cara menguji

1. Setelah boot, ketik `maks 500`, lalu `manual_ir`. Saat sensor IR mendeteksi, lampu harus menyala dengan PWM 500.
2. Reboot, lalu ketik `maks 100`, `manual_ir`, `3 80`, dan `maks 500`. Di `ir`, zona 3 harus `80 (manual)` dan zona lain `500 (maks)`.
3. Ketik `maks 50`. Semua zona harus menyala maksimal PWM 50.
4. Biarkan satu zona menyala, lalu ubah `maks`. PWM zona itu harus langsung berubah. Cek dengan `status`.
