# ChaosUhr 2.0.7 – auswählbare Automatik

**Weiter gültig für Wemos 2.0.8:** Die Automatik funktioniert wie hier beschrieben. Dort benötigen schreibende API-Aufrufe zusätzlich das Token aus `GET /api/session` als `X-CSRF-Token`. Ein neuer ESP32-Build gehört nicht zur Sicherheitsfassung 2.0.8; entsprechende Hinweise unten beschreiben den älteren Zwischenstand.

Zunächst wird nur das Wemos-Paket bereitgestellt; die ESP32-Paketpflege folgt auf Wunsch bei Bedarf.

Diese Funktion ist in beiden Builds identisch: **Wemos D1 mini / ESP8266, D6/GPIO12**, und **klassischer ESP32-WROOM-32 / DevKit, GPIO32**. Beide verwenden 4 MB Flash und eine 20×15-WS2812B-Matrix. Für ESP32-S3-/WT32-Displayboards sind diese BINs nicht vorgesehen.

Die neue Upload-Verwaltung wird zunächst nur im Wemos-Build bereitgestellt: [Eigene Animationen und .chaos-Format](ANIMATIONEN_FORMAT.md). Lesbare Uploads ergänzen dort dieselbe Automatik-Auswahl; eine reine Upload-Auswahl ist möglich.

## Bedienung

1. Die Weboberfläche als `admin` öffnen.
2. Den Modus **Automatik: Uhr / Animation** wählen.
3. Unter **Automatik** die gewünschten Animationen ankreuzen. **Alle auswählen** aktiviert die gesamte Liste; **Auswahl leeren** hilft beim Zusammenstellen einer eigenen Auswahl. Zum Speichern muss mindestens eine Animation ausgewählt sein.
4. Die Reihenfolge wählen:
   - **Zufällig (jede einmal pro Runde):** Jede gewählte Animation läuft genau einmal, danach beginnt eine neue zufällige Runde. Bei mehreren ausgewählten Animationen wiederholt sich auch an Rundengrenzen nicht unmittelbar dieselbe Animation.
   - **Der Reihe nach:** Die gewählten Animationen laufen in der Reihenfolge der Liste. Nach der letzten beginnt die erste erneut.
5. **Einstellungen speichern** drücken.

Der Ablauf ist immer **Uhr → Animation → Uhr → nächste Animation → Uhr …**. Die eingestellte Dauer je Szene gilt für Uhr und Animationen. Auch eine einzelne ausgewählte Animation wechselt mit der Uhr ab.

**Nur Uhrzeit** und **Gewählte Animation** bleiben eigenständige Modi. Die Auswahl der Einzelanimation ist unabhängig von der Automatik-Liste. Nach einem Neustart oder einer Änderung von Modus, Automatik-Auswahl oder Reihenfolge beginnt die Wiedergabe neu mit einer Uhrphase. Der Fortschritt innerhalb einer Runde wird nicht dauerhaft gespeichert.

## Vorhandene Uhren aktualisieren

Ein normales OTA-Update übernimmt gültige bisherige Uhr-Einstellungen, den gespeicherten Admin-Zugang und die WLAN-Konfiguration. Nach dem Update von 2.0.5 auf Wemos bzw. 2.0.6 auf ESP32 sind zunächst **alle 30 Animationen** und die **Zufallswiedergabe** aktiviert. Zum Anlegen einer eigenen Auswahl die Weboberfläche neu laden und speichern. Eine erneute Admin-Ersteinrichtung ist bei gültig gespeicherten Zugangsdaten nicht nötig.

Die bisherige 20-Byte-Einstellungsstruktur bleibt als kompatibler Präfix erhalten. Auswahl und Reihenfolge liegen mit eigener Versionskennung/Prüfsumme in Bytes 20–27; die Upload-Auswahl ergänzt Bytes 28–31. Der Admin-Bereich ab Offset 32 wird nicht überschrieben. Alte Einstellungen werden beim Start nur im RAM ergänzt; dauerhaft gespeichert wird die Erweiterung beim nächsten ausdrücklichen Speichern. Deshalb kann `settingsSaved` bis dahin `false` melden. Ungültige Erweiterungsdaten setzen nur den jeweiligen neuen Teil zurück, solange der alte Einstellungsblock gültig ist. Auch der erste 28-Byte-Zwischenstand von 2.0.7 wird übernommen.

Für ESP32 wird beim Update ausschließlich die **OTA-BIN** verwendet. Das vollständige **Factory-Image** ist für die USB-Erstinstallation bestimmt und überschreibt alle Daten auf dem Zielboard. Beim Wemos kann die enthaltene ESP8266-BIN über den bestehenden Web-Updater übertragen werden. Ein normaler Build lädt nichts auf ein Gerät hoch.

## API

`GET /api/status` liefert zusätzlich:

- `autoEffects`: Bitmaske der eingebauten Effekte; Bit 0 steht für Effekt 1, Bit 29 für Effekt 30. Gültig sind 0 bis 1073741823, Standard 1073741823. Bei 0 muss mindestens ein verfügbarer Upload ausgewählt sein.
- `autoUploads`: Auswahl der eigenen Uploads, 0–255; Bit 0 entspricht ID 31, Bit 7 entspricht ID 38. Standard 0.
- `autoOrder`: 0 für zufällige Runden, 1 für der Reihe nach, Standard 0.

`POST /api/settings` akzeptiert dieselben Felder, auch als Teiländerung. Beispiel: `autoEffects=536870914&autoOrder=1` aktiviert Effekt 2 und Effekt 30 der Reihe nach. Gemeinsam leere Masken (`autoEffects=0&autoUploads=0`), nicht verfügbare Uploads, unbekannte Bits, ungültige Zahlen oder doppelte Parameter werden mit HTTP 400 abgelehnt. Bei einem Speicherfehler bleibt die laufende Konfiguration erhalten.

## Prüfstand

Die lokalen Tests prüfen die tatsächlich eingebundenen Szenen- und API-Funktionen, vollständige zufällige Runden, Listenreihenfolge, Uhrphasen, Einzel-/Alle-Auswahl, Effekt 30, Speichern/Neustart, Übernahme eines alten 20-Byte-Datensatzes, Speicherfehler und unveränderte Admin-Daten. Die Wemos-BIN 2.0.7 läuft nach einem OTA-Update auf einem Gerät; vollständige geordnete und zufällige Runden wurden dort noch nicht bestätigt. Für ESP32 wird kein 2.0.7-Paket veröffentlicht. Details: [Release- und Teststand](RELEASE_v2.0.7.md).
