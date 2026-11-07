const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const header=fs.readFileSync(path.join(__dirname,'../include/public_ui.h'),'utf8');
const script=header.match(/<script>([\s\S]*?)<\/script>/)[1];
function fixture(failSave=false){
  const elements={};let posted,calls=0;
  const document={getElementById(id){return elements[id]??={value:'',textContent:'',disabled:false,addEventListener(){}}}};
  const context={document,URLSearchParams,fetch:async(route,options)=>{
    ++calls;
    if(route==='/api/session')return {ok:true,json:async()=>({token:'fixture-csrf-token'})};
    assert.equal(route,'/api/setup');posted=options;
    return {ok:!failSave,text:async()=>'Speicherfehler'};
  }};
  vm.createContext(context);vm.runInContext(script,context);
  return {context,elements,form:document.getElementById('password'),repeat:document.getElementById('repeat'),get calls(){return calls},get posted(){return posted}};
}
(async()=>{
  const app=fixture();
  for(const password of ['short','passphrase-mit-Umlaut-ä']){
    app.form.value=app.repeat.value=password;await app.context.saveAdmin({preventDefault(){}});
    assert.equal(app.calls,0);
  }
  app.form.value='fixture-admin-password-42';app.repeat.value='different';
  await app.context.saveAdmin({preventDefault(){}});assert.equal(app.calls,0);
  app.repeat.value=app.form.value;await app.context.saveAdmin({preventDefault(){}});
  assert.equal(app.posted.method,'POST');assert.equal(app.posted.body.get('password'),'fixture-admin-password-42');
  assert.equal(app.posted.headers['X-CSRF-Token'],'fixture-csrf-token');
  assert.equal(app.form.value,'');assert.equal(app.repeat.value,'');
  assert.match(app.elements.setupMessage.textContent,/startet neu/);
  const fail=fixture(true);fail.form.value=fail.repeat.value='fixture-admin-password-42';
  await fail.context.saveAdmin({preventDefault(){}});
  assert.match(fail.elements.setupMessage.textContent,/Speicherfehler/);
  assert.equal(fail.elements.setupSubmit.disabled,false);
  console.log('PASS setup GUI: new password/confirmation, input validation, CSRF request, no echoed password, save-error recovery');
})().catch(error=>{console.error(error);process.exitCode=1});
