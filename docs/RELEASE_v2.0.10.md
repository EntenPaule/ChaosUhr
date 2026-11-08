# ChaosUhr 2.0.10 – Vorabversion

Vorabversion für Wemos D1 mini / ESP8266 und klassischen ESP32-WROOM-32, 4 MiB Flash, 20×15 WS2812B. **Neue BINs noch nicht am Gerät getestet.** Bestehender Wemos-Release 2.0.8 bleibt unverändert. Beide neuen Builds sind über diesen Release verfügbar; der Webseiten-Updater bietet 2.0.10 bei aktivierter Option **Vorabversionen berücksichtigen** an.

## Neue Funktionen

- MQTT-Datenempfang für Home Assistant, ioBroker und andere Publisher: Wetterzustand, Temperatur und vier frei beschriftete Sensorplätze.
- Sonne, Wolken, Regen und weitere Wetterbilder; Temperatur unter dem Symbol. Datenbeschaffung erfolgt auf dem Publisher, ohne direkten Wetteranbieter-/HTTPS-Abruf auf dem Wemos.
- Anzeigearten Uhr, Animationen, Wetter und Sensorwerte unabhängig wählen; auch ausschließlich Uhr/Wetter. Veraltete/fehlende Daten werden übersprungen.
- LED-Datenpin in der GUI auswählen und speichern; Änderung nach Bestätigung mit Neustart. Standard Wemos D6/GPIO12, ESP32 GPIO32.
- Sekundenleiste abschaltbar; Wochentag ausgeschrieben mit echten Kleinbuchstaben oder als `mo`, `di`, … `so`.
- Eigene IP-Adresse nach erfolgreicher WLAN-Verbindung als Lauftext auf der Matrix.
- Gleiches Funktionsangebot und getrennte Firmwarepakete für beide Controller. ESP32 mit LittleFS-Uploads, Passwortmigration, Journal und eigenem GitHub-Updatekanal.
- Einstellungssicherung Schema 2 mit neuen Anzeigeoptionen; alte Schema-1-Sicherungen bleiben lesbar. LED-Pin/MQTT-Verbindungsdaten bleiben gerätespezifisch und werden nicht exportiert.
- Begrenzte MQTT-Nachrichten, atomare Konfiguration, Datenaltergrenze, Diagnose-Discovery für Home Assistant und Dateisystemreserve für Zugang/Einstellungen.

## Prüfung und Grenzen

Lokale Builds beider Controller sowie native Tests für Speicherung, Migration, Wetter-/Sensorformate, MQTT-Nachrichten, tatsächliche API-Funktionen und GUI-Abläufe bestanden. Beide tatsächlich gepackten ZIP-Quellstände wurden isoliert erfolgreich gebaut; die vollständige Host-Suite besteht auch aus dem ZIP. Die produktiven OTA-Validatoren akzeptieren die echten BINs und weisen veränderte, gekürzte, falscher Chip und falsche SHA zurück. Abschlusswerte und Paketprüfungen stehen in `BUILD_INFO.json` / `SHA256SUMS` des jeweiligen Pakets. Build- oder Host-Erfolg ist kein Gerätetest.

| Ressource | Wemos / ESP8266 | Klassischer ESP32 |
| --- | --- | --- |
| Statisches RAM | 53.104 / 81.920 Byte (64,8 %) | 67.004 / 327.680 Byte (20,4 %) |
| Programm-Flash laut Linker | 614.787 / 1.044.464 Byte (58,9 %) | 1.795.639 / 1.966.080 Byte (91,3 %) |
| OTA-Anwendungsdatei | 618.944 Byte | 1.853.040 Byte |
| Reserve bis zur OTA-Dateigrenze | 425.520 Byte, etwa 416 KiB | 113.040 Byte, etwa 110 KiB |

Das ESP32-OTA-Image enthält zusätzliche Abschnitte gegenüber dem Linkerwert; für ein Update zählt die vollständige Dateigröße. Statisches RAM ist keine Messung des freien Heaps im Betrieb. Insbesondere beim Wemos stehen MQTT-/Netzwerk-Lasttests und die dabei verbleibende Heap-Reserve noch aus. Für größere ESP32-Erweiterungen ist der bisherige OTA-Slot bereits knapp.

Die frühere ESP32-2.0.6 auf Basis 2.0.5 läuft laut Nutzerfeedback auf Hardware; daraus folgt keine Bestätigung der neuen 2.0.10-Firmware. Insbesondere LED-Timing auf den neuen Pins, MQTT unter Last, reale Altversion-Migration, Folge-OTA, Portal/Reset und Langzeitbetrieb sind noch am Gerät zu prüfen.

Lokales HTTP und MQTT sind unverschlüsselt. Keine aktivierte Herausgebersignatur; Produktkennung, Format, CRC bzw. Image-SHA und optionaler Kanal-SHA prüfen Integrität und Zielchip. Ein vollständiger Flash-Dump enthält weiterhin persönliche Daten. Keine feste WLAN-/Admin-/Broker-Konfiguration in den öffentlichen BINs.

## Dateien und Anleitung

Wemos: `chaosuhr-2.0.10-esp8266.bin` und ZIP. ESP32: `chaosuhr-2.0.10-esp32-ota.bin`, vollständige `-factory.bin` für USB und ZIP. Pro Paket Prüfsummen, passende Quellen und Lizenzen. ESP32-Factory-Image nicht im OTA-Updater verwenden.

[Upgrade](UPGRADE_2_0_10.md), [MQTT / Wetter / Sensoren](MQTT_2_0_10.md), [Wemos-Anleitung](PUBLIC_FIRMWARE.md), [ESP32-Anleitung](ESP32_FIRMWARE.md).
