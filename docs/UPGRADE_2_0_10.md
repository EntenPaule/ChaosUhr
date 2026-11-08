# Wechsel auf ChaosUhr 2.0.10

Vorabversion für **Wemos D1 mini / ESP8266** und **klassischen ESP32-WROOM-32 / esp32dev**, jeweils 4 MiB Flash und 20×15 WS2812B. Gerätetest der neuen BINs steht aus. Passenden Zielchip und ausreichenden OTA-Platz vor dem Upload prüfen.

## Welche Datei?

- Wemos: `chaosuhr-2.0.10-esp8266.bin` für den bisherigen Web-Updater.
- Klassischer ESP32: `chaosuhr-2.0.10-esp32-ota.bin` für den bisherigen Web-Updater.
- `chaosuhr-2.0.10-esp32-factory.bin` ist ausschließlich das vollständige USB-Erstinstallationsimage ab Adresse 0x0. Es überschreibt alle Daten des Zielboards; niemals in den OTA-Updater laden.
- ZIP vorher entpacken; keine ZIP-, Dateisystem- oder GZIP-Datei als Anwendung flashen.

## Vorhandene Einstellungen

Vor dem Update eigene `.chaos`-Dateien herunterladen. Ab 2.0.8 zusätzlich JSON-Einstellungssicherung anlegen. Ein normales passendes Anwendungs-OTA behält das Flash-/Dateisystemlayout und löscht WLAN oder gespeicherte Konfiguration nicht absichtlich.

**Auf einer Wemos-Uhr mit 2.0.8:** OTA-Menü öffnen, **Vorabversionen berücksichtigen** aktivieren und **Auf Updates prüfen** wählen. Die angebotene 2.0.10 erst nach Sicherung bewusst installieren; die Uhr startet danach neu. Ältere Versionen ohne GitHub-Updater verwenden das bisherige Uploadformular und die passende BIN aus dem Release. Ein ESP32 auf dem früheren 2.0.6-Basisstand verwendet ebenfalls zunächst den manuellen OTA-Upload.

Die bisherigen 32 Einstellungsbytes und der Admin-Bereich ab Offset 32 bleiben erhalten. Neue Anzeigeoptionen liegen in den bisher unbenutzten acht Bytes 120–127. Standard nach Migration: **D6/GPIO12 beim Wemos**, **GPIO32 beim ESP32**, Sekundenleiste an, ausgeschriebener Wochentag, Uhr/Animationen im Wechsel. Wetter und Sensoren sind zunächst ausgeschaltet.

Von 2.0.5/2.0.7 oder dem bisherigen ESP32-Basisstand bleiben gültige Admin-Passwörter nutzbar. Bei lesbarem LittleFS erfolgt die Migration zum gesalzenen Passwort-Verifier mit Journal. Ist LittleFS nicht lesbar, bleibt die bestehende Anmeldung aktiv; Speicherinitialisierung erst nach Sicherung vorhandener Dateien bestätigen. Kein automatisches Formatieren beim Start.

Michas Referenzsketch benötigt erstmals die Admin-Ersteinrichtung; fest im Sketch hinterlegte Anzeigeparameter werden nicht automatisch importiert. Angepasste Altfirmware kann abweichen.

## Neue Auswahlmöglichkeiten

- **LED-Datenpin:** Nur den zur tatsächlichen Verkabelung passenden Pin wählen. Beim Speichern einer Änderung startet die Uhr nach Bestätigung neu. Erlaubte Pins zeigt das Dropdown für den jeweiligen Controller.
- **Sekundenleiste:** In der GUI ein-/ausschalten.
- **Wochentag:** `Montag` mit echten Kleinbuchstaben oder `mo` als Kurzform.
- **Automatik:** Uhrzeit, Animationen, Wetter und Sensorwerte unabhängig auswählen. Nur Uhr/Wetter ist möglich.
- **MQTT:** Broker und Datenquellen separat einrichten; Details in [MQTT_2_0_10.md](MQTT_2_0_10.md).
- Nach erfolgreicher WLAN-Verbindung wird die zugewiesene IP auf der Matrix eingeblendet. Bei Verbindungsausfällen bleiben die bisherige Wiederverbindung und Einrichtung vorgesehen.

## Backup und weitere Updates

Neue Einstellungssicherungen verwenden Schema 2; Schema-1-Sicherungen aus 2.0.8 bleiben importierbar. Neue Felder bleiben beim Import einer alten Sicherung auf den am Zielgerät geladenen Werten. LED-Pin, MQTT-Zugang und Animationsdateien werden separat eingerichtet/gesichert. Eine Vorschau schreibt nichts; erst Speichern übernimmt die Anzeigeeinstellungen.

Wemos verwendet weiter `stable.json`/`preview.json`; ESP32 verwendet eigene Kanäle `stable-esp32.json`/`preview-esp32.json`. 2.0.10 wird als Vorabversion in beiden Preview-Kanälen angeboten. Der Browser lädt über HTTPS und sendet Zugangsdaten ausschließlich zur lokalen Uhr.

Ein Downgrade auf unveränderte 2.0.5/2.0.7 nach Passwortmigration ist ungeeignet. Auch 2.0.8 kennt die neuen Anzeigeoptionen und einen abweichend eingestellten LED-Pin nicht. Ein Rückweg muss Passwortformat, Pin und Speicherlayout berücksichtigen.
