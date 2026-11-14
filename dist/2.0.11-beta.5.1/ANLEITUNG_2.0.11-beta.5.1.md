# MQTT-Empfang: interne 2.0.11-beta.5.1

## Ergebnis und Grenzen

Ausgangspunkt: sauberer Entwicklungsbranch `fix/2.0.11-diagnostics`, HEAD
`06596d48183943993142a533d96837d59f9884b1`. Root ist kein gültiges Git-Repository;
verifiziert und bearbeitet wird `.pio/dev-2.0.11`.

**Nachgewiesener Firmwarefehler:** `MqttInbox::receive` lehnte jede Nachricht mit
`total == 0` sowie jeden Null-Datenzeiger ab. Ein MQTT-PUBLISH darf eine leere
Nutzlast enthalten. Beim gepinnten espMqttClient 1.7.3 erzeugt der echte Parser
hier einen Callback mit Länge/Index/Gesamtlänge 0; dessen Datenzeiger ist nicht
als Nutzlast zu lesen. Daraus wurde fälschlich MQTT-016. Der neue Wire-Test
scheitert vor der Korrektur und besteht danach. Private Belege:
`.pio/mqtt-wire-before.log`, `.pio/mqtt-wire-after.log`.

**Nicht nachgewiesen:** Dass die gemeldeten MQTT-016-Ereignisse tatsächlich leere
Nachrichten waren oder dass dieser Fehler die ausbleibenden Helligkeitswerte
50/20 verursacht hat. Ein leerer Frame verhindert nicht dauerhaft den folgenden
gültigen Frame. Deshalb ist dies keine behauptete Hardwareheilung des gesamten
Fehlerbilds. Kein Zugriff auf Uhr, USB, Broker oder private Zugangsdaten.

## Minimale Korrektur

`include/mqtt_inbox.h`: Null-Datenzeiger nur bei Länge > 0 ablehnen; leere Frames
normal abschließen und einreihen; memcpy nur bei Länge > 0. Grenzen, Topic-/Index-
und Gesamtlängenprüfung bleiben aktiv. Echte widersprüchliche Fragmente melden
weiterhin MQTT-016. Keine zusätzlichen Puffer, Felder, Heap-Allokationen,
Flash-Schreibvorgänge, Änderungen an MQTT-Themen, HTTP-APIs oder Speicherlayouts.

Ein leerer Helligkeitsframe bleibt ein ungültiger Helligkeitswert und verändert
die aktuelle Vorgabe NICHT. Er bedeutet insbesondere nicht AUTO. Bei Wetter-
oder Sensorinhalten gelten weiterhin deren bestehenden Payloadprüfungen.

## Empfangsweg und Subscription

`ClockMqtt::setBrightnessControl` setzt bei Änderung `brightnessRefresh_`.
Der nächste Loop berechnet Topic/Kollisionen und setzt `brightnessSubscribe_`.
Bei bestehender Verbindung wird sofort subscribe aufgerufen, ohne Reconnect;
bei Queuefehler (Rückgabe 0) bleibt die Anforderung für den nächsten Loop stehen.
Nach Connect/Reconnect wird erneut abonniert. Ein SUBACK-Fehler meldet MQTT-012.
Eine erfolgreiche Queueannahme beweist keine serverseitige Subscription.

Das Topic stammt aus dem gespeicherten Basistopic oder der Gerätekennung plus
`/brightness/set`. Wetter, Temperatur und alle vier Sensortopics werden auf
Kollision geprüft. Eine Kollision sperrt die Steuerung, nicht die Telemetrie.
Die separate LittleFS-Richtlinie wird im Loop an setBrightnessControl übergeben;
nach erfolgreichem GUI-Speichern wird dieselbe Funktion aufgerufen. Kein
nachgewiesener veralteter Subscriptionzustand, daher keine Änderung dieses Codes.

Callback → begrenzte Inbox → vollständiger Frame → Topicprüfung → strikter
BrightnessOverride-Parser → gemeinsames RAM-Objekt → currentBrightness → Render.
Retain wird nicht ausgefiltert. Nachtabschaltung hat weiterhin absoluten Vorrang.

## ioBroker-Server: nachgewiesene Regeln, lokale Konfiguration unbekannt

Untersucht wurde der öffentliche Adapterquellstand, nicht die installierte
Adapterversion des Nutzers. Aktuelle Quellen können von dessen Version abweichen.

Die Publish-Maske muss die eigenen Zustände umfassen, z.B. `mqtt.0.*`.
`ack=false` wird weitergeleitet; `ack=true` nur mit `sendAckToo`.
`onchange` kann unveränderte Werte unterdrücken. Eine sichtbare Datenpunktänderung
allein beweist keine MQTT-Auslieferung.
Quelle: [ioBroker main.ts](https://github.com/ioBroker/ioBroker.mqtt/blob/master/src/main.ts).

Client-Publishes werden normalerweise als bestätigte Zustände gespeichert.
Mit `extraSet` wird ein eingehendes `/set` entfernt und als Befehl behandelt;
ausgehende Befehle erhalten `/set`. Deshalb Topic und tatsächlichen Statepfad
prüfen, nicht blind ein weiteres `.set` ergänzen. Der Echo-Schutz kann Rücksendungen
an den ursprünglichen Publisher unterdrücken. `publishOnSubscribe` steuert die
Auslieferung bekannter Werte beim Abonnieren, `noRetain` beeinflusst das Flag.
Der direkte Befehl `sendMessage2Client` reicht ein explizites Topic an Clients
weiter; dessen Callback ist keine Geräte- oder Brokerpersistenzbestätigung.
Quelle: [MQTTServer.ts](https://github.com/ioBroker/ioBroker.mqtt/blob/master/src/lib/MQTTServer.ts).

Keine dieser Einstellungen wurde gelesen oder geändert. Ob die konkrete
Installation auf `/brightness/set` sendet, bleibt offen. AUTO in ioBroker
beweist ausschließlich die Richtung Uhr → Broker, nicht den Rückweg.

## Gezielter Hardwaretest durch den Nutzer

1. Vor einem Firmwarewechsel bereits mit Beta.5 prüfen: Adapterversion und die
   genannten Optionen nur ansehen. Keine Zugangsdaten oder vollständigen Logs
   teilen. Im angemeldeten GUI prüfen: mqttActive=true, conflict=false und das
   tatsächlich angezeigte Topic (geschütztes GET `/api/brightness`).
2. Einen unabhängigen MQTT-Client am vorhandenen ioBroker-Broker NUR auf dieses
   eine Topic abonnieren lassen. Kein Wildcardmitschnitt, keine Passwörter in
   Kommandozeilen/Chat. Beim manuellen Schreiben von String `50` mit ack=false
   prüfen, ob tatsächlich genau Topic und zwei Payloadbytes 0x35 0x30 eintreffen.
   Anschließend `20`. Bei ausbleibender Nachricht ist zunächst der Broker-
   Zustandsweg zu klären. Keine automatische Adapterkonfigurationsänderung.
3. Alternativ ausdrücklich den vorhandenen ioBroker-JavaScript-Adapter nutzen:
   `sendTo('mqtt.0', 'sendMessage2Client', {topic: 'chaosuhr/0d82d3/brightness/set', message: '50', retain: false});`
   Nur vom Nutzer ausführen, nicht durch diesen Auftrag. Das umgeht die
   State-/ack-Abbildung; unabhängigen Empfang trotzdem prüfen. Danach AUTO senden.
4. Beta.5.1 OTA ausschließlich zur passenden Plattform installieren (hier nicht
   ausgeführt). Nach Login zuerst bei verbundener Uhr Steuerung aus/ein speichern;
   dann 0, 1, 5, 20, 50, 100 und AUTO einzeln senden. Quelle/Wert im GUI und Matrix
   vergleichen. MQTT-Vorgabe 0 ist gültig. Nachtabschaltung separat testen.
5. Retained 50 und AUTO mit einem MQTT-Client bewusst veröffentlichen; Reconnect
   und Kaltstart prüfen. Die Uhr akzeptiert diese, sofern der Broker sie liefert.
   GUI-Freigabe und retained AUTO anschließend kontrollieren. Leere Retained-
   Löschung darf keine neue MQTT-016-Meldung erzeugen, 50 danach muss wirken.
6. Wetter/vier Sensoren, Discovery/Online/Offline, Nachtabschaltung und
   vorhandene Dateien prüfen. Heap/Stack und Fehlerzähler vor/nach einer Serie
   vergleichen; keine Rohdaten oder Zugangsdaten sammeln. AUTH-021 nach Neustart
   ist erwartbar; software-restart allein beweist keinen Absturz.

Wenn der unabhängige Client korrekte Nachrichten sieht, die Uhr aber keine
Vorgabe übernimmt: Zeitpunkt, Firmwareversion, mqttActive/conflict, Empfang auf
dem exakten Topic und neue Fehlercodes melden. Broker-Subscriptionbestätigung
für die Uhr prüfen. Kein Erfolg aus einem allein grünen Build ableiten.

## Automatisierte Prüfung

Neue `tests/test_mqtt_wire.cpp` verwendet den tatsächlich gepinnten Parser.cpp
und RemainingLength.cpp. QoS 0/1/2, alle TCP-Blockgrößen, Retainedwerte, leerer
Frame, mehrere Nachrichten im selben TCP-Block, ungültige Payloads und echte
Fragmentwidersprüche. Vorher-Negativlauf mit ursprünglicher Inbox reproduziert.
`tests/test_clock_mqtt.cpp` ergänzt nachträgliches Aktivieren ohne neue Verbindung,
leere Frames ohne Vorgabenänderung, alle geforderten Werte, retained AUTO,
Wetter und vier Sensoren. Bestehende Tests decken Reconnect, Topickonflikte,
Discovery/Availability, Sitzungen, Backup und Flash-Schreibverhalten ab.

Die Tests simulieren Transport/Broker; kein echter ioBroker- oder Hardwaretest.

## Build und Ressourcen

Beide PlatformIO-Profile SUCCESS: ESP8266 Arduino 3.1.2, ESP32 Arduino 3.3.12;
Plattformen, Bibliotheken und Partitionen unverändert. Vollständige finale
Hostsuite: 69 PASS-Gruppen, einschließlich ASan/UBSan. Artefaktvalidierung
und tatsächliche Größen werden zusätzlich im BUILD_INFO ausgewiesen.

| Plattform | OTA-BIN | OTA-Limit | Freie Reserve | Statisches RAM |
| --- | ---: | ---: | ---: | ---: |
| Wemos | 697952 B | 1044464 B | 346512 B | 51920 / 81920 B |
| ESP32 | 1895472 B | 1966080 B | 70608 B | 66588 / 327680 B |

ESP32 sektorgenau 69632 B Reserve. Statisches RAM unverändert gegenüber Beta.5;
keine neue Laufzeit-Heapmessung. Die gemeldeten 19144 B sind ein einzelner
Hardwaremesswert der Beta.5, kein garantierter Mindestheap. Empfangscode legt
keine neuen Heapobjekte an. Wemos MQTT-Speicherhandler 96 B + Speicherfunktion
272 B = 368 B vor weiteren Callees (unverändert); Empfangscallback 48 B statisch.
Keine Zusage über maximale Laufzeit-Stacktiefe aus diesen Einzelwerten.

Die neue Version ist intern, kein Tag/Release/Push. Alte Beta.5-Pakete bleiben
bytegleich. Neue Dateien ausschließlich unter `dist/2.0.11-beta.5.1/`.
OTA-BIN nur auf passende Plattform laden. Zusätzlich enthaltenes ESP32-Factory-
Image: USB ab 0x0, niemals OTA; überschreibt alle 4 MiB einschließlich Nutzerdaten.
Für bestehende Geräte die OTA-Datei verwenden. Keine Installation ausgeführt.
SHA-256, tatsächliche Partitionen, Komponentenoffsets und Validatorergebnisse
stehen in `SHA256SUMS.txt` und `BUILD_INFO_2.0.11-beta.5.1.json`.
