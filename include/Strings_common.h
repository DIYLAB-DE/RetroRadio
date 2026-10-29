#ifndef STRINGS_COMMON_H
#define STRINGS_COMMON_H

#include <stdint.h>

/**
 * @brief Schlüssel für modulweit wiederverwendete Texte
 *
 * Diese Strings tauchen in mehreren Modulen auf (Wochentage, Monate, generische Knöpfe, Datumsformate). Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_common.cpp` erweitern. Der `static_assert` dort verhindert eine Drift.
 */
enum class CommonKey : uint16_t
{
    // ------------------------------------------------------------------------
    // Wochentage (kurz)
    // ------------------------------------------------------------------------
    COMMON_WEEKDAY_SUN = 0, ///< "So" / "Sun"
    COMMON_WEEKDAY_MON,     ///< "Mo" / "Mon"
    COMMON_WEEKDAY_TUE,     ///< "Di" / "Tue"
    COMMON_WEEKDAY_WED,     ///< "Mi" / "Wed"
    COMMON_WEEKDAY_THU,     ///< "Do" / "Thu"
    COMMON_WEEKDAY_FRI,     ///< "Fr" / "Fri"
    COMMON_WEEKDAY_SAT,     ///< "Sa" / "Sat"

    // ------------------------------------------------------------------------
    // Wochentage (lang)
    // ------------------------------------------------------------------------
    COMMON_WEEKDAY_LONG_SUN, ///< "Sonntag" / "Sunday"
    COMMON_WEEKDAY_LONG_MON, ///< "Montag" / "Monday"
    COMMON_WEEKDAY_LONG_TUE, ///< "Dienstag" / "Tuesday"
    COMMON_WEEKDAY_LONG_WED, ///< "Mittwoch" / "Wednesday"
    COMMON_WEEKDAY_LONG_THU, ///< "Donnerstag" / "Thursday"
    COMMON_WEEKDAY_LONG_FRI, ///< "Freitag" / "Friday"
    COMMON_WEEKDAY_LONG_SAT, ///< "Samstag" / "Saturday"

    // ------------------------------------------------------------------------
    // Monate (kurz)
    // ------------------------------------------------------------------------
    COMMON_MONTH_JAN, ///< "Jan" / "Jan"
    COMMON_MONTH_FEB, ///< "Feb" / "Feb"
    COMMON_MONTH_MAR, ///< "Mrz" / "Mar"
    COMMON_MONTH_APR, ///< "Apr" / "Apr"
    COMMON_MONTH_MAY, ///< "Mai" / "May"
    COMMON_MONTH_JUN, ///< "Jun" / "Jun"
    COMMON_MONTH_JUL, ///< "Jul" / "Jul"
    COMMON_MONTH_AUG, ///< "Aug" / "Aug"
    COMMON_MONTH_SEP, ///< "Sep" / "Sep"
    COMMON_MONTH_OCT, ///< "Okt" / "Oct"
    COMMON_MONTH_NOV, ///< "Nov" / "Nov"
    COMMON_MONTH_DEC, ///< "Dez" / "Dec"

    // ------------------------------------------------------------------------
    // Monate (lang)
    // ------------------------------------------------------------------------
    COMMON_MONTH_LONG_JAN, ///< "Januar" / "January"
    COMMON_MONTH_LONG_FEB, ///< "Februar" / "February"
    COMMON_MONTH_LONG_MAR, ///< "März" / "March"
    COMMON_MONTH_LONG_APR, ///< "April" / "April"
    COMMON_MONTH_LONG_MAY, ///< "Mai" / "May"
    COMMON_MONTH_LONG_JUN, ///< "Juni" / "June"
    COMMON_MONTH_LONG_JUL, ///< "Juli" / "July"
    COMMON_MONTH_LONG_AUG, ///< "August" / "August"
    COMMON_MONTH_LONG_SEP, ///< "September" / "September"
    COMMON_MONTH_LONG_OCT, ///< "Oktober" / "October"
    COMMON_MONTH_LONG_NOV, ///< "November" / "November"
    COMMON_MONTH_LONG_DEC, ///< "Dezember" / "December"

    // ------------------------------------------------------------------------
    // Datums-/Zeit-Format-Templates für DS1307_RTC::getLocalizedDateTimeString()
    //
    // Platzhalter-Konvention:
    //   {0:%s} = Wochentag (kurz oder lang, je nach Format)
    //   {1:%s} = Monat (kurz oder lang, je nach Format)
    //   {2:%d} = Tag
    //   {3:%d} = Monat als Zahl
    //   {4:%d} = Jahr
    //   {5:%d} = Stunde
    //   {6:%d} = Minute
    //   {7:%d} = Sekunde
    // ------------------------------------------------------------------------
    COMMON_DT_FORMAT_FULL,        ///< "Montag, 15. Januar 2024 14:30:25" / "Monday, 15 January 2024 14:30:25"
    COMMON_DT_FORMAT_SHORT,       ///< "Mo, 15.01.2024 14:30" / "Mon, 15/01/2024 14:30"
    COMMON_DT_FORMAT_DATE,        ///< "15. Januar 2024" / "15 January 2024"
    COMMON_DT_FORMAT_TIME,        ///< "14:30:25" (sprachneutral)
    COMMON_DT_FORMAT_ISO,         ///< "2024-01-15 14:30:25" (sprachneutral)
    COMMON_DT_FORMAT_DEFAULT,     ///< "15.01.2024 14:30:25" / "15/01/2024 14:30:25"
    COMMON_DT_PLACEHOLDER_DATE,   ///< "--. ---. ----" / "-- --- ----" (Platzhalter bei ungültiger Zeit, passt zu COMMON_DT_FORMAT_DATE)

    // ------------------------------------------------------------------------
    // Allgemeine Fehler
    // ------------------------------------------------------------------------
    COMMON_ERR_RTC, ///< "-- Uhr-Fehler --" / "-- RTC error --"

    // ------------------------------------------------------------------------
    // Generische Knöpfe (Dialog-Buttons)
    // ------------------------------------------------------------------------
    COMMON_BTN_CONTINUE, ///< "weiter" / "continue"
    COMMON_BTN_CANCEL,   ///< "abbruch" / "cancel"
    COMMON_BTN_STOP,     ///< "stopp" / "stop"
    COMMON_BTN_YES,      ///< "Ja" / "Yes"
    COMMON_BTN_NO,       ///< "Nein" / "No"
    COMMON_BTN_OK,       ///< "OK" / "OK" (identisch in beiden Sprachen, Key existiert für Konsistenz und zukünftige Sprachen)

    // ------------------------------------------------------------------------
    // Allgemeine Datums-Wörter
    // ------------------------------------------------------------------------
    COMMON_TODAY,        ///< "Heute" / "Today" (Tages-Header in Listen)

    COMMON_KEY_COUNT ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_COMMON_H
