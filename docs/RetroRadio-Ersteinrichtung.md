# RetroRadio: Erste Inbetriebnahme

Diese Anleitung führt Dich vom fabrikneuen Waveshare-Display zum gestarteten RetroRadio. Du bereitest das Projekt unter Windows, Linux oder macOS in Visual Studio Code mit PlatformIO vor, überträgst die Firmware zunächst über UART1 und wechselst anschließend auf die USB-Buchse. Danach kannst Du denselben Projektordner zum Lesen und Bearbeiten des Quellcodes und für spätere Aktualisierungen verwenden.

**Stand: 28.10.2026.** Der vollständige Installationsablauf wurde am 23.09.2026 unter Linux mit einem ESP32-S3-Touch-LCD-7, PCB 1.1, 8 MB Flash und 8 MB PSRAM praktisch bestätigt: Erstinstallation über UART1, Wechsel auf USB und erneuter erfolgreicher Upload. Die Schritte für Windows und macOS stützen sich auf die offiziellen Software- und Treiberunterlagen; eigene Durchläufe auf diesen Systemen stehen noch aus. Die 16-MB-Projektumgebung ist vorhanden, eine Erstinstallation auf dieser Boardvariante ist noch nicht praktisch bestätigt.

> Beginne mit dem Displayboard ohne angeschlossene Audio-Hardware und weitere externe Module. RetroRadio aktiviert den USB-Anschluss selbst. Eine Zwischen-Firmware ist nicht erforderlich; die ab Werk installierte Demo kann direkt ersetzt werden.

## 1. Was Du brauchst

- Ein Waveshare **ESP32-S3-Touch-LCD-7 mit 800 x 480 Pixeln**, 8 oder 16 MB Flash und 8 MB PSRAM.
- Ein USB-Datenkabel, passend zu Deinem Rechner und den USB-C-Buchsen des Boards. Ein reines Ladekabel reicht nicht.
- Einen Rechner mit Windows, Linux oder macOS und Internetzugang für den Download der Entwicklungswerkzeuge und Projektabhängigkeiten.
- Visual Studio Code mit der Erweiterung PlatformIO IDE sowie den vollständigen RetroRadio-Projektordner.

Für den ersten Start genügt die Versorgung über das USB-Datenkabel. Die Audio-Baugruppen schließt Du erst nach den beiden erfolgreichen Uploads an.

## 2. Visual Studio Code und PlatformIO einrichten

### Vorbereitung für Dein Betriebssystem

Installiere die Desktop-Ausgabe von [Visual Studio Code](https://code.visualstudio.com/download). Die folgenden Schritte werden in der lokal installierten Anwendung ausgeführt.

| Betriebssystem | Vorbereitung |
| --- | --- |
| Windows | Verwende das zu Deinem Rechner passende [VS-Code-Installationsprogramm](https://code.visualstudio.com/docs/setup/windows). Die Variante **User Setup** installiert VS Code für Dein Benutzerkonto. |
| Linux | Installiere das passende Paket gemäß der [VS-Code-Anleitung für Linux](https://code.visualstudio.com/docs/setup/linux), beispielsweise DEB unter Debian/Ubuntu oder RPM unter Fedora. Für PlatformIO werden Python 3 und die Unterstützung für virtuelle Python-Umgebungen benötigt; unter Debian/Ubuntu heißt das Zusatzpaket `python3-venv`. Installiere diese bei Bedarf über die Paketverwaltung Deiner Distribution. |
| macOS | Lade die passende Ausgabe für Deinen Mac herunter und verschiebe **Visual Studio Code.app** nach **Programme / Applications**. Die [VS-Code-Anleitung für macOS](https://code.visualstudio.com/docs/setup/mac) beschreibt die Installation. |

Für das Klonen des Projekts und spätere Git-Aktualisierungen benötigst Du zusätzlich [Git für Dein Betriebssystem](https://git-scm.com/install/). Falls Du Git gerade installiert hast, starte VS Code anschließend neu. In Abschnitt 3 ist auch die Beschaffung als ZIP-Datei beschrieben.

### PlatformIO installieren

1. Starte VS Code und öffne links **Erweiterungen / Extensions**.
2. Suche nach **PlatformIO IDE** und installiere die Erweiterung des Herausgebers **PlatformIO**.
3. Warte auf den Abschluss der Einrichtung und lade das VS-Code-Fenster neu, falls Du dazu aufgefordert wirst.

PlatformIO bringt seine Kommandozeilenwerkzeuge bereits mit. Für diesen Weg brauchst Du keine zusätzliche PlatformIO-Installation per `pip` oder Betriebssystempaket. Unter Linux nennt die [offizielle PlatformIO-Installationsanleitung](https://docs.platformio.org/en/latest/integration/ide/vscode.html#installation) außerdem `python3-venv` als Voraussetzung.

Die Menüs von VS Code können deutsch oder englisch heißen. PlatformIO verwendet überwiegend englische Bezeichnungen; diese sind hier so angegeben, wie sie in der Erweiterung stehen.

## 3. Das vollständige Projekt öffnen

### Empfohlen: direkt in VS Code klonen

1. Öffne das [RetroRadio-Repository](https://github.com/DIYLAB-DE/RetroRadio) im Browser. Kopiere über **Code > HTTPS** die Repository-Adresse.
2. Öffne in VS Code über **Ansicht / View > Befehlspalette / Command Palette** die Befehlsauswahl.
3. Wähle **Git: Clone** und füge die kopierte HTTPS-Adresse ein.
4. Wähle einen lokalen Zielordner. Öffne nach dem Klonen das Projekt in VS Code.

Dieser Weg entspricht dem [Klonen eines vorhandenen Repositorys in VS Code](https://code.visualstudio.com/docs/sourcecontrol/repos-remotes#clone-repositories). Du erhältst eine vollständige lokale Arbeitskopie, die Du später auch bearbeiten kannst.

### Alternative: ZIP herunterladen

Wähle im Repository **Code > Download ZIP**, entpacke das Archiv vollständig und öffne den entpackten Projektordner über **Datei / File > Ordner öffnen / Open Folder** in VS Code. Wähle den Ordner, in dem `platformio.ini` liegt. Bei einer ZIP-Datei heißt dieser Ordner üblicherweise `RetroRadio-master`.

### Den richtigen Ordner erkennen

Im Explorer von VS Code müssen unter anderem diese Einträge sichtbar sein:

| Eintrag | Inhalt |
| --- | --- |
| `platformio.ini` | Gemeinsame Projekteinstellungen und die beiden Flashvarianten. |
| `src` | C++-Quelldateien des Radios und seiner Anwendungen. |
| `include` | Zugehörige Header-Dateien. |
| `boards` | Boarddefinitionen für 8 und 16 MB Flash. |
| `lib` | Mitgelieferte Bibliotheken, darunter die angepasste Audio-Bibliothek. |

Öffne den vollständigen Projektordner, nicht nur eine einzelne Quelldatei oder den Unterordner `src`. Bestätige eine Vertrauensabfrage von VS Code nur für die von Dir bezogene und geprüfte Projektquelle. Warte anschließend, bis PlatformIO das Projekt erkannt und vorbereitet hat. Beim ersten Öffnen und Bauen werden weitere Pakete heruntergeladen.

Zum Lesen oder Bearbeiten öffnest Du eine Datei im Explorer. Änderungen speicherst Du wie gewohnt in VS Code; danach kannst Du die Firmware erneut bauen und übertragen. Für die Erstinstallation sind keine Änderungen am Quellcode, an den Bibliotheken oder an `platformio.ini` nötig.

## 4. Die passende Flashvariante auswählen

Entscheidend ist die Flashgröße Deines Boards. Beide Varianten haben 8 MB PSRAM. Wähle nach der Speicherbestückung, nicht allein nach der PCB-Version oder der Aufbauversion des Gehäuses.

| Flashgröße | PlatformIO-Umgebung |
| --- | --- |
| 8 MB Flash, 8 MB PSRAM | `waveshare_esp32s3_lcd7_8mb` |
| 16 MB Flash, 8 MB PSRAM | `waveshare_esp32s3_lcd7_16mb` |

Öffne die Befehlspalette und wähle **PlatformIO: Pick Project Environment**. Wähle den Eintrag mit der zu Deinem Board passenden Umgebung. Die mitgelieferte Voreinstellung ist die 8-MB-Variante; für ein 16-MB-Board wählst Du ausdrücklich die 16-MB-Umgebung.

Klicke links auf das PlatformIO-Symbol und öffne **Project Tasks**. Die ausgewählte Umgebung enthält unter **General** die Einträge **Build** und **Upload**. Falls nur eine Umgebung angezeigt wird, kannst Du über die Befehlsauswahl die andere wählen. Die [Befehlsbezeichnungen der Erweiterung](https://github.com/platformio/platformio-vscode-ide/blob/develop/package.json) nennen die Umgebungs- und Portauswahl; die [PlatformIO-Dokumentation](https://docs.platformio.org/en/latest/integration/ide/vscode.html#project-tasks) beschreibt die Projektaufgaben.

Verwende für alle folgenden Schritte dieselbe, zu Deiner Flashgröße passende Umgebung. Ihre Auswahl in VS Code erfordert keine Änderung an `platformio.ini`.

## 5. Die Firmware zunächst bauen

Starte unter **Project Tasks > Deine Umgebung > General > Build** den Build. Dabei wird aus dem Quellcode die Firmware erstellt; das Board muss hierfür noch nicht angeschlossen sein. PlatformIO lädt die in der Projektkonfiguration festgelegten Werkzeuge und Abhängigkeiten. Der erste Durchlauf kann deshalb deutlich länger dauern als spätere Builds.

Warte auf **SUCCESS**. Bei einem Fehler behebst Du zuerst dessen Ursache anhand der Ausgabe im Terminalbereich von VS Code; fahre erst nach einem erfolgreichen Build mit der Übertragung fort.

## 6. Für die Erstinstallation UART1 anschließen

1. Trenne alle Stromversorgungen des Boards, einschließlich einer eventuell angeschlossenen Batterie.
2. Stelle den UART-Umschalter auf **UART1**.
3. Verbinde den Rechner über das Datenkabel mit der USB-C-Buchse **UART1** beziehungsweise **UART**.
4. Warte, bis das Betriebssystem den USB-UART-Wandler erkannt hat.

Das Board besitzt zwei unterschiedliche USB-C-Verbindungen. Für diese Anleitung gilt:

| Phase | Buchse am Board | UART-Umschalter |
| --- | --- | --- |
| Erste Übertragung auf das fabrikneue Board | UART1 / UART | UART1 |
| Zweite Übertragung zur Prüfung und spätere Aktualisierungen | USB | UART2 |

### Den Geräteanschluss in VS Code wählen

Öffne die Befehlspalette und wähle **PlatformIO: Set Project Port (upload/monitor/test)**. Wähle den seriellen Anschluss des angeschlossenen Boards. Wenn mehrere Geräte angezeigt werden, vergleiche die Liste vor dem Abziehen und nach dem Wiederanstecken des USB-Kabels. Bei nur einem passenden angeschlossenen Gerät kann auch die automatische Porterkennung verwendet werden.

| Betriebssystem | Beispiele für Portnamen | Zusätzliche Orientierung |
| --- | --- | --- |
| Windows | `COM5`, `COM6` | Im Geräte-Manager unter **Anschlüsse (COM & LPT)** nachsehen. |
| Linux | `/dev/ttyACM0`, `/dev/ttyUSB0` | Bei verweigertem Zugriff die Geräteberechtigungen prüfen, siehe Abschnitt 11. |
| macOS | `/dev/cu.usbmodem...`, `/dev/cu.usbserial...` | Den nach dem Anstecken hinzugekommenen seriellen `/dev/cu.`-Anschluss wählen. Der genaue Name hängt vom Gerät und Treiber ab. |

Die Beispiele sind keine festen Vorgaben. Beim unter Linux geprüften Board erschien der UART1-Anschluss als **USB Single Serial** mit der USB-Kennung **1A86:55D3** unter `/dev/ttyACM0`. Andere Treiber können eine andere Beschreibung oder einen anderen Portnamen anzeigen. Espressif beschreibt das [Erkennen des seriellen Anschlusses unter Windows, Linux und macOS](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/establish-serial-connection.html). Bei fehlendem Anschluss findest Du Hinweise zu Windows- und macOS-Treibern in Abschnitt 11. Die alternative Geräteliste per Terminal ist in Abschnitt 10 beschrieben.

[![Waveshare-Board und Anschlüsse](bilder/aufbau/20260529_212225-klein.jpg)](bilder/aufbau/20260529_212225.jpg)

## 7. RetroRadio erstmals übertragen

Schließe offene serielle Monitore, damit sie den Geräteanschluss nicht belegen. Starte unter **Project Tasks > Deine Umgebung > General > Upload** die Übertragung. Kontrolliere in der Ausgabe, dass die gewählte Umgebung und der Geräteanschluss zu Deinem Board passen.

PlatformIO baut bei Bedarf und überträgt die Firmware. Warte auf **SUCCESS** und trenne währenddessen das Kabel nicht. Drücke anschließend einmal **RESET** und prüfe, ob RetroRadio auf dem Display erscheint und auf Berührungen reagiert.

Fahre erst nach diesem erfolgreichen Start mit dem Wechsel zur anderen Buchse fort. RetroRadio schaltet beim Start die USB-Funktion des Boards frei.

## 8. Auf USB umstellen und erneut übertragen

1. Trenne wieder alle Stromversorgungen, einschließlich einer eventuell angeschlossenen Batterie.
2. Stelle den UART-Umschalter auf **UART2**.
3. Stecke das Datenkabel in die andere USB-C-Buchse mit der Beschriftung **USB** und verbinde sie mit dem Rechner.
4. Lass RetroRadio starten.
5. Öffne die Portauswahl aus Abschnitt 6 erneut und wähle den jetzt erkannten Anschluss der USB-Buchse. Ein zuvor fest ausgewählter UART-Port muss gegebenenfalls ersetzt werden.
6. Starte **Upload** in derselben PlatformIO-Umgebung wie zuvor. Prüfe wieder **SUCCESS** und den anschließenden Start von RetroRadio. Falls nötig, drücke einmal **RESET**.

Für den nativen USB-Anschluss wird **USB JTAG/serial debug unit** mit der Kennung **303A:1001** erwartet. Beschreibung und Portname hängen vom Betriebssystem ab. Der Port kann sich beim Buchsenwechsel ändern oder gleich bleiben; beim geprüften Linux-Gerät hieß er vorher und nachher `/dev/ttyACM0`. Verlasse Dich deshalb nicht allein auf einen unveränderten Portnamen.

> Wenn auch die zweite Übertragung erfolgreich ist und RetroRadio startet, ist die Installation samt USB-Upload abgeschlossen. Für spätere Aktualisierungen bleiben das Kabel in **USB** und der UART-Umschalter auf **UART2**.

## 9. Erste Einstellungen und Audio-Aufbau

### WLAN und Uhrzeit einrichten

1. Öffne am Gerät **Einstellungen > WLAN** und wähle ein Profil.
2. Tippe auf **scannen**, wähle Dein Netzwerk und gib über **passwort** das zugehörige Passwort ein.
3. Führe **test** aus und kontrolliere den erfolgreichen Verbindungsaufbau. Prüfe, dass das Profil über seinen Schalter aktiviert ist.
4. Tippe auf **speichern** und wähle im anschließenden Dialog **neustart**, damit die gespeicherten Änderungen übernommen werden.
5. Prüfe danach unter **Einstellungen > Zeit & Zeitserver** die Zeitzone und den Zeitserver. Speichere geänderte Einstellungen und folge dem angebotenen Neustart.

Die WLAN- und Zeitangaben werden am Gerät eingerichtet. Für die Installation brauchst Du keine eigenen Zugangsdaten in den Projektdateien einzutragen. Weitere Dienste für Kalender, Kontakte oder Aufgaben werden gesondert eingerichtet.

| WLAN einrichten | Zeit einstellen |
| --- | --- |
| [![WLAN](bilder/bedienung/einstellungen-wlan-klein.png)](bilder/bedienung/einstellungen-wlan.png) | [![Zeit](bilder/bedienung/einstellungen-zeit-klein.png)](bilder/bedienung/einstellungen-zeit.png) |

### Audio-Hardware anschließen

Trenne vor dem Anschließen der Audio-Baugruppen erneut alle Stromversorgungen. Verwende die [Gesamtverdrahtung](hardware/pläne/RetroRadio-Verdrahtungsplan-1.png), die [Detailseite](hardware/pläne/RetroRadio-Verdrahtungsplan-2.png) und den [Verdrahtungsplan als PDF](hardware/pläne/RetroRadio-Verdrahtungsplan.pdf). Die am UART2-Anschluss herausgeführten GPIO43 und GPIO44 werden für I2S-Audio verwendet. Der UART-Umschalter bleibt auf **UART2**.

Für spätere Firmware-Aktualisierungen öffnest Du denselben Projektordner in VS Code, wählst die richtige Umgebung und den aktuellen USB-Port und startest **Upload**. Die vorbereitende Übertragung über UART1 ist bei einem laufenden RetroRadio nicht erneut nötig.

## 10. Alternative: die Übertragung per Terminal

Du kannst denselben Ablauf auch mit Befehlen ausführen. Die Buchsen, Schalterstellungen und Prüfschritte aus den Abschnitten 6 bis 8 gelten dabei unverändert.

Öffne im bereits geöffneten Projekt über die Befehlspalette **PlatformIO: New Terminal**. Dieses Terminal verwendet die mit PlatformIO eingerichteten Werkzeuge. Die Befehle gelten für PowerShell beziehungsweise die Eingabeaufforderung unter Windows ebenso wie für die üblichen Terminals unter Linux und macOS.

Zeige zunächst die vorhandenen seriellen Geräte an:

```text
pio device list
```

Wähle genau einen der folgenden Upload-Befehle nach Deiner Flashgröße. **Ersetze `/dev/ttyACM0` durch den tatsächlich angezeigten Geräteanschluss**, beispielsweise `COM5` unter Windows oder den vollständigen `/dev/cu.`-Portnamen unter macOS.

**8 MB Flash:**

```text
pio run -e waveshare_esp32s3_lcd7_8mb -t upload --upload-port /dev/ttyACM0
```

**16 MB Flash:**

```text
pio run -e waveshare_esp32s3_lcd7_16mb -t upload --upload-port /dev/ttyACM0
```

Nach dem Wechsel zur USB-Buchse zeigst Du die Geräte erneut an und wiederholst den passenden Upload-Befehl mit dem jetzt geltenden Port. Warte bei beiden Übertragungen auf **SUCCESS** und kontrolliere anschließend den Start des Radios. Die offizielle Befehlsreferenz beschreibt [die Geräteliste](https://docs.platformio.org/en/latest/core/userguide/device/cmd_list.html) und [Umgebung, Upload-Ziel und Portwahl](https://docs.platformio.org/en/latest/core/userguide/cmd_run.html).

## 11. Hilfe bei Problemen

### PlatformIO erkennt das Projekt nicht

Prüfe, ob Du den entpackten oder geklonten Ordner mit `platformio.ini` geöffnet hast. Ein übergeordneter Downloadordner oder nur `src` reicht nicht. Kontrolliere außerdem, ob die PlatformIO-Erweiterung fertig eingerichtet ist und der Arbeitsbereich für diese Projektquelle freigegeben wurde. Lade das VS-Code-Fenster bei Bedarf neu.

### Die falsche Flashvariante wird gebaut

Wähle die Umgebung wie in Abschnitt 4 erneut aus und starte **Build** beziehungsweise **Upload** unter genau dieser Umgebung. Die Projektvoreinstellung ist 8 MB. Für 16 MB ist der ausdrücklich benannte Eintrag `waveshare_esp32s3_lcd7_16mb` nötig. Im Terminal legt `-e` die Umgebung für den jeweiligen Befehl fest.

### Kein passender serieller Anschluss erscheint

Prüfe Datenkabel, Buchse und UART-Umschalter anhand der Tabelle in Abschnitt 6. Verbinde das Board möglichst direkt mit dem Rechner. Die Buchse **USB** wird in diesem Ablauf erst nach dem erfolgreichen ersten Start von RetroRadio verwendet.

- **Windows:** Prüfe im Geräte-Manager, ob der UART-Wandler erkannt wird oder ein Treiber fehlt. Die UART1-Buchse verwendet laut [Waveshare-FAQ](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7/FAQ) einen CH343. Falls erforderlich, verwende den passenden [CH343-Treiber von WCH](https://www.wch-ic.com/downloads/CH343SER_ZIP.html) und dessen Installationsanleitung. Verbinde das Board danach neu.
- **macOS:** Wenn der UART1-Anschluss trotz geeignetem Datenkabel nicht erscheint, prüfe den [WCH-Treiber für macOS](https://www.wch-ic.com/downloads/CH341SER_MAC_ZIP.html). Das Paket nennt auch den CH343 und enthält eine Installationsanleitung. Beachte die Hinweise für Deine macOS-Version und verbinde das Board anschließend erneut.
- **Linux:** Prüfe, ob ein `/dev/ttyACM...`- oder `/dev/ttyUSB...`-Anschluss hinzugekommen ist. Ein sichtbarer Anschluss mit verweigertem Zugriff ist ein Berechtigungsproblem, siehe den folgenden Abschnitt.

### Unter Linux wird der Zugriff verweigert

Bei `Permission denied` richte die [PlatformIO-Geräteberechtigungen unter Linux](https://docs.platformio.org/en/latest/core/installation/udev-rules.html) für Deine Distribution ein. Die offizielle Anleitung beschreibt udev-Regeln und alternativ die passende Gruppenmitgliedschaft. Nach geänderten Regeln das Board abziehen und neu verbinden; nach einer Änderung der Gruppenmitgliedschaft zusätzlich ab- und wieder anmelden. Starte VS Code oder PlatformIO nicht ersatzweise mit `sudo`.

### Der Upload bleibt bei der Verbindungsaufnahme hängen

Schließe serielle Monitore und andere Programme, die auf denselben Port zugreifen. Prüfe den ausgewählten Port erneut, insbesondere nach dem Buchsenwechsel.

Für einen erneuten Versuch über die UART-Buchse mit Schalterstellung **UART1** kannst Du den Download-Modus manuell aufrufen: **BOOT gedrückt halten, RESET kurz drücken, BOOT loslassen**. Starte dann den Upload erneut und drücke nach erfolgreicher Übertragung einmal **RESET**.

Falls ein bereits installiertes Radio über die USB-Buchse nicht mehr erreichbar ist, kannst Du den Installationsweg über UART1 aus Abschnitt 6 wiederholen. Trenne vor dem Umstellen die Stromversorgung und angeschlossene Audio-Hardware.

### `pio` wird nicht gefunden

Verwende das in Abschnitt 10 beschriebene PlatformIO-Terminal in VS Code und warte, bis die Erweiterung fertig eingerichtet ist. Ein gewöhnliches Systemterminal kennt den Installationspfad möglicherweise noch nicht. Dafür ist keine zweite PlatformIO-Installation erforderlich. Für die Nutzung außerhalb von VS Code beschreibt PlatformIO die [Einrichtung der Shell-Befehle](https://docs.platformio.org/en/latest/core/installation/shell-commands.html).

### Der Build schlägt beim Download von Paketen fehl

Prüfe die Internetverbindung und die Fehlermeldung im Terminalbereich. Bei einem verwalteten Rechner können ein Proxy oder eingeschränkte Downloadrechte die Einrichtung verhindern. Die benötigten Bibliotheks- und Plattformversionen stehen bereits in `platformio.ini`; ersetze sie nicht pauschal durch andere Versionen.

## Technischer Hintergrund und Prüfstand

RetroRadio setzt beim Start den USB/CAN-Umschalter über `expanderPins::USB_SEL` auf USB und aktiviert USB-CDC über die Build-Einstellungen. Der [Waveshare-Hinweis zur USB-Umschaltung und den UART-Anschlüssen](https://docs.waveshare.com/ESP32-S3-Touch-LCD-7/FAQ) erklärt die Boardfunktion. Für den beschriebenen RetroRadio-Ablauf gelten zusätzlich die [Projektkonfiguration](../platformio.ini), die [Initialisierung](../src/main.cpp) und die [Pinzuordnung](../include/Pins.h).

Die dokumentierte Geräteprüfung vom 23.09.2026 betrifft PCB 1.1 mit 8 MB Flash und 8 MB PSRAM unter Linux. Die redaktionelle Überarbeitung vom 28.10.2026 ergänzt den grafischen Ablauf und die Hinweise für Windows und macOS; sie ist kein neuer Build-, Upload- oder Gerätetest.
