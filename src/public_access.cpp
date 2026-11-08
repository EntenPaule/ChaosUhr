#ifdef CHAOSUHR_PUBLIC_BUILD
#include <Arduino.h>
#include <EEPROM.h>
#include "clock_storage.h"
#include "public_access.h"
#include "public_ui.h"
#include "error_response.h"

void PublicAccess::error(int status,ErrorCode code){
  server_.sendHeader("Cache-Control","no-store");
  sendError(server_,status,code);
}
bool PublicAccess::viaSetupAP(){return server_.client().localIP()==WiFi.softAPIP();}
void PublicAccess::clearCookie(){
  server_.sendHeader("Set-Cookie","ChaosUhrSID=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
}
int PublicAccess::sessionSlot(bool report){
  sessions_.expire(millis());
  const String cookie=server_.header("Cookie");uint8_t sid[16]={},digest[32]={};bool present=false;
  ErrorCode code=ErrorCode::AUTH_REQUIRED;int slot=-1;
  if(!readAdminCookie(cookie.c_str(),cookie.length(),sid,present))code=ErrorCode::AUTH_SESSION_COOKIE;
  else if(present){
    if(!AdminSessions::fingerprint(sid,digest))code=ErrorCode::AUTH_CRYPTO;
    else {
      slot=sessions_.find(digest);code=ErrorCode::AUTH_SESSION_INVALID;
      if(slot>=0){
        const auto state=sessions_.get(slot).state;
        if(state==AdminSessions::State::Active){eraseSecret(sid,sizeof sid);eraseSecret(digest,sizeof digest);return slot;}
        if(state==AdminSessions::State::Expired)code=ErrorCode::AUTH_SESSION_EXPIRED;
      }
    }
  }
  eraseSecret(sid,sizeof sid);eraseSecret(digest,sizeof digest);
  if(report){clearCookie();error(code==ErrorCode::AUTH_CRYPTO?503:401,code);}
  return -1;
}
bool PublicAccess::authenticate(){
  if(pending()){error(503,resetting()?ErrorCode::SYS_RESETTING:ErrorCode::SYS_RESTARTING);return false;}
  if(locked()){error(503,ErrorCode::AUTH_CREDENTIALS_CORRUPT);return false;}
  if(!ready()){error(503,ErrorCode::AUTH_NOT_READY);return false;}
  // Ignore cached Basic credentials completely. Only explicit POST login creates
  // a new server-side session; no WWW-Authenticate challenge is sent.
  return sessionSlot(true)>=0;
}
bool PublicAccess::validMutationToken(){
  if(!ready() || pending() || !requestOriginAllowed())return false;
  const int slot=sessionSlot(false);if(slot<0)return false;
  const String submitted=server_.header("X-CSRF-Token");uint8_t decoded[16]={};
  const bool valid=AdminSessions::decode(submitted.c_str(),submitted.length(),decoded) && constantEqual(decoded,sessions_.get(slot).csrf,16);
  eraseSecret(decoded,sizeof decoded);
  if(valid)sessions_.touch(slot,millis()); // Authorized mutations are activity.
  return valid;
}
bool PublicAccess::loginAllowed(){
  if(pending()){error(503,resetting()?ErrorCode::SYS_RESETTING:ErrorCode::SYS_RESTARTING);return false;}
  if(locked()){error(503,ErrorCode::AUTH_CREDENTIALS_CORRUPT);return false;}
  if(!ready()){error(503,ErrorCode::AUTH_NOT_READY);return false;}
  if(!requestOriginAllowed()){error(403,policyError_);return false;}
  const String submitted=server_.header("X-CSRF-Token");
  if(submitted.length()!=32 || !constantEqual(submitted.c_str(),loginToken_,32)){error(403,ErrorCode::AUTH_CSRF);return false;}
  const uint32_t now=millis();
  if(static_cast<uint32_t>(now-failWindow_)>=5000){failedLogins_=0;failWindow_=now;}
  if(failedLogins_>=5){error(429,ErrorCode::AUTH_RATE);return false;}
  return true;
}
void PublicAccess::loginChallengeApi(){
  if(pending()){error(503,resetting()?ErrorCode::SYS_RESETTING:ErrorCode::SYS_RESTARTING);return;}
  if(locked()){error(503,ErrorCode::AUTH_CREDENTIALS_CORRUPT);return;}
  if(!ready()){error(503,ErrorCode::AUTH_NOT_READY);return;}
  if(!requestOriginAllowed()){error(403,policyError_);return;}
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json",String("{\"token\":\"")+loginToken_+"\"}");
}
void PublicAccess::loginApi(){
  if(!loginAllowed())return;
  if(server_.args()!=1 || !server_.hasArg("password")){error(400,ErrorCode::AUTH_LOGIN_FIELDS);return;}
  String password=server_.arg("password");
  const bool valid=validAdminPassword(password.c_str()) && password.length()==strlen(password.c_str()) && verifyAdminPassword(credentials_,password.c_str(),
#if defined(ARDUINO)
    [](){yield();}
#else
    nullptr
#endif
  );
  if(password.length())eraseSecret(&password[0],password.length());
  if(!valid){++failedLogins_;error(401,ErrorCode::AUTH_REJECTED);return;}
  failedLogins_=0;sessions_.expire(millis());
  uint8_t previous[16]={},digest[32]={};bool present=false;
  const String cookie=server_.header("Cookie");int replacement=-1;
  if(readAdminCookie(cookie.c_str(),cookie.length(),previous,present) && present && AdminSessions::fingerprint(previous,digest))replacement=sessions_.find(digest);
  const int slot=sessions_.available(replacement);
  eraseSecret(previous,sizeof previous);eraseSecret(digest,sizeof digest);
  if(slot<0){error(429,ErrorCode::AUTH_SESSION_LIMIT);return;}
  uint8_t sid[16],csrf[16];deviceRandomBytes(sid,sizeof sid);deviceRandomBytes(csrf,sizeof csrf);
  if(!AdminSessions::fingerprint(sid,digest)){eraseSecret(sid,sizeof sid);eraseSecret(csrf,sizeof csrf);error(503,ErrorCode::AUTH_CRYPTO);return;}
  sessions_.start(slot,digest,csrf,millis());
  char text[33];AdminSessions::hex(sid,text);
  server_.sendHeader("Set-Cookie",String("ChaosUhrSID=")+text+"; Path=/; HttpOnly; SameSite=Strict");
  eraseSecret(sid,sizeof sid);eraseSecret(csrf,sizeof csrf);eraseSecret(digest,sizeof digest);eraseSecret(text,sizeof text);
  // The new cookie is not in this request's headers yet.
  char token[33];AdminSessions::hex(sessions_.get(slot).csrf,token);
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json",String("{\"authenticated\":true,\"token\":\"")+token+"\",\"idleTimeoutSeconds\":1800,\"remainingSeconds\":1800}");
  eraseSecret(token,sizeof token);
}
void PublicAccess::logoutApi(){
  if(!authenticate())return;
  if(!validMutationToken()){error(403,ErrorCode::AUTH_CSRF);return;}
  if(server_.args()!=0){error(400,ErrorCode::CFG_UNKNOWN);return;}
  const int slot=sessionSlot(false);if(slot<0){error(401,ErrorCode::AUTH_SESSION_INVALID);return;}
  sessions_.revoke(slot);clearCookie();
  server_.sendHeader("Cache-Control","no-store");server_.send(200,"application/json","{\"ok\":true}");
}
void PublicAccess::activityApi(){
  if(!authenticate())return;
  if(!validMutationToken()){error(403,ErrorCode::AUTH_CSRF);return;}
  if(server_.args()!=0){error(400,ErrorCode::CFG_UNKNOWN);return;}
  sessionApi();
}
bool PublicAccess::requestOriginAllowed(){
  policyError_=ErrorCode::AUTH_ORIGIN;
  const String& host=server_.header("Host");const String& origin=server_.header("Origin");
  if(!originMatchesHost(origin.c_str(),host.c_str()))return false;
  if(!ready()){
#if defined(ARDUINO)
    const String address=WiFi.softAPIP().toString();
#else
    const String address="192.168.4.1";
#endif
    const bool allowed=setupHostAllowed(host.c_str(),address.c_str());
    if(!allowed)policyError_=ErrorCode::HTTP_SETUP_HOST;
    return allowed;
  }
  return true;
}
bool PublicAccess::bodyAllowed(){
  if(server_.method()!=HTTP_POST){error(405,ErrorCode::HTTP_METHOD);return false;}
  if(server_.uri()=="/api/login")return loginAllowed();
  if(server_.uri()=="/api/setup"){
    if(!provisioning()){error(403,ErrorCode::AUTH_SETUP);return false;}
    if(!viaSetupAP()){error(403,ErrorCode::AUTH_SETUP_AP);return false;}
    if(!tokenMatches()){error(403,ErrorCode::AUTH_CSRF);return false;}
    if(!requestOriginAllowed()){error(403,policyError_);return false;}
    return true;
  }
  if(!authenticate())return false;
  if(!requestOriginAllowed()){error(403,policyError_);return false;}
  if(!validMutationToken()){error(403,ErrorCode::AUTH_CSRF);return false;}
  return true;
}
bool PublicAccess::migrate(){
  if(!migrationPending())return false;
  uint8_t salt[16];deviceRandomBytes(salt,sizeof salt);
  const bool ok=migrateCredentials(clockStorage,credentials_,salt,
#if defined(ARDUINO)
    [](){yield();}
#else
    nullptr
#endif
  );
  return ok;
}
bool PublicAccess::tokenMatches(){
  const String& submitted=server_.header("X-CSRF-Token");
  return submitted.length()==32 && constantEqual(submitted.c_str(),token_,32);
}

void PublicAccess::begin(){
  sessions_.reset();action_=Action::None;failedLogins_=0;failWindow_=millis();
  clockStorage.get(CREDENTIALS_OFFSET,credentials_);
  bool erased=true;for(size_t i=0;i<sizeof credentials_;++i)if(reinterpret_cast<const uint8_t*>(&credentials_)[i]!=0xff)erased=false;
  locked_=!erased && !validCredentials(credentials_);
  if(locked_)recordError(ErrorCode::AUTH_CREDENTIALS_CORRUPT);
  configureWifiPersistence();
  WiFi.mode(ready()?WIFI_STA:WIFI_AP); // Enable the radio before using its RNG.
  snprintf(ssid_,sizeof ssid_,"ChaosUhr-Setup-%06X",static_cast<unsigned>(deviceIdentifier()));
  uint8_t randomBytes[16];deviceRandomBytes(randomBytes,sizeof randomBytes);
  AdminSessions::hex(randomBytes,token_);
  deviceRandomBytes(randomBytes,sizeof randomBytes);AdminSessions::hex(randomBytes,loginToken_);eraseSecret(randomBytes,sizeof randomBytes);
  for(size_t i=0;i<12;){
    uint8_t value;deviceRandomBytes(&value,1);
    if(value<250) initialCode_[i++]='0'+value%10; // Unbiased decimal digits.
  }
  if(resetPending(credentials_)){
    action_=Action::Reset;actionStarted_=millis();
  }
  migrate();
  collectClockHeaders(server_);
  server_.on("/api/session",HTTP_GET,[this](){sessionApi();});
  server_.on("/api/login",HTTP_GET,[this](){loginChallengeApi();});
  server_.on("/api/login",HTTP_POST,[this](){loginApi();});
  server_.on("/api/logout",HTTP_POST,[this](){logoutApi();});
  server_.on("/api/session/activity",HTTP_POST,[this](){activityApi();});
  server_.on("/api/setup",HTTP_POST,[this](){provisionApi();});
  server_.on("/api/factory-reset",HTTP_POST,[this](){resetApi();});
  server_.on("/setup",HTTP_GET,[this](){serveSetup();});
  server_.setBodyGuard([this](){return bodyAllowed();});
}
void PublicAccess::startInitialPortal(){
  if(!provisioning()) return;
  WiFi.mode(WIFI_AP);
  if(WiFi.softAP(ssid_,initialCode_)) server_.begin();else recordError(ErrorCode::WIFI_AP_FAILED);
  // The random code is shown on the matrix; never print it in logs.
}
void PublicAccess::serveSetup(){
  if(!provisioning()){error(403,ErrorCode::AUTH_SETUP);return;}
  if(!viaSetupAP()){error(403,ErrorCode::AUTH_SETUP_AP);return;}
  if(!requestOriginAllowed()){error(403,policyError_);return;}
  server_.sendHeader("Cache-Control","no-store");
  server_.send_P(200,"text/html; charset=utf-8",SETUP_HTML);
}
void PublicAccess::sessionApi(){
  if(pending()){error(503,resetting()?ErrorCode::SYS_RESETTING:ErrorCode::SYS_RESTARTING);return;}
  if(ready()){
    if(!authenticate())return;
    const int slot=sessionSlot(false);if(slot<0){error(401,ErrorCode::AUTH_SESSION_INVALID);return;}
    const auto& entry=sessions_.get(slot);
    const uint32_t elapsed=static_cast<uint32_t>(millis()-entry.lastActivity);
    if(elapsed>=AdminSessions::IDLE_MS){sessions_.expire(millis());clearCookie();error(401,ErrorCode::AUTH_SESSION_EXPIRED);return;}
    char token[33];AdminSessions::hex(entry.csrf,token);
    const unsigned long remaining=(AdminSessions::IDLE_MS-elapsed+999)/1000;
    server_.sendHeader("Cache-Control","no-store");
    server_.send(200,"application/json",String("{\"authenticated\":true,\"token\":\"")+token+"\",\"idleTimeoutSeconds\":1800,\"remainingSeconds\":"+String(
#if defined(ARDUINO)
      remaining
#else
      std::to_string(remaining)
#endif
    )+"}");
    eraseSecret(token,sizeof token);return;
  }else {
    if(!provisioning()){error(403,ErrorCode::AUTH_SETUP);return;}
    if(!viaSetupAP()){error(403,ErrorCode::AUTH_SETUP_AP);return;}
    if(!requestOriginAllowed()){error(403,policyError_);return;}
  }
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json",String("{\"token\":\"")+token_+"\"}");
}
void PublicAccess::provisionApi(){
  if(!provisioning()){error(403,ErrorCode::AUTH_SETUP);return;}
  if(!viaSetupAP()){error(403,ErrorCode::AUTH_SETUP_AP);return;}
  if(!requestOriginAllowed()){error(403,policyError_);return;}
  if(!tokenMatches()){error(403,ErrorCode::AUTH_CSRF);return;}
  if(server_.args()!=2 || !server_.hasArg("password") || !server_.hasArg("repeat")){
    error(400,ErrorCode::AUTH_PASSWORD_FIELDS);return;
  }
  const String password=server_.arg("password"),repeat=server_.arg("repeat");
  if(password!=repeat){error(400,ErrorCode::AUTH_PASSWORD_REPEAT);return;}
  if(!validAdminPassword(password.c_str()) || password!=repeat){
    error(400,ErrorCode::AUTH_PASSWORD_FORMAT);return;
  }
  uint8_t salt[16];deviceRandomBytes(salt,sizeof salt);
  if(!provisionCredentials(clockStorage,credentials_,password.c_str(),repeat.c_str(),initialCode_,salt,
#if defined(ARDUINO)
    [](){yield();}
#else
    nullptr
#endif
  )){
    error(500,ErrorCode::AUTH_STORE);return;
  }
  action_=Action::Restart;actionStarted_=millis();
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json","{\"ok\":true,\"restarting\":true}");
}
void PublicAccess::resetApi(){
  if(!authenticate()) return;
  if(server_.method()!=HTTP_POST || !resetConfirmed(true,validMutationToken(),server_.arg("confirm").c_str())){
    error(403,ErrorCode::AUTH_CONFIRM);return;
  }
  if(!markFactoryReset(clockStorage,credentials_)){error(500,ErrorCode::AUTH_RESET_MARK);return;}
  sessions_.reset();action_=Action::Reset;actionStarted_=millis();
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json","{\"ok\":true,\"resetting\":true}");
}
void PublicAccess::loop(){
  sessions_.expire(millis());
  if(!pending() || static_cast<uint32_t>(millis()-actionStarted_)<actionDelay_) return;
  if(action_==Action::Restart){ESP.restart();return;}
  const bool wifiErased=eraseDeviceWifi();
  const auto errorSequence=errorRegistry().sequence();
  bool cleared=false;
  if(wifiErased){
#if defined(ARDUINO)
    cleared=clockStorage.clearForReset();
#else
    for(size_t i=0;i<clockStorage.length();++i)clockStorage.write(i,0xff);
    cleared=clockStorage.commit();
#endif
    if(cleared && (!resetStorage_ || resetStorage_())){ESP.restart();return;}
  }
  if(!wifiErased)recordError(ErrorCode::WIFI_ERASE_FAILED);
  else if(errorRegistry().sequence()==errorSequence)recordError(cleared?ErrorCode::FS_RESET_UNKNOWN:ErrorCode::FS_EEPROM_WRITE);
  // Keep the durable marker. Retry deletion; never reopen an unsecured GUI or
  // claim successful completion when deleting SDK/EEPROM data failed.
  actionStarted_=millis();actionDelay_=5000;
}
#endif
