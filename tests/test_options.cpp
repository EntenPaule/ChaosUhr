#include "scene_pages.h"
#include <array>
#include <cassert>
#include <cstdio>

struct Storage {
  std::array<uint8_t,128> buffer,flash;bool fail=false;unsigned commits=0;
  Storage(){buffer.fill(0xa5);flash=buffer;}
  template<typename T>void get(size_t at,T& value){assert(at+sizeof value<=buffer.size());memcpy(&value,buffer.data()+at,sizeof value);}
  template<typename T>void put(size_t at,const T& value){assert(at+sizeof value<=buffer.size());memcpy(buffer.data()+at,&value,sizeof value);}
  bool commit(){++commits;if(fail)return false;flash=buffer;return true;}
};
int main(){
  for(bool esp32:{false,true}){
    auto options=defaultDisplayOptions(esp32);assert(validDisplayOptions(options,esp32));
    assert(options.ledPin==(esp32?32:12));
    for(uint8_t pin=0;pin<40;++pin){
      char text[8];snprintf(text,sizeof text,"%u",pin);auto next=options;
      assert(applyDisplayOption(next,"ledPin",text,esp32)==allowedLedPin(pin,esp32));
    }
    auto next=options;assert(applyDisplayOption(next,"secondsBar","0",esp32));
    assert(applyDisplayOption(next,"weekdayStyle","1",esp32));sealDisplayOptions(next);assert(validDisplayOptions(next,esp32));
    next.crc^=1;assert(!normalizeDisplayOptions(next,esp32));assert(!memcmp(&next,&options,sizeof next));
    assert(!applyDisplayOption(next,"secondsBar","2",esp32));assert(!applyDisplayOption(next,"arbitrary","1",esp32));
  }
  Storage storage;Settings settings=defaultSettings();auto options=defaultDisplayOptions();bool saved=false,optionsSaved=false;
  auto changed=options;changed.flags=DISPLAY_CLOCK|DISPLAY_WEATHER;changed.weekdayStyle=1;
  const auto original=storage.flash;
  assert(persistClockOptions(storage,settings,settings,options,changed,saved,optionsSaved));
  assert(saved&&optionsSaved&&storage.commits==1&&validSettings(settings)&&validDisplayOptions(options));
  for(size_t i=32;i<120;++i)assert(storage.flash[i]==original[i]);
  assert(persistClockOptions(storage,settings,settings,options,options,saved,optionsSaved)&&storage.commits==1);
  const auto before=storage.flash;const auto oldOptions=options;auto next=settings;next.brightness=80;
  storage.fail=true;changed=options;changed.ledPin=4;
  assert(!persistClockOptions(storage,settings,next,options,changed,saved,optionsSaved));
  assert(storage.buffer==before&&storage.flash==before&&!memcmp(&options,&oldOptions,sizeof options)&&settings.brightness==25);
  for(uint8_t mask=1;mask<16;++mask){uint8_t page=0;for(unsigned i=0;i<32;++i){page=nextScenePage(page,mask);assert(mask&(1<<page));}}
  assert(nextScenePage(PAGE_CLOCK,3)==PAGE_WEATHER&&nextScenePage(PAGE_WEATHER,3)==PAGE_CLOCK);
  assert(nextScenePage(PAGE_CLOCK,9)==PAGE_ANIMATION&&nextScenePage(PAGE_ANIMATION,9)==PAGE_CLOCK);
  assert(nextScenePage(2,0)==PAGE_CLOCK);
  IpAnnouncement ip;ip.start("192.0.2.4",UINT32_MAX-500);assert(ip.active(100));
  assert(ip.x(UINT32_MAX-500)==20&&strstr(ip.text(),"192.0.2.4"));assert(!ip.active(20000));
  puts("PASS display options: platform pin allowlists, lowercase/seconds settings, atomic 32+8-byte save, preserved credentials, rollback, scene selection and IP scroll rollover");
}
