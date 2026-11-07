# Sicherheit in ChaosUhr 2.0.8

Erste Härtungsstufe für Wemos nach dem Audit von 2.0.7. Als Vorabversion nach Teil-Gerätetest veröffentlicht; kein automatischer Upload durch Build oder Paketierung.

## Umgesetzt

- Projektlokaler HTTP-Server aus ESP8266-Core 3.1.2: Header-/Body-/Boundary-/Feld-/Argumentgrenzen, feste Boundary-Puffer, Zeit-/Abbruchgrenzen und sichere Argumentzählung. Quellen und LGPL-Lizenz sind enthalten; globale Toolchains bleiben unverändert.
- Zentrale Authentifizierungs-/CSRF-Prüfung bereits vor Body-Verarbeitung; auch Einstellungen und OTA sind geschützt. Vorhandener Origin muss zum Host passen; fehlender Origin befreit nicht vom Token. AP-Ersteinrichtung hat eigene Zugriffsregeln.
- WLAN-Portal nur über AP, versteckte OTA-/Neustart-/Lösch-/Parameterwege gesperrt; WLAN-Speichern und Portal-Ende per POST mit Token. Einrichtung bleibt möglich.
- Keine allgemeine CORS-Freigabe; no-store, nosniff und Frame-Schutz.
- Gesammelte Header ausdrücklich mit leerem String überschreiben: ESP8266 `String::clear()` setzt die Länge zurück, lässt aber den alten C-Puffer stehen. So blockiert ein abgewiesener Origin keine späteren Anfragen ohne Origin. Regression lokal und am Gerät geprüft.
- Zentraler begrenzter Basic-Decoder, konstantzeitlicher Verifier-Vergleich, fünf Fehlversuche je fünf Sekunden und 60-Sekunden-Erfolgs-Cache als HMAC mit zufälligem Bootschlüssel. Kein dauerhaftes Klartextpasswort im Access-/Updater-Objekt nach erfolgreicher Migration.
- PBKDF2-HMAC-SHA-256 mit 16 Salt-Bytes und 32 Verifier-Bytes. Format 1 bleibt lesbar; neue Daten verwenden Format 2. Credentials bleiben 88 Bytes ab Offset 32, Einstellungen 0–31, Gesamtpuffer 128 Bytes.
- Vollständiges neues EEPROM-Abbild zuerst geprüft und atomar in LittleFS speichern, danach EEPROM spiegeln. Das Journal bleibt als unabhängige Wiederherstellungsquelle erhalten. Bereits journalgesicherte Daten gehen bei anschließendem EEPROM-Schreibfehler nicht verloren.
- Fehlendes LittleFS: alte gültige Anmeldung erhalten und ausstehende Migration anzeigen. Kein automatisches Formatieren. Nach ausdrücklicher Speicherinitialisierung Migration versuchen. Unbekannte/defekte Credentials öffnen keine neue Admin-Registrierung.
- OTA aktiviert erst nach vollständigem Requestabschluss: genau eine Datei, Image-/Größen-/Core-CRC-/Längen-/Produktprüfung, kein rohes Dateisystem/GZIP, klare Fehler-/Abbruchzustände und Neustart nur nach Erfolg.
- Online-Update lädt im Browser über HTTPS aus fester GitHub-Quelle, ohne externe Admin-/Sitzungsheader. SHA-256 wird zusätzlich auf der Uhr geprüft; ausdrückliche Installation statt Hintergrund-Flashen.
- Einstellungen-Export nur erlaubter Felder, Import mit Schema-/Größen-/Werteprüfung und Vorschau. Upload-Zuordnung anhand des tatsächlichen Dateifingerabdrucks. Keine Zugangsdaten im Export.
- Ignore-/Release-Prüfungen berücksichtigen historische/aktuelle Passwörter, lokale GitHub-Zugangsdaten sowie Schlüssel-/Tokenmuster, ohne Werte auszugeben.

## Grenzen

**HTTP zur lokalen Uhr bleibt unverschlüsselt.** Ein Passwort-Hash schützt nicht vor mitgelesenem Basic Auth. Oberfläche nur im vertrauenswürdigen LAN, nicht offen ins Internet.

**Keine verpflichtende Herausgebersignatur.** CRC, Produktkennung und SHA-256 aus dem HTTPS-Kanal prüfen Fehler und Kompatibilität. Sie schützen nicht wie eine Signaturkette vor Kontrolle über Admin-Zugang, lokale HTTP-Verbindung oder Updatekanal. Signaturpflicht benötigt vor Aktivierung Schlüsselpolitik, Sicherung und erprobte signierte Folge-/Recovery-Updates. Geräteeigenes HTTPS ist ebenfalls nicht aktiviert.

**10.000 PBKDF2-Iterationen sind ein vorläufiger ESP8266-Kompromiss**, unter der allgemeinen OWASP-Empfehlung von 600.000 für HMAC-SHA-256. Erste gültige HTTP-Anmeldungen nach Neustart benötigten im Kurztest etwa 2,5–3,4 Sekunden einschließlich Netzwerk; eine falsche Anmeldung etwa 3,3 Sekunden. Danach reduziert der 60-Sekunden-Erfolgs-Cache den Aufwand. Das ist keine isolierte KDF-Messung. Während der synchronen Prüfung kann die Darstellung kurz pausieren. Watchdog-Aufrufe jeweils nach 32 Iterationen; native Tests gegen OpenSSL. Langzeitverhalten bleibt offen.

AP-Code und SDK-WLAN-Daten bleiben funktionsbedingt im Flash. Keine allgemeine Flash-Verschlüsselung, kein Secure-Boot- oder A/B-Rollback-Versprechen. Ein Flash-Dump kann weiterhin persönliche Daten enthalten.

Journal/Recovery wurden mit partiell gelöschten/geschriebenen EEPROM-Sektoren und Dateifehlern lokal geprüft, nicht unter echten physischen Strom-/Flashausfällen. Bei beschädigtem Journal können Schreibzugriffe gesperrt werden; vorhandene gültige EEPROM-Zugangsdaten und OTA werden dadurch nicht bewusst gelöscht. Absolute Recovery unter beliebigen Hardwarefehlern wird nicht behauptet.

## Tests und Freigabe

`python3 tests/run_host_tests.py` prüft den tatsächlichen HTTP-Parser unter Sanitizern, KDF/Basic, ein mit Original-2.0.5-Code erzeugtes fiktives EEPROM-Abbild, alle 128 partiellen EEPROM-Schreibpositionen, Journal-/Resetfehler, OTA-Abbrüche und fehlerhafte Images, Auth/CSRF und echte GUI-Skripte für Backup sowie Online-Updates.

Zusätzlich Wemos-Build und isolierter ZIP-Quellbuild. Auf einem Wemos: OTA von 2.0.7, erhaltende Passwortmigration, Folge-OTA mit SHA, Auth/CSRF/Origin-Prüfungen, Ablehnung eines ungültigen Images und Speichern/Lesen/Wiederherstellen der Einstellungen. Nutzer bestätigt Uhrzeit, deutschen Wochentag/Datum, Animationen und Weboberfläche. Gemessener Laufzeit-Heap nach korrigiertem OTA 33832 Byte, nach API-Test 33240 Byte; Momentaufnahmen, keine Worst-Case-Garantie. Keine Tests mit Werksreset, WLAN-Recovery oder physischen Stromausfällen. [Releasebericht](RELEASE_v2.0.8.md). Die ESP32-Portierung ist für diese Fassung zurückgestellt.
