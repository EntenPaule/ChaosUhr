#include "public_access.h"
#include <EEPROM.h>
#include <cassert>
#include <cstdio>
#include "upgrade_fixtures.h"

uint32_t fakeMillis=0;
FakeESP ESP;
FakeWiFi WiFi;
FakeEEPROM EEPROM;
bool FakeESP::eraseConfig(){++erases;if(eraseFails)return false;WiFi.savedSsid.clear();WiFi.savedPassword.clear();return true;}

static std::string token(ClockWebServer& server){
  server.dispatch("/api/session",HTTP_GET);assert(server.status==200);
  const auto at=server.body.find("\"token\":\"");assert(at!=std::string::npos);
  return server.body.substr(at+9,32);
}
static void attemptLogin(ClockWebServer& server,const char* password){
  server.dispatch("/api/login",HTTP_GET);assert(server.status==200);
  const auto at=server.body.find("\"token\":\"");assert(at!=std::string::npos);
  server.headers["X-CSRF-Token"]=server.body.substr(at+9,32);
  server.arguments={{"password",password}};server.dispatch("/api/login",HTTP_POST);
  if(server.status==200){const auto cookie=server.responseHeaders.at("Set-Cookie");server.headers["Cookie"]=cookie.substr(0,cookie.find(';'));}
  server.arguments.clear();
}
static void login(ClockWebServer& server,const char* password){attemptLogin(server,password);assert(server.status==200);}
int main(){
  ClockWebServer first;
  PublicAccess access(first);access.begin();access.startInitialPortal();
  assert(!access.ready() && !access.pending() && WiFi.currentMode==WIFI_AP && first.starts==1);
  const auto setupCode=WiFi.apPassword;
  assert(validSetupCode(setupCode.c_str()) && WiFi.apSsid=="ChaosUhr-Setup-123ABC");
  first.socket.address=0x7f000001;
  first.dispatch("/setup",HTTP_GET);assert(first.status==403);
  first.dispatch("/api/session",HTTP_GET);assert(first.status==403);
  first.socket.address=WiFi.softAPIP();
  first.dispatch("/setup",HTTP_GET);assert(first.status==200 && first.body.find("Admin-Passwort")!=std::string::npos);
  const auto firstToken=token(first);
  first.arguments={{"password","fixture-admin-passphrase-42"},{"repeat","fixture-admin-passphrase-42"}};
  first.dispatch("/api/setup",HTTP_POST);assert(first.status==403 && EEPROM.commits==0);
  first.headers["X-CSRF-Token"]=firstToken;
  first.arguments["repeat"]="different";
  first.dispatch("/api/setup",HTTP_POST);assert(first.status==400 && EEPROM.commits==0);
  first.arguments["repeat"]=first.arguments["password"];
  EEPROM.fail=true;first.dispatch("/api/setup",HTTP_POST);assert(first.status==500 && !access.ready());
  EEPROM.fail=false;first.dispatch("/api/setup",HTTP_POST);
  assert(first.status==200 && access.ready() && access.pending());
  access.loop();assert(ESP.restarts==0);
  fakeMillis+=250;access.loop();assert(ESP.restarts==1);

  EEPROM.reboot();ClockWebServer second;
  PublicAccess configured(second);configured.begin();
  assert(configured.ready() && !configured.pending() && !strcmp(configured.setupPassword(),setupCode.c_str()));
  second.socket.address=WiFi.softAPIP();
  second.dispatch("/setup",HTTP_GET);assert(second.status==403);
  second.dispatch("/api/setup",HTTP_POST);assert(second.status==403);
  second.dispatch("/api/session",HTTP_GET);assert(second.status==401);
  assert(second.body.find("AUTH-001")!=std::string::npos&&second.body.find("\"action\"")!=std::string::npos);
  assert(errorRegistry().last()==ErrorCode::AUTH_REQUIRED); // same registry across translation units
  second.user="admin";second.password="fixture-admin-passphrase-42";
  second.dispatch("/api/session",HTTP_GET);assert(second.status==401); // cached Basic cannot log in
  login(second,"fixture-admin-passphrase-42");
  // Simulate updater.setup replacing the header collection, then restore the
  // combined set. Reset/session tokens and the OTA origin guard must coexist.
  const char* otaHeaders[]={"Origin","Host"};second.collectHeaders(otaHeaders,2);
  collectClockHeaders(second);
  assert(second.collectedHeaders.size()==5);
  assert(second.collectedHeaders[0]=="X-CSRF-Token");
  assert(second.collectedHeaders[1]=="Origin" && second.collectedHeaders[2]=="Host");
  assert(second.collectedHeaders[3]=="X-Firmware-SHA256");
  const auto secondToken=token(second);assert(secondToken!=firstToken);
  second.arguments={{"confirm","RESET"}};
  second.headers["X-CSRF-Token"]=secondToken;second.headers["Origin"]="http://foreign.invalid";
  second.dispatch("/api/factory-reset",HTTP_POST);assert(second.status==403 && ESP.erases==0);
  second.headers["Origin"]="null";
  second.dispatch("/api/factory-reset",HTTP_POST);assert(second.status==403 && ESP.erases==0);
  second.headers.erase("Origin");
  second.dispatch("/api/factory-reset",HTTP_GET);assert(second.status==404);
  const auto savedCookie=second.headers["Cookie"];
  second.headers["Cookie"]="ChaosUhrSID=00000000000000000000000000000000";
  second.headers["X-CSRF-Token"]=secondToken;
  second.dispatch("/api/factory-reset",HTTP_POST);assert(second.status==401 && ESP.erases==0);
  second.headers["Cookie"]=savedCookie;second.headers["X-CSRF-Token"]=firstToken;
  second.dispatch("/api/factory-reset",HTTP_POST);assert(second.status==403 && ESP.erases==0);
  second.headers["X-CSRF-Token"]=secondToken;second.arguments["confirm"]="";
  second.dispatch("/api/factory-reset",HTTP_POST);assert(second.status==403 && ESP.erases==0);
  second.arguments["confirm"]="RESET";
  second.dispatch("/api/factory-reset",HTTP_POST);
  assert(second.status==200 && configured.resetting() && !configured.ready());
  second.dispatch("/api/session",HTTP_GET);assert(second.status==503);
  ESP.eraseFails=true;fakeMillis+=250;configured.loop();
  assert(ESP.erases==1 && ESP.restarts==1 && !WiFi.savedPassword.empty());
  // Simulate power loss while reset is pending: reboot must finish erasure,
  // without offering a new administrator account in between.
  EEPROM.reboot();ClockWebServer third;
  PublicAccess interrupted(third);interrupted.begin();interrupted.startInitialPortal();
  unsigned storageErases=0;bool storageFails=true;
  interrupted.setResetStorage([&](){++storageErases;return !storageFails;});
  assert(interrupted.resetting() && third.starts==0);
  third.dispatch("/api/setup",HTTP_POST);assert(third.status==403);
  ESP.eraseFails=false;
#if defined(ESP32)
  WiFi.disconnectSucceeds=false; // Already offline after interrupted reset.
#endif
  fakeMillis+=250;interrupted.loop();
  assert(ESP.erases==2 && ESP.restarts==1 && storageErases==1);
  DeviceCredentials pendingCredentials;EEPROM.get(CREDENTIALS_OFFSET,pendingCredentials);
  // This raw EEPROM adapter omits the independent FS journal. Production
  // recovery of the preserved reset marker is exercised by test_security.cpp.
  assert(interrupted.resetting() && !credentialsReady(pendingCredentials));
  storageFails=false;fakeMillis+=5000;interrupted.loop();
  assert(ESP.erases==3 && ESP.restarts==2 && storageErases==2);
  assert(WiFi.savedSsid.empty() && WiFi.savedPassword.empty());
  for(uint8_t byte:EEPROM.flash)assert(byte==0xff);
  EEPROM.reboot();ClockWebServer fourth;
  PublicAccess fresh(fourth);fresh.begin();fresh.startInitialPortal();
  assert(!fresh.ready() && fresh.provisioning() && fourth.starts==1);
  assert(WiFi.apPassword!=setupCode && validSetupCode(WiFi.apPassword.c_str()));
  fourth.dispatch("/setup",HTTP_GET);assert(fourth.status==200);
  EEPROM=FakeEEPROM();memcpy(EEPROM.flash.data(),UPGRADE_205,128);EEPROM.reboot();
  ClockWebServer fifth;PublicAccess upgraded(fifth);upgraded.begin();
  assert(upgraded.ready() && upgraded.migrationPending());
  login(fifth,"fixture-upgrade-admin-205");token(fifth);
  for(unsigned i=0;i<5;++i){attemptLogin(fifth,"fixture-wrong-passphrase");assert(fifth.status==401);}
  attemptLogin(fifth,"fixture-wrong-passphrase");assert(fifth.status==429);
  fakeMillis+=5001;login(fifth,"fixture-upgrade-admin-205");token(fifth);
  EEPROM.flash[36]=99;EEPROM.reboot();ClockWebServer sixth;PublicAccess unknown(sixth);unknown.begin();unknown.startInitialPortal();
  assert(unknown.locked() && !unknown.provisioning() && sixth.starts==0);
  sixth.dispatch("/api/setup",HTTP_POST);assert(sixth.status==403);
  sixth.dispatch("/api/session",HTTP_GET);assert(sixth.status==403);
  puts("PASS public HTTP/device adapter: AP-only setup, CSRF/auth, password confirmation, failed save, reboot, protected reset, interrupted reset, deleted Wi-Fi/admin, new commissioning code");
}
