#include <string>
#include <set>
#include <vector>
#include <cassert>
#include <cstdio>
#include <ArduinoJson.h>
using String=std::string;
#include "error_response.h"
#include "mqtt_errors.h"
static std::vector<ErrorEvent> serialEvents;
int main(){
  std::set<std::string> ids;size_t count=0;
#define CHAOS_ERROR(symbol,wire_code,wire_component,wire_message,wire_details,wire_action,client) { \
    const auto d=errorDefinition(ErrorCode::symbol);assert(ids.insert(d.code).second); \
    assert(*d.component&&*d.message&&*d.details&&*d.action);JsonDocument document; \
    assert(!deserializeJson(document,errorJson(ErrorCode::symbol,507))); \
    assert(document["code"]==wire_code&&document["status"]==507);++count; }
#include "error_catalog.def"
#undef CHAOS_ERROR
  assert(count>=145);
  ErrorRegistry registry;registry.setSink([](const ErrorEvent& event){serialEvents.push_back(event);});
  registry.record(ErrorCode::MQTT_DNS,0xfffffff0);registry.record(ErrorCode::MQTT_DNS,20);
  assert(registry.size()==1&&registry.at(0).occurrences==2&&registry.at(0).uptimeMs==20);
  assert(serialEvents.size()==1&&serialEvents[0].code==ErrorCode::MQTT_DNS);
  struct SerialBuffer {std::string text;void write(uint8_t ch){text+=static_cast<char>(ch);}void write(const uint8_t* bytes,size_t size){text.append(reinterpret_cast<const char*>(bytes),size);}} serial;
  printErrorEvent(serial,ErrorEvent{ErrorCode::FS_RENAME,42,1});
  assert(serial.text.find("FS-007")!=String::npos&&serial.text.find("uptime=42 ms")!=String::npos);
  assert(serial.text.find(errorDefinition(ErrorCode::FS_RENAME).details)!=String::npos);
  for(unsigned i=0;i<50;++i)registry.record(i&1?ErrorCode::FS_RENAME:ErrorCode::AUTH_CSRF,100+i);
  assert(registry.size()==8&&registry.at(7).uptimeMs==149);
  using T=MqttTransportCause;
  assert(mqttDisconnectCode(7,T::Dns,false)==ErrorCode::MQTT_DNS);
  assert(mqttDisconnectCode(0,T::Dns,false)==ErrorCode::MQTT_DNS);
  assert(mqttDisconnectCode(7,T::AckTimeout,false)==ErrorCode::MQTT_ACK_TIMEOUT);
  assert(mqttDisconnectCode(7,T::Tcp,false)==ErrorCode::MQTT_TCP);
  assert(mqttDisconnectCode(7,T::Timeout,false)==ErrorCode::MQTT_TIMEOUT);
  assert(mqttDisconnectCode(7,T::None,false)==ErrorCode::MQTT_CONNECT_UNKNOWN);
  assert(mqttDisconnectCode(7,T::Tcp,true)==ErrorCode::MQTT_LINK_LOST);
  assert(mqttDisconnectCode(3,T::None,false)==ErrorCode::MQTT_REJECTED);
  assert(mqttDisconnectCode(4,T::None,false)==ErrorCode::MQTT_LOGIN);
  assert(mqttDisconnectCode(5,T::None,false)==ErrorCode::MQTT_PERMISSION);
  assert(mqttDisconnectCode(1,T::None,false)==ErrorCode::MQTT_PROTOCOL);
  assert(mqttDisconnectCode(2,T::None,false)==ErrorCode::MQTT_CLIENT_ID);
  assert(mqttDisconnectCode(6,T::None,false)==ErrorCode::MQTT_TLS);
  assert(mqttDisconnectCode(0,T::None,true)==ErrorCode::None);
  puts("PASS error registry: all catalogue JSON entries, unique IDs, messages/details/actions, bounded history/coalescing/wraparound/serial sink and precise-versus-unknown MQTT causes");
}
