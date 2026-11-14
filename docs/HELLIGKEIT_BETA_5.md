# ChaosUhr 2.0.11-beta.5 – interne Helligkeitssteuerung

Nur lokaler Teststand. Kein Release/Tag/Push, kein Zugriff auf Geräte, keine
Hardwareinstallation oder Formatierung durch die Entwicklung. Ausgangscommit
dc852e15797b1fbf0e4fd5bf84ea2557638af80f. Entwicklung ausschließlich im verifizierten
Checkout `.pio/dev-2.0.11`, Branch fix/2.0.11-diagnostics. Vor späterer Verwendung
im geöffneten Projekt werden nur einzeln geprüfte öffentliche Dateien abgeglichen.
Beide Plattformen melden gemeinsam 2.0.11-beta.5. Das vorhandene klassische
ESP32-Profil heißt esp32dev_public (nicht esp32_public), das Wemos-Profil d1_mini_public.

## Bedienung

Im Nachtbereich befinden sich zwei standardmäßig deaktivierte Optionen:
„LEDs im Nachtmodus vollständig ausschalten“ und „LED-Helligkeit über MQTT steuern“.
Sie werden mit „Helligkeitsrichtlinie speichern“ unabhängig von den übrigen
Uhreinstellungen gespeichert. Ohne eingerichtetes LittleFS erscheint ein Fehler;
es erfolgt weder automatische Initialisierung noch ein vorgetäuschter Erfolg.
Die Nachthelligkeit bleibt gespeichert; ihr Regler ist bei Nacht-AUS ausgegraut.

Die Anzeige zeigt Quelle, wirksamen Prozentwert und vorhandene MQTT-Vorgabe.
Das Befehlstopic wird bei freigegebener Steuerung angezeigt. Status wird während
einer Admin-Sitzung alle fünf Sekunden ausschließlich per GET aktualisiert; dies
verlängert die Sitzung nicht und überschreibt keine ungespeicherten Checkboxen.

## Prioritäten

| Zustand | Verhalten |
|---|---|
| Gültige Nachtzeit und Nacht-AUS | Explizit schwarzer Frame, FastLED-Helligkeit 0 |
| Sonst gültige freigegebene MQTT-Vorgabe | Absoluter MQTT-Prozentwert |
| Sonst | Gespeicherte GUI-Tag-/Nachthelligkeit |
| Keine gültige Uhrzeit | Bisheriges Tagverhalten |
| MQTT 0 | Expliziter schwarzer Frame |
| MQTT 100 | Bisherige Obergrenze 120/255 bleibt bestehen |
| Nacht endet | Aktueller MQTT-Wert, sonst GUI-Taghelligkeit |
| GUI-Wert während MQTT-Steuerung geändert | Wert wird gespeichert, MQTT bleibt maßgeblich |
| Brokerverlust | Letzter gültiger RAM-Wert bleibt bestehen |
| MQTT oder Steuerung deaktiviert | RAM-Vorgabe wird gelöscht |
| Brokerhost, Port oder Basis-Topic gewechselt | Alte RAM-Vorgabe wird gelöscht |
| Neustart | Zunächst GUI, danach ggf. erneut empfangener Retained-Wert |

Ein schwarzer Frame wird vor Sonderanzeigen übertragen; auch Sekundenleiste,
IP- und Setup/Reset-Anzeigen dürfen erkanntes Nacht-AUS nicht umgehen. Netzwerk,
Uhr, MQTT und Szenenwechsel laufen im Hauptloop weiter. Ohne neue Optionen bleibt
das normale Tag-/Nachtdimmen einschließlich Sonderanzeigen unverändert.

## MQTT

Befehlstopic: `<Basis-Topic>/brightness/set`. Das bereits vorhandene Basis-Topic
bleibt unverändert, kein neues Feld in MqttConfig, keine zusätzlichen Zugangsdaten.
MQTT muss aktiviert sein UND die Helligkeits-Checkbox ausdrücklich gespeichert sein.

Erlaubt sind ASCII-Ganzzahlen 0 bis 100 und exakt AUTO. Zahlen mit führenden
Nullen bis maximal drei Ziffern sind ebenfalls vollständige Ganzzahlen im Bereich.
Nicht erlaubt: Leerzeichen, Vorzeichen, Dezimalzahlen, andere Texte, Nullbytes,
Payloads über vier Zeichen (AUTO) bzw. drei Ziffern. Ungültige Befehle ändern nichts.
Die bestehende Inbox setzt Fragmente erst vollständig zusammen; unvollständige
oder inkonsistente Nachrichten werden nicht angewendet. Retained-Nachrichten
werden auf demselben Weg wie normale Nachrichten behandelt.

Wetter-/Temperatur-/Sensortopic gleich dem Steuerungstopic: Helligkeitssteuerung
wird effektiv gesperrt, bestehende Telemetrie bleibt bestehen. GUI zeigt den
Konflikt, MQTT-023 beschreibt ihn ohne Inhalte. Bei deaktivierter Steuerung darf
ein früher noch abonniertes Steuertopic keine Helligkeit ändern; beim nächsten
Reconnect wird es nicht mehr als Steuerungstopic abonniert. Kein neues Discovery-
Entity, bestehende Discovery- und Availability-Nachrichten bleiben unverändert.

„MQTT-Vorgabe aufheben“ benötigt Cookie-Session und CSRF sowie eine aktive
MQTT-Verbindung ohne Topic-Konflikt. Es veröffentlicht retained AUTO mit QoS1.
Nur wenn publish() eine von null verschiedene Paketkennung liefert, wird die
lokale Vorgabe gelöscht und HTTP202 mit queued:true, brokerConfirmed:false geliefert.
Die Rückgabe bestätigt NUR die Annahme in die Bibliothekswarteschlange. PUBACK
oder dauerhafte Brokerpersistenz werden nicht behauptet; bei sofortigem
Verbindungsabbruch ist der Brokerzustand unbekannt. Bei Ablehnung/Offline bleibt
die lokale Vorgabe bestehen, MQTT-024 wird gemeldet. Kein blockierendes Warten.
Ein externes retained AUTO wird normal akzeptiert. Wer das Topic schreiben darf,
kann die Helligkeit steuern; bestehende Web-Admin-Sitzungen ersetzen keine Broker-ACL.

## Speicherung und Migration

`/brightness-policy.bin`: 12 Byte, Kennung CUBP, Version1, zwei Optionsbytes,
reserviertes Nullbyte, explizit little-endian gespeicherte FNV-1a-Integritätsprüfung.
Keine native Struktur mit plattformabhängigem Padding gespeichert. Temporäre
Datei `/brightness-policy.pending`, Rücklesen/Validierung und atomarer Rename.
Schreiben nur bei geänderten Werten oder ausdrücklich reparierter beschädigter
Richtlinie. Fehlende Datei bedeutet beide Optionen aus. Defekte Datei wird
nicht automatisch gelöscht oder überschrieben; FS-026, Start mit Standardwerten.
Explizites Speichern kann sie ersetzen. Alte Werte bleiben bei Speicherfehler aktiv.
Unterbrochene temporäre Dateien werden nicht als gültige Richtlinie geladen.

Unverändert: Settings32Byte, Credentials88Byte abOffset32, DisplayOptions8Byte
abOffset120, EEPROM128Byte, MqttConfig-v1 mit1088Byte. Kein neuer NVS-Namespace,
keine Partitionsänderung und keine MQTT-Schreibzugriffe auf Flash. Ältere Firmware
ignoriert die neue Datei, statt Anzeigeoptionen/LED-Pin als ungültig zu verwerfen.
Bestätigte Dateisysteminitialisierung/Werksreset löscht diese Datei wie andere
LittleFS-Dateien. Ein unformatierter FS wird nie automatisch formatiert.

## Backupformat 3

Explizit vom Nutzer freigegeben: Neue Exporte verwenden Format3 und zusätzlich
brightnessPolicy mit nightLedsOff/mqttBrightness (jeweils 0/1). Neue Firmware
importiert weiterhin Formate1/2, wobei neue Optionen auf0 gesetzt werden.
Fehlende neue Felder in Format3 bedeuten0; falsche Typen, unbekannte Felder,
Werte außerhalb0/1 und null/Arrays statt Objekt werden abgewiesen. Alte Firmware
kann Format3 NICHT importieren; für sie bisherige Backups aufbewahren.

Import ist zunächst nur Vorschau und schreibt nichts. Übernahme füllt die
Uhrfelder und die beiden Checkboxen. Uhreinstellungen und Helligkeitsrichtlinie
werden bewusst über ihre getrennten Schaltflächen gespeichert: keine behauptete
atomare Speicherung über zwei unabhängige Speichersysteme. MQTT-Laufzeitvorgaben,
Passwörter, Brokerkonfiguration und LED-Pin werden nicht exportiert.

## Automatisierte Nachweise

Native Entscheidungs-/Ausgabetests: alle Tag-/Nachtwerte1..100, 0/1/5/50/100,
AUTO, ungültige Payloads, Sicherheitsdeckel, explizite300schwarzePixel/Show-Aufruf.
Native Speicherung: Defaults, keine unveränderten Writes, Lese-/Schreib-/Rename-
Fehler, beschädigte Datei, bewusste Reparatur, temporäre Restdatei, keine Formatierung.
Echte API-Handler: Auth/CSRF, Eingabefelder, fehlgeschlagene atomare Speicherung,
korrekter Status, Release-Fehler erhält Vorgabe und Erfolg behauptet keinen BrokerACK.
Echter ClockMqtt-Controller mit Fake-Transport: Opt-in, Inbox-Befehle, Wertwiederholung,
Disconnect/Reconnect, Broker-/Basiswechsel, Deaktivierung, AUTO-Publishfehler,
QoS1/retain auf Erfolg und Kollision bei weiter funktionierender Wettertelemetrie.
GUI: geschützte getrennte Speicherung, Quelle/Topic/Vorgabe, Konfliktanzeige,
keine behauptete Brokerbestätigung. Backup1/2/3 einschließlich fehlender neuer
Felder und fehlerhafter Daten; bestehende Sitzungs-/Wetter-/Sensortests bleiben aktiv.
Hosttests ersetzen keine tatsächliche MQTT-Broker- oder LED-Hardwareprüfung.

## Hardware-Testcheckliste (beide Plattformen, noch offen)

1. Vorher Konfiguration und Animationen sichern; nur passende OTA-BIN bewusst
   installieren. Die Entwicklung hat nichts geflasht. Factory-BIN ausschließlich
   Neuinstallation ab0x0, überschreibt alle4MiB einschließlich WLAN/Admin/Dateien.
2. Beide Optionen aus: bisherige Tages-/Nachthelligkeit und Sonderanzeigen prüfen.
3. Nacht-AUS speichern, gültige Nachtzeit: alle300LEDs inklusiveSekundenleiste aus.
   Web-GUI/MQTT erreichbar, Szenenwechsel laufen weiter. Nachtende stellt aktuellen
   MQTT-/GUI-Wert her. Nachthelligkeit wurde nicht verändert.
4. MQTT-Steuerung bleibt ohne ausdrücklichen Opt-in wirkungslos. Danach Befehle
   0,1,5,50,100,AUTO (ohne äußereLeerzeichen) prüfen. Retained50, Stromtrennung,
   erstGUI/dannMQTT; Nacht-AUS darf nie übersteuert werden.
5. Falsche Werte, Text, Nullbytes, Fragmente und Last verändern die letzte gültige
   Vorgabe nicht. Wetter und vier Sensoren, Discovery und Online/Offline laufen weiter.
6. Broker stoppen: Wert bleibt. Wieder starten: Retained-Wert erneut wirksam.
   GUI-Rückgabe online: retainedAUTO beim Broker separat nachweisen; Ausfall direkt
   nachQueueannahme darf nicht als garantierte dauerhafte Rückgabe interpretiert werden.
   Offline-Rückgabe muss fehlschlagen und den Wert erhalten.
7. Steuerung/MQTT deaktivieren und Broker-/Basis-Topic wechseln: Vorgabe gelöscht.
   Kollision mit Wetter/Sensor: Sperre und MQTT-023, Telemetrie bleibt funktional.
8. GUI-Helligkeit währendMQTT ändern, AUTO senden: neuer GUI-Wert gilt.
9. Warmstart und30sStromtrennung: Richtlinie bleibt, alte Dateien/Credentials unverändert.
   Bei fehlendemFS nur Fehler, keine automatische Formatierung.
10. Backup3 exportieren/importieren, alteBackups1/2 prüfen, ungültigerImport ohneAktivierung.
    Beide getrenntenSpeicheraktionen inklusiveFehlerfällen prüfen.
11. BeiderseitigesOTA inklusiveSlotwechsel und Persistenz separat bestätigen.
    FreierHeap/Stack unterLast undLangzeitbetrieb messen; statischerRAM ist keine Laufzeitreserve.

## Grenzen

Keine neuen Wetter-/Nachtzeit-/MQTT-TLS-/Downgradefunktionen. Keine Abschaltung
von Watchdogs oder Auth/CSRF. Bei noch ungültigerUhrzeit bleibtTagverhalten.
AUTO-Queueannahme garantiert keineBrokerpersistenz. Backup3 benötigtneuenImporter.
Alle neuen Funktionen bleiben bisHardwaretests ein internerBeta-Teststand.

## Abschließende lokale Ergebnisse

GemeinsameVersion2.0.11-beta.5, beidePlatformIO-ProfileSUCCESS. Initial beide
Buildverzeichnissebereinigt, nachfinalenGUI-/Controlleränderungen beideerneutgebaut.

| Plattform | TatsächlicheBIN | OTA-Grenze | Dateireserve | StatischerRAM | Rechnerisch nicht statisch belegt | LinkerFlash |
|---|---:|---:|---:|---:|---:|---:|
| Wemos | 698000 | 1044464 | 346464 | 51920/81920 | 30000 | 693851 |
| ESP32 | 1895968 | 1966080 | 70112 | 66588/327680 | 261092 | 1833939 |

ESP32sektorgenauübrig69632Byte. Factory4194304Byte, ausschließlichUSB-Neuinstallation
ab0x0. KeinePartitionsänderung. GegenüberbestätigterWemosBeta.3: BIN+8544Byte,
statischerRAM+376Byte. GegenüberESP32Beta.4.2: BIN+11296Byte,RAM+128Byte.
DieRAM-Restwerte sind KEINE gemessenen freienLaufzeit-Heap-/Stackwerte.
MQTT-Handlerstackweiter96Byte plusStorage272Byte, kombiniert368vorUnterfunktionen.

Hostsuite nachAbschnitten:64,65,65,67PASS-Gruppen; final68PASS-Gruppen.
ZusätzlichegezielteAPI-/GUI-/MQTT-Tests wurden beiErgänzungenausgeführt.
Zwischenläufe mit veralteten komprimierten Ressourcen bzw. fehlender Timer-
Umgebung beim isolierten GUI-Test wurden nicht ignoriert: Ressourcen regeneriert,
Timerstart in die tatsächlicheGUI-Initialisierungverlegt, gesamteSuiteerneutPASS.
Logs `.pio/tests-brightness-{1,2,3,4}.log`, `.pio/tests-beta5-complete.log`,
`.pio/build-beta5-clean.log`, `.pio/build-beta5-both.log`, `.pio/build-beta5-final.log`.
Keine dieser68Gruppen ist ein echterHardwaretest.

Das lokalePaketierungsskript `scripts/package_brightness_test.py` verlangt einen
committetenöffentlichenQuellstand und prüft echteOTA-Dateien mit beiden
Produktionsvalidatoren einschließlichMutation/Kürzung/falscherSHA/Gegenplattform/
Factory-Ablehnung. ESP32Imagehashes, echtePlatformIO-Offsets, Partitionstabellen-
MD5/CSV, Factorybestandteile und sämtlicheFF-Lücken werden geprüft. Die tatsächlich
enthaltenenGzip-Ressourcen werden bytegenauverglichen. Scan mit den privaten
Vergleichswerten des ursprünglichenProjektordners, ohne derenAusgabe. NeueAssets
unterdist/2.0.11-beta.5werdennichtüberschrieben; historischeArtefaktebleibenerhalten.
