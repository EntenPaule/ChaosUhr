# ChaosUhr 2.0.11-beta.4.2 – interner ESP32-UART-Bugfix

## Befund und Grenzen des Nachweises

Hardwarebefund des Nutzers: fabrikneuer ESP32-WROOM ohne Matrix, Beta.4.1 startet,
FS-001 bei leerem LittleFS erwartbar. Ungültige erste Eingabe öffnete keinen Port.
Danach gültige Eingabe mit Timeout und vom Setup-AP abgewiesenem Code; auch nach
Stromtrennung. Kein serieller Mitschnitt/Resetgrund vorhanden, kein Gerätezugriff
bei dieser Untersuchung. Deshalb ist ein Reset beim Portöffnen eine plausible,
aber NICHT am konkreten Board nachgewiesene Erklärung.

Nachgewiesener Softwarefehler: Beta.4.1 öffnet den Port, verwirft Eingabedaten,
sendet genau einmal sofort SETUP und wartet fünf Sekunden auf ACK. Keine
Bereitschaftssynchronisation, kein Wiederholen. Der Regressionstest führt das
unveränderte historische Skript aus `tests/usb_setup_beta4_1.py.in` aus und
reproduziert dessen Timeout bei einer simulierten dreisekündigen Bootphase nach
Portöffnung; kein Befehl erreicht dabei den laufenden Firmwareparser.

Lokale pyserial-POSIX-Implementierung: os.open erfolgt VOR _update_dtr_state und
_update_rts_state. False-Werte vor dem Öffnen verhindern also keinen möglichen
Treiber-/Adapterimpuls während os.open. Es wird weiterhin kein Reset bewusst
angesteuert. 115200 Baud, 8N1, keine Hardwareflusssteuerung passen zu Serial.begin.
ESP32-WROOM verwendet UART0 über die USB-UART-Brücke, kein natives USB-CDC.

Quellprüfung: SETUP plus Leerzeichen plus zwölf Ziffern ergibt die erwarteten
18 Zeichen vor LF. CRLF ist zulässig, Puffer ist 19 Byte einschließlich Null.
ACK in Firmware und Python war bereits exakt gleich. UART-Verarbeitung liegt
vor serviceNetwork im loop, nicht hinter dem Erstsetup-Return in serviceNetwork.
Der sichtbare Setup-AP wird am Ende von setup gestartet; ein generell blockierter
Serial-Loop oder vorzeitig gesetzter Admin ist aus dem Befund NICHT bewiesen.
FS-001 allein sperrt den UART-Parser nicht. Ungültige NVS-Zugangsdaten würden
provisioning() sperren; ein wirklich leerer Credentialbereich tut dies nicht.

AP-Pfad im gepinnten Arduino3.3.12: softAP ruft AP.begin und AP.create auf;
softap_config_equal vergleicht auch das Passwort, eine Änderung führt zu
esp_wifi_set_config(WIFI_IF_AP). Kein Beleg, dass ein zusätzlicher AP-Neustart
nötig ist. Deshalb kein spekulativer Neustart des AP oder der Uhr ergänzt.

## Minimale Korrektur

- Neuer nichtgeheimer STATUS-Befehl: READY 1 bei offenem Setupfenster, sonst CLOSED.
- Skript fragt höchstens 20 Sekunden nach Bereitschaft, einmal pro Sekunde.
  Vor READY wird KEIN Einrichtungscode übertragen. Fremde/Boot-Ausgaben werden
  ignoriert und nie gedruckt, Leseabschnitte sind auf 256 Byte begrenzt.
- Nach READY maximal acht Sekunden Übertragung/ACK, Wiederholung einmal pro
  Sekunde desselben Codes. Vorangestelltes LF beendet mögliche unvollständige
  Zeilen. Wiederholungen sind idempotent; verlorenes ACK erzwingt keinen Neustart.
- Firmware liest nach softAP die AP-Konfiguration zurück, prüft SSID, WPA2-Modus
  und Passwort bytegenau, löscht die gelesene Struktur und gibt erst danach ACK.
  AP_FAILED und CLOSED sind feste geheimnisfreie Antworten. Rücklesen belegt
  SDK-Konfiguration, NICHT schon eine erfolgreiche Funkanmeldung eines Clients.
- Fünf-Minuten-Frist bleibt erhalten. Sobald abgelaufen oder Provisionierung
  geschlossen ist, bleibt der Codeeingang bis zum nächsten Boot gesperrt, auch
  über millis-Überlauf. STATUS kann nur noch CLOSED melden; keine Konfiguration.
- Linux-Port exklusiv angefordert, Lese-Timeout0,25s, Schreib-Timeout2s. Das
  ersetzt nicht das Schließen anderer serieller Programme. Keine Roh-Logs,
  Zugangsdaten oder Einrichtungscodes in Fehlermeldungen.
- Matrix/AP-Verfahren, WLAN-Persistenz, LittleFS, Partitionen, Watchdogs, MQTT,
  Wetter, GUI und Wemos-Funktionen bleiben erhalten. Nur ESP32-Versionskennung
  und Skript-Cachekennung wechseln auf beta.4.2. Keine Formatierung ergänzt.

## Tests

Regression: exaktes altes Python-Skript scheitert beim simulierten Portöffnungs-
Reset/Boot; neuer Dialog funktioniert bei drei und sieben Sekunden Bootzeit sowie
verlorenem ersten ACK. CLOSED, AP_FAILED und komplett stummer Port brechen
begrenzt ab, bei CLOSED/stumm kein Codeversand. Ungültige Eingabe ohne Portzugriff.
Parser: STATUS/CRLF, ungültiger Status, gültiger Code, Überlänge, ungültige Zeichen,
Abbruch und Neuansatz. Bestehende Hostsuite zusätzlich erneut ausgeführt.
Alle UART-Tests simuliert: kein /dev/ttyUSB0 geöffnet.

## Erneuter Hardwaretest (manuell, noch ausstehend)

1. Andere Portnutzer (PlatformIO-Monitor, Terminal, weitere Setup-Skripte) schließen.
   Für den ersten Versuch die neue Firmware beta.4.2 UND das aktualisierte Skript
   verwenden. Das Skript wartet bei beta.4.1 absichtlich vergeblich auf READY.
2. Neue Firmware nur nach eigener Entscheidung installieren. Für eine noch
   fabrikneue, nicht eingerichtete Uhr: Factory-BIN per USB ab0x0. Sie enthält
   4MiB und überschreibt vorhandene Daten! Kein vollständiges erase_flash nötig.
   Für eingerichtete Geräte vorzugsweise OTA-BIN über die authentifizierte GUI;
   Factory ist niemals eine OTA-Datei. In diesem Auftrag wurde nichts geflasht.
3. Uhr neu starten und innerhalb fünf Minuten
   `/home/paulr/.platformio/penv/bin/python scripts/esp32_usb_setup.py /dev/ttyUSB0`
   starten. Zwölf Ziffern zweimal verdeckt eingeben. Code nicht in Chat/Logs teilen.
4. Erwartet: zuerst „Firmware bereit; Einrichtungscode wird übertragen.“, danach
   „Bestätigt …“. Der Helper wartet einen möglichen Portöffnungsreset selbst ab.
   Bleibt READY aus, nur die feste Fehlermeldung und beobachtete Boot-/AP-Neustarts
   melden. Keine vollständigen Roh-Stackdumps und keine Codes senden.
5. Gespeichertes altes ChaosUhr-Setup-WLAN-Profil am Testclient entfernen,
   erneut mit dem jetzt gewählten Code verbinden und http://192.168.4.1/ öffnen.
   Vor Admin-Einrichtung darf derselbe Helper erneut erfolgreich sein. Ein
   abweichender Testcode muss nach ACK ebenfalls funktionieren; alten Code
   anschließend nicht als weiter gültig erwarten.
6. Admin einrichten, WLAN speichern; Helper danach muss CLOSED melden und darf
   nichts ändern. WLAN-Verbindung nach 30 Sekunden vollständiger Stromtrennung
   prüfen. LittleFS nur bei bewusst bestätigtem Einrichten initialisieren.
7. Separater frischer Setup-Test: länger als fünf Minuten nach Boot warten;
   STATUS muss CLOSED ergeben, keine Codeübernahme. Nicht dafür ein eingerichtetes
   Gerät zurücksetzen oder seine Einstellungen löschen.

Wenn trotz READY+ACK das AP-Passwort abgewiesen wird, ist dies ein gesonderter
AP-/Client-Hardwarebefund. Dann nicht pauschal den Schutz lockern oder formatieren.
Ohne READY unterscheidet das Skript zwischen CLOSED (Firmware antwortet) und
Timeout (kein Protokollnachweis). Ein Resetgrund am konkreten Board bleibt offen.

## Artefakte

Nur interne Version **2.0.11-beta.4.2**, klassischer ESP32-WROOM mindestens4MiB.
Layout unverändert: Bootloader0x1000, Partitionstabelle0x8000, boot_app0 0xe000,
Anwendung0x10000; beide OTA-Slots0x1e0000Byte, LittleFS0x3d0000/0x30000.
Factory gesamt4194304Byte ab0x0, LittleFSFF und bewusst uninitialisiert.
BUILD_INFO und SHA256SUMS stehen unter dist/2.0.11-beta.4.2.
Vorhandene Beta.4.1-Artefakte unverändert. Kein Push, Release, Tag oder Geräteeingriff.

## Abschlusswerte

Finaler ESP32-Build SUCCESS: OTA1884672Byte, freie Slotreserve81408Byte,
sektorgenau77824Byte. RAM66460/327680Byte, Linker-Flash1823343Byte.
62 PASS-Gruppen der kompletten Hostsuite; danach zusätzliche gezielte Prüfungen
für begrenzten AP-Rücklesevergleich und den vollständigen Python-main-Aufruf
mit exklusivem Port/Timeouts/inaktiven DTR/RTS unter Simulation erfolgreich.
Der Vergleich verwirft falsche SSID, falschen WPA-Modus und abweichende bzw.
verlängerte Passwörter, ohne Werte auszugeben. Keine Hardwaretests vorgetäuscht.
Wemos main/public_access nach Präprozessierung weiterhin äquivalent zum alten
Stand (nur triviale Formatierung/Empfängerklammern/Abschnittszeilennummer normalisiert).
Keine Wemos-BIN erzeugt. Bestehende Beta.4.1-Dateien werden gegen ihren Git-Stand
bytegenau geprüft, bevor die neue lokale Artefaktsicherung abgeschlossen wird.
