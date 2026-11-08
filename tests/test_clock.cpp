#include "clock_text.h"

#include <cassert>
#include <cstdio>
#include <cstring>

int main(){
  const char* expected[]={
    "Montag 02.11.2026","Dienstag 03.11.2026","Mittwoch 04.11.2026",
    "Donnerstag 05.11.2026","Freitag 06.11.2026","Samstag 07.11.2026","Sonntag 08.11.2026"
  };
  tm local={};local.tm_year=126;local.tm_mon=10;
  char text[CLOCK_DATE_CAPACITY];
  for(int day=0;day<7;day++){
    local.tm_wday=(day+1)%7;local.tm_mday=day+2;
    assert(formatGermanDate(local,text,sizeof text));
    assert(!strcmp(text,expected[day]));
    const char* shortNames[]={"mo","di","mi","do","fr","sa","so"};
    char abbreviated[CLOCK_DATE_CAPACITY];
    assert(formatGermanDate(local,abbreviated,sizeof abbreviated,1));
    assert(!strncmp(abbreviated,shortNames[day],2) && !strcmp(abbreviated+2,strchr(text,' ')));
    for(const char* character=text;*character;character++){
      if(*character==' ') continue;
      uint8_t combined=0;
      for(uint8_t row=0;row<FONT_HEIGHT;row++) combined|=fontRow(*character,row);
      assert(combined); // Every German weekday/date character is displayable.
    }
    const size_t characters=strlen(text);
    const uint32_t cycle=(characters*FONT_ADVANCE-1+MATRIX_WIDTH+4)*DATE_SCROLL_MS;
    assert(dateScrollX(0,characters)==MATRIX_WIDTH);
    assert(dateScrollX(DATE_SCROLL_MS,characters)==MATRIX_WIDTH-1);
    assert(dateScrollX(cycle,characters)==MATRIX_WIDTH);
    assert(dateScrollX(cycle-DATE_SCROLL_MS,characters)<-static_cast<int>(characters*FONT_ADVANCE-1));
    for(uint32_t elapsed=0;elapsed<cycle;elapsed+=DATE_SCROLL_MS){
      const int x=dateScrollX(elapsed,characters);
      assert(x<=MATRIX_WIDTH && x>=-static_cast<int>(characters*FONT_ADVANCE+3));
    }
    assert(dateScrollX(UINT32_MAX,characters)<=MATRIX_WIDTH);
  }
  local.tm_year=128;local.tm_mon=1;local.tm_mday=29;local.tm_wday=2;
  assert(formatGermanDate(local,text,sizeof text));
  assert(!strcmp(text,"Dienstag 29.02.2028"));
  char tiny[3]={'x','x','!'};
  assert(!formatGermanDate(local,tiny,2) && tiny[0]=='\0' && tiny[2]=='!');
  assert(!formatGermanDate(local,nullptr,10));
  assert(!formatGermanDate(local,text,0));
  local.tm_wday=7;assert(!formatGermanDate(local,text,sizeof text));
  local.tm_wday=0;local.tm_mon=12;assert(!formatGermanDate(local,text,sizeof text));
  assert(dateScrollX(1000,0)==MATRIX_WIDTH);
  assert(dateScrollX(1000,CLOCK_DATE_CAPACITY)==MATRIX_WIDTH);
  assert(fontRow('.',4)==2 && !fontRow(' ',0));
  for(char letter='a';letter<='z';letter++){
    uint8_t combined=0;for(uint8_t row=0;row<FONT_HEIGHT;row++)combined|=fontRow(letter,row);
    assert(combined);
  }
  assert(fontRow('o',0)==0 && fontRow('O',0)!=0);
  assert(!formatGermanDate(local,text,sizeof text,2));
  for(int character=0;character<128;character++)
    for(uint8_t row=0;row<7;row++) assert(fontRow(character,row)<=7);
  assert(CLOCK_TIME_Y+FONT_HEIGHT<CLOCK_DATE_Y && CLOCK_DATE_Y+FONT_HEIGHT<MATRIX_HEIGHT);
  puts("PASS clock/date: seven German weekdays, DD.MM.YYYY, leap day, glyph coverage, scroll cycle, layout and buffer bounds");
}
