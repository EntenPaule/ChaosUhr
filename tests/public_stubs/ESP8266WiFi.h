#pragma once
#include "Arduino.h"
static constexpr int WIFI_STA=1,WIFI_AP=2;
struct FakeWiFi {
  int currentMode=0;
  bool persistence=false,autoReconnect=false,disconnectSucceeds=true;
  unsigned disconnects=0;
  std::string savedSsid="fixture-wifi",savedPassword="fixture-wifi-password",apSsid,apPassword;
  bool mode(int value){currentMode=value;return true;}
  void persistent(bool value){persistence=value;}
  String SSID(){return savedSsid;}
  int RSSI(){return -55;}
  void setAutoReconnect(bool value){autoReconnect=value;}
  bool disconnect(bool off,bool erase){assertErase(off,erase);++disconnects;return disconnectSucceeds;}
  void assertErase(bool off,bool erase){
#if defined(ESP32)
    if(off || erase)std::abort();
#else
    if(!off || !erase || !persistence)std::abort();
#endif
  }
  bool softAP(const char* ssid,const char* password){apSsid=ssid;apPassword=password;return true;}
  uint32_t softAPIP(){return 0xc0a80401;}
};
extern FakeWiFi WiFi;
