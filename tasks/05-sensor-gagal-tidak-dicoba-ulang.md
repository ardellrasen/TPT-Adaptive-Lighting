# Task 05 — Sensor BH1750 yang gagal tidak pernah dicoba ulang

- **Prioritas:** Sedang
- **Status:** Belum dikerjakan
- **Area kode:** `initSensor()` (baris 176–202), `bacaSensorMentah()` (baris 149–153), `langkahKontrol()`
- **Referensi:** `SPESIFIKASI.md` §6, §12 poin 5

## Latar Belakang

`sensorOk[z]` hanya diisi di `initSensor()` saat boot. Jika sebuah BH1750:

- belum tersambung / kabel longgar saat boot, atau
- sempat terputus lalu tersambung lagi (BH1750 kembali ke mode power-down setelah kehilangan daya),

zona tersebut akan terus `ERR` (atau membaca nilai salah) sampai ESP32 di-reset. Di mode KONTROL, PWM zona yang `ERR` ditahan di nilai terakhir.

## Tujuan

Sensor yang bermasalah dipulihkan otomatis tanpa reset, dan pengguna bisa memicu inisialisasi ulang secara manual.

## Desain Usulan

1. **Perintah manual** `init` / `scan`: memanggil `initSensor()` ulang dan mencetak hasilnya.
2. **Retry otomatis:**
   - Tambah `uint8_t gagalBeruntun[NUM_CH]`.
   - Di `bacaSensorMentah()`: bila gagal, tambah counter; bila berhasil, reset ke 0.
   - Bila `sensorOk[z] == false` atau `gagalBeruntun[z] >= 3`, coba `sensor[z].begin(...)` lagi, **dibatasi** maksimal sekali per ~5 detik per zona agar loop kontrol tidak melambat.
3. Pertimbangkan perilaku PWM saat sensor error di mode KONTROL: tetap ditahan (sekarang), atau turun ke nilai aman. Dokumentasikan keputusannya.
4. Log ke Serial saat sensor pulih / hilang (`[SENSOR] Zona 4 pulih`).

## Catatan

- Library BH1750 v1.3.0 mengembalikan `-1` atau `-2` saat gagal; pastikan semua nilai negatif dianggap gagal (sudah benar).
- `begin()` mengirim perintah lewat I2C; pastikan `tcaSelect()` dipanggil sebelum re-init.
- Jaga total waktu `langkahKontrol()` tetap di bawah `CONTROL_INTERVAL` (500 ms).

## Kriteria Selesai

- [ ] Boot dengan satu BH1750 dicabut → sambungkan → dalam ≤ 10 detik zona tersebut membaca normal tanpa reset.
- [ ] Cabut BH1750 saat mode KONTROL → zona menampilkan `ERR`, zona lain tetap berjalan normal → sambungkan lagi → pulih otomatis.
- [ ] Perintah `init` berfungsi dan tercantum di `help`.
- [ ] Compile sukses, spesifikasi diperbarui.
