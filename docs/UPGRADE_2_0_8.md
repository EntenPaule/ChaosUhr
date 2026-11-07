# Direkt auf ChaosUhr 2.0.8 aktualisieren

Ziel: Wemos D1 mini / ESP8266, 4 MiB Flash, D6/GPIO12, 20×15 WS2812B. Passende Hardware und ausreichend OTA-Platz sind Voraussetzung. Nur Anwendungs-BIN übertragen, kein Dateisystem-/Flash-Abbild.

## Von 2.0.5 oder 2.0.7

1. Einstellungen notieren, eigene .chaos-Dateien herunterladen. Die JSON-Sicherung ist erstmals ab 2.0.8 vorhanden.
2. Im **bisherigen** geschützten Updater `chaosuhr-2.0.8-esp8266.bin` übertragen.
3. Nach Neustart mit **dem bisherigen Admin-Passwort** anmelden. Gültige 20-Byte-Einstellungen aus 2.0.5 und 28-/32-Byte-Daten aus 2.0.7 werden übernommen. Admin-Bereich bleibt ab Offset 32; Flash-/Dateisystemlayout bleibt gleich.
4. Bei lesbarem LittleFS wird dasselbe Passwort mit neuem Salt gehasht und über ein dauerhaftes Journal migriert. Das Passwort ändert sich nicht.
5. Ohne lesbaren Speicher bleibt zunächst das alte Passwortformat nutzbar. Die GUI meldet die ausstehende Migration. Erst nach Sicherung eventuell vorhandener Dateien den Animationsspeicher ausdrücklich einrichten. Danach wird die Migration versucht. Kein automatischer Formatierungs-/Werksreset.

Neu angelegte Admin-Zugänge werden unmittelbar gehasht. Die zusätzliche Journalabsicherung benötigt lesbares LittleFS. Bei vollem/defektem Speicher wird keine alte gültige Anmeldung still gelöscht.

## Von Michas Originalversion

Geprüfte Referenz: Sketch **0.18**, Commit `51aef401db22ef0fba3e5e334f141bf794d7505a`. Angepasste Fremdversionen können abweichen.

1. Neue BIN über das vorhandene Formular übertragen. Das Original verwendet dabei Feld `update`; dessen Oberfläche erledigt diesen Feldnamen. Ein Zwischenschritt über 2.0.5 ist nicht vorgesehen.
2. Beim normalen Start werden SDK-WLAN-Daten nicht absichtlich gelöscht. Ihre tatsächliche Übernahme über verschiedene alte SDK-Versionen muss am jeweiligen Gerät bestätigt werden.
3. Das Original besitzt keinen passenden Public-Admin-Datensatz: SETUP-Code ablesen, mit dem geschützten Einrichtungs-WLAN verbinden und erstmals ein Admin-Passwort setzen.
4. Danach vorhandenes Heim-WLAN weiterverwenden lassen oder im Portal neu einrichten. Im alten Programm festgelegte Anzeige-/Helligkeitswerte sind nicht automatisch auslesbar; die neuen Einstellungen in der GUI anpassen.

Ein leerer EEPROM-Bereich wurde lokal als Ausgangspunkt getestet. Unbekannte oder beschädigte Credentials werden nicht als leer behandelt. Keine pauschale Zusage für beliebig modifizierte Altfirmware oder fremde EEPROM-Belegungen.

## Folgeupdates und Rückweg

2.0.8 verwendet weiter `/update` und Feld `firmware`, jetzt mit Anmeldung und CSRF-Token. Schreibende API-Clients benötigen `GET /api/session` und `X-CSRF-Token`; Einstellungsparameter bleiben gleich.

Nach Hash-Migration sind die unveränderten alten 2.0.5/2.0.7 **kein geeigneter Downgrade**: Sie verstehen Format 2 nicht. Der neue Updater weist alte BINs ohne Produktkennung deshalb ab. Eine Recovery-Firmware muss beide Credentials-Formate lesen, dasselbe Speicherlayout verwenden und weiterhin OTA anbieten.

**Auf einem Wemos getestet:** 2.0.7 → 2.0.8 einschließlich Passwortmigration und Übernahme aller 15 Einstellungen, danach Folge-OTA über den neuen CSRF-/SHA-geschützten Updater. Anzeige und Weboberfläche vor Ort bestätigt. 2.0.5 und Michas Original wurden nicht zusätzlich auf echter Hardware installiert; hierfür liegen Quell-/Fixture-Tests vor. Keine Tests mit physischem Stromausfall, Werksreset oder WLAN-Neueinrichtung. [Genauer Testumfang](RELEASE_v2.0.8.md).
