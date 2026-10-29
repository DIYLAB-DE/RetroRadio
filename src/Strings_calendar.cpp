#include "Strings_calendar.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale
// ===================================================================

namespace
{
    // Deutsch
    const char DE_LOADING[]               PROGMEM = "- lade kalenderdaten -";
    const char DE_LOADING_CANCELLED[]     PROGMEM = "- abbruch -";
    const char DE_DATE_HEADER_TODAY_FMT[] PROGMEM = "Heute, {0}., {2:%d}. {1}";
    const char DE_DATE_HEADER_FMT[]       PROGMEM = "{0}., {2:%d}. {1}";
    const char DE_NO_EVENTS_TODAY[]       PROGMEM = "Heute keine Ereignisse";
    const char DE_NO_EVENTS[]             PROGMEM = "Keine Ereignisse";
    const char DE_NO_UPCOMING[]           PROGMEM = "Keine kommenden Termine";

    // Englisch (British)
    const char EN_LOADING[]               PROGMEM = "- loading calendar -";
    const char EN_LOADING_CANCELLED[]     PROGMEM = "- cancelled -";
    const char EN_DATE_HEADER_TODAY_FMT[] PROGMEM = "Today, {0}, {2:%d} {1}";
    const char EN_DATE_HEADER_FMT[]       PROGMEM = "{0}, {2:%d} {1}";
    const char EN_NO_EVENTS_TODAY[]       PROGMEM = "No events today";
    const char EN_NO_EVENTS[]             PROGMEM = "No events";
    const char EN_NO_UPCOMING[]           PROGMEM = "No upcoming events";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const CALENDAR_TABLE[static_cast<size_t>(CalendarKey::CALENDAR_KEY_COUNT)]
                              [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        { DE_LOADING,               EN_LOADING               },
        { DE_LOADING_CANCELLED,     EN_LOADING_CANCELLED     },
        { DE_DATE_HEADER_TODAY_FMT, EN_DATE_HEADER_TODAY_FMT },
        { DE_DATE_HEADER_FMT,       EN_DATE_HEADER_FMT       },
        { DE_NO_EVENTS_TODAY,       EN_NO_EVENTS_TODAY       },
        { DE_NO_EVENTS,             EN_NO_EVENTS             },
        { DE_NO_UPCOMING,           EN_NO_UPCOMING           }
    };

    static_assert(sizeof(CALENDAR_TABLE) / sizeof(CALENDAR_TABLE[0])
                  == static_cast<size_t>(CalendarKey::CALENDAR_KEY_COUNT),
                  "CALENDAR_TABLE / CalendarKey out of sync");

    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten Kalendertext für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getCalendar(CalendarKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(CalendarKey::CALENDAR_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&CALENDAR_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&CALENDAR_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}
