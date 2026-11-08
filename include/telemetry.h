#pragma once
#include <ArduinoJson.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mqtt_config.h"
#include "mqtt_inbox.h"

enum WeatherKind:uint8_t {WEATHER_UNKNOWN,WEATHER_SUN,WEATHER_CLOUD,WEATHER_PARTLY,
                          WEATHER_RAIN,WEATHER_SNOW,WEATHER_STORM,WEATHER_FOG,WEATHER_WIND,WEATHER_NIGHT};
inline WeatherKind weatherKind(const char* text){
  if(!text || strlen(text)>48)return WEATHER_UNKNOWN;
  char value[49];size_t i=0;for(;text[i];++i)value[i]=(text[i]>='A'&&text[i]<='Z')?text[i]+'a'-'A':text[i];value[i]=0;
  if(!strcmp(value,"sunny")||!strcmp(value,"sun")||!strcmp(value,"sonne")||!strcmp(value,"sonnig")||!strcmp(value,"clear"))return WEATHER_SUN;
  if(!strcmp(value,"cloudy")||!strcmp(value,"clouds")||!strcmp(value,"wolken")||!strcmp(value,"wolkig")||!strcmp(value,"bewoelkt"))return WEATHER_CLOUD;
  if(!strcmp(value,"partlycloudy")||!strcmp(value,"partly-cloudy"))return WEATHER_PARTLY;
  if(!strcmp(value,"rainy")||!strcmp(value,"rain")||!strcmp(value,"pouring")||!strcmp(value,"regen"))return WEATHER_RAIN;
  if(!strcmp(value,"snowy")||!strcmp(value,"snow")||!strcmp(value,"snowy-rainy")||!strcmp(value,"schnee"))return WEATHER_SNOW;
  if(!strcmp(value,"lightning")||!strcmp(value,"lightning-rainy")||!strcmp(value,"thunderstorm")||!strcmp(value,"gewitter"))return WEATHER_STORM;
  if(!strcmp(value,"fog")||!strcmp(value,"nebel"))return WEATHER_FOG;
  if(!strcmp(value,"windy")||!strcmp(value,"windy-variant")||!strcmp(value,"wind"))return WEATHER_WIND;
  if(!strcmp(value,"clear-night"))return WEATHER_NIGHT;
  return WEATHER_UNKNOWN;
}
inline bool validReadingNumber(const char* text,double& value){
  if(!text || !*text || strlen(text)>40)return false;
  char* end=nullptr;value=strtod(text,&end);
  return end && !*end && isfinite(value) && value>=-1000000 && value<=1000000;
}
inline bool unavailableReading(const char* value){
  return !value || !*value || !strcmp(value,"unknown") || !strcmp(value,"unavailable") || !strcmp(value,"null");
}
inline void displayAscii(const char* source,char* output,size_t capacity){
  if(!capacity)return;
  size_t at=0;
  for(size_t i=0;source && source[i] && at+1<capacity;++i){
    const uint8_t c=source[i];
    if(c>=32 && c<127){output[at++]=c;continue;}
    if(c==0xc2 && static_cast<uint8_t>(source[i+1])==0xb0){output[at++]='~';++i;continue;}
    if(c==0xc3 && source[i+1]){
      const uint8_t next=source[++i];char ch='?';
      if(next==0xa4||next==0x84)ch=next==0xa4?'a':'A';
      if(next==0xb6||next==0x96)ch=next==0xb6?'o':'O';
      if(next==0xbc||next==0x9c)ch=next==0xbc?'u':'U';
      if(next==0x9f)ch='s';
      output[at++]=ch;continue;
    }
    if(c>=0x80 && c<0xc0)continue;
    output[at++]='?';
  }
  output[at]=0;
}
template<typename T>struct TimedReading {
  T value={};uint32_t receivedAt=0;bool valid=false;
  bool fresh(uint32_t now,uint32_t seconds)const{return valid && static_cast<uint32_t>(now-receivedAt)<seconds*1000UL;}
  void set(const T& next,uint32_t now){value=next;receivedAt=now;valid=true;}
};
struct SensorReading {char text[33]={};uint32_t receivedAt=0;bool valid=false;
  bool fresh(uint32_t now,uint32_t seconds)const{return valid && static_cast<uint32_t>(now-receivedAt)<seconds*1000UL;}
};
class Telemetry {
 public:
  TimedReading<WeatherKind> condition;TimedReading<double> temperature;
  SensorReading sensors[SENSOR_SLOTS];
  void clear(){condition={};temperature={};for(auto& value:sensors)value={};}
  bool weatherAvailable(uint32_t now,uint32_t ttl)const{return condition.fresh(now,ttl)||temperature.fresh(now,ttl);}
  bool sensorsAvailable(uint32_t now,uint32_t ttl)const{for(const auto& s:sensors)if(s.fresh(now,ttl))return true;return false;}
  bool receive(const MqttConfig& config,const char* topic,const char* payload,size_t size,uint32_t now){
    if(!payload || !size || size>MQTT_PAYLOAD_LIMIT || memchr(payload,0,size))return false;
    char text[MQTT_PAYLOAD_LIMIT+1];memcpy(text,payload,size);text[size]=0;
    size_t begin=0;while(begin<size && (text[begin]==' '||text[begin]=='\r'||text[begin]=='\n'||text[begin]=='\t'))++begin;
    while(size>begin && (text[size-1]==' '||text[size-1]=='\r'||text[size-1]=='\n'||text[size-1]=='\t'))text[--size]=0;
    if(begin){memmove(text,text+begin,size-begin+1);size-=begin;}
    const bool isWeather=config.weatherTopic[0]&&!strcmp(topic,config.weatherTopic);
    const bool isTemperature=config.temperatureTopic[0]&&!strcmp(topic,config.temperatureTopic);
    bool relevant=isWeather||isTemperature;
    for(const auto& s:config.sensors)if(s.topic[0]&&!strcmp(topic,s.topic))relevant=true;
    if(!relevant)return false;
    JsonDocument document;
    const char* state=text;const char* weather=text;bool numeric=false,hasWeather=true;double number=0;
    if(text[0]=='"'){
      if(deserializeJson(document,text,size) || !document.is<const char*>())return false;
      state=weather=document.as<const char*>();
    }else if(text[0]=='{' || text[0]=='['){
      if(deserializeJson(document,text,size,DeserializationOption::NestingLimit(4)) || !document.is<JsonObject>())return false;
      JsonVariantConst value=document["value"];
      if(value.isNull())value=document["val"];
      if(value.isNull())value=document["state"];
      if(value.is<bool>())state=value.as<bool>()?"an":"aus";
      else if(value.is<const char*>())state=value.as<const char*>();
      else if(value.is<double>()){number=value.as<double>();numeric=isfinite(number)&&fabs(number)<=1000000;state=nullptr;}
      else state=nullptr;
      hasWeather=!document["condition"].isNull() || value.is<const char*>();
      weather=document["condition"].is<const char*>()?document["condition"].as<const char*>():state;
      if(document["temperature"].is<double>()){
        const double t=document["temperature"].as<double>();
        if(isWeather && isfinite(t)&&t>=-100&&t<=100)temperature.set(t,now);
        if(isTemperature){number=t;numeric=isfinite(t)&&fabs(t)<=100;state=nullptr;}
      }
    }
    if(!numeric && state)numeric=validReadingNumber(state,number);
    if(isWeather && hasWeather){const auto kind=weatherKind(weather);if(kind!=WEATHER_UNKNOWN)condition.set(kind,now);else if(unavailableReading(weather))condition.valid=false;}
    if(isTemperature){if(numeric && number>=-100 && number<=100)temperature.set(number,now);else temperature.valid=false;}
    for(uint8_t i=0;i<SENSOR_SLOTS;++i){
      if(!config.sensors[i].topic[0] || strcmp(topic,config.sensors[i].topic))continue;
      auto& reading=sensors[i];
      if(numeric){snprintf(reading.text,sizeof reading.text,"%.1f",number);reading.valid=true;}
      else if(!unavailableReading(state) && strlen(state)<=64){displayAscii(state,reading.text,sizeof reading.text);reading.valid=reading.text[0]!=0;}
      else reading.valid=false;
      if(reading.valid)reading.receivedAt=now;
    }
    return true;
  }
};
