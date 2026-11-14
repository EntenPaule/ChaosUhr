// Exercise the pinned production MQTT wire parser, not a callback-only mock.
#include "Packets/Parser.h"
#include "mqtt_inbox.h"
#include "brightness_control.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
using namespace espMqttClientInternals;
std::vector<uint8_t> packet(const std::string& value,bool retain=true,uint8_t qos=0){
 const std::string topic="test-clock/brightness/set";
 std::vector<uint8_t> p={static_cast<uint8_t>((retain?0x31:0x30)|(qos<<1)),static_cast<uint8_t>(2+topic.size()+value.size()+(qos?2:0)),0,static_cast<uint8_t>(topic.size())};
 p.insert(p.end(),topic.begin(),topic.end());if(qos){p.push_back(0);p.push_back(42);}p.insert(p.end(),value.begin(),value.end());return p;
}
int main(){
 for(uint8_t qos=0;qos<=2;++qos)for(const std::string value:{"0","1","5","20","50","100","AUTO","","101"," 50","50 "}){
  auto bytes=packet(value,true,qos);
  for(size_t split=1;split<=bytes.size();++split){
   Parser parser;MqttInbox inbox;BrightnessOverride remote;remote.receive("20",2);
   for(size_t start=0;start<bytes.size();){
    size_t available=std::min(split,bytes.size()-start),used=0;
    while(used<available){size_t n=0;auto result=parser.parse(bytes.data()+start+used,available-used,&n);assert(n);used+=n;
     assert(result!=ParserResult::protocolError);
     if(result==ParserResult::packet){const auto& p=parser.getPacket();assert(inbox.receive(p.variableHeader.topic,p.payload.data,p.payload.length,p.payload.index,p.payload.total));}
    }start+=used;
   }
   MqttInbox::Frame f;assert(inbox.pop(f));assert(!inbox.pop(f));assert(inbox.takeError()==ErrorCode::None);
   remote.receive(f.payload,f.size);
   if(value=="AUTO")assert(!remote.valid);
   else if(value.empty()||value=="101"||value==" 50"||value=="50 ")assert(remote.percent==20);
   else assert(remote.percent==std::stoi(value));
   BrightnessPolicy policy{true,true};assert(decideBrightness(true,policy,remote,10,2).scale==0);
  }
 }
 // Consecutive packets in a single TCP delivery, including an empty retained deletion.
 Parser parser;MqttInbox inbox;std::vector<uint8_t> bytes;
 for(const auto* s:{"50","","AUTO","20"}){auto p=packet(s);bytes.insert(bytes.end(),p.begin(),p.end());}
 size_t pos=0;while(pos<bytes.size()){size_t n=0;auto r=parser.parse(bytes.data()+pos,bytes.size()-pos,&n);assert(n);pos+=n;if(r==ParserResult::packet){const auto&p=parser.getPacket();assert(inbox.receive(p.variableHeader.topic,p.payload.data,p.payload.length,p.payload.index,p.payload.total));}}
 MqttInbox::Frame frame;for(const auto* s:{"50","","AUTO","20"}){assert(inbox.pop(frame));assert(std::string(frame.payload,frame.size)==s);}assert(!inbox.pop(frame));
 assert(inbox.receive("x",nullptr,0,0,0));assert(inbox.pop(frame)&&frame.size==0);
 assert(!inbox.receive("x",nullptr,1,0,1));assert(inbox.takeError()==ErrorCode::MQTT_FRAGMENT);
 assert(inbox.receive("x",reinterpret_cast<const uint8_t*>("5"),1,0,2));
 assert(!inbox.receive("y",reinterpret_cast<const uint8_t*>("0"),1,1,2));assert(inbox.takeError()==ErrorCode::MQTT_FRAGMENT);
 assert(!inbox.receive("x",nullptr,0,1,0));assert(inbox.takeError()==ErrorCode::MQTT_FRAGMENT);
 puts("PASS actual MQTT wire parser: all TCP splits, retained, empty payload, back-to-back messages, strict brightness, night priority and invalid fragment rejection");
}
