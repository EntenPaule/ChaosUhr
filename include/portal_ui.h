#pragma once
static const char PORTAL_HEAD[] PROGMEM=R"HTML(<script>
document.addEventListener('submit',async event=>{
 const form=event.target;if(!['/wifisave','/exit'].includes(new URL(form.action,location.href).pathname))return;
 event.preventDefault();
 try{const session=await fetch('/api/portal-session',{cache:'no-store'});if(!session.ok)throw Error('Einrichtung nicht verfügbar.');const {token}=await session.json();
 const response=await fetch(new URL(form.action,location.href).pathname,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':token},body:new URLSearchParams(new FormData(form))});
 if(!response.ok)throw Error(await response.text());document.body.textContent=form.action.endsWith('/exit')?'Einrichtung geschlossen.':'WLAN gespeichert. Die Uhr verbindet sich jetzt. Verbinde dich anschließend wieder mit deinem Heim-WLAN.';
 }catch(error){alert('Fehler: '+error.message)}
});</script>)HTML";
static const char PORTAL_EXIT_HTML[] PROGMEM=R"HTML(<!doctype html><html lang="de"><meta charset="utf-8"><title>Einrichtung schließen</title><body><form method="POST" action="/exit"><button>WLAN-Einrichtung schließen</button></form>)HTML";
