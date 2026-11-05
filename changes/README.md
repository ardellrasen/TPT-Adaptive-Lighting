# Catatan Perubahan

Satu dokumen untuk setiap perubahan yang berasal dari [`tasks/`](../tasks/README.md). Nomor dokumen mengikuti nomor task. Task 04 (kalibrasi per zona) belum dikerjakan, jadi tidak ada dokumennya.

| No | Perubahan | Perlu ubah wiring? | Status pengujian |
|---|---|---|---|
| 01 | [Sensor IR pindah dari pin strapping](01-pindah-pin-ir.md) | **Ya** | Compile OK, belum diuji di hardware |
| 02 | [`kp 0` membekukan PWM](02-kp-nol-membekukan-pwm.md) | Tidak | Compile OK, belum diuji di hardware |
| 03 | [PWM IR mengikuti `maks`](03-pwm-ir-ikut-maks.md) | Tidak | Compile OK, belum diuji di hardware |
| 05 | [Pemulihan sensor BH1750](05-pemulihan-sensor.md) | Tidak | Compile OK, belum diuji di hardware |
| 06 | [Bersihkan komentar "test edit"](06-bersihkan-komentar.md) | Tidak | Selesai |

## Urutan yang disarankan

1. Matikan daya, lalu kerjakan wiring di [dokumen 01](01-pindah-pin-ir.md).
2. Nyalakan dan upload firmware dengan `arduino-cli upload -p COM5 --fqbn esp32:esp32:esp32 .`.
3. Jalankan bagian **Cara menguji** di dokumen 01, 02, 03, dan 05.

**Jangan upload firmware baru sebelum kabel IR dipindah.** Firmware baru membaca pin baru, jadi zona 0, 1, 2, dan 6 tidak akan membaca sensornya sampai kabelnya dipindah.
