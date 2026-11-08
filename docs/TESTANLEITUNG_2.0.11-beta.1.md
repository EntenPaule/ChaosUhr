# ChaosUhr 2.0.11-beta.1 – Wemos-Testanleitung

Nur Wemos D1 mini / ESP8266, 4 MiB Flash, bestehendes 1-MiB-LittleFS-Layout. Keine ESP32-Datei enthalten. Die gemeinsame Versionsdefinition gilt auch für den erhaltenen ESP32-Quellcode, dieser wurde für diese Beta nicht gebaut oder getestet.

**Diagnose-Beta: Der gemeldete MQTT-007-Fehler auf echter Hardware ist noch nicht nachgewiesen behoben.** Die Konfigurationsregeln wurden nicht gelockert. Bei einer Ablehnung nennt MQTT-022 nun das betroffene Feld ohne Inhalt. Bitte Fehlercode, HTTP-Status und Feldkennung zurückmelden; keine Passwörter, Cookies, Tokens oder vollständigen Netzwerkanfragen teilen.

## Installation

1. Uhreinstellungen über die Weboberfläche sichern; eigene Animationen separat herunterladen. MQTT-Zugangsdaten bleiben privat. Stabile Stromversorgung sicherstellen.
2. SHA-256 mit `sha256sum -c SHA256SUMS_2.0.11-beta.1.txt` prüfen. Die Prüfsummen im ZIP prüfen den Inhalt; die externe Liste prüft zusätzlich das ZIP.
3. Auf der bestehenden Uhr als Admin anmelden, OTA-Seite öffnen und **ChaosUhr_2.0.11-beta.1_Wemos_OTA.bin** auswählen. Keine ZIP-Datei hochladen. Neustart abwarten, Browser neu laden und erneut anmelden.
4. Firmwareanzeige und Supportdiagnose müssen **2.0.11-beta.1** melden. Nicht die alte unnummerierte 2.0.11-BIN verwenden.
5. USB als Wiederherstellungsweg: ein bereits eingerichtetes ESP8266/esptool-Werkzeug für den Wemos verwenden und ausschließlich diese BIN bei Offset **0x0** schreiben. Kein Full-Erase, kein Dateisystem-Image; dadurch werden WLAN, EEPROM und Animationen nicht absichtlich gelöscht. Port vor Ort bestimmen. USB wurde für diese Beta nicht an Hardware geprüft.

Die Uhr verwendet lokales HTTP: Passwort, Cookie und MQTT-Daten sind ohne TLS im Netz nicht vertraulich. HttpOnly, SameSite=Strict und CSRF schützen nicht gegen Mitschneiden/Manipulation durch einen Angreifer im Netz. Nicht ins Internet weiterleiten.

## Hardwarecheckliste – noch offen

- [ ] WLAN verbindet sich nach Update und Stromneustart; Uhrzeit/Datum/Animationen unverändert.
- [ ] Admin anmelden; Status rechts oben; Logout beendet die Sitzung, erneute Anmeldung funktioniert.
- [ ] Ohne Cookie AUTH-001, mit widerrufenem/Neustart-Cookie AUTH-021. Das sind erwartete Schutzreaktionen.
- [ ] Nach 30 Minuten ohne Benutzeraktivität erneute Anmeldung; reine Statusabfragen verlängern nicht. Ablauf wird als AUTH-020 gemeldet.
- [ ] Ungültiges CSRF bleibt abgewiesen; gültige Sitzung und CSRF erlauben MQTT-Speichern.
- [ ] MQTT-Minimalkonfiguration: eigener Broker, Port, Discovery aus, Präfix **homeassistant**, optionale Felder leer. Zunächst nur speichern. Bei Fehler **Code, HTTP-Status und Feldkennung** notieren.
- [ ] Danach getrennt „Status aktualisieren“/Seite neu laden: enabled und Einstellungen erhalten? Ein leeres Passwortfeld bedeutet Behalten, nicht Löschen.
- [ ] Stromneustart getrennt prüfen: WLAN, MQTT-Konfiguration und Animationen erhalten?
- [ ] Erst danach Brokerverbindung prüfen. Erfolgreiches Speichern bedeutet noch keine erfolgreiche DNS-/TCP-/MQTT-Anmeldung. Unerreichbarer Broker darf gespeicherte Konfiguration nicht löschen.
- [ ] Wetter/Sensoren und bestehende Animationen prüfen, ohne neue Infrastruktur einzurichten.
- [ ] GUI zeigt den Hinweis zur Sekundenleiste unter 5 % einschließlich Nachtmodus; Verhalten ist unverändert.
- [ ] Weiteres OTA, Langzeitbetrieb und freier Heap unter Last prüfen.

Kein Werksreset und keine Formatierung zur Diagnose verwenden. Der frühere Origin/Host-403-Fix ist unabhängig geprüft. MQTT-022 plus Feld dient zur Eingrenzung des neuen Hardwarefehlers und ist noch kein Nachweis seiner Behebung.

## Versionsregeln

Entwicklung: `2.0.11-dev.N`, Hardwaretests: `2.0.11-beta.N`, Releasekandidat: `2.0.11-rc.N`. Nur nach Hardwaretests und ausdrücklicher Freigabe darf `2.0.11` als offizielles Release verwendet werden. Jede an Tester verteilte Codeänderung benötigt eine neue Nummer. Vorhandene Pakete bleiben unverändert. Updatevergleiche folgen SemVer; numerische Beta-Suffixe werden numerisch verglichen, Stable steht über jeder Vorabversion desselben Versionskerns. Entwicklungsstände gehören nicht in den produktiven Updatekanal. Der manuelle OTA-Upload prüft Bild, Zielchip und Integrität, erzwingt aber keine Versions-Rollback-Sperre; der GitHub-Updater bietet ältere/gleichwertige Versionen nicht an.
