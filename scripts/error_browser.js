// Shared by setup, portal, main GUI and OTA. No response-body dumps or secrets.
function chaosFailure(symbol,status=0,definition=null){
 const code=CHAOS_CODES[symbol]||symbol;
 const d=definition||CHAOS_CLIENT_ERRORS[code]||CHAOS_CLIENT_ERRORS[CHAOS_CODES.HTTP_STATUS];
 const e=new Error('Fehler '+code+(status?' (HTTP '+status+')':'')+'\n'+d.message+'\n'+d.details+'\nMaßnahmen: '+d.action);
 e.chaosCode=code;e.httpStatus=status;return e;
}
function chaosMessage(error){return error?.chaosCode?error.message:chaosFailure('SYS_UI').message}
async function chaosFetch(url,options={}){
 try{
  const response=await fetch(url,{credentials:'same-origin',...options});
  if(chaosAdmin.started&&chaosAdmin.active&&response.ok&&options.method==='POST'&&!['/api/login','/api/logout','/api/session/activity','/update'].includes(url)){
   try{await chaosSessionRead()}catch(_){/* A successful write is never retried or reported as failed. */}
  }
  return response;
 }
 catch(error){throw chaosFailure(error?.name==='AbortError'||error?.name==='TimeoutError'?'HTTP_TIMEOUT':'HTTP_NETWORK')}
}
async function chaosCheck(response){
 if(response.ok)return response;
 let data=null;
 try{
  if(typeof response.text==='function'){const text=await response.text();if(text)data=JSON.parse(text)}
  else if(typeof response.json==='function')data=await response.json();
 }catch(_){/* Empty/non-JSON error bodies use status-specific fallbacks. */}
 if(response.status===401)chaosSessionLost(data?.code||'AUTH_REQUIRED');
 if(data&&Object.values(CHAOS_CODES).includes(data.code)&&['message','details','action'].every(k=>typeof data[k]==='string'&&data[k].length>0&&data[k].length<=512))
  throw chaosFailure(data.code,response.status,data);
 throw chaosFailure(response.status===401?'AUTH_REQUIRED':response.status===403?'AUTH_FORBIDDEN':response.status===413?'HTTP_BODY_LIMIT':'HTTP_STATUS',response.status);
}

// Shared by the main GUI and OTA. Cookies are HttpOnly; JS only keeps CSRF in
// RAM. No password storage, automatic login or automatic mutation retry.
const chaosAdmin={started:false,active:false,csrf:'',timer:0,lastActivity:0,onReady:null};
function chaosSessionLost(reason=''){
 if(!chaosAdmin.started)return;
 chaosAdmin.active=false;chaosAdmin.csrf='';clearTimeout(chaosAdmin.timer);
 document.getElementById('protectedContent').hidden=true;
 document.getElementById('loginPanel').hidden=false;
 document.getElementById('adminLogout').hidden=true;
 document.getElementById('adminState').textContent='Nicht angemeldet';
 document.getElementById('adminPassword').value='';
 document.getElementById('adminMessage').textContent=reason?chaosFailure(reason).message:'Abgemeldet.';
 for(const id of ['mqPassword','supportDiagnostics']){const element=document.getElementById(id);if(element)element.value=''}
}
function chaosSessionApply(data){
 if(data.authenticated!==true||!/^([a-f0-9]{32})$/.test(data.token)||!Number.isInteger(data.remainingSeconds)||data.remainingSeconds<1||data.remainingSeconds>1800)throw chaosFailure('HTTP_JSON');
 chaosAdmin.active=true;chaosAdmin.csrf=data.token;clearTimeout(chaosAdmin.timer);
 // Another tab may have recorded real activity. Verify the server deadline
 // with a non-renewing GET; the browser clock is not the authority.
 chaosAdmin.timer=setTimeout(async()=>{try{await chaosSessionRead()}catch(error){chaosSessionLost(error.chaosCode||'HTTP_NETWORK')}},data.remainingSeconds*1000);
 document.getElementById('adminState').textContent='Angemeldet als admin';
 document.getElementById('adminLogout').hidden=false;
 document.getElementById('loginPanel').hidden=true;
 document.getElementById('protectedContent').hidden=false;
 document.getElementById('adminMessage').textContent='';
}
async function chaosSessionRead(){
 const data=await chaosJson(await chaosFetch('/api/session',{cache:'no-store'}));
 chaosSessionApply(data);return data;
}
async function chaosLogin(event){
 event.preventDefault();const button=document.getElementById('adminLoginButton');button.disabled=true;
 try{
  const {token}=await chaosJson(await chaosFetch('/api/login',{cache:'no-store'}));
  const body=new URLSearchParams({password:document.getElementById('adminPassword').value});
  const data=await chaosJson(await chaosFetch('/api/login',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':token},body}));
  chaosSessionApply(data);chaosAdmin.lastActivity=Date.now();
  if(chaosAdmin.onReady)await chaosAdmin.onReady();
 }catch(error){document.getElementById('adminMessage').textContent=chaosMessage(error)}
 finally{document.getElementById('adminPassword').value='';button.disabled=false}
}
async function chaosLogout(){
 const button=document.getElementById('adminLogout');button.disabled=true;
 try{
  const {token}=await chaosSessionRead();
  await chaosCheck(await chaosFetch('/api/logout',{method:'POST',headers:{'X-CSRF-Token':token,'Content-Type':'application/x-www-form-urlencoded'},body:''}));
  chaosSessionLost();
 }catch(error){document.getElementById('adminMessage').textContent=chaosMessage(error)}
 finally{button.disabled=false}
}
async function chaosUserActivity(event){
 if(!chaosAdmin.active||!event.isTrusted||!document.getElementById('protectedContent').contains(event.target)||Date.now()-chaosAdmin.lastActivity<15000)return;
 chaosAdmin.lastActivity=Date.now();
 try{
  const data=await chaosJson(await chaosFetch('/api/session/activity',{method:'POST',headers:{'X-CSRF-Token':chaosAdmin.csrf,'Content-Type':'application/x-www-form-urlencoded'},body:''}));
  chaosSessionApply(data);
 }catch(error){document.getElementById('adminMessage').textContent=chaosMessage(error)}
}
async function chaosSessionStart(onReady=null){
 chaosAdmin.started=true;chaosAdmin.onReady=onReady;
 document.getElementById('adminLogin').addEventListener('submit',chaosLogin);
 document.getElementById('adminLogout').addEventListener('click',chaosLogout);
 for(const name of ['pointerdown','keydown','input','change'])document.addEventListener(name,chaosUserActivity,true);
 globalThis.addEventListener?.('pagehide',()=>{chaosAdmin.csrf='';document.getElementById('protectedContent').hidden=true});
 globalThis.addEventListener?.('pageshow',async event=>{if(event.persisted){try{await chaosSessionRead()}catch(error){document.getElementById('adminMessage').textContent=chaosMessage(error)}}});
 try{await chaosSessionRead();if(onReady)await onReady()}
 catch(error){document.getElementById('adminMessage').textContent=chaosMessage(error)}
 // This GET deliberately does not renew the server's idle deadline.
 setInterval(async()=>{if(!chaosAdmin.active)return;try{await chaosSessionRead()}catch(error){document.getElementById('adminMessage').textContent=chaosMessage(error)}},60000);
}
async function chaosJson(response){
 await chaosCheck(response);
 let text;
 try{
  if(typeof response.text!=='function')return await response.json();
  text=await response.text();
 }catch(_){throw chaosFailure('HTTP_BODY_ABORT',response.status)}
 if(!text)throw chaosFailure('HTTP_EMPTY',response.status);
 try{return JSON.parse(text)}catch(_){throw chaosFailure('HTTP_JSON',response.status)}
}
