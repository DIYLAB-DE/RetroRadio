# Fehlerhilfe

| Beobachtung | Prüfen und beheben |
| --- | --- |
| Kein serieller Anschluss | Datenkabel statt Ladekabel verwenden. Buchse und UART-Schalter nach der [Erstinstallation](RetroRadio-Ersteinrichtung.md) prüfen. Unter Linux Geräteberechtigungen, unter Windows und macOS gegebenenfalls den passenden Treiber kontrollieren. |
| Upload-Port belegt | Serielle Monitore und andere Programme schließen, die denselben Anschluss geöffnet haben. Danach den aktuell erkannten Port erneut auswählen. |
| Board startet nach der Erstinstallation nicht | Richtiges 8-/16-MB-Environment, Buchse, Schalterstellung und stabile Versorgung kontrollieren. Nach erfolgreichem Upload RESET drücken. Den Wechsel von UART1 zu USB vollständig nach Anleitung durchführen. |
| WLAN bleibt getrennt | 2,4-GHz-Netz, Passwort und aktiviertes Profil prüfen. Verbindungstest ausführen, speichern und den angebotenen Neustart durchführen. |
| Browser erreicht das Radio nicht | Die aktuelle IP am Gerät ablesen. Rechner und Radio müssen sich erreichen können; Gastnetz-/Client-Isolation kann das verhindern. Mit `http://` öffnen. |
| Radiosteuerung fehlt im Browser | Das Internetradio am Gerät öffnen und die Weboberfläche entsperren. Andere Gerätemodule werden nicht über diesen Radiobereich gesteuert. |
| Sender bleibt stumm | Anderen Sender testen, Stummschaltung und Lautstärke prüfen. Eine direkte Audio-Stream-Adresse verwenden. Eine Webseite ersetzt keinen Stream. Bei HTTPS zusätzlich Gerätezeit und Zertifikatsfehler im seriellen Protokoll prüfen. |
| Line-Out funktioniert, Lautsprecher bleiben aus | Unter Einstellungen -> Klang die Lautsprecher aktivieren. Abschaltschaltung, gemeinsame Masse und RS485-B-Anschluss anhand des Plans prüfen. |
| Knacken, Neustarts oder Aussetzer bei höherer Lautstärke | Versorgung und USB-Kabel prüfen. Mit niedriger Lautstärke testen und ein geeignetes Netzteil verwenden. Die Versorgung der Verstärker muss dem [Verdrahtungsplan](hardware/pläne/RetroRadio-Verdrahtungsplan.pdf) entsprechen. |
| SD-Dateien fehlen | Sitz der Karte und den im Dateidialog gewählten Pfad prüfen. Im Web-Dateimanager kontrollieren, ob die Dateien auf der Karte sichtbar sind. |
| Cover fehlt | Eine Audiodatei mit eingebettetem, vom Gerät unterstütztem Cover verwenden. Nicht jede Musikdatei enthält ein Cover. Die Kassettenansicht lässt sich unabhängig davon umschalten. |
| Kalender, Kontakte oder Aufgaben zeigen DEMO | Die jeweilige Google-Web-App-Adresse fehlt oder der Abruf ist fehlgeschlagen. `/exec`-Adresse, Bereitstellung, Freigabe und erforderliche Google-Dienste nach der [Google-Anleitung](dienste/google/README.md) prüfen. |
| Google zeigt eine Anmeldung statt Daten | Die Web-App ist nicht wie beschrieben für den Gerätezugriff bereitgestellt. Bereitstellungseinstellungen und Autorisierung im Google-Konto kontrollieren. |
| Leere Google-Listen trotz vorhandener Einträge | Script im Editor testen, Ausführungsprotokoll prüfen und die tatsächlich gelieferten Inhalte kontrollieren. Kalenderauswahl, Kontaktfilter, Aufgabenlisten und Mengenbegrenzungen beachten. Nach Codeänderungen eine neue Bereitstellungsversion veröffentlichen. |
| Aufgabenänderung kommt nicht bei Google an | Verbindung und Berechtigungen der Aufgaben-Web-App prüfen. Test mit einer eigens angelegten Probeaufgabe durchführen und das Ergebnis in Google Tasks kontrollieren. |
| Wetter oder Flugradar ohne Daten | WLAN und gespeicherten Wetterstandort prüfen. Beim Flugradar Radius und OpenSky-Anmeldung kontrollieren; ein leerer Bereich oder eine Abrufgrenze kann ebenfalls die Ursache sein. |
| Uhrzeit falsch | Zeitzone, Zeitserver und letzten Abgleich unter Einstellungen -> Zeit bzw. Info prüfen. Die RTC dient dem Erhalt der Uhrzeit; ihre Batterie und den dokumentierten Aufbau kontrollieren. |
| Änderungen scheinbar verloren | Bei Einstellungen die zugehörige Speichern-Taste verwenden. Ein Neustart ohne vorheriges Speichern übernimmt noch nicht gespeicherte Eingaben nicht. |

Für einen Fehlerbericht Boardvariante, Firmwareversion, betroffene Anwendung und eine kurze reproduzierbare Schrittfolge angeben. Screenshots helfen bei Darstellungsfehlern. Netzwerkadressen, WLAN-Namen, persönliche Termine und Kontakte sowie Zugangsdaten vorher unkenntlich machen. Ein Zurücksetzen auf Werkseinstellungen ist kein allgemeiner erster Lösungsversuch.

[Erstinstallation](RetroRadio-Ersteinrichtung.md) | [Bedienung](Bedienung.md) | [Weboberfläche](Weboberfläche.md)
