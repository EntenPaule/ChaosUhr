# ChaosUhr 2.0.11 – bekannte Einschränkungen

- Finale Version durch Hosttests und beide Builds geprüft; eine Stable-Veröffentlichung ist keine neue Hardwarebestätigung. Insbesondere ESP32-OTA, USB-Ersteinrichtung und Persistenz nach Stromtrennung sind noch nicht vollständig bestätigt.
- Nutzerfeedback ESP32 Beta.4.2 bestätigt WLAN/MQTT und Online/Offline. Wemos Beta.2 bestätigt MQTT-Speichern, ioBroker IPv4/DNS, Kaltstart/Auto-Reconnect und Wetter; Beta.3 wurde auf Hardware bestätigt. Der finale GUI-/Login-/Helligkeitsstand ist weiterhin visuell und auf beiden Boards zu prüfen.
- Nur Wemos D1 mini und klassischer ESP32-WROOM-32 mit 4 MiB; keine ESP32-S3-/WT32-Displayfirmware. Tatsächliche OTA-Größe zählt; ESP32 hat eine begrenzte Reserve, siehe Release Notes und BUILD_INFO im Repository.
- **ESP32-Factory ist eine destruktive vollständige USB-Neuinstallation ab 0x0. Alle 4 MiB und persönlichen Daten werden überschrieben. Niemals OTA verwenden.**
- LittleFS wird nie automatisch formatiert. FS-001 auf fabrikneuem/leeren Flash ist möglich. Explizite Initialisierung löscht die Dateien der LittleFS-Partition, auch MQTT-/Anzeigeoptionen, Animationen und Journal. NVS-/EEPROM-Zugangsdaten werden dabei nicht pauschal gelöscht.
- Verlorene WLAN-RAM-Daten früherer ESP32-Versionen können nicht rekonstruiert werden. Einmalige Neueinrichtung kann nötig sein. Ein vorheriger Mountfehler beweist nicht automatisch eine beschädigte bestehende Installation.
- USB-Provisionierung nur vor Admin-Einrichtung und fünf Minuten nach Boot; aktuelles Skript und Firmware gemeinsam verwenden. READY/ACK-Hosttests ersetzen keinen seriellen Hardwaretest.
- EEPROM und LittleFS sind getrennte Speicheroperationen. Teilfehler werden angezeigt, keine gemeinsame atomare Transaktion oder automatische Rücknahme. Bei verlorener HTTP-Antwort kann ein Schreibzugriff bereits erfolgt sein. Mehrere Browser/Tabs können weiterhin konkurrierende Änderungen auslösen.
- MQTT verwendet TCP ohne TLS; die Weboberfläche HTTP ohne TLS. Vertrauenswürdiges LAN vorausgesetzt. MQTT-TLS ist auf später verschoben und kein Release-Blocker.
- SHA-256/CRC/Productmarker sind keine verpflichtende Herausgebersignatur; kein neues Rollback-Feature.
- Sichtbares Login ersetzt HTTP Basic. Bestehende API-Skripte benötigen Login-/Cookie-/CSRF-Verarbeitung. Zwei RAM-Sitzungen, 30 Minuten ohne bestätigte Aktivität. AUTH-021 nach Neustart ist erwartbar. Fehler unbekannter Herkunft und sicherheitsrelevante AUTH-Ereignisse werden nach Login nicht pauschal entfernt.
- Geräte- und Browserdiagnose sind begrenzte RAM-Verläufe; Neustart beziehungsweise Neuladen beendet diese Historie. Browserfehler werden nicht nachträglich auf dem Gerät erfunden. Historische Fehler beweisen keinen fortbestehenden Defekt.
- MQTT-Helligkeit nur bei freigegebener Richtlinie und ohne Themenkollision. Retained AUTO-Queueannahme ist keine PUBACK-/Persistenzbestätigung. Änderungen an ioBroker-Zuständen beweisen keine MQTT-Auslieferung; ack-/Publish-/set-Regeln separat prüfen.
- Backupformat 3 importiert alte Formate 1/2; ältere Firmware importiert Format 3 nicht. Zugangsdaten und Animationen sind nicht Bestandteil der JSON-Sicherung. Neue Einstellungen müssen nach Vorschau über die jeweiligen Boxen gespeichert werden.
- Bis zu acht Animationsplätze, aber 192 KiB LittleFS auf ESP32 begrenzen die Gesamtmenge; acht maximal große Uploads passen nicht gleichzeitig. Speicherreserve für Konfiguration beachten.
- GitHub-Release und der separate Branch firmware-updates sind getrennt. Die zunächst fehlende Kanalaktualisierung ist repariert: Stable bietet 2.0.11 an. Preview bleibt Beta.3. Die richtige OTA-BIN ist weiterhin auch manuell installierbar.

[Release Notes](RELEASE_v2.0.11.md) · [Installation](FLASHANLEITUNG_2.0.11.md)
