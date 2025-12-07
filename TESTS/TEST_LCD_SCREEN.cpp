#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>


#define TFT_CS   10
#define TFT_DC    7   
#define TFT_RST   9
#define TFT_BL    8

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

int myNumber = 42;


void displayNumber(int value) {
  tft.fillScreen(ST77XX_BLACK); // black background
  char buf[16];
  sprintf(buf, "%d", value);
  tft.setTextWrap(false);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(5);
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);

  int16_t x = (240 - w) / 2;  // screen width = 240
  int16_t y = (280 - h) / 2;  // screen height = 280

  tft.setCursor(x, y);
  tft.print(buf);
}

void setup() {
  // Turn on backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init(240, 280);   // important: (width, height) screen width 240 , height 280
  tft.setRotation(0);   // try 0–3 if orientation is weird

  // Simple test background
  tft.fillScreen(ST77XX_BLUE);

  // Draw the number
  displayNumber(myNumber);
}


