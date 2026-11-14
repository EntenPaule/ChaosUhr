const assert=require('node:assert/strict');
const app=require('./test_display_mqtt_ui');
(async()=>{
 const a=app();await a.context.load();
 a.context.applyBrightnessPolicy({nightLedsOff:true,mqttBrightness:true,source:'Nachtabschaltung',percent:0,mqttValue:50,mqttActive:true,connected:true,topic:'fixture/brightness/set'});
 assert.equal(a.elements.nightBrightness.disabled,true);assert.match(a.elements.brightnessState.textContent,/Nachtabschaltung.*0 %.*50/);assert.match(a.elements.brightnessTopic.textContent,/fixture\/brightness\/set/);
 const n=a.posts.length;await a.context.saveBrightnessPolicy();assert.equal(a.posts.length,n+2);assert.equal(a.posts[n].route,'/api/settings');
 const post=a.posts.at(-1);assert.equal(post.route,'/api/brightness');assert.equal(post.options.body.get('nightLedsOff'),'1');assert.equal(post.options.body.get('mqttBrightness'),'1');assert.equal(post.options.headers['X-CSRF-Token'],'fixture-session');
 await a.context.releaseBrightness();assert.equal(a.posts.at(-1).route,'/api/brightness/release');assert.match(a.elements.brightnessMessage.textContent,/noch nicht bestätigt/);
 a.context.applyBrightnessPolicy({nightLedsOff:false,mqttBrightness:true,conflict:true,mqttActive:false,connected:true});assert.equal(a.elements.nightBrightness.disabled,false);assert.equal(a.elements.releaseBrightness.disabled,true);assert.match(a.elements.brightnessTopic.textContent,/gesperrt/);
 console.log('PASS brightness GUI: independent authenticated saves, night slider preservation, source/override/topic status, conflict gate and honest queued AUTO response');
})().catch(error=>{console.error(error);process.exitCode=1});
