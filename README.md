# ChaosUhr 2.0

Eine Neufassung der [ChaosUhr von Micha1986](https://github.com/oOmicha1986Oo/ChaosUhr) für eine **20×15-WS2812B-Matrix**. Dieser Fork enthält den aktuellen PlatformIO-Quellcode und Firmware-Releases. Originalbilder und ihre Herkunft sind in [THIRD_PARTY.md](THIRD_PARTY.md) dokumentiert.

## Firmware herunterladen

[Release v2.0.7 – Wemos D1 mini / ESP8266](https://github.com/EntenPaule/ChaosUhr/releases/tag/v2.0.7)

Zielhardware: **Wemos D1 mini, ESP8266, 4 MiB Flash, LED-Datenpin D6/GPIO12**. Das ZIP enthält dieselbe BIN wie der einzelne Download, Anleitung, Quellcode, Lizenzen und ein Animationsbeispiel. Prüfsummen stehen in `SHA256SUMS`. Passwörter werden erst auf dem Gerät eingerichtet.

2.0.7 ist eine **Vorabversion**. Genau die veröffentlichte BIN wurde auf einem Wemos per OTA gestartet; Herz-Upload und Originaldownload sind bestätigt. Vollständige Automatik-Runden, Löschen, Werksreset und Langzeitbetrieb sind am Gerät noch nicht vollständig geprüft. [Ausführlicher Teststand](docs/RELEASE_v2.0.7.md).

Dieser Release enthält ausschließlich die Wemos-Version. Die klassische ESP32-Portierung ist im Quellcode vorhanden, ihre Upload-Verwaltung und ein neues ESP32-Paket sind zurückgestellt. WT32-/ESP32-S3-Displayboards benötigen ein eigenes Projekt.

## Funktionen

- Uhrzeit über NTP, deutsche Sommer-/Winterzeit, deutscher Wochentag und Datum.
- 30 eingebaute Effekte einschließlich der Originalanimationen.
- Automatik mit eigener Auswahl: zufällige vollständige Runden oder der Reihe nach, mit Uhranzeige zwischen den Animationen.
- Bis zu acht eigene `.chaos`-Animationen über die Weboberfläche hochladen, auswählen, herunterladen und löschen.
- Helligkeit, Nachtmodus, Tempo, Szenendauer und Matrixausrichtung über die Weboberfläche.
- Eigenes Admin-Passwort bei der Ersteinrichtung, geschütztes Web-OTA und Werksreset einschließlich WLAN-Daten und Uploads.

Eine ioBroker-/MQTT-Anbindung ist in dieser Neufassung bisher nicht enthalten.

## Einrichtung und Animationen

[Einrichtung und Updates](docs/PUBLIC_FIRMWARE.md) · [Automatik](docs/AUTOMATIK_2_0_7.md) · [Eigene Animationen und Dateiformat](docs/ANIMATIONEN_FORMAT.md) · [Änderungen](CHANGELOG.md)

Bei einer neuen Uhr zeigt die Matrix SETUP und einen Zahlencode. Mit `ChaosUhr-Setup-XXXXXX` verbinden, den angezeigten Code als WLAN-Passwort verwenden und `http://192.168.4.1/` öffnen. Dort ein eigenes Admin-Passwort setzen und anschließend das Heim-WLAN einrichten. Die normale Oberfläche verwendet den Benutzernamen `admin`.

Das Beispiel [Herz.chaos](examples/Herz.chaos) und die [ChatGPT-Vorlage](docs/CHATGPT_ANIMATION_PROMPT.txt) können zum Erstellen und Teilen eigener Animationen verwendet werden. Eine neue Animation erfordert kein Firmware-Update.

## Selbst bauen und prüfen

Mit PlatformIO Core (geprüft mit 6.2.0):

```bash
pio run -e d1_mini_public
python3 scripts/package_public.py
```

Das Paket entsteht unter `dist/2.0.7/esp8266/`. Ein Build führt keinen Geräte-Upload aus. Festgelegte Abhängigkeiten: ESP8266-Plattform 4.2.1, FastLED 3.10.9, WiFiManager 2.0.17. Ein privater Secrets-Header ist nicht erforderlich.

Lokale Tests benötigen Python 3, Node.js und einen C++17-Compiler mit Address-/UndefinedBehaviorSanitizer:

```bash
python3 tests/run_host_tests.py
```

## Lizenz

GNU GPL Version 3, siehe [LICENSE](LICENSE). Originalprojekt: Micha1986 / oOmicha1986Oo; übernommener Stand `51aef401db22ef0fba3e5e334f141bf794d7505a`. Weitere Hinweise und die FastLED-Lizenz stehen in [THIRD_PARTY.md](THIRD_PARTY.md).
