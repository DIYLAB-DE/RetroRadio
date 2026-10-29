#include "Strings_weather.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    // Ladescreen
    const char DE_LOADING[]                  PROGMEM = "- lade wetterdaten -";

    // Detail-Labels mit Platzhalter
    const char DE_FEELS_LIKE[]               PROGMEM = "Gefühlt: {0} °C";
    const char DE_WIND[]                     PROGMEM = "Wind: {0} km/h";
    const char DE_HUMIDITY[]                 PROGMEM = "Feuchtigkeit: {0} %";

    // Zeitstempel-Format im Header
    const char DE_TIMESTAMP[]                PROGMEM = "{0}. {2:%d}. {1}. {3:%d} - {4:%02d}:{5:%02d}";

    // Fehlerbildschirme
    const char DE_ERR_WIFI_TITLE[]           PROGMEM = "Keine WLAN-Verbindung";
    const char DE_ERR_WIFI_SUB[]             PROGMEM = "Prüfe Netzwerk";
    const char DE_ERR_JSON_TITLE[]           PROGMEM = "Daten-Fehler";
    const char DE_ERR_JSON_SUB[]             PROGMEM = "Antwort unlesbar";
    const char DE_ERR_API_TITLE[]            PROGMEM = "Serverfehler";

    // WMO-Wetterbeschreibungen
    const char DE_WMO_CLEAR[]                PROGMEM = "Klarer Himmel";
    const char DE_WMO_MAINLY_SUNNY[]         PROGMEM = "Meist sonnig";
    const char DE_WMO_PARTLY_CLOUDY[]        PROGMEM = "Teils bewölkt";
    const char DE_WMO_OVERCAST[]             PROGMEM = "Bedeckt";
    const char DE_WMO_FOG[]                  PROGMEM = "Nebel";
    const char DE_WMO_RIME_FOG[]             PROGMEM = "Reifnebel";
    const char DE_WMO_DRIZZLE_LIGHT[]        PROGMEM = "Leichter Niesel";
    const char DE_WMO_DRIZZLE[]              PROGMEM = "Nieselregen";
    const char DE_WMO_DRIZZLE_HEAVY[]        PROGMEM = "Starker Niesel";
    const char DE_WMO_RAIN_LIGHT[]           PROGMEM = "Leichter Regen";
    const char DE_WMO_RAIN[]                 PROGMEM = "Regen";
    const char DE_WMO_RAIN_HEAVY[]           PROGMEM = "Starker Regen";
    const char DE_WMO_FREEZING_RAIN[]        PROGMEM = "Gefrierender Regen";
    const char DE_WMO_SNOW_LIGHT[]           PROGMEM = "Leichter Schneefall";
    const char DE_WMO_SNOW[]                 PROGMEM = "Schneefall";
    const char DE_WMO_SNOW_HEAVY[]           PROGMEM = "Starker Schneefall";
    const char DE_WMO_SHOWERS_LIGHT[]        PROGMEM = "Leichte Schauer";
    const char DE_WMO_SHOWERS[]              PROGMEM = "Regenschauer";
    const char DE_WMO_SHOWERS_HEAVY[]        PROGMEM = "Starke Schauer";
    const char DE_WMO_THUNDERSTORM[]         PROGMEM = "Gewitter";
    const char DE_WMO_THUNDERSTORM_HAIL[]    PROGMEM = "Gewitter mit Hagel";
    const char DE_WMO_DEFAULT[]              PROGMEM = "Bewölkt";

    // ===================================================================
    // PROGMEM-Literale (Englisch, British)
    // ===================================================================

    const char EN_LOADING[]                  PROGMEM = "- loading weather -";

    const char EN_FEELS_LIKE[]               PROGMEM = "Feels like: {0} °C";
    const char EN_WIND[]                     PROGMEM = "Wind: {0} km/h";
    const char EN_HUMIDITY[]                 PROGMEM = "Humidity: {0} %";

    const char EN_TIMESTAMP[]                PROGMEM = "{0}, {2:%d} {1} {3:%d} - {4:%02d}:{5:%02d}";

    const char EN_ERR_WIFI_TITLE[]           PROGMEM = "No Wi-Fi connection";
    const char EN_ERR_WIFI_SUB[]             PROGMEM = "Check network";
    const char EN_ERR_JSON_TITLE[]           PROGMEM = "Data error";
    const char EN_ERR_JSON_SUB[]             PROGMEM = "Response unreadable";
    const char EN_ERR_API_TITLE[]            PROGMEM = "Server error";

    const char EN_WMO_CLEAR[]                PROGMEM = "Clear sky";
    const char EN_WMO_MAINLY_SUNNY[]         PROGMEM = "Mainly sunny";
    const char EN_WMO_PARTLY_CLOUDY[]        PROGMEM = "Partly cloudy";
    const char EN_WMO_OVERCAST[]             PROGMEM = "Overcast";
    const char EN_WMO_FOG[]                  PROGMEM = "Fog";
    const char EN_WMO_RIME_FOG[]             PROGMEM = "Rime fog";
    const char EN_WMO_DRIZZLE_LIGHT[]        PROGMEM = "Light drizzle";
    const char EN_WMO_DRIZZLE[]              PROGMEM = "Drizzle";
    const char EN_WMO_DRIZZLE_HEAVY[]        PROGMEM = "Heavy drizzle";
    const char EN_WMO_RAIN_LIGHT[]           PROGMEM = "Light rain";
    const char EN_WMO_RAIN[]                 PROGMEM = "Rain";
    const char EN_WMO_RAIN_HEAVY[]           PROGMEM = "Heavy rain";
    const char EN_WMO_FREEZING_RAIN[]        PROGMEM = "Freezing rain";
    const char EN_WMO_SNOW_LIGHT[]           PROGMEM = "Light snow";
    const char EN_WMO_SNOW[]                 PROGMEM = "Snow";
    const char EN_WMO_SNOW_HEAVY[]           PROGMEM = "Heavy snow";
    const char EN_WMO_SHOWERS_LIGHT[]        PROGMEM = "Light showers";
    const char EN_WMO_SHOWERS[]              PROGMEM = "Showers";
    const char EN_WMO_SHOWERS_HEAVY[]        PROGMEM = "Heavy showers";
    const char EN_WMO_THUNDERSTORM[]         PROGMEM = "Thunderstorm";
    const char EN_WMO_THUNDERSTORM_HAIL[]    PROGMEM = "Thunderstorm with hail";
    const char EN_WMO_DEFAULT[]              PROGMEM = "Cloudy";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const WEATHER_TABLE[static_cast<size_t>(WeatherKey::WEATHER_KEY_COUNT)]
                             [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        { DE_LOADING,                EN_LOADING                },
        { DE_FEELS_LIKE,             EN_FEELS_LIKE             },
        { DE_WIND,                   EN_WIND                   },
        { DE_HUMIDITY,               EN_HUMIDITY               },

        { DE_TIMESTAMP,              EN_TIMESTAMP              },

        { DE_ERR_WIFI_TITLE,         EN_ERR_WIFI_TITLE         },
        { DE_ERR_WIFI_SUB,           EN_ERR_WIFI_SUB           },
        { DE_ERR_JSON_TITLE,         EN_ERR_JSON_TITLE         },
        { DE_ERR_JSON_SUB,           EN_ERR_JSON_SUB           },
        { DE_ERR_API_TITLE,          EN_ERR_API_TITLE          },

        { DE_WMO_CLEAR,              EN_WMO_CLEAR              },
        { DE_WMO_MAINLY_SUNNY,       EN_WMO_MAINLY_SUNNY       },
        { DE_WMO_PARTLY_CLOUDY,      EN_WMO_PARTLY_CLOUDY      },
        { DE_WMO_OVERCAST,           EN_WMO_OVERCAST           },
        { DE_WMO_FOG,                EN_WMO_FOG                },
        { DE_WMO_RIME_FOG,           EN_WMO_RIME_FOG           },
        { DE_WMO_DRIZZLE_LIGHT,      EN_WMO_DRIZZLE_LIGHT      },
        { DE_WMO_DRIZZLE,            EN_WMO_DRIZZLE            },
        { DE_WMO_DRIZZLE_HEAVY,      EN_WMO_DRIZZLE_HEAVY      },
        { DE_WMO_RAIN_LIGHT,         EN_WMO_RAIN_LIGHT         },
        { DE_WMO_RAIN,               EN_WMO_RAIN               },
        { DE_WMO_RAIN_HEAVY,         EN_WMO_RAIN_HEAVY         },
        { DE_WMO_FREEZING_RAIN,      EN_WMO_FREEZING_RAIN      },
        { DE_WMO_SNOW_LIGHT,         EN_WMO_SNOW_LIGHT         },
        { DE_WMO_SNOW,               EN_WMO_SNOW               },
        { DE_WMO_SNOW_HEAVY,         EN_WMO_SNOW_HEAVY         },
        { DE_WMO_SHOWERS_LIGHT,      EN_WMO_SHOWERS_LIGHT      },
        { DE_WMO_SHOWERS,            EN_WMO_SHOWERS            },
        { DE_WMO_SHOWERS_HEAVY,      EN_WMO_SHOWERS_HEAVY      },
        { DE_WMO_THUNDERSTORM,       EN_WMO_THUNDERSTORM       },
        { DE_WMO_THUNDERSTORM_HAIL,  EN_WMO_THUNDERSTORM_HAIL  },
        { DE_WMO_DEFAULT,            EN_WMO_DEFAULT            }
    };

    static_assert(sizeof(WEATHER_TABLE) / sizeof(WEATHER_TABLE[0])
                  == static_cast<size_t>(WeatherKey::WEATHER_KEY_COUNT),
                  "WEATHER_TABLE / WeatherKey out of sync");

    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten Wettertext für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getWeather(WeatherKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(WeatherKey::WEATHER_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&WEATHER_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&WEATHER_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}

// ===================================================================
// WMO-Code-Mapping
// ===================================================================

/**
 * @brief Mappt einen WMO-Wettercode auf den zugehörigen WeatherKey
 */
WeatherKey weatherKeyFromWmo(int wmoCode)
{
    if (wmoCode < 0)
        return WeatherKey::WEATHER_LOADING;

    switch (wmoCode)
    {
    case 0:  return WeatherKey::WEATHER_WMO_CLEAR;
    case 1:  return WeatherKey::WEATHER_WMO_MAINLY_SUNNY;
    case 2:  return WeatherKey::WEATHER_WMO_PARTLY_CLOUDY;
    case 3:  return WeatherKey::WEATHER_WMO_OVERCAST;
    case 45: return WeatherKey::WEATHER_WMO_FOG;
    case 48: return WeatherKey::WEATHER_WMO_RIME_FOG;
    case 51: return WeatherKey::WEATHER_WMO_DRIZZLE_LIGHT;
    case 53: return WeatherKey::WEATHER_WMO_DRIZZLE;
    case 55: return WeatherKey::WEATHER_WMO_DRIZZLE_HEAVY;
    case 61: return WeatherKey::WEATHER_WMO_RAIN_LIGHT;
    case 63: return WeatherKey::WEATHER_WMO_RAIN;
    case 65: return WeatherKey::WEATHER_WMO_RAIN_HEAVY;
    case 66: return WeatherKey::WEATHER_WMO_FREEZING_RAIN;
    case 71: return WeatherKey::WEATHER_WMO_SNOW_LIGHT;
    case 73: return WeatherKey::WEATHER_WMO_SNOW;
    case 75: return WeatherKey::WEATHER_WMO_SNOW_HEAVY;
    case 80: return WeatherKey::WEATHER_WMO_SHOWERS_LIGHT;
    case 81: return WeatherKey::WEATHER_WMO_SHOWERS;
    case 82: return WeatherKey::WEATHER_WMO_SHOWERS_HEAVY;
    case 95: return WeatherKey::WEATHER_WMO_THUNDERSTORM;
    case 96: return WeatherKey::WEATHER_WMO_THUNDERSTORM_HAIL;
    default: return WeatherKey::WEATHER_WMO_DEFAULT;
    }
}
