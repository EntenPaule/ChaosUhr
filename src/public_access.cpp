#ifdef CHAOSUHR_PUBLIC_BUILD
#include <Arduino.h>
#include <EEPROM.h>
#include "public_access.h"
#include "public_ui.h"

void PublicAccess::error(int status,const char* message){
  server_.sendHeader("Cache-Control","no-store");
  server_.send(status,"text/plain; charset=utf-8",message);
}
bool PublicAccess::viaSetupAP(){return server_.client().localIP()==WiFi.softAPIP();}
bool PublicAccess::authenticate(){
  if(!ready()){error(503,"Die Uhr ist noch nicht eingerichtet oder wird zurueckgesetzt.");return false;}
  if(server_.authenticate("admin",credentials_.adminPassword)) return true;
  server_.requestAuthentication();return false;
}
bool PublicAccess::tokenMatches(){
  const String& submitted=server_.header("X-CSRF-Token");
  return submitted.length()==32 && submitted==token_;
}

void PublicAccess::begin(){
  EEPROM.get(CREDENTIALS_OFFSET,credentials_);
  WiFi.persistent(false);
  WiFi.mode(ready()?WIFI_STA:WIFI_AP); // Enable the radio before using its RNG.
  snprintf(ssid_,sizeof ssid_,"ChaosUhr-Setup-%06X",static_cast<unsigned>(deviceIdentifier()));
  uint8_t randomBytes[16];deviceRandomBytes(randomBytes,sizeof randomBytes);
  for(size_t i=0;i<16;++i) snprintf(token_+i*2,3,"%02x",randomBytes[i]);
  for(size_t i=0;i<12;){
    uint8_t value;deviceRandomBytes(&value,1);
    if(value<250) initialCode_[i++]='0'+value%10; // Unbiased decimal digits.
  }
  if(resetPending(credentials_)){
    action_=Action::Reset;actionStarted_=millis();
  }
  collectClockHeaders(server_);
  server_.on("/api/session",HTTP_GET,[this](){sessionApi();});
  server_.on("/api/setup",HTTP_POST,[this](){provisionApi();});
  server_.on("/api/factory-reset",HTTP_POST,[this](){resetApi();});
  server_.on("/setup",HTTP_GET,[this](){serveSetup();});
}
void PublicAccess::startInitialPortal(){
  if(!provisioning()) return;
  WiFi.mode(WIFI_AP);
  if(WiFi.softAP(ssid_,initialCode_)) server_.begin();
  // The random code is shown on the matrix; never print it in logs.
}
void PublicAccess::serveSetup(){
  if(!canProvision(ready(),viaSetupAP(),pending())){
    error(403,"Ersteinrichtung nur im Einrichtungs-WLAN und ohne bestehenden Admin-Zugang moeglich.");return;
  }
  server_.sendHeader("Cache-Control","no-store");
  server_.send_P(200,"text/html; charset=utf-8",SETUP_HTML);
}
void PublicAccess::sessionApi(){
  if(pending()){error(503,"Die Uhr startet neu.");return;}
  if(ready()){if(!authenticate())return;}
  else if(!canProvision(false,viaSetupAP(),pending())){error(403,"Nur im Einrichtungs-WLAN verfuegbar.");return;}
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json",String("{\"token\":\"")+token_+"\"}");
}
void PublicAccess::provisionApi(){
  if(!canProvision(ready(),viaSetupAP(),pending())){error(403,"Ersteinrichtung nicht erlaubt.");return;}
  if(!tokenMatches()){error(403,"Ungueltige Anfrage. Bitte die Seite neu laden.");return;}
  if(server_.args()!=2 || !server_.hasArg("password") || !server_.hasArg("repeat")){
    error(400,"Passwort und Wiederholung erforderlich.");return;
  }
  const String password=server_.arg("password"),repeat=server_.arg("repeat");
  if(!validAdminPassword(password.c_str()) || password!=repeat){
    error(400,"Passwort muss 16 bis 64 druckbare Zeichen ohne Umlaute haben; Wiederholung muss uebereinstimmen.");return;
  }
  if(!provisionCredentials(EEPROM,credentials_,password.c_str(),repeat.c_str(),initialCode_)){
    error(500,"Admin-Zugang konnte nicht gespeichert werden.");return;
  }
  action_=Action::Restart;actionStarted_=millis();
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json","{\"ok\":true,\"restarting\":true}");
}
void PublicAccess::resetApi(){
  if(!authenticate()) return;
  if(!resetConfirmed(true,tokenMatches(),server_.arg("confirm").c_str())){
    error(403,"Werksreset muss ausdruecklich bestaetigt werden. Bitte die Seite neu laden.");return;
  }
  if(!markFactoryReset(EEPROM,credentials_)){error(500,"Werksreset konnte nicht vorbereitet werden.");return;}
  action_=Action::Reset;actionStarted_=millis();
  server_.sendHeader("Cache-Control","no-store");
  server_.send(200,"application/json","{\"ok\":true,\"resetting\":true}");
}
void PublicAccess::loop(){
  if(!pending() || static_cast<uint32_t>(millis()-actionStarted_)<actionDelay_) return;
  if(action_==Action::Restart){ESP.restart();return;}
  struct DeviceNetwork {
    std::function<bool()>& resetStorage;
    bool eraseSavedWifi(){
      return eraseDeviceWifi() && (!resetStorage || resetStorage());
    }
  } network{resetStorage_};
  if(completeFactoryReset(EEPROM,network,credentials_)){ESP.restart();return;}
  // Keep the durable marker. Retry deletion; never reopen an unsecured GUI or
  // claim successful completion when deleting SDK/EEPROM data failed.
  actionStarted_=millis();actionDelay_=5000;
}
#endif
