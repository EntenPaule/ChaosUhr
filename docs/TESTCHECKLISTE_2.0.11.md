# ChaosUhr 2.0.11 – Hardwarecheckliste

Die Freigabe als Stable ergänzt keine fehlenden Hardwarebestätigungen.
Automatisierte Ergebnisse und tatsächliche Ressourcen: [Release Notes](RELEASE_v2.0.11.md).
Installation: [Flash-Anleitung](FLASHANLEITUNG_2.0.11.md). Keine personenbezogenen
Logs oder Zugangsdaten in Fehlerberichte aufnehmen.

- [ ] Wemos und ESP32: passende OTA-Datei, richtige Version 2.0.11, geschützte APIs.
- [ ] Erster Browseraufruf ohne Sitzung nur Passwortformular; falsches Passwort bleibt Fehler.
- [ ] Login/Logout/30 Minuten Inaktivität, Polling verlängert nicht; nach Neustart erneut anmelden.
- [ ] Anzeige-, Helligkeits-/Nacht- und Matrixbox speichern nur ihre eigenen Felder.
- [ ] Teilfehler eindeutig; kein scheinbarer Erfolg über EEPROM und LittleFS hinweg.
- [ ] Wirksame Helligkeit vom Gerät, Entwürfe bleiben bei Statusabfragen stehen.
- [ ] Nachtabschaltung zeigt wirklich schwarzen Frame einschließlich Sekunden/Sonderanzeigen.
- [ ] MQTT 0/1/5/20/50/100/AUTO, Retained/Reconnect und Offlineverhalten.
- [ ] Wetter und vier Sensoren, Automatik, hochgeladene Animationen und vorhandene Dateien erhalten.
- [ ] Backup 1/2/3, Vorschau ohne Schreibzugriff, danach explizit die jeweiligen Boxen speichern.
- [ ] Kurze Aktionsfehler, vollständige Diagnose, sicherheitsrelevante AUTH-Einträge bleiben erhalten.
- [ ] ESP32: neue UART-Firmware UND Skript, READY → ACK → AP-Login, Fünf-Minuten-Schutz.
- [ ] ESP32: WLAN-/MQTT-Daten nach vollständiger Stromtrennung, automatischer Reconnect.
- [ ] ESP32: tatsächliches OTA, beide Slots und Recovery bei Updatefehlern kontrolliert prüfen.
- [ ] Desktop-/Smartphoneansicht und Langzeit-Heap/Stack auf beiden Boards.

Teilfehler und Updateunterbrechungen nur in einer bewusst vorgesehenen Testumgebung
prüfen; keine produktiven Nutzerdaten beschädigen oder automatisch formatieren.
Nutzerbestätigungen älterer Betas sind in den Release Notes abgegrenzt.
