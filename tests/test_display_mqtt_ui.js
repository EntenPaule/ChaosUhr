const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm'),path=require('node:path');
const script=fs.readFileSync(path.join(__dirname,'../src/main.cpp'),'utf8').match(/<script>([\s\S]*?)<\/script>/)[1].replace(/load\(\);\s*$/,'');
function app(){
 const elements={},posts=[],blobs=[];let consent=false;
 const element=()=>({type:'number',value:'',checked:false,children:[],hidden:false,textContent:'',append(...items){this.children.push(...items)},replaceChildren(){this.children=[]},click(){},set id(value){this._id=value;elements[value]=this},get id(){return this._id}});
 const document={getElementById:id=>elements[id]??=element(),createElement:element};document.getElementById('nightOn').type='checkbox';
 const status={version:'2.0.10',platform:'esp8266',mode:0,effect:1,brightness:25,speed:55,duration:15,nightOn:1,nightFrom:23,nightUntil:6,nightBrightness:6,corner:1,vertical:0,zigzag:1,autoEffects:3,autoUploads:0,autoOrder:0,ledPin:12,weekdayStyle:0,secondsBar:1,showClock:1,showAnimations:1,showWeather:0,showSensors:0,mqttAvailable:true,animationUploads:true,ledPins:[{gpio:12,label:'D6'},{gpio:4,label:'D2'}]};
 const mqtt={enabled:0,host:'broker.example.test',port:1883,user:'clock',hasPassword:true,discovery:1,discoveryPrefix:'homeassistant',staleSeconds:1800,storageReady:true,sensors:Array.from({length:4},()=>({topic:'',label:'',unit:''}))};
 const context={document,URLSearchParams,FormData:class{},setTimeout:()=>0,confirm:()=>consent,
  Blob:class{constructor(parts){blobs.push(parts.join(''))}},URL:{createObjectURL:()=> 'blob:test',revokeObjectURL(){}},fetch:async(route,options)=>{
   if(options?.method==='POST'){posts.push({route,options});return {ok:true,json:async()=>({ok:true,restarting:route==='/api/settings'&&options.body.get('ledPin')==='4'})};}
   return {ok:true,json:async()=>route==='/api/status'?status:route==='/api/effects'?Array.from({length:30},(_,i)=>({id:i+1,name:'Effekt '+(i+1)})):route==='/api/mqtt'?mqtt:route==='/api/session'?{token:'fixture-session'}:{ready:true,uploads:[]}};
  }};
 for(const name of ['bVal','sVal','dVal','nVal','msg'])context[name]=document.getElementById(name);
 vm.createContext(context);vm.runInContext(require('./shared_error_script')(),context);vm.runInContext(script,context);return {context,elements,posts,blobs,approve(){consent=true}};
}
(async()=>{
 const a=app();await a.context.load();assert.equal(a.elements.mqttPanel.hidden,false);assert(a.elements.mqSensor3topic);
 a.context.selectAutomatic(false);a.elements.showAnimations.checked=false;a.elements.showWeather.checked=true;a.elements.secondsBar.checked=false;a.elements.weekdayStyle.value=1;
 await a.context.save();assert.equal(a.posts.length,1);assert.equal(a.posts[0].options.body.get('showAnimations'),'0');assert.equal(a.posts[0].options.body.get('showWeather'),'1');assert.equal(a.posts[0].options.body.get('secondsBar'),'0');assert.equal(a.posts[0].options.headers['X-CSRF-Token'],'fixture-session');
 a.elements.showClock.checked=false;a.elements.showWeather.checked=false;await a.context.save();assert.equal(a.posts.length,1);assert.match(a.elements.msg.textContent,/Anzeigeart/);
 const pin=app();await pin.context.load();pin.elements.ledPin.value=4;await pin.context.save();assert.equal(pin.posts.length,0);pin.approve();await pin.context.save();assert.equal(pin.posts.length,1);assert.match(pin.elements.msg.textContent,/startet/);
 assert.equal(a.elements.mqPassword.value,'');a.elements.mqEnabled.checked=true;a.elements.mqWeatherTopic.value='house/weather';a.elements.mqSensor0topic.value='house/living';a.elements.mqSensor0label.value='Wohnzimmer';
 await a.context.saveMqtt();const post=a.posts.find(p=>p.route==='/api/mqtt');assert(post);assert.equal(post.options.body.get('weatherTopic'),'house/weather');assert.equal(post.options.body.get('sensor0Topic'),'house/living');assert.equal(post.options.body.get('password'),'');assert.equal(post.options.headers['X-CSRF-Token'],'fixture-session');
 await a.context.exportSettings();const backup=JSON.parse(a.blobs[0]);assert.equal(backup.version,2);assert.equal(backup.settings.secondsBar,1);assert.equal(backup.settings.ledPin,undefined);assert(!a.blobs[0].includes('broker.example.test')&&!/password/i.test(a.blobs[0]));
 console.log('PASS display/MQTT GUI: clock/weather without animations, scene guard, persistent display controls, explicit pin restart, four sensor forms, CSRF and secret/network/pin-free backup v2');
})().catch(e=>{console.error(e);process.exitCode=1});
