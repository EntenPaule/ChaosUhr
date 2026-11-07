#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "display_logic.h"
#include "pixel_font.h"

static constexpr uint8_t CLOCK_TIME_Y=2, CLOCK_DATE_Y=9;
static constexpr size_t CLOCK_DATE_CAPACITY=32;
static constexpr uint16_t DATE_SCROLL_MS=125;
static_assert(CLOCK_TIME_Y+FONT_HEIGHT<CLOCK_DATE_Y,"Clock rows overlap");
static_assert(CLOCK_DATE_Y+FONT_HEIGHT<MATRIX_HEIGHT,"Keep the seconds row free");

inline bool formatGermanDate(const tm& local,char* output,size_t capacity) {
  if(!output || !capacity) return false;
  output[0]='\0';
  if(local.tm_wday<0 || local.tm_wday>6 || local.tm_mon<0 || local.tm_mon>11 ||
     local.tm_mday<1 || local.tm_mday>31 || local.tm_year<0 || local.tm_year>8099) return false;
  static constexpr const char* weekdays[]={
    "Sonntag","Montag","Dienstag","Mittwoch","Donnerstag","Freitag","Samstag"
  };
  const int length=snprintf(output,capacity,"%s %02d.%02d.%04d",weekdays[local.tm_wday],
                            local.tm_mday,local.tm_mon+1,local.tm_year+1900);
  if(length<0 || static_cast<size_t>(length)>=capacity){output[0]='\0';return false;}
  return true;
}

inline int dateScrollX(uint32_t elapsedMs,size_t characters) {
  if(!characters || characters>=CLOCK_DATE_CAPACITY) return MATRIX_WIDTH;
  const uint16_t width=characters*FONT_ADVANCE-1;
  const uint16_t travel=width+MATRIX_WIDTH+4;
  return MATRIX_WIDTH-static_cast<int>((elapsedMs/DATE_SCROLL_MS)%travel);
}
