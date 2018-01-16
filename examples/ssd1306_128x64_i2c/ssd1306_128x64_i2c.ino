/*********************************************************************
  This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

  This example is for a 128x64 size display using I2C to communicate
  3 pins are required to interface (2 I2C and one reset)

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada  for Adafruit Industries.
  BSD license, check license.txt for more information
  All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET_1 4
#define OLED_RESET_2 3
Adafruit_SSD1306_128x64 display1(OLED_RESET_1);
Adafruit_SSD1306_128x64 display2(OLED_RESET_2);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};

void runTest(Adafruit_SSD1306_Core& display);

void setup() {
  Serial.begin(9600);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display1.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  display2.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 2nd 128x64)
  // init done

  bool ok = true;
  for (int test_id = 0; ok; test_id++) {
    ok = ok && runTest(display1, test_id);
    ok = ok && runTest(display2, test_id);
  }
}



void loop() {
  // draw a bitmap icon and 'animate' movement
  testdrawbitmap(display1, logo16_glcd_bmp, LOGO16_GLCD_HEIGHT, LOGO16_GLCD_WIDTH);
}


void testdrawbitmap(Adafruit_SSD1306_Core& display, const uint8_t *bitmap, uint8_t w, uint8_t h) {
  uint8_t icons[NUMFLAKES][3];

  // initialize
  for (uint8_t f = 0; f < NUMFLAKES; f++) {
    icons[f][XPOS] = random(display.width());
    icons[f][YPOS] = 0;
    icons[f][DELTAY] = random(5) + 1;

    Serial.print("x: ");
    Serial.print(icons[f][XPOS], DEC);
    Serial.print(" y: ");
    Serial.print(icons[f][YPOS], DEC);
    Serial.print(" dy: ");
    Serial.println(icons[f][DELTAY], DEC);
  }

  Adafruit_SSD1306_Core* a = &display1;
  Adafruit_SSD1306_Core* b = &display2;
  
  while (1) {
    Adafruit_SSD1306_Core& display = *a;
    auto t = a;
    a = b;
    b = t;

    // draw each icon
    for (uint8_t f = 0; f < NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, WHITE);
    }
    display.display();
    delay(100);

    // then erase it + move it
    for (uint8_t f = 0; f < NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, BLACK);
      // move it
      icons[f][YPOS] += icons[f][DELTAY];
      // if its gone, reinit
      if (icons[f][YPOS] > display.height()) {
        icons[f][XPOS] = random(display.width());
        icons[f][YPOS] = 0;
        icons[f][DELTAY] = random(5) + 1;
      }
    }
  }
}


void testdrawchar(Adafruit_SSD1306_Core& display) {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  for (uint8_t i = 0; i < 168; i++) {
    if (i == '\n') continue;
    display.write(i);
    if ((i > 0) && (i % 21 == 0))
      display.println();
  }
  display.display();
  delay(1);
}

void testdrawcircle(Adafruit_SSD1306_Core& display) {
  for (int16_t i = 0; i < display.height(); i += 2) {
    display.drawCircle(display.width() / 2, display.height() / 2, i, WHITE);
    display.display();
    delay(1);
  }
}

void testfillrect(Adafruit_SSD1306_Core& display) {
  uint8_t color = 1;
  for (int16_t i = 0; i < display.height() / 2; i += 3) {
    // alternate colors
    display.fillRect(i, i, display.width() - i * 2, display.height() - i * 2, color % 2);
    display.display();
    delay(1);
    color++;
  }
}

void testdrawtriangle(Adafruit_SSD1306_Core& display) {
  for (int16_t i = 0; i < min(display.width(), display.height()) / 2; i += 5) {
    display.drawTriangle(display.width() / 2, display.height() / 2 - i,
                         display.width() / 2 - i, display.height() / 2 + i,
                         display.width() / 2 + i, display.height() / 2 + i, WHITE);
    display.display();
    delay(1);
  }
}

void testfilltriangle(Adafruit_SSD1306_Core& display) {
  uint8_t color = WHITE;
  for (int16_t i = min(display.width(), display.height()) / 2; i > 0; i -= 5) {
    display.fillTriangle(display.width() / 2, display.height() / 2 - i,
                         display.width() / 2 - i, display.height() / 2 + i,
                         display.width() / 2 + i, display.height() / 2 + i, WHITE);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
    delay(1);
  }
}

void testdrawroundrect(Adafruit_SSD1306_Core& display) {
  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2) {
    display.drawRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, WHITE);
    display.display();
    delay(1);
  }
}

void testfillroundrect(Adafruit_SSD1306_Core& display) {
  uint8_t color = WHITE;
  for (int16_t i = 0; i < display.height() / 2 - 2; i += 2) {
    display.fillRoundRect(i, i, display.width() - 2 * i, display.height() - 2 * i, display.height() / 4, color);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
    delay(1);
  }
}

void testdrawrect(Adafruit_SSD1306_Core& display) {
  for (int16_t i = 0; i < display.height() / 2; i += 2) {
    display.drawRect(i, i, display.width() - 2 * i, display.height() - 2 * i, WHITE);
    display.display();
    delay(1);
  }
}

void testdrawline(Adafruit_SSD1306_Core& display) {
  for (int16_t i = 0; i < display.width(); i += 4) {
    display.drawLine(0, 0, i, display.height() - 1, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i = 0; i < display.height(); i += 4) {
    display.drawLine(0, 0, display.width() - 1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i = 0; i < display.width(); i += 4) {
    display.drawLine(0, display.height() - 1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i = display.height() - 1; i >= 0; i -= 4) {
    display.drawLine(0, display.height() - 1, display.width() - 1, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i = display.width() - 1; i >= 0; i -= 4) {
    display.drawLine(display.width() - 1, display.height() - 1, i, 0, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i = display.height() - 1; i >= 0; i -= 4) {
    display.drawLine(display.width() - 1, display.height() - 1, 0, i, WHITE);
    display.display();
    delay(1);
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i = 0; i < display.height(); i += 4) {
    display.drawLine(display.width() - 1, 0, 0, i, WHITE);
    display.display();
    delay(1);
  }
  for (int16_t i = 0; i < display.width(); i += 4) {
    display.drawLine(display.width() - 1, 0, i, display.height() - 1, WHITE);
    display.display();
    delay(1);
  }
  delay(250);
}

void testscrolltext(Adafruit_SSD1306_Core& display) {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 0);
  display.clearDisplay();
  display.println("scroll");
  display.display();
  delay(1);

  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
}

bool runTest(Adafruit_SSD1306_Core& display, int test_id) {
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  switch (test_id) {
    case 0:
      display.ssd1306_command(SSD1306_DISPLAYOFF);
      delay(2000);
      display.ssd1306_command(SSD1306_DISPLAYON);
      delay(2000);
      // Clear the buffer.
      display.clearDisplay();
      return true;
    case 1:
      // draw a single pixel
      display.drawPixel(10, 10, WHITE);
      // Show the display buffer on the hardware.
      // NOTE: You _must_ call display after making any drawing commands
      // to make them visible on the display hardware!
      display.display();
      delay(2000);
      display.clearDisplay();
      return true;
    case 2:
      // draw many lines
      testdrawline(display);
      display.display();
      delay(2000);
      display.clearDisplay();
      return true;
    case 3:

      // draw rectangles
      testdrawrect(display);
      display.display();
      delay(2000);
      display.clearDisplay();
      return true;
    case 4:
      // draw multiple rectangles
      testfillrect(display);
      display.display();
      delay(2000);
      display.clearDisplay();
      return true;
    case 5:
      // draw mulitple circles
      testdrawcircle(display);
      display.display();
      delay(2000);
      display.clearDisplay();
      return true;

    case 6:
      // draw a white circle, 10 pixel radius
      display.fillCircle(display.width() / 2, display.height() / 2, 10, WHITE);
      display.display();
      delay(2000);
      display.clearDisplay();
      return true;

    case 7:
      testdrawroundrect(display);
      delay(2000);
      display.clearDisplay();
      return true;

    case 8:
      testfillroundrect(display);
      delay(2000);
      display.clearDisplay();
      return true;

    case 9:
      testdrawtriangle(display);
      delay(2000);
      display.clearDisplay();
      return true;

    case 10:
      testfilltriangle(display);
      delay(2000);
      display.clearDisplay();
      return true;

    case 11:
      // draw the first ~12 characters in the font
      testdrawchar(display);
      display.display();
      delay(2000);
      display.clearDisplay();
      return true;

    case 12:
      // draw scrolling text
      testscrolltext(display);
      delay(2000);
      display.clearDisplay();
      return true;

    case 13:
      // text display tests
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0, 0);
      display.println("Hello, world!");
      display.setTextColor(BLACK, WHITE); // 'inverted' text
      display.println(3.141592);
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.print("0x"); display.println(0xDEADBEEF, HEX);
      display.display();
      delay(2000);
      display.clearDisplay();
      return true;

    case 14:
      // miniature bitmap display
      display.drawBitmap(30, 16,  logo16_glcd_bmp, 16, 16, 1);
      display.display();
      delay(1);
      return true;

    case 15:
      // invert the display
      display.invertDisplay(true);
      delay(1000);
      display.invertDisplay(false);
      delay(1000);
      display.clearDisplay();
      return true;
    default:
      break;
  }

  return false;
}

