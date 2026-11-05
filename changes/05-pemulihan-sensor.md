# 05 — Pemulihan sensor BH1750

- **Task:** [05](../tasks/05-sensor-gagal-tidak-dicoba-ulang.md)
- **Perlu ubah wiring:** Tidak
- **Status:** Compile OK, belum diuji di hardware

## Apa yang berubah

Sebelumnya sensor yang gagal saat boot terus `ERR` sampai ESP32 di-reset. Sekarang:

| Kejadian | Perilaku |
|---|---|
| Sensor gagal dibaca 3 kali beruntun | Ditandai gagal, tercetak `[SENSOR] Zona <z> hilang` |
| Sensor ditandai gagal | Dicoba init ulang di semua mode, maksimal sekali per 5 detik per zona |
| Init ulang berhasil | Tercetak `[SENSOR] Zona <z> pulih`, bacaan normal kembali sekitar 200 ms kemudian |
| Perintah `init` atau `scan` | Semua sensor diinisialisasi ulang dan hasil per zona dicetak |

Di `sketch_oct28c.ino`:

- `bacaSensorMentah()` menghitung kegagalan beruntun di `gagalBeruntun[]`.
- Fungsi baru `pulihkanSensor()` dipanggil dari `loop()`. Fungsi ini mengecek alamat sensor dulu, supaya library tidak mencetak error ke Serial setiap 5 detik saat sensor belum terpasang.
- Parameternya ada di `SENSOR_GAGAL_MAKS`, `SENSOR_RETRY_MS`, dan `SENSOR_SIAP_MS`.

**Keputusan perilaku:** selama sensor error di mode KONTROL, PWM zona itu **ditahan** di nilai terakhir, supaya lampu tidak tiba-tiba mati atau berubah saat kabel sensor bermasalah. Zona lain tetap berjalan normal.

## Yang perlu dilakukan pada wiring

Tidak ada. Sensor yang baru dipasang atau kabel yang diperbaiki kini terbaca lagi tanpa reset.

## Yang perlu diperhatikan

- **Di MANUAL dan MANUAL_IR, sensor tidak dibaca otomatis:** sensor yang tercabut baru terdeteksi setelah `baca` gagal 3 kali. Sensor yang sudah gagal saat boot tetap dicoba ulang otomatis di semua mode.
- **Putus daya sebentar bisa tidak terdeteksi:** kalau sensor kehilangan daya tanpa 3 kali gagal baca beruntun, sensor tidak di-init ulang dan bacaannya bisa salah. Pakai `init` kalau bacaannya terlihat aneh.
- **`ERR` sesaat setelah pulih itu normal:** BH1750 butuh waktu untuk pengukuran pertamanya, sehingga bacaan selama 200 ms pertama dianggap belum siap.
- **`init` berhenti sebentar:** perintah ini blocking sekitar 400 ms bila semua sensor merespons, dan mengosongkan bacaan lux. Di mode KONTROL, zona tampil `ERR` sampai langkah kontrol berikutnya. Sensor yang tidak merespons akan memunculkan pesan error dari library, dan itu wajar.
- **Lampu tidak diturunkan saat sensor error:** kalau zona sedang terang saat sensornya putus, zona itu tetap di PWM tersebut sampai sensornya pulih.
- **Kabel I2C yang goyang bisa mengganggu zona lain:** zona 0–7 berbagi bus TCA1, dan zona 8–9 berbagi bus TCA2. Kalau beberapa zona `ERR` bersamaan, periksa kabel bus atau TCA-nya dulu, bukan setiap sensornya.
- **TCA juga harus merespons:** kalau TCA yang tidak merespons, semua zona di TCA itu tetap `ERR`. Zona-zona tersebut pulih otomatis setelah TCA kembali merespons.

## Cara menguji

1. **Boot tanpa sensor:** cabut satu BH1750, lalu nyalakan ESP32. Zona itu tertulis `BH1750 GAGAL`. Pasang sensornya. Dalam 10 detik harus muncul `[SENSOR] Zona <z> pulih`, dan `baca` harus menampilkan nilai normal.
2. **Cabut saat KONTROL:** ketik `kontrol`, lalu cabut satu sensor. Zona itu harus `ERR`, PWM-nya tetap, zona lain tetap berjalan, dan log `hilang` muncul. Pasang lagi, lalu log `pulih` harus muncul dan kontrol berjalan kembali.
3. **Perintah manual:** ketik `init` dan `scan`. Keduanya harus mencetak status semua zona. Keduanya juga tercantum di `help`.
4. **Waktu loop:** selama satu sensor tercabut, monitor KONTROL harus tetap tercetak sekitar sekali per detik.
