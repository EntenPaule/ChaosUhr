#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#define PROGMEM
using String=std::string;
extern uint32_t fakeMillis;
inline uint32_t millis(){return fakeMillis;}
struct FakeESP {
  unsigned randomByte=0,restarts=0,erases=0;
  bool eraseFails=false;
  uint32_t getChipId(){return 0x123abc;}
  uint64_t getEfuseMac(){return static_cast<uint64_t>(getChipId())<<24;}
  uint32_t getFlashChipSize(){return 4194304;}
  uint32_t getFlashChipRealSize(){return 4194304;}
  uint8_t* random(uint8_t* output,size_t count){for(size_t i=0;i<count;++i)output[i]=randomByte++%256;return output;}
  void restart(){++restarts;}
  bool eraseConfig();
};
extern FakeESP ESP;
