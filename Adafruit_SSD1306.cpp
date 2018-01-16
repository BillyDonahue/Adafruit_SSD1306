/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen below must be included in any redistribution
*********************************************************************/

#ifdef __AVR__
  #include <avr/pgmspace.h>
#elif defined(ESP8266) || defined(ESP32)
 #include <pgmspace.h>
#else
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif

#if !defined(__ARM_ARCH) && !defined(ENERGIA) && !defined(ESP8266) && !defined(ESP32) && !defined(__arc__)
 #include <util/delay.h>
#endif

#include <stdlib.h>

#include <Wire.h>
#include <SPI.h>
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

namespace {
void swap(int16_t& a, int16_t& b) { int16_t t = a; a = b; b = t; }

struct FastI2CGuard {
#ifdef TWBR
  FastI2CGuard() : t(TWBR) {
    TWBR = 12;  // upgrade to 400kHz!
  }
  ~FastI2CGuard() {
    TWBR = t;
  }
  uint8_t t;  // save I2C bitrate
#endif
};
}  // namespace

Adafruit_SSD1306_Core::Connection::Connection(int8_t sid, int8_t sclk, int8_t dc, int8_t rst, int8_t cs)
  : rst(rst), _hw(new Spi(sid, sclk, dc, cs)) {}
Adafruit_SSD1306_Core::Connection::Connection(int8_t dc, int8_t rst, int8_t cs)
  : rst(rst), _hw(new Spi(dc, cs)) {}
Adafruit_SSD1306_Core::Connection::Connection(int8_t rst)
  : rst(rst), _hw(new I2c()) {}
Adafruit_SSD1306_Core::Connection::Connection()
  : _hw(new I2c()) {}
Adafruit_SSD1306_Core::Connection::~Connection() {
  delete _hw;
}

// the most basic function, set a single pixel
void Adafruit_SSD1306_Core::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height()))
    return;

  // check rotation, move pixel around if necessary
  switch (getRotation()) {
    case 1:
      swap(x, y);
      x = WIDTH - x - 1;
      break;
    case 2:
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      break;
    case 3:
      swap(x, y);
      y = HEIGHT - y - 1;
      break;
  }

  // x is which column
  uint8_t& byte = _personality.buffer[x + (y / 8) * WIDTH];
  uint8_t mask = 1 << (y & 7);
  switch (color) {
    case WHITE:   byte |=  mask; break;
    case BLACK:   byte &= ~mask; break;
    case INVERSE: byte ^=  mask; break;
  }
}

Adafruit_SSD1306_Core::Adafruit_SSD1306_Core(Personality p, Connection conn)
  : Adafruit_GFX(p.w, p.h), _personality(p), _conn(conn) {}

void Adafruit_SSD1306_Core::Connection::Spi::begin(uint8_t) {
  dc.outputMode();
  cs.outputMode();
  if (hwSPI) {
    SPI.begin();
#ifdef SPI_HAS_TRANSACTION
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
#else
    SPI.setClockDivider(4);
#endif
  } else {
    // set pins for software-SPI
    sid.outputMode();
    sclk.outputMode();
  }
}

void Adafruit_SSD1306_Core::Connection::I2c::begin(uint8_t i2caddr) {
  _i2caddr = i2caddr;
  // I2C Init
  Wire.begin();
#ifdef __SAM3X8E__
  // Force 400 kHz I2C, rawr! (Uses pins 20, 21 for SDA, SCL)
  TWI1->TWI_CWGR = 0;
  TWI1->TWI_CWGR = ((VARIANT_MCK / (2 * 400000)) - 4) * 0x101;
#endif
}

void Adafruit_SSD1306_Core::Connection::begin(uint8_t i2caddr) {
  _hw->begin(i2caddr);
}

void Adafruit_SSD1306_Core::begin(uint8_t vccstate, uint8_t i2caddr, bool reset) {
  _vccstate = vccstate;
  _conn.begin(i2caddr);

  if (reset && _conn.rst.connected()) {
    _conn.rst.outputMode(); // Setup reset pin direction (used by both SPI and I2C)
    _conn.rst = 1;
    delay(1);           // VDD (3.3V) goes high at start, let's just chill for a ms
    _conn.rst = 0;      // bring reset low
    delay(10);          // wait 10ms
    _conn.rst = 1;      // bring out of reset
    // turn on VCC (9V?)
  }

  const bool extvcc = (_vccstate == SSD1306_EXTERNALVCC);
  uint8_t compins = _personality.compins;
  uint8_t contrast = extvcc ? _personality.contrast_extvcc : _personality.contrast;

  // Init sequence
  ssd1306_command(SSD1306_DISPLAYOFF);                    // 0xAE
  ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
  ssd1306_command(0x80);                                  // the suggested ratio 0x80

  ssd1306_command(SSD1306_SETMULTIPLEX);                  // 0xA8
  ssd1306_command(HEIGHT - 1);

  ssd1306_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
  ssd1306_command(0x0);                                   // no offset
  ssd1306_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
  ssd1306_command(SSD1306_CHARGEPUMP);                    // 0x8D
  ssd1306_command(extvcc ? 0x10 : 0x14);
  ssd1306_command(SSD1306_MEMORYMODE);                    // 0x20
  ssd1306_command(0x00);                                  // 0x0 act like ks0108
  ssd1306_command(SSD1306_SEGREMAP | 0x1);
  ssd1306_command(SSD1306_COMSCANDEC);
  ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
  ssd1306_command(compins);

  ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
  ssd1306_command(contrast);

  ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9
  ssd1306_command(extvcc ? 0x22 : 0xF1);
  ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
  ssd1306_command(0x40);
  ssd1306_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
  ssd1306_command(SSD1306_NORMALDISPLAY);                 // 0xA6

  ssd1306_command(SSD1306_DEACTIVATE_SCROLL);

  ssd1306_command(SSD1306_DISPLAYON); //--turn on oled panel
}


void Adafruit_SSD1306_Core::invertDisplay(boolean i) {
  if (i) {
    ssd1306_command(SSD1306_INVERTDISPLAY);
  } else {
    ssd1306_command(SSD1306_NORMALDISPLAY);
  }
}

void Adafruit_SSD1306_Core::Connection::Spi::command(uint8_t c) {
  // SPI
  cs = 1;
  dc = 0;
  cs = 0;
  fastSPIwrite(c);
  cs = 1;
}

void Adafruit_SSD1306_Core::Connection::I2c::command(uint8_t c) {
  uint8_t control = 0x00;   // Co = 0, D/C = 0
  Wire.beginTransmission(_i2caddr);
  Wire.write(control);
  Wire.write(c);
  Wire.endTransmission();
}

void Adafruit_SSD1306_Core::Connection::command(uint8_t c) {
  _hw->command(c);
}

void Adafruit_SSD1306_Core::ssd1306_command(uint8_t c) {
  _conn.command(c);
}

// startscrollright
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void Adafruit_SSD1306_Core::startscrollright(uint8_t start, uint8_t stop) {
  ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X00);
  ssd1306_command(0XFF);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startscrollleft
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void Adafruit_SSD1306_Core::startscrollleft(uint8_t start, uint8_t stop) {
  ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X00);
  ssd1306_command(0XFF);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagright
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void Adafruit_SSD1306_Core::startscrolldiagright(uint8_t start, uint8_t stop) {
  ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
  ssd1306_command(0X00);
  ssd1306_command(HEIGHT);
  ssd1306_command(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X01);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagleft
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F)
void Adafruit_SSD1306_Core::startscrolldiagleft(uint8_t start, uint8_t stop) {
  ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
  ssd1306_command(0X00);
  ssd1306_command(HEIGHT);
  ssd1306_command(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X01);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

void Adafruit_SSD1306_Core::stopscroll(void){
  ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
}

// Dim the display
// dim = true: display is dimmed
// dim = false: display is normal
void Adafruit_SSD1306_Core::dim(boolean dim) {
  uint8_t contrast;
  if (dim) {
    contrast = 0; // Dimmed display
  } else {
    if (_vccstate == SSD1306_EXTERNALVCC) {
      contrast = 0x9F;
    } else {
      contrast = 0xCF;
    }
  }
  // the range of contrast to too small to be really useful
  // it is useful to dim the display
  ssd1306_command(SSD1306_SETCONTRAST);
  ssd1306_command(contrast);
}

void Adafruit_SSD1306::Connection::writeBuffer(const uint8_t *buf, uint16_t n) {
  _hw->writeBuffer(buf, n);
}

void Adafruit_SSD1306::Connection::Spi::writeBuffer(const uint8_t *buf, uint16_t n) {
  cs = 1;
  dc = 1;
  cs = 0;
  for (uint16_t i = 0; i < n; i++) {
    fastSPIwrite(buf[i]);
  }
  cs = 1;
}
void Adafruit_SSD1306::Connection::I2c::writeBuffer(const uint8_t *buf, uint16_t n) {
  FastI2CGuard i2cTurbo;
  for (uint16_t i = 0; i < n; i++) {
    // send a bunch of data in one xmission
    Wire.beginTransmission(_i2caddr);
    WIRE_WRITE(0x40);
    for (uint8_t x = 0; x < 16; x++) {
      WIRE_WRITE(buf[i]);
      i++;
    }
    i--;
    Wire.endTransmission();
  }
}

void Adafruit_SSD1306_Core::display(void) {
  ssd1306_command(SSD1306_COLUMNADDR);
  ssd1306_command(0);   // Column start address (0 = reset)
  ssd1306_command(WIDTH-1); // Column end address (127 = reset)

  ssd1306_command(SSD1306_PAGEADDR);
  ssd1306_command(0); // Page start address (0 = reset)
  ssd1306_command((HEIGHT>>3) - 1); // Page end address

  _conn.writeBuffer(_personality.buffer, bufsize());
}

// clear everything
void Adafruit_SSD1306_Core::clearDisplay(void) {
  memset(_personality.buffer, 0, bufsize());
}

inline void Adafruit_SSD1306_Core::Connection::Spi::fastSPIwrite(uint8_t d) {
  if (hwSPI) {
    (void)SPI.transfer(d);
  } else {
    for (uint8_t bit = 0x80; bit; bit >>= 1) {
      sclk = 0;
      sid = d & bit;
      sclk = 1;
    }
  }
}

void Adafruit_SSD1306_Core::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  boolean bSwap = false;
  switch (rotation) {
    case 0:
      // 0 degree rotation, do nothing
      break;
    case 1:
      // 90 degree rotation, swap x & y for rotation, then invert x
      bSwap = true;
      swap(x, y);
      x = WIDTH - x - 1;
      break;
    case 2:
      // 180 degree rotation, invert x and y - then shift y around for height.
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      x -= (w-1);
      break;
    case 3:
      // 270 degree rotation, swap x & y for rotation, then invert y  and adjust y for w (not to become h)
      bSwap = true;
      swap(x, y);
      y = HEIGHT - y - 1;
      y -= (w-1);
      break;
  }

  if(bSwap) {
    drawFastVLineInternal(x, y, w, color);
  } else {
    drawFastHLineInternal(x, y, w, color);
  }
}

void Adafruit_SSD1306_Core::drawFastHLineInternal(int16_t x, int16_t y, int16_t w, uint16_t color) {
  // Do bounds/limit checks
  if (y < 0 || y >= HEIGHT) { return; }

  // make sure we don't try to draw below 0
  if (x < 0) {
    w += x;
    x = 0;
  }

  // make sure we don't go off the edge of the display
  if (x + w > WIDTH) {
    w = WIDTH - x;
  }

  // if our width is now negative, punt
  if (w <= 0) { return; }

  // set up the pointer for movement through the buffer
  register uint8_t *pBuf = _personality.buffer + (y / 8) * WIDTH + x;
  register uint8_t mask = 1 << (y&7);

  switch (color) {
    case WHITE:
      while (w--) {
        *pBuf++ |= mask;
      };
      break;
    case BLACK:
      mask = ~mask;
      while (w--) {
        *pBuf++ &= mask;
      };
      break;
    case INVERSE:
      while (w--) {
        *pBuf++ ^= mask;
      };
      break;
  }
}

void Adafruit_SSD1306_Core::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  bool bSwap = false;
  switch (rotation) {
    case 0:
      break;
    case 1:
      // 90 degree rotation, swap x & y for rotation, then invert x and adjust x for h (now to become w)
      bSwap = true;
      swap(x, y);
      x = WIDTH - x - 1;
      x -= (h-1);
      break;
    case 2:
      // 180 degree rotation, invert x and y - then shift y around for height.
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      y -= (h-1);
      break;
    case 3:
      // 270 degree rotation, swap x & y for rotation, then invert y
      bSwap = true;
      swap(x, y);
      y = HEIGHT - y - 1;
      break;
  }

  if (bSwap) {
    drawFastHLineInternal(x, y, h, color);
  } else {
    drawFastVLineInternal(x, y, h, color);
  }
}

void Adafruit_SSD1306_Core::drawFastVLineInternal(int16_t x, int16_t y, int16_t h, uint16_t color) {
  // do nothing if we're off the left or right side of the screen
  if (x < 0 || x >= WIDTH) { return; }

  // make sure we don't try to draw below 0
  if (y < 0) {
    // y is negative, this will subtract enough from h to account for y being 0
    h += y;
    y = 0;
  }

  // make sure we don't go past the height of the display
  if (y + h > HEIGHT) {
    h = HEIGHT - y;
  }

  // if our height is now negative, punt
  if (h <= 0) {
    return;
  }

  // this display doesn't need ints for coordinates, use local byte registers for faster juggling
  register uint8_t y_reg = y;
  register uint8_t h_reg = h;

  // set up the pointer for fast movement through the buffer
  register uint8_t *pBuf = _personality.buffer;
  // adjust the buffer pointer for the current row
  pBuf += (y_reg / 8) * WIDTH;
  // and offset x columns in
  pBuf += x;

  // do the first partial byte, if necessary - this requires some masking
  register uint8_t mod = y_reg & 7;
  if (mod) {
    // mask off the high n bits we want to set
    mod = 8 - mod;

    // note - lookup table results in a nearly 10% performance improvement in fill* functions
    // register uint8_t mask = ~(0xFF >> (mod));
    static uint8_t premask[8] = {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
    register uint8_t mask = premask[mod];

    // adjust the mask if we're not going to reach the end of this byte
    if (h_reg < mod) {
      mask &= (0XFF >> (mod - h_reg));
    }

    switch (color) {
      case WHITE:   *pBuf |=  mask;  break;
      case BLACK:   *pBuf &= ~mask;  break;
      case INVERSE: *pBuf ^=  mask;  break;
    }

    // fast exit if we're done here!
    if (h_reg < mod) { return; }

    h_reg -= mod;

    pBuf += WIDTH;
  }

  // write solid bytes while we can - effectively doing 8 rows at a time
  if (h_reg >= 8) {
    if (color == INVERSE)  {
      // separate copy of the code so we don't impact performance of the
      // black/white write version with an extra comparison per loop
      do {
        *pBuf = ~*pBuf;

        // adjust the buffer forward 8 rows worth of data
        pBuf += WIDTH;

        // adjust h & y (there's got to be a faster way for me to do this, but this should still help a fair bit for now)
        h_reg -= 8;
      } while (h_reg >= 8);
    } else {
      // store a local value to work with
      register uint8_t val = (color == WHITE) ? 255 : 0;

      do {
        // write our value in
        *pBuf = val;

        // adjust the buffer forward 8 rows worth of data
        pBuf += WIDTH;

        // adjust h & y (there's got to be a faster way for me to do this, but this should still help a fair bit for now)
        h_reg -= 8;
      } while (h_reg >= 8);
    }
  }

  // now do the final partial byte, if necessary
  if (h_reg) {
    mod = h_reg & 7;
    // this time we want to mask the low bits of the byte, vs the high bits we did above
    // register uint8_t mask = (1 << mod) - 1;
    // note - lookup table results in a nearly 10% performance improvement in fill* functions
    static const uint8_t postmask[8] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};
    register uint8_t mask = postmask[mod];
    switch (color) {
      case WHITE:   *pBuf |=  mask;  break;
      case BLACK:   *pBuf &= ~mask;  break;
      case INVERSE: *pBuf ^=  mask;  break;
    }
  }
}
