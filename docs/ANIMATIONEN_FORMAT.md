# Eigene ChaosUhr-Animationen teilen – Format .chaos, Version 1

Auch Wemos **2.0.8** verwendet dieses unveränderte Dateiformat. Testangaben zu 2.0.7 unten sind historisch; 2.0.8 ist bisher nur lokal geprüft. Die zusätzliche Einstellungssicherung enthält keine Animationsdateien.

Für die **Wemos-Version 2.0.7 mit Upload-Verwaltung**. Das Format beschreibt Pixelbilder und ihre Anzeigedauer; es enthält keinen auszuführenden Programmcode. Für jede neue Animation ist damit kein Firmware-Flashen nötig. Die ESP32-Erweiterung bleibt vorerst zurückgestellt.

## In der GUI benutzen

1. Als `admin` in der normalen Weboberfläche anmelden.
2. Unter **Eigene Animationen** den Animationsspeicher bei Bedarf einmalig einrichten. Die Bestätigung löscht Daten ausschließlich im Dateisystem für Animationen. WLAN, Admin-Zugang und Uhreinstellungen bleiben erhalten. Ein nicht lesbarer Speicher wird nicht automatisch formatiert.
3. Zunächst das **Herz-Beispiel herunterladen** und als `.chaos` hochladen. Die gleiche Datei liegt im Paket unter `examples/Herz.chaos`.
4. Den neuen Eintrag oben für die Automatik ankreuzen oder als Einzelanimation auswählen; **Einstellungen speichern** drücken. Neue Uploads sind zunächst nicht für die Automatik ausgewählt.
5. Ein fehlendes Häkchen nimmt eine Animation aus der Automatik. Fest eingebaute Effekte bleiben im Firmware-Speicher und für den Einzelmodus verfügbar. Eigene Uploads lassen sich zusätzlich herunterladen oder nach Bestätigung vollständig löschen.

Maximal **acht eigene Dateien**, jeweils **24 KiB (24576 Byte)**. Anzeige: **20×15 Pixel**, bis zu **16 Farben**, **1–64 Bilder**. Das aktuelle Layout hat ein eigenes LittleFS-Dateisystem von ungefähr 1 MiB; der benötigte Dateisystemspeicher wird in der GUI angezeigt. Jeder belegte Upload-Platz zählt, auch wenn er für die Automatik deaktiviert ist. Bei vollem Speicher oder acht Dateien zuerst einen eigenen Upload löschen.

Nach dem Löschen wird der zugehörige Eintrag aus den gespeicherten Auswahlwerten entfernt. War er die letzte ausgewählte Animation, schaltet die Uhr in **Nur Uhrzeit**; als Auswahl für einen späteren Automatikstart wird Effekt 1 vorgemerkt. Andere zuvor deaktivierte Effekte werden nicht pauschal eingeschaltet. Ein beschädigter Upload wird nicht abgespielt, bleibt aber in der Verwaltung zum Herunterladen oder Löschen sichtbar.

## Mit ChatGPT erstellen

Die vollständige Vorlage steht in **CHATGPT_ANIMATION_PROMPT.txt** und kann über **ChatGPT-Vorlage öffnen** direkt von der Uhr abgerufen werden. Eigenes Motiv eintragen, ChatGPT die Datei erstellen lassen und den vollständigen reinen Text als beispielsweise `rakete.chaos` speichern. Anschließend hochladen. Markdown-Zeichen wie drei Backticks gehören nicht in die Datei.

Die Textdatei enthält nur Name, Farbpalette, Pixel und Bildzeiten, keine persönlichen Zugangsdaten. Sie kann direkt im Discord geteilt werden. Die Name-/Pixelkoordinaten sind unabhängig von der LED-Verdrahtung: Bildzeile 1 ist oben, innerhalb der Zeile geht es von links nach rechts. Jede Uhr wendet ihre eigene gespeicherte Matrixausrichtung an.

## Genaue Dateiregeln

ASCII-Text ohne BOM. LF und Windows-CRLF werden akzeptiert. Groß-/Kleinschreibung ist relevant.

| Abschnitt | Inhalt |
| --- | --- |
| Erste Zeile | Genau `CHAOSUHR 1` |
| Zweite Zeile | `NAME ` und 1–32 Zeichen aus `A–Z`, `a–z`, `0–9`, Leerzeichen, `_`, `.`, `-` |
| Dritte Zeile | `PALETTE N`, wobei N zwischen 1 und 16 liegt |
| Farbzeilen | Genau N Zeilen im Format `#RRGGBB`, Hex-Zeichen ausschließlich `0–9A–F` |
| Bildanfang | `FRAME T`, wobei T eine ganze Zahl von 20 bis 10000 Millisekunden ist |
| Bilddaten | Genau 15 Zeilen mit jeweils genau 20 Zeichen, ohne Leerzeichen |
| Weitere Bilder | Jeweils wieder `FRAME T` und 15 vollständige Bildzeilen, maximal 64 Bilder insgesamt |
| Ende | Eigene Zeile `END`; danach ausschließlich leere Zeilen oder Dateiende |

Ein Pixelzeichen `0–9A–F` ist der nullbasierte Index einer vorhandenen Palettenfarbe. Bei drei Farben sind also nur `0`, `1` und `2` gültig. Farbe 0 sollte für einen schwarzen Hintergrund `#000000` sein; transparente Pixel gibt es nicht. Alle 300 Pixel eines Bildes werden dargestellt.

Leere Zeilen sind zwischen vollständigen Bildern möglich, innerhalb der Bilddaten nicht. Zahlen haben keine Vorzeichen oder Dezimalstellen. Kommentare, zusätzliche Felder und Daten nach `END` werden abgelehnt.

Die Bildfolge wiederholt sich während ihrer Szene. Das normale Tempo ist **55**; der gemeinsame Temporegler skaliert die Bildzeiten wie bei den übernommenen Bildanimationen. Automatik-Szenen enden nach der eingestellten Szenendauer, auch wenn eine sehr lange Bildfolge dann noch nicht vollständig gelaufen ist. Für den Austausch eignen sich kurze Clips mit etwa 4–12 Bildern und 80–400 ms pro Bild.

Die hochgeladene Datei wird zunächst vorübergehend gespeichert, anschließend vollständig geprüft und erst bei Erfolg veröffentlicht. Fehler nennen die betroffene Zeile. Ungültige, zu große oder abgebrochene Uploads ersetzen keine vorhandene Animation. Dieselbe Datei lässt sich unverändert wieder herunterladen und teilen. Gleiche Namen sind erlaubt; bestehende Dateien werden beim Upload nicht überschrieben.

## Speicherung, Update und Werksreset

Uhreinstellungen und Zugangsdaten bleiben im bisherigen EEPROM-Bereich. Die Upload-Auswahl ergänzt die Bytes 28–31; der Admin-Bereich beginnt weiterhin bei Offset 32. Alte 20- und 28-Byte-Einstellungsdaten werden übernommen. Ein normales OTA-Update mit unverändertem Flash-/Dateisystemlayout erhält die hochgeladenen Dateien. Die Firmware-BIN enthält keine persönlichen Uploads; zum Teilen einer Animation deren `.chaos` herunterladen.

Ein bestätigter **Werksreset löscht auch alle eigenen Animationen**. Nach dem dauerhaften Reset-Merkmal werden zuerst WLAN-Daten, dann das Animationsdateisystem und zuletzt EEPROM-Einstellungen und Zugangsdaten gelöscht. Scheitert ein Schritt, bleibt das Merkmal für einen erneuten Versuch erhalten.

## API

Alle Routen verlangen den eingerichteten Admin-Zugang. Schreibzugriffe zusätzlich den Header `X-CSRF-Token` aus `GET /api/session`.

| Route | Zweck |
| --- | --- |
| `GET /api/animations` | Speicherstatus und belegte Upload-Plätze |
| `POST /api/animations/initialize` | Bestätigung `confirm=INITIALIZE`, nur für noch nicht eingerichteten/nicht lesbaren Speicher |
| `POST /api/animations/upload` | Multipart mit genau einer Datei im Feld `animation` |
| `GET /api/animations/download?id=31` | Originaldatei eines Uploads herunterladen |
| `POST /api/animations/delete` | `id=31&confirm=DELETE`; keine fest eingebauten Effekte löschbar |
| `GET /animations/example.chaos` | Vollständiges Herz-Beispiel |
| `GET /animations/prompt` | ChatGPT-Vorlage |

Upload-Plätze haben stabile IDs **31–38**. `GET /api/effects` enthält nur lesbare/validierte Uploads zusätzlich zu den 30 eingebauten Effekten; Upload-Einträge tragen `uploaded=true`. `effectCount` zählt den aktuell verfügbaren Katalog, `builtinEffectCount` bleibt 30, `uploadedEffectCount` zählt lesbare Uploads, `maxEffectId` ist 38. Freie Plätze können Lücken im Katalog verursachen.

`autoEffects` bleibt die Bitmaske der eingebauten Effekte. Neu ist `autoUploads` (0–255): Bit 0 steht für Upload-ID 31, Bit 7 für ID 38. `autoEffects=0` ist zulässig, wenn mindestens ein verfügbarer Upload in `autoUploads` ausgewählt ist. Beide Masken gemeinsam dürfen beim Speichern nicht leer sein. Nach Löschen einer Datei werden ihre Auswahlwerte vor dem eigentlichen Löschen gespeichert; bei einem Speicherfehler bleibt die Datei erhalten. Scheitert erst das Dateilöschen, bleibt sie erhalten und kann bereits aus der Automatik herausgenommen sein.

## Teststand

Parser, Bildzeiten und Koordinaten, LF/CRLF, Datei-/Bildgrenzen, Upload-Abbruch, falsche Felder, Speicherfehler, volle Plätze, Löschen/Neustart, Datenübernahme, Admin-/CSRF-Schutz und GUI wurden lokal geprüft. Auf einem Wemos wurde 2.0.7 installiert; der Nutzer bestätigte die Einrichtung des Animationsspeichers und den Herz-Upload. Anschließend wurden zwei lesbare Bilder und der bytegleiche Originaldownload geprüft. Eine gesonderte visuelle Bestätigung der Herzanimation, tatsächliches Löschen und Werksreset stehen noch aus. Details: [Release- und Teststand](RELEASE_v2.0.7.md).
