#include "clock_mqtt.h"
#include <cassert>
#include <cstdio>
uint32_t fakeMillis=0;
FakeESP ESP;
FakeWiFi WiFi;
bool FakeESP::eraseConfig(){return true;}
int main(){
  auto config=defaultMqttConfig();config.enabled=1;config.discovery=0;
  strcpy(config.host,"fixture-broker");strcpy(config.weatherTopic,"house/weather");strcpy(config.temperatureTopic,"house/weather");
  strcpy(config.sensors[0].topic,"house/weather");strcpy(config.sensors[1].topic,"house/room");sealMqttConfig(config);
  Telemetry telemetry;ClockMqtt clock(config,telemetry);auto& client=*espMqttClientAsync::latest;
  clock.loop(true);assert(client.connections==1&&!clock.connected());
  client.failed(-55,0);clock.loop(true);assert(clock.errorCode()==ErrorCode::MQTT_DNS); // stale USER_OK must not hide DNS failure
  fakeMillis=16000;clock.loop(true);client.failed(ERR_CONN);clock.loop(true);assert(clock.errorCode()==ErrorCode::MQTT_TCP);
  fakeMillis=32000;clock.loop(true);client.failed(ERR_TIMEOUT);clock.loop(true);assert(clock.errorCode()==ErrorCode::MQTT_TIMEOUT);
  fakeMillis=48000;clock.loop(true);client.failed(0,5);clock.loop(true);assert(clock.errorCode()==ErrorCode::MQTT_PERMISSION);
  fakeMillis=64000;clock.loop(true);client.failed(0,7);clock.loop(true);assert(clock.errorCode()==ErrorCode::MQTT_CONNECT_UNKNOWN);
  fakeMillis=80000;clock.loop(true);fakeMillis=91000;clock.loop(true);assert(clock.errorCode()==ErrorCode::MQTT_TIMEOUT);
  client.connectionAccepted();clock.loop(true);assert(clock.connected()&&clock.errorCode()==ErrorCode::None);
  assert(client.subscriptions.size()==2); // duplicate weather/temperature/sensor topic subscribed once
  client.ackTimedOut();clock.loop(true);assert(errorRegistry().last()==ErrorCode::MQTT_ACK_TIMEOUT&&clock.connected());
  client.packetFailed();clock.loop(true);assert(errorRegistry().last()==ErrorCode::MQTT_PACKET_MEMORY);
  client.subscribeFailed();clock.loop(true);assert(errorRegistry().last()==ErrorCode::MQTT_SUBSCRIBE);
  client.failed(0);clock.loop(true);assert(clock.errorCode()==ErrorCode::MQTT_LINK_LOST);
  fakeMillis=107000;clock.loop(true);client.connectionAccepted();clock.loop(true);
  config.enabled=0;clock.reconfigure();clock.loop(true);clock.loop(true);
  assert(clock.errorCode()==ErrorCode::None&&!clock.connected());
  // Same real controller, now with command permission. No filesystem attached.
  config.enabled=1;strcpy(config.base,"test-clock");clock.reconfigure();clock.setBrightnessControl(true);clock.loop(true);client.connectionAccepted();clock.loop(true);
  assert(clock.brightnessActive()&&!clock.brightnessConflict());assert(!strcmp(clock.brightnessTopic(),"test-clock/brightness/set"));
  auto message=[&](const char* value,size_t length=0){if(!length)length=strlen(value);client.messageCallback({0,false,true,0},clock.brightnessTopic(),reinterpret_cast<const uint8_t*>(value),length,0,length);clock.loop(true);};
  assert(client.subscriptions.back()=="test-clock/brightness/set");
  message("50");assert(clock.brightnessValue().valid&&clock.brightnessValue().percent==50);
  message("100");message("101");assert(clock.brightnessValue().percent==100);
  message("0");assert(clock.brightnessValue().valid&&clock.brightnessValue().percent==0);
  message("AUTO");assert(!clock.brightnessValue().valid);
  message("5");
  client.messageCallback({},clock.brightnessTopic(),reinterpret_cast<const uint8_t*>("5"),1,0,2);clock.loop(true);assert(clock.brightnessValue().percent==5);
  client.messageCallback({},clock.brightnessTopic(),reinterpret_cast<const uint8_t*>("0"),1,1,2);clock.loop(true);assert(clock.brightnessValue().percent==50);
  message("5");client.failed(0);clock.loop(true);assert(clock.brightnessValue().percent==5);
  fakeMillis+=16000;clock.loop(true);client.connectionAccepted();clock.loop(true);message("5");assert(clock.brightnessValue().percent==5);
  client.publishFails=true;assert(!clock.releaseBrightness()&&clock.brightnessValue().valid);client.publishFails=false;
  assert(clock.releaseBrightness()&&!clock.brightnessValue().valid&&client.lastPayload=="AUTO"&&client.lastRetain&&client.lastQos==1);
  message("50");clock.setBrightnessControl(false);clock.loop(true);assert(!clock.brightnessValue().valid);message("100");assert(!clock.brightnessValue().valid);
  clock.setBrightnessControl(true);clock.loop(true);message("50");
  strcpy(config.base,"new-base");clock.reconfigure();assert(!clock.brightnessValue().valid);clock.loop(true);clock.loop(true);client.connectionAccepted();clock.loop(true);
  strcpy(config.weatherTopic,"new-base/brightness/set");clock.reconfigure();clock.loop(true);clock.loop(true);assert(clock.brightnessConflict()&&!clock.brightnessActive());
  assert(errorRegistry().last()==ErrorCode::MQTT_BRIGHTNESS_TOPIC);
  client.connectionAccepted();clock.loop(true);
  client.messageCallback({},config.weatherTopic,reinterpret_cast<const uint8_t*>("sunny"),5,0,5);clock.loop(true);assert(telemetry.condition.valid);
  strcpy(config.weatherTopic,"house/weather");clock.reconfigure();clock.loop(true);clock.loop(true);client.connectionAccepted();clock.loop(true);
  message("50");strcpy(config.host,"another-broker");clock.reconfigure();assert(!clock.brightnessValue().valid);
  config.enabled=0;clock.loop(true);assert(!clock.brightnessValue().valid);
  auto rebootConfig=defaultMqttConfig();rebootConfig.enabled=1;strcpy(rebootConfig.host,"fixture-reboot");Telemetry rebootTelemetry;
  ClockMqtt reboot(rebootConfig,rebootTelemetry);reboot.setBrightnessControl(true);assert(!reboot.brightnessValue().valid);
  reboot.loop(true);auto& rebootClient=*espMqttClientAsync::latest;rebootClient.connectionAccepted();reboot.loop(true);
  rebootClient.messageCallback({0,false,true,0},reboot.brightnessTopic(),reinterpret_cast<const uint8_t*>("50"),2,0,2);reboot.loop(true);assert(reboot.brightnessValue().percent==50);
  puts("PASS MQTT brightness: opt-in, retained-equivalent inbox, strict values, reconnect/hold, AUTO queue acceptance/failure, disabled/base/broker invalidation and collision preserves weather");
  puts("PASS actual ClockMqtt controller: transport callbacks, DNS/TCP/timeout/authorization/general failures, deadline, recovery, deduplicated subscriptions, packet/subscribe faults and deliberate disconnect without false error; fake transport only");
}
