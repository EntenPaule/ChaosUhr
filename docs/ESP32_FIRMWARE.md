# ChaosUhr 2.0.7 – ESP32-Testversion

**2.0.8 gilt nur für Wemos:** Das neue Sicherheitsprofil bietet keinen ESP32-Build an. Für den unveränderten ESP32-2.0.6-Stand dessen separates ZIP mit eigener Konfiguration verwenden. Die nachfolgende Beschreibung des zurückgestellten 2.0.7-Zwischenstands ist historisch.

**Paketpflege für 2.0.7 vorerst zurückgestellt:** Auf Wunsch wird zunächst nur der Wemos-Build veröffentlicht. Der gemeinsame ESP32-Code wurde bereits kompiliert; die folgenden 2.0.7-Dateinamen sind für eine spätere Paketfreigabe vorgesehen. Das vorhandene 2.0.6-Paket liegt weiterhin unter `dist/esp32/`.

**Erfolgreich kompiliert und lokal getestet; noch nicht auf einem ESP32-Gerät ausgeführt.**

Ziel ist ein **klassischer ESP32-WROOM-32 / ESP32 Dev Module mit 4 MB Flash** (`esp32dev`). LED-Matrix: **20×15 WS2812B**, Datenleitung **GPIO32**, GRB. Standardausrichtung: oben rechts, zeilenweise, Zickzack. Die LED-Versorgung bleibt 5 V; der Controller benötigt gemeinsame Masse mit der Matrix. GPIO32 darf nicht mit einer 5-V-Versorgungsleitung verbunden werden.

Dieser Build ist für den klassischen ESP32 bestimmt. **ESP8266, ESP32-S3, ESP32-C3 und andere Varianten brauchen eigene passende Builds.** Die Aufschrift `WT32-S3-WROVER` bezeichnet eine S3-Variante; ohne genaue Board-Pinbelegung ist die vorliegende BIN dafür nicht passend. Ethernet, Displays und Touch-Funktionen von WT32-Boards sind in dieser Firmware nicht eingerichtet.

## Dateien

Die aktuellen ESP32-Dateien liegen unter `dist/2.0.7/esp32/`, getrennt vom Wemos-Build und den bisherigen 2.0.6-Dateien unter `dist/esp32/`.

| Datei | Verwendung |
| --- | --- |
| `chaosuhr-2.0.7-esp32-factory.bin` | USB-Erstinstallation, vollständiges 4-MB-Image ab **Adresse 0x0**; enthält Bootloader, Partitionstabelle, OTA-Initialisierung und Anwendung. |
| `chaosuhr-2.0.7-esp32-ota.bin` | Nur die Anwendung für spätere Updates über `/update` einer passend eingerichteten ESP32-Uhr. Enthält keinen Bootloader und keine Partitionstabelle. |
| `chaosuhr-2.0.7-esp32.zip` | Beide BINs, Anleitungen zur Einrichtung/Automatik, Build-Metadaten, Prüfsummen, GPL-/Drittlizenzhinweise und passender Quellcode. |

Die OTA-BIN liegt im Flash bei 0x10000 oder im zweiten Anwendungsslot; sie ist kein vollständiges USB-Startimage. Die Factory-BIN gehört nicht in den Web-Updater. **Keine dieser Dateien auf die bisherige ESP8266-Uhr übertragen.**

## Erstinstallation per USB

1. Zielchip, 4-MB-Flash und freie GPIO32-Datenleitung am konkreten Board prüfen. LED-Datenpegel und Versorgung müssen zur vorhandenen Matrix passen.
2. Die `factory.bin` über USB mit einem ESP32-fähigen Flashwerkzeug an **0x0** schreiben. Das vollständige Image überschreibt auch bisherige Einstellungen und Zugangsdaten des Zielboards.
3. Beispiel mit esptool 5, seriellen Anschluss an das eigene Gerät anpassen:

   ```bash
   esptool --chip esp32 --port /dev/ttyUSB0 --baud 460800 write-flash 0x0 chaosuhr-2.0.7-esp32-factory.bin
   ```

4. Nach dem Start muss die LED-Matrix **SETUP** und einen laufenden zwölfstelligen Zahlencode anzeigen.
5. Mit **ChaosUhr-Setup-XXXXXX** verbinden; WLAN-Passwort ist der Code auf der Matrix. **http://192.168.4.1/** öffnen und ein Admin-Passwort mit **16–64 druckbaren ASCII-Zeichen** setzen und wiederholen.
6. Nach dem Neustart erneut mit dem Einrichtungs-WLAN verbinden und das Heim-WLAN im Portal einrichten. Danach die Uhr unter **http://chaosuhr.local/** oder ihrer Geräte-IP öffnen. Benutzername **admin**, Passwort wie selbst gewählt.

## Funktionen und spätere Updates

Enthalten sind die 30 Effekte inklusive der alten Animationen, deutscher Wochentag und Datum, Europe/Berlin-Zeit mit Sommerzeit, Helligkeit, Nachtmodus, Tempo, Szenendauer und Matrixausrichtung. Admin-Ersteinrichtung und geschützter Werksreset entsprechen der öffentlichen ESP8266-Version.

**Neu in 2.0.7:** Animationen für die Automatik auswählen und zufällige Runden oder Wiedergabe der Reihe nach einstellen; immer Uhr → Animation → Uhr → nächste Animation. Die gewählte Einzelanimation bleibt davon unabhängig. Bedienung und Übernahme bestehender Einstellungen: [AUTOMATIK_2_0_7.md](AUTOMATIK_2_0_7.md).

Für spätere Firmwarewechsel im angemeldeten Web-Updater ausschließlich die **ESP32-OTA-BIN** verwenden. Das konfigurierte 4-MB-Layout besitzt zwei Anwendungsslots mit jeweils **1.966.080 Byte**. Ein normales OTA-Update erhält die gespeicherten WLAN-, Admin- und Uhreinstellungen. Ein Werksreset verlangt Admin-Anmeldung und Bestätigung und löscht diese Daten. Die WLAN-Daten werden über den ESP32-Treiber zurückgesetzt; der EEPROM-Inhalt wird erst danach gelöscht. Unterbrochene Resets behalten ihre Markierung bis zum Abschluss.

Die bisherige 128-Byte-EEPROM-Schnittstelle bleibt für denselben getesteten Einstellungs-/Admin-Code erhalten; auf ESP32 speichert die Arduino-EEPROM-Bibliothek ihren Inhalt im NVS. 2.0.7 bewahrt den alten 20-Byte-Einstellungsblock und ergänzt die Automatik in Bytes 20–27; Admin-Daten beginnen weiterhin ab Offset 32. Die verteilten BINs enthalten keine persönlichen Passwörter. Ein Flash-Dump eines bereits eingerichteten Geräts enthält dagegen persönliche Daten.

## Bauen und lokal prüfen

```bash
pio run -e esp32dev_public
python3 tests/run_host_tests.py
python3 scripts/package_esp32.py
```

Im Hauptprojekt bleibt `pio run` auf ESP8266 voreingestellt. Im ESP32-Weitergabepaket ist der Standard auf `esp32dev_public` gesetzt. Beide Ziele verwenden dieselbe Codebasis. Für beide Controller entstehen unterschiedliche BINs.

Festgelegte Umgebung: PlatformIO **espressif32 8.0.0**, Arduino-ESP32 **3.3.12**, Xtensa GCC **14.2.0**, FastLED **3.10.9**, WiFiManager **2.0.17**. GPIO32 ist per `CHAOSUHR_LED_PIN=32` eine Build-Konstante. Die Paketprüfung kontrolliert Zielchip, Image-Prüfsumme, Partitionslayout, beide OTA-Slots, Flash-Adressen, leeren NVS-Bereich im Factory-Image und Abwesenheit persönlicher Zugangsdaten. Es erfolgt kein automatisches Flashen.

Versionsquellen: [PlatformIO Espressif32 8.0.0](https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0), [Arduino-ESP32 3.3.12](https://github.com/espressif/arduino-esp32/releases/tag/3.3.12).

## Teststand

Build-Metadaten und Dateiprüfsummen stehen in `BUILD_INFO.json` und `SHA256SUMS` im Paket. Das USB-Image ist 4.194.304 Byte groß. Der Paketcheck bestätigt dessen Anwendung ab 0x10000 als bytegleich zur OTA-BIN. In der verwendeten Toolchain sind externe Compilerwarnungen aus FastLED, WiFiManager und einem veralteten IDF-Treiberheader bekannt.

**Kein Upload und kein Gerätetest mit 2.0.7.** ESP32-Gerätetests stehen aus: sichtbare Matrixausgabe und LED-Timing während WLAN-Zugriffen, Ersteinrichtung, Wiederverbindung, Neustart/Persistenz, Automatik, OTA und Werksreset einschließlich Stromunterbrechung. Lokale Tests prüfen die echten HTTP-Handler mit Test-Doubles für ESP8266 und ESP32 sowie Datum, Animationen, Szenen, Auswahllisten, Migration und Netzwerkzustände. Das ersetzt diese Hardwaretests nicht.

Die Leistungsbegrenzung für 300 LEDs bleibt bei geschätzten 5 V/1,5 A mit zusätzlicher Helligkeitsbegrenzung. Weboberfläche und Basic Auth verwenden HTTP; vorgesehen ist der Zugriff im vertrauenswürdigen lokalen Netz.

Ursprüngliches Projekt: https://github.com/oOmicha1986Oo/ChaosUhr . Übernommene Bilder/Effekte und Anpassungen: `THIRD_PARTY.md`. Lizenz: GNU GPL Version 3, siehe `LICENSE`; FastLED-Lizenz unter `third_party/FastLED-LICENSE`.
