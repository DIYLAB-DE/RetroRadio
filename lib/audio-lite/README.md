# audio-lite

Arduino-freier Ersatz für [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) von Wolle (schreibfaul1), reduziert auf den Funktionsumfang, den das RetroRadio-Projekt tatsächlich benutzt. Die Library spricht intern ausschließlich ESP-IDF-APIs (i2s_std, lwIP, FreeRTOS) und baut trotzdem als PlatformIO-Library im Arduino-Framework.

## Herkunft und Änderungen

Dieses Projekt ist eine veränderte Fassung von **ESP32-audioI2S, Version 3.4.7** (Autor: Wolle alias schreibfaul1), übernommen im August 2026 und seitdem umgebaut. Diese Kennzeichnung erfolgt gemäß GPLv3 §5a.

Die wesentlichen Änderungen gegenüber dem Original:

- Alle Arduino-Abhängigkeiten entfernt (Arduino.h, String, WiFiClient, FS/SD). Dateizugriff läuft über POSIX (`FILE*`, `connecttoFS(const char* pfad)`), Netzwerk über eine eigene Socket-Schicht.
- Neue Netzwerkschicht: abstrakte Schnittstelle `NetSocket` mit der Implementierung `TcpSocket` direkt über lwIP. Die Lesesemantik entspricht bewusst Arduinos NetworkClient (nicht blockierende reads). HTTPS verwendet `TlsSocket` über ESP-TLS mit dem eingebauten Zertifikatsbündel; HTTP verwendet `TcpSocket`.
- Eigener Ringpuffer (`AudioBuffer`) als Ersatz für die upstream-interne Pufferklasse.
- Opus-Decoder entfernt, VU/Spektrum-Berechnung stillgelegt (das Host-Projekt verarbeitet Pegel und Spektrum selbst über den Visualizer-Callback `audio_visualizer_process_i2s`).
- Fehlerkorrekturen, die im Original latent vorhanden sind: ID3-Parser-Kette (Fenstergrenzen im MP3_TAG-Zustand, Allokationsreihenfolge in `ps_ptr`, memcpy-Wächter) sowie ein terminaler Header-Timeout beim Verbindungsaufbau.
- MP3 behält seine bestehende Pegelreserve von 181/256, rund 3,01 dB. AAC einschließlich M4A und Vorbis wenden denselben Faktor vor der PCM16-Begrenzung an. WAV und FLAC erhalten die Reserve einmal pro neuem PCM-Ausgabeblock vor Pegelanzeige, Lautstärke und Klangregelung. Die Reserve ist keine senderübergreifende Lautheitsnormalisierung; die vorhandene EQ-Pegelreserve bleibt zusätzlich wirksam. Die Decoder-Kerne für FLAC und WAV sind bis auf mechanische Anpassungen (Include-Guards, Ersatz der Arduino-min/max-Makros, Speicher-Helfer) unverändert übernommen.
- AAC/FAAD wurde für Issue #1 angepasst (02./03.11.2026): korrigierte Stereo-Konvertierung und Initialisierung bei später PS-Signalisierung, wiederverwendete Decoder-Arbeitsbereiche, blockweise QMF-Fensterung und PCM16-Rundung am S3. Häufig verwendete SBR-/PS- und Filterbankfunktionen sowie vier kleine QMF-Tabellen liegen am S3 im internen Speicher. Die Unterstützung für SBR und parametrisches Stereo bleibt erhalten.
- WAV-Ausgabeblöcke werden auch nach einem I2S-Schreibversuch ohne übernommene Frames nur einmal pegelgeregelt. Unverschlüsselte WAV-Webdateien werden pro Schleifendurchlauf in mehreren kleinen Socket-Leseschritten bis höchstens 8 KB nachgefüllt.
- Rohe AAC-Livestreams mit SBR verwenden einen laufenden PCM-Vorrat im vorhandenen PSRAM-Arbeitsbereich. Die Klangregelung verarbeitet Stereo-Frames in Blöcken. Die Ausgabe bleibt bei der Quellabtastrate.

## Lizenz

GPL-3.0, wie das Original. Der vollständige Lizenztext liegt in [LICENSE](LICENSE). Die Urheberrechte am ursprünglichen Code liegen bei Wolle (schreibfaul1) und den jeweiligen Autoren der Decoder-Kerne; die Autorenvermerke in den Dateiköpfen bleiben erhalten.

In der Library enthaltene Fremdanteile und ihre ursprüngliche Herkunft:

| Bestandteil | Herkunft | Lizenz |
|---|---|---|
| MP3-Decoder | basiert auf dem Helix-MP3-Decoder (RealNetworks), in der von ESP32-audioI2S verbreiteten Fassung | wie im Upstream verbreitet |
| AAC-Decoder | libfaad2 (M. Bakker, Nero AG) | GPL-2.0-or-later |
| Vorbis-Decoder | Xiph.Org Foundation | BSD-artig (Xiph) |
| FLAC/WAV-Decoder | ESP32-audioI2S | GPL-3.0 |

## Nutzung

Die Library ersetzt `lib/ESP32-audioI2S` als Drop-in in einem PlatformIO-Arduino-Projekt. Die öffentliche API entspricht dem Original; einziger Unterschied ist `connecttoFS(const char* pfad)` mit POSIX-Pfad (Arduinos `SD.begin()` hängt die Karte unter `/sd` ein, Pfade brauchen also dieses Präfix).
