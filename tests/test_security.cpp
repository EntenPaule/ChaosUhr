#include "public_credentials.h"
#include "journal_storage.h"
#include "firmware_upload.h"
#include "request_security.h"
#include "memory_animation_fs.h"
#include "upgrade_fixtures.h"
#include <openssl/evp.h>
#include <array>
#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>

struct Flash {
  std::array<uint8_t,128> buffer{},flash{};int torn=-1;unsigned commits=0;
  Flash(){buffer.fill(0xff);flash=buffer;}
  void begin(size_t size){assert(size==128);buffer=flash;}
  size_t length(){return 128;}
  uint8_t read(size_t at){return buffer.at(at);}
  void write(size_t at,uint8_t v){buffer.at(at)=v;}
  template<typename T>void get(size_t at,T& value){assert(at+sizeof value<=128);memcpy(&value,buffer.data()+at,sizeof value);}
  template<typename T>void put(size_t at,const T& value){assert(at+sizeof value<=128);memcpy(buffer.data()+at,&value,sizeof value);}
  bool commit(){++commits;flash.fill(0xff);if(torn>=0){memcpy(flash.data(),buffer.data(),torn);return false;}flash=buffer;return true;}
};
using Journal=JournalStorage<Flash,MemoryAnimationFS,AnimationFile>;
static const char* password="fixture-upgrade-admin-205";
static const uint8_t salt[16]={1,2,3,4,5,6,7,8};
static unsigned ticks=0;
static void crypto(){
  for(uint32_t rounds:{1u,2u,4096u,10000u}){
    uint8_t expected[32],actual[32];
    assert(PKCS5_PBKDF2_HMAC(password,strlen(password),salt,16,rounds,EVP_sha256(),32,expected));
    assert(passwordKdf(password,strlen(password),salt,rounds,actual,[](){++ticks;}));
    assert(constantEqual(expected,actual,32));
  }
  assert(ticks>300);uint8_t out[32];assert(!passwordKdf(password,strlen(password),salt,600001,out));
  auto basic=[](const std::string& raw){unsigned char output[256];int size=EVP_EncodeBlock(output,reinterpret_cast<const unsigned char*>(raw.data()),raw.size());return std::string("Basic ")+std::string(reinterpret_cast<char*>(output),size);};
  for(size_t n=16;n<=64;++n){std::string p(n,'a');p.back()=' ';char decoded[65]={};const auto encoded=basic("admin:"+p);assert(decodeAdminBasic(encoded.data(),encoded.size(),decoded)&&decoded==p);}
  for(const auto& bad:{basic("other:"+std::string(16,'a')),basic("admin:short"),basic("admin:"+std::string(65,'a')),basic("admin:bad\npassword-long"),std::string("Basic !!!!"),std::string("Basic YQ==AAAA"),std::string("Digest invalid")}){
    char decoded[65]={};assert(!decodeAdminBasic(bad.data(),bad.size(),decoded));
  }
  std::cout<<"PASS KDF: OpenSSL reference vectors, rounds, watchdog callbacks, bounded strict Basic decoder and trailing spaces\n";
}
static void migration(){
  Flash flash;memcpy(flash.flash.data(),UPGRADE_205,128);MemoryAnimationFS fs;fs.mountable=false;
  Journal storage(flash,fs);storage.begin(128);DeviceCredentials old;storage.get(32,old);
  assert(credentialsReady(old)&&old.version==1&&verifyAdminPassword(old,password));
  assert(!migrateCredentials(storage,old,salt)&&flash.commits==0&&fs.formats==0);
  Settings settings;storage.get(0,settings);assert(!normalizeLoadedSettings(settings));
  assert(settings.brightness==10&&settings.effect==16&&settings.duration==10&&settings.nightFrom==20);
  assert(storage.read(32)==UPGRADE_205[32]);
  assert(fs.format());assert(storage.durable());
  assert(migrateCredentials(storage,old,salt)&&old.version==2&&verifyAdminPassword(old,password));
  assert(!strcmp(old.setupPassword,"123456789012"));
  for(size_t i=0;i<32;++i)assert(flash.flash[i]==UPGRADE_205[i]);
  const auto& persisted=*fs.files.at(Journal::path());
  assert(std::search(persisted.begin(),persisted.end(),password,password+strlen(password))==persisted.end());
  DeviceCredentials v2=old;
  for(int cut=0;cut<128;++cut){
    Flash broken;memcpy(broken.flash.data(),UPGRADE_205,128);MemoryAnimationFS disk;
    Journal first(broken,disk);first.begin(128);DeviceCredentials current;first.get(32,current);broken.torn=cut;
    assert(storeCredentials(first,current,v2)); // The journal is already durable.
    broken.torn=-1;Journal rebooted(broken,disk);rebooted.begin(128);DeviceCredentials recovered;rebooted.get(32,recovered);
    assert(!memcmp(&recovered,&v2,sizeof v2));for(size_t i=0;i<32;++i)assert(broken.flash[i]==UPGRADE_205[i]);
  }
  for(bool renameFailure:{false,true}){
    Flash unchanged;memcpy(unchanged.flash.data(),UPGRADE_205,128);MemoryAnimationFS disk;
    Journal first(unchanged,disk);first.begin(128);DeviceCredentials current;first.get(32,current);
    disk.writeFails=!renameFailure;disk.renameFails=renameFailure;
    assert(!storeCredentials(first,current,v2));assert(current.version==1&&unchanged.commits==0);
    assert(!memcmp(unchanged.flash.data(),UPGRADE_205,128));
  }
  assert(markFactoryReset(storage,old));assert(storage.clearForReset());
  Journal interrupted(flash,fs);interrupted.begin(128);DeviceCredentials pending;interrupted.get(32,pending);assert(resetPending(pending));
  assert(interrupted.clearForReset());assert(fs.format());Journal fresh(flash,fs);fresh.begin(128);DeviceCredentials empty;fresh.get(32,empty);assert(!credentialsReady(empty));
  for(auto value:flash.flash)assert(value==0xff);
  // The original Micha sketch has no compatible EEPROM admin/settings record.
  Flash original;MemoryAnimationFS absent;absent.mountable=false;Journal direct(original,absent);direct.begin(128);
  DeviceCredentials initial;direct.get(32,initial);assert(!credentialsReady(initial));
  assert(provisionCredentials(direct,initial,password,password,"123456789012",salt));assert(initial.version==2&&verifyAdminPassword(initial,password));
  Flash corrupt;MemoryAnimationFS damaged;damaged.begin();damaged.put(Journal::path(),"truncated");Journal bad(corrupt,damaged);bad.begin(128);assert(bad.fault()&&!bad.commit());
  std::cout<<"PASS upgrades: actual 2.0.5 fixture, legacy 20-byte settings, deferred migration without formatting, hash/login/AP preservation, all 128 torn EEPROM writes, FS failures, reset journal and blank original-sketch storage\n";
}
static std::vector<uint8_t> image(){
  std::vector<uint8_t> bytes(0x1200,0);bytes[0]=bytes[0x1000]=0xe9;bytes[2]=2;bytes[3]=0x40;
  memcpy(bytes.data()+0x1100,FIRMWARE_PRODUCT,sizeof FIRMWARE_PRODUCT);
  uint32_t table[256];for(uint32_t i=0;i<256;++i){uint32_t v=i<<24;for(unsigned j=0;j<8;++j)v=(v<<1)^((v&0x80000000)?0x04c11db7:0);table[i]=v;}
  uint32_t crc=0xffffffff;for(auto value:bytes)crc=(crc<<8)^table[(crc>>24)^value];
  uint32_t size=bytes.size();memcpy(bytes.data()+0x1010,&size,4);memcpy(bytes.data()+0x1014,&crc,4);return bytes;
}
struct Updater {
  bool rejectBegin=false,rejectWrite=false,rejectEnd=false;unsigned begins=0,ends=0,cancels=0;size_t bytes=0;
  bool begin(){++begins;bytes=0;return !rejectBegin;}
  size_t write(const uint8_t*,size_t count){bytes+=count;return rejectWrite?0:count;}
  bool finish(){++ends;return !rejectEnd;}
  void cancel(){++cancels;}
};
static void ota(){
  const auto good=image();FirmwareValidator validator;for(auto b:good)assert(validator.feed(&b,1));assert(validator.complete());
  Updater backend;FirmwareUpload<Updater> upload(backend);
  auto send=[&](const std::vector<uint8_t>& bytes){for(size_t at=0;at<bytes.size();at+=37)upload.write(bytes.data()+at,std::min<size_t>(37,bytes.size()-at));upload.end();};
  upload.start(false,true);send(good);assert(!upload.finish(false,true)&&backend.begins==0&&backend.ends==0);
  upload.start(true,false);send(good);assert(!upload.finish(true,true)&&backend.begins==0);
  assert(!upload.finish(true,true));
  upload.start(true,true);send(good);assert(upload.finish(true,true)&&backend.ends==1);
  uint8_t sha[32];unsigned shaLength=0;assert(EVP_Digest(good.data(),good.size(),sha,&shaLength,EVP_sha256(),nullptr)==1&&shaLength==32);
  char hex[65];for(size_t i=0;i<32;++i)snprintf(hex+2*i,3,"%02x",sha[i]);
  upload.start(true,true,hex);send(good);assert(upload.finish(true,true));
  hex[0]=hex[0]=='0'?'1':'0';upload.start(true,true,hex);send(good);assert(!upload.finish(true,true));
  upload.start(true,true,"invalid");send(good);assert(!upload.finish(true,true));
  for(size_t index:{size_t(0),size_t(2),size_t(3),size_t(0x1000),size_t(0x1010),size_t(0x1014),size_t(0x1110)}){
    auto damaged=good;damaged[index]^=1;const auto before=backend.ends;upload.start(true,true);send(damaged);assert(!upload.finish(true,true)&&backend.ends==before);
  }
  auto truncated=good;truncated.pop_back();upload.start(true,true);send(truncated);assert(!upload.finish(true,true));
  upload.start(true,true);send(good);const auto ends=backend.ends;upload.abort();assert(backend.ends==ends&&!upload.finish(true,true));
  upload.start(true,true);send(good);upload.start(true,true);assert(!upload.finish(true,true)&&backend.ends==ends);
  upload.start(true,true);send(good);assert(!upload.finish(true,false));
  upload.start(true,true);send(good);assert(!upload.finish(false,true));
  backend.rejectBegin=true;upload.start(true,true);send(good);assert(!upload.finish(true,true));backend.rejectBegin=false;
  backend.rejectWrite=true;upload.start(true,true);send(good);assert(!upload.finish(true,true));backend.rejectWrite=false;
  backend.rejectEnd=true;upload.start(true,true);send(good);assert(!upload.finish(true,true));backend.rejectEnd=false;
  upload.start(true,true);send(good);assert(upload.finish(true,true));
  std::cout<<"PASS OTA controller: bounded image/CRC/product checks, authorization before flash, one file, no FS/gzip, abort after file end, retry, write/end failures and activation only after request completion\n";
}
int main(){
  crypto();migration();ota();
  assert(originMatchesHost("http://clock.home.arpa","clock.home.arpa"));
  assert(originMatchesHost("","chaosuhr.local"));
  for(const char* bad:{"null","https://foreign.invalid","http://chaosuhr.local.foreign.invalid"})assert(!originMatchesHost(bad,"chaosuhr.local"));
  assert(!originMatchesHost("http://a/b","a/b"));assert(!setupHostAllowed("foreign.invalid","192.168.4.1"));
  std::cout<<"PASS request policy: exact Origin/Host comparison, absent-origin token fallback and strict commissioning hosts\n";
}
