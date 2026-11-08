#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "chaos_errors.h"

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
inline bool validMqttConfig(const MqttConfig& value){
  if(value.magic!=0x4d513130 || value.version!=1 || value.enabled>1 || value.discovery>1 ||
     value.reserved || value.reserved2 || value.crc!=mqttConfigChecksum(value) || !value.port ||
     value.staleSeconds<60 || value.staleSeconds>86400 || !boundedText(value.host,true) ||
     !boundedText(value.user) || !boundedText(value.password) || !validMqttTopic(value.base) ||
     !validMqttTopic(value.discoveryPrefix) || !validMqttTopic(value.weatherTopic) ||
     !validMqttTopic(value.temperatureTopic) || (value.enabled&&!value.host[0]) || !value.discoveryPrefix[0])return false;
  for(const char* c=value.host;*c;++c)if(!((*c>='a'&&*c<='z')||(*c>='A'&&*c<='Z')||(*c>='0'&&*c<='9')||strchr(".-_:",*c)))return false;
  for(const auto& sensor:value.sensors)
    if(!validMqttTopic(sensor.topic)||!boundedText(sensor.label)||!boundedText(sensor.unit))return false;
  return true;
}
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
  bool save(MqttConfig& current,MqttConfig next){
    error_=ErrorCode::None;sealMqttConfig(next);
    if(!validMqttConfig(next))return fail(ErrorCode::MQTT_CONFIG_INVALID);
    if(!fs_.begin())return fail(ErrorCode::FS_MOUNT);
    if(!memcmp(&current,&next,sizeof next) && fs_.exists(path()))return true;
    File file=fs_.open(temporary(),"w");if(!file)return fail(ErrorCode::FS_OPEN_WRITE);
    const bool ok=file.write(reinterpret_cast<const uint8_t*>(&next),sizeof next)==sizeof next;
    file.flush();file.close();
    if(!ok)return cleanup(ErrorCode::FS_WRITE);
    File verify=fs_.open(temporary(),"r");MqttConfig checked={};
    if(!verify)return cleanup(ErrorCode::FS_VERIFY_OPEN);
    const bool verified=verify.size()==sizeof checked && verify.read(reinterpret_cast<uint8_t*>(&checked),sizeof checked)==sizeof checked;
    verify.close();if(!verified)return cleanup(ErrorCode::FS_VERIFY_READ);
    if(memcmp(&checked,&next,sizeof next))return cleanup(ErrorCode::FS_VERIFY_CONTENT);
    if(!fs_.rename(temporary(),path()))return cleanup(ErrorCode::FS_RENAME);
    current=next;return true;
  }
  static const char* path(){return "/.chaosuhr-mqtt.bin";}
  ErrorCode errorCode()const{return error_;}
 private:
  Filesystem& fs_;
  ErrorCode error_=ErrorCode::None;
  bool fail(ErrorCode code){error_=code;recordError(code);return false;}
  bool cleanup(ErrorCode code){if(!fs_.remove(temporary()))recordError(ErrorCode::FS_REMOVE);return fail(code);}
  static const char* temporary(){return "/.chaosuhr-mqtt.pending";}
};
