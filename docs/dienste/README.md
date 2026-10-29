# Internetdienste einrichten

[RetroRadio](../../README.md) > [Dokumentation](../../README.md#dokumentation) > Internetdienste einrichten

Die Dienste werden am Radio unter **Einstellungen -> Cloud-Anbindung** und **Einstellungen -> Wetter** eingerichtet. Die [Weboberfläche](../Weboberfläche.md) bietet die Eingabe der längeren Adressen ebenfalls an.

| Funktion | Voraussetzung | Anleitung |
| --- | --- | --- |
| Kalender und Termine der Tagesansicht | Eigene Google-Apps-Script-Web-App für den Kalender | [Google einrichten](google/README.md) |
| Kontakte und Geburtstage | Eigene Google-Apps-Script-Web-App mit People API | [Google einrichten](google/README.md) |
| Aufgaben und Erledigt-Markierung | Eigene Google-Apps-Script-Web-App mit Tasks API | [Google einrichten](google/README.md) |
| Flugradar | Wetterstandort, optional OpenSky-API-Client | [Flugradar einrichten](Flugradar.md) |
| Wetter | WLAN und eingestellter Standort | [Wetterstandort](#wetterstandort) |

## Wetterstandort

Unter **Einstellungen -> Wetter** die deutsche Postleitzahl eingeben und speichern. Das Radio ermittelt Ort und Koordinaten und verwendet sie auch für die Tagesansicht und den Mittelpunkt des Flugradars. Eine fehlgeschlagene Ortssuche wird im Einstellbereich angezeigt. WLAN-Verbindung und Postleitzahl prüfen, wenn noch kein Ort ermittelt wurde.

Die Firmware verwendet Open-Meteo für Wetterdaten und OpenStreetMap-basierte Ortssuche. Für die im Projekt verwendeten Abrufe ist kein persönlicher Wetter-API-Schlüssel einzutragen. Herkunft und Lizenzhinweise stehen in der Projektübersicht der [Datenquellen](../Datenquellen.md).

## Eigene Werte und Demomodus

Die Google-Adressen gehören zu Deinen eigenen Bereitstellungen. Eine leere Adresse führt bei den betreffenden Anwendungen zu den eingebauten Beispieldaten. Für Screenshots oder Vorführungen die persönlichen Google-Adressen entfernen und die Anwendung neu öffnen. **Alle löschen** im Cloud-Bereich betrifft auch OpenSky, zum Trennen einzelner Dienste nur deren Felder leeren und speichern.

Private Bereitstellungsadressen, Kalenderkennungen und Client-Secrets gehören nicht in öffentliche Repositorys, Fehlerberichte oder Bildschirmfotos.

[RetroRadio](../../README.md) > [Dokumentation](../../README.md#dokumentation) > Internetdienste einrichten
