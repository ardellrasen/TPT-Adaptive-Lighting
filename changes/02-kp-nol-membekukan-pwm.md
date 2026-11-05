# 02 — `kp 0` membekukan PWM

- **Task:** [02](../tasks/02-kp-nol-tetap-bergerak.md) (opsi A)
- **Perlu ubah wiring:** Tidak
- **Status:** Compile OK, belum diuji di hardware

## Apa yang berubah

Sebelumnya `kp 0` tetap menggerakkan PWM, karena `constrain` membuat langkahnya minimal `STEP_MIN` (≥ 1) selama lux di luar rentang target.

Sekarang `langkahKontrol()` melewati koreksi saat `KP` bernilai 0:

```cpp
if (KP <= 0) continue;   // kp 0 -> koreksi dibekukan, lux tetap dimonitor
```

Teks `help` untuk perintah `kp` juga menyebut perilaku ini.

## Yang perlu dilakukan pada wiring

Tidak ada.

## Yang perlu diperhatikan

- **PWM dibekukan, bukan dimatikan:** lampu tetap menyala di nilai terakhir. Pakai `off` untuk mematikan.
- **Lux tetap dibaca:** monitor `[KONTROL ...]` tetap berjalan dan menampilkan lux terbaru.
- **Hanya tepat 0 yang membekukan:** nilai kecil seperti `kp 0.001` tetap menggerakkan PWM sebesar `STEP_MIN` selama lux di luar rentang target.
- **Tidak ada perubahan untuk `KP` lebih dari 0:** dengan `kp 0.05`, perhitungan langkahnya sama seperti sebelumnya.
- **`kp` tidak tersimpan:** nilainya kembali ke 0.05 setelah reboot.
- **`KP = 0.05` belum tervalidasi:** asal nilainya tidak tercatat. Cara mengujinya ada di [Task 02](../tasks/02-kp-nol-tetap-bergerak.md), bagian "Cara memvalidasi dengan hardware".

## Cara menguji

1. Ketik `kontrol`, tunggu beberapa detik, lalu ketik `kp 0`.
2. Tutup atau sinari sensor supaya lux keluar dari rentang 300–400 lx. Di monitor, lux harus berubah dan PWM semua zona harus tetap.
3. Ketik `kp 0.05`. PWM harus kembali bergerak sampai lux masuk ke rentang target.
