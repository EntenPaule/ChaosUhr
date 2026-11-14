#include "brightness_storage.h"
#include "memory_animation_fs.h"
#include <cassert>
#include <cstdio>
int main(){
 MemoryAnimationFS fs;BrightnessStorage<MemoryAnimationFS,AnimationFile> storage(fs);BrightnessPolicy policy;
 assert(storage.load(policy)&&!policy.nightOff&&!policy.mqtt);assert(storage.save(policy,{false,false})&&fs.writeCalls==0);
 fs.put("/unrelated","keep");assert(storage.save(policy,{true,true}));const auto saved=*fs.files.at(storage.path());
 const auto calls=fs.writeCalls;assert(storage.save(policy,{true,true})&&fs.writeCalls==calls);
 for(int fault=0;fault<5;++fault){
  fs.writeFails=fault==0;fs.openWriteFails=fault==1;fs.renameFails=fault==2;fs.readFails=fault==3;fs.corruptRead=fault==4;
  assert(!storage.save(policy,{false,false}));assert(policy.nightOff&&policy.mqtt);assert(*fs.files.at(storage.path())==saved);
  fs.writeFails=fs.openWriteFails=fs.renameFails=fs.readFails=fs.corruptRead=false;
 }
 BrightnessPolicy loaded;assert(storage.load(loaded)&&loaded.nightOff&&loaded.mqtt);
 fs.put("/brightness-policy.pending","interrupted");assert(storage.load(loaded)&&loaded.nightOff);
 (*fs.files.at(storage.path()))[5]^=1;assert(!storage.load(loaded)&&loaded.nightOff);assert(storage.errorCode()==ErrorCode::FS_BRIGHTNESS_CORRUPT);
 fs.mountable=false;assert(!storage.save(policy,{false,false})&&policy.nightOff);assert(fs.formats==0);
 assert(*fs.files.at("/unrelated")==std::vector<uint8_t>({'k','e','e','p'}));
 puts("PASS brightness persistence: defaults/no writes, changed-only atomic save, interrupted write, read/write/rename failures, corruption, preserved files and no formatting");
}
