// Execute the real embedded GUI against independently persisted HTTP states.
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const html=fs.readFileSync(path.join(__dirname,'../src/main.cpp'),'utf8');
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1].replace(/chaosSessionStart\(load\);\s*$/,'');
function fixture(mode='ok'){
 const elements={},posts=[];let continueWrite;
 const saved={brightness:10,nightOn:1,nightFrom:23,nightUntil:6,nightBrightness:5};
 const policy={nightLedsOff:false,mqttBrightness:false};
 const node=()=>({value:'',type:'number',checked:false,disabled:false,textContent:'',addEventListener(){},append(){},replaceChildren(){}});
 const document={getElementById:id=>elements[id]??=node(),createElement:node};
 const response=(status,data)=>({ok:status===200,status,json:async()=>data});
 const context={document,URLSearchParams,setTimeout:()=>0,confirm:()=>true,fetch:async(url,options={})=>{
  if(url==='/api/session')return response(200,{token:'fixture-csrf'});
  if(options.method==='POST'){
   posts.push({url,body:Object.fromEntries(options.body)});assert.equal(options.headers['X-CSRF-Token'],'fixture-csrf');
   if(url==='/api/settings'){
    if(mode==='pending')await new Promise(resolve=>continueWrite=resolve);
    if(mode==='eeprom')return response(500,{code:'CFG-009',message:'Speichern fehlgeschlagen',details:'Fixture write failure',action:'Prüfen'});
    for(const [k,v] of options.body)saved[k]=Number(v);
    if(mode==='lost')throw Error('fixture-private-network-token');
   }else if(url==='/api/brightness'){
    if(mode==='littlefs')return response(507,{code:'FS-007',message:'Speichern fehlgeschlagen',details:'Fixture rename failure',action:'Prüfen'});
    for(const [k,v] of options.body)policy[k]=v==='1';
   }return response(200,{ok:true});
  }
  if(url==='/api/brightness')return response(200,{...policy,source:policy.mqttBrightness?'MQTT':'GUI',percent:policy.mqttBrightness?50:saved.brightness,mqttValue:policy.mqttBrightness?50:null,topic:'fixture/brightness/set',mqttActive:policy.mqttBrightness,connected:true,configError:mode==='littlefs'?'FS-026':''});
  throw Error('Unexpected fixture route '+url);
 }};
 for(const name of ['bVal','sVal','dVal','nVal','msg'])context[name]=document.getElementById(name);
 vm.createContext(context);vm.runInContext(require('./shared_error_script')(),context);vm.runInContext(script,context);
 for(const [k,v] of Object.entries(saved)){document.getElementById(k).value=v;document.getElementById(k).checked=!!v;}
 context.applyBrightnessPolicy({...policy,source:'GUI',percent:10,mqttValue:null});vm.runInContext('settingsLoaded=true;brightnessLoaded=true;settingsButtons()',context);
 elements.brightness.value=30;elements.nightLedsOff.checked=true;elements.mqttBrightness.checked=true;
 return {context,elements,saved,policy,posts,complete:()=>continueWrite()};
}
(async()=>{
 const boxes=[...html.matchAll(/<article\b([^>]*)>([\s\S]*?)<\/article>/g)];
 const brightness=boxes.find(m=>m[1].includes('brightnessPanel'))[2],hardware=boxes.find(m=>m[1].includes('hardwarePanel'))[2];
 for(const id of ['brightness','nightOn','nightFrom','nightUntil','nightBrightness','nightLedsOff','mqttBrightness','brightnessState','brightnessTopic','releaseBrightness']){
  assert(brightness.includes('id="'+id+'"'),id);assert.equal((html.match(new RegExp('id="'+id+'"','g'))||[]).length,1,id);
 }
 assert.equal((brightness.match(/onclick="saveBrightnessPolicy\(\)"/g)||[]).length,1);assert(!brightness.includes('onclick="save()"'));
 for(const id of ['ledPin','corner','vertical','zigzag'])assert(hardware.includes('id="'+id+'"'));
 assert(html.indexOf('id="hardwarePanel"')>html.indexOf('id="mqttPanel"'));
 for(const mode of ['ok','eeprom','littlefs','lost']){
  const a=fixture(mode);assert.match(a.elements.brightnessState.textContent,/GUI.*10 %/);
  a.context.brightnessFormChanged();assert.match(a.elements.brightnessState.textContent,/GUI.*10 %/); // edits never change live source
  await a.context.saveBrightnessPolicy();
  assert.deepEqual(Object.keys(a.posts[0].body).sort(),['brightness','nightOn','nightFrom','nightUntil','nightBrightness'].sort());
  if(mode==='eeprom'||mode==='lost'){
   assert.equal(a.posts.length,1);assert.equal(a.policy.mqttBrightness,false);assert.match(a.elements.brightnessMessage.textContent,/nicht gesendet/);
   assert.equal(a.saved.brightness,mode==='lost'?30:10);
  }else{
   assert.equal(a.posts.length,2);assert.equal(a.saved.brightness,30);
   assert.deepEqual(Object.keys(a.posts[1].body).sort(),['nightLedsOff','mqttBrightness'].sort());
   if(mode==='littlefs'){assert.equal(a.policy.mqttBrightness,false);assert.match(a.elements.brightnessMessage.textContent,/Taghelligkeit und Nachtzeiten gespeichert.*nicht bestätigt/);assert.match(a.elements.brightnessState.textContent,/GUI.*30 %/);}
   else{assert.equal(a.policy.mqttBrightness,true);assert.match(a.elements.brightnessMessage.textContent,/beide Speicherbereiche bestätigt/);assert.match(a.elements.brightnessState.textContent,/MQTT.*50 %/);}
  }
  const actionMessage=a.elements.brightnessMessage.textContent;vm.runInContext('chaosAdmin.active=true',a.context);await a.context.refreshBrightnessStatus();assert.equal(a.elements.brightnessMessage.textContent,actionMessage);
  assert.equal(a.elements.mqttBrightness.checked,true);assert.equal(a.elements.brightness.value,30); // retain drafts after failed save/status refresh
  assert(!/CFG-|FS-|fixture-private|Maßnahmen/.test(a.elements.brightnessMessage.textContent));
 }
 const busy=fixture('pending');const first=busy.context.saveBrightnessPolicy();await new Promise(setImmediate);
 busy.elements.brightness.value=70;await busy.context.saveBrightnessPolicy();assert.equal(busy.posts.length,1);assert.equal(busy.elements.saveBrightnessButton.disabled,true);
 busy.complete();await first;assert.equal(busy.saved.brightness,30);assert.equal(busy.elements.brightness.value,70); // snapshot versus edits during request
 const hardwareOnly=fixture();for(const [id,value] of Object.entries({ledPin:12,corner:1,vertical:0,zigzag:1}))hardwareOnly.elements[id]=Object.assign(hardwareOnly.elements[id]||{}, {value});
 await hardwareOnly.context.saveHardware();assert.deepEqual(Object.keys(hardwareOnly.posts[0].body).sort(),['ledPin','corner','vertical','zigzag'].sort());assert.equal(hardwareOnly.saved.brightness,10);
 const displayOnly=fixture();for(const [id,value] of Object.entries({mode:0,effect:1,speed:55,duration:15,autoOrder:0,weekdayStyle:0}))displayOnly.elements[id]=Object.assign(displayOnly.elements[id]||{}, {value});
 displayOnly.elements.showClock.checked=true;displayOnly.elements.showAnimations.checked=false;
 await displayOnly.context.save();assert.equal(displayOnly.posts.length,1);
 for(const key of ['brightness','nightOn','nightFrom','nightUntil','nightBrightness','ledPin','corner','vertical','zigzag'])assert(!Object.hasOwn(displayOnly.posts[0].body,key),key);
 assert.equal(displayOnly.saved.brightness,10);assert.equal(displayOnly.policy.mqttBrightness,false);
 console.log('PASS settings sections: unique box ownership, field-scoped writes, EEPROM then LittleFS, both partial failures, lost reply uncertainty, stored/live state versus drafts, shared write lock and hardware-only save');
})().catch(error=>{console.error(error);process.exitCode=1});
