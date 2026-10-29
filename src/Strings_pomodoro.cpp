#include "Strings_pomodoro.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    // Modul-Titel und Modus-Button
    const char DE_TITLE_POMODORO[] PROGMEM = "POMODORO";
    const char DE_TITLE_TIMER[]    PROGMEM = "TIMER";

    // Button-Labels
    const char DE_BTN_START[]  PROGMEM = "START";
    const char DE_BTN_STOP[]   PROGMEM = "STOP";
    const char DE_BTN_PAUSE[]  PROGMEM = "PAUSE";
    const char DE_BTN_RESUME[] PROGMEM = "WEITER";

    // Panel-Labels
    const char DE_LABEL_SESSION_FMT[] PROGMEM = "Session {0:%d}/{1:%d}";
    const char DE_LABEL_DURATION[]    PROGMEM = "Timer-Dauer";

    // Phasen-Namen
    const char DE_PHASE_FOCUS[]       PROGMEM = "FOKUS";
    const char DE_PHASE_SHORT_BREAK[] PROGMEM = "PAUSE";
    const char DE_PHASE_LONG_BREAK[]  PROGMEM = "LANGE PAUSE";
    const char DE_PHASE_TIMER[]       PROGMEM = "TIMER";
    const char DE_PHASE_PAUSED[]      PROGMEM = "PAUSIERT";
    const char DE_PHASE_READY[]       PROGMEM = "BEREIT";

    // Format-Strings
    const char DE_DURATION_FMT[]   PROGMEM = "{0:%d} Min";
    const char DE_STATS_SINGULAR[] PROGMEM = "Heute: {0:%d} Pomodoro";
    const char DE_STATS_PLURAL[]   PROGMEM = "Heute: {0:%d} Pomodoros";

    // ===================================================================
    // PROGMEM-Literale (Englisch, British)
    // ===================================================================

    // Modul-Titel und Modus-Button
    const char EN_TITLE_POMODORO[] PROGMEM = "POMODORO";
    const char EN_TITLE_TIMER[]    PROGMEM = "TIMER";

    // Button-Labels
    const char EN_BTN_START[]  PROGMEM = "START";
    const char EN_BTN_STOP[]   PROGMEM = "STOP";
    const char EN_BTN_PAUSE[]  PROGMEM = "PAUSE";
    const char EN_BTN_RESUME[] PROGMEM = "RESUME";

    // Panel-Labels
    const char EN_LABEL_SESSION_FMT[] PROGMEM = "Session {0:%d}/{1:%d}";
    const char EN_LABEL_DURATION[]    PROGMEM = "Timer duration";

    // Phasen-Namen
    const char EN_PHASE_FOCUS[]       PROGMEM = "FOCUS";
    const char EN_PHASE_SHORT_BREAK[] PROGMEM = "BREAK";
    const char EN_PHASE_LONG_BREAK[]  PROGMEM = "LONG BREAK";
    const char EN_PHASE_TIMER[]       PROGMEM = "TIMER";
    const char EN_PHASE_PAUSED[]      PROGMEM = "PAUSED";
    const char EN_PHASE_READY[]       PROGMEM = "READY";

    // Format-Strings
    const char EN_DURATION_FMT[]   PROGMEM = "{0:%d} min";
    const char EN_STATS_SINGULAR[] PROGMEM = "Today: {0:%d} pomodoro";
    const char EN_STATS_PLURAL[]   PROGMEM = "Today: {0:%d} pomodoros";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const POMODORO_TABLE[static_cast<size_t>(PomodoroKey::POMODORO_KEY_COUNT)]
                              [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        // Modul-Titel
        { DE_TITLE_POMODORO, EN_TITLE_POMODORO },
        { DE_TITLE_TIMER,    EN_TITLE_TIMER    },

        // Button-Labels
        { DE_BTN_START,  EN_BTN_START  },
        { DE_BTN_STOP,   EN_BTN_STOP   },
        { DE_BTN_PAUSE,  EN_BTN_PAUSE  },
        { DE_BTN_RESUME, EN_BTN_RESUME },

        // Panel-Labels
        { DE_LABEL_SESSION_FMT, EN_LABEL_SESSION_FMT },
        { DE_LABEL_DURATION,    EN_LABEL_DURATION    },

        // Phasen-Namen
        { DE_PHASE_FOCUS,       EN_PHASE_FOCUS       },
        { DE_PHASE_SHORT_BREAK, EN_PHASE_SHORT_BREAK },
        { DE_PHASE_LONG_BREAK,  EN_PHASE_LONG_BREAK  },
        { DE_PHASE_TIMER,       EN_PHASE_TIMER       },
        { DE_PHASE_PAUSED,      EN_PHASE_PAUSED      },
        { DE_PHASE_READY,       EN_PHASE_READY       },

        // Format-Strings
        { DE_DURATION_FMT,   EN_DURATION_FMT   },
        { DE_STATS_SINGULAR, EN_STATS_SINGULAR },
        { DE_STATS_PLURAL,   EN_STATS_PLURAL   }
    };

    static_assert(sizeof(POMODORO_TABLE) / sizeof(POMODORO_TABLE[0])
                  == static_cast<size_t>(PomodoroKey::POMODORO_KEY_COUNT),
                  "POMODORO_TABLE / PomodoroKey out of sync");

    // Fallback für den unwahrscheinlichen Fall eines fehlenden Eintrags
    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten Pomodoro-Text für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getPomodoro(PomodoroKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(PomodoroKey::POMODORO_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&POMODORO_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&POMODORO_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}
