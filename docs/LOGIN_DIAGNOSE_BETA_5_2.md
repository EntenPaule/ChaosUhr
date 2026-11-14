# Interne 2.0.11-beta.5.2: Login und zentrale Fehlerdarstellung

## Änderungen

Ausgangspunkt `056baf0de8011993f3315cdc7c1ba166b5a38d9c`, Entwicklungsbranch
`fix/2.0.11-diagnostics` im Git-Checkout `.pio/dev-2.0.11`.
Die öffentliche Root-Arbeitskopie ist kein gültiges Git-Repository.

Normales GET `/api/session` ohne Cookie, mit abgelaufener oder unbekannter alter
Sitzung behält HTTP 401 und die bisherigen Fehlercodes/JSON-Felder. Solche
Statusabfragen erzeugen keine neuen AUTH-001/020/021-Ereignisse. Missgebildete
Cookies, Kryptografiefehler und gesperrte Zugangsdaten bleiben echte Fehler.
Die Statusabfrage verlängert weiterhin niemals die Sitzung.

Im Browser führt der Erstaufruf unmittelbar zum Passwortformular, ohne Fehltext
oder Browserdiagnoseeintrag allein aufgrund fehlender Anmeldung. Nach einem
späteren Sitzungsverlust erscheint „Bitte erneut anmelden.“; geschützte Inhalte
werden verborgen, CSRF wird verworfen. Ein neues Login ist ausdrücklich nötig.
Kein Basic-Fallback, keine automatische Wiederholung von Schreibzugriffen.

Erwartbare Ereignisse werden nur bei einer im RAM bekannten abgelaufenen Sitzung
mit vorhandener passender Origin intern markiert. Ein erfolgreicher Login
markiert ausschließlich diese Einträge als erledigt. Der interne Ring mit acht
Einträgen und die serielle Fehlererfassung bleiben erhalten; die sichtbare
Diagnose überspringt erledigte Einträge. `lastError` bezeichnet den letzten noch
sichtbaren Eintrag und entspricht dem letzten Element von `errors`, falls
vorhanden. Das allgemeine Status-API verwendet dieselbe Sicht. Keine zusätzlichen
JSON-Felder; der bestehende Diagnose-API-Vertrag bleibt erhalten.

Die Kennzeichnung nutzt bisheriges Padding im ErrorEvent; sizeof bleibt 12 B.
Zusammenfassung gleicher Fehler berücksichtigt die Kennzeichnung, damit ein
späteres Sicherheitsereignis nicht mit einem erledigten Ereignis verschmilzt.
Keine Flash-Persistenz und kein zusätzliches Ereignisarray.

**Bewusste Grenze:** Bereits vorhandene Einträge unbekannter Herkunft, unbekannte
Sitzungsfingerprints nach Neustart, widerrufene Cookies und verdächtige Zugriffe
werden nicht nachträglich als harmlos eingestuft. Entsprechende Ereignisse bei
Zugriffen auf geschützte APIs bleiben sichtbar. Auch ein erfolgreicher Login
löscht weder AUTH-002 noch Cookie-/CSRF-/Origin-/Ratenlimitfehler oder MQTT-,
WLAN-, HTTP- und Dateisystemfehler. Nach Neustart ohne bekannten Fingerprint ist
eine sichere rückwirkende Unterscheidung technisch nicht möglich. Normales
GET `/api/session` selbst erzeugt in diesem Fall trotzdem keinen neuen Eintrag.

## Darstellung

Außerhalb von „Diagnose“ nur kurze Aktionsmeldungen oder konkrete Eingabehinweise.
Dies gilt für allgemeine Einstellungen, MQTT, Helligkeitsrichtlinie,
Animationsverwaltung, Backup/Import, Werkseinstellungen, Login, Admin-Einrichtung,
WLAN-Portal sowie manuelle und GitHub-OTA-Updates.

Vollständige Codes, HTTP-Status, technische Beschreibungen und Maßnahmen stehen
im Diagnosebereich. Der Browser verwendet ausschließlich den festen lokalen
Fehlerkatalog. Im Browser-RAM werden höchstens acht Einträge aus Code, Status,
Wiederholungszähler und Klassifizierung gehalten; für MQTT-022 zusätzlich nur
der schon erlaubte feste Feldname. Keine Werte, Antwortkörper, URLs, Hostnamen,
Topics, Cookies, Passwörter, CSRF- oder Sitzungstokens. Unbekannte JS-Fehler werden
als SYS_UI dargestellt; ihre rohen Texte gelangen nicht in die Diagnose.

Browserfehler werden von der Gerätehistorie getrennt unter „Browserdiagnose“
gezeigt. Gerätedaten weiterhin über geschütztes `/api/diagnostics`. Browser-
Sitzungsverlustmeldungen aus reinen Sitzungsprüfungen werden nach erfolgreichem
Login gezielt entfernt; unklassifizierte AUTH-Einträge bleiben bestehen.
Der Browserverlauf lebt ausschließlich im jeweiligen Tab/Dokument und geht
beim vollständigen Neuladen verloren. Keine localStorage-/Cookiepersistenz.

Hauptseite: bestehender Diagnosebereich ergänzt. OTA und Admin-Ersteinrichtung:
eigener Bereich „Diagnose“ für den lokalen Browserverlauf. WLAN-Portal: entsprechender
Bereich wird beim Laden eingefügt; der kurze Fehlerhinweis bleibt ein Dialog.
Bei Ausfall von errors.js zeigt der unabhängige Fallback einen kurzen Hinweis
und die technischen Details in einem ausdrücklich beschrifteten Diagnosebereich.
Die Hauptseite verbirgt Geräte-/Supportdaten bei Sitzungsverlust weiterhin.

Für ESP32 werden dieselben Quellen erneut gzip-komprimiert; das bestehende
plattformabhängige LittleFS-Datenverlust-Wording bleibt erhalten.

## Tests und Nachweise

- `tests/test_admin_sessions.cpp`: tatsächliche PublicAccess- und Diagnosehandler
  auf beiden Plattformadaptern; wiederholte stille Erstabfrage, Ablauf/Reboot,
  Login/Logout, gezieltes Ausblenden bei Erhalt interner Ereignisse, konsistente
  lastError/errors, Erhalt von AUTH-002/CSRF, Zugriffsschutz und kompletter
  24-Feld-MQTT-Speichervorgang.
- `tests/test_errors.cpp`: Kennzeichnung, unveränderte Ereignisgröße, Erhalt von
  Sicherheitsereignissen mit gleichem Code und gezielte sichtbare Bereinigung.
- `tests/test_session_ui.js`: Erstaufruf ohne/mit altem Cookie und wiederholte
  Seitenaufrufe, 30-Minuten-Ablauf, AUTH-021, erneuter Login, Schutz vor
  Hintergrundverlängerung, echte Aktivität, kein automatischer Schreibretry.
- `tests/test_central_diagnostics.js`: begrenzter Browserring, Geheimnisschutz,
  kurze Meldungen, vollständige Details nur unter Diagnose, gezielte Bereinigung
  und Prüfung aller vier HTML-Oberflächen.
- Bestehende GUI-Tests für Hauptseite, Einrichtung, MQTT und Backup auf das neue
  Anzeigekonzept angepasst; technische Fehlerobjekte und API-Codes bleiben getestet.
- Ganze Hostsuite mit ASan/UBSan für beide Plattformadapter; ursprüngliche
  Wetter-/Sensor-/MQTT-Wire-/Helligkeits-/OTA-Sicherheitstests weiterhin aktiv.

Zwischenläufe scheiterten an alten GUI-Erwartungen (Codes direkt im Formular,
8-KiB-Detail im Importhinweis) und der bisherigen Erwartung eines AUTH-001-
Historieneintrags bei Statusabfragen. Diese widersprechen dem neuen Auftrag;
Tests wurden gezielt angepasst, Schutzprüfungen nicht entfernt.

Private Belege: `.pio/tests-beta52-complete.log`, `.pio/build-beta52-final.log`,
`.pio/package-beta52-verification.log`, `.pio/verification-beta52-final.json`.
Tatsächliche Firmwaregrößen, Ressourcen, SHA-256, Partitionsoffsets und
OTA-Validatorergebnisse stehen im zugehörigen BUILD_INFO-Testartefakt.

## Hardwaretest noch offen

1. Auf beiden Boards OTA-Datei der passenden Plattform verwenden; hier wurde
   kein Gerät angesprochen und keine Firmware installiert.
2. Neues Browserfenster/mehrfaches Neuladen ohne Cookie und mit altem Cookie:
   Passwortformular, kein neuer normaler AUTH-Historieneintrag.
3. Falsches Passwort: kurzer Hinweis, Fehlversuch weiterhin nach Login in der
   Diagnose sichtbar; Schutzmechanismen unverändert. Keine Passwortwerte teilen.
4. Nach 30 Minuten ohne Aktivität: gesperrte Oberfläche, „Bitte erneut anmelden.“
   Erfolgreich anmelden: nur markierte erwartbare Sitzungsereignisse verschwinden;
   andere Fehler bleiben erhalten. Serielle Sicherheitsprotokolle nicht löschen.
5. Je eine fehlgeschlagene Einstellungs-, Backup- und OTA-Aktion prüfen:
   kurze Rückmeldung im Bereich, vollständige feste Details unter Diagnose.
6. Hauptseite, OTA, Admin-Einrichtung und Portal auf beiden Boards visuell prüfen;
   WLAN/MQTT, Wetter, Sensoren, Helligkeit/Nachtmodus und Animationen auf Regression
   prüfen. Langzeit-Heap und tatsächliche Stackreserve messen.

## Verteilung

Nur interne lokale Teststufe, kein Push, Tag, Release oder Updatekanalwechsel.
Neue Dateien unter `dist/2.0.11-beta.5.2/`; alte Artefakte bleiben unverändert.
Die vollständige ESP32-Factory-BIN ausschließlich USB ab 0x0, niemals OTA.
Sie überschreibt 4 MiB einschließlich Nutzerdaten; bestehende Geräte per OTA
aktualisieren. In diesem Auftrag weder Flashen noch Formatieren ausgeführt.

## Finaler lokaler Prüfstand

70 PASS-Gruppen der vollständigen Hostsuite. Beide tatsächlichen Builds SUCCESS:
Wemos `d1_mini_public`, ESP32 `esp32dev_public`. Frameworks, Bibliotheken,
Partitionierung und produktive Updatekanäle unverändert.

| Ziel | Tatsächliche OTA-BIN | Limit | OTA-Dateireserve | Statisches RAM |
| --- | ---: | ---: | ---: | ---: |
| Wemos | 729088 B | 1044464 B | 315376 B | 51920 / 81920 B |
| ESP32 | 1905024 B | 1966080 B | 61056 B | 66588 / 327680 B |

ESP32 sektorgenau 57344 B Reserve. Statisches RAM ist kein Laufzeit-Heapnachweis.
MQTT-Speicherhandler unverändert 96 B + 272 B vor weiteren Aufrufen.
Gegenüber Beta.5.1 wächst die BIN um 31136 B (Wemos) bzw. 9552 B (ESP32),
hauptsächlich durch den vollständigen festen Browserkatalog für sichere
Diagnosedetails aller Fehlercodes. RAM unverändert.

OTA-Validatoren prüfen echte Dateien einschließlich Mutation, Kürzung, falscher
SHA, Gegenplattform und Factory-Ablehnung. ESP32-Factory-Image wird mit den
tatsächlichen Metadaten und unveränderten Partitionen geprüft; keine erfundenen
Offsets. Gzip-Generatorcheck und tatsächliche komprimierte Arrays im ESP32-BIN
werden geprüft. Bekannte private Secrets und Tokenmuster werden ohne Ausgabe
der Werte gegen Quellen und Artefakte geprüft.

Unverändert zum Ausgangscommit: MQTT-Controller/-Parser/-Inbox, Helligkeits-
steuerung/-Speicherung/-API, Wetter-/Sensorcode, EEPROM-/Credentials-Layouts,
Partitionen und OTA-Validierung. main.cpp enthält ausschließlich geänderte
Weboberfläche und die konsistente sichtbare lastError-Ausgabe.
