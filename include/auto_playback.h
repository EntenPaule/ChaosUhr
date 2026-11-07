#pragma once
#include "settings.h"

inline uint64_t autoEffectBit(uint8_t id){
  return id>=1 && id<=TOTAL_EFFECT_COUNT ? uint64_t{1}<<(id-1) : 0;
}

struct AutoPlayback {
  uint64_t configuredMask=0,remaining=0;
  uint8_t configuredOrder=0,last=0;

  void reset(){configuredMask=remaining=0;configuredOrder=last=0;}

  // randomIndex receives the number of candidates and returns [0,count).
  template<typename RandomIndex>
  uint8_t next(const Settings& settings,RandomIndex randomIndex,uint8_t availableUploads=255){
    const uint64_t mask=(settings.autoEffects&ALL_AUTO_EFFECTS)|
                        (static_cast<uint64_t>(settings.autoUploads&availableUploads)<<EFFECT_COUNT);
    if(!mask) return 0;
    if(mask!=configuredMask || settings.autoOrder!=configuredOrder){
      reset();configuredMask=mask;configuredOrder=settings.autoOrder;
    }
    if(configuredOrder==1){
      for(uint8_t step=1;step<=TOTAL_EFFECT_COUNT;++step){
        const uint8_t id=(last+step-1)%TOTAL_EFFECT_COUNT+1;
        if(mask&autoEffectBit(id)){last=id;return id;}
      }
    } else {
      if(!remaining) remaining=mask;
      uint64_t candidates=remaining;
      // Avoid repeating across round boundaries when multiple effects exist.
      const uint64_t alternatives=candidates&~autoEffectBit(last);
      if(alternatives) candidates=alternatives;
      uint8_t count=0;
      for(uint8_t id=1;id<=TOTAL_EFFECT_COUNT;++id)if(candidates&autoEffectBit(id))++count;
      uint8_t index=randomIndex(count);
      for(uint8_t id=1;id<=TOTAL_EFFECT_COUNT;++id){
        const uint64_t bit=autoEffectBit(id);
        if((candidates&bit) && index--==0){
          remaining&=~bit;last=id;return id;
        }
      }
    }
    return 0;
  }
};
