const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const source=fs.readFileSync(path.join(__dirname,'../include/ota_ui.h'),'utf8').match(/<script>([\s\S]*?)<\/script>/)[1];
const base='https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/';
function manifest(version='2.0.9',prerelease=false){return {format:'chaosuhr-update',schema:1,version,prerelease,board:'d1_mini',chip:'esp8266',flash_bytes:4194304,led_gpio:12,binary:'chaosuhr-'+version+'-esp8266.bin',bytes:5000,sha256:'a'.repeat(64)}}
function fixture(stable=manifest(),preview=null,current={version:'2.0.8',platform:'esp8266',flash:4194304,ledPin:12}){
 const elements={},calls=[];let accepted=false,downloadSize=5000;
 const document={getElementById(id){return elements[id]??={checked:false,hidden:false,disabled:false,textContent:'',addEventListener(){}}}};
 const context={document,Uint8Array,Blob,confirm:()=>accepted,FormData:class{constructor(){this.parts=[]}append(...part){this.parts.push(part)}},fetch:async(url,options)=>{
  calls.push({url,options});
  if(url==='/api/status')return {ok:true,json:async()=>current};
  if(url==='/api/session')return {ok:true,json:async()=>({token:'fixture-online-token'})};
  if(url==='/update')return {ok:true};
  if(url.endsWith('.json')){const data=/stable(?:-esp32)?\.json$/.test(url)?stable:preview;return {ok:!!data,status:data?200:404,json:async()=>data};}
  const data=new Uint8Array(downloadSize);data[0]=233;return {ok:true,headers:{get:()=>String(downloadSize)},arrayBuffer:async()=>data.buffer};
 }};
 vm.createContext(context);vm.runInContext(require('./shared_error_script')(),context);vm.runInContext(source,context);
 return {context,elements,calls,accept(){accepted=true},truncate(){downloadSize=4999}};
}
(async()=>{
 const a=fixture();assert(a.context.compareUpdateVersions('2.0.10','2.0.9')>0);assert(a.context.compareUpdateVersions('2.0.9-beta.2','2.0.9-beta.10')<0);assert(a.context.compareUpdateVersions('2.0.9','2.0.9-rc.1')>0);
 await a.context.checkOnlineUpdates();assert.equal(a.elements.installOnline.hidden,false);assert(!a.calls.some(call=>call.url==='/update'));
 await a.context.installOnlineUpdate();assert(!a.calls.some(call=>call.url.endsWith('.bin')));a.accept();await a.context.installOnlineUpdate();
 const post=a.calls.find(call=>call.url==='/update');assert(post);assert.equal(post.options.headers['X-CSRF-Token'],'fixture-online-token');assert.equal(post.options.headers['X-Firmware-SHA256'],'a'.repeat(64));assert.equal(post.options.body.parts[0][2],'chaosuhr-2.0.9-esp8266.bin');
 for(const call of a.calls.filter(call=>call.url.startsWith('https://'))){assert(call.url.startsWith(base));assert.equal(call.options.credentials,'omit');assert.equal(call.options.referrerPolicy,'no-referrer');assert.equal(call.options.headers,undefined);}
 const older=fixture(manifest('2.0.7'));await older.context.checkOnlineUpdates();assert.equal(older.elements.installOnline.hidden,true);
 const missing=fixture(null);await missing.context.checkOnlineUpdates();assert.match(missing.elements.onlineMessage.textContent,/noch keine Version/);
 const beta=fixture(null,manifest('2.0.10',true));beta.context.document.getElementById('includePreview').checked=true;await beta.context.checkOnlineUpdates();assert.equal(beta.elements.installOnline.hidden,false);
 for(const change of [{chip:'esp32'},{led_gpio:32},{binary:'../private.bin'},{sha256:'bad'},{bytes:2000000},{schema:2},{prerelease:true}]){
  const bad=fixture({...manifest(),...change});await bad.context.checkOnlineUpdates();assert.equal(bad.elements.installOnline.hidden,true);assert.match(bad.elements.onlineMessage.textContent,/fehlgeschlagen/);
 }
 const truncated=fixture();await truncated.context.checkOnlineUpdates();truncated.accept();truncated.truncate();await truncated.context.installOnlineUpdate();assert(!truncated.calls.some(call=>call.url==='/update'));assert.match(truncated.elements.onlineMessage.textContent,/abgebrochen/);
 const configurable={...manifest('2.0.11'),configurable_led_pin:true,supported_led_gpio:[4,5,12,13,14]};
 const changedPin=fixture(configurable,null,{version:'2.0.10',platform:'esp8266',flash:4194304,ledPin:4});await changedPin.context.checkOnlineUpdates();assert.equal(changedPin.elements.installOnline.hidden,false);
 const chip32={...manifest('2.0.11'),board:'esp32dev',chip:'esp32',led_gpio:32,binary:'chaosuhr-2.0.11-esp32-ota.bin'};
 const esp32=fixture(chip32,null,{version:'2.0.10',platform:'esp32',flash:4194304,ledPin:32});await esp32.context.checkOnlineUpdates();assert.equal(esp32.elements.installOnline.hidden,false);assert(esp32.calls.some(c=>c.url.endsWith('stable-esp32.json')));
 console.log('PASS GitHub update GUI: semantic versions, explicit installation consent, stable/preview channels, hardware/schema/digest/size checks, no external credentials, SHA forwarding, missing channel and truncated downloads');
})().catch(error=>{console.error(error);process.exitCode=1});
