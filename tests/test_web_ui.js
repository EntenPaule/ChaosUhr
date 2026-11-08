// Execute the embedded UI with local DOM/fetch doubles. No network access.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const project = path.resolve(__dirname,'..');
const source = fs.readFileSync(path.join(project,'src/main.cpp'),'utf8');
const script = source.match(/<script>([\s\S]*?)<\/script>/)[1].replace(/chaosSessionStart\(load\);\s*$/,'');
const catalog = fs.readFileSync(path.join(project,'include/effect_catalog.h'),'utf8');
const names = [...catalog.match(/EFFECT_NAMES\[\]\s*=\s*\{([\s\S]*?)\}/)[1].matchAll(/"([^"]+)"/g)].map(match=>match[1]);
const effects = names.map((name,index)=>({id:index+1,name,legacy:index>=16}));
const settings = {
  version:'test',ip:'127.0.0.1',rssi:-40,heap:42000,flash:4194304,ota:2700000,timeValid:true,
  mode:2,effect:30,brightness:25,speed:55,duration:15,nightOn:1,nightFrom:23,nightUntil:6,
  nightBrightness:6,corner:1,vertical:0,zigzag:1,
  autoEffects:2+2**16+2**29,autoOrder:1
};
function environment(failCatalog=false,uploads=false){
  const elements={};
  function element(){return {type:'number',value:'',disabled:false,textContent:'',children:[],append(...children){this.children.push(...children)},replaceChildren(){this.children=[]}}}
  const document={getElementById(id){return elements[id]??=(element())},createElement:element};
  document.getElementById('nightOn').type='checkbox';
  document.getElementById('saveButton').disabled=true;
  let posted, resetPosted, posts=0, failSave=false, approved=false,animationPosts=[];
  class FormData {constructor(){this.values=new Map()}append(key,value){this.values.set(key,value)}get(key){return this.values.get(key)}}
  const uploadedEffects=[{id:31,name:'Eigener Stern',uploaded:true,legacy:false},{id:38,name:'Herz',uploaded:true,legacy:false}];
  const context={document,URLSearchParams,FormData,setTimeout:()=>0,confirm:()=>approved,fetch:async(route,options)=>{
    if(route==='/api/session')return {ok:true,json:async()=>({token:'test-session-token'})};
    if(route==='/api/factory-reset'){resetPosted=options;return {ok:true};}
    if(route==='/api/animations')return {ok:true,json:async()=>({ready:true,used:1024,total:1048576,uploads:uploadedEffects.map(effect=>({...effect,valid:true,frames:2}))})};
    if(route.startsWith('/api/animations/')){animationPosts.push({route,options});return {ok:true,json:async()=>({ok:true,id:31})};}
    if(route==='/api/settings'){
      posted=options;++posts;
      return {ok:!failSave,text:async()=>'Speicherfehler'};
    }
    return {ok:!(failCatalog && route==='/api/effects'),status:500,json:async()=>route==='/api/effects'?(uploads?[...effects,...uploadedEffects]:effects):(uploads?{...settings,animationUploads:true,autoEffects:0,autoUploads:129}:settings)};
  }};
  for(const id of ['bVal','sVal','dVal','nVal','msg'])context[id]=document.getElementById(id);
  vm.createContext(context);vm.runInContext(require('./shared_error_script')(),context);vm.runInContext(script,context);
  return {context,elements,animationPosts,get posted(){return posted},get posts(){return posts},get resetPosted(){return resetPosted},approve(){approved=true},failSave(){failSave=true}};
}
(async()=>{
  const app=environment();
  await app.context.load();
  const groups=app.elements.effect.children;
  assert.equal(groups.length,2);
  assert.equal(groups[0].children.length,16);
  assert.equal(groups[1].children.length,14);
  assert.equal(app.elements.effect.value,30);
  assert.equal(app.elements.saveButton.disabled,false);
  await app.context.save();
  assert.equal(app.posted.method,'POST');
  assert.equal(app.posted.body.get('effect'),'30');
  assert.equal(app.posted.body.get('autoEffects'),String(settings.autoEffects));
  assert.equal(app.posted.body.get('autoOrder'),'1');
  assert.equal(app.elements.autoEffects.children.length,30);
  assert.match(app.elements.autoCount.textContent,/3 von 30/);
  app.context.selectAutomatic(false);
  await app.context.save();assert.equal(app.posts,1);
  assert.match(app.elements.msg.textContent,/CFG-006/);
  app.context.selectAutomatic(true);app.elements.autoOrder.value=0;
  await app.context.save();assert.equal(app.posted.body.get('autoEffects'),String(2**30-1));
  assert.equal(app.posted.body.get('autoOrder'),'0');
  app.context.selectAutomatic(false);
  app.elements.autoEffects.children[29].children[0].checked=true;
  app.elements.autoEffects.children[29].children[0].onchange();
  await app.context.save();assert.equal(app.posted.body.get('autoEffects'),String(2**29));
  assert.match(app.elements.autoCount.textContent,/1 von 30/);
  assert.match(app.elements.msg.textContent,/gespeichert/);
  app.failSave();await app.context.save();
  assert.match(app.elements.msg.textContent,/HTTP-011/);
  const failed=environment(true);await failed.context.load();
  assert.equal(failed.elements.saveButton.disabled,true);
  assert.match(failed.elements.status.textContent,/HTTP 500/);
  await app.context.factoryReset();assert.equal(app.resetPosted,undefined);
  app.elements.resetConfirm.checked=true;
  await app.context.factoryReset();assert.equal(app.resetPosted,undefined);
  app.approve();await app.context.factoryReset();
  assert.equal(app.resetPosted.body.get('confirm'),'RESET');
  assert.equal(app.resetPosted.headers['X-CSRF-Token'],'test-session-token');
  assert.match(app.elements.resetMessage.textContent,/zurückgesetzt/);
  const custom=environment(false,true);await custom.context.load();
  assert.equal(custom.elements.uploadPanel.hidden,false);
  assert.equal(custom.elements.effect.children.length,3);
  assert.equal(custom.elements.autoEffects.children.length,32);
  assert.equal(custom.elements.uploadedAnimations.children.length,2);
  await custom.context.save();assert.equal(custom.posted.body.get('autoEffects'),'0');
  assert.equal(custom.posted.body.get('autoUploads'),'129');
  await custom.context.uploadAnimation();assert.equal(custom.animationPosts.length,0);
  custom.elements.animationFile.files=[{name:'large.chaos',size:24577}];
  await custom.context.uploadAnimation();assert.equal(custom.animationPosts.length,0);
  custom.elements.animationFile.files=[{name:'herz.chaos',size:700}];
  await custom.context.uploadAnimation();assert.equal(custom.animationPosts.length,1);
  const request=custom.animationPosts[0];assert.equal(request.route,'/api/animations/upload');
  assert.equal(request.options.headers['X-CSRF-Token'],'test-session-token');
  assert.equal(request.options.headers['Content-Type'],undefined);
  assert.equal(request.options.body.get('animation').name,'herz.chaos');
  await custom.context.deleteAnimation(31,'Eigener Stern');assert.equal(custom.animationPosts.length,1);
  custom.approve();await custom.context.deleteAnimation(31,'Eigener Stern');
  assert.equal(custom.animationPosts[1].options.body.get('confirm'),'DELETE');
  assert.equal(custom.animationPosts[1].options.headers['X-CSRF-Token'],'test-session-token');
  await custom.context.initializeAnimations();assert.equal(custom.animationPosts[2].options.body.get('confirm'),'INITIALIZE');
  console.log('PASS Web UI: all 30 choices, classic group, saved effect 30, API failure handling');
  console.log('PASS factory-reset GUI: checkbox, confirmation dialog and CSRF-protected POST');
  console.log('PASS automatic GUI: sparse saved selection, 30 checkboxes, all/none, empty-save rejection, ID 30, sequential/random order');
  console.log('PASS upload GUI: uploaded catalog/checkboxes, uploads-only selection, file size, multipart/CSRF, download links and confirmed delete/initialize');
})().catch(error=>{console.error(error);process.exitCode=1});
