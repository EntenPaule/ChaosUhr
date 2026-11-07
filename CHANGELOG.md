# Änderungen

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
- Separate klassische ESP32-Portierung als 2.0.6; nur kompiliert, nicht auf ESP32-Hardware getestet. Display-/Touchboards werden nicht unterstützt.

Die Neufassung enthält bisher keine ioBroker-/MQTT-Anbindung.
