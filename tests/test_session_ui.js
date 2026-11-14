const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const shared=require('./shared_error_script')();
function fixture(){
 let now=0,authenticated=false,expired=false,invalidSession=false,unknownCode=false,failLogout=false,logins=0,ready=0,timerId=0;
 const elements={},calls=[],timers=new Map(),intervals=[],listeners={};
 const node=()=>({value:'',hidden:false,disabled:false,textContent:'',addEventListener(){},contains:target=>!!target?.protected});
 const document={getElementById:id=>elements[id]??=node(),addEventListener:(name,fn)=>listeners[name]=fn};
 const response=(status,data)=>({status,ok:status>=200&&status<300,text:async()=>JSON.stringify(data)});
 const fail=code=>response(401,{code:unknownCode?'fixture-private-error-token':code,message:'Anmeldung erforderlich',details:'Die Sitzung ist nicht gültig.',action:'Erneut anmelden.'});
 const session=()=>({authenticated:true,token:(logins%2?'a':'b').repeat(32),remainingSeconds:1800,idleTimeoutSeconds:1800});
 const context={document,URLSearchParams,Date:{now:()=>now},setTimeout:(fn,ms)=>{timers.set(++timerId,{fn,ms});return timerId},clearTimeout:id=>timers.delete(id),setInterval:fn=>intervals.push(fn),fetch:async(url,options)=>{
  calls.push({url,options});
  assert(!options.headers?.Authorization&&!options.headers?.Cookie);
  if(url==='/api/login'&&options.method==='POST'){
   assert.equal(options.headers['X-CSRF-Token'],'c'.repeat(32));
   assert.equal(options.body.get('password'),'fixture-private-admin-password');
  authenticated=true;expired=false;invalidSession=false;unknownCode=false;++logins;return response(200,session());
  }
  if(url==='/api/login')return response(200,{token:'c'.repeat(32)});
  if(!authenticated)return fail(expired?'AUTH-020':invalidSession?'AUTH-021':'AUTH-001');
  if(url==='/api/logout'){
   if(failLogout)throw Error('fixture-private-error-password');
   authenticated=false;return response(200,{ok:true});
  }
  if(url==='/api/session'||url==='/api/session/activity')return response(200,session());
  return response(200,{ok:true});
 }};
 vm.createContext(context);vm.runInContext(shared,context);
 return {context,elements,calls,timers,intervals,advance(ms){now+=ms},expire(){authenticated=false;expired=true},invalid(){authenticated=false;invalidSession=true},unknown401(){authenticated=false;unknownCode=true},failLogout(value){failLogout=value},onReady(){++ready},ready:()=>ready,
  active:()=>vm.runInContext('chaosAdmin.active',context),csrf:()=>vm.runInContext('chaosAdmin.csrf',context)};
}
(async()=>{
 for(const name of ['src/main.cpp','include/ota_ui.h']){
  const html=fs.readFileSync(path.join(__dirname,'..',name),'utf8');
  for(const id of ['adminState','adminLogout','adminLogin','adminPassword','protectedContent'])assert(html.includes('id="'+id+'"'));
 }
 for(let reload=0;reload<3;++reload){const fresh=fixture();if(reload===1)fresh.expire();if(reload===2)fresh.invalid();await fresh.context.chaosSessionStart();assert.equal(fresh.elements.adminMessage.textContent,'');assert.equal(vm.runInContext('chaosBrowserErrors.length',fresh.context),0);assert(!fresh.active());}
 const a=fixture();await a.context.chaosSessionStart(a.onReady);
 assert(!a.active()&&a.elements.protectedContent.hidden);assert.equal(a.elements.adminMessage.textContent,'');assert.equal(vm.runInContext('chaosBrowserErrors.length',a.context),0);assert.equal(a.calls.filter(c=>c.options.method==='POST').length,0);
 a.elements.adminPassword.value='fixture-private-admin-password';await a.context.chaosLogin({preventDefault(){}});
 assert(a.active()&&!a.elements.protectedContent.hidden&&a.elements.loginPanel.hidden);assert.match(a.elements.adminState.textContent,/admin/);
 assert.equal(a.ready(),1);assert.equal(a.elements.adminPassword.value,'');
 for(let i=0;i<3;++i){a.advance(60000);await a.intervals[0]()}
 assert.equal(a.calls.filter(c=>c.options.method==='POST').length,1); // status GET only
 await a.context.chaosUserActivity({isTrusted:false,target:{protected:true}});
 await a.context.chaosUserActivity({isTrusted:true,target:{}});
 assert.equal(a.calls.filter(c=>c.url==='/api/session/activity').length,0);
 await a.context.chaosUserActivity({isTrusted:true,target:{protected:true}});
 assert.equal(a.calls.filter(c=>c.url==='/api/session/activity').length,1);
 const before=a.calls.length;await a.context.chaosFetch('/api/mqtt',{method:'POST',headers:{'X-CSRF-Token':a.csrf()},body:'test=1'});
 assert.deepEqual(a.calls.slice(before).map(c=>c.url),['/api/mqtt','/api/session']); // reflect renewed server deadline, no second write
 a.failLogout(true);await a.context.chaosLogout();assert(a.active());assert.equal(a.elements.adminMessage.textContent,'Aktion fehlgeschlagen. Details unter Diagnose.');assert.match(a.elements.browserDiagnostics.textContent,/HTTP-007/);assert(!a.elements.adminMessage.textContent.includes('fixture-private'));
 a.failLogout(false);await a.context.chaosLogout();assert(!a.active()&&a.elements.protectedContent.hidden);assert.equal(a.csrf(),'');
 const loginCount=a.calls.filter(c=>c.url==='/api/login'&&c.options.method==='POST').length;
 await assert.rejects(a.context.chaosSessionRead());assert.equal(a.calls.filter(c=>c.url==='/api/login'&&c.options.method==='POST').length,loginCount);
 a.elements.adminPassword.value='fixture-private-admin-password';await a.context.chaosLogin({preventDefault(){}});assert(a.active());
 a.context.chaosMessage(a.context.chaosFailure('AUTH_REJECTED'));a.context.chaosMessage(a.context.chaosFailure('FS_MOUNT'));a.expire();await a.intervals[0]();assert(!a.active());assert.equal(a.elements.adminMessage.textContent,'Bitte erneut anmelden.');assert.match(a.elements.browserDiagnostics.textContent,/AUTH-020/);
 a.elements.adminPassword.value='fixture-private-admin-password';await a.context.chaosLogin({preventDefault(){}});assert(a.active());assert(!a.elements.browserDiagnostics.textContent.includes('AUTH-020'));assert(a.elements.browserDiagnostics.textContent.includes('AUTH-002'));assert(a.elements.browserDiagnostics.textContent.includes('FS-001'));
 const posts=a.calls.filter(c=>c.options.method==='POST').length;
 await [...a.timers.values()].find(t=>t.ms===1800000).fn();assert(a.active()); // another tab's server deadline wins
 a.expire();await [...a.timers.values()].find(t=>t.ms===1800000).fn();assert(!a.active()&&a.elements.protectedContent.hidden);
 assert.equal(a.calls.filter(c=>c.options.method==='POST').length,posts); // idle timer sends no renewal
 a.elements.adminPassword.value='fixture-private-admin-password';await a.context.chaosLogin({preventDefault(){}});a.invalid();await a.intervals[0]();assert(!a.active());assert.equal(a.elements.adminMessage.textContent,'Bitte erneut anmelden.');assert.match(a.elements.browserDiagnostics.textContent,/AUTH-021/);a.elements.adminPassword.value='fixture-private-admin-password';await a.context.chaosLogin({preventDefault(){}});assert(!a.elements.browserDiagnostics.textContent.includes('AUTH-021'));
 a.unknown401();await assert.rejects(a.context.chaosSessionRead());
 assert(!Object.values(a.elements).some(e=>(e.textContent||'').includes('fixture-private')));
 console.log('PASS session GUI: visible login/logout on main and OTA, explicit relogin, server/idle expiry, no poll/synthetic-event renewal, real activity only, write deadline refresh and logout failure without fake success or secret echoes');
})().catch(error=>{console.error(error);process.exitCode=1});
