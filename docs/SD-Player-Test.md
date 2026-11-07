# SD-Player-Test: Wiederholung und Kassettenfortschritt

Dieser Testbranch basiert auf dem aktuellen öffentlichen Stand nach v26.11.6 und ergänzt Wiederholungsmodi sowie die Bandanzeige für die gesamte Wiedergabeliste. Die Versionsanzeige der Firmware lautet weiterhin v26.11.6. Den genauen Teststand deshalb über den Git-Commit festhalten.

## Bedienung

| Aktion | Ergebnis |
| --- | --- |
| Play kurz im Stopp | Normale Wiedergabe starten, Play-Symbol orange. Die Liste läuft einmal bis zum Ende. |
| Play kurz während normaler Wiedergabe | Aktuellen Titel wiederholen, Play-Symbol grün. |
| Play kurz bei laufender Wiedergabe mit aktiver Wiederholung | Wiederholung ausschalten, Play-Symbol orange. |
| Play mindestens 800 ms halten und loslassen | Gesamte Wiedergabeliste wiederholen, Play-Symbol rot. Im Stopp wird gestartet, in der Pause fortgesetzt. |
| Play kurz während einer Pause | Wiedergabe fortsetzen, Wiederholungsmodus erhalten. |
| Pause, Titelsprung oder direkte Titelwahl | Wiederholungsmodus erhalten. |
| Stopp, Liste leeren oder Player verlassen | Wiederholung ausschalten. |

Die rote Wiederholung gilt für alle Titel in der aktuellen Wiedergabeliste, auch wenn sie aus mehreren Ordnern stammen. Der Modus wird nicht dauerhaft gespeichert. Zweimaliges Tippen aus dem Stopp startet die Wiedergabe und aktiviert anschließend die Titelwiederholung.

Die Kassette zeigt die Position innerhalb der gesamten Liste. Jeder Titel erhält denselben Bandanteil, innerhalb dieses Anteils läuft das Band mit der Spielzeit weiter. Bei zehn Titeln bedeutet die Mitte des fünften Titels 45 Prozent. Am Ende des letzten Titels ist das Band vollständig rechts aufgewickelt. Der Fortschrittsbalken zeigt weiterhin den aktuellen Titel. Die Kassette ist keine genaue Restspielzeitanzeige.

## Kurzer Gerätetest

1. Mehrere Titel laden. Play drücken, anschließend nochmals kurz drücken. Das Symbol muss grün werden, ohne den laufenden Titel neu zu starten.
2. Über den Fortschrittsbalken kurz vor das Titelende springen. Bei Grün muss derselbe Titel erneut beginnen. Ein weiterer kurzer Play-Druck während der Wiedergabe schaltet auf Orange zurück.
3. Play mindestens 800 ms halten und loslassen. Bei Rot zum letzten Titel springen und dessen Ende erreichen. Danach muss die Liste wieder bei Titel 1 beginnen.
4. In Grün und Rot Pause, Fortsetzen, Vor/Zurück und direkte Titelwahl prüfen. Der Modus muss erhalten bleiben. Stopp muss ihn löschen, der nächste kurze Play-Druck startet normal.
5. Die Bandverteilung beim ersten, mittleren und letzten Titel sowie bei Titelsprüngen im Stopp prüfen. Am normalen Listenende bleibt das Band rechts voll. Bei Listenwiederholung beginnt es wieder links voll.
6. Eine Liste mit nur einem Titel und eine leere Liste prüfen. Bei leerer Liste dürfen kurzer und langer Play-Druck keine Wiedergabe auslösen.

## Bauen

Die Umgebung passend zur tatsächlichen Flashgröße des Geräts wählen:

```bash
pio run -e waveshare_esp32s3_lcd7_8mb
pio run -e waveshare_esp32s3_lcd7_16mb
```

Beim Testbericht den verwendeten Commit, die Flashgröße und das beobachtete Verhalten nennen. Die Wiederholungsübergänge und die Bandanzeige lassen sich durch Springen kurz vor das Titelende zügig prüfen.
