#ifndef WORKSHOPTOOL_WORLDCLOCK_H
#define WORKSHOPTOOL_WORLDCLOCK_H

// ============================================================================
// Weltzeituhr für acht Metropolen
// ============================================================================

#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_WorldClock
 * @brief Statische Weltzeit-Anzeige nach Standardzeit-Modell.
 *
 * Zeigt für acht Metropolen die jeweilige Lokalzeit im Format HH:MM, den Standardzeit-Offset zu UTC und den abgekürzten Wochentag. Berlin ist als Heimat-Zelle farblich hervorgehoben.
 *
 * Standardzeit-Modell: Jede Stadt verwendet ausschließlich ihren Standardzeit-Offset zu UTC und berücksichtigt keine Sommerzeit. Eine stadtbezogene Sommerzeitberechnung benötigt eine IANA-Zeitzonendatenbank mit Übergangstabellen, die auf dem Gerät nicht vorhanden ist. Dadurch dient das Werkzeug als Referenzanzeige und nicht als synchronisierter Wecker.
 *
 * Layout (547 × 327 px Netto-Inhaltsbereich):
 *   TOP_GAP                  10 px
 *   4 × Zellzeilen           4 × 73 px = 292 px
 *   3 × Zeilenabstand         3 ×  8 px =  24 px
 *   Restpuffer unten                       1 px
 *   Summe                                327 px
 *
 * Spaltenaufteilung:
 *   2 × OUTER_PAD_X(4) + 2 × CELL_W(264) + 1 × COL_GAP(10) = 547 px
 *
 * Zeichenfrequenz: Workshop ruft draw() nur dann auf, wenn sich der Inhaltsbereich verändert. Diese Klasse steht in der Cursor-Blink-Ausnahmeliste von Workshop und erhält stattdessen ein gezieltes invalidateWorkContent() bei jedem Minutenwechsel der Systemzeit.
 *
 * Touch: Inhaltsbereich nicht interaktiv, kein Footer-Toggle. handleTouch liefert immer false.
 */
class WorkshopTool_WorldClock : public WorkshopTool
{
public:
    WorkshopTool_WorldClock()                                           = default;
    ~WorkshopTool_WorldClock() override                                 = default;

    WorkshopTool_WorldClock(const WorkshopTool_WorldClock &)            = delete;
    WorkshopTool_WorldClock &operator=(const WorkshopTool_WorldClock &) = delete;

    /**
     * @brief Zeichnet das 2×4-Stadtgitter in den Work-Sprite.
     * @param sprite Ziel-Sprite (m_workSprite des Workshop-Moduls)
     * @param ox     X-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param oy     Y-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param w      Verfügbare Breite in Pixeln
     * @param h      Verfügbare Höhe in Pixeln
     */
    void draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse im Inhaltsbereich.
     * @return immer false, da der Inhaltsbereich nicht interaktiv ist
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    // ========================================================================
    // INNERE STRUKTUREN
    // ========================================================================

    /**
     * @brief Statische Beschreibung einer Stadt im Raster.
     */
    struct City
    {
        WorkshopKey nameKey;      ///< Anzeigename (i18n-Key)
        int16_t     stdOffsetMin; ///< Standardzeit-Offset zu UTC in Minuten
        bool        isHome;       ///< Markiert die Heimat-Zelle (Berlin)
    };

    // ========================================================================
    // STATISCHE DATENBASIS
    // ========================================================================

    static const City s_cities[];      ///< Acht Städte im Anzeigeraster
    static const int  CITY_COUNT;      ///< Anzahl Einträge in s_cities[]
};

#endif // WORKSHOPTOOL_WORLDCLOCK_H
