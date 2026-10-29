#ifndef TOOLS_H
#define TOOLS_H

#include <stdint.h>

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

/**
 * @brief Gibt FPS, Heap-/PSRAM-Metriken und WiFi-Signalstärke periodisch auf dem Logger aus.
 * @param seconds Intervall in Sekunden zwischen den Ausgaben
 */
void fps(uint32_t seconds);

// Messzähler für die Visualisierungs-Diagnose; fps() gibt sie als Rate aus und setzt sie zurück.
extern volatile uint32_t g_visDataCount;  ///< an den Visualizer übergebene Audio-Datenblöcke
extern volatile uint32_t g_audioCbCount;  ///< Aufrufe des Audio-Callbacks aus der Library

#endif // TOOLS_H
