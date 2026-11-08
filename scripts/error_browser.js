// Shared by setup, portal, main GUI and OTA. No response-body dumps or secrets.
function chaosFailure(symbol,status=0,definition=null){
 const code=CHAOS_CODES[symbol]||symbol;
 const d=definition||CHAOS_CLIENT_ERRORS[code]||CHAOS_CLIENT_ERRORS[CHAOS_CODES.HTTP_STATUS];
 const e=new Error('Fehler '+code+(status?' (HTTP '+status+')':'')+'\n'+d.message+'\n'+d.details+'\nMaßnahmen: '+d.action);
 e.chaosCode=code;e.httpStatus=status;return e;
}
function chaosMessage(error){return error?.chaosCode?error.message:chaosFailure('SYS_UI').message}
async function chaosFetch(url,options={}){
 try{return await fetch(url,{credentials:'same-origin',...options})}
 catch(error){throw chaosFailure(error?.name==='AbortError'||error?.name==='TimeoutError'?'HTTP_TIMEOUT':'HTTP_NETWORK')}
}
async function chaosCheck(response){
 if(response.ok)return response;
 let data=null;
 try{
  if(typeof response.text==='function'){const text=await response.text();if(text)data=JSON.parse(text)}
  else if(typeof response.json==='function')data=await response.json();
 }catch(_){/* Empty/non-JSON error bodies use status-specific fallbacks. */}
 if(data&&Object.values(CHAOS_CODES).includes(data.code)&&['message','details','action'].every(k=>typeof data[k]==='string'&&data[k].length>0&&data[k].length<=512))
  throw chaosFailure(data.code,response.status,data);
 throw chaosFailure(response.status===401?'AUTH_REQUIRED':response.status===403?'AUTH_FORBIDDEN':response.status===413?'HTTP_BODY_LIMIT':'HTTP_STATUS',response.status);
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
