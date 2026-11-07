#pragma once

#ifdef CHAOSUHR_PUBLIC_BUILD
#include "device_platform.h"
#include "public_credentials.h"
#include <functional>

class PublicAccess {
 public:
  explicit PublicAccess(ClockWebServer& server):server_(server){}
  void begin();
  void startInitialPortal();
  void serveSetup();
  void loop();
  bool ready() const {return credentialsReady(credentials_);}
  bool provisioning() const {return !ready() && !resetPending(credentials_) && action_==Action::None;}
  bool pending() const {return action_!=Action::None;}
  bool resetting() const {return action_==Action::Reset;}
  bool validMutationToken(){return ready() && !pending() && tokenMatches();}
  void setResetStorage(std::function<bool()> callback){resetStorage_=callback;}
  const char* adminPassword() const {return credentials_.adminPassword;}
  const char* setupPassword() const {return ready()?credentials_.setupPassword:initialCode_;}
  const char* setupSsid() const {return ssid_;}
 private:
  enum class Action {None,Restart,Reset};
  ClockWebServer& server_;
  DeviceCredentials credentials_={};
  char initialCode_[13]={},ssid_[32]={},token_[33]={};
  Action action_=Action::None;
  uint32_t actionStarted_=0,actionDelay_=250;
  std::function<bool()> resetStorage_;
  bool viaSetupAP();
  bool authenticate();
  bool tokenMatches();
  void sessionApi();
  void provisionApi();
  void resetApi();
  void error(int status,const char* message);
};
#endif
