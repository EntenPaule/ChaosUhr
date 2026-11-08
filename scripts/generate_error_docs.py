"""Generate the public code reference and small browser fallback catalogue.

The permanent source is include/error_catalog.def. --check detects drift.
"""
from pathlib import Path
import json,re,sys,html
root=Path(__file__).resolve().parents[1]
rows=[]
for line in (root/'include/error_catalog.def').read_text().splitlines():
    m=re.fullmatch(r'CHAOS_ERROR\((\w+),(.+),([01])\)',line)
    if m:
        values=json.loads('['+m[2]+']');rows.append((m[1],*values,bool(int(m[3]))))
contract=root/'tests/error_code_contract.json'
if contract.exists():
    current={r[1]:r[0] for r in rows}
    for code,symbol in json.loads(contract.read_text()).items():
        assert current.get(code)==symbol, 'Permanent code changed or reused: '+code
assert rows and len({r[1] for r in rows})==len(rows)
assert len({r[0] for r in rows})==len(rows)
for _,code,component,*_ in rows:assert re.fullmatch(r'(WIFI|MQTT|FS|HTTP|AUTH|OTA|CFG|SYS)-\d{3}',code) and code.startswith(component+'-')
text='# ChaosUhr – stabile Fehlercodes\n\nQuelle: `include/error_catalog.def`; automatisch mit `python3 scripts/generate_error_docs.py` erzeugt. Codes niemals umnummerieren oder für andere Ursachen wiederverwenden. Neue Codes mit neuer freier Nummer ergänzen; anschließend Generator und Prüfungen ausführen.\n\nHTTP-Status und ChaosUhr-Code sind getrennte Angaben. Meldungen enthalten ausschließlich feste, bereinigte Texte. Die letzte Meldung und acht Ereignisse liegen im RAM, ohne laufende Flash-Schreibzugriffe. Wiederholungen werden gezählt und zusammengefasst. Ein letzter Fehler ist ein historischer Eintrag, kein Beweis für einen weiterhin aktiven Defekt.\n\n| Code | Komponente | Bedeutung | Technische Ursache | Maßnahme |\n| --- | --- | --- | --- | --- |\n'
for _,code,component,msg,details,action,_ in rows:text+='| '+' | '.join(v.replace('|','\\|') for v in (code,component,msg,details,action))+' |\n'
required=set()
for name in ('src/main.cpp','include/ota_ui.h','include/public_ui.h','include/portal_ui.h','scripts/error_browser.js'):
    required.update(re.findall(r"chaosFailure\('([A-Z_]+)'",(root/name).read_text()))
client={r[1]:{'message':r[3],'details':r[4],'action':r[5]} for r in rows if r[-1] or r[0] in required}
symbols={r[0]:r[1] for r in rows}
js='const CHAOS_CLIENT_ERRORS='+json.dumps(client,ensure_ascii=False,separators=(',',':'))+';\nconst CHAOS_CODES='+json.dumps(symbols,separators=(',',':'))+';\n'
js+=(root/'scripts/error_browser.js').read_text()
outputs={root/'docs/FEHLERCODES.md':text,root/'include/error_ui.h':'// Generated; edit error_catalog.def or scripts/error_browser.js.\n#pragma once\nstatic const char ERROR_JS[] PROGMEM=R"CHAOSJS('+js+')CHAOSJS";\n'}
version=re.search(r'CHAOSUHR_VERSION "([^"]+)"',(root/'include/firmware_version.h').read_text())[1]
script_failure=next(row for row in rows if row[0]=='HTTP_SCRIPT')
fallback='Fehler '+script_failure[1]+'\n'+script_failure[3]+'\n'+script_failure[4]+'\nMaßnahmen: '+script_failure[5]
handler="document.addEventListener('DOMContentLoaded',()=>{document.body.textContent="+json.dumps(fallback,ensure_ascii=False)+"})"
tag='<script src="/errors.js?v='+version+'" onerror="'+html.escape(handler,quote=True)+'"></script>'
for name in ('src/main.cpp','include/ota_ui.h','include/public_ui.h','include/portal_ui.h'):
    path=root/name;original=path.read_text()
    result,count=re.subn(r'<script src="/errors\.js(?:\?v=[^"]+)?"(?: onerror="[^"]*")?></script>',lambda _:tag,original)
    assert count==1,name
    outputs[path]=result
for path,data in outputs.items():
    if '--check' in sys.argv:
        if not path.exists() or path.read_text()!=data:raise SystemExit('Generated error catalogue is stale: '+str(path.relative_to(root)))
    elif not path.exists() or path.read_text()!=data:path.write_text(data)
print('PASS error catalogue:',len(rows),'unique stable codes; firmware, browser and documentation synchronized')
