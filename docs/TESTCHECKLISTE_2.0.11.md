> Historischer unnummerierter 2.0.11-Teststand. Aktuell: [2.0.11-beta.1](BERICHT_2.0.11-beta.1.md); dessen Testumfang und ausschließliches Wemos-Paket sind maßgeblich. Kein offizielles 2.0.11-Release.

# ChaosUhr 2.0.11 – Checkliste für Tester

Bitte Boardmodell, vorherige Version, Installationsart (OTA/USB), Version nach dem Start und Ergebnisse notieren. Passwörter, WLAN-Namen, Brokeradressen und Tokens nicht öffentlich teilen. Für Fehlermeldungen die bereinigte Supportdiagnose aus der GUI kopieren.

## Installation und Anzeige

- [ ] Richtige Datei und SHA-256 geprüft; ESP32-Factory-BIN ausschließlich per USB verwendet.
- [ ] GUI meldet 2.0.11; Uhrzeit, deutscher Wochentag/Datum und gewählte Animationen funktionieren.
- [ ] Bei OTA sind Einstellungen und vorhandene eigene Animationen erhalten; bei Factory-Neuinstallation wurde die Einrichtung erfolgreich abgeschlossen.
- [ ] GUI ist nach Neustart wieder erreichbar; IP-Anzeige und gewählter LED-Pin passen.

## WLAN und Neustarts

- [ ] WLAN speichern und normalen Neustart ausführen: automatische Verbindung ohne neue Eingabe.
- [ ] **ESP32 dreimal vollständig stromlos machen**, jedes Mal wieder verbinden und GUI prüfen. Ergebnisse für Start 1, 2 und 3 getrennt notieren.
- [ ] Mit einem dafür freigegebenen Test-WLAN dessen vorübergehende Nichterreichbarkeit prüfen: bei vorhandener gespeicherter SSID mindestens etwa 30 Sekunden Wiederverbindungsversuch vor dem Recovery-Portal.
- [ ] Test-WLAN wieder bereitstellen: Wiederverbindung prüfen. Gespeicherte Daten dürfen durch die Unterbrechung nicht verschwinden.
- [ ] Ohne gespeichertes WLAN ist Einrichtung möglich; dies wird von einem nicht erreichbaren Router unterschieden. Kein Werksreset für den Ausfalltest erforderlich.

## Admin-Anmeldung und Sitzungen

- [ ] Auf Hauptseite und `/update` steht oben rechts der Anmeldestatus. Falsches Passwort wird verständlich abgewiesen, richtiges Passwort lädt die geschützten Daten.
- [ ] **Abmelden** verwenden: danach sind Speichern/Upload/OTA ohne erneute Anmeldung gesperrt. Ein alter Tab und zuvor gespeicherte Basic-Anmeldedaten dürfen keine automatische Neuanmeldung bewirken.
- [ ] Erneut bewusst anmelden; Einstellungen und MQTT sind wieder bedienbar. Zweiter Browser darf separat angemeldet sein; Logout betrifft die eigene Sitzung einschließlich ihrer Tabs.
- [ ] Seite mindestens 30 Minuten ohne Bedienung geöffnet lassen, Statusabfragen dürfen weiterlaufen: danach Login erforderlich, API meldet `AUTH-020` oder bei inzwischen unbekanntem Cookie `AUTH-021`.
- [ ] Während längerer tatsächlicher Bedienung bleibt die Sitzung aktiv. Reine Hintergrundabfragen und künstliche Eingabeereignisse zählen nicht.
- [ ] Uhr neu starten: alte Sitzung wird abgewiesen, Passwort bleibt gültig, bewusster Login funktioniert erneut.
- [ ] In einem geeigneten lokalen API-Test ungültigen Cookie und ungültiges beziehungsweise fremdes CSRF prüfen: 401 mit `AUTH-xxx` für die Sitzung, 403 mit `AUTH-003` für CSRF; keine Schreibzugriffe.
- [ ] Supportdiagnose und serielle Fehlerausgabe enthalten weder Passwort noch Cookie-/CSRF-/Loginwerte. Keine unbereinigten Netzwerk-Mitschnitte mit diesen Werten veröffentlichen.

## MQTT-Speicherung und Betrieb

- [ ] MQTT in der GUI mit eigenem Testbroker konfigurieren und speichern. Eine erfolgreiche Speicherung wird getrennt vom Brokerstatus angezeigt.
- [ ] Seite neu laden, Konfiguration kontrollieren, ohne neue Passworteingabe erneut speichern und danach Uhr neu starten: Konfiguration bleibt vorhanden; gespeichertes Passwort wird nicht zurückgelesen.
- [ ] MQTT mit gültiger Sitzung speichern: Erfolg, kein unbegründetes 403. Nach Logout/Ablauf dieselbe Speicherung abgewiesen und bisherige Datei/Einstellungen erhalten. Nach erneuter Anmeldung wieder erfolgreich.
- [ ] Wetter-/Sensor-Testdaten aus HA oder ioBroker senden: richtige Anzeige und Übergänge zwischen Uhr/Wetter/Sensoren/Animationen prüfen.
- [ ] An einer Testkonfiguration DNS-Fehler, geschlossenen TCP-Port, falsche Brokeranmeldung und bestehende Verbindungsunterbrechung unterscheiden. Timeout nur mit geeignetem Testaufbau prüfen.
- [ ] Fehlercode entspricht der tatsächlich erkennbaren Ursache. Liefert die Bibliothek keine genauere Ursache, ist ein allgemeiner Code zulässig; es darf kein Passwortfehler geraten werden.
- [ ] Nach Korrektur Verbindung wiederhergestellt; bereinigter Fehlerverlauf darf ältere Ereignisse weiterhin enthalten.

Dateisystem-Schreib-/Rename-/Lesefehler sind bereits im Hosttest injiziert. Auf dem echten Gerät keinen vorhandenen Animationsspeicher formatieren oder absichtlich beschädigen, um solche Fehler zu erzeugen.

## Fehlercodes und Diagnose

- [ ] Ohne Anmeldung liefert beispielsweise `curl -i http://UHR-IP/api/diagnostics` HTTP 401 mit nicht leerem JSON und ChaosUhr-Code. `UHR-IP` durch die eigene Uhradresse ersetzen; keine Passwörter in Befehlszeilen posten.
- [ ] GUI zeigt bei Fehlern Code, deutsche Beschreibung, technische Ursache soweit erkennbar und Maßnahme; kein leerer Grund hinter „MQTT konnte nicht gespeichert werden:“.
- [ ] Bereinigte Supportdiagnose lässt sich laden/kopieren: Version, Plattform, Uptime, letzter Fehler und höchstens acht Ereignisse.
- [ ] GUI/API/serielle Diagnose verwenden bei derselben geräteseitig erkannten Ursache denselben Code. Reine Browserfehler stehen nicht automatisch im Geräteverlauf.
- [ ] Supportdiagnose und serielle Fehlertexte enthalten keine Passwörter, Tokens, SSIDs, Brokerbenutzer oder Topics.

## OTA und Dauerbetrieb

- [ ] Nach dem ersten erfolgreichen Start dieselbe passende OTA-BIN nochmals über den lokalen Upload installieren; Neustart und Erhalt der Einstellungen/Animationen prüfen.
- [ ] Falsches Chipimage und Factory-Image werden bei OTA abgelehnt. Bei solchen Ablehnungstests nur die geprüfte GUI verwenden und Stromversorgung erhalten.
- [ ] Mehrere Stunden mit WLAN, MQTT, GUI und Animationen laufen lassen; unerwartete Neustarts, Anzeigeaussetzer und freien Heap dokumentieren.

Keine Hardwareprüfung ist durch die Hosttests vorweggenommen. Die lokalen Build-/Validatorergebnisse stehen in `BUILD_INFO_2.0.11.json`; bekannte Grenzen in `BEKANNTE_EINSCHRAENKUNGEN_2.0.11.md`, alle Codes in `FEHLERCODES_2.0.11.md`.
