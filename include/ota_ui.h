#pragma once
static const char OTA_HTML[] PROGMEM=R"HTML(
<!doctype html><html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ChaosUhr aktualisieren</title><body><header style="display:flex;justify-content:flex-end;gap:10px;align-items:center;flex-wrap:wrap" aria-label="Admin-Anmeldung"><span id="adminState" role="status">Nicht angemeldet</span><button id="adminLogout" type="button" hidden>Abmelden</button></header>
<section id="loginPanel"><h2>Admin-Anmeldung</h2><form id="adminLogin"><label for="adminPassword">Admin-Passwort</label><input id="adminPassword" name="password" type="password" autocomplete="current-password" minlength="16" maxlength="64" required><button id="adminLoginButton" type="submit">Anmelden</button></form><p>Nach 30 Minuten ohne bestätigte Aktivität wirst du abgemeldet. Lokales HTTP bitte nur im vertrauenswürdigen Netz verwenden.</p></section><p id="adminMessage" role="status" style="white-space:pre-line"></p><main id="protectedContent" hidden><h1>Firmware aktualisieren</h1>
<p>Wähle eine unkomprimierte ChaosUhr-BIN für den tatsächlich angeschlossenen Controller mit 4 MB Flash: Wemos / ESP8266 oder klassischer ESP32. Die Uhr prüft Zielchip und Produktkennung. Einstellungen und eigene Animationen bleiben bei passendem Layout erhalten.</p>
<form id="otaForm"><input id="firmware" type="file" accept=".bin" required><button id="upload">Update starten</button></form><p id="message" role="status"></p><a href="/">Zurück zur Uhr</a>
<hr><h2>Updates von GitHub</h2><p>Prüft den ChaosUhr-Updatekanal. Dein Browser benötigt Internetzugang. Der Download erfolgt über HTTPS; dein Admin-Passwort wird nur zur lokalen Uhr gesendet.</p>
<label><input id="includePreview" type="checkbox"> Vorabversionen berücksichtigen</label>
<button id="checkOnline" type="button" onclick="checkOnlineUpdates()">Auf Updates prüfen</button>
<button id="installOnline" type="button" onclick="installOnlineUpdate()" hidden>Gefundene Version installieren</button><p id="onlineMessage" role="status"></p>
</main><script src="/errors.js?v=2.0.11" onerror="document.addEventListener(&#x27;DOMContentLoaded&#x27;,()=&gt;{document.body.textContent=&quot;Fehler HTTP-018\nDiagnoseskript konnte nicht geladen werden\nDer Browser konnte das gemeinsame lokale Skript nicht laden; die genauere Ursache ist nicht erkennbar.\nMaßnahmen: Uhradresse und Netzwerk prüfen und Seite neu laden.&quot;})"></script><script>
const UPDATE_BASE='https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/';
let onlineCandidate=null,onlineBusy=false;
function updateVersion(text){
 const match=/^v?(0|[1-9][0-9]{0,2})\.(0|[1-9][0-9]{0,2})\.(0|[1-9][0-9]{0,2})(?:-([0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*))?$/.exec(text);
 return match?{numbers:match.slice(1,4).map(Number),pre:match[4]?.split('.')||[]}:null;
}
function compareUpdateVersions(a,b){
 const left=updateVersion(a),right=updateVersion(b);if(!left||!right)throw chaosFailure('OTA_MANIFEST');
 for(let i=0;i<3;i++)if(left.numbers[i]!==right.numbers[i])return left.numbers[i]>right.numbers[i]?1:-1;
 if(!left.pre.length||!right.pre.length)return left.pre.length===right.pre.length?0:left.pre.length?-1:1;
 for(let i=0;i<Math.max(left.pre.length,right.pre.length);i++){
  if(left.pre[i]===undefined)return -1;if(right.pre[i]===undefined)return 1;const x=left.pre[i],y=right.pre[i];if(x===y)continue;
  const xn=/^[0-9]+$/.test(x),yn=/^[0-9]+$/.test(y);if(xn&&yn)return Number(x)>Number(y)?1:-1;if(xn!==yn)return xn?-1:1;return x>y?1:-1;
 }return 0;
}
function validateUpdateManifest(manifest,current,allowPreview){
 if(!manifest||manifest.format!=='chaosuhr-update'||manifest.schema!==1||typeof manifest.version!=='string'||manifest.version.length>64||!updateVersion(manifest.version)||typeof manifest.prerelease!=='boolean'||(!allowPreview&&(manifest.prerelease||updateVersion(manifest.version).pre.length)))throw chaosFailure('OTA_MANIFEST');
 const esp32=current.platform==='esp32',board=esp32?'esp32dev':'d1_mini',maximum=esp32?1966080:1044464;
 if(!['esp8266','esp32'].includes(current.platform)||manifest.board!==board||manifest.chip!==current.platform||manifest.flash_bytes!==4194304||current.flash!==4194304)throw chaosFailure('OTA_TARGET');
 if(compareUpdateVersions(manifest.version,current.version)>0){
  const pinFits=manifest.configurable_led_pin===true?Array.isArray(manifest.supported_led_gpio)&&manifest.supported_led_gpio.length<=16&&manifest.supported_led_gpio.every(Number.isInteger)&&manifest.supported_led_gpio.includes(current.ledPin):manifest.led_gpio===current.ledPin;
  if(!pinFits)throw chaosFailure('OTA_TARGET');
 }
 if(manifest.binary!=='chaosuhr-'+manifest.version+'-'+(esp32?'esp32-ota':'esp8266')+'.bin'||!/^[0-9A-Za-z.-]+$/.test(manifest.binary)||!Number.isInteger(manifest.bytes)||manifest.bytes<4120||manifest.bytes>maximum||typeof manifest.sha256!=='string'||!/^[a-f0-9]{64}$/.test(manifest.sha256))throw chaosFailure('OTA_DOWNLOAD');
 return manifest;
}
async function localUpdateJson(path){const response=await chaosFetch(path,{credentials:'same-origin',cache:'no-store'});return chaosJson(response)}
function githubFetchOptions(){const options={credentials:'omit',referrerPolicy:'no-referrer',cache:'no-store'};if(typeof AbortSignal!=='undefined'&&AbortSignal.timeout)options.signal=AbortSignal.timeout(30000);return options}
async function checkOnlineUpdates(){
 if(onlineBusy)return;onlineBusy=true;onlineCandidate=null;
 const message=document.getElementById('onlineMessage'),button=document.getElementById('checkOnline');button.disabled=true;document.getElementById('installOnline').hidden=true;message.textContent='Prüfe GitHub …';
 try{
  const current=await localUpdateJson('/api/status'),preview=document.getElementById('includePreview').checked;
  const suffix=current.platform==='esp32'?'-esp32':'',channels=preview?['stable'+suffix+'.json','preview'+suffix+'.json']:['stable'+suffix+'.json'];
  const results=await Promise.allSettled(channels.map(async channel=>{const response=await chaosFetch(UPDATE_BASE+channel,githubFetchOptions());if(response.status===404)return null;if(!response.ok)throw chaosFailure('OTA_CHANNEL',response.status);const manifest=await chaosJson(response);return validateUpdateManifest(manifest,current,preview)}));
  const candidates=results.filter(result=>result.status==='fulfilled'&&result.value).map(result=>result.value).sort((a,b)=>compareUpdateVersions(b.version,a.version));
  if(!candidates.length){const error=results.find(result=>result.status==='rejected');if(error)throw error.reason;message.textContent='Im gewählten Updatekanal ist noch keine Version veröffentlicht. Das manuelle OTA bleibt verfügbar.';return;}
  const found=candidates[0];if(compareUpdateVersions(found.version,current.version)<=0){message.textContent='Installiert: '+current.version+'. Keine neuere passende Version gefunden.';return;}
  onlineCandidate=found;message.textContent='Verfügbar: '+found.version+(found.prerelease?' (Vorabversion)':'')+' · '+Math.round(found.bytes/1024)+' KiB. Vor der Installation Einstellungen sichern.';document.getElementById('installOnline').hidden=false;
 }catch(error){message.textContent='Updateprüfung fehlgeschlagen: '+chaosMessage(error)+' Manuelles OTA ist weiterhin möglich.'}
 finally{onlineBusy=false;button.disabled=false}
}
async function installOnlineUpdate(){
 if(onlineBusy||!onlineCandidate)return;
 const candidate=onlineCandidate;if(!confirm('ChaosUhr '+candidate.version+' installieren? Die Uhr startet danach neu. Einstellungen vorher sichern.'))return;
 onlineBusy=true;const message=document.getElementById('onlineMessage'),button=document.getElementById('installOnline');button.disabled=true;
 try{
  const current=await localUpdateJson('/api/status');validateUpdateManifest(candidate,current,true);if(compareUpdateVersions(candidate.version,current.version)<=0)throw chaosFailure('OTA_NOT_NEWER');
  message.textContent='Lade Firmware über HTTPS von GitHub …';
  const response=await chaosFetch(UPDATE_BASE+candidate.binary,githubFetchOptions());if(!response.ok)throw chaosFailure('OTA_CHANNEL',response.status);
  const length=response.headers?.get('Content-Length');if(length&&Number(length)!==candidate.bytes)throw chaosFailure('OTA_DOWNLOAD');
  const bytes=await response.arrayBuffer();if(bytes.byteLength!==candidate.bytes||new Uint8Array(bytes)[0]!==233)throw chaosFailure('OTA_DOWNLOAD');
  const {token}=await localUpdateJson('/api/session'),form=new FormData();form.append('firmware',new Blob([bytes],{type:'application/octet-stream'}),candidate.binary);
  message.textContent='Übertrage und prüfe Firmware. Stromversorgung nicht trennen.';
  const result=await chaosFetch('/update',{method:'POST',credentials:'same-origin',headers:{'X-CSRF-Token':token,'X-Firmware-SHA256':candidate.sha256},body:form});
  await chaosCheck(result);message.textContent='Firmware geprüft. Die Uhr startet mit '+candidate.version+' neu.';onlineCandidate=null;
 }catch(error){message.textContent='Update abgebrochen: '+chaosMessage(error);button.disabled=false}
 finally{onlineBusy=false}
}
document.getElementById('otaForm').addEventListener('submit',async event=>{
 event.preventDefault();const button=document.getElementById('upload'),message=document.getElementById('message');button.disabled=true;
 try{const file=document.getElementById('firmware').files[0];if(!file||!file.size||file.size>1966080)throw chaosFailure('OTA_SIZE');
 const current=await localUpdateJson('/api/status'),maximum=current.platform==='esp32'?1966080:1044464;if(file.size>maximum)throw chaosFailure('OTA_TARGET');
 const session=await chaosFetch('/api/session',{cache:'no-store',credentials:'same-origin'});await chaosCheck(session);const {token}=await chaosJson(session);
 const form=new FormData();form.append('firmware',file);message.textContent='Update läuft. Bitte die Stromversorgung nicht trennen.';
 const response=await chaosFetch('/update',{method:'POST',credentials:'same-origin',headers:{'X-CSRF-Token':token},body:form});
 await chaosCheck(response);message.textContent='Firmware geprüft und übertragen. Die Uhr startet neu.';
 }catch(error){message.textContent='Fehler: '+chaosMessage(error);button.disabled=false}
});
chaosSessionStart();
</script></body></html>
)HTML";
