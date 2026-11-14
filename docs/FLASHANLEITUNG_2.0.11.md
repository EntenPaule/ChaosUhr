# ChaosUhr 2.0.11 – Installation

Für Wemos D1 mini / ESP8266 und klassischen ESP32-WROOM-32 / esp32dev mit
4 MiB Flash. Nicht für ESP32-S3, WT32-SC01 Plus oder Displayboards.
Die [Release Notes](RELEASE_v2.0.11.md) beschreiben den tatsächlichen Teststand.

## Passende Datei

- Wemos: `chaosuhr-2.0.11-esp8266.bin` für OTA oder USB bei 0x0.
- ESP32: `chaosuhr-2.0.11-esp32-ota.bin` für Web-OTA auf einer kompatiblen
  ChaosUhr-Installation mit zwei 1.966.080-Byte-Appslots und 192 KiB LittleFS.
- **ESP32 Factory: `chaosuhr-2.0.11-esp32-factory.bin` ausschließlich für bewusst
  gewünschte destruktive USB-Neuinstallation bei 0x0. Überschreibt alle 4 MiB
  einschließlich WLAN-/Admin-/Uhrdaten und eigener Animationen. Niemals OTA.**

Drei BINs und `SHA256SUMS.txt` aus dem GitHub-Release v2.0.11 herunterladen.
Im Downloadordner `sha256sum -c SHA256SUMS.txt` ausführen; alle drei müssen OK sein.
Keine historischen unnummerierten 2.0.11-Test-BINs aus alten Paketen verwenden.

## Bereits eingerichtete Uhr per OTA

1. Uhreinstellungen als JSON und eigene `.chaos`-Dateien separat sichern.
2. Im bestehenden Web-Updater die passende OTA-BIN wählen und bestätigen.
3. Stabile Stromversorgung bis zum Neustart sicherstellen.
4. Danach im Formular mit dem bisherigen Admin-Passwort anmelden, Version 2.0.11
   und gespeicherte Werte prüfen. Nach Neustart sind alte Sitzungen ungültig.

Keinen Werksreset, kein Formatieren, keine LittleFS-Initialisierung für ein
normales Update. Bei unpassendem Layout nicht mit Factory überschreiben;
zuerst Daten sichern und eine bewusste Neuinstallation planen.
Für ältere 2.0.5/2.0.7-Versionen auch die Hinweise zur Passwortmigration im
[Upgrade 2.0.8](UPGRADE_2_0_8.md) beachten. Direkte Migrationen aus angepassten Fremdsketches
sind nicht pauschal hardwarebestätigt.

## ESP32 neu und ohne Matrix

Die folgenden Schritte sind für den Nutzer, wurden für diese Veröffentlichung
nicht auf echter Hardware ausgeführt. UART- und OTA-/Persistenztests bleiben offen.

1. Bei bewusst gewünschter Neuinstallation das vollständige Factory-Image mit
   esptool 5.x an Adresse 0x0 schreiben. Kein separates vollständiges Erase nötig.
2. Nach Boot meldet eine noch uninitialisierte LittleFS-Partition gegebenenfalls
   FS-001. Die Firmware formatiert nicht automatisch.
3. Ohne Matrix das aktuelle Skript `scripts/esp32_usb_setup.py` aus Tag v2.0.11
   verwenden (Python 3, pyserial). Andere serielle Portnutzer schließen.
4. Innerhalb von fünf Minuten nach Boot und vor Admin-Einrichtung ausführen:
   `python3 scripts/esp32_usb_setup.py /dev/ttyUSB0` (Port anpassen).
   Einen selbst gewählten zwölfstelligen Code zweimal verdeckt eingeben.
   Das Skript wartet auf READY und danach ACK; ohne Bestätigung nicht von
   erfolgreicher Übernahme ausgehen. Code/Passwörter nicht in Logs teilen.
5. Mit dem Setup-WLAN und diesem Code verbinden, `http://192.168.4.1/` öffnen,
   Admin-Passwort einrichten und anschließend WLAN-Zugang speichern.
6. Uninitialisiertes LittleFS nur bei bewusst akzeptiertem Datenverlust über die
   bestätigte GUI-Aktion einrichten. Diese löscht Dateien dieser Partition,
   einschließlich MQTT-/Anzeigeoptionen, Animationen und Journal; NVS bleibt
   unangetastet. Bei bestehender Installation zuerst sichern.
7. WLAN-/MQTT-Persistenz durch Kaltstart prüfen, danach weiteres OTA separat testen.

Mit Matrix bleibt die bestehende Setup-Code-Anzeige verfügbar. Ein alter Code
im gespeicherten WLAN-Profil kann eine neue AP-Anmeldung verhindern; Profil
gegebenenfalls bewusst vergessen und neu verbinden.

## USB-Befehlsbeispiele

Nur bei bewusst gewünschter Installation; hier keine Ports geöffnet oder Geräte
geflasht. Port/Bootmodus passend zum Board wählen. esptool 5.x:

```sh
# Wemos:
esptool --chip esp8266 --port /dev/ttyUSB0 write-flash 0x0 chaosuhr-2.0.11-esp8266.bin
# ESP32: DESTRUKTIV, alle 4 MiB werden überschrieben:
esptool --chip esp32 --port /dev/ttyUSB0 write-flash 0x0 chaosuhr-2.0.11-esp32-factory.bin
```

Der Factory-Download ist ein vollständiges Image; nicht zusätzlich verschieben.
Enthalten sind Bootloader 0x1000, Partitionstabelle 0x8000, corezugehöriges
boot_app0 0xe000 und Anwendung 0x10000. Diese Offsets werden gegen die tatsächlichen
Build-Metadaten und die binäre Partitionstabelle geprüft.
