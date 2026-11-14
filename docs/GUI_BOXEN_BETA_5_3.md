# Interne 2.0.11-beta.5.3: getrennte Einstellungsboxen

Ausgangsstand: `742710a46e2e5b445ffd25628d0ea88192d1014a`, Entwicklungsbranch
`fix/2.0.11-diagnostics` in `.pio/dev-2.0.11`. Nur lokale Entwicklung.

## Oberfläche

- **Anzeige & Animationen:** Modus, Animation, Tempo, Szenendauer, Automatik-
  Auswahl/Reihenfolge, Anzeigearten, Wochentag und Sekundenleiste. Eigener Button
  „Anzeige & Animationen speichern“.
- **Helligkeit & Nachtmodus:** Taghelligkeit, Nachtmodus, Beginn/Ende,
  Nachthelligkeit, Nachtabschaltung, MQTT-Helligkeitsfreigabe, wirksame Quelle,
  Prozentwert, MQTT-Vorgabe und Befehlstopic. Ein Speichern-Button
  „Helligkeit & Nachtmodus speichern“. „MQTT-Vorgabe aufheben“ bleibt eine
  eigenständige Aktion zur Freigabe der laufenden Vorgabe.
- **Matrixverdrahtung:** LED-Datenpin, Start-Ecke, Zeilen-/Spaltenverlauf und
  Zickzack. Eigene Box unterhalb von Uploads und MQTT, vor Diagnose.
  Eigener Button „Matrixverdrahtung speichern“. Pinwechsel weiterhin nur nach
  Bestätigung und mit Neustart durch die vorhandene Firmwarefunktion.

Keine doppelten Regler oder IDs. Dunkles Design, bestehende flexible Anordnung,
Boxen und Abstände bleiben erhalten. Deaktivierte Eingaben werden ausgegraut;
Nachtzeiten sind bei ausgeschaltetem Nachtmodus, die Nachthelligkeit zusätzlich
bei gewählter Nachtabschaltung deaktiviert. Ein kurzer Hinweis erklärt jeweils
warum; Werte werden dadurch nicht gelöscht.

## Tatsächliche Speicherkoordination

Es gibt keine gemeinsame Transaktion zwischen EEPROM und LittleFS.
Die bestehenden APIs und Speicherlayouts bleiben unverändert.
`/api/settings` unterstützt bereits Teilanfragen: Der Server übernimmt die
vorhandene Konfiguration und ersetzt nur die eingereichten Felder.

1. Eingaben beim Klick in einen festen Snapshot übernehmen und Helligkeit/
   Stunden lokal prüfen. Ungültige Eingaben: keine Schreibanfrage.
2. POST `/api/settings` ausschließlich mit `brightness`, `nightOn`, `nightFrom`,
   `nightUntil`, `nightBrightness`. Bestätigung einschließlich gültiger JSON-
   Antwort abwarten. Bei Fehler oder verlorener Antwort kein zweiter Schreibzugriff.
3. Erst nach bestätigtem ersten Schritt POST `/api/brightness` mit exakt
   `nightLedsOff` und `mqttBrightness` senden.
4. Anschließend den aktuellen Status lesend über GET `/api/brightness` abrufen.
   Dabei keine Formularfelder aus dem Status überschreiben.

| Ausgang | Kurze Rückmeldung |
| --- | --- |
| Beide Schritte bestätigt | Helligkeit & Nachtmodus gespeichert; beide Speicherbereiche bestätigt. |
| EEPROM-Schritt nicht bestätigt | Taghelligkeit und Nachtzeiten: Speicherung nicht bestätigt. Nachtabschaltung und MQTT-Freigabe: nicht gesendet. |
| LittleFS-Schritt nicht bestätigt | Taghelligkeit und Nachtzeiten gespeichert. Nachtabschaltung und MQTT-Freigabe: Speicherung nicht bestätigt. |
| Status nicht abrufbar | Zusätzlicher Hinweis; „Wirksam“ wird als derzeit nicht abrufbar gekennzeichnet. |

„Nicht bestätigt“ ist bewusst gewählt: Bei einer verlorenen HTTP-Antwort kann
der Schreibzugriff bereits erfolgt sein. Es gibt keine automatische Wiederholung,
keine vorgetäuschte Rücknahme und keine Erfolgsmeldung für beide Speicherbereiche.
Ein erneuter Klick ist eine bewusste Wiederholung durch den Nutzer.

Eine GUI-Sperre verhindert konkurrierende Schreibvorgänge über die drei Box-
Speichern-Buttons. Sie wird auch bei Fehlern wieder aufgehoben. Eingaben dürfen
während einer laufenden Anfrage weiter bearbeitet werden; gespeichert wird der
Snapshot vom Klick. Statusabfragen und Antworten überschreiben spätere Entwürfe
nicht. Andere Browser/Tabs werden nicht serverseitig gesperrt; keine neue
Versions-/Transaktions-API wird behauptet. Bei konkurrierenden Änderungen gelten
weiterhin die vorhandenen Serverregeln.

## Gespeichert, wirksam und Entwurf

Die Anzeige „Wirksam“ und das MQTT-Befehlstopic stammen ausschließlich aus der
Geräteantwort von `/api/brightness`. Checkbox-Änderungen ändern nur den Entwurf
und die Bedienbarkeit der Regler. Die Fünf-Sekunden-Statusabfrage schreibt keine
Checkboxen zurück. Auch „MQTT-Vorgabe aufheben“ aktualisiert nur den laufenden
Status und lässt Entwürfe stehen.

Dateisystemprobleme des geladenen Status erscheinen in einem eigenen kurzen
Statushinweis; sie überschreiben keine Meldung über einen Speicher-Teilfehler.
Ausführliche Fehlerdetails bleiben ausschließlich unter Diagnose. Bestehende
AUTH-/CSRF-Prüfungen und nicht verlängernde Hintergrundabfragen bleiben erhalten.

Allgemeines Speichern sendet keine Helligkeits-, Nacht- oder Hardwarefelder.
Hardware-Speichern sendet nur `ledPin`, `corner`, `vertical`, `zigzag`.
Backupformat unverändert 3; Vorschau übernimmt weiterhin nur Formularentwürfe.
Der Hinweis nennt jetzt alle drei Boxen, über die wiederhergestellte Werte
bewusst gespeichert werden müssen. LED-Pin bleibt vom Backup ausgeschlossen.

## Prüfungen

`tests/test_settings_sections.js` führt das tatsächliche eingebettete JS mit
getrenntem EEPROM-/LittleFS-Zustand aus:

- Eindeutige Boxen/IDs, Matrixbox unterhalb von MQTT, genau ein Helligkeits-Save.
- Feldgenaue Anfragen und CSRF, Reihenfolge EEPROM → LittleFS.
- EEPROM-Fehler: keine LittleFS-Schreibanfrage.
- LittleFS-Fehler: bestätigter erster Schritt bleibt erhalten und wird gemeldet.
- Verlorene Antwort nach erfolgreichem EEPROM-Schreiben: keine falsche
  Erfolgsmeldung und kein automatischer zweiter Schreibzugriff.
- Live-Quelle/-Wert aus gespeichertem Zustand, nicht aus Checkboxentwürfen.
- Statuspoll erhält Entwürfe und Teilfehlerhinweise.
- Snapshot und paralleler Klick während einer laufenden Speicherung.
- Anzeige- und Hardware-Speichern ändern keine fremden Boxwerte.

Bestehende GUI-Tests an die neue Bedienung angepasst. Der alte Testtransport,
der bei HTTP 200 einen Nicht-JSON-Text lieferte, wurde auf die tatsächlich
vorhandene API-Antwort `{ok:true}` korrigiert; Erfolg muss bestätigt sein.
Bestehende native Tests prüfen echte EEPROM-/LittleFS-Fehlerpfade und die
API-Sicherheitsmechanismen. Ganze Hostsuite: **71 PASS-Gruppen**, mit ASan/UBSan und beiden Adaptern.
Keine Tests mit angeschlossenen Geräten oder echtem Browser durchgeführt;
Desktop-/Smartphone-Darstellung bleibt visuell zu prüfen.

## Build und Abgrenzung

| Ziel | OTA-BIN | OTA-Limit | Reserve | Statisches RAM |
| --- | ---: | ---: | ---: | ---: |
| Wemos D1 mini | 733280 B | 1044464 B | 311184 B | 51920 / 81920 B |
| ESP32 WROOM 4 MB | 1905952 B | 1966080 B | 60128 B | 66588 / 327680 B |

ESP32 sektorgenau 57344 B Reserve. RAM unverändert zu Beta.5.2, keine neue
Laufzeit-Heapmessung. MQTT-Speicherpfad 96+272=368 B vor weiteren Aufrufen.
Die ESP32-BIN ist 928 B, die Wemos-BIN 4192 B größer als Beta.5.2.

main.cpp außerhalb des eingebetteten HTML/JS bytegleich zum Ausgangscommit.
Keine Änderung an Uhr-/Nacht-/Helligkeitsberechnung, Matrixansteuerung,
MQTT-Controller/-Parser/-Topics, Wetter/Sensoren, HTTP-API-Verträgen,
EEPROM-/Credentials-Layouts, Partitionen und OTA-Sicherheitsprüfungen.
Beide PlatformIO-Profile bauen dieselbe neue interne Versionsnummer.
Generierte ESP32-Gzip-Ressourcen werden auf Quellgleichheit und im echten BIN
geprüft. Tatsächliche Dateien werden durch die produktiven OTA-Validatoren
inklusive negativer Gegenplattform-/Factory-/Manipulationsfälle geprüft.

Build-/Test-/Paketnachweise privat unter `.pio/*beta53*` im Entwicklungscheckout.
SHA-256 und detaillierte Buildinformationen unter `dist/2.0.11-beta.5.3/`.
Ältere Testartefakte werden nicht überschrieben.

## Noch notwendige Hardware-/Browserprüfung

- Auf beiden Boards die drei Boxen, Mobilbreite, Beschriftung und ausgegraute
  Eingaben visuell prüfen. Jeder Speichern-Button darf nur die eigene Box ändern.
- Tag-/Nachtwerte und beide Richtlinien gemeinsam speichern und nach Neuladen/
  Kaltstart kontrollieren. Kein Formatieren für diesen Test.
- MQTT-Vorgaben und Nachtabschaltung weiter auf Vorrang prüfen; „Wirksam“ muss
  der tatsächlichen Ausgabe entsprechen. Nicht gespeicherte Auswahl darf nicht
  als aktiver Zustand erscheinen.
- Einzelne Matrixoptionen und bewusst bestätigten Pin-Neustart prüfen.
- Teilfehler sind automatisiert simuliert; keine absichtliche Beschädigung oder
  Stromunterbrechung während Schreibzugriffen auf produktiven Geräten.
- Login/Ablauf, Backupvorschau, Wetter/Sensoren und weiteres OTA prüfen.

Kein Flashen, Gerätekontakt, Formatieren, Push, Tag oder Release in diesem Auftrag.
ESP32-Factory-Datei ausschließlich für USB-Neuinstallation ab 0x0, niemals OTA;
sie überschreibt alle 4 MiB einschließlich Nutzerdaten. Für Updates die jeweils
passende OTA-BIN verwenden.
