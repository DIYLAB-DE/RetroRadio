#ifndef WEATHER_H
#define WEATHER_H

#include <atomic>
#include <cmath> // NAN, std::isnan

#include "BaseModule.h"
#include "Button.h"
#include "DotMatrix.h"
#include "Gfx.h"
#include "Logger.h"
#include "Symbols.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "I18n.h"

// ============================================================================
// WEATHER-KONFIGURATION
// ============================================================================

/**
 * @brief Konfigurationskonstanten für das Wetter-Modul
 */
namespace WeatherCfg
{
    constexpr int NUM_FORECAST_DAYS = 5;     ///< Anzahl Vorhersage-Tage
    constexpr int BUFFER_COUNT = 2;          ///< Anzahl der Double-Buffer
    constexpr size_t CITY_NAME_LEN = 64;     ///< Maximale Länge des Stadtnamens
    constexpr size_t PENDING_CITY_LEN = 128; ///< Buffer für anstehende Stadt-Suche
    constexpr size_t VALUE_LEN = 8;          ///< Länge für Temperatur/Wind etc.
    constexpr size_t TIMESTAMP_LEN = 64;     ///< Länge für Update-Zeitstempel

    // Timing-Konstanten
    constexpr unsigned long API_CALL_INTERVAL_MS = 900000UL; ///< Mindestintervall zwischen API-Aufrufen (15 Minuten)
    constexpr unsigned long MIN_BACKOFF_MS = 30000UL;        ///< Minimales Backoff bei Fehlern (30 Sekunden)
    constexpr unsigned long MAX_BACKOFF_MS = 480000UL;       ///< Maximales Backoff bei Fehlern (8 Minuten)
}

// ============================================================================
// UI-LAYOUT KONSTANTEN
// ============================================================================

/**
 * @brief Layout-Konstanten für die Wetter-Benutzeroberfläche
 */
namespace WeatherLayout
{
    // ========================================================================
    // Allgemeine Abstände und Ränder
    // ========================================================================

    constexpr int16_t MARGIN_LEFT = 30;       ///< Linker Rand für Textinhalte
    constexpr int16_t FORECAST_MARGIN_X = 20; ///< Seitlicher Rand für Forecast-Bereich
    constexpr int16_t FORECAST_PADDING = 40;  ///< Gesamtes Padding (links + rechts) für Forecast

    // ========================================================================
    // Exit-Button
    // ========================================================================

    constexpr int16_t EXIT_BTN_X = 765;   ///< X-Position des Exit-Buttons
    constexpr int16_t EXIT_BTN_Y = 457;   ///< Y-Position des Exit-Buttons
    constexpr uint16_t BUTTON_SIZE = 45;  ///< Größe der Buttons (quadratisch)
    constexpr uint16_t BUTTON_RADIUS = 4; ///< Eckenradius der Buttons

    // ========================================================================
    // Ladebildschirm
    // ========================================================================

    constexpr int16_t LOADING_TEXT_OFFSET_Y = -25; ///< Vertikaler Offset für Ladetext (von Mitte)
    constexpr int16_t CANCEL_BTN_OFFSET_Y = 25;    ///< Vertikaler Offset für Cancel-Button (von Mitte)

    // ========================================================================
    // Fehlerbildschirm
    // ========================================================================

    constexpr int16_t ERROR_SUB_OFFSET_Y = 30; ///< Vertikaler Abstand für Unter-Nachricht

    // ========================================================================
    // Großes Wetter-Icon
    // ========================================================================

    constexpr int16_t MAIN_ICON_X = 470;     ///< X-Position des großen Wetter-Icons
    constexpr int16_t MAIN_ICON_Y = 20;      ///< Y-Position des großen Wetter-Icons
    constexpr int16_t MAIN_ICON_WIDTH = 300; ///< Breite des großen Wetter-Icons

    // ========================================================================
    // Header-Bereich (Ort und Zeitstempel)
    // ========================================================================

    constexpr int16_t CITY_Y = 30;      ///< Y-Position Stadtname
    constexpr int16_t TIMESTAMP_Y = 60; ///< Y-Position Zeitstempel

    // ========================================================================
    // Detail-Bereich
    // ========================================================================

    constexpr int16_t DESCRIPTION_Y = 110; ///< Y-Position Wetterbeschreibung
    constexpr int16_t FEELS_LIKE_Y = 140;  ///< Y-Position "Gefühlt"
    constexpr int16_t WIND_Y = 170;        ///< Y-Position Wind
    constexpr int16_t HUMIDITY_Y = 200;    ///< Y-Position Feuchtigkeit

    // ========================================================================
    // Haupt-Temperatur (DotMatrix)
    // ========================================================================

    constexpr int16_t MAIN_TEMP_Y = 135; ///< Y-Position der großen Temperaturanzeige
    constexpr int TEMP_DOT_W = 7;        ///< DotMatrix Dot-Breite
    constexpr int TEMP_DOT_H = 10;       ///< DotMatrix Dot-Höhe
    constexpr int TEMP_DOT_SPACE = 2;    ///< Dot-Abstand
    constexpr int TEMP_CHAR_SPACE = 4;   ///< Zeichen-Abstand

    constexpr int TEMP_CHAR_W = 5 * TEMP_DOT_W + 4 * TEMP_DOT_SPACE; ///< 43px pro Zeichen

    // Maximale Zeichenanzahl für Temperaturanzeige (inkl. °-Zeichen) Fälle: "25.3°"(5), "-9.5°"(5), "-20°"(4), "9.5°"(4)
    constexpr int TEMP_MAX_CHARS = 5;

    // Sprite-Breite: dynamisch aus Zeichenanzahl und Dot-Konfiguration
    constexpr int TEMP_DISP_W = TEMP_MAX_CHARS * TEMP_CHAR_W + (TEMP_MAX_CHARS - 1) * TEMP_CHAR_SPACE;

    // Sprite-Höhe: 7 Dot-Zeilen + 6 Abstände
    constexpr int TEMP_DISP_H = 7 * TEMP_DOT_H + 6 * TEMP_DOT_SPACE;

    // X-Position: dynamisch berechnet, 5px Abstand vor dem Wetter-Icon
    constexpr int16_t MAIN_TEMP_X = MAIN_ICON_X - TEMP_DISP_W - 5;
    constexpr uint16_t TEMP_DOT_ON = TFT_SILVER; ///< Leuchtende Dots
    constexpr uint16_t TEMP_DOT_OFF = 0x2104;    ///< Dunkle Dots (subtiles Raster)

    // ========================================================================
    // Vorhersage-Bereich
    // ========================================================================

    constexpr int16_t FORECAST_DAY_NAME_Y = 260;   ///< Y-Position Wochentag-Namen
    constexpr int16_t FORECAST_ICON_Y = 280;       ///< Y-Position Forecast-Icons
    constexpr int16_t FORECAST_ICON_WIDTH = 120;   ///< Breite der Forecast-Icons
    constexpr int16_t FORECAST_ICON_OFFSET_X = 60; ///< Horizontaler Offset für Icon-Zentrierung
    constexpr int16_t FORECAST_TEMP_Y = 410;       ///< Y-Position Temperatur (Max/Min)
    constexpr int16_t FORECAST_POP_Y = 435;        ///< Y-Position Regenwahrscheinlichkeit
}

// ============================================================================
// UI-FARBEN
// ============================================================================

/**
 * @brief Farbkonstanten für die Wetter-Benutzeroberfläche
 */
namespace WeatherColors
{
    // ========================================================================
    // Button-Farben
    // ========================================================================

    constexpr uint16_t BTN_FILL = 0x39E8; ///< Füllfarbe für Buttons (dunkelgrau-blau)

    // ========================================================================
    // Text-Farben
    // ========================================================================

    constexpr uint16_t TEXT_LOADING = 0xD69A; ///< Textfarbe Ladebildschirm (gedämpftes weiß)
    constexpr uint16_t TEXT_WEEKDAY = 0x3666; ///< Textfarbe Wochentag in Forecast (dunkelblau)
    constexpr uint16_t TEXT_POP = 0x5370;     ///< Textfarbe Regenwahrscheinlichkeit (blaugrün)

    // ========================================================================
    // Progressbar
    // ========================================================================

    constexpr uint16_t PROGRESS_BAR = 0x8010; ///< Farbe der Fortschrittsanzeige (grün)
}

// ============================================================================
// DATENSTRUKTUREN
// ============================================================================

/**
 * @brief Status-Aufzählung für das Wetter-Modul
 * @details Definiert die verschiedenen Zustände des Wetter-Moduls während der Datenabfrage und -verarbeitung
 */
enum class WeatherStatus
{
    LOADING,    ///< Initial beim Laden
    SUCCESS,    ///< Daten erfolgreich geladen
    ERROR_WIFI, ///< Keine WiFi-Verbindung
    ERROR_API,  ///< API-Fehler
    ERROR_JSON  ///< JSON-Parse-Fehler
};

/**
 * @brief Datenstruktur für die Wettervorhersage eines Tages
 * @details Speichert alle relevanten Wetterdaten für die tägliche Vorschau einschließlich Temperatur und Regenwahrscheinlichkeit
 */
struct ForecastDay
{
    int wmoCode = 0;        ///< WMO-Wettercode von Open-Meteo
    int8_t weekdayIdx = -1; ///< Wochentag als Index (0=So, 1=Mo, ..., 6=Sa), -1 = ungültig
    int maxTemp = 0;        ///< Höchsttemperatur
    int minTemp = 0;        ///< Tiefsttemperatur
    int pop = 0;            ///< Regenwahrscheinlichkeit in %

    /**
     * @brief Setzt alle Felder auf Initialwerte zurück.
     */
    void reset()
    {
        wmoCode = 0;
        weekdayIdx = -1;
        maxTemp = 0;
        minTemp = 0;
        pop = 0;
    }
};

/**
 * @brief Thread-sichere Datenstruktur für alle Wetter-Daten
 *
 * Wird im Double-Buffer-Verfahren zwischen Worker-Task und UI-Thread verwendet. Der Worker schreibt in den inaktiven Buffer, nach erfolgreichem Fetch werden die Buffer atomar getauscht.
 *
 * Die Wetterbeschreibung wird NICHT als String gespeichert, sondern beim Zeichnen aus `currentWmoCode` über `weatherKeyFromWmo()` aufgelöst. Das spart Speicher und ermöglicht einen Live-Sprachwechsel, ohne die Daten neu zu fetchen. Sonderfall: `currentWmoCode == -1` bedeutet "noch keine Daten" und ergibt im Lookup den Loading-Text.
 */
struct WeatherData
{
    // Aktuelle Wetterdaten
    char temperature[WeatherCfg::VALUE_LEN] = "--";      ///< Aktuelle Temperatur
    char feelsLike[WeatherCfg::VALUE_LEN] = "--";        ///< Gefühlte Temperatur
    char windSpeed[WeatherCfg::VALUE_LEN] = "--";        ///< Windgeschwindigkeit
    char humidity[WeatherCfg::VALUE_LEN] = "--";         ///< Luftfeuchtigkeit
    char cityName[WeatherCfg::CITY_NAME_LEN] = "";       ///< Stadtname
    char lastUpdateTime[WeatherCfg::TIMESTAMP_LEN] = ""; ///< Zeitstempel letztes Update
    int currentWmoCode = -1;                             ///< WMO-Code aktuell, -1 = noch keine Daten
    bool isDay = true;                                   ///< Tag oder Nacht

    // Vorhersage-Daten
    ForecastDay forecasts[WeatherCfg::NUM_FORECAST_DAYS]; ///< 5-Tage-Vorhersage

    // Standort-Daten
    float latitude = NAN;  ///< Breitengrad - kein Fallback, muss über AppState gesetzt werden
    float longitude = NAN; ///< Längengrad - kein Fallback, muss über AppState gesetzt werden

    // Zeitstempel
    unsigned long timestamp = 0; ///< Abruf-Zeitstempel

    /**
     * @brief Setzt alle Felder auf sichere Initialwerte zurück
     */
    void reset();
};

/**
 * @brief Callback-Funktionen für das Wetter-Modul
 * @details Definiert die verfügbaren Event-Handler für Benutzerinteraktionen mit dem Wetter-Modul
 */
struct WeatherCallbacks
{
    void (*onExitRequested)() = nullptr; ///< Wird aufgerufen wenn Benutzer Modul verlassen will
};

// ============================================================================
// WEATHER-KLASSE
// ============================================================================

/**
 * @brief Wetter-Anzeigemodul basierend auf der Open-Meteo API
 * @details Implementiert ein vollständiges Wettermodul mit aktuellen Wetterdaten
 *          und 5-Tage-Vorhersage. Unterstützt Geocoding für Ortsnamen und
 *          asynchrone Datenaktualisierung im Hintergrund.
 */
class Weather : public BaseModule
{
public:
    /**
     * @brief Konstruktor
     * @details Initialisiert alle Member-Variablen mit sicheren Standardwerten
     */
    Weather();

    /**
     * @brief Destruktor
     * @details Gibt alle Ressourcen frei und beendet den Worker-Task sauber
     */
    ~Weather();

    Weather(const Weather &) = delete;
    Weather &operator=(const Weather &) = delete;
    Weather(Weather &&) = delete;
    Weather &operator=(Weather &&) = delete;

    /**
     * @brief Initialisiert das Wetter-Modul
     * @param tft Referenz auf das Display-Objekt
     * @param callbacks Callback-Funktionen für Events
     * @return true bei erfolgreicher Initialisierung
     *
     * Startet den Hintergrund-Worker-Task und lädt persistierte Standortdaten aus dem AppState.
     *
     * @note HTTP-Requests werden über den zentralen HttpService abgewickelt.
     *       HttpService muss vorher initialisiert sein (HTTP_SERVICE.begin()).
     */
    bool init(LGFX &tft, const WeatherCallbacks &callbacks);

    /**
     * @brief Zeichnet die Wetter-Benutzeroberfläche
     * @details Wird vom Hauptloop aufgerufen, übernimmt neue Daten vom Worker falls verfügbar und rendert die aktuelle Ansicht.
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück
     * @return AppModule::WEATHER
     */
    AppModule type() const override;

    /**
     * @brief Setzt den Standort über einen Ortsnamen
     * @param name Ortsname (optional mit Ländercode: "Berlin, DE")
     * @return true bei erfolgreichem Trigger des Geocoding-Jobs
     *
     * Startet asynchrones Geocoding im Worker-Task. Bei bereits persistiertem Ort werden die gespeicherten Koordinaten verwendet.
     */
    bool setLocationByName(const char *name);

    /**
     * @brief Setzt den Standort über Koordinaten
     * @param lat Breitengrad
     * @param lon Längengrad
     * @param cityName Optionaler Stadtname für die Anzeige
     * @return true bei erfolgreicher Übernahme der Koordinaten
     */
    bool setLocation(float lat, float lon, const char *cityName = nullptr);

    /**
     * @brief Fordert eine Aktualisierung der Wetterdaten an
     * @param force true um Cooldown-Timer zu umgehen
     * @return true wenn Request tatsächlich getriggert wurde, false bei aktivem Cooldown
     *
     * Signalisiert dem Worker-Task, neue Daten abzurufen.
     */
    bool requestRefresh(bool force = false);

    /**
     * @brief Beendet den Hintergrund-Worker-Task
     * @param fast true für sofortiges Beenden ohne Aufräumen
     *
     * Wartet bei laufendem HTTP-Request auf dessen Beendigung bevor der Task gelöscht wird (außer bei fast=true).
     */
    void shutdownWorker(bool fast = false);

private:
    // ========================================================================
    // TYPEN
    // ========================================================================

    /**
     * @brief Job-Typen für den Worker-Task
     */
    enum class FetchJob
    {
        NONE,                 ///< Kein Job anstehend
        FORECAST,             ///< Nur Wetterdaten abrufen
        GEOCODE_THEN_FORECAST ///< Erst Geocoding, dann Wetterdaten
    };

    // ========================================================================
    // DOUBLE-BUFFER FUNKTIONEN
    // ========================================================================

    WeatherData *getActiveBuffer();   ///< Liefert aktiven Buffer (für UI)
    WeatherData *getInactiveBuffer(); ///< Liefert inaktiven Buffer (für Worker)
    void swapBuffers();               ///< Tauscht aktiven/inaktiven Buffer atomar

    // ========================================================================
    // ZEICHENFUNKTIONEN
    // ========================================================================

    void drawWeatherUI();                                                                   ///< Zeichnet die vollständige Oberfläche
    void drawErrorScreen(WeatherKey titleKey, WeatherKey subKey);                           ///< Zeichnet Fehlerbildschirm (subKey == WEATHER_KEY_COUNT = kein Untertitel)
    void drawWeatherIcon(int wmoCode, bool isDay, int x, int y, int width, uint16_t color); ///< Zeichnet Wetter-Symbol

    // ========================================================================
    // WORKER-TASK FUNKTIONEN
    // ========================================================================

    static void TaskEntry(void *arg);                                                                       ///< Task-Einsprungpunkt
    bool fetchWeatherDataCore(WeatherData *outData, WeatherStatus &outStatus, int &outHttpCode);            ///< Kernfunktion Wetterdaten
    bool fetchGeocodeCore(const char *name, float &outLat, float &outLon, char *outNice, size_t outNiceSz); ///< Kernfunktion Geocoding

    // ========================================================================
    // HILFSFUNKTIONEN
    // ========================================================================

    void copyPendingCity(char *dest, size_t destSize);                                     ///< Thread-sichere Kopie pendingCity
    void setPendingCity(const char *city);                                                 ///< Thread-sichere Zuweisung pendingCity
    void applyLocation(float lat, float lon, const char *cityName);                        ///< Übernimmt Standort (UI-Thread)
    void copySharedLocation(float &outLat, float &outLon, char *outCity, size_t citySize); ///< Thread-sichere Kopie Location (Worker)
    void setSharedLocation(float lat, float lon, const char *cityName);                    ///< Thread-sichere Zuweisung Location (UI)

    // ========================================================================
    // MEMBER-VARIABLEN
    // ========================================================================

    // Core-Komponenten
    LGFX *m_tft = nullptr;                  ///< Display-Objekt
    Symbols *m_symbols = nullptr;           ///< Symbol-Renderer
    Gfx_Sprite *m_progressSprite = nullptr; ///< 1px Fortschrittsbalken
    DotMatrix m_tempDotMatrix;              ///< DotMatrix-Anzeige für Haupttemperatur
    Gfx_Sprite *m_tempSprite = nullptr;     ///< Sprite für Temperatur-DotMatrix
    WeatherCallbacks m_callbacks;           ///< Callback-Funktionen
    bool m_initialized = false;             ///< Modul initialisiert?

    // UI-Buttons
    Button m_exitButton;       ///< Beenden-Button
    Button m_cancelLoadButton; ///< Abbrechen-Button (Ladescreen)

    // ========================================================================
    // DOUBLE-BUFFER FÜR WETTER-DATEN
    // ========================================================================

    WeatherData m_dataBuffers[WeatherCfg::BUFFER_COUNT]; ///< Double-Buffer Array
    std::atomic<int> m_activeBufferIndex{0};             ///< Index des aktiven Buffers (0 oder 1)

    // ========================================================================
    // UI-STATE (nur UI-Thread)
    // ========================================================================

    WeatherStatus m_displayedStatus = WeatherStatus::LOADING; ///< Aktuell angezeigte Status
    unsigned long m_lastProgressUpdate = 0;                   ///< Letztes Progressbar-Update
    unsigned long m_uiLastApiCall = 0;                        ///< UI-lokale Kopie des API-Zeitstempels

    // ========================================================================
    // THREADING & SYNCHRONISATION
    // ========================================================================

    std::atomic<TaskHandle_t> m_taskHandle{nullptr};                      ///< Handle des Worker-Tasks (atomar!)
    std::atomic<bool> m_shutdownRequested{false};                         ///< Shutdown angefordert?
    std::atomic<bool> m_refreshRequested{false};                          ///< Refresh angefordert? (UI zu Worker)
    std::atomic<bool> m_inFlight{false};                                  ///< HTTP-Request läuft?
    std::atomic<FetchJob> m_pendingJob{FetchJob::NONE};                   ///< Anstehender Job-Typ
    std::atomic<bool> m_hasFreshData{false};                              ///< Neue Daten verfügbar? (Worker zu UI)
    std::atomic<WeatherStatus> m_lastFetchStatus{WeatherStatus::LOADING}; ///< Status des letzten Fetch

    // Timing und Rate-Limiting (atomar für Thread-Safety)
    std::atomic<unsigned long> m_nextAllowedFetch{0}; ///< Frühester nächster Fetch
    std::atomic<unsigned long> m_backoffMs{0};        ///< Aktuelle Backoff-Zeit

    // Geschützter Pending-City Buffer
    char m_pendingCity[WeatherCfg::PENDING_CITY_LEN] = {0};       ///< Anstehende Stadt für Geocoding
    portMUX_TYPE m_pendingCityMux = portMUX_INITIALIZER_UNLOCKED; ///< Spinlock für m_pendingCity

    // ========================================================================
    // GESCHÜTZTE STANDORT-DATEN (UI <-> Worker)
    // ========================================================================

    /**
     * @brief Struktur für thread-sicher geteilte Standort-Daten
     * @details Nur diese Felder werden zwischen UI und Worker geteilt und benötigen Synchronisation.
     */
    struct SharedLocation
    {
        float latitude = NAN;                          ///< Breitengrad - kein Fallback, wird aus AppState geladen
        float longitude = NAN;                         ///< Längengrad - kein Fallback, wird aus AppState geladen
        char cityName[WeatherCfg::CITY_NAME_LEN] = ""; ///< Stadtname
    };

    SharedLocation m_sharedLocation;                           ///< Geteilte Standort-Daten
    portMUX_TYPE m_locationMux = portMUX_INITIALIZER_UNLOCKED; ///< Spinlock für Standort-Daten
};

#endif // WEATHER_H
