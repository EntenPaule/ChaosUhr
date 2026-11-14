#include <ArduinoJson.h>
#include "brightness_storage.h"
#include "memory_animation_fs.h"

#include <cassert>
#include <cstdio>
#include <vector>
using String=std::string;
#include "error_response.h"
static bool authorized=true,csrf=true;
static bool auth(){return authorized;}
static struct {bool validMutationToken(){return csrf;}} publicAccess;
static MemoryAnimationFS filesystem;
static BrightnessPolicy brightnessPolicy;static BrightnessOverride brightnessRemote;
static BrightnessStorage<MemoryAnimationFS,AnimationFile> brightnessStorage(filesystem);
static struct {
 bool allowRelease=false;bool connected(){return false;}bool brightnessActive(){return false;}bool brightnessConflict(){return false;}
 const char* brightnessTopic(){return "fixture/brightness/set";}void setBrightnessControl(bool){}
 bool releaseBrightness(){if(!allowRelease)return false;brightnessRemote.clear();return true;}
} clockMqtt;
static BrightnessDecision currentBrightness(){return decideBrightness(false,brightnessPolicy,brightnessRemote,25,6);}
static struct {
 std::vector<std::pair<String,String>> fields;int status=0;String body;
 int args(){return fields.size();}String argName(int i){return fields.at(i).first;}String arg(int i){return fields.at(i).second;}
 void send(int code,const char*,String data){status=code;body=data;}
 void sendHeader(const char*,const char*){}
} server;
#include "brightness_api.h"
int main(){
 server.fields={{"nightLedsOff","1"},{"mqttBrightness","1"}};
 authorized=false;brightnessSaveApi();assert(filesystem.files.empty());authorized=true;
 csrf=false;brightnessSaveApi();assert(server.status==403&&filesystem.files.empty());csrf=true;
 brightnessSaveApi();assert(server.status==200&&brightnessPolicy.nightOff&&brightnessPolicy.mqtt);
 const auto writes=filesystem.writeCalls;brightnessSaveApi();assert(filesystem.writeCalls==writes);
 server.fields={{"nightLedsOff","0"},{"nightLedsOff","0"}};brightnessSaveApi();assert(server.status==400&&brightnessPolicy.nightOff);
 server.fields={{"nightLedsOff","0"},{"mqttBrightness","0"}};filesystem.renameFails=true;
 brightnessSaveApi();assert(server.status==507&&brightnessPolicy.nightOff);filesystem.renameFails=false;
 assert(brightnessRemote.receive("0",1));brightnessSaveApi();assert(server.status==200&&!brightnessRemote.valid);
 server.fields.clear();assert(brightnessRemote.receive("50",2));brightnessReleaseApi();assert(server.status==503&&brightnessRemote.valid);
 clockMqtt.allowRelease=true;brightnessReleaseApi();assert(server.status==202&&!brightnessRemote.valid&&server.body.find("false")!=String::npos);
 brightnessStatusApi();assert(server.status==200&&server.body.find("GUI")!=String::npos);
 puts("PASS actual brightness HTTP API: auth/CSRF, duplicate/invalid fields, atomic failed save, changed-only writes and safe status");
}
