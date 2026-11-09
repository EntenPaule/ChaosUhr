#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "chaos_errors.h"
#include "reset_diagnostics.h"
#include <memory>
#include <new>

static constexpr uint8_t SENSOR_SLOTS=4;
struct SensorConfig {char topic[96],label[25],unit[9];};
struct MqttConfig {
  uint32_t magic;
  uint8_t version,enabled,discovery,reserved;
  uint16_t port,reserved2;
  uint32_t staleSeconds;
  char host[96],user[65],password[65],base[64],discoveryPrefix[64];
  char weatherTopic[96],temperatureTopic[96];
  SensorConfig sensors[SENSOR_SLOTS];
  uint32_t crc;
};
inline uint32_t mqttConfigChecksum(const MqttConfig& value){
  uint32_t hash=2166136261u;const auto* data=reinterpret_cast<const uint8_t*>(&value);
  for(size_t i=0;i<offsetof(MqttConfig,crc);++i)hash=(hash^data[i])*16777619u;
  return hash;
}
inline void sealMqttConfig(MqttConfig& value){value.magic=0x4d513130;value.version=1;value.crc=mqttConfigChecksum(value);}
inline MqttConfig defaultMqttConfig(){
  MqttConfig value={};value.port=1883;value.staleSeconds=1800;value.discovery=1;
  strcpy(value.discoveryPrefix,"homeassistant");sealMqttConfig(value);return value;
}
template<size_t N>inline bool boundedText(const char (&text)[N],bool ascii=false){
  const size_t size=strnlen(text,N);if(size==N)return false;
  for(size_t i=0;i<size;++i){const uint8_t ch=text[i];if(ch<32 || ch==127 || (ascii&&ch>126))return false;}
  return true;
}
template<size_t N>inline bool validMqttTopic(const char (&text)[N]){
  return boundedText(text,true) && !strchr(text,'#') && !strchr(text,'+');
}
// Return only fixed field identifiers, never configuration contents.
// Keep the persisted v1 layout and every existing validation rule unchanged.
inline const char* mqttConfigInvalidField(const MqttConfig& value){
  if(value.magic!=0x4d513130 || value.version!=1)return "format";
  if(value.enabled>1)return "enabled";
  if(value.discovery>1)return "discovery";
  if(value.reserved || value.reserved2)return "reserved";
  if(value.crc!=mqttConfigChecksum(value))return "checksum";
  if(!value.port)return "port";
  if(value.staleSeconds<60 || value.staleSeconds>86400)return "staleSeconds";
  if(!boundedText(value.host,true) || (value.enabled&&!value.host[0]))return "host";
  if(!boundedText(value.user))return "user";
  if(!boundedText(value.password))return "password";
  if(!validMqttTopic(value.base))return "base";
  if(!validMqttTopic(value.discoveryPrefix) || !value.discoveryPrefix[0])return "discoveryPrefix";
  if(!validMqttTopic(value.weatherTopic))return "weatherTopic";
  if(!validMqttTopic(value.temperatureTopic))return "temperatureTopic";
  for(const char* c=value.host;*c;++c)if(!((*c>='a'&&*c<='z')||(*c>='A'&&*c<='Z')||(*c>='0'&&*c<='9')||strchr(".-_:",*c)))return "host";
  for(const auto& sensor:value.sensors){
    if(!validMqttTopic(sensor.topic))return "sensorTopic";
    if(!boundedText(sensor.label))return "sensorLabel";
    if(!boundedText(sensor.unit))return "sensorUnit";
  }
  return nullptr;
}
inline bool validMqttConfig(const MqttConfig& value){return mqttConfigInvalidField(value)==nullptr;}
// The candidate lives on the heap, never as another 1088-byte HTTP stack frame.
struct MqttCandidateDelete {
  void operator()(MqttConfig* value)const{if(value){volatile uint8_t* p=reinterpret_cast<volatile uint8_t*>(value);for(size_t i=0;i<sizeof *value;++i)p[i]=0;delete value;}}
};
using MqttCandidate=std::unique_ptr<MqttConfig,MqttCandidateDelete>;
template<typename Filesystem,typename File>class MqttConfigStorage {
 public:
  explicit MqttConfigStorage(Filesystem& fs):fs_(fs){}
  bool load(MqttConfig& value){
    // Keep the caller's last known good configuration on any read failure.
    // The boot caller initializes defaults explicitly before loading.
    error_=ErrorCode::None;
    if(!fs_.begin())return fail(ErrorCode::FS_MOUNT);
    // A missing file is normal on a fresh installation, not a corrupt login.
    if(!fs_.exists(path()))return false;
    File file=fs_.open(path(),"r");if(!file)return fail(ErrorCode::FS_OPEN_READ);
    MqttConfig saved={};
    if(file.size()!=sizeof saved){file.close();return fail(ErrorCode::FS_CONFIG_CORRUPT);}
    const bool ok=file.read(reinterpret_cast<uint8_t*>(&saved),sizeof saved)==sizeof saved;
    file.close();if(!ok)return fail(ErrorCode::FS_READ);
    if(!validMqttConfig(saved))return fail(ErrorCode::FS_CONFIG_CORRUPT);
    value=saved;return true;
  }
  bool save(MqttConfig& current,MqttConfig& next){
    error_=ErrorCode::None;invalidField_=nullptr;sealMqttConfig(next);
    if((invalidField_=mqttConfigInvalidField(next)))return fail(ErrorCode::MQTT_CONFIG_FIELD);
    mqttCheckpoint(MqttSavePhase::Mount);
    if(!fs_.begin())return fail(ErrorCode::FS_MOUNT);
    if(!memcmp(&current,&next,sizeof next) && fs_.exists(path()))return true;
    mqttCheckpoint(MqttSavePhase::Open);
    File file=fs_.open(temporary(),"w");if(!file)return fail(ErrorCode::FS_OPEN_WRITE);
    mqttCheckpoint(MqttSavePhase::Write);
    const bool ok=file.write(reinterpret_cast<const uint8_t*>(&next),sizeof next)==sizeof next;
    mqttCheckpoint(MqttSavePhase::Flush);
    file.flush();file.close();
    if(!ok)return cleanup(ErrorCode::FS_WRITE);
    mqttCheckpoint(MqttSavePhase::Verify);
    File verify=fs_.open(temporary(),"r");
    if(!verify)return cleanup(ErrorCode::FS_VERIFY_OPEN);
    bool verified=verify.size()==sizeof next,identical=true;
    uint8_t chunk[128];const auto* expected=reinterpret_cast<const uint8_t*>(&next);
    for(size_t offset=0;verified && offset<sizeof next;){
      const size_t remaining=sizeof next-offset,bytes=remaining<sizeof chunk?remaining:sizeof chunk;
      verified=static_cast<size_t>(verify.read(chunk,bytes))==bytes;
      if(verified && memcmp(chunk,expected+offset,bytes))identical=false;
      offset+=bytes;
    }
    verify.close();if(!verified)return cleanup(ErrorCode::FS_VERIFY_READ);
    if(!identical)return cleanup(ErrorCode::FS_VERIFY_CONTENT);
    mqttCheckpoint(MqttSavePhase::Rename);
    if(!fs_.rename(temporary(),path()))return cleanup(ErrorCode::FS_RENAME);
    current=next;mqttCheckpoint(MqttSavePhase::Committed);return true;
  }
  static const char* path(){return "/.chaosuhr-mqtt.bin";}
  ErrorCode errorCode()const{return error_;}
  const char* invalidField()const{return invalidField_;}
 private:
  Filesystem& fs_;
  ErrorCode error_=ErrorCode::None;
  const char* invalidField_=nullptr;
  bool fail(ErrorCode code){error_=code;recordError(code);return false;}
  bool cleanup(ErrorCode code){if(!fs_.remove(temporary()))recordError(ErrorCode::FS_REMOVE);return fail(code);}
  static const char* temporary(){return "/.chaosuhr-mqtt.pending";}
};
