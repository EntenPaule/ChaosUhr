# ChaosUhr 2.0.11 – lokale Versionsinformationen

Dieser Entwicklungsstand ist **nicht veröffentlicht**. Kein Release, kein neuer Versions-Tag, keine Änderung am produktiven Branch oder an Updatekanälen. Firmware für Wemos D1 mini / ESP8266 und klassischen ESP32-WROOM-32, jeweils 4 MiB Flash und 20×15 WS2812B.

## Korrekturen

- ESP32 startet den WLAN-Treiber mit Flash/NVS-Speicherung. WiFiManager kann neue Zugangsdaten damit dauerhaft speichern; Routerausfälle lösen keine Löschung aus.
- Gespeicherte SSID und verbundener Access Point werden getrennt behandelt. Vorhandene Zugangsdaten behalten die 30-Sekunden-Frist vor dem Recovery-Portal.
- MQTT-Konfiguration meldet konkrete Validierungs- und Dateioperationsfehler. Laufende Konfiguration und bisherige Datei bleiben bei fehlgeschlagenem Speichern erhalten. Fehlgeschlagenes Laden ersetzt nicht die letzte gültige Konfiguration.
- HTTP 401 mit leerer Antwort, abgelaufene Sitzung, Netzwerk-/Timeoutfehler und ungültige Antworten führen zu verständlichen Fehlercodes statt einem leeren Suffix. Eine begrenzte, reine Verwerfung bereits gesendeter Request-Daten erleichtert die Zustellung früher 401-/403-Antworten; es werden dabei keine Felder verarbeitet oder Daten gespeichert.
- MQTT-Verbindungsdiagnose beobachtet die tatsächlichen AsyncTCP-Fehler und Brokerantworten. Wenn nur ein allgemeiner Bibliotheksfehler vorliegt, wird keine DNS-, TCP- oder Passwortursache geraten.
- Zentrale Fehlerverwaltung mit acht RAM-Einträgen, Uptime, Komponente und Wiederholungszähler. Kopierbare Supportdiagnose in der GUI und unter `GET /api/diagnostics`, mit Admin-Anmeldung.

Alle Codes, Ursachen und Maßnahmen: [FEHLERCODES.md](FEHLERCODES.md). Nachgewiesene Ursachen, Implementierung, Änderungen und Testergebnisse: [Entwicklungsbericht](ENTWICKLUNGSBERICHT_2_0_11.md).

## Übernahme vorhandener Daten

Einstellungs-, Admin- und MQTT-Dateiformate sowie Flash-/OTA-Partitionen bleiben unverändert. Kein automatisches Formatieren von LittleFS. Ein vorhandener gesalzener Admin-Zugang wird nicht neu eingerichtet.

Wenn eine frühere ESP32-Firmware das WLAN nur im RAM gehalten hat und die Daten bereits durch Stromtrennung verloren sind, können sie nicht rekonstruiert werden. In diesem Fall ist einmalige erneute WLAN-Einrichtung im geschützten Portal erforderlich. Die korrigierte Firmware wählt den Flash-Speicher vor dem Funkstart, damit neu gespeicherte Daten anschließend erhalten bleiben.

## Testgrenzen

Host-Modelle und simulierte Transport-/Dateifehler ersetzen keine Messung am Gerät. Drei physische ESP32-Kaltstarts, tatsächlicher Routerausfall/Wiederverbindung, MQTT-Speichern und Neustart auf einem Wemos, reale Brokerfehler und Langzeitbetrieb sind noch am passenden Testgerät auszuführen. Aus früherem Nutzerfeedback zu anderen Versionen wird keine neue Hardwarebestätigung abgeleitet.

Lokales HTTP und MQTT bleiben unverschlüsselt. Es wurden weder WLAN-Geräte noch Broker, Home Assistant oder ioBroker verändert.
