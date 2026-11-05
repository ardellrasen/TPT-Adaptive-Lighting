# Bab 7 — Mode Sensor IR (MANUAL_IR)

## 7.1 Ide dasarnya

> **Perumpamaan: lampu toilet di mall.**
> Kamu masuk → lampu menyala. Selama kamu bergerak, lampu tetap menyala. Kamu keluar → beberapa detik kemudian lampu mati sendiri.

Itulah mode `MANUAL_IR`, dengan satu aturan per zona:

- **Sensor IR zona z** mendeteksi → **lampu zona z** menyala.
- Setiap ada deteksi, **timer di-reset** (dihitung ulang dari awal).
- Kalau tidak ada deteksi selama `irTimerMs` (default **10 detik**) → lampu mati.

## 7.2 Timeline contoh

Timer = 10 detik. Seseorang lewat di zona 3:

```
waktu (s):   0    2    4    6    8   10   12   14   16   18   20   22   24
IR zona 3:   ____‾‾‾‾‾‾______‾‾‾‾_________________________________________
             (‾ = terdeteksi)
                 ^          ^   ^
                 |          |   terakhir terdeteksi di detik 8
                 |          terdeteksi lagi di detik 7 -> timer reset
                 pertama terdeteksi di detik 2 -> lampu ON

Lampu z3:    ____‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾______________
                 ON                                      OFF di detik 18
                                                         (8 + 10)
```

Yang penting: lampu mati **10 detik setelah deteksi terakhir**, bukan 10 detik setelah pertama menyala.

> **Perumpamaan:** Seperti batas waktu jawab di kuis. Setiap kali kamu menjawab, waktunya diulang dari 10 detik. Kalau 10 detik diam, giliranmu habis.

## 7.3 Logika di kode: `prosesIR()`

Fungsi ini dipanggil di **setiap putaran `loop()`** (ribuan kali per detik) saat mode MANUAL_IR.

```cpp
void prosesIR(unsigned long now) {
  for (uint8_t z = 0; z < NUM_CH; z++) {
    bool deteksi = (digitalRead(IR_PIN[z]) == IR_AKTIF);

    if (deteksi) {
      irTerakhir[z] = now;             // catat "terakhir terlihat"
      if (!irNyala[z]) {               // kalau lampu masih mati
        irNyala[z] = true;
        setChannel(z, pwmIrZona(z));   // nyalakan
      }
    } else if (irNyala[z] && (now - irTerakhir[z] >= irTimerMs)) {
      irNyala[z] = false;              // sudah lama tidak terlihat
      setChannel(z, 0);                // matikan
    }
  }
}
```

### Diagram keadaan satu zona

```mermaid
stateDiagram-v2
    [*] --> MATI: masuk mode manual_ir
    MATI --> NYALA: IR terdeteksi, catat waktu, PWM = pwmIrZona
    NYALA --> NYALA: IR terdeteksi lagi, timer di-reset
    NYALA --> MATI: tidak terdeteksi selama irTimerMs
```

Setiap zona punya "ingatan" sendiri:

| Variabel | Isi | Perumpamaan |
|---|---|---|
| `irNyala[z]` | Apakah lampu zona z sedang menyala karena IR | Catatan "lampu toilet 3: ON" |
| `irTerakhir[z]` | Kapan terakhir zona z mendeteksi orang | "Terakhir ada orang jam 12:00:08" |
| `irIkutMaks[z]` | Apakah terangnya mengikuti `pwmMaks` | Catatan "lampu toilet 3: pakai setelan pusat" |
| `irPwmOn[z]` | Terang pilihan sendiri, dipakai bila `irIkutMaks[z]` bernilai false | Setelan khusus lampu toilet 3 |

Kenapa perlu `irNyala[z]`? Supaya `setChannel()` dan pesan `[IR] ... ON` hanya dijalankan **sekali** saat lampu baru menyala, bukan ribuan kali per detik selama orangnya berdiri di situ.

### Seberapa terang lampunya: `pwmIrZona()`

```cpp
uint16_t pwmIrZona(uint8_t z) {
  return irIkutMaks[z] ? pwmMaks : min(irPwmOn[z], pwmMaks);
}
```

Tanda `? :` artinya "kalau benar pakai yang kiri, kalau salah pakai yang kanan".

| Kondisi zona | Hasil `pwmIrZona(z)` | Tampilan di `ir` / `status` |
|---|---|---|
| Belum pernah diset (default) | `pwmMaks` | `100 (maks)` |
| Sudah diset, misalnya `3 60` | 60, tetapi tidak boleh melebihi `pwmMaks` | `60 (manual)` |

> **Perumpamaan:** Semua lampu mengikuti setelan pusat gedung, kecuali yang sudah diberi setelan sendiri. Setelan sendiri tetap tidak boleh melewati batas gedung.

## 7.4 Contoh sesi Serial

```
> manual_ir
========== MODE MANUAL_IR ==========
Semua lampu mati. Sensor IR zona z mendeteksi -> lampu CH z menyala.
...
Timer: 10.0 s | PWM maks: 100

> timer 5
>> Timer IR: 5.0 s

> 3 60
Zona 3 -> PWM saat terdeteksi: 60

(tangan di depan sensor zona 3)
[IR] Zona 3 terdeteksi -> lampu ON (PWM 60, 5.0 s)

> ir
---------- SENSOR IR ----------
Timer: 5.0 s
Zona 0 | GPIO 13 | kosong     | PWM nyala 100 (maks) | lampu OFF
...
Zona 3 | GPIO 4 | kosong     | PWM nyala 60 (manual) | lampu ON (sisa 3.2 s)
...

(5 detik kemudian)
[IR] Zona 3 timer habis -> lampu OFF
```

## 7.5 Hal yang perlu diperhatikan

1. **`3 60` di mode ini tidak langsung menyalakan lampu.** Artinya "kalau nanti terdeteksi, pakai PWM 60". Kalau lampu zona 3 sedang menyala, nilainya langsung ikut berubah.
2. **`timer` berlaku langsung**, termasuk untuk lampu yang sedang menyala.
3. **PWM IR mengikuti `maks` secara default.** Setelah zona diset manual (mis. `3 60`), zona itu memakai nilai tersebut dan hanya dibatasi oleh `maks`. Lihat [Task 03](../../tasks/03-pwm-ir-tidak-ikut-maks.md).
   - Lampu yang sedang menyala langsung ikut berubah saat `maks` diubah.
   - `all <nilai>` membuat **semua** zona menjadi manual. Zona manual baru mengikuti `maks` lagi setelah reboot.
   - Nilai dipotong saat diketik. Dengan `maks 100`, perintah `3 500` menyimpan 100, jadi naikkan `maks` lebih dulu.
4. **Sensor IR hanya diproses di mode MANUAL_IR.** Di mode lain, perintah `ir` tetap bisa menampilkan status sensor, tapi lampu tidak bereaksi.

## Rangkuman Bab 7

- IR terdeteksi → lampu zona itu menyala dengan `pwmIrZona(z)`.
- Timer **di-reset** setiap deteksi; lampu mati `irTimerMs` setelah deteksi **terakhir**.
- `irNyala[z]` mencegah aksi "nyalakan" diulang ribuan kali.
- Terang lampu mengikuti `maks`, kecuali zona yang sudah diset manual.

Lanjut ke [Bab 8 — Latihan](08-latihan.md).
