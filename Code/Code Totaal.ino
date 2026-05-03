// ============================================================
// MONSTERA PLANT MONITOR
// ============================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <DHT.h>
#include "images.h"   // <-- jouw gegenereerde bitmap arrays

// ============================================================
// PIN CONFIGURATION
// ============================================================
constexpr uint8_t SOIL_PIN = A0;
constexpr uint8_t SDA_PIN  = A1;
constexpr uint8_t SCL_PIN  = A2;
constexpr uint8_t DHT_PIN  = A3;

constexpr uint8_t DHT_TYPE    = DHT11;
constexpr uint8_t BH1750_ADDR = 0x23;

constexpr int TFT_CS   = 12;
constexpr int TFT_DC   = 11;
constexpr int TFT_MOSI = 10;
constexpr int TFT_SCLK = 9;
constexpr int TFT_RST  = -1;

// ============================================================
// THRESHOLDS
// ============================================================
constexpr float TEMP_MIN = 18.0f;
constexpr float TEMP_MAX = 27.0f;

constexpr float HUM_MIN  = 50.0f;
constexpr float HUM_MAX  = 70.0f;

constexpr float LIGHT_MIN    = 2000.0f;
constexpr float LIGHT_MAX    = 12000.0f;
constexpr float LIGHT_IGNORE = 150.0f;

constexpr int SOIL_WET = 277;
constexpr int SOIL_DRY = 540;
constexpr int SOIL_MIN = 35;
constexpr int SOIL_MAX = 70;

constexpr unsigned long REFRESH_MS = 2500;

// ============================================================
// IMAGE INDEX (komt overeen met volgorde in images.h)
// ============================================================
constexpr uint8_t IMG_HAPPY      = 0;  // IMG_1935 - blije plant
constexpr uint8_t IMG_SOIL_LOW   = 1;  // IMG_1936 - te droog
constexpr uint8_t IMG_SOIL_HIGH  = 2;  // IMG_1937 - te nat
constexpr uint8_t IMG_TEMP_LOW   = 3;  // IMG_1938 - te koud
constexpr uint8_t IMG_TEMP_HIGH  = 4;  // IMG_1939 - te warm
constexpr uint8_t IMG_LIGHT_LOW  = 5;  // IMG_1941 - te donker
constexpr uint8_t IMG_LIGHT_HIGH = 6;  // IMG_1942 - te veel zon
constexpr uint8_t IMG_NIGHT      = 7;  // IMG_1944 - nacht (slapend)
constexpr uint8_t IMG_ERROR      = 8;  // IMG_1946 - sensor fout

// Bitmap array (volgorde = index hierboven)
const uint16_t* const epd_bitmap_allArray[] PROGMEM = {
  epd_bitmap_IMG_1935,  // 0 happy
  epd_bitmap_IMG_1936,  // 1 soil low
  epd_bitmap_IMG_1937,  // 2 soil high
  epd_bitmap_IMG_1938,  // 3 temp low
  epd_bitmap_IMG_1939,  // 4 temp high
  epd_bitmap_IMG_1941,  // 5 light low
  epd_bitmap_IMG_1942,  // 6 light high
  epd_bitmap_IMG_1944,  // 7 night
  epd_bitmap_IMG_1946   // 8 error
};

constexpr uint8_t IMG_W = 128;
constexpr uint8_t IMG_H = 128;

// ================== OBJECTS ==================
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
// DATA STRUCTURES
// ============================================================
struct SensorData {
  bool  validDHT;
  float temperature;
  float humidity;
  float lux;
  int   soilRaw;
  int   soilPct;
};

enum State { OK, LOW, HIGH, IGNORED, UNKNOWN };

// ============================================================
// 1. SENSORS (ongewijzigd)
// ============================================================
void i2cDelay()    { delayMicroseconds(10); }
void sdaLow()      { pinMode(SDA_PIN, OUTPUT); digitalWrite(SDA_PIN, LOW); }
void sdaRelease()  { pinMode(SDA_PIN, INPUT_PULLUP); }
void sclLow()      { pinMode(SCL_PIN, OUTPUT); digitalWrite(SCL_PIN, LOW); }
void sclRelease()  { pinMode(SCL_PIN, INPUT_PULLUP); }

void i2cStart() { sdaRelease(); sclRelease(); i2cDelay(); sdaLow(); i2cDelay(); sclLow(); }
void i2cStop()  { sdaLow(); i2cDelay(); sclRelease(); i2cDelay(); sdaRelease(); i2cDelay(); }

bool i2cWrite(uint8_t v) {
  for (int i = 0; i < 8; i++) {
    (v & 0x80) ? sdaRelease() : sdaLow();
    i2cDelay(); sclRelease(); i2cDelay(); sclLow(); i2cDelay(); v <<= 1;
  }
  sdaRelease(); i2cDelay(); sclRelease(); i2cDelay();
  bool ack = !digitalRead(SDA_PIN);
  sclLow(); i2cDelay();
  return ack;
}

uint8_t i2cRead(bool ack) {
  uint8_t v = 0; sdaRelease();
  for (int i = 0; i < 8; i++) {
    v <<= 1; sclRelease(); i2cDelay();
    if (digitalRead(SDA_PIN)) v |= 1;
    sclLow(); i2cDelay();
  }
  ack ? sdaLow() : sdaRelease();
  i2cDelay(); sclRelease(); i2cDelay(); sclLow(); i2cDelay(); sdaRelease();
  return v;
}

void bh1750Init() {
  i2cStart(); i2cWrite((BH1750_ADDR << 1) | 0); i2cWrite(0x10); i2cStop();
}

float readLux() {
  i2cStart(); i2cWrite((BH1750_ADDR << 1) | 1);
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
  static bool  cache = false;
  static float ct, ch;
  SensorData d;
  d.soilRaw = readSoil();
  d.soilPct = soilPct(d.soilRaw);
  d.lux     = readLux();
  float h   = dht.readHumidity();
  float t   = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) { cache = true; ct = t; ch = h; }
  d.validDHT   = cache;
  d.temperature = ct;
  d.humidity    = ch;
  return d;
}

// ============================================================
// 2. LOGIC
// ============================================================
State range(float v, float mn, float mx) {
  if (isnan(v)) return UNKNOWN;
  if (v < mn)   return LOW;
  if (v > mx)   return HIGH;
  return OK;
}

State soilState(int p) {
  if (p < SOIL_MIN) return LOW;
  if (p > SOIL_MAX) return HIGH;
  return OK;
}

State lightState(float l) {
  if (isnan(l))         return UNKNOWN;
  if (l < LIGHT_IGNORE) return IGNORED;
  return range(l, LIGHT_MIN, LIGHT_MAX);
}

bool plantHappy(const SensorData& d) {
  if (!d.validDHT) return false;
  return range(d.temperature, TEMP_MIN, TEMP_MAX) == OK &&
         range(d.humidity,    HUM_MIN,  HUM_MAX)  == OK &&
         soilState(d.soilPct)  == OK &&
         (lightState(d.lux) == OK || lightState(d.lux) == IGNORED);
}

// Bepaalt welke image getoond wordt — prioriteit: sensor > licht > happy
uint8_t chooseImage(const SensorData& d) {
  if (!d.validDHT)                        return IMG_ERROR;
  if (lightState(d.lux) == IGNORED)       return IMG_NIGHT;
  if (soilState(d.soilPct)  == LOW)       return IMG_SOIL_LOW;
  if (soilState(d.soilPct)  == HIGH)      return IMG_SOIL_HIGH;
  if (d.temperature < TEMP_MIN)           return IMG_TEMP_LOW;
  if (d.temperature > TEMP_MAX)           return IMG_TEMP_HIGH;
  if (lightState(d.lux)     == LOW)       return IMG_LIGHT_LOW;
  if (lightState(d.lux)     == HIGH)      return IMG_LIGHT_HIGH;
  return IMG_HAPPY;
}

const char* advice(const SensorData& d) {
  if (!d.validDHT)            return "DHT error";
  if (d.soilPct < SOIL_MIN)   return "Water me!";
  if (d.soilPct > SOIL_MAX)   return "Too wet";
  if (d.temperature < TEMP_MIN) return "Too cold";
  if (d.temperature > TEMP_MAX) return "Too hot";
  if (d.humidity < HUM_MIN)   return "Dry air";
  if (d.humidity > HUM_MAX)   return "Humid";
  if (d.lux < LIGHT_MIN && d.lux > LIGHT_IGNORE) return "More light";
  if (d.lux > LIGHT_MAX)      return "Too sunny";
  return "Happy plant";
}

// ============================================================
// 3. DISPLAY
// ============================================================

// Teken een 128x128 bitmap gecentreerd op het ronde 240x240 scherm
// Gecentreerd = x offset: (240-128)/2 = 56, y offset: (240-128)/2 = 56
void drawFaceBitmap(uint8_t index) {
  const uint16_t* bmp = (const uint16_t*)pgm_read_ptr(&epd_bitmap_allArray[index]);
  const int16_t x0 = (240 - IMG_W) / 2;  // 56
  const int16_t y0 = (240 - IMG_H) / 2;  // 56

  tft.startWrite();
  tft.setAddrWindow(x0, y0, IMG_W, IMG_H);

  // Stuur pixels rij per rij rechtstreeks via SPI voor snelheid
  for (uint16_t i = 0; i < (uint16_t)(IMG_W * IMG_H); i++) {
    uint16_t color = pgm_read_word(&bmp[i]);
    tft.SPI_WRITE16(color);
  }
  tft.endWrite();
}

void center(const char* t, int y, uint16_t c) {
  int16_t x1, y1; uint16_t w, h;
  tft.setTextSize(2);
  tft.getTextBounds(t, 0, y, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, y);
  tft.setTextColor(c);
  tft.print(t);
}

void drawScreen(const SensorData& d) {
  tft.fillScreen(GC9A01A_BLACK);

  bool happy = plantHappy(d);

  // Titel bovenaan
  center("Monstera", 10, GC9A01A_WHITE);

  // Gezicht bitmap gecentreerd
  drawFaceBitmap(chooseImage(d));

  // Status tekst
  center(advice(d), 190, happy ? GC9A01A_GREEN : GC9A01A_RED);

  // Sensorwaarden onderaan
  tft.setTextSize(1);
  tft.setTextColor(GC9A01A_WHITE);

  tft.setCursor(20, 210);
  tft.print("T:"); tft.print(d.temperature, 1); tft.print("C ");
  tft.print("H:"); tft.print(d.humidity, 0);    tft.print("%");

  tft.setCursor(20, 222);
  tft.print("Lux:"); tft.print((int)d.lux);     tft.print("  ");
  tft.print("Soil:"); tft.print(d.soilPct);      tft.print("%");
}

// ============================================================
// MAIN
// ============================================================
void setup() {
  Serial.begin(115200);
  pinMode(SOIL_PIN, INPUT);
  sdaRelease(); sclRelease();
  bh1750Init();
  dht.begin();
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);
}

void loop() {
  SensorData d = readSensors();
  drawScreen(d);
  Serial.println(advice(d));
  delay(REFRESH_MS);
}
