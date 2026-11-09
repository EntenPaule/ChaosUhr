# ChaosUhr 2.0.11-beta.2 – Wemos-Neustart beim MQTT-Speichern

## Befund und Beweisgrenze

Der Nutzer hat bei Beta.1 einen tatsächlichen Neustart mit eingefrorener Matrix, Startanzeige und IP-Anzeige beobachtet. Danach ist AUTH-021 erwartbar: RAM-Sitzungen werden beim Boot verworfen. Das ist kein Grund, Authentifizierung oder CSRF abzuschalten. Eine serielle Reset-/Exception-Aufzeichnung liegt bisher nicht vor.

**Nachgewiesen ist ein erheblicher Stackverbrauch im Speicherpfad. Der genaue Hardware-Resetgrund ist noch nicht nachgewiesen.** Ob Watchdog, Exception/Stacküberlauf oder Stromversorgung den beobachteten Reset auslösen, muss Beta.2 am Gerät unterscheiden. Der frühere MQTT-007 ist ein gesonderter, weiterhin nicht abschließend erklärter Befund.

## Stackanalyse und gezielte Korrektur

Aus den tatsächlichen Xtensa-Objektprologen der ausgelieferten Beta.1:

| Funktion | Beta.1 Frame | Beta.2 Frame |
|---|---:|---:|
| mqttSaveApi | 2272 Byte | 96 Byte |
| MqttConfigStorage::save | 1232 Byte | 272 Byte |
| Zusammen, ohne aufgerufene Unterfunktionen | **3504 Byte** | **368 Byte** |

Beta.1: API-Prolog reserviert 32 + 192 + 2048 Byte; Storage-Prolog 32 + 1200 Byte. Der konfigurierte ESP8266-Sketch-Stack ist 4096 Byte groß. Es bleiben rechnerisch nur 592 Byte für sämtliche bereits aktiven HTTP-/Loop-Frames und tieferen Funktionen. Das beweist den ungünstigen Speicherbedarf, aber ohne Laufzeittrace noch keinen konkreten Overflow auf der Nutzeruhr.

Beta.2 wird mit `-fstack-usage` gebaut. `scripts/check_mqtt_stack.py` prüft die tatsächlichen Xtensa-Compilerberichte und begrenzt die beiden Frames auf 256 bzw. 512 Byte. Das ist keine Garantie für die vollständige dynamische Stacktiefe.

Gezielte Änderungen:

- Die 1088-Byte-Konfigurationskopie des Handlers wird mit `new(std::nothrow)` auf dem Heap angelegt. Allokationsfehler werden als HTTP 503 / SYS-002 beantwortet; keine Speicherung oder Neustartaktion. RAII gibt die Kopie auf allen Rückwegen frei und überschreibt vorher ihren Inhalt.
- `MqttConfigStorage::save` übernimmt den veränderbaren Kandidaten per Referenz, statt eine zweite vollständige Stackkopie anzulegen. Nur der Kandidat wird versiegelt; die aktive Konfiguration wird erst nach erfolgreichem atomarem Dateiaustausch ersetzt.
- Die Dateirückprüfung verwendet einen 128-Byte-Puffer statt einer dritten vollständigen Konfigurationskopie. Länge, jedes Byte und kurze/fehlgeschlagene Lesezugriffe werden weiterhin geprüft.
- Binärlayout v1 (1088 Byte), Prüfsumme, Validierungsregeln, bestehende MQTT-Daten, Einstellungen und Animationen bleiben kompatibel. Kein Formatieren, kein Reset und keine Layoutmigration. Fehler beim Schreiben/Prüfen/Umbenennen erhalten den bisherigen aktiven Datensatz.

## Neustart- und Speicherschrittdiagnose

Geschützt unter `/api/diagnostics`, im GUI über „Supportdiagnose laden“, erscheint `reset`:

- SDK-Resetgrund als Zahl und feste Bezeichnung: Hardware-/Software-Watchdog, Exception, Software-Neustart, Deep-Sleep, externer Reset oder Power-on/unclassified.
- Wenn verfügbar: Exception-Nummer und EPC1-Codeadresse. Keine Rohspeicherinhalte.
- Ein kleiner Crash-Callback merkt auch den Core-Grund (z. B. 253 = Stack-Guard) vor dem Neustart. Das ist getrennt vom SDK-Resetgrund ausgewiesen. Der Callback kann bei schweren Abstürzen ausbleiben.
- `previousMqtt`: letzter gültiger Kontrollpunkt des vorherigen Boots; `currentMqtt`: letzter Kontrollpunkt dieses Boots. Phasen: Entered, Authenticated, Candidate, Validated, Mount, Open, Write, Flush, Verify, Rename, Committed, Reconfigure, Response, Complete oder Rejected.
- Numerische Uptime, freier Heap, größter freier Heapblock (falls Core verfügbar) und `stackHighWaterFreeBytes`. Letzteres ist der kleinste unbenutzte Stackbereich seit Boot laut Core, **nicht** der gerade verfügbare Stack oder eine exakte Messung nur dieses Speicherversuchs.

Nur feste Bezeichnungen und Zahlen werden gespeichert. RTC-RAM statt Flash-Log: je 32 Byte bei User-RTC Byte 128..159 und 160..191. Die ersten 128 User-RTC-Bytes des eboot-OTA-Kommandos sowie der SDK-Resetbereich bleiben unangetastet. Version/Magic/Prüfsumme prüfen die Aufzeichnungen. Der Crashmarker wird beim Boot geleert; die vorherige MQTT-Aufzeichnung bleibt für diesen Boot als RAM-Snapshot lesbar. Bei Power-on-Grund, unlesbarem oder ungültigem RTC wird keine verlässliche Vorgängeraufzeichnung behauptet.

Ein Kontrollpunkt ist die zuletzt erreichte Phase, kein Beweis, dass diese Phase den späteren Reset verursacht hat. Complete bedeutet beispielsweise nur, dass ein vorangegangener Save abgeschlossen war. Ein Spannungsabfall lässt sich aus einem generischen Resetgrund **nicht sicher** ableiten; dafür sind Stromversorgung/Kabel/Messung am Gerät erforderlich.

## Neustartpfade, Parser und LittleFS

Die expliziten Anwendungs-Neustarts liegen in `PublicAccess::loop`: geplante Neustarts nach OTA, LED-Pinwechsel bzw. WLAN-/Portalaktion und der bestätigte Werksreset. Der MQTT-Handler plant keinen Neustart. In dieser Beta wurden keine zusätzlichen Software-Neustarts eingeführt. SDK-/Watchdog-/Exception-Neustarts können ohne `ESP.restart()` auftreten.

Der HTTP-Parser ist unverändert und bleibt durch echte Parser- und Browser-24-Feld-Tests abgedeckt. Auth, Origin, CSRF, Argument-/Bodylimits bleiben aktiv. LittleFS-Schreibfehler werden weiterhin differenziert behandelt; der neue Kontrollpunkt zeigt, ob Mount/Open/Write/Flush/Verify/Rename erreicht wurde. Fehler werden nicht durch automatische Formatierung behandelt. Der Watchdog bleibt unverändert aktiv; es gibt keine neue WDT-Fütterung, Zeitverlängerung oder Schutzabschaltung.

## MQTT-007 gesondert betrachtet

Beta.1 reproduzierte lokal die Ablehnung eines leeren Discovery-Präfixes. Der Nutzer bestätigte jedoch `homeassistant`; dieser Testfall erklärt seine ursprüngliche Meldung nicht. Die vollständige synthetische Browseranfrage mit gefülltem Präfix, DNS/IP und Minimalfeldern besteht weiter. Ob Stackkorruption den früheren Fehler verursacht hat, ist bislang nur eine Hypothese. Die spezifische MQTT-022-Feldkennung bleibt erhalten, um Validierungsfehler von Reset-/Speicherfehlern zu unterscheiden.

## Serielle Exception-Auswertung ohne Weitergabe von Secrets

Beta.2 gibt nach dem Boot eine feste `[RESET]`-Zeile bei 115200 Baud aus. Bei Exceptions bleibt die Core-Stackausgabe aktiv. **Ein vollständiger Core-Stackdump kann Zugangsdaten oder andere Speicherinhalte enthalten; er gehört weder in Discord noch in Git oder eine öffentliche Supportdiagnose.**

`scripts/decode_reset_trace.py` liest einen ausschließlich lokal gespeicherten Rohdump und gibt nur zum angegebenen ELF passende ausführbare Adresskandidaten mit Funktions-/Zeilenauflösung aus. Es ist kein verifizierter Stack-Unwinder; besonders bei zerstörtem Stack können Adressen fehlen oder zufällig passen. Das genaue Beta.2-ELF wird lokal unter `.pio/debug/2.0.11-beta.2/firmware.elf` aufbewahrt, dessen SHA im BUILD_INFO dokumentiert. Kein ungefragter Zugriff auf den seriellen Port.

## Prüfung und bestätigte Hardwareergebnisse (Nutzerrückmeldung 09.11.2026)

Vorhandene Wemos-Hostsuite: **51 PASS-Gruppen**; vorhandener Clean-Build SUCCESS. Tatsächliche BIN 686800 Byte, OTA-Dateireserve 357664 Byte; statisches RAM 51544/81920 Byte. BIN SHA-256 `4b7e62453a267d1f88deaa9995c083d138a38a691e9b7fc3fff78ca4b2e23814`. Matching BIN/ELF lokal unter `.pio/debug/2.0.11-beta.2/` gesichert. Die bisherigen Untersuchungen wurden für diese Dokumentation nicht wiederholt.

Am echten Wemos D1 mini vom Nutzer bestätigt:

- MQTT-Konfiguration speicherbar, vorheriger Absturz beim Speichern tritt nicht mehr auf; Diagnose `phase: Complete`, rund 1,3 KiB freie Stackreserve.
- Erfolgreiche Verbindung zu ioBroker über IPv4 und DNS-Namen.
- 30 Sekunden vollständige Stromtrennung: WLAN und MQTT bleiben gespeichert; automatische MQTT-Verbindung nach Kaltstart.
- Start mit angeschlossener LED-Matrix erfolgreich.
- Wetterdaten von ioBroker werden angezeigt; Wetteranimationen funktionieren.

Der frühere Stacküberlauf ist damit eine **sehr wahrscheinliche Ursache**, ohne damaligen Crashtrace weiterhin nicht endgültig bewiesen. Einmaliges MQTT-011 dokumentiert; ohne Wiederholung oder konkreten Fehlerhinweis keine zusätzliche Untersuchung. AUTH-021 nach Neustarts ist erwartetes Verhalten und kein belegter neuer Defekt. Der frühere MQTT-007 ist weiterhin nicht als identische Ursache bewiesen.

Offen bleiben Langzeit-/Heapverhalten, erneute Auffälligkeiten bei MQTT-011, weitere GUI-/Login-/Auto-Logout-/OTA-Hardwaretests und zukünftige Beta.3-Anzeigevorschau. Keine ESP32-Firmware gebaut oder geprüft. Kein Release, kein produktiver Branch-/Kanalwechsel; alte Pakete unverändert.

## Primärquellen

Geprüfter lokaler Core 3.1.2: `cont.h` (4096-Byte-Stack), `cont_util.cpp`/`Esp.cpp` (Stack-Watermark, RTC-Offsets), `eboot_command.h` (128-Byte-OTA-Kommando), `core_esp8266_postmortem.cpp` (Crashcallback/Stackdump). Ergänzend: [ESP8266-Core: Abstürze und kleiner Stack](https://arduino-esp8266.readthedocs.io/en/stable/faq/a02-my-esp-crashes.html). Globale Core-Dateien wurden nicht verändert.
