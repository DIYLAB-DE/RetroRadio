# Google-Kalender, Kontakte und Aufgaben

RetroRadio meldet sich nicht direkt bei Deinem Google-Konto an. Drei kleine Google-Apps-Script-Web-Apps stellen die benötigten Daten für das Gerät bereit. Jede Anwendung erhält eine eigene Adresse. Diese Anleitung gilt gleichermaßen unter Windows, Linux und macOS, da die Einrichtung im Browser erfolgt.

## Dateien und Berechtigungen

| Projekt | Datei | Benötigter Dienst | Was das Script tut |
| --- | --- | --- | --- |
| RetroRadio Kalender | [ESP32-Calendar-API.gs](apps-script/ESP32-Calendar-API.gs) | Eingebauter Dienst CalendarApp | Termine und Monatsübersichten lesen |
| RetroRadio Kontakte | [ESP32-Contacts-API.gs](apps-script/ESP32-Contacts-API.gs) | Erweiterter Dienst People API, Kennung People | Namen, Telefonnummern, Adressen, Organisation und Geburtstage lesen |
| RetroRadio Aufgaben | [ESP32-ToDo-API.gs](apps-script/ESP32-ToDo-API.gs) | Erweiterter Dienst Tasks API, Kennung Tasks | Aufgaben lesen und ihren Erledigt-Zustand ändern |

**Drei getrennte Script-Projekte anlegen.** Die Dateien enthalten jeweils eine Funktion doGet und teilweise gleichnamige Konfigurationen. Sie gehören deshalb nicht gemeinsam in ein einziges Projekt.

Die bereitgestellten Dateien enthalten keine persönliche Kalenderkennung und keine fertige Web-App-Adresse. Die Kontoauswahl erfolgt bei der Installation in Deinem Google-Konto.

## 1. Projekte anlegen

Öffne [Google Apps Script](https://script.google.com/) mit dem Google-Konto, dessen Daten auf dem Radio erscheinen sollen. Erstelle ein neues Projekt und benenne es etwa **RetroRadio Kalender**. Ersetze den Inhalt der anfänglichen Datei Code.gs durch den vollständigen Inhalt des zugehörigen Scripts aus der Tabelle. Speichere das Projekt. Wiederhole das für Kontakte und Aufgaben.

Stelle in den Projekteinstellungen die zu Deinem Einsatz passende Zeitzone ein, für Deutschland etwa **Europe/Berlin**. Die Scripts verwenden die Projektzeitzone für Datumsangaben. Radio und Script-Projekte sollten dieselbe Ortszeit verwenden.

## 2. Dienste hinzufügen

Im Kontakte-Projekt links im Editor neben **Dienste** auf **+** klicken und **People API** hinzufügen. Die Kennung muss **People** lauten. Im Aufgaben-Projekt entsprechend **Tasks API** mit der Kennung **Tasks** hinzufügen. Das Kalender-Projekt nutzt CalendarApp und benötigt keinen zusätzlichen erweiterten Kalenderdienst.

Bei einem automatisch angelegten Google-Cloud-Projekt wird die zugehörige API damit automatisch aktiviert. Bei einem selbst zugeordneten Standard-Cloud-Projekt muss sie zusätzlich in dessen Cloud Console aktiviert werden. [Google: Erweiterte Dienste aktivieren](https://developers.google.com/apps-script/guides/services/advanced).

## 3. Eigene Auswahl einstellen

### Kalender

Im Kalender-Script enthält calendarIds zunächst nur **primary**. Dieser Eintrag verwendet den Hauptkalender des ausführenden Kontos. Weitere eigene oder freigegebene Kalender können mit ihrer Kalender-ID ergänzt werden. Die ID findest Du in Google Kalender in den Einstellungen des jeweiligen Kalenders unter **Kalender integrieren**. Verwende die Kalender-ID, nicht dessen öffentliche Browseradresse. Hauptkalender werden im Script über [CalendarApp.getDefaultCalendar](https://developers.google.com/apps-script/reference/calendar/calendar-app#getDefaultCalendar()) geöffnet.

Die beiden weiteren Kennungen birthdayCalendarId und holidayCalendarId betreffen den Geburtstagskalender und den deutschen Feiertagskalender. Diese Kalender müssen für das Konto verfügbar sein. Fehlender Zugriff auf einen dieser Zusatzkalender verhindert den übrigen Kalenderabruf nicht.

Die zum Radio passenden Grenzen sind voreingestellt: höchstens **25 kommende Termine**, **5 Termine je Tag** und ein Ausblick über **14 Kalendertage einschließlich heute**. Mehrtägige Termine werden für die betroffenen Tage dargestellt. Die Monatsübersichten betreffen den aktuellen und den nächsten Monat. Die Grenzen nicht unabhängig von der Firmware erhöhen.

### Kontakte

MAX_CONTACTS ist auf **100** gesetzt und entspricht der Geräteanzeige. REQUIRE_PHONE kann auf true gestellt werden, wenn nur Kontakte mit Telefonnummer geliefert werden sollen. DEBUG bleibt normalerweise false. CONTACT_GROUPS ist in dieser Fassung zwar als Konfigurationsfeld vorhanden, wird beim Abruf aber nicht ausgewertet; es filtert deshalb keine Kontaktgruppen.

Das Script liest eine Seite mit höchstens 1000 Kontakten aus der People API, sortiert sie nach Nachname beziehungsweise Vorname und liefert höchstens 100 davon an das Radio. Es ändert keine Kontakte. Die Hilfsfunktionen zur Auflistung schreiben Kontaktdaten in das Ausführungsprotokoll und werden für den normalen Betrieb nicht benötigt.

### Aufgaben

USE_ALL_LISTS steht zunächst auf true. Damit werden die gefundenen Aufgabenlisten verwendet. Für eine feste Auswahl auf false stellen und die gewünschten IDs in TASK_LISTS eintragen; **@default** bezeichnet die Standardliste. Die Hilfsfunktion listTaskLists zeigt die IDs Deiner Listen im Ausführungsprotokoll.

LIST_PRIORITY legt eine bevorzugte Reihenfolge anhand der Listennamen fest. TITLE_PREFIXES kann Aufgaben anhand ihrer Titelanfänge filtern; eine leere Liste filtert nicht. MAX_TASKS ist auf **30** gesetzt und passt zur Firmware. Die Abrufe lesen je Aufgabenliste höchstens 100 Einträge; zusätzliche Ergebnisseiten werden in dieser Fassung nicht nachgeladen.

Das Radio kann Aufgaben als erledigt oder wieder offen markieren. Eine solche Bestätigung verändert den Zustand der Aufgabe im Google-Konto. Das Script legt keine neuen Aufgaben an.

## 4. Ersten Zugriff erlauben

Wähle im Kalender-Projekt die Funktion **doGet** und starte sie im Editor. In den beiden anderen Projekten **testDoGet** starten. Google fordert die zugehörigen Berechtigungen an. Prüfe dabei Konto, Projekt und die angeforderten Zugriffe anhand der Tabelle oben. Die Ausgabe der Testfunktionen kann persönliche Daten enthalten und bleibt in Deinem Konto.

Für ein selbst erstelltes Projekt kann Google einen Hinweis auf eine nicht überprüfte Anwendung anzeigen. Erteile Zugriffe nur dem von Dir angelegten und geprüften Projekt. Bei einem verwalteten Firmen- oder Schulkonto können Administratorvorgaben die Bereitstellung oder die Zustimmung verhindern. [Google: Autorisierung von Apps Script](https://developers.google.com/apps-script/guides/services/authorization).

## 5. Als Web-Apps bereitstellen

In jedem der drei Projekte **Bereitstellen -> Neue Bereitstellung** öffnen, als Typ **Web-App** wählen und eine Beschreibung eintragen. Für den aktuellen Gerätezugriff wird die Ausführung als **Ich** beziehungsweise als Eigentümer benötigt. Der Zugriff muss ohne Google-Anmeldung möglich sein, in der Auswahl üblicherweise **Jeder**. Das Radio kann keinen Google-Anmeldedialog bearbeiten. [Google: Web-Apps bereitstellen](https://developers.google.com/apps-script/guides/web).

**Bedeutung dieser Freigabe:** Die mitgelieferten Scripts prüfen keinen zusätzlichen Zugriffsschlüssel. Wer die jeweilige Bereitstellungsadresse kennt, kann die gelieferten Kalender- oder Kontaktdaten lesen. Über die Aufgabenadresse können außerdem Aufgaben als erledigt oder offen markiert werden. Die Adresse daher wie ein Geheimnis behandeln. Für eine begrenzte Datenauswahl nur die gewünschten Kalender und Aufgabenlisten einbinden beziehungsweise ein dafür vorgesehenes Konto verwenden.

Kopiere nach dem Bereitstellen die Web-App-Adresse. Sie beginnt mit **https://script.google.com/** und endet auf **/exec**. Die Testadresse mit **/dev** eignet sich nicht für das Radio. Öffne die /exec-Adresse zur Kontrolle in einem privaten Browserfenster ohne Google-Anmeldung: Es soll die Datenausgabe erscheinen, keine Anmeldeseite. Persönliche Ausgaben dabei nicht weitergeben.

## 6. Adressen im Radio speichern

Unter **Einstellungen -> Cloud-Anbindung** die drei Adressen in die Felder **Kalender**, **Aufgaben** und **Kontakte** eintragen und speichern. Längere Adressen lassen sich auch über die [Weboberfläche](../../Weboberfläche.md) eingeben. Die vollständige /exec-Adresse übernehmen, ohne zusätzliche Testparameter.

Öffne danach die jeweiligen Anwendungen neu. Kalender, Kontakte und Aufgaben müssen nun Deine Daten anstelle der eingebauten Beispiele anzeigen. Die Tagesansicht verwendet dieselben Verbindungen. Bei Aufgaben zunächst nur eine bewusst gewählte Testaufgabe als erledigt markieren und die Änderung in Google Tasks kontrollieren.

## Änderungen und Abschalten

Nach Änderungen am Script die vorhandene Bereitstellung über **Bereitstellungen verwalten** bearbeiten und eine neue Version auswählen. Nur Speichern im Editor aktualisiert eine bereits versionierte Web-App nicht. Wird eine völlig neue Bereitstellung erstellt, deren neue Adresse auch im Radio eintragen. [Google: Bereitstellungen verwalten](https://developers.google.com/apps-script/concepts/deployments).

Zum Demomodus die betreffende Adresse im Radio leeren und speichern. Zum vollständigen Entziehen des externen Zugriffs zusätzlich die Web-App-Bereitstellung im Google-Konto deaktivieren beziehungsweise archivieren. Das Leeren im Radio beendet nicht die Bereitstellung bei Google.

## Fehler zuordnen

| Anzeige oder Verhalten | Prüfen |
| --- | --- |
| People oder Tasks ist nicht definiert | Richtigen erweiterten Dienst im richtigen Script-Projekt hinzufügen. |
| Anmeldeseite statt Daten | Bereitstellung, Zugriff ohne Anmeldung und Ausführung als Eigentümer prüfen. |
| Änderungen am Code wirken nicht | Neue Version der bestehenden Bereitstellung aktivieren. |
| Leerer Kalender | Konto, Kalender-ID und Zugriffsrecht prüfen; Zusatzkalender müssen abonniert sein. |
| Keine Kontakte | People API und erteilte Berechtigungen prüfen; REQUIRE_PHONE kann Einträge ausfiltern. |
| Aufgaben fehlen | Ausgewählte Listen, Titelpräfixe und die beschriebenen Mengenbegrenzungen prüfen. |
| Gerät meldet ungültige Adresse | Vollständige Adresse mit https://script.google.com/ und /exec verwenden. |
| Falscher Tag oder Zeitversatz | Zeitzone des Script-Projekts und Zeiteinstellung des Radios abgleichen. |

Die Einrichtung neuer Bereitstellungen muss in Deinem eigenen Konto durchgeführt werden. Die lokale Prüfung der mitgelieferten Dateien ersetzt keine erfolgreiche Autorisierung und keinen Abruf in diesem Konto.

Auch die Abfrage der Aufgabenlisten erfolgt einmalig ohne Folgeseiten. Falls nicht alle Listen erscheinen, die Auswahl und die Script-Ausführungen prüfen.
