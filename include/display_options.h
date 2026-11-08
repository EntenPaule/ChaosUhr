#pragma once
#include "settings.h"
#include <initializer_list>

static constexpr size_t DISPLAY_OPTIONS_OFFSET=120;
static constexpr uint8_t DISPLAY_SECONDS=1,DISPLAY_CLOCK=2,DISPLAY_ANIMATIONS=4,
                         DISPLAY_WEATHER=8,DISPLAY_SENSORS=16;
struct DisplayOptions {
  uint16_t magic;
  uint8_t version,ledPin,flags,weekdayStyle;
  uint16_t crc;
};
static_assert(sizeof(DisplayOptions)==8 && DISPLAY_OPTIONS_OFFSET+sizeof(DisplayOptions)==128,
              "Display options must follow the 88-byte admin record without overlap");
inline uint16_t displayOptionsChecksum(const DisplayOptions& value){
  uint16_t crc=0x2110;const auto* bytes=reinterpret_cast<const uint8_t*>(&value);
  for(size_t i=0;i<offsetof(DisplayOptions,crc);++i)crc=static_cast<uint16_t>((crc<<5)|(crc>>11))^bytes[i];
  return crc;
}
inline void sealDisplayOptions(DisplayOptions& value){value.magic=0xd110;value.version=1;value.crc=displayOptionsChecksum(value);}
inline bool allowedLedPin(uint8_t pin,bool esp32){
  if(esp32){for(uint8_t p:{13,14,18,19,21,22,23,25,26,27,32,33})if(pin==p)return true;}
  else{for(uint8_t p:{4,5,12,13,14})if(pin==p)return true;}
  return false;
}
inline DisplayOptions defaultDisplayOptions(bool esp32=false){
  DisplayOptions value={};value.ledPin=esp32?32:12;
  value.flags=DISPLAY_SECONDS|DISPLAY_CLOCK|DISPLAY_ANIMATIONS;sealDisplayOptions(value);return value;
}
inline bool validDisplayOptions(const DisplayOptions& value,bool esp32=false){
  return value.magic==0xd110 && value.version==1 && value.crc==displayOptionsChecksum(value) &&
         allowedLedPin(value.ledPin,esp32) && !(value.flags&~31) && (value.flags&30) && value.weekdayStyle<=1;
}
inline bool normalizeDisplayOptions(DisplayOptions& value,bool esp32=false){
  if(validDisplayOptions(value,esp32))return true;
  value=defaultDisplayOptions(esp32);return false;
}
inline bool displayOptionKey(const char* key){
  for(const char* name:{"ledPin","secondsBar","weekdayStyle","showClock","showAnimations","showWeather","showSensors"})
    if(!strcmp(name,key))return true;
  return false;
}
inline bool applyDisplayOption(DisplayOptions& value,const char* key,const char* text,bool esp32=false){
  uint8_t number;
  if(!strcmp(key,"ledPin")){
    if(!parseSetting(text,0,33,number) || !allowedLedPin(number,esp32))return false;
    value.ledPin=number;return true;
  }
  if(!parseSetting(text,0,1,number))return false;
  if(!strcmp(key,"weekdayStyle")){value.weekdayStyle=number;return true;}
  uint8_t bit=0;
  if(!strcmp(key,"secondsBar"))bit=DISPLAY_SECONDS;
  if(!strcmp(key,"showClock"))bit=DISPLAY_CLOCK;
  if(!strcmp(key,"showAnimations"))bit=DISPLAY_ANIMATIONS;
  if(!strcmp(key,"showWeather"))bit=DISPLAY_WEATHER;
  if(!strcmp(key,"showSensors"))bit=DISPLAY_SENSORS;
  if(!bit)return false;
  value.flags=number?(value.flags|bit):(value.flags&~bit);return true;
}
// One commit for settings and options; preserve the full credentials area.
template<typename Storage>bool persistClockOptions(Storage& storage,Settings& current,Settings next,
    DisplayOptions& options,DisplayOptions nextOptions,bool& settingsSaved,bool& optionsSaved){
  sealSettings(next);sealDisplayOptions(nextOptions);
  if(!validSettings(next))return false;
  if(settingsSaved && optionsSaved && !memcmp(&current,&next,sizeof next) && !memcmp(&options,&nextOptions,sizeof nextOptions))return true;
  Settings oldSettings;DisplayOptions oldOptions;
  storage.get(0,oldSettings);storage.get(DISPLAY_OPTIONS_OFFSET,oldOptions);
  storage.put(0,next);storage.put(DISPLAY_OPTIONS_OFFSET,nextOptions);
  if(!storage.commit()){
    storage.put(0,oldSettings);storage.put(DISPLAY_OPTIONS_OFFSET,oldOptions);
    settingsSaved=optionsSaved=false;return false;
  }
  current=next;options=nextOptions;settingsSaved=optionsSaved=true;return true;
}
