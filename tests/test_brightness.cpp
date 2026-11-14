#include "brightness_control.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 BrightnessPolicy policy;BrightnessOverride remote;
 for(unsigned day=1;day<=100;++day)for(unsigned dim=1;dim<=100;++dim)for(bool night:{false,true}){
  const auto decision=decideBrightness(night,policy,remote,day,dim);
  const unsigned pct=night?dim:day;assert(decision.percent==pct&&decision.scale==2+(pct-1)*118/99);
 }
 policy.mqtt=true;
 for(const char* text:{"0","1","5","50","100"}){
  assert(remote.receive(text,strlen(text)));assert(decideBrightness(false,policy,remote,25,6).percent==remote.percent);
  policy.nightOff=true;assert(decideBrightness(true,policy,remote,25,6).scale==0);
  policy.nightOff=false;assert(decideBrightness(true,policy,remote,25,6).percent==remote.percent);
 }
 assert(remote.percent==100);assert(decideBrightness(false,policy,remote,25,6).scale==120);
 for(const char* bad:{"","-1","101","1.0","auto"," 1","1 ","1x","1000"}){assert(!remote.receive(bad,strlen(bad)));assert(remote.valid&&remote.percent==100);}
 const char binary[]={'1',0,'0'};assert(!remote.receive(binary,sizeof binary));
 assert(remote.receive("AUTO",4)&&!remote.valid);assert(decideBrightness(true,policy,remote,25,6).percent==6);
 assert(remote.receive("0",1)&&decideBrightness(false,policy,remote,25,6).scale==0);
 policy.mqtt=false;assert(decideBrightness(false,policy,remote,25,6).percent==25);
 puts("PASS brightness priority: unchanged day/night, zero, cap, strict commands/AUTO, nighttime override and invalid preservation");
}
