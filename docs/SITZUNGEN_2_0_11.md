# ChaosUhr 2.0.11 – Anmeldung, Sitzungen und MQTT-403

Diese Beschreibung gilt für das erneuerte lokale Testpaket 2.0.11. Veröffentlichte 2.0.10-Geräte und Updatekanäle werden dadurch nicht verändert. Reale Geräte-/Browsertests stehen noch aus.

## Bedienung

Die Hauptseite und die lokale OTA-Seite zeigen oben rechts den Anmeldestatus. Vor dem Zugriff auf Gerätedaten meldet man sich im Formular mit dem vorhandenen Admin-Passwort an. Das Passwort wird weder geändert noch neu eingerichtet. Nach erfolgreicher Anmeldung werden Einstellungen und andere geschützte Daten geladen.

**Abmelden** widerruft die aktuelle Sitzung auf dem Gerät, löscht ihr CSRF-Token und lässt den Browser das Sitzungs-Cookie entfernen. Alle Tabs mit diesem Cookie verlieren damit ihre Berechtigung. Eine andere, separat angemeldete Browsersitzung bleibt gültig. Bei einem Netzwerkfehler meldet die GUI den fehlgeschlagenen Logout, statt eine erfolgreiche serverseitige Abmeldung vorzutäuschen.

Nach **30 Minuten ohne bestätigte Aktivität** wird die Sitzung serverseitig ungültig. Gültige geschützte POST-Aktionen und ausdrücklich gemeldete Bedienaktivität zählen als Aktivität. Die GUI meldet echte Eingabeereignisse gedrosselt, höchstens einmal in 15 Sekunden. Reine GET-Abfragen – auch Status, MQTT-Status, Diagnose und Abruf des CSRF-Tokens – verlängern die Frist nicht. Die verbleibende Zeit kommt vom Gerät; der Browser-Ablauftimer prüft den Serverstatus ohne Verlängerung, damit tatsächliche Aktivität in einem anderen Tab berücksichtigt wird.

Nach Logout, Ablauf oder Neustart muss man sich erneut bewusst anmelden. Kein automatisches Wiederholen fehlgeschlagener Schreibaktionen und keine automatische Anmeldung mit gespeicherten Zugangsdaten. Ein Browser darf das Passwort über seinen eigenen Passwortmanager anbieten; die Firmware speichert es nicht im Browser.

## Server und Cookie

- Zwei feste Sitzungsplätze im RAM, zusammen höchstens 128 Byte. Keine Sitzungsdateien, keine EEPROM-/NVS-Schreibzugriffe und keine fortlaufenden Flash-Logs.
- Neue zufällige Sitzungskennung und ein davon getrenntes zufälliges CSRF-Token bei jeder erfolgreichen Anmeldung, jeweils 128 Bit aus dem Hardware-Zufallsgenerator. Eine vom Client vorgegebene Sitzungskennung wird nicht übernommen.
- Auf dem Gerät bleibt nur ein SHA-256-Fingerprint der Sitzungskennung. Für aktive Sitzungen wird zusätzlich das CSRF-Token gehalten. Bei Logout/Ablauf wird dieses gelöscht und der Eintrag deaktiviert. Begrenzte inaktive Fingerprints dienen nur der Fehlerunterscheidung und erlauben keine Anmeldung.
- Cookie: `ChaosUhrSID`, hostgebunden ohne `Domain`, `Path=/`, `HttpOnly`, `SameSite=Strict`. Es ist ein Browser-Sitzungscookie ohne festes `Max-Age`; die Inaktivitätsfrist prüft der Server. Beim ausdrücklichen Logout wird `Max-Age=0` gesendet. Fehlerantworten auf Hintergrundabfragen löschen keinen Cookie, damit eine spät eintreffende 401-Antwort keine inzwischen neu angemeldete Sitzung eines anderen Tabs überschreibt. Ein abgelaufener Cookie-Wert bleibt serverseitig ungültig, auch wenn er im Browser noch vorhanden ist.
- Nach Neustart sind alle RAM-Sitzungen weg; auch der Login-CSRF-Wert wird neu erzeugt. Kein alter Cookie oder CSRF-Wert kann damit eine Sitzung wiederherstellen.
- Bei zwei aktiven Sitzungen benötigt ein weiterer Browser einen freien Platz. Sonst folgt `AUTH-023`; eine andere Sitzung abmelden oder deren Ablauf abwarten. Verschiedene Uhradressen wie IP und mDNS-Name können getrennte Browser-Cookies und damit getrennte Sitzungen belegen.

Die früher verwendete HTTP-Basic-Authentifizierung ist für die öffentliche Firmware als Zugangsweg abgeschaltet. Es wird kein `WWW-Authenticate`-Challenge mehr gesendet; vorhandene Basic-Header werden nicht als Anmeldung akzeptiert. Ein alter Browsercache lässt sich serverseitig nicht löschen und kann solche Header noch mitsenden. Sie können weder einen Logout rückgängig machen noch eine Sitzung verlängern.

## CSRF und API

| Endpoint | Zugriff und Wirkung |
| --- | --- |
| `GET /api/login` | Liefert einen Boot-Login-CSRF-Wert, keine Admin-Sitzung. Origin-/Host-Prüfung und `no-store`. |
| `POST /api/login` | Genau ein Formularfeld `password`; Header `X-CSRF-Token` aus obiger Antwort und gültiger Origin/Host erforderlich. Passwortprüfung mit bisheriger Speicher-/Migrationslogik und Anmelderatenbegrenzung. Erfolgreich: Cookie und eigener Sitzungs-CSRF. |
| `GET /api/session` | Bei eingerichteter Uhr nur mit gültigem Cookie: `authenticated`, `token`, `idleTimeoutSeconds`, `remainingSeconds`. Verlängert nicht. |
| `POST /api/session/activity` | Leerer Formularbody, gültiger Cookie, eigener CSRF-Header und zulässiger Origin. Erneuert die Inaktivitätsfrist. |
| `POST /api/logout` | Leerer Formularbody und dieselben Schutzprüfungen. Widerruft die aktuelle Sitzung und ihr CSRF. |
| Geschützte GET-APIs | Cookie erforderlich, keine Fristverlängerung. |
| Einstellungen, MQTT, Uploads, Reset und OTA | Cookie plus zugehöriger CSRF-Header und zulässiger Origin. Prüfung vor Verarbeitung beziehungsweise Schreiben des Bodys bleibt erhalten. |

Das CSRF eines anderen Sitzungsplatzes ist ungültig. Sitzung und Berechtigung kommen ausschließlich aus dem Cookie; keine Sitzungskennungen in URL, Formularfeld oder Authorization-Fallback. Die statischen HTML-/Skriptdateien enthalten keine privaten Gerätedaten und können zur Darstellung des Loginformulars ohne Sitzung geladen werden. Gerätedaten und Schreibzugriffe sind weiterhin serverseitig geschützt.

Vor der erstmaligen Admin-Einrichtung behält `GET /api/session` seine bisherige, ausschließlich im geschützten Setup-AP erlaubte Bedeutung für das Einrichtungs-CSRF. Auch das WLAN-Recovery-Portal hat seine eigene AP-gebundene Boot-Berechtigung. Diese Einrichtungswerte sind keine Admin-Sitzung und können keine geschützte Geräteaktion nach Logout autorisieren.

| Zustand | HTTP | Code |
| --- | ---: | --- |
| Keine Admin-Sitzung | 401 | `AUTH-001` |
| Falsches Anmeldepasswort | 401 | `AUTH-002` |
| CSRF fehlt, ist falsch oder gehört zu anderer Sitzung | 403 | `AUTH-003` |
| Origin gehört nicht zum lokalen Host/Port | 403 | `AUTH-004` |
| Erkennbar abgelaufene Sitzung | 401 | `AUTH-020` |
| Widerrufene, unbekannte oder nach Neustart alte Sitzung | 401 | `AUTH-021` |
| Zu langer, doppelter oder fehlerhaft formatierter Cookie | 401 | `AUTH-022` |
| Zwei aktive RAM-Sitzungsplätze belegt | 429 | `AUTH-023` |
| Unzulässige Anmeldeformulardaten | 400 | `AUTH-024` |

Nach Wiederverwendung eines begrenzten RAM-Platzes ist ein früherer Cookie nicht mehr als abgelaufen erkennbar und wird als unbekannt (`AUTH-021`) abgelehnt. Vollständige Meldungen, Ursachen und Maßnahmen stehen in [FEHLERCODES.md](FEHLERCODES.md). HTTP-Status und ChaosUhr-Code bleiben getrennt.

## Unabhängiger MQTT-403-Bugfix

Der vorherige Origin-Schutz verglich komplette Texte. Zum Beispiel wurden `Host: ChaosUhr.local:80` und `Origin: http://chaosuhr.local` abgelehnt, obwohl Hostnamen nicht nach Groß-/Kleinschreibung unterschieden werden und 80 der HTTP-Standardport ist. Eine tatsächliche MQTT-Speicherfunktion mit akzeptierter Anmeldung und CSRF lieferte dadurch 403. Dieser Negativlauf wurde **vor** dem Sitzungsumbau separat aufgezeichnet.

Die Korrektur normalisiert die Authority für den Vergleich: gleicher Host ohne Beachtung der ASCII-Großschreibung und gleicher effektiver Port. Fremde Hosts/Ports, `null`, Userinfo, Pfade und fehlerhafte Authorities bleiben unzulässig. Fehlender Origin bleibt ausschließlich mit gültiger Anmeldung und CSRF möglich. Diese Regeln entsprechen dem Origin-Modell aus [RFC 6454](https://www.rfc-editor.org/rfc/rfc6454.html#section-5).

Die unabhängige MQTT-API-Regression besteht nach der Korrektur ohne neue Loginlogik. Zusätzlich ist derselbe Fall mit tatsächlichem `PublicAccess`, gültigem Sitzungs-Cookie und allen 24 MQTT-GUI-Feldern für beide Plattformadapter geprüft. Ungültige Sitzung oder CSRF werden vor dem Speichern abgewiesen; gültige alte Datei und Konfiguration bleiben erhalten. Es wird niemals zur Fehlerbehebung automatisch formatiert.

Der ursprüngliche reale HTTP-403-Mitschnitt der betroffenen Uhr liegt nicht vor. Deshalb wird nicht behauptet, die genaue ursprüngliche Host-/Origin-Kombination oder einen Broker-/Dateisystemfehler am Gerät nachgewiesen zu haben. Der hier beschriebene 403-Pfad ist reproduziert und behoben; Login allein wäre dafür keine Korrektur.

## Grenzen ohne TLS und Geheimnisschutz

Die Uhr verwendet weiterhin lokales **HTTP**. Deshalb wird kein `Secure`-Attribut gesetzt: Ein solches Cookie wäre für den normalen HTTP-Betrieb ungeeignet. `HttpOnly` erschwert JavaScript-Zugriff auf den Cookie; `SameSite` und CSRF reduzieren fremde Browseranfragen. Diese Eigenschaften verschlüsseln weder Passwort noch Sitzung und schützen nicht vor Mitschneiden oder Manipulation im Netz. Grundlagen: [RFC 6265, Secure/HttpOnly](https://www.rfc-editor.org/rfc/rfc6265.html#section-4.1.2.5), [OWASP Session Management](https://cheatsheetseries.owasp.org/cheatsheets/Session_Management_Cheat_Sheet.html).

Nur in einem vertrauenswürdigen lokalen Netz verwenden. Es wurde kein TLS-Zugang oder Reverse-Proxy eingerichtet und kein Netzwerkgerät verändert. MQTT bleibt ebenfalls unverändert lokales TCP ohne TLS.

Sitzungskennungen, CSRF-/Loginwerte und Passwörter werden nicht seriell ausgegeben oder in die Supportdiagnose aufgenommen. Auch optionale Debugausgaben der projektlokalen HTTP-Parser wurden für Request-/Header-/Body-/Authentifizierungsinhalte entfernt. Fehlerlogs bestehen aus festen Katalogtexten und Uptime. Supportdiagnose und ihre Zugriffskontrolle bleiben erhalten.

## Nachweise und offene Prüfungen

- Tatsächliche PublicAccess-/MQTT-Handler: Login, Logout, erneuter Login, Cookie-/CSRF-Rotation, 30-Minuten-Ablauf trotz wiederholter GETs, echte Aktivität, ungültige/abgelaufene/mehrfach angegebene Cookies, Neustart, alte Boot-Challenge, millis-Überlauf und Zwei-Sitzungs-Grenze auf beiden nativen Plattformadaptern.
- MQTT-403-Negativkontrolle vor dem Umbau und eigenständiger Nachher-Test; zusätzlich Speicherung mit gültiger und Ablehnung mit ungültiger Sitzung/CSRF und vollständigen 24 Formularfeldern.
- Beide tatsächlichen HTTP-Parser: Cookie korrekt gesammelt und nachfolgend ohne Cookie kein alter Wert weiterverwendet; bisherige Body-/Uploadgrenzen und Vorabprüfung bestehen weiter.
- GUI: Status auf Haupt-/OTA-Seite, expliziter Login/Logout, gescheiterter Logout ohne vorgetäuschten Erfolg, erneuter Login, Ablauf, keine Verlängerung durch Polling/künstliche Events und keine ungeprüften Passwort-/Fehlertextausgaben.

Buildgrößen, vollständige Regressionsergebnisse und finale Testpaket-Prüfsummen: [Entwicklungsbericht](ENTWICKLUNGSBERICHT_2_0_11.md). Echte Browser-/Gerätetests von Login/Logout/Ablauf, MQTT-Speicherung, Folge-OTA, Kaltstarts und Langzeitbetrieb bleiben offen; Hosttests sind kein Hardwaretest.
