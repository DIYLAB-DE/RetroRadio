# Weboberfläche

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > Weboberfläche

Verbinde Rechner und Radio mit demselben erreichbaren Netzwerk. Die Adresse des Radios steht im Hauptmenü und unter **Einstellungen -> Info**. Öffne diese Adresse im Browser mit `http://` davor. Bei eingeschalteter PIN-Sperre gib den am Radio eingestellten vierstelligen Sperrcode ein. Die voreingestellte Start-PIN lautet **`0000`**. Falls Du den Sperrcode bereits geändert hast, verwende Deinen eigenen Code. Die Oberfläche funktioniert im Browser unter Windows, Linux und macOS, für die Bedienung ist kein serieller Monitor erforderlich.

[![Weboberfläche entsperren](bilder/web/sperre-klein.png)](bilder/web/sperre.png)

Die Navigationsleiste springt zu den einzelnen Bereichen derselben Seite. Bei eingeschalteter PIN-Sperre sperrt das Schloss die Browseransicht wieder. Mit dem Mond- beziehungsweise Sonnensymbol wechselst Du zwischen heller und dunkler Darstellung, unten stehen sechs Farbschemata zur Wahl. Die Browseransicht merkt sich diese Auswahl. Alle Webaufnahmen dieser Anleitung zeigen die dunkle Darstellung.

## Status

Der Status zeigt WLAN-Verbindung, Zeitsynchronisation und Systemdaten. Hier lassen sich unter anderem Flashgröße, Firmwareversion, freie Ressourcen und die letzte Zeitsynchronisation ablesen. Netzwerkangaben sind in den Abbildungen unkenntlich gemacht.

[![Systemstatus im Browser](bilder/web/status-klein.png)](bilder/web/status.png)

## Radiosteuerung und Senderpflege

Die Radiosteuerung erscheint, wenn das **Internetradio am Gerät geöffnet** ist. Sie zeigt aktuellen Sender, Titel und Favoriten sowie die Senderskala. Die Regler ändern Lautstärke, Balance, Tiefen, Mitten und Höhen. Die beiden Pfeile wechseln den Sender, die Lautsprechertaste schaltet stumm beziehungsweise wieder hörbar.

[![Radiosteuerung](bilder/web/radio-klein.png)](bilder/web/radio.png)

Klicke den kleinen farbigen **Sendermarker** auf der Skala an. Die Aktionsleiste bietet Aufrufen, Bearbeiten und Löschen. Am Rechner öffnet ein Einfachklick das Menü, ein Doppelklick startet den Sender. Auf dem Touchscreen öffnet der erste Tipp das Menü, der zweite Tipp auf denselben Sender startet ihn. Auch nach erneutem Laden oder Zurücksetzen der Senderliste bleiben Menü und Editor erreichbar. Im Bearbeitungsdialog stehen Kurzname für die Skala, vollständiger Name, Stream-Adresse und Favoritenmarkierung. **Speichern** übernimmt die Änderung, **Abbrechen** verwirft sie. Verwende eine tatsächlich erreichbare Audio-Stream-Adresse. HTTP- und HTTPS-Streams werden unterstützt. Bei HTTPS muss das Serverzertifikat akzeptiert werden können, außerdem muss die Gerätezeit stimmen.

[![Sender bearbeiten](bilder/web/sender-bearbeiten-klein.png)](bilder/web/sender-bearbeiten.png)

### Sender sortieren

**Sortieren** öffnet die Liste aller 70 Senderplätze. Ziehe einen Eintrag am Griff auf seinen neuen Platz. Am oberen und unteren Listenrand scrollt die Liste automatisch weiter. Alternativ klickst oder tippst Du den Sender an, gibst einen Zielplatz von 1 bis 70 ein und bestätigst mit **Übernehmen**. Die Einträge dazwischen rücken nach, es wird kein Sender überschrieben. **Zurück** führt ohne Übernahme der Zielplatzwahl zur Liste.

Sendernamen, Streamadressen und Favoriten werden gemeinsam verschoben. Auch der laufende Sender bleibt seinem Eintrag zugeordnet. Erst **Speichern** übernimmt die gesamte neue Reihenfolge auf das Radio. **Abbrechen** verwirft alle Änderungen im Dialog. Hat sich die Senderliste währenddessen anderweitig geändert, wird die veraltete Reihenfolge nicht gespeichert. Öffne den Dialog dann erneut.

[![Sender sortieren](bilder/web/sender-sortieren-klein.png)](bilder/web/sender-sortieren.png)

[![Zielplatz auswählen](bilder/web/sender-zielplatz-klein.png)](bilder/web/sender-zielplatz.png)

### Sender sichern und wiederherstellen

**Sichern** lädt alle 70 Senderplätze einschließlich der Favoriten als Datei `retroradio-sender_JJJJ-MM-TT.json` herunter. Die Datei lässt sich auf RetroRadio und RetroRadioTab5 laden.

Mit **Laden** wählst Du eine Sicherungsdatei aus. Das Portal prüft die Datei und zeigt Dir die Zahl der belegten Plätze und Favoriten. Erst wenn Du bestätigst, ersetzt es die gesamte Liste. Bei einem Fehler gibt die Meldung den betroffenen Platz oder die Zeile und Spalte an. Nach dem Laden kann der laufende Sender kurz neu starten.

**Reset** bei der Senderliste stellt die Werkssender wieder her und überschreibt die eigene Liste. Die getrennte EQ-Rücksetztaste setzt die Klangregelung zurück.

## Systemeinstellungen

Hier richtest Du Zeitzone, Zeitserver, Synchronisationsintervall, Wetterstandort, Sprache, Nachrichtenressort und Gerätenamen ein. Eine vollständig abgewählte Nachrichten-Ressortauswahl wird ebenfalls gespeichert. Die Browser-Zeit kann in das Eingabefeld übernommen und mit **Zeit setzen** an das Gerät übergeben werden.

Unter **Netzwerkname** und **Sprache** zeigt die Zeile **PIN-Sperre für Radio und Web**, ob die Sperre ein- oder ausgeschaltet ist. Klicke auf **Ein**, gib Deine aktuelle PIN ein und bestätige mit **Ausschalten**. Mit **Aus** schaltest Du die Sperre sofort wieder ein. Dann gilt wieder Deine bisherige PIN. Unter **PIN ändern** gibst Du die aktuelle und die neue vierstellige PIN ein. Mit **Speichern** gilt die neue PIN sofort.

[![Systemeinstellungen](bilder/web/system-klein.png)](bilder/web/system.png)

**Speichern & neu starten** übernimmt die Systemeinstellungen und startet das Radio neu. **Neu starten** und **Werkseinstellungen** sind eigene Aktionen. Ein Zurücksetzen löscht persönliche Einstellungen, es ist kein normaler Schritt für ein Firmware-Update.

## Cloud-Anbindung

Trage hier die drei eigenen Google-Web-App-Adressen und bei Bedarf den OpenSky-Client ein. Die Schloss-Taste neben der Überschrift zeigt beziehungsweise verbirgt die Adressen. **Speichern** übernimmt die Werte. **Alle löschen** entfernt die gespeicherten Verbindungen, Kalender, Aufgaben und Kontakte können anschließend ihre eingebauten Demodaten anzeigen.

[![Cloud-Anbindung](bilder/web/cloud-klein.png)](bilder/web/cloud.png)

Die vollständigen Einrichtungsschritte stehen unter [Google](dienste/google/README.md) und [Flugradar/OpenSky](dienste/Flugradar.md). Persönliche Verbindungen sind in der Abbildung verdeckt.

## Dateien auf der SD-Karte

Der Dateimanager zeigt Ordner und Dateien der eingelegten Karte. Öffne Ordner durch Anklicken. Über die Pfadleiste gelangst Du zurück. Dateien lassen sich über die Upload-Fläche auswählen oder hineinziehen. Die Aktionen an einem Eintrag dienen zum Umbenennen beziehungsweise Löschen, **Neuer Ordner** legt einen Ordner im angezeigten Pfad an.

[![Dateimanager](bilder/web/dateien-klein.png)](bilder/web/dateien.png)

Alternativ ist FTP über die im Dateibereich angezeigte Adresse und die dort genannten Zugangsdaten möglich. Verwende eigene Musikdateien und halte vor dem Löschen oder Ersetzen eine Sicherung vor. Das Projekt enthält keine Musiksammlung. Für die Ordnerauswahl und Coveranzeige am Gerät siehe [SD-Player](Radio-und-Musik.md#sd-player).

## Display aufnehmen

**Screenshot laden** liest das aktuelle Gerätebild ein. Warte, bis die Vorschau vollständig erscheint. Danach speichert **Als PNG speichern** die Aufnahme auf dem Rechner. Ein neues Bild wird erst nach einem erneuten Laden abgerufen. Die Vorschau ist kein Livevideo.

[![Screenshot-Funktion](bilder/web/screenshot-klein.png)](bilder/web/screenshot.png)

## Netzwerkzugriff

Weboberfläche und FTP sind für das eigene vertrauenswürdige Netzwerk vorgesehen. Der Sperrcode sperrt die Browseransicht, er stellt keine umfassende Anmeldung für sämtliche Geräteschnittstellen dar. HTTP und FTP übertragen unverschlüsselt. Richte keine öffentliche Portweiterleitung zum Radio ein. Google-Web-App-Adressen und OpenSky-Zugangsdaten gehören weder in öffentliche Bilder noch in das Repository.

[Zur Gerätebedienung](Bedienung.md) | [Fehlerhilfe](Fehlerhilfe.md)

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > Weboberfläche
