#!/usr/bin/env python

# Convert a 128x64 bmp file into a C++ static initializer
# usable as a splash screen for the Adafruit_SSD1306 driver.
#
#     python make_splash.py splash.bmp

import sys
import getopt

# $ pip install --user Pillow
from PIL import Image

# Every byte is a 1x8 vertical strip
def verticalBytes(im):
  out = []
  for j in range(im.height / 8):
    for i in range(im.width):
      byte = 0
      for jbit in range(8):
        if im.getpixel((i, j * 8 + jbit)) != 0:
          byte = byte | (1 << jbit)
      out.append(byte)
  return out

def arrayDefinition(out, w, h):
  decl = str()
  decl += 'uint8_t buffer_{0}x{1}[{0} * {1} / 8] = {{\n'.format(w, h)
  # decl += 'Adafruit_SSD1306_Driver::Splash<{0}, {1}>::buffer[{0} * {1} / 8] = {{\n'.format(w, h)
  bytes_per_row = 16
  row_groups = int(h / 8)
  bytes = []
  for row_group in range(row_groups):
    for col in range(w):
      bytes.append('0x{:02X}'.format(out[row_group * w + col]))
  decl += '  '
  while len(bytes):
    decl += ', '.join(bytes[0:16])
    bytes[0:16] = []
    if len(bytes):
      decl += ',\n  '
    else:
      decl += '};\n'
  return decl

def usage():
  sys.stderr.write('python make_splash.py -b image.bmp [-o output_prefix]\n')
  sys.exit(2)

def main():
  bmp_file = None
  out_prefix = None

  try:
    opts, args = getopt.getopt(sys.argv[1:], 'b:o:')
  except getopt.GetoptError as err:
    sys.stderr.write(str(err)+"\n")
    usage()
  for o, a in opts:
    if o == '-b':
      bmp_file = a
    if o == '-o':
      out_prefix = a

  if bmp_file == None:
    usage()

  gray = Image.open(bmp_file).convert(mode='L')

  def apply_thresh(p):
    if p < 0x20:
      return 0x00
    return 0xff

  for w, h in [(96, 16), (128, 32), (128, 64)]:
    im = gray.resize((w, h), resample=Image.BILINEAR)
    im = im.point(apply_thresh)
    out = verticalBytes(im)
    print(arrayDefinition(out, w, h))
    if out_prefix:
      im.convert(mode='1').save('{}_{}x{}.bmp'.format(out_prefix, w, h))

main()
