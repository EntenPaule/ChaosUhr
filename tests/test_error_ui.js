const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const shared=require('./shared_error_script')();
const main=fs.readFileSync(path.join(__dirname,'../src/main.cpp'),'utf8');
const mqtt=main.slice(main.indexOf('const mqttFields='),main.indexOf('async function exportSettings()'));
function fixture(mode){
 const elements={},posts=[];
 const document={getElementById(id){return elements[id]??={value:'',checked:false,textContent:''}}};
 const context={document,URLSearchParams,readJson:async()=>({token:'fixture-session-private'}),fetch:async(url,options)=>{
  posts.push({url,options});
  if(mode==='network')throw Error('fixture-secret-in-raw-error');
  if(mode==='timeout'){const e=new Error();e.name='AbortError';throw e;}
  const status=mode==='fs'?507:mode==='csrf'?403:401;
  const body=mode==='fs'?JSON.stringify({code:'FS-007',message:'Atomarer Dateiaustausch fehlgeschlagen',details:'Rename fehlgeschlagen',action:'Dateisystem prüfen'}):mode==='csrf'?JSON.stringify({code:'AUTH-003',message:'Sitzung ungültig',details:'Token abgelaufen',action:'Seite neu laden'}):'';
  return {ok:false,status,text:async()=>body};
 }};
 vm.createContext(context);vm.runInContext(shared,context);vm.runInContext(mqtt,context);
 document.getElementById('mqHost').value='fixture-broker';document.getElementById('mqEnabled').checked=true;
 document.getElementById('mqPassword').value='fixture-mqtt-private-password';
 return {context,elements,posts};
}
const response=(status,body)=>({ok:status>=200&&status<300,status,text:async()=>body});
(async()=>{
 for(const [mode,code] of [['empty401','AUTH-001'],['csrf','AUTH-003'],['fs','FS-007'],['network','HTTP-007'],['timeout','HTTP-008']]){
  const a=fixture(mode);await a.context.saveMqtt();
  const text=a.elements.mqttMessage.textContent;assert(text.includes(code));assert(/Maßnahmen/.test(text));
  if(mode==='empty401')assert(text.includes('HTTP 401'));
  assert(!/fixture-.*private|fixture-secret/.test(text));assert.equal(a.posts.length,1);
 }
 const a=fixture('empty401');
 for(const [r,code] of [[response(403,''),'AUTH-010'],[response(200,''),'HTTP-009'],[response(200,'{bad'),'HTTP-010'],[response(500,'fixture-private-secret'),'HTTP-011']]){
  await assert.rejects(a.context.chaosJson(r),e=>e.message.includes(code)&&!e.message.includes('fixture-private-secret'));
 }
 await assert.rejects(a.context.chaosJson({ok:true,status:200,text:async()=>{throw Error('fixture-private-secret')}}),e=>e.message.includes('HTTP-012')&&!e.message.includes('fixture-private-secret'));
 // A failed shared script still produces a coded message without any helper loaded.
 const attribute=main.match(/src="\/errors\.js[^\"]*" onerror="([^\"]*)"/)[1];
 const handler=attribute.replaceAll('&quot;','"').replaceAll('&#x27;',"'").replaceAll('&amp;','&').replaceAll('&lt;','<').replaceAll('&gt;','>');
 let loaded;const bootstrap={document:{body:{textContent:''},addEventListener(_,callback){loaded=callback}}};vm.createContext(bootstrap);vm.runInContext(handler,bootstrap);loaded();assert(bootstrap.document.body.textContent.includes('HTTP-018'));
 console.log('PASS error GUI: actual MQTT form empty 401/session/FS errors, timeout/connection/body abort, empty/malformed JSON, no raw secret echoes or automatic POST retries, coded script-load fallback');
})().catch(e=>{console.error(e);process.exitCode=1});
