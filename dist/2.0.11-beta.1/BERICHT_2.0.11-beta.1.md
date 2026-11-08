# ChaosUhr 2.0.11-beta.1 – Analyse und Teststand

## Status und Grenze

**MQTT-007 auf dem gemeldeten echten Wemos ist noch nicht abschließend erklärt oder nachweislich behoben.** Die Rückmeldung bestätigt einen gefüllten Discovery-Präfix `homeassistant`. Das mit leerem Präfix reproduzierte Problem darf deshalb nicht als Ursache dieses Geräts ausgegeben werden. Die Beta ist eine Diagnose-/Testversion, kein fertiger MQTT-Bugfix und kein Release. Keine Geräteänderung, Veröffentlichung, produktive Branchänderung oder Kanaländerung vorgenommen.

## Tatsächlich untersuchter Speicherweg

1. `src/main.cpp`: `loadMqtt()` liest die bestehende Konfiguration; `saveMqtt()` erzeugt 24 Felder mit URLSearchParams, liest Session-CSRF und sendet einen URL-encoded POST an `/api/mqtt`.
2. Projektlokaler ESP8266-Parser `lib/ChaosUhrWebServer/src/Parsing-impl.h`: begrenzte Header/Body, Origin-/Session-/CSRF-Prüfung vor Body, URL-Dekodierung und maximal 32 Argumente. Die neue Browserregression führt das tatsächliche JavaScript aus und übergibt dessen vollständigen HTTP-Request an diesen Parser. Jedes der 24 dekodierten Felder wird gegen die Browserwerte geprüft und anschließend an den echten MQTT-Handler übergeben. Im Parser-/MQTT-Test ist Auth gestubbt; die tatsächliche Auth-/CSRF-Kette wird separat durch `test_admin_sessions.cpp` geprüft.
3. `include/mqtt_api.h`: private Kopie der bestehenden Konfiguration; Zahlen-/Text-/Topic-/Hostregeln; Prüfsumme setzen; Gesamtvalidierung. Keine Änderung der aktiven Konfiguration vor erfolgreichem Dateiaustausch.
4. `include/mqtt_config.h`: bestehendes v1-Binärlayout unverändert. Temporäre Datei schreiben, zurücklesen/vergleichen, atomar umbenennen, erst dann aktive Konfiguration ersetzen. Keine automatische Formatierung und keine Änderung von WLAN/EEPROM/Animationslayout.
5. `ClockMqtt::reconfigure()` plant den Verbindungsaufbau. Broker-DNS/TCP/Auth sind spätere, getrennte Vorgänge und können eine Formularvalidierung vor dem Speichern nicht erklären.

### Reproduktion und konkrete Regel

Mit synthetischem DNS-Host **und** IPv4, ohne Zugangsdaten/Sensor-/Wetterdaten, ausgeschalteter Discovery und Präfix `homeassistant` wird die vollständige Anfrage im Hosttest akzeptiert, gespeichert und durch einen neuen Storage-Controller wieder geladen. Ein leerer Präfix wird dagegen auch bei deaktivierter Discovery von der bestehenden Regel `!value.discoveryPrefix[0]` abgelehnt. Vorher-Negativlauf lokal: `.pio/mqtt007-before.log`. **Dieser zweite Fall ist nicht die gemeldete Hardwarekonfiguration.** Die Regel wurde deshalb nicht auf Verdacht gelockert.

### Gezielte Änderung

Die bestehende Gesamtprüfung liefert nun zusätzlich eine feste Feldkennung. Alle bisherigen Regeln bleiben erhalten. `MQTT-022` ersetzt an dieser Stelle das pauschale MQTT-007; möglich sind ausschließlich `format`, `enabled`, `discovery`, `reserved`, `checksum`, `port`, `staleSeconds`, `host`, `user`, `password`, `base`, `discoveryPrefix`, `weatherTopic`, `temperatureTopic`, `sensorTopic`, `sensorLabel`, `sensorUnit`. Selbst `password` ist nur der Feldname, niemals sein Wert. Die Zuordnung gilt vor dem Speichern und in der Storage-Validierung. Die GUI akzeptiert nur diese Kennungen. Unbekannte Inhalte werden nicht übernommen. Andere vorhandene CFG-/FS-/AUTH-Codes bleiben erhalten. Fehlender Host in der GUI wird jetzt spezifisch als CFG-025 angezeigt.

Für die tatsächliche Wemos-Ursache wird der neue Fehlercode samt Feldkennung benötigt. Ohne diese Beobachtung wäre eine Änderung der Validierungsregeln spekulativ. Falls die Beta plötzlich speichern kann, ist auch das zunächst nur eine Hardwarebeobachtung; daraus darf kein bislang unbewiesener Ursachenbefund abgeleitet werden.

## Erhaltene Funktionen

Login, serverseitiger Logout, zwei RAM-Sitzungen, 30 Minuten Inaktivität, kein Verlängern durch GET-Polling, Neustartinvalidierung und Session-CSRF bleiben bestehen. Späte 401-Antworten setzen kein Lösch-Cookie, das eine neuere Anmeldung überschreiben könnte; expliziter Logout löscht das Cookie. Initiale Sitzungsprüfung blockiert den Login-Knopf bis zum Abschluss. Keine Zugangsdaten-/Tokenausgabe in Logs oder Diagnose.

Der zuvor separat reproduzierte Origin-/Host-403 bei Standardport/Großschreibung bleibt durch die bestehenden Regressionen abgedeckt. Die neue Anmeldung allein wird nicht als MQTT-Bugfix gewertet.

Die Sekundenleiste erhält ausschließlich den verlangten GUI-Hinweis. Keine Änderung am Helligkeits-, Nachtmodus- oder Anzeigeverhalten.

## Versionierung und Plattform

Einheitliche Definition in `include/firmware_version.h`: **2.0.11-beta.1**. Firmware, Webseiten-/Scriptversion, Status und Supportdiagnose beziehen die Version daraus. Neue Dateinamen/Testdokumente tragen dieselbe Nummer. Bestehende unnummerierte 2.0.11-Dateien bleiben historische Artefakte; neue Tests verwenden sie nicht.

Nur Wemos/ESP8266 gebaut. Hosttests und Parserprüfungen verwenden standardmäßig nur ESP8266; optionale bisherige ESP32-Hostpfade bleiben ausdrücklich mit `--esp32` erhalten. Keine ESP32-OTA-/Factory-BIN, kein ESP32-Build und keine neue Factory-Prüfung. Keine Aussage über ESP32-Lauffähigkeit dieser Beta.

## Prüfergebnisse

- Vollständige aktive Wemos-Hostsuite: **50 PASS-Gruppen**, ASan/UBSan, einschließlich Login/Logout/erneuter Anmeldung, Ablauf exakt nach 30 Minuten, GET ohne Verlängerung, Neustartinvalidierung, AUTH-001/AUTH-021, gültigem/ungültigem CSRF und MQTT-Save. Keine ESP32-Hostpfade ausgeführt.
- Tatsächliche Browseranfrage: DNS und IPv4, 24 Felder, Speicherung, API-Neuladen und neuer Storage-Controller nach simuliertem Neustart. Leerer Präfix bleibt abgelehnt, bisherige Datei und Animationstestdatei bleiben erhalten. Reale Flash-/Stromneustartprüfung bleibt offen.
- MQTT-Verbindungscontroller separat mit simuliertem Transport: Verbindungsannahme, DNS/TCP/Timeout/Broker-Ablehnung, Recovery. Kein echter Broker-Verbindungsnachweis.
- Alter/neuer Gesamtvalidator bei allen einzelnen Bitmutationen beider Grundkonfigurationen (an/aus), jeweils vor/nach neuer Prüfsumme verglichen: gleiche Annahme-/Ablehnungsentscheidung. Damit keine absichtliche Lockerung der Konfigurationsregeln.
- Fehler-GUI: nur feste Feldnamen, keine unbekannten Feldinhalte oder Secrets; 170 stabile Codes und generierte GUI/Dokumentation synchron.
- Versionsvergleich: beta.1 < beta.2 < beta.10 < rc.1 < Stable; dev.1 < dev.2. Stable 2.0.11, beta.2 und 2.0.12 bieten beta.1 auch mit aktivierten Vorabversionen nicht zur Installation an.
- Frischer Clean-Build **Wemos SUCCESS**, gepinnte bestehende Toolchain. Tatsächliche BIN **684624 Byte**, Limit **1044464 Byte**, OTA-Dateireserve **359840 Byte**. Statisches RAM **50772/81920 Byte**, Linker-Flash **680471/1044464 Byte**; keine Laufzeit-Heapmessung.
- BIN SHA-256: `5f88706a4bab9e75c4ea767523c99554c5a3144d238d306642c39a5d02e7a34f`.
- Produktions-OTA-Dateivalidator, ZIP-CRC/Inhaltsidentität, interne/externe SHA und Geheimnisscan werden durch `scripts/package_wemos_test.py` vor Ausgabe erzwungen. Prüfprotokoll und Quellcommit im Paket: `BUILD_INFO_2.0.11-beta.1.json`.
- Kein ESP32-Build, keine ESP32-BIN-/Factoryprüfung; deshalb keine neue ESP32-Größe oder OTA-Reserve angegeben.
- Das Paket ist ausschließlich für den manuellen Wemos-Test vorgesehen. Produktiver Updatekanal unverändert; GitHub-Updater hat mangels veröffentlichter Beta nichts Neues anzubieten.

## Hardwareprüfungen

Alle Prüfungen am echten Gerät sind noch offen: neue Feldkennung beim problematischen Save, danach Save/Reload/Stromneustart/Brokerverbindung jeweils getrennt, WLAN-Recovery, Login/Logout/30-Minuten-Ablauf, nachfolgendes OTA, Animationserhalt und Heap/Langzeitlast. Kein echter Broker wurde angesprochen. Details stehen in TESTANLEITUNG_2.0.11-beta.1.md.
