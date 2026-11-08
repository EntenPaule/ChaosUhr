# ChaosUhr – auswählbare Automatik ab 2.0.7, erweitert in 2.0.10

**2.0.10 für Wemos und klassischen ESP32:** Die Animationsauswahl wird um unabhängige Kategorien für Uhrzeit, Animationen, Wetter und Sensorwerte ergänzt. Schreibende API-Aufrufe benötigen das Token aus `GET /api/session` als `X-CSRF-Token`. Wetter und Sensoren werden separat über [MQTT](MQTT_2_0_10.md) eingerichtet.

Die bisherigen Wemos-Versionen 2.0.7 und 2.0.8 haben die unten beschriebene Animationsauswahl ohne Wetter-/Sensorkategorien. Ihre veröffentlichten Pakete bleiben unverändert.

Die 2.0.10-Builds verwenden **Wemos D1 mini / ESP8266, Standard D6/GPIO12**, und **klassischen ESP32-WROOM-32 / esp32dev, Standard GPIO32**. Beide verwenden 4 MiB Flash und eine 20×15-WS2812B-Matrix. Weitere erlaubte LED-Pins sind in der GUI wählbar.

Die Upload-Verwaltung ist in beiden neuen Builds enthalten: [Eigene Animationen und .chaos-Format](ANIMATIONEN_FORMAT.md). Lesbare Uploads ergänzen dieselbe Automatik-Auswahl; eine reine Upload-Auswahl ist möglich.

## Bedienung

1. Die Weboberfläche als `admin` öffnen.
2. Den Modus **Automatik: gewählte Anzeigen** wählen und die gewünschten Kategorien ankreuzen. Für nur Uhr/Wetter Animationen und Sensorwerte abwählen. Auf 2.0.7/2.0.8 heißt der Modus **Automatik: Uhr / Animation**.
3. Falls Animationen aktiviert sind, unter **Automatik** die gewünschten Animationen ankreuzen. **Alle auswählen** aktiviert die gesamte Liste; **Auswahl leeren** hilft beim Zusammenstellen einer eigenen Auswahl. Bei aktiver Animationskategorie muss mindestens eine verfügbare Animation ausgewählt sein.
4. Die Reihenfolge wählen:
   - **Zufällig (jede einmal pro Runde):** Jede gewählte Animation läuft genau einmal, danach beginnt eine neue zufällige Runde. Bei mehreren ausgewählten Animationen wiederholt sich auch an Rundengrenzen nicht unmittelbar dieselbe Animation.
   - **Der Reihe nach:** Die gewählten Animationen laufen in der Reihenfolge der Liste. Nach der letzten beginnt die erste erneut.
5. **Einstellungen speichern** drücken.

Bei Auswahl nur von Uhr und Animationen ist der Ablauf **Uhr → Animation → Uhr → nächste Animation → Uhr …**. Weitere gewählte Kategorien ergänzen ab 2.0.10 die Runde in der Reihenfolge Uhr, Wetter, Sensorwert, Animation. Pro Sensorszene erscheint der nächste aktuelle Sensor. Die eingestellte Dauer gilt für jede Szene. Fehlende oder veraltete Daten werden übersprungen; ohne verfügbare gewählte Szene wird die Uhr angezeigt.

**Nur Uhrzeit** und **Gewählte Animation** bleiben eigenständige Modi. Die Auswahl der Einzelanimation ist unabhängig von der Automatik-Liste. Nach einem Neustart oder einer Änderung von Modus, Automatik-Auswahl oder Reihenfolge beginnt die Wiedergabe neu; bei gewählter Uhrkategorie mit einer Uhrphase. Der Fortschritt innerhalb einer Runde wird nicht dauerhaft gespeichert.

## Vorhandene Uhren aktualisieren

Ein normales OTA-Update übernimmt gültige bisherige Uhr-Einstellungen, den gespeicherten Admin-Zugang und die WLAN-Konfiguration. Nach dem Update von 2.0.5 auf Wemos bzw. 2.0.6 auf ESP32 sind zunächst **alle 30 Animationen** und die **Zufallswiedergabe** aktiviert. Zum Anlegen einer eigenen Auswahl die Weboberfläche neu laden und speichern. Eine erneute Admin-Ersteinrichtung ist bei gültig gespeicherten Zugangsdaten nicht nötig.

Die bisherige 20-Byte-Einstellungsstruktur bleibt als kompatibler Präfix erhalten. Auswahl und Reihenfolge liegen mit eigener Versionskennung/Prüfsumme in Bytes 20–27; die Upload-Auswahl ergänzt Bytes 28–31. Der Admin-Bereich ab Offset 32 wird nicht überschrieben. Alte Einstellungen werden beim Start nur im RAM ergänzt; dauerhaft gespeichert wird die Erweiterung beim nächsten ausdrücklichen Speichern. Deshalb kann `settingsSaved` bis dahin `false` melden. Ungültige Erweiterungsdaten setzen nur den jeweiligen neuen Teil zurück, solange der alte Einstellungsblock gültig ist. Auch der erste 28-Byte-Zwischenstand von 2.0.7 wird übernommen.

Für ESP32 wird beim Update ausschließlich die **OTA-BIN** verwendet. Das vollständige **Factory-Image** ist für die USB-Erstinstallation bestimmt und überschreibt alle Daten auf dem Zielboard. Beim Wemos kann die enthaltene ESP8266-BIN über den bestehenden Web-Updater übertragen werden. Ein normaler Build lädt nichts auf ein Gerät hoch.

## API

`GET /api/status` liefert zusätzlich:

- `autoEffects`: Bitmaske der eingebauten Effekte; Bit 0 steht für Effekt 1, Bit 29 für Effekt 30. Gültig sind 0 bis 1073741823, Standard 1073741823. Bei aktiver Animationskategorie muss ein eingebauter Effekt oder verfügbarer Upload ausgewählt sein. Ist die Kategorie aus, bleibt eine zuvor gültige Animationsauswahl als Vorgabe erhalten.
- `autoUploads`: Auswahl der eigenen Uploads, 0–255; Bit 0 entspricht ID 31, Bit 7 entspricht ID 38. Standard 0.
- `autoOrder`: 0 für zufällige Runden, 1 für der Reihe nach, Standard 0.

`POST /api/settings` akzeptiert dieselben Felder, auch als Teiländerung. Beispiel: `autoEffects=536870914&autoOrder=1` aktiviert Effekt 2 und Effekt 30 der Reihe nach. Bei aktiver Animationskategorie werden gemeinsam leere Masken (`autoEffects=0&autoUploads=0`) abgelehnt. Nicht verfügbare Uploads, unbekannte Bits, ungültige Zahlen oder doppelte Parameter ergeben ebenfalls HTTP 400. Bei einem Speicherfehler bleibt die laufende Konfiguration erhalten. Die neuen Kategorien heißen `showClock`, `showAnimations`, `showWeather` und `showSensors` (jeweils 0/1).

## Prüfstand

Die lokalen Tests prüfen die tatsächlich eingebundenen Szenen- und API-Funktionen, vollständige zufällige Runden, Listenreihenfolge, Uhrphasen, Einzel-/Alle-Auswahl, Effekt 30, Speichern/Neustart, Übernahme alter Einstellungsdaten, Speicherfehler und unveränderte Admin-Daten. Dazu kommen Kategorien ohne Animationen, Wetter-/Sensorwerte, GPIO-Optionen und GUI-Abläufe. Die neuen 2.0.10-BINs sind noch nicht am Gerät getestet. [Aktueller Release- und Teststand](RELEASE_v2.0.10.md); historische 2.0.7-Gerätebelege stehen im [damaligen Bericht](RELEASE_v2.0.7.md).
