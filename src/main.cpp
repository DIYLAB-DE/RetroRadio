#include "main.h"

#include <CH422G_Wire.h>       // Treiber für CH422G I/O-Expander
#include <ESPAsyncWebServer.h> // Asynchroner Webserver für HTTP- und WebSocket-Kommunikation
#include <Preferences.h>       // NVS-Preferences für persistente Speicherung von Einstellungen
#include <WiFi.h>              // WLAN-Funktionalität für ESP32-S3
#include <esp_sntp.h>          // SNTP-Client für NTP-Zeit-Synchronisation
#include <esp_wifi.h>          // WLAN-Funktionen und -Konfiguration
#include <mbedtls/platform.h>  // mbedTLS für sichere Verbindungen (z.B. HTTPS)
#include <nvs_flash.h>         // NVS-Flash für persistente Speicherung (z.B. WLAN-Zugangsdaten)
#include <stdint.h>            // Standard-Integer-Typen
#include <time.h>              // Zeitfunktionen für RTC und NTP

#include "FtpService.h"     // FTP-Server für SD-Karten-Zugriff
#include "HttpService.h"    // HTTP
#include "AppState.h"       // Persistierungen
#include "SystemStats.h"    // Systemstatistiken
#include "Audio.h"          // Audio-Steuerung und -Wiedergabe
#include "DS1307_RTC.h"     // Echtzeituhr (RTC) DS1307
#include "Gfx.h"            // Grafikfunktionen und Displayinitialisierung
#include "Keyboard.h"       // modales Keyboard
#include "Logger.h"         // Logger
#include "MainScreen.h"     // Startbildschirm
#include "Pins.h"           // Pinbelegungen für das Projekt
#include "RetroRadio.h"     // Hauptlogik des Retro-Radios
#include "Routes.h"         // Definition der Radiostationen und Routen
#include "SDCard.h"         // SD-Karten-Zugriff und Dateisystem
#include "SDPlayer.h"       // SDPlayer Klasse
#include "StringUtils.h"    // sichere String-Helfer
#include "Symbols.h"        // Symbole
#include "Tools.h"          // Hilfsfunktionen und Utility-Methoden
#include "WifiList.h"       // modales Fenster mit verfügbaren WiFi-Netzen
#include "WifiManager.h"    // WLAN-Verwaltung und -Konfiguration
#include "StationManager.h" // Verwaltung der Radiostationen

// ===================================================================
// Globale/Static Objekte
// ===================================================================

LGFX tft;
Symbols symbols(&tft);
WifiList wifiList(&tft);
Keyboard keyboard(&tft);
AsyncWebServer server(80);
WifiManager wifiManager;
StationManager stationManager;
Preferences prefs;

// IO-Expander
CH422G_Wire expander;

DS1307_RTC rtc;
Audio audio;
static MainScreen *mainScreen = nullptr;

// RTC
bool rtcInit = false;
volatile bool g_time_needs_sync_to_rtc = false;

// Minimales Jahr, ab dem eine aus der RTC gelesene Zeit als plausibel übernommen wird. Unterhalb dieses Werts gilt die RTC als ungestellt (z.B. 2000-01-01 bei frischem Modul).
constexpr uint16_t MIN_VALID_RTC_YEAR = 2026;

// One-shot Marker für Netzwerk-Init (NTP/Routes/Webserver)
static bool g_netInitDone = false;

namespace
{
    constexpr uint8_t LOUDNESS_FADE_OUT_VOLUME = 90; ///< Ab dieser Lautstärke ist Loudness neutral; 90 statt 60, damit sie im üblichen Hörbereich (50-70) überhaupt wirkt
    constexpr float LOUDNESS_MAX_LOW_DB = 8.0f;      ///< Maximale Tiefen-Anhebung bei Loudness 100 %
    constexpr float LOUDNESS_MAX_HIGH_DB = 3.0f;     ///< Maximale Höhen-Anhebung bei Loudness 100 %
    constexpr float TONE_MIN_DB = -12.0f;            ///< Untere Grenze der AudioLib-Klangregelung
    constexpr float TONE_MAX_DB = 12.0f;             ///< Obere Grenze der AudioLib-Klangregelung
    constexpr uint8_t I2C_SCAN_ADDR_FIRST = 0x03;    ///< Erste gültige 7-Bit-Adresse für den Startscan
    constexpr uint8_t I2C_SCAN_ADDR_LAST = 0x77;     ///< Letzte gültige 7-Bit-Adresse für den Startscan
    constexpr size_t I2C_ADDR_TEXT_CAP = 5;           ///< Präfix, zwei Hex-Zeichen plus Nullbyte
    constexpr uint8_t CH422G_ADDR_LOW_FIRST = 0x20;   ///< Erste 7-Bit-Adresse im unteren CH422G-Adressblock
    constexpr uint8_t CH422G_ADDR_LOW_LAST = 0x27;    ///< Letzte 7-Bit-Adresse im unteren CH422G-Adressblock
    constexpr uint8_t CH422G_ADDR_HIGH_FIRST = 0x30;  ///< Erste 7-Bit-Adresse im oberen CH422G-Adressblock
    constexpr uint8_t CH422G_ADDR_HIGH_LAST = 0x3F;   ///< Letzte 7-Bit-Adresse im oberen CH422G-Adressblock
    constexpr const char *CH422G_SCAN_TEXT = "0x20..0x27, 0x30..0x3F"; ///< Kompakte Anzeige der CH422G-Adressblöcke
    constexpr uint16_t I2C_WIRE_TIMEOUT_MS = 8;      ///< Kurzer Timeout gegen langes Blockieren bei Busproblemen
    constexpr uint8_t I2C_TOUCH_READY_DELAY_MS = 20; ///< Wartezeit nach Touch-Reset vor dem Startscan
    /**
     * @brief Begrenzt einen Klangwert auf den Bereich der AudioLib.
     */
    float clampToneDb(float value)
    {
        if (value < TONE_MIN_DB)
            return TONE_MIN_DB;
        if (value > TONE_MAX_DB)
            return TONE_MAX_DB;
        return value;
    }

    /**
     * @brief Berechnet den Lautstärkeanteil der Loudness-Korrektur.
     */
    float loudnessVolumeFactor(uint8_t volume)
    {
        if (volume >= LOUDNESS_FADE_OUT_VOLUME)
            return 0.0f;

        return static_cast<float>(LOUDNESS_FADE_OUT_VOLUME - volume) / static_cast<float>(LOUDNESS_FADE_OUT_VOLUME);
    }

    /**
     * @brief Prüft, ob der I2C-Bus offensichtlich blockiert ist.
     */
    bool isI2cBusLineLow()
    {
        return digitalRead(i2cPins::I2C_MASTER_SDA_IO) == LOW ||
               digitalRead(i2cPins::I2C_MASTER_SCL_IO) == LOW;
    }

    /**
     * @brief Prüft, ob eine Adresse zum bekannten CH422G-Adressbereich gehört.
     */
    bool isCh422gAddress(uint8_t address)
    {
        return (address >= CH422G_ADDR_LOW_FIRST && address <= CH422G_ADDR_LOW_LAST) ||
               (address >= CH422G_ADDR_HIGH_FIRST && address <= CH422G_ADDR_HIGH_LAST);
    }

    /**
     * @brief Prüft, ob eine I2C-Adresse quittiert wird.
     */
    bool probeI2cAddress(TwoWire &wire, uint8_t address)
    {
        wire.beginTransmission(address);
        return wire.endTransmission() == 0;
    }

    /**
     * @brief Hängt einen Eintrag an die kommagetrennte I2C-Ergebnisliste an.
     */
    bool appendI2cScanText(char *dst, size_t dstSize, const char *item)
    {
        if (!dst || dstSize == 0 || !item)
            return false;

        const size_t currentLen = strnlen(dst, dstSize);
        const size_t itemLen = strlen(item);
        const size_t separatorLen = currentLen > 0 ? 2 : 0;
        if (currentLen + separatorLen + itemLen >= dstSize)
            return false;

        if (currentLen > 0)
            stringutils::safeAppend(dst, dstSize, ", ");
        stringutils::safeAppend(dst, dstSize, item);
        return true;
    }

    /**
     * @brief Hängt eine I2C-Adresse an die kommagetrennte Ergebnisliste an.
     */
    bool appendI2cAddress(char *dst, size_t dstSize, uint8_t address)
    {
        if (!dst || dstSize == 0)
            return false;

        char item[I2C_ADDR_TEXT_CAP] = {0};
        stringutils::safeFormat(item, "0x%02X", static_cast<unsigned>(address));

        return appendI2cScanText(dst, dstSize, item);
    }

    /**
     * @brief Scannt den I2C-Bus einmal beim Start und speichert das Ergebnis im AppState.
     */
    void scanStartupI2cBus(TwoWire &wire)
    {
        char result[AppState::I2C_SCAN_RESULT_CAP] = {0};

        if (isI2cBusLineLow())
        {
            APP_STATE.setI2cScanResult("", true, true);
            LOG_WARNING("I2C-Startscan abgebrochen: SDA oder SCL ist LOW");
            return;
        }

        bool truncated = false;
        bool ch422gRangeAdded = false;
        for (uint8_t address = I2C_SCAN_ADDR_FIRST; address <= I2C_SCAN_ADDR_LAST; ++address)
        {
            wire.beginTransmission(address);
            const uint8_t error = wire.endTransmission();

            if (error == 0 && isCh422gAddress(address))
            {
                if (!ch422gRangeAdded && !appendI2cScanText(result, sizeof(result), CH422G_SCAN_TEXT))
                {
                    truncated = true;
                    break;
                }
                ch422gRangeAdded = true;
            }
            else if (error == 0 && !appendI2cAddress(result, sizeof(result), address))
            {
                truncated = true;
                break;
            }

            if (isI2cBusLineLow())
            {
                APP_STATE.setI2cScanResult("", true, true);
                LOG_WARNING_F("I2C-Startscan abgebrochen: Bus nach Adresse 0x%02X blockiert", address);
                return;
            }
        }

        if (truncated)
            stringutils::safeAppend(result, sizeof(result), ", ...");

        APP_STATE.setI2cScanResult(result, true, false);
        if (result[0] != '\0')
            LOG_INFO_F("I2C-Startscan: %s", result);
        else
            LOG_WARNING("I2C-Startscan: keine Geräte gefunden");
    }

}

/**
 * @brief Setzt den globalen SDPlayer-Zeiger.
 */
static SdPlayer *g_activeSdPlayer = nullptr;
/**
 * @brief Setzt die aktive SD-Player-Instanz für globale Rückrufe.
 */
void setActiveSdPlayer(SdPlayer *player)
{
    g_activeSdPlayer = player;
}

// ===================================================================
// Zeit-Sync Callback
// ===================================================================

/**
 * @brief Callback-Funktion für NTP-Zeit-Synchronisation.
 *        Setzt nur einen Merker; Ausführung erfolgt in loop().
 */
void time_sync_callback(struct timeval * /*tv*/)
{
    g_time_needs_sync_to_rtc = true;
    SYSTEM_STATS.setLastNtpSyncTime(); // NTP-Sync-Zeit und Zähler aktualisieren
}

// ===================================================================
// Bedien-Handler
// ===================================================================

/**
 * @brief Handler für Stations-Schritt-Buttons.
 */
void handleButtonPressed(uint16_t btnIdx, bool /*buttonState*/)
{
    const uint8_t current = APP_STATE.getCurrentRetroStation();

    if (btnIdx == 1) // Zurück
    {
        // Nächsten nicht-leeren Slot rückwärts suchen
        for (int i = current - 1; i >= 0; i--)
        {
            const char *url = stationManager.getStationUrl(i);
            if (url && url[0] != '\0')
            {
                APP_STATE.setCurrentRetroStation(i);
                setNewStation(i);
                return;
            }
        }
    }
    else if (btnIdx == 2) // Vor
    {
        // Nächsten nicht-leeren Slot vorwärts suchen
        for (int i = current + 1; i < MAX_STATIONS; i++)
        {
            const char *url = stationManager.getStationUrl(i);
            if (url && url[0] != '\0')
            {
                APP_STATE.setCurrentRetroStation(i);
                setNewStation(i);
                return;
            }
        }
    }
}

/**
 * @brief Handler für Stationsauswahl über Skala.
 */
void handleStationPressed(uint16_t staIdx)
{
    APP_STATE.setCurrentRetroStation(staIdx);
}

/**
 * @brief Lautstärke geändert.
 */
void handleVolumeChanged(uint8_t volume)
{
    APP_STATE.setCurrentVolume(volume);

    // Eine hörbare Lautstärke beendet die Stummschaltung, sonst bleibt das Flag zwischen Radio und SD-Player hängen
    if (volume > 0 && APP_STATE.getIsMuted())
        APP_STATE.setIsMuted(false);

    audio.setVolume(volume, 1);
    applyAudioTone(APP_STATE.getEqFreqLow(), APP_STATE.getEqFreqPeak(), APP_STATE.getEqFreqHigh(), APP_STATE.getLoudness());

    // SSE-Event an Web-Clients senden
    extern void sendRadioSSEEvent();
    sendRadioSSEEvent();
}

/**
 * @brief Lautstärke vor Mute gesichert.
 */
void handleVolumeBeforeMute(uint8_t volume)
{
    APP_STATE.setVolumeBeforeMute(volume);
}

/**
 * @brief Mute geändert.
 */
void handleMuteChanged(bool mute)
{
    APP_STATE.setIsMuted(mute);

    // SSE-Event an Web-Clients senden
    extern void sendRadioSSEEvent();
    sendRadioSSEEvent();
}

/**
 * @brief Balance geändert.
 */
void handleBalanceChanged(int8_t balance)
{
    APP_STATE.setCurrentBalance(balance);
    audio.setBalance(balance);

    LOG_PRINTF(Logger::AUDIO, "Balance geändert: %d", balance);

    // SSE-Event an Web-Clients senden
    extern void sendRadioSSEEvent();
    sendRadioSSEEvent();
}

/**
 * @brief Tiefenregler (Low-Pass) geändert.
 */
void handleGainLowPassChanged(int8_t gainLowPass)
{
    APP_STATE.setCurrentGainLowPass(gainLowPass);
    applyAudioTone(APP_STATE.getEqFreqLow(), APP_STATE.getEqFreqPeak(), APP_STATE.getEqFreqHigh(), APP_STATE.getLoudness());

    LOG_PRINTF(Logger::AUDIO, "EQ geändert: Tiefen=%d dB, Mitten=%d dB, Höhen=%d dB",
               APP_STATE.getCurrentGainLowPass(), APP_STATE.getCurrentGainBandPass(), APP_STATE.getCurrentGainHighPass());

    // SSE-Event an Web-Clients senden
    extern void sendRadioSSEEvent();
    sendRadioSSEEvent();
}

/**
 * @brief Mittenregler (Band-Pass) geändert.
 */
void handleGainBandPassChanged(int8_t gainBandPass)
{
    APP_STATE.setCurrentGainBandPass(gainBandPass);
    applyAudioTone(APP_STATE.getEqFreqLow(), APP_STATE.getEqFreqPeak(), APP_STATE.getEqFreqHigh(), APP_STATE.getLoudness());

    LOG_PRINTF(Logger::AUDIO, "EQ geändert: Tiefen=%d dB, Mitten=%d dB, Höhen=%d dB",
               APP_STATE.getCurrentGainLowPass(), APP_STATE.getCurrentGainBandPass(), APP_STATE.getCurrentGainHighPass());

    // SSE-Event an Web-Clients senden
    extern void sendRadioSSEEvent();
    sendRadioSSEEvent();
}

/**
 * @brief Höhenregler (High-Pass) geändert.
 */
void handleGainHighPassChanged(int8_t gainHighPass)
{
    APP_STATE.setCurrentGainHighPass(gainHighPass);
    applyAudioTone(APP_STATE.getEqFreqLow(), APP_STATE.getEqFreqPeak(), APP_STATE.getEqFreqHigh(), APP_STATE.getLoudness());

    LOG_PRINTF(Logger::AUDIO, "EQ geändert: Tiefen=%d dB, Mitten=%d dB, Höhen=%d dB",
               APP_STATE.getCurrentGainLowPass(), APP_STATE.getCurrentGainBandPass(), APP_STATE.getCurrentGainHighPass());

    // SSE-Event an Web-Clients senden
    extern void sendRadioSSEEvent();
    sendRadioSSEEvent();
}

/**
 * @brief Wendet die drei Filter-Eckfrequenzen auf die Klangregelung an
 */
void applyEqFrequencies(uint16_t freqLow, uint16_t freqPeak, uint16_t freqHigh)
{
    applyAudioTone(freqLow, freqPeak, freqHigh, APP_STATE.getLoudness());
}

/**
 * @brief Wendet Eckfrequenzen und Loudness-Korrektur auf die Klangregelung an.
 */
void applyAudioTone(uint16_t freqLow, uint16_t freqPeak, uint16_t freqHigh, uint8_t loudness)
{
    audio.settings.FREQ_LS_HZ = freqLow;
    audio.settings.FREQ_PEAK_HZ = freqPeak;
    audio.settings.FREQ_HS_HZ = freqHigh;

    const float intensity = static_cast<float>(constrain(loudness, 0, 100)) / 100.0f;
    const float volumeFactor = loudnessVolumeFactor(APP_STATE.getCurrentVolume());
    const float lowBoost = LOUDNESS_MAX_LOW_DB * intensity * volumeFactor;
    const float highBoost = LOUDNESS_MAX_HIGH_DB * intensity * volumeFactor;

    const float low = clampToneDb(static_cast<float>(APP_STATE.getCurrentGainLowPass()) + lowBoost);
    const float mid = clampToneDb(static_cast<float>(APP_STATE.getCurrentGainBandPass()));
    const float high = clampToneDb(static_cast<float>(APP_STATE.getCurrentGainHighPass()) + highBoost);

    audio.setTone(low, mid, high);
}

/**
 * @brief Wendet die Basisbreite auf die Audio-Ausgabe an.
 */
void applyStereoWidth(uint8_t width)
{
    audio.setStereoWidth(width);
}

/**
 * @brief Schaltet die internen Lautsprecher unabhängig vom digitalen Audiostrom.
 */
void applySpeakerOutput(bool enabled)
{
    gpio_set_level(speakerPins::RS485_CONTROL, enabled ? 1 : 0);
    LOG_INFO_F("Lautsprecher: %s", enabled ? "an" : "aus");
}

// ===================================================================
// SDPlayer-Handler
// ===================================================================

/**
 * @brief Spielt eine Datei von der SD-Karte ab
 * @param path SD-relativer Pfad, zum Beispiel "/system/sounds/pomodoro.mp3"
 * @return true wenn die Wiedergabe gestartet wurde
 *
 * audio-lite nimmt POSIX-Pfade entgegen, nicht mehr ein Dateisystem-Objekt. SD.begin() hängt die Karte unter /sd ins VFS ein, die Pfade im Projekt sind aber SD-relativ. Deshalb steht das Präfix an genau dieser einen Stelle statt verstreut an jedem Aufruf.
 */
bool playFileFromSd(const char *path)
{
    if (!path || path[0] == '\0')
        return false;

    char vfsPath[320];
    const int n = snprintf(vfsPath, sizeof(vfsPath), "/sd%s", path);
    if (n < 0 || static_cast<size_t>(n) >= sizeof(vfsPath))
    {
        LOG_ERROR_F("Dateipfad zu lang: %s", path);
        return false;
    }

    return audio.connecttoFS(vfsPath);
}

/**
 * @brief Startet SD-Datei.
 */
void handlePlayFileRequested(const char *path)
{
    if (!path || path[0] == '\0')
    {
        LOG_ERROR("Ungültiger Dateipfad für SD-Wiedergabe.");
        return;
    }

    LOG_PRINTF(Logger::SENDER, "Befehl zum Abspielen von SD: %s", path);

    // Metadaten im UI vor dem Neuaufbau leeren. Das eigentliche Stoppen erfolgt zentral über den Stop-Callback des Players.
    if (g_activeSdPlayer)
        g_activeSdPlayer->updateMetadata("", "", "");

    // Musikdatei abspielen
    playFileFromSd(path);
}

/**
 * @brief Stoppt Wiedergabe.
 */
void handleStopRequested()
{
    LOG_PRINTF(Logger::SENDER, "Befehl: Wiedergabe stoppen");
    audio.stopSong();
}

/**
 * @brief Pause/Resume umschalten.
 */
void handlePauseResumeRequested()
{
    bool isPlaying = audio.isRunning();
    LOG_PRINTF(Logger::SENDER, "Befehl: Pause/Resume (aktuell: %s)", isPlaying ? "läuft" : "pausiert");
    audio.pauseResume();
}

/**
 * @brief Spult in der laufenden Wiedergabe um die angegebenen Sekunden.
 */
void handleSeekOffsetRequested(int sec)
{
    audio.setTimeOffset(sec);
}

// ===================================================================
// Arduino setup/loop
// ===================================================================

/**
 * @brief Speicher-Zuteiler für mbedTLS mit gezielter Aufteilung
 * @param n Anzahl der Elemente
 * @param size Größe eines Elements in Bytes
 * @return Genullter Speicherblock oder nullptr bei Fehler
 *
 * Große Blöcke (vor allem der TLS-Record-Puffer, über den die Entschlüsselung läuft) gehen in den internen RAM, damit die häufigen AES-Zugriffe nicht den PSRAM-Bus belegen und die LCD-DMA ausbremsen. Kleinere Allokationen (Handshake-Transienten) gehen ins PSRAM, um den knappen internen RAM nicht zu sprengen. Reicht der interne RAM für einen großen Block nicht, fällt die Zuteilung auf PSRAM zurück, damit der Handshake nicht scheitert.
 */
static void *mbedtlsCalloc(size_t n, size_t size)
{
    constexpr size_t INTERNAL_THRESHOLD = 8192;

    const size_t total = n * size;
    if (total >= INTERNAL_THRESHOLD)
    {
        void *internal = heap_caps_calloc(n, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (internal)
            return internal;
        // interner RAM zu knapp -> auf PSRAM ausweichen
    }

    return heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM);
}

/**
 * @brief Initialisierung.
 */
void setup()
{
    Serial.begin(115200);

    // Bis zum Laden der gespeicherten Freigabe bleiben die Verstärker abgeschaltet.
    gpio_set_level(speakerPins::RS485_CONTROL, 0);
    gpio_set_direction(speakerPins::RS485_CONTROL, GPIO_MODE_OUTPUT);

    // Lenkt die mbedTLS-Allokationen über einen Zuteiler
    mbedtls_platform_set_calloc_free(mbedtlsCalloc, free);

    // Logger initialisieren
    Logger::init(true, true, 512);

    // I2C-Bus starten
    Wire.begin(i2cPins::I2C_MASTER_SDA_IO, i2cPins::I2C_MASTER_SCL_IO);
    Wire.setClock(100000); // 100 kHz für DS1307 RTC mehr kann sie nicht laut Datenblatt
    Wire.setTimeOut(I2C_WIRE_TIMEOUT_MS);

    // Kurz warten, damit die Stromversorgung stabil ist, bevor der IO-Expander initialisiert wird.
    delay(10);

    // CH422G IO-Expander initialisieren
    expander.begin();
    expander.setAllIOOutput();

    // IO-Expander -> SD-Kartenleser
    expander.digitalWrite(expanderPins::SD_CS, LOW);

    // IO-Expander -> LCD/Touch
    expander.digitalWrite(expanderPins::TP_RST, HIGH);
    expander.digitalWrite(expanderPins::LCD_RST, HIGH);
    delay(I2C_TOUCH_READY_DELAY_MS);
    scanStartupI2cBus(Wire);

    // Display-Hintergrundbeleuchtung einschalten
    setBacklight(true);

    // USB_SEL auf LOW -> USB-Funktion aktiv
    expander.digitalWrite(expanderPins::USB_SEL, LOW);

    // SD-Karte initialisieren. Ohne Argumente taktet SD.begin() den Bus mit 4 MHz, das sind
    // rund 400 KB/s. Ein Nachfuellen des Audio-Puffers ueber die vollen 64 KB blockiert die
    // Hauptschleife dann eine Fuenftelsekunde lang, und genau so lange stehen die Wickeldorne
    // der Kassette still. Ueber SPI vertragen Karten ueblicherweise 20 MHz; gelingt das nicht,
    // wird auf den langsamen Takt zurueckgefallen, damit eine zickige Karte nutzbar bleibt.
    constexpr uint32_t SD_SPI_TAKT_SCHNELL = 20000000; ///< Bevorzugter SPI-Takt der SD-Karte
    constexpr uint32_t SD_SPI_TAKT_SICHER = 4000000;   ///< Rueckfalltakt, Vorgabe der Bibliothek

    SPI.setHwCs(false);
    SPI.begin(spiPins::SD_CLK, spiPins::SD_MISO, spiPins::SD_MOSI, spiPins::SD_SS);

    bool sdCardPresent = SD.begin(spiPins::SD_SS, SPI, SD_SPI_TAKT_SCHNELL);
    if (!sdCardPresent)
    {
        LOG_WARNING("SD-Karte bei 20 MHz nicht ansprechbar, Versuch mit 4 MHz");
        sdCardPresent = SD.begin(spiPins::SD_SS, SPI, SD_SPI_TAKT_SICHER);
    }

    // AppState initialisieren und gespeicherte Einstellungen laden. Muss VOR setenv("TZ") erfolgen, damit die Zeitzone aus Preferences geladen wird.
    if (!APP_STATE.begin(true, &Wire))
        LOG_ERROR("AppState konnte nicht initialisiert werden!");
    applySpeakerOutput(APP_STATE.getSpeakersEnabled());

    // Zeitzone aus AppState setzen. TZ-Regel jetzt setzen, damit bereits RTC -> Systemzeit korrekt lokal formatiert wird (Sommer-/Winterzeit automatisch).
    setenv("TZ", APP_STATE.getTimezoneStr(), 1);
    tzset();
    LOG_INFO_F("Zeitzone gesetzt: %s (%s)", APP_STATE.getTimezoneRegion(), APP_STATE.getTimezoneStr());

    // RTC initialisieren. Schlägt begin() fehl, läuft das System ohne gepufferte Zeit weiter und wartet auf NTP.
    rtcInit = rtc.begin();

    if (rtcInit)
    {
        // Nur plausible Zeiten aus der RTC übernehmen. Eine frische oder gepufferte aber nie gestellte RTC liefert 2000-01-01 - das wollen wir nicht als Systemzeit.
        DateTime dt;
        if (rtc.getDateTime(dt) && dt.year >= MIN_VALID_RTC_YEAR)
        {
            rtc.setAsSystemTime();
            LOG_INFO("Systemzeit wurde von der RTC gesetzt.");
        }
        else
        {
            LOG_WARNING("RTC erreichbar, aber Zeit nicht plausibel - warte auf NTP.");
        }
    }
    else
    {
        LOG_WARNING("RTC nicht erreichbar - System läuft ohne gepufferte Zeit.");
    }

    // Display initialisieren. Die Reihenfolge ist hier bewusst spät: Das Panel benutzt mit GPIO 0, 3, 45 und 46 mehrere Strapping-Pins, und ein früheres tft.begin() führte zu falscher Farbausgabe, während der Bildinhalt im Speicher korrekt blieb.
    tft.begin();
    tft.setRotation(2);
    tft.setColorDepth(16);
    tft.setSwapBytes(true);
    tft.clear(TFT_BLACK);

    // Logo zeigen, solange Senderliste und Startbildschirm eingerichtet werden.
    LockScreen::drawBootLogo(tft);

    // Der Touch teilt sich Port 0 mit Wire; beim Einrichten der Anzeige wird der Bus neu aufgesetzt. Der Takt muss für die DS1307 bei 100 kHz bleiben.
    Wire.setClock(100000);

    // Senderliste einmalig laden, damit alle Module sie unabhängig von der Aufrufreihenfolge nutzen können
    if (stationManager.loadStations())
        LOG_SUCCESS_F("Senderliste geladen: %d Sender", stationManager.getStationCount());
    else
        LOG_WARNING("Senderliste konnte nicht geladen werden");

    // MainScreen erstellen (Sperrbildschirm wird intern als Boot-Gate aktiviert)
    mainScreen = new MainScreen();
    mainScreen->init(tft, rtc, stationManager, audio);

    // Systeminformationen im Log ausgeben
    systemInfo();
    sdCardInfo(sdCardPresent);
    rtcInfo(rtcInit);

    // I2S-Audio mit Settings konfigurieren
    audio.setConnectionTimeout(330, 3200);
    audio.settings.DMA_DESC_NUM = 20;//16;
    audio.settings.DMA_FRAME_NUM = 256;
    audio.setPinout(i2sPins::I2S_BCLK, i2sPins::I2S_LRC, i2sPins::I2S_DOUT);
    audio.forceMono(false);
    audio.setBalance(APP_STATE.getCurrentBalance());
    audio.setVolumeSteps(100);
    audio.setVolume(APP_STATE.getCurrentVolume(), 1);
    applyEqFrequencies(APP_STATE.getEqFreqLow(), APP_STATE.getEqFreqPeak(), APP_STATE.getEqFreqHigh());
    applyStereoWidth(APP_STATE.getStereoWidth());

    // AudioManager initialisieren und Callback registrieren
    g_audioManager.init(audio);

    // WLAN starten
    wifiManager.begin();

    // HttpService initialisieren
    if (!HTTP_SERVICE.begin())
        LOG_ERROR("HttpService Init fehlgeschlagen!");
}

/**
 * @brief Hauptschleife.
 */
void loop()
{
    // WiFi-Manager Update
    wifiManager.update();

    // WiFi-Reconnect-Tracking
    static bool wasConnected = false;
    bool isConnected = wifiManager.isConnected();
    if (isConnected && !wasConnected && g_netInitDone)
    {
        // Reconnect erkannt (nicht der erste Connect)
        SYSTEM_STATS.incrementWifiReconnects();
        LOG_INFO("WiFi Reconnect erkannt");
    }
    wasConnected = isConnected;

    // NTP/ Webserver erst beim ersten Connect einmalig initialisieren
    if (!g_netInitDone && wifiManager.isConnected())
    {
        LOG_SUCCESS_F("WiFi verbunden: %s", wifiManager.getCurrentSSID().c_str());

        // Zeitzone und NTP-Server aus AppState konfigurieren
        configTzTime(APP_STATE.getTimezoneStr(), APP_STATE.getNtpServer(), "time.nist.gov", "ptbtime1.ptb.de");

        LOG_INFO_F("NTP konfiguriert: Server=%s, TZ=%s, Intervall=%lu ms",
                   APP_STATE.getNtpServer(),
                   APP_STATE.getTimezoneRegion(),
                   APP_STATE.getNtpSyncInterval());

        // Callback zum RTC synchronisieren
        sntp_set_time_sync_notification_cb(time_sync_callback);
        sntp_set_sync_interval(APP_STATE.getNtpSyncInterval());
        esp_sntp_init();

        // Webserver starten
        setupRoutes(server, tft, stationManager);
        server.begin();
        LOG_SUCCESS("Webserver gestartet auf Port 80");

        // FTP-Server starten (SD-Zugriff über FileZilla & Co.)
        FTP_SERVICE.begin("esp32", "esp32");

        g_netInitDone = true;
    }

    // NTP-Sync-Nachbearbeitung aus der Hauptschleife. Bei erreichbarer RTC wird die Zeit zusätzlich persistiert, andernfalls bleibt die Systemzeit als einzige Quelle.
    if (g_time_needs_sync_to_rtc)
    {
        if (rtcInit)
        {
            rtc.syncFromSystem();
            LOG_SUCCESS("NTP-Zeit synchronisiert (-> RTC aktualisiert).");
        }
        else
        {
            LOG_SUCCESS("NTP-Zeit synchronisiert (ohne RTC).");
        }
        
        g_time_needs_sync_to_rtc = false;
        APP_STATE.setNtpSynced(true);
    }

    // Audio-Transport nur wenn Display an ist, Aufruf nur alle 5 ms.
    if (APP_STATE.isDisplayOn())
    {
        static unsigned long lastAudioLoop = 0;
        if (millis() - lastAudioLoop >= 5)
        {
            audio.loop();
            lastAudioLoop = millis();
        }
    }

    // Status aktualisieren (WLAN/DST etc.)
    updateStatus(APP_STATE.isDisplayOn());

    // SystemStats aktualisieren (sendet SSE-Events automatisch)
    SYSTEM_STATS.update();

    // FTP-Server bedienen (Dateitransfers verarbeiten)
    FTP_SERVICE.handle();

    // MainScreen
    if (mainScreen)
        mainScreen->update();

    // FPS-Log bei Bedarf
    // fps(5); // Messsonde der Visualisierungs-Vergleichsmessung; bei Bedarf aktivieren
}

// ===================================================================
// Radio-/System-Funktionen
// ===================================================================

/**
 * @brief Wechselt die aktuelle Radiostation.
 */
void setNewStation(uint8_t id)
{
    if (wifiManager.isConnected())
    {
        const char *url = stationManager.getStationUrl(id);

        // Falls der gewählte Sender keine gültige URL hat, den nächsten gültigen suchen
        if (url == nullptr || strlen(url) == 0)
        {
            const int count = stationManager.getStationCount();
            for (int i = 0; i < count; ++i)
            {
                const char *candidate = stationManager.getStationUrl(static_cast<uint8_t>(i));
                if (candidate != nullptr && strlen(candidate) > 0)
                {
                    id = static_cast<uint8_t>(i);
                    url = candidate;
                    break;
                }
            }
        }

        // Nur fortfahren, wenn die URL gültig ist
        if (url != nullptr && strlen(url) > 0)
        {
            LOG_PRINTF(Logger::SENDER, "Wechsel zu Sender: %d: %s", id, url);

            APP_STATE.setCurrentRetroStation(id);
            audio.connecttohost(url);

            // Fernsteuerung über Senderwechsel informieren (SSE-Event)
            extern void notifyRadioStationChange(int stationId);
            notifyRadioStationChange(id);
        }
        else
        {
            LOG_ERROR_F("Konnte keine gültige URL für Sender-ID %d finden!", id);
        }
    }
    else
    {
        LOG_ERROR("Keine WiFi-Verbindung, kann Sender nicht wechseln.");
    }
}

/**
 * @brief Aktuellen Stream sofort beenden.
 */
void stopCurrentStation()
{
    audio.stopSong();
}

/**
 * @brief Aktualisiert periodisch System-Status (WLAN, RSSI, DST).
 */
void updateStatus(bool displayOn)
{
    static unsigned long lastStatusUpdate = 0;

    // Im Sleep-Mode nur alle 10 Sekunden, sonst jede Sekunde
    unsigned long interval = displayOn ? 1000 : 10000;

    if (millis() - lastStatusUpdate >= interval)
    {
        lastStatusUpdate = millis();

        // WLAN-Status über WifiManager aktualisieren
        APP_STATE.setWifiConnected(wifiManager.isConnected());

        if (wifiManager.isConnected())
        {
            // Signalstärke berechnen
            int32_t rssi = wifiManager.getRSSI();
            uint8_t quality = WifiManager::rssiToQuality(rssi);
            APP_STATE.setWifiSignalStrength(quality / 10);
        }
        else
        {
            APP_STATE.setWifiSignalStrength(0);
        }

        // Sommerzeit-Status aus der Systemzeit ableiten - funktioniert auch ohne RTC
        struct tm localTime = DS1307_RTC::getCurrentLocalTime();
        APP_STATE.setDstActive(localTime.tm_isdst > 0);
    }
}

/**
 * @brief Schaltet die Display-Hintergrundbeleuchtung ein oder aus.
 */
void setBacklight(bool on)
{
    expander.digitalWrite(expanderPins::LCD_BL, on ? HIGH : LOW);

    if (!on)
    {
        tft.clear(TFT_BLACK);

        // FTP-Server stoppen (kein Transfer im Sleep)
        if (FTP_SERVICE.isRunning())
            FTP_SERVICE.stop();

        esp_wifi_set_ps(WIFI_PS_MAX_MODEM);

        setCpuFrequencyMhz(80);
        LOG_INFO_F("CPU Frequenz: %d MHz", getCpuFrequencyMhz());
    }
    else
    {
        setCpuFrequencyMhz(240);
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        LOG_INFO_F("CPU Frequenz: %d MHz", getCpuFrequencyMhz());

        // FTP-Server neu starten
        if (g_netInitDone && !FTP_SERVICE.isRunning())
            FTP_SERVICE.begin("esp32", "esp32");
    }

    APP_STATE.setDisplayOn(on);
}
