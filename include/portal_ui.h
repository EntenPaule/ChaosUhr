#pragma once
static const char PORTAL_HEAD[] PROGMEM=R"HTML(<script src="/errors.js?v=2.0.11" onerror="document.addEventListener(&#x27;DOMContentLoaded&#x27;,()=&gt;{document.body.textContent=&#x27;Oberfläche konnte nicht geladen werden. Bitte Seite neu laden.&#x27;;const section=document.createElement(&#x27;section&#x27;);const title=document.createElement(&#x27;h2&#x27;);title.textContent=&#x27;Diagnose&#x27;;const text=document.createElement(&#x27;pre&#x27;);text.style.whiteSpace=&#x27;pre-wrap&#x27;;text.textContent=&quot;Fehler HTTP-018\nDiagnoseskript konnte nicht geladen werden\nDer Browser konnte das gemeinsame lokale Skript nicht laden; die genauere Ursache ist nicht erkennbar.\nMaßnahmen: Uhradresse und Netzwerk prüfen und Seite neu laden.&quot;;section.append(title,text);document.body.append(section)})"></script><script>
document.addEventListener('DOMContentLoaded',()=>{const section=document.createElement('section');section.setAttribute('aria-label','Diagnose');const title=document.createElement('h2');title.textContent='Diagnose';const text=document.createElement('pre');text.id='browserDiagnostics';text.style.whiteSpace='pre-wrap';section.append(title,text);document.body.append(section);chaosRenderDiagnostics()});
document.addEventListener('submit',async event=>{
 const form=event.target;if(!['/wifisave','/exit'].includes(new URL(form.action,location.href).pathname))return;
 event.preventDefault();
 try{const session=await chaosFetch('/api/portal-session',{cache:'no-store'});await chaosCheck(session);const {token}=await chaosJson(session);
 const response=await chaosFetch(new URL(form.action,location.href).pathname,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':token},body:new URLSearchParams(new FormData(form))});
 await chaosCheck(response);document.body.textContent=form.action.endsWith('/exit')?'Einrichtung geschlossen.':'WLAN gespeichert. Die Uhr verbindet sich jetzt. Verbinde dich anschließend wieder mit deinem Heim-WLAN.';
 }catch(error){alert(chaosMessage(error))}
});</script>)HTML";
static const char PORTAL_EXIT_HTML[] PROGMEM=R"HTML(<!doctype html><html lang="de"><meta charset="utf-8"><title>Einrichtung schließen</title><body><form method="POST" action="/exit"><button>WLAN-Einrichtung schließen</button></form>)HTML";
