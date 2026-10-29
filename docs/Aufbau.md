# RetroRadio aufbauen

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > RetroRadio aufbauen

Diese Anleitung beschreibt die Rev.2 mit zwei Visaton BF 37. Die frühere Rev.1 verwendet andere Lautsprecher und passende Einsätze, sie ist in der [Fotogalerie](Fotogalerie.md) gekennzeichnet. Beide Aufbauversionen sehen von vorn gleich aus. Die Wahl zwischen 8 und 16 MB Flash betrifft die Firmwareumgebung, nicht die hier beschriebene Verdrahtung.

[![Fertige Geräte](bilder/gerät/20260607_141617-klein.jpg)](bilder/gerät/20260607_141617.jpg)

## Unterlagen und Reihenfolge

- [Bauteileliste](Bauteile.md)
- [Gehäuse, Druckprojekte, STEP-Dateien und Befestigung](hardware/README.md)
- [Verdrahtungsplan als PDF zum Herunterladen](hardware/pläne/RetroRadio-Verdrahtungsplan.pdf)
- [Alle Aufbaufotos mit anklickbaren Vorschaubildern](Fotogalerie.md)
- [Erstinstallation unter Windows, Linux und macOS](RetroRadio-Ersteinrichtung.md)

Installiere die Firmware zunächst auf dem Displayboard ohne angeschlossene Audio-Baugruppen. Dafür gelten die Buchsen und Schalterstellungen der Erstinstallationsanleitung. Prüfe danach die einzelnen Verbindungen bei ausgeschalteter Versorgung und ergänze Audio, RTC und Gehäuse. Zum normalen Betrieb steht der UART-Umschalter auf UART2, weil die betreffenden Anschlüsse für I2S verwendet werden.

| Gesamtverdrahtung | Einzelheiten |
| --- | --- |
| [![Gesamtplan](hardware/pläne/RetroRadio-Verdrahtungsplan-1.png)](hardware/pläne/RetroRadio-Verdrahtungsplan-1.png) | [![Detailplan](hardware/pläne/RetroRadio-Verdrahtungsplan-2.png)](hardware/pläne/RetroRadio-Verdrahtungsplan-2.png) |

Die Zeichnung ist funktional und nicht maßstäblich. Maßgeblich sind die Signalnamen, der konkrete Platinenaufdruck und die Anschlussbelegung der verwendeten Bauteile.

## Display und Befestigungsbolzen

Das Waveshare-Display trägt die Hauptplatine auf seiner Rückseite. Die Fotos zeigen das Board, den Lieferumfang und die bearbeiteten Abstandshalter.

| Displayeinheit | Mitgelieferte Teile | Befestigungsbolzen |
| --- | --- | --- |
| [![Display](bilder/aufbau/20260529_212225-klein.jpg)](bilder/aufbau/20260529_212225.jpg) | [![Kabel](bilder/aufbau/20260529_212209-klein.jpg)](bilder/aufbau/20260529_212209.jpg) | [![Bolzen](bilder/aufbau/20261028_213523-klein.jpg)](bilder/aufbau/20261028_213523.jpg) |

Benötigt werden vier Bolzen mit M2,5-Gewinde und **4 mm langem Körper** sowie zwei Bolzen mit M3-Gewinde und **15 mm langem Körper**. Die kurzen Bolzen halten die Grundplatte für die Baugruppen. Die beiden 15-mm-M3-Bolzen sitzen diagonal am Display an den in den STEP-/Druckdateien vorgesehenen Punkten. Die unterschiedlichen Befestigungen sind an der Konstruktion zuzuordnen.

> **Nur die Außengewinde kürzen.** Die Bolzenkörper selbst bleiben 4 mm beziehungsweise 15 mm lang. Links im Foto liegen die Ausgangsteile, rechts die Teile mit gekürzten Gewinden. Zu weit eingeschraubte Gewinde können das Display beschädigen. Prüfe vor dem Einschrauben die verfügbare Tiefe und kürze die Außengewinde passend zur jeweiligen Befestigung.

## Gehäuse und Lautsprecher

Drucke Gehäuse, Abdeckung, Grundplatte, Aufsteller und die zum Lautsprecher passenden Einsätze aus den [bereitgestellten Projekten](hardware/README.md). Die Rev.2 verwendet **Visaton BF 37 mit 4 Ohm**. Für die frühere Rev.1 ist der Einsatz für **Visaton SC 4.9 FL mit 8 Ohm** enthalten.

Klebe die Lautsprechereinsätze mit UV-Resin in das Gehäuse und härte die Klebenähte aus. Achte beim Einpassen auf die Ausrichtung der Lautsprecher und die spätere Leitungsführung.

| Einsätze BF 37 | Verklebung | Lautsprecher | Einbau |
| --- | --- | --- | --- |
| [![Einsätze](bilder/aufbau/20260822_135305-klein.jpg)](bilder/aufbau/20260822_135305.jpg) | [![Klebenaht](bilder/aufbau/20260822_141135-klein.jpg)](bilder/aufbau/20260822_141135.jpg) | [![BF 37](bilder/aufbau/20260822_185122-klein.jpg)](bilder/aufbau/20260822_185122.jpg) | [![Lautsprechereinbau](bilder/aufbau/20260822_193828-klein.jpg)](bilder/aufbau/20260822_193828.jpg) |

## Baugruppen auf der Grundplatte

Auf der Grundplatte sitzen ein PCM5102-DAC, zwei MAX98357A-Verstärker und die RTC. Die kleinen selbstschneidenden Schrauben heißen im verwendeten Sortiment **M2*5**, sie sind keine metrischen M2-Maschinenschrauben. Die übrigen Schrauben sind M2,5 und M3. Wähle ihre Länge passend zur verfügbaren Einschraubtiefe. Die vollständigen Sortimentsbezeichnungen und Druckdateien stehen in der [Hardwareübersicht](hardware/README.md).

[![Bestückte Grundplatte](bilder/aufbau/20260923_120917-klein.jpg)](bilder/aufbau/20260923_120917.jpg)

## Mechanische Endmontage

1. Lautsprechereinsätze einpassen und verkleben, danach die Lautsprecher in ihren Einsätzen befestigen. Die Lautsprecherabdeckung am Gehäuse montieren.
2. Die Außengewinde der Bolzen auf die am eigenen Display verfügbare Tiefe kürzen und entgraten. Die vier kurzen M2,5-Bolzen für die Grundplatte und die beiden langen M3-Bolzen diagonal am Display zuordnen.
3. Das Display in das Gehäuse einsetzen und an den vorgesehenen Befestigungspunkten verschrauben. Grundplatte mit den vier Baugruppen auf den kurzen Bolzen montieren.
4. Den Aufsteller an den vorgesehenen Gehäuseaufnahmen befestigen. Leitungen so führen, dass sie nicht an Lautsprechermembranen, Display oder Schraubspitzen anliegen. Schrauben nach Gewinde und verfügbarer Einschraubtiefe auswählen, ohne sie gegen Widerstand weiterzudrehen.
5. Für den nach hinten geführten USB-Anschluss verwendet der gezeigte Aufbau einen [UGREEN-USB-C-Winkeladapter, 90 Grad](https://www.amazon.de/dp/B0FNCT8NS7). Das zugehörige Foto zeigt die zusätzliche Abstützung aus UV-Resin. Adapter und Datenkabel vor der endgültigen Befestigung auf Sitz und Datenverbindung prüfen.

[![USB-C-Winkeladapter](bilder/aufbau/20261001_193314-klein.jpg)](bilder/aufbau/20261001_193314.jpg)

## Gemeinsamer I2S-Anschluss

Alle drei Audio-Module erhalten dieselben Audiodaten und Takte. Die Kanalwahl erfolgt an den beiden Mono-Verstärkern.

| Signal | Displayboard | Verstärker links | Verstärker rechts | DAC |
| --- | --- | --- | --- | --- |
| Audiodaten | UART2 TXD, GPIO43 | DIN | DIN | DIN |
| Bit-Takt | UART2 RXD, GPIO44 | BCLK | BCLK | BCK |
| Word-Select | Sensor AD, GPIO6 | LRC | LRC | WSEL |
| Masse | GND | GND | GND | GND |
| Versorgung | Eingang IN von U5, 5 V | VIN | VIN | VIN |

Der UART-Umschalter steht hierfür auf **UART2**. GPIO44 ist trotz der Beschriftung RXD ein Taktausgang. Die I2S-Signale haben 3,3-V-Pegel, die Audio-Module werden mit 5 V versorgt. Eine zusätzliche MCLK-Leitung wird nicht benutzt. Verbindungen kurz halten und die Masse aller Baugruppen verbinden.

### Versorgung und Elkos

Die 5 V werden am Eingang **IN des Reglers U5** abgegriffen. Je Verstärker sitzt ein **100-µF-/25-V-Elko** unmittelbar zwischen VIN und GND: Plus an VIN, Minus an GND. Verwechsle den 5-V-Eingang des Reglers nicht mit seinem 3,3-V-Ausgang.

| 5-V-Abgriff | Elkos | Steckverbinder |
| --- | --- | --- |
| [![U5](bilder/aufbau/20260923_191638-klein.jpg)](bilder/aufbau/20260923_191638.jpg) | [![Elkos](bilder/aufbau/20260923_122918-klein.jpg)](bilder/aufbau/20260923_122918.jpg) | [![Stecker](bilder/aufbau/20260924_121947-klein.jpg)](bilder/aufbau/20260924_121947.jpg) |

### Linker und rechter Verstärker

| Verstärker | Zusatzwiderstand | Verbindung | GAIN |
| --- | --- | --- | --- |
| Links | 100 kΩ | Eigener SD-Pin an VIN über den Widerstand | offen |
| Rechts | 680 kΩ | Eigener SD-Pin an VIN über den Widerstand | offen |

Die vorhandenen Board-Widerstände bleiben bestückt. Die beiden SD-Pins bleiben voneinander getrennt. Sie dürfen weder direkt an 5 V noch direkt miteinander verbunden werden. Die offenen GAIN-Pins ergeben beim verwendeten Adafruit-Modul 9 dB. Der dokumentierte Aufbau wurde mit dieser Einstellung betrieben.

Jeder Lautsprecher wird ausschließlich zwischen die beiden Lautsprecherausgänge seines Verstärkers angeschlossen. **Keinen dieser Ausgänge mit GND verbinden.**

## Lautsprecherabschaltung

Je Verstärker zieht ein N-Kanal-FET dessen SD-Pin auf GND, wenn die Lautsprecher abgeschaltet werden. Die Bilder zeigen zwei ausgeführte Varianten mit **2N7000** und **AO3400**. Prüfe die tatsächliche Anschlussfolge von Gate, Drain und Source am verwendeten Bauteil, die Gehäuse sind nicht allgemein anschlussgleich.

| Anschluss | Verbindung |
| --- | --- |
| Drain links | SD des linken Verstärkers |
| Drain rechts | SD des rechten Verstärkers |
| Beide Sources | GND |
| Beide Gates | Gemeinsamer Knoten an RS485 B / TX- |
| 47-kΩ-Widerstand | Zwischen gemeinsamem Gate-Knoten und GND |
| RS485 A / TX+ | unbenutzt |

**Den 120-Ω-Abschluss am RS485-Anschluss ausschalten.** Die 47 kΩ liegen als Pulldown an GND, nicht in Reihe zur Steuerleitung. Der RS485-Anschluss wird hier als Schaltausgang benutzt, es läuft darüber keine serielle Kommunikation.

| FET 2N7000 | Eingebaute Variante 2N7000 | Eingebaute Variante AO3400 |
| --- | --- | --- |
| [![2N7000](bilder/aufbau/20260923_135827-klein.jpg)](bilder/aufbau/20260923_135827.jpg) | [![2N7000 eingebaut](bilder/aufbau/20260923_143153-klein.jpg)](bilder/aufbau/20260923_143153.jpg) | [![AO3400 eingebaut](bilder/aufbau/20260929_124701-klein.jpg)](bilder/aufbau/20260929_124701.jpg) |

Die Firmware steuert GPIO16: HIGH schaltet die Lautsprecher ein, LOW schaltet sie aus. Der RS485-Treiber invertiert dabei den Pegel an B. Der DAC und der Line-Out laufen auch bei abgeschalteten Lautsprechern weiter. Die Bedienung erfolgt unter **Einstellungen -> Klang -> Lautsprecher**.

## RTC vorbereiten und anschließen

Verwendet wird ein Tiny-RTC-Modul mit DS1307, Platinenaufdruck HW-111 und einer CR2032. Im dokumentierten Aufbau sind **Diode D1 und EEPROM U3 entfernt**. D1 darf bei der verwendeten CR2032 nicht als Ladepfad bestückt bleiben.

| Bestückungsseite, D1 bereits entfernt | Batterieseite mit CR2032 | Kabelbelegung |
| --- | --- | --- |
| [![RTC nach Entfernung von D1](bilder/aufbau/20260923_114835-klein.jpg)](bilder/aufbau/20260923_114835.jpg) | [![RTC-Batterieseite](bilder/aufbau/20260923_120530-klein.jpg)](bilder/aufbau/20260923_120530.jpg) | [![I2C-Kabel](bilder/aufbau/20260530_205745-klein.jpg)](bilder/aufbau/20260530_205745.jpg) |

| Display-I2C | RTC |
| --- | --- |
| SCL, GPIO9 | SCL |
| SDA, GPIO8 | SDA |
| VCC, 3V3 | VCC |
| GND | GND |

Die Spannungswahl der Displaybuchse bleibt auf **3V3**. DS bleibt frei. Prüfe jede Ader beidseitig: Beim fotografierten Kabel müssen die Adern wie gezeigt umgesetzt werden, sonst stimmen VCC und GND nicht. Verlasse Dich nicht allein auf die Kabelfarben. Der Bus arbeitet mit 100 kHz.

Der beschriebene DS1307-Betrieb an 3,3 V liegt außerhalb seiner spezifizierten Versorgung von 4,5 bis 5,5 V. Die RTC wird nicht an die 5-V-Audioversorgung angeschlossen. Siehe [DS1307-Datenblatt](https://www.analog.com/media/en/technical-documentation/data-sheets/DS1307.pdf).

Auf dem Foto der Bestückungsseite ist D1 bereits entfernt, U3 ist noch bestückt. Entferne vor dem Einbau auch U3 und kontrolliere die Bestückungsseite.

## Stereo-Line-Out

Am PCM5102 wird **Lout über 1 kΩ an Tip**, **Rout über 1 kΩ an Ring** und **G direkt an Sleeve** einer Stereo-Klinkenverbindung geführt. Die verwendete Buchse hat vier Kontakte, der zusätzliche Kontakt bleibt unbenutzt. Ermittle die Lötfahnen mit einem eingesteckten Stecker und einem Durchgangsprüfer. Eine universelle Zuordnung nach der äußeren Form der Buchse ist nicht möglich.

| Buchse | Verdrahtung |
| --- | --- |
| [![Line-Out-Buchse](bilder/aufbau/20261007_200803-klein.jpg)](bilder/aufbau/20261007_200803.jpg) | [![Line-Out-Verdrahtung](bilder/aufbau/20261007_202331-klein.jpg)](bilder/aufbau/20261007_202331.jpg) |

Der Line-Out kann einen externen Verstärker oder einen externen Bluetooth-Sender versorgen. Eine integrierte Bluetooth-Audioübertragung gehört nicht zur beschriebenen Firmware.

## Abschluss und Inbetriebnahme

Prüfe bei abgeschalteter Versorgung die Polarität der Elkos, 5 V und GND, die getrennten SD-Leitungen, die FET-Anschlüsse und die RTC-Kabelbelegung. Prüfe, ob Leitungen frei von Schrauben und Gehäusekanten liegen. Schließe danach die Versorgung an und beginne mit kleiner Lautstärke. Prüfe beide Lautsprecher, den Line-Out und das Abschalten der Lautsprecher in den Klangeinstellungen.

| Fertig verdrahtet | Bestückte Displayeinheiten | Gehäusevarianten |
| --- | --- | --- |
| [![Gesamtaufbau](bilder/aufbau/20260925_202741-klein.jpg)](bilder/aufbau/20260925_202741.jpg) | [![Module montiert](bilder/aufbau/20261003_133322-klein.jpg)](bilder/aufbau/20261003_133322.jpg) | [![Gehäuse](bilder/aufbau/20261003_134104-klein.jpg)](bilder/aufbau/20261003_134104.jpg) |

Bei moderater Lautstärke lassen sich die Geräte an Rechner-USB-Buchsen betreiben. Als separates Netzteil empfiehlt sich das offizielle Raspberry-Pi-USB-C-Netzteil mit **5,1 V / 3,0 A**, erhältlich in Schwarz oder Weiß. Die erreichbare Lautstärke hängt auch von der Versorgung ab. Danach helfen [Erstinstallation](RetroRadio-Ersteinrichtung.md) und [Bedienung](Bedienung.md) weiter.

## Technische Quellen

[MAX98357A-Anschlüsse von Adafruit](https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts), [PCM510x-Anschlüsse von Adafruit](https://learn.adafruit.com/adafruit-pcm510x-i2s-dac/pinouts) und [Waveshare-Schaltplan Rev.1.2](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-7/ESP32-S3-Touch-LCD-7-Sch.pdf). Die Displayplatine auf den Fotos trägt VER1.1. Der verlinkte Waveshare-Schaltplan beschreibt Rev.1.2.

[RetroRadio](../README.md) > [Dokumentation](../README.md#dokumentation) > RetroRadio aufbauen
