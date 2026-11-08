#pragma once
#include <functional>
#include <string>
#include <vector>
#include <cstdint>
static constexpr int8_t ERR_TIMEOUT=-3,ERR_CONN=-11,ERR_RST=-14,ERR_ABRT=-13,ERR_CLSD=-15,ERR_RTE=-4;
class AsyncClient {
 public:
  std::function<void(void*,AsyncClient*,int8_t)> error;
  std::function<void(void*,AsyncClient*,uint32_t)> timeout;
  void *errorArg=nullptr,*timeoutArg=nullptr;
  void onError(decltype(error) fn,void* arg){error=fn;errorArg=arg;}
  void onTimeout(decltype(timeout) fn,void* arg){timeout=fn;timeoutArg=arg;}
};
namespace espMqttClientTypes {
enum class DisconnectReason:uint8_t {USER_OK=0,TCP_DISCONNECTED=7};
enum class Error:uint8_t {OUT_OF_MEMORY=1};
enum class SubscribeReturncode:uint8_t {QOS0=0,FAIL=0x80};
struct MessageProperties {};
using OnErrorCallback=std::function<void(uint16_t,Error)>;
}
class espMqttClientAsync {
 public:
  inline static espMqttClientAsync* latest=nullptr;
  bool connected_=false,disconnected_=true;unsigned connections=0,clears=0,publishes=0;
  std::vector<std::string> subscriptions;
  std::function<void(bool)> connectedCallback;
  std::function<void(espMqttClientTypes::DisconnectReason)> disconnectCallback;
  std::function<void(const espMqttClientTypes::MessageProperties&,const char*,const uint8_t*,size_t,size_t,size_t)> messageCallback;
  std::function<void(uint16_t,const espMqttClientTypes::SubscribeReturncode*,size_t)> subscribeCallback;
  espMqttClientAsync(){latest=this;}
  template<typename F>void onConnect(F fn){connectedCallback=fn;}
  template<typename F>void onDisconnect(F fn){disconnectCallback=fn;}
  template<typename F>void onMessage(F fn){messageCallback=fn;}
  template<typename F>void onSubscribe(F fn){subscribeCallback=fn;}
  espMqttClientAsync& setServer(const char*,uint16_t){return *this;}
  espMqttClientAsync& setClientId(const char*){return *this;}
  espMqttClientAsync& setCleanSession(bool){return *this;}
  espMqttClientAsync& setKeepAlive(unsigned){return *this;}
  espMqttClientAsync& setTimeout(unsigned){return *this;}
  void setCredentials(const char*,const char*){}
  void setWill(const char*,int,bool,const char*){}
  void clearQueue(bool){++clears;}
  bool connect(){++connections;disconnected_=false;return true;}
  void disconnect(bool){connected_=false;disconnected_=true;disconnectCallback(espMqttClientTypes::DisconnectReason::USER_OK);}
  bool connected()const{return connected_;}bool disconnected()const{return disconnected_;}
  unsigned queueSize(){return 0;}
  void publish(const char*,int,bool,const char*){++publishes;}
  void subscribe(const char* topic,int){subscriptions.emplace_back(topic);}
  void connectionAccepted(){connected_=true;disconnected_=false;connectedCallback(false);}
  void failed(int8_t transport,uint8_t reason=7){if(transport)_clientAsync.client.error(_clientAsync.client.errorArg,&_clientAsync.client,transport);connected_=false;disconnected_=true;disconnectCallback(static_cast<espMqttClientTypes::DisconnectReason>(reason));}
  void packetFailed(){_onErrorCallback(0,espMqttClientTypes::Error::OUT_OF_MEMORY);}
  void ackTimedOut(){_clientAsync.client.timeout(_clientAsync.client.timeoutArg,&_clientAsync.client,10000);}
  void subscribeFailed(){const auto code=espMqttClientTypes::SubscribeReturncode::FAIL;subscribeCallback(1,&code,1);}
 protected:
  struct {AsyncClient client;}_clientAsync;
  espMqttClientTypes::OnErrorCallback _onErrorCallback;
};
