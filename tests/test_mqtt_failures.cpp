#include <string>
#include <cassert>
#include <cstdio>
#include <ArduinoJson.h>
#include "mqtt_config.h"
#include "memory_animation_fs.h"
using String=std::string;
#include "error_response.h"
static constexpr char VERSION[]="2.0.11",DEVICE_FAMILY[]="fixture";
static uint32_t millis(){return 12345;}
static constexpr int WL_CONNECTED=3;
enum class SavedWifiState:uint8_t {Missing,Available,Unreadable};
static SavedWifiState savedWifiState(){return SavedWifiState::Available;}
static struct {unsigned getFreeHeap(){return 24000;}} ESP;
static struct {int status(){return WL_CONNECTED;}} WiFi;
static struct {bool connected(){return false;}} clockMqtt;
static MqttConfig mqttConfig=defaultMqttConfig();
static bool authorized=true;
static bool auth(){return authorized;}
static struct {int status=0;String body;void send(int code,const char*,const String& text){status=code;body=text;}} server;
#include "diagnostics_api.h"
int main(){
  MemoryAnimationFS fs;MqttConfigStorage<MemoryAnimationFS,AnimationFile> store(fs);
  static_assert(sizeof(MqttConfig)==1088,"Persisted MQTT v1 layout must not change");
  auto current=defaultMqttConfig(),next=current;next.enabled=1;
  strcpy(next.host,"fixture-private-broker");strcpy(next.password,"fixture-mqtt-private-password");sealMqttConfig(next);
  assert(store.save(current,next));const auto saved=current;
  auto invalid=next;invalid.discoveryPrefix[0]=0;
  assert(!store.save(current,invalid)&&store.errorCode()==ErrorCode::MQTT_CONFIG_FIELD);
  assert(!strcmp(store.invalidField(),"discoveryPrefix"));
  assert(!memcmp(&current,&saved,sizeof saved));
  assert(resetDiagnostics().current.phase==static_cast<unsigned>(MqttSavePhase::Committed));
  const auto original=*fs.files.at(store.path());
  strcpy(next.user,"fixture-user");
  auto attempt=[&](bool& flag,ErrorCode expected){
    flag=true;assert(!store.save(current,next));assert(store.errorCode()==expected);
    assert(!memcmp(&current,&saved,sizeof saved));assert(*fs.files.at(store.path())==original);
    flag=false;MqttConfig reboot=defaultMqttConfig();assert(store.load(reboot));assert(!memcmp(&reboot,&saved,sizeof saved));
  };
  fs.mountable=false;assert(!store.save(current,next)&&store.errorCode()==ErrorCode::FS_MOUNT);fs.mountable=true;
  attempt(fs.openWriteFails,ErrorCode::FS_OPEN_WRITE);
  attempt(fs.writeFails,ErrorCode::FS_WRITE);
  attempt(fs.openReadFails,ErrorCode::FS_VERIFY_OPEN);
  attempt(fs.readFails,ErrorCode::FS_VERIFY_READ);
  attempt(fs.corruptRead,ErrorCode::FS_VERIFY_CONTENT);
  attempt(fs.renameFails,ErrorCode::FS_RENAME);
  fs.removeFails=fs.writeFails=true;assert(!store.save(current,next));
  assert(store.errorCode()==ErrorCode::FS_WRITE && *fs.files.at(store.path())==original);
  fs.removeFails=fs.writeFails=false;
  assert(store.save(current,next));const auto writes=fs.writeCalls;
  assert(store.save(current,next)&&fs.writeCalls==writes); // unchanged saves do not write flash
  auto retained=current;fs.openReadFails=true;
  assert(!store.load(retained)&&store.errorCode()==ErrorCode::FS_OPEN_READ);
  assert(!memcmp(&retained,&current,sizeof current));fs.openReadFails=false;
  fs.readFails=true;assert(!store.load(retained)&&store.errorCode()==ErrorCode::FS_READ);fs.readFails=false;
  (*fs.files.at(store.path()))[4]^=1;
  assert(!store.load(retained)&&store.errorCode()==ErrorCode::FS_CONFIG_CORRUPT);
  assert(!memcmp(&retained,&current,sizeof current));assert(fs.formats==0);
  mqttConfig=current;
  auto& reset=resetDiagnostics();reset.available=true;reset.reason=2;reset.exceptionCause=28;
  recordError(ErrorCode::FS_RENAME);diagnosticsApi();assert(server.status==200);
  assert(server.body.find(current.password)==String::npos&&server.body.find(current.host)==String::npos);
  assert(server.body.find("fixture-user")==String::npos);
  JsonDocument document;assert(!deserializeJson(document,server.body));
  assert(document["reset"]["reason"]=="exception"&&document["reset"]["exceptionCause"]==28);
  assert(document["lastError"]=="FS-007"&&document["errors"].size()<=8);
  assert(document["uptimeMs"]==12345&&document["wifiCredentials"]=="available");
  for(const char* key:{"token","password","ssid","host","user","topic","sensors"})assert(document[key].isNull());
  authorized=false;server.status=0;server.body="";diagnosticsApi();assert(server.status==0&&server.body.empty());
  puts("PASS MQTT faults/diagnostics: mount/open/write/read/verify/rename/cleanup faults, repeated saves/reload, prior file/config preservation, no autoformat or unchanged writes, protected secret-free support JSON");
}
