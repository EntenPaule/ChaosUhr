# ChaosUhr 2.0

Eine bunte LED-Uhr für die **20×15-WS2812B-Matrix**: Uhrzeit, deutscher Wochentag und Datum wechseln sich mit Animationen ab. Über die Weboberfläche lassen sich Anzeige, Automatik und eigene Animationen einstellen – direkt im lokalen WLAN.

Dieser Fork entwickelt die [ChaosUhr von Micha1986 / oOmicha1986Oo](https://github.com/oOmicha1986Oo/ChaosUhr) als PlatformIO-Firmware weiter. Die ursprünglichen Bilder und Animationen sind mit Herkunfts- und Lizenznachweisen übernommen.

**Aktueller Release: [2.0.8 für Wemos D1 mini / ESP8266](https://github.com/EntenPaule/ChaosUhr/releases/tag/v2.0.8), Vorabversion.** Die veröffentlichte BIN läuft auf einer Wemos-Uhr; Anzeige, Weboberfläche und Folge-OTA wurden bestätigt. Der genaue [Teststand](docs/RELEASE_v2.0.8.md) beschreibt auch die noch offenen Prüfungen.

[Downloads](#downloads) · [Funktionen](#funktionen) · [Hardware](#hardware) · [Einrichtung](#erste-einrichtung) · [Eigene Animationen](#eigene-animationen-erstellen-und-teilen) · [Selbst bauen](#selbst-bauen)

## Downloads

Die Dateien stehen im [GitHub-Release v2.0.8](https://github.com/EntenPaule/ChaosUhr/releases/tag/v2.0.8):

| Datei | Inhalt / Verwendung |
| --- | --- |
| [chaosuhr-2.0.8-esp8266.bin](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.8/chaosuhr-2.0.8-esp8266.bin) | Firmware für Wemos D1 mini / ESP8266; diese Datei im Web-Updater auswählen. |
| [chaosuhr-2.0.8-esp8266.zip](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.8/chaosuhr-2.0.8-esp8266.zip) | Dieselbe BIN, Anleitungen, passender Quellcode, Tests, Herz-Beispiel, ChatGPT-Vorlage und Lizenzen. |
| [SHA256SUMS](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.8/SHA256SUMS) | Prüfsummen für BIN und ZIP. |
| [BUILD_INFO.json](https://github.com/EntenPaule/ChaosUhr/releases/download/v2.0.8/BUILD_INFO.json) | Zielhardware, Firmwaregröße, Prüfsumme und Umfang des Gerätetests. |

**Zum Aktualisieren die `.bin` verwenden.** Das ZIP wird vorher entpackt. Die öffentliche BIN enthält keine persönlichen WLAN-, Admin- oder Einrichtungspasswörter; diese werden auf der Uhr eingerichtet. Ein vollständiger Flash-Dump eines bereits eingerichteten Geräts enthält dagegen persönliche Daten.

## Funktionen

- **Uhrzeit und Datum:** NTP-Zeit, deutsche Sommer-/Winterzeit, deutscher Wochentag und Datum als Lauftext unter der Uhrzeit sowie eine Sekundenleiste.
- **30 eingebaute Effekte:** unter anderem Matrix-Regen, Regenbogenwellen, Feuer, Sternenhimmel, Plasma, Polarlicht und die übernommenen Klassiker wie DigiDug, Q*bert, Gesichter, Drink und BombJack.
- **Drei Anzeigemodi:** nur Uhrzeit, eine gewählte Animation oder Automatik mit Uhrphasen zwischen den Animationen.
- **Eigene Automatik-Auswahl:** gewünschte eingebaute und hochgeladene Animationen ankreuzen; zufällige vollständige Runden oder Wiedergabe der Reihe nach.
- **Eigene Animationen:** bis zu acht `.chaos`-Dateien im Browser hochladen, abspielen, herunterladen und löschen. Eingebaute Effekte lassen sich aus der Automatik nehmen.
- **Weboberfläche:** Helligkeit, Animationstempo, Szenendauer und Matrixausrichtung einstellen; Nachtzeitraum mit eigener Helligkeit, auch über Mitternacht.
- **Einstellungen sichern:** JSON-Export und Import mit Vorschau; eigene Animationsdateien separat sichern und teilen.
- **Updates über WLAN:** manueller BIN-Upload sowie bestätigte Updateinstallation aus dem GitHub-Kanal ab 2.0.8.
- **Einrichtung und Verwaltung:** selbst gewähltes Admin-Passwort, geschütztes Einrichtungs-WLAN und bestätigter Werksreset einschließlich WLAN-Daten und eigener Uploads.
- **Diagnose und HTTP-API:** Firmwareversion, Zeitstatus, WLAN-Signal, Heap, Flash-/OTA-Platz, Effektkatalog und Einstellungen auslesen.

Eine ioBroker-/MQTT-Anbindung ist in dieser Neufassung bisher **nicht implementiert**. Bestehende Automationen aus dem Originalprojekt sind deshalb nicht automatisch kompatibel.

### Neu in Version 2.0.8

- Gehärteter HTTP-Parser, zentrale Anmeldung und CSRF-Schutz auch für Einstellungen und Firmware-Updates.
- Gesalzene Speicherung des Admin-Passworts als Verifier, Migration des bisherigen Passworts und internes Wiederherstellungsjournal.
- JSON-Einstellungssicherung mit geprüfter Import-Vorschau. Eigene Animationen werden anhand ihres Dateifingerabdrucks zugeordnet.
- GitHub-Updateprüfung und HTTPS-Download im Browser; Installation erst nach Bestätigung, zusätzliche SHA-256-Prüfung auf der Uhr.
- Strengere Prüfung hochgeladener Firmware auf Zielhardware, Größe, CRC und Produktkennung.

Die Automatik-Auswahl und eigenen `.chaos`-Animationen aus 2.0.7 bleiben erhalten. Einzelheiten stehen im [Änderungsverlauf](CHANGELOG.md) und [Releasebericht 2.0.8](docs/RELEASE_v2.0.8.md).

## Hardware

| Eigenschaft | Aktuelle Wemos-Version |
| --- | --- |
| Controller | Wemos D1 mini / ESP8266 |
| Flash | 4 MiB, Layout mit ungefähr 1 MiB LittleFS für eigene Animationen und Journal |
| LED-Matrix | 300 WS2812B, 20 Spalten × 15 Zeilen |
| LED-Datenpin | **D6 / GPIO12** |
| Ausrichtung ab Werk | Oben rechts, zeilenweise, Zickzack; über die GUI anpassbar |
| Versorgung | Passende 5-V-Versorgung für die Matrix; geschätztes Software-Leistungslimit 5 V / 1,5 A |

Die Software-Leistungsbegrenzung ersetzt keine passend ausgelegte Stromversorgung und Verdrahtung. Der LED-Datenpin ist eine Build-Konstante; die GUI ändert die Matrixausrichtung, nicht den GPIO.

## Erste Einrichtung

Für eine neue Uhr oder nach einem Werksreset:

1. Die zur Hardware passende Firmware installieren. Für eine bereits laufende Uhr den [Upgrade-Ablauf](docs/UPGRADE_2_0_8.md) verwenden.
2. Die Matrix zeigt **SETUP** und einen zwölfstelligen Zahlencode.
3. Mit dem WLAN **`ChaosUhr-Setup-XXXXXX`** verbinden. Sein WLAN-Passwort ist der angezeigte Code.
4. **http://192.168.4.1/** öffnen und ein eigenes Admin-Passwort mit **16–64 druckbaren ASCII-Zeichen** setzen und wiederholen. Leer- und Sonderzeichen sind möglich, Umlaute nicht.
5. Nach dem Neustart vorhandene Heim-WLAN-Daten weiterverwenden lassen oder das Heim-WLAN im Einrichtungsportal hinterlegen.
6. Im Heimnetz **http://chaosuhr.local/** oder die im Router angezeigte Geräte-IP öffnen. Benutzername: **`admin`**, Passwort: das selbst gesetzte Admin-Passwort.

Der Zahlencode für das Einrichtungs-WLAN und das Admin-Passwort sind getrennt. Nach dem Verbinden synchronisiert die Uhr ihre Zeit über NTP. Eine ausführliche Anleitung steht in [Einrichtung und Bedienung](docs/PUBLIC_FIRMWARE.md).

## Vorhandene Uhr aktualisieren

**Von 2.0.5 oder 2.0.7:** Einstellungen notieren und eigene `.chaos`-Dateien herunterladen. Die neue BIN über den bisherigen geschützten Web-Updater installieren. Gültige Uhreinstellungen und der vorhandene Admin-Zugang werden übernommen; das Admin-Passwort bleibt gleich.

Bei bereits lesbarem LittleFS wird das alte Passwortformat migriert. Ohne lesbaren Speicher bleibt die bisherige Anmeldung zunächst aktiv, und die GUI meldet die ausstehende Migration. Eventuell vorhandene Dateien sichern, bevor der Speicher ausdrücklich eingerichtet wird; es gibt kein automatisches Formatieren.

**Von Michas Originalversion:** Die neue BIN wird über das vorhandene Uploadformular übertragen; anschließend ist erstmals die Admin-Ersteinrichtung nötig. Im alten Sketch fest eingestellte Anzeigeparameter lassen sich nicht automatisch übernehmen. Geprüfte Referenz ist Sketch 0.18; angepasste Fremdversionen können abweichen.

**Nach der Passwortmigration sind unveränderte 2.0.5/2.0.7 kein geeigneter Downgrade.** Vor dem Update die [Upgrade-Anleitung für 2.0.8](docs/UPGRADE_2_0_8.md) lesen. Direktupdates von 2.0.5 und Michas Original wurden noch nicht zusätzlich auf echter Hardware getestet.

### Weitere Updates aus der GUI

Unter **OTA-Upload öffnen** bleibt der manuelle Upload einer passenden BIN verfügbar. Ab 2.0.8 kann **Auf Updates prüfen** zusätzlich den GitHub-Kanal abfragen.

Für aktuelle Testversionen **Vorabversionen berücksichtigen** aktivieren: 2.0.8 ist im Testkanal veröffentlicht, eine stabile Version wird noch nicht angeboten. Der Browser braucht Internetzugang; das lokale Admin-Passwort wird nicht an GitHub gesendet. Nur eine neuere passende Version wird zur bestätigten Installation angeboten – keine unbeaufsichtigten Updates und keine erneute Installation derselben Version. [So funktioniert der Updatekanal](docs/GITHUB_UPDATES.md).

## Automatik einstellen

Die gewünschten Animationen auswählen und zwischen zwei Abläufen wählen:

- **Zufällig:** Jede gewählte Animation läuft einmal pro Runde, danach beginnt eine neue zufällige Runde.
- **Der Reihe nach:** Die Auswahl läuft in der Reihenfolge des Katalogs und beginnt anschließend wieder von vorn.

In beiden Fällen gilt: **Uhr → Animation → Uhr → nächste Animation → Uhr …**

Die Szenendauer ist von 5 bis 120 Sekunden einstellbar. **Nur Uhrzeit** und **Gewählte Animation** bleiben unabhängig von der Automatik nutzbar. Zum Speichern einer Automatik-Auswahl muss mindestens eine Animation gewählt sein. [Ausführliche Automatik-Anleitung](docs/AUTOMATIK_2_0_7.md).

## Eigene Animationen erstellen und teilen

Eigene Motive werden als einfache **`.chaos`-Textdateien** hochgeladen. Sie enthalten Farbpalette, Pixelbilder und Bildzeiten; für ein neues Motiv ist kein Firmware-Update nötig.

- **Acht Upload-Plätze**, höchstens **24 KiB je Datei**.
- **20×15 Pixel**, bis zu **16 Farben** und **64 Bilder**.
- Upload, Download, Auswahl und Löschen über die Weboberfläche.
- Eigene und eingebaute Animationen können gemeinsam in der Automatik laufen.

Zum Ausprobieren gibt es [Herz.chaos](examples/Herz.chaos). Mit der [ChatGPT-Vorlage](docs/CHATGPT_ANIMATION_PROMPT.txt) kann man weitere Motive erstellen lassen und die fertigen Dateien beispielsweise im Discord teilen. Die vollständigen Regeln stehen in der [Formatbeschreibung](docs/ANIMATIONEN_FORMAT.md).

Unter **Eigene Animationen** einen noch nicht eingerichteten Speicher bei Bedarf ausdrücklich initialisieren. Diese Aktion löscht vorhandene Dateien in diesem Speicher. Neue Uploads anschließend in der Automatik ankreuzen oder als Einzelanimation auswählen und die Einstellungen speichern.

## Einstellungen sichern und zurücksetzen

**Einstellungen sichern** lädt ab 2.0.8 eine JSON-Datei herunter. Sie enthält die erlaubten Uhreinstellungen, keine WLAN- oder Admin-Passwörter. Eigene Animationen zusätzlich als `.chaos` herunterladen.

Beim Wiederherstellen zuerst die Vorschau prüfen und übernehmen. Das füllt die Formulare; erst **Einstellungen speichern** ändert die Uhr. Benötigte Animationsdateien müssen bereits hochgeladen sein. Ihre Zuordnung erfolgt über SHA-256, auch wenn sie auf anderen Upload-Plätzen liegen.

Ein bestätigter **Werksreset** löscht Uhreinstellungen, Admin-Zugang, Einrichtungs-Code, Heim-WLAN-Daten und eigene Animationen. Danach beginnt die Ersteinrichtung erneut.

## Teststand und Sicherheitsgrenzen

Version 2.0.8 wird als **Pre-release** angeboten. Bestätigt sind:

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

Festgelegte Umgebung: ESP8266-Plattform **4.2.1**, Arduino-Core **3.1.2**, FastLED **3.10.9**, WiFiManager **2.0.17**; geprüft mit PlatformIO Core **6.2.0**. Ein privater Secrets-Header ist nicht erforderlich. Ein normaler Build führt keinen Geräte-Upload aus.

Die gebaute BIN liegt unter `.pio/build/d1_mini_public/firmware.bin`. Ein neues lokales Weitergabepaket lässt sich mit folgendem Befehl erstellen:

```bash
python3 scripts/package_public.py
```

Ausgabe: `dist/2.0.8/esp8266/`. Das Skript packt BIN, vollständige öffentliche README, Dokumentation, passende Quellen, Beispiele, Prüfsummen und Lizenzen. Es führt Prüfungen auf persönliche Zugangsdaten durch und lädt selbst nichts hoch. Der Tag [`v2.0.8`](https://github.com/EntenPaule/ChaosUhr/tree/v2.0.8) hält den zum veröffentlichten Paket gehörenden Quellstand fest; die Projektübersicht auf dem Hauptbranch wird weiter gepflegt.

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
| `GET /update` | Manuellen und GitHub-Updater öffnen |

Weitere Upload-Routen stehen in der [Animationsdokumentation](docs/ANIMATIONEN_FORMAT.md). Eine MQTT-/ioBroker-Integration gehört noch nicht zum Funktionsumfang.

## Dokumentation

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

Hinweise zu Originalanimationen, FastLED, BearSSL und dem projektlokal angepassten ESP8266-Webserver stehen in [THIRD_PARTY.md](THIRD_PARTY.md). Die jeweiligen GPL-, LGPL- und MIT-Lizenztexte sind im Repository und im Weitergabe-ZIP enthalten.
