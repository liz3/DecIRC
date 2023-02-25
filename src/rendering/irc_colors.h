#ifndef DEC_IRC_COLORS_H
#define DEC_IRC_COLORS_H

#include "la.h"
#include <iostream>
Vec4f IRC_COLORS[255];

class ColorUtils {
 public:
  static void simple(size_t index, uint8_t value) {
    float v = (float)value / 255;
    IRC_COLORS[index] = vec4f(v, v, v, 1);
  }
  static void triple(size_t index, uint8_t r, uint8_t g, uint8_t b) {
    IRC_COLORS[index] =
        vec4f((float)r / 255, (float)g / 255, (float)b / 255, 1);
  }
};

void init_irc_colors() {
  memset(IRC_COLORS, 0, sizeof(IRC_COLORS));
  ColorUtils::simple(0, 0xff);               // white;
  ColorUtils::simple(1, 0x00);               // black
  ColorUtils::triple(2, 0, 0, 0xff);         // blue
  ColorUtils::triple(3, 0, 0xff, 0);         // green
  ColorUtils::triple(4, 0xff, 0, 0);         // red
  ColorUtils::triple(5, 0x7f, 0, 0);         // brown
  ColorUtils::triple(6, 0x9c, 0, 0x9c);      // magenta
  ColorUtils::triple(7, 0xfc, 0x7f, 0);      // orange
  ColorUtils::triple(8, 0xff, 0xff, 0);      // yellow
  ColorUtils::triple(9, 0, 0xfc, 0);         // light green
  ColorUtils::triple(10, 0, 0x93, 0x93);     // cyan
  ColorUtils::triple(11, 0, 0xff, 0xff);     // light cyan
  ColorUtils::triple(12, 0, 0, 0xfc);        // light blue
  ColorUtils::triple(13, 0xff, 0, 0xff);     // pink
  ColorUtils::triple(14, 0x7f, 0x7f, 0x7f);  // gray
  ColorUtils::triple(15, 0xd2, 0xd2, 0xd2);  // light gray
}

#endif