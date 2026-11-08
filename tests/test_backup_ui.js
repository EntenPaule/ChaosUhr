const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const root=path.resolve(__dirname,'..');
const script=fs.readFileSync(path.join(root,'src/main.cpp'),'utf8').match(/<script>([\s\S]*?)<\/script>/)[1].replace(/load\(\);\s*$/,'');
const settings={mode:0,effect:38,brightness:25,speed:55,duration:15,nightOn:1,nightFrom:23,nightUntil:6,nightBrightness:6,corner:1,vertical:0,zigzag:1,autoOrder:1,autoEffects:2,autoUploads:128};
const sha='a'.repeat(64),uploads=[{id:38,name:'Herz',valid:true,sha256:sha,frames:2}];
const effects=[...Array.from({length:30},(_,i)=>({id:i+1,name:'Effekt '+(i+1)})),{id:38,name:'Herz',uploaded:true}];
function app(){
 const elements={},posts=[],blobs=[];
 function element(){return {type:'number',value:'',textContent:'',children:[],hidden:false,append(...children){this.children.push(...children)},replaceChildren(){this.children=[]},click(){this.clicked=true}}}
 const document={getElementById(id){return elements[id]??=element()},createElement:element};document.getElementById('nightOn').type='checkbox';
 document.getElementById('settingsFile');
 const context={document,URLSearchParams,FormData:class{},setTimeout:()=>0,confirm:()=>false,
  Blob:class{constructor(parts){blobs.push(parts.join(''))}},URL:{createObjectURL:()=> 'blob:fixture',revokeObjectURL:()=>{}},
  fetch:async(route,options)=>{
   if(route==='/api/session')return {ok:true,json:async()=>({token:'fixture-session'})};
   if(route==='/api/settings'){posts.push(options);return {ok:true};}
   return {ok:true,json:async()=>route==='/api/status'?{...settings,animationUploads:true,version:'2.0.8',ip:'127.0.0.1',adminPassword:'fixture-never-export-this',ssid:'fixture-private-wifi'}:route==='/api/effects'?effects:{ready:true,uploads}};
  }};
 for(const name of ['bVal','sVal','dVal','nVal','msg'])context[name]=document.getElementById(name);
 vm.createContext(context);vm.runInContext(require('./shared_error_script')(),context);vm.runInContext(script,context);return {context,elements,posts,blobs};
}
function backup(){return {format:'chaosuhr-settings',version:1,firmware:'2.0.8',settings:{...settings,effect:31,autoUploads:1},uploads:[{id:31,sha256:sha}]}}
(async()=>{
 const a=app();await a.context.load();await a.context.exportSettings();
 const saved=JSON.parse(a.blobs[0]);assert.equal(saved.settings.brightness,25);assert.equal(saved.uploads[0].sha256,sha);
 assert.deepEqual(Object.keys(saved).sort(),['firmware','format','settings','uploads','version']);
 assert(!a.blobs[0].includes('fixture-never-export-this')&&!a.blobs[0].includes('fixture-private-wifi')&&!a.blobs[0].includes('127.0.0.1'));
 const file=text=>({size:text.length,text:async()=>text});
 a.elements.settingsFile.files=[file(JSON.stringify(backup()))];await a.context.previewSettingsImport();
 assert.equal(a.posts.length,0);assert.equal(a.elements.applyBackup.hidden,false);assert.match(a.elements.backupPreview.textContent,/noch nicht gespeichert/);
 a.context.applySettingsImport();assert.equal(a.posts.length,0);assert.equal(a.elements.effect.value,38);await a.context.save();
 assert.equal(a.posts.length,1);assert.equal(a.posts[0].headers['X-CSRF-Token'],'fixture-session');assert.equal(a.posts[0].body.get('autoUploads'),'128');assert.equal(a.posts[0].body.get('effect'),'38');
 const invalid=[];
 let b=backup();b.settings.brightness=101;invalid.push(b);
 b=backup();b.settings.brightness='25';invalid.push(b);
 b=backup();b.settings.password='fixture';invalid.push(b);
 b=backup();b.password='fixture';invalid.push(b);
 b=backup();b.version=2;invalid.push(b);
 b=backup();b.uploads[0].sha256='b'.repeat(64);invalid.push(b);
 b=backup();b.uploads.push({...b.uploads[0]});invalid.push(b);
 invalid.push(JSON.parse('{"__proto__":{"polluted":true}}'));
 for(const bad of invalid){a.elements.settingsFile.files=[file(JSON.stringify(bad))];await a.context.previewSettingsImport();assert.equal(a.elements.applyBackup.hidden,true);assert.match(a.elements.backupMessage.textContent,/Import abgelehnt/);}
 a.elements.settingsFile.files=[{size:8193,text:async()=>{throw Error('must not read oversized file')}}];await a.context.previewSettingsImport();assert.match(a.elements.backupMessage.textContent,/8 KiB/);
 assert.equal(a.posts.length,1);assert.equal({}.polluted,undefined);
 console.log('PASS settings backup GUI: secret-free export, preview without writes, explicit apply/save with CSRF, hash-based upload remapping and invalid/missing/oversized backup rejection');
})().catch(error=>{console.error(error);process.exitCode=1});
