#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

/*
  Simpele test voor GC9A01 rond display (software SPI)
  Aansluitingen:
  CS   -> D12
  DC   -> D11
  MOSI -> D10
  SCLK -> D9
  VCC  -> 3.3V
  GND  -> GND
*/

//Pin Configuratie
constexpr int TFT_CS = 12;
constexpr int TFT_DC = 11;
constexpr int TFT_MOSI = 10;
constexpr int TFT_SCLK = 9;
constexpr int TFT_RST = -1; // Niet aangesloten

// TFT object
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Functie: Smiley tekenen
void drawHappySmiley() {
  tft.fillCircle(120, 120, 120, GC9A01A_YELLOW);
  tft.drawCircle(120, 120, 90, GC9A01A_WHITE);
  tft.fillCircle(90, 95, 8, GC9A01A_BLACK);
  tft.fillCircle(150, 95, 8, GC9A01A_BLACK);
  for (int i = 0; i < 3; i++) {
    tft.drawLine(82, 138 + i, 102, 156 + i, GC9A01A_BLACK);
    tft.drawLine(102, 156 + i, 138, 156 + i, GC9A01A_BLACK);
    tft.drawLine(138, 156 + i, 158, 138 + i, GC9A01A_BLACK);
  }
}

// Setup
void setup() {
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);
  drawHappySmiley();
}

// Loop (niet gebruikt)
void loop() {
}
