# Michas Referenzsketch und ChaosUhr 2.0.10

Verglichen wird der Sketch `ChaosBlinkiBuntiUhr.ino`, Versionsangabe **0.18**, aus [oOmicha1986Oo/ChaosUhr](https://github.com/oOmicha1986Oo/ChaosUhr/blob/51aef401db22ef0fba3e5e334f141bf794d7505a/ChaosBlinkiBuntiUhr.ino), Referenz-Commit `51aef401db22ef0fba3e5e334f141bf794d7505a`, mit der als stabil freigegebenen **2.0.10**. Aussagen beziehen sich auf diesen Quellstand, nicht auf beliebige ältere BINs oder angepasste Fremdversionen.

| Bereich | Referenzsketch 0.18 | ChaosUhr 2.0.10 |
| --- | --- | --- |
| Controller | ESP8266 | Wemos D1 mini / ESP8266 und klassischer ESP32-WROOM-32, getrennte 4-MiB-Builds |
| LED-Matrix | 20×15 WS2812B | Gleiches Matrixformat; Datenpin aus erlaubter GPIO-Liste in der GUI wählbar |
| Animationen | 13 aktive Animationen neben der Uhr | 30 eingebaute Effekte; alle 13 aktiven Originalanimationen und 52 verwendete Originalbilder bleiben erhalten |
| Eigene Motive | Als Bilddaten im Sketch | Bis zu acht geprüfte `.chaos`-Dateien hochladen, abspielen, herunterladen oder löschen; kein Firmwarewechsel pro Motiv |
| Automatik | Uhr und zufällige Animationen | Eigene Auswahl, Zufallsrunden ohne Wiederholung oder Katalogreihenfolge; Kategorien Uhr, Animationen, Wetter und Sensoren separat wählen |
| Uhrzeit und Datum | NTP, deutscher Wochentag und Datum | NTP und deutsche Sommer-/Winterzeit; Wochentag als `Montag` oder `mo`, echte Kleinbuchstaben, schaltbare Sekundenleiste |
| Wetter und Sensoren | Keine hier beschriebene MQTT-Datenanzeige | Wettersymbole und Temperatur sowie vier frei beschriftete Sensorplätze aus HA/ioBroker oder anderen MQTT-Publishern |
| Bedienung | Uploadformular, viele Parameter im Quellcode | GUI für Modus, Helligkeit, Tempo, Szenendauer, Nachtmodus, Ausrichtung, Pin, Anzeigearten und MQTT |
| Bildwiedergabe | Wiederholungs- und Warteaufrufe im Sketch | Zeitgesteuerte Einzelbilder; WLAN und Webserver werden zwischen Bildern bedient |
| Einstellungen | Überwiegend Programmkonstanten | Persistente Daten, Übernahme unterstützter Altformate, Journal, JSON-Sicherung mit Vorschau und getrennte Animationsdateien |
| Anmeldung | Uploadformular ohne diese Admin-/Sitzungsverwaltung | Admin-Ersteinrichtung mit eigenem Passwort; gesalzener Verifier und CSRF-Schutz für Schreibaktionen |
| Updates | Manuelles Firmwareformular | Manueller Upload oder bestätigte GitHub-Installation; Browser lädt per HTTPS, Uhr prüft Zielchip, Produkt, Größe und Integrität |
| WLAN-Einrichtung | Konfigurationsportal | Geschütztes Setup-WLAN mit Code auf der Matrix, IP-Lauftext nach WLAN-Verbindung, bestätigter Werksreset |
| Diagnose | Vor allem serielle Ausgabe | HTTP-Status/API und optionale MQTT-Diagnose mit Home-Assistant-Discovery für Firmware und WLAN-Signal |
| Entwicklung | Arduino-Sketch | PlatformIO mit festgelegten Abhängigkeiten, projektlokalen HTTP-Servern, Host-Tests, getrennten Paketen und Lizenznachweisen |

Die 16 zusätzlichen Effekte sind Matrix-Regen, Regenbogenwellen, prozedurales Feuer, Sternenhimmel, Konfetti, Plasma, blauer Regen, Scanner, Glitzer, Farbringe, Snake-Spur, Meteoriten, Polarlicht, Schachbrett-Puls, Farbbalken und Blasen. BombJack war in der Referenz als Bilddatei vorhanden und ist hier zusätzlich auswählbar. Die Snake-Spur ist ein Effekt, kein spielbares Snake.

Die übernommenen Bildinhalte und Originalzeiten bei Tempo 55 sind lokal gegen die gepinnte Quelle geprüft. Gesamtablauf, Helligkeit und Szenendauer können abweichen. Szenendauer 5–120 Sekunden; ein Szenenwechsel kann einen laufenden Bilderzyklus beenden.

## Wetter und Datenaustausch

Home Assistant, ioBroker oder ein anderer Publisher beschaffen die Wetter-/Sensorwerte. Die Uhr empfängt sie per MQTT; konkrete Topics und Broker werden in der GUI eingerichtet. Dadurch benötigt der Wemos keinen eigenen Wetteranbieter-/HTTPS-Abruf. Diese Fassung bietet Datenempfang und Diagnose, keine MQTT-Fernsteuerung der Einstellungen. [Formate und Einrichtung](MQTT_2_0_10.md).

## Teststand und Grenzen

Die 2.0.10-Builds und isolierte Builds beider ZIP-Quellstände bestehen. Native Tests prüfen unter anderem Speicherung/Migration, Darstellung, MQTT-Payloads, API, GUI und die OTA-Validatoren mit echten BINs und ungültigen Gegenbeispielen. **Die neue 2.0.10 ist noch nicht am Gerät getestet**, insbesondere nicht unter realer MQTT-/Netzwerklast.

Bestätigte Wemos-Gerätetests betreffen den früheren 2.0.8-Release. Nutzerfeedback zur ESP32-2.0.6 bestätigt den älteren 2.0.5-Basisstand. Beides ist keine Bestätigung dieser neuen BINs. [Releasebericht 2.0.10](RELEASE_v2.0.10.md).

Lokales HTTP und MQTT bleiben unverschlüsselt. CRC und SHA-256 ersetzen keine Herausgebersignatur; eine Signaturpflicht ist nicht aktiviert. Persönliche Zugangsdaten werden am Gerät eingerichtet und befinden sich nicht in den öffentlichen BINs. In vollständigen Flash-Dumps einer eingerichteten Uhr sind weiterhin persönliche Daten enthalten.

[Upgrade und Migration](UPGRADE_2_0_10.md), [Eigene Animationen](ANIMATIONEN_FORMAT.md), [Herkunft und Lizenzen](../THIRD_PARTY.md).
