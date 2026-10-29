#include "AppState.h"

#include <Wire.h>  // Für I2C-Kommunikation
#include <cmath>   // Für std::isnan
#include <cstring> // Für strcmp

#include "Logger.h"      // Für Logging
#include "StringUtils.h" // Für stringutils::safeCopy, stringutils::formatBytes, etc.
#include "I18n.h"        // Für i18n::getSupportedLanguages()

// ===================================================================
// KONSTANTEN
// ===================================================================

namespace
{
    // NVRAM-Adressen (RTC hat 56 Bytes NVRAM)
    constexpr uint8_t NVRAM_ADDR_STATION = 0;
    constexpr uint8_t NVRAM_ADDR_VOLUME = 1;
    constexpr uint8_t NVRAM_ADDR_VOLUME_BEFORE_MUTE = 2;
    constexpr uint8_t NVRAM_ADDR_BALANCE = 3;
    constexpr uint8_t NVRAM_ADDR_GAIN_LOW = 4;
    constexpr uint8_t NVRAM_ADDR_GAIN_BAND = 5;
    constexpr uint8_t NVRAM_ADDR_GAIN_HIGH = 6;
    constexpr uint8_t NVRAM_ADDR_MUTED = 7;
    constexpr uint8_t NVRAM_ADDR_FORCE_MONO = 8;
    constexpr uint8_t NVRAM_ADDR_VIS_MODE = 9;
    constexpr uint8_t NVRAM_ADDR_VU_STYLE = 10;
    constexpr uint8_t NVRAM_ADDR_SPECTRUM_STYLE = 11;
    constexpr uint8_t NVRAM_ADDR_WAVEFORM_STYLE = 12;
    constexpr uint8_t NVRAM_ADDR_GONIO_STYLE = 13;
    constexpr uint8_t NVRAM_ADDR_TODO_SHOW_COMPLETED = 14;
    constexpr uint8_t NVRAM_ADDR_EQ_MARKER = 15;
    constexpr uint8_t NVRAM_EQ_SCHEMA_VERSION = 1;
    constexpr uint8_t NVRAM_EQ_MARKER[] = {'E', NVRAM_EQ_SCHEMA_VERSION};
    constexpr uint8_t NVRAM_EQ_GAIN_COUNT = 3;
    constexpr uint8_t NVRAM_READ_SIZE = NVRAM_ADDR_EQ_MARKER + sizeof(NVRAM_EQ_MARKER);

    // System-Limits
    constexpr uint8_t MAX_STATIONS = 70;
    constexpr uint8_t MAX_VOLUME = 100;
    constexpr int8_t MIN_BALANCE = -16;
    constexpr int8_t MAX_BALANCE = 16;
    constexpr int8_t MIN_GAIN_DB = -12;
    constexpr int8_t MAX_GAIN_DB = 12;
    constexpr uint8_t DEFAULT_STEREO_WIDTH = 130;
    constexpr uint8_t MAX_STEREO_WIDTH = 200;
    constexpr uint8_t DEFAULT_LOUDNESS = 70;
    constexpr uint16_t DEFAULT_EQ_FREQ_LOW = 100;    ///< Bass-Shelf: hebt nur echten Bass, lässt Stimmen unangetastet
    constexpr uint16_t DEFAULT_EQ_FREQ_PEAK = 2000;  ///< Mitten-Glocke
    constexpr uint16_t DEFAULT_EQ_FREQ_HIGH = 10000; ///< Höhen-Shelf oberhalb der S-Laute: Brillanz ohne Schärfe
    constexpr uint8_t MAX_LOUDNESS = 100;
    constexpr uint8_t MAX_VIS_MODE = 7;
    constexpr uint8_t MAX_STYLE = 1;
    constexpr uint8_t DEFAULT_CENTERED_SPECTRUM_STYLE = 0; ///< Werksvorgabe des mittigen Spektrums: grün
    constexpr uint8_t DEFAULT_WATERFALL_STYLE = 1; ///< Werksvorgabe des Wasserfalls: mehrfarbig
    constexpr uint8_t DEFAULT_AURORA_STYLE = 0; ///< Werksvorgabe des Nordlichts: kühl
    constexpr uint8_t DEFAULT_MAGIC_EYE_STYLE = 0; ///< Werksvorgabe des magischen Auges: EM11
    constexpr uint8_t MAX_MAGIC_EYE_STYLE = 3;     ///< Höchster gültiger Stil des magischen Auges: EM84, muss zur Aufzählung MagicEyeStyle in include/RetroRadio.h passen
    constexpr uint8_t DEFAULT_VU_BALLISTICS = 1;   ///< Werksvorgabe der Zeigerträgheit: 300 ms, so verlangt es die Norm
    constexpr uint8_t MAX_VU_BALLISTICS = 2;       ///< Höchste gültige Trägheit, muss zur Aufzählung VuBallistics in include/Visualizer.h passen
    constexpr uint8_t DEFAULT_VU_SENSITIVITY = 2;  ///< Werksvorgabe des Bezugspegels: 0 VU bei -10 dBFS
    constexpr uint8_t MAX_VU_SENSITIVITY = 4;      ///< Höchste Empfindlichkeitsstufe, muss zur Tabelle VU_ALIGNMENT_DB in src/Visualizer.cpp passen
    constexpr uint8_t DEFAULT_ANALOG_VU_STYLE = 0; ///< Werksvorgabe der Farbfassung: klassisch
    constexpr uint8_t MAX_ANALOG_VU_STYLE = 1;     ///< Höchste gültige Fassung, muss zur Aufzählung AnalogVUStyle in include/Visualizer.h passen

    // Geografische Limits
    constexpr double MIN_LATITUDE = -90.0;
    constexpr double MAX_LATITUDE = 90.0;
    constexpr double MIN_LONGITUDE = -180.0;
    constexpr double MAX_LONGITUDE = 180.0;

    // Wetter-Defaults
    constexpr const char *DEFAULT_WEATHER_LOCATION = "Berlin, DE";
    constexpr double DEFAULT_WEATHER_LAT = 52.5200;
    constexpr double DEFAULT_WEATHER_LON = 13.4050;

    // WiFi
    constexpr uint8_t WIFI_PROFILE_COUNT = 3;

    // NTP Limits
    constexpr unsigned long MIN_NTP_INTERVAL_MS = 60000;       // 1 Minute
    constexpr unsigned long MAX_NTP_INTERVAL_MS = 86400000;    // 24 Stunden
    constexpr unsigned long DEFAULT_NTP_INTERVAL_MS = 3600000; // 1 Stunde

    // NTP Defaults
    constexpr const char *DEFAULT_NTP_SERVER = "pool.ntp.org";
    constexpr const char *DEFAULT_TZ_REGION = "Europe/Berlin";
    constexpr const char *DEFAULT_TZ_STR = "CET-1CEST,M3.5.0,M10.5.0/3";
    constexpr const char *DEFAULT_UI_LANGUAGE = "de";

    // Geräte-Netzwerkname Default
    constexpr const char *DEFAULT_HOSTNAME = "ESP32-RetroRadio";
    constexpr uint32_t TIME_WINDOW_MASK_24H = 0x00FFFFFFUL; ///< Alle 24 Stunden aktiv
    constexpr const char *LEGACY_LOTTO_SUPER_HIT_KEY = "lottoSuper"; ///< Alter ja/nein-Schalter der Lotto-Superzahl
    constexpr uint8_t LOTTO_SUPER_UNSET = 10; ///< Noch nicht gesetzte Lotto-Superzahl
    constexpr uint8_t FLIGHT_RADAR_DEFAULT_RADIUS_KM = 10; ///< Standard-Suchradius des Flugradars
    constexpr uint8_t FLIGHT_RADAR_RADIUS_VALUES[] = {5, 10, 20, 50}; ///< Erlaubte Suchradien des Flugradars

    /**
     * @brief Prüft, ob der angegebene Sprachcode unterstützt wird.
     */
    bool isSupportedUiLanguage(const char *language)
    {
        return language && (strcmp(language, "de") == 0 || strcmp(language, "en") == 0);
    }

    /**
     * @brief Prüft, ob der angegebene Flight-Radar-Radius zulässig ist.
     */
    bool isFlightRadarRadiusKm(uint8_t radiusKm)
    {
        for (uint8_t i = 0; i < sizeof(FLIGHT_RADAR_RADIUS_VALUES) / sizeof(FLIGHT_RADAR_RADIUS_VALUES[0]); ++i)
        {
            if (FLIGHT_RADAR_RADIUS_VALUES[i] == radiusKm)
                return true;
        }

        return false;
    }

    /**
     * @brief Lädt einen Textwert aus den Einstellungen oder verwendet den Ersatzwert.
     */
    void loadPrefText(Preferences &prefs, const char *key, char *buffer, size_t bufferSize, const char *fallback = "")
    {
        if (!buffer || bufferSize == 0)
            return;

        buffer[0] = '\0';
        if (prefs.isKey(key))
        {
            prefs.getString(key, buffer, bufferSize);
            return;
        }

        if (fallback)
            stringutils::safeCopy(buffer, bufferSize, fallback);
    }

    /**
     * @brief Lädt eine 64-Bit-Zahlenmaske aus zwei 32-Bit-Preference-Werten.
     */
    uint64_t loadPrefMask64(Preferences &prefs, const char *keyLo, const char *keyHi)
    {
        const uint32_t lo = static_cast<uint32_t>(prefs.getULong(keyLo, 0));
        const uint32_t hi = static_cast<uint32_t>(prefs.getULong(keyHi, 0));
        return (static_cast<uint64_t>(hi) << 32) | lo;
    }

    /**
     * @brief Speichert eine 64-Bit-Zahlenmaske als zwei 32-Bit-Preference-Werte.
     */
    void savePrefMask64(Preferences &prefs, const char *keyLo, const char *keyHi, uint64_t mask)
    {
        prefs.putULong(keyLo, static_cast<unsigned long>(mask & 0xFFFFFFFFULL));
        prefs.putULong(keyHi, static_cast<unsigned long>((mask >> 32) & 0xFFFFFFFFULL));
    }
}

// Preferences Keys - Wetter
const char *AppState::P_NS = "RetroRadio";
const char *AppState::P_KEY_LOC = "weatherLoc";
const char *AppState::P_KEY_LAT = "weatherLat";
const char *AppState::P_KEY_LON = "weatherLon";

// Preferences Keys - NTP
const char *AppState::P_KEY_NTP_SERVER = "ntpServer";
const char *AppState::P_KEY_TZ_REGION = "tzRegion";
const char *AppState::P_KEY_TZ_STR = "tzStr";
const char *AppState::P_KEY_NTP_INTERVAL = "ntpInterval";
const char *AppState::P_KEY_NEWS_REGION = "newsRegion";
const char *AppState::P_KEY_NEWS_RESSORT = "newsRessort";
const char *AppState::P_KEY_PLZ = "plz";

// Preferences Keys - Cloud-Anbindung
const char *AppState::P_KEY_CALENDAR_URL = "calendarUrl";
const char *AppState::P_KEY_TODO_URL = "todoUrl";
const char *AppState::P_KEY_CONTACTS_URL = "contactsUrl";
const char *AppState::P_KEY_OPENSKY_CLIENT_ID = "openSkyId";
const char *AppState::P_KEY_OPENSKY_CLIENT_SECRET = "openSkySecret";

// WiFi Preferences Keys
const char *AppState::P_KEY_WIFI_SSID[3] = {"wifi_ssid_0", "wifi_ssid_1", "wifi_ssid_2"};
const char *AppState::P_KEY_WIFI_PASS[3] = {"wifi_pass_0", "wifi_pass_1", "wifi_pass_2"};
const char *AppState::P_KEY_WIFI_ENABLED[3] = {"wifi_en_0", "wifi_en_1", "wifi_en_2"};

// Sperrbildschirm Preferences Key
const char *AppState::P_KEY_LOCK_PIN = "lockPin";

// UI Preferences Key
const char *AppState::P_KEY_UI_LANGUAGE = "uiLanguage";

// Geräte-Netzwerkname Preferences Key
const char *AppState::P_KEY_HOSTNAME = "hostname";

// Zeitfenster Preferences Keys
const char *AppState::P_KEY_TIME_WINDOW[] = {
    "tw0", "tw1", "tw2", "tw3", "tw4", "tw5", "tw6"};

// Klangregelung Preferences Keys
const char *AppState::P_KEY_EQ_FREQ_LOW = "eqFreqLow";
const char *AppState::P_KEY_EQ_FREQ_PEAK = "eqFreqPeak";
const char *AppState::P_KEY_EQ_FREQ_HIGH = "eqFreqHigh";
const char *AppState::P_KEY_STEREO_WIDTH = "stereoWidth";
const char *AppState::P_KEY_LOUDNESS = "loudness";
const char *AppState::P_KEY_SPEAKERS_ENABLED = "speakersOn";

// Anzeige Preferences Key
const char *AppState::P_KEY_MAGIC_EYE_STYLE = "magicEyeStyle";
const char *AppState::P_KEY_VU_BALLISTICS = "vuBallistics";
const char *AppState::P_KEY_VU_SENSITIVITY = "vuSensitivity";
const char *AppState::P_KEY_ANALOG_VU_STYLE = "analogVuStyle";
const char *AppState::P_KEY_CENTERED_SPECTRUM_STYLE = "centerSpecStyle";
const char *AppState::P_KEY_WATERFALL_STYLE = "waterfallStyle";
const char *AppState::P_KEY_AURORA_STYLE = "auroraStyle";

// Preferences Keys - Workshop-Lotto
const char *AppState::P_KEY_LOTTO_SYSTEM_INDEX = "lottoSys";
const char *AppState::P_KEY_LOTTO_INPUT_MODE = "lottoMode";
const char *AppState::P_KEY_LOTTO_TICKET_LO = "lottoTipLo";
const char *AppState::P_KEY_LOTTO_TICKET_HI = "lottoTipHi";
const char *AppState::P_KEY_LOTTO_DRAW_LO = "lottoDrawLo";
const char *AppState::P_KEY_LOTTO_DRAW_HI = "lottoDrawHi";
const char *AppState::P_KEY_LOTTO_TICKET_SUPER = "lottoTipSz";
const char *AppState::P_KEY_LOTTO_DRAW_SUPER = "lottoDrawSz";

// Preferences Keys - Workshop-Normal-Lotto
const char *AppState::P_KEY_LOTTO_NORMAL_INPUT_MODE = "lottoNMode";
const char *AppState::P_KEY_LOTTO_NORMAL_TICKET_LO = "lottoNTipLo";
const char *AppState::P_KEY_LOTTO_NORMAL_TICKET_HI = "lottoNTipHi";
const char *AppState::P_KEY_LOTTO_NORMAL_DRAW_LO = "lottoNDrawLo";
const char *AppState::P_KEY_LOTTO_NORMAL_DRAW_HI = "lottoNDrawHi";
const char *AppState::P_KEY_LOTTO_NORMAL_TICKET_SUPER = "lottoNTipSz";
const char *AppState::P_KEY_LOTTO_NORMAL_DRAW_SUPER = "lottoNDrawSz";

// Preferences Keys - Workshop-Flugradar
const char *AppState::P_KEY_FLIGHT_RADAR_RADIUS = "flightRadKm";

/**
 * @brief Privater Konstruktor, setzt RAM-Defaults für nicht direkt initialisierte Felder.
 */
AppState::AppState()
{
    for (uint8_t day = 0; day < TIME_WINDOW_DAY_COUNT; ++day)
        m_timeWindowMasks[day] = TIME_WINDOW_MASK_24H;
}

/**
 * @brief Zugriff auf die Singleton-Instanz
 */
AppState &AppState::instance()
{
    static AppState instance;
    return instance;
}

// ===================================================================
// LADEN AUS NVRAM
// ===================================================================

/**
 * @brief Speichert alle drei EQ-Werte und ihre Kennung im RTC-NVRAM
 */
bool AppState::saveEqToNVRAM()
{
    const uint8_t gains[NVRAM_EQ_GAIN_COUNT] = {
        static_cast<uint8_t>(m_currentGainLowPass),
        static_cast<uint8_t>(m_currentGainBandPass),
        static_cast<uint8_t>(m_currentGainHighPass)};

    if (!m_rtc.writeNVRAMBlock(NVRAM_ADDR_GAIN_LOW, gains, NVRAM_EQ_GAIN_COUNT))
        return false;

    if (!m_rtc.writeNVRAMBlock(NVRAM_ADDR_EQ_MARKER, NVRAM_EQ_MARKER, sizeof(NVRAM_EQ_MARKER)))
        return false;

    m_eqNVRAMInitialized = true;
    return true;
}

/**
 * @brief Lädt alle Einstellungen aus dem NVRAM
 */
void AppState::loadFromNVRAM()
{
    uint8_t data[NVRAM_READ_SIZE] = {};
    if (!m_rtc.readNVRAMBlock(0, data, NVRAM_READ_SIZE))
    {
        LOG_WARNING("AppState: RTC-NVRAM konnte nicht gelesen werden - verwende Standardwerte");
        return;
    }

    m_currentRetroStation = constrain(data[NVRAM_ADDR_STATION], 0, MAX_STATIONS - 1);
    m_currentVolume = constrain(data[NVRAM_ADDR_VOLUME], 0, MAX_VOLUME);
    m_volumeBeforeMute = constrain(data[NVRAM_ADDR_VOLUME_BEFORE_MUTE], 0, MAX_VOLUME);
    m_currentBalance = constrain(static_cast<int8_t>(data[NVRAM_ADDR_BALANCE]), MIN_BALANCE, MAX_BALANCE);
    m_isMuted = data[NVRAM_ADDR_MUTED] != 0;
    m_forceMono = data[NVRAM_ADDR_FORCE_MONO] != 0;

    m_eqNVRAMInitialized = data[NVRAM_ADDR_EQ_MARKER] == NVRAM_EQ_MARKER[0] &&
                           data[NVRAM_ADDR_EQ_MARKER + 1] == NVRAM_EQ_MARKER[1];
    if (m_eqNVRAMInitialized)
    {
        m_currentGainLowPass = constrain(static_cast<int8_t>(data[NVRAM_ADDR_GAIN_LOW]), MIN_GAIN_DB, MAX_GAIN_DB);
        m_currentGainBandPass = constrain(static_cast<int8_t>(data[NVRAM_ADDR_GAIN_BAND]), MIN_GAIN_DB, MAX_GAIN_DB);
        m_currentGainHighPass = constrain(static_cast<int8_t>(data[NVRAM_ADDR_GAIN_HIGH]), MIN_GAIN_DB, MAX_GAIN_DB);
    }
    else
    {
        m_currentGainLowPass = DEFAULT_GAIN_LOW_PASS_DB;
        m_currentGainBandPass = DEFAULT_GAIN_BAND_PASS_DB;
        m_currentGainHighPass = DEFAULT_GAIN_HIGH_PASS_DB;

        // Frisches NVRAM: auch Lautstärke und Balance auf die Standardwerte bringen, statt Rohdaten zu übernehmen
        setCurrentVolume(DEFAULT_VOLUME);
        setVolumeBeforeMute(DEFAULT_VOLUME);
        setCurrentBalance(0);

        if (saveEqToNVRAM())
            LOG_INFO_F("EQ-Standardwerte im RTC-NVRAM initialisiert: Tiefen=%d dB, Mitten=%d dB, Höhen=%d dB",
                       m_currentGainLowPass, m_currentGainBandPass, m_currentGainHighPass);
        else
            LOG_WARNING("AppState: EQ-Standardwerte konnten nicht im RTC-NVRAM gespeichert werden");
    }

    // Visualisierungs-Settings
    m_visMode = constrain(data[NVRAM_ADDR_VIS_MODE], 0, MAX_VIS_MODE);
    m_digitalVUStyle = constrain(data[NVRAM_ADDR_VU_STYLE], 0, MAX_STYLE);
    m_spectrumStyle = constrain(data[NVRAM_ADDR_SPECTRUM_STYLE], 0, MAX_STYLE);
    m_waveformStyle = constrain(data[NVRAM_ADDR_WAVEFORM_STYLE], 0, MAX_STYLE);
    m_gonioStyle = constrain(data[NVRAM_ADDR_GONIO_STYLE], 0, MAX_STYLE);

    // ToDo-Einstellungen
    m_toDoShowCompleted = data[NVRAM_ADDR_TODO_SHOW_COMPLETED] != 0;
}

// ===================================================================
// LADEN AUS PREFERENCES
// ===================================================================

/**
 * @brief Lädt die Wetter- und NTP-Einstellungen aus Preferences
 */
void AppState::loadFromPreferences()
{
    // Wetter-Einstellungen
    loadPrefText(m_prefs, P_KEY_LOC, m_weatherLocation, sizeof(m_weatherLocation));
    m_weatherLat = m_prefs.getDouble(P_KEY_LAT, NAN);
    m_weatherLon = m_prefs.getDouble(P_KEY_LON, NAN);

    // NTP-Einstellungen
    loadPrefText(m_prefs, P_KEY_NTP_SERVER, m_ntpServer, sizeof(m_ntpServer), DEFAULT_NTP_SERVER);
    loadPrefText(m_prefs, P_KEY_TZ_REGION, m_timezoneRegion, sizeof(m_timezoneRegion), DEFAULT_TZ_REGION);
    loadPrefText(m_prefs, P_KEY_TZ_STR, m_timezoneStr, sizeof(m_timezoneStr), DEFAULT_TZ_STR);

    m_ntpSyncIntervalMs = m_prefs.getULong(P_KEY_NTP_INTERVAL, DEFAULT_NTP_INTERVAL_MS);
    if (m_ntpSyncIntervalMs < MIN_NTP_INTERVAL_MS)
        m_ntpSyncIntervalMs = MIN_NTP_INTERVAL_MS;
    if (m_ntpSyncIntervalMs > MAX_NTP_INTERVAL_MS)
        m_ntpSyncIntervalMs = MAX_NTP_INTERVAL_MS;

    LOG_INFO_F("NTP geladen: Server=%s, TZ=%s, Intervall=%lu ms", m_ntpServer, m_timezoneRegion, m_ntpSyncIntervalMs);

    // Regionale Einstellungen
    m_newsRegion = m_prefs.getInt(P_KEY_NEWS_REGION, 4);
    if (m_newsRegion < 1 || m_newsRegion > 16)
        m_newsRegion = 4;

    loadPrefText(m_prefs, P_KEY_PLZ, m_plz, sizeof(m_plz));
    loadPrefText(m_prefs, P_KEY_NEWS_RESSORT, m_newsRessort, sizeof(m_newsRessort));

    // Cloud-Anbindung
    loadPrefText(m_prefs, P_KEY_CALENDAR_URL, m_calendarScriptUrl, sizeof(m_calendarScriptUrl));
    loadPrefText(m_prefs, P_KEY_TODO_URL, m_todoScriptUrl, sizeof(m_todoScriptUrl));
    loadPrefText(m_prefs, P_KEY_CONTACTS_URL, m_contactsScriptUrl, sizeof(m_contactsScriptUrl));
    loadPrefText(m_prefs, P_KEY_OPENSKY_CLIENT_ID, m_openSkyClientId, sizeof(m_openSkyClientId));
    loadPrefText(m_prefs, P_KEY_OPENSKY_CLIENT_SECRET, m_openSkyClientSecret, sizeof(m_openSkyClientSecret));

    LOG_INFO_F("Cloud-Anbindung geladen: Calendar=%s, ToDo=%s, Contacts=%s, OpenSky=%s",
               strlen(m_calendarScriptUrl) > 0 ? "✓" : "✗",
               strlen(m_todoScriptUrl) > 0 ? "✓" : "✗",
               strlen(m_contactsScriptUrl) > 0 ? "✓" : "✗",
               hasOpenSkyCredentials() ? "✓" : "✗");

    // Sperrbildschirm-PIN
    loadPrefText(m_prefs, P_KEY_LOCK_PIN, m_lockPin, sizeof(m_lockPin), "0000");

    // UI-Einstellungen
    loadPrefText(m_prefs, P_KEY_UI_LANGUAGE, m_uiLanguage, sizeof(m_uiLanguage), DEFAULT_UI_LANGUAGE);
    if (!isSupportedUiLanguage(m_uiLanguage))
        stringutils::safeCopy(m_uiLanguage, DEFAULT_UI_LANGUAGE);

    // Geräte-Netzwerkname
    loadPrefText(m_prefs, P_KEY_HOSTNAME, m_hostname, sizeof(m_hostname), DEFAULT_HOSTNAME);
    if (m_hostname[0] == '\0')
        stringutils::safeCopy(m_hostname, DEFAULT_HOSTNAME);
    LOG_INFO_F("Hostname geladen: %s", m_hostname);

    // Zeitfenster: pro Wochentag 24 Bit, Default alle Stunden aktiv
    for (uint8_t day = 0; day < TIME_WINDOW_DAY_COUNT; ++day)
        m_timeWindowMasks[day] = static_cast<uint32_t>(m_prefs.getULong(P_KEY_TIME_WINDOW[day], TIME_WINDOW_MASK_24H)) & TIME_WINDOW_MASK_24H;

    // Workshop-Lotto
    m_lottoSystemIndex = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_LOTTO_SYSTEM_INDEX, 0));
    if (m_lottoSystemIndex > 4)
        m_lottoSystemIndex = 0;

    m_lottoInputMode = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_LOTTO_INPUT_MODE, 0));
    if (m_lottoInputMode > 1)
        m_lottoInputMode = 0;

    m_lottoTicketMask = loadPrefMask64(m_prefs, P_KEY_LOTTO_TICKET_LO, P_KEY_LOTTO_TICKET_HI);
    m_lottoDrawMask = loadPrefMask64(m_prefs, P_KEY_LOTTO_DRAW_LO, P_KEY_LOTTO_DRAW_HI);
    m_lottoTicketSuper = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_LOTTO_TICKET_SUPER, LOTTO_SUPER_UNSET));
    if (m_lottoTicketSuper > LOTTO_SUPER_UNSET)
        m_lottoTicketSuper = LOTTO_SUPER_UNSET;
    m_lottoDrawSuper = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_LOTTO_DRAW_SUPER, LOTTO_SUPER_UNSET));
    if (m_lottoDrawSuper > LOTTO_SUPER_UNSET)
        m_lottoDrawSuper = LOTTO_SUPER_UNSET;

    // Workshop-Normal-Lotto
    m_lottoNormalInputMode = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_LOTTO_NORMAL_INPUT_MODE, 0));
    if (m_lottoNormalInputMode > 1)
        m_lottoNormalInputMode = 0;

    m_lottoNormalTicketMask = loadPrefMask64(m_prefs, P_KEY_LOTTO_NORMAL_TICKET_LO, P_KEY_LOTTO_NORMAL_TICKET_HI);
    m_lottoNormalDrawMask = loadPrefMask64(m_prefs, P_KEY_LOTTO_NORMAL_DRAW_LO, P_KEY_LOTTO_NORMAL_DRAW_HI);
    m_lottoNormalTicketSuper = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_LOTTO_NORMAL_TICKET_SUPER, LOTTO_SUPER_UNSET));
    if (m_lottoNormalTicketSuper > LOTTO_SUPER_UNSET)
        m_lottoNormalTicketSuper = LOTTO_SUPER_UNSET;
    m_lottoNormalDrawSuper = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_LOTTO_NORMAL_DRAW_SUPER, LOTTO_SUPER_UNSET));
    if (m_lottoNormalDrawSuper > LOTTO_SUPER_UNSET)
        m_lottoNormalDrawSuper = LOTTO_SUPER_UNSET;

    // Workshop-Flugradar
    m_flightRadarRadiusKm = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_FLIGHT_RADAR_RADIUS, FLIGHT_RADAR_DEFAULT_RADIUS_KM));
    if (!isFlightRadarRadiusKm(m_flightRadarRadiusKm))
        m_flightRadarRadiusKm = FLIGHT_RADAR_DEFAULT_RADIUS_KM;

    // Klangregelung: Eckfrequenzen der drei Filter
    m_eqFreqLow = static_cast<uint16_t>(m_prefs.getUShort(P_KEY_EQ_FREQ_LOW, DEFAULT_EQ_FREQ_LOW));
    m_eqFreqPeak = static_cast<uint16_t>(m_prefs.getUShort(P_KEY_EQ_FREQ_PEAK, DEFAULT_EQ_FREQ_PEAK));
    m_eqFreqHigh = static_cast<uint16_t>(m_prefs.getUShort(P_KEY_EQ_FREQ_HIGH, DEFAULT_EQ_FREQ_HIGH));
    m_stereoWidth = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_STEREO_WIDTH, DEFAULT_STEREO_WIDTH));
    if (m_stereoWidth > MAX_STEREO_WIDTH)
        m_stereoWidth = DEFAULT_STEREO_WIDTH;
    m_loudness = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_LOUDNESS, DEFAULT_LOUDNESS));
    if (m_loudness > MAX_LOUDNESS)
        m_loudness = DEFAULT_LOUDNESS;
    m_speakersEnabled = m_prefs.getBool(P_KEY_SPEAKERS_ENABLED, DEFAULT_SPEAKERS_ENABLED);

    // Anzeige: Stil des magischen Auges
    m_magicEyeStyle = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_MAGIC_EYE_STYLE, DEFAULT_MAGIC_EYE_STYLE));
    if (m_magicEyeStyle > MAX_MAGIC_EYE_STYLE)
        m_magicEyeStyle = DEFAULT_MAGIC_EYE_STYLE;

    // Anzeige: Trägheit des analogen VU-Zeigers
    m_vuBallistics = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_VU_BALLISTICS, DEFAULT_VU_BALLISTICS));
    if (m_vuBallistics > MAX_VU_BALLISTICS)
        m_vuBallistics = DEFAULT_VU_BALLISTICS;

    m_vuSensitivity = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_VU_SENSITIVITY, DEFAULT_VU_SENSITIVITY));
    if (m_vuSensitivity > MAX_VU_SENSITIVITY)
        m_vuSensitivity = DEFAULT_VU_SENSITIVITY;

    m_analogVUStyle = static_cast<uint8_t>(m_prefs.getUChar(P_KEY_ANALOG_VU_STYLE, DEFAULT_ANALOG_VU_STYLE));
    if (m_analogVUStyle > MAX_ANALOG_VU_STYLE)
        m_analogVUStyle = DEFAULT_ANALOG_VU_STYLE;

    m_centeredSpectrumStyle = m_prefs.getUChar(P_KEY_CENTERED_SPECTRUM_STYLE, DEFAULT_CENTERED_SPECTRUM_STYLE);
    if (m_centeredSpectrumStyle > MAX_STYLE)
        m_centeredSpectrumStyle = DEFAULT_CENTERED_SPECTRUM_STYLE;

    m_waterfallStyle = m_prefs.getUChar(P_KEY_WATERFALL_STYLE, DEFAULT_WATERFALL_STYLE);
    if (m_waterfallStyle > MAX_STYLE)
        m_waterfallStyle = DEFAULT_WATERFALL_STYLE;

    m_auroraStyle = m_prefs.getUChar(P_KEY_AURORA_STYLE, DEFAULT_AURORA_STYLE);
    if (m_auroraStyle > MAX_STYLE)
        m_auroraStyle = DEFAULT_AURORA_STYLE;
}

/**
 * @brief Setzt den Standard-Wetterstandort, falls noch keine Wetter-Preferences existieren
 */
void AppState::ensureWeatherDefaults()
{
    if (hasWeatherLocation() || hasWeatherCoordinates())
        return;

    setWeatherLocation(DEFAULT_WEATHER_LOCATION);
    setWeatherLatitude(DEFAULT_WEATHER_LAT);
    setWeatherLongitude(DEFAULT_WEATHER_LON);

    LOG_INFO_F("Standard-Wetterstandort gesetzt: %s (%.4f, %.4f)",
               DEFAULT_WEATHER_LOCATION,
               DEFAULT_WEATHER_LAT,
               DEFAULT_WEATHER_LON);
}

// ===================================================================
// INITIALISIERUNG
// ===================================================================

/**
 * @brief Initialisiert AppState
 */
bool AppState::begin(bool skipRtcInit, TwoWire *wire)
{
    (void)skipRtcInit;

    if (m_initialized)
        return true;

    if (!wire)
        wire = &Wire;

    // I2C im Standard-Mode halten (DS1307 ist empfindlich bei 400 kHz)
    wire->setClock(100000);
    delay(5);

    // Preferences initialisieren (WiFi-Profile & Einstellungen dürfen nicht von der RTC abhängen)
    if (!m_prefs.begin(P_NS, /*readOnly=*/false))
    {
        LOG_ERROR("AppState: Preferences konnten nicht geöffnet werden!");
        return false;
    }
    LOG_SUCCESS("AppState: Preferences bereit");

    // RTC initialisieren mit Retry. Bei Fehler läuft das System weiter, NVRAM-Zugriffe werden intern zu No-Ops.
    bool rtcOk = false;
    for (uint8_t attempt = 0; attempt < 5 && !rtcOk; ++attempt)
    {
        rtcOk = m_rtc.begin();
        if (rtcOk)
        {
            LOG_SUCCESS("AppState: RTC initialisiert");
            break;
        }

        delay(50);
    }

    if (!rtcOk)
        LOG_ERROR("AppState: RTC nicht erreichbar - starte ohne gespeicherte Einstellungen");

    // Bestehende Werte laden
    if (rtcOk)
        loadFromNVRAM();

    loadFromPreferences();
    ensureWeatherDefaults();
    loadWiFiProfiles();

    m_initialized = true;
    LOG_SUCCESS("AppState: Erfolgreich initialisiert");
    return true;
}

/**
 * @brief Speichert das Ergebnis des I2C-Startscans im RAM.
 */
void AppState::setI2cScanResult(const char *result, bool completed, bool busBlocked)
{
    stringutils::safeCopy(m_i2cScanResult, result ? result : "");
    m_i2cScanCompleted = completed;
    m_i2cScanBusBlocked = busBlocked;
}

// ===================================================================
// NVRAM SETTER
// ===================================================================

/**
 * @brief Setzt die aktuelle Station
 */
void AppState::setCurrentRetroStation(uint8_t value)
{
    m_currentRetroStation = constrain(value, 0, MAX_STATIONS - 1);
    m_rtc.writeUint8(NVRAM_ADDR_STATION, m_currentRetroStation);
}

/**
 * @brief Setzt die Lautstärke
 */
void AppState::setCurrentVolume(uint8_t value)
{
    m_currentVolume = constrain(value, 0, MAX_VOLUME);
    m_rtc.writeUint8(NVRAM_ADDR_VOLUME, m_currentVolume);
}

/**
 * @brief Setzt die Lautstärke vor Mute
 */
void AppState::setVolumeBeforeMute(uint8_t value)
{
    m_volumeBeforeMute = constrain(value, 0, MAX_VOLUME);
    m_rtc.writeUint8(NVRAM_ADDR_VOLUME_BEFORE_MUTE, m_volumeBeforeMute);
}

/**
 * @brief Setzt die Balance
 */
void AppState::setCurrentBalance(int8_t value)
{
    m_currentBalance = constrain(value, MIN_BALANCE, MAX_BALANCE);
    m_rtc.writeInt8(NVRAM_ADDR_BALANCE, m_currentBalance);
}

/**
 * @brief Setzt die Tiefen (Bass)
 */
void AppState::setCurrentGainLowPass(int8_t value)
{
    m_currentGainLowPass = constrain(value, MIN_GAIN_DB, MAX_GAIN_DB);
    if (m_eqNVRAMInitialized)
        m_rtc.writeInt8(NVRAM_ADDR_GAIN_LOW, m_currentGainLowPass);
    else
        saveEqToNVRAM();
}

/**
 * @brief Setzt die Mitten
 */
void AppState::setCurrentGainBandPass(int8_t value)
{
    m_currentGainBandPass = constrain(value, MIN_GAIN_DB, MAX_GAIN_DB);
    if (m_eqNVRAMInitialized)
        m_rtc.writeInt8(NVRAM_ADDR_GAIN_BAND, m_currentGainBandPass);
    else
        saveEqToNVRAM();
}

/**
 * @brief Setzt die Höhen
 */
void AppState::setCurrentGainHighPass(int8_t value)
{
    m_currentGainHighPass = constrain(value, MIN_GAIN_DB, MAX_GAIN_DB);
    if (m_eqNVRAMInitialized)
        m_rtc.writeInt8(NVRAM_ADDR_GAIN_HIGH, m_currentGainHighPass);
    else
        saveEqToNVRAM();
}

/**
 * @brief Setzt die Eckfrequenz des Tiefen-Filters in Hz
 */
void AppState::setEqFreqLow(uint16_t value)
{
    m_eqFreqLow = value;
    m_prefs.putUShort(P_KEY_EQ_FREQ_LOW, m_eqFreqLow);
}

/**
 * @brief Setzt die Mittenfrequenz des Mitten-Filters in Hz
 */
void AppState::setEqFreqPeak(uint16_t value)
{
    m_eqFreqPeak = value;
    m_prefs.putUShort(P_KEY_EQ_FREQ_PEAK, m_eqFreqPeak);
}

/**
 * @brief Setzt die Eckfrequenz des Höhen-Filters in Hz
 */
void AppState::setEqFreqHigh(uint16_t value)
{
    m_eqFreqHigh = value;
    m_prefs.putUShort(P_KEY_EQ_FREQ_HIGH, m_eqFreqHigh);
}

/**
 * @brief Setzt die Basisbreite in Prozent
 */
void AppState::setStereoWidth(uint8_t value)
{
    m_stereoWidth = constrain(value, 0, MAX_STEREO_WIDTH);
    m_prefs.putUChar(P_KEY_STEREO_WIDTH, m_stereoWidth);
}

/**
 * @brief Setzt die Loudness-Stärke in Prozent
 */
void AppState::setLoudness(uint8_t value)
{
    m_loudness = constrain(value, 0, MAX_LOUDNESS);
    m_prefs.putUChar(P_KEY_LOUDNESS, m_loudness);
}

/**
 * @brief Speichert die Freigabe der internen Lautsprecher.
 */
void AppState::setSpeakersEnabled(bool enabled)
{
    m_speakersEnabled = enabled;
    m_prefs.putBool(P_KEY_SPEAKERS_ENABLED, m_speakersEnabled);
}

/**
 * @brief Setzt den Stil des magischen Auges
 */
void AppState::setMagicEyeStyle(uint8_t value)
{
    m_magicEyeStyle = constrain(value, 0, MAX_MAGIC_EYE_STYLE);
    m_prefs.putUChar(P_KEY_MAGIC_EYE_STYLE, m_magicEyeStyle);
}

/**
 * @brief Setzt die Trägheit des analogen VU-Zeigers
 */
void AppState::setVuBallistics(uint8_t value)
{
    m_vuBallistics = constrain(value, 0, MAX_VU_BALLISTICS);
    m_prefs.putUChar(P_KEY_VU_BALLISTICS, m_vuBallistics);
}

/**
 * @brief Setzt die Empfindlichkeit des analogen VU-Zeigers
 */
void AppState::setVuSensitivity(uint8_t value)
{
    m_vuSensitivity = constrain(value, 0, MAX_VU_SENSITIVITY);
    m_prefs.putUChar(P_KEY_VU_SENSITIVITY, m_vuSensitivity);
}

/**
 * @brief Setzt die Farbfassung des analogen VU-Meters
 */
void AppState::setAnalogVUStyle(uint8_t value)
{
    m_analogVUStyle = constrain(value, 0, MAX_ANALOG_VU_STYLE);
    m_prefs.putUChar(P_KEY_ANALOG_VU_STYLE, m_analogVUStyle);
}

/**
 * @brief Setzt den Mute-Status
 */
void AppState::setIsMuted(bool value)
{
    m_isMuted = value;
    m_rtc.writeBool(NVRAM_ADDR_MUTED, m_isMuted);
}

/**
 * @brief Setzt den Mono-Modus
 */
void AppState::setForceMono(bool value)
{
    m_forceMono = value;
    m_rtc.writeBool(NVRAM_ADDR_FORCE_MONO, m_forceMono);
}

/**
 * @brief Setzt den Visualisierungsmodus
 */
void AppState::setVisMode(uint8_t value)
{
    m_visMode = constrain(value, 0, MAX_VIS_MODE);
    m_rtc.writeUint8(NVRAM_ADDR_VIS_MODE, m_visMode);
}

/**
 * @brief Setzt den Digital-VU Style
 */
void AppState::setDigitalVUStyle(uint8_t value)
{
    m_digitalVUStyle = constrain(value, 0, MAX_STYLE);
    m_rtc.writeUint8(NVRAM_ADDR_VU_STYLE, m_digitalVUStyle);
}

/**
 * @brief Setzt den Spectrum Style
 */
void AppState::setSpectrumStyle(uint8_t value)
{
    m_spectrumStyle = constrain(value, 0, MAX_STYLE);
    m_rtc.writeUint8(NVRAM_ADDR_SPECTRUM_STYLE, m_spectrumStyle);
}

/**
 * @brief Speichert die Farbwahl des mittigen Spektrums
 */
void AppState::setCenteredSpectrumStyle(uint8_t value)
{
    m_centeredSpectrumStyle = constrain(value, 0, MAX_STYLE);
    m_prefs.putUChar(P_KEY_CENTERED_SPECTRUM_STYLE, m_centeredSpectrumStyle);
}

/**
 * @brief Speichert die Farbwahl des Wasserfalls
 */
void AppState::setWaterfallStyle(uint8_t value)
{
    m_waterfallStyle = constrain(value, 0, MAX_STYLE);
    m_prefs.putUChar(P_KEY_WATERFALL_STYLE, m_waterfallStyle);
}

/**
 * @brief Speichert die Farbwelt des Nordlichts
 */
void AppState::setAuroraStyle(uint8_t value)
{
    m_auroraStyle = constrain(value, 0, MAX_STYLE);
    m_prefs.putUChar(P_KEY_AURORA_STYLE, m_auroraStyle);
}

/**
 * @brief Setzt den Waveform Style
 */
void AppState::setWaveformStyle(uint8_t value)
{
    m_waveformStyle = constrain(value, 0, MAX_STYLE);
    m_rtc.writeUint8(NVRAM_ADDR_WAVEFORM_STYLE, m_waveformStyle);
}

/**
 * @brief Setzt den Gonio Style
 */
void AppState::setGonioStyle(uint8_t value)
{
    m_gonioStyle = constrain(value, 0, MAX_STYLE);
    m_rtc.writeUint8(NVRAM_ADDR_GONIO_STYLE, m_gonioStyle);
}

/**
 * @brief Setzt den ToDo Show Completed Status
 */
void AppState::setToDoShowCompleted(bool value)
{
    m_toDoShowCompleted = value;
    m_rtc.writeBool(NVRAM_ADDR_TODO_SHOW_COMPLETED, m_toDoShowCompleted);
}

// ===================================================================
// PREFERENCES: WETTER
// ===================================================================

/**
 * @brief Ort setzen und speichern
 */
void AppState::setWeatherLocation(const char *location)
{
    stringutils::safeCopy(m_weatherLocation, location ? location : "");
    m_prefs.putString(P_KEY_LOC, m_weatherLocation);
}

/**
 * @brief Persistenter Ort (leer, falls nicht gesetzt)
 */
void AppState::getWeatherLocation(char *buffer, size_t bufferSize) const
{
    if (buffer && bufferSize > 0)
        stringutils::safeCopy(buffer, bufferSize, m_weatherLocation);
}

/**
 * @brief Breitengrad setzen und speichern
 */
void AppState::setWeatherLatitude(double lat)
{
    if (std::isnan(lat))
        return;

    if (lat < MIN_LATITUDE)
        lat = MIN_LATITUDE;

    if (lat > MAX_LATITUDE)
        lat = MAX_LATITUDE;

    m_weatherLat = lat;
    m_prefs.putDouble(P_KEY_LAT, m_weatherLat);
}

/**
 * @brief Längengrad setzen und speichern
 */
void AppState::setWeatherLongitude(double lon)
{
    if (std::isnan(lon))
        return;

    if (lon < MIN_LONGITUDE)
        lon = MIN_LONGITUDE;

    if (lon > MAX_LONGITUDE)
        lon = MAX_LONGITUDE;

    m_weatherLon = lon;
    m_prefs.putDouble(P_KEY_LON, m_weatherLon);
}

/**
 * @brief Löscht nur die Wetter-Preferences und setzt die In-Memory-Werte zurück
 */
void AppState::resetPreferencesWeather()
{
    m_prefs.remove(P_KEY_LOC);
    m_prefs.remove(P_KEY_LAT);
    m_prefs.remove(P_KEY_LON);

    m_weatherLocation[0] = '\0';
    m_weatherLat = NAN;
    m_weatherLon = NAN;
}

// ===================================================================
// PREFERENCES: NTP-EINSTELLUNGEN
// ===================================================================

/**
 * @brief NTP-Server setzen und speichern
 */
void AppState::setNtpServer(const char *server)
{
    stringutils::safeCopy(m_ntpServer, server ? server : DEFAULT_NTP_SERVER);
    m_prefs.putString(P_KEY_NTP_SERVER, m_ntpServer);
    LOG_INFO_F("NTP-Server gesetzt: %s", m_ntpServer);
}

/**
 * @brief Zeitzone-Region setzen und speichern
 */
void AppState::setTimezoneRegion(const char *region)
{
    stringutils::safeCopy(m_timezoneRegion, region ? region : DEFAULT_TZ_REGION);
    m_prefs.putString(P_KEY_TZ_REGION, m_timezoneRegion);
    LOG_INFO_F("Zeitzone-Region gesetzt: %s", m_timezoneRegion);
}

/**
 * @brief POSIX Zeitzone-String setzen und speichern
 */
void AppState::setTimezoneStr(const char *tzStr)
{
    stringutils::safeCopy(m_timezoneStr, tzStr ? tzStr : DEFAULT_TZ_STR);
    m_prefs.putString(P_KEY_TZ_STR, m_timezoneStr);
    LOG_INFO_F("Zeitzone-String gesetzt: %s", m_timezoneStr);
}

/**
 * @brief NTP Sync-Intervall setzen und speichern
 */
void AppState::setNtpSyncInterval(unsigned long intervalMs)
{
    // Intervall validieren
    if (intervalMs < MIN_NTP_INTERVAL_MS)
        intervalMs = MIN_NTP_INTERVAL_MS;
    if (intervalMs > MAX_NTP_INTERVAL_MS)
        intervalMs = MAX_NTP_INTERVAL_MS;

    m_ntpSyncIntervalMs = intervalMs;
    m_prefs.putULong(P_KEY_NTP_INTERVAL, m_ntpSyncIntervalMs);
    LOG_INFO_F("NTP-Intervall gesetzt: %lu ms", m_ntpSyncIntervalMs);
}

/**
 * @brief Löscht nur die NTP-Preferences und setzt die Defaultwerte
 */
void AppState::resetPreferencesNtp()
{
    m_prefs.remove(P_KEY_NTP_SERVER);
    m_prefs.remove(P_KEY_TZ_REGION);
    m_prefs.remove(P_KEY_TZ_STR);
    m_prefs.remove(P_KEY_NTP_INTERVAL);

    stringutils::safeCopy(m_ntpServer, DEFAULT_NTP_SERVER);
    stringutils::safeCopy(m_timezoneRegion, DEFAULT_TZ_REGION);
    stringutils::safeCopy(m_timezoneStr, DEFAULT_TZ_STR);
    m_ntpSyncIntervalMs = DEFAULT_NTP_INTERVAL_MS;

    LOG_INFO("NTP-Einstellungen zurückgesetzt");
}

// ===================================================================
// PREFERENCES: CLOUD-ANBINDUNG
// ===================================================================

/**
 * @brief Google Calendar Script-URL setzen und speichern
 */
void AppState::setCalendarScriptUrl(const char *url)
{
    stringutils::safeCopy(m_calendarScriptUrl, url ? url : "");
    m_prefs.putString(P_KEY_CALENDAR_URL, m_calendarScriptUrl);
    LOG_INFO_F("Calendar Script-URL gesetzt: %s", strlen(m_calendarScriptUrl) > 0 ? "✓" : "(leer)");
}

/**
 * @brief Google ToDo Script-URL setzen und speichern
 */
void AppState::setTodoScriptUrl(const char *url)
{
    stringutils::safeCopy(m_todoScriptUrl, url ? url : "");
    m_prefs.putString(P_KEY_TODO_URL, m_todoScriptUrl);
    LOG_INFO_F("ToDo Script-URL gesetzt: %s", strlen(m_todoScriptUrl) > 0 ? "✓" : "(leer)");
}

/**
 * @brief Google Contacts Script-URL setzen und speichern
 */
void AppState::setContactsScriptUrl(const char *url)
{
    stringutils::safeCopy(m_contactsScriptUrl, url ? url : "");
    m_prefs.putString(P_KEY_CONTACTS_URL, m_contactsScriptUrl);
    LOG_INFO_F("Contacts Script-URL gesetzt: %s", strlen(m_contactsScriptUrl) > 0 ? "✓" : "(leer)");
}

/**
 * @brief OpenSky Client-ID setzen und speichern
 */
void AppState::setOpenSkyClientId(const char *clientId)
{
    stringutils::safeCopy(m_openSkyClientId, clientId ? clientId : "");
    m_prefs.putString(P_KEY_OPENSKY_CLIENT_ID, m_openSkyClientId);
    LOG_INFO_F("OpenSky Client-ID gesetzt: %s", strlen(m_openSkyClientId) > 0 ? "✓" : "(leer)");
}

/**
 * @brief OpenSky Client-Secret setzen und speichern
 */
void AppState::setOpenSkyClientSecret(const char *clientSecret)
{
    stringutils::safeCopy(m_openSkyClientSecret, clientSecret ? clientSecret : "");
    m_prefs.putString(P_KEY_OPENSKY_CLIENT_SECRET, m_openSkyClientSecret);
    LOG_INFO_F("OpenSky Client-Secret gesetzt: %s", strlen(m_openSkyClientSecret) > 0 ? "✓" : "(leer)");
}

/**
 * @brief Löscht die Cloud-Preferences und setzt die Defaultwerte
 */
void AppState::resetPreferencesGoogleApi()
{
    m_prefs.remove(P_KEY_CALENDAR_URL);
    m_prefs.remove(P_KEY_TODO_URL);
    m_prefs.remove(P_KEY_CONTACTS_URL);
    m_prefs.remove(P_KEY_OPENSKY_CLIENT_ID);
    m_prefs.remove(P_KEY_OPENSKY_CLIENT_SECRET);

    m_calendarScriptUrl[0] = '\0';
    m_todoScriptUrl[0] = '\0';
    m_contactsScriptUrl[0] = '\0';
    m_openSkyClientId[0] = '\0';
    m_openSkyClientSecret[0] = '\0';

    LOG_INFO("Cloud-Anbindung zurückgesetzt");
}

// ===================================================================
// PREFERENCES: UI-EINSTELLUNGEN
// ===================================================================

/**
 * @brief UI-Sprache setzen und speichern
 */
void AppState::setUiLanguage(const char *language)
{
    if (!isSupportedUiLanguage(language))
        return;

    const bool changed = (strcmp(m_uiLanguage, language) != 0);
    stringutils::safeCopy(m_uiLanguage, language);
    m_prefs.putString(P_KEY_UI_LANGUAGE, m_uiLanguage);
    LOG_INFO_F("UI-Sprache gesetzt: %s", m_uiLanguage);

    if (changed)
        I18n::onLanguageChanged();
}

/**
 * @brief Löscht die UI-Sprach-Preference und setzt Deutsch als Standard
 */
void AppState::resetPreferencesUiLanguage()
{
    m_prefs.remove(P_KEY_UI_LANGUAGE);
    stringutils::safeCopy(m_uiLanguage, DEFAULT_UI_LANGUAGE);
    LOG_INFO("UI-Sprache zurückgesetzt");
}

// ===================================================================
// PREFERENCES: ZEITFENSTER
// ===================================================================

/**
 * @brief Prüft einen Zeitfenster-Eintrag.
 */
bool AppState::isTimeWindowEnabled(uint8_t day, uint8_t hour) const
{
    if (day >= TIME_WINDOW_DAY_COUNT || hour >= TIME_WINDOW_HOUR_COUNT)
        return false;

    return (m_timeWindowMasks[day] & (1UL << hour)) != 0;
}

/**
 * @brief Gibt die Tagesmaske eines Zeitfenster-Tages zurück.
 */
uint32_t AppState::getTimeWindowDayMask(uint8_t day) const
{
    if (day >= TIME_WINDOW_DAY_COUNT)
        return TIME_WINDOW_MASK_24H;

    return m_timeWindowMasks[day] & TIME_WINDOW_MASK_24H;
}

/**
 * @brief Setzt und speichert eine Tagesmaske des Zeitfensters.
 */
void AppState::setTimeWindowDayMask(uint8_t day, uint32_t mask)
{
    if (day >= TIME_WINDOW_DAY_COUNT)
        return;

    m_timeWindowMasks[day] = mask & TIME_WINDOW_MASK_24H;
    m_prefs.putULong(P_KEY_TIME_WINDOW[day], m_timeWindowMasks[day]);
}

/**
 * @brief Setzt das Zeitfenster auf Standard und speichert alle Tagesmasken.
 */
void AppState::resetPreferencesTimeWindow()
{
    for (uint8_t day = 0; day < TIME_WINDOW_DAY_COUNT; ++day)
        setTimeWindowDayMask(day, TIME_WINDOW_MASK_24H);

    LOG_INFO("Zeitfenster zurückgesetzt");
}

// ===================================================================
// PREFERENCES: WORKSHOP-LOTTO
// ===================================================================

/**
 * @brief Speichert den aktuellen Lotto-Systemschein.
 */
void AppState::setWorkshopLottoState(uint8_t systemIndex, uint8_t mode, uint64_t ticketMask, uint64_t drawMask, uint8_t ticketSuper, uint8_t drawSuper)
{
    if (systemIndex > 4)
        systemIndex = 0;
    if (mode > 1)
        mode = 0;
    if (ticketSuper > LOTTO_SUPER_UNSET)
        ticketSuper = LOTTO_SUPER_UNSET;
    if (drawSuper > LOTTO_SUPER_UNSET)
        drawSuper = LOTTO_SUPER_UNSET;

    m_lottoSystemIndex = systemIndex;
    m_lottoInputMode = mode;
    m_lottoTicketMask = ticketMask;
    m_lottoDrawMask = drawMask;
    m_lottoTicketSuper = ticketSuper;
    m_lottoDrawSuper = drawSuper;

    m_prefs.putUChar(P_KEY_LOTTO_SYSTEM_INDEX, m_lottoSystemIndex);
    m_prefs.putUChar(P_KEY_LOTTO_INPUT_MODE, m_lottoInputMode);
    savePrefMask64(m_prefs, P_KEY_LOTTO_TICKET_LO, P_KEY_LOTTO_TICKET_HI, m_lottoTicketMask);
    savePrefMask64(m_prefs, P_KEY_LOTTO_DRAW_LO, P_KEY_LOTTO_DRAW_HI, m_lottoDrawMask);
    m_prefs.putUChar(P_KEY_LOTTO_TICKET_SUPER, m_lottoTicketSuper);
    m_prefs.putUChar(P_KEY_LOTTO_DRAW_SUPER, m_lottoDrawSuper);
}

/**
 * @brief Gibt den persistenten Lotto-Systemschein zurück.
 */
void AppState::getWorkshopLottoState(uint8_t &systemIndex, uint8_t &mode, uint64_t &ticketMask, uint64_t &drawMask, uint8_t &ticketSuper, uint8_t &drawSuper) const
{
    systemIndex = m_lottoSystemIndex;
    mode = m_lottoInputMode;
    ticketMask = m_lottoTicketMask;
    drawMask = m_lottoDrawMask;
    ticketSuper = m_lottoTicketSuper;
    drawSuper = m_lottoDrawSuper;
}

/**
 * @brief Löscht den persistenten Lotto-Systemschein.
 */
void AppState::resetPreferencesWorkshopLotto()
{
    m_lottoSystemIndex = 0;
    m_lottoInputMode = 0;
    m_lottoTicketMask = 0;
    m_lottoDrawMask = 0;
    m_lottoTicketSuper = LOTTO_SUPER_UNSET;
    m_lottoDrawSuper = LOTTO_SUPER_UNSET;

    m_prefs.remove(P_KEY_LOTTO_SYSTEM_INDEX);
    m_prefs.remove(P_KEY_LOTTO_INPUT_MODE);
    m_prefs.remove(P_KEY_LOTTO_TICKET_LO);
    m_prefs.remove(P_KEY_LOTTO_TICKET_HI);
    m_prefs.remove(P_KEY_LOTTO_DRAW_LO);
    m_prefs.remove(P_KEY_LOTTO_DRAW_HI);
    m_prefs.remove(P_KEY_LOTTO_TICKET_SUPER);
    m_prefs.remove(P_KEY_LOTTO_DRAW_SUPER);
    m_prefs.remove(LEGACY_LOTTO_SUPER_HIT_KEY);

    LOG_INFO("Workshop-Lotto zurückgesetzt");
}

/**
 * @brief Speichert den normalen Lotto-Schein.
 */
void AppState::setWorkshopNormalLottoState(uint8_t mode, uint64_t ticketMask, uint64_t drawMask, uint8_t ticketSuper, uint8_t drawSuper)
{
    if (mode > 1)
        mode = 0;
    if (ticketSuper > LOTTO_SUPER_UNSET)
        ticketSuper = LOTTO_SUPER_UNSET;
    if (drawSuper > LOTTO_SUPER_UNSET)
        drawSuper = LOTTO_SUPER_UNSET;

    m_lottoNormalInputMode = mode;
    m_lottoNormalTicketMask = ticketMask;
    m_lottoNormalDrawMask = drawMask;
    m_lottoNormalTicketSuper = ticketSuper;
    m_lottoNormalDrawSuper = drawSuper;

    m_prefs.putUChar(P_KEY_LOTTO_NORMAL_INPUT_MODE, m_lottoNormalInputMode);
    savePrefMask64(m_prefs, P_KEY_LOTTO_NORMAL_TICKET_LO, P_KEY_LOTTO_NORMAL_TICKET_HI, m_lottoNormalTicketMask);
    savePrefMask64(m_prefs, P_KEY_LOTTO_NORMAL_DRAW_LO, P_KEY_LOTTO_NORMAL_DRAW_HI, m_lottoNormalDrawMask);
    m_prefs.putUChar(P_KEY_LOTTO_NORMAL_TICKET_SUPER, m_lottoNormalTicketSuper);
    m_prefs.putUChar(P_KEY_LOTTO_NORMAL_DRAW_SUPER, m_lottoNormalDrawSuper);
}

/**
 * @brief Gibt den persistenten normalen Lotto-Schein zurück.
 */
void AppState::getWorkshopNormalLottoState(uint8_t &mode, uint64_t &ticketMask, uint64_t &drawMask, uint8_t &ticketSuper, uint8_t &drawSuper) const
{
    mode = m_lottoNormalInputMode;
    ticketMask = m_lottoNormalTicketMask;
    drawMask = m_lottoNormalDrawMask;
    ticketSuper = m_lottoNormalTicketSuper;
    drawSuper = m_lottoNormalDrawSuper;
}

/**
 * @brief Löscht den persistenten normalen Lotto-Schein.
 */
void AppState::resetPreferencesWorkshopNormalLotto()
{
    m_lottoNormalInputMode = 0;
    m_lottoNormalTicketMask = 0;
    m_lottoNormalDrawMask = 0;
    m_lottoNormalTicketSuper = LOTTO_SUPER_UNSET;
    m_lottoNormalDrawSuper = LOTTO_SUPER_UNSET;

    m_prefs.remove(P_KEY_LOTTO_NORMAL_INPUT_MODE);
    m_prefs.remove(P_KEY_LOTTO_NORMAL_TICKET_LO);
    m_prefs.remove(P_KEY_LOTTO_NORMAL_TICKET_HI);
    m_prefs.remove(P_KEY_LOTTO_NORMAL_DRAW_LO);
    m_prefs.remove(P_KEY_LOTTO_NORMAL_DRAW_HI);
    m_prefs.remove(P_KEY_LOTTO_NORMAL_TICKET_SUPER);
    m_prefs.remove(P_KEY_LOTTO_NORMAL_DRAW_SUPER);

    LOG_INFO("Workshop-Normal-Lotto zurückgesetzt");
}

// ===================================================================
// PREFERENCES: WORKSHOP-FLUGRADAR
// ===================================================================

/**
 * @brief Speichert den Flugradar-Suchradius.
 */
void AppState::setWorkshopFlightRadarRadiusKm(uint8_t radiusKm)
{
    if (!isFlightRadarRadiusKm(radiusKm))
        radiusKm = FLIGHT_RADAR_DEFAULT_RADIUS_KM;

    m_flightRadarRadiusKm = radiusKm;
    m_prefs.putUChar(P_KEY_FLIGHT_RADAR_RADIUS, m_flightRadarRadiusKm);
}

/**
 * @brief Gibt den persistenten Flugradar-Suchradius zurück.
 */
uint8_t AppState::getWorkshopFlightRadarRadiusKm() const
{
    return m_flightRadarRadiusKm;
}

/**
 * @brief Löscht den persistenten Flugradar-Suchradius.
 */
void AppState::resetPreferencesWorkshopFlightRadar()
{
    m_flightRadarRadiusKm = FLIGHT_RADAR_DEFAULT_RADIUS_KM;
    m_prefs.remove(P_KEY_FLIGHT_RADAR_RADIUS);
    LOG_INFO("Workshop-Flugradar zurückgesetzt");
}

// ===================================================================
// PREFERENCES: GERÄTE-NETZWERKNAME
// ===================================================================

/**
 * @brief Geräte-Netzwerknamen setzen und speichern
 */
void AppState::setHostname(const char *hostname)
{
    if (hostname && hostname[0] != '\0')
        stringutils::safeCopy(m_hostname, hostname);
    else
        stringutils::safeCopy(m_hostname, DEFAULT_HOSTNAME);

    m_prefs.putString(P_KEY_HOSTNAME, m_hostname);
    LOG_INFO_F("Hostname gesetzt: %s", m_hostname);
}

// ===================================================================
// PREFERENCES: SPERRBILDSCHIRM
// ===================================================================

/**
 * @brief Lock-PIN setzen und speichern
 */
void AppState::setLockPin(const char *pin)
{
    if (!pin || strlen(pin) != 4)
        return;

    for (int i = 0; i < 4; ++i)
    {
        if (pin[i] < '0' || pin[i] > '9')
            return;
    }

    stringutils::safeCopy(m_lockPin, pin);
    m_prefs.putString(P_KEY_LOCK_PIN, m_lockPin);
    LOG_INFO("Lock-PIN aktualisiert");
}

// ===================================================================
// PREFERENCES: WIFI-PROFILE
// ===================================================================

/**
 * @brief Setzt ein WiFi-Profil
 */
void AppState::setWiFiProfile(uint8_t index, const char *ssid, const char *password, bool enabled)
{
    if (index >= WIFI_PROFILE_COUNT)
        return;

    // Sicher in char-Arrays kopieren
    memset(m_wifiProfiles[index].ssid, 0, sizeof(m_wifiProfiles[index].ssid));
    memset(m_wifiProfiles[index].password, 0, sizeof(m_wifiProfiles[index].password));

    if (ssid)
        stringutils::safeCopy(m_wifiProfiles[index].ssid, ssid);

    if (password)
        stringutils::safeCopy(m_wifiProfiles[index].password, password);

    m_wifiProfiles[index].enabled = enabled;

    if (strlen(m_wifiProfiles[index].ssid) > 0)
    {
        // Wenn eine SSID vorhanden ist, werden die Zugangsdaten immer gespeichert.
        m_prefs.putString(P_KEY_WIFI_SSID[index], m_wifiProfiles[index].ssid);
        m_prefs.putString(P_KEY_WIFI_PASS[index], m_wifiProfiles[index].password);

        // Der enabled-Status wird separat und unabhängig davon gespeichert.
        m_prefs.putBool(P_KEY_WIFI_ENABLED[index], enabled);
    }
    else
    {
        // Nur wenn die SSID leer ist, wird das gesamte Profil aus dem Speicher gelöscht.
        clearWiFiProfile(index);
    }
}

/**
 * @brief Holt ein WiFi-Profil
 */
bool AppState::getWiFiProfile(uint8_t index, char *ssid, size_t ssidSize, char *password, size_t passSize) const
{
    if (index >= WIFI_PROFILE_COUNT)
        return false;

    if (!ssid || !password)
        return false;

    // Profil existiert nur, wenn eine SSID hinterlegt ist
    if (m_wifiProfiles[index].ssid[0] == '\0')
        return false;

    stringutils::safeCopy(ssid, ssidSize, m_wifiProfiles[index].ssid);
    stringutils::safeCopy(password, passSize, m_wifiProfiles[index].password);

    return true;
}

/**
 * @brief Prüft ob ein WiFi-Profil aktiviert ist
 */
bool AppState::isWiFiProfileEnabled(uint8_t index) const
{
    if (index >= WIFI_PROFILE_COUNT)
        return false;

    return m_wifiProfiles[index].enabled;
}

/**
 * @brief Löscht ein WiFi-Profil
 */
void AppState::clearWiFiProfile(uint8_t index)
{
    if (index >= WIFI_PROFILE_COUNT)
        return;

    // Arrays leeren
    memset(m_wifiProfiles[index].ssid, 0, sizeof(m_wifiProfiles[index].ssid));
    memset(m_wifiProfiles[index].password, 0, sizeof(m_wifiProfiles[index].password));
    m_wifiProfiles[index].enabled = false;

    // Aus Preferences löschen
    m_prefs.remove(P_KEY_WIFI_SSID[index]);
    m_prefs.remove(P_KEY_WIFI_PASS[index]);
    m_prefs.remove(P_KEY_WIFI_ENABLED[index]);
}

/**
 * @brief Zählt die aktiven WiFi-Profile
 */
uint8_t AppState::getActiveWiFiProfileCount() const
{
    uint8_t count = 0;

    for (uint8_t i = 0; i < WIFI_PROFILE_COUNT; i++)
    {
        if (m_wifiProfiles[i].enabled && strlen(m_wifiProfiles[i].ssid) > 0)
            count++;
    }

    return count;
}

/**
 * @brief Lädt alle WiFi-Profile aus den Preferences
 */
void AppState::loadWiFiProfiles()
{
    for (uint8_t i = 0; i < WIFI_PROFILE_COUNT; i++)
    {
        memset(m_wifiProfiles[i].ssid, 0, sizeof(m_wifiProfiles[i].ssid));
        memset(m_wifiProfiles[i].password, 0, sizeof(m_wifiProfiles[i].password));
        m_wifiProfiles[i].enabled = false;

        if (m_prefs.isKey(P_KEY_WIFI_SSID[i]))
        {
            m_prefs.getString(P_KEY_WIFI_SSID[i], m_wifiProfiles[i].ssid, sizeof(m_wifiProfiles[i].ssid));
            m_prefs.getString(P_KEY_WIFI_PASS[i], m_wifiProfiles[i].password, sizeof(m_wifiProfiles[i].password));
            m_wifiProfiles[i].enabled = m_prefs.getBool(P_KEY_WIFI_ENABLED[i], false);
        }
    }

    LOG_INFO_F("WiFi-Profile geladen: %d aktiv", getActiveWiFiProfileCount());
}

/**
 * @brief Speichert alle WiFi-Profile in die Preferences
 */
void AppState::saveWiFiProfiles()
{
    // Persistierung zentral über setWiFiProfile halten.
    for (uint8_t i = 0; i < WIFI_PROFILE_COUNT; i++)
    {
        setWiFiProfile(i, m_wifiProfiles[i].ssid, m_wifiProfiles[i].password, m_wifiProfiles[i].enabled);
    }

    LOG_INFO("WiFi-Profile gespeichert");
}

// ===================================================================
// RESET
// ===================================================================

/**
 * @brief Setzt alle Einstellungen auf Standardwerte zurück
 */
void AppState::resetToDefaults(bool clearNVRAM, bool clearPrefs)
{
    m_currentRetroStation = 0;
    m_currentVolume = DEFAULT_VOLUME;
    m_volumeBeforeMute = DEFAULT_VOLUME;
    m_currentBalance = 0;
    m_currentGainLowPass = DEFAULT_GAIN_LOW_PASS_DB;
    m_currentGainBandPass = DEFAULT_GAIN_BAND_PASS_DB;
    m_currentGainHighPass = DEFAULT_GAIN_HIGH_PASS_DB;
    m_isMuted = false;
    m_forceMono = false;

    // Visualisierungen
    m_visMode = 0;
    m_digitalVUStyle = 0;
    m_spectrumStyle = 0;
    m_waveformStyle = 0;
    m_gonioStyle = 0;

    // Runtime-Variablen
    m_stationSetDone = false;
    m_blockGFX = false;

    // NVRAM vollständig löschen und Standardwerte hineinschreiben
    if (clearNVRAM)
    {
        m_rtc.clearNVRAM();
        m_eqNVRAMInitialized = false;
        setCurrentRetroStation(m_currentRetroStation);
        setCurrentVolume(m_currentVolume);
        setVolumeBeforeMute(m_volumeBeforeMute);
        setCurrentBalance(m_currentBalance);
        setCurrentGainLowPass(m_currentGainLowPass);
        setCurrentGainBandPass(m_currentGainBandPass);
        setCurrentGainHighPass(m_currentGainHighPass);
        setIsMuted(m_isMuted);
        setForceMono(m_forceMono);
        setVisMode(m_visMode);
        setDigitalVUStyle(m_digitalVUStyle);
        setSpectrumStyle(m_spectrumStyle);
        setWaveformStyle(m_waveformStyle);
        setGonioStyle(m_gonioStyle);
    }

    // Preferences löschen
    if (clearPrefs)
    {
        resetPreferencesWeather();
        resetPreferencesNtp();
        resetPreferencesGoogleApi();
        resetPreferencesUiLanguage();
        resetPreferencesTimeWindow();
        resetPreferencesWorkshopLotto();
        resetPreferencesWorkshopNormalLotto();
        resetPreferencesWorkshopFlightRadar();

        m_prefs.remove(P_KEY_HOSTNAME);
        stringutils::safeCopy(m_hostname, DEFAULT_HOSTNAME);

        // Gespeicherte Klangwerte und Lautsprecher-Freigabe löschen und auf Werkseinstellungen setzen.
        m_prefs.remove(P_KEY_EQ_FREQ_LOW);
        m_prefs.remove(P_KEY_EQ_FREQ_PEAK);
        m_prefs.remove(P_KEY_EQ_FREQ_HIGH);
        m_prefs.remove(P_KEY_STEREO_WIDTH);
        m_prefs.remove(P_KEY_LOUDNESS);
        m_prefs.remove(P_KEY_SPEAKERS_ENABLED);
        m_eqFreqLow = DEFAULT_EQ_FREQ_LOW;
        m_eqFreqPeak = DEFAULT_EQ_FREQ_PEAK;
        m_eqFreqHigh = DEFAULT_EQ_FREQ_HIGH;
        m_stereoWidth = DEFAULT_STEREO_WIDTH;
        m_loudness = DEFAULT_LOUDNESS;
        m_speakersEnabled = DEFAULT_SPEAKERS_ENABLED;

        // Anzeige-Panel: gespeicherten Stil des magischen Auges löschen, damit nach dem Neustart die Werksvorgabe aus loadFromPreferences greift.
        m_prefs.remove(P_KEY_MAGIC_EYE_STYLE);
        m_magicEyeStyle = DEFAULT_MAGIC_EYE_STYLE;
        m_prefs.remove(P_KEY_VU_BALLISTICS);
        m_vuBallistics = DEFAULT_VU_BALLISTICS;
        m_prefs.remove(P_KEY_VU_SENSITIVITY);
        m_vuSensitivity = DEFAULT_VU_SENSITIVITY;
        m_prefs.remove(P_KEY_ANALOG_VU_STYLE);
        m_analogVUStyle = DEFAULT_ANALOG_VU_STYLE;
        m_prefs.remove(P_KEY_CENTERED_SPECTRUM_STYLE);
        m_centeredSpectrumStyle = DEFAULT_CENTERED_SPECTRUM_STYLE;
        m_prefs.remove(P_KEY_WATERFALL_STYLE);
        m_waterfallStyle = DEFAULT_WATERFALL_STYLE;
        m_prefs.remove(P_KEY_AURORA_STYLE);
        m_auroraStyle = DEFAULT_AURORA_STYLE;
    }
}

// ===================================================================
// PREFERENCES: REGIONALE EINSTELLUNGEN
// ===================================================================

/**
 * @brief Tagesschau-Regionalfilter setzen und speichern
 */
void AppState::setNewsRegion(int region)
{
    if (region < 1 || region > 16)
        region = 4;

    m_newsRegion = region;
    m_prefs.putInt(P_KEY_NEWS_REGION, m_newsRegion);
}

/**
 * @brief Postleitzahl setzen und speichern
 */
void AppState::setPlz(const char *plz)
{
    // Nur speichern wenn 5 Ziffern
    if (plz && strlen(plz) == 5)
        stringutils::safeCopy(m_plz, plz);
    else
        m_plz[0] = '\0';

    m_newsRessort[0] = '\0';
    m_prefs.putString(P_KEY_PLZ, m_plz);
}

/**
 * @brief Tagesschau-Ressortfilter setzen und speichern
 */
void AppState::setNewsRessort(const char *ressort)
{
    // Nur erlaubte Werte akzeptieren; leerer String = kein Filter
    static const char *const VALID[] =
        {"inland", "ausland", "wirtschaft", "sport", "video", "investigativ", "wissen"};

    if (!ressort || ressort[0] == '\0')
    {
        m_newsRessort[0] = '\0';
    }
    else
    {
        bool found = false;
        for (const char *v : VALID)
        {
            if (strcmp(ressort, v) == 0)
            {
                found = true;
                break;
            }
        }

        if (found)
            stringutils::safeCopy(m_newsRessort, ressort);
        else
            return; // Unbekannter Wert - ignorieren
    }

    m_prefs.putString(P_KEY_NEWS_RESSORT, m_newsRessort);
}
