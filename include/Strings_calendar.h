#ifndef STRINGS_CALENDAR_H
#define STRINGS_CALENDAR_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte des Calendar-Moduls
 *
 * Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_calendar.cpp` erweitern. Der `static_assert` dort verhindert eine Drift.
 *
 * Die Datums-Header-Templates erwarten:
 *   {0:%s} = Wochentag-Kurzform (z.B. "Mo", "Tue")
 *   {1:%s} = Monatsname (kurz)
 *   {2:%d} = Tag des Monats
 */
enum class CalendarKey : uint16_t
{
    // Ladescreen
    CALENDAR_LOADING,                ///< "- lade kalenderdaten -" / "- loading calendar -"
    CALENDAR_LOADING_CANCELLED,      ///< "- abbruch -" / "- cancelled -"

    // Termin-Liste
    CALENDAR_DATE_HEADER_TODAY_FMT,  ///< "Heute, {0}., {2:%d}. {1}" / "Today, {0}, {2:%d} {1}"
    CALENDAR_DATE_HEADER_FMT,        ///< "{0}., {2:%d}. {1}" / "{0}, {2:%d} {1}"
    CALENDAR_NO_EVENTS_TODAY,        ///< "Heute keine Ereignisse" / "No events today"
    CALENDAR_NO_EVENTS,              ///< "Keine Ereignisse" / "No events"
    CALENDAR_NO_UPCOMING,            ///< "Keine kommenden Termine" / "No upcoming events"

    CALENDAR_KEY_COUNT               ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_CALENDAR_H
