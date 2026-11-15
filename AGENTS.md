# ChaosUhr – verbindliche Projektanweisungen

## Stable-Veröffentlichungen und OTA gehören zusammen

Ein Stable-Release ist erst vollständig abgeschlossen, wenn die Firmware,
das GitHub-Release und die OTA-Stable-Manifeste für **Wemos/ESP8266 und ESP32**
zusammenpassen und öffentlich verifiziert wurden. Ein GitHub-Latest-Release
allein genügt nicht.

Diese Anweisung ist **keine Veröffentlichungsfreigabe**. Push, Tag, Release und
Stable-/Preview-Aktivierung benötigen die ausdrückliche Autorisierung des Nutzers.
Keine Gerätezugriffe, Flash-/Formatieraktionen oder Änderungen am Gaming-Fork
ableiten. ESP32 außerhalb ausdrücklich freigegebener gemeinsamer Release-Builds
nur auf Anforderung bauen.

Nach jeder autorisierten Stable-Veröffentlichung ist die feste Reihenfolge in
[docs/RELEASEPROZESS.md](docs/RELEASEPROZESS.md) einzuhalten:
Version → Hosttests/Builds → echte OTA-Validatoren → Größen/SHA → GitHub-Release
mit Assets → öffentliche Assetintegrität → versionierte OTA-BINs im Kanal
veröffentlichen und öffentlich prüfen → beide Stable-Manifeste aktualisieren
und pushen → kanonische öffentliche Manifeste erneut abrufen → tatsächliche
Firmware-Versionsvergleichslogik mit älteren Stable/Beta, gleicher Version und
neuerer Beta auf beiden Plattformen testen → erst dann vollständiger Abschluss.

- Tatsächliche Release-Asset-URLs und Byteinhalte verwenden; öffentliche Downloads,
  SHA-256, Größe, Board/Chip, Flash-/OTA-Limit und LED-Pin-Metadaten prüfen.
- Niemals ESP32-Factory als OTA veröffentlichen. Keine existierenden versionierten
  BINs ersetzen, keine stillen Downgrades und kein Force-Push.
- Preview und Stable getrennt halten; ohne eigene Freigabe keine Preview-Änderung.
- Bei fehlgeschlagenem Schritt Veröffentlichung als **unvollständig** melden.
  Ohne erfolgreichen öffentlichen OTA-Check keinen vollständigen Abschluss melden.
- Publisher und Workflow sind freigabegesteuert. Release-Marker oder manuelles
  approve_stable dürfen nur bei vorliegender ausdrücklicher Freigabe verwendet
  werden. Normale Commits/Pushes sind keine Stable-Freigabe.
- Keine Zugangsdaten, Tokens oder private Gerätekonfigurationen veröffentlichen.
- Hardwaretests strikt von Host-/Browser-Simulationen unterscheiden.
