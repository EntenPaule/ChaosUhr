> Historischer unnummerierter 2.0.11-Teststand. Aktuell: [2.0.11-beta.1](BERICHT_2.0.11-beta.1.md); dessen Testumfang und ausschließliches Wemos-Paket sind maßgeblich. Kein offizielles 2.0.11-Release.

# ChaosUhr 2.0.11 – Flash-Anleitung zur Testversion

Lokaler Teststand, noch keine Hardwarefreigabe. Geeignet für Wemos D1 mini / ESP8266 und klassischen ESP32-WROOM-32 / `esp32dev`, jeweils mit 4 MiB Flash. Nicht für ESP32-S3, WT32-SC01 Plus oder andere Displayboards.

## Dateien auswählen

| Datei | Verwendung |
| --- | --- |
| `ChaosUhr_2.0.11_Wemos_OTA.bin` | Wemos: Web-OTA; für USB auch bei Adresse `0x0` verwendbar. |
| `ChaosUhr_2.0.11_ESP32_OTA.bin` | Klassischer ESP32: ausschließlich Anwendungsimage für Web-OTA auf einer kompatiblen ChaosUhr-Installation mit passendem Layout. |
| `ChaosUhr_2.0.11_ESP32_Factory.bin` | Klassischer ESP32: vollständige USB-Neuinstallation bei Adresse `0x0`. **Nie für OTA auswählen.** |

Vorher Einstellungen als JSON und eigene Animationen separat sichern. Passwörter sind nicht in der Einstellungssicherung enthalten. ZIP entpacken und im entpackten Verzeichnis `sha256sum -c SHA256SUMS_2.0.11.txt` ausführen. Alle aufgeführten Dateien müssen `OK` ergeben. Die externe Prüfsummendatei neben dem ZIP enthält zusätzlich die ZIP-Prüfsumme; die Datei im ZIP prüft dessen Inhalte.

## Bestehende Installation per Web-OTA aktualisieren

1. Weboberfläche der bisherigen Uhr öffnen und als `admin` anmelden.
2. Lokalen Firmware-Upload unter `/update` öffnen. Für das eigene Board genau die passende **OTA**-BIN aus obiger Tabelle auswählen.
3. Upload bestätigen, stabile Stromversorgung sicherstellen und Neustart abwarten.
4. Version **2.0.11**, Anzeige, Einstellungen und gespeicherte Animationen prüfen. Anschließend die Testcheckliste abarbeiten.

Nach dem Update auf diese Testfassung erfolgt die Anmeldung über das sichtbare Formular mit dem bisherigen Admin-Passwort. HTTP-Basic-Caches ersetzen keine Anmeldung mehr. Hauptseite und `/update` zeigen den Status und **Abmelden** oben rechts. Nach 30 Minuten ohne bestätigte Aktivität oder nach Neustart erneut anmelden; Hintergrund-Statusabfragen verlängern nicht. Einzelheiten stehen in `SITZUNGEN_2.0.11.md` im Test-ZIP beziehungsweise `docs/SITZUNGEN_2_0_11.md` im Quellcode.

Bestehende kompatible Einstellungen und Zugangsdaten sollen bei OTA erhalten bleiben. Keine Formatierung und keinen Werksreset auslösen. Die GitHub-Updatekanäle bleiben unverändert; sie bieten diese lokale Testversion nicht an.

## USB-Neuinstallation

Die Befehle sind Beispiele für **esptool 5.x**. `/dev/ttyUSB0` durch den eigenen seriellen Port ersetzen, einen eventuell geöffneten seriellen Monitor schließen und gegebenenfalls den Bootmodus des Boards aktivieren. Die Befehle wurden offline auf korrekte esptool-Syntax geprüft; es wurde kein Gerät geflasht.

**ESP32-Factory-Installation überschreibt alle 4 MiB einschließlich WLAN-/Admin-/Uhrdaten und Animationen.** Nur für eine beabsichtigte Neuinstallation verwenden. Ein zusätzliches `erase-flash` ist für dieses vollständig aufgefüllte Factory-Image nicht erforderlich.

Wemos D1 mini, passende OTA-BIN auch für USB:

```sh
python -m esptool --chip esp8266 --port /dev/ttyUSB0 write-flash --flash-mode dio --flash-freq 40m --flash-size 4MB 0x0 ChaosUhr_2.0.11_Wemos_OTA.bin
```

Klassischer ESP32-WROOM-32 mit 4 MiB, vollständiges Factory-Image:

```sh
python -m esptool --chip esp32 --port /dev/ttyUSB0 write-flash --flash-mode dio --flash-freq 40m --flash-size 4MB 0x0 ChaosUhr_2.0.11_ESP32_Factory.bin
```

Der Wemos-USB-Befehl schreibt das Programm, aber kein leeres vollständiges Dateisystem-/Konfigurationsimage. Vorhandene Gerätekonfiguration kann dort erhalten bleiben. Für ein absichtliches Zurücksetzen die bestätigungspflichtige Funktion der GUI verwenden.

## Verifizierte Bestandteile des ESP32-Factory-Images

Die Offsets stammen aus den tatsächlichen PlatformIO-Metadaten von `esp32dev_public`. Die erzeugte binäre Partitionstabelle wurde mit der Projekt-CSV und dem Partitionstool des installierten Cores verglichen, einschließlich MD5-Prüfung.

| Bestandteil | Flash-Adresse |
| --- | --- |
| Bootloader aus dem frischen Build | `0x1000` |
| Erzeugte Partitionstabelle | `0x8000` |
| Zum Core gehörendes `boot_app0.bin` | `0xe000` |
| Bytegleiches ESP32-OTA-Anwendungsimage | `0x10000` |

| Partition | Start | Größe |
| --- | --- | --- |
| NVS | `0x9000` | `0x5000` |
| OTA-Verwaltung | `0xe000` | `0x2000` |
| Anwendung / OTA 0 | `0x10000` | `0x1e0000` |
| OTA 1 | `0x1f0000` | `0x1e0000` |
| LittleFS, Partitionsname `spiffs` | `0x3d0000` | `0x30000` |

Flashmodus DIO, 40 MHz, 4 MiB. Das Factory-Image beginnt bei `0x0`, ist exakt 4.194.304 Byte groß und enthält außerhalb der geprüften Komponenten ausschließlich `0xff`, insbesondere leeres NVS, leeren zweiten OTA-Slot und leeres LittleFS. Die Partitionen werden durch dieses Testpaket nicht geändert.

## Ersteinrichtung

Nach einer Neuinstallation erscheinen `SETUP` und ein Zahlencode auf der Matrix. Mit dem geschützten Setup-WLAN der Uhr verbinden, dessen angezeigten Code verwenden und `http://192.168.4.1/` öffnen. Eigenes Admin-Passwort setzen und Heim-WLAN einrichten. Anschließend zeigt die Uhr ihre IP-Adresse. Standard-LED-Pin: Wemos D6 / GPIO12, ESP32 GPIO32; andere erlaubte Pins können in der GUI gewählt werden.

Wurde WLAN von einer älteren ESP32-Firmware nur im RAM gehalten und ist bereits verloren gegangen, muss es einmal neu eingegeben werden. Die Testversion korrigiert die Persistenzpolitik; drei echte Kaltstarts müssen noch bestätigt werden.
