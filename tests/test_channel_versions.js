// Execute actual firmware updater sources against supplied canonical manifests. No network/device calls.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const directory=process.argv[2],sources=process.argv.slice(3);
const base='https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/';
function cases(stable){const [major,minor,patch]=stable.split('.').map(Number);const previous=patch>0?`${major}.${minor}.${patch-1}`:minor>0?`${major}.${minor-1}.0`:`${major-1}.0.0`;return [[previous,true],[stable+'-beta.5.3',true],[stable+'-beta.5.2',true],[stable,false],[`${major}.${minor}.${patch+1}-beta.1`,false]];}
(async()=>{
 let checks=0;
 for(const sourceFile of sources){
  const source=fs.readFileSync(sourceFile,'utf8').match(/<script>([\s\S]*?)<\/script>/)[1].replace(/chaosSessionStart\(\);\s*$/,'');
  for(const chip of ['esp8266','esp32']){
  const suffix=chip==='esp32'?'-esp32':'',manifest=JSON.parse(fs.readFileSync(path.join(directory,'stable'+suffix+'.json')));
  for(const [installed,expected] of cases(manifest.version))for(const preview of [false,true]){
   const current={version:installed,platform:chip,flash:4194304,ledPin:chip==='esp32'?32:12};
   const elements={},calls=[];const document={getElementById:id=>elements[id]??={textContent:'',hidden:false,disabled:false,checked:false,addEventListener(){}}};
   document.getElementById('includePreview').checked=preview;
   const context={document,Uint8Array,Blob,URLSearchParams,FormData:class{},confirm:()=>false,fetch:async(url,options)=>{
    calls.push(url);assert.notEqual(url,'/update','No device upload permitted');
    if(url==='/api/status')return {ok:true,json:async()=>current};
    assert(url.startsWith(base));assert.equal(options.credentials,'omit');
    const name=url.slice(base.length),file=path.join(directory,name);
    if(!fs.existsSync(file))return {ok:false,status:404};
    assert(name.endsWith('.json'),'Version check must not download a BIN');
    return {ok:true,status:200,json:async()=>JSON.parse(fs.readFileSync(file))};
   }};
   vm.createContext(context);vm.runInContext(require('./shared_error_script')(),context);vm.runInContext(source,context);
   assert.equal(context.compareUpdateVersions(manifest.version,installed)>0,expected,`${chip}/${installed}`);
   await context.checkOnlineUpdates();assert.equal(!elements.installOnline.hidden,expected,`${sourceFile}/${chip}/${installed}/preview=${preview}`);
   if(expected)assert(elements.onlineMessage.textContent.includes(manifest.version));
   for(const pin of manifest.supported_led_gpio)context.validateUpdateManifest(manifest,{...current,ledPin:pin},preview);
   assert(calls.includes(base+'stable'+suffix+'.json'));checks++;
  }
  }
 }
 console.log('PASS actual released OTA version logic: '+checks+' cases, both boards, five requested versions, Preview on/off, all pins; no upload');
})().catch(error=>{console.error(error);process.exitCode=1});
