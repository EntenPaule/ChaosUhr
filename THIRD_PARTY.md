# Übernommene Bestandteile

## ChaosUhr / ChaosBlinkiBuntiUhr

Quelle: https://github.com/oOmicha1986Oo/ChaosUhr

Autor im Original-Sketch: **Micha1986** / **oOmicha1986Oo**.
Übernommener Stand: Commit **51aef401db22ef0fba3e5e334f141bf794d7505a**.
Lizenz: **GNU GPL Version 3**, vollständiger Text in `LICENSE`.

`src/legacy_assets.h` enthält 52 unveränderte RGB888-Bildinhalte aus
`DigiDug.h`, `Qbert.h`, `Face.h`, `Face2.h`, `Face3.h`, `Feuer.h`, `Drink.h`
und `BombJack.h`. Deklarationen sind von `long` auf `uint32_t` festgelegt;
Farbwerte und ursprüngliche physische LED-Reihenfolge bleiben erhalten.
Unbenutzte Bilder `Face006` und `Face201` sowie `Icons.h` werden nicht eingebunden.

Die Bildfolgen und Zeiten in `src/legacy_animation.cpp` folgen dem Original-
Sketch. BombJack war dort nur als Bilddatei vorhanden; hier werden die zwei
Bilder zusätzlich mit je 250 ms angezeigt. Die Wiederholschleifen wurden durch
einen zeitgesteuerten Player ersetzt; Szenendauer und Helligkeit verwaltet die
neue Firmware. `tests/legacy_assets_manifest.json` enthält SHA-256-Nachweise
der importierten Dateien und jedes verwendeten Bildes. Ein erneuter Import ist über
`scripts/import_legacy_assets.py` aus dem ZIP desselben Commits möglich.

## FastLED DemoReel100

Die sechs klassischen prozeduralen Effekte (`rainbow`, `confetti`,
`rainbowWithGlitter`, `sinelon`, `bpm`, `juggle`) wurden über den Original-
Sketch übernommen und für Einzelbild-Aufrufe mit gemeinsamem Helligkeitslimit,
Tempo und Matrixmapping angepasst. Sie gehen auf FastLED DemoReel100 zurück
(Mark Kriegsman, FastLED). FastLED steht unter der MIT-Lizenz; der vollständige
Hinweis liegt in `third_party/FastLED-LICENSE`.
