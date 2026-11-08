#pragma once
#include "display_options.h"
#include <stdio.h>

enum ScenePage:uint8_t {PAGE_CLOCK=0,PAGE_WEATHER=1,PAGE_SENSOR=2,PAGE_ANIMATION=3};
inline uint8_t selectedScenePages(const DisplayOptions& value){
  return ((value.flags&DISPLAY_CLOCK)?1:0)|((value.flags&DISPLAY_WEATHER)?2:0)|
         ((value.flags&DISPLAY_SENSORS)?4:0)|((value.flags&DISPLAY_ANIMATIONS)?8:0);
}
inline uint8_t nextScenePage(uint8_t current,uint8_t available){
  for(uint8_t step=1;step<=4;++step){const uint8_t next=(current+step)%4;if(available&(1<<next))return next;}
  return PAGE_CLOCK;
}
class IpAnnouncement {
 public:
  void start(const char* address,uint32_t now){
    snprintf(text_,sizeof text_,"IP %s",address);started_=now;
    duration_=static_cast<uint32_t>((strlen(text_)*4+24)*100);visible_=true;
  }
  bool active(uint32_t now){if(visible_ && static_cast<uint32_t>(now-started_)>=duration_)visible_=false;return visible_;}
  int x(uint32_t now)const{return 20-static_cast<int>(static_cast<uint32_t>(now-started_)/100);}
  const char* text()const{return text_;}
 private:
  char text_[24]={};uint32_t started_=0,duration_=0;bool visible_=false;
};
