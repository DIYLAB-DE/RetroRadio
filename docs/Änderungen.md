# Änderungen

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > Änderungen

## v26.11.24

### Geändert

- **Radio und SD-Player:** Ein kurzer Tipp auf das Haus beendet die Wiedergabe und öffnet das Hauptmenü. Halten ab 0,8 Sekunden öffnet den Sleep-Timer. Das anschließende Loslassen löst keinen weiteren Menüwechsel aus.
- **Audiobibliothek:** RetroRadio verwendet audio-lite v0.2.0. PlatformIO bezieht diese feste Version beim Bauen automatisch. Git wird auch bei einem als ZIP heruntergeladenen Projekt benötigt.

### Behoben

- **Weboberfläche:** Nach einem Neustart wartet die Seite auf eine gültige Antwort des Radios. Bei fehlender Verbindung erscheint ein Hinweis. Abgelehnte Neustartaufträge werden als Fehler angezeigt.
- **Sender und SD-Dateien im Browser:** Nach einer Wiederverbindung werden die Listen erneut geladen. Fehlgeschlagene Abrufe werden wiederholt, verspätete Antworten einer vorherigen Verbindung überschreiben die Listen nicht.
- **Sprachwahl:** Ein Speicherfehler wird angezeigt und löst keinen Neustart aus.
- **Werkseinstellungen am Gerät:** WLAN-Profile, persönliche PIN, Postleitzahl und Nachrichtenauswahl werden ebenfalls zurückgesetzt.
- **SD-Wiedergabe:** audio-lite enthält die Korrektur für das Springen innerhalb nativer FLAC-Dateien.

### Bekannt und offen

Das Displaybild kann weiterhin kurz seitlich zucken. Diese Version enthält keine nachgewiesene Behebung. Die 16-MB-Boardvariante und vollständige Erstinstallationen unter Windows und macOS sind noch nicht praktisch geprüft.

Frühere Veröffentlichungen stehen unter [GitHub-Releases](https://github.com/DIYLAB-DE/RetroRadio/releases). Die EM34-Korrektur ist bereits Bestandteil von v26.11.12.

[Gerätebedienung](Bedienung.md) | [Weboberfläche](Weboberfläche.md) | [Erstinstallation](RetroRadio-Ersteinrichtung.md)

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > Änderungen
