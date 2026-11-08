# ChaosUhr 2.0.10 – Wemos D1 mini / ESP8266

Für Wemos D1 mini / ESP8266 mit **4 MiB Flash, 20×15 WS2812B**, Standard-Datenpin **D6/GPIO12**, weitere erlaubte Pins über die GUI. Standard: oben rechts, zeilenweise, Zickzack. Geschätztes Software-Leistungslimit: 5 V/1,5 A; eine passende Stromversorgung bleibt erforderlich.

Die BIN enthält keine persönlichen WLAN-, Admin- oder Einrichtungspasswörter. Ein vollständiger Flash-Dump einer eingerichteten Uhr enthält dagegen persönliche Daten und gehört nicht ins Weitergabepaket.

## Neu in 2.0.10

- Uhr, Animationen, Wetter und Sensoren unabhängig für die Automatik wählen; auch nur Uhr/Wetter.
- MQTT für Wetter und vier Sensorplätze, passend zu Home Assistant und ioBroker. Quelle/Broker/Topics in der GUI einrichten; [MQTT-Anleitung](MQTT_2_0_10.md).
- LED-Datenpin über Dropdown wählen; Änderung mit bestätigtem Neustart, Standard weiterhin D6/GPIO12.
- Sekundenleiste ein-/ausschalten; Wochentag ausgeschrieben mit echten Kleinbuchstaben oder zweibuchstabig.
- IP-Adresse nach erfolgreicher WLAN-Verbindung auf der Matrix.
- Einstellungssicherung Schema 2; alte Sicherungen bleiben lesbar. LED-Pin und MQTT-Verbindungsdaten separat am Gerät einrichten.

## Seit 2.0.8 enthalten

- Sicherheitskorrekturen für HTTP-Parser, zentrale Anmeldung, CSRF bei Einstellungen/OTA und WLAN-Recovery-Portal.
- Gesalzener Admin-Passwort-Verifier mit erhaltender Migration und internem Wiederherstellungsjournal.
- Einstellungen als JSON sichern, mit Vorschau importieren und anschließend bewusst speichern. Eigene Animationen werden anhand ihres Dateiinhalts zugeordnet; Passwörter sind nicht enthalten.
- Updateprüfung über einen statischen GitHub-Kanal, HTTPS-Download durch den Browser, bestätigte Installation und geräteseitige SHA-256-Prüfung.

30 Effekte, deutscher Wochentag und Datum, Nachtmodus, Matrixausrichtung, auswählbare Automatik und acht eigene .chaos-Dateien bleiben enthalten. [Automatik](AUTOMATIK_2_0_7.md), [Animationsformat](ANIMATIONEN_FORMAT.md), [Sicherheit](SICHERHEIT_2_0_8.md).

## Vorhandene Uhr aktualisieren

Siehe [Upgrade auf 2.0.10](UPGRADE_2_0_10.md). Übertragen wird nur die passende Anwendungs-BIN. Kein USB-Schritt ist für den normalen Versionswechsel vorgesehen. Gültige Einstellungen und Zugangsdaten werden bei unverändertem Layout übernommen; der neue LED-Pin startet beim Wemos mit dem bisherigen Standard D6/GPIO12. Das Original benötigt erstmals die Admin-Ersteinrichtung; dort im Programm festgelegte Anzeige-Einstellungen lassen sich nicht automatisch auslesen.

**Bei fehlendem/noch nicht eingerichtetem LittleFS bleibt eine bestehende alte Anmeldung zunächst aktiv.** Die GUI meldet die ausstehende Passwortmigration. Nach Sicherung eventuell vorhandener Dateien den Animationsspeicher ausdrücklich einrichten; danach wird die Migration versucht. Kein automatisches Formatieren, kein stiller Admin-Reset und keine Änderung des bisherigen Passworts.

## Erste Einrichtung

1. Nach passendem Firmware-Upload zeigt die Matrix **SETUP** und einen zwölfstelligen Zahlencode.
2. Mit **ChaosUhr-Setup-XXXXXX** verbinden; WLAN-Passwort ist der angezeigte Code.
3. **http://192.168.4.1/** öffnen, eigenes Admin-Passwort setzen und wiederholen: **16–64 druckbare ASCII-Zeichen**, auch Leer-/Sonderzeichen, ohne Umlaute.
4. Nach Neustart werden vorhandene Heim-WLAN-Daten versucht weiterzuverwenden. Sonst erneut mit dem Einrichtungs-WLAN verbinden und im Portal das Heim-WLAN speichern.
5. Normale Oberfläche unter **http://chaosuhr.local/** oder der Geräte-IP öffnen. Benutzername **admin**, Passwort wie selbst gesetzt.

Der AP-Code ist getrennt vom Admin-Passwort. Bei WLAN-Ausfall öffnet nach ungefähr 30 Sekunden wieder das geschützte Einrichtungsportal; der Code erscheint auf der Matrix. Heim-WLAN-Einrichtung bleibt möglich, versteckte Neustart-/Lösch-/OTA-Aktionen sind gesperrt.

**LOCK** bedeutet unbekannte oder beschädigte Zugangsdaten. Sie werden nicht durch eine neue Admin-Registrierung überschrieben. Zuerst wird die Wiederherstellung aus einem vorhandenen gültigen Journal versucht. Hardware-Recovery unter beliebigen Flashfehlern ist nicht garantiert.

## Sicherung und Import

Unter **Einstellungen sichern** die Sicherung herunterladen. Eigene Animationsdateien zusätzlich als .chaos herunterladen. Die JSON-Datei enthält gespeicherte Uhreinstellungen, keine WLAN-/Admin-Daten.

Zum Import Datei auswählen, Vorschau prüfen und **Vorschau übernehmen**. Das füllt nur die Formulare. Erst **Einstellungen speichern** ändert die Uhr. Benötigte Upload-Dateien müssen vorhanden sein; SHA-256 ermöglicht die Zuordnung zu anderen Upload-Plätzen. Fehlende Dateien und ungültige Sicherungen werden gemeldet. Auch die Matrixausrichtung wird importiert.

## Firmware / OTA

**OTA-Upload öffnen** nimmt weiterhin eine unkomprimierte Wemos-BIN entgegen. Ab 2.0.8 werden Länge, Core-CRC und Produktkennung geprüft. Dateisystem-Images, ZIP, GZIP und alte BINs ohne passende Produktkennung werden abgewiesen. Eine Altfirmware kann die neue Anwendungs-BIN weiterhin über ihren bisherigen Updater empfangen. Kein erfolgreicher Updateabschluss oder Neustart bei fehlgeschlagenem Upload.

**Auf Updates prüfen** liest den GitHub-Kanal. Standardmäßig werden freigegebene Versionen geprüft; **Vorabversionen berücksichtigen** ergänzt den Testkanal. Der Browser braucht Internet und lädt über HTTPS, ohne das lokale Admin-Passwort an GitHub zu schicken. Nach ausdrücklicher Bestätigung wird eine neuere passende Version lokal übertragen. Die Uhr prüft zusätzlich SHA-256. Keine unbeaufsichtigten Updates.

Für die veröffentlichte 2.0.10 **Vorabversionen berücksichtigen** aktivieren. [Updatekanal und Veröffentlichung](GITHUB_UPDATES.md). Fehlender Kanal oder fehlendes Internet verhindert kein manuelles OTA. HTTP zur lokalen Uhr bleibt unverschlüsselt. SHA-256 ist keine Herausgebersignatur; Signaturpflicht und geräteeigenes HTTPS sind nicht aktiviert.

## Werkseinstellungen

Als Admin Lösch-Checkbox und Bestätigungsdialog verwenden. Gelöscht werden Uhreinstellungen, Admin-Zugang, AP-Code, Heim-WLAN und eigene Animationen. Danach beginnt die Ersteinrichtung mit neuem Code.

Zuerst wird ein dauerhaftes Reset-Merkmal gespeichert. Dann WLAN löschen, EEPROM leeren und zuletzt LittleFS einschließlich Journal formatieren. Das Journal erhält den Reset-Auftrag während der EEPROM-Löschung. Unterbrechungen werden nach Möglichkeit wiederaufgenommen; echte physische Flashfehler bleiben eine Grenze. Ein vergessenes Admin-Passwort lässt sich nicht über einen ungeschützten GUI-Reset umgehen.

## Bauen und Paket

```bash
pio run -e d1_mini_public
python3 tests/run_host_tests.py
python3 scripts/package_public.py
```

Plattform espressif8266 4.2.1, Arduino-Core 3.1.2, FastLED 3.10.9, WiFiManager 2.0.17; geprüft mit PlatformIO Core 6.2.0. Der gehärtete HTTP-Server ist projektlokal enthalten. Tests benötigen Python 3, Node.js, C++17 mit Sanitizern und OpenSSL-Entwicklungsdateien/libcrypto. Kein privater Secrets-Header erforderlich.

Ausgabe unter `dist/2.0.10/esp8266/`: BIN, ZIP mit derselben BIN, Anleitung, passenden Quellen/Lizenzen, SHA256SUMS und vorbereitete Kanaldateien. Kein automatischer Upload oder Git-Push.

**2.0.10 ist eine Vorabversion ohne Gerätetest.** Der bisherige 2.0.8-Gerätetest gilt nicht automatisch für diese BIN. Lokale Tests prüfen zusätzlich neue Anzeige-/GPIO-Optionen, MQTT-Daten, atomare Speicherung und die GUI. Details und offene Prüfungen im [Releasebericht 2.0.10](RELEASE_v2.0.10.md). Für klassischen ESP32 gibt es einen getrennten [Build und eine eigene Anleitung](ESP32_FIRMWARE.md).

Original: https://github.com/oOmicha1986Oo/ChaosUhr. Herkunft in THIRD_PARTY.md; GPL Version 3, LGPL für den übernommenen Webserver und MIT-Hinweise für FastLED.
