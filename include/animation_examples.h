#pragma once
#include <Arduino.h>
static const char ANIMATION_EXAMPLE[] PROGMEM=R"CHAOS(CHAOSUHR 1
NAME Herz
PALETTE 3
#000000
#FF2020
#FF8080
FRAME 300
00000000000000000000
00000111000011100000
00001111100111110000
00011111111111111000
00011111111111111000
00001111111111110000
00000111111111100000
00000011111111000000
00000001111110000000
00000000111100000000
00000000011000000000
00000000000000000000
00000000000000000000
00000000000000000000
00000000000000000000
FRAME 300
00000000000000000000
00000222000022200000
00002222200222220000
00022222222222222000
00022222222222222000
00002222222222220000
00000222222222200000
00000022222222000000
00000002222220000000
00000000222200000000
00000000022000000000
00000000000000000000
00000000000000000000
00000000000000000000
00000000000000000000
END
)CHAOS";
static const char ANIMATION_PROMPT[] PROGMEM=R"PROMPT(Erstelle eine Pixelanimation fuer die ChaosUhr als reine .chaos-Textdatei. Motiv: [HIER DEINE IDEE EINTRAGEN].
Gib ausschliesslich den vollstaendigen Dateitext aus, ohne Markdown, Erklaerungen oder ausgelassene Zeilen.
Formatversion 1, feste Matrix 20 Spalten x 15 Zeilen. Jede Bildzeile wird von links nach rechts gelesen; die erste ist oben. Koordinaten sind unabhaengig von der LED-Verdrahtung.
Erste Zeile exakt: CHAOSUHR 1
Zweite Zeile: NAME gefolgt von einem Leerzeichen und einem Namen mit 1-32 Zeichen aus A-Z, a-z, 0-9, Leerzeichen, Unterstrich, Punkt oder Bindestrich. Keine Umlaute.
Dritte Zeile: PALETTE gefolgt von einem Leerzeichen und der Farbanzahl 1 bis 16.
Danach genau diese Anzahl Farbzeilen, jeweils #RRGGBB mit Grossbuchstaben. Farbe 0 soll schwarz sein (#000000).
Danach 1 bis 64 Bilder. Jedes beginnt mit FRAME und einer ganzzahligen Dauer von 20 bis 10000 Millisekunden, getrennt durch ein Leerzeichen.
Direkt darauf genau 15 Zeilen mit jeweils exakt 20 Zeichen. Jedes Zeichen 0-9 oder A-F verweist auf den Index einer vorhandenen Palettenfarbe. Keine Leerzeichen in Bildzeilen.
Nach allen Bildern eine eigene Zeile END. Keine weiteren Inhalte. Gesamte Datei maximal 24576 Bytes. Keine Programmcodes, JSON, Kommentare oder Abkuerzungen.
Die Uhr wiederholt die Bildfolge; 55 ist das normale Animationstempo. Nutze vorzugsweise 4-12 Bilder und 80-400 ms pro Bild. Pruefe vor der Ausgabe alle Zeilenlaengen und Palettenindizes.
)PROMPT";
