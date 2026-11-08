// Generated; edit error_catalog.def or scripts/error_browser.js.
#pragma once
static const char ERROR_JS[] PROGMEM=R"CHAOSJS(const CHAOS_CLIENT_ERRORS={"AUTH-020":{"message":"Admin-Sitzung abgelaufen","details":"Seit der letzten bestätigten Aktivität sind mindestens 30 Minuten vergangen; Sitzung und CSRF sind ungültig.","action":"Erneut über das Anmeldeformular anmelden; Hintergrundabfragen verlängern die Sitzung nicht."},"AUTH-021":{"message":"Admin-Sitzung ungültig","details":"Die Sitzung wurde abgemeldet, ersetzt oder durch einen Geräteneustart ungültig.","action":"Erneut anmelden und die gewünschte Aktion anschließend bewusst wiederholen."},"AUTH-022":{"message":"Sitzungs-Cookie ungültig","details":"Der Cookie ist zu lang, mehrfach angegeben oder enthält kein gültiges Sitzungsformat.","action":"Seite neu öffnen und erneut anmelden; keine Sitzungstokens manuell ändern."},"AUTH-023":{"message":"Alle Admin-Sitzungen belegt","details":"Die zwei RAM-Sitzungsplätze werden bereits von aktiven Anmeldungen verwendet.","action":"Eine andere Sitzung abmelden oder deren Ablauf abwarten."},"SYS-007":{"message":"Unerwarteter Fehler in der Weboberfläche","details":"Eine lokale Browseroperation ist fehlgeschlagen; eine genauere Ursache ist nicht erkennbar.","action":"Seite neu laden, Firmwareversion prüfen und bereinigte Diagnose sichern."},"MQTT-007":{"message":"MQTT-Konfiguration ungültig","details":"Mindestens eine Konfigurationsregel ist verletzt.","action":"Broker, Port, Topics und Wertebereiche prüfen."},"FS-001":{"message":"Datenspeicher nicht lesbar","details":"LittleFS konnte ohne Formatierung nicht eingebunden werden.","action":"Vorhandene Dateien sichern; Initialisierung nur ausdrücklich bestätigen."},"HTTP-005":{"message":"HTTP-Inhalt zu groß","details":"Der Request-Body überschreitet die Endpunktgrenze.","action":"Datei oder Formularinhalt verkleinern."},"HTTP-007":{"message":"HTTP-Verbindung fehlgeschlagen","details":"Der Browser konnte keine verwertbare Antwort empfangen; die Ursache ist nicht genauer erkennbar.","action":"Uhradresse, Netzwerk und Erreichbarkeit prüfen."},"HTTP-008":{"message":"HTTP-Zeitlimit überschritten","details":"Eine HTTP-Operation hat ihre Zeitgrenze überschritten.","action":"Verbindung prüfen und Status vor einem erneuten Schreibversuch lesen."},"HTTP-009":{"message":"Leere HTTP-Antwort","details":"Eine erwartete strukturierte Antwort enthält keinen Inhalt.","action":"Status und Anmeldung prüfen; Seite neu laden."},"HTTP-010":{"message":"HTTP-Antwort enthält kein gültiges JSON","details":"Die Antwort konnte nicht als erwartetes JSON gelesen werden.","action":"Uhradresse und Firmwareversion prüfen; Seite neu laden."},"HTTP-011":{"message":"HTTP-Fehler ohne detaillierte Diagnose","details":"Der Server meldet einen Fehlerstatus ohne bekannten ChaosUhr-Code.","action":"HTTP-Status notieren und Supportdiagnose sichern."},"HTTP-012":{"message":"HTTP-Antwort abgebrochen","details":"Das Lesen des Antwortinhalts ist fehlgeschlagen.","action":"Netzwerk prüfen und Gerätestatus vor Wiederholung lesen."},"AUTH-001":{"message":"Anmeldung erforderlich","details":"Die Anfrage enthält keine akzeptierte Admin-Anmeldung.","action":"Weboberfläche neu öffnen und als admin anmelden."},"AUTH-010":{"message":"Zugriff abgewiesen","details":"Die Anfrage hat keine ausreichende Berechtigung; genauere Ursache fehlt.","action":"Anmeldung und geöffnete Uhradresse prüfen."},"AUTH-011":{"message":"Admin-Passwortformat ungültig","details":"Es sind 16 bis 64 druckbare ASCII-Zeichen erforderlich.","action":"Ein passendes Passwort eingeben, ohne es in Supportdaten zu teilen."},"AUTH-012":{"message":"Passwortwiederholung stimmt nicht überein","details":"Die beiden eingegebenen Passwörter unterscheiden sich.","action":"Beide Passwortfelder erneut eingeben."},"AUTH-014":{"message":"Ausdrückliche Bestätigung fehlt","details":"Die destruktive Aktion wurde nicht vollständig bestätigt.","action":"Seite neu laden und Bestätigungsfelder bewusst ausfüllen."},"OTA-004":{"message":"Firmware überschreitet OTA-Grenze","details":"Die vollständige Anwendungsdatei passt nicht in den Zielslot.","action":"Passende Anwendungs-BIN verwenden; kein Factory-Image hochladen."},"OTA-006":{"message":"Firmware passt nicht zum Zielchip oder Layout","details":"Zielchip, Flashmodus oder Flashlayout stimmt nicht überein.","action":"Firmware für genau das eigene Board auswählen."},"OTA-016":{"message":"Update-Manifest ungültig","details":"Version, Schema oder Downloadangaben sind nicht gültig.","action":"Updatequelle prüfen; vorhandene Firmware bleibt aktiv."},"OTA-017":{"message":"Updatekanal nicht erreichbar","details":"Der externe Server lieferte keinen erfolgreichen HTTP-Status.","action":"Internetzugang und GitHub-Verfügbarkeit prüfen."},"OTA-018":{"message":"Firmwaredownload unvollständig","details":"Downloadlänge oder Dateikopf passt nicht zum Manifest.","action":"Download erneut prüfen; keine Aktivierung erfolgt."},"OTA-019":{"message":"Firmware ist nicht neuer","details":"Ein erneutes Update oder Downgrade wurde abgewiesen.","action":"Aktuellen Versionsstatus prüfen."},"CFG-003":{"message":"Konfigurationswert ungültig","details":"Ein bekannter Parameter verletzt Format oder Wertebereich.","action":"Eingabe und zulässigen Wertebereich prüfen."},"CFG-005":{"message":"Keine gültige Anzeigeart gewählt","details":"Mindestens eine zulässige Anzeigeart muss aktiv sein.","action":"Uhr, Animationen, Wetter oder Sensoren wählen."},"CFG-006":{"message":"Keine verfügbare Animation gewählt","details":"Die aktive Animationskategorie hat keine verfügbare Auswahl.","action":"Mindestens eine vorhandene Animation wählen oder Kategorie abschalten."},"CFG-010":{"message":"Einstellungssicherung ungültig","details":"Schema, Inhalt oder Zuordnung der Sicherung ist ungültig.","action":"Eine unveränderte passende Sicherung verwenden."},"CFG-011":{"message":"MQTT-Passwortaktion widersprüchlich","details":"Ersetzen und Löschen wurden gleichzeitig angefordert.","action":"Entweder neues Passwort setzen oder gespeichertes Passwort löschen."},"CFG-017":{"message":"Animationsdatei unvollständig","details":"Dateidaten oder Upload-Abschluss fehlen.","action":"Eine vollständige .chaos-Datei auswählen."},"CFG-201":{"message":"Einstellungssicherung: Schema ungültig","details":"Dateiformat, Version oder Kopfstruktur wird nicht unterstützt.","action":"Eine unveränderte ChaosUhr-JSON-Sicherung verwenden."},"CFG-202":{"message":"Einstellungssicherung: Felder fehlen oder haben einen falschen Typ","details":"Die Einstellungsstruktur ist nicht vollständig oder enthält keine ganzen Zahlen.","action":"Vollständige Sicherung aus einer unterstützten Version verwenden."},"CFG-203":{"message":"Einstellungssicherung: Wert außerhalb des Bereichs","details":"Mindestens ein importierter Einstellungswert überschreitet seinen Wertebereich.","action":"Sicherung prüfen und passende Werte verwenden."},"CFG-204":{"message":"Einstellungssicherung: Upload-Liste ungültig","details":"Die Upload-Zuordnung ist keine zulässige Liste mit höchstens acht Einträgen.","action":"Eigene Animationen separat sichern und gültige Sicherung verwenden."},"CFG-205":{"message":"Einstellungssicherung: Upload-Fingerabdruck ungültig","details":"Upload-ID, SHA-256 oder Eindeutigkeit der Zuordnung ist ungültig.","action":"Unveränderte Sicherung verwenden und benötigte Animationen hochladen."},"CFG-206":{"message":"Einstellungssicherung: Automatik-Upload fehlt","details":"Eine ausgewählte eigene Animation wurde am Zielgerät nicht gefunden.","action":"Die passende .chaos-Datei vor dem Import hochladen."},"CFG-207":{"message":"Einstellungssicherung: Einzelanimation fehlt","details":"Die gespeicherte Einzelanimation ist am Zielgerät nicht verfügbar.","action":"Passende Animation hochladen oder Auswahl korrigieren."},"CFG-208":{"message":"Einstellungssicherung größer als 8 KiB","details":"Die ausgewählte Sicherungsdatei überschreitet die feste Importgrenze.","action":"Eine gültige Sicherung mit höchstens 8 KiB auswählen."},"CFG-209":{"message":"Einstellungssicherung enthält kein gültiges JSON","details":"Die ausgewählte Datei lässt sich nicht als JSON lesen.","action":"Originale unveränderte Sicherungsdatei verwenden."},"CFG-210":{"message":"Einstellungssicherung konnte nicht gelesen werden","details":"Der Browser konnte den Inhalt der lokalen Datei nicht lesen.","action":"Dateizugriff prüfen und Datei erneut auswählen."}};
const CHAOS_CODES={"AUTH_SESSION_EXPIRED":"AUTH-020","AUTH_SESSION_INVALID":"AUTH-021","AUTH_SESSION_COOKIE":"AUTH-022","AUTH_SESSION_LIMIT":"AUTH-023","AUTH_LOGIN_FIELDS":"AUTH-024","MQTT_ACK_TIMEOUT":"MQTT-021","SYS_UI":"SYS-007","FS_RESET_UNKNOWN":"FS-019","HTTP_SCRIPT":"HTTP-018","AUTH_HEADER":"AUTH-018","AUTH_CREDENTIALS_CORRUPT":"AUTH-019","SYS_RESETTING":"SYS-006","HTTP_SETUP_HOST":"HTTP-017","MQTT_PERMISSION":"MQTT-020","SYS_CRYPTO":"SYS-005","OTA_TRAILING":"OTA-020","OTA_PADDING":"OTA-021","CFG_NUMBER_FORMAT":"CFG-020","CFG_RANGE":"CFG-021","CFG_TEXT_LENGTH":"CFG-022","CFG_TEXT_CHARACTER":"CFG-023","CFG_TOPIC":"CFG-024","CFG_HOST":"CFG-025","CFG_GPIO":"CFG-026","MQTT_TLS":"MQTT-019","WIFI_NO_CONFIG":"WIFI-001","WIFI_CONNECT_TIMEOUT":"WIFI-002","WIFI_LINK_LOST":"WIFI-003","WIFI_START_FAILED":"WIFI-004","WIFI_AP_FAILED":"WIFI-005","WIFI_PORTAL_FAILED":"WIFI-006","WIFI_ERASE_FAILED":"WIFI-007","WIFI_MDNS_FAILED":"WIFI-008","MQTT_DNS":"MQTT-001","MQTT_TCP":"MQTT-002","MQTT_TIMEOUT":"MQTT-003","MQTT_REJECTED":"MQTT-004","MQTT_LOGIN":"MQTT-005","MQTT_LINK_LOST":"MQTT-006","MQTT_CONFIG_INVALID":"MQTT-007","MQTT_SAVE_UNKNOWN":"MQTT-008","MQTT_LOAD_UNKNOWN":"MQTT-009","MQTT_CONNECT_UNKNOWN":"MQTT-010","MQTT_PACKET_MEMORY":"MQTT-011","MQTT_SUBSCRIBE":"MQTT-012","MQTT_PAYLOAD":"MQTT-013","MQTT_OVERSIZE":"MQTT-014","MQTT_QUEUE":"MQTT-015","MQTT_FRAGMENT":"MQTT-016","MQTT_PROTOCOL":"MQTT-017","MQTT_CLIENT_ID":"MQTT-018","FS_MOUNT":"FS-001","FS_OPEN_WRITE":"FS-002","FS_WRITE":"FS-003","FS_VERIFY_OPEN":"FS-004","FS_VERIFY_READ":"FS-005","FS_VERIFY_CONTENT":"FS-006","FS_RENAME":"FS-007","FS_REMOVE":"FS-008","FS_OPEN_READ":"FS-009","FS_CONFIG_CORRUPT":"FS-010","FS_FORMAT":"FS-011","FS_DIRECTORY":"FS-012","FS_READ":"FS-013","FS_ALREADY_READY":"FS-014","FS_RESERVE":"FS-015","FS_JOURNAL_CORRUPT":"FS-016","FS_EEPROM_MIRROR":"FS-017","FS_EEPROM_WRITE":"FS-018","HTTP_METHOD":"HTTP-001","HTTP_REQUEST":"HTTP-002","HTTP_HEADERS":"HTTP-003","HTTP_LENGTH":"HTTP-004","HTTP_BODY_LIMIT":"HTTP-005","HTTP_TRANSFER":"HTTP-006","HTTP_NETWORK":"HTTP-007","HTTP_TIMEOUT":"HTTP-008","HTTP_EMPTY":"HTTP-009","HTTP_JSON":"HTTP-010","HTTP_STATUS":"HTTP-011","HTTP_BODY_ABORT":"HTTP-012","HTTP_ARGUMENTS":"HTTP-013","HTTP_BOUNDARY":"HTTP-014","HTTP_NOT_FOUND":"HTTP-015","HTTP_URI":"HTTP-016","AUTH_REQUIRED":"AUTH-001","AUTH_REJECTED":"AUTH-002","AUTH_CSRF":"AUTH-003","AUTH_ORIGIN":"AUTH-004","AUTH_NOT_READY":"AUTH-005","AUTH_RATE":"AUTH-006","AUTH_CRYPTO":"AUTH-007","AUTH_SETUP":"AUTH-008","AUTH_SETUP_AP":"AUTH-009","AUTH_FORBIDDEN":"AUTH-010","AUTH_PASSWORD_FORMAT":"AUTH-011","AUTH_PASSWORD_REPEAT":"AUTH-012","AUTH_STORE":"AUTH-013","AUTH_CONFIRM":"AUTH-014","AUTH_RESET_MARK":"AUTH-015","AUTH_PORTAL_ACTION":"AUTH-016","AUTH_PASSWORD_FIELDS":"AUTH-017","OTA_PARTS":"OTA-001","OTA_FIELD":"OTA-002","OTA_EXPECTED_SHA":"OTA-003","OTA_SIZE":"OTA-004","OTA_HEADER":"OTA-005","OTA_TARGET":"OTA-006","OTA_BEGIN":"OTA-007","OTA_WRITE":"OTA-008","OTA_FINISH":"OTA-009","OTA_ABORT":"OTA-010","OTA_INCOMPLETE":"OTA-011","OTA_CRC":"OTA-012","OTA_SHA":"OTA-013","OTA_PRODUCT":"OTA-014","OTA_EXTRA_FIELDS":"OTA-015","OTA_MANIFEST":"OTA-016","OTA_CHANNEL":"OTA-017","OTA_DOWNLOAD":"OTA-018","OTA_NOT_NEWER":"OTA-019","CFG_EMPTY":"CFG-001","CFG_UNKNOWN":"CFG-002","CFG_VALUE":"CFG-003","CFG_DUPLICATE":"CFG-004","CFG_DISPLAY":"CFG-005","CFG_ANIMATION_SELECTION":"CFG-006","CFG_UPLOAD_MISSING":"CFG-007","CFG_UPLOAD_UNSUPPORTED":"CFG-008","CFG_SAVE":"CFG-009","CFG_BACKUP":"CFG-010","CFG_PASSWORD_CONFLICT":"CFG-011","CFG_UPLOAD_PARTS":"CFG-012","CFG_UPLOAD_FIELD":"CFG-013","CFG_UPLOAD_SLOTS":"CFG-014","CFG_UPLOAD_SIZE":"CFG-015","CFG_UPLOAD_ABORT":"CFG-016","CFG_UPLOAD_EMPTY":"CFG-017","CFG_UPLOAD_NOT_FOUND":"CFG-018","CFG_DEFAULTS":"CFG-019","SYS_RESTARTING":"SYS-001","SYS_MEMORY":"SYS-002","SYS_STORAGE_SIZE":"SYS-003","SYS_UNKNOWN":"SYS-004","ANIMATION_FORMAT_101":"CFG-101","ANIMATION_FORMAT_102":"CFG-102","ANIMATION_FORMAT_103":"CFG-103","ANIMATION_FORMAT_104":"CFG-104","ANIMATION_FORMAT_105":"CFG-105","ANIMATION_FORMAT_106":"CFG-106","ANIMATION_FORMAT_107":"CFG-107","ANIMATION_FORMAT_108":"CFG-108","ANIMATION_FORMAT_109":"CFG-109","ANIMATION_FORMAT_110":"CFG-110","ANIMATION_FORMAT_111":"CFG-111","ANIMATION_FORMAT_112":"CFG-112","ANIMATION_FORMAT_113":"CFG-113","ANIMATION_FORMAT_114":"CFG-114","ANIMATION_FORMAT_115":"CFG-115","BACKUP_SCHEMA":"CFG-201","BACKUP_FIELDS":"CFG-202","BACKUP_RANGE":"CFG-203","BACKUP_UPLOAD_LIST":"CFG-204","BACKUP_UPLOAD_HASH":"CFG-205","BACKUP_AUTO_UPLOAD":"CFG-206","BACKUP_SINGLE_UPLOAD":"CFG-207","BACKUP_SIZE":"CFG-208","BACKUP_JSON":"CFG-209","BACKUP_READ":"CFG-210"};
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
)CHAOSJS";
