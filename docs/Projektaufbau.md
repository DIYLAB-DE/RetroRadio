# Quellcode und Projektstruktur

RetroRadio ist ein PlatformIO-Projekt mit Arduino-ESP32 für den ESP32-S3. Das Display besitzt 800 x 480 Pixel. Die beiden Projektumgebungen unterscheiden sich durch Flashgröße, Boardbeschreibung und Partitionstabelle; beide verwenden 8 MB PSRAM und denselben Anwendungsquellcode.

| Verzeichnis oder Datei | Inhalt |
| --- | --- |
| `src/` | Implementierungen der Anwendungen, Geräteoberfläche, Dienste und Werkstattwerkzeuge |
| `include/` | Header, Konfiguration, Zeichenfunktionen, Schriften und eingebettete Bilder |
| `lib/audio-lite/` | Mitgelieferte, veränderte Audiobibliothek auf Grundlage von ESP32-audioI2S 3.4.7 |
| `lib/kissfft/` | FFT-Bibliothek für die Audioauswertung |
| `boards/` | Boardbeschreibungen für 8 und 16 MB Flash |
| `webui/index.html` | Quelltext der eingebetteten Weboberfläche |
| `scripts/` | Erzeugen des Web-Headers und der Firmware-Versionsangaben |
| `platformio.ini` | Gemeinsame Build-Einstellungen und die beiden Umgebungen |
| `partitions_8mb.csv`, `partitions_16mb.csv` | Partitionierung der jeweiligen Flashgröße |
| `docs/` | Aufbau, Bedienung, Dienste, Hardwaredateien, Abbildungen und Lizenzbeilagen |

## Anwendungen und gemeinsame Bausteine

`main.cpp` initialisiert Display, Touch, RTC, Audio, Netzwerk und die Anwendungssteuerung. `MainScreen` verwaltet Hauptmenü und Modulwechsel. Die Anwendungen verwenden die gemeinsame `BaseModule`-Schnittstelle. `Workshop` stellt die Werkzeugsammlung und deren Navigation bereit; die einzelnen Implementierungen liegen in den Dateien `WorkshopTool_*`.

`RetroRadio` und `SDPlayer` steuern ihre jeweiligen Wiedergabeansichten. `AudioManager` verwaltet die gemeinsame Audioausgabe. `Visualizer` verarbeitet die Audiodaten für die verschiedenen Darstellungen. Die Audioausgabe läuft über I2S an Verstärker und DAC; beide Anwendungen benutzen dieselbe Hardware.

`Gfx` erweitert LovyanGFX um die gemeinsamen Zeichenfunktionen. Größere Bildbereiche werden in PSRAM-Zeichenflächen vorbereitet. Die Schrift- und Bilddaten sind Bestandteil der Firmware. Die Weboberfläche wird beim Build aus ihrem HTML-Quelltext komprimiert und als Header eingebettet; Änderungen gehören deshalb in `webui/index.html`.

`AppState` hält gemeinsame Einstellungen und Laufzeitzustände. Dauerhafte Werte verwenden die vorhandenen Speichermechanismen einschließlich Preferences und RTC-NVRAM. `HttpService` übernimmt gemeinsame Netzabrufe. Kalender, Kontakte, Aufgaben und Tagesansicht verarbeiten die jeweils eingerichteten Datenquellen; die Google-Web-Apps liegen unter [docs/dienste/google/apps-script](dienste/google/apps-script/).

## Öffnen und übersetzen

Öffne den gesamten Projektordner in VS Code mit PlatformIO. Für die vollständige Einrichtung unter Windows, Linux und macOS gilt die [Erstinstallationsanleitung](RetroRadio-Ersteinrichtung.md). Alternativ im PlatformIO-Terminal:

```text
pio run -e waveshare_esp32s3_lcd7_8mb
pio run -e waveshare_esp32s3_lcd7_16mb
```

Die erste Zeile baut die 8-MB-Fassung, die zweite die 16-MB-Fassung. Für das Übertragen die tatsächliche Flashgröße und den aktuell erkannten Geräteanschluss wählen. WLAN- und Dienstzugänge werden am Gerät eingerichtet und gehören nicht in den Quellcode.

[Zur Bedienung](Bedienung.md) | [Bibliotheken und Lizenzen](LIZENZEN.md)
