#include <string>
#include <vector>
#include <utility>
#include <cassert>
#include <cstdio>
#include "settings.h"
#include "telemetry.h"
#include "memory_animation_fs.h"
using String=std::string;
static uint32_t millis(){return 100;}
static MqttConfig mqttConfig=defaultMqttConfig();static Telemetry telemetry;
static MemoryAnimationFS filesystem;
static MqttConfigStorage<MemoryAnimationFS,AnimationFile> mqttStorage(filesystem);
static struct {bool readyValue=true;bool ready(){return readyValue;}} animations;
static struct {bool token=true;bool validMutationToken(){return token;}} publicAccess;
static struct {unsigned reconfigurations=0;const char* base(){return "test-clock";}bool connected(){return false;}uint8_t lastReason(){return 0;}ErrorCode errorCode(){return ErrorCode::None;}void reconfigure(){++reconfigurations;}} clockMqtt;
static bool authorized=true;
static bool auth(){return authorized;}
static struct {
  std::vector<std::pair<String,String>> values;int status=0;String response;
  int args(){return values.size();}String argName(int i){return values.at(i).first;}String arg(int i){return values.at(i).second;}
  void send(int code,const char*,const String& body){status=code;response=body;}
} server;
#include "mqtt_api.h"
int main(){
  strcpy(mqttConfig.password,"fixture-previous-password");sealMqttConfig(mqttConfig);
  mqttConfigApi();assert(server.status==200&&server.response.find("fixture-previous-password")==String::npos);
  JsonDocument response;assert(!deserializeJson(response,server.response));assert(response["sensors"].size()==4&&response["hasPassword"].as<bool>());
  server.values={{"enabled","1"},{"host","mqtt.example.test"},{"sensor0Topic","rooms/living"},{"sensor0Label","Wohnzimmer"},{"sensor0Unit","C"},{"password",""}};
  publicAccess.token=false;mqttSaveApi();assert(server.status==403&&filesystem.files.empty());publicAccess.token=true;
  mqttSaveApi();assert(server.status==200&&mqttConfig.enabled&&clockMqtt.reconfigurations==1);
  assert(!strcmp(mqttConfig.password,"fixture-previous-password"));
  const auto good=mqttConfig;const auto files=filesystem.files;
  for(const auto& bad:std::vector<std::vector<std::pair<String,String>>>{{{"port","0"}},{{"weatherTopic","x/#"}},{{"host","x"},{"host","y"}},{{"sensor4Topic","x"}},{{"password",String(65,'a')}},{{"clearPassword","1"},{"password","new-password"}}}){
    server.values=bad;mqttSaveApi();assert(server.status==400&&!memcmp(&mqttConfig,&good,sizeof good));
  }
  server.values={{"clearPassword","1"}};filesystem.renameFails=true;mqttSaveApi();assert(server.status==507&&!memcmp(&mqttConfig,&good,sizeof good));filesystem.renameFails=false;
  MqttConfig rebooted;assert(mqttStorage.load(rebooted)&&!memcmp(&rebooted,&good,sizeof good));
  animations.readyValue=false;mqttSaveApi();assert(server.status==503&&filesystem.formats==0);animations.readyValue=true;
  mqttSaveApi();assert(server.status==200&&!mqttConfig.password[0]);
  authorized=false;server.status=0;mqttSaveApi();assert(server.status==0);
  puts("PASS actual MQTT API: password redaction/retention/deletion, four sensors, auth/CSRF, strict values/topics/duplicates, atomic failures and no autoformat");
}
