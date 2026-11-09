# ChaosUhr 2.0.11-beta.3 – Automatik verständlich erklären

## Übernommener Entwicklungsstand

Zu Beginn war der Entwicklungsworktree `fix/2.0.11-diagnostics` sauber auf `24373c4` (Beta.1). Die aktive Arbeitskopie enthielt die noch nicht committeten Beta.2-Änderungen. Diese wurden mit den bereits erfolgreichen 51 Hosttestgruppen, dem vorhandenen Clean-Build und der neuen Hardware-Rückmeldung erhalten und als `2598df6` gesichert. Keine erneute Untersuchung des inzwischen erfolgreichen MQTT-Speicherns, keine zusätzlichen ESP32-Builds.

## Bestätigte Beta.2-Hardwaretests

Nutzerrückmeldung vom 09.11.2026 am echten Wemos D1 mini:

- MQTT-Konfiguration speicherbar, vorheriger Absturz tritt nicht mehr auf; Phase **Complete**, rund **1,3 KiB freie Stackreserve** laut Diagnose.
- ioBroker-Verbindung über IPv4 und DNS-Namen erfolgreich.
- 30 Sekunden vollständige Stromtrennung; WLAN- und MQTT-Daten erhalten, automatischer MQTT-Verbindungsaufbau nach Kaltstart.
- Start mit angeschlossener LED-Matrix erfolgreich.
- MQTT-Wetterdaten aus ioBroker und Wetteranimationen funktionieren.

Stacküberlauf ist die sehr wahrscheinliche ursprüngliche Absturzursache; ein endgültiger damaliger Crashnachweis fehlt. Einmaliges MQTT-011 wird dokumentiert, ohne konkrete Wiederholung oder weiteren Hinweis keine neue Untersuchung. AUTH-021 nach Neustarts ist erwartete Invalidierung alter RAM-Sitzungen. Der frühere MQTT-007 wird nicht ohne Beleg mit dem Absturz gleichgesetzt.

## Tatsächlicher Automatikablauf

Im Automatikmodus läuft die feste zyklische Seitenreihenfolge **Uhrzeit → Wetter → ein Sensorwert → eine Animation → Uhrzeit …**. Abgewählte oder nicht verfügbare Seiten werden übersprungen. Bei nur Uhr und Animationen ergibt sich Uhr → Animation → Uhr; mit nur Uhr/Wetter ergibt sich Uhr → Wetter → Uhr.

Jede verfügbare Szene läuft für `duration`, in der GUI „Dauer je Szene“, **5–120 Sekunden**. Das gilt gleichermaßen für Uhr, Wetter, einen Sensor und eine Animation; keine zusätzliche Wetterdauer. Die Dauer garantiert nicht, dass ein langer Datumslauftext oder eine komplette Animationssequenz beendet wird. Die Erst-/IP-Anzeige kann die Szene vorübergehend überdecken. Wird die aktuelle Seite wegen fehlender Daten unverfügbar, erfolgt der nächste Szenenwechsel vorzeitig.

Bei jedem Sensorseitenbesuch wird zyklisch der nächste frische Sensor aus den vier Slots gewählt. Mehrere Sensoren werden somit über mehrere Runden verteilt. Wird nur der gerade gezeigte Sensor während seiner Szene veraltet, während ein anderer frisch bleibt, zeigt `drawSensor()` bis zum nächsten Sensorwechsel die Uhr als Fallback.

Bei jedem Animationsseitenbesuch wird die nächste ausgewählte, verfügbare Animation gewählt: nach ID aufsteigend oder zufällige Runden, jede gewählte Animation einmal pro Runde. Eine Runde der Animationsauswahl ist nicht identisch mit einer Runde aller Anzeigearten. Nicht verfügbare Uploads werden nicht als neue Wetter-/Sensorseite behandelt; die bestehenden Fallbacks bleiben erhalten.

Wetter benötigt aktiviertes MQTT und mindestens einen frischen Wetterzustand **oder** eine frische Temperatur. Sensorseiten benötigen aktiviertes MQTT und mindestens einen frischen Sensor. Frisch heißt: gültiger Messwert und Alter kleiner als `staleSeconds` (60–86400 Sekunden, Standard 1800). MQTT-Verbindungsverlust blendet bereits empfangene, noch frische Daten nicht sofort aus. Werden alle gewählten Seiten unverfügbar, erscheint die Uhr, auch wenn sie abgewählt war.

Der **Nachtmodus verändert ausschließlich die Helligkeit**, nicht Reihenfolge, Zeitablauf oder Wetterfreigabe. Aktiv ist er nur bei gültiger Uhrzeit und passender Stundenregel; auch Zeiträume über Mitternacht sind unterstützt. Gedimmte Symbole können schlechter erkennbar sein. Die vorangegangene Vermutung „Wetter fehlt in der Automatik“ ist durch Hardwaretest und Codeprüfung widerlegt.

„Nur Uhrzeit“ und „Gewählte Animation“ sind eigenständige Modi ohne diesen Automatikzyklus.

## GUI-Änderungen

Die statische Erklärung beschreibt nun Reihenfolge, Szenendauer, Sensor-/Animationswechsel, Datenverfügbarkeit und Nachtmodus. „Dein aktueller Anzeigeablauf“ ist eine **lokale Auswahlvorschau**, keine Live-Anzeige der aktuellen Szene oder Datenverfügbarkeit. Sie reagiert auf Modus, Anzeigearten, Dauer und Animationsreihenfolge, berücksichtigt unvollständige Auswahl und speichert nichts. Änderungen müssen weiterhin mit der vorhandenen Schaltfläche gespeichert werden. Keine zusätzlichen Statusabfragen oder Sitzungsverlängerungen.

### Sekundenleiste: genaue Bedingung bestätigt

`drawClock()` zeichnet sie ausschließlich bei gültiger Uhrzeit und gesetztem `DISPLAY_SECONDS`-Flag. Eine ausdrückliche Bedingung „Helligkeit < 5 %“ existiert **nicht**. Die gesamte LED-Ausgabe wird mit Tages- bzw. Nachthelligkeit gedimmt (`map(pct,1,100,2,120)`), die Leiste selbst ist dunkelblau (0,0,32). Der bereits seit Beta.1 vorhandene pauschale 5-%-Hinweis war deshalb zu streng.

Sachlich präzisierte GUI: „Bei sehr geringer Helligkeit kann die Sekundenleiste kaum oder nicht sichtbar sein. Dies gilt auch im Nachtmodus.“ Keine feste Schwelle erfunden und keine Zeichen-/Dimmfunktion geändert. Zusätzlich hängt die sichtbare Leistenlänge von der Sekunde ab; zu Beginn der Minute sind noch keine Fortschrittspixel gezeichnet.

## Unveränderte Funktionen

Bytegleich zu `2598df6` geprüft: `drawClock`, `drawWeather`, `drawSensor`, `activeEffect`, `availableScenePages`, `render`, `nextScene`, `loop` sowie Wetterbilder, Seiten-/Zeit-/Animationslogik, MQTT-Speicherpfad und Sitzungsschutz. Regen bleibt animiert, Schnee mit unbewegten weißen Punkten, Gewitter mit gelben Zickzack-Blitzen; keine neuen Wetterfunktionen. Keine Löschung oder Änderung gespeicherter Daten.

## Geänderte Dateien gegenüber dem gesicherten Beta.2-Stand

- `src/main.cpp`: ausschließlich GUI-Erklärung, Vorschau und Versionsreferenz des generierten Fehlerskripts.
- `include/firmware_version.h`: gemeinsame Version 2.0.11-beta.3; generierte Referenzen in `include/public_ui.h`, `include/portal_ui.h`, `include/ota_ui.h`.
- `tests/test_display_mqtt_ui.js`: Vorschau und ausschließlich lokale Aktualisierung prüfen.
- `scripts/package_wemos_test.py`: aktuelle Testgrenzen und Matching-ELF nachvollziehbar dokumentieren.
- `scripts/public_sources.py`: neue öffentliche Dokumentation aufnehmen.
- `README.md`, `CHANGELOG.md`, diese Analyse und `TESTANLEITUNG_2.0.11-beta.3.md`.
- Neue Distributionsdateien ausschließlich unter `dist/2.0.11-beta.3/`.

## Finale Tests, Build und Paket

- Vollständige aktive Wemos-Hostsuite: **51 PASS-Gruppen**, einschließlich echter Browser-/HTTP-Parseranfrage mit 24 MQTT-Feldern, Speicherung/Rücklesen, Dateisystemfehlern und Speichermangel ohne Datenverlust sowie Login/Logout/30-Minuten-Ablauf/CSRF/Neustartinvalidierung.
- Geänderte GUI zusätzlich auf Auswahlreihenfolge, Szenendauer, Solo-Modi, Fallbackhinweis und ausbleibende automatische Schreibzugriffe geprüft.
- Wemos Clean-Build **SUCCESS**; tatsächliche BIN **689456 Byte**, OTA-Limit **1044464 Byte**, Reserve **355008 Byte**.
- Statisches RAM **51544/81920 Byte**, Linker-Flash **685307/1044464 Byte**. Keine neue Laufzeit-Heapmessung auf Hardware.
- Tatsächlicher Compiler-Stackbericht: MQTT-Handler **96 Byte**, Storage-Save **272 Byte**, zusammen **368 Byte** vor Unterfunktionen. Guard des vorhandenen Stackprüfskripts bestanden; Optimierung unverändert.
- BIN SHA-256: `d5f5c6c8026c66125cc9c651b6109fdae787611574e5d5c9313e55e9d5bed6c2`.
- `scripts/package_wemos_test.py` erzwingt Quellcommit-Identität, Bild-/Versions-/Größenprüfung, Produktions-OTA-Validator mit Mutation/Kürzung/falscher SHA, ZIP-CRC und Geheimnisscan. Die entpackte BIN und interne/externe Prüfsummen werden abschließend nochmals gegen die tatsächlichen Dateien geprüft.
- Testpaket: `dist/2.0.11-beta.3/ChaosUhr_2.0.11-beta.3_TEST.zip`; Test-BIN: `dist/2.0.11-beta.3/ChaosUhr_2.0.11-beta.3_Wemos_OTA.bin`. SHA-256-Liste und BUILD_INFO liegen daneben und im Paket. Das Paket enthält keine ESP32-Firmware.
- Vorhandene Beta.2-Untersuchungen und Hardwaretests wurden nicht unnötig wiederholt. Ausschließlich Wemos/ESP8266; kein ESP32-Build und keine ESP32-Hardwareaussage für Beta.3.

## Noch offene Hardwaretests

Beta.3: GUI-Text und Vorschau für die eigene Auswahl, anschließendes Speichern und realer Ablauf, Nachtmodus-Sichtbarkeit und geringe Helligkeit; weiter Login/Logout/30-Minuten-Ablauf, nachfolgendes OTA sowie Langzeit-/Heapbeobachtung. Bereits bestätigte Beta.2-Kaltstart-/Wetter-/Brokerergebnisse gelten für den dokumentierten Beta.2-Stand; Beta.3 ist noch nicht vor Ort getestet.

Kein offizielles Release, kein Stable-Tag, kein Push, kein produktiver Branch-/Updatekanalwechsel und kein Gerätezugriff.
