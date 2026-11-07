#include "public_credentials.h"
#include "settings.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <string>

struct Storage {
  std::array<uint8_t,128> buffer,flash;
  bool fail=false;unsigned commits=0,writes=0;
  Storage(){buffer.fill(0xff);flash=buffer;}
  size_t length()const{return buffer.size();}
  template<typename T>void put(size_t offset,const T& data){assert(offset+sizeof(T)<=length());memcpy(buffer.data()+offset,&data,sizeof data);}
  template<typename T>void get(size_t offset,T& data){assert(offset+sizeof(T)<=length());memcpy(&data,buffer.data()+offset,sizeof data);}
  void write(size_t offset,uint8_t data){buffer.at(offset)=data;++writes;}
  bool commit(){++commits;if(fail)return false;flash=buffer;return true;}
  void reboot(){buffer=flash;}
};
struct Network {
  bool fail=false,erased=false;unsigned calls=0;
  bool eraseSavedWifi(){++calls;if(fail)return false;erased=true;return true;}
};

int main(){
  Storage storage;
  DeviceCredentials credentials={};
  const char* password="fixture-admin-passphrase-42";
  const char* code="123456789012";
  assert(!credentialsReady(credentials));
  for(int configured=0;configured<2;++configured)
  for(int ap=0;ap<2;++ap)
  for(int resetting=0;resetting<2;++resetting)
    assert(canProvision(configured,ap,resetting)==(!configured && ap && !resetting));
  for(int auth=0;auth<2;++auth)
  for(int token=0;token<2;++token){
    assert(resetConfirmed(auth,token,"RESET")==static_cast<bool>(auth && token));
    assert(!resetConfirmed(auth,token,"reset") && !resetConfirmed(auth,token,nullptr));
  }
  assert(!validAdminPassword(nullptr) && !validAdminPassword("too-short"));
  assert(validAdminPassword(std::string(16,'a').c_str()));
  assert(validAdminPassword(std::string(64,'b').c_str()));
  assert(!validAdminPassword(std::string(65,'b').c_str()));
  assert(!validAdminPassword("test-password-with\nnewline"));
  assert(!validAdminPassword("passphrase-with-umlaut-\xc3\xa4"));
  assert(!validSetupCode("123") && !validSetupCode("12345678901x"));
  assert(!provisionCredentials(storage,credentials,password,"different",code));
  assert(storage.commits==0);
  Settings settings=defaultSettings();settings.brightness=10;sealSettings(settings);
  storage.put(0,settings);assert(storage.commit());
  const auto settingsBytes=storage.flash;
  storage.fail=true;
  assert(!provisionCredentials(storage,credentials,password,password,code));
  assert(!credentialsReady(credentials));
  storage.fail=false;
  assert(provisionCredentials(storage,credentials,password,password,code));
  assert(credentialsReady(credentials) && !strcmp(credentials.adminPassword,password));
  for(size_t i=0;i<CREDENTIALS_OFFSET;++i)assert(storage.flash[i]==settingsBytes[i]);
  const auto withAdmin=storage.flash;
  Settings next=settings;next.autoEffects=(uint32_t{1}<<29)|1;next.autoOrder=1;
  bool settingsSaved=true;
  assert(persistSettings(storage,settings,next,settingsSaved));
  for(size_t i=CREDENTIALS_OFFSET;i<storage.length();++i)assert(storage.flash[i]==withAdmin[i]);
  DeviceCredentials retained;storage.get(CREDENTIALS_OFFSET,retained);
  assert(credentialsReady(retained) && !strcmp(retained.adminPassword,password));
  const unsigned commits=storage.commits;
  assert(!provisionCredentials(storage,credentials,password,password,code));
  assert(storage.commits==commits); // An existing admin cannot be claimed again.
  for(size_t i=0;i<sizeof credentials;++i){
    auto damaged=credentials;reinterpret_cast<uint8_t*>(&damaged)[i]^=1;
    assert(!validCredentials(damaged));
  }
  Network network;
  assert(!completeFactoryReset(storage,network,credentials) && network.calls==0);
  storage.fail=true;
  assert(!markFactoryReset(storage,credentials));
  assert(credentialsReady(credentials));
  storage.fail=false;
  assert(markFactoryReset(storage,credentials));
  assert(resetPending(credentials) && !credentialsReady(credentials));
  assert(!canProvision(false,true,true));
  network.fail=true;
  const unsigned writes=storage.writes;
  assert(!completeFactoryReset(storage,network,credentials));
  assert(storage.writes==writes && !network.erased);
  storage.reboot();storage.get(CREDENTIALS_OFFSET,credentials);
  assert(resetPending(credentials)); // Power loss resumes the reset marker.
  network.fail=false;storage.fail=true;
  assert(!completeFactoryReset(storage,network,credentials));
  assert(network.erased); // SDK credentials are erased before touching EEPROM.
  storage.reboot();storage.get(CREDENTIALS_OFFSET,credentials);
  assert(resetPending(credentials));
  storage.fail=false;
  assert(completeFactoryReset(storage,network,credentials));
  for(uint8_t byte:storage.flash)assert(byte==0xff);
  storage.reboot();storage.get(CREDENTIALS_OFFSET,credentials);storage.get(0,settings);
  assert(!validCredentials(credentials) && !validSettings(settings));
  puts("PASS public credentials: provisioning, bounds, immutable admin, failed commits, reset authorization, power-loss resume, WLAN-first erase, full defaults");
}
