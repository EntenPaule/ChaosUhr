# Änderungen

## 2.0.11 – lokale Entwicklung, nicht veröffentlicht

- Vollständiges lokales Testpaket mit eindeutig benannten OTA-Dateien für beide Controller und vollständigem 4-MiB-ESP32-Factory-Image für USB. Komponenten/Offsets aus tatsächlichen PlatformIO-Metadaten, binäre Partitionstabelle einschließlich MD5 geprüft, alle Komponenten und leeren Flash-Bereiche byteweise verifiziert. Flash-Anleitung, Checkliste, Einschränkungen, Prüfsummen, Quellcode und Lizenzen enthalten; keine Änderung der Updatekanäle.

- ESP32: Flash/NVS als WLAN-Speicher vor dem ersten Funkstart wählen; gespeicherte Konfiguration statt aktuell verbundener SSID prüfen. Routerausfall löscht keine Zugangsdaten, die 30-Sekunden-Frist und das geschützte Recovery-Portal bleiben erhalten.
- Einheitliche stabile Fehlercodes mit deutscher Beschreibung, technischer Ursache und Handlungsempfehlung für API, GUI und serielle Diagnose. Acht Einträge im RAM, ohne laufende Flash-Schreibzugriffe; Codevertrag und automatisch geprüfte Referenz.
- MQTT: keine leeren Fehlermeldungen bei HTTP 401, fehlgeschlagenen Antworten oder Sitzungsfehlern. Speicherfehler getrennt von DNS/TCP/Timeout, Brokerablehnung, Berechtigungen und Verbindungsabbruch melden; unbekannte Ursachen ausdrücklich allgemein lassen.
- MQTT-Dateispeicherung: einzelne LittleFS-Operationen unterscheiden und vorhandene Konfiguration bei Fehlern erhalten. Keine automatische Formatierung oder Löschung.
- Bereinigte, kopierbare Supportdiagnose über GUI und authentifiziertes `GET /api/diagnostics`, ohne Passwörter, Tokens, Topics oder Messwerte.
- Regressionstests für Coldstart-Modell, tatsächlichen MQTT-Controller mit simuliertem Transport, Dateifehler, 401-/GUI-Verhalten, Parser und Fehlercodes. Reale Hardwaretests stehen aus; keine Geräteinstallation oder Veröffentlichung.

[Entwicklungsbericht und Testergebnisse](docs/ENTWICKLUNGSBERICHT_2_0_11.md), [Fehlercodeliste](docs/FEHLERCODES.md).

## 2.0.10 – stabile Freigabe für Wemos und klassischen ESP32

- MQTT-Wetter und vier Sensorplätze für HA/ioBroker; wählbare Anzeigearten einschließlich nur Uhr/Wetter.
- LED-Datenpin über GUI mit gespeichertem Wert und bestätigtem Neustart.
- Sekundenleiste schaltbar, echte Kleinbuchstaben und Wochentag-Kurzform, eigene IP nach WLAN-Verbindung.
- ESP32 mit Uploads, Passwortmigration/Journal, abgesichertem OTA und eigenem Updatekanal.
- Einstellungssicherung Schema 2 mit Import alter Sicherungen, ohne Netzwerkzugänge oder LED-Pin.
- Separate Pakete mit passenden Quellen und getrennte GitHub-Updatekanäle; Installation nach Bestätigung in der GUI.

Neue Firmware noch nicht am Gerät getestet. [Releasebericht](docs/RELEASE_v2.0.10.md), [Upgrade](docs/UPGRADE_2_0_10.md), [MQTT](docs/MQTT_2_0_10.md).

## 2.0.8 – Wemos / ESP8266

- Härtung von HTTP-Parser, zentraler Anmeldung, CSRF für Einstellungen/OTA und WLAN-Portal.
- Gesalzener Passwort-Verifier mit erhaltender Migration, internem Journal und Tests aus Original-2.0.5-Daten.
- Kontrollierter OTA-Abschluss, Image-/CRC-/Produkt-/optionale SHA-256-Prüfung; keine rohen Dateisystem-Uploads.
- JSON-Einstellungssicherung mit Import-Vorschau und Zuordnung eigener Animationen über Dateifingerabdrücke.
- GUI für Online-Updates nach ausdrücklicher Bestätigung über einen statischen GitHub-Kanal; 2.0.8 wird als Vorabversion im Testkanal angeboten.
- Beim Gerätetest gefundene Origin-Restwerte im ESP8266-Stringpuffer bereinigt; Regressionstest bildet das tatsächliche `String::clear()`-Verhalten nach.
- Erweiterte Tests und Prüfung der Weitergabedateien. Nur Wemos; OTA von 2.0.7, Passwortmigration, Folge-OTA und sichtbare Anzeige auf einer Uhr bestätigt. [Testumfang](docs/RELEASE_v2.0.8.md).

HTTP bleibt lokal unverschlüsselt; Signaturpflicht ist nicht aktiviert. [Upgrade](docs/UPGRADE_2_0_8.md), [Sicherheit](docs/SICHERHEIT_2_0_8.md), [Updatekanal](docs/GITHUB_UPDATES.md).

## 2.0.7 – Wemos / ESP8266

- Gewünschte Animationen für die Automatik auswählen, einschließlich eigener Uploads.
- Zufällige Runden ohne Wiederholung oder Wiedergabe in Katalogreihenfolge; zwischen Animationen erscheint die Uhr.
- Bis zu acht eigene `.chaos`-Dateien in der Weboberfläche hochladen, herunterladen, deaktivieren und löschen.
- Strikt geprüftes Textformat für 20×15 Pixel, bis zu 16 Farben und 64 Bilder; Herz-Beispiel und ChatGPT-Vorlage enthalten.
- Übernahme alter Einstellungen bei unverändertem Admin-Speicher; gesonderter LittleFS-Speicher ohne automatische Formatierung.
- Erweiterte lokale Tests für Wiedergabe, Migration, Uploads, Speicherfehler und Weboberfläche.

Vorabversion: OTA-Start und Herz-Upload auf einem Wemos bestätigt; vollständiger Teststand in [docs/RELEASE_v2.0.7.md](docs/RELEASE_v2.0.7.md). Kein ESP32-Paket dieses Releases.

## Bisheriger Stand der Neufassung

- PlatformIO-Build, nicht blockierende Animationen, 30 Effekte mit Originalbildern.
- Deutsche Wochentage und Datum, NTP-Zeit mit deutscher Sommer-/Winterzeit.
- Weboberfläche für Helligkeit, Nachtmodus, Tempo und Matrixausrichtung.
- Admin-Ersteinrichtung ohne festes Passwort, geschütztes Web-OTA und bestätigter Werksreset einschließlich WLAN-Daten.
- Separate klassische ESP32-Portierung als 2.0.6 auf 2.0.5-Basis; deren Betrieb wurde später vom Nutzer bestätigt. Diese Rückmeldung gilt nicht für den neuen 2.0.10-Build.

MQTT-Datenempfang für ioBroker/Home Assistant ist seit 2.0.10 enthalten; MQTT-Fernsteuerung von Einstellungen ist nicht implementiert.
