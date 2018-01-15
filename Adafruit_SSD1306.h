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
All text above, and the splash screen must be included in any redistribution
*********************************************************************/
#ifndef _Adafruit_SSD1306_H_
#define _Adafruit_SSD1306_H_

#if ARDUINO >= 100
 #include "Arduino.h"
 #define WIRE_WRITE Wire.write
#else
 #include "WProgram.h"
  #define WIRE_WRITE Wire.send
#endif

#if defined(__SAM3X8E__)
 typedef volatile RwReg PortReg;
 typedef uint32_t PortMask;
 #define HAVE_PORTREG
#elif defined(ARDUINO_ARCH_SAMD)
// not supported
#elif defined(ESP8266) || defined(ESP32) || defined(ARDUINO_STM32_FEATHER) || defined(__arc__)
  typedef volatile uint32_t PortReg;
  typedef uint32_t PortMask;
#elif defined(__AVR__)
  typedef volatile uint8_t PortReg;
  typedef uint8_t PortMask;
  #define HAVE_PORTREG
#else
  // chances are its 32 bit so assume that
  typedef volatile uint32_t PortReg;
  typedef uint32_t PortMask;
#endif

#include <SPI.h>
#include <Adafruit_GFX.h>

#define BLACK 0
#define WHITE 1
#define INVERSE 2

#define SSD1306_I2C_ADDRESS   0x3C  // 011110+SA0+RW - 0x3C or 0x3D
// Address for 128x32 is 0x3C
// Address for 128x64 is 0x3D (default) or 0x3C (if SA0 is grounded)

#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF

#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA

#define SSD1306_SETVCOMDETECT 0xDB

#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9

#define SSD1306_SETMULTIPLEX 0xA8

#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10

#define SSD1306_SETSTARTLINE 0x40

#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22

#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8

#define SSD1306_SEGREMAP 0xA0

#define SSD1306_CHARGEPUMP 0x8D

#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2

// Scrolling #defines
#define SSD1306_ACTIVATE_SCROLL 0x2F
#define SSD1306_DEACTIVATE_SCROLL 0x2E
#define SSD1306_SET_VERTICAL_SCROLL_AREA 0xA3
#define SSD1306_RIGHT_HORIZONTAL_SCROLL 0x26
#define SSD1306_LEFT_HORIZONTAL_SCROLL 0x27
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL 0x2A

class Adafruit_SSD1306_Core : public Adafruit_GFX {
public:
  struct Personality {
    uint8_t w;
    uint8_t h;
    uint8_t* buffer;
    uint8_t compins;
    uint8_t contrast_extvcc;
    uint8_t contrast;
  };
  struct Connection {
    Connection(int8_t sid, int8_t sclk, int8_t dc, int8_t rst, int8_t cs)
      : sid(sid), sclk(sclk), dc(dc), rst(rst), cs(cs), hwSPI(false) {}
    Connection(int8_t dc, int8_t rst, int8_t cs)
      : sid(-1), sclk(-1), dc(dc), rst(rst), cs(cs), hwSPI(true) {}
    Connection(int8_t rst)
      : sid(-1), sclk(-1), dc(-1), rst(rst), cs(-1) {}

    int8_t sid, sclk, dc, rst, cs;
    boolean hwSPI;
#ifdef HAVE_PORTREG
    PortReg *mosiport, *clkport, *csport, *dcport;
    PortMask mosipinmask, clkpinmask, cspinmask, dcpinmask;
#endif
  };
  Adafruit_SSD1306_Core(Personality p, Connection conn);

  void startscrollright(uint8_t start, uint8_t stop);
  void startscrollleft(uint8_t start, uint8_t stop);
  void startscrolldiagright(uint8_t start, uint8_t stop);
  void startscrolldiagleft(uint8_t start, uint8_t stop);
  void stopscroll(void);

  void begin(uint8_t switchvcc = SSD1306_SWITCHCAPVCC, uint8_t i2caddr = SSD1306_I2C_ADDRESS, bool reset=true);
  void ssd1306_command(uint8_t c);

  void clearDisplay(void);
  void invertDisplay(boolean i) override;
  void display();

  void dim(boolean dim);

  void drawPixel(int16_t x, int16_t y, uint16_t color) override;

  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;

private:
  void fastSPIwrite(uint8_t c);
  inline void drawFastVLineInternal(int16_t x, int16_t y, int16_t h, uint16_t color) __attribute__((always_inline));
  inline void drawFastHLineInternal(int16_t x, int16_t y, int16_t w, uint16_t color) __attribute__((always_inline));

  uint16_t bufsize() { return _personality.w * _personality.h / 8; }

  Personality _personality;
  Connection _conn;
  int8_t _i2caddr;
  int8_t _vccstate;
};

template <typename D>
class Adafruit_SSD1306_Basic : public Adafruit_SSD1306_Core {
public:
  // software SPI
  Adafruit_SSD1306_Basic(int8_t sid_pin, int8_t sclk_pin, int8_t dc_pin, int8_t rst_pin, int8_t cs_pin)
    : Adafruit_SSD1306_Core(D::personality(), Connection{sid_pin, sclk_pin, dc_pin, rst_pin, cs_pin}) {}
  // hardware SPI - we indicate DataCommand, ChipSelect, Reset
  Adafruit_SSD1306_Basic(int8_t dc_pin, int8_t rst_pin, int8_t cs_pin)
    : Adafruit_SSD1306_Core(D::personality(), Connection{dc_pin, rst_pin, cs_pin}) {}
  // I2C - we only indicate the reset pin!
  explicit Adafruit_SSD1306_Basic(int8_t rst_pin)
    : Adafruit_SSD1306_Core(D::personality(), Connection{rst_pin}) {}
  // I2C without reset
  Adafruit_SSD1306_Basic() : Adafruit_SSD1306_Basic(-1) {}

 private:
  D& asD() { return *this; }
  const D& asD() const { return *this; }
};

class Adafruit_SSD1306_96x16
    : public Adafruit_SSD1306_Basic<Adafruit_SSD1306_96x16> {
  using Base = Adafruit_SSD1306_Basic<Adafruit_SSD1306_96x16>;
public:
  using Base::Base;
  static uint8_t* const buffer;
  static Personality personality() {
    return {96, 16, buffer, 0x02 /*ada 0x12*/, 0x10, 0xAF};
  }
};

class Adafruit_SSD1306_128x32
    : public Adafruit_SSD1306_Basic<Adafruit_SSD1306_128x32> {
  using Base = Adafruit_SSD1306_Basic<Adafruit_SSD1306_128x32>;
public:
  using Base::Base;
  static uint8_t* const buffer;
  static Personality personality() {
    return {128, 32, buffer, 0x02, 0x8F, 0x8F};
  }
};

class Adafruit_SSD1306_128x64
    : public Adafruit_SSD1306_Basic<Adafruit_SSD1306_128x64> {
  using Base = Adafruit_SSD1306_Basic<Adafruit_SSD1306_128x64>;
public:
  using Base::Base;
  static uint8_t* const buffer;
  static Personality personality() {
    return {128, 64, buffer, 0x12, 0x9F, 0xCF};
  }
};

// The default device, for backward compatibility, is the 128x32 device.
using Adafruit_SSD1306 = Adafruit_SSD1306_128x32;

#endif /* _Adafruit_SSD1306_H_ */
