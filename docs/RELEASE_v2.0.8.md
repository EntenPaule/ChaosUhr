# ChaosUhr v2.0.8 – Wemos D1 mini

Vorabversion für **Wemos D1 mini / ESP8266, 4 MiB Flash, D6/GPIO12, 20×15 WS2812B**. Die enthaltene BIN läuft auf einer Uhr. WLAN-/Admin-Passwörter werden auf dem Gerät eingerichtet und sind nicht in der BIN enthalten. Kein neues ESP32-Paket.

## Neu gegenüber 2.0.7

- Gehärteter HTTP-Parser, zentrale Anmeldung und CSRF-Schutz auch für Einstellungen und OTA; eingeschränktes WLAN-Einrichtungsportal.
- Gesalzener Admin-Passwort-Hash mit Migration des bisherigen Passworts und internem Wiederherstellungsjournal. Kein erzwungener Werksreset.
- Einstellungen als JSON sichern und mit Vorschau wiederherstellen; keine Passwörter im Export. Eigene `.chaos`-Dateien bleiben separate Downloads.
- Bestätigte Online-Updates aus dem GitHub-Testkanal; HTTPS-Download im Browser, SHA-256-Prüfung auf der Uhr. Für Vorabversionen das entsprechende Häkchen aktivieren.
- Strengere OTA-Prüfung von Zielhardware, Länge, CRC und Produktkennung. Beim Gerätetest gefundener Origin-Restwertfehler korrigiert und durch Regressionstest abgedeckt.

Die 30 Effekte, deutscher Wochentag/Datum, Nachtmodus, auswählbare Automatik und acht eigene Animationen bleiben enthalten. Keine ioBroker-/MQTT-Anbindung.

## Aktualisieren und Dateien

Nur die Anwendungs-BIN über den bisherigen Updater übertragen. Das ZIP enthält **dieselbe BIN**, Anleitung, passenden Quellcode, Tests, Beispielanimation und Lizenzen. `SHA256SUMS` enthält die Prüfsummen von BIN und ZIP.

Von 2.0.5/2.0.7 bleibt das vorhandene Admin-Passwort erhalten. Bei lesbarem LittleFS wird es migriert; ohne lesbaren Speicher bleibt die alte Anmeldung zunächst aktiv und die GUI meldet den ausstehenden Schritt. **Nicht ungefragt formatieren.** Von Michas Referenzversion ist erstmals die Admin-Ersteinrichtung nötig. [Upgrade-Anleitung](UPGRADE_2_0_8.md).

Nach Passwortmigration sind unveränderte 2.0.5/2.0.7 kein geeigneter Downgrade. Die neue OTA-Prüfung weist alte BINs ohne Produktkennung ab.

## Tatsächlich geprüft

- Vollständige native Tests einschließlich tatsächlichem HTTP-Parser unter Sanitizern, KDF-Vektoren, Original-2.0.5-Fixture, Speicherfehlern, Auth/CSRF, OTA-Abbrüchen und GUI-Skripten erfolgreich.
- Wemos-Build und isolierter Build der Quellen aus dem endgültigen ZIP erfolgreich. Echte BIN mit demselben OTA-Validator geprüft; manipulierte Kopie abgewiesen.
- OTA von einer eingerichteten 2.0.7 auf 2.0.8: vorhandenes Passwort und alle 15 Einstellungen übernommen, Passwortmigration bei bereits lesbarem LittleFS abgeschlossen.
- Folge-OTA über den neuen Updater mit Anmeldung, CSRF, passendem Origin und SHA-256: HTTP 200, erfolgreicher Abschluss und Neustart anhand gewechselten Sitzungstokens bestätigt. Dabei wurde die hier enthaltene korrigierte BIN installiert.
- Anmeldung, fehlendes Token, fremder Origin, gesperrte erneute Ersteinrichtung und ungültiges Image geprüft. Danach normale Anfragen ohne Origin erfolgreich; keine dauerhafte Blockade durch alte Headerwerte.
- Kleine Helligkeitsänderung gespeichert und gelesen; anschließend alle ursprünglichen Einstellungen wiederhergestellt und gelesen. Kein Werksreset, keine Speicherformatierung und kein Löschen eigener Dateien. Vor diesem Test waren keine eigenen Animationen gespeichert.
- Backup-Export/Import mit dem tatsächlich vom Gerät gelieferten GUI-Skript und Geräteeinstellungen in Node geprüft: 15 erlaubte Felder, keine Zugangsdaten/IP, Vorschau ohne Schreibzugriff, ausdrückliches Speichern mit CSRF. Das ersetzt keinen vollständigen Importtest im echten Browser.
- Nutzer bestätigt nach dem korrigierten Update Uhrzeit, deutschen Wochentag/Datum, Animationen und Weboberfläche.

Erste Anmeldung nach Neustart etwa 2,5–3,4 Sekunden einschließlich Netzwerk; danach Erfolgs-Cache. Heap-Momentaufnahmen 33832 Byte nach OTA und 33240 Byte nach API-Test. Statisches RAM 41432/81920 Byte; Programm-Flash 556307/1044464 Byte.

Offen bleiben echte Updates direkt von 2.0.5/Michas Version auf Hardware, Werksreset, WLAN-Recovery, physische Stromausfälle und Langzeitbetrieb. Online-Installation über den öffentlichen Kanal wurde nicht im echten Browser ausgeführt; lokaler GUI-Ablauf und geräteseitiger OTA-Pfad wurden getrennt geprüft. Deshalb **Pre-release**, keine allgemeine Produktionsfreigabe.

## Identität und Sicherheitsgrenzen

`chaosuhr-2.0.8-esp8266.bin`, **560464 Byte**.

SHA-256: `ad323ec69cd2401217cc997b901a2c8a9e1ee9ce508bce992d3d626fa936efe1`.

Genau diese BIN wurde auf der Testuhr installiert. Dokumentation und ZIP werden für die Veröffentlichung ergänzt; die getestete Firmware bleibt unverändert.

Lokale HTTP-Verbindung bleibt unverschlüsselt; keine verpflichtende Herausgebersignatur. Nur im vertrauenswürdigen LAN betreiben. [Sicherheitsgrenzen](SICHERHEIT_2_0_8.md), [Updatekanal](GITHUB_UPDATES.md).

Fork von [oOmicha1986Oo/ChaosUhr](https://github.com/oOmicha1986Oo/ChaosUhr), GNU GPL Version 3. Herkunft und weitere Lizenzen in `THIRD_PARTY.md`.
