// ============================================================
// MONSTERA PLANT MONITOR
// ============================================================
// Benodigde libraries (installeer via Arduino Library Manager):
//   - Adafruit GFX Library
//   - Adafruit GC9A01A  (zoek op "GC9A01")
//   - DHT sensor library (by Adafruit)
//
// Bestanden in je projectmap:
//   - monstera_monitor.ino  (dit bestand)
//   - images.h              (de bitmap arrays)
// ============================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <DHT.h>
#include "images.h"

// ============================================================
// PIN CONFIGURATIE
// ============================================================
constexpr uint8_t SOIL_PIN = A0;  // bodemvochtsensor
constexpr uint8_t SDA_PIN  = A1;  // I2C data  (BH1750)
constexpr uint8_t SCL_PIN  = A2;  // I2C clock (BH1750)
constexpr uint8_t DHT_PIN  = A3;  // DHT11

constexpr uint8_t DHT_TYPE    = DHT11;
constexpr uint8_t BH1750_ADDR = 0x23;

constexpr int TFT_CS   = 12;
constexpr int TFT_DC   = 11;
constexpr int TFT_MOSI = 10;
constexpr int TFT_SCLK = 9;
constexpr int TFT_RST  = -1;

// ============================================================
// DREMPELWAARDEN
// ============================================================
constexpr float TEMP_MIN = 18.0f;
constexpr float TEMP_MAX = 27.0f;

constexpr float HUM_MIN  = 50.0f;
constexpr float HUM_MAX  = 70.0f;

constexpr float LIGHT_MIN    = 2000.0f;
constexpr float LIGHT_MAX    = 12000.0f;
constexpr float LIGHT_IGNORE = 150.0f;   // onder deze waarde = nacht

constexpr int SOIL_WET = 277;
constexpr int SOIL_DRY = 540;
constexpr int SOIL_MIN = 35;
constexpr int SOIL_MAX = 70;

constexpr unsigned long REFRESH_MS = 2500;

// ============================================================
// IMAGE INDICES
// ============================================================
// Volgorde moet overeenkomen met epd_bitmap_allArray[] in images.h
// ============================================================
// OM EEN IMAGE TE VERWIJDEREN:
//   1. Verwijder de array in images.h
//   2. Verwijder de entry in epd_bitmap_allArray[] in images.h
//   3. Pas IMG_COUNT aan in images.h
//   4. Update de indices hieronder zodat ze kloppen
//   5. Vervang de bijhorende return in chooseImage() door een fallback
// ============================================================
constexpr uint8_t IMG_HAPPY      = 0;  // epd_bitmap_IMG_1935 - alles OK
constexpr uint8_t IMG_SOIL_LOW   = 1;  // epd_bitmap_IMG_1936 - te droog
constexpr uint8_t IMG_SOIL_HIGH  = 2;  // epd_bitmap_IMG_1937 - te nat
constexpr uint8_t IMG_TEMP_LOW   = 3;  // epd_bitmap_IMG_1938 - te koud
constexpr uint8_t IMG_TEMP_HIGH  = 4;  // epd_bitmap_IMG_1939 - te warm
constexpr uint8_t IMG_LIGHT_LOW  = 5;  // epd_bitmap_IMG_1941 - te donker
constexpr uint8_t IMG_LIGHT_HIGH = 6;  // epd_bitmap_IMG_1942 - te zonnig
constexpr uint8_t IMG_NIGHT      = 7;  // epd_bitmap_IMG_1944 - nacht
constexpr uint8_t IMG_ERROR      = 8;  // epd_bitmap_IMG_1946 - sensor fout

constexpr uint16_t SCREEN_W = 240;
constexpr uint16_t SCREEN_H = 240;
constexpr uint8_t  IMG_W    = 128;
constexpr uint8_t  IMG_H    = 128;

// Gecentreerde positie op het ronde scherm
constexpr int16_t IMG_X = (SCREEN_W - IMG_W) / 2;  // = 56
constexpr int16_t IMG_Y = (SCREEN_H - IMG_H) / 2;  // = 56

// ============================================================
// OBJECTEN
// ============================================================
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
// DATA STRUCTUREN
// ============================================================
struct SensorData {
  bool  validDHT;
  float temperature;
  float humidity;
  float lux;
  int   soilRaw;
  int   soilPct;
};

enum State { OK, TOO_LOW, TOO_HIGH, IGNORED, UNKNOWN };

// ============================================================
// 1. SENSOREN
// ============================================================

// --- Software I2C voor BH1750 lichtsensor ---
void i2cDelay()   { delayMicroseconds(10); }
void sdaLow()     { pinMode(SDA_PIN, OUTPUT); digitalWrite(SDA_PIN, LOW); }
void sdaRelease() { pinMode(SDA_PIN, INPUT_PULLUP); }
void sclLow()     { pinMode(SCL_PIN, OUTPUT); digitalWrite(SCL_PIN, LOW); }
void sclRelease() { pinMode(SCL_PIN, INPUT_PULLUP); }

void i2cStart() {
  sdaRelease(); sclRelease(); i2cDelay();
  sdaLow(); i2cDelay(); sclLow();
}

void i2cStop() {
  sdaLow(); i2cDelay();
  sclRelease(); i2cDelay();
  sdaRelease(); i2cDelay();
}

bool i2cWrite(uint8_t v) {
  for (int i = 0; i < 8; i++) {
    (v & 0x80) ? sdaRelease() : sdaLow();
    i2cDelay(); sclRelease(); i2cDelay(); sclLow(); i2cDelay();
    v <<= 1;
  }
  sdaRelease(); i2cDelay(); sclRelease(); i2cDelay();
  bool ack = !digitalRead(SDA_PIN);
  sclLow(); i2cDelay();
  return ack;
}

uint8_t i2cRead(bool ack) {
  uint8_t v = 0;
  sdaRelease();
  for (int i = 0; i < 8; i++) {
    v <<= 1;
    sclRelease(); i2cDelay();
    if (digitalRead(SDA_PIN)) v |= 1;
    sclLow(); i2cDelay();
  }
  ack ? sdaLow() : sdaRelease();
  i2cDelay(); sclRelease(); i2cDelay(); sclLow(); i2cDelay();
  sdaRelease();
  return v;
}

void bh1750Init() {
  i2cStart();
  i2cWrite((BH1750_ADDR << 1) | 0);
  i2cWrite(0x10);  // continuous high resolution
  i2cStop();
}

float readLux() {
  i2cStart();
  i2cWrite((BH1750_ADDR << 1) | 1);
  uint8_t msb = i2cRead(true);
  uint8_t lsb = i2cRead(false);
  i2cStop();
  return ((msb << 8) | lsb) / 1.2f;
}

int readSoil() {
  long s = 0;
  for (int i = 0; i < 10; i++) { s += analogRead(SOIL_PIN); delay(5); }
  return s / 10;
}

int soilPct(int r) {
  r = constrain(r, SOIL_WET, SOIL_DRY);
  return (SOIL_DRY - r) * 100 / (SOIL_DRY - SOIL_WET);
}

SensorData readSensors() {
  static bool  cached = false;
  static float cachedTemp, cachedHum;
  SensorData d;

  d.soilRaw = readSoil();
  d.soilPct = soilPct(d.soilRaw);
  d.lux     = readLux();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) { cached = true; cachedTemp = t; cachedHum = h; }

  d.validDHT    = cached;
  d.temperature = cachedTemp;
  d.humidity    = cachedHum;
  return d;
}

// ============================================================
// 2. LOGICA
// ============================================================
State rangeCheck(float v, float mn, float mx) {
  if (isnan(v)) return UNKNOWN;
  if (v < mn)   return TOO_LOW;
  if (v > mx)   return TOO_HIGH;
  return OK;
}

State soilState(int p) {
  if (p < SOIL_MIN) return TOO_LOW;
  if (p > SOIL_MAX) return TOO_HIGH;
  return OK;
}

State lightState(float l) {
  if (isnan(l))         return UNKNOWN;
  if (l < LIGHT_IGNORE) return IGNORED;
  return rangeCheck(l, LIGHT_MIN, LIGHT_MAX);
}

bool plantHappy(const SensorData& d) {
  if (!d.validDHT) return false;
  return rangeCheck(d.temperature, TEMP_MIN, TEMP_MAX) == OK
      && rangeCheck(d.humidity,    HUM_MIN,  HUM_MAX)  == OK
      && soilState(d.soilPct) == OK
      && (lightState(d.lux) == OK || lightState(d.lux) == IGNORED);
}

// Kiest welk gezicht getoond wordt
// Prioriteit: error -> nacht -> bodem -> temp -> licht -> blij
uint8_t chooseImage(const SensorData& d) {
  if (!d.validDHT)                       return IMG_ERROR;
  if (lightState(d.lux) == IGNORED)      return IMG_NIGHT;
  if (soilState(d.soilPct) == TOO_LOW)   return IMG_SOIL_LOW;
  if (soilState(d.soilPct) == TOO_HIGH)  return IMG_SOIL_HIGH;
  if (d.temperature < TEMP_MIN)          return IMG_TEMP_LOW;
  if (d.temperature > TEMP_MAX)          return IMG_TEMP_HIGH;
  if (lightState(d.lux) == TOO_LOW)      return IMG_LIGHT_LOW;
  if (lightState(d.lux) == TOO_HIGH)     return IMG_LIGHT_HIGH;
  return IMG_HAPPY;
}

const char* adviceText(const SensorData& d) {
  if (!d.validDHT)              return "Sensor fout";
  if (d.soilPct < SOIL_MIN)     return "Water geven!";
  if (d.soilPct > SOIL_MAX)     return "Te nat";
  if (d.temperature < TEMP_MIN) return "Te koud";
  if (d.temperature > TEMP_MAX) return "Te warm";
  if (d.humidity < HUM_MIN)     return "Lucht te droog";
  if (d.humidity > HUM_MAX)     return "Te vochtig";
  if (lightState(d.lux) == TOO_LOW)  return "Meer licht";
  if (lightState(d.lux) == TOO_HIGH) return "Te veel zon";
  return "Happy plant :)";
}

// ============================================================
// 3. DISPLAY
// ============================================================

// Teken bitmap gecentreerd op het ronde 240x240 scherm
void drawFaceBitmap(uint8_t index) {
  const uint16_t* bmp = (const uint16_t*) pgm_read_ptr(&epd_bitmap_allArray[index]);
  tft.startWrite();
  tft.setAddrWindow(IMG_X, IMG_Y, IMG_W, IMG_H);
  for (uint16_t i = 0; i < (uint16_t)(IMG_W * IMG_H); i++) {
    tft.SPI_WRITE16(pgm_read_word(&bmp[i]));
  }
  tft.endWrite();
}

// Tekst horizontaal gecentreerd
void centerText(const char* t, int y, uint16_t c, uint8_t size = 2) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.getTextBounds(t, 0, y, &x1, &y1, &w, &h);
  tft.setCursor((SCREEN_W - w) / 2, y);
  tft.setTextColor(c);
  tft.print(t);
}

void drawScreen(const SensorData& d) {
  tft.fillScreen(GC9A01A_BLACK);

  bool happy = plantHappy(d);

  // Titel bovenaan
  centerText("Monstera", 8, GC9A01A_WHITE, 2);

  // Gezicht bitmap gecentreerd op scherm
  drawFaceBitmap(chooseImage(d));

  // Status boodschap
  centerText(adviceText(d), 188, happy ? GC9A01A_GREEN : GC9A01A_RED, 2);

  // Sensorwaarden onderaan in kleine tekst
  char buf[28];
  snprintf(buf, sizeof(buf), "%.1fC  Vocht: %.0f%%", d.temperature, d.humidity);
  centerText(buf, 210, GC9A01A_WHITE, 1);

  snprintf(buf, sizeof(buf), "Lux:%.0f  Grond:%d%%", d.lux, d.soilPct);
  centerText(buf, 222, GC9A01A_WHITE, 1);
}

// ============================================================
// MAIN
// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(SOIL_PIN, INPUT);

  sdaRelease();
  sclRelease();
  bh1750Init();
  dht.begin();

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);

  Serial.println("Monstera Monitor gestart");
}

void loop() {
  SensorData d = readSensors();
  drawScreen(d);

  Serial.print("Temp:");    Serial.print(d.temperature);
  Serial.print(" Hum:");    Serial.print(d.humidity);
  Serial.print(" Lux:");    Serial.print(d.lux);
  Serial.print(" Grond:");  Serial.print(d.soilPct);
  Serial.print("% -> ");    Serial.println(adviceText(d));

  delay(REFRESH_MS);
}
