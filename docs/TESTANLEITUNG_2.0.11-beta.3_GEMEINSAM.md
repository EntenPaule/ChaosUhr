# ChaosUhr 2.0.11-beta.3 – Testanleitung für beide Plattformen

## Zielboard bestimmen

Wemos D1 mini / ESP8266: Wemos-OTA-Datei. Klassischer ESP32-WROOM-32 / esp32dev mit 4 MiB Flash: ESP32-OTA oder ESP32-Factory. Keine ESP32-S3-/WT32-Display-Firmware. LED-Datenpin passend zur Verkabelung wählen; Standard D6/GPIO12 beim Wemos, GPIO32 beim ESP32.

Die Wemos-BIN ist bereits hardwaregetestet und unverändert. ESP32 ist nur gebaut/offline validiert, mit offenen AP- und WLAN-Persistenztests.

## Vor Installation

- Passende Datei auswählen und `sha256sum -c SHA256SUMS_2.0.11-beta.3_GEMEINSAM.txt` ausführen.
- Uhreinstellungen sichern und eigene Animationen separat herunterladen; MQTT-Zugangsdaten privat bereithalten.
- Stabile Stromversorgung verwenden und USB-Wiederherstellung ermöglichen.

## Web-OTA

Als Admin anmelden, die OTA-Seite öffnen und die zum Board passende **OTA-BIN** wählen. Keine Factory-Datei und kein ZIP hochladen. Neustart abwarten, neu anmelden und Version 2.0.11-beta.3 prüfen. AUTH-021 für die alte Sitzung nach einem Neustart ist korrekt.

ESP32-OTA: 1965072 Byte, Slot 1966080 Byte, nur 1008 Byte Reserve. Die echte BIN wurde geprüft; nicht durch eine selbst neu gebaute Datei unbekannter Größe ersetzen.

## ESP32 vollständig über USB installieren

`ChaosUhr_2.0.11-beta.3_ESP32_Factory.bin` ist ein vollständiges 4-MiB-Image, zu schreiben ab **0x0**. Es überschreibt NVS/WLAN, Uhr-/Admin-Konfiguration und Animationen. Nur für eine beabsichtigte vollständige Neuinstallation verwenden.

Mit eingerichtetem esptool, den Port lokal ersetzen:

```sh
esptool --chip esp32 --port /dev/ttyUSB0 write-flash 0x0 ChaosUhr_2.0.11-beta.3_ESP32_Factory.bin
```

Die tatsächlich geprüften Komponenten liegen bei Bootloader 0x1000, Partitionstabelle 0x8000, boot_app0 0xe000 und Anwendung 0x10000. Die Metadaten und binäre Partitionstabelle wurden aus dem gepinnten Build gelesen; das vollständige Image benötigt nur den einen Schreibvorgang ab 0x0. **Factory niemals für Web-OTA verwenden.**

## ESP32-Hardwarecheckliste (offen)

- [ ] Setup-AP sichtbar; Client kann sich verbinden und das Portal öffnen.
- [ ] Admin-Passwort und WLAN einrichten, Start im normalen Netz prüfen.
- [ ] WLAN speichern; Gerät 30 Sekunden vollständig stromlos machen; WLAN-Daten, Einstellungen und automatische Wiederverbindung prüfen.
- [ ] Matrix mit passender Stromversorgung angeschlossen starten; Pin, Ausrichtung, Uhr/Datum und Animationen prüfen.
- [ ] LittleFS bei Ersteinrichtung ausdrücklich einrichten, MQTT speichern, neu laden und nach Kaltstart wieder verbinden.
- [ ] Broker über DNS und IPv4, Wetter-/Sensorwerte und Recovery prüfen.
- [ ] Login, Logout, erneute Anmeldung, Auto-Logout nach 30 Minuten ohne Verlängerung durch Hintergrund-GETs und CSRF prüfen.
- [ ] Geschütztes ESP32-OTA und Neustart prüfen; Konfiguration und Animationen bleiben bei OTA erhalten.
- [ ] Langzeit-/Heapverhalten und WLAN-Ausfall/Wiederverbindung prüfen.

Die früheren ESP32-AP-/Persistenzprobleme bleiben offen, bis diese Tests tatsächlich bestätigt wurden. Die Firmware meldet auf ESP32 `reset.available: false`; die RTC-Diagnose gehört ausschließlich zum ESP8266.

## Wemos-Status

Der Nutzer bestätigt Beta.3 auf echter Hardware. Der frühere Beta.2-Befund umfasst MQTT-Speichern ohne Absturz, ioBroker über DNS/IPv4, 30 Sekunden Stromtrennung, erhaltene WLAN-/MQTT-Daten, automatische MQTT-Verbindung sowie Wetterdaten und Wetteranimationen. Der Stacküberlauf als damalige Absturzursache bleibt sehr wahrscheinlich, aber nicht endgültig bewiesen. Einmaliges MQTT-011 und erwartetes AUTH-021 nach Neustart sind dokumentiert.

Lokales HTTP/MQTT hat keine TLS-Vertraulichkeit. Roh-Stackdumps können Secrets enthalten und bleiben privat; zuerst die bereinigte Supportdiagnose verwenden. Keine automatischen Änderungen an Brokern oder Netzwerkgeräten vornehmen.
