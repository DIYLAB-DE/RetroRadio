#ifndef APP_STATE_H
#define APP_STATE_H

#include <Arduino.h>
#include <cmath>         // Für std::isnan
#include <Preferences.h> // Für Flash-basierte Key-Value-Persistenz

#include "DS1307_RTC.h" // Für RTC mit NVRAM
#include "BaseModule.h" // Für BaseModule::getInstance() und Module-IDs

class TwoWire;

/**
 * @brief Singleton-Klasse zur zentralen Verwaltung des Anwendungszustands
 *
 * Überall im Code direkt verwendbar:
 * APP_STATE.setCurrentVolume(75);
 * uint8_t vol = APP_STATE.getCurrentVolume();
 */
class AppState
{
public:
    static constexpr uint8_t TIME_WINDOW_DAY_COUNT = 7;   ///< Anzahl Wochentage im Zeitfenster
    static constexpr uint8_t TIME_WINDOW_HOUR_COUNT = 24; ///< Anzahl voller Stunden im Zeitfenster
    static constexpr size_t I2C_SCAN_RESULT_CAP = 96;     ///< Puffergröße für die I2C-Startscan-Anzeige
    static constexpr int8_t DEFAULT_GAIN_LOW_PASS_DB = 10; ///< Standardverstärkung der Tiefen
    static constexpr int8_t DEFAULT_GAIN_BAND_PASS_DB = 1; ///< Standardverstärkung der Mitten
    static constexpr int8_t DEFAULT_GAIN_HIGH_PASS_DB = 10; ///< Standardverstärkung der Höhen
    static constexpr uint8_t DEFAULT_VOLUME = 30;           ///< Standard-Lautstärke
    static constexpr bool DEFAULT_SPEAKERS_ENABLED = true; ///< Lautsprecher sind ab Werk eingeschaltet

private:
    static constexpr uint8_t LOTTO_SUPER_UNSET_VALUE = 10; ///< Noch nicht gesetzte Lotto-Superzahl

    DS1307_RTC m_rtc;    ///< Eigene RTC-Instanz
    Preferences m_prefs; ///< Flash-basierte Key-Value-Persistenz (ESP32 Preferences)

    // Wetter-Einstellungen
    char m_weatherLocation[64]; ///< z.B. "Berlin"
    double m_weatherLat = NAN;  ///< Breitengrad  (-90 .. +90), NAN = nicht konfiguriert
    double m_weatherLon = NAN;  ///< Längengrad   (-180 .. +180), NAN = nicht konfiguriert

    // NTP-Einstellungen
    char m_ntpServer[64];                        ///< NTP-Server Adresse, z.B. "de.pool.ntp.org"
    char m_timezoneRegion[64];                   ///< Region/Stadt, z.B. "Europe/Berlin"
    char m_timezoneStr[64];                      ///< POSIX Zeitzone, z.B. "CET-1CEST,M3.5.0,M10.5.0/3"
    unsigned long m_ntpSyncIntervalMs = 3600000; ///< NTP Sync-Intervall in ms, Standard 1h

    // Regionale Einstellungen
    int m_newsRegion = 4;   ///< Tagesschau-Regionalfilter (1=BW ... 16=TH), 3 = Berlin
    char m_plz[8];          ///< Postleitzahl (5 Stellen)
    char m_newsRessort[16]; ///< Tagesschau-Ressortfilter, z.B. "inland" (leer = kein Filter)

    // Cloud-Anbindung
    char m_calendarScriptUrl[256]; ///< Google Apps Script URL für Kalender
    char m_todoScriptUrl[256];     ///< Google Apps Script URL für ToDo
    char m_contactsScriptUrl[256]; ///< Google Apps Script URL für Kontakte
    char m_openSkyClientId[128];   ///< OpenSky Client-ID für das Flugradar
    char m_openSkyClientSecret[192]; ///< OpenSky Client-Secret für das Flugradar

    // Visualisierungen und Allgemeines
    uint8_t m_currentRetroStation = 0; ///< Aktuelle Station (0-69)
    uint8_t m_currentVolume = DEFAULT_VOLUME;    ///< Lautstärke (0-100)
    uint8_t m_volumeBeforeMute = DEFAULT_VOLUME; ///< Lautstärke vor Mute (0-100)
    int8_t m_currentBalance = 0;       ///< Balance (-16 bis +16)
    int8_t m_currentGainLowPass = DEFAULT_GAIN_LOW_PASS_DB;   ///< Tiefen (-12 bis +12 dB)
    int8_t m_currentGainBandPass = DEFAULT_GAIN_BAND_PASS_DB; ///< Mitten (-12 bis +12 dB)
    int8_t m_currentGainHighPass = DEFAULT_GAIN_HIGH_PASS_DB; ///< Höhen (-12 bis +12 dB)
    uint16_t m_eqFreqLow = 500;        ///< Eckfrequenz Tiefen-Filter in Hz
    uint16_t m_eqFreqPeak = 2000;      ///< Mittenfrequenz Mitten-Filter in Hz
    uint16_t m_eqFreqHigh = 7000;      ///< Eckfrequenz Höhen-Filter in Hz
    uint8_t m_stereoWidth = 100;       ///< Basisbreite in Prozent, 100 bedeutet unverändert
    uint8_t m_loudness = 0;            ///< Loudness-Stärke in Prozent
    bool m_speakersEnabled = DEFAULT_SPEAKERS_ENABLED; ///< Gespeicherte Freigabe der internen Lautsprecher
    bool m_isMuted = false;            ///< Stumm-Schaltung
    bool m_forceMono = false;          ///< Mono-/Stereo-Flag
    bool m_eqNVRAMInitialized = false; ///< true, wenn die EQ-Werte im RTC-NVRAM gekennzeichnet sind

    uint8_t m_visMode = 0;           ///< 0..7 (VisMode)
    uint8_t m_digitalVUStyle = 0;    ///< 0..1
    uint8_t m_spectrumStyle = 0;     ///< 0..1
    uint8_t m_centeredSpectrumStyle = 0; ///< Mittiges Spektrum: 0 = grün, 1 = Regenbogen
    uint8_t m_waterfallStyle = 1; ///< Wasserfall: 0 = grün, 1 = mehrfarbig
    uint8_t m_auroraStyle = 0; ///< Nordlicht: 0 = kühl, 1 = warm
    uint8_t m_waveformStyle = 0;     ///< 0..1
    uint8_t m_gonioStyle = 0;        ///< 0..1
    uint8_t m_magicEyeStyle = 0;     ///< 0 = EM11, 1 = EM34, 2 = EM80, 3 = EM84
    uint8_t m_vuBallistics = 1;      ///< Trägheit des analogen VU-Zeigers: 0 = 150 ms, 1 = 300 ms, 2 = 600 ms
    uint8_t m_vuSensitivity = 2;     ///< Bezugspegel des analogen VU: 0 = -2 dBFS bis 4 = -18 dBFS
    uint8_t m_analogVUStyle = 0;     ///< Farbfassung des analogen VU: 0 = klassisch, 1 = grün
    bool m_toDoShowCompleted = true; ///< Zeige erledigte Aufgaben

    // Sperrbildschirm
    char m_lockPin[5] = "0000";            ///< 4-stellige PIN für den Sperrbildschirm (Standard: 0000)
    bool m_lockRequested = false;          ///< Flag: Sperrbildschirm angefordert (von beliebigem Modul setzbar)
    bool m_displayUnlockRequested = false; ///< Flag: Sperrbildschirm soll extern entsperrt werden (z. B. via Webinterface)

    // UI-Einstellungen
    char m_uiLanguage[8] = "de"; ///< UI-Sprache, z.B. "de" oder "en"

    // Zeitfenster
    uint32_t m_timeWindowMasks[TIME_WINDOW_DAY_COUNT] = {}; ///< Tagesmasken, Bit 0..23 steht für Stunde 0..23

    // Workshop-Lotto
    uint8_t m_lottoSystemIndex = 0; ///< Vollsystem-Index 0..4
    uint8_t m_lottoInputMode = 0;   ///< Eingabemodus 0=Tipp, 1=Ziehung
    uint64_t m_lottoTicketMask = 0; ///< Tippzahlen, Bit n steht für Zahl n
    uint64_t m_lottoDrawMask = 0;   ///< Gewinnzahlen, Bit n steht für Zahl n
    uint8_t m_lottoTicketSuper = LOTTO_SUPER_UNSET_VALUE; ///< Tipp-Superzahl 0..9, 10=offen
    uint8_t m_lottoDrawSuper = LOTTO_SUPER_UNSET_VALUE;   ///< Ziehungs-Superzahl 0..9, 10=offen

    // Workshop-Normal-Lotto
    uint8_t m_lottoNormalInputMode = 0;   ///< Eingabemodus 0=Tipp, 1=Ziehung
    uint64_t m_lottoNormalTicketMask = 0; ///< Tippzahlen, Bit n steht für Zahl n
    uint64_t m_lottoNormalDrawMask = 0;   ///< Gewinnzahlen, Bit n steht für Zahl n
    uint8_t m_lottoNormalTicketSuper = LOTTO_SUPER_UNSET_VALUE; ///< Tipp-Superzahl 0..9, 10=offen
    uint8_t m_lottoNormalDrawSuper = LOTTO_SUPER_UNSET_VALUE;   ///< Ziehungs-Superzahl 0..9, 10=offen

    // Workshop-Flugradar
    uint8_t m_flightRadarRadiusKm = 10; ///< Suchradius in Kilometern

    // Geräte-Netzwerkname
    char m_hostname[33]; ///< Netzwerkname des Geräts (WiFi-Hostname und mDNS), max. 32 Zeichen + Nullbyte

    // I2C-Startscan
    char m_i2cScanResult[I2C_SCAN_RESULT_CAP] = ""; ///< Beim Start gefundene I2C-Adressen als kommagetrennte Liste
    bool m_i2cScanCompleted = false;     ///< true wenn der Startscan ausgeführt wurde
    bool m_i2cScanBusBlocked = false;    ///< true wenn SDA oder SCL beim Startscan dauerhaft LOW war

    // Laufzeit-Variablen im RAM
    bool m_sleepModeTriggered = false;          ///< Flag: Sleep-Modus ausgelöst
    bool m_displayOn = true;                    ///< Flag: Display ein/aus
    bool m_stationSetDone = false;              ///< Flag: Station-Setup abgeschlossen
    bool m_blockGFX = false;                    ///< Flag: Grafik-Updates blockieren
    bool m_wifiConnected = false;               ///< Flag: WiFi verbunden
    bool m_initialized = false;                 ///< Flag: Initialisierung erfolgt
    unsigned long m_lastNtpSyncTime = 0;        ///< Zeitstempel des letzten NTP-Syncs
    bool m_dstActive = false;                   ///< Flag: Sommerzeit
    uint8_t m_wifiSignalStrength = 0;           ///< WiFi-Signalstärke 0 bis 10
    uint8_t m_wifiSignalPercent = 0;            ///< WiFi-Signalstärke 0..100 (%)
    AppModule m_activeModule = AppModule::NONE; ///< Aktuell aktives Modul

    // Preferences-Namespaces und -Keys
    static const char *P_NS;      ///< "RetroRadio"
    static const char *P_KEY_LOC; ///< "weatherLoc"
    static const char *P_KEY_LAT; ///< "weatherLat"
    static const char *P_KEY_LON; ///< "weatherLon"

    // NTP Preferences Keys
    static const char *P_KEY_NTP_SERVER;   ///< "ntpServer"
    static const char *P_KEY_TZ_REGION;    ///< "tzRegion"
    static const char *P_KEY_TZ_STR;       ///< "tzStr"
    static const char *P_KEY_NTP_INTERVAL; ///< "ntpInterval"
    static const char *P_KEY_NEWS_REGION;  ///< "newsRegion"
    static const char *P_KEY_PLZ;          ///< "plz"
    static const char *P_KEY_NEWS_RESSORT; ///< "newsRessort"

    // Cloud-Anbindung Preferences Keys
    static const char *P_KEY_CALENDAR_URL; ///< "calendarUrl"
    static const char *P_KEY_TODO_URL;     ///< "todoUrl"
    static const char *P_KEY_CONTACTS_URL; ///< "contactsUrl"
    static const char *P_KEY_OPENSKY_CLIENT_ID; ///< "openSkyId"
    static const char *P_KEY_OPENSKY_CLIENT_SECRET; ///< "openSkySecret"

    // Sperrbildschirm Preferences Key
    static const char *P_KEY_LOCK_PIN; ///< "lockPin"

    // UI Preferences Key
    static const char *P_KEY_UI_LANGUAGE; ///< "uiLanguage"

    // Geräte-Netzwerkname Preferences Key
    static const char *P_KEY_HOSTNAME; ///< "hostname"

    // Zeitfenster Preferences Keys
    static const char *P_KEY_TIME_WINDOW[TIME_WINDOW_DAY_COUNT]; ///< Tagesmasken für das Zeitfenster

    // Klangregelung Preferences Keys
    static const char *P_KEY_EQ_FREQ_LOW;  ///< "eqFreqLow"
    static const char *P_KEY_EQ_FREQ_PEAK; ///< "eqFreqPeak"
    static const char *P_KEY_EQ_FREQ_HIGH; ///< "eqFreqHigh"
    static const char *P_KEY_STEREO_WIDTH; ///< "stereoWidth"
    static const char *P_KEY_LOUDNESS;     ///< "loudness"
    static const char *P_KEY_SPEAKERS_ENABLED; ///< "speakersOn"

    // Anzeige Preferences Key
    static const char *P_KEY_MAGIC_EYE_STYLE; ///< "magicEyeStyle"
    static const char *P_KEY_VU_BALLISTICS;   ///< "vuBallistics"
    static const char *P_KEY_VU_SENSITIVITY;  ///< "vuSensitivity"
    static const char *P_KEY_ANALOG_VU_STYLE; ///< "analogVuStyle"
    static const char *P_KEY_CENTERED_SPECTRUM_STYLE; ///< "centerSpecStyle"
    static const char *P_KEY_WATERFALL_STYLE; ///< "waterfallStyle"
    static const char *P_KEY_AURORA_STYLE; ///< "auroraStyle"

    // Workshop-Lotto Preferences Keys
    static const char *P_KEY_LOTTO_SYSTEM_INDEX; ///< "lottoSys"
    static const char *P_KEY_LOTTO_INPUT_MODE;   ///< "lottoMode"
    static const char *P_KEY_LOTTO_TICKET_LO;    ///< Untere 32 Bit der Tippzahlen
    static const char *P_KEY_LOTTO_TICKET_HI;    ///< Obere 32 Bit der Tippzahlen
    static const char *P_KEY_LOTTO_DRAW_LO;      ///< Untere 32 Bit der Gewinnzahlen
    static const char *P_KEY_LOTTO_DRAW_HI;      ///< Obere 32 Bit der Gewinnzahlen
    static const char *P_KEY_LOTTO_TICKET_SUPER; ///< "lottoTipSz"
    static const char *P_KEY_LOTTO_DRAW_SUPER;   ///< "lottoDrawSz"

    // Workshop-Normal-Lotto Preferences Keys
    static const char *P_KEY_LOTTO_NORMAL_INPUT_MODE;   ///< "lottoNMode"
    static const char *P_KEY_LOTTO_NORMAL_TICKET_LO;    ///< Untere 32 Bit der Tippzahlen
    static const char *P_KEY_LOTTO_NORMAL_TICKET_HI;    ///< Obere 32 Bit der Tippzahlen
    static const char *P_KEY_LOTTO_NORMAL_DRAW_LO;      ///< Untere 32 Bit der Gewinnzahlen
    static const char *P_KEY_LOTTO_NORMAL_DRAW_HI;      ///< Obere 32 Bit der Gewinnzahlen
    static const char *P_KEY_LOTTO_NORMAL_TICKET_SUPER; ///< "lottoNTipSz"
    static const char *P_KEY_LOTTO_NORMAL_DRAW_SUPER;   ///< "lottoNDrawSz"

    // Workshop-Flugradar Preferences Keys
    static const char *P_KEY_FLIGHT_RADAR_RADIUS; ///< "flightRadKm"

    // WiFi-Konfigurationen (3 Profile)
    struct WiFiProfile
    {
        char ssid[33];     ///< Maximal 32 Zeichen plus Null-Terminierung
        char password[65]; ///< Maximal 64 Zeichen plus Null-Terminierung
        bool enabled = false;
    };
    WiFiProfile m_wifiProfiles[3];

    // Preferences Keys für WiFi
    static const char *P_KEY_WIFI_SSID[3];
    static const char *P_KEY_WIFI_PASS[3];
    static const char *P_KEY_WIFI_ENABLED[3];

    /**
     * @brief Privater Konstruktor für Singleton
     */
    AppState();

    /**
     * @brief Lädt AppState-Werte aus Preferences (Wetter + NTP)
     */
    void loadFromPreferences();

    /**
     * @brief Speichert alle drei EQ-Werte und ihre Kennung im RTC-NVRAM
     * @return true bei vollständig erfolgreichem Schreiben
     */
    bool saveEqToNVRAM();

    /**
     * @brief Setzt den Standard-Wetterstandort, falls noch keine Wetter-Preferences existieren
     */
    void ensureWeatherDefaults();

public:
    /**
     * @brief Zugriff auf die Singleton-Instanz
     * @return Referenz auf die einzige AppState-Instanz
     */
    static AppState &instance();

    // Singleton: Kopieren und Zuweisen verbieten
    AppState(const AppState &) = delete;
    AppState &operator=(const AppState &) = delete;

    ~AppState() = default;

    /**
     * @brief Initialisiert AppState (muss einmal in setup() aufgerufen werden)
     * @param skipRtcInit Legacy-Parameter ohne Wirkung, bleibt aus Kompatibilitätsgründen erhalten
     * @param wire Optional: Spezifisches Wire-Interface für RTC
     * @return true bei Erfolg, false bei Fehler
     */
    bool begin(bool skipRtcInit = false, TwoWire *wire = &Wire);

    /**
     * @brief Lädt alle Einstellungen aus dem NVRAM neu
     * @note Kann jederzeit aufgerufen werden um Werte zu aktualisieren
     */
    void loadFromNVRAM();

    /**
     * @brief Direkter Zugriff auf die RTC-Instanz
     * @return Referenz auf die interne RTC-Instanz
     */
    DS1307_RTC &getRTC() { return m_rtc; }

    // Getter und Setter für NVRAM
    uint8_t getCurrentRetroStation() const { return m_currentRetroStation; }
    void setCurrentRetroStation(uint8_t value);

    uint8_t getCurrentVolume() const { return m_currentVolume; }
    void setCurrentVolume(uint8_t value);

    uint8_t getVolumeBeforeMute() const { return m_volumeBeforeMute; }
    void setVolumeBeforeMute(uint8_t value);

    int8_t getCurrentBalance() const { return m_currentBalance; }
    void setCurrentBalance(int8_t value);

    int8_t getCurrentGainLowPass() const { return m_currentGainLowPass; }
    void setCurrentGainLowPass(int8_t value);

    int8_t getCurrentGainBandPass() const { return m_currentGainBandPass; }
    void setCurrentGainBandPass(int8_t value);

    int8_t getCurrentGainHighPass() const { return m_currentGainHighPass; }
    void setCurrentGainHighPass(int8_t value);

    uint16_t getEqFreqLow() const { return m_eqFreqLow; }
    void setEqFreqLow(uint16_t value);

    uint16_t getEqFreqPeak() const { return m_eqFreqPeak; }
    void setEqFreqPeak(uint16_t value);

    uint16_t getEqFreqHigh() const { return m_eqFreqHigh; }
    void setEqFreqHigh(uint16_t value);

    uint8_t getStereoWidth() const { return m_stereoWidth; }
    void setStereoWidth(uint8_t value);

    uint8_t getLoudness() const { return m_loudness; }
    void setLoudness(uint8_t value);

    /**
     * @brief Gibt die gespeicherte Freigabe der internen Lautsprecher zurück.
     * @return true bei eingeschalteten Lautsprechern, false bei abgeschalteten Lautsprechern.
     */
    bool getSpeakersEnabled() const { return m_speakersEnabled; }

    /**
     * @brief Speichert die Freigabe der internen Lautsprecher in den Preferences.
     * @param enabled true für eingeschaltete, false für abgeschaltete Lautsprecher.
     */
    void setSpeakersEnabled(bool enabled);

    // Anzeige-Einstellungen

    /**
     * @brief Gibt den Stil des magischen Auges zurück
     * @return 0 = EM11, 1 = EM34, 2 = EM80, 3 = EM84
     */
    uint8_t getMagicEyeStyle() const { return m_magicEyeStyle; }

    /**
     * @brief Setzt den Stil des magischen Auges und speichert ihn persistent
     * @param value 0 = EM11, 1 = EM34, größere Werte werden geklemmt
     */
    void setMagicEyeStyle(uint8_t value);

    /**
     * @brief Gibt die Trägheit des analogen VU-Zeigers zurück
     * @return 0 = 150 ms, 1 = 300 ms, 2 = 600 ms
     */
    uint8_t getVuBallistics() const { return m_vuBallistics; }

    /**
     * @brief Setzt die Trägheit des analogen VU-Zeigers und speichert sie persistent
     * @param value 0 = 150 ms, 1 = 300 ms, 2 = 600 ms, größere Werte werden geklemmt
     */
    void setVuBallistics(uint8_t value);

    /**
     * @brief Gibt die Empfindlichkeit des analogen VU-Zeigers zurück
     * @return 0 bis 4, 0 entspricht -2 dBFS und 4 entspricht -18 dBFS
     */
    uint8_t getVuSensitivity() const { return m_vuSensitivity; }

    /**
     * @brief Setzt die Empfindlichkeit des analogen VU-Zeigers und speichert sie persistent
     * @param value 0 bis 4, größere Werte werden geklemmt
     */
    void setVuSensitivity(uint8_t value);

    /**
     * @brief Gibt die Farbfassung des analogen VU-Meters zurück
     * @return 0 = klassisch, 1 = grün
     */
    uint8_t getAnalogVUStyle() const { return m_analogVUStyle; }

    /**
     * @brief Setzt die Farbfassung des analogen VU-Meters und speichert sie persistent
     * @param value 0 = klassisch, 1 = grün, größere Werte werden geklemmt
     */
    void setAnalogVUStyle(uint8_t value);

    bool getIsMuted() const { return m_isMuted; }
    void setIsMuted(bool value);

    bool getForceMono() const { return m_forceMono; }
    void setForceMono(bool value);

    // Visualisierungen
    uint8_t getVisMode() const { return m_visMode; }
    void setVisMode(uint8_t value);

    uint8_t getDigitalVUStyle() const { return m_digitalVUStyle; }
    void setDigitalVUStyle(uint8_t value);

    uint8_t getSpectrumStyle() const { return m_spectrumStyle; }
    void setSpectrumStyle(uint8_t value);

    /**
     * @brief Gibt die unabhängig gespeicherte Farbwahl des mittigen Spektrums zurück
     * @return 0 = grün, 1 = Regenbogen
     */
    uint8_t getCenteredSpectrumStyle() const { return m_centeredSpectrumStyle; }

    /**
     * @brief Speichert die Farbwahl des mittigen Spektrums in den Preferences
     * @param value 0 = grün, 1 = Regenbogen, größere Werte werden geklemmt
     */
    void setCenteredSpectrumStyle(uint8_t value);

    /**
     * @brief Gibt die unabhängig gespeicherte Farbwahl des Wasserfalls zurück
     * @return 0 = grün, 1 = mehrfarbig
     */
    uint8_t getWaterfallStyle() const { return m_waterfallStyle; }

    /**
     * @brief Speichert die Farbwahl des Wasserfalls in den Preferences
     * @param value 0 = grün, 1 = mehrfarbig, größere Werte werden geklemmt
     */
    void setWaterfallStyle(uint8_t value);

    /**
     * @brief Gibt die gespeicherte Farbwelt des Nordlichts zurück
     * @return 0 = kühl, 1 = warm
     */
    uint8_t getAuroraStyle() const { return m_auroraStyle; }

    /**
     * @brief Speichert die Farbwelt des Nordlichts in den Preferences
     * @param value 0 = kühl, 1 = warm, größere Werte werden geklemmt
     */
    void setAuroraStyle(uint8_t value);

    uint8_t getWaveformStyle() const { return m_waveformStyle; }
    void setWaveformStyle(uint8_t value);

    uint8_t getGonioStyle() const { return m_gonioStyle; }
    void setGonioStyle(uint8_t value);

    // ToDo-Einstellungen
    bool getToDoShowCompleted() const { return m_toDoShowCompleted; }
    void setToDoShowCompleted(bool value);

    // Runtime-Variablen
    bool getSleepModeTriggered() const { return m_sleepModeTriggered; }
    void setSleepModeTriggered(bool triggered) { m_sleepModeTriggered = triggered; }

    bool isDisplayOn() const { return m_displayOn; }
    void setDisplayOn(bool on) { m_displayOn = on; }

    bool getWifiConnected() const { return m_wifiConnected; }
    void setWifiConnected(bool value) { m_wifiConnected = value; }

    bool getStationSetDone() const { return m_stationSetDone; }
    void setStationSetDone(bool value) { m_stationSetDone = value; }

    bool getBlockGFX() const { return m_blockGFX; }
    void setBlockGFX(bool value) { m_blockGFX = value; }

    void setNtpSynced(bool synced)
    {
        m_lastNtpSyncTime = synced ? millis() : 0;
    }

    bool getNtpSynced() const
    {
        if (m_lastNtpSyncTime == 0)
            return false;

        const unsigned long timeout = m_ntpSyncIntervalMs + 300000UL; // +5 min Puffer
        return (millis() - m_lastNtpSyncTime) < timeout;
    }

    bool getDstActive() const { return m_dstActive; }
    void setDstActive(bool value) { m_dstActive = value; }

    uint8_t getWifiSignalStrength() const { return m_wifiSignalStrength; }
    void setWifiSignalStrength(uint8_t value) { m_wifiSignalStrength = constrain(value, 0, 10); }

    uint8_t getWifiSignalPercent() const { return m_wifiSignalPercent; }
    void setWifiSignalPercent(uint8_t value) { m_wifiSignalPercent = constrain(value, 0, 100); }

    /**
     * @brief Speichert das Ergebnis des I2C-Startscans im RAM.
     * @param result Kommagetrennte Liste der gefundenen Adressen oder leer
     * @param completed true wenn der Scan ausgeführt wurde
     * @param busBlocked true wenn der Bus beim Scan blockiert war
     */
    void setI2cScanResult(const char *result, bool completed, bool busBlocked);

    /**
     * @brief Liefert die beim Start gefundenen I2C-Adressen.
     * @return Kommagetrennte Adressliste oder leerer String
     */
    const char *getI2cScanResult() const { return m_i2cScanResult; }

    bool wasI2cScanCompleted() const { return m_i2cScanCompleted; } ///< true wenn der Startscan ausgeführt wurde
    bool wasI2cScanBusBlocked() const { return m_i2cScanBusBlocked; } ///< true wenn SDA oder SCL beim Scan LOW blieb

    // Aktives Modul

    /**
     * @brief Gibt das aktuell aktive Modul zurück
     * @return AppModule Enum des aktiven Moduls
     */
    AppModule getActiveModule() const { return m_activeModule; }

    /**
     * @brief Setzt das aktuell aktive Modul
     * @param module Das neue aktive Modul
     * @note Wird von MainScreen beim Modulwechsel aufgerufen
     */
    void setActiveModule(AppModule module) { m_activeModule = module; }

    // Sperrbildschirm

    /**
     * @brief Gibt die gespeicherte Lock-PIN zurück
     * @return Zeiger auf internen 4-Zeichen-Puffer
     */
    const char *getLockPin() const { return m_lockPin; }

    /**
     * @brief Setzt die Lock-PIN und speichert persistent
     * @param pin Genau 4 Ziffern als C-String
     */
    void setLockPin(const char *pin);

    /**
     * @brief Fordert den Sperrbildschirm an (von jedem Modul aufrufbar)
     */
    void requestLock() { m_lockRequested = true; }

    /**
     * @brief Prüft und löscht das Lock-Request-Flag
     * @return true wenn ein Lock angefordert wurde
     */
    bool consumeLockRequest()
    {
        if (!m_lockRequested)
            return false;

        m_lockRequested = false;
        return true;
    }

    /**
     * @brief Fordert das externe Entsperren des Sperrbildschirms an
     *
     * Wird gesetzt, wenn der korrekte Code im Webinterface eingegeben wurde. Der Sperrbildschirm entsperrt sich daraufhin im nächsten Frame, sodass der Code nicht zusätzlich am Display eingegeben werden muss.
     */
    void requestDisplayUnlock() { m_displayUnlockRequested = true; }

    /**
     * @brief Prüft und löscht das externe Entsperr-Flag
     * @return true wenn ein externes Entsperren angefordert wurde
     */
    bool consumeDisplayUnlockRequest()
    {
        if (!m_displayUnlockRequested)
            return false;

        m_displayUnlockRequested = false;
        return true;
    }

    // Wetter-Preferences
    /**
     * @brief Ort setzen und persistent speichern (Preferences)
     */
    void setWeatherLocation(const char *location);
    /**
     * @brief Persistenter Ort (leer, falls nicht gesetzt)
     */
    void getWeatherLocation(char *buffer, size_t bufferSize) const;

    bool hasWeatherLocation() const { return strlen(m_weatherLocation) > 0; }

    /**
     * @brief True wenn gültige, nicht-triviale Koordinaten gespeichert sind
     * @note Unabhängig von hasWeatherLocation() - ermöglicht Koordinaten-Laden auch ohne gespeicherten Stadtnamen
     */
    bool hasWeatherCoordinates() const
    {
        return !std::isnan(m_weatherLat) && !std::isnan(m_weatherLon) && (m_weatherLat != 0.0 || m_weatherLon != 0.0);
    }

    const char *getWeatherLocationDirect() const { return m_weatherLocation; }

    /**
     * @brief Breitengrad setzen und persistent speichern (Preferences)
     * Wertebereich wird auf [-90, +90] begrenzt.
     */
    void setWeatherLatitude(double lat);
    double getWeatherLatitude() const { return m_weatherLat; }

    /**
     * @brief Längengrad setzen und persistent speichern (Preferences)
     * Wertebereich wird auf [-180, +180] begrenzt.
     */
    void setWeatherLongitude(double lon);
    double getWeatherLongitude() const { return m_weatherLon; }

    /**
     * @brief Löscht NUR die Wetter-Preferences und setzt die In-Memory-Werte zurück.
     */
    void resetPreferencesWeather();

    // NTP-Einstellungen

    /**
     * @brief Setzt den NTP-Server und speichert persistent
     * @param server NTP-Server Adresse (z.B. "de.pool.ntp.org")
     */
    void setNtpServer(const char *server);

    /**
     * @brief Gibt den NTP-Server zurück
     * @return Zeiger auf internen Puffer mit NTP-Server
     */
    const char *getNtpServer() const { return m_ntpServer; }

    /**
     * @brief Prüft ob ein NTP-Server konfiguriert ist
     * @return true wenn NTP-Server nicht leer
     */
    bool hasNtpServer() const { return strlen(m_ntpServer) > 0; }

    /**
     * @brief Setzt die Zeitzone-Region und speichert persistent
     * @param region Zeitzone-Region (z.B. "Europe/Berlin")
     */
    void setTimezoneRegion(const char *region);

    /**
     * @brief Gibt die Zeitzone-Region zurück
     * @return Zeiger auf internen Puffer mit Region
     */
    const char *getTimezoneRegion() const { return m_timezoneRegion; }

    /**
     * @brief Setzt den POSIX Zeitzone-String und speichert persistent
     * @param tzStr POSIX Zeitzone (z.B. "CET-1CEST,M3.5.0,M10.5.0/3")
     */
    void setTimezoneStr(const char *tzStr);

    /**
     * @brief Gibt den POSIX Zeitzone-String zurück
     * @return Zeiger auf internen Puffer mit Zeitzone
     */
    const char *getTimezoneStr() const { return m_timezoneStr; }

    /**
     * @brief Prüft ob eine Zeitzone konfiguriert ist
     * @return true wenn Zeitzone-String nicht leer
     */
    bool hasTimezone() const { return strlen(m_timezoneStr) > 0; }

    /**
     * @brief Setzt das NTP Sync-Intervall und speichert persistent
     * @param intervalMs Intervall in Millisekunden (min. 60000 = 1 Minute)
     */
    void setNtpSyncInterval(unsigned long intervalMs);

    /**
     * @brief Gibt das NTP Sync-Intervall zurück
     * @return Intervall in Millisekunden
     */
    unsigned long getNtpSyncInterval() const { return m_ntpSyncIntervalMs; }

    /**
     * @brief Löscht NUR die NTP-Preferences und setzt Defaultwerte
     */
    void resetPreferencesNtp();

    // Regionale Einstellungen

    /**
     * @brief Setzt den Tagesschau-Regionalfilter und speichert persistent
     * @param region Bundesland-ID (1=BW, 2=BY, 3=BE, 4=BB, 5=HB, 6=HH, 7=HE,
     *               8=MV, 9=NI, 10=NRW, 11=RLP, 12=SL, 13=SN, 14=ST, 15=SH, 16=TH)
     */
    void setNewsRegion(int region);

    /** @brief Gibt den Tagesschau-Regionalfilter zurück */
    int getNewsRegion() const { return m_newsRegion; }

    /**
     * @brief Setzt die Postleitzahl und speichert persistent
     * @param plz Fünfstellige PLZ, z.B. "15745" - wird auf Länge 5 validiert
     */
    void setPlz(const char *plz);

    /** @brief Gibt die gespeicherte Postleitzahl zurück */
    const char *getPlz() const { return m_plz; }

    /** @brief Tagesschau-Ressortfilter lesen (leer = kein Filter) */
    const char *getNewsRessort() const { return m_newsRessort; }

    /** @brief Tagesschau-Ressortfilter setzen und persistieren */
    void setNewsRessort(const char *ressort);

    /** @brief True wenn eine PLZ gesetzt ist */
    bool hasPlz() const { return m_plz[0] != '\0'; }

    // Cloud-Anbindung

    /**
     * @brief Setzt die Google Calendar Script-URL und speichert persistent
     * @param url Google Apps Script URL für Kalender
     */
    void setCalendarScriptUrl(const char *url);

    /**
     * @brief Gibt die Google Calendar Script-URL zurück
     */
    const char *getCalendarScriptUrl() const { return m_calendarScriptUrl; }

    /**
     * @brief Setzt die Google ToDo Script-URL und speichert persistent
     * @param url Google Apps Script URL für ToDo
     */
    void setTodoScriptUrl(const char *url);

    /**
     * @brief Gibt die Google ToDo Script-URL zurück
     */
    const char *getTodoScriptUrl() const { return m_todoScriptUrl; }

    /**
     * @brief Setzt die Google Contacts Script-URL und speichert persistent
     * @param url Google Apps Script URL für Kontakte
     */
    void setContactsScriptUrl(const char *url);

    /**
     * @brief Gibt die Google Contacts Script-URL zurück
     */
    const char *getContactsScriptUrl() const { return m_contactsScriptUrl; }

    /**
     * @brief Setzt die OpenSky Client-ID und speichert persistent
     * @param clientId OpenSky Client-ID für das Flugradar
     */
    void setOpenSkyClientId(const char *clientId);

    /**
     * @brief Gibt die OpenSky Client-ID zurück
     */
    const char *getOpenSkyClientId() const { return m_openSkyClientId; }

    /**
     * @brief Setzt das OpenSky Client-Secret und speichert persistent
     * @param clientSecret OpenSky Client-Secret für das Flugradar
     */
    void setOpenSkyClientSecret(const char *clientSecret);

    /**
     * @brief Gibt das OpenSky Client-Secret zurück
     */
    const char *getOpenSkyClientSecret() const { return m_openSkyClientSecret; }

    /**
     * @brief Prüft, ob beide OpenSky-Zugangsdaten gespeichert sind
     */
    bool hasOpenSkyCredentials() const { return m_openSkyClientId[0] != '\0' && m_openSkyClientSecret[0] != '\0'; }

    /**
     * @brief Löscht die Cloud-Preferences und setzt Defaultwerte (leer)
     */
    void resetPreferencesGoogleApi();

    // UI-Einstellungen

    /**
     * @brief Setzt die UI-Sprache und speichert persistent
     * @param language Sprachkennung, erlaubt sind "de" und "en"
     */
    void setUiLanguage(const char *language);

    /**
     * @brief Gibt die gespeicherte UI-Sprache zurück
     * @return Sprachkennung, z.B. "de" oder "en"
     */
    const char *getUiLanguage() const { return m_uiLanguage; }

    /**
     * @brief Löscht die UI-Sprach-Preference und setzt Deutsch als Standard
     */
    void resetPreferencesUiLanguage();

    // Zeitfenster

    /**
     * @brief Gibt zurück, ob ein Zeitfenster für einen Wochentag und eine Stunde aktiv ist.
     * @param day Wochentag-Index 0..6 (Montag..Sonntag)
     * @param hour Stunde 0..23
     * @return true wenn aktiv, sonst false
     */
    bool isTimeWindowEnabled(uint8_t day, uint8_t hour) const;

    /**
     * @brief Gibt die 24-Bit-Tagesmaske eines Zeitfenster-Tages zurück.
     * @param day Wochentag-Index 0..6 (Montag..Sonntag)
     * @return Bitmaske, Bit 0..23 steht für Stunde 0..23
     */
    uint32_t getTimeWindowDayMask(uint8_t day) const;

    /**
     * @brief Setzt und speichert die 24-Bit-Tagesmaske eines Zeitfenster-Tages.
     * @param day Wochentag-Index 0..6 (Montag..Sonntag)
     * @param mask Bitmaske, Bit 0..23 steht für Stunde 0..23
     */
    void setTimeWindowDayMask(uint8_t day, uint32_t mask);

    /**
     * @brief Setzt alle Zeitfenster auf Standard (alle Stunden aktiv) und speichert sie.
     */
    void resetPreferencesTimeWindow();

    // Workshop-Lotto

    /**
     * @brief Speichert den aktuellen Lotto-Systemschein persistent.
     * @param systemIndex Vollsystem-Index 0..4
     * @param mode Eingabemodus 0=Tipp, 1=Ziehung
     * @param ticketMask Tippzahlen, Bit n steht für Zahl n
     * @param drawMask Gewinnzahlen, Bit n steht für Zahl n
     * @param ticketSuper Tipp-Superzahl 0..9, 10=offen
     * @param drawSuper Ziehungs-Superzahl 0..9, 10=offen
     */
    void setWorkshopLottoState(uint8_t systemIndex, uint8_t mode, uint64_t ticketMask, uint64_t drawMask, uint8_t ticketSuper, uint8_t drawSuper);

    /**
     * @brief Gibt den persistenten Lotto-Systemschein zurück.
     * @param systemIndex Vollsystem-Index 0..4
     * @param mode Eingabemodus 0=Tipp, 1=Ziehung
     * @param ticketMask Tippzahlen, Bit n steht für Zahl n
     * @param drawMask Gewinnzahlen, Bit n steht für Zahl n
     * @param ticketSuper Tipp-Superzahl 0..9, 10=offen
     * @param drawSuper Ziehungs-Superzahl 0..9, 10=offen
     */
    void getWorkshopLottoState(uint8_t &systemIndex, uint8_t &mode, uint64_t &ticketMask, uint64_t &drawMask, uint8_t &ticketSuper, uint8_t &drawSuper) const;

    /**
     * @brief Löscht den persistenten Lotto-Systemschein.
     */
    void resetPreferencesWorkshopLotto();

    /**
     * @brief Speichert den normalen Lotto-Schein persistent.
     * @param mode Eingabemodus 0=Tipp, 1=Ziehung
     * @param ticketMask Tippzahlen, Bit n steht für Zahl n
     * @param drawMask Gewinnzahlen, Bit n steht für Zahl n
     * @param ticketSuper Tipp-Superzahl 0..9, 10=offen
     * @param drawSuper Ziehungs-Superzahl 0..9, 10=offen
     */
    void setWorkshopNormalLottoState(uint8_t mode, uint64_t ticketMask, uint64_t drawMask, uint8_t ticketSuper, uint8_t drawSuper);

    /**
     * @brief Gibt den persistenten normalen Lotto-Schein zurück.
     * @param mode Eingabemodus 0=Tipp, 1=Ziehung
     * @param ticketMask Tippzahlen, Bit n steht für Zahl n
     * @param drawMask Gewinnzahlen, Bit n steht für Zahl n
     * @param ticketSuper Tipp-Superzahl 0..9, 10=offen
     * @param drawSuper Ziehungs-Superzahl 0..9, 10=offen
     */
    void getWorkshopNormalLottoState(uint8_t &mode, uint64_t &ticketMask, uint64_t &drawMask, uint8_t &ticketSuper, uint8_t &drawSuper) const;

    /**
     * @brief Löscht den persistenten normalen Lotto-Schein.
     */
    void resetPreferencesWorkshopNormalLotto();

    /**
     * @brief Speichert den Flugradar-Suchradius persistent.
     * @param radiusKm Suchradius in Kilometern
     */
    void setWorkshopFlightRadarRadiusKm(uint8_t radiusKm);

    /**
     * @brief Gibt den persistenten Flugradar-Suchradius zurück.
     * @return Suchradius in Kilometern
     */
    uint8_t getWorkshopFlightRadarRadiusKm() const;

    /**
     * @brief Löscht den persistenten Flugradar-Suchradius.
     */
    void resetPreferencesWorkshopFlightRadar();

    // WiFi-Profile

    /**
     * @brief Setzt ein WiFi-Profil
     * @param index Profil-Index (0-2)
     * @param ssid SSID als C-String
     * @param password Passwort als C-String
     * @param enabled Profil aktiviert/deaktiviert
     */
    void setWiFiProfile(uint8_t index, const char *ssid, const char *password, bool enabled = true);

    /**
     * @brief Holt ein WiFi-Profil
     * @param index Profil-Index (0-2)
     * @param ssid Buffer für SSID (min. 33 Bytes)
     * @param ssidSize Größe des SSID-Buffers
     * @param password Buffer für Passwort (min. 65 Bytes)
     * @param passSize Größe des Passwort-Buffers
     * @return true, wenn ein Profil vorhanden ist (SSID ≠ leer) und Daten kopiert wurden
     */
    bool getWiFiProfile(uint8_t index, char *ssid, size_t ssidSize, char *password, size_t passSize) const;

    /**
     * @brief Prüft ob ein WiFi-Profil aktiviert ist
     * @param index Profil-Index (0-2)
     * @return true wenn aktiviert
     */
    bool isWiFiProfileEnabled(uint8_t index) const;

    /**
     * @brief Löscht ein WiFi-Profil
     * @param index Profil-Index (0-2)
     */
    void clearWiFiProfile(uint8_t index);

    /**
     * @brief Zählt die aktiven WiFi-Profile
     * @return Anzahl der aktivierten Profile
     */
    uint8_t getActiveWiFiProfileCount() const;

    /**
     * @brief Lädt alle WiFi-Profile aus den Preferences
     */
    void loadWiFiProfiles();

    /**
     * @brief Speichert alle WiFi-Profile in die Preferences
     */
    void saveWiFiProfiles();

    // Geräte-Netzwerkname

    /**
     * @brief Setzt den Geräte-Netzwerknamen und speichert ihn persistent
     * @param hostname Netzwerkname als C-String
     *
     * Ein leerer oder fehlender Name setzt den Standardnamen ein. Die Prüfung auf erlaubte Zeichen findet in der jeweiligen Eingabeschicht statt; hier wird nur sichergestellt, dass nie ein leerer Name gespeichert wird. Der Name wird erst beim nächsten Verbindungsaufbau wirksam.
     */
    void setHostname(const char *hostname);

    /**
     * @brief Gibt den Geräte-Netzwerknamen zurück
     * @return Zeiger auf internen Puffer mit dem Netzwerknamen (nie leer)
     */
    const char *getHostname() const { return m_hostname; }

    // Reset

    /**
     * @brief Setzt alle Einstellungen auf Standardwerte zurück
     * @param clearNVRAM Wenn true, wird auch das RTC-NVRAM gelöscht
     * @param clearPrefs Wenn true, werden zusätzlich Wetter-, NTP- und Cloud-Preferences gelöscht
     */
    void resetToDefaults(bool clearNVRAM = true, bool clearPrefs = false);
};

/**
 * @brief Makro für kürzeren Zugriff:
 *
 * APP_STATE.setCurrentVolume(75);
 * uint8_t vol = APP_STATE.getCurrentVolume();
 */
#define APP_STATE AppState::instance()

#endif // APP_STATE_H
