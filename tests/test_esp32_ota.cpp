#include "firmware_upload.h"
#include <vector>
#include <cassert>
#include <cstdio>
#include <algorithm>
static std::vector<uint8_t> image(){
  std::vector<uint8_t> data(24,0);data[0]=0xe9;data[1]=1;data[2]=2;data[3]=0x20;data[23]=1;
  uint32_t address=0x3f400020,length=128;const size_t header=data.size();data.resize(header+8);
  memcpy(data.data()+header,&address,4);memcpy(data.data()+header+4,&length,4);
  const size_t segment=data.size();data.resize(segment+length,0x21);memcpy(data.data()+segment,FIRMWARE_PRODUCT,sizeof FIRMWARE_PRODUCT);
  uint8_t checksum=0xef;for(size_t i=segment;i<data.size();++i)checksum^=data[i];
  while((data.size()&15)!=15)data.push_back(0);
  data.push_back(checksum);
  Sha256Stream hash;hash.add(data.data(),data.size());uint8_t digest[32];assert(hash.digest(digest));data.insert(data.end(),digest,digest+32);return data;
}
struct Backend {unsigned starts=0,ends=0,cancels=0;bool begin(){++starts;return true;}size_t write(const uint8_t*,size_t n){return n;}bool finish(){++ends;return true;}void cancel(){++cancels;}};
int main(){
  auto good=image();FirmwareValidator validator;for(uint8_t byte:good)assert(validator.feed(&byte,1));assert(validator.complete());
  for(size_t position:{size_t(0),size_t(2),size_t(3),size_t(12),size_t(23),size_t(28),size_t(55),good.size()-1}){
    auto changed=good;changed[position]^=1;FirmwareValidator bad;bool invalid=false;
    for(uint8_t byte:changed)if(!bad.feed(&byte,1)){invalid=true;break;}
    assert(invalid||!bad.complete());
  }
  Backend backend;FirmwareUpload<Backend> upload(backend);upload.start(true,true);upload.write(good.data(),good.size());upload.end();assert(!backend.ends);upload.abort();assert(!backend.ends&&backend.cancels);
  upload.start(true,true);upload.write(good.data(),good.size());upload.end();assert(upload.finish(true,true)&&backend.ends==1);
  auto truncated=good;truncated.pop_back();upload.start(true,true);upload.write(truncated.data(),truncated.size());upload.end();assert(!upload.finish(true,true)&&backend.ends==1);
  puts("PASS ESP32 OTA: classic-chip/flash/product and segment checksum/SHA checks, altered headers/payload/hash, truncated upload, abort and deferred activation");
}
