#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <BH1750.h>
#include <Preferences.h>

// =====================================================================
//  BUS I2C PCA9685 (Wire, SDA 22 / SCL 23) -- test edit
// =====================================================================
#define PCA_SDA 22
#define PCA_SCL 23
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

#define PWM_FREQ 1000      // ~1000Hz agar LED COB tidak flicker
#define NUM_CH   10        // jumlah zona / channel

// Mapping zona (0-9) ke pin fisik PCA9685
const uint8_t PIN_MAP[NUM_CH] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
uint16_t nilaiCh[NUM_CH] = {0};

// Batas maksimum PWM yang boleh dikeluarkan (semua mode)
// Bisa diubah lewat Serial: "maks <nilai>"
uint16_t pwmMaks = 100;

// =====================================================================
//  SENSOR IR HW-201 (OUT LOW = ada benda / terdeteksi) -- test edit lagi ah
//  IR zona z mengatur lampu CH z -- test edit lagi ah lagi
// =====================================================================
const uint8_t IR_PIN[NUM_CH] = {15, 2, 0, 4, 16, 17, 5, 18, 19, 21};
#define IR_AKTIF LOW

unsigned long irTimerMs = 10000;       // lama lampu menyala (ms), "timer <detik>"
uint16_t irPwmOn[NUM_CH];              // kecerahan saat terdeteksi per zona
bool irNyala[NUM_CH] = {false};
unsigned long irTerakhir[NUM_CH] = {0};

// =====================================================================
//  BUS I2C SENSOR CAHAYA (Wire1, pin berpindah antara TCA1 dan TCA2)
// =====================================================================
#define TCA1_SDA 32
#define TCA1_SCL 33
#define TCA2_SDA 25
#define TCA2_SCL 26

#define TCA_ADDR    0x70     // kedua TCA default (A0,A1,A2 = GND)
#define BH1750_ADDR 0x23     // pin ADDR BH1750 = GND / tidak disambung
#define I2C_FREQ_SENSOR 100000

const uint8_t BUS_SDA[2] = {TCA1_SDA, TCA2_SDA};
const uint8_t BUS_SCL[2] = {TCA1_SCL, TCA2_SCL};
int8_t busAktif = -1;        // -1 = belum ada bus aktif

// Zona -> (modul TCA, channel TCA)
// Zona 0-7 : TCA1 SD0-7/SC0-7 -> PWM 0-7
// Zona 8-9 : TCA2 SD0-1/SC0-1 -> PWM 8-9
const uint8_t SENSOR_TCA[NUM_CH] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1};
const uint8_t SENSOR_CH[NUM_CH]  = {0, 1, 2, 3, 4, 5, 6, 7, 0, 1};

BH1750 sensor[NUM_CH];
bool  sensorOk[NUM_CH] = {false};
float luxCh[NUM_CH];

// =====================================================================
//  KALIBRASI (berdasarkan zona 0, dipakai untuk semua zona)
//  lux_asli = kalGain * lux_mentah + kalOffset
// =====================================================================
#define KAL_ZONA   0       // zona acuan kalibrasi
#define KAL_MAKS   10      // maksimum titik kalibrasi
#define KAL_SAMPEL 8       // jumlah sampel yang dirata-rata per titik

Preferences prefs;
uint8_t kalN = 0;
float   kalRaw[KAL_MAKS];  // bacaan BH1750 mentah
float   kalRef[KAL_MAKS];  // bacaan luxmeter
float   kalGain   = 1.0;
float   kalOffset = 0.0;
float   kalR2     = -1;    // -1 = belum bisa dihitung

// =====================================================================
//  PARAMETER KONTROL
//  Bisa diubah lewat Serial: "th <bawah> <atas>", "kp <nilai>", "step <min> <maks>"
// =====================================================================
float thBawah = 300.0;     // threshold bawah (lux terkalibrasi)
float thAtas  = 400.0;     // threshold atas  (lux terkalibrasi)

float KP       = 0.05;     // tambahan PWM per 1 lux selisih dari threshold
int   STEP_MIN = 1;        // perubahan PWM minimum per langkah
int   STEP_MAX = 5;        // perubahan PWM maksimum per langkah

#define CONTROL_INTERVAL 500   // ms, periode loop kontrol
#define PRINT_INTERVAL   1000  // ms, periode cetak monitoring

enum Mode { MODE_MANUAL, MODE_KONTROL, MODE_KALIBRASI, MODE_MANUAL_IR };
Mode mode = MODE_MANUAL;

unsigned long tKontrol = 0;
unsigned long tPrint   = 0;
String bufferInput = "";

const char* namaMode() {
  switch (mode) {
    case MODE_MANUAL:    return "MANUAL";
    case MODE_KONTROL:   return "KONTROL";
    case MODE_KALIBRASI: return "KALIBRASI";
    case MODE_MANUAL_IR: return "MANUAL_IR";
  }
  return "?";
}

// =====================================================================
//  FUNGSI PCA9685
// =====================================================================
void setChannel(uint8_t ch, uint16_t nilai) {
  if (nilai > pwmMaks) nilai = pwmMaks;   // batas maksimum PWM
  nilaiCh[ch] = nilai;
  pwm.setPin(PIN_MAP[ch], nilai);         // 0 = full OFF, 4095 = full ON
}

void semuaOff() {
  for (uint8_t z = 0; z < NUM_CH; z++) {
    setChannel(z, 0);
    irNyala[z] = false;
  }
}

// =====================================================================
//  FUNGSI BUS SENSOR, TCA9548A, BH1750
// =====================================================================
void pilihBus(uint8_t modul) {
  if (busAktif == modul) return;
  if (busAktif >= 0) Wire1.end();
  Wire1.begin(BUS_SDA[modul], BUS_SCL[modul], I2C_FREQ_SENSOR);
  busAktif = modul;
}

bool tcaSelect(uint8_t modul, uint8_t ch) {
  pilihBus(modul);
  Wire1.beginTransmission(TCA_ADDR);
  Wire1.write(1 << ch);
  return Wire1.endTransmission() == 0;
}

bool cekTCA(uint8_t modul) {
  pilihBus(modul);
  Wire1.beginTransmission(TCA_ADDR);
  return Wire1.endTransmission() == 0;
}

// Bacaan mentah BH1750 (tanpa kalibrasi), negatif jika gagal
float bacaSensorMentah(uint8_t z) {
  if (!sensorOk[z]) return -1;
  if (!tcaSelect(SENSOR_TCA[z], SENSOR_CH[z])) return -1;
  return sensor[z].readLightLevel();
}

// Bacaan terkalibrasi (dipakai baca, status, dan sistem kontrol)
float bacaSensor(uint8_t z) {
  float mentah = bacaSensorMentah(z);
  if (mentah < 0) return mentah;
  float lux = kalGain * mentah + kalOffset;
  return (lux < 0) ? 0 : lux;
}

// Rata-rata beberapa bacaan mentah (untuk kalibrasi)
float bacaMentahRata(uint8_t z) {
  float total = 0;
  uint8_t ok = 0;
  for (uint8_t i = 0; i < KAL_SAMPEL; i++) {
    float m = bacaSensorMentah(z);
    if (m >= 0) { total += m; ok++; }
    delay(130);   // BH1750 high-res update tiap ~120ms
  }
  if (ok == 0) return -1;
  return total / ok;
}

void initSensor() {
  for (uint8_t m = 0; m < 2; m++) {
    Serial.print("TCA");
    Serial.print(m + 1);
    Serial.print(" (SDA ");
    Serial.print(BUS_SDA[m]);
    Serial.print(", SCL ");
    Serial.print(BUS_SCL[m]);
    Serial.println(cekTCA(m) ? ") terdeteksi" : ") TIDAK terdeteksi!");
  }

  for (uint8_t z = 0; z < NUM_CH; z++) {
    sensorOk[z] = false;
    luxCh[z] = -1;
    if (tcaSelect(SENSOR_TCA[z], SENSOR_CH[z])) {
      sensorOk[z] = sensor[z].begin(BH1750::CONTINUOUS_HIGH_RES_MODE, BH1750_ADDR, &Wire1);
    }
    Serial.print("  Zona ");
    Serial.print(z);
    Serial.print(" -> TCA");
    Serial.print(SENSOR_TCA[z] + 1);
    Serial.print(" CH");
    Serial.print(SENSOR_CH[z]);
    Serial.println(sensorOk[z] ? " : BH1750 OK" : " : BH1750 GAGAL");
  }
  delay(200);
}

// =====================================================================
//  FUNGSI KALIBRASI
// =====================================================================
// 1 titik   -> GAIN = asli/mentah, OFFSET = 0
// >=2 titik -> regresi linear (least squares)
void hitungKalibrasi() {
  kalR2 = -1;
  if (kalN == 0) { kalGain = 1.0; kalOffset = 0.0; return; }

  double sx = 0, sy = 0, sxx = 0, sxy = 0, syy = 0;
  for (uint8_t i = 0; i < kalN; i++) {
    double x = kalRaw[i], y = kalRef[i];
    sx += x; sy += y; sxx += x * x; sxy += x * y; syy += y * y;
  }
  double n = kalN;
  double den = n * sxx - sx * sx;

  // 1 titik, atau semua bacaan mentah hampir sama -> pakai rasio
  if (kalN == 1 || den < 1.0 * n) {
    kalGain = (sx > 0) ? (float)(sy / sx) : 1.0;
    kalOffset = 0.0;
    return;
  }

  double gain = (n * sxy - sx * sy) / den;
  double offset = (sy - gain * sx) / n;
  if (gain <= 0) {   // tidak masuk akal -> fallback rasio
    kalGain = (sx > 0) ? (float)(sy / sx) : 1.0;
    kalOffset = 0.0;
    return;
  }
  kalGain = (float)gain;
  kalOffset = (float)offset;

  double num = n * sxy - sx * sy;
  double dy = n * syy - sy * sy;
  if (dy > 0) kalR2 = (float)((num * num) / (den * dy));
}

void simpanKalibrasi() {
  prefs.putUChar("n", kalN);
  prefs.putBytes("raw", kalRaw, sizeof(kalRaw));
  prefs.putBytes("ref", kalRef, sizeof(kalRef));
}

void muatKalibrasi() {
  kalN = prefs.getUChar("n", 0);
  if (kalN > KAL_MAKS ||
      prefs.getBytesLength("raw") != sizeof(kalRaw) ||
      prefs.getBytesLength("ref") != sizeof(kalRef)) {
    kalN = 0;
  }
  if (kalN > 0) {
    prefs.getBytes("raw", kalRaw, sizeof(kalRaw));
    prefs.getBytes("ref", kalRef, sizeof(kalRef));
  }
  hitungKalibrasi();
  if (kalN > 0) {
    Serial.print("Kalibrasi dimuat: GAIN ");
    Serial.print(kalGain, 4);
    Serial.print("  OFFSET ");
    Serial.println(kalOffset, 2);
  } else {
    Serial.println("Belum ada kalibrasi (GAIN=1, OFFSET=0).");
  }
}

void cetakKalibrasi() {
  Serial.println("---------- KALIBRASI (acuan zona 0, dipakai semua zona) ----------");
  Serial.print("Titik  : ");
  Serial.println(kalN);
  Serial.print("GAIN   : ");
  Serial.println(kalGain, 4);
  Serial.print("OFFSET : ");
  Serial.println(kalOffset, 2);
  Serial.print("R2     : ");
  if (kalR2 < 0) Serial.println("-");
  else Serial.println(kalR2, 4);
  for (uint8_t i = 0; i < kalN; i++) {
    Serial.print("  #");
    Serial.print(i + 1);
    Serial.print("  mentah ");
    Serial.print(kalRaw[i], 1);
    Serial.print("  -> luxmeter ");
    Serial.print(kalRef[i], 1);
    Serial.print("  (hasil kalibrasi ");
    Serial.print(kalGain * kalRaw[i] + kalOffset, 1);
    Serial.println(")");
  }
}

void tambahTitikKalibrasi(float luxAsli) {
  if (!sensorOk[KAL_ZONA]) {
    Serial.println("Sensor zona 0 tidak terdeteksi, kalibrasi tidak bisa dilakukan.");
    return;
  }
  if (kalN >= KAL_MAKS) {
    Serial.println("Titik kalibrasi penuh. Ketik 'batal' atau 'reset' dulu.");
    return;
  }

  Serial.println("Membaca BH1750 zona 0 (~1 detik), jangan ubah cahaya...");
  float mentah = bacaMentahRata(KAL_ZONA);
  if (mentah < 0) {
    Serial.println("Gagal membaca sensor zona 0.");
    return;
  }

  kalRaw[kalN] = mentah;
  kalRef[kalN] = luxAsli;
  kalN++;
  hitungKalibrasi();
  simpanKalibrasi();

  Serial.print(">> Titik #");
  Serial.print(kalN);
  Serial.print(": BH1750 mentah ");
  Serial.print(mentah, 1);
  Serial.print(" lx  |  luxmeter ");
  Serial.print(luxAsli, 1);
  Serial.println(" lx");
  Serial.print(">> GAIN ");
  Serial.print(kalGain, 4);
  Serial.print("  OFFSET ");
  Serial.print(kalOffset, 2);
  Serial.print("  R2 ");
  if (kalR2 < 0) Serial.println("-");
  else Serial.println(kalR2, 4);
  Serial.println("Ubah cahaya lalu ketik nilai luxmeter berikutnya, atau 'selesai'.");
}

void cekKalibrasi() {
  float m = bacaSensorMentah(KAL_ZONA);
  Serial.print("Zona 0 | mentah ");
  if (m < 0) { Serial.println("ERR"); return; }
  Serial.print(m, 1);
  Serial.print(" lx  ->  terkalibrasi ");
  Serial.print(max(0.0f, kalGain * m + kalOffset), 1);
  Serial.println(" lx");
}

// =====================================================================
//  SISTEM KONTROL (BH1750)
// =====================================================================
void langkahKontrol() {
  for (uint8_t z = 0; z < NUM_CH; z++) {
    float lux = bacaSensor(z);
    luxCh[z] = lux;
    if (lux < 0) continue;   // sensor error -> PWM zona ini ditahan

    int delta = 0;
    if (lux < thBawah) {
      delta = constrain((int)((thBawah - lux) * KP), STEP_MIN, STEP_MAX);
    } else if (lux > thAtas) {
      delta = -constrain((int)((lux - thAtas) * KP), STEP_MIN, STEP_MAX);
    }

    if (delta != 0) {
      int baru = constrain((int)nilaiCh[z] + delta, 0, (int)pwmMaks);
      setChannel(z, baru);
    }
  }
}

void cetakMonitor() {
  Serial.print("[KONTROL ");
  Serial.print(thBawah, 0);
  Serial.print("-");
  Serial.print(thAtas, 0);
  Serial.print(" lx, maks ");
  Serial.print(pwmMaks);
  Serial.print("] ");
  for (uint8_t z = 0; z < NUM_CH; z++) {
    Serial.print("Z");
    Serial.print(z);
    Serial.print(":");
    if (luxCh[z] < 0) Serial.print("ERR");
    else Serial.print(luxCh[z], 1);
    Serial.print("/");
    Serial.print(nilaiCh[z]);
    Serial.print("  ");
  }
  Serial.println();
}

// =====================================================================
//  MODE MANUAL_IR (HW-201)
//  Default lampu mati. IR zona z terdeteksi -> lampu CH z menyala
//  dengan PWM irPwmOn[z] selama irTimerMs. Timer direset setiap deteksi.
// =====================================================================
void prosesIR(unsigned long now) {
  for (uint8_t z = 0; z < NUM_CH; z++) {
    bool deteksi = (digitalRead(IR_PIN[z]) == IR_AKTIF);

    if (deteksi) {
      irTerakhir[z] = now;             // reset timer
      if (!irNyala[z]) {
        irNyala[z] = true;
        setChannel(z, irPwmOn[z]);
        Serial.print("[IR] Zona ");
        Serial.print(z);
        Serial.print(" terdeteksi -> lampu ON (PWM ");
        Serial.print(nilaiCh[z]);
        Serial.print(", ");
        Serial.print(irTimerMs / 1000.0, 1);
        Serial.println(" s)");
      }
    } else if (irNyala[z] && (now - irTerakhir[z] >= irTimerMs)) {
      irNyala[z] = false;
      setChannel(z, 0);
      Serial.print("[IR] Zona ");
      Serial.print(z);
      Serial.println(" timer habis -> lampu OFF");
    }
  }
}

void cetakIR() {
  unsigned long now = millis();
  Serial.println("---------- SENSOR IR ----------");
  Serial.print("Timer: ");
  Serial.print(irTimerMs / 1000.0, 1);
  Serial.println(" s");
  for (uint8_t z = 0; z < NUM_CH; z++) {
    bool deteksi = (digitalRead(IR_PIN[z]) == IR_AKTIF);
    Serial.print("Zona ");
    Serial.print(z);
    Serial.print(" | GPIO ");
    Serial.print(IR_PIN[z]);
    Serial.print(" | ");
    Serial.print(deteksi ? "TERDETEKSI" : "kosong    ");
    Serial.print(" | PWM nyala ");
    Serial.print(min(irPwmOn[z], pwmMaks));
    Serial.print(" | lampu ");
    if (mode == MODE_MANUAL_IR && irNyala[z]) {
      unsigned long lewat = now - irTerakhir[z];
      unsigned long sisa = (lewat < irTimerMs) ? (irTimerMs - lewat) : 0;
      Serial.print("ON (sisa ");
      Serial.print(sisa / 1000.0, 1);
      Serial.println(" s)");
    } else {
      Serial.println(nilaiCh[z] > 0 ? "ON" : "OFF");
    }
  }
}

// =====================================================================
//  TAMPILAN SERIAL
// =====================================================================
void cetakBantuan() {
  Serial.println("========== PERINTAH ==========");
  Serial.println("  manual              masuk mode manual");
  Serial.println("  manual_ir           masuk mode lampu berbasis sensor IR + timer");
  Serial.println("  kontrol             masuk mode sistem kontrol (BH1750)");
  Serial.println("  kalibrasi           masuk mode kalibrasi BH1750 (acuan zona 0)");
  Serial.println("  <zona> <nilai>      set PWM zona (di manual_ir: PWM saat terdeteksi)");
  Serial.println("  all <nilai>         set PWM semua zona");
  Serial.println("  off                 matikan semua zona (otomatis pindah ke manual)");
  Serial.println("  maks <nilai>        batas maksimum PWM semua mode, contoh: maks 100");
  Serial.println("  timer <detik>       lama lampu menyala di manual_ir, contoh: timer 10");
  Serial.println("  ir                  tampilkan status semua sensor IR");
  Serial.println("  baca                baca semua BH1750 (terkalibrasi)");
  Serial.println("  th <bawah> <atas>   ubah threshold, contoh: th 300 400");
  Serial.println("  kp <nilai>          ubah gain kontrol, contoh: kp 0.05");
  Serial.println("  step <min> <maks>   ubah langkah PWM kontrol, contoh: step 1 5");
  Serial.println("  kalinfo             tampilkan GAIN, OFFSET, dan titik kalibrasi");
  Serial.println("  status              tampilkan mode, threshold, PWM, lux");
  Serial.println("  help                tampilkan bantuan");
  Serial.print("PWM maksimum saat ini: ");
  Serial.println(pwmMaks);
  Serial.println("Format monitor: Z<zona>:<lux>/<pwm>");
}

void cetakBantuanKalibrasi() {
  Serial.println("========== MODE KALIBRASI ==========");
  Serial.println("Letakkan luxmeter tepat di sebelah BH1750 zona 0, menghadap arah yang sama.");
  Serial.println("  <angka>             ketik nilai luxmeter, contoh: 152   (titik kalibrasi)");
  Serial.println("  0 <nilai>           ubah cahaya zona 0, contoh: 0 50");
  Serial.println("  all <nilai>         ubah cahaya semua zona");
  Serial.println("  maks <nilai>        ubah batas maksimum PWM");
  Serial.println("  cek                 lihat bacaan zona 0 (mentah & terkalibrasi)");
  Serial.println("  batal               hapus titik terakhir");
  Serial.println("  reset               hapus semua titik (GAIN=1, OFFSET=0)");
  Serial.println("  kalinfo             tampilkan hasil kalibrasi");
  Serial.println("  selesai             keluar ke mode manual");
  Serial.print("Ambil 3-5 titik dari gelap sampai terang. PWM maksimum saat ini: ");
  Serial.println(pwmMaks);
}

void cetakBantuanIR() {
  Serial.println("========== MODE MANUAL_IR ==========");
  Serial.println("Semua lampu mati. Sensor IR zona z mendeteksi -> lampu CH z menyala.");
  Serial.println("Timer direset setiap ada deteksi, lampu mati setelah timer habis.");
  Serial.println("  <zona> <nilai>      PWM saat terdeteksi, contoh: 3 80");
  Serial.println("  all <nilai>         PWM saat terdeteksi untuk semua zona");
  Serial.println("  timer <detik>       lama lampu menyala, contoh: timer 10");
  Serial.println("  ir                  status sensor IR & sisa timer");
  Serial.println("  manual              keluar ke mode manual");
  Serial.print("Timer: ");
  Serial.print(irTimerMs / 1000.0, 1);
  Serial.print(" s | PWM maks: ");
  Serial.println(pwmMaks);
}

void cetakStatus() {
  Serial.println("---------- STATUS ----------");
  Serial.print("Mode      : ");
  Serial.println(namaMode());
  Serial.print("PWM maks  : ");
  Serial.print(pwmMaks);
  Serial.print(" (~");
  Serial.print(pwmMaks * 100.0 / 4095.0, 1);
  Serial.println("%)");
  Serial.print("Timer IR  : ");
  Serial.print(irTimerMs / 1000.0, 1);
  Serial.println(" s");
  Serial.print("Threshold : ");
  Serial.print(thBawah, 1);
  Serial.print(" - ");
  Serial.print(thAtas, 1);
  Serial.println(" lx");
  Serial.print("KP        : ");
  Serial.print(KP, 3);
  Serial.print("  | Step: ");
  Serial.print(STEP_MIN);
  Serial.print(" - ");
  Serial.println(STEP_MAX);
  Serial.print("Kalibrasi : GAIN ");
  Serial.print(kalGain, 4);
  Serial.print("  OFFSET ");
  Serial.print(kalOffset, 2);
  Serial.print("  (");
  Serial.print(kalN);
  Serial.println(" titik)");
  for (uint8_t z = 0; z < NUM_CH; z++) {
    Serial.print("Zona ");
    Serial.print(z);
    Serial.print(" | PWM ");
    Serial.print(nilaiCh[z]);
    Serial.print(" (~");
    Serial.print(nilaiCh[z] * 100.0 / 4095.0, 1);
    Serial.print("%) | PWM IR ");
    Serial.print(min(irPwmOn[z], pwmMaks));
    Serial.print(" | Lux ");
    if (luxCh[z] < 0) Serial.println("ERR / belum dibaca");
    else Serial.println(luxCh[z], 1);
  }
}

void bacaSemua() {
  Serial.println("---------- BACA BH1750 (terkalibrasi) ----------");
  for (uint8_t z = 0; z < NUM_CH; z++) {
    luxCh[z] = bacaSensor(z);
    Serial.print("Zona ");
    Serial.print(z);
    Serial.print(" : ");
    if (luxCh[z] < 0) Serial.print("ERR");
    else Serial.print(luxCh[z], 1);
    Serial.print(" lx  (PWM ");
    Serial.print(nilaiCh[z]);
    Serial.println(")");
  }
}

// =====================================================================
//  PARSING INPUT
// =====================================================================
bool isAngka(const String &s) {
  if (s.length() == 0) return false;
  for (unsigned int i = 0; i < s.length(); i++) {
    if (!isDigit(s[i])) return false;
  }
  return true;
}

bool isDesimal(const String &s) {
  if (s.length() == 0) return false;
  int titik = 0;
  for (unsigned int i = 0; i < s.length(); i++) {
    if (s[i] == '.') { if (++titik > 1) return false; }
    else if (!isDigit(s[i])) return false;
  }
  return true;
}

int pisah(String s, String tok[], int maks) {
  int n = 0;
  s.replace(',', ' ');
  s.trim();
  while (s.length() > 0 && n < maks) {
    int sp = s.indexOf(' ');
    if (sp < 0) { tok[n++] = s; break; }
    tok[n++] = s.substring(0, sp);
    s = s.substring(sp + 1);
    s.trim();
  }
  return n;
}

bool bacaBaris(String &out) {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (bufferInput.length() > 0) {
        out = bufferInput;
        bufferInput = "";
        return true;
      }
    } else if (bufferInput.length() < 64) {
      bufferInput += c;
    }
  }
  return false;
}

void prosesPerintah(String input) {
  input.trim();
  input.toLowerCase();
  if (input.length() == 0) return;

  String tok[3];
  int n = pisah(input, tok, 3);

  // ---------- perintah umum ----------
  if (tok[0] == "help") {
    if (mode == MODE_KALIBRASI) cetakBantuanKalibrasi();
    else if (mode == MODE_MANUAL_IR) cetakBantuanIR();
    else cetakBantuan();
    return;
  }
  if (tok[0] == "status")  { cetakStatus();    return; }
  if (tok[0] == "baca")    { bacaSemua();      return; }
  if (tok[0] == "kalinfo") { cetakKalibrasi(); return; }
  if (tok[0] == "ir")      { cetakIR();        return; }

  if (tok[0] == "maks") {
    if (n < 2 || !isAngka(tok[1])) {
      Serial.println("Format: maks <nilai>, contoh: maks 100 (0-4095)");
      return;
    }
    long m = tok[1].toInt();
    if (m > 4095) m = 4095;
    pwmMaks = (uint16_t)m;

    // Turunkan zona yang sedang di atas batas baru
    uint8_t diturunkan = 0;
    for (uint8_t z = 0; z < NUM_CH; z++) {
      if (nilaiCh[z] > pwmMaks) { setChannel(z, pwmMaks); diturunkan++; }
    }

    Serial.print(">> PWM maksimum: ");
    Serial.print(pwmMaks);
    Serial.print(" (~");
    Serial.print(pwmMaks * 100.0 / 4095.0, 1);
    Serial.println("%)");
    if (diturunkan > 0) {
      Serial.print("   ");
      Serial.print(diturunkan);
      Serial.println(" zona diturunkan ke batas baru.");
    }
    return;
  }

  if (tok[0] == "timer") {
    if (n < 2 || !isDesimal(tok[1]) || tok[1].toFloat() <= 0) {
      Serial.println("Format: timer <detik>, contoh: timer 10");
      return;
    }
    irTimerMs = (unsigned long)(tok[1].toFloat() * 1000.0);
    Serial.print(">> Timer IR: ");
    Serial.print(irTimerMs / 1000.0, 1);
    Serial.println(" s");
    return;
  }

  if (tok[0] == "manual" || tok[0] == "selesai") {
    if (mode == MODE_KALIBRASI) {
      Serial.print(">> Kalibrasi selesai. GAIN ");
      Serial.print(kalGain, 4);
      Serial.print("  OFFSET ");
      Serial.print(kalOffset, 2);
      Serial.println(" (tersimpan, berlaku untuk semua zona)");
    }
    mode = MODE_MANUAL;
    for (uint8_t z = 0; z < NUM_CH; z++) irNyala[z] = false;
    Serial.println(">> Mode MANUAL. PWM terakhir dipertahankan.");
    return;
  }

  if (tok[0] == "manual_ir") {
    semuaOff();
    mode = MODE_MANUAL_IR;
    cetakBantuanIR();
    return;
  }

  if (tok[0] == "kalibrasi") {
    mode = MODE_KALIBRASI;
    for (uint8_t z = 0; z < NUM_CH; z++) irNyala[z] = false;
    cetakBantuanKalibrasi();
    if (kalN > 0) {
      Serial.print("Sudah ada ");
      Serial.print(kalN);
      Serial.println(" titik tersimpan. Ketik 'reset' jika ingin mulai dari awal.");
    }
    cekKalibrasi();
    return;
  }

  if (tok[0] == "kontrol" || tok[0] == "auto") {
    if (thBawah >= thAtas) {
      Serial.println("Threshold tidak valid (bawah harus < atas).");
      return;
    }
    mode = MODE_KONTROL;
    for (uint8_t z = 0; z < NUM_CH; z++) irNyala[z] = false;
    tKontrol = tPrint = millis();
    Serial.print(">> Mode KONTROL. Target ");
    Serial.print(thBawah, 1);
    Serial.print(" - ");
    Serial.print(thAtas, 1);
    Serial.print(" lx, PWM maks ");
    Serial.println(pwmMaks);
    return;
  }

  if (tok[0] == "off") {
    semuaOff();
    mode = MODE_MANUAL;
    Serial.println(">> Semua zona OFF, mode MANUAL.");
    return;
  }

  if (tok[0] == "th") {
    if (n < 3 || !isDesimal(tok[1]) || !isDesimal(tok[2])) {
      Serial.println("Format: th <bawah> <atas>, contoh: th 300 400");
      return;
    }
    float b = tok[1].toFloat();
    float a = tok[2].toFloat();
    if (b >= a) {
      Serial.println("Threshold bawah harus lebih kecil dari threshold atas.");
      return;
    }
    thBawah = b;
    thAtas = a;
    Serial.print(">> Threshold baru: ");
    Serial.print(thBawah, 1);
    Serial.print(" - ");
    Serial.print(thAtas, 1);
    Serial.println(" lx");
    return;
  }

  if (tok[0] == "kp") {
    if (n < 2 || !isDesimal(tok[1])) {
      Serial.println("Format: kp <nilai>, contoh: kp 0.05");
      return;
    }
    KP = tok[1].toFloat();
    Serial.print(">> KP baru: ");
    Serial.println(KP, 3);
    return;
  }

  if (tok[0] == "step") {
    if (n < 3 || !isAngka(tok[1]) || !isAngka(tok[2])) {
      Serial.println("Format: step <min> <maks>, contoh: step 1 5");
      return;
    }
    int smin = tok[1].toInt();
    int smax = tok[2].toInt();
    if (smin < 1 || smin > smax) {
      Serial.println("Syarat: 1 <= min <= maks");
      return;
    }
    STEP_MIN = smin;
    STEP_MAX = smax;
    Serial.print(">> Step PWM: min ");
    Serial.print(STEP_MIN);
    Serial.print(", maks ");
    Serial.println(STEP_MAX);
    return;
  }

  // ---------- perintah khusus mode kalibrasi ----------
  if (mode == MODE_KALIBRASI) {
    // Satu angka saja = nilai luxmeter
    if (n == 1 && isDesimal(tok[0])) {
      tambahTitikKalibrasi(tok[0].toFloat());
      return;
    }
    if (tok[0] == "cek") { cekKalibrasi(); return; }
    if (tok[0] == "batal") {
      if (kalN == 0) { Serial.println("Belum ada titik kalibrasi."); return; }
      kalN--;
      hitungKalibrasi();
      simpanKalibrasi();
      Serial.println(">> Titik terakhir dihapus.");
      cetakKalibrasi();
      return;
    }
    if (tok[0] == "reset") {
      kalN = 0;
      hitungKalibrasi();
      simpanKalibrasi();
      Serial.println(">> Semua titik kalibrasi dihapus (GAIN=1, OFFSET=0).");
      return;
    }
  }

  // ---------- perintah PWM per zona ----------
  if (mode == MODE_KONTROL) {
    Serial.println("Sedang mode KONTROL. Ketik 'manual' dulu untuk dimming manual.");
    return;
  }

  if (n < 2) {
    Serial.println("Format salah. Ketik 'help' untuk bantuan.");
    return;
  }

  if (!isAngka(tok[1])) {
    Serial.println("Nilai PWM tidak valid, masukkan angka 0-4095.");
    return;
  }
  long nilai = tok[1].toInt();
  if (nilai > pwmMaks) {
    Serial.print("Nilai melebihi PWM maksimum, dibatasi ke ");
    Serial.println(pwmMaks);
    nilai = pwmMaks;
  }

  // Tentukan zona (all = -1)
  int zona;
  if (tok[0] == "all") {
    zona = -1;
  } else {
    if (!isAngka(tok[0]) || tok[0].toInt() >= NUM_CH) {
      Serial.println("Zona tidak valid, masukkan 0-9 atau 'all'.");
      return;
    }
    zona = tok[0].toInt();
  }

  // Di manual_ir: atur PWM saat terdeteksi, bukan langsung menyalakan
  if (mode == MODE_MANUAL_IR) {
    for (uint8_t z = 0; z < NUM_CH; z++) {
      if (zona >= 0 && z != zona) continue;
      irPwmOn[z] = nilai;
      if (irNyala[z]) setChannel(z, nilai);   // zona yang sedang menyala ikut berubah
    }
    if (zona < 0) Serial.print("Semua zona");
    else { Serial.print("Zona "); Serial.print(zona); }
    Serial.print(" -> PWM saat terdeteksi: ");
    Serial.println(nilai);
    return;
  }

  // Mode manual & kalibrasi: langsung set PWM
  for (uint8_t z = 0; z < NUM_CH; z++) {
    if (zona >= 0 && z != zona) continue;
    setChannel(z, nilai);
  }
  if (zona < 0) Serial.print("Semua zona");
  else { Serial.print("Zona "); Serial.print(zona); }
  Serial.print(" -> ");
  Serial.print(nilai);
  Serial.print(" (~");
  Serial.print(nilai * 100.0 / 4095.0, 1);
  Serial.println("%)");
}

// =====================================================================
//  SETUP & LOOP
// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Adaptive Lighting 10 Zona: PCA9685 + 2x TCA9548A + 10x BH1750 + 10x IR ===");

  // Sensor IR
  for (uint8_t z = 0; z < NUM_CH; z++) {
    pinMode(IR_PIN[z], INPUT_PULLUP);
    irPwmOn[z] = pwmMaks;          // default PWM saat terdeteksi = batas maks
  }

  // Bus PCA9685
  Wire.begin(PCA_SDA, PCA_SCL);
  pwm.begin();
  pwm.setPWMFreq(PWM_FREQ);
  for (int i = 0; i < 16; i++) pwm.setPin(i, 0);

  // Bus sensor cahaya
  initSensor();

  // Kalibrasi dari flash
  prefs.begin("kalbh", false);
  muatKalibrasi();

  cetakBantuan();
  Serial.println(">> Mode awal: MANUAL");
}

void loop() {
  String baris;
  if (bacaBaris(baris)) prosesPerintah(baris);

  unsigned long now = millis();

  if (mode == MODE_KONTROL) {
    if (now - tKontrol >= CONTROL_INTERVAL) {
      tKontrol = now;
      langkahKontrol();
    }
    if (now - tPrint >= PRINT_INTERVAL) {
      tPrint = now;
      cetakMonitor();
    }
  } else if (mode == MODE_MANUAL_IR) {
    prosesIR(now);
  }
}
