# ChaosUhr – stabile Fehlercodes

Quelle: `include/error_catalog.def`; automatisch mit `python3 scripts/generate_error_docs.py` erzeugt. Codes niemals umnummerieren oder für andere Ursachen wiederverwenden. Neue Codes mit neuer freier Nummer ergänzen; anschließend Generator und Prüfungen ausführen.

HTTP-Status und ChaosUhr-Code sind getrennte Angaben. Meldungen enthalten ausschließlich feste, bereinigte Texte. Die letzte Meldung und acht Ereignisse liegen im RAM, ohne laufende Flash-Schreibzugriffe. Wiederholungen werden gezählt und zusammengefasst. Ein letzter Fehler ist ein historischer Eintrag, kein Beweis für einen weiterhin aktiven Defekt.

| Code | Komponente | Bedeutung | Technische Ursache | Maßnahme |
| --- | --- | --- | --- | --- |
| AUTH-020 | AUTH | Admin-Sitzung abgelaufen | Seit der letzten bestätigten Aktivität sind mindestens 30 Minuten vergangen; Sitzung und CSRF sind ungültig. | Erneut über das Anmeldeformular anmelden; Hintergrundabfragen verlängern die Sitzung nicht. |
| AUTH-021 | AUTH | Admin-Sitzung ungültig | Die Sitzung wurde abgemeldet, ersetzt oder durch einen Geräteneustart ungültig. | Erneut anmelden und die gewünschte Aktion anschließend bewusst wiederholen. |
| AUTH-022 | AUTH | Sitzungs-Cookie ungültig | Der Cookie ist zu lang, mehrfach angegeben oder enthält kein gültiges Sitzungsformat. | Seite neu öffnen und erneut anmelden; keine Sitzungstokens manuell ändern. |
| AUTH-023 | AUTH | Alle Admin-Sitzungen belegt | Die zwei RAM-Sitzungsplätze werden bereits von aktiven Anmeldungen verwendet. | Eine andere Sitzung abmelden oder deren Ablauf abwarten. |
| AUTH-024 | AUTH | Anmeldeformular ungültig | Die Anmeldung erwartet genau ein Passwortfeld im Formular. | Das Anmeldeformular der aktuellen Weboberfläche verwenden. |
| MQTT-021 | MQTT | MQTT-Transport wartet zu lange auf Bestätigung | AsyncTCP meldet einen ACK-Timeout; die Brokerverbindung ist damit nicht nachweislich getrennt. | Netzwerk, Brokerlast und TCP-Verbindung prüfen; Diagnose sichern. |
| SYS-007 | SYS | Unerwarteter Fehler in der Weboberfläche | Eine lokale Browseroperation ist fehlgeschlagen; eine genauere Ursache ist nicht erkennbar. | Seite neu laden, Firmwareversion prüfen und bereinigte Diagnose sichern. |
| FS-019 | FS | Speicherreset nicht abgeschlossen | Der Reset-Rückruf meldet einen Fehler ohne genauere Ursache. | Bereinigte Diagnose prüfen; Reset-Merkmal bleibt erhalten. |
| HTTP-018 | HTTP | Diagnoseskript konnte nicht geladen werden | Der Browser konnte das gemeinsame lokale Skript nicht laden; die genauere Ursache ist nicht erkennbar. | Uhradresse und Netzwerk prüfen und Seite neu laden. |
| AUTH-018 | AUTH | Anmeldeheader ungültig | HTTP Basic enthält keinen gültigen Admin-Benutzer mit zulässiger Kodierung und Länge. | Weboberfläche neu öffnen und als admin anmelden. |
| AUTH-019 | AUTH | Gespeicherter Admin-Zugang beschädigt | Die vorhandenen Zugangsdaten haben ein unbekanntes oder beschädigtes Format. | Diagnose sichern; bestehende Daten nicht durch eine Neuregistrierung überschreiben. |
| SYS-006 | SYS | Werksreset läuft | Ein ausdrücklich bestätigter Werksreset ist noch nicht abgeschlossen. | Stromversorgung erhalten und Abschluss abwarten. |
| HTTP-017 | HTTP | Setup-Host nicht erlaubt | Der Host-Header entspricht nicht der lokalen Einrichtungsadresse. | Die Setup-Seite über die lokale AP-Adresse öffnen. |
| MQTT-020 | MQTT | MQTT-Berechtigung abgewiesen | Der Broker sendet CONNACK 5: not authorized. | Zugriffsrechte und Brokerregeln für den MQTT-Benutzer prüfen. |
| SYS-005 | SYS | Kryptographische Operation fehlgeschlagen | Die Hash- oder HMAC-Implementierung konnte die Berechnung nicht abschließen. | Freien Heap und Firmware prüfen; Diagnose sichern. |
| OTA-020 | OTA | Zusätzliche Daten nach dem Imageende | Das Image enthält nicht unterstützte nachgestellte Daten. | Eine unveränderte passende Anwendungs-BIN verwenden. |
| OTA-021 | OTA | Firmware-Padding ungültig | Ein reserviertes Auffüllbyte des Images ist nicht null. | Firmwaredatei erneut von der vorgesehenen Quelle laden. |
| CFG-020 | CFG | Zahlenformat ungültig | Der Parameter erwartet eine ganze Zahl ohne Vorzeichen oder Dezimalpunkt. | Zulässige ganze Zahl eingeben. |
| CFG-021 | CFG | Wert außerhalb des erlaubten Bereichs | Die ganze Zahl überschreitet den zulässigen Wertebereich. | Wertebereich des Eingabefelds beachten. |
| CFG-022 | CFG | Texteingabe zu lang | Die Eingabe überschreitet die feste Feldlänge. | Eingabe auf die angegebene Länge kürzen. |
| CFG-023 | CFG | Ungültiges Zeichen im Textfeld | Die Eingabe enthält ein Nullbyte oder unzulässige Steuerzeichen. | Nur zulässige Textzeichen verwenden. |
| CFG-024 | CFG | MQTT-Topic ungültig | Das Topic enthält Wildcards oder unzulässige Zeichen. | Ein konkretes ASCII-Topic ohne + oder # eingeben. |
| CFG-025 | CFG | Broker-Hostname ungültig | Die Brokeradresse verletzt die erlaubten Host-Zeichenregeln. | Nur Hostname oder IP ohne Schema und ohne Leerzeichen eingeben. |
| CFG-026 | CFG | LED-Pin nicht erlaubt | Die GPIO-Nummer gehört nicht zur erlaubten Liste des Boards. | Einen Pin aus dem Dropdown verwenden. |
| MQTT-019 | MQTT | MQTT-TLS-Prüfung fehlgeschlagen | Die Bibliothek meldet einen abweichenden TLS-Fingerabdruck. | Broker-Zertifikat und Firmware-Konfiguration prüfen; diese Fassung verwendet standardmäßig TCP ohne TLS. |
| WIFI-001 | WIFI | Keine gespeicherte WLAN-Konfiguration | Der Stationsspeicher enthält keine SSID. | Heim-WLAN im geschützten Setup-Portal einrichten. |
| WIFI-002 | WIFI | WLAN-Verbindung nicht hergestellt | Die Verbindungsfrist ist abgelaufen; das beweist keinen Verlust der Zugangsdaten. | Router, Reichweite und WLAN-Zugang prüfen; gespeicherte Daten bleiben erhalten. |
| WIFI-003 | WIFI | WLAN-Verbindung unterbrochen | Eine zuvor bestehende Stationsverbindung ist ausgefallen. | Router und Empfang prüfen; die Uhr versucht die Wiederverbindung. |
| WIFI-004 | WIFI | WLAN-Start fehlgeschlagen | Stationsschnittstelle oder SDK-Verbindungsstart meldet einen Fehler. | Neu starten und Diagnose sichern; Zugangsdaten nicht vorschnell löschen. |
| WIFI-005 | WIFI | Einrichtungs-WLAN konnte nicht starten | Das SDK hat den AP-Start abgelehnt. | Neu starten und Stromversorgung prüfen. |
| WIFI-006 | WIFI | Setup-Portal konnte nicht starten | WiFiManager konnte das Portal nicht öffnen. | WLAN-Diagnose prüfen und erneut versuchen. |
| WIFI-007 | WIFI | WLAN-Löschung fehlgeschlagen | Der ausdrücklich angeforderte Werksreset konnte WLAN-Daten nicht löschen. | Stromversorgung prüfen; der Reset-Auftrag bleibt erhalten. |
| WIFI-008 | WIFI | Namensdienst konnte nicht starten | mDNS-Registrierung ist fehlgeschlagen. | Die angezeigte IP-Adresse statt des Gerätenamens verwenden. |
| MQTT-001 | MQTT | Broker-Hostname nicht auflösbar | AsyncTCP meldet DNS-Auflösung fehlgeschlagen (-55). | Broker-Hostname und DNS im lokalen Netz prüfen. |
| MQTT-002 | MQTT | MQTT-Server nicht erreichbar | Der Transport meldet einen konkreten TCP-Verbindungsfehler. | Broker-Verfügbarkeit, Netzwerk und Port prüfen. |
| MQTT-003 | MQTT | MQTT-Verbindungsaufbau abgelaufen | Transport oder Verbindungsfrist meldet einen Timeout. | Broker-Port, Firewall und Netzwerk prüfen. |
| MQTT-004 | MQTT | Broker lehnt die Verbindung ab | Der Broker hat eine negative CONNACK-Antwort gesendet. | Broker-Protokoll und zulässige Client-Verbindungen prüfen. |
| MQTT-005 | MQTT | Broker lehnt die Anmeldung ab | CONNACK meldet ungültige Zugangsdaten oder fehlende Berechtigung. | MQTT-Benutzer, Passwort und Broker-Rechte prüfen. |
| MQTT-006 | MQTT | MQTT-Verbindung unterbrochen | Eine zuvor bestätigte Brokerverbindung ist ausgefallen. | Broker und Netzwerk prüfen; die Uhr verbindet sich erneut. |
| MQTT-007 | MQTT | MQTT-Konfiguration ungültig | Mindestens eine Konfigurationsregel ist verletzt. | Broker, Port, Topics und Wertebereiche prüfen. |
| MQTT-008 | MQTT | MQTT-Konfiguration nicht gespeichert | Speicherung fehlgeschlagen; keine genauere Ursache verfügbar. | Dateisystemstatus prüfen und Diagnose sichern. |
| MQTT-009 | MQTT | MQTT-Konfiguration nicht geladen | Laden fehlgeschlagen; keine genauere Ursache verfügbar. | Dateisystemstatus prüfen und Konfiguration erneut speichern. |
| MQTT-010 | MQTT | MQTT-Verbindungsfehler ohne genauere Ursache | Die Bibliothek meldet nur TCP_DISCONNECTED; DNS/TCP/Timeout sind damit nicht unterscheidbar. | Broker und Netzwerk prüfen; die Diagnose enthält keine geratene Ursache. |
| MQTT-011 | MQTT | MQTT-Nachricht konnte nicht gepuffert werden | Die Bibliothek meldet Speicher- oder Warteschlangenfehler. | Nachrichtenrate reduzieren und freien Heap prüfen. |
| MQTT-012 | MQTT | MQTT-Abonnement abgelehnt | Der Broker hat mindestens ein Topic-Abonnement abgelehnt. | Topic-Namen und Leserechte im Broker prüfen. |
| MQTT-013 | MQTT | MQTT-Nutzdaten ungültig | Wetter- oder Sensorpayload entspricht keinem unterstützten Format. | Payload nach MQTT-Anleitung korrigieren. |
| MQTT-014 | MQTT | MQTT-Nachricht zu groß | Die Nachricht überschreitet die feste Empfangsgrenze. | Höchstens 512 Payload-Bytes und 95 Topic-Zeichen senden. |
| MQTT-015 | MQTT | MQTT-Empfangswarteschlange voll | Eine ältere Nachricht musste verworfen werden. | Nachrichtenrate reduzieren. |
| MQTT-016 | MQTT | MQTT-Fragmente unvollständig oder widersprüchlich | Fragmentfolge, Topic oder deklarierte Länge passen nicht zusammen. | Publisher und Broker prüfen; nur vollständige Nachrichten werden verarbeitet. |
| MQTT-017 | MQTT | MQTT-Protokollversion abgelehnt | CONNACK meldet nicht unterstützte Protokollversion. | MQTT-3.1.1-Unterstützung am Broker prüfen. |
| MQTT-018 | MQTT | MQTT-Clientkennung abgelehnt | CONNACK meldet abgelehnte Client-ID. | Brokerregeln und doppelte Gerätekennungen prüfen. |
| FS-001 | FS | Datenspeicher nicht lesbar | LittleFS konnte ohne Formatierung nicht eingebunden werden. | Vorhandene Dateien sichern; Initialisierung nur ausdrücklich bestätigen. |
| FS-002 | FS | Datei konnte nicht zum Schreiben geöffnet werden | Die temporäre Datei konnte nicht angelegt werden. | Freien Speicher und Dateisystemzustand prüfen. |
| FS-003 | FS | Datei nicht vollständig geschrieben | Die Schreiboperation lieferte weniger Bytes als angefordert. | Freien Speicher und Flash prüfen; bisherige Konfiguration bleibt erhalten. |
| FS-004 | FS | Prüfdatei konnte nicht geöffnet werden | Die geschriebene temporäre Datei ließ sich nicht erneut öffnen. | Dateisystem prüfen und erneut speichern. |
| FS-005 | FS | Prüfdatei unvollständig gelesen | Länge oder Read-Ergebnis der temporären Datei ist falsch. | Dateisystem prüfen und erneut speichern. |
| FS-006 | FS | Prüfdatei stimmt nicht überein | Die zurückgelesenen Bytes unterscheiden sich vom Schreibauftrag. | Flash und Stromversorgung prüfen. |
| FS-007 | FS | Atomarer Dateiaustausch fehlgeschlagen | LittleFS konnte die geprüfte temporäre Datei nicht umbenennen. | Dateisystem und freien Speicher prüfen; alte Datei bleibt maßgeblich. |
| FS-008 | FS | Datei konnte nicht gelöscht werden | LittleFS remove meldet einen Fehler. | Dateisystem prüfen; vorhandene Dateien sichern. |
| FS-009 | FS | Datei konnte nicht gelesen werden | Eine vorhandene Datei konnte nicht geöffnet oder gelesen werden. | Dateisystem prüfen und Daten sichern. |
| FS-010 | FS | Gespeicherte MQTT-Konfiguration beschädigt | Größe, Prüfsumme oder Konfigurationsformat sind ungültig. | Konfiguration prüfen und ausdrücklich neu speichern; keine automatische Löschung. |
| FS-011 | FS | Bestätigte Speicherinitialisierung fehlgeschlagen | Die ausdrücklich angeforderte Formatierung ist fehlgeschlagen. | Flash und Stromversorgung prüfen. |
| FS-012 | FS | Verzeichnis konnte nicht angelegt werden | mkdir für den Animationsspeicher ist fehlgeschlagen. | Dateisystem und freien Speicher prüfen. |
| FS-013 | FS | Datei nicht vollständig lesbar | Seek oder Read einer Datei ist fehlgeschlagen. | Datei sichern und Speicher prüfen. |
| FS-014 | FS | Datenspeicher bereits eingerichtet | Eine erneute Initialisierung wurde abgewiesen. | Vorhandenen Speicher ohne Formatierung verwenden. |
| FS-015 | FS | Datenspeicherreserve erreicht | Für Konfiguration und Zugangsjournal muss Reserve frei bleiben. | Nicht benötigte eigene Animationen löschen. |
| FS-016 | FS | Sicherheitsjournal beschädigt | Das vorhandene Journal besteht die Integritätsprüfung nicht. | Diagnose sichern; keine automatische Formatierung oder Neuregistrierung. |
| FS-017 | FS | EEPROM-Spiegel nicht aktualisiert | Das gültige Journal ist dauerhaft, der EEPROM-Spiegel konnte nicht geschrieben werden. | Stromversorgung prüfen; Wiederherstellung erfolgt aus dem Journal. |
| FS-018 | FS | EEPROM konnte nicht gespeichert werden | Die Speicherbibliothek meldet Commit fehlgeschlagen. | Stromversorgung und Flash prüfen. |
| HTTP-001 | HTTP | HTTP-Methode nicht erlaubt | Für diesen Endpunkt ist POST erforderlich. | Weboberfläche neu laden und vorgesehene Aktion verwenden. |
| HTTP-002 | HTTP | HTTP-Anfrage ungültig | Request-Zeile oder Protokollformat ist ungültig. | Browser neu laden; keine selbst gebauten unvollständigen Anfragen senden. |
| HTTP-003 | HTTP | HTTP-Header zu groß | Headerzahl, Zeilenlänge oder Gesamtgröße überschreitet eine Grenze. | Anfrage mit begrenzten Headern erneut senden. |
| HTTP-004 | HTTP | Content-Length ungültig | Längenangabe fehlt, ist doppelt oder enthält ungültige Zeichen. | Vollständige HTTP-Anfrage mit genau einer gültigen Länge senden. |
| HTTP-005 | HTTP | HTTP-Inhalt zu groß | Der Request-Body überschreitet die Endpunktgrenze. | Datei oder Formularinhalt verkleinern. |
| HTTP-006 | HTTP | Transfer-Encoding nicht unterstützt | Chunked Transfer-Encoding wird nicht akzeptiert. | Eine Anfrage mit bekannter Content-Length verwenden. |
| HTTP-007 | HTTP | HTTP-Verbindung fehlgeschlagen | Der Browser konnte keine verwertbare Antwort empfangen; die Ursache ist nicht genauer erkennbar. | Uhradresse, Netzwerk und Erreichbarkeit prüfen. |
| HTTP-008 | HTTP | HTTP-Zeitlimit überschritten | Eine HTTP-Operation hat ihre Zeitgrenze überschritten. | Verbindung prüfen und Status vor einem erneuten Schreibversuch lesen. |
| HTTP-009 | HTTP | Leere HTTP-Antwort | Eine erwartete strukturierte Antwort enthält keinen Inhalt. | Status und Anmeldung prüfen; Seite neu laden. |
| HTTP-010 | HTTP | HTTP-Antwort enthält kein gültiges JSON | Die Antwort konnte nicht als erwartetes JSON gelesen werden. | Uhradresse und Firmwareversion prüfen; Seite neu laden. |
| HTTP-011 | HTTP | HTTP-Fehler ohne detaillierte Diagnose | Der Server meldet einen Fehlerstatus ohne bekannten ChaosUhr-Code. | HTTP-Status notieren und Supportdiagnose sichern. |
| HTTP-012 | HTTP | HTTP-Antwort abgebrochen | Das Lesen des Antwortinhalts ist fehlgeschlagen. | Netzwerk prüfen und Gerätestatus vor Wiederholung lesen. |
| HTTP-013 | HTTP | Zu viele Formularparameter | Die feste Parametergrenze wurde überschritten. | Aktuelle Weboberfläche neu laden. |
| HTTP-014 | HTTP | Multipart-Begrenzung ungültig | Boundary fehlt, ist zu lang oder unzulässig. | Datei über das normale Uploadformular senden. |
| HTTP-015 | HTTP | HTTP-Endpunkt nicht gefunden | Für diesen Pfad ist kein Handler registriert. | Aktuelle Weboberfläche öffnen. |
| HTTP-016 | HTTP | HTTP-Adresse zu lang | Die Request-URI überschreitet die feste Grenze. | Eine kürzere Adresse verwenden. |
| AUTH-001 | AUTH | Anmeldung erforderlich | Die Anfrage enthält keine akzeptierte Admin-Anmeldung. | Weboberfläche neu öffnen und als admin anmelden. |
| AUTH-002 | AUTH | Admin-Anmeldung abgewiesen | Die übermittelten Anmeldedaten wurden nicht akzeptiert. | Admin-Benutzer und Passwort prüfen; danach Seite neu laden. |
| AUTH-003 | AUTH | Sitzungstoken ungültig oder abgelaufen | Das Mutationstoken fehlt oder gehört nicht zur aktuellen Gerätesitzung. | Seite nach einem Neustart neu laden und Aktion erneut ausführen. |
| AUTH-004 | AUTH | Fremde Webseite abgewiesen | Origin passt nicht zum Host der lokalen Uhr. | Die Weboberfläche direkt über die Uhradresse öffnen. |
| AUTH-005 | AUTH | Admin-Zugang noch nicht verfügbar | Die Uhr ist nicht eingerichtet, gesperrt oder setzt sich zurück. | Setup-Anzeige und Gerätestatus prüfen. |
| AUTH-006 | AUTH | Zu viele Anmeldeversuche | Die begrenzte Anmelderate ist überschritten. | Mindestens fünf Sekunden warten und Zugang prüfen. |
| AUTH-007 | AUTH | Anmeldeprüfung nicht verfügbar | Die kryptographische Prüfung ist fehlgeschlagen. | Gerät neu starten und bereinigte Diagnose sichern. |
| AUTH-008 | AUTH | Ersteinrichtung nicht erlaubt | Neuregistrierung ist nur ohne bestehenden Admin im Setup-WLAN erlaubt. | Setup-WLAN verwenden; bestehenden Zugang nicht überschreiben. |
| AUTH-009 | AUTH | Einrichtung nur im Setup-WLAN | Die Anfrage kam nicht über den erlaubten AP-Zugang. | Mit dem auf der Matrix angezeigten Setup-WLAN verbinden. |
| AUTH-010 | AUTH | Zugriff abgewiesen | Die Anfrage hat keine ausreichende Berechtigung; genauere Ursache fehlt. | Anmeldung und geöffnete Uhradresse prüfen. |
| AUTH-011 | AUTH | Admin-Passwortformat ungültig | Es sind 16 bis 64 druckbare ASCII-Zeichen erforderlich. | Ein passendes Passwort eingeben, ohne es in Supportdaten zu teilen. |
| AUTH-012 | AUTH | Passwortwiederholung stimmt nicht überein | Die beiden eingegebenen Passwörter unterscheiden sich. | Beide Passwortfelder erneut eingeben. |
| AUTH-013 | AUTH | Admin-Zugang nicht gespeichert | Die Zugangsdaten konnten nicht dauerhaft gespeichert werden. | Speicherdiagnose prüfen; vorhandenen Zugang erhalten. |
| AUTH-014 | AUTH | Ausdrückliche Bestätigung fehlt | Die destruktive Aktion wurde nicht vollständig bestätigt. | Seite neu laden und Bestätigungsfelder bewusst ausfüllen. |
| AUTH-015 | AUTH | Werksreset konnte nicht vorbereitet werden | Das dauerhafte Reset-Merkmal konnte nicht gespeichert werden. | Speicher prüfen; kein Reset wird als abgeschlossen gemeldet. |
| AUTH-016 | AUTH | Portalaktion gesperrt | Die aufgerufene Verwaltungsaktion ist im WLAN-Portal nicht erlaubt. | Normale Admin-Oberfläche im Heimnetz verwenden. |
| AUTH-017 | AUTH | Passwortfelder fehlen | Passwort und Wiederholung wurden nicht vollständig übertragen. | Aktuelle Setup-Seite neu laden. |
| OTA-001 | OTA | Genau eine Firmwaredatei erforderlich | Es wurden mehrere Dateiabschnitte übertragen. | Eine einzelne passende Anwendungs-BIN auswählen. |
| OTA-002 | OTA | Falsches Firmware-Dateifeld | Es wird eine unkomprimierte Anwendungs-BIN im Feld firmware erwartet. | Normales Uploadformular und passende BIN verwenden. |
| OTA-003 | OTA | Ungültige erwartete SHA-256 | Die übergebene Prüfsumme hat kein gültiges Format. | Manifest und Downloadquelle prüfen. |
| OTA-004 | OTA | Firmware überschreitet OTA-Grenze | Die vollständige Anwendungsdatei passt nicht in den Zielslot. | Passende Anwendungs-BIN verwenden; kein Factory-Image hochladen. |
| OTA-005 | OTA | Firmwareheader ungültig | Der Imageheader hat kein unterstütztes Format. | Unkomprimierte passende Anwendungs-BIN verwenden. |
| OTA-006 | OTA | Firmware passt nicht zum Zielchip oder Layout | Zielchip, Flashmodus oder Flashlayout stimmt nicht überein. | Firmware für genau das eigene Board auswählen. |
| OTA-007 | OTA | OTA-Speicher nicht bereit | Update.begin ist fehlgeschlagen oder es fehlt Slotreserve. | Freien OTA-Platz und passende Partitionen prüfen. |
| OTA-008 | OTA | Firmware konnte nicht geschrieben werden | Update.write lieferte eine unvollständige Schreibmenge. | Stromversorgung und Flash prüfen; Gerät wurde nicht aktiviert. |
| OTA-009 | OTA | Firmwareabschluss fehlgeschlagen | Update.end hat die Aktivierung abgelehnt. | Diagnose sichern und Firmwareintegrität prüfen. |
| OTA-010 | OTA | Firmwareübertragung abgebrochen | Die Übertragung wurde vor dem regulären Abschluss beendet. | Gerätestatus prüfen; nur nach bewusster Entscheidung erneut übertragen. |
| OTA-011 | OTA | Firmware unvollständig | Deklarierte Länge, Imageende oder vollständiger Upload fehlt. | Download und Netzwerk prüfen. |
| OTA-012 | OTA | Firmware-Prüfsumme ungültig | Core-CRC oder Segmentprüfsumme stimmt nicht. | Datei erneut von der vorgesehenen Quelle laden. |
| OTA-013 | OTA | Firmware-SHA stimmt nicht überein | Image-Hash oder erwartete Transport-SHA ist falsch. | Datei und Manifest nicht mischen; Quelle prüfen. |
| OTA-014 | OTA | ChaosUhr-Produktkennung fehlt | Das Image enthält keine passende ChaosUhr-Produktkennung. | Passende ChaosUhr-Anwendungs-BIN ab 2.0.8 verwenden. |
| OTA-015 | OTA | Unerwartete Firmware-Formularfelder | Die Firmwareanfrage enthält zusätzliche Felder. | Nur das normale Uploadformular verwenden. |
| OTA-016 | OTA | Update-Manifest ungültig | Version, Schema oder Downloadangaben sind nicht gültig. | Updatequelle prüfen; vorhandene Firmware bleibt aktiv. |
| OTA-017 | OTA | Updatekanal nicht erreichbar | Der externe Server lieferte keinen erfolgreichen HTTP-Status. | Internetzugang und GitHub-Verfügbarkeit prüfen. |
| OTA-018 | OTA | Firmwaredownload unvollständig | Downloadlänge oder Dateikopf passt nicht zum Manifest. | Download erneut prüfen; keine Aktivierung erfolgt. |
| OTA-019 | OTA | Firmware ist nicht neuer | Ein erneutes Update oder Downgrade wurde abgewiesen. | Aktuellen Versionsstatus prüfen. |
| CFG-001 | CFG | Keine Einstellungen angegeben | Die Konfigurationsanfrage enthält keine Werte. | Formular neu laden und erneut ausfüllen. |
| CFG-002 | CFG | Unbekannter Konfigurationsparameter | Ein übertragener Feldname gehört nicht zur aktuellen API. | Aktuelle Weboberfläche neu laden. |
| CFG-003 | CFG | Konfigurationswert ungültig | Ein bekannter Parameter verletzt Format oder Wertebereich. | Eingabe und zulässigen Wertebereich prüfen. |
| CFG-004 | CFG | Doppelter Konfigurationsparameter | Derselbe Feldname wurde mehrfach gesendet. | Aktuelle Weboberfläche verwenden. |
| CFG-005 | CFG | Keine gültige Anzeigeart gewählt | Mindestens eine zulässige Anzeigeart muss aktiv sein. | Uhr, Animationen, Wetter oder Sensoren wählen. |
| CFG-006 | CFG | Keine verfügbare Animation gewählt | Die aktive Animationskategorie hat keine verfügbare Auswahl. | Mindestens eine vorhandene Animation wählen oder Kategorie abschalten. |
| CFG-007 | CFG | Animation nicht verfügbar | Die ausgewählte Upload-ID fehlt oder ist nicht abspielbar. | Katalog neu laden und Animation prüfen. |
| CFG-008 | CFG | Uploads auf diesem Build nicht verfügbar | Die Konfiguration benötigt eine nicht vorhandene Upload-Funktion. | Passende Firmware und verfügbare Animationen verwenden. |
| CFG-009 | CFG | Uhreinstellungen nicht gespeichert | Die dauerhafte Speicherung wurde abgelehnt. | Speicherdiagnose prüfen; laufende Einstellungen bleiben erhalten. |
| CFG-010 | CFG | Einstellungssicherung ungültig | Schema, Inhalt oder Zuordnung der Sicherung ist ungültig. | Eine unveränderte passende Sicherung verwenden. |
| CFG-011 | CFG | MQTT-Passwortaktion widersprüchlich | Ersetzen und Löschen wurden gleichzeitig angefordert. | Entweder neues Passwort setzen oder gespeichertes Passwort löschen. |
| CFG-012 | CFG | Genau eine Animation erforderlich | Mehrere Animationsdateien wurden in einer Anfrage übertragen. | Eine einzelne .chaos-Datei auswählen. |
| CFG-013 | CFG | Falsches Animations-Dateifeld | Es wird eine .chaos-Datei im Feld animation erwartet. | Normales Uploadformular verwenden. |
| CFG-014 | CFG | Alle Animationsplätze belegt | Acht eigene Dateien sind bereits vorhanden. | Nicht benötigte eigene Animation zuerst löschen. |
| CFG-015 | CFG | Animation größer als 24 KiB | Die feste Dateigrößengrenze wurde überschritten. | Animation verkürzen oder Palette und Bilder vereinfachen. |
| CFG-016 | CFG | Animationsupload abgebrochen | Es wurde keine vollständige Upload-Anfrage abgeschlossen. | Status prüfen und vollständige Datei erneut senden. |
| CFG-017 | CFG | Animationsdatei unvollständig | Dateidaten oder Upload-Abschluss fehlen. | Eine vollständige .chaos-Datei auswählen. |
| CFG-018 | CFG | Animationsdatei nicht gefunden | Für die angefragte ID existiert kein Upload. | Animationsliste neu laden. |
| CFG-019 | CFG | Ungültige gespeicherte Anzeigeoptionen | Eine gespeicherte Struktur besteht ihre Validierung nicht. | Angezeigte Standardwerte prüfen und bei Bedarf speichern. |
| SYS-001 | SYS | Gerät startet neu | Ein ausdrücklich angeforderter Neustart läuft. | Kurz warten und Seite neu laden. |
| SYS-002 | SYS | Nicht genügend Arbeitsspeicher | Eine notwendige Speicherreservierung ist fehlgeschlagen. | Last reduzieren und freien Heap prüfen. |
| SYS-003 | SYS | Unerwartete Speicherstrukturgröße | Die interne Speichergröße ist nicht kompatibel. | Passende Firmware verwenden und Diagnose sichern. |
| SYS-004 | SYS | Nicht genauer bestimmbarer interner Fehler | Die Komponente liefert keinen spezifischeren Fehler. | Bereinigte Supportdiagnose sichern. |
| CFG-101 | CFG | Animationsformat ungültig | CHAOSUHR 1 erwartet (ohne Markdown/BOM). | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-102 | CFG | Animationsformat ungültig | NAME mit 1 bis 32 Zeichen erwartet. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-103 | CFG | Animationsformat ungültig | Name: nur Buchstaben, Zahlen, Leerzeichen, _.- | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-104 | CFG | Animationsformat ungültig | PALETTE 1 bis 16 erwartet. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-105 | CFG | Animationsformat ungültig | Farbe als #RRGGBB erwartet. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-106 | CFG | Animationsformat ungültig | Ungueltige Farbe; Grossbuchstaben verwenden. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-107 | CFG | Animationsformat ungültig | FRAME oder END erwartet. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-108 | CFG | Animationsformat ungültig | Hoechstens 64 Bilder erlaubt. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-109 | CFG | Animationsformat ungültig | FRAME mit 20 bis 10000 Millisekunden erwartet. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-110 | CFG | Animationsformat ungültig | Jede der 15 Bildzeilen braucht genau 20 Pixel. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-111 | CFG | Animationsformat ungültig | Pixel verweist auf eine ungueltige Palettenfarbe. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-112 | CFG | Animationsformat ungültig | Ungueltige Bilddaten. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-113 | CFG | Animationsformat ungültig | Mindestens ein Bild erforderlich. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-114 | CFG | Animationsformat ungültig | Datei zu gross oder ungueltig. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-115 | CFG | Animationsformat ungültig | Nach END sind keine weiteren Daten erlaubt. | Datei nach der .chaos-Formatbeschreibung korrigieren. |
| CFG-201 | CFG | Einstellungssicherung: Schema ungültig | Dateiformat, Version oder Kopfstruktur wird nicht unterstützt. | Eine unveränderte ChaosUhr-JSON-Sicherung verwenden. |
| CFG-202 | CFG | Einstellungssicherung: Felder fehlen oder haben einen falschen Typ | Die Einstellungsstruktur ist nicht vollständig oder enthält keine ganzen Zahlen. | Vollständige Sicherung aus einer unterstützten Version verwenden. |
| CFG-203 | CFG | Einstellungssicherung: Wert außerhalb des Bereichs | Mindestens ein importierter Einstellungswert überschreitet seinen Wertebereich. | Sicherung prüfen und passende Werte verwenden. |
| CFG-204 | CFG | Einstellungssicherung: Upload-Liste ungültig | Die Upload-Zuordnung ist keine zulässige Liste mit höchstens acht Einträgen. | Eigene Animationen separat sichern und gültige Sicherung verwenden. |
| CFG-205 | CFG | Einstellungssicherung: Upload-Fingerabdruck ungültig | Upload-ID, SHA-256 oder Eindeutigkeit der Zuordnung ist ungültig. | Unveränderte Sicherung verwenden und benötigte Animationen hochladen. |
| CFG-206 | CFG | Einstellungssicherung: Automatik-Upload fehlt | Eine ausgewählte eigene Animation wurde am Zielgerät nicht gefunden. | Die passende .chaos-Datei vor dem Import hochladen. |
| CFG-207 | CFG | Einstellungssicherung: Einzelanimation fehlt | Die gespeicherte Einzelanimation ist am Zielgerät nicht verfügbar. | Passende Animation hochladen oder Auswahl korrigieren. |
| CFG-208 | CFG | Einstellungssicherung größer als 8 KiB | Die ausgewählte Sicherungsdatei überschreitet die feste Importgrenze. | Eine gültige Sicherung mit höchstens 8 KiB auswählen. |
| CFG-209 | CFG | Einstellungssicherung enthält kein gültiges JSON | Die ausgewählte Datei lässt sich nicht als JSON lesen. | Originale unveränderte Sicherungsdatei verwenden. |
| CFG-210 | CFG | Einstellungssicherung konnte nicht gelesen werden | Der Browser konnte den Inhalt der lokalen Datei nicht lesen. | Dateizugriff prüfen und Datei erneut auswählen. |
