# Datenquellen und externe Dienste

Die Dienste werden nur für die jeweils zugehörigen Funktionen benötigt. Ihre Verfügbarkeit und Abrufbedingungen können sich ändern. Eigene Termine, Aufgaben und Kontakte werden über die selbst bereitgestellten Google-Web-Apps angebunden; ohne Verbindung stehen die eingebauten Demodaten zur Verfügung.

| Funktion | Datenquelle | Einrichtung und Hinweise |
| --- | --- | --- |
| Wetter und Vorhersage | [Open-Meteo](https://open-meteo.com/) | Wetterstandort am Gerät einstellen; für den vorgesehenen freien, nicht kommerziellen Zugang ist kein eigener Schlüssel erforderlich. |
| Ortsbestimmung | Open-Meteo-Geocoding und [Nominatim/OpenStreetMap](https://nominatim.org/) | Die PLZ-/Ortseingabe wird zur Standortermittlung verwendet. |
| Flugradar | [OpenSky Network](https://opensky-network.org/) | Wetterstandort und optional eigener OAuth-Client, siehe [Flugradar einrichten](dienste/Flugradar.md). |
| Kalender, Aufgaben, Kontakte | Google Kalender, Google Tasks, Google Kontakte | Drei eigene Apps-Script-Web-Apps, siehe [Google einrichten](dienste/google/README.md). |
| Nachrichtenlaufzeile | [tagesschau](https://www.tagesschau.de/) | Das Nachrichtenressort wird in den Einstellungen ausgewählt. Die Laufzeile zeigt Meldungen der Quelle; Rechte an den Nachrichten bleiben bei den jeweiligen Anbietern. |
| Uhrzeit | Eingestellter NTP-Zeitserver | Voreinstellung `pool.ntp.org`; Zeitzone und Abgleich in den Zeiteinstellungen. |
| Radiosendungen, Titel und Cover | Jeweiliger Radiosender beziehungsweise eigene Audiodatei | Senderadressen lassen sich in der Weboberfläche pflegen. Das Repository enthält keine Musikdateien. |

## Wetterdaten

Wetterdaten: **Open-Meteo**, bereitgestellt unter [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Die Firmware wählt Werte für die Anzeige aus und bereitet sie grafisch auf. Der freie API-Zugang ist für nicht kommerzielle Nutzung vorgesehen und unterliegt Abrufgrenzen. Datenlizenz und Zugangsbedingungen sind getrennt zu beachten. Maßgeblich sind die [Open-Meteo-Bedingungen](https://open-meteo.com/en/terms).

## Ortsdaten

Ortsdaten: **© OpenStreetMap-Mitwirkende**, verfügbar unter der [Open Database License](https://www.openstreetmap.org/copyright). Für den öffentlichen Nominatim-Dienst gelten zusätzlich seine [Nutzungsregeln](https://operations.osmfoundation.org/policies/nominatim/): geringe Abruflast, höchstens eine Anfrage pro Sekunde, eine erkennbare Anwendungskennung und Quellenangabe. Kein automatisches Durchprobieren von Postleitzahlen und keine Adress-Autovervollständigung. Der öffentliche Dienst ist kein unbegrenzt verfügbarer Massendienst; bei größerer Verbreitung sind dessen Kapazitäts- und Wechselanforderungen zu berücksichtigen.

## Flugdaten

Flugpositionsdaten: **The OpenSky Network**. Die Anzeige verarbeitet empfangene Positionsmeldungen und berechnet den Abstand zum eingestellten Standort. Verfügbarkeit, Abdeckung und Aktualität hängen vom Datenbestand ab. Kontozugang, Abrufgrenzen und Nutzungszweck richten sich nach den [OpenSky-Bedingungen und FAQ](https://opensky-network.org/about/faq). Die Darstellung dient der Information und liefert keine vollständigen Flugpläne.

## Persönliche Dienste

Für Google erteilst Du den eigenen Scripten die beschriebenen Zugriffe in Deinem Konto. Kalender und Kontakte werden gelesen; die Aufgaben-Web-App kann zusätzlich den Erledigt-Zustand ändern. Die Bereitstellungsadressen gewähren Zugriff auf diese Funktionen und müssen vertraulich bleiben. [Einrichtung und Zugriffsmodell](dienste/google/README.md).

[Dienste einrichten](dienste/README.md) | [Bibliotheken und Bildherkunft](LIZENZEN.md)
