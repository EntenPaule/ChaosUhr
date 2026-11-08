#ifdef CHAOSUHR_PUBLIC_BUILD
#include <Arduino.h>
#include <EEPROM.h>
#include "clock_storage.h"
#include "public_access.h"
#include "public_ui.h"

void PublicAccess::error(int status,const char* message){
  server_.sendHeader("Cache-Control","no-store");
  server_.send(status,"text/plain; charset=utf-8",message);
}
bool PublicAccess::viaSetupAP(){return server_.client().localIP()==WiFi.softAPIP();}
bool PublicAccess::authenticate(){
  if(!ready() || pending()){error(503,"Die Uhr ist noch nicht eingerichtet oder wird zurueckgesetzt.");return false;}
  const uint32_t now=millis();
  if(static_cast<uint32_t>(now-failWindow_)>=5000){failedLogins_=0;failWindow_=now;}
  if(failedLogins_>=5){error(429,"Zu viele Anmeldeversuche. Bitte fuenf Sekunden warten.");return false;}
  const String& header=server_.header("Authorization");uint8_t fingerprint[32];
  if(header.length()<=102){
    if(!hmac256(cacheKey_,sizeof cacheKey_,header.c_str(),header.length(),fingerprint)){error(503,"Anmeldepruefung nicht verfuegbar.");return false;}
    if(cacheValid_ && static_cast<uint32_t>(now-cachedAt_)<60000 && constantEqual(fingerprint,cachedAuth_,32))return true;
    char password[65]={};
    const bool ok=decodeAdminBasic(header.c_str(),header.length(),password) && verifyAdminPassword(credentials_,password,
#if defined(ARDUINO)
      [](){yield();}
#else
      nullptr
#endif
    );
    eraseSecret(password,sizeof password);
    if(ok){memcpy(cachedAuth_,fingerprint,32);cachedAt_=now;cacheValid_=true;failedLogins_=0;return true;}
  }
  ++failedLogins_;server_.requestAuthentication();return false;
}
bool PublicAccess::requestOriginAllowed(){
  const String& host=server_.header("Host");const String& origin=server_.header("Origin");
  if(!originMatchesHost(origin.c_str(),host.c_str()))return false;
  if(!ready()){
#if defined(ARDUINO)
    const String address=WiFi.softAPIP().toString();
#else
    const String address="192.168.4.1";
#endif
    return setupHostAllowed(host.c_str(),address.c_str());
  }
  return true;
}
bool PublicAccess::bodyAllowed(){
  if(server_.method()!=HTTP_POST){error(405,"POST erforderlich.");return false;}
  if(server_.uri()=="/api/setup"){
    if(provisioning() && viaSetupAP() && tokenMatches() && requestOriginAllowed())return true;
    error(403,"Ersteinrichtung nicht erlaubt.");return false;
  }
  if(!authenticate())return false;
  if(!validMutationToken()){error(403,"Ungueltiges Sitzungstoken oder fremde Webseite.");return false;}
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
  if(ok)cacheValid_=false;
  return ok;
}
bool PublicAccess::tokenMatches(){
  const String& submitted=server_.header("X-CSRF-Token");
  return submitted.length()==32 && constantEqual(submitted.c_str(),token_,32);
}

void PublicAccess::begin(){
  clockStorage.get(CREDENTIALS_OFFSET,credentials_);
  bool erased=true;for(size_t i=0;i<sizeof credentials_;++i)if(reinterpret_cast<const uint8_t*>(&credentials_)[i]!=0xff)erased=false;
  locked_=!erased && !validCredentials(credentials_);
  WiFi.persistent(false);
  WiFi.mode(ready()?WIFI_STA:WIFI_AP); // Enable the radio before using its RNG.
  snprintf(ssid_,sizeof ssid_,"ChaosUhr-Setup-%06X",static_cast<unsigned>(deviceIdentifier()));
  uint8_t randomBytes[16];deviceRandomBytes(randomBytes,sizeof randomBytes);
  deviceRandomBytes(cacheKey_,sizeof cacheKey_);
  for(size_t i=0;i<16;++i) snprintf(token_+i*2,3,"%02x",randomBytes[i]);
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
  server_.on("/api/setup",HTTP_POST,[this](){provisionApi();});
  server_.on("/api/factory-reset",HTTP_POST,[this](){resetApi();});
  server_.on("/setup",HTTP_GET,[this](){serveSetup();});
#if defined(ARDUINO)
  server_.setBodyGuard([this](){return bodyAllowed();});
#endif
}
void PublicAccess::startInitialPortal(){
  if(!provisioning()) return;
  WiFi.mode(WIFI_AP);
  if(WiFi.softAP(ssid_,initialCode_)) server_.begin();
  // The random code is shown on the matrix; never print it in logs.
}
void PublicAccess::serveSetup(){
  if(!provisioning() || !viaSetupAP() || !requestOriginAllowed()){
    error(403,"Ersteinrichtung nur im Einrichtungs-WLAN und ohne bestehenden Admin-Zugang moeglich.");return;
  }
  server_.sendHeader("Cache-Control","no-store");
  server_.send_P(200,"text/html; charset=utf-8",SETUP_HTML);
}
void PublicAccess::sessionApi(){
  if(pending()){error(503,"Die Uhr startet neu.");return;}
  if(ready()){if(!authenticate())return;}
  else if(!provisioning() || !viaSetupAP() || !requestOriginAllowed()){error(403,"Nur im Einrichtungs-WLAN verfuegbar.");return;}
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json",String("{\"token\":\"")+token_+"\"}");
}
void PublicAccess::provisionApi(){
  if(!provisioning() || !viaSetupAP() || !requestOriginAllowed()){error(403,"Ersteinrichtung nicht erlaubt.");return;}
  if(!tokenMatches()){error(403,"Ungueltige Anfrage. Bitte die Seite neu laden.");return;}
  if(server_.args()!=2 || !server_.hasArg("password") || !server_.hasArg("repeat")){
    error(400,"Passwort und Wiederholung erforderlich.");return;
  }
  const String password=server_.arg("password"),repeat=server_.arg("repeat");
  if(!validAdminPassword(password.c_str()) || password!=repeat){
    error(400,"Passwort muss 16 bis 64 druckbare Zeichen ohne Umlaute haben; Wiederholung muss uebereinstimmen.");return;
  }
  uint8_t salt[16];deviceRandomBytes(salt,sizeof salt);
  if(!provisionCredentials(clockStorage,credentials_,password.c_str(),repeat.c_str(),initialCode_,salt,
#if defined(ARDUINO)
    [](){yield();}
#else
    nullptr
#endif
  )){
    error(500,"Admin-Zugang konnte nicht gespeichert werden.");return;
  }
  action_=Action::Restart;actionStarted_=millis();
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json","{\"ok\":true,\"restarting\":true}");
}
void PublicAccess::resetApi(){
  if(!authenticate()) return;
  if(server_.method()!=HTTP_POST || !resetConfirmed(true,validMutationToken(),server_.arg("confirm").c_str())){
    error(403,"Werksreset muss ausdruecklich bestaetigt werden. Bitte die Seite neu laden.");return;
  }
  if(!markFactoryReset(clockStorage,credentials_)){error(500,"Werksreset konnte nicht vorbereitet werden.");return;}
  action_=Action::Reset;actionStarted_=millis();
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json","{\"ok\":true,\"resetting\":true}");
}
void PublicAccess::loop(){
  if(!pending() || static_cast<uint32_t>(millis()-actionStarted_)<actionDelay_) return;
  if(action_==Action::Restart){ESP.restart();return;}
  if(eraseDeviceWifi()){
#if defined(ARDUINO)
    const bool cleared=clockStorage.clearForReset();
#else
    for(size_t i=0;i<clockStorage.length();++i)clockStorage.write(i,0xff);
    const bool cleared=clockStorage.commit();
#endif
    if(cleared && (!resetStorage_ || resetStorage_())){ESP.restart();return;}
  }
  // Keep the durable marker. Retry deletion; never reopen an unsecured GUI or
  // claim successful completion when deleting SDK/EEPROM data failed.
  actionStarted_=millis();actionDelay_=5000;
}
#endif
