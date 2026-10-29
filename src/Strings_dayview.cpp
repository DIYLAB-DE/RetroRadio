#include "Strings_dayview.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    // Sektions-Überschriften
    const char DE_LABEL_EVENTS[]       PROGMEM = "TERMINE";
    const char DE_LABEL_TASKS[]        PROGMEM = "AUFGABEN";
    const char DE_LABEL_BIRTHDAYS[]    PROGMEM = "GEBURTSTAGE";
    const char DE_LABEL_EVENTS_DEMO[]  PROGMEM = "TERMINE (DEMO)";
    const char DE_LABEL_TASKS_DEMO[]   PROGMEM = "AUFGABEN (DEMO)";
    const char DE_LABEL_BIRTHDAYS_DEMO[] PROGMEM = "GEBURTSTAGE (DEMO)";

    // Platzhalter-Texte
    const char DE_NO_EVENTS[]          PROGMEM = "keine Termine";
    const char DE_NO_TASKS[]           PROGMEM = "keine offenen Aufgaben";
    const char DE_NO_BIRTHDAYS[]       PROGMEM = "heute keine Geburtstage";
    const char DE_LOADING_EVENTS[]     PROGMEM = "Lade Termine ...";
    const char DE_LOADING_TASKS[]      PROGMEM = "Lade Aufgaben ...";

    // Wetter-Detailzeilen
    const char DE_WEATHER_FEELS_LIKE[] PROGMEM = "Gefühlt: {0:%d}°C";
    const char DE_WEATHER_HUMIDITY[]   PROGMEM = "Feuchtigkeit: {0:%d} %";
    const char DE_WEATHER_WIND[]       PROGMEM = "Wind: {0:%d} km/h";

    // Datums-Templates
    const char DE_DATE_HEADER_FMT[]    PROGMEM = "{0}, {2:%d}. {1} {4:%d}";
    const char DE_DAYLABEL_FMT[]       PROGMEM = "{0}, {2:%d}. {1}";

    // Badges
    const char DE_BADGE_MORE[]         PROGMEM = "+{0:%d} weitere";

    // News-Zeile
    const char DE_NEWS_FMT[]           PROGMEM = "+++ {0} +++";

    // Dialog-Texte
    const char DE_PRESET_TITLE[]       PROGMEM = "FAVORITEN";
    const char DE_CLEAR_TITLE[]        PROGMEM = "LISTE LEEREN?";
    const char DE_CLEAR_MESSAGE[]      PROGMEM = "{0:%d} Titel in der Abspielliste.\nWirklich leeren?";

    // StationPicker-Texte
    const char DE_STATIONPICKER_TITLE[]  PROGMEM = "SENDER";
    const char DE_STATIONPICKER_CANCEL[] PROGMEM = "Abbruch";

    // WMO-Wettercodes
    const char DE_WMO_CLEAR[]               PROGMEM = "Klarer Himmel";
    const char DE_WMO_MOSTLY_SUNNY[]        PROGMEM = "Meist sonnig";
    const char DE_WMO_PARTLY_CLOUDY[]       PROGMEM = "Teils bewölkt";
    const char DE_WMO_OVERCAST[]            PROGMEM = "Bedeckt";
    const char DE_WMO_FOG[]                 PROGMEM = "Nebel";
    const char DE_WMO_RIME_FOG[]            PROGMEM = "Reifnebel";
    const char DE_WMO_DRIZZLE_LIGHT[]       PROGMEM = "Leichter Niesel";
    const char DE_WMO_DRIZZLE[]             PROGMEM = "Nieselregen";
    const char DE_WMO_DRIZZLE_HEAVY[]       PROGMEM = "Starker Niesel";
    const char DE_WMO_RAIN_LIGHT[]          PROGMEM = "Leichter Regen";
    const char DE_WMO_RAIN[]                PROGMEM = "Regen";
    const char DE_WMO_RAIN_HEAVY[]          PROGMEM = "Starker Regen";
    const char DE_WMO_FREEZING_RAIN[]       PROGMEM = "Gefrierender Regen";
    const char DE_WMO_FREEZING_RAIN_HEAVY[] PROGMEM = "Starker Eisregen";
    const char DE_WMO_SNOW_LIGHT[]          PROGMEM = "Leichter Schneefall";
    const char DE_WMO_SNOW[]                PROGMEM = "Schneefall";
    const char DE_WMO_SNOW_HEAVY[]          PROGMEM = "Starker Schneefall";
    const char DE_WMO_SNOW_GRAINS[]         PROGMEM = "Schneegriesel";
    const char DE_WMO_SHOWERS_LIGHT[]       PROGMEM = "Leichte Schauer";
    const char DE_WMO_SHOWERS[]             PROGMEM = "Regenschauer";
    const char DE_WMO_SHOWERS_HEAVY[]       PROGMEM = "Starke Schauer";
    const char DE_WMO_SNOW_SHOWERS[]        PROGMEM = "Schneeschauer";
    const char DE_WMO_SNOW_SHOWERS_HEAVY[]  PROGMEM = "Starke Schneeschauer";
    const char DE_WMO_THUNDER[]             PROGMEM = "Gewitter";
    const char DE_WMO_THUNDER_HAIL[]        PROGMEM = "Gewitter mit Hagel";
    const char DE_WMO_THUNDER_HEAVY[]       PROGMEM = "Starkes Gewitter";
    const char DE_WMO_UNKNOWN[]             PROGMEM = "Unbekannt";

    // ===================================================================
    // PROGMEM-Literale (Englisch, British)
    // ===================================================================

    const char EN_LABEL_EVENTS[]       PROGMEM = "EVENTS";
    const char EN_LABEL_TASKS[]        PROGMEM = "TASKS";
    const char EN_LABEL_BIRTHDAYS[]    PROGMEM = "BIRTHDAYS";
    const char EN_LABEL_EVENTS_DEMO[]  PROGMEM = "EVENTS (DEMO)";
    const char EN_LABEL_TASKS_DEMO[]   PROGMEM = "TASKS (DEMO)";
    const char EN_LABEL_BIRTHDAYS_DEMO[] PROGMEM = "BIRTHDAYS (DEMO)";

    const char EN_NO_EVENTS[]          PROGMEM = "no events";
    const char EN_NO_TASKS[]           PROGMEM = "no open tasks";
    const char EN_NO_BIRTHDAYS[]       PROGMEM = "no birthdays today";
    const char EN_LOADING_EVENTS[]     PROGMEM = "Loading events ...";
    const char EN_LOADING_TASKS[]      PROGMEM = "Loading tasks ...";

    const char EN_WEATHER_FEELS_LIKE[] PROGMEM = "Feels like: {0:%d}°C";
    const char EN_WEATHER_HUMIDITY[]   PROGMEM = "Humidity: {0:%d}%";
    const char EN_WEATHER_WIND[]       PROGMEM = "Wind: {0:%d} km/h";

    const char EN_DATE_HEADER_FMT[]    PROGMEM = "{0}, {2:%d} {1} {4:%d}";
    const char EN_DAYLABEL_FMT[]       PROGMEM = "{0}, {2:%d} {1}";

    const char EN_BADGE_MORE[]         PROGMEM = "+{0:%d} more";

    const char EN_NEWS_FMT[]           PROGMEM = "+++ {0} +++";

    const char EN_PRESET_TITLE[]       PROGMEM = "FAVOURITES";
    const char EN_CLEAR_TITLE[]        PROGMEM = "CLEAR LIST?";
    const char EN_CLEAR_MESSAGE[]      PROGMEM = "{0:%d} tracks in the playlist.\nReally clear?";

    const char EN_STATIONPICKER_TITLE[]  PROGMEM = "STATIONS";
    const char EN_STATIONPICKER_CANCEL[] PROGMEM = "Cancel";

    const char EN_WMO_CLEAR[]               PROGMEM = "Clear sky";
    const char EN_WMO_MOSTLY_SUNNY[]        PROGMEM = "Mostly sunny";
    const char EN_WMO_PARTLY_CLOUDY[]       PROGMEM = "Partly cloudy";
    const char EN_WMO_OVERCAST[]            PROGMEM = "Overcast";
    const char EN_WMO_FOG[]                 PROGMEM = "Fog";
    const char EN_WMO_RIME_FOG[]            PROGMEM = "Rime fog";
    const char EN_WMO_DRIZZLE_LIGHT[]       PROGMEM = "Light drizzle";
    const char EN_WMO_DRIZZLE[]             PROGMEM = "Drizzle";
    const char EN_WMO_DRIZZLE_HEAVY[]       PROGMEM = "Heavy drizzle";
    const char EN_WMO_RAIN_LIGHT[]          PROGMEM = "Light rain";
    const char EN_WMO_RAIN[]                PROGMEM = "Rain";
    const char EN_WMO_RAIN_HEAVY[]          PROGMEM = "Heavy rain";
    const char EN_WMO_FREEZING_RAIN[]       PROGMEM = "Freezing rain";
    const char EN_WMO_FREEZING_RAIN_HEAVY[] PROGMEM = "Heavy freezing rain";
    const char EN_WMO_SNOW_LIGHT[]          PROGMEM = "Light snowfall";
    const char EN_WMO_SNOW[]                PROGMEM = "Snowfall";
    const char EN_WMO_SNOW_HEAVY[]          PROGMEM = "Heavy snowfall";
    const char EN_WMO_SNOW_GRAINS[]         PROGMEM = "Snow grains";
    const char EN_WMO_SHOWERS_LIGHT[]       PROGMEM = "Light showers";
    const char EN_WMO_SHOWERS[]             PROGMEM = "Rain showers";
    const char EN_WMO_SHOWERS_HEAVY[]       PROGMEM = "Heavy showers";
    const char EN_WMO_SNOW_SHOWERS[]        PROGMEM = "Snow showers";
    const char EN_WMO_SNOW_SHOWERS_HEAVY[]  PROGMEM = "Heavy snow showers";
    const char EN_WMO_THUNDER[]             PROGMEM = "Thunderstorm";
    const char EN_WMO_THUNDER_HAIL[]        PROGMEM = "Thunderstorm with hail";
    const char EN_WMO_THUNDER_HEAVY[]       PROGMEM = "Severe thunderstorm";
    const char EN_WMO_UNKNOWN[]             PROGMEM = "Unknown";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const DAYVIEW_TABLE[static_cast<size_t>(DayViewKey::DAYVIEW_KEY_COUNT)]
                             [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        // Sektions-Überschriften
        { DE_LABEL_EVENTS,      EN_LABEL_EVENTS      },
        { DE_LABEL_TASKS,       EN_LABEL_TASKS       },
        { DE_LABEL_BIRTHDAYS,   EN_LABEL_BIRTHDAYS   },
        { DE_LABEL_EVENTS_DEMO, EN_LABEL_EVENTS_DEMO },
        { DE_LABEL_TASKS_DEMO,  EN_LABEL_TASKS_DEMO  },
        { DE_LABEL_BIRTHDAYS_DEMO, EN_LABEL_BIRTHDAYS_DEMO },

        // Platzhalter-Texte
        { DE_NO_EVENTS,      EN_NO_EVENTS      },
        { DE_NO_TASKS,       EN_NO_TASKS       },
        { DE_NO_BIRTHDAYS,   EN_NO_BIRTHDAYS   },
        { DE_LOADING_EVENTS, EN_LOADING_EVENTS },
        { DE_LOADING_TASKS,  EN_LOADING_TASKS  },

        // Wetter-Detailzeilen
        { DE_WEATHER_FEELS_LIKE, EN_WEATHER_FEELS_LIKE },
        { DE_WEATHER_HUMIDITY,   EN_WEATHER_HUMIDITY   },
        { DE_WEATHER_WIND,       EN_WEATHER_WIND       },

        // Datums-Templates
        { DE_DATE_HEADER_FMT, EN_DATE_HEADER_FMT },
        { DE_DAYLABEL_FMT,    EN_DAYLABEL_FMT    },

        // Badges
        { DE_BADGE_MORE, EN_BADGE_MORE },

        // News-Zeile
        { DE_NEWS_FMT, EN_NEWS_FMT },

        // Dialog-Texte
        { DE_PRESET_TITLE,  EN_PRESET_TITLE  },
        { DE_CLEAR_TITLE,   EN_CLEAR_TITLE   },
        { DE_CLEAR_MESSAGE, EN_CLEAR_MESSAGE },

        // StationPicker-Texte
        { DE_STATIONPICKER_TITLE,  EN_STATIONPICKER_TITLE  },
        { DE_STATIONPICKER_CANCEL, EN_STATIONPICKER_CANCEL },

        // WMO-Wettercodes
        { DE_WMO_CLEAR,                EN_WMO_CLEAR                },
        { DE_WMO_MOSTLY_SUNNY,         EN_WMO_MOSTLY_SUNNY         },
        { DE_WMO_PARTLY_CLOUDY,        EN_WMO_PARTLY_CLOUDY        },
        { DE_WMO_OVERCAST,             EN_WMO_OVERCAST             },
        { DE_WMO_FOG,                  EN_WMO_FOG                  },
        { DE_WMO_RIME_FOG,             EN_WMO_RIME_FOG             },
        { DE_WMO_DRIZZLE_LIGHT,        EN_WMO_DRIZZLE_LIGHT        },
        { DE_WMO_DRIZZLE,              EN_WMO_DRIZZLE              },
        { DE_WMO_DRIZZLE_HEAVY,        EN_WMO_DRIZZLE_HEAVY        },
        { DE_WMO_RAIN_LIGHT,           EN_WMO_RAIN_LIGHT           },
        { DE_WMO_RAIN,                 EN_WMO_RAIN                 },
        { DE_WMO_RAIN_HEAVY,           EN_WMO_RAIN_HEAVY           },
        { DE_WMO_FREEZING_RAIN,        EN_WMO_FREEZING_RAIN        },
        { DE_WMO_FREEZING_RAIN_HEAVY,  EN_WMO_FREEZING_RAIN_HEAVY  },
        { DE_WMO_SNOW_LIGHT,           EN_WMO_SNOW_LIGHT           },
        { DE_WMO_SNOW,                 EN_WMO_SNOW                 },
        { DE_WMO_SNOW_HEAVY,           EN_WMO_SNOW_HEAVY           },
        { DE_WMO_SNOW_GRAINS,          EN_WMO_SNOW_GRAINS          },
        { DE_WMO_SHOWERS_LIGHT,        EN_WMO_SHOWERS_LIGHT        },
        { DE_WMO_SHOWERS,              EN_WMO_SHOWERS              },
        { DE_WMO_SHOWERS_HEAVY,        EN_WMO_SHOWERS_HEAVY        },
        { DE_WMO_SNOW_SHOWERS,         EN_WMO_SNOW_SHOWERS         },
        { DE_WMO_SNOW_SHOWERS_HEAVY,   EN_WMO_SNOW_SHOWERS_HEAVY   },
        { DE_WMO_THUNDER,              EN_WMO_THUNDER              },
        { DE_WMO_THUNDER_HAIL,         EN_WMO_THUNDER_HAIL         },
        { DE_WMO_THUNDER_HEAVY,        EN_WMO_THUNDER_HEAVY        },
        { DE_WMO_UNKNOWN,              EN_WMO_UNKNOWN              }
    };

    static_assert(sizeof(DAYVIEW_TABLE) / sizeof(DAYVIEW_TABLE[0])
                  == static_cast<size_t>(DayViewKey::DAYVIEW_KEY_COUNT),
                  "DAYVIEW_TABLE / DayViewKey out of sync");

    // Fallback für den unwahrscheinlichen Fall eines fehlenden Eintrags
    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten DayView-Text für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getDayView(DayViewKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(DayViewKey::DAYVIEW_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&DAYVIEW_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&DAYVIEW_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}
