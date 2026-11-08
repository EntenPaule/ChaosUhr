#include "telemetry.h"
#include "weather_icons.h"
#include "memory_animation_fs.h"
#include "animation_library.h"
#include <cassert>
#include <cstdio>
#include <string>

int main(){
  MqttConfig config=defaultMqttConfig();assert(validMqttConfig(config));
  strcpy(config.host,"broker.example.test");strcpy(config.weatherTopic,"test/weather");strcpy(config.temperatureTopic,"test/temperature");
  strcpy(config.sensors[0].topic,"test/living-room");strcpy(config.sensors[0].label,"Wohnzimmer");strcpy(config.sensors[0].unit,"C");
  config.enabled=1;sealMqttConfig(config);assert(validMqttConfig(config));
  Telemetry telemetry;
  auto receive=[&](const char* topic,const char* value,uint32_t now=100){return telemetry.receive(config,topic,value,strlen(value),now);};
  assert(receive("test/weather","{\"condition\":\"rainy\",\"temperature\":12.3}"));
  assert(telemetry.condition.value==WEATHER_RAIN&&fabs(telemetry.temperature.value-12.3)<0.000001);
  assert(receive("test/temperature"," -2.5\n"));assert(telemetry.temperature.value==-2.5);
  assert(receive("test/living-room","{\"val\":21.4}"));assert(!strcmp(telemetry.sensors[0].text,"21.4"));
  assert(receive("test/living-room","{\"state\":false}"));assert(!strcmp(telemetry.sensors[0].text,"aus"));
  assert(receive("test/living-room","unavailable"));assert(!telemetry.sensorsAvailable(100,60));
  assert(receive("test/living-room","21.5",UINT32_MAX-500));assert(telemetry.sensors[0].fresh(100,60));assert(!telemetry.sensors[0].fresh(60000,60));
  assert(!receive("wrong/topic","23"));assert(!receive("test/weather","{broken}"));
  assert(receive("test/temperature","nan"));assert(!telemetry.temperature.valid);
  assert(receive("test/temperature","9999"));assert(!telemetry.temperature.valid);
  std::string large(513,'X');assert(!telemetry.receive(config,"test/weather",large.data(),large.size(),1));
  char unicode[32];displayAscii("K\xc3\xbc" "che 21\xc2\xb0" "C",unicode,sizeof unicode);assert(!strcmp(unicode,"Kuche 21~C"));
  assert(weatherKind("SonNe")==WEATHER_SUN&&weatherKind("cloudy")==WEATHER_CLOUD&&weatherKind("clear-night")==WEATHER_NIGHT);
  for(uint8_t kind=WEATHER_SUN;kind<=WEATHER_NIGHT;++kind){unsigned pixels=0;
    weatherIcon(static_cast<WeatherKind>(kind),1234,[&](int x,int y,uint32_t color){assert(x>=0&&x<20&&y>=0&&y<9&&color<=0xffffff);++pixels;});assert(pixels);
  }
  MqttInbox inbox;const std::string payload="{\"value\":22}";
  assert(inbox.receive("test/sensor",reinterpret_cast<const uint8_t*>(payload.data()),5,0,payload.size()));
  MqttInbox::Frame frame;assert(!inbox.pop(frame));
  assert(inbox.receive("test/sensor",reinterpret_cast<const uint8_t*>(payload.data()+5),payload.size()-5,5,payload.size()));
  assert(inbox.pop(frame)&&frame.size==payload.size()&&!strcmp(frame.payload,payload.c_str()));
  assert(!inbox.receive("x",reinterpret_cast<const uint8_t*>(payload.data()),1,1,payload.size()));
  assert(!inbox.receive("x",reinterpret_cast<const uint8_t*>(large.data()),large.size(),0,large.size()));
  for(int i=0;i<8;++i)assert(inbox.receive("x",reinterpret_cast<const uint8_t*>("1"),1,0,1));
  assert(inbox.dropped()==2);
  MemoryAnimationFS fs;MqttConfigStorage<MemoryAnimationFS,AnimationFile> store(fs);auto current=defaultMqttConfig();
  assert(store.save(current,config));MqttConfig rebooted;assert(store.load(rebooted)&&!memcmp(&config,&rebooted,sizeof config));
  auto changed=config;strcpy(changed.password,"fixture-mqtt-password");fs.renameFails=true;
  assert(!store.save(current,changed));assert(!memcmp(&current,&config,sizeof config));fs.renameFails=false;
  assert(store.load(rebooted)&&!memcmp(&config,&rebooted,sizeof config));
  fs.mountable=false;assert(!store.save(current,changed)&&fs.formats==0);
  MemoryAnimationFS full;AnimationLibrary<MemoryAnimationFS,AnimationFile> library(full);library.begin();
  library.setReserveGuard([](size_t){return false;});library.startUpload(true);
  library.writeUpload(reinterpret_cast<const uint8_t*>("test"),4);assert(library.status()==507);
  library.endUpload();assert(!library.finishUpload(true)&&full.files.empty()&&full.formats==0);
  puts("PASS telemetry: MQTT chunk bounds/overflow, weather and sensor formats, units, stale/invalid data, icons, atomic MQTT configuration and no autoformat");
}
