/* ChaosUhr 2.0 -- ESP8266 / ESP32 / 20x15 WS2812B
 * Independent new implementation inspired by the public wiring specification.
 * Prototype: compile/test before deployment. Source available under GPL-3.0.
 */
#include <Arduino.h>
#include "device_platform.h"
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
#include <FastLED.h>
#include <time.h>
#include <stddef.h>
#include "settings.h"
#include "display_logic.h"
#include "legacy_effects.h"
#include "clock_text.h"
#include "firmware_version.h"
#include "auto_playback.h"
#if defined(ESP8266) && defined(CHAOSUHR_PUBLIC_BUILD)
#define CHAOSUHR_HAS_UPLOADS
#include <LittleFS.h>
#include "animation_library.h"
#include "legacy_animation.h"
#include "animation_examples.h"
#endif
#ifdef CHAOSUHR_PUBLIC_BUILD
#include "public_access.h"
#else
#include "secrets.local.h"
#endif

#ifndef CHAOSUHR_LED_PIN
#define CHAOSUHR_LED_PIN D6  // Wemos D1 mini: D6 / GPIO12
#endif
static constexpr uint8_t LED_PIN=CHAOSUHR_LED_PIN;
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
ClockUpdateServer updater;
WiFiManager wifiManager;
#ifdef CHAOSUHR_PUBLIC_BUILD
PublicAccess publicAccess(server);
#endif
#ifdef CHAOSUHR_HAS_UPLOADS
AnimationLibrary<fs::FS,fs::File> animations(LittleFS,[](){yield();});
#endif
static bool webStarted=false;
static uint32_t wifiAttemptStarted=0;
static constexpr uint32_t WIFI_RETRY_MS=30000;

static Settings cfg;
static bool settingsPersisted=false;
static void loadSettings(){
  EEPROM.begin(128); EEPROM.get(0,cfg);
  settingsPersisted=normalizeLoadedSettings(cfg);
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
  if(!valid || !formatGermanDate(local,date,sizeof date))
    snprintf(date,sizeof date,"Warte auf Zeit");
  drawText(date,dateScrollX(millis()-scene.startedAt,strlen(date)),CLOCK_DATE_Y,
           valid?CRGB(255,140,35):color);
  // A small second-progress bar, separate from digits.
  if(valid) for(int i=0;i<(local.tm_sec*W/60);i++) pixel(i,H-1,CRGB(0,0,32));
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
static void render(){
#ifdef CHAOSUHR_PUBLIC_BUILD
  if(publicAccess.pending() || !publicAccess.ready() || wifiManager.getConfigPortalActive()){
    clear();FastLED.setBrightness(40);
    const char* label=publicAccess.pending()?(publicAccess.resetting()?"RESET":"START"):"SETUP";
    drawText(label,0,CLOCK_TIME_Y,CRGB(0,180,240));
    if(!publicAccess.pending()) drawText(publicAccess.setupPassword(),dateScrollX(millis(),12),CLOCK_DATE_Y,CRGB(255,140,35));
    FastLED.show();return;
  }
#endif
  time_t now=time(nullptr); tm loc;
  bool night=now>1700000000 && localtime_r(&now,&loc) && isNightHour(loc.tm_hour,cfg);
  uint8_t pct=night?cfg.nightBrightness:cfg.brightness;
  FastLED.setBrightness((uint8_t)map(pct,1,100,2,120)); // safety cap: 120/255
  const uint8_t effect=activeEffect();
  if(!effect) drawClock();
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
  const uint8_t effect=scene.clock ? autoPlayback.next(cfg,[](uint8_t count){return random8(count);},availableUploads) : scene.effect;
  scene.advance(millis(),effect);
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
</style></head><body><h1>🌈 ChaosUhr 2.0</h1><small>20×15 LEDs · lokale Steuerung</small>
<article><div id="status">Lade Status …</div></article>
<article><h2>Anzeige & Animationen</h2>
<label>Modus</label><select id="mode"><option value="0">Automatik: Uhr / Animation</option><option value="1">Nur Uhrzeit</option><option value="2">Gewählte Animation</option></select>
<label>Einzelanimation (Modus „Gewählte Animation“)</label><select id="effect"><option value="">Lade Animationen …</option></select>
<label>Helligkeit <output id="bVal"></output></label><input type="range" id="brightness" min="1" max="100" oninput="bVal.value=this.value+' %'">
<label>Tempo <output id="sVal"></output></label><input type="range" id="speed" min="1" max="100" oninput="sVal.value=this.value+' %'">
<label>Dauer je Szene <output id="dVal"></output></label><input type="range" id="duration" min="5" max="120" step="5" oninput="dVal.value=this.value+' s'">
<h2>Automatik</h2>
<p><small>Uhr → Animation → Uhr → nächste Animation. Diese Auswahl gilt nur für den Modus Automatik; jede ausgewählte Animation läuft einmal pro Runde.</small></p>
<label for="autoOrder">Reihenfolge</label><select id="autoOrder"><option value="0">Zufällig (jede einmal pro Runde)</option><option value="1">Der Reihe nach</option></select>
<details open><summary>Animationen für die Automatik auswählen</summary>
<div class="flex auto-actions"><button type="button" onclick="selectAutomatic(true)">Alle auswählen</button><button type="button" onclick="selectAutomatic(false)">Auswahl leeren</button></div>
<div id="autoEffects" class="auto-list"></div><small id="autoCount" role="status"></small>
<p><small>Mindestens eine Animation auswählen. Für eine dauerhafte Uhranzeige den Modus „Nur Uhrzeit“ verwenden.</small></p></details>
<h2>Matrixverdrahtung</h2>
<p><small>Vorsicht: Diese Einstellungen nur aendern, wenn die Anzeige gespiegelt oder verdreht erscheint.</small></p>
<label>Start-Ecke</label><select id="corner"><option value="0">Oben links</option><option value="1">Oben rechts (Standard)</option><option value="2">Unten links</option><option value="3">Unten rechts</option></select>
<label>Verlauf</label><select id="vertical"><option value="0">Zeilenweise (Standard)</option><option value="1">Spaltenweise</option></select>
<label>LED-Muster</label><select id="zigzag"><option value="1">Zickzack (Standard)</option><option value="0">Gleichgerichtet</option></select>
<h2>Nachtmodus</h2><label><input id="nightOn" type="checkbox"> Aktiv</label>
<div class="flex"><label>Beginn (Stunde)<br><input id="nightFrom" type="number" min="0" max="23" style="width:75px"></label><label>Ende (Stunde)<br><input id="nightUntil" type="number" min="0" max="23" style="width:75px"></label></div>
<label>Nachthelligkeit <output id="nVal"></output></label><input type="range" id="nightBrightness" min="1" max="100" oninput="nVal.value=this.value+' %'">
<button id="saveButton" onclick="save()" disabled>Einstellungen speichern</button> <span id="msg"></span></article>
<article id="uploadPanel" hidden><h2>Eigene Animationen</h2>
<p>Bis zu acht <code>.chaos</code>-Dateien mit je 24 KiB, 20×15 Pixeln, bis zu 16 Farben und 64 Bildern. Nach dem Upload oben für die Automatik oder als Einzelanimation auswählen.</p>
<p id="animationStorage"></p>
<button id="initializeAnimations" type="button" onclick="initializeAnimations()" hidden>Animationsspeicher einrichten</button>
<label for="animationFile">Animationsdatei</label><input id="animationFile" type="file" accept=".chaos">
<button id="uploadAnimation" type="button" onclick="uploadAnimation()">Animation hochladen</button>
<p><a class="btn" href="/animations/example.chaos">Herz-Beispiel herunterladen</a> <a class="btn" href="/animations/prompt" target="_blank" rel="noopener">ChatGPT-Vorlage öffnen</a></p>
<div id="uploadedAnimations"></div><p id="uploadMessage" role="status"></p>
<p><small>Uploads können heruntergeladen, geteilt und gelöscht werden. Bei den fest eingebauten Effekten entfernt ein fehlendes Häkchen die Animation aus der Automatik.</small></p></article>
<article><h2>Firmware / OTA</h2><p>Neue <code>firmware.bin</code> über den geschützten Updater hochladen.</p><a class="btn" href="/update">OTA-Upload öffnen</a><p><small>Vorher alte Firmware sichern. Update niemals bei instabiler Stromversorgung.</small></p></article>
)HTML"
#ifdef CHAOSUHR_PUBLIC_BUILD
R"HTML(<article><h2>Werkseinstellungen</h2><p>Alle Uhr-Einstellungen, eigene Animationen, der Admin-Zugang und die gespeicherten WLAN-Daten werden gelöscht. Danach beginnt die Ersteinrichtung erneut.</p><label><input id="resetConfirm" type="checkbox" onchange="document.getElementById('resetButton').disabled=!this.checked"> Ich möchte alle Einstellungen, Uploads und Zugangsdaten löschen.</label><button id="resetButton" onclick="factoryReset()" disabled>Auf Werkseinstellungen zurücksetzen</button><p id="resetMessage" role="status"></p></article>)HTML"
#endif
R"HTML(
<script>
const ids=['mode','effect','brightness','speed','duration','nightOn','nightFrom','nightUntil','nightBrightness','corner','vertical','zigzag','autoOrder'];
let automaticChoices=[];
function automaticMask(){return automaticChoices.reduce((mask,choice)=>mask+(choice.input.checked&&!choice.uploaded?2**(choice.id-1):0),0)}
function automaticUploadMask(){return automaticChoices.reduce((mask,choice)=>mask+(choice.input.checked&&choice.uploaded?2**(choice.id-31):0),0)}
function updateAutomaticCount(){document.getElementById('autoCount').textContent=`${automaticChoices.filter(choice=>choice.input.checked).length} von ${automaticChoices.length} ausgewählt`}
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
async function readJson(path){const r=await fetch(path,{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);return r.json()}
async function load(){
  document.getElementById('saveButton').disabled=true;
  try{
    const [d,effects]=await Promise.all([readJson('/api/status'),readJson('/api/effects')]);
    const select=document.getElementById('effect');select.replaceChildren();
    for(const [kind,label] of [['normal','Animationen'],['legacy','Klassiker der ChaosUhr'],['uploaded','Eigene Uploads']]){
      const choices=effects.filter(effect=>(effect.uploaded?'uploaded':effect.legacy?'legacy':'normal')===kind);
      if(!choices.length)continue;
      const group=document.createElement('optgroup');group.label=label;
      for(const effect of choices){
        const option=document.createElement('option');option.value=effect.id;option.textContent=effect.name;group.append(option);
      }select.append(group);
    }
    buildAutomaticChoices(effects,d.autoEffects,d.autoUploads||0);
    document.getElementById('status').textContent=`Firmware ${d.version} · ${d.ip} · WLAN ${d.rssi} dBm · Freier Heap ${d.heap} B · Flash ${d.flash} B · Freier OTA-Platz ${d.ota} B · Uhrzeit ${d.timeValid?'bereit':'wird synchronisiert'}`;
    ids.forEach(k=>{const el=document.getElementById(k);el[el.type==='checkbox'?'checked':'value']=d[k]});
    if(!effects.some(effect=>effect.id===d.effect))document.getElementById('effect').value=1;
    document.getElementById('uploadPanel').hidden=!d.animationUploads;
    if(d.animationUploads)showAnimationStorage(await readJson('/api/animations'));
    bVal.value=d.brightness+' %';sVal.value=d.speed+' %';dVal.value=d.duration+' s';nVal.value=d.nightBrightness+' %';
    document.getElementById('saveButton').disabled=false;
  }catch(e){document.getElementById('status').textContent='Uhr nicht erreichbar: '+e}
}
async function save(){
  try{
    const d={};ids.forEach(k=>{const e=document.getElementById(k);d[k]=e.type==='checkbox'?(e.checked?1:0):Number(e.value)});
    d.autoEffects=automaticMask();
    d.autoUploads=automaticUploadMask();
    if(!d.autoEffects&&!d.autoUploads)throw Error('Bitte mindestens eine Animation für die Automatik auswählen.');
    const r=await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(d)});
    if(!r.ok)throw Error(await r.text());
    msg.textContent='✔ gespeichert';setTimeout(()=>msg.textContent='',3000);
  }catch(e){msg.textContent='Fehler: '+e}
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
  const response=await fetch(path,{method:'POST',headers,body});
  if(!response.ok)throw Error(await response.text());
  return response.json();
}
async function runAnimationChange(action,success){
  if(animationBusy)return;animationBusy=true;
  const message=document.getElementById('uploadMessage');message.textContent='Bitte warten …';
  try{await action();await load();message.textContent=success}catch(error){message.textContent='Fehler: '+error}
  finally{animationBusy=false}
}
async function initializeAnimations(){
  if(!confirm('Animationsspeicher einrichten? Dabei werden vorhandene Dateien in diesem Speicher gelöscht. WLAN, Admin-Zugang und Uhreinstellungen bleiben erhalten.'))return;
  await runAnimationChange(()=>changeAnimation('/api/animations/initialize',new URLSearchParams({confirm:'INITIALIZE'})),'Animationsspeicher ist bereit.');
}
async function uploadAnimation(){
  const file=document.getElementById('animationFile').files?.[0];
  if(!file){document.getElementById('uploadMessage').textContent='Bitte eine .chaos-Datei auswählen.';return}
  if(file.size>24576){document.getElementById('uploadMessage').textContent='Datei darf höchstens 24 KiB groß sein.';return}
  const form=new FormData();form.append('animation',file);
  await runAnimationChange(()=>changeAnimation('/api/animations/upload',form),'Hochgeladen. Jetzt für die Automatik oder als Einzelanimation auswählen und Einstellungen speichern.');
}
async function deleteAnimation(id,name){
  if(!confirm(`Upload „${name}“ wirklich löschen?`))return;
  await runAnimationChange(()=>changeAnimation('/api/animations/delete',new URLSearchParams({id,confirm:'DELETE'})),'Upload gelöscht.');
}
async function factoryReset(){const message=document.getElementById('resetMessage');if(!document.getElementById('resetConfirm').checked){message.textContent='Bitte das Löschen zuerst bestätigen.';return}if(!confirm('Wirklich alle Einstellungen, eigene Animationen, den Admin-Zugang und die WLAN-Daten löschen?'))return;const button=document.getElementById('resetButton');button.disabled=true;try{const {token}=await readJson('/api/session');const response=await fetch('/api/factory-reset',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':token},body:new URLSearchParams({confirm:'RESET'})});if(!response.ok)throw Error(await response.text());message.textContent='Die Uhr wird zurückgesetzt und startet neu. Verbinde dich anschließend mit dem Einrichtungs-WLAN; den neuen Code zeigt die Uhr an.';}catch(error){message.textContent='Fehler: '+error;button.disabled=false}}
load();
</script></body></html>
)HTML";

static bool auth(){
#ifdef CHAOSUHR_PUBLIC_BUILD
  if(!publicAccess.ready()){server.send(503,"text/plain","Bitte zuerst die Uhr einrichten.");return false;}
  if(server.authenticate(OTA_USER,publicAccess.adminPassword())) return true;
#else
  if(server.authenticate(OTA_USER,OTA_PASS)) return true;
#endif
  server.requestAuthentication(); return false;
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
static void statusApi(){
  if(!auth()) return;
  String s="{";
  s+="\"version\":\""+String(VERSION)+"\",\"ip\":\""+WiFi.localIP().toString()+"\",";
  s+="\"rssi\":"+String(WiFi.RSSI())+",\"heap\":"+String(ESP.getFreeHeap());
  s+=",\"flash\":"+String(deviceFlashSize())+",\"ota\":"+String(ESP.getFreeSketchSpace());
  s+=",\"platform\":\""+String(DEVICE_FAMILY)+"\",\"ledPin\":"+String(LED_PIN);
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
  if(server.method()!=HTTP_POST){server.send(405,"text/plain","POST only");return;}
  if(!server.args()){server.send(400,"text/plain; charset=utf-8","Keine Einstellungen angegeben");return;}
  Settings next=cfg;
  for(int i=0;i<server.args();i++){
    const String key=server.argName(i);
    for(int j=0;j<i;j++){
      if(key==server.argName(j)){server.send(400,"text/plain; charset=utf-8","Doppelter Parameter: "+key);return;}
    }
    if(!applySetting(next,key.c_str(),server.arg(i).c_str())){
      server.send(400,"text/plain; charset=utf-8","Unbekannter Parameter oder ungueltiger Wert: "+key);return;
    }
  }
  if(!next.autoEffects && !next.autoUploads){server.send(400,"text/plain; charset=utf-8","Mindestens eine Animation auswaehlen.");return;}
#ifdef CHAOSUHR_HAS_UPLOADS
  if((next.autoUploads&~animations.available()) || (next.effect>EFFECT_COUNT && !animations.playable(next.effect))){
    server.send(400,"text/plain; charset=utf-8","Ausgewaehlter Upload ist nicht verfuegbar. Seite neu laden.");return;
  }
#else
  if(next.autoUploads || next.effect>EFFECT_COUNT){server.send(400,"text/plain","Uploads sind auf diesem Build nicht verfuegbar.");return;}
#endif
  const bool changed=memcmp(&cfg,&next,sizeof cfg)!=0;
  const bool restartAutomatic=cfg.mode!=next.mode || cfg.autoEffects!=next.autoEffects || cfg.autoOrder!=next.autoOrder || cfg.autoUploads!=next.autoUploads;
  if(!persistSettings(EEPROM,cfg,next,settingsPersisted)){
    server.send(500,"text/plain; charset=utf-8","Einstellungen konnten nicht gespeichert werden");return;
  }
  if(changed){
    if(restartAutomatic) autoPlayback.reset();
    scene.reset(millis(),cfg.effect);resetAnimation();
  }
  server.send(200,"application/json","{\"ok\":true}");
}

#ifdef CHAOSUHR_HAS_UPLOADS
static bool animationMutationAllowed(){
  return publicAccess.ready() && server.authenticate(OTA_USER,publicAccess.adminPassword()) && publicAccess.validMutationToken();
}
static void animationStatusApi(){
  if(!auth())return;
  FSInfo info={};if(animations.ready())LittleFS.info(info);
  String result="{\"ready\":"+String(animations.ready()?"true":"false")+",\"used\":"+String(info.usedBytes)+",\"total\":"+String(info.totalBytes)+",\"uploads\":[";
  bool first=true;
  for(uint8_t i=0;i<UPLOAD_SLOTS;++i){
    const auto& slot=animations.slot(i);if(!slot.present)continue;
    if(!first)result+=',';
    first=false;
    result+="{\"id\":"+String(EFFECT_COUNT+1+i)+",\"name\":\""+String(slot.name)+"\",\"valid\":"+String(slot.valid?"true":"false")+",\"frames\":"+String(slot.frames)+"}";
  }
  result+="]}";server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",result);
}
static void animationInitApi(){
  if(!auth())return;
  if(!animationMutationAllowed() || server.args()!=1 || server.arg("confirm")!="INITIALIZE"){
    server.send(403,"text/plain","Einrichtung bestaetigen und Seite neu laden.");return;
  }
  if(!animations.initialize()){server.send(animations.status(),"text/plain",animations.error());return;}
  server.send(200,"application/json","{\"ok\":true}");
}
static void animationUploadDone(){
  if(server.args())animations.abortUpload();
  const uint8_t id=animations.finishUpload(animationMutationAllowed());
  if(!id){server.send(animations.status(),"text/plain; charset=utf-8",animations.error());return;}
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
    server.send(403,"text/plain","Loeschen bestaetigen und Seite neu laden.");return;
  }
  uint8_t id;
  if(!parseSetting(server.arg("id").c_str(),EFFECT_COUNT+1,TOTAL_EFFECT_COUNT,id) || !animations.exists(id)){
    server.send(404,"text/plain","Upload nicht gefunden.");return;
  }
  Settings next=cfg;forgetUploadedAnimation(next,id);
  if(!persistSettings(EEPROM,cfg,next,settingsPersisted)){server.send(500,"text/plain","Einstellungen konnten nicht gespeichert werden; Upload bleibt erhalten.");return;}
  if(!animations.remove(id)){server.send(animations.status(),"text/plain",animations.error());return;}
  autoPlayback.reset();scene.reset(millis(),cfg.effect);resetAnimation();
  server.send(200,"application/json","{\"ok\":true}");
}
static void animationDownloadApi(){
  if(!auth())return;
  uint8_t id;
  if(server.args()!=1 || !parseSetting(server.arg("id").c_str(),EFFECT_COUNT+1,TOTAL_EFFECT_COUNT,id) || !animations.exists(id)){
    server.send(404,"text/plain","Upload nicht gefunden.");return;
  }
  auto file=animations.openDownload(id);if(!file){server.send(500,"text/plain","Lesefehler.");return;}
  server.sendHeader("Content-Disposition","attachment; filename=animation-"+String(id)+".chaos");
  server.sendHeader("Cache-Control","no-store");server.streamFile(file,"text/plain");file.close();
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
    WiFi.persistent(false); WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true);
    if(WiFi.status()!=WL_CONNECTED) WiFi.begin();
    wifiAttemptStarted=now;
  }
  if(WiFi.status()==WL_CONNECTED){
    if(portalActive){wifiManager.stopConfigPortal();WiFi.persistent(false);return;}
    if(!webStarted){
      server.begin(); webStarted=true;
      if(MDNS.begin(HOSTNAME)) MDNS.addService("http","tcp",80);
      Serial.printf("ChaosUhr %s at http://%s/\n",VERSION,WiFi.localIP().toString().c_str());
    }
    wifiAttemptStarted=now;
    server.handleClient();
#if !defined(ESP32)
    MDNS.update();
#endif
  } else {
    if(webStarted){
      server.stop();
#if defined(ESP32)
      MDNS.end();
#else
      MDNS.close();
#endif
      webStarted=false;
    }
    if(!portalActive && now-wifiAttemptStarted>=WIFI_RETRY_MS){
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
  loadSettings();
  FastLED.addLeds<WS2812B,LED_PIN,GRB>(leds,LED_COUNT);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,1500); // estimated software power limit
  clear(); FastLED.show();
#ifdef CHAOSUHR_PUBLIC_BUILD
#ifdef CHAOSUHR_HAS_UPLOADS
  LittleFSConfig filesystemConfig;filesystemConfig.setAutoFormat(false);LittleFS.setConfig(filesystemConfig);
  animations.begin();publicAccess.setResetStorage([](){return animations.factoryReset();});
#endif
  publicAccess.begin();
#endif
  WiFi.persistent(false);
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
    for(const char* path:{"/update","/u"}) wifiManager.server->on(path,HTTP_ANY,[](){
      wifiManager.server->send(403,"text/plain; charset=utf-8","Firmware-Updates nur ueber den angemeldeten Admin-Zugang.");
    });
  });
#endif
  wifiAttemptStarted=millis();
#ifdef CHAOSUHR_PUBLIC_BUILD
  if(publicAccess.ready()){
    WiFi.begin();
    if(WiFi.SSID().isEmpty()) wifiAttemptStarted-=WIFI_RETRY_MS;
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
    if(!auth()) return;
    server.send_P(200,"text/html; charset=utf-8",INDEX_HTML);
  });
  server.on("/api/status",HTTP_GET,statusApi);
  server.on("/api/effects",HTTP_GET,effectsApi);
  server.on("/api/settings",HTTP_POST,saveApi);
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
  if(publicAccess.ready()) updater.setup(&server,"/update",OTA_USER,publicAccess.adminPassword());
  else if(publicAccess.provisioning()) publicAccess.startInitialPortal();
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
  if(scene.due(now,cfg.mode,cfg.duration)) nextScene();
  uint16_t interval=(isLegacyEffect(activeEffect()) || activeEffect()>EFFECT_COUNT)?20:map(cfg.speed,1,100,150,25);
  if(now-lastFrame>=interval){ lastFrame=now; tick++; render(); }
  yield();
}
