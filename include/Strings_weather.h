#ifndef STRINGS_WEATHER_H
#define STRINGS_WEATHER_H

#include <stdint.h>

/**
 * @brief Schlüssel für alle Texte des Wetter-Moduls
 *
 * Die WMO-Codes werden nicht direkt als Enum-Werte gemappt, sondern über eine separate Mapping-Funktion (`weatherKeyFromWmo()` in `Strings_weather.cpp`). So bleibt das Enum kompakt und die WMO-Code-Liste ist an einer Stelle gebündelt.
 */
enum class WeatherKey : uint16_t
{
    // ------------------------------------------------------------------------
    // Ladescreen und Status
    // ------------------------------------------------------------------------
    WEATHER_LOADING = 0,            ///< "- lade wetterdaten -" / "- loading weather -"

    // ------------------------------------------------------------------------
    // Detail-Labels (mit Platzhalter {0:%s} für den Wert)
    // ------------------------------------------------------------------------
    WEATHER_FEELS_LIKE,             ///< "Gefühlt: {0} °C" / "Feels like: {0} °C"
    WEATHER_WIND,                   ///< "Wind: {0} km/h"
    WEATHER_HUMIDITY,               ///< "Feuchtigkeit: {0} %" / "Humidity: {0} %"

    // ------------------------------------------------------------------------
    // Zeitstempel-Format im Header
    //
    // Platzhalter:
    //   {0:%s} = Wochentag (kurz)
    //   {1:%s} = Monat (kurz)
    //   {2:%d} = Tag
    //   {3:%d} = Jahr
    //   {4:%d} = Stunde
    //   {5:%d} = Minute
    // ------------------------------------------------------------------------
    WEATHER_TIMESTAMP,              ///< "Mo. 19. Mai. 2026 - 13:01" / "Mon, 19 May 2026 - 13:01"

    // ------------------------------------------------------------------------
    // Fehlerbildschirme
    // ------------------------------------------------------------------------
    WEATHER_ERR_WIFI_TITLE,         ///< "Keine WLAN-Verbindung" / "No Wi-Fi connection"
    WEATHER_ERR_WIFI_SUB,           ///< "Prüfe Netzwerk" / "Check network"
    WEATHER_ERR_JSON_TITLE,         ///< "Daten-Fehler" / "Data error"
    WEATHER_ERR_JSON_SUB,           ///< "Antwort unlesbar" / "Response unreadable"
    WEATHER_ERR_API_TITLE,          ///< "Serverfehler" / "Server error"

    // ------------------------------------------------------------------------
    // Wetter-Beschreibungen nach WMO-Code
    // ------------------------------------------------------------------------
    WEATHER_WMO_CLEAR,              ///< 0: "Klarer Himmel" / "Clear sky"
    WEATHER_WMO_MAINLY_SUNNY,       ///< 1: "Meist sonnig" / "Mainly sunny"
    WEATHER_WMO_PARTLY_CLOUDY,      ///< 2: "Teils bewölkt" / "Partly cloudy"
    WEATHER_WMO_OVERCAST,           ///< 3: "Bedeckt" / "Overcast"
    WEATHER_WMO_FOG,                ///< 45: "Nebel" / "Fog"
    WEATHER_WMO_RIME_FOG,           ///< 48: "Reifnebel" / "Rime fog"
    WEATHER_WMO_DRIZZLE_LIGHT,      ///< 51: "Leichter Niesel" / "Light drizzle"
    WEATHER_WMO_DRIZZLE,            ///< 53: "Nieselregen" / "Drizzle"
    WEATHER_WMO_DRIZZLE_HEAVY,      ///< 55: "Starker Niesel" / "Heavy drizzle"
    WEATHER_WMO_RAIN_LIGHT,         ///< 61: "Leichter Regen" / "Light rain"
    WEATHER_WMO_RAIN,               ///< 63: "Regen" / "Rain"
    WEATHER_WMO_RAIN_HEAVY,         ///< 65: "Starker Regen" / "Heavy rain"
    WEATHER_WMO_FREEZING_RAIN,      ///< 66: "Gefrierender Regen" / "Freezing rain"
    WEATHER_WMO_SNOW_LIGHT,         ///< 71: "Leichter Schneefall" / "Light snow"
    WEATHER_WMO_SNOW,               ///< 73: "Schneefall" / "Snow"
    WEATHER_WMO_SNOW_HEAVY,         ///< 75: "Starker Schneefall" / "Heavy snow"
    WEATHER_WMO_SHOWERS_LIGHT,      ///< 80: "Leichte Schauer" / "Light showers"
    WEATHER_WMO_SHOWERS,            ///< 81: "Regenschauer" / "Showers"
    WEATHER_WMO_SHOWERS_HEAVY,      ///< 82: "Starke Schauer" / "Heavy showers"
    WEATHER_WMO_THUNDERSTORM,       ///< 95: "Gewitter" / "Thunderstorm"
    WEATHER_WMO_THUNDERSTORM_HAIL,  ///< 96: "Gewitter mit Hagel" / "Thunderstorm with hail"
    WEATHER_WMO_DEFAULT,            ///< default: "Bewölkt" / "Cloudy"

    WEATHER_KEY_COUNT               ///< Endmarker
};

/**
 * @brief Mappt einen WMO-Wettercode auf den zugehörigen WeatherKey
 * @param wmoCode WMO-Code von Open-Meteo, oder -1 für "noch keine Daten"
 * @return Passender WeatherKey - bei wmoCode == -1 ist das WEATHER_LOADING,
 *         bei unbekanntem Code WEATHER_WMO_DEFAULT
 */
WeatherKey weatherKeyFromWmo(int wmoCode);

#endif // STRINGS_WEATHER_H
