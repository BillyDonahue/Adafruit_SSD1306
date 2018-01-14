#!/usr/bin/env python

# Convert a 128x64 bmp file into a C++ static initializer
# usable as a splash screen for the Adafruit_SSD1306 driver.
#
#     python make_splash.py splash.bmp

import sys

# $ pip install --user Pillow
from PIL import Image

def makeMatrix(im):
  bits = []
  for y in range(64):
    row = []
    for x in range(128):
      v = 1
      if im.getpixel((x,y)): v = 0
      row.append(v)
    bits.append(row)
  return bits

def dumpBmp(f, bits):
  f.write('/*\n')
  for row in bits:
    for c in row:
      f.write('{:d}'.format(c)),
    f.write('\n')
  f.write('*/\n')

# Every byte is a 1x8 vertical strip
def verticalBytes(bits):
  w = len(bits[0])
  h = len(bits)
  out = [0] * (w * int(h/8))
  for j in range(h):
    for i in range(w):
      if bits[j][i]:
        idx = i + int(j/8)*w
        bit = j & 7
        out[idx] = out[idx] | (1 << bit)
  return out

def arrayDefinition(out, w, h):
  decl = str()
  decl += "uint8_t Adafruit_SSD1306_Driver::Splash<{0}, {1}>::buffer[{0} * {1} / 8] = {{\n".format(w, h)
  for j in range(int(h / 8)):
    for i in range(w):
      ir = j * w + i
      if ir % 16 == 0:
        if ir != 0:
          decl += '\n'
      else:
        decl += ' '
      decl += '0x{:02X},'.format(out[ir])
  decl = decl[:-1]  # trim trailing comma
  decl += '};\n'
  return decl

def main(argv):
  im = Image.open(argv[1])
  bits = makeMatrix(im)
  # dumpBmp(sys.stdout, bits)
  out = verticalBytes(bits)
  print(arrayDefinition(out, 96, 16))
  print(arrayDefinition(out, 128, 32))
  print(arrayDefinition(out, 128, 64))

main(sys.argv)
