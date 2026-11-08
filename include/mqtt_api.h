#pragma once
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
  document["lastReason"]=clockMqtt.lastReason();document["storageReady"]=animations.ready();
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
  if(!publicAccess.validMutationToken()){server.send(403,"text/plain","Sitzungstoken fehlt.");return;}
  if(!server.args() || server.args()>24){server.send(400,"text/plain","Ungueltige MQTT-Konfiguration.");return;}
  MqttConfig next=mqttConfig;bool clearPassword=false,newPassword=false;
  for(int i=0;i<server.args();++i){
    const String key=server.argName(i),value=server.arg(i);bool accepted=true;
    for(int j=0;j<i;++j)if(key==server.argName(j)){server.send(400,"text/plain","Doppelter Parameter.");return;}
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
    else accepted=false;
    if(!accepted){server.send(400,"text/plain","Unbekannter Parameter oder ungueltiger MQTT-Wert.");return;}
  }
  if(clearPassword && newPassword){server.send(400,"text/plain","Passwort entweder ersetzen oder loeschen.");return;}
  if(clearPassword)memset(next.password,0,sizeof next.password);
  sealMqttConfig(next);
  if(!validMqttConfig(next)){server.send(400,"text/plain","MQTT-Konfiguration pruefen: Host, Port, Topic und Wertebereiche.");return;}
  if(!animations.ready()){server.send(503,"text/plain","Zuerst den Datenspeicher unter Eigene Animationen / Speicher einrichten.");return;}
  if(!mqttStorage.save(mqttConfig,next)){server.send(507,"text/plain","MQTT-Konfiguration konnte nicht gespeichert werden.");return;}
  telemetry.clear();clockMqtt.reconfigure();server.send(200,"application/json","{\"ok\":true}");
}
