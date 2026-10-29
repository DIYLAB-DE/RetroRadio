#include "Strings_breather.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    // Auswahl-Bildschirm
    const char DE_HEADER_TITLE[]    PROGMEM = "AUSZEIT";
    const char DE_HEADER_SUBTITLE[] PROGMEM = "Was tut Dir gerade gut?";

    // Button-Beschriftungen
    const char DE_BTN_BACK[]   PROGMEM = "zurück";
    const char DE_BTN_PAUSE[]  PROGMEM = "pause";
    const char DE_BTN_RESUME[] PROGMEM = "weiter";
    const char DE_BTN_DONE[]   PROGMEM = "fertig";

    // Statische Labels
    const char DE_REMAINING_LABEL[] PROGMEM = "Verbleibend:";
    const char DE_EYE_CLOSED[]      PROGMEM = "Augen zu";
    const char DE_PHASE_INHALE[]    PROGMEM = "Einatmen";
    const char DE_PHASE_HOLD[]      PROGMEM = "Halten";
    const char DE_PHASE_EXHALE[]    PROGMEM = "Ausatmen";
    const char DE_EYE_FAR[]         PROGMEM = "fern";
    const char DE_EYE_NEAR[]        PROGMEM = "nah";
    const char DE_AXIS_HEAD[]       PROGMEM = "Kopf";
    const char DE_AXIS_TWIST[]      PROGMEM = "Drehung";

    // Done-Bildschirm
    const char DE_DONE_TITLE[] PROGMEM = "Geschafft.";
    const char DE_DONE_HINT[]  PROGMEM = "So geht es gut weiter.";

    // Format-Strings
    const char DE_DURATION_MIN_INT_FMT[] PROGMEM = "{0:%u} Min.";
    const char DE_DURATION_MIN_STR_FMT[] PROGMEM = "{0} Min.";
    const char DE_EXERCISE_X_OF_Y_FMT[]  PROGMEM = "Übung {0:%u} von {1:%u}";
    const char DE_DONE_DURATION_FMT[]    PROGMEM = "{0} genommen";

    // ===================================================================
    // PROGMEM-Literale (Englisch, British)
    // ===================================================================

    // Auswahl-Bildschirm
    const char EN_HEADER_TITLE[]    PROGMEM = "TIME OUT";
    const char EN_HEADER_SUBTITLE[] PROGMEM = "What do you need right now?";

    // Button-Beschriftungen
    const char EN_BTN_BACK[]   PROGMEM = "back";
    const char EN_BTN_PAUSE[]  PROGMEM = "pause";
    const char EN_BTN_RESUME[] PROGMEM = "resume";
    const char EN_BTN_DONE[]   PROGMEM = "done";

    // Statische Labels
    const char EN_REMAINING_LABEL[] PROGMEM = "Remaining:";
    const char EN_EYE_CLOSED[]      PROGMEM = "close eyes";
    const char EN_PHASE_INHALE[]    PROGMEM = "Inhale";
    const char EN_PHASE_HOLD[]      PROGMEM = "Hold";
    const char EN_PHASE_EXHALE[]    PROGMEM = "Exhale";
    const char EN_EYE_FAR[]         PROGMEM = "far";
    const char EN_EYE_NEAR[]        PROGMEM = "near";
    const char EN_AXIS_HEAD[]       PROGMEM = "Head";
    const char EN_AXIS_TWIST[]      PROGMEM = "Twist";

    // Done-Bildschirm
    const char EN_DONE_TITLE[] PROGMEM = "Well done.";
    const char EN_DONE_HINT[]  PROGMEM = "Carry on from there.";

    // Format-Strings
    const char EN_DURATION_MIN_INT_FMT[] PROGMEM = "{0:%u} min";
    const char EN_DURATION_MIN_STR_FMT[] PROGMEM = "{0} min";
    const char EN_EXERCISE_X_OF_Y_FMT[]  PROGMEM = "Exercise {0:%u} of {1:%u}";
    const char EN_DONE_DURATION_FMT[]    PROGMEM = "Took {0}";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const BREATHER_TABLE[static_cast<size_t>(BreatherKey::BREATHER_KEY_COUNT)]
                              [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        // Auswahl-Bildschirm
        { DE_HEADER_TITLE,    EN_HEADER_TITLE    },
        { DE_HEADER_SUBTITLE, EN_HEADER_SUBTITLE },

        // Button-Beschriftungen
        { DE_BTN_BACK,   EN_BTN_BACK   },
        { DE_BTN_PAUSE,  EN_BTN_PAUSE  },
        { DE_BTN_RESUME, EN_BTN_RESUME },
        { DE_BTN_DONE,   EN_BTN_DONE   },

        // Statische Labels
        { DE_REMAINING_LABEL, EN_REMAINING_LABEL },
        { DE_EYE_CLOSED,      EN_EYE_CLOSED      },
        { DE_PHASE_INHALE,    EN_PHASE_INHALE    },
        { DE_PHASE_HOLD,      EN_PHASE_HOLD      },
        { DE_PHASE_EXHALE,    EN_PHASE_EXHALE    },
        { DE_EYE_FAR,         EN_EYE_FAR         },
        { DE_EYE_NEAR,        EN_EYE_NEAR        },
        { DE_AXIS_HEAD,       EN_AXIS_HEAD       },
        { DE_AXIS_TWIST,      EN_AXIS_TWIST      },

        // Done-Bildschirm
        { DE_DONE_TITLE, EN_DONE_TITLE },
        { DE_DONE_HINT,  EN_DONE_HINT  },

        // Format-Strings
        { DE_DURATION_MIN_INT_FMT, EN_DURATION_MIN_INT_FMT },
        { DE_DURATION_MIN_STR_FMT, EN_DURATION_MIN_STR_FMT },
        { DE_EXERCISE_X_OF_Y_FMT,  EN_EXERCISE_X_OF_Y_FMT  },
        { DE_DONE_DURATION_FMT,    EN_DONE_DURATION_FMT    }
    };

    static_assert(sizeof(BREATHER_TABLE) / sizeof(BREATHER_TABLE[0])
                  == static_cast<size_t>(BreatherKey::BREATHER_KEY_COUNT),
                  "BREATHER_TABLE / BreatherKey out of sync");

    // Fallback für den unwahrscheinlichen Fall eines fehlenden Eintrags
    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten Breather-Text für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getBreather(BreatherKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(BreatherKey::BREATHER_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&BREATHER_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&BREATHER_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}
