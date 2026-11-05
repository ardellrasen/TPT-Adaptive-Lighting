# Daftar Task

Task hasil review kode (lihat `../SPESIFIKASI.md` §12).

| No | Task | Prioritas | Status |
|---|---|---|---|
| 01 | [Pindahkan sensor IR dari pin strapping ESP32](01-pin-strapping-ir.md) | Tinggi | Kode selesai, perlu rewiring dan uji hardware |
| 02 | [`kp 0` seharusnya tidak menggerakkan PWM](02-kp-nol-tetap-bergerak.md) | Sedang | Kode selesai, perlu uji hardware |
| 03 | [PWM IR tidak ikut berubah saat `maks` diubah](03-pwm-ir-tidak-ikut-maks.md) | Sedang | Kode selesai, perlu uji hardware |
| 04 | [Kalibrasi BH1750 per zona](04-kalibrasi-per-zona.md) | Sedang | Belum |
| 05 | [Sensor BH1750 yang gagal tidak pernah dicoba ulang](05-sensor-gagal-tidak-dicoba-ulang.md) | Sedang | Kode selesai, perlu uji hardware |
| 06 | [Bersihkan sisa komentar "test edit"](06-bersihkan-komentar-test.md) | Rendah | Selesai |

Setiap selesai mengerjakan task: compile (`arduino-cli compile --fqbn esp32:esp32:esp32 .`), perbarui `SPESIFIKASI.md` bila perilaku berubah, dan ubah status di tabel ini.
