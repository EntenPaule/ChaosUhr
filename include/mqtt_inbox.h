#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static constexpr size_t MQTT_PAYLOAD_LIMIT=512;
// Callers serialize access when MQTT callbacks run on another CPU/task.
class MqttInbox {
 public:
  struct Frame {char topic[96];char payload[MQTT_PAYLOAD_LIMIT+1];size_t size=0;};
  bool receive(const char* topic,const uint8_t* data,size_t length,size_t index,size_t total){
    if(!topic || !data || !total || total>MQTT_PAYLOAD_LIMIT || strnlen(topic,96)>=96 ||
       index>total || length>total-index){assembling_=false;return false;}
    if(index==0){strcpy(current_.topic,topic);current_.size=0;assembling_=true;expected_=total;}
    if(!assembling_ || index!=current_.size || total!=expected_ || strcmp(topic,current_.topic)){
      assembling_=false;return false;
    }
    memcpy(current_.payload+index,data,length);current_.size+=length;
    if(current_.size==total){
      current_.payload[total]=0;
      if(count_==6){read_=(read_+1)%6;--count_;++dropped_;}
      frames_[(read_+count_)%6]=current_;++count_;assembling_=false;
    }
    return true;
  }
  bool pop(Frame& frame){if(!count_)return false;frame=frames_[read_];read_=(read_+1)%6;--count_;return true;}
  void clear(){read_=count_=0;assembling_=false;}
  uint32_t dropped()const{return dropped_;}
 private:
  Frame frames_[6]={},current_={};uint8_t read_=0,count_=0;
  size_t expected_=0;bool assembling_=false;uint32_t dropped_=0;
};
