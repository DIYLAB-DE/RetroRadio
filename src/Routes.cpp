#include "Routes.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <cstring>
#include <ctype.h>
#include <strings.h>
#include "index_html_gz.h" // WebUI als PROGMEM gzip-Array
#include <SD.h>
#include <nvs_flash.h>
#include <sys/time.h>

#include <memory>

#include "AppState.h"
#include "DS1307_RTC.h"
#include "Gfx.h"
#include "Logger.h"
#include "StationManager.h"
#include "StringUtils.h"
#include "SystemStats.h"
#include "BaseModule.h" // Für AppModule enum (Fernsteuerung)

// Externe RTC-Instanz (definiert in main.cpp)
extern DS1307_RTC rtc;

// ===================================================================
// Globale Variablen
// ===================================================================

static AsyncWebServer *g_server = nullptr;
static LGFX *g_tft = nullptr;
static StationManager *g_stationManager = nullptr;

/// SSE EventSource für Live-Updates
static AsyncEventSource *g_events = nullptr;

/// Sperrzustand des Displays, gespiegelt für neu verbundene SSE-Clients
static volatile bool g_displayLocked = false;

// ===================================================================
// Zustand der Radio-Fernsteuerung
// ===================================================================

/**
 * @brief Struktur für den aktuellen Radio-Status (SSE-Updates)
 */
struct RadioState
{
    char stationName[64] = "";  ///< Name der aktuellen Station
    char streamTitle[128] = ""; ///< Aktueller Stream-Titel
    char bitrate[16] = "";      ///< Bitrate als String
};

static RadioState g_radioState;

/// Flag: Senderliste wurde per WebUI geändert -> RetroRadio muss neu laden
static volatile bool g_stationsReloadPending = false;

namespace
{
    /**
     * @brief Prüft, ob der angegebene Sprachcode unterstützt wird.
     */
    bool isSupportedUiLanguage(const char *language)
    {
        return language && (strcmp(language, "de") == 0 || strcmp(language, "en") == 0);
    }

    // ===================================================================
    // Geschützte Verzeichnis- und Dateinamen (SD-Karte)
    // ===================================================================

    /// Namen, die von externen Betriebssystemen automatisch auf der SD-Karte
    /// angelegt werden. Werden im Dateimanager-Listing ausgeblendet und können
    /// weder erstellt, umbenannt, gelöscht noch überschrieben werden.
    constexpr const char *PROTECTED_NAMES[] = {
        "System Volume Information", ///< Windows
        "$RECYCLE.BIN",              ///< Windows-Papierkorb
        ".fseventsd",                ///< macOS
        ".Spotlight-V100",           ///< macOS
        ".Trashes",                  ///< macOS
        ".TemporaryItems",           ///< macOS
        ".DS_Store"                  ///< macOS-Metadaten
    };
    constexpr size_t PROTECTED_NAMES_COUNT = sizeof(PROTECTED_NAMES) / sizeof(PROTECTED_NAMES[0]);

    /**
     * @brief Prüft, ob ein einzelner Pfadname (Basename) auf der Schutzliste steht
     *
     * Vergleich ohne Berücksichtigung der Groß-/Kleinschreibung, damit Eingaben in beliebiger Schreibweise zuverlässig erkannt werden.
     */
    bool isProtectedBasename(const char *name)
    {
        if (!name || !*name)
            return false;
        for (size_t i = 0; i < PROTECTED_NAMES_COUNT; ++i)
        {
            if (strcasecmp(name, PROTECTED_NAMES[i]) == 0)
                return true;
        }
        return false;
    }

    /**
     * @brief Prüft, ob in einem vollständigen Pfad ein geschütztes Segment vorkommt
     *
     * Der Pfad wird an Slashes zerlegt und jedes Segment einzeln gegen die Schutzliste geprüft. Auch tief verschachtelte Pfade wie "/foo/System Volume Information/bar" werden so erkannt.
     */
    bool pathHasProtectedSegment(const String &path)
    {
        const int len = path.length();
        int start = 0;
        while (start < len)
        {
            while (start < len && path.charAt(start) == '/')
                ++start;
            int end = start;
            while (end < len && path.charAt(end) != '/')
                ++end;
            int segLen = end - start;
            if (segLen > 0)
            {
                char segment[64];
                if (segLen >= (int)sizeof(segment))
                    segLen = sizeof(segment) - 1;
                for (int i = 0; i < segLen; ++i)
                    segment[i] = path.charAt(start + i);
                segment[segLen] = '\0';
                if (isProtectedBasename(segment))
                    return true;
            }
            start = end;
        }
        return false;
    }

    // ===================================================================
    // Verzögerter Neustart (außerhalb des AsyncTCP-Tasks)
    // ===================================================================

    constexpr uint32_t RESTART_DELAY_MS = 500;  ///< Wartezeit vor dem Neustart, damit die Antwort den Client erreicht

    /**
     * @brief Task-Rumpf für den verzögerten Neustart
     *
     * Wartet RESTART_DELAY_MS und startet dann neu. ESP.restart() kehrt nicht zurück, das nachfolgende vTaskDelete wird nie erreicht.
     */
    void deferredRestartTask(void *)
    {
        vTaskDelay(pdMS_TO_TICKS(RESTART_DELAY_MS));
        ESP.restart();
        vTaskDelete(nullptr);
    }

    /**
     * @brief Stößt einen Neustart an, ohne den aufrufenden Task zu blockieren
     *
     * Der Neustart läuft in einem eigenen Task. Direkt im Body-Handler des Webservers aufgerufen, blockiert ESP.restart() den AsyncTCP-Task und kappt die Verbindung, bevor die Antwort gesendet ist.
     */
    void requestDeferredRestart()
    {
        xTaskCreate(deferredRestartTask, "deferred_restart", 4096, nullptr, 1, nullptr);
    }

    /**
     * @brief Task-Rumpf für den verzögerten Werksreset
     *
     * Wartet, bis die Antwort den Client erreicht hat, setzt dann Senderliste und Einstellungen zurück und startet neu. ESP.restart() kehrt nicht zurück.
     */
    void deferredFactoryResetTask(void *)
    {
        vTaskDelay(pdMS_TO_TICKS(RESTART_DELAY_MS));
        if (g_stationManager)
            g_stationManager->factoryReset();
        nvs_flash_erase();
        ESP.restart();
        vTaskDelete(nullptr);
    }

    /**
     * @brief Stößt einen Werksreset an, ohne den aufrufenden Task zu blockieren
     */
    void requestDeferredFactoryReset()
    {
        xTaskCreate(deferredFactoryResetTask, "factory_reset", 8192, nullptr, 1, nullptr);
    }
}

/// Upload-Schutz: false, wenn der aktuelle Upload wegen geschütztem Pfad abgelehnt wurde
static bool s_fileUploadAllowed = true;

/// Upload-Fehler: true, wenn der aktuelle Upload nicht vollständig auf der SD-Karte gelandet ist (Öffnen, Schreiben, Größenprüfung oder Umbenennen fehlgeschlagen)
static bool s_fileUploadFailed = false;

/**
 * @brief Prüft ob die Senderliste per WebUI geändert wurde und ein Reload nötig ist
 */
bool isStationsReloadPending()
{
    return g_stationsReloadPending;
}

/**
 * @brief Setzt das Stations-Reload-Flag zurück (nach erfolgtem Reload)
 */
void clearStationsReloadPending()
{
    g_stationsReloadPending = false;
}

/**
 * @brief Prüft auf exakt vier Ziffern
 */
static bool isFourDigitPin(const char *pin)
{
    if (!pin || strlen(pin) != 4)
        return false;

    for (int i = 0; i < 4; ++i)
    {
        if (pin[i] < '0' || pin[i] > '9')
            return false;
    }

    return true;
}

/**
 * @brief Prüft einen Netzwerknamen nach den Hostname-Regeln (RFC 1123).
 *
 * Erlaubt sind Buchstaben, Ziffern und der Bindestrich, 1 bis 32 Zeichen, kein Bindestrich am Anfang oder Ende.
 */
static bool isValidHostname(const char *hostname)
{
    if (!hostname)
        return false;

    const size_t len = strlen(hostname);
    if (len < 1 || len > 32)
        return false;

    if (hostname[0] == '-' || hostname[len - 1] == '-')
        return false;

    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(hostname[i]);
        if (!isalnum(c) && c != '-')
            return false;
    }

    return true;
}

/**
 * @brief Konvertiert AppModule enum zu String für JSON
 */
static const char *getModuleName(AppModule module)
{
    switch (module)
    {
    case AppModule::RETRO_RADIO:
        return "RETRO_RADIO";
    case AppModule::SD_PLAYER:
        return "SD_PLAYER";
    case AppModule::CALENDAR:
        return "CALENDAR";
    case AppModule::TODO:
        return "TODO";
    case AppModule::WEATHER:
        return "WEATHER";
    case AppModule::CONTACTS:
        return "CONTACTS";
    case AppModule::BREATHER:
        return "BREATHER";
    case AppModule::WORKSHOP:
        return "WORKSHOP";
    case AppModule::SETTINGS:
        return "SETTINGS";
    case AppModule::OFF:
        return "OFF";
    default:
        return "NONE";
    }
}

/**
 * @brief Baut den Radio-Status als JSON-String (ArduinoJson für korrektes Escaping)
 * @param buffer Ausgabe-Buffer
 * @param bufferSize Größe des Buffers
 * @return Anzahl geschriebener Bytes (ohne Null-Terminator)
 */
static size_t buildRadioStatusJson(char *buffer, size_t bufferSize)
{
    JsonDocument doc;

    doc["module"] = getModuleName(APP_STATE.getActiveModule());
    doc["stationId"] = APP_STATE.getCurrentRetroStation();
    doc["stationName"] = g_radioState.stationName;
    doc["streamTitle"] = g_radioState.streamTitle;
    doc["bitrate"] = g_radioState.bitrate;
    doc["volume"] = APP_STATE.getCurrentVolume();
    doc["muted"] = APP_STATE.getIsMuted();
    doc["balance"] = APP_STATE.getCurrentBalance();
    doc["gainLow"] = APP_STATE.getCurrentGainLowPass();
    doc["gainMid"] = APP_STATE.getCurrentGainBandPass();
    doc["gainHigh"] = APP_STATE.getCurrentGainHighPass();
    doc["forceMono"] = APP_STATE.getForceMono();
    doc["magicEyeStyle"] = APP_STATE.getMagicEyeStyle();

    return serializeJson(doc, buffer, bufferSize);
}

/**
 * @brief Sendet den aktuellen Radio-Status als SSE-Event
 */
void sendRadioSSEEvent()
{
    if (!g_events)
        return;

    char jsonBuffer[640];
    size_t len = buildRadioStatusJson(jsonBuffer, sizeof(jsonBuffer));
    if (len > 0)
    {
        g_events->send(jsonBuffer, "radio", millis());
    }
}

/**
 * @brief Aktualisiert den Stationsnamen und sendet SSE-Event
 */
void updateRadioStation(const char *stationName)
{
    if (stationName)
    {
        strncpy(g_radioState.stationName, stationName, sizeof(g_radioState.stationName) - 1);
        g_radioState.stationName[sizeof(g_radioState.stationName) - 1] = '\0';
    }
    sendRadioSSEEvent();
}

/**
 * @brief Aktualisiert den Stream-Titel und sendet SSE-Event
 */
void updateRadioStreamTitle(const char *streamTitle)
{
    if (streamTitle)
    {
        strncpy(g_radioState.streamTitle, streamTitle, sizeof(g_radioState.streamTitle) - 1);
        g_radioState.streamTitle[sizeof(g_radioState.streamTitle) - 1] = '\0';
    }
    sendRadioSSEEvent();
}

/**
 * @brief Aktualisiert die Bitrate und sendet SSE-Event
 */
void updateRadioBitrate(const char *bitrate)
{
    if (bitrate)
    {
        strncpy(g_radioState.bitrate, bitrate, sizeof(g_radioState.bitrate) - 1);
        g_radioState.bitrate[sizeof(g_radioState.bitrate) - 1] = '\0';
    }
    sendRadioSSEEvent();
}

/**
 * @brief Aktualisiert alle Metadaten auf einmal und sendet EIN SSE-Event
 *
 * Wird von RetroRadio::update() aufgerufen wenn sich Metadaten geändert haben.
 */
void updateRadioMetadata(const char *stationName, const char *streamTitle, const char *bitrate)
{
    if (stationName)
    {
        strncpy(g_radioState.stationName, stationName, sizeof(g_radioState.stationName) - 1);
        g_radioState.stationName[sizeof(g_radioState.stationName) - 1] = '\0';
    }
    if (streamTitle)
    {
        strncpy(g_radioState.streamTitle, streamTitle, sizeof(g_radioState.streamTitle) - 1);
        g_radioState.streamTitle[sizeof(g_radioState.streamTitle) - 1] = '\0';
    }
    if (bitrate)
    {
        strncpy(g_radioState.bitrate, bitrate, sizeof(g_radioState.bitrate) - 1);
        g_radioState.bitrate[sizeof(g_radioState.bitrate) - 1] = '\0';
    }
    sendRadioSSEEvent();
}

/**
 * @brief Benachrichtigt über Senderwechsel
 *
 * Holt den Stationsnamen aus dem StationManager (RAM-Cache), löscht alte Stream-Daten und sendet ein SSE-Event.
 */
void notifyRadioStationChange(int stationId)
{
    // Alte Stream-Daten löschen (neuer Sender = neue Daten)
    g_radioState.streamTitle[0] = '\0';
    g_radioState.bitrate[0] = '\0';
    g_radioState.stationName[0] = '\0';

    // Stationsname aus RAM-Cache holen (statt Flash-Datei zu parsen)
    if (g_stationManager)
    {
        Station station;
        if (g_stationManager->getStation(stationId, station))
        {
            strncpy(g_radioState.stationName, station.name, sizeof(g_radioState.stationName) - 1);
            g_radioState.stationName[sizeof(g_radioState.stationName) - 1] = '\0';
        }
    }

    sendRadioSSEEvent();
}

/**
 * @brief Sendet den aktuellen Display-Sperrzustand als SSE-Event an alle Clients
 */
static void sendLockSSEEvent()
{
    if (!g_events)
        return;

    g_events->send(g_displayLocked ? "{\"locked\":true}" : "{\"locked\":false}", "lock", millis());
}

/**
 * @brief Meldet dem Webinterface den Sperrzustand des Displays
 */
void notifyDisplayLockChanged(bool locked)
{
    g_displayLocked = locked;
    sendLockSSEEvent();
}

/**
 * @brief Struktur zur Verwaltung des Screenshot-Status während des Streaming
 */
struct ScreenshotState
{
    int y;                    ///< Aktuelle Y-Position (bottom-up, beginnt bei HEIGHT-1)
    int x;                    ///< Aktuelle X-Position innerhalb der Zeile für Chunk-basiertes Senden
    bool initialized;         ///< Flag, ob der Header bereits gesendet wurde
    uint8_t header[66];       ///< BMP-Header (54 Bytes) + RGB565-Masken (12 Bytes)
    uint16_t *lineBuf;        ///< Puffer für eine Zeile Pixeldaten (WIDTH * 2 Bytes)
    bool gfxWasBlockedByThis; ///< Flag, ob diese Instanz GFX blockiert hat
    const int imageWidth;     ///< Bildbreite für Pufferberechnungen
    bool lineRead;            ///< Flag, ob die aktuelle Zeile bereits aus GFX gelesen wurde

    /**
     * @brief Konstruktor für ScreenshotState
     * @param w Bildbreite in Pixeln
     * @param h Bildhöhe in Pixeln
     */
    ScreenshotState(int w, int h);

    /**
     * @brief Destruktor für automatisches Ressourcen-Cleanup
     */
    ~ScreenshotState();

    // Verhindere Kopieren und Zuweisen
    ScreenshotState(const ScreenshotState &) = delete;
    ScreenshotState &operator=(const ScreenshotState &) = delete;
};

// Forward-Deklaration
static void createBmpHeader(uint8_t *bmpHeader);

/**
 * @brief Erstellt einen BMP-Header mit RGB565 Farbmasken für Screenshots
 */
static void createBmpHeader(uint8_t *bmpHeader)
{
    const uint16_t WIDTH = 800;
    const uint16_t HEIGHT = 480;
    const uint32_t pixelDataSize = (uint32_t)WIDTH * HEIGHT * 2; // 2 Bytes pro Pixel
    const uint32_t fileSize = 54 + 12 + pixelDataSize;           // Header + Masken + Daten

    // BMP-Header aufbauen
    uint8_t header_template[54] = {
        'B', 'M',
        (uint8_t)(fileSize), (uint8_t)(fileSize >> 8), (uint8_t)(fileSize >> 16), (uint8_t)(fileSize >> 24),
        0, 0, 0, 0,
        54 + 12, 0, 0, 0,
        40, 0, 0, 0,
        (uint8_t)(WIDTH), (uint8_t)(WIDTH >> 8), 0, 0,
        (uint8_t)(HEIGHT), (uint8_t)(HEIGHT >> 8), 0, 0,
        1, 0,
        16, 0,
        3, 0, 0, 0,
        (uint8_t)(pixelDataSize), (uint8_t)(pixelDataSize >> 8), (uint8_t)(pixelDataSize >> 16), (uint8_t)(pixelDataSize >> 24),
        0x13, 0x0B, 0, 0,
        0x13, 0x0B, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0};

    memcpy(bmpHeader, header_template, 54);

    // RGB565 Farbmasken
    uint8_t colorMasks[12] = {
        0x00, 0xF8, 0x00, 0x00, // Rot-Maske   (0xF800)
        0xE0, 0x07, 0x00, 0x00, // Grün-Maske  (0x07E0)
        0x1F, 0x00, 0x00, 0x00  // Blau-Maske  (0x001F)
    };

    memcpy(bmpHeader + 54, colorMasks, 12);
}

// ScreenshotState Implementierung
/**
 * @brief Initialisiert den Zustand für die schrittweise Screenshot-Ausgabe.
 */
ScreenshotState::ScreenshotState(int w, int h)
    : y(h - 1),
      x(0),
      initialized(false),
      lineBuf(nullptr),
      gfxWasBlockedByThis(false),
      imageWidth(w),
      lineRead(false)
{
    // Speicher für eine Zeile allokieren
    lineBuf = (uint16_t *)malloc(imageWidth * 2);
    if (!lineBuf)
        LOG_ERROR("ScreenshotState konnte lineBuf nicht allozieren!");

    createBmpHeader(header);
    LOG_CUSTOM_COLOR("screenshot", "Konstruktor: State erstellt", Logger::CYAN);
}

/**
 * @brief Gibt den Zeilenpuffer des Screenshot-Zustands frei.
 */
ScreenshotState::~ScreenshotState()
{
    // Pufferspeicher freigeben
    if (lineBuf)
    {
        free(lineBuf);
        lineBuf = nullptr;
        LOG_CUSTOM_COLOR("screenshot", "Destruktor: lineBuf freigegeben", Logger::CYAN);
    }

    // GFX freigeben wenn nötig (Fallback für abnormale Beendigung)
    if (gfxWasBlockedByThis && APP_STATE.getBlockGFX())
    {
        APP_STATE.setBlockGFX(false);
        LOG_CUSTOM_COLOR("screenshot", "WARNUNG: GFX im Destruktor freigegeben (Screenshot wurde abgebrochen?)", Logger::YELLOW);
    }
    else if (gfxWasBlockedByThis && !APP_STATE.getBlockGFX())
    {
        LOG_CUSTOM_COLOR("screenshot", "FEHLER: GFX-Status inkonsistent im Destruktor", Logger::RED);
    }

    LOG_CUSTOM_COLOR("screenshot", "Destruktor: State zerstört", Logger::CYAN);
}

// ===================================================================
// SSE-Event Handler
// ===================================================================

/**
 * @brief Handler für neue SSE-Client-Verbindungen
 */
static void onSSEConnect(AsyncEventSourceClient *client)
{
    if (client->lastId())
    {
        LOG_INFO_F("SSE Client reconnected, last ID: %u", client->lastId());
    }
    else
    {
        LOG_INFO("Neuer SSE Client verbunden");
    }

    // Initiale Daten an neuen Client senden
    char jsonBuffer[1200];
    size_t len = SYSTEM_STATS.toJson(jsonBuffer, sizeof(jsonBuffer));
    if (len > 0)
    {
        client->send(jsonBuffer, "stats", millis());
    }

    // Initiale Radio-Daten an neuen Client senden (Fernsteuerung)
    {
        char radioJson[640];
        size_t len = buildRadioStatusJson(radioJson, sizeof(radioJson));
        if (len > 0)
        {
            client->send(radioJson, "radio", millis());
        }
    }

    // Aktuellen Display-Sperrzustand mitschicken, damit ein nach einer Sperre neu verbundener Browser den Stand kennt
    client->send(g_displayLocked ? "{\"locked\":true}" : "{\"locked\":false}", "lock", millis());
}

// ===================================================================
// Route-Setup
// ===================================================================

/**
 * @brief Hauptimplementierung der setupRoutes Funktion
 */
void setupRoutes(AsyncWebServer &server, LGFX &tft, StationManager &stationManager)
{
    // Globale Pointer setzen
    g_server = &server;
    g_tft = &tft;
    g_stationManager = &stationManager;

    // ===================================================================
    // SSE EventSource erstellen und registrieren
    // ===================================================================

    g_events = new AsyncEventSource("/events");
    g_events->onConnect(onSSEConnect);
    g_server->addHandler(g_events);

    // SystemStats mit EventSource verbinden
    SYSTEM_STATS.begin(g_events);

    LOG_SUCCESS("SSE EventSource '/events' registriert");

    // ===================================================================
    // API: System-Status (initiale Daten und manuelle Abfrage)
    // ===================================================================

    g_server->on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        char jsonBuffer[1200];
        size_t len = SYSTEM_STATS.toJson(jsonBuffer, sizeof(jsonBuffer));
        
        if (len > 0)
        {
            request->send(200, "application/json", jsonBuffer);
        }
        else
        {
            request->send(500, "application/json", "{\"error\":\"Statusdaten konnten nicht erstellt werden\"}");
        } });

    LOG_SUCCESS("Route '/api/status' registriert");

    // ===================================================================
    // API: Webinterface-Sperre (PIN-Prüfung)
    // ===================================================================

    g_server->on("/api/lock/verify", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (!request->hasParam("pin", true))
        {
            request->send(400, "application/json", "{\"error\":\"Parameter 'pin' fehlt\"}");
            return;
        }

        String pin = request->getParam("pin", true)->value();
        const bool ok = isFourDigitPin(pin.c_str()) && strcmp(pin.c_str(), APP_STATE.getLockPin()) == 0;

        if (ok)
        {
            LOG_SUCCESS("Webinterface per Code entsperrt");
            // Ist das Display gerade gesperrt, wird es gleich mit entsperrt
            if (g_displayLocked)
                APP_STATE.requestDisplayUnlock();
            request->send(200, "application/json", "{\"ok\":true}");
        }
        else
        {
            LOG_WARNING("Webinterface: falscher Code abgelehnt");
            request->send(200, "application/json", "{\"ok\":false}");
        } });

    LOG_SUCCESS("Route 'POST /api/lock/verify' registriert");

    // ===================================================================
    // API: NTP-Einstellungen (GET/POST)
    // ===================================================================

    // GET: Aktuelle NTP-Einstellungen abrufen
    g_server->on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        char jsonBuffer[900];
        stringutils::safeFormat(jsonBuffer,
            "{"
            "\"hostname\":\"%s\","
            "\"ntpServer\":\"%s\","
            "\"timezoneRegion\":\"%s\","
            "\"timezoneStr\":\"%s\","
            "\"syncIntervalMs\":%lu,"
            "\"newsRegion\":%d,"
            "\"ressort\":\"%s\","
            "\"plz\":\"%s\","
            "\"city\":\"%s\","
            "\"lat\":%.5f,"
            "\"lon\":%.5f,"
            "\"language\":\"%s\""
            "}",
            APP_STATE.getHostname(),
            APP_STATE.getNtpServer(),
            APP_STATE.getTimezoneRegion(),
            APP_STATE.getTimezoneStr(),
            APP_STATE.getNtpSyncInterval(),
            APP_STATE.getNewsRegion(),
            APP_STATE.getNewsRessort(),
            APP_STATE.getPlz(),
            APP_STATE.getWeatherLocationDirect(),
            APP_STATE.getWeatherLatitude(),
            APP_STATE.getWeatherLongitude(),
            APP_STATE.getUiLanguage()
        );
        
        request->send(200, "application/json", jsonBuffer); });

    LOG_SUCCESS("Route 'GET /api/settings' registriert");

    // POST: NTP-Einstellungen speichern
    g_server->on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
                     // Leer - Response wird im Body-Handler gesendet
                 },
                 nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                 {
        // Statischer Buffer für Body-Assembly
        static char* bodyBuffer = nullptr;
        static size_t bodyCapacity = 0;
        static size_t bodyLength = 0;

        // Beim ersten Chunk initialisieren
        if (index == 0) {
            if (bodyBuffer)
                free(bodyBuffer);
            
            bodyCapacity = total + 1;
            bodyBuffer = (char*)malloc(bodyCapacity);
            
            if (!bodyBuffer) {
                LOG_ERROR("Settings: Speicherallokierung fehlgeschlagen");
                request->send(500, "application/json", "{\"error\":\"Zu wenig freier Speicher\"}");
                return;
            }
            
            bodyLength = 0;
            bodyBuffer[0] = '\0';
        }

        // Daten anhängen
        if (bodyBuffer && (bodyLength + len < bodyCapacity)) {
            memcpy(bodyBuffer + bodyLength, data, len);
            bodyLength += len;
            bodyBuffer[bodyLength] = '\0';
        }

        // Alle Daten empfangen - JSON verarbeiten
        if (index + len == total && bodyBuffer) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, bodyBuffer);

            if (err) {
                LOG_ERROR("Settings: Ungültiges JSON empfangen");
                request->send(400, "application/json", "{\"error\":\"Ungültige Daten empfangen\"}");
                free(bodyBuffer);
                bodyBuffer = nullptr;
                return;
            }

            const char *lockPinCurrent = doc["lockPinCurrent"] | "";
            const char *lockPinNew = doc["lockPinNew"] | "";
            const bool lockPinChangeRequested = strlen(lockPinCurrent) > 0 || strlen(lockPinNew) > 0;

            if (lockPinChangeRequested)
            {
                if (!isFourDigitPin(lockPinCurrent) || !isFourDigitPin(lockPinNew))
                {
                    LOG_WARNING("Settings: Lock-PIN-Änderung mit ungültigem Format abgelehnt");
                    request->send(400, "application/json", "{\"error\":\"Code-Eingabe ist ungültig\"}");
                    free(bodyBuffer);
                    bodyBuffer = nullptr;
                    return;
                }

                if (strcmp(lockPinCurrent, APP_STATE.getLockPin()) != 0)
                {
                    LOG_WARNING("Settings: Lock-PIN-Änderung mit falschem aktuellem Code abgelehnt");
                    request->send(403, "application/json", "{\"error\":\"Aktueller Code ist falsch\"}");
                    free(bodyBuffer);
                    bodyBuffer = nullptr;
                    return;
                }
            }

            // Werte aus JSON extrahieren und speichern
            bool hasChanges = false;

            if (doc["hostname"].is<const char*>()) {
                const char* hostname = doc["hostname"];
                if (hostname && strlen(hostname) > 0) {
                    if (!isValidHostname(hostname)) {
                        LOG_WARNING("Settings: Ungültiger Netzwerkname abgelehnt");
                        request->send(400, "application/json", "{\"error\":\"Netzwerkname ist ungültig\"}");
                        free(bodyBuffer);
                        bodyBuffer = nullptr;
                        return;
                    }
                    if (strcmp(hostname, APP_STATE.getHostname()) != 0) {
                        APP_STATE.setHostname(hostname);
                        hasChanges = true;
                    }
                }
            }

            if (doc["ntpServer"].is<const char*>()) {
                const char* server = doc["ntpServer"];
                if (server && strlen(server) > 0) {
                    APP_STATE.setNtpServer(server);
                    hasChanges = true;
                }
            }
            
            if (doc["timezoneRegion"].is<const char*>()) {
                const char* region = doc["timezoneRegion"];
                if (region) {
                    APP_STATE.setTimezoneRegion(region);
                    hasChanges = true;
                }
            }
            
            if (doc["timezoneStr"].is<const char*>()) {
                const char* tzStr = doc["timezoneStr"];
                if (tzStr && strlen(tzStr) > 0) {
                    APP_STATE.setTimezoneStr(tzStr);
                    hasChanges = true;
                }
            }
            
            if (doc["syncIntervalMs"].is<unsigned long>()) {
                unsigned long interval = doc["syncIntervalMs"];
                if (interval >= 60000) {
                    APP_STATE.setNtpSyncInterval(interval);
                    hasChanges = true;
                }
            }

            if (doc["newsRegion"].is<int>()) {
                int region = doc["newsRegion"];
                if (region >= 1 && region <= 16) {
                    APP_STATE.setNewsRegion(region);
                    hasChanges = true;
                }
            }

            if (doc["plz"].is<const char*>()) {
                const char* plz = doc["plz"];
                // Validierung: genau 5 Ziffern
                bool valid = plz && strlen(plz) == 5;
                for (int i = 0; valid && i < 5; ++i)
                    if (plz[i] < '0' || plz[i] > '9') valid = false;
                if (valid) {
                    APP_STATE.setPlz(plz);
                    hasChanges = true;
                }
            }

            if (doc["ressort"].is<const char*>()) {
                APP_STATE.setNewsRessort(doc["ressort"].as<const char*>());
                hasChanges = true;
            }

            if (lockPinChangeRequested)
            {
                APP_STATE.setLockPin(lockPinNew);
                hasChanges = true;
            }

            if (doc["city"].is<const char*>()) {
                const char* city = doc["city"];
                if (city && strlen(city) > 0) {
                    APP_STATE.setWeatherLocation(city);
                    hasChanges = true;
                }
            }

            // Koordinaten: .is<double>() schlägt für JS-Float fehl -> as<double>() mit NAN-Fallback
            {
                const double lat = doc["lat"] | (double)NAN;
                const double lon = doc["lon"] | (double)NAN;
                // Plausibilitätsprüfung: Deutschland-Bounding-Box
                if (!isnan(lat) && !isnan(lon)
                    && lat >= 47.2 && lat <= 55.1
                    && lon >= 5.8  && lon <= 15.1) {
                    APP_STATE.setWeatherLatitude(lat);
                    APP_STATE.setWeatherLongitude(lon);
                    LOG_INFO_F("Standort gespeichert: lat=%.5f lon=%.5f", lat, lon);
                    hasChanges = true;
                }
            }

            if (doc["language"].is<const char *>()) {
                const char *language = doc["language"];
                if (!isSupportedUiLanguage(language)) {
                    LOG_WARNING("Settings: Ungültige UI-Sprache abgelehnt");
                    request->send(400, "application/json", "{\"error\":\"Sprache ist ungültig\"}");
                    free(bodyBuffer);
                    bodyBuffer = nullptr;
                    return;
                }

                if (strcmp(APP_STATE.getUiLanguage(), language) != 0) {
                    APP_STATE.setUiLanguage(language);
                    hasChanges = true;
                }
            }

            // restart-Flag aus doc lesen, bevor der zugehörige Puffer freigegeben wird
            const bool doRestart = doc["restart"].is<bool>() ? doc["restart"].as<bool>() : false;

            free(bodyBuffer);
            bodyBuffer = nullptr;

            if (hasChanges) {
                LOG_SUCCESS("NTP-Einstellungen gespeichert");

                if (doRestart) {
                    request->send(200, "application/json", "{\"success\":true,\"message\":\"Einstellungen gespeichert. Neustart wird durchgeführt...\"}");
                    requestDeferredRestart();
                } else {
                    request->send(200, "application/json", "{\"success\":true,\"message\":\"Einstellungen gespeichert\"}");
                }
            } else {
                request->send(200, "application/json", "{\"success\":true,\"message\":\"Keine Änderungen\"}");
            }
        } });

    LOG_SUCCESS("Route 'POST /api/settings' registriert");

    // ===================================================================
    // API: Cloud-Anbindung (GET/POST)
    // ===================================================================

    // GET: Aktuelle Cloud-Anbindung abrufen
    g_server->on("/api/google-api", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        JsonDocument doc;
        doc["calendarUrl"] = APP_STATE.getCalendarScriptUrl();
        doc["todoUrl"] = APP_STATE.getTodoScriptUrl();
        doc["contactsUrl"] = APP_STATE.getContactsScriptUrl();
        doc["openSkyClientId"] = APP_STATE.getOpenSkyClientId();
        doc["openSkyClientSecret"] = APP_STATE.getOpenSkyClientSecret();

        char jsonBuffer[1280];
        serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
        jsonBuffer[sizeof(jsonBuffer) - 1] = '\0';

        request->send(200, "application/json", jsonBuffer); });

    LOG_SUCCESS("Route 'GET /api/google-api' registriert");

    // POST: Cloud-Anbindung speichern
    g_server->on("/api/google-api", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
                     // Leer - Response wird im Body-Handler gesendet
                 },
                 nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                 {
        static char* bodyBuffer = nullptr;
        static size_t bodyCapacity = 0;
        static size_t bodyLength = 0;

        if (index == 0) {
            if (bodyBuffer) free(bodyBuffer);
            bodyCapacity = total + 1;
            bodyBuffer = (char*)malloc(bodyCapacity);
            if (!bodyBuffer) {
                request->send(500, "application/json", "{\"error\":\"Zu wenig freier Speicher\"}");
                return;
            }
            bodyLength = 0;
            bodyBuffer[0] = '\0';
        }

        if (bodyBuffer && (bodyLength + len < bodyCapacity)) {
            memcpy(bodyBuffer + bodyLength, data, len);
            bodyLength += len;
            bodyBuffer[bodyLength] = '\0';
        }

        if (index + len == total && bodyBuffer) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, bodyBuffer);

            if (err) {
                request->send(400, "application/json", "{\"error\":\"Ungültige Daten empfangen\"}");
                free(bodyBuffer);
                bodyBuffer = nullptr;
                return;
            }

            bool hasChanges = false;

            if (doc["calendarUrl"].is<const char*>()) {
                APP_STATE.setCalendarScriptUrl(doc["calendarUrl"]);
                hasChanges = true;
            }

            if (doc["todoUrl"].is<const char*>()) {
                APP_STATE.setTodoScriptUrl(doc["todoUrl"]);
                hasChanges = true;
            }

            if (doc["contactsUrl"].is<const char*>()) {
                APP_STATE.setContactsScriptUrl(doc["contactsUrl"]);
                hasChanges = true;
            }

            if (doc["openSkyClientId"].is<const char*>()) {
                APP_STATE.setOpenSkyClientId(doc["openSkyClientId"]);
                hasChanges = true;
            }

            if (doc["openSkyClientSecret"].is<const char*>()) {
                APP_STATE.setOpenSkyClientSecret(doc["openSkyClientSecret"]);
                hasChanges = true;
            }

            free(bodyBuffer);
            bodyBuffer = nullptr;

            if (hasChanges) {
                LOG_SUCCESS("Cloud-Anbindung gespeichert");
                request->send(200, "application/json", "{\"success\":true,\"message\":\"Cloud-Anbindung gespeichert\"}");
            } else {
                request->send(200, "application/json", "{\"success\":true,\"message\":\"Keine Änderungen\"}");
            }
        } });

    LOG_SUCCESS("Route 'POST /api/google-api' registriert");

    // POST: Controller neu starten
    g_server->on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        LOG_WARNING("Neustart angefordert via API");
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Neustart wird durchgeführt...\"}");
        requestDeferredRestart(); });

    LOG_SUCCESS("Route 'POST /api/reboot' registriert");

    // ===================================================================
    // API: Manuelle Zeit setzen
    // ===================================================================

    g_server->on("/api/settime", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
                     // Leer - Response wird im Body-Handler gesendet
                 },
                 nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                 {
        // Statischer Buffer für Body-Assembly
        static char* bodyBuffer = nullptr;
        static size_t bodyCapacity = 0;
        static size_t bodyLength = 0;

        // Beim ersten Chunk initialisieren
        if (index == 0) {
            if (bodyBuffer)
                free(bodyBuffer);
            
            bodyCapacity = total + 1;
            bodyBuffer = (char*)malloc(bodyCapacity);
            
            if (!bodyBuffer) {
                LOG_ERROR("SetTime: Speicherallokierung fehlgeschlagen");
                request->send(500, "application/json", "{\"error\":\"Zu wenig freier Speicher\"}");
                return;
            }
            
            bodyLength = 0;
            bodyBuffer[0] = '\0';
        }

        // Daten anhängen
        if (bodyBuffer && (bodyLength + len < bodyCapacity)) {
            memcpy(bodyBuffer + bodyLength, data, len);
            bodyLength += len;
            bodyBuffer[bodyLength] = '\0';
        }

        // Alle Daten empfangen - JSON verarbeiten
        if (index + len == total && bodyBuffer) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, bodyBuffer);

            // Buffer sofort freigeben
            char datetimeStr[32] = {0};
            if (!err && doc["datetime"].is<const char*>()) {
                strncpy(datetimeStr, doc["datetime"], sizeof(datetimeStr) - 1);
            }
            free(bodyBuffer);
            bodyBuffer = nullptr;

            if (err) {
                LOG_ERROR("SetTime: Ungültiges JSON empfangen");
                request->send(400, "application/json", "{\"error\":\"Ungültige Daten empfangen\"}");
                return;
            }

            if (strlen(datetimeStr) == 0) {
                LOG_ERROR("SetTime: Kein datetime-Wert empfangen");
                request->send(400, "application/json", "{\"error\":\"Kein datetime-Wert\"}");
                return;
            }

            // datetime-local Format parsen: "YYYY-MM-DDTHH:MM:SS" oder "YYYY-MM-DDTHH:MM"
            int year, month, day, hour, minute, second = 0;
            int parsed = sscanf(datetimeStr, "%d-%d-%dT%d:%d:%d", 
                               &year, &month, &day, &hour, &minute, &second);

            if (parsed < 5) {
                LOG_ERROR_F("SetTime: Ungültiges Format: %s", datetimeStr);
                request->send(400, "application/json", "{\"error\":\"Ungültiges Datumsformat\"}");
                return;
            }

            // Validierung
            if (year < 2000 || year > 2099 || month < 1 || month > 12 || 
                day < 1 || day > 31 || hour < 0 || hour > 23 || 
                minute < 0 || minute > 59 || second < 0 || second > 59) {
                LOG_ERROR_F("SetTime: Werte außerhalb des gültigen Bereichs: %s", datetimeStr);
                request->send(400, "application/json", "{\"error\":\"Ungültige Datums-/Zeitwerte\"}");
                return;
            }

            // Systemzeit setzen (über struct tm -> time_t -> settimeofday)
            struct tm timeinfo = {0};
            timeinfo.tm_year = year - 1900;  // Jahre seit 1900
            timeinfo.tm_mon = month - 1;     // 0-basiert
            timeinfo.tm_mday = day;
            timeinfo.tm_hour = hour;
            timeinfo.tm_min = minute;
            timeinfo.tm_sec = second;
            timeinfo.tm_isdst = -1;          // Automatisch ermitteln

            time_t epochTime = mktime(&timeinfo);
            if (epochTime == -1) {
                LOG_ERROR("SetTime: mktime fehlgeschlagen");
                request->send(500, "application/json", "{\"error\":\"Zeitkonvertierung fehlgeschlagen\"}");
                return;
            }

            // Systemzeit setzen
            struct timeval tv = { .tv_sec = epochTime, .tv_usec = 0 };
            if (settimeofday(&tv, nullptr) != 0) {
                LOG_ERROR("SetTime: settimeofday fehlgeschlagen");
                request->send(500, "application/json", "{\"error\":\"Systemzeit konnte nicht gesetzt werden\"}");
                return;
            }

            // RTC synchronisieren
            if (!rtc.syncFromSystem()) {
                LOG_WARNING("SetTime: RTC-Sync fehlgeschlagen (Systemzeit wurde trotzdem gesetzt)");
                // Kein Fehler zurückgeben, da Systemzeit gesetzt wurde
            }

            // NTP-Sync-Status zurücksetzen (da manuell gesetzt)
            APP_STATE.setNtpSynced(false);

            LOG_SUCCESS_F("Manuelle Zeit gesetzt: %04d-%02d-%02d %02d:%02d:%02d", 
                         year, month, day, hour, minute, second);

            request->send(200, "application/json", "{\"success\":true,\"message\":\"Zeit wurde gesetzt\"}");
        } });

    LOG_SUCCESS("Route 'POST /api/settime' registriert");

    // ===================================================================
    // API: Sender-Verwaltung (GET/POST)
    // ===================================================================

    // GET: Aktuelle Senderliste aus RAM serialisieren
    g_server->on("/api/stations", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        if (!g_stationManager) {
            request->send(500, "application/json", "{\"error\":\"Senderverwaltung nicht bereit\"}");
            return;
        }

        // Sicherheitsnetz: Falls Stationen noch nicht geladen (Race Condition beim Boot)
        if (g_stationManager->getStationCount() == 0) {
            LOG_WARNING("GET /api/stations: Stationen noch nicht geladen, lade jetzt...");
            g_stationManager->loadStations();
        }

        // JSON aus RAM-Array generieren (PSRAM-Buffer für ~24 KB JSON)
        g_stationManager->withStations([request](const Station* stations, int count) {
            JsonDocument doc;
            JsonArray arr = doc.to<JsonArray>();

            for (int i = 0; i < count; i++) {
                JsonObject obj = arr.add<JsonObject>();
                obj["shortName"] = stations[i].shortName;
                obj["name"]      = stations[i].name;
                obj["url"]       = stations[i].url;
                obj["preset"]    = stations[i].preset;
            }

            // In PSRAM-Buffer serialisieren
            size_t jsonLen = measureJson(doc);
            char* buffer = static_cast<char*>(
                heap_caps_malloc(jsonLen + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

            if (!buffer) {
                buffer = static_cast<char*>(malloc(jsonLen + 1));
            }

            if (!buffer) {
                request->send(500, "application/json", "{\"error\":\"Zu wenig freier Speicher\"}");
                return;
            }

            serializeJson(doc, buffer, jsonLen + 1);
            
            // AsyncWebServer kopiert den Buffer intern
            request->send(200, "application/json", buffer);
            free(buffer);
        }); });

    LOG_SUCCESS("Route 'GET /api/stations' registriert");

    // POST: Senderliste speichern (JSON -> StationManager RAM -> Flash-Partition)
    g_server->on("/api/stations", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
                     // Leer - Response wird im Body-Handler gesendet
                 },
                 nullptr, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                 {
        // Statischer Buffer für Body-Assembly
        static char* bodyBuffer = nullptr;
        static size_t bodyCapacity = 0;
        static size_t bodyLength = 0;

        // Beim ersten Chunk initialisieren
        if (index == 0) {
            if (bodyBuffer)
                free(bodyBuffer);

            bodyCapacity = total + 1;
            bodyBuffer = (char*)heap_caps_malloc(bodyCapacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!bodyBuffer) {
                bodyBuffer = (char*)malloc(bodyCapacity);
            }

            if (!bodyBuffer) {
                LOG_ERROR("Stations API: Speicherallokierung fehlgeschlagen");
                request->send(500, "application/json", "{\"error\":\"Zu wenig freier Speicher\"}");
                return;
            }

            bodyLength = 0;
            bodyBuffer[0] = '\0';
        }

        // Daten anhängen
        if (bodyBuffer && (bodyLength + len < bodyCapacity)) {
            memcpy(bodyBuffer + bodyLength, data, len);
            bodyLength += len;
            bodyBuffer[bodyLength] = '\0';
        }

        // Alle Daten empfangen - JSON verarbeiten
        if (index + len == total && bodyBuffer) {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, bodyBuffer);

            if (err) {
                LOG_ERROR("Stations API: Ungültiges JSON empfangen");
                request->send(400, "application/json", "{\"error\":\"Ungültige Daten empfangen\"}");
                free(bodyBuffer);
                bodyBuffer = nullptr;
                return;
            }

            // JSON -> StationManager RAM-Array aktualisieren
            if (g_stationManager) {
                g_stationManager->withStationsMutable([&doc](Station* stations, int count) {
                    JsonArray arr = doc.as<JsonArray>();
                    int idx = 0;

                    for (JsonObject obj : arr) {
                        if (idx >= MAX_STATIONS) break;

                        stations[idx] = Station();  // Reset
                        stringutils::safeCopy(stations[idx].shortName, obj["shortName"] | "");
                        stringutils::safeCopy(stations[idx].name,      obj["name"]      | "");
                        stringutils::safeCopy(stations[idx].url,       obj["url"]       | "");
                        stations[idx].preset = obj["preset"] | false;
                        idx++;
                    }

                    // Restliche Slots leeren
                    for (int i = idx; i < MAX_STATIONS; i++) {
                        stations[i] = Station();
                    }
                });

                // In Flash-Partition speichern
                if (!g_stationManager->saveStations()) {
                    LOG_ERROR("Stations API: Speichern in Flash-Partition fehlgeschlagen");
                    request->send(500, "application/json", "{\"error\":\"Senderliste konnte nicht gespeichert werden\"}");
                    free(bodyBuffer);
                    bodyBuffer = nullptr;
                    return;
                }

                g_stationsReloadPending = true;
            }

            LOG_SUCCESS("Senderliste gespeichert (RAM + Flash-Partition)");
            request->send(200, "application/json", "{\"success\":true}");

            free(bodyBuffer);
            bodyBuffer = nullptr;
        } });

    LOG_SUCCESS("Route 'POST /api/stations' registriert");

    // POST: Senderliste auf Werkseinstellungen zurücksetzen
    g_server->on("/api/radio/reset-stations", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (!g_stationManager) {
            request->send(500, "application/json", "{\"error\":\"Senderverwaltung nicht bereit\"}");
            return;
        }

        if (g_stationManager->factoryReset()) {
            g_stationsReloadPending = true;
            LOG_SUCCESS("Senderliste auf Werkseinstellungen zurückgesetzt");
            request->send(200, "application/json", "{\"success\":true,\"message\":\"Senderliste zurückgesetzt\"}");
        } else {
            LOG_ERROR("Factory-Reset der Senderliste fehlgeschlagen");
            request->send(500, "application/json", "{\"error\":\"Reset fehlgeschlagen\"}");
        } });

    LOG_SUCCESS("Route 'POST /api/radio/reset-stations' registriert");

    // ===================================================================
    // API: Radio-Fernsteuerung
    // ===================================================================

    // GET: Aktueller Radio-Status
    g_server->on("/api/radio/status", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        char jsonBuffer[640];
        size_t len = buildRadioStatusJson(jsonBuffer, sizeof(jsonBuffer));
        if (len > 0) {
            request->send(200, "application/json", jsonBuffer);
        } else {
            request->send(500, "application/json", "{\"error\":\"Statusdaten konnten nicht erstellt werden\"}");
        } });

    LOG_SUCCESS("Route 'GET /api/radio/status' registriert");

    // GET: Favoriten/Presets aus StationManager RAM abrufen
    g_server->on("/api/radio/presets", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        if (!g_stationManager) {
            request->send(500, "application/json", "{\"error\":\"Senderverwaltung nicht bereit\"}");
            return;
        }

        // Sicherheitsnetz: Falls Stationen noch nicht geladen
        if (g_stationManager->getStationCount() == 0) {
            g_stationManager->loadStations();
        }

        // Presets direkt aus RAM-Array filtern
        g_stationManager->withStations([request](const Station* stations, int count) {
            JsonDocument presetsDoc;
            JsonArray presets = presetsDoc["presets"].to<JsonArray>();

            for (int i = 0; i < count; i++) {
                if (stations[i].preset) {
                    JsonObject preset = presets.add<JsonObject>();
                    preset["id"] = i;
                    preset["shortName"] = (strlen(stations[i].shortName) > 0)
                                           ? stations[i].shortName
                                           : stations[i].name;
                    preset["name"] = stations[i].name;
                }
            }

            String response;
            serializeJson(presetsDoc, response);
            request->send(200, "application/json", response);
        }); });

    LOG_SUCCESS("Route 'GET /api/radio/presets' registriert");

    // POST: Lautstärke setzen
    g_server->on("/api/radio/volume", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (!request->hasParam("volume", true)) {
            request->send(400, "application/json", "{\"error\":\"Parameter 'volume' fehlt\"}");
            return;
        }
        
        int volume = request->getParam("volume", true)->value().toInt();
        volume = constrain(volume, 0, 100);
        
        // Handler aufrufen (sendet SSE-Event)
        extern void handleVolumeChanged(uint8_t volume);
        handleVolumeChanged(volume);
        
        request->send(200, "application/json", "{\"success\":true}"); });

    LOG_SUCCESS("Route 'POST /api/radio/volume' registriert");

    // POST: Mute setzen
    g_server->on("/api/radio/mute", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (!request->hasParam("mute", true)) {
            request->send(400, "application/json", "{\"error\":\"Parameter 'mute' fehlt\"}");
            return;
        }
        
        String muteStr = request->getParam("mute", true)->value();
        bool mute = (muteStr == "true" || muteStr == "1");
        
        extern void handleMuteChanged(bool mute);
        extern void handleVolumeChanged(uint8_t volume);
        extern void handleVolumeBeforeMute(uint8_t volume);
        
        if (mute && !APP_STATE.getIsMuted()) {
            // Muten: Lautstärke merken
            handleVolumeBeforeMute(APP_STATE.getCurrentVolume());
            handleMuteChanged(true);
            handleVolumeChanged(0);
        } else if (!mute && APP_STATE.getIsMuted()) {
            // Unmuten: Lautstärke wiederherstellen
            handleMuteChanged(false);
            handleVolumeChanged(APP_STATE.getVolumeBeforeMute());
        }
        
        // Handler senden bereits SSE-Events
        request->send(200, "application/json", "{\"success\":true}"); });

    LOG_SUCCESS("Route 'POST /api/radio/mute' registriert");

    // POST: Balance setzen
    g_server->on("/api/radio/balance", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (!request->hasParam("balance", true)) {
            request->send(400, "application/json", "{\"error\":\"Parameter 'balance' fehlt\"}");
            return;
        }
        
        int balance = request->getParam("balance", true)->value().toInt();
        balance = constrain(balance, -16, 16);
        
        // Handler aufrufen (sendet SSE-Event)
        extern void handleBalanceChanged(int8_t balance);
        handleBalanceChanged(balance);
        
        request->send(200, "application/json", "{\"success\":true}"); });

    LOG_SUCCESS("Route 'POST /api/radio/balance' registriert");

    // POST: Equalizer/Tone setzen
    g_server->on("/api/radio/tone", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        extern void handleGainLowPassChanged(int8_t gain);
        extern void handleGainBandPassChanged(int8_t gain);
        extern void handleGainHighPassChanged(int8_t gain);
        
        // Handler senden jeweils SSE-Events
        if (request->hasParam("low", true)) {
            int low = constrain(request->getParam("low", true)->value().toInt(), -12, 12);
            handleGainLowPassChanged(low);
        }

        if (request->hasParam("mid", true)) {
            int mid = constrain(request->getParam("mid", true)->value().toInt(), -12, 12);
            handleGainBandPassChanged(mid);
        }

        if (request->hasParam("high", true)) {
            int high = constrain(request->getParam("high", true)->value().toInt(), -12, 12);
            handleGainHighPassChanged(high);
        }
        
        request->send(200, "application/json", "{\"success\":true}"); });

    LOG_SUCCESS("Route 'POST /api/radio/tone' registriert");

    // POST: Station wechseln
    g_server->on("/api/radio/station", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        // Nur erlauben wenn RetroRadio aktiv
        if (APP_STATE.getActiveModule() != AppModule::RETRO_RADIO) {
            request->send(403, "application/json", "{\"error\":\"Senderwechsel nur bei aktivem RetroRadio möglich\"}");
            return;
        }
        
        if (!request->hasParam("id", true)) {
            request->send(400, "application/json", "{\"error\":\"Parameter 'id' fehlt\"}");
            return;
        }
        
        int stationId = request->getParam("id", true)->value().toInt();
        if (stationId < 0 || stationId > 69) {
            request->send(400, "application/json", "{\"error\":\"Ungültige Station-ID (0-69)\"}");
            return;
        }

        // Nur das Ziel setzen: Stopp, Zeigerfahrt und Senderstart bei Ankunft übernimmt RetroRadio::update. Ein connecttohost aus dem Webserver-Task liefe parallel zur Zeigerfahrt: der Sender spielte zu früh und die Verbindungsarbeit ließe den Zeiger stocken.
        const uint8_t target = static_cast<uint8_t>(stationId);
        if (target != APP_STATE.getCurrentRetroStation()) {
            APP_STATE.setCurrentRetroStation(target);
            notifyRadioStationChange(stationId);
        }
        else if (APP_STATE.getStationSetDone()) {
            // Bereits laufender Sender: sofort neu verbinden, wie erneutes Antippen am Gerät
            extern void setNewStation(uint8_t id);
            setNewStation(target);
        }

        request->send(200, "application/json", "{\"success\":true}"); });

    LOG_SUCCESS("Route 'POST /api/radio/station' registriert");

    // ===================================================================
    // Statische Dateien
    // ===================================================================

    // Route: Hauptseite (gzip-komprimiert aus PROGMEM)
    g_server->on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        AsyncWebServerResponse *response = request->beginResponse_P(
            200, "text/html", INDEX_HTML_GZ, INDEX_HTML_GZ_SIZE);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response); });

    // Route: Werksreset (NVS + Senderliste). Bewusst POST: ein GET kann von Browser-Prefetch oder Linkvorschau ausgelöst werden. Der Reset selbst läuft verzögert in einem eigenen Task, damit die Antwort den Client noch erreicht.
    g_server->on("/factoryreset", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        LOG_WARNING("Werksreset angefordert!");
        request->send(200, "text/plain", "Werksreset wird durchgeführt...");
        requestDeferredFactoryReset(); });

    // ===================================================================
    // Screenshot-Route
    // ===================================================================

    g_server->on("/screenshot", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        const uint16_t WIDTH = 800;
        const uint16_t HEIGHT = 480;

        LOG_CUSTOM_COLOR("screenshot", "Screenshot-Request empfangen", Logger::CYAN);

        // State-Objekt mit shared_ptr verwalten
        auto state = std::make_shared<ScreenshotState>(WIDTH, HEIGHT);
        
        if (!state->lineBuf) {
            LOG_ERROR("Screenshot: Speicherallokierung fehlgeschlagen");
            request->send(500, "text/plain", "Zu wenig freier Speicher");
            return;
        }

        // GFX blockieren für Screenshot
        APP_STATE.setBlockGFX(true);
        state->gfxWasBlockedByThis = true;
        LOG_CUSTOM_COLOR("screenshot", "GFX blockiert für Screenshot", Logger::CYAN);

        // Chunked Response starten
        AsyncWebServerResponse *response = request->beginChunkedResponse(
            "image/bmp",
            [state](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
                const uint16_t WIDTH = 800;
                const uint16_t HEIGHT = 480;
                size_t bytesWritten = 0;
                
                // Phase 1: Header senden
                if (!state->initialized) {
                    size_t headerSize = 66;
                    if (maxLen >= headerSize) {
                        memcpy(buffer, state->header, headerSize);
                        state->initialized = true;
                        return headerSize;
                    }
                    return 0;
                }
                
                // Phase 2: Pixeldaten senden
                while (state->y >= 0 && bytesWritten < maxLen) {
                    // Zeile lesen wenn nötig
                    if (!state->lineRead && g_tft) {
                        g_tft->readRect(0, state->y, WIDTH, 1, state->lineBuf);
                        state->lineRead = true;
                    }
                    
                    // Bytes aus aktueller Zeile senden
                    size_t bytesInLine = (WIDTH - state->x) * 2;
                    size_t canWrite = maxLen - bytesWritten;
                    size_t toWrite = (bytesInLine < canWrite) ? bytesInLine : canWrite;
                    
                    // Nur ganze Pixel
                    toWrite = (toWrite / 2) * 2;
                    
                    if (toWrite > 0) {
                        memcpy(buffer + bytesWritten, ((uint8_t*)state->lineBuf) + (state->x * 2), toWrite);
                        bytesWritten += toWrite;
                        state->x += toWrite / 2;
                    }
                    
                    // Zeile fertig?
                    if (state->x >= WIDTH) {
                        state->y--;
                        state->x = 0;
                        state->lineRead = false;
                    } else {
                        break;
                    }
                }
                
                // Fertig?
                if (state->y < 0) {
                    // GFX wieder freigeben
                    if (state->gfxWasBlockedByThis) {
                        APP_STATE.setBlockGFX(false);
                        state->gfxWasBlockedByThis = false;
                        LOG_CUSTOM_COLOR("screenshot", "Screenshot fertig, GFX freigegeben", Logger::GREEN);
                    }
                    return bytesWritten;
                }
                
                return bytesWritten;
            });

        response->addHeader("Content-Disposition", "inline; filename=\"screenshot.bmp\"");
        request->send(response); });

    // ===================================================================
    // SD-Karten API
    // ===================================================================

    // Route: SD-Karten-Info
    g_server->on("/api/sdcard/info", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        uint8_t cardType = SD.cardType();
        
        if (cardType == CARD_NONE) {
            request->send(200, "application/json", "{\"available\":false}");
            return;
        }
        
        const char* cardTypeName;
        switch(cardType) {
            case CARD_MMC:  cardTypeName = "MMC";   break;
            case CARD_SD:   cardTypeName = "SD";    break;
            case CARD_SDHC: cardTypeName = "SDHC";  break;
            default:        cardTypeName = "Unbekannt";
        }
        
        uint64_t totalBytes = SD.totalBytes();
        uint64_t usedBytes = SD.usedBytes();
        uint64_t freeBytes = totalBytes - usedBytes;
        
        char response[256];
        stringutils::safeFormat(response,
            "{\"available\":true,\"cardType\":\"%s\",\"totalBytes\":%llu,\"usedBytes\":%llu,\"freeBytes\":%llu}",
            cardTypeName,
            (unsigned long long)totalBytes,
            (unsigned long long)usedBytes,
            (unsigned long long)freeBytes);
        
        request->send(200, "application/json", response); });

    // Route: Dateien auflisten
    g_server->on("/api/files/list", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
        if (SD.cardType() == CARD_NONE)
        {
            request->send(200, "application/json", "{\"available\":false,\"files\":[]}");
            return;
        }

        String path = "/";
        if (request->hasParam("path"))
            path = request->getParam("path")->value();

        if (!SD.exists(path)) {
            request->send(404, "application/json", "{\"error\":\"Pfad nicht gefunden\"}");
            return;
        }

        File dir = SD.open(path);
        if (!dir.isDirectory()) {
            request->send(400, "application/json", "{\"error\":\"Kein Verzeichnis\"}");
            dir.close();
            return;
        }

        JsonDocument doc;
        doc["available"] = true;
        JsonArray files = doc["files"].to<JsonArray>();

        File entry = dir.openNextFile();
        while (entry) {
            if (!isProtectedBasename(entry.name())) {
                JsonObject fileObj = files.add<JsonObject>();
                fileObj["name"] = String(entry.name());
                fileObj["size"] = entry.size();
                fileObj["isDirectory"] = entry.isDirectory();
            }
            entry = dir.openNextFile();
        }
        dir.close();

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
        LOG_INFO_F("Verzeichnis aufgelistet: %s (%d Einträge)", path.c_str(), files.size()); });

    // Route: Neues Verzeichnis erstellen
    g_server->on("/api/files/mkdir", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (!request->hasParam("path", true)) {
            request->send(400, "application/json", "{\"error\":\"Kein Pfad angegeben\"}");
            return;
        }

        String path = request->getParam("path", true)->value();

        if (pathHasProtectedSegment(path)) {
            LOG_WARNING_F("mkdir abgelehnt (geschützter Pfad): %s", path.c_str());
            request->send(403, "application/json", "{\"error\":\"Dieser Name ist geschützt\"}");
            return;
        }

        if (SD.exists(path)) {
            LOG_WARNING_F("Pfad existiert bereits: %s", path.c_str());
            request->send(409, "application/json", "{\"error\":\"Pfad existiert bereits\"}");
            return;
        }

        if (SD.mkdir(path)) {
            LOG_SUCCESS_F("Verzeichnis erstellt: %s", path.c_str());
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            LOG_ERROR_F("Fehler beim Erstellen von: %s", path.c_str());
            request->send(500, "application/json", "{\"error\":\"Verzeichnis konnte nicht erstellt werden\"}");
        } });

    // Route: Datei oder Verzeichnis löschen
    g_server->on("/api/files/delete", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (!request->hasParam("path", true)) {
            request->send(400, "application/json", "{\"error\":\"Kein Pfad angegeben\"}");
            return;
        }

        String path = request->getParam("path", true)->value();

        if (pathHasProtectedSegment(path)) {
            LOG_WARNING_F("delete abgelehnt (geschützter Pfad): %s", path.c_str());
            request->send(403, "application/json", "{\"error\":\"Dieser Name ist geschützt\"}");
            return;
        }

        if (!SD.exists(path)) {
            LOG_ERROR_F("Pfad nicht gefunden: %s", path.c_str());
            request->send(404, "application/json", "{\"error\":\"Pfad nicht gefunden\"}");
            return;
        }

        File file = SD.open(path);
        bool isDir = file.isDirectory();
        file.close();

        bool success = false;
        if (isDir) {
            success = SD.rmdir(path);
        } else {
            success = SD.remove(path);
        }

        if (success) {
            LOG_SUCCESS_F("Gelöscht: %s", path.c_str());
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            LOG_ERROR_F("Fehler beim Löschen von: %s", path.c_str());
            request->send(500, "application/json", "{\"error\":\"Konnte nicht gelöscht werden\"}");
        } });

    // Route: Datei oder Verzeichnis umbenennen
    g_server->on("/api/files/rename", HTTP_POST, [](AsyncWebServerRequest *request)
                 {
        if (!request->hasParam("oldPath", true) || !request->hasParam("newPath", true)) {
            request->send(400, "application/json", "{\"error\":\"Fehlende Parameter\"}");
            return;
        }

        String oldPath = request->getParam("oldPath", true)->value();
        String newPath = request->getParam("newPath", true)->value();

        if (pathHasProtectedSegment(oldPath) || pathHasProtectedSegment(newPath)) {
            LOG_WARNING_F("rename abgelehnt (geschützter Pfad): %s -> %s", oldPath.c_str(), newPath.c_str());
            request->send(403, "application/json", "{\"error\":\"Dieser Name ist geschützt\"}");
            return;
        }

        if (!SD.exists(oldPath)) {
            LOG_ERROR_F("Alter Pfad nicht gefunden: %s", oldPath.c_str());
            request->send(404, "application/json", "{\"error\":\"Alter Pfad nicht gefunden\"}");
            return;
        }

        if (SD.exists(newPath)) {
            LOG_WARNING_F("Neuer Pfad existiert bereits: %s", newPath.c_str());
            request->send(409, "application/json", "{\"error\":\"Neuer Pfad existiert bereits\"}");
            return;
        }

        if (SD.rename(oldPath, newPath)) {
            LOG_SUCCESS_F("Umbenannt: %s -> %s", oldPath.c_str(), newPath.c_str());
            request->send(200, "application/json", "{\"success\":true}");
        } else {
            LOG_ERROR_F("Fehler beim Umbenennen: %s -> %s", oldPath.c_str(), newPath.c_str());
            request->send(500, "application/json", "{\"error\":\"Umbenennen fehlgeschlagen\"}");
        } });

    // Route: Datei-Upload mit Integritätsprüfung
    g_server->on(
        "/api/files/upload",
        HTTP_POST,
        // Request-Handler (wird nach Upload aufgerufen)
        [](AsyncWebServerRequest *request)
        {
            if (!s_fileUploadAllowed)
            {
                s_fileUploadAllowed = true;
                request->send(403, "application/json", "{\"error\":\"Dieser Name ist geschützt\"}");
                return;
            }
            if (s_fileUploadFailed)
            {
                s_fileUploadFailed = false;
                request->send(500, "application/json", "{\"error\":\"Datei konnte nicht vollständig gespeichert werden\"}");
                return;
            }
            request->send(200, "application/json", "{\"success\":true}");
        },
        // Upload-Handler (während Upload)
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            static File uploadFile;
            static String tmpFilePath;
            static String finalFilePath;
            static size_t expectedSize = 0;
            static size_t bytesWritten = 0;

            // Upload-Start
            if (index == 0)
            {
                s_fileUploadAllowed = true;
                s_fileUploadFailed = false;
                bytesWritten = 0;

                // Pfad aus URL-Query-Parameter holen
                String path = "/";
                if (request->hasParam("path"))
                    path = request->getParam("path")->value();

                // Erwartete Dateigröße holen
                expectedSize = 0;
                if (request->hasParam("size"))
                    expectedSize = request->getParam("size")->value().toInt();

                // Stelle sicher, dass Pfad mit / endet
                if (!path.endsWith("/"))
                    path += "/";

                // Temporäre Datei erstellen
                finalFilePath = path + filename;
                tmpFilePath = finalFilePath + ".tmp";

                // Schutz-Check: geschützte Namen im Ziel-Pfad oder Dateinamen ablehnen
                if (pathHasProtectedSegment(finalFilePath))
                {
                    LOG_WARNING_F("Upload abgelehnt (geschützter Pfad): %s", finalFilePath.c_str());
                    s_fileUploadAllowed = false;
                    uploadFile = File();
                    return;
                }

                LOG_INFO_F("Upload Start: %s -> %s (erwartete Größe: %d Bytes)", filename.c_str(), tmpFilePath.c_str(), expectedSize);

                // Falls alte .tmp existiert, löschen
                if (SD.exists(tmpFilePath))
                {
                    SD.remove(tmpFilePath);
                    LOG_WARNING_F("Alte temporäre Datei gelöscht: %s", tmpFilePath.c_str());
                }

                // Öffne temporäre Datei zum Schreiben; die Antwort sendet erst der Request-Handler nach dem letzten Datenblock
                uploadFile = SD.open(tmpFilePath, FILE_WRITE);
                if (!uploadFile)
                {
                    LOG_ERROR_F("Fehler beim Öffnen der Upload-Datei: %s", tmpFilePath.c_str());
                    s_fileUploadFailed = true;
                    return;
                }
            }

            // Daten schreiben
            if (uploadFile && len)
            {
                size_t written = uploadFile.write(data, len);
                bytesWritten += written;

                if (written != len)
                {
                    LOG_ERROR_F("Schreibfehler: Nur %d von %d Bytes geschrieben (gesamt bisher: %d)", written, len, bytesWritten);
                    s_fileUploadFailed = true;
                }
            }

            // Upload-Ende
            if (final)
            {
                if (uploadFile)
                {
                    uploadFile.close();

                    // Integritätsprüfung: Geschriebene Bytes mit erwarteter Größe vergleichen
                    size_t actualSize = bytesWritten;
                    bool sizeValid = (expectedSize == 0) || (actualSize == expectedSize);

                    if (sizeValid)
                    {
                        // Prüfung erfolgreich -> Umbenennen
                        if (SD.exists(finalFilePath))
                        {
                            SD.remove(finalFilePath);
                            LOG_WARNING_F("Existierende Datei überschrieben: %s", finalFilePath.c_str());
                        }

                        if (SD.rename(tmpFilePath, finalFilePath))
                        {
                            LOG_SUCCESS_F("Upload erfolgreich: %s (%d Bytes, verifiziert ✓)", filename.c_str(), actualSize);
                        }
                        else
                        {
                            LOG_ERROR_F("Fehler beim Umbenennen: %s -> %s", tmpFilePath.c_str(), finalFilePath.c_str());
                            SD.remove(tmpFilePath);
                            s_fileUploadFailed = true;
                        }
                    }
                    else
                    {
                        // Prüfung fehlgeschlagen -> .tmp löschen
                        LOG_ERROR_F("Integritätsprüfung fehlgeschlagen: %s (erwartet: %d Bytes, erhalten: %d Bytes)", filename.c_str(), expectedSize, actualSize);
                        SD.remove(tmpFilePath);
                        LOG_WARNING_F("Fehlerhafte temporäre Datei gelöscht: %s", tmpFilePath.c_str());
                        s_fileUploadFailed = true;
                    }

                    // Reset für nächsten Upload
                    bytesWritten = 0;
                }
            }
        });

    LOG_SUCCESS("Alle Routen inkl. File-Manager und SSE konfiguriert");
}
