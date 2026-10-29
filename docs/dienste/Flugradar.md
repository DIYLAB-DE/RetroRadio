# Flugradar und OpenSky einrichten

Das Werkzeug **Flugradar** liegt in der Werkstatt. Sein Mittelpunkt ist der gespeicherte Wetterstandort. Unter **Einstellungen -> Wetter** zunächst eine deutsche Postleitzahl speichern und die Ortsermittlung abwarten. Im Radar stehen Suchradien von **5, 10, 20 und 50 km** zur Wahl.

## Eigenen API-Zugang anlegen

Melde Dich beim [OpenSky Network](https://opensky-network.org/) an und öffne die Kontoseite. Lege dort im Bereich **API Client** einen Client an. Du erhältst **client_id** und **client_secret**. Dies sind die Zugangsdaten für den programmatischen REST-Zugriff; Benutzername und Passwort der Webseite werden dafür nicht verwendet. OpenSky beschreibt diesen Ablauf in seiner [offiziellen FAQ, Abschnitte 7 und 19](https://opensky-network.org/about/faq).

Trage am Radio unter **Einstellungen -> Cloud-Anbindung** die Werte in **OpenSky Client-ID** und **OpenSky Client-Secret** ein und speichere. Alternativ die entsprechenden Felder der [Weboberfläche](../Weboberfläche.md) verwenden. Den Secret-Wert nicht in Screenshots oder öffentliche Konfigurationsdateien übernehmen.

Das Radio beschafft und erneuert die benötigten Zugriffstokens selbst. Du brauchst keinen Token von Hand anzufordern oder einzutragen. Die Anzeige **Konto** im Flugradar kennzeichnet eine nach lokalem Zeitstand noch gültige Anmeldung; sie bestätigt keinen erfolgreichen Datenabruf. **Gast** zeigt einen Abruf ohne erfolgreich verwendeten Kontozugang; die Firmware versucht diesen Weg auch, wenn die Anmeldung fehlschlägt.

## Bedienung

Die Radius-Tasten legen den Suchbereich fest. Die Ringtaste daneben wechselt die Beschriftung der Flugzeuge: Kennung mit Höhe und Tempo, nur Kennung oder nur Symbol. Die rechte Liste zeigt die empfangenen Flugzeuge. Ein Tipp auf einen Eintrag öffnet die Detailansicht mit Kennung, ICAO24, Herkunftsland, Entfernung, Höhe, Geschwindigkeit, Kurs und letzter Sichtung, soweit diese Daten vorliegen.

Nach einem Radius- oder Standortwechsel lädt das Werkzeug den neuen Bereich. Der schmale Balken rechts zeigt die Zeit bis zum nächsten Abruf. Es werden Positionsmeldungen aus dem OpenSky-Datenbestand angezeigt; die REST-Schnittstelle liefert damit keine vollständigen Flugpläne oder garantierten Routen. [OpenSky REST API](https://openskynetwork.github.io/opensky-api/rest.html).

## Wenn keine Flugzeuge erscheinen

Prüfe WLAN, Wetterstandort und den gewählten Radius. Ein leerer Bereich kann auch bedeuten, dass gerade keine geeigneten Meldungen vorliegen. Bei **Gast** trotz eingetragener Werte Client-ID und Secret auf vollständige Übernahme prüfen. Nach einer Änderung speichern und das Werkzeug neu öffnen. Abrufgrenzen des Dienstes können weitere Aktualisierungen vorübergehend verhindern; wiederholtes schnelles Neuöffnen erhöht das verfügbare Kontingent nicht.

Für persönliche und nicht kommerzielle Anwendungen bietet OpenSky den beschriebenen API-Zugang an; für kommerzielle Verwendung verlangt der Anbieter seine Zustimmung. Die jeweils aktuellen Bedingungen stehen bei [OpenSky](https://opensky-network.org/about/faq). Die angezeigten Daten stammen vom **OpenSky Network**.

[Zur Werkzeugbeschreibung](../werkstatt/flight-radar.md) | [Zur Diensteübersicht](README.md)
