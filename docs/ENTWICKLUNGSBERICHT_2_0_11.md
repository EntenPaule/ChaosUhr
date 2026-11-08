# ChaosUhr 2.0.11 – Ursachen, Änderungen und Tests

Lokaler Entwicklungszweig `fix/2.0.11-diagnostics`, Basis des produktiven Quellbranches: `0f06cd6f5fcb394b8873f154c848b1cbfe1f14e8`. Keine Veröffentlichung, kein Tag und keine Veränderung des produktiven Branches oder des Updatekanals. Die vorliegenden BINs sind Entwicklungsartefakte, keine Hardwarefreigabe.

## Nachgewiesene Ursachen

### ESP32-WLAN

1. `PublicAccess::begin()` schaltete mit `WiFi.persistent(false)` vor dem ersten `WiFi.mode()` die Persistenz aus. Arduino-ESP32 3.3.12 wählt bei dieser ersten Treiberinitialisierung `WIFI_STORAGE_RAM`. Sein späteres `persistent(true)` ändert nur das Initialisierungsflag, nicht den schon gewählten SDK-Speicher. Deshalb reicht WiFiManagers spätere Umschaltung beim Speichern nicht aus, um NVS-Speicherung zu garantieren.
2. Der Startcode behandelte `WiFi.SSID().isEmpty()` als fehlende gespeicherte Konfiguration. In diesem Core liest `STA.SSID()` aber den verbundenen Access Point. Während des Verbindungsaufbaus oder bei Routerausfall ist dieser Wert leer. Dadurch wurde die 30-Sekunden-Frist übersprungen, obwohl gespeicherte Zugangsdaten vorliegen konnten.

Korrektur: gemeinsame plattformspezifische Persistenzpolitik vor dem Funkstart, Flash/NVS auf ESP32 und bisheriges Verhalten auf ESP8266. Auf ESP32 fragt die Anwendung die gespeicherte SDK-Konfiguration ab und unterscheidet vorhanden, fehlend und nicht lesbar; nach erfolgreichem Lesen wird die temporäre Struktur gelöscht. Routerausfälle führen weiterhin über Wiederverbindung/Recovery, ohne Credentials-Erase. Ein Werksreset bleibt ausdrücklich zu bestätigen.

Primärquellen des gepinnten Cores: [WiFiGeneric.cpp](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/WiFiGeneric.cpp), [STA.cpp](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/STA.cpp). Zusätzlich wurde die tatsächlich installierte WiFiManager-Version 2.0.17 im Quellcode geprüft. Globale Framework- und Bibliotheksdateien wurden nicht verändert.

### MQTT-Speichermeldung

Nachgewiesen ist der Pfad für die leere Meldung: Der bisherige Authentifizierungsfehler konnte HTTP 401 ohne Text liefern. Die GUI übernahm `response.text()` ungeprüft in `Error(...)` und zeigte nur das Präfix. Das ist mit leerer 401-Antwort gegen die tatsächliche Formularfunktion reproduziert.

Die konkrete ursprüngliche HTTP-Antwort des gemeldeten Geräts wurde nicht aufgezeichnet und das Gerät nicht angesprochen. Deshalb wird **nicht behauptet**, dass sein Broker-Passwort falsch war oder ein bestimmter LittleFS-Defekt vorlag. Es wurde auch kein reproduzierbarer Rename-Defekt als ursprüngliche Ursache nachgewiesen.

Korrektur: authentifizierte Fehlerantworten als JSON mit Code, Beschreibung, technischer Ursache und Maßnahme; HTTP-Status bleibt separat. Alle GUI-Oberflächen verwenden dieselbe Antwortauswertung. Leere/alte Antworten erhalten statusgerechte Fallbacks, Netzwerkfehler geben keine ungeprüften Exception-Texte aus. Fehlgeschlagene Schreibaktionen werden nicht automatisch wiederholt.

LittleFS-Schritte werden einzeln geprüft: Mount, Öffnen, vollständiges Schreiben, erneutes Öffnen, vollständiges Lesen, Inhaltsvergleich und atomarer Rename. Der gültige alte Dateiname wird nicht vorab entfernt. Laufende und gespeicherte Konfiguration bleiben bei Fehlern erhalten, ebenso eine letzte gültige Konfiguration bei Lesefehlern. Fehlende MQTT-Dateien auf frischen Geräten sind kein Defekt. Es wird niemals aufgrund eines MQTT-Fehlers automatisch formatiert.

## Fehlercodes und Schnittstellen

Die vollständige Liste wird aus `include/error_catalog.def` erzeugt: [FEHLERCODES.md](FEHLERCODES.md). Der Codevertrag unter `tests/error_code_contract.json` verhindert Umnummerieren und Wiederverwendung bestehender IDs. `python3 scripts/generate_error_docs.py --check` prüft die Synchronität von Quelle, GUI-Fallbacks und Dokumentation.

Erfasst sind WLAN/Setup, MQTT-Konfiguration und -Transport, Dateisystem/Journal, HTTP/API, Anmeldung/Berechtigung, OTA, Einstellungen einschließlich Sicherung/Animationen und Systemfehler. Benannte Fehlerpfade verwenden dieselben Definitionen in API, GUI und serieller Ausgabe. Bei nicht genauer erkennbaren Ursachen wird der dafür vorgesehene allgemeine Code verwendet.

- API-Fehler: `code`, `component`, `message`, `details`, `action`, `status`; Erfolgsschemata und HTTP-Statuscodes bleiben soweit möglich erhalten.
- `GET /api/mqtt`: Speicher-/Konfigurationsfehler und Verbindungsfehler getrennt. Brokerantworten 4/5 und konkrete Transportfehler werden von allgemeinen Abbrüchen unterschieden.
- `GET /api/status`: letzter Fehlercode zusätzlich zum bisherigen Status.
- `GET /api/diagnostics`: Admin-geschützte Supportdiagnose mit Version, Plattform, Uptime, Heap, Verbindungsflags, gespeichertem WLAN-Status und höchstens acht RAM-Ereignissen.
- Keine Passwörter, SSIDs, Brokeradressen, Benutzernamen, Topics, Sensorwerte oder Session-/CSRF-Daten in der Supportdiagnose. Serielle Fehlerausgaben verwenden ausschließlich statische Katalogtexte und Uptime.
- Wiederholungen werden begrenzt zusammengefasst. Fehleraufzeichnung verursacht keine fortlaufenden Flash-Schreibzugriffe. Letzter Fehler und Verlauf sind historisch; ein alter Eintrag ist kein Beweis für einen noch aktiven Defekt.
- Das gemeinsame Browser-Skript ist lokal, versioniert und ohne Zugangsdaten. Scheitert sein Laden, zeigt ein aus demselben Katalog erzeugter Bootstrap einen Fehlercode an.

## Ressourcenschutz

Ein Zwischenbuild zeigte, dass das tatsächliche ESP32-Image trotz erfolgreicher Linker-Größenprüfung nicht mehr in den unveränderten OTA-Slot passte. Der Katalog wird deshalb für die Firmware in genau einer Übersetzungseinheit abgelegt. Damit entfallen mehrfach im Flash gehaltene Zeichenketten. Die vollständige BIN-Größe wird zusätzlich zur Linkeranzeige geprüft; Partitionen wurden nicht vergrößert.

## Testergebnisse

Die vollständige lokale Host-Suite ist erfolgreich durchgelaufen: **52 PASS-Gruppen**, darunter native C++-Tests mit AddressSanitizer/UndefinedBehaviorSanitizer und JavaScript-Tests der tatsächlichen GUI-Funktionen. Beide PlatformIO-Builds sind erfolgreich. Es gab keine Gerätezugriffe, keinen Upload, keine Broker-/Routeränderungen und keine GitHub-Schreibzugriffe.

| Prüfung | Ergebnis und Aussagegrenze |
| --- | --- |
| ESP32-Persistenz | Tatsächliche plattformspezifische Richtlinie mit SDK-Modell geprüft: Negativkontrollen reproduzieren RAM-Speicherung und die SSID-Verwechslung; drei simulierte Kaltstarts, Routerausfall, Moduswechsel und nicht lesbare Konfiguration bestehen. Keine physischen Kaltstarts. |
| MQTT-Dateispeicher | Mount-/Open-/Schreib-/Lese-/Verifikations-/Rename-/Cleanup-Fehler injiziert; alte Datei und letzte gültige Konfiguration bleiben erhalten. Wiederholtes Speichern/Laden sowie unveränderte Speicherung ohne Schreibzugriff geprüft; kein automatisches Formatieren. |
| MQTT-Verbindung | Tatsächlicher `ClockMqtt`-Controller mit simuliertem Transport: DNS, TCP, Timeout, Broker-/Login-/Berechtigungsablehnung, allgemeiner Abbruch, Wiederverbindung, Paket-/Abonnementfehler und absichtlicher Verbindungsabbau. Ein ACK-Timeout wird nicht als nachgewiesene Trennung ausgegeben. Kein realer Broker. |
| HTTP und Anmeldung | Beide tatsächlichen Projektparser unter Sanitizern: Authentifizierung vor Verarbeitung, Grenzen, Abbrüche und strukturierte Fehler. Leere HTTP-401-Antwort gegen die tatsächliche MQTT-Formularfunktion reproduziert und abgefangen; CSRF-, Speicher-, Timeout-, Body-Abbruch- und ungültige JSON-Antworten geprüft. |
| Fehlercodevertrag | Alle 164 IDs eindeutig, fest zugeordnet und mit Beschreibung/Ursache/Maßnahme versehen. Generatorprüfung bestätigt Firmware-/Browser-/Dokumentationsgleichheit. RAM-Verlauf, Wiederholungszählung, Zeitüberlauf und tatsächlich verwendeter serieller Formatierer geprüft. |
| Supportdiagnose und Geheimnisse | Admin-Schutz und Ausschluss von Test-Passwörtern, Host-/Benutzer-/Topic-/Tokenwerten geprüft. Zusätzlich 166 öffentliche Quelldateien, beide tatsächlichen BINs und finale Protokolle auf bekannte private Zugangsdaten und Token-/Schlüsselmuster geprüft. |
| Bestehende Funktionen | Bestehende Einstellungen, Migration ab 2.0.5, Journal und unterbrochene Resets, Animationen/Uploads, deutsche Datumsanzeige, Mapping, MQTT-/Anzeigeoptionen, Sicherung, GUI und Updater bestehen die Regressionstests. |
| Tatsächliche OTA-BINs | Beide vollständigen Dateien mit dem produktiven OTA-Validator akzeptiert; geänderte, gekürzte, für den falschen Chip bestimmte und mit falscher SHA deklarierte Dateien zurückgewiesen. Offlineprüfung, kein reales Flashen. |
| Toolchain-Isolation | 219 Wemos- und 160 ESP32-Compiler-Abhängigkeitsdateien ohne `secrets.local.h`. Beide WiFiManager-Builds verwenden den jeweiligen Projekt-Webserver. Alle 30 per Upstream-Manifest erfassten globalen HTTP-Framework-Dateien unverändert. |

### Finale Buildgrößen

| Zielprofil | Statisches RAM | Linker-Flash | Tatsächliche Anwendungs-BIN | Max. Anwendungs-BIN | OTA-Dateireserve |
| --- | ---: | ---: | ---: | ---: | ---: |
| `d1_mini_public` | 50.312 / 81.920 Byte | 666.223 Byte | 670.368 Byte | 1.044.464 Byte | 374.096 Byte |
| `esp32dev_public` | 67.276 / 327.680 Byte | 1.875.167 Byte | 1.936.400 Byte | 1.966.080 Byte | 29.680 Byte |

Insbesondere der ESP32 hat mit etwa **29 KiB OTA-Dateireserve** wenig Platz für weitere Funktionen. Die tatsächliche BIN-Größe ist maßgeblich; die Linkeranzeige allein reicht zur OTA-Platzprüfung nicht aus. Statisches RAM sagt nichts über Heap-Reserven unter realer Netzwerk-/MQTT-Last aus. Die bestehenden Partitionen bleiben unverändert.

### Entwicklungsartefakte und Prüfsummen

- Wemos: `.pio/build/d1_mini_public/firmware.bin`, SHA-256 `9127c120a623c32bf9106a900f9c5347bc617ffc08d5741f71424e97defc8b4f`.
- Klassischer ESP32: `.pio/build/esp32dev_public/firmware.bin`, SHA-256 `aae84580b8a3a4de118d502322cf9bce81cdb67aef64e4e0d2fac1392184970a`.

Reproduzierbare Aufrufe aus dem Projektstamm: `python3 tests/run_host_tests.py`, `python3 scripts/generate_error_docs.py --check`, `pio run -e d1_mini_public` und `pio run -e esp32dev_public`. Falls PlatformIO nicht im Suchpfad liegt, kann dessen Installation unter `~/.platformio/penv/bin/pio` verwendet werden.

.pio-Protokolle des ersten lokalen Abschlusses bleiben privat erhalten. Erneute Clean-Build-/Hosttest-Nachweise für das Testpaket siehe unten. Private Nachweise: `.pio/tests-2.0.11-final.log`, `.pio/build-2.0.11-wemos-final.log`, `.pio/build-2.0.11-esp32-final.log`, `.pio/validate-bins-2.0.11.log` und `.pio/verification-2.0.11.json`. Sie gehören nicht zur öffentlichen Quellensammlung.

Produktiver Quellbranch unverändert auf `0f06cd6f5fcb394b8873f154c848b1cbfe1f14e8`, Kanal-Checkout unverändert auf `907d954040752ce5c23c15aa869bae0a7c6e4043`, Tag `v2.0.10` unverändert auf `c976bf04552562a70e21625b78c789c0436424e5`. Die fünf bisherigen 2.0.10-BIN-/ZIP-Artefakte sowie die 2.0.8-BIN und ihr ZIP wurden mit den vorherigen SHA-256-Werten verglichen und sind unverändert. Für 2.0.11 wurden kein Release, kein Tag und keine Update-Manifeste angelegt.

## Geänderte öffentliche Dateien

Diese 66 öffentlichen Quelldateien unterscheiden sich vom genannten produktiven Basiscommit. Die vollständige Fehlercodeliste steht in [FEHLERCODES.md](FEHLERCODES.md); alle 164 Einträge werden aus dem zentralen Katalog erzeugt. Hinzu kommen die gesondert aufgeführten Testpaket-Artefakte.

- [CHANGELOG.md](../CHANGELOG.md)
- [README.md](../README.md)
- [docs/BEKANNTE_EINSCHRAENKUNGEN_2.0.11.md](BEKANNTE_EINSCHRAENKUNGEN_2.0.11.md)
- [docs/ENTWICKLUNGSBERICHT_2_0_11.md](ENTWICKLUNGSBERICHT_2_0_11.md)
- [docs/FEHLERCODES.md](FEHLERCODES.md)
- [docs/FLASHANLEITUNG_2.0.11.md](FLASHANLEITUNG_2.0.11.md)
- [docs/RELEASE_v2.0.11.md](RELEASE_v2.0.11.md)
- [docs/TESTCHECKLISTE_2.0.11.md](TESTCHECKLISTE_2.0.11.md)
- [include/animation_library.h](../include/animation_library.h)
- [include/chaos_animation.h](../include/chaos_animation.h)
- [include/chaos_errors.h](../include/chaos_errors.h)
- [include/clock_mqtt.h](../include/clock_mqtt.h)
- [include/device_platform.h](../include/device_platform.h)
- [include/diagnostics_api.h](../include/diagnostics_api.h)
- [include/error_catalog.def](../include/error_catalog.def)
- [include/error_response.h](../include/error_response.h)
- [include/error_ui.h](../include/error_ui.h)
- [include/esp32_image.h](../include/esp32_image.h)
- [include/firmware_upload.h](../include/firmware_upload.h)
- [include/firmware_version.h](../include/firmware_version.h)
- [include/journal_storage.h](../include/journal_storage.h)
- [include/mqtt_api.h](../include/mqtt_api.h)
- [include/mqtt_config.h](../include/mqtt_config.h)
- [include/mqtt_errors.h](../include/mqtt_errors.h)
- [include/mqtt_inbox.h](../include/mqtt_inbox.h)
- [include/ota_ui.h](../include/ota_ui.h)
- [include/portal_ui.h](../include/portal_ui.h)
- [include/public_access.h](../include/public_access.h)
- [include/public_ui.h](../include/public_ui.h)
- [lib/ChaosUhrESP32WebServer/src/Parsing.cpp](../lib/ChaosUhrESP32WebServer/src/Parsing.cpp)
- [lib/ChaosUhrWebServer/src/Parsing-impl.h](../lib/ChaosUhrWebServer/src/Parsing-impl.h)
- [scripts/error_browser.js](../scripts/error_browser.js)
- [scripts/generate_error_docs.py](../scripts/generate_error_docs.py)
- [scripts/package_test_2_0_11.py](../scripts/package_test_2_0_11.py)
- [scripts/public_sources.py](../scripts/public_sources.py)
- [src/chaos_errors.cpp](../src/chaos_errors.cpp)
- [src/main.cpp](../src/main.cpp)
- [src/public_access.cpp](../src/public_access.cpp)
- [tests/error_code_contract.json](../tests/error_code_contract.json)
- [tests/memory_animation_fs.h](../tests/memory_animation_fs.h)
- [tests/mqtt_stubs/espMqttClientAsync.h](../tests/mqtt_stubs/espMqttClientAsync.h)
- [tests/public_stubs/Arduino.h](../tests/public_stubs/Arduino.h)
- [tests/public_stubs/ESP8266WiFi.h](../tests/public_stubs/ESP8266WiFi.h)
- [tests/public_stubs/esp_wifi.h](../tests/public_stubs/esp_wifi.h)
- [tests/run_host_tests.py](../tests/run_host_tests.py)
- [tests/run_http_parser_test.py](../tests/run_http_parser_test.py)
- [tests/shared_error_script.js](../tests/shared_error_script.js)
- [tests/test_animation_api.cpp.in](../tests/test_animation_api.cpp.in)
- [tests/test_automatic.cpp.in](../tests/test_automatic.cpp.in)
- [tests/test_backup_ui.js](../tests/test_backup_ui.js)
- [tests/test_clock_mqtt.cpp](../tests/test_clock_mqtt.cpp)
- [tests/test_display_mqtt_ui.js](../tests/test_display_mqtt_ui.js)
- [tests/test_error_ui.js](../tests/test_error_ui.js)
- [tests/test_errors.cpp](../tests/test_errors.cpp)
- [tests/test_esp32_ingress.cpp.in](../tests/test_esp32_ingress.cpp.in)
- [tests/test_github_update_ui.js](../tests/test_github_update_ui.js)
- [tests/test_http_parser.cpp.in](../tests/test_http_parser.cpp.in)
- [tests/test_mqtt_api.cpp](../tests/test_mqtt_api.cpp)
- [tests/test_mqtt_failures.cpp](../tests/test_mqtt_failures.cpp)
- [tests/test_network.cpp.in](../tests/test_network.cpp.in)
- [tests/test_ota_ui.js](../tests/test_ota_ui.js)
- [tests/test_public_access.cpp](../tests/test_public_access.cpp)
- [tests/test_public_ui.js](../tests/test_public_ui.js)
- [tests/test_web_ui.js](../tests/test_web_ui.js)
- [tests/test_wifi_coldstart.cpp.in](../tests/test_wifi_coldstart.cpp.in)
- [tests/validate_firmware_file.cpp](../tests/validate_firmware_file.cpp)

## Noch offene reale Hardwaretests

- ESP32-WROOM: korrigierte Firmware auf freigegebenem Testgerät installieren, WLAN speichern, dreimal vollständig stromlos machen und gespeicherten Zugang nach jedem Kaltstart prüfen.
- Router zeitweise unerreichbar machen, Erhalt der Zugangsdaten, 30-Sekunden-Frist, geschütztes Recovery-Portal und Wiederverbindung prüfen. Dafür wurde keine produktive Netzwerkkonfiguration geändert.
- Wemos: MQTT-Konfiguration mit realem LittleFS speichern, erneut lesen und nach Neustart prüfen; Erfolg der Speicherung getrennt vom Broker-Verbindungsstatus bewerten.
- Reale Brokerfälle für DNS, TCP, Timeout, Ablehnung und Berechtigung überprüfen. Hosttests verwenden simulierten Transport, keinen heimischen Broker.
- GUI und tatsächliche serielle Ausgabe am Gerät, neues Folge-OTA und Last-/Langzeitbetrieb prüfen. Statisches RAM ist keine Messung des freien Heaps im Betrieb.


## Vollständig bereitgestelltes Testpaket 2.0.11

Der Folgeauftrag ergänzt die Verteilung. Firmware-Funktionen und Partitionslayout bleiben beim geprüften Stand; beide Ziele wurden zuvor mit `pio run -e PROFIL -t clean` bereinigt und vollständig neu kompiliert. Die frisch entstandenen BINs haben die oben aktualisierten Prüfsummen. Alle Firmware-Quelldateien stimmen mit Commit `a3d23360ce1f27a05fcf2110788193126aa2dee9` überein; die ergänzten Paketierwerkzeuge und Dokumente werden zusätzlich auf demselben Entwicklungsbranch versioniert.

Fertiger Pfad: `dist/2.0.11/ChaosUhr_2.0.11_TEST.zip`. Neben dem ZIP liegen alle drei BINs einzeln. Das ZIP enthält außerdem versionierte Flash-Anleitung, Testcheckliste, Einschränkungen, Fehlercodeliste, Buildinformationen, Prüfsummen sowie ein separates bereinigtes Quellcode-ZIP mit vollständigen Lizenzen. Die externe `SHA256SUMS_2.0.11.txt` enthält zusätzlich die ZIP-Prüfsumme; die Liste im ZIP prüft dessen übrige Inhalte. Kein Selbsthash einer Datei wird behauptet.

### Dateigrößen und Integrität

| Datei | Byte | SHA-256 |
| --- | ---: | --- |
| `ChaosUhr_2.0.11_Wemos_OTA.bin` | 670368 | `9127c120a623c32bf9106a900f9c5347bc617ffc08d5741f71424e97defc8b4f` |
| `ChaosUhr_2.0.11_ESP32_OTA.bin` | 1936400 | `aae84580b8a3a4de118d502322cf9bce81cdb67aef64e4e0d2fac1392184970a` |
| `ChaosUhr_2.0.11_ESP32_Factory.bin` | 4194304 | `e55d09bca9871e4f4c729d08c489f95ae19163da9e677a127fca586062233275` |

Die ZIP-Prüfsumme steht nach Abschluss der Paketierung in der versionierten externen `dist/2.0.11/SHA256SUMS_2.0.11.txt`. Dateinamen aller direkt zu verteilenden Paketbestandteile enthalten `2.0.11`; historische Dokumente innerhalb des Quellcode-Snapshots behalten ihre ursprünglichen Versionsnamen.

### Tatsächlich verwendete ESP32-Flashkonfiguration

`pio project metadata -e esp32dev_public --json-output-path .pio/metadata-2.0.11-esp32.json` liefert die tatsächlichen Extra-Images und den Anwendungsoffset. Verwendet werden:

| Komponente | Offset | Byte | SHA-256 |
| --- | --- | ---: | --- |
| `bootloader.bin` | `0x1000` | 23520 | `f7ec7b7d9026e12dd3616bf79cee0e78974a2605afbc2863fcfb0ac9aeb159d4` |
| `partitions.bin` | `0x8000` | 3072 | `1ae446228d79cf83a4b83de41c33d1e01dc21a3557149d7da90fbcbfc303311d` |
| `boot_app0.bin` | `0xe000` | 8192 | `f94c5d786a7a8fab06ac5d10e33bf37711a6697636dc037559ea19cc410a17f0` |
| `firmware.bin` | `0x10000` | 1936400 | `aae84580b8a3a4de118d502322cf9bce81cdb67aef64e4e0d2fac1392184970a` |

Die erzeugte binäre Partitionstabelle ist mit `partitions/esp32_4mb_ota.csv` bytegleich rekonstruierbar. Das Partitionstool des installierten Arduino-ESP32-Cores prüft MD5, Eintragsgrenzen und Flashgröße; eine absichtlich veränderte Tabelle wird abgewiesen. NVS `0x9000/0x5000`, OTA-Verwaltung `0xe000/0x2000`, OTA 0 `0x10000/0x1e0000`, OTA 1 `0x1f0000/0x1e0000`, LittleFS (`spiffs`) `0x3d0000/0x30000`. DIO, 40 MHz, 4 MiB entsprechen Boarddefinition und Imageheader.

Das mit esptool 5.4.0 zusammengefügte Factory-Image ist exakt 4.194.304 Byte groß, wird ausschließlich per USB ab `0x0` geschrieben und ist bytegleich zur aus den tatsächlichen Komponenten rekonstruierten Datei. Alle unbenutzten Bereiche sind `0xff`, einschließlich NVS, zweitem OTA-Slot und LittleFS. Bootloader und Anwendung bestehen zusätzlich `image-info` einschließlich Hash. Es werden keine alten Geräte-Flashdumps oder persönliche Konfigurationen verwendet.

### Erneute Prüfungen für die Verteilung

- Clean-Builds Wemos und ESP32: **SUCCESS**, Größen unverändert gegenüber obiger Tabelle. Logs `.pio/build-2.0.11-test-wemos-clean.log` und `.pio/build-2.0.11-test-esp32-clean.log`.
- Vollständige Host-Suite erneut: **52 PASS-Gruppen**, Log `.pio/tests-2.0.11-testpackage.log`.
- Der zusätzliche Lauf aus dem tatsächlich entpackten Quellcode-ZIP wird separat in `.pio/tests-2.0.11-testpackage-extracted.log` protokolliert; die geprüften Archiv- und Quellprüfsummen werden in `.pio/verification-2.0.11-testpackage.json` festgehalten.
- Beide tatsächlichen OTA-Dateien mit dem produktiven C++-Validator und ASan/UBSan geprüft. Manipulierte, gekürzte, falscher Chip, falsche SHA und ausdrücklich auch die Factory-BIN werden für OTA zurückgewiesen.
- Factory-Prüfung: tatsächliche Metadaten, Core-Partitionstool/MD5, CSV, alle Komponenten und vollständig geprüfte Padding-Bereiche. Kein serieller Port geöffnet.
- ZIP-Einträge eindeutig, CRC geprüft, enthaltene Firmware bytegleich zu den einzeln bereitgestellten Dateien, sämtliche internen und externen Prüfsummen nachgerechnet. Öffentliche Quellen, entpackte Paketbestandteile und finale Protokolle auf bekannte private Zugangsdaten beziehungsweise Token-/Schlüsselmuster geprüft; keine privaten Dateien oder privaten Header im Build.
- Nachvollziehbares Werkzeug: `scripts/package_test_2_0_11.py`. Es erwartet die genannten erfolgreichen Clean-Build-Protokolle und das Hosttest-Protokoll, liest PlatformIO-Metadaten aus und erzeugt ausschließlich `dist/2.0.11/`. Es erstellt keine Update-Manifeste und führt weder Uploads noch GitHub-Zugriffe aus. Die tatsächlichen BIN-Prüfungen liegen in `tests/validate_firmware_file.cpp`.

Die weiterhin offenen realen Hardwaretests stehen oben und in `docs/TESTCHECKLISTE_2.0.11.md`. Dieses Testpaket ist keine Bestätigung eines Geräte-, Broker- oder Langzeittests. Produktiver Branch und Updatekanal bleiben unverändert; es wird weder ein Tag noch ein GitHub-Release erzeugt.
