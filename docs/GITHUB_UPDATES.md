# GitHub-Updatekanal

Seit 2.0.8 lädt die GUI Updates im Browser über HTTPS und überträgt sie anschließend mit Anmeldung, CSRF-Token und SHA-256 zur lokalen Uhr. Auf dem ESP8266 entsteht kein zusätzlicher TLS-Verbindungspuffer. Das lokale Passwort wird nicht an GitHub gesendet. 2.0.10 erweitert dies um klassische ESP32-Ziele und auswählbare LED-Pins.

Feste Basis: `https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/`

Der separate Branch `firmware-updates` enthält versionierte Anwendungs-BINs und die Manifeste. **2.0.10 wird über `preview.json` für Wemos und `preview-esp32.json` für klassischen ESP32 angeboten; stabile Kanäle fehlen bis zur ersten stabil freigegebenen Version.** Vorabversionen berücksichtigt die GUI nur nach Wahl des entsprechenden Häkchens. Kein automatischer Downgrade; dieselbe bereits installierte Version wird nicht erneut angeboten. Der vorherige 2.0.8-Release und seine versionierte BIN bleiben erhalten.

Manifest: `format=chaosuhr-update`, `schema=1`, Version, Vorabstatus, `board=d1_mini`, `chip=esp8266`, `flash_bytes=4194304`, `led_gpio=12`, BIN-Dateiname, Bytezahl und SHA-256. Die GUI prüft diese Angaben; beliebige fremde URLs lassen sich nicht aus einem Manifest einschleusen.

## Warum ein separater Kanal?

Beim geprüften normalen GitHub-Release-Anhang führte die Weiterleitung auf einen Asset-Host ohne Browser-CORS-Freigabe. Der GitHub-Raw-Host wurde dagegen lesend mit HTTPS und `Access-Control-Allow-Origin: *` bestätigt. Kein fremder CORS-Proxy wird benötigt. Ein alternatives Hosting müsste HTTPS und CORS unterstützen und bewusst im Quellcode konfiguriert werden.

## Veröffentlichung vorbereiten

`python3 scripts/package_public.py` erzeugt unter `dist/2.0.10/esp8266/update-channel/` die passende BIN und `preview.json` lokal. `python3 scripts/package_esp32.py` erzeugt unter `dist/2.0.10/esp32/update-channel/` die ESP32-OTA-BIN und `preview-esp32.json`. Die Skripte laden selbst nichts hoch. Der öffentliche Testkanal wird bei einer ausdrücklich beauftragten Veröffentlichung separat aktualisiert.

Wemos verwendet weiterhin `stable.json` und `preview.json`; klassische ESP32-Geräte verwenden getrennt `stable-esp32.json` und `preview-esp32.json`. Die neuen Manifeste enthalten zusätzlich `configurable_led_pin=true` und die Liste `supported_led_gpio`. So bleibt ein am Gerät gewählter Pin zulässig. `led_gpio` bezeichnet für ältere GUIs weiterhin den Standardpin. Chip und Board müssen immer passen; das ESP32-Factory-Image gehört nicht in den Updatekanal.

Nach ausdrücklicher Veröffentlichungsfreigabe: BIN zuerst auf den Kanal-Branch hochladen und öffentlich mit SHA-256 prüfen; danach das zugehörige Manifest veröffentlichen. Versionierte BINs nicht ersetzen. Bei einer Vorabversion vorhandenes `stable.json` erhalten. Für eine später ausdrücklich als stabil freigegebene Version dasselbe Schema mit `prerelease=false` als `stable.json` verwenden. Diesen Kanal bei jeder künftigen Veröffentlichung mitpflegen; Release-Anhänge allein aktualisieren ihn nicht.

Keine echte Browser-/Geräte-Installation über den Kanal ausgeführt. GUI-Abläufe wurden lokal getestet; der geräteseitige CSRF-/SHA-OTA-Pfad wurde mit erfolgreichem Folgeupdate und Neustart auf einem Wemos mit 2.0.8 geprüft. Die neuen 2.0.10-BINs sind noch nicht am Gerät getestet. SHA-256 ist keine Herausgebersignatur; die lokale HTTP-Strecke und der Besitz des GitHub-Kontos bleiben Teil des Vertrauensmodells. [Sicherheitsgrenzen](SICHERHEIT_2_0_8.md).
