#include "Strings_contacts.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    // Ladescreen
    const char DE_LOADING_TITLE[]     PROGMEM = "- lade kontakte -";
    const char DE_LOADING_CANCEL[]    PROGMEM = "- abbruch -";

    // ProgressBox
    const char DE_PROGRESS_TITLE[]    PROGMEM = "Kontakte laden";
    const char DE_PROGRESS_MESSAGE[]  PROGMEM = "Aktualisierung läuft...";

    // Action-Buttons
    const char DE_BTN_SEARCH[]        PROGMEM = "suchen";
    const char DE_BTN_RESET[]         PROGMEM = "reset";
    const char DE_BTN_REFRESH[]       PROGMEM = "aktualisieren";

    // Platzhalter
    const char DE_NO_RESULTS[]        PROGMEM = "Keine Treffer";
    const char DE_NO_CONTACTS[]       PROGMEM = "Keine Kontakte";

    // Detail-Labels
    const char DE_LABEL_COMPANY[]     PROGMEM = "Firma:";
    const char DE_LABEL_PHONE_HOME[]  PROGMEM = "Privat:";
    const char DE_LABEL_PHONE_MOBILE[] PROGMEM = "Mobil:";
    const char DE_LABEL_PHONE_WORK[]  PROGMEM = "Dienst:";
    const char DE_LABEL_EMAIL[]       PROGMEM = "E-Mail:";
    const char DE_LABEL_ADDRESS[]     PROGMEM = "Adresse:";
    const char DE_LABEL_BIRTHDAY[]    PROGMEM = "Geburtstag:";

    // Statistik (Format)
    const char DE_STATS_FILTERED[]    PROGMEM = "{0:%d} von {1:%d} Kontakten";
    const char DE_STATS_TOTAL[]       PROGMEM = "{0:%d} Kontakte";

    // Geburtstags-Datum (Format)
    const char DE_DATE_FULL_FMT[]     PROGMEM = "{0:%02d}.{1:%02d}.{2:%04d}";
    const char DE_DATE_SHORT_FMT[]    PROGMEM = "{0:%02d}.{1:%02d}.";

    // Suchfunktion
    const char DE_SEARCH_PROMPT[]     PROGMEM = "Nachname suchen (* ? Wildcards):";

    // Reset-Dialog
    const char DE_RESET_TITLE[]       PROGMEM = "Filter zurücksetzen";
    const char DE_RESET_MESSAGE[]     PROGMEM = "Suchfilter zurücksetzen und alle Kontakte anzeigen?";

    // ===================================================================
    // PROGMEM-Literale (Englisch, British)
    // ===================================================================

    // Ladescreen
    const char EN_LOADING_TITLE[]     PROGMEM = "- loading contacts -";
    const char EN_LOADING_CANCEL[]    PROGMEM = "- cancelled -";

    // ProgressBox
    const char EN_PROGRESS_TITLE[]    PROGMEM = "Loading contacts";
    const char EN_PROGRESS_MESSAGE[]  PROGMEM = "Update in progress...";

    // Action-Buttons
    const char EN_BTN_SEARCH[]        PROGMEM = "search";
    const char EN_BTN_RESET[]         PROGMEM = "reset";
    const char EN_BTN_REFRESH[]       PROGMEM = "refresh";

    // Platzhalter
    const char EN_NO_RESULTS[]        PROGMEM = "No matches";
    const char EN_NO_CONTACTS[]       PROGMEM = "No contacts";

    // Detail-Labels
    const char EN_LABEL_COMPANY[]     PROGMEM = "Company:";
    const char EN_LABEL_PHONE_HOME[]  PROGMEM = "Home:";
    const char EN_LABEL_PHONE_MOBILE[] PROGMEM = "Mobile:";
    const char EN_LABEL_PHONE_WORK[]  PROGMEM = "Work:";
    const char EN_LABEL_EMAIL[]       PROGMEM = "Email:";
    const char EN_LABEL_ADDRESS[]     PROGMEM = "Address:";
    const char EN_LABEL_BIRTHDAY[]    PROGMEM = "Birthday:";

    // Statistik (Format)
    const char EN_STATS_FILTERED[]    PROGMEM = "{0:%d} of {1:%d} contacts";
    const char EN_STATS_TOTAL[]       PROGMEM = "{0:%d} contacts";

    // Geburtstags-Datum (Format, British: DD/MM/YYYY)
    const char EN_DATE_FULL_FMT[]     PROGMEM = "{0:%02d}/{1:%02d}/{2:%04d}";
    const char EN_DATE_SHORT_FMT[]    PROGMEM = "{0:%02d}/{1:%02d}";

    // Suchfunktion
    const char EN_SEARCH_PROMPT[]     PROGMEM = "Search surname (* ? wildcards):";

    // Reset-Dialog
    const char EN_RESET_TITLE[]       PROGMEM = "Reset filter";
    const char EN_RESET_MESSAGE[]     PROGMEM = "Reset search filter and show all contacts?";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const CONTACTS_TABLE[static_cast<size_t>(ContactsKey::CONTACTS_KEY_COUNT)]
                              [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        // Ladescreen
        { DE_LOADING_TITLE,    EN_LOADING_TITLE    },
        { DE_LOADING_CANCEL,   EN_LOADING_CANCEL   },

        // ProgressBox
        { DE_PROGRESS_TITLE,   EN_PROGRESS_TITLE   },
        { DE_PROGRESS_MESSAGE, EN_PROGRESS_MESSAGE },

        // Action-Buttons
        { DE_BTN_SEARCH,       EN_BTN_SEARCH       },
        { DE_BTN_RESET,        EN_BTN_RESET        },
        { DE_BTN_REFRESH,      EN_BTN_REFRESH      },

        // Platzhalter
        { DE_NO_RESULTS,       EN_NO_RESULTS       },
        { DE_NO_CONTACTS,      EN_NO_CONTACTS      },

        // Detail-Labels
        { DE_LABEL_COMPANY,      EN_LABEL_COMPANY      },
        { DE_LABEL_PHONE_HOME,   EN_LABEL_PHONE_HOME   },
        { DE_LABEL_PHONE_MOBILE, EN_LABEL_PHONE_MOBILE },
        { DE_LABEL_PHONE_WORK,   EN_LABEL_PHONE_WORK   },
        { DE_LABEL_EMAIL,        EN_LABEL_EMAIL        },
        { DE_LABEL_ADDRESS,      EN_LABEL_ADDRESS      },
        { DE_LABEL_BIRTHDAY,     EN_LABEL_BIRTHDAY     },

        // Statistik
        { DE_STATS_FILTERED,   EN_STATS_FILTERED   },
        { DE_STATS_TOTAL,      EN_STATS_TOTAL      },

        // Geburtstags-Datum
        { DE_DATE_FULL_FMT,    EN_DATE_FULL_FMT    },
        { DE_DATE_SHORT_FMT,   EN_DATE_SHORT_FMT   },

        // Suchfunktion
        { DE_SEARCH_PROMPT,    EN_SEARCH_PROMPT    },

        // Reset-Dialog
        { DE_RESET_TITLE,      EN_RESET_TITLE      },
        { DE_RESET_MESSAGE,    EN_RESET_MESSAGE    }
    };

    static_assert(sizeof(CONTACTS_TABLE) / sizeof(CONTACTS_TABLE[0])
                  == static_cast<size_t>(ContactsKey::CONTACTS_KEY_COUNT),
                  "CONTACTS_TABLE / ContactsKey out of sync");

    // Fallback für den unwahrscheinlichen Fall eines fehlenden Eintrags
    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten Kontakttext für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getContacts(ContactsKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(ContactsKey::CONTACTS_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&CONTACTS_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&CONTACTS_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}
