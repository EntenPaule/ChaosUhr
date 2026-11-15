# ChaosUhr 2.0.11

Stable-Release für Wemos D1 mini / ESP8266 und klassischen ESP32-WROOM-32
(`esp32dev`), jeweils 4 MiB Flash und 20×15 WS2812B. Gemeinsamer Quellstand,
getrennte passende Firmwaredateien. Finale Änderung gegenüber Beta.5.3:
Versionskennzeichnung und Release-Dokumentation, keine neuen Funktionen.

## Änderungen seit 2.0.10

- **Vollständige LED-Nachtabschaltung:** Expliziter schwarzer Frame, auch
  Sekundenleiste und Sonderanzeigen dunkel. Netzwerk, MQTT und Automatik laufen
  weiter. Gespeicherte Tag-/Nachthelligkeit bleibt erhalten.
- **MQTT-Helligkeit als Opt-in:** `<Basis>/brightness/set` akzeptiert ASCII
  `0`–`100` und exakt `AUTO`. Retained-Werte werden angenommen; Nachtabschaltung
  hat Vorrang. Vorgaben liegen im RAM, ohne Flash-Schreiben pro Nachricht.
  „MQTT-Vorgabe aufheben“ sendet retained AUTO; Queueannahme ist kein Nachweis
  einer Brokerbestätigung oder dauerhaften Speicherung. Themenkollisionen
  sperren die Steuerung, ohne Wetter-/Sensorverarbeitung zu deaktivieren.
- **Übersichtliche Weboberfläche:** Eigene Boxen für Anzeige/Animationen,
  Helligkeit/Nachtmodus und Matrixverdrahtung. Helligkeitsspeichern bestätigt
  EEPROM vor LittleFS; Teilerfolge und unbestätigte Antworten werden ausdrücklich
  gemeldet. Keine vorgetäuschte gemeinsame Transaktion.
- **Admin-Sitzungen:** Formularlogin, HttpOnly-/SameSite-Strict-Cookie, echter
  serverseitiger Logout, 30 Minuten Inaktivität ohne Verlängerung durch
  Hintergrundabfragen. Neustart invalidiert alte Sitzungen/CSRF. Kein Basic-
  Fallback. Normale initiale Sitzungsprüfung erzeugt keine AUTH-001/020/021-
  Historieneinträge. Falsches Passwort und Sicherheitsereignisse bleiben erfasst.
- **Zentrale Diagnose:** Kurze Aktionsmeldungen, feste technische Details nur
  unter Diagnose. Acht Ereignisse im Geräte-RAM sowie begrenzte Browserdiagnose
  ohne Passwort-/Cookie-/Token-/Antwortkörperprotokollierung. Nur gekennzeichnete
  erwartbare Sitzungsereignisse werden nach Login aus der sichtbaren Diagnose
  ausgeblendet; intern und seriell bleiben sie erhalten.
- **Backupformat 3:** Nachtabschaltung/MQTT-Helligkeitsfreigabe zusätzlich zu den
  Uhreinstellungen. Import alter Formate 1/2 bleibt möglich. Keine Passwörter,
  MQTT-Konfiguration, WLAN-Daten oder LED-Pins im Export. Animationen separat sichern.
- **MQTT-/Speicheroptimierung:** Kleinerer MQTT-Speicherstack auf dem Wemos,
  präzisere Konfigurationsfehler, atomare geprüfte Dateischreibvorgänge. Ein
  zulässiger leerer MQTT-PUBLISH erzeugt keinen falschen MQTT-016 mehr. Dies
  beweist nicht, dass ein ioBroker-Datenpunkt tatsächlich an Clients publiziert;
  Broker-Publish-Maske, ack und /set-Abbildung sind getrennt zu prüfen.
- **OTA-Prüfungen:** Zielchip, Produktkennung/Layout, Dateityp, Größe und
  Integrität werden geprüft; Online-OTA zusätzlich gegen SHA-256. ESP32-Factory
  und Firmware der Gegenplattform werden als OTA abgewiesen.
- **ESP32:** Unverändertes 4-MiB-OTA-Layout, komprimierte Webressourcen,
  lesende LittleFS-Diagnose und ausdrücklich bestätigte Initialisierung.
  Begrenztes UART-Setup mit READY/ACK, Bootwartezeit und Retry, ausschließlich
  vor Admin-Einrichtung im Fünf-Minuten-Fenster. Keine automatische Formatierung.

## Downloads und Installation

| Release-Datei | Verwendung |
| --- | --- |
| `chaosuhr-2.0.11-esp8266.bin` | Wemos D1 mini: OTA; USB bei 0x0 ebenfalls möglich. |
| `chaosuhr-2.0.11-esp32-ota.bin` | Klassischer ESP32: OTA-Anwendungsimage für kompatibles ChaosUhr-Layout. |
| `chaosuhr-2.0.11-esp32-factory.bin` | **Destruktive USB-Neuinstallation ab 0x0. Überschreibt alle 4 MiB einschließlich WLAN, Admin, Einstellungen und Animationen. Niemals OTA.** |
| `SHA256SUMS.txt` | Prüfsummen der drei BINs. |

Vor einem Update Einstellungen als JSON und eigene `.chaos`-Dateien sichern.
Passende OTA-BIN über `/update` manuell auswählen. Bestehende kompatible
Einstellungen, Admin-/WLAN-/MQTT-Daten und Animationen bleiben erhalten;
EEPROM-/Credentials-/MQTT-Layouts und Partitionierung wurden nicht geändert.
Keinen Werksreset und keine LittleFS-Initialisierung für ein normales Update auslösen.

Nach Update erneut mit dem bestehenden Admin-Passwort im Formular anmelden.
API-Skripte mit HTTP Basic benötigen die vorhandenen Login-/Cookie-/CSRF-Routen.
Neue Backups benötigen einen Format-3-Importer; alte Firmware liest Format 3 nicht.
Bei Wiederherstellung zunächst Vorschau prüfen, dann die betreffenden Boxen speichern.

Quellen, Tests und Dokumentation: Tag `v2.0.11` im Hauptbranch `chaosuhr-2.0`.
[Flash-Anleitung](FLASHANLEITUNG_2.0.11.md),
[bekannte Einschränkungen](BEKANNTE_EINSCHRAENKUNGEN_2.0.11.md),
[Fehlercodes](FEHLERCODES.md), [Sitzungen](SITZUNGEN_2_0_11.md).
Historische Testpakete mit unnummeriertem 2.0.11 unter `dist/2.0.11/` sind kein
Stable-Release; ausschließlich die Assets dieses GitHub-Releases verwenden.

## Verifikation und Hardwaregrenzen

Vor Veröffentlichung: vollständige Hostsuite, beide vollständig neu gebauten
PlatformIO-Profile, tatsächliche OTA-Dateivalidierung mit Gegenplattform-/Factory-
und Manipulationsfällen, Factory-Komponenten/Partitionstabelle/MD5/FF-Bereiche,
Gzip-/GUI-Quellgleichheit und Version sowie Source-/Historien-/Artefakt-Secretscan.
**71 Host-PASS-Gruppen**, beide Clean-Builds erfolgreich (ESP8266-Plattform 4.2.1 /
Arduino 3.1.2; ESP32-Plattform 8.0.0 / Arduino 3.3.12).

| Ziel | Tatsächliche OTA-BIN | OTA-Limit | Reserve | Statisches RAM |
| --- | ---: | ---: | ---: | ---: |
| Wemos | 733264 B | 1044464 B | 311200 B | 51904 / 81920 B |
| ESP32 | 1906480 B | 1966080 B | 59600 B | 66588 / 327680 B |

ESP32 sektorgenau 57344 B Reserve. Linker-Flash: Wemos 729115 B,
ESP32 1844443 B. Statisches RAM ist keine Laufzeit-Heapmessung.
MQTT-Speicherstack auf Wemos unverändert 96+272=368 B vor weiteren Aufrufen.
Factory-Datei 4194304 B, USB ab 0x0; Bootloader 0x1000, Partitionstabelle
0x8000, boot_app0 0xe000, Anwendung 0x10000. Zwei Appslots zu 1966080 B,
LittleFS 192 KiB; Offsets und Inhalte gegen tatsächliche Build-Metadaten geprüft.

SHA-256 der Release-Dateien:

```text
91ca2e0467a426fc21663f4973ca7b9066da3a72e8aafd78a71ca5aee6530083  chaosuhr-2.0.11-esp8266.bin
102cdef378373e6aa93687f801319a156d167f1d841d14e6cf8ef1cf37d95c15  chaosuhr-2.0.11-esp32-ota.bin
1cc0d2b2cf7f69f7a7217dedc6e5b1ca71eb547e05ce72d2761ceaff9792e2f2  chaosuhr-2.0.11-esp32-factory.bin
```

Das lokale Verpackungsskript prüft die realen Dateien vor Veröffentlichung erneut.
Buildinformationen im Repository: `dist/2.0.11-stable/BUILD_INFO.json`;
die vier Release-Assets liegen dort im Unterordner `release/`.

**ESP32-Hardwaretests für OTA, USB-Ersteinrichtung und Persistenz sind noch nicht
vollständig bestätigt.** Nutzerbestätigung für Beta.4.2: WLAN/MQTT und MQTT
Online/Offline. Das ist keine vollständige Prüfung der finalen 2.0.11.
Die UART-Bootrace wurde mit Hostsimulation nachgewiesen; der ursprüngliche
physische Reset beim Portöffnen wurde nicht aufgezeichnet.

Wemos Beta.2: MQTT-Speichern ohne Neustart, ioBroker IPv4/DNS, 30 Sekunden
Stromtrennung mit erhaltenen WLAN-/MQTT-Daten, Auto-Reconnect und Wetter bestätigt.
Wemos Beta.3: vom Nutzer als auf Hardware getestet bestätigt. Der finale
Boxen-/Login-/Helligkeitsstand benötigt weiterhin visuelle/Hardware-/Langzeittests.
Der frühere Stacküberlauf bleibt eine sehr wahrscheinliche, nicht endgültig
bewiesene Absturzursache. AUTH-021 nach Neustart ist erwartetes Sitzungsverhalten.

Keine Hardwarezugriffe oder Flashvorgänge für diese Veröffentlichung.
Lokales HTTP und MQTT sind unverschlüsselt; vertrauenswürdiges LAN verwenden.
SHA-256/CRC sind keine Herausgebersignatur. MQTT-TLS ist auf eine spätere Version
verschoben. Kein Rollback-Feature. Die zunächst separat auf 2.0.10 verbliebenen Manifeste im Branch
`firmware-updates` wurden anschließend auf die unveränderten 2.0.11-OTA-Assets
aktualisiert und öffentlich mit beiden Firmware-Updatern geprüft. Preview bleibt
Beta.3. Der verbindliche Releaseabschluss umfasst künftig immer beide Stable-Kanäle.
