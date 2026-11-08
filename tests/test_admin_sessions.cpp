#include "public_access.h"
#include "settings.h"
#include "telemetry.h"
#include "memory_animation_fs.h"
#include "upgrade_fixtures.h"
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <cassert>
#include <cstdio>

uint32_t fakeMillis=100;
FakeESP ESP;FakeWiFi WiFi;FakeEEPROM EEPROM;
bool FakeESP::eraseConfig(){++erases;return true;}
static ClockWebServer server;
static PublicAccess publicAccess(server);
static MqttConfig mqttConfig=defaultMqttConfig();static Telemetry telemetry;
static MemoryAnimationFS filesystem;
static MqttConfigStorage<MemoryAnimationFS,AnimationFile> mqttStorage(filesystem);
static struct {bool ready(){return true;}} animations;
static struct {
  unsigned reconfigurations=0;
  const char* base(){return "test-clock";}bool connected(){return false;}uint8_t lastReason(){return 0;}
  ErrorCode errorCode(){return ErrorCode::None;}void reconfigure(){++reconfigurations;}
} clockMqtt;
static bool auth(){return publicAccess.authenticate();}
#include "mqtt_api.h"
static constexpr char VERSION[]="2.0.11";
#include "diagnostics_api.h"
static struct SerialBuffer {
  std::string text;
  void write(uint8_t ch){text+=static_cast<char>(ch);}
  void write(const uint8_t* bytes,size_t size){text.append(reinterpret_cast<const char*>(bytes),size);}
} serial;

static std::string field(const char* name){JsonDocument d;assert(!deserializeJson(d,server.body));return d[name].as<std::string>();}
static unsigned remaining(){JsonDocument d;assert(!deserializeJson(d,server.body));return d["remainingSeconds"].as<unsigned>();}
static std::string session(){server.dispatch("/api/session",HTTP_GET);assert(server.status==200);return field("token");}
static std::string challenge(){server.dispatch("/api/login",HTTP_GET);assert(server.status==200);return field("token");}
static void attempt(const char* password){
  server.headers["X-CSRF-Token"]=challenge();server.arguments={{"password",password}};
  server.dispatch("/api/login",HTTP_POST);server.arguments.clear();
}
static std::string login(){
  attempt("fixture-upgrade-admin-205");assert(server.status==200);
  const auto header=server.responseHeaders.at("Set-Cookie");
  assert(header.find("HttpOnly")!=std::string::npos&&header.find("SameSite=Strict")!=std::string::npos);
  assert(header.find("Secure")==std::string::npos&&header.find("Domain=")==std::string::npos);
  server.headers["Cookie"]=header.substr(0,header.find(';'));
  assert(server.body.find(server.headers["Cookie"].substr(12))==std::string::npos);
  return field("token");
}
static void expect(int status,const char* code){assert(server.status==status&&field("code")==code);}
static void save(const std::string& csrf){
  server.headers["X-CSRF-Token"]=csrf;
  server.arguments={{"enabled","1"},{"host","fixture-broker.invalid"},{"port","1883"},{"user","fixture-user"},
    {"password","fixture-mqtt-password"},{"clearPassword","0"},{"base","test/clock"},{"discovery","1"},
    {"discoveryPrefix","homeassistant"},{"weatherTopic","test/weather"},{"temperatureTopic","test/temperature"},{"staleSeconds","1800"}};
  for(unsigned i=0;i<4;++i)for(const char* suffix:{"Topic","Label","Unit"})server.arguments["sensor"+std::to_string(i)+suffix]="";
  assert(server.args()==24);server.dispatch("/api/mqtt",HTTP_POST);server.arguments.clear();
}
int main(){
  static_assert(sizeof(AdminSessions)<=128,"Session storage must stay bounded");
  memcpy(EEPROM.flash.data(),UPGRADE_205,128);EEPROM.reboot();publicAccess.begin();
  errorRegistry().setSink([](const ErrorEvent& event){printErrorEvent(serial,event);});
  server.on("/api/mqtt",HTTP_POST,mqttSaveApi);server.on("/api/mqtt",HTTP_GET,mqttConfigApi);
  server.on("/api/diagnostics",HTTP_GET,diagnosticsApi);
  // Cached Basic headers do not create, renew or resurrect sessions.
  server.user="admin";server.password="fixture-upgrade-admin-205";
  server.dispatch("/api/session",HTTP_GET);expect(401,"AUTH-001");
  assert(!server.responseHeaders.count("WWW-Authenticate"));
  attempt("fixture-incorrect-password");expect(401,"AUTH-002");
  auto csrf=login();const auto originalCookie=server.headers["Cookie"];
  assert(session()==csrf&&remaining()==1800);
  // Independent same-origin regression, now also through real PublicAccess and
  // the actual MQTT handler with its 24 GUI fields and pre-body guard.
  server.headers["Host"]="ChaosUhr.local:80";server.headers["Origin"]="http://chaosuhr.local";
  save(csrf);assert(server.status==200&&mqttConfig.enabled&&filesystem.formats==0);
  server.dispatch("/api/diagnostics",HTTP_GET);assert(server.status==200);
  for(const auto& secret:{csrf,originalCookie.substr(12),std::string("fixture-upgrade-admin-205"),std::string("fixture-mqtt-password")})assert(server.body.find(secret)==std::string::npos);
  server.dispatch("/api/mqtt",HTTP_GET);assert(server.status==200&&server.body.find("fixture-mqtt-password")==std::string::npos);
  const auto files=filesystem.files;
  server.headers["Origin"]="http://foreign.invalid";save(csrf);expect(403,"AUTH-004");assert(filesystem.files==files);
  server.headers["Origin"]="http://chaosuhr.local";save(std::string(32,'0'));expect(403,"AUTH-003");assert(filesystem.files==files);
  // Pure GET polling, including token reads, must never extend last activity.
  for(unsigned i=1;i<30;++i){fakeMillis+=60000;assert(session()==csrf&&remaining()==1800-i*60);}
  fakeMillis+=59999;assert(session()==csrf&&remaining()==1);
  ++fakeMillis;server.dispatch("/api/session",HTTP_GET);expect(401,"AUTH-020");
  assert(!server.responseHeaders.count("Set-Cookie")); // late background 401 cannot delete a newer login cookie
  save(csrf);expect(401,"AUTH-020");assert(filesystem.files==files);
  // Successful explicit relogin rotates both identifiers. Logout revokes on
  // the server, even if a client keeps the previous cookie and Basic header.
  csrf=login();assert(server.headers["Cookie"]!=originalCookie);
  save(csrf);assert(server.status==200);
  server.headers["X-CSRF-Token"]=csrf;server.dispatch("/api/logout",HTTP_POST);assert(server.status==200);
  assert(server.responseHeaders.at("Set-Cookie").find("Max-Age=0")!=std::string::npos);
  server.dispatch("/api/session",HTTP_GET);expect(401,"AUTH-021");
  server.dispatch("/api/diagnostics",HTTP_GET);expect(401,"AUTH-021");
  save(csrf);expect(401,"AUTH-021");assert(filesystem.files==files);
  const auto oldCsrf=csrf;csrf=login();assert(csrf!=oldCsrf);
  // User activity renews; GET does not. Bad CSRF cannot renew or log out.
  fakeMillis+=1700000;session();assert(remaining()==100);
  server.headers["X-CSRF-Token"]=oldCsrf;server.dispatch("/api/session/activity",HTTP_POST);expect(403,"AUTH-003");
  server.headers["X-CSRF-Token"]=csrf;server.dispatch("/api/session/activity",HTTP_POST);assert(server.status==200&&remaining()==1800);
  const auto cookieA=server.headers["Cookie"],csrfA=csrf;
  server.headers.erase("Cookie");const auto csrfB=login(),cookieB=server.headers["Cookie"];
  server.headers.erase("Cookie");attempt("fixture-upgrade-admin-205");expect(429,"AUTH-023");
  server.headers["Cookie"]=cookieA;save(csrfB);expect(403,"AUTH-003");
  server.headers["X-CSRF-Token"]=csrfA;server.dispatch("/api/logout",HTTP_POST);assert(server.status==200);
  server.headers["Cookie"]=cookieB;assert(session()==csrfB);
  for(const char* invalid:{"ChaosUhrSID=wrong","ChaosUhrSID=00000000000000000000000000000000; ChaosUhrSID=11111111111111111111111111111111"}){
    server.headers["Cookie"]=invalid;server.dispatch("/api/session",HTTP_GET);expect(401,"AUTH-022");
  }
  server.headers["Cookie"]="ChaosUhrSID=00000000000000000000000000000000";save(csrfB);expect(401,"AUTH-021");
  server.headers.erase("Cookie");save(csrfB);expect(401,"AUTH-001");
  // Reboot clears all RAM sessions; a previous login challenge is also invalid.
  const auto oldChallenge=challenge();publicAccess.begin();
  server.headers["Cookie"]=cookieB;server.dispatch("/api/session",HTTP_GET);expect(401,"AUTH-021");
  save(csrfB);expect(401,"AUTH-021");
  server.headers["X-CSRF-Token"]=oldChallenge;server.arguments={{"password","fixture-upgrade-admin-205"}};
  server.dispatch("/api/login",HTTP_POST);expect(403,"AUTH-003");server.arguments.clear();
  server.headers.erase("Cookie");fakeMillis=0xfffffff0;csrf=login();
  fakeMillis+=AdminSessions::IDLE_MS-1;assert(session()==csrf&&remaining()==1);
  ++fakeMillis;publicAccess.loop();server.dispatch("/api/session",HTTP_GET);expect(401,"AUTH-020");
  assert(filesystem.formats==0);
  for(const auto& secret:{csrf,oldCsrf,csrfA,csrfB,cookieA.substr(12),cookieB.substr(12),oldChallenge,std::string("fixture-upgrade-admin-205"),std::string("fixture-mqtt-password")})assert(serial.text.find(secret)==std::string::npos);
  puts("PASS actual admin sessions/MQTT: login/logout/relogin, 30-minute idle without GET renewal, user activity, cross-session CSRF, two-slot limit, malformed/revoked/reboot cookies, rollover and full 24-field MQTT save; no device access");
}
