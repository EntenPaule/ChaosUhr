const assert=require('node:assert/strict'),fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const source=fs.readFileSync(path.join(__dirname,'../include/ota_ui.h'),'utf8').match(/<script>([\s\S]*?)<\/script>/)[1].replace(/chaosSessionStart\(\);\s*$/,'');
function fixture(sessionOk=true,platform='esp8266'){
 const elements={firmware:{files:[{name:'fixture.bin',size:600000}]},upload:{},message:{},otaForm:{}},calls=[];let submit;
 elements.otaForm.addEventListener=(name,handler)=>{assert.equal(name,'submit');submit=handler};
 const context={document:{getElementById:id=>elements[id]},FormData:class{constructor(){this.parts=[]}append(...part){this.parts.push(part)}},fetch:async(route,options)=>{calls.push({route,options});return route==='/api/status'?{ok:true,json:async()=>({platform})}:route==='/api/session'?{ok:sessionOk,json:async()=>({token:'fixture-ota-token'})}:{ok:true};}};
 vm.createContext(context);vm.runInContext(require('./shared_error_script')(),context);vm.runInContext(source,context);return {elements,calls,send:()=>submit({preventDefault(){}})};
}
(async()=>{
 const good=fixture();await good.send();assert.equal(good.calls.length,3);assert.equal(good.calls[2].options.method,'POST');assert.equal(good.calls[2].options.headers['X-CSRF-Token'],'fixture-ota-token');assert.equal(good.calls[2].options.body.parts[0][0],'firmware');assert.match(good.elements.message.textContent,/startet neu/);
 const failed=fixture(false);await failed.send();assert.equal(failed.calls.length,2);assert.equal(failed.elements.upload.disabled,false);
 const large=fixture();large.elements.firmware.files[0].size=1044465;await large.send();assert.equal(large.calls.length,1);
 const esp32=fixture(true,'esp32');esp32.elements.firmware.files[0].size=1600000;await esp32.send();assert.equal(esp32.calls.length,3);
 console.log('PASS OTA GUI: authenticated token acquisition, CSRF multipart firmware field, size limit and recoverable login failure');
})().catch(error=>{console.error(error);process.exitCode=1});
