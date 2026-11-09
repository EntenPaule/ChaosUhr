/* ChaosUhr 2.0 -- ESP8266 / ESP32 / 20x15 WS2812B
 * Independent new implementation inspired by the public wiring specification.
 * Prototype: compile/test before deployment. Source available under GPL-3.0.
 */
#include <Arduino.h>
#include "device_platform.h"
#include "error_response.h"
#include "error_ui.h"
#if defined(ESP32)
#include <HTTPUpdateServer.h>
#include <ESPmDNS.h>
using ClockUpdateServer = HTTPUpdateServer;
#else
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
using ClockUpdateServer = ESP8266HTTPUpdateServer;
#endif
#include <WiFiManager.h>
#include <EEPROM.h>
#include "clock_storage.h"
#include <FastLED.h>
#include <time.h>
#include <stddef.h>
#include "settings.h"
#include "display_logic.h"
#include "legacy_effects.h"
#include "clock_text.h"
#include "firmware_version.h"
#include "auto_playback.h"
#include "scene_pages.h"
#include "clock_mqtt.h"
#include "weather_icons.h"
#include "firmware_upload.h"
#include "ota_ui.h"
#include "portal_ui.h"
#if defined(CHAOSUHR_PUBLIC_BUILD)
#define CHAOSUHR_HAS_UPLOADS
#include <LittleFS.h>
#include "animation_library.h"
#include "legacy_animation.h"
#include "animation_examples.h"
#endif
#ifdef CHAOSUHR_PUBLIC_BUILD
#include "public_access.h"
static_assert(DISPLAY_OPTIONS_OFFSET==CREDENTIALS_OFFSET+sizeof(DeviceCredentials),"Keep new options after credentials");
#else
#include "secrets.local.h"
#endif

#if defined(ESP32)
static constexpr bool TARGET_ESP32=true;
#else
static constexpr bool TARGET_ESP32=false;
#endif
static constexpr uint8_t W=MATRIX_WIDTH, H=MATRIX_HEIGHT;
static constexpr uint16_t LED_COUNT=W*H;
static constexpr char HOSTNAME[]="chaosuhr";
static constexpr char OTA_USER[]="admin";
#ifdef CHAOSUHR_PUBLIC_BUILD
static constexpr char VERSION[]=CHAOSUHR_VERSION;
#else
static constexpr char VERSION[]="2.0.4-dev";
// Generated once by the pre-build script; never printed or checked into Git.
static constexpr char OTA_PASS[]=CHAOSUHR_OTA_PASSWORD;
static constexpr char SETUP_PASS[]=CHAOSUHR_SETUP_PASSWORD;
static_assert(sizeof(OTA_PASS)>16, "Select a long OTA password");
static_assert(sizeof(SETUP_PASS)>8 && sizeof(SETUP_PASS)<=64, "Setup password must have 8..63 characters");
constexpr bool sameText(const char* a, const char* b) {
  return *a == *b && (*a == '\0' || sameText(a+1,b+1));
}
static_assert(!sameText(OTA_PASS,"CHANGE_THIS_PASSWORD_BEFORE_BUILD"), "Replace the OTA placeholder");
#endif

CRGB leds[LED_COUNT];
ClockWebServer server(80);
#if !defined(CHAOSUHR_PUBLIC_BUILD)
ClockUpdateServer updater;
#endif
class ClockWiFiManager:public WiFiManager {
 public:void saveClockWifi(){handleWifiSave();}
  void requestPortalExit(){abort=true;}
};
ClockWiFiManager wifiManager;
#ifdef CHAOSUHR_PUBLIC_BUILD
PublicAccess publicAccess(server);
#endif
#ifdef CHAOSUHR_HAS_UPLOADS
AnimationLibrary<decltype(LittleFS),fs::File> animations(LittleFS,[](){yield();});
#endif
static bool webStarted=false;
static uint32_t wifiAttemptStarted=0;
static constexpr uint32_t WIFI_RETRY_MS=30000;

static Settings cfg;
static DisplayOptions displayOptions=defaultDisplayOptions(TARGET_ESP32);
static bool settingsPersisted=false,optionsPersisted=false;
static MqttConfig mqttConfig=defaultMqttConfig();
static Telemetry telemetry;
static ClockMqtt clockMqtt(mqttConfig,telemetry);
static MqttConfigStorage<decltype(LittleFS),fs::File> mqttStorage(LittleFS);
static IpAnnouncement ipAnnouncement;
static uint8_t currentSensor=SENSOR_SLOTS-1;
static void loadSettings(){
  clockStorage.begin(128); clockStorage.get(0,cfg);
  settingsPersisted=normalizeLoadedSettings(cfg);
  clockStorage.get(DISPLAY_OPTIONS_OFFSET,displayOptions);
  optionsPersisted=normalizeDisplayOptions(displayOptions,TARGET_ESP32);
  mqttStorage.load(mqttConfig);
}
template<uint8_t Pin>static void addLedController(){FastLED.addLeds<WS2812B,Pin,GRB>(leds,LED_COUNT);}
static void setupLedController(){
  switch(displayOptions.ledPin){
#if defined(ESP32)
    case 13:addLedController<13>();break;case 14:addLedController<14>();break;
    case 18:addLedController<18>();break;case 19:addLedController<19>();break;
    case 21:addLedController<21>();break;case 22:addLedController<22>();break;
    case 23:addLedController<23>();break;case 25:addLedController<25>();break;
    case 26:addLedController<26>();break;case 27:addLedController<27>();break;
    case 33:addLedController<33>();break;default:addLedController<32>();break;
#else
    case 4:addLedController<4>();break;case 5:addLedController<5>();break;
    case 13:addLedController<13>();break;case 14:addLedController<14>();break;
    default:addLedController<12>();break;
#endif
  }
}
static uint16_t indexXY(int x,int y) {
  return matrixIndex(x,y,cfg);
}
static void pixel(int x,int y,const CRGB& c){ if(x>=0 && x<W && y>=0 && y<H) leds[indexXY(x,y)]=c; }
static void clear(){ fill_solid(leds,LED_COUNT,CRGB::Black); }
static void fade(uint8_t amount){ fadeToBlackBy(leds,LED_COUNT,amount); }
static uint32_t tick=0, lastFrame=0, animationStarted=0;
static SceneState scene;
static AutoPlayback autoPlayback;
static uint8_t heat[W][H] = {};
static uint8_t rainDrops[W] = {};
static uint8_t snakeX=0, snakeY=6;
static uint8_t drops[W]={};

static void drawGlyph(char character,int x,int y,const CRGB& color){
  for(uint8_t row=0;row<FONT_HEIGHT;row++){
    const uint8_t bits=fontRow(character,row);
    for(uint8_t col=0;col<FONT_WIDTH;col++)
      if(bits&(1<<(FONT_WIDTH-1-col))) pixel(x+col,y+row,color);
  }
}
static void drawDigit(uint8_t digit,int x,int y,const CRGB& color){
  drawGlyph('0'+digit%10,x,y,color);
}
static void drawText(const char* text,int x,int y,const CRGB& color){
  for(;*text && x<W;text++,x+=FONT_ADVANCE)
    if(x+FONT_WIDTH>0) drawGlyph(*text,x,y,color);
}
static void drawClock(){
  clear();
  time_t now=time(nullptr); tm local;
  bool valid=now>1700000000 && localtime_r(&now,&local);
  int hh=valid?local.tm_hour:0, mm=valid?local.tm_min:0;
  CRGB color=valid?CRGB(0,180,240):CRGB(190,70,0);
  const int x=2,y=CLOCK_TIME_Y;
  drawDigit(hh/10,x,y,color); drawDigit(hh%10,x+4,y,color);
  if((millis()/500)%2){ pixel(x+8,y+1,color); pixel(x+8,y+3,color); }
  drawDigit(mm/10,x+10,y,color); drawDigit(mm%10,x+14,y,color);
  char date[CLOCK_DATE_CAPACITY];
  if(!valid || !formatGermanDate(local,date,sizeof date,displayOptions.weekdayStyle))
    snprintf(date,sizeof date,"Warte auf Zeit");
  drawText(date,dateScrollX(millis()-scene.startedAt,strlen(date)),CLOCK_DATE_Y,
           valid?CRGB(255,140,35):color);
  // A small second-progress bar, separate from digits.
  if(valid && (displayOptions.flags&DISPLAY_SECONDS)) for(int i=0;i<(local.tm_sec*W/60);i++) pixel(i,H-1,CRGB(0,0,32));
}
static void drawEffect(uint8_t e){
  if(renderLegacyEffect(e,millis()-animationStarted,cfg.speed,leds,cfg)) return;
  switch(e){
    case 1: { // Matrix rain
      fade(90);
      for(int x=0;x<W;x++){
        if(rainDrops[x]>=H){ if(random8()<28) rainDrops[x]=0; }
        else { uint8_t y=rainDrops[x]++; pixel(x,y,CRGB(130,255,135));
          if(y>0) pixel(x,y-1,CRGB(0,130,25)); }
      } break;
    }
    case 2: { // Rainbow waves
      for(int y=0;y<H;y++) for(int x=0;x<W;x++) pixel(x,y,CHSV(x*9+y*13+tick*2,255,200));
      break;
    }
    case 3: { // Fire, 2D heatfield
      for(int x=0;x<W;x++){
        for(int y=0;y<H;y++) heat[x][y]=qsub8(heat[x][y],random8(0,42));
        for(int y=0;y<H-1;y++) heat[x][y]=(heat[x][y+1]+heat[x][min(y+2,(int)H-1)])/2;
        if(random8()<180) heat[x][H-1]=qadd8(heat[x][H-1],random8(160,255));
      }
      for(int y=0;y<H;y++) for(int x=0;x<W;x++) pixel(x,y,HeatColor(heat[x][y]));
      break;
    }
    case 4: { // Stars
      fade(18);
      for(int i=0;i<2;i++) if(random8()<115) pixel(random8(W),random8(H),CHSV(random8(145,205),60,random8(100,255)));
      break;
    }
    case 5: { // Confetti
      fade(35);
      for(int i=0;i<3;i++) pixel(random8(W),random8(H),CHSV(random8(),220,255));
      break;
    }
    case 6: { // Plasma
      for(int y=0;y<H;y++) for(int x=0;x<W;x++){
        uint8_t v=sin8(x*19+tick*3)/3+sin8(y*23+tick*2)/3+sin8((x+y)*12+tick)/3;
        pixel(x,y,CHSV(v+tick,220,205));
      } break;
    }
    case 7: { // Blue rain
      fade(70);
      for(int y=H-1;y>=1;y--) for(int x=0;x<W;x++) leds[indexXY(x,y)]=leds[indexXY(x,y-1)];
      for(int x=0;x<W;x++) pixel(x,0,random8()<90?CRGB(70,130,255):CRGB::Black);
      break;
    }
    case 8: { // Scanner
      fade(45);
      int x=(tick%(2*W-2)); if(x>=W) x=2*W-2-x;
      for(int y=0;y<H;y++) pixel(x,y,CHSV(tick,255,255));
      break;
    }
    case 9: { // Sparkles
      fade(90);
      for(int i=0;i<5;i++) pixel(random8(W),random8(H),CRGB::White);
      break;
    }
    case 10: { // Color rings
      for(int y=0;y<H;y++) for(int x=0;x<W;x++){
        int dx=x-10, dy=y-7; uint8_t v=(uint8_t)(sqrtf(dx*dx+dy*dy)*23);
        pixel(x,y,CHSV(v*3-tick*3,255,sin8(v*4-tick*7)));
      } break;
    }
    case 11: { // Snake trail (procedural)
      fade(45);
      snakeX=(tick/2)%W;
      snakeY=7+(int)(sin8(tick*7)/32)-4;
      pixel(snakeX,snakeY,CRGB(70,255,70));
      break;
    }
    case 12: { // Meteor shower
      fade(50);
      for(int i=0;i<3;i++) if(random8()<100) pixel(random8(W),random8(H),CHSV(150+random8(55),90,255));
      break;
    }
    case 13: { // Aurora / flowing gradients
      for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
        uint8_t v=sin8(x*14+tick*3+y*8);
        pixel(x,y,CHSV(75+(v>>2),210,scale8(v,210)));
      } break;
    }
    case 14: { // Pixel checkerboard pulse
      for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
        bool checker=(((x+tick/5)/2+(y+tick/9)/2)&1);
        pixel(x,y,checker?CRGB(CHSV(tick*2+x*7,235,180)):CRGB(CRGB::Black));
      } break;
    }
    case 15: { // Color wipe / scrolling bars
      for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
        pixel(x,y,CHSV((x*14+y*6+tick*4),255,((x+y+tick/3)%9<5)?180:20));
      } break;
    }
    case 16: { // Floating bubbles
      fade(38);
      for(int x=0;x<W;x++) {
        if(drops[x]>=H) { if(random8()<28) drops[x]=0; }
        else { pixel(x,H-1-drops[x],CHSV(125+x*5,140,255)); ++drops[x]; }
      } break;
    }
    default: drawClock();
  }
}
static uint8_t activeEffect(){
  const uint8_t selected=cfg.mode==1 || (cfg.mode==0 && scene.clock)?0:(cfg.mode==2?cfg.effect:scene.effect);
#ifdef CHAOSUHR_HAS_UPLOADS
  if(selected>EFFECT_COUNT && !animations.playable(selected))return 0;
#else
  if(selected>EFFECT_COUNT)return 0;
#endif
  return selected;
}
static uint8_t availableScenePages(){
  uint8_t mask=selectedScenePages(displayOptions);const uint32_t now=millis();
  if(!mqttConfig.enabled || !telemetry.weatherAvailable(now,mqttConfig.staleSeconds))mask&=~(1<<PAGE_WEATHER);
  if(!mqttConfig.enabled || !telemetry.sensorsAvailable(now,mqttConfig.staleSeconds))mask&=~(1<<PAGE_SENSOR);
  return mask?mask:1;
}
static int dataTextX(const char* text){
  const int width=strlen(text)*FONT_ADVANCE-1;
  return width<=W?(W-width)/2:W-static_cast<int>(((millis()-scene.startedAt)/DATE_SCROLL_MS)%(width+W+4));
}
static void drawWeather(){
  clear();const uint32_t now=millis();
  if(telemetry.condition.fresh(now,mqttConfig.staleSeconds))
    weatherIcon(telemetry.condition.value,now,[](int x,int y,uint32_t rgb){pixel(x,y,CRGB(rgb));});
  else drawText("Wetter",dataTextX("Wetter"),2,CRGB(100,170,255));
  if(telemetry.temperature.fresh(now,mqttConfig.staleSeconds)){
    char value[20];snprintf(value,sizeof value,"%.1fC",telemetry.temperature.value);
    drawText(value,dataTextX(value),10,CRGB(255,170,50));
  }
}
static void drawSensor(){
  clear();const auto& reading=telemetry.sensors[currentSensor];
  if(!reading.fresh(millis(),mqttConfig.staleSeconds)){drawClock();return;}
  char label[32],unit[16],value[48];displayAscii(mqttConfig.sensors[currentSensor].label,label,sizeof label);
  displayAscii(mqttConfig.sensors[currentSensor].unit,unit,sizeof unit);
  if(!label[0])snprintf(label,sizeof label,"Sensor %u",currentSensor+1);
  snprintf(value,sizeof value,"%s%s",reading.text,unit);
  drawText(label,dataTextX(label),1,CRGB(100,180,255));drawText(value,dataTextX(value),9,CRGB(255,180,60));
}
static void render(){
#ifdef CHAOSUHR_PUBLIC_BUILD
  if(publicAccess.pending() || !publicAccess.ready() || wifiManager.getConfigPortalActive()){
    clear();FastLED.setBrightness(40);
    const char* label=publicAccess.locked()?"LOCK":publicAccess.pending()?(publicAccess.resetting()?"RESET":"START"):"SETUP";
    drawText(label,0,CLOCK_TIME_Y,CRGB(0,180,240));
    if(!publicAccess.pending() && !publicAccess.locked()) drawText(publicAccess.setupPassword(),dateScrollX(millis(),12),CLOCK_DATE_Y,CRGB(255,140,35));
    FastLED.show();return;
  }
#endif
  time_t now=time(nullptr); tm loc;
  bool night=now>1700000000 && localtime_r(&now,&loc) && isNightHour(loc.tm_hour,cfg);
  uint8_t pct=night?cfg.nightBrightness:cfg.brightness;
  FastLED.setBrightness((uint8_t)map(pct,1,100,2,120)); // safety cap: 120/255
  const uint8_t effect=activeEffect();
  if(ipAnnouncement.active(millis())){clear();drawText(ipAnnouncement.text(),ipAnnouncement.x(millis()),5,CRGB(120,210,255));}
  else if(cfg.mode==0 && scene.page==PAGE_WEATHER)drawWeather();
  else if(cfg.mode==0 && scene.page==PAGE_SENSOR)drawSensor();
  else if(!effect) drawClock();
  else {
#ifdef CHAOSUHR_HAS_UPLOADS
    if(effect>EFFECT_COUNT){
      if(!animations.render(effect,legacyElapsed(millis()-animationStarted,cfg.speed),[](uint16_t i,uint32_t color){
        leds[indexXY(i%MATRIX_WIDTH,i/MATRIX_WIDTH)]=CRGB(color);
      }))drawClock();
    } else
#endif
    drawEffect(effect);
  }
  FastLED.show();
}
static void resetAnimation(){
  tick=0; animationStarted=millis(); clear();
#ifdef CHAOSUHR_HAS_UPLOADS
  animations.resetPlayback();
#endif
  memset(heat,0,sizeof heat);
  memset(rainDrops,H,sizeof rainDrops); memset(drops,H,sizeof drops);
}
static void nextScene(){
  uint8_t availableUploads=0;
#ifdef CHAOSUHR_HAS_UPLOADS
  availableUploads=animations.available();
#endif
  scene.page=nextScenePage(scene.page,availableScenePages());scene.clock=scene.page!=PAGE_ANIMATION;
  if(!scene.clock)scene.previousEffect=scene.effect=autoPlayback.next(cfg,[](uint8_t count){return random8(count);},availableUploads);
  if(scene.page==PAGE_SENSOR){
    for(uint8_t step=1;step<=SENSOR_SLOTS;++step){const uint8_t next=(currentSensor+step)%SENSOR_SLOTS;
      if(telemetry.sensors[next].fresh(millis(),mqttConfig.staleSeconds)){currentSensor=next;break;}}
  }
  scene.startedAt=millis();
  resetAnimation();
}

const char INDEX_HTML[] PROGMEM=R"HTML(
<!doctype html><html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><link rel="icon" href="data:,"><title>ChaosUhr 2.0</title>
<style>
:root{color-scheme:dark;font-family:system-ui,Arial}body{background:#0b1020;color:#e9f0ff;max-width:650px;margin:25px auto;padding:0 16px}
article{background:#171f35;border:1px solid #34415f;border-radius:18px;padding:22px;margin:18px 0}h1{font-size:28px;margin:8px 0}h2{font-size:18px}
label{display:block;margin:13px 0 5px}select,input,button{font:inherit}input[type=range],select{width:100%}select{background:#0c1426;color:white;border:1px solid #445575;border-radius:8px;padding:10px}
button,a.btn{background:#53b9ff;color:#081221;border:0;border-radius:10px;padding:12px 16px;cursor:pointer;display:inline-block;text-decoration:none;font-weight:700}
small{color:#abb9d0}output{float:right;color:#8bd5ff}.flex{display:flex;gap:10px;flex-wrap:wrap}.status{color:#92e4ae}
.auto-list{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:4px;max-height:330px;overflow:auto;margin:12px 0}.auto-list label{display:flex;align-items:center;gap:8px;margin:0;padding:7px}.auto-list input{flex-shrink:0}summary{cursor:pointer;margin:12px 0}.auto-actions button{padding:8px 12px;font-size:14px}
</style></head><body><header style="display:flex;justify-content:flex-end;gap:10px;align-items:center;flex-wrap:wrap" aria-label="Admin-Anmeldung"><span id="adminState" role="status">Nicht angemeldet</span><button id="adminLogout" type="button" hidden>Abmelden</button></header>
<section id="loginPanel"><h2>Admin-Anmeldung</h2><form id="adminLogin"><label for="adminPassword">Admin-Passwort</label><input id="adminPassword" name="password" type="password" autocomplete="current-password" minlength="16" maxlength="64" required><button id="adminLoginButton" type="submit">Anmelden</button></form><p>Nach 30 Minuten ohne bestätigte Aktivität wirst du abgemeldet. Lokales HTTP bitte nur im vertrauenswürdigen Netz verwenden.</p></section><p id="adminMessage" role="status" style="white-space:pre-line"></p><main id="protectedContent" hidden><h1>🌈 ChaosUhr 2.0</h1><small>20×15 LEDs · lokale Steuerung</small>
<article><div id="status">Lade Status …</div><p id="securityStatus" role="status"></p></article>
<article><h2>Anzeige & Animationen</h2>
<label>Modus</label><select id="mode"><option value="0">Automatik: gewählte Anzeigen</option><option value="1">Nur Uhrzeit</option><option value="2">Gewählte Animation</option></select>
<label>Einzelanimation (Modus „Gewählte Animation“)</label><select id="effect"><option value="">Lade Animationen …</option></select>
<label>Helligkeit <output id="bVal"></output></label><input type="range" id="brightness" min="1" max="100" oninput="bVal.value=this.value+' %'">
<label>Tempo <output id="sVal"></output></label><input type="range" id="speed" min="1" max="100" oninput="sVal.value=this.value+' %'">
<label>Dauer je Szene <output id="dVal"></output></label><input type="range" id="duration" min="5" max="120" step="5" oninput="dVal.value=this.value+' s'">
<h2>Automatik</h2>
<p>Welche Anzeigen sollen abwechselnd laufen?</p>
<div class="flex"><label><input id="showClock" type="checkbox" checked> Uhrzeit</label><label><input id="showAnimations" type="checkbox" checked> Animationen</label><label><input id="showWeather" type="checkbox"> Wetter</label><label><input id="showSensors" type="checkbox"> Sensorwerte</label></div>
<p><small>Die Automatik läuft in dieser Reihenfolge: Uhrzeit → Wetter → ein Sensorwert → eine Animation → von vorn. Abgewählte Anzeigen werden übersprungen. Jede verfügbare Szene läuft für die eingestellte „Dauer je Szene“; fällt ihre Datenverfügbarkeit weg, wird vorzeitig weitergeschaltet. Beim nächsten Sensordurchlauf folgt der nächste aktuelle Sensor, beim nächsten Animationsdurchlauf die nächste gewählte Animation.</small></p>
<p><small>Wetter und Sensorwerte erscheinen nur bei aktiviertem MQTT und aktuellen Daten. Fehlende oder veraltete Daten werden übersprungen; bleibt keine Anzeige verfügbar, erscheint die Uhr. Mit nur Uhr und Animationen ergibt sich Uhr → Animation → Uhr. Die Animationsauswahl läuft der Reihe nach oder in zufälligen Runden, jeweils jede einmal.</small></p>
<p><small>Der Nachtmodus dimmt alle Anzeigen auf die Nachthelligkeit. Die Automatik und Wetteranzeige laufen dabei weiter.</small></p>
<p><strong>Dein aktueller Anzeigeablauf</strong><br><span id="displayFlow" role="status"></span><br><small>Vorschau deiner Auswahl; Änderungen mit „Speichern“ übernehmen. Wetter/Sensoren werden nur mit aktuellen MQTT-Daten eingeblendet. Dies ist keine Live-Anzeige der gerade laufenden Szene.</small></p>
<label for="autoOrder">Reihenfolge</label><select id="autoOrder"><option value="0">Zufällig (jede einmal pro Runde)</option><option value="1">Der Reihe nach</option></select>
<details open><summary>Animationen für die Automatik auswählen</summary>
<div class="flex auto-actions"><button type="button" onclick="selectAutomatic(true)">Alle auswählen</button><button type="button" onclick="selectAutomatic(false)">Auswahl leeren</button></div>
<div id="autoEffects" class="auto-list"></div><small id="autoCount" role="status"></small>
<p><small>Bei aktivierter Anzeigeart Animationen mindestens eine Animation auswählen. Für eine dauerhafte Uhranzeige den Modus „Nur Uhrzeit“ verwenden.</small></p></details>
<h2>Uhranzeige</h2>
<label><input id="secondsBar" type="checkbox" checked> Sekundenleiste unten anzeigen</label>
<p><small>Bei sehr geringer Helligkeit kann die Sekundenleiste kaum oder nicht sichtbar sein. Dies gilt auch im Nachtmodus.</small></p>
<label for="weekdayStyle">Wochentag</label><select id="weekdayStyle"><option value="0">Ausgeschrieben: Montag, Dienstag …</option><option value="1">Kurzform: mo, di, mi, do, fr, sa, so</option></select>
<h2>Matrixverdrahtung</h2>
<label for="ledPin">LED-Datenpin</label><select id="ledPin"></select>
<p><small>Passend zur tatsächlichen Verkabelung wählen. Eine Pin-Änderung startet die Uhr nach dem Speichern neu.</small></p>
<p><small>Vorsicht: Diese Einstellungen nur aendern, wenn die Anzeige gespiegelt oder verdreht erscheint.</small></p>
<label>Start-Ecke</label><select id="corner"><option value="0">Oben links</option><option value="1">Oben rechts (Standard)</option><option value="2">Unten links</option><option value="3">Unten rechts</option></select>
<label>Verlauf</label><select id="vertical"><option value="0">Zeilenweise (Standard)</option><option value="1">Spaltenweise</option></select>
<label>LED-Muster</label><select id="zigzag"><option value="1">Zickzack (Standard)</option><option value="0">Gleichgerichtet</option></select>
<h2>Nachtmodus</h2><label><input id="nightOn" type="checkbox"> Aktiv</label>
<div class="flex"><label>Beginn (Stunde)<br><input id="nightFrom" type="number" min="0" max="23" style="width:75px"></label><label>Ende (Stunde)<br><input id="nightUntil" type="number" min="0" max="23" style="width:75px"></label></div>
<label>Nachthelligkeit <output id="nVal"></output></label><input type="range" id="nightBrightness" min="1" max="100" oninput="nVal.value=this.value+' %'">
<button id="saveButton" onclick="save()" disabled>Einstellungen speichern</button> <span id="msg"></span></article>
<article id="uploadPanel" hidden><h2>Eigene Animationen / Speicher</h2>
<p>Bis zu acht <code>.chaos</code>-Dateien mit je 24 KiB, 20×15 Pixeln, bis zu 16 Farben und 64 Bildern. Nach dem Upload oben für die Automatik oder als Einzelanimation auswählen.</p>
<p id="animationStorage"></p>
<button id="initializeAnimations" type="button" onclick="initializeAnimations()" hidden>Datenspeicher einrichten</button>
<label for="animationFile">Animationsdatei</label><input id="animationFile" type="file" accept=".chaos">
<button id="uploadAnimation" type="button" onclick="uploadAnimation()">Animation hochladen</button>
<p><a class="btn" href="/animations/example.chaos">Herz-Beispiel herunterladen</a> <a class="btn" href="/animations/prompt" target="_blank" rel="noopener">ChatGPT-Vorlage öffnen</a></p>
<div id="uploadedAnimations"></div><p id="uploadMessage" role="status"></p>
<p><small>Uploads können heruntergeladen, geteilt und gelöscht werden. Bei den fest eingebauten Effekten entfernt ein fehlendes Häkchen die Animation aus der Automatik.</small></p></article>
<article id="mqttPanel" hidden><h2>MQTT, Wetter und Sensoren</h2>
<p id="mqttStatus" role="status"></p>
<label><input id="mqEnabled" type="checkbox"> MQTT aktivieren</label>
<label>Broker-Hostname oder IP</label><input id="mqHost" type="text" maxlength="95" placeholder="MQTT-Broker im lokalen Netz">
<label>Port</label><input id="mqPort" type="number" min="1" max="65535" value="1883">
<label>Benutzername</label><input id="mqUser" type="text" maxlength="64" autocomplete="off">
<label>MQTT-Passwort</label><input id="mqPassword" type="password" maxlength="64" autocomplete="new-password" placeholder="Leer lassen: bisheriges Passwort behalten">
<label><input id="mqClearPassword" type="checkbox"> Gespeichertes MQTT-Passwort löschen</label>
<label>Topic-Basis der Uhr (optional)</label><input id="mqBase" type="text" maxlength="63" placeholder="Leer: automatisch je Gerät">
<label><input id="mqDiscovery" type="checkbox" checked> Home-Assistant-Erkennung der Gerätediagnose</label>
<label>Discovery-Präfix</label><input id="mqDiscoveryPrefix" type="text" maxlength="63" value="homeassistant">
<label>Wetter-Topic</label><input id="mqWeatherTopic" type="text" maxlength="95">
<p><small>Wetterzustand als Text, z. B. sunny, cloudy oder rainy. Alternativ JSON mit condition und temperature. HA oder ioBroker liefert die Daten.</small></p>
<label>Temperatur-Topic (optional bei gemeinsamem Wetter-JSON)</label><input id="mqTemperatureTopic" type="text" maxlength="95">
<label>Daten nach so vielen Sekunden als veraltet behandeln</label><input id="mqStaleSeconds" type="number" min="60" max="86400" value="1800">
<div id="mqttSensors"></div>
<p><small>Sensoren akzeptieren Zahlen, kurze Texte oder JSON mit value, val oder state. Wetter/Sensoren anschließend oben als Anzeigeart auswählen. Der lokale MQTT-Zugang verwendet kein TLS.</small></p>
<button type="button" onclick="saveMqtt()">MQTT speichern</button> <button type="button" onclick="loadMqtt()">Status aktualisieren</button>
<p id="mqttMessage" role="status"></p></article>
<article><h2>Diagnose</h2><p>Letzter Fehler und begrenzter RAM-Verlauf. Keine Zugangsdaten, Tokens, Topics oder Messwerte enthalten.</p><button type="button" onclick="loadDiagnostics()">Supportdiagnose laden</button><p id="diagnosticMessage" role="status"></p><textarea id="supportDiagnostics" rows="12" readonly style="width:100%" aria-label="Bereinigte Supportdiagnose"></textarea></article>
<article><h2>Einstellungen sichern</h2><p>Speichere deine Uhreinstellungen vor einem Update. Zugangsdaten, MQTT-Konfiguration, LED-Datenpin und Animationsdateien werden nicht exportiert.</p>
<button type="button" onclick="exportSettings()">Sicherung herunterladen</button>
<label for="settingsFile">Sicherungsdatei wiederherstellen</label><input id="settingsFile" type="file" accept=".json" onchange="previewSettingsImport()">
<p id="backupMessage" role="status"></p><pre id="backupPreview" style="white-space:pre-wrap"></pre><button id="applyBackup" type="button" onclick="applySettingsImport()" hidden>Vorschau übernehmen</button></article>
<article><h2>Firmware / OTA</h2><p>Neue <code>firmware.bin</code> über den geschützten Updater hochladen.</p><a class="btn" href="/update">OTA-Upload öffnen</a><p><small>Vorher Einstellungen sichern. Update niemals bei instabiler Stromversorgung.</small></p></article>
)HTML"
#ifdef CHAOSUHR_PUBLIC_BUILD
R"HTML(<article><h2>Werkseinstellungen</h2><p>Alle Uhr-Einstellungen, eigene Animationen, der Admin-Zugang und die gespeicherten WLAN-Daten werden gelöscht. Danach beginnt die Ersteinrichtung erneut.</p><label><input id="resetConfirm" type="checkbox" onchange="document.getElementById('resetButton').disabled=!this.checked"> Ich möchte alle Einstellungen, Uploads und Zugangsdaten löschen.</label><button id="resetButton" onclick="factoryReset()" disabled>Auf Werkseinstellungen zurücksetzen</button><p id="resetMessage" role="status"></p></article>)HTML"
#endif
R"HTML(
</main><script src="/errors.js?v=2.0.11-beta.4" onerror="document.addEventListener(&#x27;DOMContentLoaded&#x27;,()=&gt;{document.body.textContent=&quot;Fehler HTTP-018\nDiagnoseskript konnte nicht geladen werden\nDer Browser konnte das gemeinsame lokale Skript nicht laden; die genauere Ursache ist nicht erkennbar.\nMaßnahmen: Uhradresse und Netzwerk prüfen und Seite neu laden.&quot;})"></script><script>
const ids=['mode','effect','brightness','speed','duration','nightOn','nightFrom','nightUntil','nightBrightness','corner','vertical','zigzag','autoOrder'];
let automaticChoices=[];
let effectChoices=[],pendingImport=null;
const optionFlags=['secondsBar','showClock','showAnimations','showWeather','showSensors'];
const optionIds=['ledPin','weekdayStyle',...optionFlags];
let currentOptions={ledPin:12,weekdayStyle:0,secondsBar:1,showClock:1,showAnimations:1,showWeather:0,showSensors:0};
let savedMasks={autoEffects:1073741823,autoUploads:0};
function updateDisplayFlow(){
  const mode=Number(document.getElementById('mode').value),target=document.getElementById('displayFlow');
  if(mode===1){target.textContent='Uhrzeit – dauerhaft.';return}
  if(mode===2){target.textContent='Gewählte Animation – dauerhaft; bei nicht verfügbarer Upload-Datei erscheint die Uhr.';return}
  const pages=[['showClock','Uhrzeit'],['showWeather','Wetter (wenn aktuell)'],['showSensors','ein Sensorwert (wenn aktuell)'],['showAnimations','eine gewählte Animation']]
    .filter(([id])=>document.getElementById(id).checked).map(([,label])=>label);
  if(!pages.length){target.textContent='Bitte mindestens eine Anzeigeart wählen; diese Auswahl lässt sich nicht speichern.';return}
  if(document.getElementById('showAnimations').checked&&!automaticChoices.some(choice=>choice.input.checked)){target.textContent='Bitte mindestens eine Animation wählen oder die Anzeigeart Animationen abwählen; diese Auswahl lässt sich nicht speichern.';return}
  const seconds=Number(document.getElementById('duration').value);
  target.textContent=pages.join(' → ')+(pages.length>1?' → von vorn':' – wiederholt')+` · je ${seconds} s. `+
    (document.getElementById('showAnimations').checked?'Animationen: '+(Number(document.getElementById('autoOrder').value)?'der Reihe nach.':'zufällige Runden, jede einmal.'):'')+
    ' Ohne verfügbare Anzeige: Uhrzeit. Nachtmodus: gleicher Ablauf mit Nachthelligkeit.';
}
for(const id of ['mode','duration','autoOrder','showClock','showWeather','showSensors','showAnimations'])
  document.getElementById(id).addEventListener?.('input',updateDisplayFlow);
function loadDisplayOptions(d){
  const pins=d.ledPins||[{gpio:d.ledPin??12,label:'LED-Datenpin'}],select=document.getElementById('ledPin');select.replaceChildren();
  for(const pin of pins){const option=document.createElement('option');option.value=pin.gpio;option.textContent=pin.label;select.append(option)}
  currentOptions=Object.fromEntries(optionIds.map(key=>[key,d[key]??currentOptions[key]]));
  optionIds.forEach(key=>{const element=document.getElementById(key);element[optionFlags.includes(key)?'checked':'value']=optionFlags.includes(key)?!!currentOptions[key]:currentOptions[key]});
  savedMasks={autoEffects:d.autoEffects,autoUploads:d.autoUploads||0};
}
function formOptions(){return Object.fromEntries(optionIds.map(key=>[key,optionFlags.includes(key)?(document.getElementById(key).checked?1:0):Number(document.getElementById(key).value)]))}
const legacyBackupKeys=[...ids,'autoEffects','autoUploads'];
const backupKeys=[...legacyBackupKeys,...optionIds.filter(key=>key!=='ledPin')];
function automaticMask(){return automaticChoices.reduce((mask,choice)=>mask+(choice.input.checked&&!choice.uploaded?2**(choice.id-1):0),0)}
function automaticUploadMask(){return automaticChoices.reduce((mask,choice)=>mask+(choice.input.checked&&choice.uploaded?2**(choice.id-31):0),0)}
function updateAutomaticCount(){document.getElementById('autoCount').textContent=`${automaticChoices.filter(choice=>choice.input.checked).length} von ${automaticChoices.length} ausgewählt`;updateDisplayFlow()}
function selectAutomatic(checked){automaticChoices.forEach(choice=>choice.input.checked=checked);updateAutomaticCount()}
function buildAutomaticChoices(effects,mask,uploads=0){
  const list=document.getElementById('autoEffects');list.replaceChildren();
  automaticChoices=effects.map(effect=>{
    const label=document.createElement('label'),input=document.createElement('input'),name=document.createElement('span');
    input.type='checkbox';input.checked=((effect.uploaded?uploads:mask)&2**(effect.uploaded?effect.id-31:effect.id-1))!==0;input.onchange=updateAutomaticCount;
    name.textContent=effect.name;label.append(input,name);list.append(label);
    return {id:effect.id,input,uploaded:!!effect.uploaded};
  });updateAutomaticCount();
}
async function readJson(path){const r=await chaosFetch(path,{cache:'no-store'});return chaosJson(r)}
async function load(){
  document.getElementById('saveButton').disabled=true;
  try{
    const [d,effects]=await Promise.all([readJson('/api/status'),readJson('/api/effects')]);
    effectChoices=effects;
    const select=document.getElementById('effect');select.replaceChildren();
    for(const [kind,label] of [['normal','Animationen'],['legacy','Klassiker der ChaosUhr'],['uploaded','Eigene Uploads']]){
      const choices=effects.filter(effect=>(effect.uploaded?'uploaded':effect.legacy?'legacy':'normal')===kind);
      if(!choices.length)continue;
      const group=document.createElement('optgroup');group.label=label;
      for(const effect of choices){
        const option=document.createElement('option');option.value=effect.id;option.textContent=effect.name;group.append(option);
      }select.append(group);
    }
    buildAutomaticChoices(effects,d.autoEffects,d.autoUploads||0);loadDisplayOptions(d);
    document.getElementById('status').textContent=`Firmware ${d.version} · ${d.ip} · WLAN ${d.rssi} dBm · Freier Heap ${d.heap} B · Flash ${d.flash} B · Freier OTA-Platz ${d.ota} B · Uhrzeit ${d.timeValid?'bereit':'wird synchronisiert'}`;
    ids.forEach(k=>{const el=document.getElementById(k);el[el.type==='checkbox'?'checked':'value']=d[k]});
    if(!effects.some(effect=>effect.id===d.effect))document.getElementById('effect').value=1;
    document.getElementById('uploadPanel').hidden=!d.animationUploads;
    document.getElementById('securityStatus').textContent=d.credentialsMigrationPending?'Admin-Zugang wird noch im alten Format gespeichert. Richte den Animationsspeicher ein, um die Passwortspeicherung sicher zu migrieren. Dein bisheriges Passwort bleibt gültig.':'Admin-Passwort wird als gesalzener Hash gespeichert. Die lokale Oberfläche verwendet weiterhin HTTP.';
    if(d.animationUploads)showAnimationStorage(await readJson('/api/animations'));
    bVal.value=d.brightness+' %';sVal.value=d.speed+' %';dVal.value=d.duration+' s';nVal.value=d.nightBrightness+' %';updateDisplayFlow();
    document.getElementById('saveButton').disabled=false;
    document.getElementById('mqttPanel').hidden=!d.mqttAvailable;
    if(d.mqttAvailable)await loadMqtt();
    updateDisplayFlow();
  }catch(e){document.getElementById('status').textContent=chaosMessage(e)}
}
async function save(){
  try{
    const d={};ids.forEach(k=>{const e=document.getElementById(k);d[k]=e.type==='checkbox'?(e.checked?1:0):Number(e.value)});
    d.autoEffects=automaticMask();
    d.autoUploads=automaticUploadMask();
    Object.assign(d,formOptions());
    if(!d.showClock&&!d.showAnimations&&!d.showWeather&&!d.showSensors)throw chaosFailure('CFG_DISPLAY');
    if(!d.autoEffects&&!d.autoUploads){
      if(d.showAnimations)throw chaosFailure('CFG_ANIMATION_SELECTION');
      Object.assign(d,savedMasks);
    }
    if(d.ledPin!==currentOptions.ledPin&&!confirm('LED-Datenpin ändern und die Uhr nach dem Speichern neu starten?'))return;
    const {token}=await readJson('/api/session');
    const r=await chaosFetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':token},body:new URLSearchParams(d)});
    await chaosCheck(r);
    const result=typeof r.json==='function'?await chaosJson(r):{};
    msg.textContent=result.restarting?'Gespeichert – die Uhr startet mit dem neuen LED-Pin neu.':'✔ gespeichert';
    if(!result.restarting){currentOptions=formOptions();savedMasks={autoEffects:d.autoEffects,autoUploads:d.autoUploads};setTimeout(()=>msg.textContent='',3000)}
  }catch(e){msg.textContent=chaosMessage(e)}
}
const mqttFields={host:'mqHost',port:'mqPort',user:'mqUser',base:'mqBase',discoveryPrefix:'mqDiscoveryPrefix',weatherTopic:'mqWeatherTopic',temperatureTopic:'mqTemperatureTopic',staleSeconds:'mqStaleSeconds'};
async function loadMqtt(){
  try{
    const d=await readJson('/api/mqtt');
    Object.entries(mqttFields).forEach(([key,id])=>document.getElementById(id).value=d[key]??'');
    document.getElementById('mqEnabled').checked=!!d.enabled;document.getElementById('mqDiscovery').checked=!!d.discovery;
    document.getElementById('mqPassword').value='';document.getElementById('mqClearPassword').checked=false;
    document.getElementById('mqPassword').placeholder=d.hasPassword?'Passwort gespeichert – leer lassen zum Behalten':'MQTT-Passwort (optional)';
    const list=document.getElementById('mqttSensors');list.replaceChildren();
    for(let i=0;i<4;i++){
      const entry=d.sensors?.[i]||{},group=document.createElement('div'),title=document.createElement('h3');title.textContent='Sensor '+(i+1);group.append(title);
      for(const [key,label,max] of [['topic','Topic',95],['label','Anzeigename',24],['unit','Einheit, z. B. C, % oder W',8]]){
        const caption=document.createElement('label'),input=document.createElement('input');caption.textContent=label;input.id='mqSensor'+i+key;input.type='text';input.maxLength=max;input.value=entry[key]||'';caption.append(input);group.append(caption);
      }list.append(group);
    }
    document.getElementById('mqttStatus').textContent=!d.storageReady?chaosFailure('FS_MOUNT').message:!d.enabled?'MQTT ist ausgeschaltet.':d.connected?'MQTT ist verbunden.':d.connectionFailure?chaosFailure(d.connectionFailure.code,0,d.connectionFailure).message:d.connectionError?'MQTT-Verbindungsfehler '+d.connectionError+'. Details unter Diagnose.':'MQTT-Verbindung wird aufgebaut.';
  }catch(error){document.getElementById('mqttStatus').textContent='MQTT-Status nicht verfügbar: '+chaosMessage(error)}
}
async function saveMqtt(){
  const message=document.getElementById('mqttMessage');
  try{
    const values={};Object.entries(mqttFields).forEach(([key,id])=>values[key]=document.getElementById(id).value);
    values.enabled=document.getElementById('mqEnabled').checked?1:0;values.discovery=document.getElementById('mqDiscovery').checked?1:0;
    values.password=document.getElementById('mqPassword').value;values.clearPassword=document.getElementById('mqClearPassword').checked?1:0;
    for(let i=0;i<4;i++)for(const [field,suffix] of [['topic','Topic'],['label','Label'],['unit','Unit']])values['sensor'+i+suffix]=document.getElementById('mqSensor'+i+field).value;
    if(values.enabled&&!values.host)throw chaosFailure('CFG_HOST');
    const {token}=await readJson('/api/session'),r=await chaosFetch('/api/mqtt',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':token},body:new URLSearchParams(values)});
    await chaosCheck(r);message.textContent='MQTT-Konfiguration gespeichert. Die Verbindung wird neu aufgebaut.';await loadMqtt();
  }catch(error){message.textContent='MQTT konnte nicht gespeichert werden: '+chaosMessage(error)}
}
async function loadDiagnostics(){
 const message=document.getElementById('diagnosticMessage');
 try{const data=await readJson('/api/diagnostics');document.getElementById('supportDiagnostics').value=JSON.stringify(data,null,2);message.textContent=data.lastError?'Letzter Fehler: '+data.lastError+'. Details und Maßnahmen stehen im kopierbaren Text.':'Keine Fehler aufgezeichnet.';}
 catch(error){message.textContent=chaosMessage(error)}
}
async function exportSettings(){
  const message=document.getElementById('backupMessage');
  try{
    const current=await readJson('/api/status');
    const storage=current.animationUploads?await readJson('/api/animations'):{uploads:[]};
    const settings=Object.fromEntries(backupKeys.map(key=>[key,current[key]??currentOptions[key]??0]));
    const uploads=storage.uploads.filter(upload=>upload.valid).map(upload=>({id:upload.id,sha256:upload.sha256}));
    const backup={format:'chaosuhr-settings',version:2,firmware:current.version,settings,uploads};
    const blob=new Blob([JSON.stringify(backup,null,2)+'\n'],{type:'application/json'}),url=URL.createObjectURL(blob),link=document.createElement('a');
    link.href=url;link.download='chaosuhr-einstellungen-'+current.version+'.json';link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
    message.textContent='Einstellungen gesichert. Eigene Animationen bitte zusätzlich als .chaos herunterladen.';
  }catch(error){message.textContent='Sicherung fehlgeschlagen: '+chaosMessage(error)}
}
function validateSettingsBackup(backup,currentUploads){
  if(!backup || typeof backup!=='object' || Array.isArray(backup) || Object.keys(backup).some(key=>!['format','version','firmware','settings','uploads'].includes(key)) || backup.format!=='chaosuhr-settings' || ![1,2].includes(backup.version) || typeof backup.firmware!=='string' || backup.firmware.length>32)throw chaosFailure('BACKUP_SCHEMA');
  const settings=backup.settings,keys=backup.version===1?legacyBackupKeys:backupKeys;
  if(!settings || typeof settings!=='object' || Array.isArray(settings) || Object.keys(settings).length!==keys.length || keys.some(key=>!Object.hasOwn(settings,key) || !Number.isInteger(settings[key])))throw chaosFailure('BACKUP_FIELDS');
  const bounds={mode:[0,2],effect:[1,38],brightness:[1,100],speed:[1,100],duration:[5,120],nightOn:[0,1],nightFrom:[0,23],nightUntil:[0,23],nightBrightness:[1,100],corner:[0,3],vertical:[0,1],zigzag:[0,1],autoOrder:[0,1],autoEffects:[0,1073741823],autoUploads:[0,255],secondsBar:[0,1],weekdayStyle:[0,1],showClock:[0,1],showAnimations:[0,1],showWeather:[0,1],showSensors:[0,1]};
  for(const key of keys)if(settings[key]<bounds[key][0]||settings[key]>bounds[key][1])throw chaosFailure('BACKUP_RANGE');
  if(!settings.autoEffects&&!settings.autoUploads)throw chaosFailure('CFG_ANIMATION_SELECTION');
  if(!Array.isArray(backup.uploads)||backup.uploads.length>8)throw chaosFailure('BACKUP_UPLOAD_LIST');
  const mapping=new Map();
  for(const upload of backup.uploads){
    if(!upload || Object.keys(upload).length!==2 || !Number.isInteger(upload.id) || upload.id<31 || upload.id>38 || typeof upload.sha256!=='string' || !/^[a-f0-9]{64}$/.test(upload.sha256) || mapping.has(upload.id))throw chaosFailure('BACKUP_UPLOAD_HASH');
    const match=currentUploads.find(current=>current.valid&&current.sha256===upload.sha256);mapping.set(upload.id,match?.id);
  }
  const {ledPin,...displayDefaults}=currentOptions;
  const result={...displayDefaults,...settings};result.autoUploads=0;
  if(!result.showClock&&!result.showAnimations&&!result.showWeather&&!result.showSensors)throw chaosFailure('CFG_DISPLAY');
  for(let bit=0;bit<8;bit++)if(settings.autoUploads&2**bit){const mapped=mapping.get(31+bit);if(!mapped)throw chaosFailure('BACKUP_AUTO_UPLOAD');result.autoUploads+=2**(mapped-31)}
  if(settings.effect>30){const mapped=mapping.get(settings.effect);if(!mapped)throw chaosFailure('BACKUP_SINGLE_UPLOAD');result.effect=mapped}
  return result;
}
async function previewSettingsImport(){
  pendingImport=null;document.getElementById('applyBackup').hidden=true;document.getElementById('backupPreview').textContent='';
  const message=document.getElementById('backupMessage');
  try{
    const file=document.getElementById('settingsFile').files?.[0];if(!file)return;
    if(file.size>8192)throw chaosFailure('BACKUP_SIZE');
    const current=await readJson('/api/status'),storage=current.animationUploads?await readJson('/api/animations'):{uploads:[]};
    let text;try{text=await file.text()}catch(_){throw chaosFailure('BACKUP_READ')}
    let backup;try{backup=JSON.parse(text)}catch(_){throw chaosFailure('BACKUP_JSON')}
    pendingImport=validateSettingsBackup(backup,storage.uploads);
    const count=value=>value.toString(2).replaceAll('0','').length;
    const d=pendingImport;
    document.getElementById('backupPreview').textContent=`Vorschau – noch nicht gespeichert\nModus: ${['Automatik','Nur Uhrzeit','Einzelanimation'][d.mode]}\nHelligkeit: ${d.brightness} % · Tempo: ${d.speed} %\nSzenendauer: ${d.duration} s\nAutomatik: ${count(d.autoEffects)+count(d.autoUploads)} Animationen, ${d.autoOrder?'der Reihe nach':'zufällige Runden'}\nNachtmodus: ${d.nightOn?'an':'aus'}, ${d.nightFrom} bis ${d.nightUntil} Uhr, ${d.nightBrightness} %\nAuch die Matrixausrichtung wird übernommen.\nWLAN und Admin-Zugang bleiben erhalten.`;
    document.getElementById('applyBackup').hidden=false;message.textContent='Prüfe die Vorschau, bevor du sie übernimmst.';
  }catch(error){pendingImport=null;message.textContent='Import abgelehnt: '+chaosMessage(error)}
}
function applySettingsImport(){
  if(!pendingImport)return;
  const d=pendingImport;ids.forEach(key=>{const element=document.getElementById(key);element[element.type==='checkbox'?'checked':'value']=element.type==='checkbox'?!!d[key]:d[key]});
  buildAutomaticChoices(effectChoices,d.autoEffects,d.autoUploads);
  optionIds.filter(key=>key!=='ledPin').forEach(key=>{document.getElementById(key)[optionFlags.includes(key)?'checked':'value']=optionFlags.includes(key)?!!d[key]:d[key]});
  bVal.value=d.brightness+' %';sVal.value=d.speed+' %';dVal.value=d.duration+' s';nVal.value=d.nightBrightness+' %';updateDisplayFlow();
  document.getElementById('backupMessage').textContent='Vorschau übernommen. Zum dauerhaften Anwenden Einstellungen speichern.';document.getElementById('applyBackup').hidden=true;pendingImport=null;
}
let animationBusy=false;
function showAnimationStorage(storage){
  document.getElementById('animationStorage').textContent=storage.ready?`${storage.uploads.length} von 8 Plätzen belegt · ${Math.round(storage.used/1024)} / ${Math.round(storage.total/1024)} KiB Speicher`:'Animationsspeicher noch nicht eingerichtet oder nicht lesbar. Vor dem ersten Upload einrichten.';
  document.getElementById('initializeAnimations').hidden=storage.ready;
  document.getElementById('uploadAnimation').disabled=!storage.ready;
  const list=document.getElementById('uploadedAnimations');list.replaceChildren();
  for(const upload of storage.uploads){
    const row=document.createElement('p'),name=document.createElement('span'),download=document.createElement('a'),remove=document.createElement('button');
    name.textContent=upload.name+(upload.valid?` (${upload.frames} Bilder) `:' (Datei beschädigt) ');
    download.href='/api/animations/download?id='+encodeURIComponent(upload.id);download.textContent='Herunterladen';
    remove.type='button';remove.textContent='Löschen';remove.onclick=()=>deleteAnimation(upload.id,upload.name);
    row.append(name,download,remove);list.append(row);
  }
}
async function changeAnimation(path,body){
  const {token}=await readJson('/api/session');
  const headers={'X-CSRF-Token':token};
  if(body instanceof URLSearchParams)headers['Content-Type']='application/x-www-form-urlencoded';
  const response=await chaosFetch(path,{method:'POST',headers,body});
  await chaosCheck(response);
  return chaosJson(response);
}
async function runAnimationChange(action,success){
  if(animationBusy)return;animationBusy=true;
  const message=document.getElementById('uploadMessage');message.textContent='Bitte warten …';
  try{await action();await load();message.textContent=success}catch(error){message.textContent=chaosMessage(error)}
  finally{animationBusy=false}
}
async function initializeAnimations(){
  if(!confirm('Animationsspeicher einrichten? Dabei werden vorhandene Dateien in diesem Speicher gelöscht. WLAN, Admin-Zugang und Uhreinstellungen bleiben erhalten.'))return;
  await runAnimationChange(()=>changeAnimation('/api/animations/initialize',new URLSearchParams({confirm:'INITIALIZE'})),'Animationsspeicher ist bereit.');
}
async function uploadAnimation(){
  const file=document.getElementById('animationFile').files?.[0];
  if(!file){document.getElementById('uploadMessage').textContent=chaosFailure('CFG_UPLOAD_EMPTY').message;return}
  if(file.size>24576){document.getElementById('uploadMessage').textContent='Datei darf höchstens 24 KiB groß sein.';return}
  const form=new FormData();form.append('animation',file);
  await runAnimationChange(()=>changeAnimation('/api/animations/upload',form),'Hochgeladen. Jetzt für die Automatik oder als Einzelanimation auswählen und Einstellungen speichern.');
}
async function deleteAnimation(id,name){
  if(!confirm(`Upload „${name}“ wirklich löschen?`))return;
  await runAnimationChange(()=>changeAnimation('/api/animations/delete',new URLSearchParams({id,confirm:'DELETE'})),'Upload gelöscht.');
}
async function factoryReset(){const message=document.getElementById('resetMessage');if(!document.getElementById('resetConfirm').checked){message.textContent=chaosFailure('AUTH_CONFIRM').message;return}if(!confirm('Wirklich alle Einstellungen, eigene Animationen, den Admin-Zugang und die WLAN-Daten löschen?'))return;const button=document.getElementById('resetButton');button.disabled=true;try{const {token}=await readJson('/api/session');const response=await chaosFetch('/api/factory-reset',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':token},body:new URLSearchParams({confirm:'RESET'})});await chaosCheck(response);message.textContent='Die Uhr wird zurückgesetzt und startet neu. Verbinde dich anschließend mit dem Einrichtungs-WLAN; den neuen Code zeigt die Uhr an.';}catch(error){message.textContent='Fehler: '+chaosMessage(error);button.disabled=false}}
chaosSessionStart(load);
</script></body></html>
)HTML";

static bool auth(){
#ifdef CHAOSUHR_PUBLIC_BUILD
  return publicAccess.authenticate();
#else
  if(server.authenticate(OTA_USER,OTA_PASS)) return true;
  server.sendHeader("WWW-Authenticate","Basic realm=\"ChaosUhr\"");sendError(server,401,ErrorCode::AUTH_REJECTED);return false;
#endif
}
static void effectsApi(){
  if(!auth()) return;
  String result="[";
  result.reserve(2400);
  for(uint8_t id=1;id<=EFFECT_COUNT;id++){
    if(id>1) result+=',';
    result+="{\"id\":"+String(id)+",\"name\":\""+String(effectName(id))+"\",\"legacy\":";
    result+=isLegacyEffect(id)?"true":"false";
    result+=",\"uploaded\":false}";
  }
#ifdef CHAOSUHR_HAS_UPLOADS
  for(uint8_t i=0;i<UPLOAD_SLOTS;++i){
    const auto& upload=animations.slot(i);if(!upload.valid)continue;
    result+=",{\"id\":"+String(EFFECT_COUNT+1+i)+",\"name\":\""+String(upload.name)+"\",\"legacy\":false,\"uploaded\":true}";
  }
#endif
  result+=']';
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json; charset=utf-8",result);
}
#include "mqtt_api.h"
#include "diagnostics_api.h"
static String gpioChoicesJson(){
  struct Pin {uint8_t gpio;const char* label;};
#if defined(ESP32)
  const Pin pins[]={{13,"GPIO13"},{14,"GPIO14"},{18,"GPIO18"},{19,"GPIO19"},{21,"GPIO21"},{22,"GPIO22"},{23,"GPIO23"},{25,"GPIO25"},{26,"GPIO26"},{27,"GPIO27"},{32,"GPIO32 (Standard)"},{33,"GPIO33"}};
#else
  const Pin pins[]={{4,"D2 / GPIO4"},{5,"D1 / GPIO5"},{12,"D6 / GPIO12 (Standard)"},{13,"D7 / GPIO13"},{14,"D5 / GPIO14"}};
#endif
  String result="[";bool first=true;
  for(const auto& pin:pins){if(!first)result+=',';first=false;
    result+="{\"gpio\":"+String(pin.gpio)+",\"label\":\""+String(pin.label)+"\"}";}
  return result+"]";
}
static void statusApi(){
  if(!auth()) return;
  String s="{";
  s+="\"version\":\""+String(VERSION)+"\",\"ip\":\""+WiFi.localIP().toString()+"\",";
  s+="\"rssi\":"+String(WiFi.RSSI())+",\"heap\":"+String(ESP.getFreeHeap());
  s+=",\"flash\":"+String(deviceFlashSize())+",\"ota\":"+String(ESP.getFreeSketchSpace());
  s+=",\"lastError\":\""+errorText(errorDefinition(errorRegistry().last()).code)+"\"";
  s+=",\"platform\":\""+String(DEVICE_FAMILY)+"\",\"ledPin\":"+String(displayOptions.ledPin);
  s+=",\"secondsBar\":"+String((displayOptions.flags&DISPLAY_SECONDS)?1:0)+",\"weekdayStyle\":"+String(displayOptions.weekdayStyle);
  s+=",\"showClock\":"+String((displayOptions.flags&DISPLAY_CLOCK)?1:0)+",\"showAnimations\":"+String((displayOptions.flags&DISPLAY_ANIMATIONS)?1:0);
  s+=",\"showWeather\":"+String((displayOptions.flags&DISPLAY_WEATHER)?1:0)+",\"showSensors\":"+String((displayOptions.flags&DISPLAY_SENSORS)?1:0);
  s+=",\"optionsSaved\":"+String(optionsPersisted?"true":"false")+",\"mqttAvailable\":true,\"mqttConnected\":"+String(clockMqtt.connected()?"true":"false");
  s+=",\"ledPins\":"+gpioChoicesJson();
  s+=",\"otaProduct\":\""+String(FIRMWARE_PRODUCT)+"\"";
  s+=",\"otaMaxBytes\":"+String(FIRMWARE_MAX_BYTES);
#ifdef CHAOSUHR_PUBLIC_BUILD
  s+=",\"credentialsMigrationPending\":"+String(publicAccess.migrationPending()?"true":"false");
#endif
  s+=",\"mode\":"+String(cfg.mode)+",\"effect\":"+String(cfg.effect);
  s+=",\"activeEffect\":"+String(activeEffect());
  uint8_t uploadedCount=0;
#ifdef CHAOSUHR_HAS_UPLOADS
  for(uint8_t i=0;i<UPLOAD_SLOTS;++i)if(animations.slot(i).valid)++uploadedCount;
#endif
  s+=",\"effectCount\":"+String(EFFECT_COUNT+uploadedCount)+",\"builtinEffectCount\":"+String(EFFECT_COUNT);
  s+=",\"uploadedEffectCount\":"+String(uploadedCount)+",\"maxEffectId\":"+String(TOTAL_EFFECT_COUNT);
  s+=",\"autoEffects\":"+String(cfg.autoEffects)+",\"autoOrder\":"+String(cfg.autoOrder);
  s+=",\"autoUploads\":"+String(cfg.autoUploads);
#ifdef CHAOSUHR_HAS_UPLOADS
  s+=",\"animationUploads\":true";
#else
  s+=",\"animationUploads\":false";
#endif
  s+=",\"timeValid\":"+String(time(nullptr)>1700000000?"true":"false");
  s+=",\"settingsSaved\":"+String(settingsPersisted?"true":"false");
  s+=",\"brightness\":"+String(cfg.brightness)+",\"speed\":"+String(cfg.speed);
  s+=",\"duration\":"+String(cfg.duration);
  s+=",\"nightOn\":"+String(cfg.nightOn)+",\"nightFrom\":"+String(cfg.nightFrom);
  s+=",\"nightUntil\":"+String(cfg.nightUntil)+",\"nightBrightness\":"+String(cfg.nightBrightness);
  s+=",\"corner\":"+String(cfg.corner)+",\"vertical\":"+String(cfg.vertical)+",\"zigzag\":"+String(cfg.zigzag)+"}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",s);
}
static void saveApi(){
  if(!auth())return;
  if(server.method()!=HTTP_POST){sendError(server,405,ErrorCode::HTTP_METHOD);return;}
#ifdef CHAOSUHR_PUBLIC_BUILD
  if(!publicAccess.validMutationToken()){sendError(server,403,ErrorCode::AUTH_CSRF);return;}
#endif
  if(!server.args()){sendError(server,400,ErrorCode::CFG_EMPTY);return;}
  Settings next=cfg;DisplayOptions nextOptions=displayOptions;
  for(int i=0;i<server.args();i++){
    const String key=server.argName(i);
    for(int j=0;j<i;j++){
      if(key==server.argName(j)){sendError(server,400,ErrorCode::CFG_DUPLICATE);return;}
    }
    const bool accepted=displayOptionKey(key.c_str())?applyDisplayOption(nextOptions,key.c_str(),server.arg(i).c_str(),TARGET_ESP32):applySetting(next,key.c_str(),server.arg(i).c_str());
    if(!accepted){
      const char* knownKeys[]={"brightness","speed","effect","mode","duration","nightBrightness","nightFrom","nightUntil","nightOn","corner","vertical","zigzag","autoEffects","autoUploads","autoOrder"};
      bool known=displayOptionKey(key.c_str());for(const auto* candidate:knownKeys)if(key==candidate)known=true;
      const String value=server.arg(i);bool digits=value.length()!=0;for(size_t at=0;at<value.length();++at)if(value[at]<'0'||value[at]>'9')digits=false;
      sendError(server,400,!known?ErrorCode::CFG_UNKNOWN:!digits?ErrorCode::CFG_NUMBER_FORMAT:key=="ledPin"?ErrorCode::CFG_GPIO:ErrorCode::CFG_RANGE);return;
    }
  }
  sealDisplayOptions(nextOptions);
  if(!validDisplayOptions(nextOptions,TARGET_ESP32)){sendError(server,400,ErrorCode::CFG_DISPLAY);return;}
  if(!next.autoEffects && !next.autoUploads){
    if(nextOptions.flags&DISPLAY_ANIMATIONS){sendError(server,400,ErrorCode::CFG_ANIMATION_SELECTION);return;}
    next.autoEffects=cfg.autoEffects;next.autoUploads=cfg.autoUploads;
  }
#ifdef CHAOSUHR_HAS_UPLOADS
  if((next.autoUploads&~animations.available()) || (next.effect>EFFECT_COUNT && !animations.playable(next.effect))){
    sendError(server,400,ErrorCode::CFG_UPLOAD_MISSING);return;
  }
#else
  if(next.autoUploads || next.effect>EFFECT_COUNT){sendError(server,400,ErrorCode::CFG_UPLOAD_UNSUPPORTED);return;}
#endif
  const bool pinChanged=displayOptions.ledPin!=nextOptions.ledPin;
  const bool changed=memcmp(&cfg,&next,sizeof cfg)!=0 || memcmp(&displayOptions,&nextOptions,sizeof nextOptions)!=0;
  const bool restartAutomatic=cfg.mode!=next.mode || cfg.autoEffects!=next.autoEffects || cfg.autoOrder!=next.autoOrder || cfg.autoUploads!=next.autoUploads;
  if(!persistClockOptions(clockStorage,cfg,next,displayOptions,nextOptions,settingsPersisted,optionsPersisted)){
    sendError(server,500,ErrorCode::CFG_SAVE);return;
  }
  if(changed){
    if(restartAutomatic) autoPlayback.reset();
    scene.reset(millis(),cfg.effect);resetAnimation();
  }
  server.send(200,"application/json",pinChanged?"{\"ok\":true,\"restarting\":true}":"{\"ok\":true}");
  if(pinChanged)publicAccess.restartAfterUpdate();
}

#ifdef CHAOSUHR_HAS_UPLOADS
static String animationDigest(uint8_t id){
  auto file=animations.openDownload(id);if(!file)return "";
  Sha256Stream hash;uint8_t buffer[128];
  for(;;){const size_t length=file.read(buffer,sizeof buffer);if(!length)break;hash.add(buffer,length);yield();}
  file.close();uint8_t digest[32];char text[65];if(!hash.digest(digest))return "";
  for(size_t i=0;i<32;++i)snprintf(text+2*i,3,"%02x",digest[i]);
  return String(text);
}
static bool animationMutationAllowed(){
  return publicAccess.authenticate() && publicAccess.validMutationToken();
}
static bool animationSpaceAvailable(size_t additional){
  size_t used=0,total=0;
#if defined(ESP32)
  used=LittleFS.usedBytes();total=LittleFS.totalBytes();
#else
  FSInfo info={};if(!LittleFS.info(info))return false;used=info.usedBytes;total=info.totalBytes;
#endif
  return total>=used && total-used>=additional+12288;
}
static void animationStatusApi(){
  if(!auth())return;
  size_t usedBytes=0,totalBytes=0;
  if(animations.ready()){
#if defined(ESP32)
    usedBytes=LittleFS.usedBytes();totalBytes=LittleFS.totalBytes();
#else
    FSInfo info={};LittleFS.info(info);usedBytes=info.usedBytes;totalBytes=info.totalBytes;
#endif
  }
  String result="{\"ready\":"+String(animations.ready()?"true":"false")+",\"used\":"+String(usedBytes)+",\"total\":"+String(totalBytes)+",\"uploads\":[";
  bool first=true;
  for(uint8_t i=0;i<UPLOAD_SLOTS;++i){
    const auto& slot=animations.slot(i);if(!slot.present)continue;
    if(!first)result+=',';
    first=false;
    result+="{\"id\":"+String(EFFECT_COUNT+1+i)+",\"name\":\""+String(slot.name)+"\",\"valid\":"+String(slot.valid?"true":"false")+",\"frames\":"+String(slot.frames)+",\"sha256\":\""+(slot.valid?animationDigest(EFFECT_COUNT+1+i):String(""))+"\"}";
  }
  result+="]}";server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",result);
}
static void animationInitApi(){
  if(!auth())return;
  if(!animationMutationAllowed() || server.args()!=1 || server.arg("confirm")!="INITIALIZE"){
    sendError(server,403,ErrorCode::AUTH_CONFIRM);return;
  }
  if(!animations.initialize()){sendError(server,animations.status(),animations.errorCode());return;}
  publicAccess.migrate();
  server.send(200,"application/json","{\"ok\":true}");
}
static void animationUploadDone(){
  if(server.args())animations.abortUpload();
  const uint8_t id=animations.finishUpload(animationMutationAllowed());
  if(!id){sendError(server,animations.status(),animations.errorCode());return;}
  server.send(200,"application/json",String("{\"ok\":true,\"id\":")+String(id)+"}");
}
static void animationUploadChunk(){
  auto& upload=server.upload();
  switch(upload.status){
    case UPLOAD_FILE_START:animations.startUpload(animationMutationAllowed(),upload.name=="animation");break;
    case UPLOAD_FILE_WRITE:animations.writeUpload(upload.buf,upload.currentSize);break;
    case UPLOAD_FILE_END:animations.endUpload();break;
    case UPLOAD_FILE_ABORTED:animations.abortUpload();break;
    default:break;
  }
}
static void animationDeleteApi(){
  if(!auth())return;
  if(!animationMutationAllowed() || server.args()!=2 || server.arg("confirm")!="DELETE"){
    sendError(server,403,ErrorCode::AUTH_CONFIRM);return;
  }
  uint8_t id;
  if(!parseSetting(server.arg("id").c_str(),EFFECT_COUNT+1,TOTAL_EFFECT_COUNT,id) || !animations.exists(id)){
    sendError(server,404,ErrorCode::CFG_UPLOAD_NOT_FOUND);return;
  }
  Settings next=cfg;forgetUploadedAnimation(next,id);
  if(!persistSettings(clockStorage,cfg,next,settingsPersisted)){sendError(server,500,ErrorCode::CFG_SAVE);return;}
  if(!animations.remove(id)){sendError(server,animations.status(),animations.errorCode());return;}
  autoPlayback.reset();scene.reset(millis(),cfg.effect);resetAnimation();
  server.send(200,"application/json","{\"ok\":true}");
}
static void animationDownloadApi(){
  if(!auth())return;
  uint8_t id;
  if(server.args()!=1 || !parseSetting(server.arg("id").c_str(),EFFECT_COUNT+1,TOTAL_EFFECT_COUNT,id) || !animations.exists(id)){
    sendError(server,404,ErrorCode::CFG_UPLOAD_NOT_FOUND);return;
  }
  auto file=animations.openDownload(id);if(!file){sendError(server,500,ErrorCode::FS_OPEN_READ);return;}
  server.sendHeader("Content-Disposition","attachment; filename=animation-"+String(id)+".chaos");
  server.sendHeader("Cache-Control","no-store");server.streamFile(file,"text/plain");file.close();
}
#endif

#if defined(CHAOSUHR_PUBLIC_BUILD)
struct OtaBackend {
  bool begin(){
    const size_t available=ESP.getFreeSketchSpace();
#if defined(ESP32)
    if(available<FIRMWARE_MAX_BYTES)return false;
    return Update.begin(available,U_FLASH);
#else
    if(available<=FIRMWARE_MAX_BYTES+4096)return false;
    // Keep the reserved capacity strictly larger than any accepted payload, so
    // end(false) can never accidentally finalize an aborted complete-size file.
    return Update.begin((available-4096)&0xfffff000,U_FLASH);
#endif
  }
  size_t write(const uint8_t* data,size_t size){return Update.write(const_cast<uint8_t*>(data),size);}
  bool finish(){return Update.end(true);}
  void cancel(){
#if defined(ESP32)
    Update.abort();
#else
    Update.end(false);
#endif
  }
} otaBackend;
FirmwareUpload<OtaBackend> firmwareUpload(otaBackend);
static void setupSecureOta(){
  server.on("/update",HTTP_GET,[](){server.sendHeader("Cache-Control","no-store");server.send_P(200,"text/html; charset=utf-8",OTA_HTML);});
  server.on("/update",HTTP_POST,[](){
    if(firmwareUpload.finish(publicAccess.authenticate() && publicAccess.validMutationToken(),server.args()==0)){
      server.send(200,"application/json","{\"ok\":true,\"restarting\":true}");publicAccess.restartAfterUpdate();
    }else sendError(server,firmwareUpload.status(),firmwareUpload.errorCode());
  },[](){
    auto& upload=server.upload();
    switch(upload.status){
      case UPLOAD_FILE_START:firmwareUpload.start(publicAccess.authenticate() && publicAccess.validMutationToken(),upload.name=="firmware" && upload.filename.endsWith(".bin"),server.header("X-Firmware-SHA256").c_str());break;
      case UPLOAD_FILE_WRITE:firmwareUpload.write(upload.buf,upload.currentSize);break;
      case UPLOAD_FILE_END:firmwareUpload.end();break;
      case UPLOAD_FILE_ABORTED:firmwareUpload.abort();break;
      default:break;
    }
  });
}
static bool portalRequestAllowed(ClockWebServer& target,bool mutation){
  const String host=target.header("Host"),origin=target.header("Origin"),address=WiFi.softAPIP().toString();
  if(target.client().localIP()!=WiFi.softAPIP()){recordError(ErrorCode::AUTH_SETUP_AP);return false;}
  if(!setupHostAllowed(host.c_str(),address.c_str())){recordError(ErrorCode::HTTP_SETUP_HOST);return false;}
  if(!originMatchesHost(origin.c_str(),host.c_str())){recordError(ErrorCode::AUTH_ORIGIN);return false;}
  const String token=target.header("X-CSRF-Token");
  if(mutation && target.method()!=HTTP_POST){recordError(ErrorCode::HTTP_METHOD);return false;}
  if(mutation && (token.length()!=32 || !constantEqual(token.c_str(),publicAccess.portalToken(),32))){recordError(ErrorCode::AUTH_CSRF);return false;}
  return true;
}
static void setupSecurePortal(){
  auto& portal=*wifiManager.server;collectClockHeaders(portal);
  portal.on("/errors.js",HTTP_GET,[](){wifiManager.server->send_P(200,"application/javascript; charset=utf-8",ERROR_JS);});
#if defined(ESP32)
  portal.setConnectionGuard([](){
    auto& current=*wifiManager.server;
    if(current.client().localIP()==WiFi.softAPIP())return true;
    sendError(current,403,ErrorCode::AUTH_SETUP_AP);return false;
  });
#else
  portal.addHook([](const String&,const String&,WiFiClient* client,ClockWebServer::ContentTypeFunction){
    if(client->localIP()==WiFi.softAPIP())return ClockWebServer::CLIENT_REQUEST_CAN_CONTINUE;
    sendError(*wifiManager.server,403,ErrorCode::AUTH_SETUP_AP);return ClockWebServer::CLIENT_MUST_STOP;
  });
#endif
  portal.setBodyGuard([](){
    auto& current=*wifiManager.server;
    if(current.uri()!="/wifisave" && current.uri()!="/exit"){sendError(current,403,ErrorCode::AUTH_PORTAL_ACTION);return false;}
    if(portalRequestAllowed(current,true))return true;
    sendError(current,403,errorRegistry().last());return false;
  });
  for(const char* path:{"/update","/u","/erase","/restart","/paramsave","/param","/close","/info"})portal.on(path,HTTP_ANY,[](){
    sendError(*wifiManager.server,403,ErrorCode::AUTH_PORTAL_ACTION);
  });
  portal.on("/api/portal-session",HTTP_GET,[](){
    auto& current=*wifiManager.server;if(!portalRequestAllowed(current,false)){sendError(current,403,errorRegistry().last());return;}
    current.send(200,"application/json",String("{\"token\":\"")+publicAccess.portalToken()+"\"}");
  });
  portal.on("/wifisave",HTTP_GET,[](){sendError(*wifiManager.server,405,ErrorCode::HTTP_METHOD);});
  portal.on("/wifisave",HTTP_POST,[](){
    if(!portalRequestAllowed(*wifiManager.server,true)){sendError(*wifiManager.server,403,errorRegistry().last());return;}
    wifiManager.saveClockWifi();
  });
  portal.on("/exit",HTTP_GET,[](){
    if(!portalRequestAllowed(*wifiManager.server,false)){sendError(*wifiManager.server,403,errorRegistry().last());return;}
    wifiManager.server->send(200,"text/html",String(FPSTR(PORTAL_EXIT_HTML))+FPSTR(PORTAL_HEAD)+"</body></html>");
  });
  portal.on("/exit",HTTP_POST,[](){
    if(!portalRequestAllowed(*wifiManager.server,true)){sendError(*wifiManager.server,403,errorRegistry().last());return;}
    wifiManager.server->send(200,"text/plain","Einrichtung geschlossen.");wifiManager.requestPortalExit();
  });
}
#endif

// WiFiManager and the application use port 80 at different times. Keep the
// display running during the portal and retry saved credentials after timeout.
static void serviceNetwork(){
#ifdef CHAOSUHR_PUBLIC_BUILD
  publicAccess.loop();
  if(publicAccess.pending()) return;
  if(!publicAccess.ready()){
    if(publicAccess.provisioning()) server.handleClient();
    return;
  }
#endif
  const bool portalWasActive=wifiManager.getConfigPortalActive();
  if(portalWasActive) wifiManager.process();
  const bool portalActive=wifiManager.getConfigPortalActive();
  const uint32_t now=millis();
  if(portalWasActive && !portalActive){
    configureWifiPersistence(); WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
    if(WiFi.status()!=WL_CONNECTED) WiFi.begin();
    wifiAttemptStarted=now;
  }
  if(WiFi.status()==WL_CONNECTED){
    if(portalActive){wifiManager.stopConfigPortal();configureWifiPersistence();return;}
    if(!webStarted){
      server.begin(); webStarted=true;ipAnnouncement.start(WiFi.localIP().toString().c_str(),now);
      if(MDNS.begin(HOSTNAME)) MDNS.addService("http","tcp",80);else recordError(ErrorCode::WIFI_MDNS_FAILED);
      Serial.printf("ChaosUhr %s at http://%s/\n",VERSION,WiFi.localIP().toString().c_str());
    }
    wifiAttemptStarted=now;
    server.handleClient();
#if !defined(ESP32)
    MDNS.update();
#endif
  } else {
    if(webStarted){
      recordError(ErrorCode::WIFI_LINK_LOST);
      server.stop();
#if defined(ESP32)
      MDNS.end();
#else
      MDNS.close();
#endif
      webStarted=false;
    }
    if(!portalActive && now-wifiAttemptStarted>=WIFI_RETRY_MS){
      const auto saved=savedWifiState();
      recordError(saved==SavedWifiState::Missing?ErrorCode::WIFI_NO_CONFIG:saved==SavedWifiState::Unreadable?ErrorCode::WIFI_START_FAILED:ErrorCode::WIFI_CONNECT_TIMEOUT);
      wifiManager.startConfigPortal(
#ifdef CHAOSUHR_PUBLIC_BUILD
        publicAccess.setupSsid(),publicAccess.setupPassword()
#else
        "ChaosUhr-Setup",SETUP_PASS
#endif
      );
      wifiAttemptStarted=now;
    }
  }
}
void setup(){
  Serial.begin(115200);
  initializeResetDiagnostics();
  errorRegistry().setSink([](const ErrorEvent& event){
    printErrorEvent(Serial,event);
  });
#ifdef CHAOSUHR_HAS_UPLOADS
#if defined(ESP8266)
  LittleFSConfig filesystemConfig;filesystemConfig.setAutoFormat(false);LittleFS.setConfig(filesystemConfig);
#endif
#endif
  loadSettings();
  setupLedController();
  FastLED.setMaxPowerInVoltsAndMilliamps(5,1500); // estimated software power limit
  clear(); FastLED.show();
#ifdef CHAOSUHR_PUBLIC_BUILD
#ifdef CHAOSUHR_HAS_UPLOADS
  animations.begin();animations.setReserveGuard(animationSpaceAvailable);publicAccess.setResetStorage([](){return animations.factoryReset();});
#endif
  publicAccess.begin();
#endif
  configureWifiPersistence();
#ifdef CHAOSUHR_PUBLIC_BUILD
  if(publicAccess.ready()) WiFi.mode(WIFI_STA);
#else
  WiFi.mode(WIFI_STA);
#endif
#if defined(ESP32)
  WiFi.setHostname(HOSTNAME);
#else
  WiFi.hostname(HOSTNAME);
#endif
  WiFi.setAutoReconnect(true);
  wifiManager.setDebugOutput(false); // library debug output may contain secrets
  wifiManager.setHostname(HOSTNAME);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setSaveConnectTimeout(5);
  wifiManager.setConnectRetries(1);
#ifdef CHAOSUHR_PUBLIC_BUILD
  const char* menu[]={"wifi","exit"};wifiManager.setMenu(menu,2);
  wifiManager.setShowInfoUpdate(false);
  // These handlers precede the library routes, so its configuration portal
  // cannot bypass the application's admin-protected firmware updater.
  wifiManager.setWebServerCallback([](){
    setupSecurePortal();
  });
  wifiManager.setCustomHeadElement(PORTAL_HEAD);
#endif
  wifiAttemptStarted=millis();
#ifdef CHAOSUHR_PUBLIC_BUILD
  if(publicAccess.ready()){
    WiFi.begin();
    const auto saved=savedWifiState();
    if(saved==SavedWifiState::Missing){recordError(ErrorCode::WIFI_NO_CONFIG);wifiAttemptStarted-=WIFI_RETRY_MS;}
    else if(saved==SavedWifiState::Unreadable)recordError(ErrorCode::WIFI_START_FAILED);
  }
#else
  WiFi.begin();
#endif
#if defined(ESP32)
  configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3","de.pool.ntp.org","pool.ntp.org");
#else
  configTime("CET-1CEST,M3.5.0/2,M10.5.0/3","de.pool.ntp.org","pool.ntp.org");
#endif
  server.on("/",HTTP_GET,[]{
#ifdef CHAOSUHR_PUBLIC_BUILD
    if(!publicAccess.ready()){publicAccess.serveSetup();return;}
#endif
    server.sendHeader("Cache-Control","no-store");
    server.send_P(200,"text/html; charset=utf-8",INDEX_HTML);
  });
  server.on("/api/status",HTTP_GET,statusApi);
  server.on("/api/diagnostics",HTTP_GET,diagnosticsApi);
  server.onNotFound([](){sendError(server,404,ErrorCode::HTTP_NOT_FOUND);});
  server.on("/errors.js",HTTP_GET,[](){server.send_P(200,"application/javascript; charset=utf-8",ERROR_JS);});
  server.on("/api/effects",HTTP_GET,effectsApi);
  server.on("/api/settings",HTTP_POST,saveApi);
  server.on("/api/mqtt",HTTP_GET,mqttConfigApi);
  server.on("/api/mqtt",HTTP_POST,mqttSaveApi);
#ifdef CHAOSUHR_HAS_UPLOADS
  server.on("/api/animations",HTTP_GET,animationStatusApi);
  server.on("/api/animations/initialize",HTTP_POST,animationInitApi);
  server.on("/api/animations/upload",HTTP_POST,animationUploadDone,animationUploadChunk);
  server.on("/api/animations/delete",HTTP_POST,animationDeleteApi);
  server.on("/api/animations/download",HTTP_GET,animationDownloadApi);
  server.on("/animations/prompt",HTTP_GET,[](){if(auth())server.send_P(200,"text/plain; charset=utf-8",ANIMATION_PROMPT);});
  server.on("/animations/example.chaos",HTTP_GET,[](){
    if(!auth())return;
    server.sendHeader("Content-Disposition","attachment; filename=herz.chaos");
    server.send_P(200,"text/plain",ANIMATION_EXAMPLE);
  });
#endif
#ifdef CHAOSUHR_PUBLIC_BUILD
  setupSecureOta();
  if(publicAccess.provisioning()) publicAccess.startInitialPortal();
#else
  updater.setup(&server,"/update",OTA_USER,OTA_PASS);
#endif
  collectClockHeaders(server);
  random16_add_entropy(ESP.getCycleCount());
  scene.reset(millis(),cfg.effect); resetAnimation();
}
void loop(){
  serviceNetwork();
  uint32_t now=millis();
  clockMqtt.loop(WiFi.status()==WL_CONNECTED && publicAccess.ready() && !publicAccess.pending());
  if(scene.due(now,cfg.mode,cfg.duration) || (cfg.mode==0 && !(availableScenePages()&(1<<scene.page)))) nextScene();
  uint16_t interval=(isLegacyEffect(activeEffect()) || activeEffect()>EFFECT_COUNT)?20:map(cfg.speed,1,100,150,25);
  if(now-lastFrame>=interval){ lastFrame=now; tick++; render(); }
  yield();
}
