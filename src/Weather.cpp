#include "Weather.h"

#include <ArduinoJson.h>
#include <cmath>
#include <ctype.h>
#include <cstdlib>
#include <time.h>

#include "AppState.h"
#include "DS1307_RTC.h"
#include "HttpService.h"
#include "StringUtils.h"
#include "Theme.h"

// Schriftarten
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// KONSTANTEN
// ===================================================================

namespace
{

    // Buffer-Größen
    constexpr size_t URL_BUFFER_SIZE = 512;                    ///< Größe für URL-Strings
    constexpr size_t ENCODED_BUFFER_SIZE = 256;                ///< Größe für URL-kodierte Strings
    constexpr size_t RESOLVED_LOCATION_NAME_BUFFER_SIZE = 128; ///< Größe für aufgelöste Ortsnamen

    // Task Konstanten
    constexpr size_t WORKER_STACK_SIZE = 8192; ///< Stack-Größe für den Worker-Task
    constexpr UBaseType_t WORKER_PRIORITY = 1; ///< Priorität für den Worker-Task (niedrig)
    constexpr BaseType_t WORKER_CORE = 1;      ///< Core 1 verwenden (Core 0 ist für WiFi/BT)

    // Shutdown-Timeout
    constexpr int SHUTDOWN_WAIT_MS = 10000; ///< Wartezeit before Shutdown
    constexpr int SHUTDOWN_POLL_MS = 100;   ///< Prüfintervall während des Herunterfahrens

    // Standard-Texte
    constexpr const char *DEFAULT_NO_DATA = "-";

    // Timing
    constexpr unsigned long PROGRESS_UPDATE_INTERVAL_MS = 1000; ///< Intervall für Progressbar-Update
}

// ===================================================================
// HILFSFUNKTIONEN
// ===================================================================

namespace
{
    /**
     * @brief Entfernt Leerzeichen am Anfang und Ende eines Strings
     */
    void trim(char *s)
    {
        if (!s)
            return;

        // Anfang - UTF-8 sicher durch unsigned char cast
        char *p = s;
        while (*p && isspace(static_cast<unsigned char>(*p)))
            ++p;

        if (p != s)
            memmove(s, p, strlen(p) + 1);

        // Ende
        size_t len = strlen(s);
        while (len > 0 && isspace(static_cast<unsigned char>(s[len - 1])))
            s[--len] = '\0';
    }

    /**
 * @brief Vergleicht zwei ASCII-Zeichenfolgen ohne Beachtung der Groß- und Kleinschreibung.
     */
    bool iequals(const char *a, const char *b)
    {
        if (!a || !b)
            return (a == b);

        while (*a && *b)
        {
            // UTF-8 sicher durch unsigned char cast
            if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b)))
                return false;

            ++a;
            ++b;
        }

        return (*a == '\0' && *b == '\0');
    }

    constexpr const char *UPDATE_TIME_PLACEHOLDER = "--. --. --- ----";

    /**
     * @brief Formatiert einen tm-Zeitstempel im Wetter-Layout
     *
     * Format-Template ist sprachabhängig (Deutsch: "Mo. 19. Mai. 2026 - 13:01", Englisch: "Mon, 19 May 2026 - 13:01"). Wochentag- und Monatsnamen werden aus dem Common-Pool aufgelöst.
     */
    bool formatWeatherTimestamp(const struct tm &timeInfo, char *buffer, size_t bufferSize)
    {
        if (!buffer || bufferSize == 0)
            return false;

        if (timeInfo.tm_wday < 0 || timeInfo.tm_wday >= 7 ||
            timeInfo.tm_mon < 0 || timeInfo.tm_mon >= 12 ||
            timeInfo.tm_mday < 1 || timeInfo.tm_mday > 31 ||
            timeInfo.tm_hour < 0 || timeInfo.tm_hour > 23 ||
            timeInfo.tm_min < 0 || timeInfo.tm_min > 59)
        {
            buffer[0] = '\0';
            return false;
        }

        // Wochentag- und Monatsnamen (jeweils kurz) aus Common-Pool
        const auto wdKey = static_cast<CommonKey>(
            static_cast<size_t>(CommonKey::COMMON_WEEKDAY_SUN) + timeInfo.tm_wday);
        const auto monKey = static_cast<CommonKey>(
            static_cast<size_t>(CommonKey::COMMON_MONTH_JAN) + timeInfo.tm_mon);

        char wdBuf[8] = {0};
        char monBuf[8] = {0};
        strncpy_P(wdBuf, reinterpret_cast<PGM_P>(I18n::str(wdKey)), sizeof(wdBuf) - 1);
        strncpy_P(monBuf, reinterpret_cast<PGM_P>(I18n::str(monKey)), sizeof(monBuf) - 1);

        return I18n::format(buffer, bufferSize, WeatherKey::WEATHER_TIMESTAMP,
                            wdBuf, monBuf,
                            timeInfo.tm_mday,
                            timeInfo.tm_year + 1900,
                            timeInfo.tm_hour,
                            timeInfo.tm_min) > 0;
    }

    /**
     * @brief Formatiert den Wetter-Zeitstempel mit Fallback auf die I2C-RTC
     */
    void updateLastUpdateTime(char *buffer, size_t bufferSize)
    {
        if (!buffer || bufferSize == 0)
            return;

        if (APP_STATE.getNtpSynced() && DS1307_RTC::isSystemTimeValid())
        {
            const struct tm localTime = DS1307_RTC::getCurrentLocalTime();
            if (formatWeatherTimestamp(localTime, buffer, bufferSize))
                return;

            LOG_WARNING("Zeitstempel aus Systemzeit ungültig");
        }

        DateTime rtcTime;
        if (APP_STATE.getRTC().getDateTime(rtcTime))
        {
            struct tm rtcTm{};
            rtcTm.tm_year = rtcTime.year - 1900;
            rtcTm.tm_mon = rtcTime.month - 1;
            rtcTm.tm_mday = rtcTime.day;
            rtcTm.tm_hour = rtcTime.hour;
            rtcTm.tm_min = rtcTime.minute;
            rtcTm.tm_sec = rtcTime.second;
            rtcTm.tm_isdst = -1;

            if (mktime(&rtcTm) != -1 && formatWeatherTimestamp(rtcTm, buffer, bufferSize))
            {
                LOG_WARNING("Zeitstempel aus I2C-RTC (NTP nicht verfügbar)");
                return;
            }

            LOG_WARNING("Zeitstempel aus I2C-RTC ungültig");
        }

        stringutils::safeCopy(buffer, bufferSize, UPDATE_TIME_PLACEHOLDER);
        LOG_WARNING("Zeitstempel-Platzhalter (NTP und I2C-RTC nicht verfügbar)");
    }

    /**
     * @brief Prüft, ob ein Temperatur-String numerisch ist
     */
    bool isNumericTemperatureString(const char *value)
    {
        if (!value || !*value)
            return false;

        char *endPtr = nullptr;
        strtof(value, &endPtr);
        return (endPtr != value && endPtr && *endPtr == '\0');
    }
}

// ===================================================================
// WEATHERDATA IMPLEMENTIERUNG
// ===================================================================

/**
 * @brief Setzt alle Felder auf sichere Initialwerte zurück
 */
void WeatherData::reset()
{
    stringutils::safeCopy(temperature, "--");
    stringutils::safeCopy(feelsLike, "--");
    stringutils::safeCopy(windSpeed, "--");
    stringutils::safeCopy(humidity, "--");
    cityName[0] = '\0';
    lastUpdateTime[0] = '\0';
    currentWmoCode = -1;
    isDay = true;
    latitude = NAN;
    longitude = NAN;
    timestamp = 0;

    for (int i = 0; i < WeatherCfg::NUM_FORECAST_DAYS; ++i)
        forecasts[i].reset();
}

// ===================================================================
// KONSTRUKTOR / DESTRUKTOR
// ===================================================================

/**
 * @brief Konstruktor
 */
Weather::Weather()
{
    m_dataBuffers[0].reset();
    m_dataBuffers[1].reset();
    m_activeBufferIndex.store(0);
}

/**
 * @brief Destruktor
 */
Weather::~Weather()
{
    // Worker sauber beenden (mit Warten)
    shutdownWorker(false);

    delete m_symbols;
    m_symbols = nullptr;

    if (m_tft)
    {
        m_tft->safeDeleteSprite(m_progressSprite);
        m_tft->safeDeleteSprite(m_tempSprite);
    }
}

// ===================================================================
// DOUBLE-BUFFER FUNKTIONEN
// ===================================================================

/**
 * @brief Liefert Zeiger auf aktiven Buffer (für UI-Thread)
 */
WeatherData *Weather::getActiveBuffer()
{
    return &m_dataBuffers[m_activeBufferIndex.load()];
}

/**
 * @brief Liefert Zeiger auf inaktiven Buffer
 */
WeatherData *Weather::getInactiveBuffer()
{
    const int active = m_activeBufferIndex.load();
    return &m_dataBuffers[1 - active];
}

/**
 * @brief Tauscht aktiven und inaktiven Buffer atomar
 */
void Weather::swapBuffers()
{
    const int current = m_activeBufferIndex.load();
    m_activeBufferIndex.store(1 - current);
    LOG_INFO_F("Weather Buffer-Swap: %d -> %d", current, 1 - current);
}

// ===================================================================
// THREAD-SICHERE PENDING-CITY FUNKTIONEN
// ===================================================================

/**
 * @brief Thread-sichere Kopie des pending city
 */
void Weather::copyPendingCity(char *dest, size_t destSize)
{
    portENTER_CRITICAL(&m_pendingCityMux);
    stringutils::safeCopy(dest, destSize, m_pendingCity);
    portEXIT_CRITICAL(&m_pendingCityMux);
}

/**
 * @brief Thread-sichere Zuweisung des pending city
 */
void Weather::setPendingCity(const char *city)
{
    portENTER_CRITICAL(&m_pendingCityMux);
    stringutils::safeCopy(m_pendingCity, city);
    portEXIT_CRITICAL(&m_pendingCityMux);
}

// ===================================================================
// THREAD-SICHERE STANDORT-FUNKTIONEN
// ===================================================================

/**
 * @brief Kopiert Standort-Daten thread-sicher (für Worker)
 */
void Weather::copySharedLocation(float &outLat, float &outLon, char *outCity, size_t citySize)
{
    portENTER_CRITICAL(&m_locationMux);
    outLat = m_sharedLocation.latitude;
    outLon = m_sharedLocation.longitude;

    if (outCity && citySize > 0)
        stringutils::safeCopy(outCity, citySize, m_sharedLocation.cityName);

    portEXIT_CRITICAL(&m_locationMux);
}

/**
 * @brief Setzt Standort-Daten thread-sicher (für UI)
 */
void Weather::setSharedLocation(float lat, float lon, const char *cityName)
{
    portENTER_CRITICAL(&m_locationMux);
    m_sharedLocation.latitude = lat;
    m_sharedLocation.longitude = lon;

    if (cityName && *cityName)
        stringutils::safeCopy(m_sharedLocation.cityName, cityName);

    portEXIT_CRITICAL(&m_locationMux);
}

/**
 * @brief Übernimmt Standort thread-sicher
 */
void Weather::applyLocation(float lat, float lon, const char *cityName)
{
    setSharedLocation(lat, lon, cityName);
}

// ===================================================================
// ÖFFENTLICHE METHODEN
// ===================================================================

/**
 * @brief Beendet den Worker-Task
 */
void Weather::shutdownWorker(bool fast)
{
    // Doppelaufrufe verhindern
    if (m_shutdownRequested.exchange(true))
        return;

    TaskHandle_t handle = m_taskHandle.load();
    if (handle == nullptr)
    {
        // Task existiert nicht, nur Zustände zurücksetzen
        m_inFlight.store(false);
        m_refreshRequested.store(false);
        m_pendingJob.store(FetchJob::NONE);
        m_nextAllowedFetch.store(0);
        return;
    }

    if (!fast && m_inFlight.load())
    {
        // Bei aktivem HTTP-Request auf Abschluss warten
        LOG_WARNING("Shutdown während HTTP-Request - warte auf Abschluss");

        int waited = 0;
        while (m_inFlight.load() && waited < SHUTDOWN_WAIT_MS)
        {
            vTaskDelay(pdMS_TO_TICKS(SHUTDOWN_POLL_MS));
            waited += SHUTDOWN_POLL_MS;
        }

        if (m_inFlight.load())
            LOG_WARNING("Shutdown-Timeout - erzwinge Task-Beendigung");
    }

    // Handle nochmal laden (könnte sich geändert haben)
    handle = m_taskHandle.load();
    if (handle != nullptr)
    {
        LOG_WARNING("Weather Worker-Task wird beendet");
        vTaskDelete(handle);
        m_taskHandle.store(nullptr);
    }

    // Zustände zurücksetzen
    m_inFlight.store(false);
    m_refreshRequested.store(false);
    m_pendingJob.store(FetchJob::NONE);
    m_nextAllowedFetch.store(0);
}

/**
 * @brief Gibt den Modultyp zurück
 */
AppModule Weather::type() const
{
    return AppModule::WEATHER;
}

/**
 * @brief Initialisiert das Wetter-Modul
 */
bool Weather::init(LGFX &tft, const WeatherCallbacks &callbacks)
{
    m_shutdownRequested.store(false);

    m_tft = &tft;
    m_callbacks = callbacks;
    m_symbols = new Symbols(m_tft);

    if (!m_tft->safeCreateSprite(m_progressSprite, 1, m_tft->height()))
    {
        LOG_ERROR("Konnte Ladebalken-Sprite nicht erstellen!");
        delete m_symbols;
        m_symbols = nullptr;
        return false;
    }

    // DotMatrix für Haupt-Temperatur konfigurieren (ohne Schatten, quadratische Dots)
    m_tempDotMatrix.init(
        WeatherLayout::TEMP_DOT_W,       // Dot-Breite aus Layout-Konstanten
        WeatherLayout::TEMP_DOT_H,       // Dot-Höhe aus Layout-Konstanten
        WeatherLayout::TEMP_DOT_SPACE,   // Dot-Abstand aus Layout-Konstanten
        WeatherLayout::TEMP_CHAR_SPACE); // Zeichenabstand aus Layout-Konstanten

    m_tempDotMatrix.setOnColor(WeatherLayout::TEMP_DOT_ON, WeatherLayout::TEMP_DOT_ON);
    m_tempDotMatrix.setOffColor(WeatherLayout::TEMP_DOT_OFF, WeatherLayout::TEMP_DOT_OFF);
    m_tempDotMatrix.setBackgroundColor(Theme::ColorMainBackGround);
    m_tempDotMatrix.setShowOffDots(false);
    m_tempDotMatrix.setGradientEnabled(false);
    m_tempDotMatrix.setDotShape(DotMatrix::DotShape::SQUARE);
    m_tempDotMatrix.reserveText(10);

    // Sprite für DotMatrix-Rendering (Größe dynamisch aus Layout-Konstanten)
    if (!m_tft->safeCreateSprite(m_tempSprite, WeatherLayout::TEMP_DISP_W, WeatherLayout::TEMP_DISP_H))
        LOG_WARNING("Weather: Temperatur-Sprite fehlgeschlagen");

    // Exit-Button initialisieren
    m_exitButton.initButton(
        m_tft,
        WeatherLayout::EXIT_BTN_X, WeatherLayout::EXIT_BTN_Y,
        WeatherLayout::BUTTON_SIZE, WeatherLayout::BUTTON_SIZE,
        WeatherLayout::BUTTON_RADIUS,
        WeatherColors::BTN_FILL, static_cast<uint16_t>(TFT_BLACK), static_cast<uint16_t>(TFT_DARKGREY),
        "^POWER:32:D206^",
        nullptr);

    // Ladescreen anzeigen
    m_exitButton.setFont(LeagueGothicCond34);
    m_tft->clear(Theme::ColorMainBackGround);
    m_tft->loadFont(LeagueGothicCond34);
    m_tft->setTextColor(WeatherColors::TEXT_LOADING, Theme::ColorMainBackGround);
    m_tft->setTextDatum(MC_DATUM);

    const int16_t centerX = m_tft->width() / 2;
    const int16_t centerY = m_tft->height() / 2;
    m_tft->drawString(STR(WeatherKey::WEATHER_LOADING), centerX, centerY + WeatherLayout::LOADING_TEXT_OFFSET_Y);

    // Cancel-Button zentriert unter dem Text
    m_cancelLoadButton.initButton(
        m_tft,
        centerX, static_cast<int16_t>(centerY + WeatherLayout::CANCEL_BTN_OFFSET_Y),
        WeatherLayout::BUTTON_SIZE, WeatherLayout::BUTTON_SIZE,
        WeatherLayout::BUTTON_RADIUS,
        WeatherColors::BTN_FILL, static_cast<uint16_t>(TFT_BLACK), static_cast<uint16_t>(TFT_DARKGREY),
        "^DEL:32:F800^",
        nullptr);

    m_tft->unloadFont();
    m_cancelLoadButton.drawButton(false);

    // Persistierten Standort in aktiven Buffer und geteilten State laden
    WeatherData *activeData = getActiveBuffer();

    // Koordinaten laden sobald Stadtname oder gültige Koordinaten vorhanden sind
    if (APP_STATE.hasWeatherLocation() || APP_STATE.hasWeatherCoordinates())
    {
        const double lat = APP_STATE.getWeatherLatitude();
        const double lon = APP_STATE.getWeatherLongitude();

        char savedCity[64];
        APP_STATE.getWeatherLocation(savedCity, sizeof(savedCity));

        LOG_INFO_F("Persistierter Wetter-Standort: %s (%.5f, %.5f)", savedCity, lat, lon);

        if (!std::isnan(lat) && !std::isnan(lon) && (lat != 0.0 || lon != 0.0))
        {
            activeData->latitude = static_cast<float>(lat);
            activeData->longitude = static_cast<float>(lon);
            m_sharedLocation.latitude = static_cast<float>(lat);
            m_sharedLocation.longitude = static_cast<float>(lon);
        }

        if (strlen(savedCity) > 0)
        {
            stringutils::safeCopy(activeData->cityName, savedCity);
            stringutils::safeCopy(m_sharedLocation.cityName, savedCity);
        }

        LOG_INFO_F("Persistierte Standortdaten: lat=%.5f lon=%.5f", activeData->latitude, activeData->longitude);
    }

    m_uiLastApiCall = 0;

    // Worker-Task starten (Stack MUSS im normalen RAM bleiben wegen lwip/TCP)
    if (m_taskHandle.load() == nullptr)
    {
        TaskHandle_t newHandle = nullptr;
        BaseType_t result = xTaskCreatePinnedToCore(
            &Weather::TaskEntry,
            "WeatherWorker",
            WORKER_STACK_SIZE,
            this,
            WORKER_PRIORITY,
            &newHandle,
            WORKER_CORE);

        if (result == pdPASS && newHandle != nullptr)
        {
            m_taskHandle.store(newHandle);
            LOG_INFO("Weather Worker-Task gestartet");
        }
        else
        {
            LOG_ERROR("Konnte Weather Worker-Task nicht erstellen!");
            m_tft->safeDeleteSprite(m_progressSprite);
            m_tft->safeDeleteSprite(m_tempSprite);
            delete m_symbols;
            m_symbols = nullptr;
            return false;
        }
    }

    m_initialized = true;
    LOG_SUCCESS("Wetter-Modul erfolgreich initialisiert.");
    return true;
}

/**
 * @brief Setzt den Standort über einen Ortsnamen
 */
bool Weather::setLocationByName(const char *name)
{
    if (!name || !*name)
    {
        LOG_ERROR("Leerer Ortsname");
        return false;
    }

    if (!m_initialized)
    {
        LOG_ERROR("Weather nicht initialisiert");
        return false;
    }

    // Gültige Koordinaten aus AppState haben immer Vorrang. Die Geokodierung wird unabhängig vom übergebenen Namen übersprungen, damit ein abweichender Ortsname die maßgeblichen gespeicherten Koordinaten nicht überschreibt.
    if (APP_STATE.hasWeatherCoordinates())
    {
        const double lat = APP_STATE.getWeatherLatitude();
        const double lon = APP_STATE.getWeatherLongitude();

        char savedCity[64];
        APP_STATE.getWeatherLocation(savedCity, sizeof(savedCity));
        const char *displayName = (savedCity[0] != '\0') ? savedCity : name;

        applyLocation(static_cast<float>(lat), static_cast<float>(lon), displayName);

        LOG_INFO_F("Geocoding übersprungen: WebUI-Koordinaten aktiv für %s (%.5f, %.5f)", displayName, lat, lon);

        const unsigned long nextAllowed = m_nextAllowedFetch.load();
        if ((m_displayedStatus != WeatherStatus::SUCCESS) || (millis() > nextAllowed))
        {
            if (requestRefresh(true))
                m_uiLastApiCall = millis();
        }

        return true;
    }

    // Keine gespeicherten Koordinaten - Geocoding über Ortsnamen
    setPendingCity(name);
    m_pendingJob.store(FetchJob::GEOCODE_THEN_FORECAST);
    requestRefresh(true);

    LOG_INFO_F("Geocoding angefordert: '%s'", name);
    return true;
}

/**
 * @brief Setzt den Standort über Koordinaten
 */
bool Weather::setLocation(float lat, float lon, const char *cityName)
{
    if (!m_initialized)
    {
        LOG_ERROR("Weather nicht initialisiert");
        return false;
    }

    // Stadtname erstellen falls nicht angegeben
    char locationName[WeatherCfg::CITY_NAME_LEN];
    if (cityName && *cityName)
        stringutils::safeCopy(locationName, cityName);
    else
        stringutils::safeFormat(locationName, "%.2f, %.2f", lat, lon);

    // Thread-sicher in sharedLocation speichern
    applyLocation(lat, lon, locationName);

    APP_STATE.setWeatherLocation(locationName);
    APP_STATE.setWeatherLatitude(lat);
    APP_STATE.setWeatherLongitude(lon);

    LOG_INFO_F("Standort gesetzt: %s (%.5f, %.5f)", locationName, lat, lon);

    m_uiLastApiCall = 0; // Sofort neuen Abruf ermöglichen
    return true;
}

/**
 * @brief Fordert eine Aktualisierung der Wetterdaten an
 */
bool Weather::requestRefresh(bool force)
{
    if (!m_initialized)
        return false;

    if (force)
    {
        m_nextAllowedFetch.store(0);
        m_backoffMs.store(0);
    }

    const unsigned long nextAllowed = m_nextAllowedFetch.load();
    if (millis() < nextAllowed)
    {
        LOG_INFO_F("Refresh blockiert - Cooldown aktiv (noch %lu ms)", nextAllowed - millis());
        return false;
    }

    m_refreshRequested.store(true);

    TaskHandle_t handle = m_taskHandle.load();
    if (handle != nullptr)
        xTaskNotifyGive(handle);

    LOG_INFO("Wetterdaten-Aktualisierung angefordert");
    return true;
}

// ===================================================================
// ZEICHENFUNKTIONEN
// ===================================================================

/**
 * @brief Hauptzeichenfunktion
 */
void Weather::draw()
{
    if (!m_initialized)
        return;

    // Sprachwechsel: aktuell sichtbaren Pfad neu zeichnen, damit die UI sofort in der neuen Sprache erscheint und nicht erst beim nächsten Daten-Refresh.
    if (I18n::consumeLanguageChanged())
    {
        switch (m_displayedStatus)
        {
        case WeatherStatus::SUCCESS:
            // Zeitstempel im aktiven Buffer in die neue Sprache materialisieren, bevor die UI neu gezeichnet wird. Sonst bliebe der Zeitstempel bis zum nächsten Daten-Refresh in der alten Sprache.
            {
                WeatherData *active = getActiveBuffer();
                if (active)
                    updateLastUpdateTime(active->lastUpdateTime, sizeof(active->lastUpdateTime));
            }
            drawWeatherUI();
            break;
        case WeatherStatus::ERROR_WIFI:
            drawErrorScreen(WeatherKey::WEATHER_ERR_WIFI_TITLE, WeatherKey::WEATHER_ERR_WIFI_SUB);
            break;
        case WeatherStatus::ERROR_JSON:
            drawErrorScreen(WeatherKey::WEATHER_ERR_JSON_TITLE, WeatherKey::WEATHER_ERR_JSON_SUB);
            break;
        case WeatherStatus::ERROR_API:
            drawErrorScreen(WeatherKey::WEATHER_ERR_API_TITLE, WeatherKey::WEATHER_KEY_COUNT);
            break;
        case WeatherStatus::LOADING:
        default:
            // Ladescreen neu zeichnen
            m_tft->clear(Theme::ColorMainBackGround);
            m_tft->loadFont(LeagueGothicCond34);
            m_tft->setTextColor(WeatherColors::TEXT_LOADING, Theme::ColorMainBackGround);
            m_tft->setTextDatum(MC_DATUM);
            {
                const int16_t centerX = m_tft->width() / 2;
                const int16_t centerY = m_tft->height() / 2;
                m_tft->drawString(STR(WeatherKey::WEATHER_LOADING),
                                  centerX, centerY + WeatherLayout::LOADING_TEXT_OFFSET_Y);
            }
            m_tft->unloadFont();
            m_cancelLoadButton.drawButton(false);
            break;
        }
    }

    // Soll ein neuer Fetch angestoßen werden?
    const bool intervalFaellig = (m_uiLastApiCall == 0) || (m_displayedStatus == WeatherStatus::SUCCESS && millis() - m_uiLastApiCall > WeatherCfg::API_CALL_INTERVAL_MS);
    const bool canTrigger = !m_refreshRequested.load() && (m_pendingJob.load() == FetchJob::NONE) && !m_inFlight.load();

    if (intervalFaellig && canTrigger)
    {
        // Nur bei erfolgreichem Trigger den Zeitstempel aktualisieren
        if (requestRefresh(false))
            m_uiLastApiCall = millis();
    }

    // Wenn der Worker neue Daten gebracht hat, einmalig UI aktualisieren
    if (m_hasFreshData.exchange(false))
    {
        m_displayedStatus = m_lastFetchStatus.load();
        m_uiLastApiCall = millis();

        if (m_displayedStatus == WeatherStatus::SUCCESS)
        {
            drawWeatherUI();
        }
        else
        {
            switch (m_displayedStatus)
            {
            case WeatherStatus::ERROR_WIFI:
                drawErrorScreen(WeatherKey::WEATHER_ERR_WIFI_TITLE, WeatherKey::WEATHER_ERR_WIFI_SUB);
                break;
            case WeatherStatus::ERROR_JSON:
                drawErrorScreen(WeatherKey::WEATHER_ERR_JSON_TITLE, WeatherKey::WEATHER_ERR_JSON_SUB);
                break;
            default:
                drawErrorScreen(WeatherKey::WEATHER_ERR_API_TITLE, WeatherKey::WEATHER_KEY_COUNT);
                break;
            }
        }
    }

    // Progressbar nur bei erfolgreichem Status
    if (m_displayedStatus == WeatherStatus::SUCCESS &&
        millis() - m_lastProgressUpdate > PROGRESS_UPDATE_INTERVAL_MS)
    {
        m_lastProgressUpdate = millis();

        if (m_progressSprite)
        {
            const unsigned long elapsedTime = millis() - m_uiLastApiCall;
            float progress = 1.0f - (static_cast<float>(elapsedTime) / static_cast<float>(WeatherCfg::API_CALL_INTERVAL_MS));
            int barHeight = static_cast<int>(m_tft->height() * progress);
            barHeight = constrain(barHeight, 0, m_tft->height());
            const int startY = m_tft->height() - barHeight;

            m_progressSprite->drawFastVLine(0, 0, startY, Theme::ColorMainBackGround);
            m_progressSprite->drawFastVLine(0, startY, barHeight, WeatherColors::PROGRESS_BAR);
            m_progressSprite->pushSprite(m_tft->width() - 1, 0);
        }
    }

    // Touch-Handling
    int32_t tx, ty;
    const bool is_touched = m_tft->getTouch(&tx, &ty);

    // Cancel-Button im Ladescreen
    if (m_displayedStatus == WeatherStatus::LOADING)
    {
        if (m_cancelLoadButton.isPressed() && !is_touched)
        {
            m_cancelLoadButton.press(false);
            shutdownWorker(true);

            if (m_callbacks.onExitRequested)
                m_callbacks.onExitRequested();

            return;
        }

        if (is_touched && m_cancelLoadButton.contains(tx, ty))
        {
            if (!m_cancelLoadButton.isPressed())
            {
                m_cancelLoadButton.press(true);
                m_cancelLoadButton.drawButton(true);
            }
        }
        else if (m_cancelLoadButton.isPressed())
        {
            m_cancelLoadButton.press(false);
            m_cancelLoadButton.drawButton(false);
        }
    }

    // Exit-Button
    if (m_exitButton.isPressed() && !is_touched)
    {
        m_exitButton.press(false);

        if (m_callbacks.onExitRequested)
            m_callbacks.onExitRequested();
    }
    else if (is_touched)
    {
        if (m_exitButton.contains(tx, ty) && !m_exitButton.isPressed())
            m_exitButton.press(true);
    }
}

/**
 * @brief Zeichnet die vollständige Wetter-UI aus aktivem Buffer
 */
void Weather::drawWeatherUI()
{
    if (!m_tft)
        return;

    const WeatherData *data = getActiveBuffer();

    m_tft->clear(Theme::ColorMainBackGround);
    m_tft->setTextDatum(TL_DATUM);

    // Großes Wetter-Icon
    drawWeatherIcon(data->currentWmoCode, data->isDay, WeatherLayout::MAIN_ICON_X, WeatherLayout::MAIN_ICON_Y, WeatherLayout::MAIN_ICON_WIDTH, TFT_SILVER);

    // Ort
    m_tft->setFont(&fonts::Orbitron_Light_24);
    m_tft->setTextColor(TFT_SILVER, Theme::ColorMainBackGround);

    char cityAscii[128];
    stringutils::replaceUmlautsUtf8ToAscii(data->cityName, cityAscii, sizeof(cityAscii));
    m_tft->drawString(cityAscii, WeatherLayout::MARGIN_LEFT, WeatherLayout::CITY_Y);

    // Datum und Uhrzeit des letzten Updates
    m_tft->drawString(data->lastUpdateTime, WeatherLayout::MARGIN_LEFT, WeatherLayout::TIMESTAMP_Y);

    // Details
    char tempBuffer[32];
    m_tft->loadFont(ComfortaaBold18);
    m_tft->drawString(STR(weatherKeyFromWmo(data->currentWmoCode)), WeatherLayout::MARGIN_LEFT, WeatherLayout::DESCRIPTION_Y);

    STRF(tempBuffer, WeatherKey::WEATHER_FEELS_LIKE, data->feelsLike);
    m_tft->drawString(tempBuffer, WeatherLayout::MARGIN_LEFT, WeatherLayout::FEELS_LIKE_Y);

    STRF(tempBuffer, WeatherKey::WEATHER_WIND, data->windSpeed);
    m_tft->drawString(tempBuffer, WeatherLayout::MARGIN_LEFT, WeatherLayout::WIND_Y);

    STRF(tempBuffer, WeatherKey::WEATHER_HUMIDITY, data->humidity);
    m_tft->drawString(tempBuffer, WeatherLayout::MARGIN_LEFT, WeatherLayout::HUMIDITY_Y);

    m_tft->unloadFont();

    // Haupt-Temperatur als DotMatrix
    // Format: max. 5 Stellen inkl. °-Zeichen
    // -9.9 bis 99.9 mit Dezimal ("25.3°", "-9.5°")
    // <= -10 oder >= 100 ganzzahlig ("-20°", "100°")
    if (m_tempSprite)
    {
        if (isNumericTemperatureString(data->temperature))
        {
            const float temp = strtof(data->temperature, nullptr);
            if (temp <= -10.0f || temp >= 100.0f)
                stringutils::safeFormat(tempBuffer, "%d°", static_cast<int>(lroundf(temp)));
            else
                stringutils::safeFormat(tempBuffer, "%s°", data->temperature);
        }
        else
        {
            stringutils::safeFormat(tempBuffer, "%s°", DEFAULT_NO_DATA);
        }

        m_tempDotMatrix.setText(tempBuffer);
        m_tempSprite->clear(Theme::ColorMainBackGround);
        m_tempDotMatrix.draw(m_tempSprite, 0, 0);
        m_tempSprite->pushSprite(WeatherLayout::MAIN_TEMP_X, WeatherLayout::MAIN_TEMP_Y);
    }

    // Vorschau der nächsten Tage
    m_tft->loadFont(ComfortaaBold18);

    // Berechne Abstände dynamisch basierend auf Displaybreite
    const int availableWidth = m_tft->width() - WeatherLayout::FORECAST_PADDING;
    const int forecastSpacing = availableWidth / WeatherCfg::NUM_FORECAST_DAYS;

    for (int i = 0; i < WeatherCfg::NUM_FORECAST_DAYS; ++i)
    {
        // Zentriere jedes Element in seinem Abschnitt
        const int currentX = WeatherLayout::FORECAST_MARGIN_X + (i * forecastSpacing) + (forecastSpacing / 2);

        // Wochentag
        m_tft->setTextDatum(TC_DATUM);
        m_tft->setTextColor(WeatherColors::TEXT_WEEKDAY, Theme::ColorMainBackGround);
        const int8_t wdIdx = data->forecasts[i].weekdayIdx;
        if (wdIdx >= 0 && wdIdx <= 6)
        {
            const auto wdKey = static_cast<CommonKey>(static_cast<size_t>(CommonKey::COMMON_WEEKDAY_SUN) + wdIdx);
            m_tft->drawString(STR(wdKey), currentX, WeatherLayout::FORECAST_DAY_NAME_Y);
        }

        // Kleines Wetter-Icon (für Vorschau immer Tag-Icon)
        drawWeatherIcon(data->forecasts[i].wmoCode, true, currentX - WeatherLayout::FORECAST_ICON_OFFSET_X, WeatherLayout::FORECAST_ICON_Y, WeatherLayout::FORECAST_ICON_WIDTH, TFT_SILVER);

        // Temperatur (Max/Min)
        stringutils::safeFormat(tempBuffer, "%d° / %d°", data->forecasts[i].maxTemp, data->forecasts[i].minTemp);
        m_tft->setTextDatum(BC_DATUM);
        m_tft->setTextColor(TFT_SILVER, Theme::ColorMainBackGround);
        m_tft->drawString(tempBuffer, currentX, WeatherLayout::FORECAST_TEMP_Y);

        // Regenwahrscheinlichkeit
        stringutils::safeFormat(tempBuffer, "%d%%", data->forecasts[i].pop);
        m_tft->setTextColor(WeatherColors::TEXT_POP, Theme::ColorMainBackGround);
        m_tft->drawString(tempBuffer, currentX, WeatherLayout::FORECAST_POP_Y);
    }

    m_tft->setTextDatum(TL_DATUM);
    m_tft->unloadFont();
    m_exitButton.drawButton();
}

/**
 * @brief Zeichnet Fehlerbildschirm
 *
 * @param titleKey Hauptüberschrift als WeatherKey
 * @param subKey Unter-Nachricht als WeatherKey - WEATHER_KEY_COUNT bedeutet "kein Untertitel"
 */
void Weather::drawErrorScreen(WeatherKey titleKey, WeatherKey subKey)
{
    if (!m_tft)
        return;

    m_tft->clear(Theme::ColorMainBackGround);
    m_tft->setTextDatum(MC_DATUM);

    const int16_t centerX = m_tft->width() / 2;
    const int16_t centerY = m_tft->height() / 2;

    m_tft->setTextColor(TFT_ORANGE, Theme::ColorMainBackGround);
    m_tft->drawString(STR(titleKey), centerX, centerY);

    if (subKey != WeatherKey::WEATHER_KEY_COUNT)
    {
        m_tft->setTextColor(TFT_LIGHTGREY, Theme::ColorMainBackGround);
        m_tft->drawString(STR(subKey), centerX, centerY + WeatherLayout::ERROR_SUB_OFFSET_Y);
    }

    m_exitButton.drawButton();
}

// ===================================================================
// WETTER-ICON UND BESCHREIBUNGEN
// ===================================================================

/**
 * @brief Zeichnet das passende Wetter-Icon basierend auf WMO-Code
 */
void Weather::drawWeatherIcon(int wmoCode, bool isDay, int x, int y, int width, uint16_t color)
{
    if (!m_symbols)
        return;

    switch (wmoCode)
    {
    case 0: // Klarer Himmel
        isDay ? m_symbols->showSun(x, y, width, color, Theme::ColorMainBackGround)
              : m_symbols->showMoon(x, y, width, color, Theme::ColorMainBackGround);
        break;

    case 1: // Hauptsächlich klar
    case 2: // Teils bewölkt
    case 3: // Bedeckt
        isDay ? m_symbols->showSunAndCloud(x, y, width, color, Theme::ColorMainBackGround)
              : m_symbols->showMoonAndCloud(x, y, width, color, Theme::ColorMainBackGround);
        break;

    case 45: // Nebel
    case 48: // Reifnebel
        m_symbols->showCloudAndFog(x, y, width, color, Theme::ColorMainBackGround);
        break;

    case 51:
    case 53:
    case 55: // Nieselregen
    case 56:
    case 57: // Gefrierender Nieselregen
    case 61:
    case 63:
    case 65: // Regen
    case 80:
    case 81:
    case 82: // Regenschauer
        isDay ? m_symbols->showCloudAndRain(x, y, width, color, Theme::ColorMainBackGround)
              : m_symbols->showMoonAndCloudAndRain(x, y, width, color, Theme::ColorMainBackGround);
        break;

    case 66:
    case 67: // Gefrierender Regen
    case 71:
    case 73:
    case 75: // Schneefall
    case 77: // Schneekörner
    case 85:
    case 86: // Schneeschauer
        m_symbols->showCloudAndSnow(x, y, width, color, Theme::ColorMainBackGround);
        break;

    case 95: // Gewitter
    case 96:
    case 99: // Gewitter mit Hagel
        m_symbols->showCloudAndThunderstorm(x, y, width, color, Theme::ColorMainBackGround);
        break;

    default:
        m_symbols->showCloud(x, y, width, color, Theme::ColorMainBackGround);
        break;
    }
}

// ===================================================================
// WORKER-TASK
// ===================================================================

/**
 * @brief Task-Entry-Point für den Hintergrund-Worker
 */
void Weather::TaskEntry(void *arg)
{
    auto *self = static_cast<Weather *>(arg);
    if (!self)
    {
        LOG_ERROR("TaskEntry: Ungültiger Kontext-Pointer");
        vTaskDelete(nullptr);
        return;
    }

    LOG_INFO("Weather Worker-Task gestartet");

    for (;;)
    {
        // Auf Trigger warten
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Shutdown-Check
        if (self->m_shutdownRequested.load())
        {
            LOG_INFO("Weather Worker: Shutdown angefordert");
            break;
        }

        // Mehrere Trigger zusammenfassen
        self->m_refreshRequested.store(false);

        // Worker aktiv
        self->m_inFlight.store(true);

        // Was ist zu tun?
        const FetchJob job = self->m_pendingJob.exchange(FetchJob::NONE);

        // Inaktiven Buffer für neue Daten holen
        WeatherData *inactiveBuffer = self->getInactiveBuffer();
        inactiveBuffer->reset();

        // Nur Standort-Daten thread-sicher aus geteiltem State kopieren
        float lat, lon;
        char cityName[WeatherCfg::CITY_NAME_LEN];
        self->copySharedLocation(lat, lon, cityName, sizeof(cityName));

        // In inaktiven Buffer übernehmen
        inactiveBuffer->latitude = lat;
        inactiveBuffer->longitude = lon;
        stringutils::safeCopy(inactiveBuffer->cityName, cityName);

        bool success = false;
        WeatherStatus status = WeatherStatus::LOADING;
        int httpCode = 0;

        // Job ausführen
        if (job == FetchJob::GEOCODE_THEN_FORECAST)
        {
            // Geocoding zuerst
            char pendingCity[WeatherCfg::PENDING_CITY_LEN];
            self->copyPendingCity(pendingCity, sizeof(pendingCity));

            float newLat = NAN, newLon = NAN;
            char resolvedDisplayName[RESOLVED_LOCATION_NAME_BUFFER_SIZE];

            if (self->fetchGeocodeCore(pendingCity, newLat, newLon, resolvedDisplayName, sizeof(resolvedDisplayName)))
            {
                // Neue Koordinaten übernehmen
                inactiveBuffer->latitude = newLat;
                inactiveBuffer->longitude = newLon;
                stringutils::safeCopy(inactiveBuffer->cityName, resolvedDisplayName);

                // Auch in geteilten State für zukünftige Requests
                self->setSharedLocation(newLat, newLon, resolvedDisplayName);

                // Persistieren (3 separate Aufrufe)
                APP_STATE.setWeatherLocation(resolvedDisplayName);
                APP_STATE.setWeatherLatitude(newLat);
                APP_STATE.setWeatherLongitude(newLon);

                // Dann Wetterdaten abrufen
                success = self->fetchWeatherDataCore(inactiveBuffer, status, httpCode);
            }
            else
            {
                status = WeatherStatus::ERROR_API;
            }
        }
        else
        {
            // Nur Wetterdaten abrufen
            success = self->fetchWeatherDataCore(inactiveBuffer, status, httpCode);
        }

        // Ergebnis verarbeiten
        if (success)
        {
            // Buffer tauschen
            self->swapBuffers();
            self->m_backoffMs.store(0);
            self->m_nextAllowedFetch.store(millis() + WeatherCfg::API_CALL_INTERVAL_MS);
        }
        else
        {
            // Backoff erhöhen
            unsigned long backoff = self->m_backoffMs.load();
            if (backoff == 0)
                backoff = WeatherCfg::MIN_BACKOFF_MS;
            else
                backoff = min(backoff * 2, WeatherCfg::MAX_BACKOFF_MS);

            self->m_backoffMs.store(backoff);
            self->m_nextAllowedFetch.store(millis() + backoff);

            LOG_WARNING_F("Wetter-Fetch fehlgeschlagen (HTTP %d), nächster Versuch in %lu ms", httpCode, backoff);
        }

        // Status für UI setzen
        self->m_lastFetchStatus.store(status);
        self->m_hasFreshData.store(true);
        self->m_inFlight.store(false);
    }

    LOG_INFO("Weather Worker-Task beendet");
    vTaskDelete(nullptr);
}

/**
 * @brief Kernfunktion für Wetterdaten-Abruf über Open-Meteo API
 */
bool Weather::fetchWeatherDataCore(WeatherData *outData, WeatherStatus &outStatus, int &outHttpCode)
{
    outHttpCode = 0;
    outStatus = WeatherStatus::LOADING;

    if (!outData)
    {
        LOG_ERROR("fetchWeatherDataCore: outData ist nullptr");
        outStatus = WeatherStatus::ERROR_API;
        return false;
    }

    // WiFi-Check
    if (!APP_STATE.getWifiConnected())
    {
        LOG_ERROR("Kein WiFi für Wetterdaten-Abruf");
        outStatus = WeatherStatus::ERROR_WIFI;
        return false;
    }

    // HttpService-Check
    if (!HTTP_SERVICE.isInitialized())
    {
        LOG_ERROR("Weather: HttpService nicht initialisiert!");
        outStatus = WeatherStatus::ERROR_API;
        return false;
    }

    // API-URL erstellen
    char url[URL_BUFFER_SIZE];
    stringutils::safeFormat(url,
                            "https://api.open-meteo.com/v1/forecast?"
                            "latitude=%.5f&longitude=%.5f"
                            "&current=temperature_2m,relative_humidity_2m,apparent_temperature,"
                            "weather_code,wind_speed_10m,is_day"
                            "&daily=weather_code,temperature_2m_max,temperature_2m_min,"
                            "precipitation_probability_max"
                            "&timezone=Europe%%2FBerlin"
                            "&forecast_days=%d",
                            outData->latitude, outData->longitude,
                            WeatherCfg::NUM_FORECAST_DAYS);

    LOG_INFO_F("Wetter-API: %s", url);

    // HTTP-Request über zentralen Service
    HttpResponse response = HTTP_SERVICE.get(url, &m_shutdownRequested);

    if (m_shutdownRequested.load())
    {
        LOG_INFO("Wetter-Fetch abgebrochen - Shutdown");
        return false;
    }

    outHttpCode = response.httpCode;

    if (!response.isSuccess())
    {
        LOG_ERROR_F("Wetter HTTP %d", response.httpCode);
        outStatus = WeatherStatus::ERROR_API;
        return false;
    }

    if (!response.hasData())
    {
        LOG_ERROR("Wetter: Leere Antwort");
        outStatus = WeatherStatus::ERROR_API;
        return false;
    }

    // JSON-Filter für effizientes Parsen
    JsonDocument filter;
    filter["current"]["temperature_2m"] = true;
    filter["current"]["apparent_temperature"] = true;
    filter["current"]["relative_humidity_2m"] = true;
    filter["current"]["wind_speed_10m"] = true;
    filter["current"]["weather_code"] = true;
    filter["current"]["is_day"] = true;
    filter["daily"]["time"] = true;
    filter["daily"]["weather_code"] = true;
    filter["daily"]["temperature_2m_max"] = true;
    filter["daily"]["temperature_2m_min"] = true;
    filter["daily"]["precipitation_probability_max"] = true;

    JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
    const DeserializationError err = deserializeJson(doc, response.data, DeserializationOption::Filter(filter));

    if (err)
    {
        LOG_ERROR_F("Wetter JSON: %s", err.c_str());
        outStatus = WeatherStatus::ERROR_JSON;
        return false;
    }

    // Aktuelle Wetterdaten parsen
    const JsonObject cur = doc["current"];
    const float t = cur["temperature_2m"] | NAN;
    const float ft = cur["apparent_temperature"] | NAN;
    const int rh = cur["relative_humidity_2m"] | -1;
    const float ws = cur["wind_speed_10m"] | NAN;
    const int ww = cur["weather_code"] | 0;
    const int id = cur["is_day"] | 1;

    // Temperatur: Zehntelgrad für Hauptanzeige, Ganzzahl für Gefühlt
    if (!isnan(t))
        stringutils::safeFormat(outData->temperature, "%.1f", roundf(t * 10.0f) / 10.0f);
    else
        stringutils::safeCopy(outData->temperature, DEFAULT_NO_DATA);

    if (!isnan(ft))
        stringutils::safeFormat(outData->feelsLike, "%d", lroundf(ft));
    else
        stringutils::safeCopy(outData->feelsLike, DEFAULT_NO_DATA);

    if (rh >= 0)
        stringutils::safeFormat(outData->humidity, "%d", rh);
    else
        stringutils::safeCopy(outData->humidity, DEFAULT_NO_DATA);

    if (!isnan(ws))
        stringutils::safeFormat(outData->windSpeed, "%.0f", ws);
    else
        stringutils::safeCopy(outData->windSpeed, DEFAULT_NO_DATA);

    outData->currentWmoCode = ww;
    outData->isDay = (id == 1);

    // Vorhersage-Daten parsen
    const JsonObject daily = doc["daily"];
    const JsonArray times = daily["time"];
    const JsonArray wcode = daily["weather_code"];
    const JsonArray tmax = daily["temperature_2m_max"];
    const JsonArray tmin = daily["temperature_2m_min"];
    const JsonArray pop = daily["precipitation_probability_max"];

    int n = WeatherCfg::NUM_FORECAST_DAYS;
    n = min(n, static_cast<int>(times.size()));
    n = min(n, static_cast<int>(wcode.size()));
    n = min(n, static_cast<int>(tmax.size()));
    n = min(n, static_cast<int>(tmin.size()));
    n = min(n, static_cast<int>(pop.size()));

    for (int i = 0; i < n; ++i)
    {
        struct tm timeinfo{};
        const char *iso = times[i] | "1970-01-01";

        // Datum parsen
        if (!strptime(iso, "%Y-%m-%d", &timeinfo))
        {
            // Fallback: manuelles Parsen
            int Y = 1970, M = 1, D = 1;
            sscanf(iso, "%d-%d-%d", &Y, &M, &D);
            timeinfo.tm_year = Y - 1900;
            timeinfo.tm_mon = M - 1;
            timeinfo.tm_mday = D;
        }

        mktime(&timeinfo);

        // Forecast-Daten speichern mit Bounds-Check
        outData->forecasts[i].weekdayIdx = (timeinfo.tm_wday >= 0 && timeinfo.tm_wday <= 6)
                                               ? static_cast<int8_t>(timeinfo.tm_wday)
                                               : static_cast<int8_t>(-1);
        outData->forecasts[i].wmoCode = static_cast<int>(wcode[i] | 0);
        outData->forecasts[i].maxTemp = lroundf(static_cast<float>(tmax[i] | 0.0f));
        outData->forecasts[i].minTemp = lroundf(static_cast<float>(tmin[i] | 0.0f));
        outData->forecasts[i].pop = static_cast<int>(pop[i] | 0);
    }

    // Zeitstempel aktualisieren mit Systemzeit, RTC-Fallback und Platzhalter
    updateLastUpdateTime(outData->lastUpdateTime, sizeof(outData->lastUpdateTime));

    outData->timestamp = millis();
    outStatus = WeatherStatus::SUCCESS;
    return true;
}

/**
 * @brief Kernfunktion für Geocoding über Open-Meteo API
 */
bool Weather::fetchGeocodeCore(const char *name, float &outLat, float &outLon, char *outNice, size_t outNiceSz)
{
    outLat = NAN;
    outLon = NAN;

    if (outNice && outNiceSz > 0)
        outNice[0] = '\0';

    if (!name || !*name)
    {
        LOG_ERROR("Leerer Ortsname für Geocoding");
        return false;
    }

    if (!APP_STATE.getWifiConnected())
    {
        LOG_ERROR("Geocoding ohne WiFi");
        return false;
    }

    if (!HTTP_SERVICE.isInitialized())
    {
        LOG_ERROR("Weather: HttpService nicht initialisiert für Geocoding!");
        return false;
    }

    // Stadt und Ländercode extrahieren
    char city[128] = {0};
    char cc2[8] = {0};

    const char *comma = strrchr(name, ',');
    if (comma)
    {
        size_t nCity = static_cast<size_t>(comma - name);
        if (nCity >= sizeof(city))
            nCity = sizeof(city) - 1;

        memcpy(city, name, nCity);
        city[nCity] = '\0';

        stringutils::safeCopy(cc2, comma + 1);
        trim(city);
        trim(cc2);

        // Ländercode normalisieren
        if (strlen(cc2) == 2)
        {
            cc2[0] = static_cast<char>(toupper(static_cast<unsigned char>(cc2[0])));
            cc2[1] = static_cast<char>(toupper(static_cast<unsigned char>(cc2[1])));
        }
    }
    else
    {
        stringutils::safeCopy(city, name);
        trim(city);
    }

    // URL-Encoding für City-Parameter
    char encodedCity[ENCODED_BUFFER_SIZE];
    stringutils::urlEncode(city, encodedCity, sizeof(encodedCity));

    // API-URL erstellen
    char url[URL_BUFFER_SIZE];
    if (strlen(cc2) == 2)
        stringutils::safeFormat(url, "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1&language=de&format=json&country=%s", encodedCity, cc2);
    else
        stringutils::safeFormat(url, "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=1&language=de&format=json", encodedCity);

    LOG_INFO_F("Geocoding: %s", url);

    // HTTP Request über zentralen Service
    HttpResponse response = HTTP_SERVICE.get(url, &m_shutdownRequested);

    if (m_shutdownRequested.load())
    {
        LOG_INFO("Geocoding abgebrochen - Shutdown");
        return false;
    }

    if (!response.isSuccess())
    {
        LOG_ERROR_F("Geocoding HTTP %d", response.httpCode);
        return false;
    }

    if (!response.hasData())
    {
        LOG_ERROR("Geocoding: Leere Antwort");
        return false;
    }

    // JSON-Filter
    JsonDocument filter;
    filter["results"][0]["name"] = true;
    filter["results"][0]["admin1"] = true;
    filter["results"][0]["country_code"] = true;
    filter["results"][0]["latitude"] = true;
    filter["results"][0]["longitude"] = true;

    JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
    const DeserializationError err = deserializeJson(doc, response.data, DeserializationOption::Filter(filter));

    if (err)
    {
        LOG_ERROR_F("Geocoding JSON: %s", err.c_str());
        return false;
    }

    const JsonArray results = doc["results"];
    if (results.isNull() || results.size() == 0)
    {
        LOG_WARNING("Geocoding: keine Treffer");
        return false;
    }

    const JsonObject r = results[0];
    const float lat = r["latitude"] | NAN;
    const float lon = r["longitude"] | NAN;
    const char *n = r["name"] | "";
    const char *region = r["admin1"] | "";
    const char *cc = r["country_code"] | "";

    if (isnan(lat) || isnan(lon))
    {
        LOG_ERROR("Geocoding: Koordinaten fehlen");
        return false;
    }

    // Namen formatieren
    char resolvedDisplayName[RESOLVED_LOCATION_NAME_BUFFER_SIZE];

    if (region[0] && !iequals(n, region))
        stringutils::safeFormat(resolvedDisplayName, "%s, %s, %s", n, region, cc);
    else
        stringutils::safeFormat(resolvedDisplayName, "%s, %s", n, cc);

    outLat = lat;
    outLon = lon;

    if (outNice && outNiceSz > 0)
        stringutils::safeCopy(outNice, outNiceSz, resolvedDisplayName);

    LOG_SUCCESS_F("Geocoding erfolgreich: %s (%.5f, %.5f)", resolvedDisplayName, lat, lon);

    return true;
}
