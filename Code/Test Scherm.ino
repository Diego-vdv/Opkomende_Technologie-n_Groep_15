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

// Display configuratie
constexpr int SCREEN_CENTER_X = 120;
constexpr int SCREEN_CENTER_Y = 120;
constexpr int FACE_RADIUS     = 120;
constexpr int BORDER_RADIUS   = 90;

// Ogen
constexpr int EYE_OFFSET_X = 30;
constexpr int EYE_POS_Y    = 95;
constexpr int EYE_RADIUS   = 8;

// Mond
constexpr int MOUTH_TOP_Y    = 138;
constexpr int MOUTH_BOTTOM_Y = 156;
constexpr int MOUTH_THICKNESS = 3;

// TFT object
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Functie: Smiley tekenen
void drawHappySmiley() {

  // Gezicht (achtergrond)
  tft.fillCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, FACE_RADIUS, GC9A01A_YELLOW);
  // Rand
  tft.drawCircle(SCREEN_CENTER_X, SCREEN_CENTER_Y, BORDER_RADIUS, GC9A01A_WHITE);
  // Ogen
  tft.fillCircle(SCREEN_CENTER_X - EYE_OFFSET_X, EYE_POS_Y, EYE_RADIUS, GC9A01A_BLACK);
  tft.fillCircle(SCREEN_CENTER_X + EYE_OFFSET_X, EYE_POS_Y, EYE_RADIUS, GC9A01A_BLACK);
  // Mond (iets dikker gemaakt met meerdere lijnen)
  for (int i = 0; i < MOUTH_THICKNESS; i++) {
    int yOffset = i;
    tft.drawLine(82,  MOUTH_TOP_Y    + yOffset, 102, MOUTH_BOTTOM_Y + yOffset, GC9A01A_BLACK);
    tft.drawLine(102, MOUTH_BOTTOM_Y + yOffset, 138, MOUTH_BOTTOM_Y + yOffset, GC9A01A_BLACK);
    tft.drawLine(138, MOUTH_BOTTOM_Y + yOffset, 158, MOUTH_TOP_Y    + yOffset, GC9A01A_BLACK);
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
