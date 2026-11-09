#pragma once
static const char PORTAL_HEAD[] PROGMEM=R"HTML(<script src="/errors.js?v=2.0.11-beta.2" onerror="document.addEventListener(&#x27;DOMContentLoaded&#x27;,()=&gt;{document.body.textContent=&quot;Fehler HTTP-018\nDiagnoseskript konnte nicht geladen werden\nDer Browser konnte das gemeinsame lokale Skript nicht laden; die genauere Ursache ist nicht erkennbar.\nMaßnahmen: Uhradresse und Netzwerk prüfen und Seite neu laden.&quot;})"></script><script>
document.addEventListener('submit',async event=>{
 const form=event.target;if(!['/wifisave','/exit'].includes(new URL(form.action,location.href).pathname))return;
 event.preventDefault();
 try{const session=await chaosFetch('/api/portal-session',{cache:'no-store'});await chaosCheck(session);const {token}=await chaosJson(session);
 const response=await chaosFetch(new URL(form.action,location.href).pathname,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':token},body:new URLSearchParams(new FormData(form))});
 await chaosCheck(response);document.body.textContent=form.action.endsWith('/exit')?'Einrichtung geschlossen.':'WLAN gespeichert. Die Uhr verbindet sich jetzt. Verbinde dich anschließend wieder mit deinem Heim-WLAN.';
 }catch(error){alert('Fehler: '+chaosMessage(error))}
});</script>)HTML";
static const char PORTAL_EXIT_HTML[] PROGMEM=R"HTML(<!doctype html><html lang="de"><meta charset="utf-8"><title>Einrichtung schließen</title><body><form method="POST" action="/exit"><button>WLAN-Einrichtung schließen</button></form>)HTML";
