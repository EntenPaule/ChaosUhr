# ChaosUhr v2.0.7 – Wemos D1 mini

Vorabversion für **Wemos D1 mini / ESP8266, 4 MiB Flash, D6/GPIO12, 20×15 WS2812B**. Die enthaltene BIN läuft auf einer Uhr. Sie ist kein vollständiges Flash-Abbild und enthält keine persönlichen WLAN- oder Admin-Zugangsdaten. Für ESP32 wird in diesem Release kein Paket angeboten.

## Änderungen gegenüber dem Originalprojekt

- Neue PlatformIO-Firmware mit 30 Effekten einschließlich der übernommenen Originalanimationen.
- Deutsche Wochentage und Datum, NTP-Zeit und Sommer-/Winterzeit für Deutschland.
- Weboberfläche für Helligkeit, Nachtmodus, Matrixausrichtung, Tempo und Szenendauer.
- Admin-Passwort bei der Ersteinrichtung, geschütztes Web-OTA und bestätigter Werksreset zum Löschen von Einstellungen, Zugangsdaten und eigenen Animationen.
- Automatik mit auswählbaren Animationen: zufällige vollständige Runden oder Katalogreihenfolge, jeweils mit Uhranzeige dazwischen.
- Bis zu acht eigene `.chaos`-Animationen hochladen, aus der Automatik nehmen, herunterladen und löschen; Beispiel und ChatGPT-Vorlage liegen bei.

Die bisherige ioBroker-/MQTT-Anbindung des Originalprojekts ist in dieser Neufassung nicht implementiert.

## Dateien und Einrichtung

Das ZIP enthält dieselbe BIN wie der einzelne Download, dazu Anleitung, Beispiel, passenden Quellcode und Lizenzen. `SHA256SUMS` enthält die Prüfsummen beider Downloads.

Bei einem Update einer bereits eingerichteten öffentlichen ChaosUhr-Firmware bleiben vorhandene Einstellungen und Zugangsdaten erhalten. Nach der Erstinstallation zeigt die Matrix SETUP und einen Zahlencode: Mit `ChaosUhr-Setup-XXXXXX` verbinden, diesen Code als WLAN-Passwort verwenden und unter `http://192.168.4.1/` ein eigenes Admin-Passwort setzen. Danach das Heim-WLAN einrichten. Die normale Oberfläche verwendet den Benutzernamen `admin`.

Eigene Animationen benötigen bei noch nicht eingerichtetem Speicher einmalig dessen Initialisierung über die GUI. Diese ausdrücklich bestätigte Aktion löscht vorhandene Dateien im Animationsspeicher. Vorhandene eigene Animationen vor einem Werksreset herunterladen.

## Bestätigt und noch offen

- Lokaler Wemos-Build und isolierter Build aus dem Weitergabe-ZIP erfolgreich. Native Tests und GUI-Tests erfolgreich.
- Genau diese BIN wurde per Web-OTA installiert; Start, Admin-Anmeldung und Übernahme der bisherigen Einstellungen wurden geprüft.
- Der Nutzer bestätigte Speicherinitialisierung und Herz-Upload. Zwei gültige Bilder sowie bytegleicher Originaldownload wurden anschließend gelesen.
- Vollständige geordnete/zufällige Automatik-Runden und die sichtbare Herzanimation sind am Gerät noch nicht gesondert bestätigt. Ebenso offen: echtes Löschen, Werksreset, Stromunterbrechung, WLAN-Recovery und Langzeitbetrieb.

Die Veröffentlichung erfolgt deshalb als **Pre-release**. Ein erfolgreicher lokaler Test ist kein Ersatz für einen Gerätetest.

## Identität der getesteten BIN

Datei: `chaosuhr-2.0.7-esp8266.bin`, 539376 Bytes.

SHA-256: `ce3a7db86fb17ca7439f5656a4039c196dbee8395139a771f222f7889f1b5fba`.

Die Firmware bleibt gegenüber dem Gerätetest unverändert. Für die Veröffentlichung werden Dokumentation, Testangaben und ZIP-Prüfsumme aktualisiert.

## Herkunft und Lizenz

Fork von [oOmicha1986Oo/ChaosUhr](https://github.com/oOmicha1986Oo/ChaosUhr). Originalanimationen aus Commit `51aef401db22ef0fba3e5e334f141bf794d7505a`, Autor Micha1986 / oOmicha1986Oo. GNU GPL Version 3; Einzelheiten in `LICENSE` und `THIRD_PARTY.md`.
