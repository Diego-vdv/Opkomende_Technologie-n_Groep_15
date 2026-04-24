#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
/*
  Simpelste test voor jouw ronde GC9A01 scherm.
  Aangesloten volgens je foto:
  - CS  -> D12
  - DC  -> D11
  - SDA -> D10
  - SCL -> D9
  - GND -> GND
  - VCC -> 3.3V
  - RST -> niet aangesloten
  Omdat SDA/SCL in je foto niet op de standaard SPI-pinnen lijken te zitten,
  gebruiken we hier software SPI.
*/
constexpr int TFT_CS = 12;
constexpr int TFT_DC = 11;
constexpr int TFT_MOSI = 10;
constexpr int TFT_SCLK = 9;
constexpr int TFT_RST = -1;
Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
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
void setup() {
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(GC9A01A_BLACK);
  drawHappySmiley();
}
void loop() {
}
