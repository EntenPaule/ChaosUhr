# ChaosUhr 2.0.7 – Wemos D1 mini / ESP8266

Diese Firmware enthält keine persönlichen Admin-, WLAN- oder Einrichtungspasswörter. Sie ist der ESP8266-Standard-Build für die eigene Uhr und die Weitergabe. Jeder Nutzer legt bei der ersten Einrichtung sein eigenes Admin-Passwort fest. Die 30 Effekte und der deutsche Wochentag-/Datums-Lauftext aus 2.0.4-dev sind enthalten. Eine separat zu kompilierende ESP32-Portierung aus derselben Codebasis ist unter [ESP32_FIRMWARE.md](ESP32_FIRMWARE.md) beschrieben; dafür gelten andere BINs und Anschlüsse.

Zielhardware: Wemos D1 mini / ESP8266 mit **4 MiB Flash**, **20×15 WS2812B**, Datenpin **D6/GPIO12**, 5-V-LED-Versorgung. Standardausrichtung: oben rechts, zeilenweise, Zickzack. Die Firmware begrenzt die geschätzte LED-Leistung softwareseitig auf 5 V/1,5 A; sie ersetzt keine passende Versorgung/Absicherung.

## Neu in 2.0.7

Die Automatik bietet eine eigene Animationen-Auswahl und zwei Reihenfolgen: zufällige Runden ohne Wiederholung oder der Reihe nach. Zwischen zwei Animationen erscheint immer die Uhr. Einrichtung, Übernahme alter Einstellungen und API: [AUTOMATIK_2_0_7.md](AUTOMATIK_2_0_7.md).

Zusätzlich können **acht eigene `.chaos`-Animationen über die GUI hochgeladen, in der Automatik deaktiviert, heruntergeladen und gelöscht** werden. Dateiformat, Beispiel und Anleitung zum Erstellen mit ChatGPT: [ANIMATIONEN_FORMAT.md](ANIMATIONEN_FORMAT.md), **CHATGPT_ANIMATION_PROMPT.txt**, `examples/Herz.chaos`. Den Animationsspeicher bei Bedarf einmalig in der GUI einrichten. Dateien werden vollständig geprüft, bevor sie verfügbar werden.

## Dateien zum Teilen

- `chaosuhr-2.0.7-esp8266.bin`: öffentliche Firmware, 4-MiB-/1-MiB-Dateisystem-Layout.
- `chaosuhr-2.0.7-esp8266.zip`: BIN, Anleitung zur Einrichtung/Automatik, Prüfsummen, GPL-/Drittlizenzhinweise und der zugehörige bereinigte Quellcode.
- `SHA256SUMS`: Prüfsummen von BIN und Paket.

Im Projekt liegen die Dateien unter `dist/2.0.7/esp8266/`. Für die Weitergabe vorzugsweise das vollständige ZIP mit Anleitung und Quellcode verwenden. Die persönliche `2.0.4-dev`-BIN und vollständige Flash-Dumps enthalten Zugangsdaten und gehören nicht in dieses Paket. Vorhandene öffentliche Pakete älterer Versionen bleiben in ihren bisherigen Ordnern erhalten.

## Erste Einrichtung

1. Die Public-BIN auf die passende Hardware flashen. Eine vorhandene OTA-Funktion kann verwendet werden, sofern Zielboard, Speicherlayout und freier OTA-Platz passen. Ein normaler Build führt keinen Upload aus.
2. Solange noch kein Public-Admin gespeichert ist, zeigt die LED-Matrix **SETUP** und darunter einen wiederholten **zwölfstelligen Zahlencode**. Der Code wird auf dem Gerät zufällig erzeugt; er steht nicht in der verteilten BIN.
3. Mit dem WLAN **ChaosUhr-Setup-XXXXXX** verbinden. Die Endung unterscheidet Geräte. Das WLAN-Passwort ist der auf der Uhr angezeigte Zahlencode.
4. Im Browser **http://192.168.4.1/** öffnen. Ein eigenes Admin-Passwort setzen und wiederholen: **16–64 druckbare Zeichen**, ohne Umlaute. Buchstaben, Ziffern, Leerzeichen und Sonderzeichen sind möglich.
5. Nach dem Speichern startet die Uhr neu. Bereits im Wemos gespeicherte Heim-WLAN-Daten werden weiterverwendet. Bei einer neuen oder zurückgesetzten Uhr erneut mit dem gleichen Einrichtungs-WLAN und Zahlencode verbinden und im WLAN-Portal das Heim-WLAN auswählen und dessen Passwort speichern.
6. Danach die Weboberfläche unter **http://chaosuhr.local/** bzw. der im Router angezeigten Geräte-IP öffnen. Benutzername: **admin**, Passwort: das selbst gewählte Admin-Passwort. Es schützt Weboberfläche, API und den Anwendungs-Updater `/update`.

Bei einem späteren WLAN-Ausfall öffnet sich nach ungefähr 30 Sekunden erneut das passwortgeschützte Einrichtungsportal. Die Uhr zeigt dabei wieder ihren Einrichtungscode. Das Passwort der WLAN-Einrichtung ist vom Admin-Passwort getrennt; der zufällige Code wird bei der Ersteinrichtung im Gerät gespeichert. Firmware-Updates ersetzen diese gespeicherten Zugangsdaten nicht.

Die Ersteinrichtung ist ausschließlich über den Einrichtungs-Access-Point möglich. Ein vorhandener Admin kann über das Einrichtungsformular nicht überschrieben werden. Während die WLAN-Konfiguration läuft, ist der separate ungeschützte Firmware-Updater von WiFiManager gesperrt; Updates erfolgen nach Anmeldung über die normale Weboberfläche.

## Auf Werkseinstellungen zurücksetzen

1. In der normalen Weboberfläche als Admin anmelden.
2. Unter **Werkseinstellungen** die Checkbox zum Löschen bestätigen.
3. **Auf Werkseinstellungen zurücksetzen** drücken und den zusätzlichen Bestätigungsdialog annehmen.

Gelöscht werden:

- sämtliche Uhr-Einstellungen, einschließlich Helligkeit, Nachtmodus, Effektwahl und Matrixausrichtung;
- alle eigenen hochgeladenen Animationen;
- das gespeicherte Admin-Passwort und der bisherige Einrichtungscode;
- die dauerhaft gespeicherte WLAN-Konfiguration des ESP8266, einschließlich SSID und WLAN-Passwort.

Anschließend startet die Uhr neu und zeigt einen neuen Einrichtungscode. Die oben beschriebene Ersteinrichtung beginnt erneut. Ein normaler Neustart allein löscht nichts. Beim Reset wird zuerst eine persistente Markierung geschrieben; die WLAN-Konfiguration wird vor dem übrigen EEPROM-Inhalt gelöscht. Ein noch markierter Reset wird beim nächsten Start fortgesetzt.

Der Reset verlangt die Admin-Anmeldung, eine ausdrückliche Bestätigung und ein an die aktuelle Sitzung gebundenes Anfragetoken. Das ist kein Weg, ein vergessenes Admin-Passwort ohne Anmeldung zu umgehen. In diesem Fall ist ein vollständiges Löschen per USB und anschließendes erneutes Flashen der Rückweg.

## Selbst bauen

```bash
pio run
python3 scripts/package_public.py
```

PlatformIO-Plattform `espressif8266 4.2.1`, Arduino-Core `3.1.2`, FastLED `3.10.9`, WiFiManager `2.0.17`. Die Standardumgebung ist `d1_mini_public`, optional ausdrücklich mit `pio run -e d1_mini_public` wählbar. Die zusätzliche Umgebung `esp32dev_public` erzeugt einen anderen Hardware-Build. Beide verwenden `CHAOSUHR_PUBLIC_BUILD=1`, binden `include/secrets.local.h` nicht ein und führen keinen persönlichen Passwortgenerator aus. Für 2.0.7 wird ausschließlich das Wemos-Paket bereitgestellt; die Upload-Verwaltung ist bislang nur dort enthalten.

Die Admin-/Einrichtungsdaten werden erst auf dem Gerät im EEPROM-Bereich ab Offset 32 gespeichert. Uhr-Einstellungen liegen ab Offset 0; 2.0.7 ergänzt den kompatiblen alten 20-Byte-Block in den Bytes 20–27 um die Automatik-Konfiguration und in Bytes 28–31 um die Upload-Auswahl. Eigene Dateien liegen im separaten LittleFS-Dateisystem. Die WLAN-Daten verwaltet weiterhin der ESP8266-SDK. Der gesamte Flash eines eingerichteten Geräts enthält daher weiterhin persönliche Daten, auch wenn die verteilte BIN keine enthält.

Lokale Tests:

```bash
python3 tests/run_host_tests.py
```

Benötigt Python 3, Node.js und einen C++17-Compiler mit Sanitizern. Abgedeckt sind zusätzlich Ersteinrichtung, Passwortbestätigung, Zugriff über AP statt LAN, Authentifizierung/Anfragetoken, Reset-Bestätigung, fehlgeschlagene Speicheroperationen, Wiederaufnahme eines unterbrochenen Resets und Löschung von Admin-/WLAN-Daten gegen lokale Test-Doubles. Ein Host-Test löscht keine Daten auf einer echten Uhr.

## Teststand

Die veröffentlichte 2.0.7-BIN wurde auf einem Wemos D1 mini per Web-OTA installiert. Firmwarestart, Admin-Anmeldung und die Übernahme vorhandener Uhreinstellungen wurden geprüft. Der Nutzer hat den Animationsspeicher eingerichtet und den Herz-Upload bestätigt; anschließend waren die zwei Bilder lesbar und der Originaldownload bytegleich zum Beispiel. Die BIN im Paket ist unverändert dieselbe Datei. Details und Prüfsumme: [Release- und Teststand](RELEASE_v2.0.7.md).

Lokale Tests prüfen die echten Szenen-/API-/Upload-Funktionen, Dateiformat, Dateisystemfehler, Auswahllisten, zufällige Runden, Reihenfolge, alte 20-/28-Byte-Einstellungsdaten und unveränderten Admin-Speicher. Vollständige Automatik-Runden sind am Gerät noch nicht bestätigt. Auch Upload-Löschen, Werksreset, Stromunterbrechung, WLAN-Recovery und Langzeitbetrieb bleiben auf echter Hardware zu prüfen; daher erscheint 2.0.7 als Vorabversion. HTTP Basic Auth verwendet HTTP ohne TLS; die Weboberfläche gehört ins vertrauenswürdige LAN.

Ursprüngliches Projekt: https://github.com/oOmicha1986Oo/ChaosUhr (Micha1986 / oOmicha1986Oo). Herkunft und Anpassungen der übernommenen Bilder/Effekte: `THIRD_PARTY.md`. Lizenz: GNU GPL Version 3, siehe `LICENSE`; FastLED-Lizenzhinweis unter `third_party/FastLED-LICENSE`.
