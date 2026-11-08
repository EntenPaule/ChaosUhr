// Only synthetic public fixtures. Execute the actual production saveMqtt().
const app=require('./test_display_mqtt_ui'),assert=require('node:assert/strict');
(async()=>{
 const requests=[];
 for(const host of ['broker.example.test','192.0.2.10'])for(const prefix of ['homeassistant','']){
  const a=app();await a.context.load();
  a.elements.mqEnabled.checked=true;a.elements.mqHost.value=host;
  a.elements.mqUser.value='';a.elements.mqDiscovery.checked=false;
  a.elements.mqDiscoveryPrefix.value=prefix;
  await a.context.saveMqtt();
  const post=a.posts.find(p=>p.route==='/api/mqtt');assert(post);
  const body=post.options.body.toString();assert.equal([...post.options.body].length,24);
  requests.push({prefix,fields:[...post.options.body],http:'POST /api/mqtt HTTP/1.1\r\nHost: chaosuhr.local\r\nOrigin: http://chaosuhr.local\r\nCookie: ChaosUhrSID=synthetic-only\r\nX-CSRF-Token: fixture-session\r\nContent-Type: '+post.options.headers['Content-Type']+'\r\nContent-Length: '+Buffer.byteLength(body)+'\r\n\r\n'+body});
 }
 process.stdout.write(JSON.stringify(requests));
})().catch(()=>{console.error('Browser request fixture failed; values withheld');process.exitCode=1});
