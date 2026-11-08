# ChaosUhr 2.0

**Lokaler Entwicklungsstand 2.0.11:** Korrekturen für ESP32-WLAN-Persistenz, verständliche MQTT-/HTTP-Fehler und ein gemeinsames Fehlercodesystem. Dieser Stand ist **nicht veröffentlicht**; produktiver Branch und Updatekanäle bleiben auf 2.0.10. [Änderungen und Tests](docs/ENTWICKLUNGSBERICHT_2_0_11.md), [Fehlercodes](docs/FEHLERCODES.md), [Versionsinformationen](docs/RELEASE_v2.0.11.md).

Das lokale Testpaket liegt unter `dist/2.0.11/ChaosUhr_2.0.11_TEST.zip`: Wemos-OTA, ESP32-OTA und vollständiges ESP32-USB-Factory-Image, Prüfsummen, bereinigter Quellcode und Lizenztexte. [Flash-Anleitung](docs/FLASHANLEITUNG_2.0.11.md), [Testcheckliste](docs/TESTCHECKLISTE_2.0.11.md), [bekannte Einschränkungen](docs/BEKANNTE_EINSCHRAENKUNGEN_2.0.11.md). Die ESP32-Factory-BIN ist ausschließlich für USB bestimmt.

**Aktuelle stabile Version: [2.0.10 für Wemos / ESP8266 und klassischen ESP32](https://github.com/EntenPaule/ChaosUhr/releases/tag/v2.0.10).** MQTT-Wetter und Sensoren, wählbare Anzeigearten, konfigurierbarer LED-Pin, Sekundenleisten-Schalter und Wochentag-Auswahl. Beide Builds und die ZIP-Quellen sind lokal geprüft; die neuen BINs sind **noch nicht am Gerät getestet**. [Teststand](docs/RELEASE_v2.0.10.md), [MQTT-Anleitung](docs/MQTT_2_0_10.md), [Upgrade](docs/UPGRADE_2_0_10.md).

Eine bunte LED-Uhr für die **20×15-WS2812B-Matrix**: Uhrzeit, deutscher Wochentag und Datum, Animationen sowie Wetter- und Sensorwerte aus Home Assistant oder ioBroker. Über die Weboberfläche lassen sich Anzeige, Automatik und eigene Animationen einstellen – direkt im lokalen WLAN. Auch ausschließlich Uhr/Wetter ist möglich.

Dieser Fork entwickelt die [ChaosUhr von Micha1986 / oOmicha1986Oo](https://github.com/oOmicha1986Oo/ChaosUhr) als PlatformIO-Firmware weiter. Die ursprünglichen Bilder und Animationen sind mit Herkunfts- und Lizenznachweisen übernommen.

Der vorherige [Wemos-Release 2.0.8](https://github.com/EntenPaule/ChaosUhr/releases/tag/v2.0.8) bleibt unverändert verfügbar. Dessen bestätigte Anzeige-, Web- und OTA-Tests gelten für 2.0.8; sie ersetzen keine Hardwareprüfung von 2.0.10.

[Downloads](#downloads) · [Funktionen](#funktionen) · [Hardware](#hardware) · [Einrichtung](#erste-einrichtung) · [MQTT und Wetter](#mqtt-wetter-und-sensoren) · [Eigene Animationen](#eigene-animationen-erstellen-und-teilen) · [Selbst bauen](#selbst-bauen)

## Downloads

Die Dateien stehen im [GitHub-Release v2.0.10](https://github.com/EntenPaule/ChaosUhr/releases/tag/v2.0.10). **Die Datei muss zum Controller passen.**

| Datei | Inhalt / Verwendung |
| --- | --- |
| [chaosuhr-2.0.10-esp8266.bin](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.10/chaosuhr-2.0.10-esp8266.bin) | Wemos D1 mini / ESP8266: Anwendungs-BIN für den Web-Updater. |
| [chaosuhr-2.0.10-esp8266.zip](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.10/chaosuhr-2.0.10-esp8266.zip) | Dieselbe Wemos-BIN, Anleitungen, vollständige passende Quellen, Tests, Beispiele und Lizenzen. |
| [chaosuhr-2.0.10-esp32-ota.bin](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.10/chaosuhr-2.0.10-esp32-ota.bin) | Klassischer ESP32-WROOM-32 / esp32dev: Anwendungs-BIN für den Web-Updater. |
| [chaosuhr-2.0.10-esp32-factory.bin](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.10/chaosuhr-2.0.10-esp32-factory.bin) | Klassischer ESP32: vollständiges USB-Erstinstallationsimage ab 0x0; überschreibt vorhandene Daten. **Nicht für OTA.** |
| [chaosuhr-2.0.10-esp32.zip](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.10/chaosuhr-2.0.10-esp32.zip) | Beide ESP32-BINs, Anleitungen, vollständige passende Quellen, Tests, Beispiele und Lizenzen. |
| [SHA256SUMS](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.10/SHA256SUMS) | Gemeinsame Prüfsummen für beide Controller-Pakete und Release-Metadaten. |
| [BUILD_INFO.json](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.10/BUILD_INFO.json) | Zielhardware, Größen, Layout, Prüfsummen und Teststand beider Builds. |

**Zum Aktualisieren die passende Anwendungs-BIN verwenden.** Das ZIP wird vorher entpackt. Die öffentlichen BINs enthalten keine persönlichen WLAN-, Admin-, Einrichtungs- oder Broker-Passwörter; diese werden auf der Uhr eingerichtet. Ein vollständiger Flash-Dump eines bereits eingerichteten Geräts enthält dagegen persönliche Daten.

## Funktionen

- **Uhrzeit und Datum:** NTP-Zeit, deutsche Sommer-/Winterzeit, Datumslauftext und Wochentag als `Montag` oder `mo`. Sekundenleiste ein-/ausschaltbar.
- **30 eingebaute Effekte:** unter anderem Matrix-Regen, Regenbogenwellen, Feuer, Sternenhimmel, Plasma, Polarlicht und die übernommenen Klassiker wie DigiDug, Q*bert, Gesichter, Drink und BombJack.
- **Drei Anzeigemodi:** nur Uhrzeit, eine gewählte Animation oder Automatik mit frei gewählten Kategorien Uhr, Animationen, Wetter und Sensorwerte.
- **Eigene Automatik-Auswahl:** gewünschte eingebaute und hochgeladene Animationen ankreuzen; zufällige vollständige Runden oder Wiedergabe der Reihe nach.
- **Eigene Animationen:** bis zu acht `.chaos`-Dateien im Browser hochladen, abspielen, herunterladen und löschen. Eingebaute Effekte lassen sich aus der Automatik nehmen.
- **Weboberfläche:** Helligkeit, Animationstempo, Szenendauer und Matrixausrichtung einstellen; Nachtzeitraum mit eigener Helligkeit, auch über Mitternacht.
- **MQTT-Wetter und Sensoren:** Sonne, Wolken, Regen, Temperatur und weitere Wetterzustände sowie vier frei beschriftete Sensorplätze. Topics und Broker für Home Assistant/ioBroker in der GUI konfigurieren.
- **Pin und Erreichbarkeit:** LED-Datenpin im Dropdown wählen; gespeicherte Änderung nach bestätigtem Neustart. Eigene IP nach erfolgreicher WLAN-Verbindung als Lauftext.
- **Einstellungen sichern:** JSON-Export und Import mit Vorschau; eigene Animationsdateien separat sichern und teilen.
- **Updates über WLAN:** manueller BIN-Upload sowie bestätigte Updateinstallation aus dem GitHub-Kanal ab 2.0.8.
- **Einrichtung und Verwaltung:** selbst gewähltes Admin-Passwort, geschütztes Einrichtungs-WLAN und bestätigter Werksreset einschließlich WLAN-Daten und eigener Uploads.
- **Diagnose und HTTP-API:** Firmwareversion, Zeitstatus, WLAN-Signal, Heap, Flash-/OTA-Platz, Effektkatalog und Einstellungen auslesen.

Wetter- und Sensorwerte empfängt die Uhr per MQTT aus Home Assistant, ioBroker oder einem anderen Publisher. Die Topics sind konfigurierbar; bestehende Automationen werden nicht automatisch umgeschrieben.

### Neu in 2.0.10

- Uhr, Animationen, Wetter und Sensorwerte unabhängig auswählen, auch nur Uhr/Wetter.
- MQTT-Wetter mit Sonne/Wolken/Regen und Temperatur; vier Sensorplätze mit eigenen Topics, Namen und Einheiten.
- LED-Datenpin in der GUI wählen und dauerhaft speichern; Änderung nach Bestätigung mit Neustart.
- Sekundenleiste ein-/ausschalten; Wochentag ausgeschrieben mit echten Kleinbuchstaben oder als `mo`, `di`, … `so`.
- Eigene IP-Adresse nach erfolgreicher WLAN-Verbindung auf der Matrix.
- Parallele Builds und getrennte Pakete für Wemos und klassischen ESP32.

Die neue Firmware ist noch nicht auf Hardware getestet. [Anleitung und Teststand](docs/RELEASE_v2.0.10.md).

### Seit Version 2.0.8 enthalten

- Gehärteter HTTP-Parser, zentrale Anmeldung und CSRF-Schutz auch für Einstellungen und Firmware-Updates.
- Gesalzene Speicherung des Admin-Passworts als Verifier, Migration des bisherigen Passworts und internes Wiederherstellungsjournal.
- JSON-Einstellungssicherung mit geprüfter Import-Vorschau. Eigene Animationen werden anhand ihres Dateifingerabdrucks zugeordnet.
- GitHub-Updateprüfung und HTTPS-Download im Browser; Installation erst nach Bestätigung, zusätzliche SHA-256-Prüfung auf der Uhr.
- Strengere Prüfung hochgeladener Firmware auf Zielhardware, Größe, CRC und Produktkennung.

Die Automatik-Auswahl und eigenen `.chaos`-Animationen aus 2.0.7 bleiben erhalten. Einzelheiten stehen im [Änderungsverlauf](CHANGELOG.md) und [Releasebericht 2.0.8](docs/RELEASE_v2.0.8.md).

## Hardware

| Eigenschaft | Wemos / ESP8266 | Klassischer ESP32 |
| --- | --- | --- |
| Controller | Wemos D1 mini | ESP32-WROOM-32 / esp32dev |
| Flash | 4 MiB; ungefähr 1 MiB LittleFS | 4 MiB; zwei OTA-Slots mit je 1.966.080 Byte, 192 KiB LittleFS |
| LED-Matrix | 300 WS2812B, 20×15 | 300 WS2812B, 20×15 |
| Standard-Datenpin | **D6 / GPIO12** | **GPIO32** |
| Wählbare GPIOs | 4, 5, 12, 13, 14 | 13, 14, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33 |
| Ausrichtung ab Werk | Oben rechts, Zeilen, Zickzack | Oben rechts, Zeilen, Zickzack |
| Software-Leistungslimit | Geschätzt 5 V / 1,5 A | Geschätzt 5 V / 1,5 A |

Die Software-Leistungsbegrenzung ersetzt keine passend ausgelegte Stromversorgung und Verdrahtung. Ab 2.0.10 wird der LED-Datenpin im Dropdown gewählt und gespeichert; die Pin-Auswahl gilt nach einem bestätigten Neustart. Der bisherige Wemos-Standard D6/GPIO12 bleibt bei der Migration erhalten.

[ESP32-Paket und Anleitung](docs/ESP32_FIRMWARE.md). Die neue ESP32-Firmware ist noch nicht am Gerät getestet; das Nutzerfeedback zur älteren 2.0.6 auf 2.0.5-Basis bestätigt nur diesen früheren Stand. Beide neuen Builds sind für die oben genannten Controller ausgelegt.

## Erste Einrichtung

Für eine neue Uhr oder nach einem Werksreset:

1. Die zur Hardware passende Firmware installieren. Für eine bereits laufende Uhr den [Upgrade-Ablauf](docs/UPGRADE_2_0_10.md) verwenden.
2. Die Matrix zeigt **SETUP** und einen zwölfstelligen Zahlencode.
3. Mit dem WLAN **`ChaosUhr-Setup-XXXXXX`** verbinden. Sein WLAN-Passwort ist der angezeigte Code.
4. **http://192.168.4.1/** öffnen und ein eigenes Admin-Passwort mit **16–64 druckbaren ASCII-Zeichen** setzen und wiederholen. Leer- und Sonderzeichen sind möglich, Umlaute nicht.
5. Nach dem Neustart vorhandene Heim-WLAN-Daten weiterverwenden lassen oder das Heim-WLAN im Einrichtungsportal hinterlegen.
6. Nach WLAN-Verbindung erscheint die eigene IP als Lauftext. Im Heimnetz diese Adresse oder **http://chaosuhr.local/** öffnen. Benutzername: **`admin`**, Passwort: das selbst gesetzte Admin-Passwort.

Der Zahlencode für das Einrichtungs-WLAN und das Admin-Passwort sind getrennt. Nach dem Verbinden synchronisiert die Uhr ihre Zeit über NTP. Eine ausführliche Anleitung steht in [Einrichtung und Bedienung](docs/PUBLIC_FIRMWARE.md).

## Vorhandene Uhr aktualisieren

**Von 2.0.8:** Zuerst Einstellungen als JSON und eigene `.chaos`-Dateien sichern. Im OTA-Menü **Auf Updates prüfen** und anschließend 2.0.10 bewusst installieren. Das Häkchen für Vorabversionen ist dafür nicht erforderlich. Alternativ die passende Anwendungs-BIN manuell hochladen. Der bisherige Wemos-Pin D6/GPIO12 bleibt Standard; Wetter und Sensoren sind nach der Migration zunächst ausgeschaltet.

**Von 2.0.5 oder 2.0.7:** Einstellungen notieren und eigene `.chaos`-Dateien herunterladen. Die neue BIN über den bisherigen geschützten Web-Updater installieren. Gültige Uhreinstellungen und der vorhandene Admin-Zugang werden übernommen; das Admin-Passwort bleibt gleich.

Bei bereits lesbarem LittleFS wird das alte Passwortformat migriert. Ohne lesbaren Speicher bleibt die bisherige Anmeldung zunächst aktiv, und die GUI meldet die ausstehende Migration. Eventuell vorhandene Dateien sichern, bevor der Speicher ausdrücklich eingerichtet wird; es gibt kein automatisches Formatieren.

**Von Michas Originalversion:** Die neue BIN wird über das vorhandene Uploadformular übertragen; anschließend ist erstmals die Admin-Ersteinrichtung nötig. Im alten Sketch fest eingestellte Anzeigeparameter lassen sich nicht automatisch übernehmen. Geprüfte Referenz ist Sketch 0.18; angepasste Fremdversionen können abweichen.

**Nach der Passwortmigration sind unveränderte 2.0.5/2.0.7 kein geeigneter Downgrade.** Auch 2.0.8 kennt die neue Pin-Auswahl nicht. Vor dem Update die [Upgrade-Anleitung für 2.0.10](docs/UPGRADE_2_0_10.md) lesen. Direktupdates von 2.0.5 und Michas Original wurden noch nicht zusätzlich auf echter Hardware getestet.

### Weitere Updates aus der GUI

Unter **OTA-Upload öffnen** bleibt der manuelle Upload einer passenden BIN verfügbar. Ab 2.0.8 kann **Auf Updates prüfen** zusätzlich den GitHub-Kanal abfragen.

2.0.10 wird im stabilen Kanal angeboten; **Vorabversionen berücksichtigen** ist dafür nicht erforderlich. Der Browser braucht Internetzugang; das lokale Admin-Passwort wird nicht an GitHub gesendet. Nur eine neuere passende Version wird zur bestätigten Installation angeboten – keine unbeaufsichtigten Updates und keine erneute Installation derselben Version. ESP32 verwendet einen eigenen Kanal mit passendem Zielchip. [So funktioniert der Updatekanal](docs/GITHUB_UPDATES.md).

## Automatik einstellen

Die gewünschten Animationen auswählen und zwischen zwei Abläufen wählen:

- **Zufällig:** Jede gewählte Animation läuft einmal pro Runde, danach beginnt eine neue zufällige Runde.
- **Der Reihe nach:** Die Auswahl läuft in der Reihenfolge des Katalogs und beginnt anschließend wieder von vorn.

Bei Auswahl von Uhr und Animationen gilt weiterhin: **Uhr → Animation → Uhr → nächste Animation → Uhr …** Ab 2.0.10 ergänzen die gewählten Wetter-/Sensorszenen den Ablauf. Für nur Uhr/Wetter die Anzeigeart Animationen ausschalten.

Die Szenendauer ist von 5 bis 120 Sekunden einstellbar. **Nur Uhrzeit** und **Gewählte Animation** bleiben unabhängig von der Automatik nutzbar. Wenn die Kategorie Animationen aktiv ist, mindestens eine verfügbare Animation auswählen. Für Uhr/Wetter darf die Animationskategorie vollständig aus sein. [Details zur Animationsauswahl](docs/AUTOMATIK_2_0_7.md).

## MQTT, Wetter und Sensoren

MQTT ist ab Werk ausgeschaltet. In der GUI Broker, Zugang und eigene Daten-Topics eintragen; anschließend Wetter und/oder Sensorwerte in der Automatik einschalten. Für nur Uhr/Wetter die Kategorien Uhrzeit und Wetter wählen, Animationen und Sensorwerte abwählen.

Beispiel für den Wetter-Topic:

```json
{"condition":"partlycloudy","temperature":17.2}
```

Alternativ Wetterzustand und Temperatur in getrennten Topics senden. Vier Sensorplätze akzeptieren Zahlen, kurze Texte oder JSON mit `value`, `val` oder `state`; Name und Einheit sind frei wählbar. So können Raumtemperaturen, Luftfeuchtigkeit, CO2, Leistung oder Fensterzustände erscheinen. Fehlende/veraltete Daten werden übersprungen.

Home Assistant oder ioBroker beschaffen die Daten und veröffentlichen sie am Broker; die Uhr benötigt keinen eigenen Wetteranbieter-/HTTPS-Abruf. Home-Assistant-Discovery bietet optional Firmware und WLAN-Signal als Diagnose-Entitäten. Diese Version empfängt Daten und veröffentlicht Diagnose; Einstellungen werden nicht per MQTT ferngesteuert.

[Einrichtung, unterstützte Wetterzustände, Payloads und Datenalter](docs/MQTT_2_0_10.md). MQTT verwendet lokales TCP ohne TLS. Ein gespeichertes Broker-Passwort wird nicht in API oder Einstellungssicherung zurückgegeben.

## Eigene Animationen erstellen und teilen

Eigene Motive werden als einfache **`.chaos`-Textdateien** hochgeladen. Sie enthalten Farbpalette, Pixelbilder und Bildzeiten; für ein neues Motiv ist kein Firmware-Update nötig.

- **Acht Upload-Plätze**, höchstens **24 KiB je Datei**.
- **20×15 Pixel**, bis zu **16 Farben** und **64 Bilder**.
- Upload, Download, Auswahl und Löschen über die Weboberfläche.
- Eigene und eingebaute Animationen können gemeinsam in der Automatik laufen.

Uploads halten 12 KiB Dateisystemreserve für Konfiguration und Journal frei. Beim ESP32 begrenzen 192 KiB LittleFS den Gesamtumfang zusätzlich; acht maximal große Dateien passen dort nicht gleichzeitig.

Zum Ausprobieren gibt es [Herz.chaos](examples/Herz.chaos). Mit der [ChatGPT-Vorlage](docs/CHATGPT_ANIMATION_PROMPT.txt) kann man weitere Motive erstellen lassen und die fertigen Dateien beispielsweise im Discord teilen. Die vollständigen Regeln stehen in der [Formatbeschreibung](docs/ANIMATIONEN_FORMAT.md).

Unter **Eigene Animationen** einen noch nicht eingerichteten Speicher bei Bedarf ausdrücklich initialisieren. Diese Aktion löscht vorhandene Dateien in diesem Speicher. Neue Uploads anschließend in der Automatik ankreuzen oder als Einzelanimation auswählen und die Einstellungen speichern.

## Einstellungen sichern und zurücksetzen

**Einstellungen sichern** lädt ab 2.0.8 eine JSON-Datei herunter. Schema 2 ab 2.0.10 enthält zusätzlich Anzeigeoptionen und liest weiterhin alte Sicherungen. Netzwerk-/Admin-/MQTT-Zugangsdaten und der physische LED-Pin werden nicht exportiert. Eigene Animationen zusätzlich als `.chaos` herunterladen.

Beim Wiederherstellen zuerst die Vorschau prüfen und übernehmen. Das füllt die Formulare; erst **Einstellungen speichern** ändert die Uhr. Benötigte Animationsdateien müssen bereits hochgeladen sein. Ihre Zuordnung erfolgt über SHA-256, auch wenn sie auf anderen Upload-Plätzen liegen.

Ein bestätigter **Werksreset** löscht Uhreinstellungen, Admin-Zugang, Einrichtungs-Code, Heim-WLAN-Daten und eigene Animationen. Danach beginnt die Ersteinrichtung erneut.

## Teststand und Sicherheitsgrenzen

**2.0.10:** beide Builds erfolgreich, vollständige Host-/GUI-Tests bestanden; beide ZIP-Quellstände isoliert erfolgreich gebaut. Die tatsächlichen BINs bestehen den produktiven OTA-Validator; manipulierte, gekürzte und für den anderen Chip bestimmte Images werden abgewiesen. Hardwareprüfung und realer MQTT-/Lasttest stehen aus. OTA-Dateireserve: etwa 416 KiB beim Wemos und 110 KiB beim ESP32. [Messwerte und Grenzen](docs/RELEASE_v2.0.10.md).

Die nachfolgenden bestätigten Gerätetests beziehen sich ausdrücklich auf den vorherigen **2.0.8-Pre-release**:

- Lokale Tests sowie Wemos-Build und isolierter Build der Quellen aus dem veröffentlichten ZIP.
- OTA von einer eingerichteten 2.0.7 auf 2.0.8 mit Passwortmigration und Übernahme aller 15 Einstellungen.
- Folge-OTA über den neuen Updater mit CSRF und SHA-256, erfolgreichem Abschluss und bestätigtem Neustart.
- Geräteprüfungen für Anmeldung, CSRF/Origin-Schutz, ungültige Images sowie Speichern, Lesen und Wiederherstellen der Einstellungen.
- Sichtbare Uhrzeit, deutscher Wochentag/Datum, Animationen und erreichbare Weboberfläche auf einer Wemos-Uhr.

Offen bleiben unter anderem Werksreset, WLAN-Recovery, physische Stromausfälle und Langzeitbetrieb auf Hardware. Der komplette Online-Installationsablauf wurde noch nicht im echten Browser ausgeführt; GUI-Skripte und geräteseitiger OTA-Pfad wurden getrennt geprüft. [Ausführlicher Release- und Testbericht](docs/RELEASE_v2.0.8.md).

Die lokale Weboberfläche verwendet weiterhin **HTTP ohne TLS** und gehört ins vertrauenswürdige LAN. CRC, Produktkennung und SHA-256 sind **keine Herausgebersignatur**; eine verpflichtende Firmware-Signatur ist nicht aktiviert. Die erste Passwortprüfung nach Neustart kann einige Sekunden benötigen. [Sicherheitskonzept und verbleibende Grenzen](docs/SICHERHEIT_2_0_8.md).

## Selbst bauen

Das Repository mit VS Code und PlatformIO öffnen oder PlatformIO Core verwenden:

```bash
git clone --branch chaosuhr-2.0 https://github.com/EntenPaule/ChaosUhr.git
cd ChaosUhr
pio run -e d1_mini_public
```

Zusätzlich für den klassischen ESP32: `pio run -e esp32dev_public`.

Festgelegte Wemos-Umgebung: ESP8266-Plattform **4.2.1**, Arduino-Core **3.1.2**; ESP32: Plattform **8.0.0**, Arduino-Core **3.3.12**. Gemeinsam FastLED **3.10.9**, WiFiManager **2.0.17**, espMqttClient **1.7.3** und ArduinoJson **7.4.3**; Transportbibliotheken stehen exakt gepinnt in `platformio.ini`. Geprüft mit PlatformIO Core **6.2.0**. Ein privater Secrets-Header ist nicht erforderlich. Ein normaler Build führt keinen Geräte-Upload aus.

Die gebaute BIN liegt unter `.pio/build/d1_mini_public/firmware.bin`. Ein neues lokales Weitergabepaket lässt sich mit folgendem Befehl erstellen:

```bash
python3 scripts/package_public.py
```

Ausgabe: `dist/2.0.10/esp8266/`; für ESP32 `python3 scripts/package_esp32.py` und `dist/2.0.10/esp32/`. Das Skript packt BIN, vollständige öffentliche README, Dokumentation, passende Quellen, Beispiele, Prüfsummen und Lizenzen. Es führt Prüfungen auf persönliche Zugangsdaten durch und lädt selbst nichts hoch. Der Tag [`v2.0.10`](https://github.com/EntenPaule/ChaosUhr/tree/v2.0.10) hält den zum veröffentlichten Paket gehörenden Quellstand fest; die Projektübersicht auf dem Hauptbranch wird weiter gepflegt.

Lokale Tests benötigen Python 3, Node.js, einen C++17-Compiler mit Address-/UndefinedBehaviorSanitizer und OpenSSL-Entwicklungsdateien / `libcrypto`:

```bash
python3 tests/run_host_tests.py
```

### HTTP-API

Die Weboberfläche verwendet dieselbe API. Alle normalen Routen verlangen die Admin-Anmeldung; schreibende Anfragen ab 2.0.8 zusätzlich `X-CSRF-Token` aus `GET /api/session`.

| Route | Zweck |
| --- | --- |
| `GET /api/status` | Version, Uhreinstellungen, Zeit-, WLAN- und Speicherstatus |
| `GET /api/effects` | Verfügbare eingebaute und eigene Animationen |
| `GET /api/animations` | Upload-Plätze und Animationsspeicher |
| `GET /api/session` | Sitzungstoken für schreibende Aufrufe |
| `POST /api/settings` | Einstellungen speichern, auch als Teiländerung |
| `GET /api/mqtt` | MQTT-Konfiguration und Verbindungsstatus ohne Passwort |
| `POST /api/mqtt` | Broker, Topics und Sensorplätze speichern |
| `GET /update` | Manuellen und GitHub-Updater öffnen |

Weitere Upload-Routen stehen in der [Animationsdokumentation](docs/ANIMATIONEN_FORMAT.md). MQTT-Datenempfang und Diagnose werden separat über die [MQTT-Konfiguration](docs/MQTT_2_0_10.md) eingerichtet.

## Dokumentation

- [Neue Version 2.0.10: Teststand](docs/RELEASE_v2.0.10.md), [Upgrade](docs/UPGRADE_2_0_10.md), [MQTT](docs/MQTT_2_0_10.md) und [ESP32](docs/ESP32_FIRMWARE.md)
- [Unterschiede zu Michas Referenzsketch](docs/VERGLEICH_GITHUB.md)
- [Einrichtung und Bedienung](docs/PUBLIC_FIRMWARE.md)
- [Upgrade von 2.0.5, 2.0.7 und Michas Originalversion](docs/UPGRADE_2_0_8.md)
- [Automatik: Auswahl, Zufall und Reihenfolge](docs/AUTOMATIK_2_0_7.md)
- [Eigene Animationen und Dateiformat](docs/ANIMATIONEN_FORMAT.md)
- [ChatGPT-Vorlage](docs/CHATGPT_ANIMATION_PROMPT.txt) und [Herz-Beispiel](examples/Herz.chaos)
- [GitHub-Updatekanal](docs/GITHUB_UPDATES.md)
- [Release- und Teststand 2.0.8](docs/RELEASE_v2.0.8.md)
- [Sicherheit](docs/SICHERHEIT_2_0_8.md) und [Änderungsverlauf](CHANGELOG.md)

## Herkunft und Lizenz

GNU GPL Version 3, siehe [LICENSE](LICENSE). Originalprojekt: **Micha1986 / oOmicha1986Oo**, übernommener Referenzstand `51aef401db22ef0fba3e5e334f141bf794d7505a`. Die 52 verwendeten Originalbilder, Bildfolgen und Zeiten sind durch lokale Tests gegen die Referenz abgesichert.

Hinweise zu Originalanimationen, FastLED, MQTT-/JSON-Bibliotheken, BearSSL/Mbed TLS und beiden projektlokalen HTTP-Servern stehen in [THIRD_PARTY.md](THIRD_PARTY.md). Die zugehörigen vollständigen Lizenztexte sind im Repository und im Weitergabe-ZIP enthalten.
