#ifndef STRINGS_DAYVIEW_H
#define STRINGS_DAYVIEW_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte des DayView-Moduls
 *
 * Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_dayview.cpp` erweitern. Der `static_assert` dort verhindert eine Drift zwischen Enum und Tabelle.
 *
 * Datums-Platzhalter-Konvention (Format-Templates):
 *   {0:%s} = Wochentag (Kurzform)
 *   {1:%s} = Monat (Kurzform)
 *   {2:%d} = Tag
 *   {4:%d} = Jahr
 *   Indizes ohne Vorkommen werden vom Parser nicht aus va_args konsumiert.
 */
enum class DayViewKey : uint16_t
{
    // ===================================================================
    // Sektions-Überschriften
    // ===================================================================
    DAYVIEW_LABEL_EVENTS = 0,       ///< "TERMINE" / "EVENTS"
    DAYVIEW_LABEL_TASKS,            ///< "AUFGABEN" / "TASKS"
    DAYVIEW_LABEL_BIRTHDAYS,        ///< "GEBURTSTAGE" / "BIRTHDAYS"
    DAYVIEW_LABEL_EVENTS_DEMO,      ///< "TERMINE (DEMO)" / "EVENTS (DEMO)"
    DAYVIEW_LABEL_TASKS_DEMO,       ///< "AUFGABEN (DEMO)" / "TASKS (DEMO)"
    DAYVIEW_LABEL_BIRTHDAYS_DEMO,   ///< "GEBURTSTAGE (DEMO)" / "BIRTHDAYS (DEMO)"

    // ===================================================================
    // Platzhalter-Texte
    // ===================================================================
    DAYVIEW_NO_EVENTS,              ///< "keine Termine" / "no events"
    DAYVIEW_NO_TASKS,               ///< "keine offenen Aufgaben" / "no open tasks"
    DAYVIEW_NO_BIRTHDAYS,           ///< "heute keine Geburtstage" / "no birthdays today"
    DAYVIEW_LOADING_EVENTS,         ///< "Lade Termine ..." / "Loading events ..."
    DAYVIEW_LOADING_TASKS,          ///< "Lade Aufgaben ..." / "Loading tasks ..."

    // ===================================================================
    // Wetter-Detailzeilen (Format-Strings)
    // ===================================================================
    DAYVIEW_WEATHER_FEELS_LIKE,     ///< "Gefühlt: {0:%d}°C" / "Feels like: {0:%d}°C"
    DAYVIEW_WEATHER_HUMIDITY,       ///< "Feuchtigkeit: {0:%d} %" / "Humidity: {0:%d}%"
    DAYVIEW_WEATHER_WIND,           ///< "Wind: {0:%d} km/h" / "Wind: {0:%d} km/h"

    // ===================================================================
    // Datums-Templates (Header und Tageslabels in der Terminliste)
    // ===================================================================
    DAYVIEW_DATE_HEADER_FMT,        ///< "{0}, {2:%d}. {1} {4:%d}" -> "Mo, 15. Jan 2024"
    DAYVIEW_DAYLABEL_FMT,           ///< "{0}, {2:%d}. {1}" -> "Mo, 24. Feb"

    // ===================================================================
    // Badges (Format-Strings)
    // ===================================================================
    DAYVIEW_BADGE_MORE,             ///< "+{0:%d} weitere" / "+{0:%d} more"

    // ===================================================================
    // News-Zeile (Format-String, Zier-Zeichen sprachneutral)
    // ===================================================================
    DAYVIEW_NEWS_FMT,               ///< "+++ {0} +++" - einheitliches Meldungsformat

    // ===================================================================
    // Dialog-Titel und -Nachrichten
    // ===================================================================
    DAYVIEW_PRESET_TITLE,           ///< "FAVORITEN" / "FAVOURITES"
    DAYVIEW_CLEAR_TITLE,            ///< "LISTE LEEREN?" / "CLEAR LIST?"
    DAYVIEW_CLEAR_MESSAGE,          ///< "{0:%d} Titel in der Abspielliste.\nWirklich leeren?" / "{0:%d} tracks in the playlist.\nReally clear?"

    // ===================================================================
    // StationPicker-Texte (Sub-Komponente des DayView-Moduls)
    // ===================================================================
    DAYVIEW_STATIONPICKER_TITLE,    ///< "SENDER" / "STATIONS"
    DAYVIEW_STATIONPICKER_CANCEL,   ///< "Abbruch" / "Cancel"

    // ===================================================================
    // WMO-Wettercodes (open-meteo)
    // ===================================================================
    DAYVIEW_WMO_CLEAR,              ///< 0: "Klarer Himmel" / "Clear sky"
    DAYVIEW_WMO_MOSTLY_SUNNY,       ///< 1: "Meist sonnig" / "Mostly sunny"
    DAYVIEW_WMO_PARTLY_CLOUDY,      ///< 2: "Teils bewölkt" / "Partly cloudy"
    DAYVIEW_WMO_OVERCAST,           ///< 3: "Bedeckt" / "Overcast"
    DAYVIEW_WMO_FOG,                ///< 45: "Nebel" / "Fog"
    DAYVIEW_WMO_RIME_FOG,           ///< 48: "Reifnebel" / "Rime fog"
    DAYVIEW_WMO_DRIZZLE_LIGHT,      ///< 51: "Leichter Niesel" / "Light drizzle"
    DAYVIEW_WMO_DRIZZLE,            ///< 53: "Nieselregen" / "Drizzle"
    DAYVIEW_WMO_DRIZZLE_HEAVY,      ///< 55: "Starker Niesel" / "Heavy drizzle"
    DAYVIEW_WMO_RAIN_LIGHT,         ///< 61: "Leichter Regen" / "Light rain"
    DAYVIEW_WMO_RAIN,               ///< 63: "Regen" / "Rain"
    DAYVIEW_WMO_RAIN_HEAVY,         ///< 65: "Starker Regen" / "Heavy rain"
    DAYVIEW_WMO_FREEZING_RAIN,      ///< 66: "Gefrierender Regen" / "Freezing rain"
    DAYVIEW_WMO_FREEZING_RAIN_HEAVY,///< 67: "Starker Eisregen" / "Heavy freezing rain"
    DAYVIEW_WMO_SNOW_LIGHT,         ///< 71: "Leichter Schneefall" / "Light snowfall"
    DAYVIEW_WMO_SNOW,               ///< 73: "Schneefall" / "Snowfall"
    DAYVIEW_WMO_SNOW_HEAVY,         ///< 75: "Starker Schneefall" / "Heavy snowfall"
    DAYVIEW_WMO_SNOW_GRAINS,        ///< 77: "Schneegriesel" / "Snow grains"
    DAYVIEW_WMO_SHOWERS_LIGHT,      ///< 80: "Leichte Schauer" / "Light showers"
    DAYVIEW_WMO_SHOWERS,            ///< 81: "Regenschauer" / "Rain showers"
    DAYVIEW_WMO_SHOWERS_HEAVY,      ///< 82: "Starke Schauer" / "Heavy showers"
    DAYVIEW_WMO_SNOW_SHOWERS,       ///< 85: "Schneeschauer" / "Snow showers"
    DAYVIEW_WMO_SNOW_SHOWERS_HEAVY, ///< 86: "Starke Schneeschauer" / "Heavy snow showers"
    DAYVIEW_WMO_THUNDER,            ///< 95: "Gewitter" / "Thunderstorm"
    DAYVIEW_WMO_THUNDER_HAIL,       ///< 96: "Gewitter mit Hagel" / "Thunderstorm with hail"
    DAYVIEW_WMO_THUNDER_HEAVY,      ///< 99: "Starkes Gewitter" / "Severe thunderstorm"
    DAYVIEW_WMO_UNKNOWN,            ///< "Unbekannt" / "Unknown"

    DAYVIEW_KEY_COUNT ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_DAYVIEW_H
