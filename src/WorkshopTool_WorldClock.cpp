// ===================================================================
// WorkshopTool_WorldClock.cpp - Weltzeit-Uhr für acht Metropolen
// ===================================================================

#include "WorkshopTool_WorldClock.h"

#include <time.h>

#include "I18n.h"
#include "StringUtils.h"

#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold30.h"

// ===================================================================
// Anonymer Namespace - file-local Konstanten
// ===================================================================

namespace
{
    // ===================================================================
    // TOP_GAP bestimmt den oberen Abstand des Inhaltsbereichs
    // ===================================================================
    constexpr int TOP_GAP = 10; ///< Abstand zwischen Header und erster Zellzeile

    // ===================================================================
    // Rasterabmessungen (Inhaltsbereich 547 × 327 px)
    // ===================================================================

    // Vertikal: TOP_GAP(10) + 4×CELL_H(73) + 3×ROW_GAP(8) + 1px Restpuffer = 327 Horizontal: 2×OUTER_PAD_X(4) + 2×CELL_W(264) + 1×COL_GAP(10) = 547
    constexpr int OUTER_PAD_X = 4;   ///< Linker und rechter Aussenrand
    constexpr int COL_GAP     = 10;  ///< Abstand zwischen den beiden Spalten
    constexpr int ROW_GAP     = 8;   ///< Abstand zwischen den vier Zeilen
    constexpr int CELL_W      = 264; ///< Zellbreite
    constexpr int CELL_H      = 73;  ///< Zellhöhe
    constexpr int CELL_R      = 6;   ///< Eckenradius der Zell-Hintergründe

    // ===================================================================
    // Innenlayout einer Zelle (264 × 73 px)
    // ===================================================================

    // Obere Reihe: Stadtname links, UTC-Offset rechts (beide Bold16, top-aligned) Untere Reihe: Zeit links (Bold30), Wochentag rechts (Bold14)
    constexpr int CELL_PAD_X = 12; ///< Innenabstand links und rechts
    constexpr int CITY_Y     = 8;  ///< Y-Position Stadtname  (TL_DATUM, Bold16)
    constexpr int OFFSET_Y   = 8;  ///< Y-Position Offset-Badge (TR_DATUM, Bold16)
    constexpr int TIME_Y     = 36; ///< Y-Position Zeit       (TL_DATUM, Bold30)
    constexpr int WDAY_Y     = 47; ///< Y-Position Wochentag  (TR_DATUM, Bold14)

    // ===================================================================
    // Farben - abgeleitet aus dem zentralen Workshop-Schema
    // ===================================================================
    constexpr uint16_t C_CELL_NORMAL = 0x2945; ///< Normale Zelle (= C_FIELD_INPUT)
    constexpr uint16_t C_CELL_HOME   = 0x328A; ///< Heimat-Zelle Berlin (= C_FIELD_FOCUS)

    // Stadtname und UTC-Offset teilen sich die Top-Row-Farbe, damit sie als zusammengehörige Kopfzeile gelesen werden.
    constexpr uint16_t C_TOP_NORMAL = 0xDEDB; ///< Top-Row normal (= C_LBL_INPUT)
    constexpr uint16_t C_TOP_HOME   = 0xFFFF; ///< Top-Row Heimat (= C_LBL_FOCUS)

    constexpr uint16_t C_TIME_NORMAL = 0xEF7D; ///< Zeit-Text normal (= C_VAL_INPUT)
    constexpr uint16_t C_TIME_HOME   = 0xFFFF; ///< Zeit-Text Heimat (= C_VAL_FOCUS)

    constexpr uint16_t C_WDAY_NORMAL = 0x8410; ///< Wochentag normal (= C_UNIT_INPUT)
    constexpr uint16_t C_WDAY_HOME   = 0xAD75; ///< Wochentag Heimat (= C_UNIT_FOCUS)

    // ===================================================================
    // Pufferdimensionierung
    // ===================================================================

    // Längster Offset-String: "UTC -10h" oder "UTC +12h" -> 8 Zeichen + Null. Längster Zeit-String: "HH:MM" -> 5 Zeichen + Null. Wochentagsabkürzung über I18n::copyWeekday in einen 8-Byte-Puffer.
    constexpr size_t OFFSET_BUF_SIZE = 12;
    constexpr size_t TIME_BUF_SIZE   = 8;
    constexpr size_t WDAY_BUF_SIZE   = 8;

    /**
     * @brief Lokaler Datensatz für eine Zelle, vorberechnet vor dem Zeichnen.
     *
     * Wird einmal pro Zelle im Stack-Array befüllt und anschliessend in mehreren font-spezifischen Pässen abgearbeitet, damit jede Schrift nur einmal pro Frame geladen und entladen werden muss.
     */
    struct CellRender
    {
        int                          x;                          ///< Linke Kante der Zelle (sprite-relativ)
        int                          y;                          ///< Obere Kante der Zelle (sprite-relativ)
        const __FlashStringHelper   *cityName;                   ///< Stadtname (PROGMEM-Pointer)
        char                         wdayBuf[WDAY_BUF_SIZE];     ///< Lokalisierte Wochentagsabkürzung
        char                         timeBuf[TIME_BUF_SIZE];     ///< Vorformatierte Zeit "HH:MM"
        char                         offsetBuf[OFFSET_BUF_SIZE]; ///< Vorformatierter UTC-Offset
        uint16_t                     bgColor;                    ///< Zellhintergrund
        uint16_t                     topColor;                   ///< Farbe für Stadtname und Offset
        uint16_t                     timeColor;                  ///< Farbe für Uhrzeit
        uint16_t                     wdayColor;                  ///< Farbe für Wochentag
    };

} // namespace

// ===================================================================
// STATISCHE DATENBASIS
// ===================================================================

// Acht Städte, geografisch von West nach Ost, zeilenweise gelesen:
//   Zeile 0: Los Angeles | New York
//   Zeile 1: London      | Berlin (Heimat)
//   Zeile 2: Moskau      | Dubai
//   Zeile 3: Tokio       | Sydney
const WorkshopTool_WorldClock::City
WorkshopTool_WorldClock::s_cities[] = {
    {WorkshopKey::WORKSHOP_WORLDCLOCK_CITY_LOS_ANGELES, -8 * 60, false},
    {WorkshopKey::WORKSHOP_WORLDCLOCK_CITY_NEW_YORK,    -5 * 60, false},
    {WorkshopKey::WORKSHOP_WORLDCLOCK_CITY_LONDON,            0, false},
    {WorkshopKey::WORKSHOP_WORLDCLOCK_CITY_BERLIN,       1 * 60, true },
    {WorkshopKey::WORKSHOP_WORLDCLOCK_CITY_MOSCOW,       3 * 60, false},
    {WorkshopKey::WORKSHOP_WORLDCLOCK_CITY_DUBAI,        4 * 60, false},
    {WorkshopKey::WORKSHOP_WORLDCLOCK_CITY_TOKYO,        9 * 60, false},
    {WorkshopKey::WORKSHOP_WORLDCLOCK_CITY_SYDNEY,      10 * 60, false},
};

const int WorkshopTool_WorldClock::CITY_COUNT =
    static_cast<int>(sizeof(WorkshopTool_WorldClock::s_cities) /
                     sizeof(WorkshopTool_WorldClock::s_cities[0]));

// ===================================================================
// Touch im Inhaltsbereich
// ===================================================================

/**
 * @brief Inhaltsbereich ist nicht interaktiv.
 */
bool WorkshopTool_WorldClock::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    (void)tx;
    (void)ty;
    (void)touched;
    return false;
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet das 2×4-Stadtgitter in den Work-Sprite.
 *
 * Ablauf in vier Pässen, damit jede der drei Schriften nur einmal geladen werden muss:
 *   1. Vorberechnung aller Zell-Daten + Hintergründe
 *   2. Bold16 für Stadtname und UTC-Offset (obere Reihe)
 *   3. Bold30 für die Uhrzeit
 *   4. Bold14 für den Wochentag
 */
void WorkshopTool_WorldClock::draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h)
{
    (void)w;
    (void)h;

    if (!sprite)
        return;

    // Aktuellen UTC-Epoch aus der ESP32-Systemzeit (NTP/RTC-synchronisiert).
    const long utcEpoch = static_cast<long>(time(nullptr));

    const int row0Y = oy + TOP_GAP;
    const int col0X = ox + OUTER_PAD_X;
    const int col1X = col0X + CELL_W + COL_GAP;

    // ===================================================================
    // Pass 1: Zell-Daten vorberechnen und Hintergründe zeichnen
    // ===================================================================
    CellRender cells[8]; // CITY_COUNT ist 8, statisches Array ohne VLA

    for (int idx = 0; idx < CITY_COUNT; ++idx)
    {
        const City &city = s_cities[idx];
        CellRender &c    = cells[idx];

        // Zellposition im Raster
        const int row = idx / 2;
        const int col = idx % 2;
        c.x = (col == 0) ? col0X : col1X;
        c.y = row0Y + row * (CELL_H + ROW_GAP);

        // Lokalzeit der Stadt: UTC-Epoch plus Standardoffset
        const int32_t offsetSec = static_cast<int32_t>(city.stdOffsetMin) * 60;
        const time_t  target    = static_cast<time_t>(utcEpoch + offsetSec);
        struct tm     tmCity;
        gmtime_r(&target, &tmCity);

        // UTC-Offset als Badge formatieren ("UTC", "UTC +5h", "UTC -8h")
        const int offsetHours = static_cast<int>(city.stdOffsetMin / 60);
        if (offsetHours == 0)
        {
            stringutils::safeCopy(c.offsetBuf, "UTC");
        }
        else if (offsetHours > 0)
        {
            stringutils::safeFormat(c.offsetBuf, sizeof(c.offsetBuf),
                                    "UTC +%dh", offsetHours);
        }
        else
        {
            // %d liefert das Minuszeichen automatisch
            stringutils::safeFormat(c.offsetBuf, sizeof(c.offsetBuf),
                                    "UTC %dh", offsetHours);
        }

        // Uhrzeit "HH:MM"
        stringutils::safeFormat(c.timeBuf, sizeof(c.timeBuf), "%02d:%02d",
                                tmCity.tm_hour, tmCity.tm_min);

        // Wochentagsabkürzung
        const int wIdx = (tmCity.tm_wday >= 0 && tmCity.tm_wday <= 6)
                             ? tmCity.tm_wday
                             : 0;
        I18n::copyWeekday(c.wdayBuf, sizeof(c.wdayBuf), wIdx, false);
        c.cityName = I18n::str(city.nameKey);

        // Farbpalette der Zelle
        c.bgColor   = city.isHome ? C_CELL_HOME   : C_CELL_NORMAL;
        c.topColor  = city.isHome ? C_TOP_HOME    : C_TOP_NORMAL;
        c.timeColor = city.isHome ? C_TIME_HOME   : C_TIME_NORMAL;
        c.wdayColor = city.isHome ? C_WDAY_HOME   : C_WDAY_NORMAL;

        // Zell-Hintergrund mit weichen Ecken
        sprite->fillSmoothRoundRect(c.x, c.y, CELL_W, CELL_H, CELL_R, c.bgColor);
    }

    // ===================================================================
    // Pass 2: Bold16 - Stadtname links, UTC-Offset rechts
    // ===================================================================
    sprite->loadFont(ComfortaaBold16);
    for (int idx = 0; idx < CITY_COUNT; ++idx)
    {
        const CellRender &c = cells[idx];

        sprite->setTextColor(c.topColor, c.bgColor);

        sprite->setTextDatum(TL_DATUM);
        sprite->drawString(c.cityName, c.x + CELL_PAD_X, c.y + CITY_Y);

        sprite->setTextDatum(TR_DATUM);
        sprite->drawString(c.offsetBuf, c.x + CELL_W - CELL_PAD_X, c.y + OFFSET_Y);
    }
    sprite->unloadFont();

    // ===================================================================
    // Pass 3: Bold30 - Uhrzeit
    // ===================================================================
    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(TL_DATUM);
    for (int idx = 0; idx < CITY_COUNT; ++idx)
    {
        const CellRender &c = cells[idx];
        sprite->setTextColor(c.timeColor, c.bgColor);
        sprite->drawString(c.timeBuf, c.x + CELL_PAD_X, c.y + TIME_Y);
    }
    sprite->unloadFont();

    // ===================================================================
    // Pass 4: Bold14 - Wochentag
    // ===================================================================
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TR_DATUM);
    for (int idx = 0; idx < CITY_COUNT; ++idx)
    {
        const CellRender &c = cells[idx];
        sprite->setTextColor(c.wdayColor, c.bgColor);
        sprite->drawString(c.wdayBuf, c.x + CELL_W - CELL_PAD_X, c.y + WDAY_Y);
    }
    sprite->unloadFont();
}
