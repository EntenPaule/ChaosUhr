#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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
    value=defaultMqttConfig();if(!fs_.begin())return false;
    File file=fs_.open(path(),"r");if(!file)return false;
    MqttConfig saved={};const bool ok=file.size()==sizeof saved &&
      file.read(reinterpret_cast<uint8_t*>(&saved),sizeof saved)==sizeof saved;
    file.close();if(!ok || !validMqttConfig(saved))return false;value=saved;return true;
  }
  bool save(MqttConfig& current,MqttConfig next){
    sealMqttConfig(next);if(!validMqttConfig(next) || !fs_.begin())return false;
    if(!memcmp(&current,&next,sizeof next) && fs_.exists(path()))return true;
    File file=fs_.open(temporary(),"w");if(!file)return false;
    const bool ok=file.write(reinterpret_cast<const uint8_t*>(&next),sizeof next)==sizeof next;
    file.flush();file.close();
    File verify=fs_.open(temporary(),"r");MqttConfig checked={};
    const bool verified=verify && verify.size()==sizeof checked &&
      verify.read(reinterpret_cast<uint8_t*>(&checked),sizeof checked)==sizeof checked;
    if(verify)verify.close();
    if(!ok || !verified || memcmp(&checked,&next,sizeof next) || !fs_.rename(temporary(),path())){
      fs_.remove(temporary());return false;
    }
    current=next;return true;
  }
  static const char* path(){return "/.chaosuhr-mqtt.bin";}
 private:
  Filesystem& fs_;
  static const char* temporary(){return "/.chaosuhr-mqtt.pending";}
};
