#pragma once
#include "error_response.h"
// Included after the clock's authenticated HTTP helpers and stable storage.
static void mqttConfigApi(){
  if(!auth())return;
  JsonDocument document;
  document["enabled"]=mqttConfig.enabled;document["host"]=mqttConfig.host;document["port"]=mqttConfig.port;
  document["user"]=mqttConfig.user;document["hasPassword"]=mqttConfig.password[0]!=0;
  document["base"]=mqttConfig.base;document["effectiveBase"]=clockMqtt.base();
  document["discovery"]=mqttConfig.discovery;document["discoveryPrefix"]=mqttConfig.discoveryPrefix;
  document["staleSeconds"]=mqttConfig.staleSeconds;document["weatherTopic"]=mqttConfig.weatherTopic;
  document["temperatureTopic"]=mqttConfig.temperatureTopic;document["connected"]=clockMqtt.connected();
  document["lastReason"]=clockMqtt.lastReason();
  document["configError"]=errorText(errorDefinition(mqttStorage.errorCode()).code);
  document["connectionError"]=errorText(errorDefinition(clockMqtt.errorCode()).code);document["storageReady"]=animations.ready();
  auto failure=[&](const char* key,ErrorCode code){
    if(code==ErrorCode::None)return;
    const auto d=errorDefinition(code);auto entry=document[key].to<JsonObject>();
    entry["code"]=errorText(d.code);entry["component"]=errorText(d.component);entry["message"]=errorText(d.message);entry["details"]=errorText(d.details);entry["action"]=errorText(d.action);
  };
  failure("configFailure",mqttStorage.errorCode());failure("connectionFailure",clockMqtt.errorCode());
  auto sensors=document["sensors"].to<JsonArray>();
  for(uint8_t i=0;i<SENSOR_SLOTS;++i){
    auto entry=sensors.add<JsonObject>();
    entry["topic"]=mqttConfig.sensors[i].topic;entry["label"]=mqttConfig.sensors[i].label;entry["unit"]=mqttConfig.sensors[i].unit;
    entry["fresh"]=telemetry.sensors[i].fresh(millis(),mqttConfig.staleSeconds);
    if(entry["fresh"].as<bool>())entry["value"]=telemetry.sensors[i].text;
  }
  String output;serializeJson(document,output);server.send(200,"application/json",output);
}
template<size_t N>static bool mqttApiText(char (&target)[N],const String& value){
  if(value.length()>=N || memchr(value.c_str(),0,value.length()))return false;
  memcpy(target,value.c_str(),value.length()+1);return boundedText(target);
}
static void mqttSaveApi(){
  if(!auth())return;
  if(!publicAccess.validMutationToken()){sendError(server,403,ErrorCode::AUTH_CSRF);return;}
  if(!server.args()){sendError(server,400,ErrorCode::CFG_EMPTY);return;}
  if(server.args()>24){sendError(server,400,ErrorCode::HTTP_ARGUMENTS);return;}
  MqttConfig next=mqttConfig;bool clearPassword=false,newPassword=false;
  for(int i=0;i<server.args();++i){
    const String key=server.argName(i),value=server.arg(i);bool accepted=true,known=true;
    for(int j=0;j<i;++j)if(key==server.argName(j)){sendError(server,400,ErrorCode::CFG_DUPLICATE);return;}
    if(key=="enabled")accepted=parseSetting(value.c_str(),0,1,next.enabled);
    else if(key=="host")accepted=mqttApiText(next.host,value);
    else if(key=="port")accepted=parseSetting(value.c_str(),1,65535,next.port);
    else if(key=="user")accepted=mqttApiText(next.user,value);
    else if(key=="password"){if(value.length()){accepted=mqttApiText(next.password,value);newPassword=true;}}
    else if(key=="clearPassword"){uint8_t flag;accepted=parseSetting(value.c_str(),0,1,flag);if(accepted)clearPassword=flag;}
    else if(key=="base")accepted=mqttApiText(next.base,value);
    else if(key=="discovery")accepted=parseSetting(value.c_str(),0,1,next.discovery);
    else if(key=="discoveryPrefix")accepted=mqttApiText(next.discoveryPrefix,value);
    else if(key=="staleSeconds")accepted=parseSetting(value.c_str(),60,86400,next.staleSeconds);
    else if(key=="weatherTopic")accepted=mqttApiText(next.weatherTopic,value);
    else if(key=="temperatureTopic")accepted=mqttApiText(next.temperatureTopic,value);
    else if(key.length()==12 && !strncmp(key.c_str(),"sensor",6) && key[6]>='0'&&key[6]<'0'+SENSOR_SLOTS && !strcmp(key.c_str()+7,"Topic"))accepted=mqttApiText(next.sensors[key[6]-'0'].topic,value);
    else if(key.length()==12 && !strncmp(key.c_str(),"sensor",6) && key[6]>='0'&&key[6]<'0'+SENSOR_SLOTS && !strcmp(key.c_str()+7,"Label"))accepted=mqttApiText(next.sensors[key[6]-'0'].label,value);
    else if(key.length()==11 && !strncmp(key.c_str(),"sensor",6) && key[6]>='0'&&key[6]<'0'+SENSOR_SLOTS && !strcmp(key.c_str()+7,"Unit"))accepted=mqttApiText(next.sensors[key[6]-'0'].unit,value);
    else {accepted=false;known=false;}
    if(!accepted){
      ErrorCode code=known?ErrorCode::CFG_VALUE:ErrorCode::CFG_UNKNOWN;
      if(known){
        const bool numeric=key=="enabled"||key=="port"||key=="clearPassword"||key=="discovery"||key=="staleSeconds";
        if(numeric){bool digits=value.length()!=0;for(size_t at=0;at<value.length();++at)if(value[at]<'0'||value[at]>'9')digits=false;code=digits?ErrorCode::CFG_RANGE:ErrorCode::CFG_NUMBER_FORMAT;}
        else {bool control=false;for(size_t at=0;at<value.length();++at)if(static_cast<uint8_t>(value[at])<32||value[at]==127)control=true;code=control?ErrorCode::CFG_TEXT_CHARACTER:ErrorCode::CFG_TEXT_LENGTH;}
      }
      sendError(server,400,code);return;
    }
  }
  if(clearPassword && newPassword){sendError(server,400,ErrorCode::CFG_PASSWORD_CONFLICT);return;}
  if(clearPassword)memset(next.password,0,sizeof next.password);
  bool topics=validMqttTopic(next.base)&&validMqttTopic(next.discoveryPrefix)&&validMqttTopic(next.weatherTopic)&&validMqttTopic(next.temperatureTopic);
  for(const auto& sensor:next.sensors)topics=topics&&validMqttTopic(sensor.topic);
  if(!topics){sendError(server,400,ErrorCode::CFG_TOPIC);return;}
  for(const char* c=next.host;*c;++c)if(!((*c>='a'&&*c<='z')||(*c>='A'&&*c<='Z')||(*c>='0'&&*c<='9')||strchr(".-_:",*c))){sendError(server,400,ErrorCode::CFG_HOST);return;}
  sealMqttConfig(next);
  if(!validMqttConfig(next)){sendError(server,400,ErrorCode::MQTT_CONFIG_INVALID);return;}
  if(!animations.ready()){sendError(server,503,ErrorCode::FS_MOUNT);return;}
  if(!mqttStorage.save(mqttConfig,next)){sendError(server,507,mqttStorage.errorCode());return;}
  telemetry.clear();clockMqtt.reconfigure();server.send(200,"application/json","{\"ok\":true}");
}
