# ChaosUhr 2.0.10 – klassischer ESP32

Für **ESP32-WROOM-32 / esp32dev**, 4 MiB Flash, 20×15 WS2812B. Standard-LED-Datenpin **GPIO32**, weitere erlaubte Pins im GUI-Dropdown. Eigene Anwendung mit passendem Zielchip; die Wemos-BIN darf nicht auf ESP32 installiert werden.

Die neue 2.0.10 ist lokal gebaut und geprüft, **noch nicht am Gerät getestet**. Laut Nutzerfeedback läuft die ältere als 2.0.6 bezeichnete ESP32-Portierung auf 2.0.5-Basis; daraus folgt keine Bestätigung dieser neuen Firmware.

## Paket und Speicherlayout

Unter `dist/2.0.10/esp32/` entstehen:

| Datei | Verwendung |
| --- | --- |
| `chaosuhr-2.0.10-esp32-ota.bin` | Anwendungsupdate über den bisherigen Web-Updater |
| `chaosuhr-2.0.10-esp32-factory.bin` | Vollständiges USB-Erstinstallationsimage ab 0x0; überschreibt alle Daten des Zielboards |
| `chaosuhr-2.0.10-esp32.zip` | Beide BINs, Anleitungen, Quellen, Beispiele und Lizenzen |
| `SHA256SUMS`, `BUILD_INFO.json` | Prüfsummen, Zielchip, Layout und Teststand |

Unverändertes 4-MiB-Layout: zwei OTA-Anwendungsslots mit je **1.966.080 Byte**, NVS für die bisherigen EEPROM-/WLAN-Daten und **192 KiB LittleFS** am bisherigen Dateisystemplatz. Keine Vergrößerung des Dateisystems auf Kosten der vorhandenen OTA-Slots. Von den acht Upload-Plätzen lassen sich abhängig von Dateigrößen und Dateisystemreserve nicht zwingend alle mit maximal großen Dateien füllen.

## Vorhandenen ESP32 aktualisieren

Einstellungen und eigene Dateien vorher sichern. **Ausschließlich die ESP32-OTA-BIN** über das bisherige Formular installieren. Gültige Einstellungs- und Admin-Daten werden bei passendem Layout übernommen. Die neue Firmware hat auch auf ESP32 gesalzene Passwortmigration mit Journal, Uploads und den geschützten Folge-Updater. Ohne lesbares LittleFS wird zunächst die alte Anmeldung erhalten; keine automatische Formatierung.

[Upgrade und Rückweg](UPGRADE_2_0_10.md), [MQTT / Wetter / Sensoren](MQTT_2_0_10.md).

## USB-Erstinstallation

Nur für ein passendes leeres Board oder ausdrücklich gewollte Neuinstallation: das vollständige `-factory.bin` mit einem ESP32-fähigen Flashwerkzeug an **0x0** schreiben. Beispielsweise mit esptool 5, Port an das eigene Gerät anpassen:

```bash
esptool --chip esp32 --port /dev/ttyUSB0 write-flash 0x0 chaosuhr-2.0.10-esp32-factory.bin
```

Dieser Schritt löscht vorhandene Konfiguration. Danach SETUP-Code auf der Matrix ablesen, mit `ChaosUhr-Setup-XXXXXX` verbinden und unter `http://192.168.4.1/` das eigene Admin-Passwort setzen. Heim-WLAN im Portal einrichten; normale GUI unter `http://chaosuhr.local/` oder der im Router angezeigten Adresse öffnen. Benutzername `admin`. Ein erfolgreicher WLAN-Start blendet die eigene IP auf der Matrix ein.

## Funktionen und Updates

Gleiches Funktionsangebot wie beim Wemos: 30 Effekte, Datum/Wochentag mit Kleinbuchstaben oder Kurzform, abschaltbare Sekundenleiste, Anzeigearten-Auswahl, eigene .chaos-Dateien, MQTT-Wetter und vier Sensorplätze. Standard GPIO32 bleibt bei der Übernahme alter Daten erhalten; eine Pinänderung über die GUI benötigt Bestätigung und Neustart.

Der GitHub-Updater liest getrennte ESP32-Manifeste `stable-esp32.json` / `preview-esp32.json`. Die 2.0.10 ist im Preview-Kanal veröffentlicht. Ein Update wird erst nach Bestätigung installiert. Vorabversionen über das entsprechende Häkchen berücksichtigen. Der frühere ESP32-2.0.6-Basisstand benötigt für diesen Wechsel zunächst den manuellen OTA-Upload.

## Selbst bauen

```bash
pio run -e esp32dev_public
python3 tests/run_host_tests.py
python3 scripts/package_esp32.py
```

Plattform espressif32 8.0.0, Arduino-Core 3.3.12, FastLED 3.10.9, WiFiManager 2.0.17; MQTT-/JSON-Abhängigkeiten exakt in `platformio.ini` festgelegt. Die Paketierung prüft Chip, Anwendungs-Hash, Partitionen, Boot-/Anwendungs-Offsets, leeres Factory-NVS und Abwesenheit lokaler Zugangsdaten. Der komplette passende Quellstand einschließlich projektlokalem HTTP-Server und Tests liegt bei. Kein Build führt einen Upload aus.

Lokales HTTP und MQTT bleiben unverschlüsselt; keine verpflichtende Herausgebersignatur. Hardwareprüfungen für LED-Timing, neue Pins, WLAN-/MQTT-Last, Migration, Folge-OTA, Reset und Langzeitbetrieb stehen aus. [Release- und Teststand](RELEASE_v2.0.10.md), [Lizenzen](../THIRD_PARTY.md).
