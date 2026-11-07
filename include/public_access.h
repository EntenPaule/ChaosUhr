#pragma once

#ifdef CHAOSUHR_PUBLIC_BUILD
#include "device_platform.h"
#include "public_credentials.h"
#include "request_security.h"
#include <functional>

class PublicAccess {
 public:
  explicit PublicAccess(ClockWebServer& server):server_(server){}
  void begin();
  void startInitialPortal();
  void serveSetup();
  void loop();
  bool ready() const {return credentialsReady(credentials_);}
  bool provisioning() const {return !locked_ && !ready() && !resetPending(credentials_) && action_==Action::None;}
  bool pending() const {return action_!=Action::None;}
  bool resetting() const {return action_==Action::Reset;}
  bool validMutationToken(){return ready() && !pending() && tokenMatches() && requestOriginAllowed();}
  bool authenticate();
  bool requestOriginAllowed();
  bool bodyAllowed();
  bool migrate();
  void restartAfterUpdate(){action_=Action::Restart;actionStarted_=millis();actionDelay_=500;}
  bool migrationPending()const{return ready() && credentials_.version==1;}
  bool locked()const{return locked_;}
  void setResetStorage(std::function<bool()> callback){resetStorage_=callback;}
  const char* setupPassword() const {return ready()?credentials_.setupPassword:initialCode_;}
  const char* setupSsid() const {return ssid_;}
  const char* portalToken()const{return token_;}
 private:
  enum class Action {None,Restart,Reset};
  ClockWebServer& server_;
  DeviceCredentials credentials_={};
  char initialCode_[13]={},ssid_[32]={},token_[33]={};
  uint8_t cacheKey_[32]={},cachedAuth_[32]={};
  uint32_t cachedAt_=0,failWindow_=0;
  uint8_t failedLogins_=0;bool cacheValid_=false,locked_=false;
  Action action_=Action::None;
  uint32_t actionStarted_=0,actionDelay_=250;
  std::function<bool()> resetStorage_;
  bool viaSetupAP();
  bool tokenMatches();
  void sessionApi();
  void provisionApi();
  void resetApi();
  void error(int status,const char* message);
};
#endif
