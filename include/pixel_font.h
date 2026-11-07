#pragma once

#include <stdint.h>

static constexpr uint8_t FONT_WIDTH=3, FONT_HEIGHT=5, FONT_ADVANCE=4;
constexpr uint16_t fontBits(uint8_t a,uint8_t b,uint8_t c,uint8_t d,uint8_t e) {
  return (a<<12)|(b<<9)|(c<<6)|(d<<3)|e;
}

// Three-bit rows, digits followed by A..Z. Lowercase uses the same glyphs.
inline uint8_t fontRow(char character,uint8_t row) {
  if(row>=FONT_HEIGHT) return 0;
  static constexpr uint16_t glyphs[]={
    fontBits(7,5,5,5,7),fontBits(2,6,2,2,7),fontBits(7,1,7,4,7),
    fontBits(7,1,7,1,7),fontBits(5,5,7,1,1),fontBits(7,4,7,1,7),
    fontBits(7,4,7,5,7),fontBits(7,1,1,1,1),fontBits(7,5,7,5,7),
    fontBits(7,5,7,1,7),
    fontBits(2,5,7,5,5),fontBits(6,5,6,5,6),fontBits(7,4,4,4,7),
    fontBits(6,5,5,5,6),fontBits(7,4,6,4,7),fontBits(7,4,6,4,4),
    fontBits(7,4,5,5,7),fontBits(5,5,7,5,5),fontBits(7,2,2,2,7),
    fontBits(1,1,1,5,7),fontBits(5,5,6,5,5),fontBits(4,4,4,4,7),
    fontBits(5,7,7,5,5),fontBits(5,7,7,7,5),fontBits(7,5,5,5,7),
    fontBits(6,5,6,4,4),fontBits(7,5,5,7,1),fontBits(6,5,6,5,5),
    fontBits(7,4,7,1,7),fontBits(7,2,2,2,2),fontBits(5,5,5,5,7),
    fontBits(5,5,5,5,2),fontBits(5,5,7,7,5),fontBits(5,5,2,5,5),
    fontBits(5,5,2,2,2),fontBits(7,1,2,4,7)
  };
  if(character>='a' && character<='z') character-=('a'-'A');
  uint16_t bits=0;
  if(character>='0' && character<='9') bits=glyphs[character-'0'];
  else if(character>='A' && character<='Z') bits=glyphs[10+character-'A'];
  else if(character=='.') bits=fontBits(0,0,0,0,2);
  else if(character==':') bits=fontBits(0,2,0,2,0);
  else if(character=='-') bits=fontBits(0,0,7,0,0);
  return (bits>>(3*(FONT_HEIGHT-1-row)))&7;
}
