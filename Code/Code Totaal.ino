// ============================================================
// MONSTERA PLANT MONITOR
// ============================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <DHT.h>

// ============================================================
// PIN CONFIGURATION
// ============================================================
// Analog pins for sensors
constexpr uint8_t SOIL_PIN = A0;   // bodemvochtsensor
constexpr uint8_t SDA_PIN  = A1;   // I2C data (software I2C)
constexpr uint8_t SCL_PIN  = A2;   // I2C clock
constexpr uint8_t DHT_PIN  = A3;   // DHT11 sensor

// Sensor instellingen
constexpr uint8_t DHT_TYPE = DHT11;
constexpr uint8_t BH1750_ADDR = 0x23; // lichtsensor adres

// TFT scherm pinnen
constexpr int TFT_CS   = 12;
constexpr int TFT_DC   = 11;
constexpr int TFT_MOSI = 10;
constexpr int TFT_SCLK = 9;
constexpr int TFT_RST  = -1;

// ============================================================
// THRESHOLDS (PLANT RULES)
// Deze waarden bepalen wanneer de plant "ok" is
// ============================================================
constexpr float TEMP_MIN = 18.0f;  // minimum temperatuur (°C)
constexpr float TEMP_MAX = 27.0f;  // maximum temperatuur (°C)

constexpr float HUM_MIN  = 50.0f;  // minimum luchtvochtigheid (%)
constexpr float HUM_MAX  = 70.0f;  // maximum luchtvochtigheid (%)

constexpr float LIGHT_MIN = 2000.0f;   // minimum licht (lux)
constexpr float LIGHT_MAX = 12000.0f;  // maximum licht (lux)
constexpr float LIGHT_IGNORE = 150.0f; // onder deze waarde = nacht

// Kalibratie bodem sensor (afhankelijk van jouw grond/sensor!)
constexpr int SOIL_WET = 277; // volledig nat
constexpr int SOIL_DRY = 540; // volledig droog

constexpr int SOIL_MIN = 35; // minimum % voor "ok"
constexpr int SOIL_MAX = 70; // maximum % voor "ok"

// Refresh snelheid van het systeem
constexpr unsigned long REFRESH_MS = 2500;

// ============================================================
// HARDWARE OBJECTEN
// ============================================================
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
DHT dht(DHT_PIN, DHT_TYPE);

// ============================================================
// DATA STRUCTURES
// ============================================================
// Hier slaan we alle sensorwaarden samen op
struct SensorData {
  bool validDHT;      // is DHT meting geldig?
  float temperature;  // temperatuur in °C
  float humidity;     // luchtvochtigheid in %
  float lux;          // licht in lux
  int soilRaw;        // ruwe analoge waarde
  int soilPct;        // omgerekend naar %
};

// Mogelijke statussen van een meting
enum State { OK, LOW, HIGH, IGNORED, UNKNOWN };

// ============================================================
// 1. SENSORS
// ============================================================

// -------- I2C (software implementation) --------
// Omdat geen Wire library gebruikt wordt
void i2cDelay() { delayMicroseconds(10); }

void sdaLow(){ pinMode(SDA_PIN, OUTPUT); digitalWrite(SDA_PIN, LOW); }
void sdaRelease(){ pinMode(SDA_PIN, INPUT_PULLUP); }

void sclLow(){ pinMode(SCL_PIN, OUTPUT); digitalWrite(SCL_PIN, LOW); }
void sclRelease(){ pinMode(SCL_PIN, INPUT_PULLUP); }

// Start conditie voor I2C
void i2cStart(){ sdaRelease(); sclRelease(); i2cDelay(); sdaLow(); i2cDelay(); sclLow(); }

// Stop conditie
void i2cStop(){ sdaLow(); i2cDelay(); sclRelease(); i2cDelay(); sdaRelease(); i2cDelay(); }

// Schrijf 1 byte naar I2C bus
bool i2cWrite(uint8_t v){
  for(int i=0;i<8;i++){
    (v&0x80)?sdaRelease():sdaLow();
    i2cDelay(); sclRelease(); i2cDelay(); sclLow(); i2cDelay(); v<<=1;
  }

  // wacht op ACK van sensor
  sdaRelease(); i2cDelay(); sclRelease(); i2cDelay();
  bool ack=!digitalRead(SDA_PIN);
  sclLow(); i2cDelay();
  return ack;
}

// Lees 1 byte van I2C bus
uint8_t i2cRead(bool ack){
  uint8_t v=0; sdaRelease();

  for(int i=0;i<8;i++){
    v<<=1;
    sclRelease(); i2cDelay();
    if(digitalRead(SDA_PIN)) v|=1;
    sclLow(); i2cDelay();
  }

  // stuur ACK of NACK terug
  ack?sdaLow():sdaRelease();

  i2cDelay(); sclRelease(); i2cDelay(); sclLow(); i2cDelay(); sdaRelease();
  return v;
}

// Initialiseer lichtsensor
void bh1750Init(){
  i2cStart();
  i2cWrite((BH1750_ADDR<<1)|0);
  i2cWrite(0x10); // continuous high resolution mode
  i2cStop();
}

// Lees lux waarde
float readLux(){
  i2cStart();
  i2cWrite((BH1750_ADDR<<1)|1);

  uint8_t msb=i2cRead(true);
  uint8_t lsb=i2cRead(false);

  i2cStop();

  return ((msb<<8)|lsb)/1.2f;
}

// Lees bodemwaarde (gemiddelde voor stabiliteit)
int readSoil(){
  long s=0;
  for(int i=0;i<10;i++){
    s+=analogRead(SOIL_PIN);
    delay(5);
  }
  return s/10;
}

// Zet raw waarde om naar percentage
int soilPct(int r){
  r=constrain(r,SOIL_WET,SOIL_DRY);
  return (SOIL_DRY-r)*100/(SOIL_DRY-SOIL_WET);
}

// Lees alle sensoren samen
SensorData readSensors(){
  static bool cache=false;
  static float ct,ch;

  SensorData d;

  d.soilRaw=readSoil();
  d.soilPct=soilPct(d.soilRaw);
  d.lux=readLux();

  float h=dht.readHumidity();
  float t=dht.readTemperature();

  // cache gebruiken als DHT faalt
  if(!isnan(h)&&!isnan(t)){
    cache=true; ct=t; ch=h;
  }

  d.validDHT=cache;
  d.temperature=ct;
  d.humidity=ch;

  return d;
}

// ============================================================
// 2. LOGIC
// ============================================================

// Bepaalt status van een waarde
State range(float v,float mn,float mx){
  if(isnan(v)) return UNKNOWN;
  if(v<mn) return LOW;
  if(v>mx) return HIGH;
  return OK;
}

// Specifieke evaluatie voor bodem
State soilState(int p){
  if(p<SOIL_MIN) return LOW;
  if(p>SOIL_MAX) return HIGH;
  return OK;
}

// Licht evaluatie (nacht wordt genegeerd)
State lightState(float l){
  if(isnan(l)) return UNKNOWN;
  if(l<LIGHT_IGNORE) return IGNORED;
  return range(l,LIGHT_MIN,LIGHT_MAX);
}

// Is plant in ideale toestand?
bool plantHappy(const SensorData& d){
  if(!d.validDHT) return false;

  return range(d.temperature,TEMP_MIN,TEMP_MAX)==OK &&
         range(d.humidity,HUM_MIN,HUM_MAX)==OK &&
         soilState(d.soilPct)==OK &&
         (lightState(d.lux)==OK || lightState(d.lux)==IGNORED);
}

// Geeft advies tekst terug
const char* advice(const SensorData& d){
  if(!d.validDHT) return "DHT error";
  if(d.soilPct<SOIL_MIN) return "Water";
  if(d.soilPct>SOIL_MAX) return "Too wet";
  if(d.temperature<TEMP_MIN) return "Cold";
  if(d.temperature>TEMP_MAX) return "Hot";
  if(d.humidity<HUM_MIN) return "Dry air";
  if(d.humidity>HUM_MAX) return "Humid";
  if(d.lux<LIGHT_MIN&&d.lux>LIGHT_IGNORE) return "More light";
  if(d.lux>LIGHT_MAX) return "Too sunny";
  return "Happy plant";
}

// ============================================================
// 3. DISPLAY
// ============================================================

// Kleur afhankelijk van status
uint16_t color(State s){
  switch(s){
    case OK: return GC9A01A_GREEN;
    case LOW:
    case HIGH: return GC9A01A_RED;
    case IGNORED: return GC9A01A_CYAN;
    default: return GC9A01A_YELLOW;
  }
}

// Tekst centreren op scherm
void center(const char* t,int y,uint16_t c){
  int16_t x1,y1; uint16_t w,h;
  tft.setTextSize(2);
  tft.getTextBounds(t,0,y,&x1,&y1,&w,&h);
  tft.setCursor((240-w)/2,y);
  tft.setTextColor(c);
  tft.print(t);
}

// Simpel gezichtje (feedback UI)
void face(bool happy){
  int cx=120,cy=63,r=44;

  // hoofd
  tft.fillCircle(cx,cy,r,GC9A01A_YELLOW);
  tft.drawCircle(cx,cy,r,GC9A01A_WHITE);

  // ogen
  tft.fillCircle(cx-16,cy-10,5,GC9A01A_BLACK);
  tft.fillCircle(cx+16,cy-10,5,GC9A01A_BLACK);

  // mond
  if(happy){
    tft.drawLine(cx-23,cy+14,cx,cy+27,GC9A01A_BLACK);
    tft.drawLine(cx,cy+27,cx+23,cy+14,GC9A01A_BLACK);
  } else {
    tft.drawLine(cx-20,cy+25,cx+20,cy+25,GC9A01A_BLACK);
  }
}

// Hoofd UI functie
void drawScreen(const SensorData& d){
  tft.fillScreen(GC9A01A_BLACK);

  bool happy=plantHappy(d);

  center("Monstera",10,GC9A01A_WHITE);
  face(happy);
  center(advice(d),120,happy?GC9A01A_GREEN:GC9A01A_RED);

  // waarden tonen
  tft.setCursor(20,150);
  tft.print("Temp: "); tft.print(d.temperature);

  tft.setCursor(20,170);
  tft.print("Hum: "); tft.print(d.humidity);

  tft.setCursor(20,190);
  tft.print("Lux: "); tft.print(d.lux);

  tft.setCursor(20,210);
  tft.print("Soil: "); tft.print(d.soilPct);
}

// ============================================================
// MAIN
// ============================================================

void setup(){
  Serial.begin(115200);
  pinMode(SOIL_PIN,INPUT);

  // I2C lijnen initialiseren
  sdaRelease(); sclRelease();

  bh1750Init();
  dht.begin();

  // scherm starten
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);
}

void loop(){
  SensorData d=readSensors();

  // UI updaten
  drawScreen(d);

  // debug info
  Serial.println(advice(d));

  delay(REFRESH_MS);
}
