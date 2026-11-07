# GitHub-Updatekanal

2.0.8 lädt Updates im Browser über HTTPS und überträgt sie anschließend mit Anmeldung, CSRF-Token und SHA-256 zur lokalen Uhr. Auf dem ESP8266 entsteht kein zusätzlicher TLS-Verbindungspuffer. Das lokale Passwort wird nicht an GitHub gesendet.

Feste Basis: `https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/`

Der separate Branch `firmware-updates` enthält versionierte `chaosuhr-VERSION-esp8266.bin`-Dateien und die Manifeste. **2.0.8 wird ausschließlich über `preview.json` angeboten; `stable.json` fehlt bis zur ersten stabil freigegebenen Version.** Vorabversionen berücksichtigt die GUI nur nach Wahl des entsprechenden Häkchens. Kein automatischer Downgrade; auf einer bereits aktuellen 2.0.8 wird keine erneute Installation angeboten.

Manifest: `format=chaosuhr-update`, `schema=1`, Version, Vorabstatus, `board=d1_mini`, `chip=esp8266`, `flash_bytes=4194304`, `led_gpio=12`, BIN-Dateiname, Bytezahl und SHA-256. Die GUI prüft diese Angaben; beliebige fremde URLs lassen sich nicht aus einem Manifest einschleusen.

## Warum ein separater Kanal?

Beim geprüften normalen GitHub-Release-Anhang führte die Weiterleitung auf einen Asset-Host ohne Browser-CORS-Freigabe. Der GitHub-Raw-Host wurde dagegen lesend mit HTTPS und `Access-Control-Allow-Origin: *` bestätigt. Kein fremder CORS-Proxy wird benötigt. Ein alternatives Hosting müsste HTTPS und CORS unterstützen und bewusst im Quellcode konfiguriert werden.

## Veröffentlichung vorbereiten

`python3 scripts/package_public.py` erzeugt unter `dist/2.0.8/esp8266/update-channel/` die passende BIN und `preview.json` lokal. Das Skript lädt selbst nichts hoch. Der öffentliche 2.0.8-Testkanal wird zusammen mit dem GitHub-Vorabrelease gepflegt.

Nach ausdrücklicher Veröffentlichungsfreigabe: BIN zuerst auf den Kanal-Branch hochladen und öffentlich mit SHA-256 prüfen; danach das zugehörige Manifest veröffentlichen. Versionierte BINs nicht ersetzen. Bei einer Vorabversion vorhandenes `stable.json` erhalten. Für eine später ausdrücklich als stabil freigegebene Version dasselbe Schema mit `prerelease=false` als `stable.json` verwenden. Diesen Kanal bei jeder künftigen Veröffentlichung mitpflegen; Release-Anhänge allein aktualisieren ihn nicht.

Keine echte Browser-/Geräte-Installation über den Kanal ausgeführt. GUI-Abläufe wurden lokal getestet; der geräteseitige CSRF-/SHA-OTA-Pfad wurde mit erfolgreichem Folgeupdate und Neustart auf einem Wemos geprüft. SHA-256 ist keine Herausgebersignatur; die lokale HTTP-Strecke und der Besitz des GitHub-Kontos bleiben Teil des Vertrauensmodells. [Sicherheitsgrenzen](SICHERHEIT_2_0_8.md).
