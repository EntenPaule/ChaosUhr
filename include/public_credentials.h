#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "settings.h"

static constexpr size_t CREDENTIALS_OFFSET=32, DEVICE_EEPROM_BYTES=128;
static_assert(sizeof(Settings)<=CREDENTIALS_OFFSET,"Settings must not overwrite admin credentials");
static constexpr uint32_t CREDENTIALS_MAGIC=0x43555031;
static constexpr uint8_t RESET_PENDING=1;

struct DeviceCredentials {
  uint32_t magic;
  uint8_t version;
  uint8_t flags;
  char adminPassword[65];
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
  return data.magic==CREDENTIALS_MAGIC && data.version==1 && data.flags<=RESET_PENDING &&
         data.adminPassword[64]=='\0' && validAdminPassword(data.adminPassword) &&
         validSetupCode(data.setupPassword) && credentialsChecksum(data)==data.crc;
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
                          const char* repeat,const char* setupCode) {
  if(credentialsReady(current) || resetPending(current) || !validAdminPassword(password) ||
     !repeat || strcmp(password,repeat) || !validSetupCode(setupCode)) return false;
  DeviceCredentials next={};
  next.magic=CREDENTIALS_MAGIC;next.version=1;
  memcpy(next.adminPassword,password,strlen(password)+1);
  memcpy(next.setupPassword,setupCode,13);
  return storeCredentials(storage,current,next);
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
