# GitHub-Updatekanal

Seit 2.0.8 lädt die GUI Updates im Browser über HTTPS und überträgt sie anschließend mit Anmeldung, CSRF-Token und SHA-256 zur lokalen Uhr. Auf dem ESP8266 entsteht kein zusätzlicher TLS-Verbindungspuffer. Das lokale Passwort wird nicht an GitHub gesendet. 2.0.10 erweitert dies um klassische ESP32-Ziele und auswählbare LED-Pins.

Feste Basis: `https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/`

Der separate Branch `firmware-updates` enthält versionierte Anwendungs-BINs und die Manifeste. Stable bietet **2.0.11** über `stable.json` (Wemos) und `stable-esp32.json` (ESP32) an. Preview bleibt unverändert auf **2.0.11-beta.3**, jeweils `prerelease:true`. Bei aktiviertem Preview wird der neuere passende Kandidat aus beiden Kanälen gewählt; damit bleibt 2.0.11 Stable vor Beta.3. Ein GitHub-Release aktualisiert diese Dateien nicht von selbst.

- Wemos Stable: https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/stable.json
- ESP32 Stable: https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/stable-esp32.json
- Wemos Preview: https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/preview.json
- ESP32 Preview: https://raw.githubusercontent.com/EntenPaule/ChaosUhr/firmware-updates/preview-esp32.json

Die Firmware liest diese URLs aus `include/ota_ui.h` (`UPDATE_BASE`, `checkOnlineUpdates`) und lädt BINs relativ zu derselben Basis. Deshalb werden die tatsächlich veröffentlichten Release-Assets bytegleich in den Kanal kopiert; beliebige Release-URLs im Manifest wären mit vorhandenen Firmwares nicht kompatibel.

Manifest: `format=chaosuhr-update`, `schema=1`, Version, Vorabstatus, `board=d1_mini`, `chip=esp8266`, `flash_bytes=4194304`, `led_gpio=12`, BIN-Dateiname, Bytezahl und SHA-256. Die GUI prüft diese Angaben; beliebige fremde URLs lassen sich nicht aus einem Manifest einschleusen.

## Warum ein separater Kanal?

Beim geprüften normalen GitHub-Release-Anhang führte die Weiterleitung auf einen Asset-Host ohne Browser-CORS-Freigabe. Der GitHub-Raw-Host wurde dagegen lesend mit HTTPS und `Access-Control-Allow-Origin: *` bestätigt. Kein fremder CORS-Proxy wird benötigt. Ein alternatives Hosting müsste HTTPS und CORS unterstützen und bewusst im Quellcode konfiguriert werden.

## Verbindliche Veröffentlichung

[Releaseprozess und Freigaberegel](RELEASEPROZESS.md) sind verbindlich: kein vollständiger Stable-Abschluss ohne öffentlich geprüfte Release-Assets, beide Stable-Manifeste und echte Versionsvergleichstests. Der freigabegesteuerte Workflow `.github/workflows/stable-ota.yml` und `scripts/publish_stable_channel.py` implementieren denselben Ablauf. Historische 2.0.10-Paketierskripte sind keine aktuelle Veröffentlichungsanweisung.

Wemos verwendet weiterhin `stable.json` und `preview.json`; klassische ESP32-Geräte verwenden getrennt `stable-esp32.json` und `preview-esp32.json`. Die neuen Manifeste enthalten zusätzlich `configurable_led_pin=true` und die Liste `supported_led_gpio`. So bleibt ein am Gerät gewählter Pin zulässig. `led_gpio` bezeichnet für ältere GUIs weiterhin den Standardpin. Chip und Board müssen immer passen; das ESP32-Factory-Image gehört nicht in den Updatekanal.

Nach ausdrücklicher Veröffentlichungsfreigabe: BIN zuerst auf den Kanal-Branch hochladen und öffentlich mit SHA-256 prüfen; danach das zugehörige Manifest veröffentlichen. Versionierte BINs nicht ersetzen. Bei einer Vorabversion vorhandene stabile Kanäle erhalten. Für eine ausdrücklich als stabil freigegebene Version dasselbe Schema mit `prerelease=false` als `stable.json` beziehungsweise `stable-esp32.json` verwenden. Diesen Kanal bei jeder künftigen Veröffentlichung mitpflegen; Release-Anhänge allein aktualisieren ihn nicht.

Keine echte Browser-/Geräte-Installation über den Kanal ausgeführt. GUI-Abläufe wurden lokal getestet; der geräteseitige CSRF-/SHA-OTA-Pfad wurde mit erfolgreichem Folgeupdate und Neustart auf einem Wemos mit 2.0.8 geprüft. Die Kanalreparatur verwendet die bereits veröffentlichten 2.0.11-BINs; sie ist keine zusätzliche Hardwarebestätigung. SHA-256 ist keine Herausgebersignatur; die lokale HTTP-Strecke und der Besitz des GitHub-Kontos bleiben Teil des Vertrauensmodells. [Sicherheitsgrenzen](SICHERHEIT_2_0_8.md).


## Reparatur für 2.0.11

Ursache war kein Firmware-Vergleichsfehler: GitHub Latest zeigte 2.0.11, beide
Stable-Manifeste aber weiterhin 2.0.10. Preview zeigte Beta.3. Die existierenden
2.0.11-Release-Assets wurden übernommen, keine neue Firmware gebaut.

| Ziel | OTA-Datei | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| Wemos | chaosuhr-2.0.11-esp8266.bin | 733264 | 91ca2e0467a426fc21663f4973ca7b9066da3a72e8aafd78a71ca5aee6530083 |
| ESP32 | chaosuhr-2.0.11-esp32-ota.bin | 1906480 | 102cdef378373e6aa93687f801319a156d167f1d841d14e6cf8ef1cf37d95c15 |

Versionsmatrix bei Stable 2.0.11, auf beiden Boards mit echtem OTA-JavaScript:
2.0.10 → anbieten; 2.0.11-beta.5.3 → anbieten; 2.0.11-beta.5.2 → anbieten;
2.0.11 → kein Update; 2.0.12-beta.1 → kein Downgrade. Getestet werden Original-
Quellen aus Tag v2.0.10 und aktueller 2.0.11, jeweils Preview ein und aus.
Das sind simulierte Browseranfragen gegen die veröffentlichten Dateien,
keine Behauptung einer Installation auf einem angeschlossenen Gerät.

Korrekturcommits: BINs zuerst `20374713f750a17269ffaaef95e38ec5307ed36e`, anschließend ausschließlich Stable-Manifeste `3f4776ce40acad2341008d960013356655fcb126`. Beide kanonischen Manifeste und BINs wurden nach dem Push öffentlich geprüft; Preview-Dateien und ältere Dateien blieben bytegleich.
