# ChaosUhr 2.0.11-beta.3 – Wemos-Testanleitung

Wemos D1 mini / ESP8266, 4 MiB Flash, vorhandenes 1-MiB-LittleFS-Layout. Dieses Paket enthält ausschließlich Wemos-Firmware. Beta.2 hat MQTT-Speichern, ioBroker über IPv4/DNS, Kaltstart nach 30 Sekunden Stromtrennung und MQTT-Wetter am Gerät bestanden. Beta.3 aktualisiert die GUI-Erklärung und ergänzt eine lokale Ablaufvorschau; die Anzeige-, Wetter-, Speicher- und Sitzungslogik bleiben erhalten.

## Installation

1. Einstellungen sichern; eigene Animationen separat herunterladen. Stabile Stromversorgung verwenden.
2. Prüfsummen mit `sha256sum -c SHA256SUMS_2.0.11-beta.3.txt` prüfen. Die externe Liste prüft zusätzlich das ZIP, die innere Liste die enthaltenen Dateien.
3. Als Admin anmelden, OTA-Seite öffnen und **ChaosUhr_2.0.11-beta.3_Wemos_OTA.bin** hochladen. Keine ZIP-Datei auswählen. Nach Neustart erneut anmelden; AUTH-021 für ein altes Cookie ist erwartbar.
4. Oberfläche und Supportdiagnose müssen **2.0.11-beta.3** melden. Kein Werksreset, Full-Erase oder Dateisystemformatieren.
5. USB-Wiederherstellung bei Bedarf: dieselbe Wemos-BIN mit einem eingerichteten ESP8266-Werkzeug bei **0x0** schreiben; Port vor Ort wählen. Kein zusätzliches Dateisystemimage.

## Gezielter GUI-Test

- [ ] Modus Automatik; Vorschau für Uhr → Wetter → Sensorwert → Animation prüfen. Abgewählte Arten verschwinden aus der Vorschau.
- [ ] Dauer ändern: Vorschau nennt die gewählte Dauer. Änderungen wirken auf der Uhr erst nach „Speichern“.
- [ ] Modus „Nur Uhrzeit“ und „Gewählte Animation“ zeigen eine dauerhafte Einzelanzeige in der Vorschau.
- [ ] Wetter/Sensoren als bedingt verfügbar erkennen: nur bei aktiviertem MQTT und frischen Daten. Keine verfügbare Seite führt zur Uhr als Fallback.
- [ ] Mehrere Sensoren erscheinen über mehrere Runden, pro Sensorseite ein aktueller Sensor. Animationen folgen ihrer eigenen gewählten Reihenfolge.
- [ ] Nachtmodus: gleicher Ablauf mit Nachthelligkeit; Wetter läuft weiter. Bestehende Regen-, Schnee- und Gewitterdarstellung unverändert.
- [ ] Sekundenleistenhinweis bei geringer Helligkeit verständlich. Es gibt im Code keine feste 5-%-Abschaltung. Keine Änderung des bisherigen Anzeigeverhaltens.

## Weitere Prüfungen

- [ ] MQTT-Speichern ohne Neustart; MQTT/WLAN/Einstellungen/Animationen bleiben erhalten. Nur bei neuer Auffälligkeit zusätzliche Diagnose.
- [ ] Login, Logout und 30 Minuten Inaktivität; reine Statusabfragen verlängern nicht. AUTH-001 ohne Cookie, AUTH-021 nach Neustart, AUTH-020 bei Ablauf sind erwartete Schutzreaktionen.
- [ ] Nachfolgendes OTA, Langzeitbetrieb und freier Heap.

Einmaliges MQTT-011 aus Beta.2 ist dokumentiert. Ohne weiteren konkreten Hinweis keine Konfiguration löschen oder Infrastruktur ändern. Bei wiederholtem Fehler zuerst die bereinigte Supportdiagnose verwenden. Roh-Stackdumps privat halten; sie können Zugangsdaten enthalten. Das passende ELF für lokale Auswertung liegt im Entwicklungsprojekt unter `.pio/debug/2.0.11-beta.3/firmware.elf`.

Lokales HTTP bietet keine TLS-Vertraulichkeit; bestehende Cookie-/CSRF-Schutzmaßnahmen bleiben aktiv. Keine offizielle Veröffentlichung dieser Beta, produktiver Updatekanal unverändert.
