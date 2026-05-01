#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <DHT.h>

constexpr uint8_t SOIL_PIN = A0;
constexpr uint8_t SDA_PIN = A1;
constexpr uint8_t SCL_PIN = A2;
constexpr uint8_t DHT_PIN = A3;
constexpr uint8_t DHT_TYPE = DHT11;
constexpr uint8_t BH1750_ADDR = 0x23;

constexpr int TFT_CS = 12;
constexpr int TFT_DC = 11;
constexpr int TFT_MOSI = 10;
constexpr int TFT_SCLK = 9;
constexpr int TFT_RST = -1;

/*
  Monstera-richtwaarden voor deze sketch:
  - temperatuur: 18-27 C
  - luchtvochtigheid: 50-70%
  - licht: helder indirect licht, praktisch gezet op 2000-12000 lux
  - bodem: licht vochtig maar niet nat, hier gezet op 35-70%

  De bodemdrempels zijn omgerekend met jullie eigen testwaarden:
  - nat  = ongeveer 277
  - droog = ongeveer 540
  Kalibreer SOIL_RAW_WET en SOIL_RAW_DRY opnieuw als jullie sensor of potgrond verandert.
*/
constexpr float TEMP_MIN_C = 18.0f;
constexpr float TEMP_MAX_C = 27.0f;
constexpr float HUMIDITY_MIN_PCT = 50.0f;
constexpr float HUMIDITY_MAX_PCT = 70.0f;
constexpr float LIGHT_MIN_LUX = 2000.0f;
constexpr float LIGHT_MAX_LUX = 12000.0f;
constexpr float LIGHT_IGNORE_BELOW_LUX = 150.0f;
constexpr int SOIL_RAW_WET = 277;
constexpr int SOIL_RAW_DRY = 540;
constexpr int SOIL_MIN_PCT = 35;
constexpr int SOIL_MAX_PCT = 70;

constexpr unsigned long REFRESH_INTERVAL_MS = 2500UL;
constexpr uint16_t COLOR_WARNING = 0xFD20;

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
DHT dht(DHT_PIN, DHT_TYPE);

enum SensorState {
  SENSOR_LOW,
  SENSOR_OK,
  SENSOR_HIGH,
  SENSOR_IGNORED,
  SENSOR_UNKNOWN
};

struct SensorData {
  bool hasTemperature;
  float temperatureC;
  float humidityPct;
  float lux;
  int soilRaw;
  int soilPct;
};

void i2cDelay() {
  delayMicroseconds(10);
}

void sdaLow() {
  pinMode(SDA_PIN, OUTPUT);
  digitalWrite(SDA_PIN, LOW);
}

void sdaRelease() {
  pinMode(SDA_PIN, INPUT_PULLUP);
}

void sclLow() {
  pinMode(SCL_PIN, OUTPUT);
  digitalWrite(SCL_PIN, LOW);
}

void sclRelease() {
  pinMode(SCL_PIN, INPUT_PULLUP);
}

bool lineHigh(uint8_t pin) {
  return digitalRead(pin) == HIGH;
}

void i2cStart() {
  sdaRelease();
  sclRelease();
  i2cDelay();
  sdaLow();
  i2cDelay();
  sclLow();
}

void i2cStop() {
  sdaLow();
  i2cDelay();
  sclRelease();
  i2cDelay();
  sdaRelease();
  i2cDelay();
}

bool i2cWriteByte(uint8_t value) {
  for (uint8_t bit = 0; bit < 8; bit++) {
    if (value & 0x80) {
      sdaRelease();
    } else {
      sdaLow();
    }

    i2cDelay();
    sclRelease();
    i2cDelay();
    sclLow();
    i2cDelay();
    value <<= 1;
  }

  sdaRelease();
  i2cDelay();
  sclRelease();
  i2cDelay();
  const bool ack = !lineHigh(SDA_PIN);
  sclLow();
  i2cDelay();
  return ack;
}

uint8_t i2cReadByte(bool sendAck) {
  uint8_t value = 0;
  sdaRelease();

  for (uint8_t bit = 0; bit < 8; bit++) {
    value <<= 1;
    sclRelease();
    i2cDelay();
    if (lineHigh(SDA_PIN)) {
      value |= 1;
    }
    sclLow();
    i2cDelay();
  }

  if (sendAck) {
    sdaLow();
  } else {
    sdaRelease();
  }

  i2cDelay();
  sclRelease();
  i2cDelay();
  sclLow();
  i2cDelay();
  sdaRelease();

  return value;
}

void bh1750Start() {
  i2cStart();
  i2cWriteByte((BH1750_ADDR << 1) | 0);
  i2cWriteByte(0x10);
  i2cStop();
}

float bh1750ReadLux() {
  i2cStart();
  i2cWriteByte((BH1750_ADDR << 1) | 1);
  const uint8_t msb = i2cReadByte(true);
  const uint8_t lsb = i2cReadByte(false);
  i2cStop();

  const uint16_t raw = (static_cast<uint16_t>(msb) << 8) | lsb;
  return raw / 1.2f;
}

int readSoilRawAveraged() {
  long total = 0;

  for (uint8_t i = 0; i < 10; i++) {
    total += analogRead(SOIL_PIN);
    delay(5);
  }

  return total / 10;
}

int soilRawToPercent(int soilRaw) {
  const int clamped = constrain(soilRaw, SOIL_RAW_WET, SOIL_RAW_DRY);
  return (SOIL_RAW_DRY - clamped) * 100L / (SOIL_RAW_DRY - SOIL_RAW_WET);
}

SensorState evaluateRange(float value, float minimum, float maximum) {
  if (isnan(value)) {
    return SENSOR_UNKNOWN;
  }
  if (value < minimum) {
    return SENSOR_LOW;
  }
  if (value > maximum) {
    return SENSOR_HIGH;
  }
  return SENSOR_OK;
}

SensorState evaluateSoil(int soilPct) {
  if (soilPct < SOIL_MIN_PCT) {
    return SENSOR_LOW;
  }
  if (soilPct > SOIL_MAX_PCT) {
    return SENSOR_HIGH;
  }
  return SENSOR_OK;
}

SensorState evaluateLight(float lux) {
  if (isnan(lux)) {
    return SENSOR_UNKNOWN;
  }
  if (lux < LIGHT_IGNORE_BELOW_LUX) {
    return SENSOR_IGNORED;
  }
  return evaluateRange(lux, LIGHT_MIN_LUX, LIGHT_MAX_LUX);
}

uint16_t colorForState(SensorState state) {
  switch (state) {
    case SENSOR_OK:
      return GC9A01A_GREEN;
    case SENSOR_IGNORED:
      return GC9A01A_CYAN;
    case SENSOR_UNKNOWN:
      return COLOR_WARNING;
    case SENSOR_LOW:
    case SENSOR_HIGH:
    default:
      return GC9A01A_RED;
  }
}

bool isPlantHappy(const SensorData& data) {
  if (!data.hasTemperature) {
    return false;
  }

  const SensorState tempState = evaluateRange(data.temperatureC, TEMP_MIN_C, TEMP_MAX_C);
  const SensorState humidityState = evaluateRange(data.humidityPct, HUMIDITY_MIN_PCT, HUMIDITY_MAX_PCT);
  const SensorState soilState = evaluateSoil(data.soilPct);
  const SensorState lightState = evaluateLight(data.lux);

  const bool lightIsGoodEnough = (lightState == SENSOR_OK || lightState == SENSOR_IGNORED);

  return tempState == SENSOR_OK &&
         humidityState == SENSOR_OK &&
         soilState == SENSOR_OK &&
         lightIsGoodEnough;
}

const char* buildAdvice(const SensorData& data) {
  if (!data.hasTemperature) {
    return "Controleer DHT11";
  }

  if (data.soilPct < SOIL_MIN_PCT) {
    return "Geef water";
  }
  if (data.soilPct > SOIL_MAX_PCT) {
    return "Bodem te nat";
  }
  if (data.temperatureC < TEMP_MIN_C) {
    return "Te koud";
  }
  if (data.temperatureC > TEMP_MAX_C) {
    return "Te warm";
  }
  if (data.humidityPct < HUMIDITY_MIN_PCT) {
    return "Meer luchtvocht";
  }
  if (data.humidityPct > HUMIDITY_MAX_PCT) {
    return "Te vochtig";
  }
  if (data.lux >= LIGHT_IGNORE_BELOW_LUX && data.lux < LIGHT_MIN_LUX) {
    return "Meer indirect licht";
  }
  if (data.lux > LIGHT_MAX_LUX) {
    return "Te veel direct zon";
  }

  return "Monstera blij";
}

void drawCenteredText(const char* text, int16_t y, uint8_t size, uint16_t color) {
  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;

  tft.setTextSize(size);
  tft.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  const int16_t x = (240 - static_cast<int16_t>(w)) / 2;

  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.print(text);
}

void drawFace(bool happy) {
  const int cx = 120;
  const int cy = 63;
  const int radius = 44;

  tft.fillCircle(cx, cy, radius, GC9A01A_YELLOW);
  tft.drawCircle(cx, cy, radius, GC9A01A_WHITE);
  tft.fillCircle(cx - 16, cy - 10, 5, GC9A01A_BLACK);
  tft.fillCircle(cx + 16, cy - 10, 5, GC9A01A_BLACK);

  if (happy) {
    for (int i = 0; i < 3; i++) {
      tft.drawLine(cx - 23, cy + 14 + i, cx - 8, cy + 27 + i, GC9A01A_BLACK);
      tft.drawLine(cx - 8, cy + 27 + i, cx + 8, cy + 27 + i, GC9A01A_BLACK);
      tft.drawLine(cx + 8, cy + 27 + i, cx + 23, cy + 14 + i, GC9A01A_BLACK);
    }
  } else {
    tft.drawLine(cx - 25, cy - 25, cx - 8, cy - 19, GC9A01A_BLACK);
    tft.drawLine(cx + 8, cy - 19, cx + 25, cy - 25, GC9A01A_BLACK);

    for (int i = 0; i < 3; i++) {
      tft.drawLine(cx - 23, cy + 30 - i, cx - 8, cy + 18 - i, GC9A01A_BLACK);
      tft.drawLine(cx - 8, cy + 18 - i, cx + 8, cy + 18 - i, GC9A01A_BLACK);
      tft.drawLine(cx + 8, cy + 18 - i, cx + 23, cy + 30 - i, GC9A01A_BLACK);
    }
  }
}

void drawMetricLabel(int16_t x, int16_t y, const char* label, uint16_t color) {
  tft.setTextSize(2);
  tft.setTextColor(color);
  tft.setCursor(x, y);
  tft.print(label);
}

void drawScreen(const SensorData& data) {
  const bool happy = isPlantHappy(data);
  const char* advice = buildAdvice(data);

  const SensorState tempState = evaluateRange(data.temperatureC, TEMP_MIN_C, TEMP_MAX_C);
  const SensorState humidityState = evaluateRange(data.humidityPct, HUMIDITY_MIN_PCT, HUMIDITY_MAX_PCT);
  const SensorState soilState = evaluateSoil(data.soilPct);
  const SensorState lightState = evaluateLight(data.lux);

  tft.fillScreen(GC9A01A_BLACK);
  drawCenteredText("Monstera monitor", 8, 2, GC9A01A_WHITE);
  drawFace(happy);
  drawCenteredText(advice, 118, 2, happy ? GC9A01A_GREEN : GC9A01A_RED);

  drawMetricLabel(18, 150, "Temp:", colorForState(tempState));
  if (data.hasTemperature) {
    tft.print(data.temperatureC, 1);
    tft.print(" C");
  } else {
    tft.print("--");
  }

  drawMetricLabel(18, 170, "LV:", colorForState(humidityState));
  if (data.hasTemperature) {
    tft.print(data.humidityPct, 0);
    tft.print(" %");
  } else {
    tft.print("--");
  }

  drawMetricLabel(18, 190, "Lux:", colorForState(lightState));
  tft.print(data.lux, 0);
  if (lightState == SENSOR_IGNORED) {
    tft.print(" (nacht)");
  }

  drawMetricLabel(18, 210, "Bodem:", colorForState(soilState));
  tft.print(data.soilPct);
  tft.print(" %");
}

SensorData readSensors() {
  static bool hasCachedDht = false;
  static float cachedTemperatureC = 0.0f;
  static float cachedHumidityPct = 0.0f;

  SensorData data;
  data.hasTemperature = false;
  data.temperatureC = NAN;
  data.humidityPct = NAN;
  data.lux = NAN;
  data.soilRaw = 0;
  data.soilPct = 0;

  data.soilRaw = readSoilRawAveraged();
  data.soilPct = soilRawToPercent(data.soilRaw);
  data.lux = bh1750ReadLux();

  const float humidity = dht.readHumidity();
  const float temperatureC = dht.readTemperature();

  if (!isnan(humidity) && !isnan(temperatureC)) {
    hasCachedDht = true;
    cachedHumidityPct = humidity;
    cachedTemperatureC = temperatureC;
    data.hasTemperature = true;
    data.humidityPct = humidity;
    data.temperatureC = temperatureC;
  } else {
    data.hasTemperature = hasCachedDht;
    data.humidityPct = cachedHumidityPct;
    data.temperatureC = cachedTemperatureC;
  }

  return data;
}

void printSerialReport(const SensorData& data) {
  Serial.println(F("---- Monstera meting ----"));
  Serial.print(F("Bodem raw: "));
  Serial.println(data.soilRaw);
  Serial.print(F("Bodem %: "));
  Serial.println(data.soilPct);
  Serial.print(F("Lux: "));
  Serial.println(data.lux, 0);

  if (data.hasTemperature) {
    Serial.print(F("Temperatuur: "));
    Serial.print(data.temperatureC, 1);
    Serial.println(F(" C"));
    Serial.print(F("Luchtvochtigheid: "));
    Serial.print(data.humidityPct, 0);
    Serial.println(F(" %"));
  } else {
    Serial.println(F("DHT11: geen geldige meting"));
  }

  Serial.print(F("Status: "));
  Serial.println(buildAdvice(data));
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  pinMode(SOIL_PIN, INPUT);

  sdaRelease();
  sclRelease();
  delay(200);
  bh1750Start();

  dht.begin();

  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);
  drawCenteredText("Monstera monitor", 20, 2, GC9A01A_WHITE);
  drawCenteredText("Start op...", 110, 2, GC9A01A_CYAN);

  Serial.println(F("Plant Health Monitor voor Monstera gestart"));
}

void loop() {
  const SensorData data = readSensors();
  drawScreen(data);
  printSerialReport(data);
  delay(REFRESH_INTERVAL_MS);
}
