# Lizenzen und Herkunft

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > Lizenzen und Herkunft

Der eigene RetroRadio-Quellcode steht unter der **GNU GPL Version 3**, siehe [vollständigen Lizenztext](../LICENSE). Enthaltene Fremdanteile behalten ihre Urhebervermerke und ihre jeweiligen Lizenzen. Diese Übersicht ordnet die Bestandteile und die beigefügten Texte zu.

## Quellcode und Bibliotheken

| Bestandteil | Herkunft und Lizenz | Beilage |
| --- | --- | --- |
| RetroRadio | DIYLAB-DE, GPL-3.0 | [GPLv3](../LICENSE) |
| audio-lite | Veränderte Fassung von Wolle/schreibfaul1s ESP32-audioI2S 3.4.7, GPL-3.0 | [Herkunft und Änderungen](../lib/audio-lite/README.md), [Lizenz](../lib/audio-lite/LICENSE) |
| MP3-Decoder in audio-lite | Auf Helix von RealNetworks basierend, separate Helix-Texte siehe unten | [Helix-Hinweis](lizenzen/Helix-LICENSE.txt), [RPSL](lizenzen/Helix-RPSL.txt), [RCSL](lizenzen/Helix-RCSL.txt) |
| AAC-Decoder | libfaad2, M. Bakker/Nero AG, GPL-2.0-or-later laut Dateiköpfen | [GPLv2](lizenzen/GPL-2.txt), Urhebervermerke unter `lib/audio-lite/src/aac_decoder/` |
| Vorbis-Decoder | Xiph.Org Foundation, Dateikopf nennt Copyright 1994-2007 und BSD-Bedingungen | [Xiph-Lizenztext](lizenzen/Xiph-Vorbis-COPYING.txt), ursprüngliche Vermerke im Decoder |
| KissFFT | Mark Borgerding, Copyright 2003-2010, BSD-3-Clause | [COPYING](lizenzen/KissFFT-COPYING.txt), [BSD-Bedingungen](lizenzen/KissFFT-BSD-3-Clause.txt) |
| QR-Code-Generator | Project Nayuki, MIT | Vollständiger Text in [qrcodegen.h](../include/qrcodegen.h) und [qrcodegen.c](../src/qrcodegen.c) |
| LovyanGFX 1.2.27 | lovyan03 und genannte Mitwirkende, BSD-2-Clause | [Lizenz](lizenzen/LovyanGFX-Lizenz.txt) |
| Button-Klasse | Erweiterte Fassung von LGFX_Button, Herkunftsvermerke in den Dateiköpfen | [LovyanGFX-Beilage](LICENSE-LovyanGFX.txt) |
| ArduinoJson 7.4.3 | Benoît Blanchon, MIT | [Lizenz](lizenzen/ArduinoJson-Lizenz.txt) |
| ESPAsyncWebServer 3.6.0 | Genannte Autoren, LGPL-3.0 | [Lizenz](lizenzen/ESPAsyncWebServer-Lizenz.txt) |
| AsyncTCP | Genannte Autoren, LGPL-3.0 | [Lizenz](lizenzen/AsyncTCP-Lizenz.txt) |
| JPEGDEC 1.8.4 und bb_spi_lcd | Larry Bank/BitBank, Apache-2.0 | [JPEGDEC](lizenzen/JPEGDEC-Lizenz.txt), [bb_spi_lcd](lizenzen/bb_spi_lcd-Lizenz.txt) |
| SimpleFTPServer 3.0.1 | Genannte Autoren, MIT | [Lizenz](lizenzen/SimpleFTPServer-Lizenz.txt) |
| Arduino-ESP32 3.3.7 | Espressif und Mitwirkende, LGPL-2.1-or-later, weitere Bestandteile mit eigenen Vermerken | [LGPL-2.1](lizenzen/LGPL-2.1.txt), [Arduino-ESP32](https://github.com/espressif/arduino-esp32) |
| ESP-IDF und esp-dsp | Espressif und Mitwirkende, überwiegend Apache-2.0, jeweilige Komponentenvermerke beachten | [Apache-2.0](lizenzen/Apache-2.0.txt), [ESP-IDF](https://github.com/espressif/esp-idf) |

Die PlatformIO-Abhängigkeiten werden anhand der Projektkonfiguration bezogen. Die Lizenzbeilagen der externen Bibliotheken stammen aus den für den Build installierten Paketen. Weitere Bedingungen eingebundener Framework-Komponenten bleiben erhalten. [LGPLv3](lizenzen/LGPL-3.txt) liegt ebenfalls bei.

### Helix-Herkunft

audio-lite übernimmt den MP3-Decoder aus [ESP32-audioI2S 3.4.7, Commit 59bf36b](https://github.com/schreibfaul1/ESP32-audioI2S/tree/59bf36b214580ad2d1157748cd0268c9376d6640). Der [Dateikopf dieses Stands](https://github.com/schreibfaul1/ESP32-audioI2S/blob/59bf36b214580ad2d1157748cd0268c9376d6640/src/mp3_decoder/mp3_decoder.h) nennt Helix als Grundlage. Die Änderungen für RetroRadio sind unter [audio-lite](../lib/audio-lite/README.md) beschrieben.

Die öffentlich belegbaren Stationen der Herkunft sind:

| Stand | Beleg |
| --- | --- |
| 15. Januar 2019 | Der [Import 2d96710](https://github.com/schreibfaul1/ESP32-audioI2S/commit/2d96710ebfd1e80899bee8aaae1edb6e9d45b98e) enthält den MP3-Decoder, die [damalige README](https://github.com/schreibfaul1/ESP32-audioI2S/blob/2d96710ebfd1e80899bee8aaae1edb6e9d45b98e/README.md) nennt den enthaltenen Helix-MP3-Decoder. |
| 5. September 2019 | Wolle/schreibfaul1 fügt mit [Commit 2e43c40 die GPLv3](https://github.com/schreibfaul1/ESP32-audioI2S/blob/2e43c4089fd571d776e95d2a9f625eb4d7a80e16/LICENSE) hinzu und bestätigt die ergänzte Lizenz in [Issue 5](https://github.com/schreibfaul1/ESP32-audioI2S/issues/5#issuecomment-528328654). |
| Übernahme für RetroRadio | Grundlage ist ESP32-audioI2S 3.4.7. Herkunft, Änderungen und die GPLv3 des Herkunftsprojekts bleiben in audio-lite dokumentiert. |

Die separat beigelegten ursprünglichen Helix-Hinweise stammen aus der erhaltenen Fassung unter [ESP8266Audio/libhelix-mp3](https://github.com/earlephilhower/ESP8266Audio/tree/master/src/libhelix-mp3) und nennen Copyright 1995-2004 RealNetworks sowie RPSL/RCSL. Damit sind die Herkunft aus ESP32-audioI2S und dessen GPLv3-Angabe nachvollziehbar belegt. Die genaue Zuordnung der ursprünglichen Helix-Lizenzbedingungen zum übernommenen Decoder und eine zusätzliche GPL-Lizenzoption für dessen RealNetworks-Anteile sind damit noch nicht abschließend geklärt.

## CAD- und Druckdateien

Die 3MF-Dateien unter `docs/hardware/3d-druck/` und die STEP-Dateien unter `docs/hardware/cad/` stehen unter **Creative Commons Namensnennung-Nicht kommerziell-Share Alike 4.0 International (CC BY-NC-SA 4.0)**. Teilen und Bearbeiten sind für nichtkommerzielle Zwecke erlaubt. Die Urheber müssen genannt und Bearbeitungen unter gleichen Lizenzbedingungen weitergegeben werden. Kommerzielle Nutzung ist nicht gestattet.

Die Konstruktion stammt von Bruno / DIYLAB-DE. Die Lautsprechergitter hat Frank alias Tubefan aus dem Radio-Bastler-Forum im CAD erstellt. Die [Dateizuordnung und Urheberangaben](hardware/LIZENZ.txt), der [vollständige Lizenztext](lizenzen/CC-BY-NC-SA-4.0.txt) und die [deutsche Zusammenfassung bei Creative Commons](https://creativecommons.org/licenses/by-nc-sa/4.0/deed.de) beschreiben die Bedingungen. Diese Lizenz gilt ausschließlich für die CAD- und Druckdateien. Für den RetroRadio-Quellcode gilt weiterhin die GPLv3.

## Schriften und Symbole

Comfortaa Bold, Atkinson Hyperlegible Next Medium und League Gothic stehen unter SIL OFL 1.1. Die vollständigen Texte und Urheberangaben stehen unter [Schriftarten](fonts/README.md). Die Webschrift ist lokal im HTML eingebettet, dort ist ihr Lizenztext beigefügt. Die PDF-Anleitung verwendet DejaVu Sans und DejaVu Sans Mono, die [zugehörigen Lizenzvermerke](lizenzen/DejaVu-Lizenz.txt) liegen ebenfalls bei.

Die verwendeten Phosphor-Symbole stammen von **Phosphor Icons**, Copyright 2023, unter MIT. Das Taschenmesser-Symbol stammt von **Lucide Icons and Contributors**, Copyright 2026, unter ISC. Die vollständigen Texte liegen als [Phosphor-Lizenz](lizenzen/Phosphor-Lizenz.txt) und [Lucide-Lizenz](lizenzen/Lucide-Lizenz.txt) bei. Die Daten in `IconShapeData.h` enthalten die zugehörigen Herkunftshinweise.

## Darstellungen und Bilder

Die vier magischen Augen EM11, EM34, EM80 und EM84 werden durch eigenen Zeichencode dargestellt. Die EM34-Optik orientiert sich am **EM34-Emulator von N. Waldt**, dessen Veröffentlichung steht unter MIT. Danke für das Vorbild und die zusätzlich erteilte Erlaubnis. Die Vorlagenfotos beziehungsweise Röhrentabellen sind nicht Bestandteil des Repositorys.

Das mittig wachsende Spektrum orientiert sich optisch an [spectrum24 von Volos Projects](https://github.com/VolosR/spectrum24). Es wurden keine Quelltextpassagen, Treiber oder Schriften aus diesem Projekt übernommen.

Alle acht Kassettenmotive **Auralite, Auralux, Aurion, Ember, Glasstone, Lumotape, Modutape und Sunbeam** wurden mit ChatGPT erzeugt und für die Darstellung aufbereitet. Die beiden analogen VU-Skalen und die BurdaAudio-Projektgrafik gehören zu den Projektbildern. Frühere, inzwischen ersetzte Grafiken werden nicht mitgeliefert.

Die Aufbaufotos stammen von Bruno. Die Geräte- und Browserbilder zeigen echte Aufnahmen. Kalender, Kontakte und Aufgaben sind mit eingebauten Demodaten abgebildet. Die Bildschirmabbildung des SD-Players zeigt zur Funktionsdemonstration auch ein Albumcover, Rechte an solchen Coverbildern, Sendernamen und ausgestrahlten Inhalten verbleiben bei ihren jeweiligen Rechteinhabern. Musikdateien sind nicht enthalten.

Externe Wetter-, Orts-, Nachrichten- und Flugdaten sind unter [Datenquellen](Datenquellen.md) zugeordnet. Deren Daten- und Zugangsbedingungen sind von der Quellcodelizenz zu unterscheiden.

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > Lizenzen und Herkunft
