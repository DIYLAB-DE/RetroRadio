#include "Strings_off.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    const char DE_HINT[]         PROGMEM = "RetroRadio ist dein selbstgebautes 7-Zoll-Radio mit Webradio, SD-Player, Wetter, Kalender, Aufgaben und kleinen Helfern für den Alltag.";
    const char DE_LINK_LABEL[]   PROGMEM = "Projektseite:";
    const char DE_BTN_BACK[]     PROGMEM = "zurück";
    const char DE_BTN_SHUTDOWN[] PROGMEM = "ausschalten";

    // ===================================================================
    // PROGMEM-Literale (Englisch, British)
    // ===================================================================

    const char EN_HINT[]         PROGMEM = "RetroRadio is your self-built 7-inch radio with web radio, an SD player, weather, calendar, tasks, and handy everyday tools.";
    const char EN_LINK_LABEL[]   PROGMEM = "Project page:";
    const char EN_BTN_BACK[]     PROGMEM = "back";
    const char EN_BTN_SHUTDOWN[] PROGMEM = "switch off";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const OFF_TABLE[static_cast<size_t>(OffKey::OFF_KEY_COUNT)]
                         [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        { DE_HINT,         EN_HINT         },
        { DE_LINK_LABEL,   EN_LINK_LABEL   },
        { DE_BTN_BACK,     EN_BTN_BACK     },
        { DE_BTN_SHUTDOWN, EN_BTN_SHUTDOWN }
    };

    static_assert(sizeof(OFF_TABLE) / sizeof(OFF_TABLE[0])
                  == static_cast<size_t>(OffKey::OFF_KEY_COUNT),
                  "OFF_TABLE / OffKey out of sync");

    // Fallback für den unwahrscheinlichen Fall eines fehlenden Eintrags
    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten Ausschalttext für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getOff(OffKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(OffKey::OFF_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&OFF_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&OFF_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}
