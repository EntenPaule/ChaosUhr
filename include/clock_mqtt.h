#pragma once
#include <Arduino.h>
#include <espMqttClientAsync.h>
#include "device_platform.h"
#include "telemetry.h"
#include "firmware_version.h"
#include "mqtt_errors.h"

// The pinned client does not expose transport errors through DisconnectReason.
// Observe the protected AsyncTCP transport without replacing its own callbacks.
class DiagnosedMqttClient:public espMqttClientAsync {
 public:
  void observePacketError(espMqttClientTypes::OnErrorCallback callback){_onErrorCallback=callback;}
  void observeTransport(std::function<void(MqttTransportCause)> callback){
    transport_=callback;
    _clientAsync.client.onError([](void* arg,AsyncClient*,int8_t error){
      auto* self=static_cast<DiagnosedMqttClient*>(arg);
      const auto cause=error==-55?MqttTransportCause::Dns:error==ERR_TIMEOUT?MqttTransportCause::Timeout:
        (error==ERR_CONN||error==ERR_RST||error==ERR_ABRT||error==ERR_CLSD||error==ERR_RTE)?MqttTransportCause::Tcp:MqttTransportCause::Other;
      if(self->transport_)self->transport_(cause);
    },this);
    _clientAsync.client.onTimeout([](void* arg,AsyncClient*,uint32_t){auto* self=static_cast<DiagnosedMqttClient*>(arg);if(self->transport_)self->transport_(MqttTransportCause::AckTimeout);},this);
  }
 private:std::function<void(MqttTransportCause)> transport_;
};

class ClockMqtt {
 public:
  ClockMqtt(MqttConfig& config,Telemetry& telemetry):config_(config),telemetry_(telemetry){
    client_.onConnect([this](bool){lock();connectedEvent_=true;unlock();});
    client_.observeTransport([this](MqttTransportCause cause){lock();transportCause_=cause;transportEvent_=true;unlock();});
    client_.onDisconnect([this](espMqttClientTypes::DisconnectReason reason){lock();lastReason_=static_cast<uint8_t>(reason);disconnectedEvent_=true;unlock();});
    client_.observePacketError([this](uint16_t,espMqttClientTypes::Error){lock();packetError_=true;unlock();});
    client_.onSubscribe([this](uint16_t,const espMqttClientTypes::SubscribeReturncode* codes,size_t size){for(size_t i=0;i<size;++i)if(codes[i]==espMqttClientTypes::SubscribeReturncode::FAIL){lock();subscribeError_=true;unlock();}});
    client_.onMessage([this](const espMqttClientTypes::MessageProperties&,const char* topic,const uint8_t* data,size_t len,size_t index,size_t total){
      lock();inbox_.receive(topic,data,len,index,total);unlock();
    });
  }
  void reconfigure(){pending_=true;}
  bool connected()const{return client_.connected();}
  uint8_t lastReason(){lock();const auto reason=lastReason_;unlock();return reason;}
  const char* base()const{return base_;}
  ErrorCode errorCode()const{return error_;}
  void loop(bool networkReady){
    const uint32_t now=millis();
    lock();const auto inboxError=inbox_.takeError();unlock();recordError(inboxError);
    lock();const bool disconnected=disconnectedEvent_;disconnectedEvent_=false;
    const bool connectedEvent=connectedEvent_;connectedEvent_=false;
    const auto transport=transportCause_;const uint8_t reason=lastReason_;
    const bool transportEvent=transportEvent_;transportEvent_=false;
    const bool packet=packetError_,subscription=subscribeError_;packetError_=subscribeError_=false;unlock();
    if(disconnected){
      if(!pending_ && networkReady && active_.enabled){error_=mqttDisconnectCode(reason,transport,established_);recordError(error_);}
      established_=false;
    }
    if(transportEvent&&!disconnected&&!pending_&&networkReady&&active_.enabled){
      if(transport==MqttTransportCause::AckTimeout)recordError(ErrorCode::MQTT_ACK_TIMEOUT);
      else {error_=mqttDisconnectCode(7,transport,established_);recordError(error_);}
    }
    if(packet)recordError(ErrorCode::MQTT_PACKET_MEMORY);
    if(subscription)recordError(ErrorCode::MQTT_SUBSCRIBE);
    if(pending_){
      if(!client_.disconnected()){
        if(!disconnectStarted_){
          if(client_.connected()){client_.publish(availability_,0,true,"offline");client_.disconnect(false);}
          else client_.disconnect(true);
          disconnectStarted_=true;disconnectAt_=now;
        }else if(static_cast<uint32_t>(now-disconnectAt_)>5000)client_.disconnect(true);
        return;
      }
      client_.clearQueue(true);
      active_=config_;telemetry_.clear();lock();inbox_.clear();unlock();
      snprintf(id_,sizeof id_,"chaosuhr-%06x",static_cast<unsigned>(deviceIdentifier()));
      if(active_.base[0])snprintf(base_,sizeof base_,"%s",active_.base);else snprintf(base_,sizeof base_,"chaosuhr/%06x",static_cast<unsigned>(deviceIdentifier()));
      snprintf(availability_,sizeof availability_,"%s/availability",base_);
      client_.setServer(active_.host,active_.port).setClientId(id_).setCleanSession(true).setKeepAlive(30).setTimeout(5);
      client_.setCredentials(active_.user[0]?active_.user:nullptr,active_.password[0]?active_.password:nullptr);
      client_.setWill(availability_,0,true,"offline");
      pending_=disconnectStarted_=false;lock();connectedEvent_=false;unlock();lastAttempt_=now-15000;
      error_=ErrorCode::None;established_=false;deadlineReported_=false;
    }
    if(!active_.enabled || !networkReady){if(!client_.disconnected())client_.disconnect(true);return;}
    if(client_.disconnected() && static_cast<uint32_t>(now-lastAttempt_)>=15000){
      lastAttempt_=now;deadlineReported_=false;lock();transportCause_=MqttTransportCause::None;unlock();
      if(!client_.connect()){error_=ErrorCode::MQTT_CONNECT_UNKNOWN;recordError(error_);}
    }
    if(!client_.disconnected() && !connected() && !deadlineReported_ && static_cast<uint32_t>(now-lastAttempt_)>=10000){deadlineReported_=true;error_=ErrorCode::MQTT_TIMEOUT;recordError(error_);}
    if(connectedEvent && connected()){
      established_=true;error_=ErrorCode::None;client_.publish(availability_,0,true,"online");
      if(active_.weatherTopic[0])client_.subscribe(active_.weatherTopic,0);
      if(active_.temperatureTopic[0] && strcmp(active_.temperatureTopic,active_.weatherTopic))client_.subscribe(active_.temperatureTopic,0);
      for(uint8_t i=0;i<SENSOR_SLOTS;++i){
        const char* topic=active_.sensors[i].topic;if(!*topic || !strcmp(topic,active_.weatherTopic) || !strcmp(topic,active_.temperatureTopic))continue;
        bool repeated=false;for(uint8_t j=0;j<i;++j)if(!strcmp(topic,active_.sensors[j].topic))repeated=true;
        if(!repeated)client_.subscribe(topic,0);
      }
      if(active_.discovery)publishDiscovery();
      lastPublish_=now-30000;
    }
    MqttInbox::Frame frame;
    for(uint8_t i=0;i<6;++i){lock();const bool read=inbox_.pop(frame);unlock();if(!read)break;
      if(!telemetry_.receive(active_,frame.topic,frame.payload,frame.size,now))recordError(ErrorCode::MQTT_PAYLOAD);
    }
    if(connected() && static_cast<uint32_t>(now-lastPublish_)>=30000 && client_.queueSize()<4){
      lastPublish_=now;char topic[96],message[160];snprintf(topic,sizeof topic,"%s/state",base_);
      snprintf(message,sizeof message,"{\"version\":\"%s\",\"rssi\":%d,\"heap\":%u}",CHAOSUHR_VERSION,WiFi.RSSI(),static_cast<unsigned>(ESP.getFreeHeap()));
      client_.publish(topic,0,true,message);
    }
  }
 private:
  MqttConfig& config_;Telemetry& telemetry_;MqttConfig active_=defaultMqttConfig();
  DiagnosedMqttClient client_;MqttInbox inbox_;
  char id_[32]={},base_[64]={},availability_[96]={};
  uint32_t lastAttempt_=0,lastPublish_=0,disconnectAt_=0;
  bool pending_=true,disconnectStarted_=false,connectedEvent_=false;uint8_t lastReason_=0;
  bool established_=false,deadlineReported_=false;
  bool disconnectedEvent_=false,packetError_=false,subscribeError_=false,transportEvent_=false;
  MqttTransportCause transportCause_=MqttTransportCause::None;ErrorCode error_=ErrorCode::None;
#if defined(ESP32)
  portMUX_TYPE mutex_=portMUX_INITIALIZER_UNLOCKED;
  void lock(){portENTER_CRITICAL(&mutex_);}void unlock(){portEXIT_CRITICAL(&mutex_);}
#else
  void lock(){}void unlock(){}
#endif
  void publishDiscovery(){
    for(uint8_t i=0;i<2;++i){
      const char* name=i?"Firmware":"WLAN-Signal";const char* key=i?"version":"rssi";
      JsonDocument document;document["name"]=name;document["unique_id"]=String(id_)+"_"+key;
      document["state_topic"]=String(base_)+"/state";document["value_template"]=String("{{ value_json.")+key+" }}";
      document["availability_topic"]=availability_;document["entity_category"]="diagnostic";
      document["device"]["identifiers"][0]=id_;document["device"]["name"]="ChaosUhr";
      document["device"]["sw_version"]=CHAOSUHR_VERSION;document["origin"]["name"]="ChaosUhr";
      document["origin"]["sw_version"]=CHAOSUHR_VERSION;
      if(!i){document["device_class"]="signal_strength";document["unit_of_measurement"]="dBm";document["state_class"]="measurement";}
      char topic[160],payload[768];snprintf(topic,sizeof topic,"%s/sensor/%s_%s/config",active_.discoveryPrefix,id_,key);
      if(measureJson(document)<sizeof payload){serializeJson(document,payload,sizeof payload);client_.publish(topic,0,true,payload);}
    }
  }
};
