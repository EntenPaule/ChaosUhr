# ChaosUhr 2.0.11-beta.2 – Wemos-Test

Ausschließlich Wemos D1 mini / ESP8266, 4 MiB Flash, bestehendes 1-MiB-LittleFS-Layout. Kein ESP32-Build im Paket. Die Stackkopien im MQTT-Speicherpfad wurden reduziert; die genaue Ursache des beobachteten Hardware-Resets ist noch zu bestätigen.

## Installation

1. Uhreinstellungen sichern, eigene Animationen separat herunterladen; vorhandene MQTT-Zugangsdaten privat bereithalten. Stabile Stromversorgung verwenden.
2. `sha256sum -c SHA256SUMS_2.0.11-beta.2.txt` prüft die Dateien. Die externe Liste umfasst zusätzlich das ZIP, die ZIP-interne Liste seinen Inhalt.
3. Als Admin auf der Uhr anmelden, OTA-Seite öffnen, **ChaosUhr_2.0.11-beta.2_Wemos_OTA.bin** auswählen. Keine ZIP-Datei hochladen. Neustart abwarten und neu anmelden; alte Sitzung wird absichtlich ungültig.
4. GUI und Supportdiagnose müssen **2.0.11-beta.2** melden. Kein Werksreset, kein Full-Erase, keine Dateisystemformatierung.
5. Falls USB benötigt wird: dieselbe Wemos-BIN mit einem eingerichteten ESP8266-Flashwerkzeug bei Offset **0x0** schreiben; seriellen Port vor Ort bestimmen. Kein zusätzliches Dateisystem-Image. USB und OTA dieser Beta sind noch nicht am Gerät bestätigt.

## Vorrangiger MQTT-Test

- [ ] Admin anmelden; einmal „Supportdiagnose laden“ und Ausgangs-Uptime/Version notieren.
- [ ] Minimalfelder setzen: eigener Broker, korrekter Port, Discovery aus, Präfix `homeassistant`, keine optionalen Wetter-/Sensortopics. Ein leeres Passwortfeld behält ein zuvor gespeichertes Passwort; zum bewussten Löschen dient die vorhandene Checkbox.
- [ ] Einmal speichern. Bleibt die Uhr sichtbar in Betrieb? Bei Erfolg: Seite neu laden, `mqttEnabled`/Einstellungen getrennt prüfen.
- [ ] Bei sichtbarem Reset: erneut anmelden, **vor einem weiteren Speicherversuch** Supportdiagnose laden. `reset.reason`, `reasonCode`, `crashCallbackValid`, ggf. `crashReason`, `previousMqttValid` und `previousMqtt` prüfen. Diese bereinigten Daten sind ohne Passwörter/Topics/Sitzungstokens gestaltet.
- [ ] Bei normaler HTTP-Ablehnung ohne Reset: Fehlercode, HTTP-Status und MQTT-022-Feldkennung notieren. Keine vollständige Browseranfrage/Headers teilen.
- [ ] Stromneustart separat: WLAN, MQTT-Daten, Uhreinstellungen und Animationen erhalten?
- [ ] Erst anschließend tatsächliche Brokerverbindung und Wetter-/Sensorempfang prüfen. DNS/TCP/Broker-Anmeldung sind kein Nachweis für die vorherige Dateispeicherung und umgekehrt.

`previousMqtt` ist nur ein letzter Kontrollpunkt, kein automatischer Ursachenbeweis. `stackHighWaterFreeBytes` ist die Stack-Watermark seit Boot. „power-on-or-unclassified“ beweist keinen Spannungsabfall. Keine Schutzfunktion deaktivieren und nicht formatieren, um den Test zu erzwingen.

## Serielle Aufzeichnung bei erneutem Reset

- USB nur vor Ort anschließen. Für einen vollständigen Crashdump den seriellen Monitor **vor** dem Speichern mit 115200 Baud starten. Das Öffnen eines USB-Seriell-Adapters kann selbst einen Reset auslösen; diesen vom anschließenden MQTT-Test unterscheiden. ROM-Boottext kann mit anderer Baudrate erscheinen.
- Die kurze `[RESET]`-Zeile der Anwendung enthält nur feste Gründe, Zahlen und Codeadresse. Der unveränderte Core kann zusätzlich einen vollständigen Stackdump ausgeben. **Rohdump privat halten: darin können Passwörter stehen.** Nicht in Discord/Git/Chat veröffentlichen.
- In der lokalen Entwicklungsumgebung mit dem exakt passenden archivierten ELF auswerten:

```sh
python3 scripts/decode_reset_trace.py --log .pio/privater-crash.log --elf .pio/debug/2.0.11-beta.2/firmware.elf
```

Das Werkzeug gibt nur symbolisierte ausführbare Adresskandidaten aus; keine Roh-Stackwörter. Bei unbekanntem/zerstörtem Stack ist keine vollständige Aufrufkette garantiert. Zunächst reichen die bereinigte Supportdiagnose und die `[RESET]`-Zeile.

## Weitere offene Hardwareprüfungen

- [ ] Login/Logout/erneute Anmeldung, 30 Minuten Inaktivität ohne Verlängerung durch Status-GET.
- [ ] AUTH-021 nach Neustart/widerrufener Sitzung; AUTH-001 ohne Cookie, AUTH-020 bei Ablauf, CSRF-Schutz unverändert.
- [ ] MQTT-Save mit gültiger Sitzung, Ablehnung mit ungültiger Sitzung, kein Verlust vorhandener Daten.
- [ ] WLAN-Recovery, Animationen/Datum/Uhrzeit, nachfolgendes OTA und Langzeit-/Heapverhalten.

HTTP bietet weiterhin keine TLS-Vertraulichkeit. Die Uhr nicht ins Internet weiterleiten. Keine offizielle Veröffentlichung dieser Beta; produktiver Updatekanal unverändert. Für neue verteilte Codeänderungen folgt eine neue Beta-Nummer.
