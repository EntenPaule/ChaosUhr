# ChaosUhr 2.0.11-beta.3 – gemeinsamer Pre-Release

**Vorabversion für Wemos D1 mini / ESP8266 und klassischen ESP32-WROOM-32 / esp32dev, jeweils 4 MiB Flash.**

- **ESP8266:** vom Nutzer auf echter Hardware erfolgreich getestet. Die bereits verteilte Wemos-BIN wird bytegleich übernommen: SHA-256 `d5f5c6c8026c66125cc9c651b6109fdae787611574e5d5c9313e55e9d5bed6c2`.
- **ESP32:** frisch gebaut und offline validiert; **Hardwaretest ausstehend**. Keine Freigabe für ESP32-S3/WT32-Displayboards.

Beta.3 erklärt die Automatik Uhr → Wetter → Sensorwert → Animation und ergänzt eine lokale Auswahlvorschau. MQTT-Stackoptimierung, Admin-Login/Logout mit 30 Minuten Inaktivität, Fehlercodes und unveränderte Wetterdarstellungen sind enthalten. Datenabhängige Seiten werden bei fehlenden/veralteten Werten übersprungen; der Nachtmodus dimmt die Anzeige.

## Downloads und richtige Verwendung

- `ChaosUhr_2.0.11-beta.3_Wemos_OTA.bin`: bestehende, unveränderte Wemos-Firmware für Web-OTA.
- `ChaosUhr_2.0.11-beta.3_ESP32_OTA.bin`: ausschließlich ESP32-Anwendungsimage für den geschützten Web-Updater.
- `ChaosUhr_2.0.11-beta.3_ESP32_Factory.bin`: vollständiges 4-MiB-USB-Image ab **0x0** für ESP32. **Niemals für OTA.** Die vollständige USB-Neuinstallation überschreibt vorhandene Daten einschließlich WLAN, Einstellungen und Animationen.
- `SHA256SUMS_2.0.11-beta.3_GEMEINSAM.txt`: Prüfsummen aller Assets.
- `TESTANLEITUNG_2.0.11-beta.3_GEMEINSAM.md` und `BUILD_INFO_2.0.11-beta.3_GEMEINSAM.json`: Einrichtung, Testgrenzen, tatsächliche Offsets/Partitionen und Buildinformationen.

## ESP32-Anpassung und knapper OTA-Platz

Der erste ESP32-Build kompilierte erfolgreich, seine BIN war mit 1969280 Byte aber **3200 Byte zu groß** für den vorhandenen OTA-Slot. Diese Datei wird nicht veröffentlicht.

Auf ESP32 wurde deshalb ausschließlich der bereits funktionslose ESP8266-RTC-Diagnosepfad von der Codeerzeugung ausgenommen. ESP32 meldet weiterhin `reset.available: false`. Die vorhandene ESP32-WLAN-Logik, MQTT, Anzeige und GUI bleiben erhalten. Für ESP8266 ist der vorverarbeitete Code vor/nach der Änderung identisch. **Wemos wurde nicht neu gebaut oder verändert.**

Der finale ESP32-Build hat **1965072 Byte** und passt in beide **1966080-Byte-OTA-Slots**; Reserve nur **1008 Byte**. Auch die auf Flashsektoren aufgerundete Größe passt. Jede künftige Codeänderung muss erneut anhand der tatsächlichen BIN geprüft werden. Keine Partition wurde vergrößert.

## Bekannte ESP32-Einschränkungen und offene Hardwaretests

- Verbindung eines Clients zum Einrichtungs-WLAN/AP und Durchlaufen der Ersteinrichtung.
- WLAN-Zugangsdaten speichern, vollständig stromlos machen, danach Persistenz und automatische Wiederverbindung prüfen. Frühere AP-/Persistenzprobleme sind **nicht durch einen neuen Hardwaretest als behoben bestätigt**.
- Admin-Einrichtung, Login/Logout/30-Minuten-Ablauf, CSRF und Sitzung nach Neustart.
- LittleFS ausdrücklich einrichten, MQTT-Speichern/Neuladen, echter Broker, Kaltstart und Recovery.
- LED-Pin/Matrixausrichtung, Uhr/Datum, Wetter, Sensoren und Animationen.
- ESP32-Web-OTA einschließlich Neustart; sehr knappe OTA-Reserve beachten.
- Freier Heap unter Last, Langzeitbetrieb und stabile Stromversorgung.

Die ESP8266-RTC-/Stack-Neustartdiagnose ist auf ESP32 nicht verfügbar. Lokales HTTP/MQTT bietet keine TLS-Vertraulichkeit. Einmaliges MQTT-011 aus dem Wemos-Beta.2-Test ist dokumentiert; AUTH-021 nach Neustart ist erwartetes Verhalten.

## Veröffentlichungsumfang

Ein gemeinsamer Branch, ein gemeinsamer finaler Quellstand und Tag `v2.0.11-beta.3` für beide Plattformen. Die ESP32-Änderung ist im gemeinsamen Quellstand nachvollziehbar; die Wemos-BIN behält ihren ursprünglichen Build- und Testnachweis. Frühere Wemos-Testpakete bleiben unverändert erhalten.

**Nur Pre-Release, kein Stable-Release.** Produktiver Branch sowie Stable-/Preview-Updatekanäle werden nicht umgeschaltet. Installation erfolgt für diesen Test manuell über die Release-Dateien. Beta.4-Entwicklung beginnt erst nach Veröffentlichung und gehört nicht zum Release-Tag.

## Geprüfte Firmwaredateien

| Datei | Byte | SHA-256 |
|---|---:|---|
| ChaosUhr_2.0.11-beta.3_Wemos_OTA.bin | 689456 | `d5f5c6c8026c66125cc9c651b6109fdae787611574e5d5c9313e55e9d5bed6c2` |
| ChaosUhr_2.0.11-beta.3_ESP32_OTA.bin | 1965072 | `09b563d69ca2826327716a96f14ed4a62bc88b16fc2fd21af02ba5ea7e2ea2a6` |
| ChaosUhr_2.0.11-beta.3_ESP32_Factory.bin | 4194304 | `7b2e97a69276fcafa40afb4a7e6c640c260b9faa27f506a38771e1b4d5bfc9ae` |

Gemeinsamer Firmware-Quellcommit: `ed7d65cf83a6d8dbbc154a73dc5e694b025315ac`.
