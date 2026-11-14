#pragma once
// Included after auth helpers and currentBrightness(). No EEPROM writes.
static void brightnessStatusApi(){
 if(!auth())return;
 const auto decision=currentBrightness();JsonDocument doc;
 doc["nightLedsOff"]=brightnessPolicy.nightOff;doc["mqttBrightness"]=brightnessPolicy.mqtt;
 doc["source"]=brightnessSourceName(decision.source);doc["percent"]=decision.percent;
 if(brightnessRemote.valid)doc["mqttValue"]=brightnessRemote.percent;else doc["mqttValue"]=nullptr;
 doc["connected"]=clockMqtt.connected();doc["mqttActive"]=clockMqtt.brightnessActive();
 doc["conflict"]=clockMqtt.brightnessConflict();doc["topic"]=clockMqtt.brightnessTopic();
 doc["configError"]=errorText(errorDefinition(brightnessStorage.errorCode()).code);
 String result;serializeJson(doc,result);server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",result);
}
static void brightnessSaveApi(){
 if(!auth())return;
 if(!publicAccess.validMutationToken()){sendError(server,403,ErrorCode::AUTH_CSRF);return;}
 if(server.args()!=2 || server.argName(0)==server.argName(1)){sendError(server,400,ErrorCode::CFG_VALUE);return;}
 BrightnessPolicy next=brightnessPolicy;
 for(int i=0;i<server.args();++i){
  const String key=server.argName(i),value=server.arg(i);
  if(value!="0"&&value!="1"){sendError(server,400,ErrorCode::CFG_VALUE);return;}
  if(key=="nightLedsOff")next.nightOff=value=="1";
  else if(key=="mqttBrightness")next.mqtt=value=="1";
  else {sendError(server,400,ErrorCode::CFG_UNKNOWN);return;}
 }
 if(!brightnessStorage.save(brightnessPolicy,next)){sendError(server,507,brightnessStorage.errorCode());return;}
 clockMqtt.setBrightnessControl(brightnessPolicy.mqtt);
 if(!brightnessPolicy.mqtt)brightnessRemote.clear();
 server.send(200,"application/json","{\"ok\":true}");
}

static void brightnessReleaseApi(){
 if(!auth())return;
 if(!publicAccess.validMutationToken()){sendError(server,403,ErrorCode::AUTH_CSRF);return;}
 if(server.args()){sendError(server,400,ErrorCode::CFG_VALUE);return;}
 if(!clockMqtt.releaseBrightness()){sendError(server,503,ErrorCode::MQTT_BRIGHTNESS_PUBLISH);return;}
 server.send(202,"application/json","{\"ok\":true,\"queued\":true,\"brokerConfirmed\":false}");
}
