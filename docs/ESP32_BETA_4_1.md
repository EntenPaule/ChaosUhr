# ESP32 2.0.11-beta.4.1 – interne Untersuchung

## Phase A (vor Änderungen)

Ausgang: Entwicklungscommit 0709bb1, sauber. ESP32 espressif32 8.0.0,
Arduino 3.3.12 / IDF 5.5.5, esp32dev, DIO/40 MHz, 4 MiB. Kein 8-MiB-Zwang.
Vorhandene geprüfte Beta.3-BIN: 1.965.072 Byte; Slot 1.966.080 Byte,
Reserve 1008 Byte. Diese Angabe beschreibt die vorhandene Beta.3, keinen Beta.4-Neubuild.
Partitionen: NVS 0x9000/0x5000, OTA-Daten 0xe000/0x2000,
app0 0x10000/0x1e0000, app1 0x1f0000/0x1e0000,
LittleFS (Label spiffs) 0x3d0000/0x30000. Tabelle unverändert beibehalten.
NVS enthält WLAN und EEPROM-Emulation (Uhr/Admin); LittleFS enthält MQTT,
Anzeigeeinstellungen, Uploads sowie Speicherjournal. OTA-Daten wählen Startslot.

nm/Map: INDEX_HTML 34432, ERROR_JS 20836, OTA_HTML 10262 Byte,
zusammen 65530 Byte statische, unkomprimierte Browserdaten. Hinzu kommen
WiFiManager, Netzwerkstack und Arduino/IDF. Größte einzelne Funktionen u.a.
printf/scanf, HTTP-Parser, WiFiManager und Fehlerkatalog. Keine TLS-Bibliothek
als nachgewiesenen Hauptverursacher identifiziert. Geplante gezielte Optimierung:
verlustfreie Gzip-Auslieferung dieser drei Ressourcen nur auf ESP32; keine
Funktionen entfernen und keine Partitionen vergrößern.

LittleFS begin() formatiert standardmäßig NICHT. FS-001 sagt nur Mount fehlgeschlagen.
Ein vollständig mit FF gefüllter Bereich ist uninitialisiert; nichtleere Bereiche
können beschädigt sein, ein anderes Layout/Dateisystem enthalten oder einen anderen
Fehler haben. -84 allein unterscheidet diese Fälle nicht. Bisherige Factory-BIN
füllt LittleFS mit FF. Bewusste Initialisierung existiert im authentifizierten GUI
mit CSRF und INITIALIZE-Bestätigung. Keine automatische Formatierung ergänzen.

ESP32-WROOM hat kein natives USB: USB-UART-Brücke (Boardabhängig) an UART0,
115200 Baud. Aktuell benötigt das geschützte Erstsetup den Matrixcode. Die
WLAN-Persistenz wird bereits vor Funkinitialisierung eingeschaltet. Ein physisch
lokaler, begrenzter UART-Einrichtungsweg soll nur bei noch fehlendem Admin einen
vom Benutzer eingegebenen AP-Code setzen, ohne Geheimnisse auszugeben. Danach
bleibt der vorhandene AP-/Browserablauf erhalten.

## Implementierter interner Stand

ESP32-Version **2.0.11-beta.4.1** über ausschließlich ESP32 gesetztes Build-Flag;
der Wemos behält **2.0.11-beta.4**, seine vorhandenen Distributions-BINs bleiben unverändert.
Keine MQTT-, Wetter-, Automatik-, Nachtmodus- oder Sitzungslogik geändert.

- INDEX_HTML, OTA_HTML und ERROR_JS werden auf ESP32 als Gzip aus Flash gesendet.
  Kein Entpacken auf dem Controller, kein zusätzlicher dynamischer RAM-Puffer.
  Der Generator kompiliert die originalen C++-Stringinitialisierungen, damit auch
  bedingte Stringverkettungen korrekt bleiben. ESP32 erhält zusätzlich den präzisierten
  Initialisierungswarntext und die neue Skript-Cachekennung. Der Wemos sendet weiter Klartext.
  Nach GUI-Änderungen `python3 scripts/compress_esp32_web.py` ausführen;
  `--check` und die ESP32-Hostsuite erkennen veraltete Daten. Generator benötigt g++.
- `WM_NODEBUG` entfernt die bereits per setDebugOutput(false) deaktivierten
  WiFiManager-Debugpfade einschließlich potentieller Zugangsdaten-Ausgaben.
- Core-Dump-Policy: Der gepinnte Arduino-Core enthält Flash-Dumps ohne passende
  Partition. Zwei explizite Linker-Wrapper ersetzen ausschließlich
  `esp_core_dump_init` und `esp_core_dump_write` durch leere IRAM-Funktionen.
  Die Firmware enthält laut nm keine ursprünglichen Dump-Funktionen mehr,
  aber weiterhin die Panic-Handler. Kein Flash-Core-Dump, keine Dump-Partition,
  keine Abschaltung von Watchdogs/Panic/Neustart. UART-Panic-Backtrace bleibt
  vorgesehen; echte Panic-Prüfung steht aus. Compile-Guard erzwingt erneute
  Prüfung bei Frameworkwechsel. Kein bloßes Überdecken der Warnung durch Logfilter.
  Hintergrund: [Espressif Core-Dump-Konfiguration](https://github.com/espressif/esp-idf/blob/v5.5.5/components/espcoredump/Kconfig).
- Authentifizierte Supportdiagnose enthält bei ESP32 `filesystemEvidence`:
  `mounted`, `erased-uninitialized`, `nonempty-mount-failed`, `read-error` oder
  `layout-mismatch`. Vollständiger FF-Nachweis liest ausschließlich 192 KiB
  in 256-Byte-Schritten mit yield. Nichtleere Fehlerfälle werden nicht als
  eindeutig beschädigt bezeichnet. Es werden keine Dateiinhalte ausgegeben.
- Vor GUI-Initialisierung muss die Partition exakt Label spiffs, Typ data/spiffs,
  Offset 0x3d0000, Größe 0x30000 und mindestens 4 MiB physischem Flash entsprechen.
  Weiterhin Admin-Sitzung, CSRF, Origin und bewusste INITIALIZE-Bestätigung.
  Kein automatischer Formatierungsweg ergänzt. NVS und OTA-Partitionen unverändert.

## USB-Ersteinrichtung ohne Matrix

1. Klassischer ESP32-WROOM/esp32dev mit **mindestens 4 MiB Flash** und USB-UART-Brücke.
   Kein ESP32-S3/WT32-Display-Build. Größere Flashchips nutzen dasselbe untere
   4-MiB-Layout; 2-MiB-Boards werden nicht unterstützt. Keine Portsuche/kein Flashen
   erfolgt automatisch. Ein Daten-USB-Kabel und ggf. passender USB-UART-Treiber sind nötig.
2. Das Factory-Image ist ausschließlich für eine bewusst bestätigte Neuinstallation
   bestimmt: vollständiges 4-MiB-Abbild, USB-Schreibadresse **0x0**, DIO/40 MHz.
   **Es überschreibt vorhandene NVS-, WLAN-, Admin-, MQTT- und Animationsdaten.**
   Deshalb für bestehende Installationen nur das OTA-Image verwenden. Keine
   erase-flash-Anweisung; kein Gerät wurde in diesem Auftrag beschrieben.
3. Nach Start binnen fünf Minuten, solange noch kein Admin angelegt ist:
   `/home/paulr/.platformio/penv/bin/python scripts/esp32_usb_setup.py /dev/ttyUSB0`
   (Port selbst passend wählen; alternativ Python mit pyserial.) Ein eigener,
   zufälliger zwölfstelliger Einrichtungscode wird zweimal verdeckt abgefragt.
   Das Hilfsprogramm setzt DTR/RTS auf inaktiv und gibt keine Roh-Logs aus.
   Boardabhängige Resetimpulse beim Öffnen sind trotzdem am Gerät zu prüfen.
4. Bestätigung abwarten. Mit `ChaosUhr-Setup-…` verbinden, den gewählten Code als
   AP-Passwort eingeben und `http://192.168.4.1/` öffnen. Dort wie bisher Admin-Passwort
   setzen, danach Heim-WLAN über den bestehenden Konfigurations-AP einrichten.
   Der Code gilt auch für den anschließenden Konfigurations-AP. WLAN-Persistenz
   wird weiterhin vor Radioinitialisierung aktiviert. Matrix/AP bleiben verfügbar.
5. GUI anmelden und LittleFS nur nach bewusst akzeptiertem Datenverlust einrichten.
   Factory-LittleFS ist absichtlich vollständig FF, kein implizit formatiertes Image.
   FS-001 ist vor diesem Schritt erwartbar; nach erfolgreicher Initialisierung darf
   er nicht erneut als aktueller Mountfehler auftreten. Historische Einträge bleiben möglich.
6. WLAN-Konfiguration nach mindestens 30 Sekunden vollständiger Stromtrennung prüfen.

UART-Eingaben werden nie zurückgeschrieben oder in Logs übernommen. Fester
19-Byte-Puffer, maximal 32 Zeichen pro Loop, Überlängen/ungültige Zeilen verworfen,
Puffer nach Zeile/Fristende/Zustandswechsel gelöscht. Nach Admin-Einrichtung,
bei gesperrten Zugangsdaten oder Reset ist der Weg gesperrt. Physischer Zugriff
innerhalb des Erstsetup-Fensters ist die Vertrauensgrenze; kein zusätzlicher
Netzwerkendpunkt, keine Übernahme eines eingerichteten Admins. Python-Strings sind
nicht garantiert aus dem PC-Speicher löschbar; keine dauerhafte Speicherung vorgesehen.
HTTP ist lokal unverschlüsselt und bietet keinen TLS-Schutz gegen Mitschneiden.
WebSerial wurde nicht ergänzt: Der lokale Python-/UART-Weg benötigt keine
Webseite, Browserfreigabe oder externe Skriptquelle und funktioniert browserunabhängig.

## Verifikation und Artefakte

Finaler ESP32-Build SUCCESS (Arduino 3.3.12, espressif32 8.0.0).
Tatsächliche OTA-BIN **1.883.936 Byte**, Slot **1.966.080 Byte**, freie Reserve
**82.144 Byte** (80,22 KiB), sektorgenau frei **81.920 Byte**.
Das sind 81.136 Byte weniger als die Beta.3-BIN, ohne Partitionsänderung.
Statischer RAM **66.452 / 327.680 Byte**, Linker-Flash **1.822.639 Byte**;
Linkeranzeige und tatsächliche BIN-Größe sind ausdrücklich unterschiedlich.

59 bestehende Host-PASS-Gruppen für beide Plattformadapter, zusätzlich gezielte
Tests für seriellen Parser (gültig/CRLF/zu lang/ungültig/Abbruch) und lesende
FS-Diagnose (FF bis letztes Byte, nichtleer, Lesefehler, falsches Layout,
2-/4-/8-MiB-Prüfung), Gzip-Generator --check und Original-C++-Stringauswertung.
USB-Hilfsprogramm zusätzlich mit simuliertem Port geprüft (Öffnungsreihenfolge,
verdeckte Bestätigung und keine Codeausgabe); kein echter Port geöffnet.
Keine erneute Wemos-Firmware gebaut und keine bestehende BIN überschrieben.

`python3 scripts/package_esp32_foundations.py` prüft vor dem Anlegen des neuen
Verzeichnisses: echte OTA-Validierung inkl. falschem Chip, Factory, Kürzung,
Manipulation und falscher SHA; Espressif-Image-Hashes; echte PlatformIO-Offsets;
Partition-CSV/BIN/MD5; Factory-Komponenten und alle FF-Lücken bytegenau;
min. 64 KiB OTA-Reserve; Linkersymbole für Gzip/Dump-Policy; Secrets-Scan.
Das Skript verweigert vorhandene Zielverzeichnisse. Prüfsummen und BUILD_INFO
liegen neben den beiden Images unter `dist/2.0.11-beta.4.1/`.
Kein separates LittleFS-Abbild: Factory enthält an genau 0x3d0000 die unveränderte
192-KiB-FF-Partition. OTA enthält ausschließlich die Anwendung und überschreibt
weder NVS noch LittleFS noch die Partitionstabelle. Factory niemals als OTA verwenden.

## Offene Hardwaretests / Grenzen

**Interner, hardwareungetesteter Kandidat, keine Freigabe für produktive Geräte.**
- Ohne Matrix: UART-Fenster, Codewechsel, AP-Verbindung, Admin- und WLAN-Einrichtung.
- Persistenz nach vollständiger Stromtrennung, IPv4/DNS und Wiederverbindung.
- Frischer FF-Speicher, bewusst initialisierte Partition, bereits belegtes und
  beschädigtes LittleFS: Diagnose korrekt, keine selbsttätige Formatierung.
- MQTT-, Datei- und Anzeigeeinstellungen vor/nach OTA byteinhaltlich erhalten.
- Browser lädt Gzip-GUI/OTA/Fehler-JS; Login/Logout/CSRF, Upload und OTA in beide Slots.
- Unterbrochenes OTA startet weiterhin eine gültige Anwendung; Wiederherstellung
  notfalls per USB. Kein neues Rollback-Feature implementiert.
- UART-Panic-Backtrace und Ausbleiben der Flash-Core-Dump-Warnung prüfen;
  absichtliche Crash-/Watchdogtests nur auf ausdrücklich freigegebenem Testgerät.
- 4-MiB-Board sowie vorhandene größere Flashvarianten und Betrieb mit Matrix testen.

## Zusätzliche Abschlussnachweise

Wemos und klassischer ESP32 verwenden üblicherweise eine USB-UART-Brücke.
Der Wemos behält seinen bisherigen Matrix-/AP-Einrichtungsweg; der neue
verdeckte UART-Codeeingang ist ausschließlich für ESP32 kompiliert.
Die beiden betroffenen Wemos-Übersetzungseinheiten main.cpp/public_access.cpp
sind nach Präprozessierung zum Ausgangscommit 0709bb1 äquivalent; normalisiert
wurden Leerzeilen, gleichwertige Empfängerklammern/Pointer-Schreibweise und der
rein zeilennummerabhängige INDEX_HTML-Abschnittsname. Keine neue Wemos-BIN.
Alle drei Gzip-Ressourcen sind bytegenau in der tatsächlich erzeugten ESP32-BIN
enthalten; entpacktes JavaScript zusätzlich mit node --check geprüft.
Firmwarequellstand lokal: 4612602a63e4f21e55d243e21d87867ceed5a7d4.
Die nachfolgende Dokument-/Artefaktsicherung enthält keine weiteren Firmwareänderungen.
