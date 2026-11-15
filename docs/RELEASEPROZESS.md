# Verbindlicher Stable-Releaseprozess

Ein Stable-Release ist erst abgeschlossen, wenn Firmware, GitHub-Release und
beide OTA-Stable-Manifeste zusammenpassen und öffentlich überprüft wurden.
Ein GitHub-„Latest“-Release allein ist kein vollständiger Releaseabschluss.
Diese Regel ersetzt niemals die ausdrückliche Nutzerfreigabe zur Veröffentlichung.

## Reihenfolge

1. Explizite Freigabe für Stable-Veröffentlichung und OTA-Aktivierung prüfen,
   Zielrepository/-branch sowie Versionsnummer festlegen.
2. Hosttests und die beauftragten Builds durchführen. Für einen gemeinsamen
   Stable-Release beide Plattformen bauen; außerhalb solcher Freigaben bleibt
   ESP32 ausdrücklich auf Anforderung beschränkt.
3. Tatsächliche OTA-Dateien mit den produktiven Validatoren prüfen, insbesondere
   Plattformschutz, Dateityp, Integrität, Größe und ESP32-Partitionsreserve.
4. Tatsächliche Größen/SHA-256 feststellen; Secret-Scan, Testgrenzen und Factory-
   Datenverlust dokumentieren. Keine verteilte BIN unter gleicher Version ersetzen.
5. Erst danach GitHub-Release und vollständige Assets veröffentlichen.
6. Öffentliche Release-Downloads erneut abrufen, Größen und SHA-256 mit den
   lokalen geprüften Dateien und SHA256SUMS.txt vergleichen.
7. OTA-Dateien aus genau diesen Release-Assets für Wemos und ESP32 bereitstellen,
   niemals Factory. Erst BINs auf firmware-updates pushen und öffentliche
   kanonische Download-URLs einschließlich CORS/Größe/SHA prüfen.
8. Erst anschließend stable.json und stable-esp32.json mit passender Version,
   prerelease:false und tatsächlichen Ziel-/Pin-/Dateimetadaten veröffentlichen.
   Preview-Dateien und alte versionierte BINs bytegleich erhalten. Kein Force-Push.
9. Öffentliche kanonische Stable-Manifeste erneut abrufen, nicht nur lokale
   Dateien oder einen Cache-Buster prüfen. GitHub-Raw-Caches können vorübergehend
   alte Inhalte liefern; bis zur Beobachtung der richtigen Inhalte warten.
10. Die tatsächliche Firmware-Versionsvergleichs- und Updateprüfungslogik mit
    älteren Stable-/Beta-Versionen, gleicher Version und neuerer Beta testen,
    auf beiden Plattformen und mit Preview ein/aus. Keine Geräteinstallation.
11. Erst nach erfolgreichem öffentlichen OTA-Check vollständig abgeschlossen
    melden. Bei Fehlern „unvollständig“ mit fehlendem Schritt melden; keine
    Erfolgsmeldung aus allein erfolgreichem Build/Release ableiten.

## Automatisierung mit vorhandenen Berechtigungen

`.github/workflows/stable-ota.yml` veröffentlicht keine Firmware und erstellt
keinen Tag/Release. Er verarbeitet nur bereits veröffentlichte Stable-Releases.
Normale Commits/Pushes lösen ihn nicht aus; Prereleases ändern Stable niemals.
Preview bleibt eine eigenständige, ausdrücklich freizugebende Politik.

Für automatische Kanalaktivierung nach `release: published` muss die nach
Nutzerfreigabe erstellte Releasebeschreibung diesen unsichtbaren Marker enthalten:

```html
<!-- chaosuhr-stable-ota-approved -->
```

Den Marker nur bei vorliegender ausdrücklicher Stable-/OTA-Freigabe setzen.
Ein normaler Commit oder eine Anweisung in AGENTS.md ist keine solche Freigabe.
Fehlt der Marker, wird der automatische Job übersprungen: Releaseabschluss ist
bis zum separat erfolgreich geprüften OTA-Schritt unvollständig. Das unveränderte
Release v2.0.11 wird für die aktuelle Reparatur ausdrücklich manuell aktiviert.

Als Wiederaufnahme für ein vorhandenes Release kann ein berechtigter Nutzer den
Workflow manuell mit existierendem Tag und `approve_stable=true` starten. Das ist
bewusstes Aktivieren, kein automatischer Standard. `false` startet keine Publikation.
Auch im manuellen Pfad muss das Ziel ein veröffentlichtes Latest-Stable sein.

Der Workflow benötigt nur den vorhandenen GITHUB_TOKEN mit `contents: write`,
keine neuen Secrets, keine Geräte-/Broker-Zugänge. Repositorybindung, fester
Checkout-Commit der Action, keine Shell-Interpolation des Release-Tags, feste
Dateinamensregeln, Prüfungen vor Schreibzugriff und serialisierte Jobs schützen
gegen Fehlzuordnung. Tags müssen dem Stable-Schema entsprechen; Herkunftsrelease,
Quellversion, echter Produkt-/Chipvalidator, Größe und Prüfsummen werden geprüft.

Der Publisher `scripts/publish_stable_channel.py` wird lokal und im Workflow
verwendet. Er akzeptiert standardmäßig nur Validierung; für Schreibzugriffe
sind `--approved` und `--publish` erforderlich. Die Release-Datei SHA256SUMS.txt
ist obligatorisch. Nicht vorhandene, falsche oder zu große OTA-Dateien verhindern
die Aktivierung. Neue Layouts/Pin-Quellformen müssen bewusst geprüft werden;
der Publisher rät keine Metadaten. Factory landet niemals im Kanal.

Beispiel nach ausdrücklicher Freigabe:

```sh
python3 scripts/publish_stable_channel.py --tag v2.0.11 --approved --publish --report .pio/ota-report.json
```

Ohne `--publish` werden Release-Dateien und Versionslogik geprüft, aber kein
vollständiger Abschluss gemeldet. Ein Fehler setzt den Bericht nicht auf complete.
Bei bereits bytegleichen Dateien entsteht kein weiterer Commit. Eine Unterbrechung
nach dem BIN-Push lässt die alten Manifeste erhalten; ein erneuter Lauf kann
fortsetzen. Konkurrierende Branchänderungen führen zu normaler Git-Ablehnung,
niemals zu einem Force-Push. Eine ältere Release-Version ersetzt keinen neueren
Stable-Kanal.

## Grenzen der Automatisierung

Das explizite Veröffentlichen des GitHub-Releases und das bewusste Setzen des
Markers bzw. manuelle Zustimmen bleiben menschliche Freigabeschritte. Der
Workflow bestätigt Download/Validator/Versionslogik, keine Hardwareinstallation.
Cache-Verzögerungen oder GitHub-Ausfälle können den Abschluss verhindern. In
solchen Fällen den Fehler beheben und denselben Lauf wiederholen, keine neue
Firmware bauen oder eine neue Versionsnummer erfinden.

Prüfbelege: Actions-Run/Job-Summary und vollständiger JSON-Bericht. Der lokale
Codex-Abschluss muss zusätzlich öffentliche URLs, beide Größen/SHA-256,
Kanalcommit und getestete Firmwareversionen nennen. Die aktuelle
[Kanaldokumentation](GITHUB_UPDATES.md) enthält URLs und Bestandsaufnahme.
