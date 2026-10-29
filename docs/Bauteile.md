# Bauteile für Rev.2

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > Bauteile für Rev.2

Die Stückzahlen beziehen sich auf ein Radio mit zwei Visaton BF 37. Die [Aufbauanleitung](Aufbau.md) zeigt die Verbindung der Baugruppen, die [Fotogalerie](Fotogalerie.md) hilft bei der Zuordnung.

| Anzahl | Bauteil | Verwendung |
| --- | --- | --- |
| 1 | Waveshare ESP32-S3-Touch-LCD-7, 800 x 480, 8 MB PSRAM, 8 oder 16 MB Flash | Display und Hauptcontroller |
| 2 | Adafruit MAX98357A | Linker und rechter Mono-Verstärker |
| 1 | Adafruit PCM5102 | Stereo-Line-Out |
| 2 | Visaton BF 37, 4 Ohm | Lautsprecher der Rev.2 |
| 2 | N-Kanal-FET, fotografierte Varianten 2N7000 oder AO3400 | Lautsprecherabschaltung, Anschlussfolge am konkreten Typ prüfen |
| 1 | 100 kΩ | Linke Kanalwahl |
| 1 | 680 kΩ | Rechte Kanalwahl |
| 1 | 47 kΩ | Gemeinsamer Gate-Pulldown |
| 2 | 1 kΩ | Je eine Line-Out-Signalleitung |
| 2 | Elko 100 µF / 25 V | Je ein Verstärker |
| 1 | Tiny-RTC HW-111 mit DS1307 | Uhr, Umbau und Betriebsgrenzen in der Aufbauanleitung |
| 1 | CR2032 | RTC-Batterie |
| 1 | 3,5-mm-Klinkenbuchse | Stereo-Line-Out, im fotografierten Aufbau vierpolig mit einem freien Kontakt |
| nach Bedarf | Anschlussleitungen und passende Steckverbinder für das Waveshare-Board | I2S, I2C, Versorgung und Abschaltung |
| 1 | [UGREEN USB-C-Winkeladapter, 90 Grad](https://www.amazon.de/dp/B0FNCT8NS7) | Im gezeigten Gehäuseaufbau zur Anschlussführung verwendet |
| 1 | USB-Datenkabel | Programmierung und Einrichtung |
| optional | MicroSD-Karte | Musik für den SD-Player |
| 1 | Geeignete USB-C-Stromversorgung | Empfehlung: offizielles Raspberry-Pi-Netzteil, 5,1 V / 3,0 A |

Empfohlen wird das offizielle Raspberry-Pi-USB-C-Netzteil mit 5,1 V / 3,0 A in [Schwarz](https://www.berrybase.de/offizielles-raspberry-pi-usb-c-netzteil-5-1v-3-0a-eu-schwarz) oder Weiß. Bei moderater Lautstärke lassen sich die Geräte auch an Rechner-USB-Buchsen betreiben.

## Gehäuse und Montage

- Funkelndes TINMORRY PETG Filament von Amazon für den aktuellen Aufbau.
- Gedrucktes Gehäuse, Lautsprecherabdeckung, zwei BF-37-Einsätze, Grundplatte und Aufsteller aus den [3MF- und STEP-Dateien](hardware/README.md).
- Vier Bolzen mit M2,5-Gewinde und 4 mm langem Körper sowie zwei Bolzen mit M3-Gewinde und 15 mm langem Körper. **Nur die Außengewinde werden passend gekürzt.**
- Selbstschneidende Schrauben mit Sortimentsbezeichnung M2*5 für DAC, Verstärker und RTC. Keine metrischen M2-Maschinenschrauben daraus ableiten.
- Weitere Schrauben M2,5 und M3 in passender Länge, Schraubensortimente und Auswahlhinweise stehen in der Hardwareübersicht.
- UV-Resin und UV-Lampe für die fotografierte Verklebung der Lautsprechereinsätze.

Für die frühere Rev.1 werden statt BF 37 zwei Visaton SC 4.9 FL mit 8 Ohm und die zugehörigen Einsätze verwendet. Die Flashgröße ist unabhängig von dieser Gehäuse- und Lautsprechervariante.

## Werkzeug

Benötigt werden Lötausrüstung, Seitenschneider, passende Schraubendreher beziehungsweise Innensechskantschlüssel und ein Multimeter mit Durchgangsprüfung. Zum Kürzen der Außengewinde ein geeignetes Trenn- und Entgratwerkzeug verwenden. Gedruckte Teile vor der Montage auf Passung prüfen.

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > Bauteile für Rev.2
