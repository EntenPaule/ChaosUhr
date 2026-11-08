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
  puts("PASS actual ClockMqtt controller: transport callbacks, DNS/TCP/timeout/authorization/general failures, deadline, recovery, deduplicated subscriptions, packet/subscribe faults and deliberate disconnect without false error; fake transport only");
}
