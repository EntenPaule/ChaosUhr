#pragma once

#ifdef CHAOSUHR_PUBLIC_BUILD
static const char SETUP_HTML[] PROGMEM=R"HTML(
<!doctype html><html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ChaosUhr einrichten</title>
<style>:root{color-scheme:dark;font-family:system-ui}body{max-width:540px;margin:32px auto;padding:0 18px;background:#0b1020;color:#e9f0ff}input,button{box-sizing:border-box;width:100%;font:inherit;padding:12px;margin:8px 0 18px;border-radius:8px}button{background:#53b9ff;border:0;color:#081221;font-weight:bold}small{color:#b7c6df}</style></head>
<body><h1>ChaosUhr einrichten</h1><p>Lege zuerst dein eigenes Admin-Passwort fest. Damit meldest du dich später als <strong>admin</strong> an und schützt auch Firmware-Updates.</p>
<form id="setupForm"><label for="password">Admin-Passwort</label><input id="password" type="password" minlength="16" maxlength="64" autocomplete="new-password" required>
<label for="repeat">Passwort wiederholen</label><input id="repeat" type="password" minlength="16" maxlength="64" autocomplete="new-password" required>
<small>16–64 Zeichen. Verwende Buchstaben ohne Umlaute, Zahlen, Leerzeichen und Sonderzeichen.</small><button id="setupSubmit" type="submit">Admin-Zugang speichern</button></form>
<p id="setupMessage" role="status"></p><p>Danach startet die Uhr neu. Falls noch kein WLAN gespeichert ist, verbinde dich erneut mit dem gleichen Einrichtungs-WLAN und wähle dort dein Heim-WLAN. Der auf der Uhr angezeigte Einrichtungscode bleibt dafür gültig.</p>
<script src="/errors.js?v=2.0.11-beta.4" onerror="document.addEventListener(&#x27;DOMContentLoaded&#x27;,()=&gt;{document.body.textContent=&quot;Fehler HTTP-018\nDiagnoseskript konnte nicht geladen werden\nDer Browser konnte das gemeinsame lokale Skript nicht laden; die genauere Ursache ist nicht erkennbar.\nMaßnahmen: Uhradresse und Netzwerk prüfen und Seite neu laden.&quot;})"></script><script>
async function saveAdmin(event){event.preventDefault();const password=document.getElementById('password').value;const repeat=document.getElementById('repeat').value;const message=document.getElementById('setupMessage');const button=document.getElementById('setupSubmit');if(!/^[\x20-\x7e]{16,64}$/.test(password)){message.textContent=chaosFailure('AUTH_PASSWORD_FORMAT').message;return}if(password!==repeat){message.textContent=chaosFailure('AUTH_PASSWORD_REPEAT').message;return}button.disabled=true;try{const session=await chaosFetch('/api/session',{cache:'no-store'});await chaosCheck(session);const {token}=await chaosJson(session);const response=await chaosFetch('/api/setup',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded','X-CSRF-Token':token},body:new URLSearchParams({password,repeat})});await chaosCheck(response);document.getElementById('password').value='';document.getElementById('repeat').value='';message.textContent='Admin-Zugang gespeichert. Die Uhr startet neu. Danach kannst du dein WLAN einrichten.';}catch(error){message.textContent='Fehler: '+chaosMessage(error);button.disabled=false}}
document.getElementById('setupForm').addEventListener('submit',saveAdmin);
</script></body></html>
)HTML";
#endif
