#ifndef TOOLS_H
#define TOOLS_H

/**
 * @brief Gibt Chip-Modell, Core-Anzahl, Taktfrequenz, Heap-, PSRAM- und Flash-Größen auf dem Logger aus.
 */
void systemInfo();

/**
 * @brief Gibt Kartentyp, Kartengröße, Gesamtplatz und belegten Platz der SD-Karte auf dem Logger aus.
 * @param mount true wenn das Mounten der SD-Karte erfolgreich war, false bei Fehler
 */
void sdCardInfo(bool mount);

/**
 * @brief Gibt den Initialisierungsstatus der RTC auf dem Logger aus.
 * @param init true wenn die RTC-Initialisierung erfolgreich war, false bei Fehler
 */
void rtcInfo(bool init);

#endif // TOOLS_H
