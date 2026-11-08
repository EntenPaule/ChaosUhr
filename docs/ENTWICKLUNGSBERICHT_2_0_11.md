> Historischer unnummerierter 2.0.11-Teststand. Aktuell: [2.0.11-beta.1](BERICHT_2.0.11-beta.1.md); dessen Testumfang und ausschließliches Wemos-Paket sind maßgeblich. Kein offizielles 2.0.11-Release.

# ChaosUhr 2.0.11 – Entwicklungsbericht, Sitzungen und Testpaket

Aktueller lokaler Entwicklungsbranch: `fix/2.0.11-diagnostics`, Worktree `.pio/dev-2.0.11/`. Produktive Quellbasis unverändert auf `0f06cd6f5fcb394b8873f154c848b1cbfe1f14e8`, Kanal-Checkout unverändert auf `907d954040752ce5c23c15aa869bae0a7c6e4043`. Kein Release, kein Tag, kein Push, keine Änderung eines produktiven Updatekanals und kein Kontakt zu Uhr, Router oder Broker. Die BINs sind Testartefakte und keine Hardwarefreigabe.

Die lokale Testfassung wurde nach dem Paketcommit `d497648` um Admin-Sitzungen und den unabhängigen MQTT-403-Fix ergänzt. Das vorige unveröffentlichte Paket und seine Prüfnachweise bleiben privat unter `.pio/2.0.11-before-sessions/` sowie im bisherigen Git-Commit erhalten. Quellen und erneuerte Testdateien werden vollständig auf demselben Entwicklungsbranch gesichert; veröffentlichte 2.0.10-/2.0.8-Dateien bleiben unverändert.

## Nachgewiesene Fehlerursachen und Korrekturen

### ESP32-WLAN-Persistenz

`PublicAccess::begin()` schaltete ursprünglich vor dem ersten Funkstart mit `persistent(false)` auf RAM-Speicherung. Im gepinnten Arduino-ESP32 3.3.12 bestimmt diese erste Initialisierung den aktiven SDK-Speicher; ein späteres Umschalten des Persistenzflags durch WiFiManager ändert diesen nicht rückwirkend. Zusätzlich wurde `STA.SSID()` vor der Verbindung als Nachweis fehlender gespeicherter Zugangsdaten benutzt. Der Core liefert dort den verbundenen AP; dadurch konnte die 30-Sekunden-Frist trotz gespeicherter Konfiguration übersprungen werden.

Korrektur: ESP32-Persistenz vor Funkstart einschalten und gespeicherte SDK-Konfiguration statt verbundenem AP prüfen; vorhanden/fehlend/unlesbar unterscheiden. ESP8266 behält seine bisherige Persistenzpolitik. Routerausfälle löschen keine Zugangsdaten, bestehendes Setup/Recovery bleibt erhalten. Bereits verlorene alte RAM-Daten können nicht rekonstruiert werden und erfordern gegebenenfalls einmalige erneute Einrichtung.

Primärquellen: [WiFiGeneric.cpp](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/WiFiGeneric.cpp), [STA.cpp](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/STA.cpp), installierter WiFiManager 2.0.17. Globale Framework-Dateien wurden nicht geändert.

### Leere MQTT-Speichermeldung

Der bisherige Code konnte HTTP 401 ohne Body liefern. Die tatsächliche GUI-Funktion übernahm den leeren Antworttext ungeprüft in ein Error-Objekt, wodurch nur das Präfix sichtbar blieb. Dieser Pfad wurde reproduziert. Strukturierte API-Fehler und zentrale Browser-Fallbacks liefern nun Code, verständlichen Text, Ursache soweit erkennbar und Maßnahme. HTTP-Status bleibt separat; Schreibaktionen werden nicht automatisch wiederholt.

Die konkrete ursprüngliche Geräteantwort wurde nicht aufgezeichnet. Weder falsches Brokerpasswort noch ein ursprünglicher LittleFS-/Rename-Defekt werden als nachgewiesene Ursache behauptet. Speicher- und Verbindungsfehler sind getrennte Zustände. LittleFS-Schritte werden einzeln geprüft; alte Datei und letzte gültige Konfiguration bleiben bei Fehlern erhalten. Keine automatische Formatierung.

### MQTT HTTP 403 – unabhängig von der neuen Anmeldung

Der ursprüngliche Origin-Schutz verglich Host/Origin als vollständige Texte. Dadurch wurde beispielsweise `Host: ChaosUhr.local:80` zusammen mit `Origin: http://chaosuhr.local` abgewiesen, obwohl dies derselbe HTTP-Origin ist. Der tatsächliche MQTT-Speicherhandler wurde **vor dem Sitzungsumbau** mit akzeptierter Anmeldung und CSRF ausgeführt: diese Variante schlug mit 403 fehl. Private Negativkontrolle: `.pio/mqtt-403-before-fix.log`; identischer Nachher-Test: `.pio/mqtt-403-after-fix.log`.

`request_security.h` vergleicht jetzt Hostnamen ohne Beachtung der ASCII-Großschreibung und berücksichtigt den HTTP-Standardport 80. Fremde Hosts/Ports, null-Origins, Userinfo, Pfade und ungültige Authorities bleiben abgewiesen. Fehlender Origin erlaubt keinen Schreibzugriff ohne gültige Anmeldung und CSRF.

Die eigenständige MQTT-API-Regression funktioniert ohne neue Loginlogik. Zusätzlich ist der Fall mit tatsächlichem PublicAccess und MQTT-Handler, Cookie, sitzungsgebundenem CSRF und allen 24 GUI-Feldern für beide Plattformadapter geprüft. Die genaue ursprüngliche Host-/Origin-Kombination des betroffenen Geräts ist ohne dessen HTTP-Mitschnitt weiterhin unbekannt. Eine neue Loginfunktion allein wird nicht als MQTT-Bugfix gewertet.

## Admin-Sitzungen

Vollständige Bedienung, API, Cookie-Eigenschaften und Grenzen: [SITZUNGEN_2_0_11.md](SITZUNGEN_2_0_11.md).

- Sichtbarer Status und expliziter Login/Logout auf Haupt- und OTA-Seite. Bisheriges Admin-Passwort und seine Speicherung bleiben gültig. PublicAccess akzeptiert keine Basic-Header mehr und sendet keine Basic-Challenge. Ein Browsercache kann die serverseitige Abmeldung nicht rückgängig machen.
- Zwei feste RAM-Slots, zusammen höchstens 128 Byte. Je neue 128-Bit-SID und eigenes 128-Bit-CSRF aus Hardware-RNG; nur SID-Fingerprint auf dem Gerät. Kein Übernehmen eines vorgegebenen Identifiers, keine Sitzungsdaten in NVS/EEPROM/Dateisystem.
- Hostgebundener HttpOnly-/SameSite=Strict-Cookie mit Path=/; kein Secure-Flag wegen lokalem HTTP. Ohne TLS bleiben Passwort und Cookie im Netz mitlesbar/manipulierbar; diese Lösung verschlüsselt die Verbindung nicht.
- Server prüft 30 Minuten Inaktivität mit rolloverfester Uptime. Nur gültige geschützte POSTs und ausdrücklich gemeldete Bedienaktivität verlängern; kein GET, Status- oder Tokenabruf. Client-Ablauftimer prüft den Serverstatus ohne Verlängerung und berücksichtigt andere Tabs.
- Logout und Ablauf deaktivieren den Eintrag und löschen sein CSRF. Neustart löscht alle RAM-Sitzungen und erneuert die Login-Challenge. Alte Cookies/Tokens können keine neue Anmeldung erzeugen.
- Origin und zum Cookie gehörendes CSRF werden vor Verarbeitung/Schreiben geprüft; Schutz von Einstellungen, MQTT, Animationen, Reset und OTA bleibt erhalten. Statisches HTML ist eine Login-Shell ohne private Gerätedaten.
- Keine Kennungen, Passwörter oder Tokens in Logs/Supportdiagnose. Auch optionale Projektserver-Debugausgaben geben keine rohen HTTP-/Authentifizierungsinhalte mehr aus. Ein fehlgeschlagener Netzwerk-Logout wird nicht als erfolgreicher serverseitiger Logout angezeigt.

## Zentrale Fehlerverwaltung

Jetzt **169 dauerhafte Codes** aus `include/error_catalog.def`; neue AUTH-020..024 ergänzen die bestehenden IDs. Vollständige Liste: [FEHLERCODES.md](FEHLERCODES.md). Generator und Codevertrag prüfen Synchronität und unveränderte Zuordnung alter IDs.

API/GUI/serielle Diagnose verwenden dieselben festen Texte und Codes, HTTP-Status bleibt separat. Geschütztes `GET /api/diagnostics` bietet Uptime/Heap/Verbindungsflags und maximal acht RAM-Ereignisse mit Wiederholungszählung. Keine Passwörter, SSIDs, Brokeradressen, Benutzer, Topics, Sensorwerte, Cookies oder CSRF-/Loginwerte. Historische Fehler sind kein Nachweis eines noch aktiven Defekts. Kein laufendes Flash-Logging.

## Builds, Speicher und tatsächliche Dateien

Beide Profile wurden mit `-t clean` bereinigt und vollständig frisch gebaut. Bestehende NVS-/EEPROM-/MQTT-Dateiformate und 4-MiB-Partitionierung bleiben unverändert. Tatsächliche BIN-Größe wird zusätzlich zur Linkeranzeige geprüft. Der Fehlerkatalog wird weiterhin einmal out-of-line gespeichert, um duplizierte Flash-Zeichenketten zu vermeiden.

| Profil | Statisches RAM | Linker-Flash | Tatsächliche OTA-BIN | Slot/Maximum | OTA-Dateireserve |
| --- | ---: | ---: | ---: | ---: | ---: |
| `d1_mini_public` | 50684 / 81920 | 679047 | 683200 | 1044464 | 361264 |
| `esp32dev_public` | 67356 / 327680 | 1896663 | 1958784 | 1966080 | 7296 |

Alle Angaben in Byte. **ESP32 hat nur noch 7.296 Byte OTA-Dateireserve**; weitere Erweiterungen benötigen eine erneute Platzprüfung. Statisches RAM ist keine Messung des freien Heaps unter Last. Es wurde keine Partition vergrößert.

Fertiges Paket: `dist/2.0.11/ChaosUhr_2.0.11_TEST.zip`. Drei Firmwaredateien liegen dort auch einzeln:

| Datei | Byte | SHA-256 |
| --- | ---: | --- |
| `ChaosUhr_2.0.11_Wemos_OTA.bin` | 683200 | `267e738d5faf68e1299b0e85689ee508af5bec0ad1779d4bede5e40eab9a4a92` |
| `ChaosUhr_2.0.11_ESP32_OTA.bin` | 1958784 | `b8af0ddaff26c308f13c3fc45d02b5ab96a236b4633e25d49ae932c5bf3a3459` |
| `ChaosUhr_2.0.11_ESP32_Factory.bin` | 4194304 | `714bbeb57a32f2ffe7aeea7333948dcf96414d6bc143f96289f0eb2abd0db98c` |

Das ZIP enthält deutsche Flash-Anleitung, Testcheckliste, Einschränkungen, Sitzungs-/API-Anleitung, Fehlercodeliste, Buildinformationen, Prüfsummen, bereinigten Quellcode-Snapshot und vollständige Lizenzen. Alle direkt zu verteilenden Dateinamen enthalten 2.0.11. Historische Versionsnamen innerhalb der Quellen bleiben korrekt erhalten.

Die externe `dist/2.0.11/SHA256SUMS_2.0.11.txt` enthält zusätzlich die fertige ZIP-Prüfsumme; die Liste im ZIP prüft dessen übrige Inhalte. Kein Selbsthash einer Datei. `BUILD_INFO_2.0.11.json` dokumentiert den Firmware-Quellcommit, Ressourcen, Komponenten, Größen und Hashes.

### Vollständiges ESP32-Factory-Image

**Ausschließlich USB bei 0x0, niemals OTA.** Größe 4.194.304 Byte. Die tatsächlichen PlatformIO-Metadaten liefern Bootloader 0x1000, Partitionstabelle 0x8000, zum Core gehörendes boot_app0 0xe000 und Anwendung 0x10000. DIO, 40 MHz, 4 MiB stimmen mit Boarddefinition und Imageheader überein.

Die erzeugte binäre Tabelle wird mit `partitions/esp32_4mb_ota.csv` und dem Core-Partitionstool einschließlich MD5, Eintragsgrenzen und Flashgröße geprüft. NVS 0x9000/0x5000, OTA-Verwaltung 0xe000/0x2000, OTA 0 0x10000/0x1e0000, OTA 1 0x1f0000/0x1e0000, LittleFS (`spiffs`) 0x3d0000/0x30000. Alle Komponenten bytegleich zu den frischen Dateien; alle unbenutzten Bereiche 0xff, insbesondere NVS, zweiter OTA-Slot und LittleFS. Keine alten Flashdumps oder persönlichen Konfigurationen. USB-Factory-Neuinstallation überschreibt die gesamten 4 MiB.

## Testergebnisse und Nachweise

| Prüfung | Ergebnis / Aussagegrenze |
| --- | --- |
| Frische Wemos-/ESP32-Builds | SUCCESS, tatsächliche BINs passen in unveränderte Grenzen. |
| Vollständige Host-Suite | 55 PASS-Gruppen, C++ mit ASan/UBSan und tatsächliche JavaScript-Funktionen. Kein realer Browser-/Gerätebetrieb. |
| Admin-/MQTT-Integration | Tatsächliche PublicAccess-/MQTT-Handler mit beiden Plattformadaptern: Login/Logout/Relogin, GET-Polling bis genau 30 Minuten, Aktivität, Neustart, altes CSRF/Boot-Challenge, Zwei-Slot-Grenze, ungültige Cookies, millis-Überlauf und vollständige 24-Feld-Speicherung. |
| Eigenständiger MQTT-403-Fix | Vorher fehlgeschlagen, nachher erfolgreich; fremde/malformed Origins bleiben 403. Zusätzlich mit echten Cookie-/CSRF-Handlern geprüft. |
| Geheimnisschutz | Tatsächliche geschützte Diagnose und serieller Fehlerformatter auf beiden Adaptern unter Sanitizern: keine bekannten Passwort-/SID-/CSRF-/Loginwerte. Persönliche Zugangsdaten und Tokenmuster werden zusätzlich für Quellen, BINs, entpackte Dateien und Protokolle gescannt. |
| HTTP-Parser | Beide tatsächlichen Projektparser, Cookie-Sammlung/-Rücksetzung, Vorabprüfung, 32/33+ Argumente, Header/Body-Grenzen, Upload/Abbruch/Timeout. |
| ESP32-WLAN | Tatsächliche Persistenzpolitik mit SDK-Modell und Negativkontrollen: drei simulierte Kaltstarts, Routerausfall, Moduswechsel, NVS-Erhalt und unlesbare Konfiguration. Physische Tests offen. |
| MQTT-/Dateifehler | DNS/TCP/Timeout/Broker/Login/Berechtigung soweit erkennbar getrennt; tatsächlicher Controller mit simuliertem Transport. Mount/Open/Write/Read/Verify/Rename/Cleanup injiziert, alte Datei/Konfiguration erhalten, keine Autoformatierung. |
| OTA-Dateivalidator | Beide tatsächlichen Dateien akzeptiert; Mutation, Kürzung, anderer Chip, Factory-BIN und falsche SHA zurückgewiesen. Offline, kein Flashen. |
| Factory/ZIP | Komponenten-/Padding-/Partitionsprüfung, ZIP-CRC, eindeutige Einträge, Bytegleichheit der Firmware und alle internen/externen SHA-Prüfsummen. Vollständiger Hostlauf aus tatsächlich entpackten Quellen wird zusätzlich protokolliert. |
| Bestehende Regressionen | Einstellungen/Migration ab 2.0.5, Journal/Reset, alle 4.800 Mappingkoordinaten, originale Animationen, Uploads, deutsche Anzeige, MQTT/Wetter/Sensoren, Sicherung, GUI und Updater. |

Private Protokolle: `.pio/build-2.0.11-test-{wemos,esp32}-clean.log`, `.pio/tests-2.0.11-testpackage.log`, `.pio/tests-2.0.11-testpackage-extracted.log`, `.pio/validate-testpackage-2.0.11.log`, `.pio/sessions-secret-tests.log`, `.pio/mqtt-403-{before,after}-fix.log`, `.pio/verification-2.0.11-sessions.json`. Sie werden nicht als öffentliche Quellen gepackt oder committed.

Reproduzierbare Prüfungen: `python3 tests/run_host_tests.py`, `python3 scripts/generate_error_docs.py --check`, Clean-Builds beider öffentlichen PlatformIO-Profile und `python3 scripts/package_test_2_0_11.py`. Das Paketierwerkzeug erwartet erfolgreiche Clean-Build-/Host-Protokolle mit den oben genannten Namen, holt die tatsächlichen Metadaten und verwendet den produktiven nativen OTA-Validator. Es erstellt keine Update-Manifeste und öffnet keine Geräteverbindung.

## Geänderte öffentliche Quellen

75 Dateien unterscheiden sich vom produktiven Basiscommit. Hinzu kommen die versionierten Distributionsdateien unter `dist/2.0.11/`; deren vollständige Liste und Hashes stehen im Testpaket-Verzeichnis.

- [CHANGELOG.md](../CHANGELOG.md)
- [README.md](../README.md)
- [THIRD_PARTY.md](../THIRD_PARTY.md)
- [docs/BEKANNTE_EINSCHRAENKUNGEN_2.0.11.md](BEKANNTE_EINSCHRAENKUNGEN_2.0.11.md)
- [docs/ENTWICKLUNGSBERICHT_2_0_11.md](ENTWICKLUNGSBERICHT_2_0_11.md)
- [docs/FEHLERCODES.md](FEHLERCODES.md)
- [docs/FLASHANLEITUNG_2.0.11.md](FLASHANLEITUNG_2.0.11.md)
- [docs/RELEASE_v2.0.11.md](RELEASE_v2.0.11.md)
- [docs/SITZUNGEN_2_0_11.md](SITZUNGEN_2_0_11.md)
- [docs/TESTCHECKLISTE_2.0.11.md](TESTCHECKLISTE_2.0.11.md)
- [include/admin_sessions.h](../include/admin_sessions.h)
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
- [include/request_security.h](../include/request_security.h)
- [lib/ChaosUhrESP32WebServer/src/Parsing.cpp](../lib/ChaosUhrESP32WebServer/src/Parsing.cpp)
- [lib/ChaosUhrESP32WebServer/src/WebServer.cpp](../lib/ChaosUhrESP32WebServer/src/WebServer.cpp)
- [lib/ChaosUhrWebServer/src/ESP8266WebServer-impl.h](../lib/ChaosUhrWebServer/src/ESP8266WebServer-impl.h)
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
- [tests/public_stubs/ESP8266WebServer.h](../tests/public_stubs/ESP8266WebServer.h)
- [tests/public_stubs/ESP8266WiFi.h](../tests/public_stubs/ESP8266WiFi.h)
- [tests/public_stubs/esp_wifi.h](../tests/public_stubs/esp_wifi.h)
- [tests/run_host_tests.py](../tests/run_host_tests.py)
- [tests/run_http_parser_test.py](../tests/run_http_parser_test.py)
- [tests/shared_error_script.js](../tests/shared_error_script.js)
- [tests/test_admin_sessions.cpp](../tests/test_admin_sessions.cpp)
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
- [tests/test_session_ui.js](../tests/test_session_ui.js)
- [tests/test_web_ui.js](../tests/test_web_ui.js)
- [tests/test_wifi_coldstart.cpp.in](../tests/test_wifi_coldstart.cpp.in)
- [tests/validate_firmware_file.cpp](../tests/validate_firmware_file.cpp)

## Noch offene reale Hardware-/Browsertests

- Haupt-/OTA-Webseite in einem echten Browser: Login, Logout einschließlich altem Basic-Cache, erneuter Login, zwei Sitzungen/mehrere Tabs, 30-Minuten-Ablauf trotz Statuspolling und Verhalten nach Geräteneustart.
- Wemos: MQTT mit gültiger Sitzung speichern/lesen, Neustart und anschließender Login, unveränderte Konfiguration nach ungültiger Sitzung/CSRF. Ursprünglichen realen 403-Fall gegebenenfalls mit bereinigtem HTTP-Status/Code prüfen, ohne Passwörter oder Tokens zu veröffentlichen.
- ESP32: WLAN speichern und drei vollständige Stromtrennungen; zeitweise nicht verfügbares Test-WLAN, 30-Sekunden-Recovery und Wiederverbindung ohne Verlust.
- Reale Brokerfehler und Wetter-/Sensorbetrieb; tatsächliche serielle Ausgabe und GUI-Diagnose; weiteres OTA einschließlich Anmeldung und Erhalt der Einstellungen/Animationen.
- Heap unter WLAN-/MQTT-/GUI-Last sowie Langzeitbetrieb auf beiden Zielgeräten.

Keine dieser physischen Prüfungen wird aus Hostmodellen oder früherem Feedback zu anderen Firmwareständen abgeleitet. Es wurde nichts auf ein Gerät hochgeladen oder als offizieller Release veröffentlicht.
