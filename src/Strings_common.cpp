#include "Strings_common.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    // Wochentage kurz
    const char DE_WEEKDAY_SUN[] PROGMEM = "So";
    const char DE_WEEKDAY_MON[] PROGMEM = "Mo";
    const char DE_WEEKDAY_TUE[] PROGMEM = "Di";
    const char DE_WEEKDAY_WED[] PROGMEM = "Mi";
    const char DE_WEEKDAY_THU[] PROGMEM = "Do";
    const char DE_WEEKDAY_FRI[] PROGMEM = "Fr";
    const char DE_WEEKDAY_SAT[] PROGMEM = "Sa";

    // Wochentage lang
    const char DE_WEEKDAY_LONG_SUN[] PROGMEM = "Sonntag";
    const char DE_WEEKDAY_LONG_MON[] PROGMEM = "Montag";
    const char DE_WEEKDAY_LONG_TUE[] PROGMEM = "Dienstag";
    const char DE_WEEKDAY_LONG_WED[] PROGMEM = "Mittwoch";
    const char DE_WEEKDAY_LONG_THU[] PROGMEM = "Donnerstag";
    const char DE_WEEKDAY_LONG_FRI[] PROGMEM = "Freitag";
    const char DE_WEEKDAY_LONG_SAT[] PROGMEM = "Samstag";

    // Monate kurz
    const char DE_MONTH_JAN[] PROGMEM = "Jan";
    const char DE_MONTH_FEB[] PROGMEM = "Feb";
    const char DE_MONTH_MAR[] PROGMEM = "Mrz";
    const char DE_MONTH_APR[] PROGMEM = "Apr";
    const char DE_MONTH_MAY[] PROGMEM = "Mai";
    const char DE_MONTH_JUN[] PROGMEM = "Jun";
    const char DE_MONTH_JUL[] PROGMEM = "Jul";
    const char DE_MONTH_AUG[] PROGMEM = "Aug";
    const char DE_MONTH_SEP[] PROGMEM = "Sep";
    const char DE_MONTH_OCT[] PROGMEM = "Okt";
    const char DE_MONTH_NOV[] PROGMEM = "Nov";
    const char DE_MONTH_DEC[] PROGMEM = "Dez";

    // Monate lang
    const char DE_MONTH_LONG_JAN[] PROGMEM = "Januar";
    const char DE_MONTH_LONG_FEB[] PROGMEM = "Februar";
    const char DE_MONTH_LONG_MAR[] PROGMEM = "März";
    const char DE_MONTH_LONG_APR[] PROGMEM = "April";
    const char DE_MONTH_LONG_MAY[] PROGMEM = "Mai";
    const char DE_MONTH_LONG_JUN[] PROGMEM = "Juni";
    const char DE_MONTH_LONG_JUL[] PROGMEM = "Juli";
    const char DE_MONTH_LONG_AUG[] PROGMEM = "August";
    const char DE_MONTH_LONG_SEP[] PROGMEM = "September";
    const char DE_MONTH_LONG_OCT[] PROGMEM = "Oktober";
    const char DE_MONTH_LONG_NOV[] PROGMEM = "November";
    const char DE_MONTH_LONG_DEC[] PROGMEM = "Dezember";

    // Datum/Zeit-Format-Templates
    const char DE_DT_FORMAT_FULL[]      PROGMEM = "{0}, {2:%d}. {1} {4:%d} {5:%02d}:{6:%02d}:{7:%02d}";
    const char DE_DT_FORMAT_SHORT[]     PROGMEM = "{0}, {2:%02d}.{3:%02d}.{4:%d} {5:%02d}:{6:%02d}";
    const char DE_DT_FORMAT_DATE[]      PROGMEM = "{2:%d}. {1} {4:%d}";
    const char DE_DT_FORMAT_TIME[]      PROGMEM = "{5:%02d}:{6:%02d}:{7:%02d}";
    const char DE_DT_FORMAT_ISO[]       PROGMEM = "{4:%04d}-{3:%02d}-{2:%02d} {5:%02d}:{6:%02d}:{7:%02d}";
    const char DE_DT_FORMAT_DEFAULT[]   PROGMEM = "{2:%02d}.{3:%02d}.{4:%04d} {5:%02d}:{6:%02d}:{7:%02d}";
    const char DE_DT_PLACEHOLDER_DATE[] PROGMEM = "--. ---. ----";

    // Fehler
    const char DE_ERR_RTC[] PROGMEM = "-- Uhr-Fehler --";

    // Generische Knöpfe
    const char DE_BTN_CONTINUE[] PROGMEM = "weiter";
    const char DE_BTN_CANCEL[]   PROGMEM = "abbruch";
    const char DE_BTN_STOP[]     PROGMEM = "stopp";
    const char DE_BTN_YES[]      PROGMEM = "Ja";
    const char DE_BTN_NO[]       PROGMEM = "Nein";
    const char DE_BTN_OK[]       PROGMEM = "OK";

    // Allgemeine Datums-Wörter
    const char DE_TODAY[]        PROGMEM = "Heute";

    // ===================================================================
    // PROGMEM-Literale (Englisch, British)
    // ===================================================================

    // Wochentage kurz
    const char EN_WEEKDAY_SUN[] PROGMEM = "Sun";
    const char EN_WEEKDAY_MON[] PROGMEM = "Mon";
    const char EN_WEEKDAY_TUE[] PROGMEM = "Tue";
    const char EN_WEEKDAY_WED[] PROGMEM = "Wed";
    const char EN_WEEKDAY_THU[] PROGMEM = "Thu";
    const char EN_WEEKDAY_FRI[] PROGMEM = "Fri";
    const char EN_WEEKDAY_SAT[] PROGMEM = "Sat";

    // Wochentage lang
    const char EN_WEEKDAY_LONG_SUN[] PROGMEM = "Sunday";
    const char EN_WEEKDAY_LONG_MON[] PROGMEM = "Monday";
    const char EN_WEEKDAY_LONG_TUE[] PROGMEM = "Tuesday";
    const char EN_WEEKDAY_LONG_WED[] PROGMEM = "Wednesday";
    const char EN_WEEKDAY_LONG_THU[] PROGMEM = "Thursday";
    const char EN_WEEKDAY_LONG_FRI[] PROGMEM = "Friday";
    const char EN_WEEKDAY_LONG_SAT[] PROGMEM = "Saturday";

    // Monate kurz
    const char EN_MONTH_JAN[] PROGMEM = "Jan";
    const char EN_MONTH_FEB[] PROGMEM = "Feb";
    const char EN_MONTH_MAR[] PROGMEM = "Mar";
    const char EN_MONTH_APR[] PROGMEM = "Apr";
    const char EN_MONTH_MAY[] PROGMEM = "May";
    const char EN_MONTH_JUN[] PROGMEM = "Jun";
    const char EN_MONTH_JUL[] PROGMEM = "Jul";
    const char EN_MONTH_AUG[] PROGMEM = "Aug";
    const char EN_MONTH_SEP[] PROGMEM = "Sep";
    const char EN_MONTH_OCT[] PROGMEM = "Oct";
    const char EN_MONTH_NOV[] PROGMEM = "Nov";
    const char EN_MONTH_DEC[] PROGMEM = "Dec";

    // Monate lang
    const char EN_MONTH_LONG_JAN[] PROGMEM = "January";
    const char EN_MONTH_LONG_FEB[] PROGMEM = "February";
    const char EN_MONTH_LONG_MAR[] PROGMEM = "March";
    const char EN_MONTH_LONG_APR[] PROGMEM = "April";
    const char EN_MONTH_LONG_MAY[] PROGMEM = "May";
    const char EN_MONTH_LONG_JUN[] PROGMEM = "June";
    const char EN_MONTH_LONG_JUL[] PROGMEM = "July";
    const char EN_MONTH_LONG_AUG[] PROGMEM = "August";
    const char EN_MONTH_LONG_SEP[] PROGMEM = "September";
    const char EN_MONTH_LONG_OCT[] PROGMEM = "October";
    const char EN_MONTH_LONG_NOV[] PROGMEM = "November";
    const char EN_MONTH_LONG_DEC[] PROGMEM = "December";

    // Datum/Zeit-Format-Templates (British)
    const char EN_DT_FORMAT_FULL[]      PROGMEM = "{0}, {2:%d} {1} {4:%d} {5:%02d}:{6:%02d}:{7:%02d}";
    const char EN_DT_FORMAT_SHORT[]     PROGMEM = "{0}, {2:%02d}/{3:%02d}/{4:%d} {5:%02d}:{6:%02d}";
    const char EN_DT_FORMAT_DATE[]      PROGMEM = "{2:%d} {1} {4:%d}";
    const char EN_DT_FORMAT_TIME[]      PROGMEM = "{5:%02d}:{6:%02d}:{7:%02d}";
    const char EN_DT_FORMAT_ISO[]       PROGMEM = "{4:%04d}-{3:%02d}-{2:%02d} {5:%02d}:{6:%02d}:{7:%02d}";
    const char EN_DT_FORMAT_DEFAULT[]   PROGMEM = "{2:%02d}/{3:%02d}/{4:%04d} {5:%02d}:{6:%02d}:{7:%02d}";
    const char EN_DT_PLACEHOLDER_DATE[] PROGMEM = "-- --- ----";

    // Fehler
    const char EN_ERR_RTC[] PROGMEM = "-- RTC error --";

    // Generische Knöpfe
    const char EN_BTN_CONTINUE[] PROGMEM = "continue";
    const char EN_BTN_CANCEL[]   PROGMEM = "cancel";
    const char EN_BTN_STOP[]     PROGMEM = "stop";
    const char EN_BTN_YES[]      PROGMEM = "Yes";
    const char EN_BTN_NO[]       PROGMEM = "No";
    const char EN_BTN_OK[]       PROGMEM = "OK";

    // Allgemeine Datums-Wörter
    const char EN_TODAY[]        PROGMEM = "Today";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const COMMON_TABLE[static_cast<size_t>(CommonKey::COMMON_KEY_COUNT)]
                            [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        // Wochentage kurz
        { DE_WEEKDAY_SUN, EN_WEEKDAY_SUN },
        { DE_WEEKDAY_MON, EN_WEEKDAY_MON },
        { DE_WEEKDAY_TUE, EN_WEEKDAY_TUE },
        { DE_WEEKDAY_WED, EN_WEEKDAY_WED },
        { DE_WEEKDAY_THU, EN_WEEKDAY_THU },
        { DE_WEEKDAY_FRI, EN_WEEKDAY_FRI },
        { DE_WEEKDAY_SAT, EN_WEEKDAY_SAT },

        // Wochentage lang
        { DE_WEEKDAY_LONG_SUN, EN_WEEKDAY_LONG_SUN },
        { DE_WEEKDAY_LONG_MON, EN_WEEKDAY_LONG_MON },
        { DE_WEEKDAY_LONG_TUE, EN_WEEKDAY_LONG_TUE },
        { DE_WEEKDAY_LONG_WED, EN_WEEKDAY_LONG_WED },
        { DE_WEEKDAY_LONG_THU, EN_WEEKDAY_LONG_THU },
        { DE_WEEKDAY_LONG_FRI, EN_WEEKDAY_LONG_FRI },
        { DE_WEEKDAY_LONG_SAT, EN_WEEKDAY_LONG_SAT },

        // Monate kurz
        { DE_MONTH_JAN, EN_MONTH_JAN },
        { DE_MONTH_FEB, EN_MONTH_FEB },
        { DE_MONTH_MAR, EN_MONTH_MAR },
        { DE_MONTH_APR, EN_MONTH_APR },
        { DE_MONTH_MAY, EN_MONTH_MAY },
        { DE_MONTH_JUN, EN_MONTH_JUN },
        { DE_MONTH_JUL, EN_MONTH_JUL },
        { DE_MONTH_AUG, EN_MONTH_AUG },
        { DE_MONTH_SEP, EN_MONTH_SEP },
        { DE_MONTH_OCT, EN_MONTH_OCT },
        { DE_MONTH_NOV, EN_MONTH_NOV },
        { DE_MONTH_DEC, EN_MONTH_DEC },

        // Monate lang
        { DE_MONTH_LONG_JAN, EN_MONTH_LONG_JAN },
        { DE_MONTH_LONG_FEB, EN_MONTH_LONG_FEB },
        { DE_MONTH_LONG_MAR, EN_MONTH_LONG_MAR },
        { DE_MONTH_LONG_APR, EN_MONTH_LONG_APR },
        { DE_MONTH_LONG_MAY, EN_MONTH_LONG_MAY },
        { DE_MONTH_LONG_JUN, EN_MONTH_LONG_JUN },
        { DE_MONTH_LONG_JUL, EN_MONTH_LONG_JUL },
        { DE_MONTH_LONG_AUG, EN_MONTH_LONG_AUG },
        { DE_MONTH_LONG_SEP, EN_MONTH_LONG_SEP },
        { DE_MONTH_LONG_OCT, EN_MONTH_LONG_OCT },
        { DE_MONTH_LONG_NOV, EN_MONTH_LONG_NOV },
        { DE_MONTH_LONG_DEC, EN_MONTH_LONG_DEC },

        // Datum/Zeit-Format-Templates
        { DE_DT_FORMAT_FULL,      EN_DT_FORMAT_FULL      },
        { DE_DT_FORMAT_SHORT,     EN_DT_FORMAT_SHORT     },
        { DE_DT_FORMAT_DATE,      EN_DT_FORMAT_DATE      },
        { DE_DT_FORMAT_TIME,      EN_DT_FORMAT_TIME      },
        { DE_DT_FORMAT_ISO,       EN_DT_FORMAT_ISO       },
        { DE_DT_FORMAT_DEFAULT,   EN_DT_FORMAT_DEFAULT   },
        { DE_DT_PLACEHOLDER_DATE, EN_DT_PLACEHOLDER_DATE },

        // Fehler
        { DE_ERR_RTC, EN_ERR_RTC },

        // Generische Knöpfe
        { DE_BTN_CONTINUE, EN_BTN_CONTINUE },
        { DE_BTN_CANCEL,   EN_BTN_CANCEL   },
        { DE_BTN_STOP,     EN_BTN_STOP     },
        { DE_BTN_YES,      EN_BTN_YES      },
        { DE_BTN_NO,       EN_BTN_NO       },
        { DE_BTN_OK,       EN_BTN_OK       },

        // Allgemeine Datums-Wörter
        { DE_TODAY, EN_TODAY }
    };

    static_assert(sizeof(COMMON_TABLE) / sizeof(COMMON_TABLE[0])
                  == static_cast<size_t>(CommonKey::COMMON_KEY_COUNT),
                  "COMMON_TABLE / CommonKey out of sync");

    // Fallback für den unwahrscheinlichen Fall eines fehlenden Eintrags
    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten allgemeinen Text für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getCommon(CommonKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(CommonKey::COMMON_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&COMMON_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&COMMON_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}
