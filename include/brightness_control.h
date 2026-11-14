#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
struct BrightnessPolicy {bool nightOff=false,mqtt=false;};
struct BrightnessOverride {
  bool valid=false;uint8_t percent=0;
  void clear(){valid=false;percent=0;}
  bool receive(const char* data,size_t size){
    if(size==4 && !memcmp(data,"AUTO",4)){clear();return true;}
    if(!data || size<1 || size>3)return false;
    unsigned value=0;
    for(size_t i=0;i<size;++i){if(data[i]<'0'||data[i]>'9')return false;value=value*10+data[i]-'0';}
    if(value>100)return false;
    valid=true;percent=static_cast<uint8_t>(value);return true;
  }
};
enum class BrightnessSource:uint8_t {Gui,Mqtt,NightOff};
struct BrightnessDecision {uint8_t percent,scale;BrightnessSource source;};
inline BrightnessDecision decideBrightness(bool night,const BrightnessPolicy& policy,const BrightnessOverride& remote,uint8_t day,uint8_t dim){
  const bool forced=night&&policy.nightOff;
  const bool mqtt=policy.mqtt&&remote.valid;
  const uint8_t pct=forced?0:mqtt?remote.percent:night?dim:day;
  return {pct,static_cast<uint8_t>(pct?2+(static_cast<unsigned>(pct)-1)*118/99:0),forced?BrightnessSource::NightOff:mqtt?BrightnessSource::Mqtt:BrightnessSource::Gui};
}
inline const char* brightnessSourceName(BrightnessSource source){return source==BrightnessSource::NightOff?"Nachtabschaltung":source==BrightnessSource::Mqtt?"MQTT":"GUI";}
