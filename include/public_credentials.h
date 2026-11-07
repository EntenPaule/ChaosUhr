#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "settings.h"
#include "password_crypto.h"

static constexpr size_t CREDENTIALS_OFFSET=32, DEVICE_EEPROM_BYTES=128;
static_assert(sizeof(Settings)<=CREDENTIALS_OFFSET,"Settings must not overwrite admin credentials");
static constexpr uint32_t CREDENTIALS_MAGIC=0x43555031;
static constexpr uint8_t RESET_PENDING=1;
static constexpr uint32_t PASSWORD_ROUNDS=10000;

struct DeviceCredentials {
  uint32_t magic;
  uint8_t version;
  uint8_t flags;
  union {char adminPassword[65];uint8_t verifierData[65];}; // v1 plaintext, v2 KDF record
  char setupPassword[13];
  uint32_t crc;
};
static_assert(sizeof(DeviceCredentials)==88 && CREDENTIALS_OFFSET+sizeof(DeviceCredentials)<=DEVICE_EEPROM_BYTES,
              "Credentials must not overlap clock settings or leave the EEPROM buffer");

inline uint32_t credentialsChecksum(const DeviceCredentials& data) {
  uint32_t hash=2166136261u;
  const auto* bytes=reinterpret_cast<const uint8_t*>(&data);
  for(size_t i=0;i<offsetof(DeviceCredentials,crc);++i) hash=(hash^bytes[i])*16777619u;
  return hash;
}
inline bool validAdminPassword(const char* password) {
  if(!password) return false;
  size_t size=0;
  for(;size<65 && password[size];++size)
    if(static_cast<unsigned char>(password[size])<32 || static_cast<unsigned char>(password[size])>126) return false;
  return size>=16 && size<=64;
}
inline bool validSetupCode(const char* code) {
  if(!code) return false;
  for(size_t i=0;i<12;++i) if(code[i]<'0' || code[i]>'9') return false;
  return code[12]=='\0';
}
inline bool validCredentials(const DeviceCredentials& data) {
  if(data.magic!=CREDENTIALS_MAGIC || data.flags>RESET_PENDING || !validSetupCode(data.setupPassword) ||
     credentialsChecksum(data)!=data.crc)return false;
  if(data.version==1)return data.adminPassword[64]=='\0' && validAdminPassword(data.adminPassword);
  if(data.version!=2 || data.verifierData[0]!=1)return false;
  uint32_t rounds;memcpy(&rounds,data.verifierData+1,4);
  if(rounds<PASSWORD_ROUNDS || rounds>600000)return false;
  for(size_t i=53;i<65;++i)if(data.verifierData[i])return false;
  return true;
}
inline bool verifyAdminPassword(const DeviceCredentials& data,const char* password,void(*tick)()=nullptr){
  if(!validCredentials(data) || data.flags || !validAdminPassword(password))return false;
  if(data.version==1){
    const size_t length=strlen(password);return length==strlen(data.adminPassword) && constantEqual(password,data.adminPassword,length);
  }
  uint32_t rounds;memcpy(&rounds,data.verifierData+1,4);uint8_t calculated[32];
  const bool ok=passwordKdf(password,strlen(password),data.verifierData+5,rounds,calculated,tick) && constantEqual(calculated,data.verifierData+21,32);
  eraseSecret(calculated,sizeof calculated);return ok;
}
inline bool hashAdminPassword(DeviceCredentials& data,const char* password,const uint8_t salt[16],void(*tick)()=nullptr){
  if(!validAdminPassword(password) || !salt)return false;
  uint8_t value[32];if(!passwordKdf(password,strlen(password),salt,PASSWORD_ROUNDS,value,tick))return false;
  memset(data.verifierData,0,sizeof data.verifierData);data.verifierData[0]=1;
  memcpy(data.verifierData+1,&PASSWORD_ROUNDS,4);memcpy(data.verifierData+5,salt,16);memcpy(data.verifierData+21,value,32);
  data.version=2;eraseSecret(value,sizeof value);return true;
}
inline bool credentialsReady(const DeviceCredentials& data) {
  return validCredentials(data) && data.flags==0;
}
inline bool resetPending(const DeviceCredentials& data) {
  return validCredentials(data) && data.flags==RESET_PENDING;
}
inline bool canProvision(bool configured,bool viaSetupAP,bool resetting) {
  return !configured && viaSetupAP && !resetting;
}
inline bool resetConfirmed(bool authenticated,bool tokenMatches,const char* confirmation) {
  return authenticated && tokenMatches && confirmation && !strcmp(confirmation,"RESET");
}

template <typename Storage>
bool storeCredentials(Storage& storage,DeviceCredentials& current,DeviceCredentials next) {
  next.crc=credentialsChecksum(next);
  if(!validCredentials(next) || storage.length()<CREDENTIALS_OFFSET+sizeof next) return false;
  storage.put(CREDENTIALS_OFFSET,next);
  if(!storage.commit()){storage.put(CREDENTIALS_OFFSET,current);return false;}
  current=next;
  return true;
}
template <typename Storage>
bool provisionCredentials(Storage& storage,DeviceCredentials& current,const char* password,
                          const char* repeat,const char* setupCode,const uint8_t salt[16],void(*tick)()=nullptr) {
  if(credentialsReady(current) || resetPending(current) || !validAdminPassword(password) ||
     !repeat || strcmp(password,repeat) || !validSetupCode(setupCode)) return false;
  DeviceCredentials next={};
  next.magic=CREDENTIALS_MAGIC;
  if(!hashAdminPassword(next,password,salt,tick))return false;
  memcpy(next.setupPassword,setupCode,13);
  return storeCredentials(storage,current,next);
}
template<typename Storage>
bool migrateCredentials(Storage& storage,DeviceCredentials& current,const uint8_t salt[16],void(*tick)()=nullptr){
  if(!credentialsReady(current) || current.version!=1 || !storage.durable())return false;
  DeviceCredentials next=current;
  if(!hashAdminPassword(next,current.adminPassword,salt,tick))return false;
  const bool ok=storeCredentials(storage,current,next);eraseSecret(&next,sizeof next);return ok;
}
template <typename Storage>
bool markFactoryReset(Storage& storage,DeviceCredentials& current) {
  if(!credentialsReady(current)) return false;
  DeviceCredentials next=current;
  next.flags=RESET_PENDING;
  return storeCredentials(storage,current,next);
}
// The durable marker survives reboot until the SDK Wi-Fi data has been erased.
// Never clear it before that erase succeeds, even if power fails mid-reset.
template <typename Storage,typename Network>
bool completeFactoryReset(Storage& storage,Network& network,const DeviceCredentials& current) {
  if(!resetPending(current)) return false;
  if(!network.eraseSavedWifi()) return false;
  for(size_t i=0;i<storage.length();++i) storage.write(i,0xff);
  return storage.commit();
}
