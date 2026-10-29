#include "Strings_todo.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    // Ladescreen
    const char DE_LOADING[]         PROGMEM = "- lade aufgaben -";
    const char DE_LOADING_CANCEL[]  PROGMEM = "- abbruch -";

    // Buttons
    const char DE_BUTTON_REFRESH[]  PROGMEM = "aktualisieren";

    // Stats-Panel
    const char DE_STATS_TITLE[]     PROGMEM = "Aufgaben";
    const char DE_STATS_TOTAL[]     PROGMEM = "Gesamt:";
    const char DE_STATS_OPEN[]      PROGMEM = "Offen:";
    const char DE_STATS_TODAY[]     PROGMEM = "Heute:";
    const char DE_STATS_OVERDUE[]   PROGMEM = "Überfällig:";

    // Toggle-Switch und Platzhalter
    const char DE_TOGGLE_LABEL[]    PROGMEM = "Erledigte";
    const char DE_EMPTY_LIST[]      PROGMEM = "Keine Aufgaben vorhanden";

    // Backend-Default
    const char DE_LIST_DEFAULT_NAME[] PROGMEM = "Aufgaben";

    // ProgressBox-Texte
    const char DE_PROGRESS_TITLE[]      PROGMEM = "Aktualisierung";
    const char DE_PROGRESS_MESSAGE[]    PROGMEM = "Aktualisierung läuft...";
    const char DE_PROGRESS_MANUAL_MSG[] PROGMEM = "Aktualisierung wird ausgeführt...";

    // MessageBox-Texte
    const char DE_DLG_TITLE_COMPLETE[] PROGMEM = "Aufgabe erledigt?";
    const char DE_DLG_TITLE_REOPEN[]   PROGMEM = "Aufgabe wieder öffnen?";
    const char DE_DLG_LOADING[]        PROGMEM = "Wird aktualisiert...";

    // ===================================================================
    // PROGMEM-Literale (Englisch, British)
    // ===================================================================

    const char EN_LOADING[]         PROGMEM = "- loading tasks -";
    const char EN_LOADING_CANCEL[]  PROGMEM = "- cancel -";

    const char EN_BUTTON_REFRESH[]  PROGMEM = "refresh";

    const char EN_STATS_TITLE[]     PROGMEM = "Tasks";
    const char EN_STATS_TOTAL[]     PROGMEM = "Total:";
    const char EN_STATS_OPEN[]      PROGMEM = "Open:";
    const char EN_STATS_TODAY[]     PROGMEM = "Today:";
    const char EN_STATS_OVERDUE[]   PROGMEM = "Overdue:";

    const char EN_TOGGLE_LABEL[]    PROGMEM = "Completed";
    const char EN_EMPTY_LIST[]      PROGMEM = "No tasks available";

    const char EN_LIST_DEFAULT_NAME[] PROGMEM = "Tasks";

    const char EN_PROGRESS_TITLE[]      PROGMEM = "Refresh";
    const char EN_PROGRESS_MESSAGE[]    PROGMEM = "Refresh in progress...";
    const char EN_PROGRESS_MANUAL_MSG[] PROGMEM = "Performing refresh...";

    const char EN_DLG_TITLE_COMPLETE[] PROGMEM = "Task completed?";
    const char EN_DLG_TITLE_REOPEN[]   PROGMEM = "Reopen task?";
    const char EN_DLG_LOADING[]        PROGMEM = "Updating...";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const TODO_TABLE[static_cast<size_t>(ToDoKey::TODO_KEY_COUNT)]
                          [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        // Ladescreen
        { DE_LOADING,        EN_LOADING        },
        { DE_LOADING_CANCEL, EN_LOADING_CANCEL },

        // Buttons
        { DE_BUTTON_REFRESH, EN_BUTTON_REFRESH },

        // Stats-Panel
        { DE_STATS_TITLE,    EN_STATS_TITLE    },
        { DE_STATS_TOTAL,    EN_STATS_TOTAL    },
        { DE_STATS_OPEN,     EN_STATS_OPEN     },
        { DE_STATS_TODAY,    EN_STATS_TODAY    },
        { DE_STATS_OVERDUE,  EN_STATS_OVERDUE  },

        // Toggle-Switch und Platzhalter
        { DE_TOGGLE_LABEL,   EN_TOGGLE_LABEL   },
        { DE_EMPTY_LIST,     EN_EMPTY_LIST     },

        // Backend-Default
        { DE_LIST_DEFAULT_NAME, EN_LIST_DEFAULT_NAME },

        // ProgressBox-Texte
        { DE_PROGRESS_TITLE,      EN_PROGRESS_TITLE      },
        { DE_PROGRESS_MESSAGE,    EN_PROGRESS_MESSAGE    },
        { DE_PROGRESS_MANUAL_MSG, EN_PROGRESS_MANUAL_MSG },

        // MessageBox-Texte
        { DE_DLG_TITLE_COMPLETE, EN_DLG_TITLE_COMPLETE },
        { DE_DLG_TITLE_REOPEN,   EN_DLG_TITLE_REOPEN   },
        { DE_DLG_LOADING,        EN_DLG_LOADING        }
    };

    static_assert(sizeof(TODO_TABLE) / sizeof(TODO_TABLE[0])
                  == static_cast<size_t>(ToDoKey::TODO_KEY_COUNT),
                  "TODO_TABLE / ToDoKey out of sync");

    // Fallback für den unwahrscheinlichen Fall eines fehlenden Eintrags
    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten Aufgabentext für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getToDo(ToDoKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(ToDoKey::TODO_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&TODO_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&TODO_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}
