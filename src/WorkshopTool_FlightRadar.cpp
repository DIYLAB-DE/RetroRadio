#include "WorkshopTool_FlightRadar.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "AppState.h"
#include "HttpService.h"
#include "I18n.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Theme.h"
#include "Workshop.h"

#include "fonts/AtkinsonHyperlegibleNextMedium11.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Anonymer Namespace
// ===================================================================

namespace
{
    constexpr uint32_t AUTH_FETCH_INTERVAL_MS = 22000UL;   ///< 4000 Abrufe pro Tag mit Puffer
    constexpr uint32_t ANON_FETCH_INTERVAL_MS = 220000UL;  ///< 400 Abrufe pro Tag mit Puffer
    constexpr uint32_t WORKER_SLEEP_MS = 200UL;            ///< Schlafintervall des Workers
    constexpr uint32_t INITIAL_FETCH_DELAY_MS = 1500UL;    ///< Startpuffer vor dem ersten Abruf
    constexpr uint32_t WORKER_STACK_BYTES = 20480UL;       ///< Stackgröße für Netzwerk und JSON
    constexpr uint32_t ANIMATION_FRAME_MS = 33UL;          ///< Zielperiode zwischen zwei Animationsframes, gemessen ab Frame-Start
    constexpr uint32_t PROGRESS_UPDATE_INTERVAL_MS = 1000UL; ///< Aktualisierungsintervall des Abrufbalkens
    constexpr uint32_t SNAPSHOT_LOCK_MS = 20UL;            ///< Wartezeit des Zeichenpfads auf den Schnappschuss
    constexpr uint32_t DIALOG_LOCK_MS = 100UL;             ///< Wartezeit des Detaildialogs auf den Schnappschuss
    constexpr uint32_t LIST_REDRAW_DELAY_MS = 300UL;       ///< Abstand zwischen Dateneingang und Neuzeichnen der Liste
    constexpr uint8_t LIST_PUSH_BANDS = 4;                 ///< Anzahl der Frames, auf die die Listenübertragung verteilt wird

    constexpr uint8_t DEFAULT_RADIUS_KM = 10;               ///< Standard-Suchradius
    constexpr uint8_t RADIUS_VALUES[4] = {5, 10, 20, 50};   ///< auswählbare Suchradien
    constexpr const char *RADIUS_LABELS[4] = {"5 km", "10 km", "20 km", "50 km"}; ///< Beschriftungen der Radius-Buttons
    constexpr int MAX_AIRCRAFT = 32;                       ///< Feste Obergrenze für den lokalen Schnappschuss
    constexpr int PUSH_RECT_CAP = 2 + 4 * MAX_AIRCRAFT;    ///< Kapazität der Push-Liste: Sweep, Liste, je Flugzeug alt und neu
    constexpr int LIST_ROWS = 7;                           ///< Sichtbare Zeilen in der rechten Liste
    constexpr int LIST_HEADER_H = 36;                      ///< Höhe des Listenkopfs
    constexpr int LIST_ROW_H = 40;                         ///< Höhe einer Listenzeile
    constexpr int ICAO_CAP = 8;                            ///< ICAO24 inklusive Nullterminator
    constexpr int CALLSIGN_CAP = 12;                       ///< Callsign inklusive Nullterminator
    constexpr int ORIGIN_COUNTRY_CAP = 32;                 ///< Herkunftsland inklusive Nullterminator
    constexpr int URL_CAP = 192;                           ///< OpenSky-URL mit Bounding-Box
    constexpr int TOKEN_BODY_CAP = 512;                    ///< OAuth-Formular-Body
    constexpr int TOKEN_CAP = 1536;                        ///< Bearer-Token-Puffer
    constexpr int AUTH_HEADER_CAP = TOKEN_CAP + 16;        ///< Authorization-Header
    constexpr int RESPONSE_PREVIEW_CAP = 96;               ///< Länge des Diagnose-Ausschnitts
    constexpr size_t OPENSKY_RESPONSE_COPY_BYTES = 256UL * 1024UL; ///< Eigener OpenSky-Antwortpuffer
    constexpr float BLEND_SPEED = 0.15f;                   ///< Geschwindigkeit für weiches Einblenden neuer Positionen
    constexpr float SCAN_PERIOD_MS = 24000.0f;             ///< Dauer eines Scan-Umlaufs
    constexpr float SCAN_SWEEP_DEG = 22.0f;                ///< Winkelbreite des Verlaufs
    constexpr float FULL_CIRCLE_DEG = 360.0f;              ///< Vollkreis in Grad
    constexpr float FULL_CIRCLE_RAD = 6.2831853f;          ///< Vollkreis in Radiant
    constexpr int SCAN_SWEEP_INSET = 2;                    ///< Abstand des Sweeps zum Außenkreis
    constexpr int SCAN_SWEEP_SPRITE_PAD = 1;               ///< Transparenter Zusatzrand des Keil-Sprites in Pixeln
    constexpr int OVERLAY_PIXEL_WORDS = 3;                 ///< Einträge je Overlay-Pixel: X, Y, Farbe
    constexpr int RADAR_DIRTY_PAD = 3;                     ///< Sicherheitsrand um dynamische Bereiche in Pixeln
    constexpr float SCAN_FRONT_THICKNESS = 1.2f;           ///< Linienstärke der Scanline
    constexpr float RADAR_AXIS_THICKNESS = 0.8f;           ///< Linienstärke der Achsen
    constexpr float RADAR_RING_THICKNESS = 2.1f;           ///< Linienstärke der Radarringe
    constexpr float RADAR_OUTER_RING_THICKNESS = 2.4f;     ///< Linienstärke des Außenrings
    constexpr int RADAR_CENTER_DOT_R = 4;                  ///< Radius des Mittelpunktpunkts
    constexpr int COMPASS_INSET = 13;                      ///< Abstand der Richtungszeichen vom Außenkreis
    constexpr int COMPASS_GLYPH_HALF = 6;                  ///< Halbe Kantenlänge der Richtungszeichen
    constexpr float COMPASS_AXIS_LABEL_GAP = 4.0f;         ///< Abstand zwischen Achsenlinie und Richtungszeichen
    constexpr uint8_t SCAN_SWEEP_MIN_BRIGHTNESS = 2;       ///< Mindesthelligkeit am Trail-Ende
    constexpr uint8_t SCAN_SWEEP_MAX_BRIGHTNESS = 95;      ///< Maximalhelligkeit im Verlauf
    constexpr uint8_t SCAN_FRONT_BRIGHTNESS = 230;         ///< Helligkeit der Frontlinie
    constexpr int AIRCRAFT_SPRITE_SIZE = 21;               ///< Seitenlänge des Flugzeugmarkers
    constexpr int AIRCRAFT_MARKER_PAD = 1;                 ///< zusätzlicher Zeichenrand des Markers
    constexpr int AIRCRAFT_MARKER_DRAW_SIZE = AIRCRAFT_SPRITE_SIZE + AIRCRAFT_MARKER_PAD * 2; ///< Seitenlänge der Markermaske
    constexpr int AIRCRAFT_MARKER_PIXELS = AIRCRAFT_MARKER_DRAW_SIZE * AIRCRAFT_MARKER_DRAW_SIZE; ///< Pixelanzahl einer Markermaske
    constexpr int AIRCRAFT_MARKER_STEPS = 72;              ///< Anzahl vorberechneter Kurswinkel
    constexpr size_t AIRCRAFT_MARKER_CACHE_BYTES = AIRCRAFT_MARKER_STEPS * AIRCRAFT_MARKER_PIXELS; ///< Speicherbedarf des Markercaches
    constexpr int AIRCRAFT_LABEL_W = 128;                  ///< Breite einer Flugzeugbeschriftungsmaske
    constexpr int AIRCRAFT_LABEL_H = 48;                   ///< Höhe einer Flugzeugbeschriftungsmaske
    constexpr int AIRCRAFT_LABEL_PIXELS = AIRCRAFT_LABEL_W * AIRCRAFT_LABEL_H; ///< Pixelanzahl einer Beschriftungsmaske
    constexpr int AIRCRAFT_LABEL_NAME_Y = 0;               ///< Y-Position der Flugzeugkennung in der Maske
    constexpr int AIRCRAFT_LABEL_DETAIL1_Y = 14;           ///< Y-Position der ersten Detailzeile
    constexpr int AIRCRAFT_LABEL_DETAIL2_Y = 25;           ///< Y-Position der zweiten Detailzeile
    constexpr uint8_t AIRCRAFT_LABEL_ALPHA_MASK = 0x7F;    ///< Alphaanteil der Beschriftungsmaske
    constexpr uint8_t AIRCRAFT_LABEL_DETAIL_FLAG = 0x80;   ///< Farbmarkierung für Detailzeilen
    constexpr float AIRCRAFT_MARKER_STEP_DEG = FULL_CIRCLE_DEG / AIRCRAFT_MARKER_STEPS; ///< Winkelabstand der Markermasken
    constexpr uint8_t AIRCRAFT_MARKER_ALPHA_MASK = 0x7F;   ///< Alphaanteil der Markermaske
    constexpr int AIRCRAFT_LABEL_OFFSET_X = 12;            ///< Textabstand rechts vom Markerzentrum
    constexpr float MS_TO_KMH = 3.6f;                      ///< Umrechnung von m/s in km/h
    constexpr float BACKWARD_BLEND_EPS_KM = 0.05f;         ///< Toleranz gegen sichtbares Zurückziehen

    constexpr uint16_t C_RADAR_BG = 0x0000;                ///< Radar-Innenfläche
    constexpr uint16_t C_RADAR_GRID = 0x0640;              ///< Außenkreis
    constexpr uint16_t C_RADAR_GRID_MID = 0x0320;          ///< mittleres Raster
    constexpr uint16_t C_RADAR_GRID_DIM = 0x0200;          ///< inneres Raster
    constexpr uint16_t C_RADAR_AXIS = 0x0320;              ///< Achsenlinien
    constexpr uint16_t C_RADAR_COMPASS = 0x07E0;           ///< Richtungsbuchstaben
    const uint16_t C_AIRCRAFT_MARKER = lgfx::color888(0, 238, 0); ///< Flugzeugsymbol
    constexpr uint16_t C_AIRCRAFT_CALLSIGN = 0x7D7A;       ///< Flugzeugkennung
    constexpr uint16_t C_AIRCRAFT_DETAIL = 0xDE54;         ///< Flugzeugdetails
    constexpr uint16_t C_PANEL_BG = 0x2945;                ///< Hintergrund der Statuskarte
    constexpr uint16_t C_PANEL_DIM = 0xB5B6;               ///< Gedimmte Werte-Farbe
    constexpr uint16_t C_BUTTON_ACTIVE_FILL = 0x328A;      ///< Aktiver Button-Hintergrund
    constexpr uint16_t C_BUTTON_ACTIVE_TEXT = 0xFFFF;      ///< Aktiver Button-Text
    constexpr uint16_t C_BUTTON_ACTIVE_OUTLINE = 0x328A;   ///< Aktiver Button-Rand
    constexpr uint16_t C_BUTTON_INACTIVE_FILL = 0x0000;    ///< Inaktiver Button-Hintergrund
    constexpr uint16_t C_BUTTON_INACTIVE_TEXT = 0x7BEF;    ///< Inaktiver Button-Text
    constexpr uint16_t C_BUTTON_INACTIVE_OUTLINE = 0x39E8; ///< Inaktiver Button-Rand
    constexpr uint16_t C_PROGRESS_BAR = 0x8010;            ///< Farbe des Abrufbalkens
    constexpr int PROGRESS_BAR_W = 1;                      ///< Breite des Abrufbalkens
    constexpr int FOOTER_BTN_H = 44;                       ///< Höhe der Footer-Buttons
    constexpr int FOOTER_BTN_R = 6;                        ///< Eckenradius der Footer-Buttons
    constexpr int FOOTER_BTN_GAP = 5;                      ///< Abstand zwischen Footer-Buttons
    constexpr int FOOTER_BTN_MARGIN = 2;                   ///< Linker Rand im Footer
    constexpr int RADIUS_BTN_W = 64;                       ///< Breite eines Radius-Buttons
    constexpr int LABEL_MODE_BTN_W = 48;                   ///< Breite des Label-Modus-Buttons
    constexpr int HELP_BTN_W = 48;                         ///< Breite des Hilfe-Buttons
    constexpr uint8_t LABEL_MODE_NAME = 1;                 ///< Symbol und Bezeichnung
    constexpr uint8_t LABEL_MODE_SYMBOL = 2;               ///< Nur Symbol
    constexpr int DIALOG_HELP_W = 700;                     ///< Breite des Hilfe-Dialogs
    constexpr int DIALOG_HELP_X = (WorkshopCfg::SCREEN_W - DIALOG_HELP_W) / 2; ///< X-Position des Hilfe-Dialogs
    constexpr int DIALOG_AIRCRAFT_W = 480;                 ///< Breite des Flugzeugdetail-Dialogs
    constexpr int DIALOG_AIRCRAFT_X = (WorkshopCfg::SCREEN_W - DIALOG_AIRCRAFT_W) / 2; ///< X-Position des Flugzeugdetail-Dialogs

    /**
     * @brief Status des gemeinsamen Radar-Schnappschusses.
     */
    enum class RadarStatus : uint8_t
    {
        Idle,       ///< Noch kein Abruf gestartet
        Fetching,   ///< Abruf läuft
        Ready,      ///< Daten vorhanden
        Empty,      ///< Abruf erfolgreich, aber keine Flugzeuge im Radius
        Offline,    ///< Keine Netzwerkverbindung
        Error       ///< Abruf oder Parsing fehlgeschlagen
    };

    /**
     * @brief Kompakter Flugzeugdatensatz für Anzeige und Projektion.
     */
    struct Aircraft
    {
        char icao24[ICAO_CAP] = {};        ///< ICAO24-Adresse
        char callsign[CALLSIGN_CAP] = {}; ///< Callsign oder ICAO24
        char originCountry[ORIGIN_COUNTRY_CAP] = {}; ///< Herkunftsland der ICAO24-Adresse
        double latitude = 0.0;             ///< letzte API-Breite
        double longitude = 0.0;            ///< letzte API-Länge
        double blendFromLat = 0.0;         ///< Startposition für weiches Einblenden
        double blendFromLon = 0.0;         ///< Startposition für weiches Einblenden
        float blendAlpha = 1.0f;           ///< 1.0 = Einblenden abgeschlossen
        float eastKm = 0.0f;               ///< Ostversatz zum Mittelpunkt
        float northKm = 0.0f;              ///< Nordversatz zum Mittelpunkt
        float distanceKm = 0.0f;           ///< Entfernung zum Mittelpunkt
        float trackDeg = 0.0f;             ///< Kurs in Grad, 0 = Norden
        float velocityMps = 0.0f;          ///< Geschwindigkeit in m/s
        int32_t timePosition = 0;          ///< Zeitpunkt der letzten Position
        int32_t lastContact = 0;           ///< letzter Transponderkontakt
        uint32_t lastSeenMs = 0;           ///< lokaler Zeitpunkt der letzten API-Sichtung
        uint32_t lastTickMs = 0;           ///< lokaler Zeitpunkt der letzten Projektion
        int altitudeM = -1;                ///< Höhe in Meter, -1 bei unbekannt
        bool onGround = false;             ///< true wenn am Boden
        bool present = false;              ///< true wenn im letzten API-Schnappschuss enthalten
    };

    /**
     * @brief Gemeinsamer statischer Zustand außerhalb der Werkzeug-Instanz.
     */
    struct SharedRadarState
    {
        SemaphoreHandle_t mutex = nullptr;             ///< Schutz für Schnappschussdaten
        TaskHandle_t task = nullptr;                   ///< Worker-Task
        std::atomic<bool> active { false };            ///< Werkzeug aktiv
        std::atomic<bool> abort { false };             ///< Abbruchanforderung für HttpService
        std::atomic<uint32_t> fetchCycleStartMs { 0 }; ///< Startzeit des aktuellen Abrufintervalls
        std::atomic<uint32_t> nextFetchMs { 0 };       ///< nächster Abrufzeitpunkt
        Aircraft aircraft[MAX_AIRCRAFT] = {};          ///< letzter Schnappschuss
        int aircraftCount = 0;                         ///< Anzahl gültiger Einträge
        RadarStatus status = RadarStatus::Idle;        ///< letzter Status
        uint32_t dataVersion = 0;                      ///< Version des letzten API-Schnappschusses
        char bearerToken[TOKEN_CAP] = {};              ///< OpenSky OAuth-Token
        std::atomic<uint32_t> tokenExpiryMs { 0 };     ///< Ablaufzeit des Tokens mit Puffer, 0 = kein Token
        std::atomic<uint8_t> radiusKm { DEFAULT_RADIUS_KM }; ///< Suchradius des aktiven Werkzeugs
    };

    /**
     * @brief Gepufferte Alpha-Maske für eine Flugzeugbeschriftung.
     */
    struct LabelCacheEntry
    {
        uint8_t *mask = nullptr;             ///< Alpha-Maske in PSRAM oder Fallback-Heap
        char icao24[ICAO_CAP] = {};          ///< Zuordnung zum Flugzeug
        char callsign[CALLSIGN_CAP] = {};    ///< gerenderte Flugzeugkennung
        char detail1[24] = {};               ///< gerenderte erste Detailzeile
        char detail2[24] = {};               ///< gerenderte zweite Detailzeile
        int width = 0;                       ///< genutzte Maskenbreite
        int height = 0;                      ///< genutzte Maskenhöhe
        uint8_t labelMode = 0;               ///< gerenderter Beschriftungsmodus
        bool active = false;                 ///< Eintrag gehört zum aktuellen Schnappschuss
        bool ready = false;                  ///< Maske ist gültig
    };

    /**
     * @brief Rechteck im Work-Sprite, Breite oder Höhe 0 bedeutet leer.
     */
    struct RadarRect
    {
        int16_t x = 0; ///< Linke Kante
        int16_t y = 0; ///< Obere Kante
        int16_t w = 0; ///< Breite in Pixeln
        int16_t h = 0; ///< Höhe in Pixeln
    };

    /**
     * @brief Gezeichnete Bereiche eines Flugzeugs in einem Frame.
     */
    struct AircraftRects
    {
        char icao24[ICAO_CAP] = {}; ///< Zuordnung zum Flugzeug
        RadarRect marker;           ///< Bereich des Markers
        RadarRect label;            ///< Bereich der Beschriftung
    };

    /**
     * @brief Alle dynamischen Bereiche eines Radarframes.
     */
    struct FrameRects
    {
        RadarRect sweep;                      ///< Bereich von Sweep-Keil und Frontlinie
        AircraftRects aircraft[MAX_AIRCRAFT]; ///< Bereiche je Flugzeug in Listenreihenfolge
        int aircraftCount = 0;                ///< Anzahl gültiger Flugzeugeinträge
    };

    SharedRadarState g_radar;
    float g_sweepAngleDeg = 0.0f;
    uint32_t g_sweepLastMs = 0;
    uint8_t *g_aircraftMarkerCache = nullptr;
    bool g_aircraftMarkerCacheReady = false;
    bool g_aircraftMarkerCacheFailed = false;
    char *g_openSkyResponseCopy = nullptr;
    Aircraft g_uiAircraftScratch[MAX_AIRCRAFT] = {};
    LabelCacheEntry g_labelCache[MAX_AIRCRAFT] = {};
    Gfx_Sprite *g_labelRenderSprite = nullptr;
    char g_listRowIcao[LIST_ROWS][ICAO_CAP] = {}; ///< ICAO24 der zuletzt vollständig gezeichneten Listenzeilen
    FrameRects g_prevFrameRects;          ///< Dynamische Bereiche des zuletzt gezeichneten Frames
    FrameRects g_frameRects;              ///< Dynamische Bereiche des aktuellen Frames
    RadarRect g_pushRects[PUSH_RECT_CAP]; ///< Bereiche, die zum Display übertragen werden
    int g_pushRectCount = 0;              ///< Anzahl gültiger Einträge in g_pushRects

    /**
     * @brief Prüft einen Radar-Mittelpunkt.
     */
    bool isValidRadarCenter(double lat, double lon)
    {
        return !std::isnan(lat) && !std::isnan(lon) &&
               lat >= -90.0 && lat <= 90.0 &&
               lon >= -180.0 && lon <= 180.0 &&
               (lat != 0.0 || lon != 0.0);
    }

    /**
     * @brief Lädt den Radar-Mittelpunkt aus dem gespeicherten Wetterstandort.
     */
    bool loadRadarCenter(double &lat, double &lon)
    {
        if (!APP_STATE.hasWeatherCoordinates())
            return false;

        lat = APP_STATE.getWeatherLatitude();
        lon = APP_STATE.getWeatherLongitude();
        return isValidRadarCenter(lat, lon);
    }

    /**
     * @brief Kopiert den Anzeigenamen des Radar-Mittelpunkts.
     */
    void copyRadarLocationName(char *dst, size_t dstSize)
    {
        if (!dst || dstSize == 0)
            return;

        APP_STATE.getWeatherLocation(dst, dstSize);
        if (dst[0] != '\0')
            return;

        double lat = 0.0;
        double lon = 0.0;
        if (loadRadarCenter(lat, lon))
        {
            stringutils::safeFormat(dst, dstSize, "%.4f, %.4f", lat, lon);
            return;
        }

        I18n::format(dst, dstSize, WorkshopKey::WORKSHOP_FLIGHT_RADAR_LOCATION_UNKNOWN);
    }

    /**
     * @brief Erstellt den gemeinsamen Mutex bei Bedarf.
     */
    bool ensureMutex()
    {
        if (g_radar.mutex)
            return true;

        g_radar.mutex = xSemaphoreCreateMutex();
        if (!g_radar.mutex)
            LOG_ERROR("FlightRadar: Mutex konnte nicht erstellt werden");

        return g_radar.mutex != nullptr;
    }

    /**
     * @brief Erstellt den eigenen OpenSky-Antwortpuffer bei Bedarf.
     */
    bool ensureOpenSkyResponseCopy()
    {
        if (g_openSkyResponseCopy)
            return true;

        g_openSkyResponseCopy = static_cast<char *>(heap_caps_malloc(OPENSKY_RESPONSE_COPY_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!g_openSkyResponseCopy)
        {
            LOG_ERROR("FlightRadar: OpenSky-Antwortpuffer konnte nicht erstellt werden");
            return false;
        }

        g_openSkyResponseCopy[0] = '\0';
        return true;
    }

    /**
     * @brief Sperrt den gemeinsamen Zustand kurzzeitig.
     */
    bool lockRadar(uint32_t timeoutMs = 20)
    {
        if (!ensureMutex())
            return false;

        return xSemaphoreTake(g_radar.mutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
    }

    /**
     * @brief Gibt die Sperre des gemeinsamen Zustands frei.
     */
    void unlockRadar()
    {
        if (g_radar.mutex)
            xSemaphoreGive(g_radar.mutex);
    }

    /**
     * @brief Setzt den Status.
     */
    void setStatus(RadarStatus status)
    {
        if (!lockRadar())
            return;

        g_radar.status = status;
        if (status != RadarStatus::Ready && status != RadarStatus::Fetching)
            g_radar.aircraftCount = 0;
        unlockRadar();
    }

    /**
     * @brief Kopiert einen Text ohne führende und nachlaufende Leerzeichen.
     */
    void copyTrimmed(char *dst, size_t dstSize, const char *src, const char *fallback)
    {
        if (!dst || dstSize == 0)
            return;

        dst[0] = '\0';
        const char *source = (src && src[0]) ? src : fallback;
        if (!source)
            return;

        while (*source == ' ')
            ++source;

        size_t len = strlen(source);
        while (len > 0 && source[len - 1] == ' ')
            --len;

        if (len >= dstSize)
            len = dstSize - 1;

        memcpy(dst, source, len);
        dst[len] = '\0';
    }

    /**
     * @brief Begrenzung für float-Werte.
     */
    float clampf(float value, float lo, float hi)
    {
        if (value < lo)
            return lo;
        if (value > hi)
            return hi;
        return value;
    }

    /**
     * @brief Prüft, ob OpenSky-Zugangsdaten gespeichert wurden.
     */
    bool hasOpenSkyCredentials()
    {
        return APP_STATE.hasOpenSkyCredentials();
    }

    /**
     * @brief Prüft, ob aktuell ein gültiger OpenSky-Kontozugang genutzt werden kann.
     */
    bool hasValidOpenSkyBearerToken()
    {
        if (!hasOpenSkyCredentials())
            return false;

        const uint32_t expiryMs = g_radar.tokenExpiryMs.load();
        return expiryMs != 0 && static_cast<int32_t>(millis() - expiryMs) < 0;
    }

    /**
     * @brief Liefert das zum aktuellen Auth-Zustand passende Abrufintervall.
     */
    uint32_t fetchIntervalMs()
    {
        if (!hasOpenSkyCredentials())
            return ANON_FETCH_INTERVAL_MS;

        if (hasValidOpenSkyBearerToken())
            return AUTH_FETCH_INTERVAL_MS;

        return ANON_FETCH_INTERVAL_MS;
    }

    /**
     * @brief Prüft einen Flugradar-Suchradius.
     */
    bool isValidRadiusKm(uint8_t radiusKm)
    {
        for (uint8_t i = 0; i < 4; ++i)
        {
            if (RADIUS_VALUES[i] == radiusKm)
                return true;
        }

        return false;
    }

    /**
     * @brief Begrenzung auf einen erlaubten Flugradar-Suchradius.
     */
    uint8_t normalizeRadiusKm(uint8_t radiusKm)
    {
        return isValidRadiusKm(radiusKm) ? radiusKm : DEFAULT_RADIUS_KM;
    }

    /**
     * @brief Liefert den Index eines Flugradar-Suchradius.
     */
    uint8_t radiusIndex(uint8_t radiusKm)
    {
        const uint8_t normalized = normalizeRadiusKm(radiusKm);
        for (uint8_t i = 0; i < 4; ++i)
        {
            if (RADIUS_VALUES[i] == normalized)
                return i;
        }

        return 1;
    }

    /**
     * @brief Aktualisiert den gemeinsamen Suchradius.
     */
    void configureSharedRadiusKm(uint8_t radiusKm)
    {
        const uint8_t normalized = normalizeRadiusKm(radiusKm);
        const uint8_t previous = g_radar.radiusKm.exchange(normalized);

        if (previous != normalized && lockRadar(200))
        {
            g_radar.aircraftCount = 0;
            g_radar.status = RadarStatus::Idle;
            ++g_radar.dataVersion;
            unlockRadar();
        }

        g_radar.fetchCycleStartMs.store(0);
        g_radar.nextFetchMs.store(0);
    }

    /**
     * @brief Setzt die Sweep-Animation auf den Startzustand.
     */
    void resetSweepAnimation()
    {
        g_sweepAngleDeg = 0.0f;
        g_sweepLastMs = millis();
    }

    /**
     * @brief Liefert den Statuscode für die Änderungserkennung des Zeichenpfads. Fetching verändert die Anzeige nicht und behält deshalb den vorherigen Code.
     * @param status Aktueller Radarstatus
     * @param previousCode Zuletzt vollständig gezeichneter Statuscode
     */
    uint8_t visibleStatusCode(RadarStatus status, uint8_t previousCode)
    {
        if (status == RadarStatus::Fetching)
            return previousCode;

        return static_cast<uint8_t>(status);
    }

    /**
     * @brief Schreibt einen kurzen Ausschnitt einer Antwort in einen Puffer.
     */
    void responsePreview(char *dst, size_t dstSize, const char *data)
    {
        if (!dst || dstSize == 0)
            return;

        dst[0] = '\0';
        if (!data)
            return;

        size_t dstPos = 0;
        while (data[dstPos] && dstPos + 1 < dstSize)
        {
            const char ch = data[dstPos];
            dst[dstPos] = (ch == '\r' || ch == '\n' || ch == '\t') ? ' ' : ch;
            ++dstPos;
        }
        dst[dstPos] = '\0';
    }

    /**
     * @brief Erzeugt die OpenSky-Bounding-Box-URL für den aktuellen Mittelpunkt.
     */
    void buildOpenSkyUrl(char *url, size_t urlSize, uint8_t radiusKm, double centerLat, double centerLon)
    {
        constexpr double KM_PER_LAT_DEG = 110.574;
        constexpr double KM_PER_LON_DEG_AT_EQUATOR = 111.320;
        const double radius = static_cast<double>(normalizeRadiusKm(radiusKm));
        const double latDelta = radius / KM_PER_LAT_DEG;
        const double lonDelta = radius /
                                (KM_PER_LON_DEG_AT_EQUATOR * cos(centerLat * DEG_TO_RAD));

        stringutils::safeFormat(url, urlSize,
                                "https://opensky-network.org/api/states/all?lamin=%.5f&lomin=%.5f&lamax=%.5f&lomax=%.5f",
                                centerLat - latDelta,
                                centerLon - lonDelta,
                                centerLat + latDelta,
                                centerLon + lonDelta);
    }

    /**
     * @brief Formatiert den OAuth-Body ohne Arduino-String.
     */
    void buildTokenBody(char *body, size_t bodySize)
    {
        char id[192];
        char secret[256];
        stringutils::urlEncode(APP_STATE.getOpenSkyClientId(), id, sizeof(id));
        stringutils::urlEncode(APP_STATE.getOpenSkyClientSecret(), secret, sizeof(secret));
        stringutils::safeFormat(body, bodySize,
                                "grant_type=client_credentials&client_id=%s&client_secret=%s",
                                id, secret);
    }

    /**
     * @brief Aktualisiert den OAuth-Token bei Bedarf.
     */
    bool refreshBearerToken()
    {
        if (!hasOpenSkyCredentials())
            return false;

        if (!ensureOpenSkyResponseCopy())
            return false;

        const uint32_t now = millis();
        if (g_radar.bearerToken[0] != '\0' &&
            static_cast<int32_t>(now - g_radar.tokenExpiryMs) < 0)
            return true;

        char body[TOKEN_BODY_CAP];
        buildTokenBody(body, sizeof(body));

        constexpr const char *TOKEN_URL = "https://auth.opensky-network.org/auth/realms/opensky-network/protocol/openid-connect/token";
        HttpResponse response = HTTP_SERVICE.postCopy(TOKEN_URL, body,
                                                      "application/x-www-form-urlencoded",
                                                      g_openSkyResponseCopy,
                                                      OPENSKY_RESPONSE_COPY_BYTES,
                                                      &g_radar.abort);

        if (!response.isSuccess() || !response.hasData())
        {
            g_radar.bearerToken[0] = '\0';
            g_radar.tokenExpiryMs = 0;
            LOG_WARNING_F("FlightRadar: OpenSky-Token fehlgeschlagen, Code %d", response.httpCode);
            return false;
        }

        JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
        DeserializationError err = deserializeJson(doc, response.data);
        if (err)
        {
            g_radar.bearerToken[0] = '\0';
            g_radar.tokenExpiryMs = 0;
            LOG_WARNING_F("FlightRadar: Token-JSON fehlerhaft: %s", err.c_str());
            return false;
        }

        const char *token = doc["access_token"] | "";
        if (!token[0])
        {
            g_radar.bearerToken[0] = '\0';
            g_radar.tokenExpiryMs = 0;
            LOG_WARNING("FlightRadar: Token-Antwort ohne access_token");
            return false;
        }

        stringutils::safeCopy(g_radar.bearerToken, sizeof(g_radar.bearerToken), token);
        g_radar.tokenExpiryMs = millis() + 29UL * 60UL * 1000UL;
        return true;
    }

    /**
     * @brief Berechnet Entfernung und Versatz zum Mittelpunkt.
     */
    void computeOffset(double centerLat, double centerLon, double lat, double lon, float &eastKm, float &northKm, float &distanceKm)
    {
        constexpr float KM_PER_LAT_DEG = 110.574f;
        const float kmPerLonDeg = 111.320f * cosf(static_cast<float>(centerLat) * DEG_TO_RAD);
        eastKm = static_cast<float>((lon - centerLon) * kmPerLonDeg);
        northKm = static_cast<float>((lat - centerLat) * KM_PER_LAT_DEG);
        distanceKm = sqrtf(eastKm * eastKm + northKm * northKm);
    }

    /**
     * @brief Rechnet die Position anhand Kurs und Geschwindigkeit weiter.
     */
    void predictPosition(const Aircraft &aircraft, double &lat, double &lon)
    {
        float dataAgeOnArrival = 0.0f;
        if (aircraft.timePosition > 0 && aircraft.lastContact > 0)
            dataAgeOnArrival = static_cast<float>(aircraft.lastContact - aircraft.timePosition);

        const float localElapsed = (millis() - aircraft.lastSeenMs) / 1000.0f;
        const float dt = localElapsed + dataAgeOnArrival;
        const float headingRad = aircraft.trackDeg * DEG_TO_RAD;
        constexpr float LAT_METERS_PER_DEG = 111320.0f;

        lat = aircraft.latitude + (aircraft.velocityMps * dt * cosf(headingRad)) / LAT_METERS_PER_DEG;
        lon = aircraft.longitude + (aircraft.velocityMps * dt * sinf(headingRad)) /
                                  (LAT_METERS_PER_DEG * cosf(static_cast<float>(aircraft.latitude) * DEG_TO_RAD));
    }

    /**
     * @brief Liefert die aktuelle Anzeige-Position mit weichem Einblenden.
     */
    void displayPosition(const Aircraft &aircraft, double &lat, double &lon)
    {
        double predictedLat = 0.0;
        double predictedLon = 0.0;
        predictPosition(aircraft, predictedLat, predictedLon);

        if (aircraft.blendAlpha >= 1.0f)
        {
            lat = predictedLat;
            lon = predictedLon;
            return;
        }

        const float t = aircraft.blendAlpha * aircraft.blendAlpha * (3.0f - 2.0f * aircraft.blendAlpha);
        lat = aircraft.blendFromLat + t * (predictedLat - aircraft.blendFromLat);
        lon = aircraft.blendFromLon + t * (predictedLon - aircraft.blendFromLon);
    }

    /**
     * @brief Aktualisiert die projizierte Anzeige-Position eines Eintrags.
     */
    void tickAircraft(Aircraft &aircraft, uint32_t now, double centerLat, double centerLon)
    {
        const uint32_t deltaMs = (aircraft.lastTickMs == 0) ? 0 : (now - aircraft.lastTickMs);
        aircraft.lastTickMs = now;

        if (deltaMs > 0 && aircraft.blendAlpha < 1.0f)
        {
            aircraft.blendAlpha += (deltaMs / 1000.0f) * BLEND_SPEED;
            if (aircraft.blendAlpha > 1.0f)
                aircraft.blendAlpha = 1.0f;
        }

        double lat = 0.0;
        double lon = 0.0;
        displayPosition(aircraft, lat, lon);
        computeOffset(centerLat, centerLon, lat, lon, aircraft.eastKm, aircraft.northKm, aircraft.distanceKm);
    }

    /**
     * @brief Sucht einen vorhandenen Eintrag anhand ICAO24.
     */
    int findAircraftByIcao(const char *icao)
    {
        if (!icao || !icao[0])
            return -1;

        for (int i = 0; i < g_radar.aircraftCount; ++i)
        {
            if (strcmp(g_radar.aircraft[i].icao24, icao) == 0)
                return i;
        }

        return -1;
    }

    /**
     * @brief Liefert einen freien oder den entferntesten Eintrag.
     */
    int pickSlotForAircraft(float distanceKm)
    {
        if (g_radar.aircraftCount < MAX_AIRCRAFT)
            return g_radar.aircraftCount++;

        int farthestIdx = 0;
        float farthest = g_radar.aircraft[0].distanceKm;
        for (int i = 1; i < g_radar.aircraftCount; ++i)
        {
            if (g_radar.aircraft[i].distanceKm > farthest)
            {
                farthest = g_radar.aircraft[i].distanceKm;
                farthestIdx = i;
            }
        }

        return (distanceKm < farthest) ? farthestIdx : -1;
    }

    /**
     * @brief Entfernt Einträge, die im letzten Feed nicht mehr vorhanden waren.
     */
    void compactMissingAircraft()
    {
        int dst = 0;
        for (int src = 0; src < g_radar.aircraftCount; ++src)
        {
            if (!g_radar.aircraft[src].present)
                continue;

            if (dst != src)
                g_radar.aircraft[dst] = g_radar.aircraft[src];
            ++dst;
        }

        g_radar.aircraftCount = dst;
    }

    /**
     * @brief Parst die OpenSky-Antwort in die Tracking-Liste.
     */
    bool parseOpenSky(const char *data, uint8_t radiusKm, double centerLat, double centerLon)
    {
        if (!data || !data[0])
            return false;

        const float searchRadiusKm = static_cast<float>(normalizeRadiusKm(radiusKm));

        JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
        DeserializationError err = deserializeJson(doc, data);
        if (err)
        {
            char preview[RESPONSE_PREVIEW_CAP];
            responsePreview(preview, sizeof(preview), data);
            LOG_WARNING_F("FlightRadar: JSON-Parsing fehlgeschlagen: %s, Anfang: %s", err.c_str(), preview);
            return false;
        }

        JsonArray states = doc["states"].as<JsonArray>();
        if (!lockRadar(200))
            return false;

        const uint32_t now = millis();
        for (int i = 0; i < g_radar.aircraftCount; ++i)
            g_radar.aircraft[i].present = false;

        if (!states.isNull())
        {
            for (JsonVariant stateVariant : states)
            {
                JsonArray row = stateVariant.as<JsonArray>();
                if (row.isNull() || row.size() < 11)
                    continue;

                if (row[0].isNull() || row[5].isNull() || row[6].isNull())
                    continue;

                const double lon = row[5].as<double>();
                const double lat = row[6].as<double>();
                float eastKm = 0.0f;
                float northKm = 0.0f;
                float distanceKm = 0.0f;
                computeOffset(centerLat, centerLon, lat, lon, eastKm, northKm, distanceKm);

                if (distanceKm > searchRadiusKm)
                    continue;

                char icao[ICAO_CAP];
                copyTrimmed(icao, sizeof(icao), row[0] | "", "");
                const int existingIdx = findAircraftByIcao(icao);
                const int slot = (existingIdx >= 0) ? existingIdx : pickSlotForAircraft(distanceKm);
                if (slot < 0)
                    continue;

                const float velocityMps = row[9].isNull() ? 0.0f : row[9].as<float>();
                const float trackDeg = row[10].isNull() ? 0.0f : row[10].as<float>();
                Aircraft &aircraft = g_radar.aircraft[slot];
                double displayLat = lat;
                double displayLon = lon;
                bool smoothUpdate = existingIdx >= 0;
                if (existingIdx >= 0)
                {
                    displayPosition(aircraft, displayLat, displayLon);

                    float displayEastKm = 0.0f;
                    float displayNorthKm = 0.0f;
                    float displayDistanceKm = 0.0f;
                    computeOffset(centerLat, centerLon, displayLat, displayLon, displayEastKm, displayNorthKm, displayDistanceKm);

                    const float headingRad = trackDeg * DEG_TO_RAD;
                    const float stepEastKm = eastKm - displayEastKm;
                    const float stepNorthKm = northKm - displayNorthKm;
                    const float alongTrackKm = stepEastKm * sinf(headingRad) + stepNorthKm * cosf(headingRad);
                    if (alongTrackKm < -BACKWARD_BLEND_EPS_KM)
                        smoothUpdate = false;
                }

                stringutils::safeCopy(aircraft.icao24, sizeof(aircraft.icao24), icao);
                copyTrimmed(aircraft.callsign, sizeof(aircraft.callsign), row[1] | "", aircraft.icao24);
                if (aircraft.callsign[0] == '\0')
                    stringutils::safeCopy(aircraft.callsign, "---");
                copyTrimmed(aircraft.originCountry, sizeof(aircraft.originCountry), row[2] | "", "");

                aircraft.blendFromLat = displayLat;
                aircraft.blendFromLon = displayLon;
                aircraft.blendAlpha = smoothUpdate ? 0.0f : 1.0f;
                aircraft.latitude = lat;
                aircraft.longitude = lon;
                aircraft.eastKm = eastKm;
                aircraft.northKm = northKm;
                aircraft.distanceKm = distanceKm;
                aircraft.onGround = row[8] | false;
                aircraft.velocityMps = velocityMps;
                aircraft.trackDeg = trackDeg;
                aircraft.timePosition = row[3].isNull() ? 0 : row[3].as<int32_t>();
                aircraft.lastContact = row[4].isNull() ? 0 : row[4].as<int32_t>();
                aircraft.lastSeenMs = now;
                aircraft.lastTickMs = now;
                aircraft.present = true;
                aircraft.altitudeM = row[13].isNull() ? -1 : static_cast<int>(row[13].as<float>() + 0.5f);
                if (aircraft.altitudeM < 0 && !row[7].isNull())
                    aircraft.altitudeM = static_cast<int>(row[7].as<float>() + 0.5f);
            }
        }

        compactMissingAircraft();
        g_radar.status = (g_radar.aircraftCount > 0) ? RadarStatus::Ready : RadarStatus::Empty;
        ++g_radar.dataVersion;
        unlockRadar();
        return true;
    }

    /**
     * @brief Führt einen einzelnen OpenSky-Abruf aus.
     */
    bool fetchOnce()
    {
        if (!HTTP_SERVICE.isInitialized())
        {
            setStatus(RadarStatus::Error);
            return true;
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            setStatus(RadarStatus::Offline);
            return true;
        }

        if (!ensureOpenSkyResponseCopy())
        {
            setStatus(RadarStatus::Error);
            return true;
        }

        setStatus(RadarStatus::Fetching);

        char url[URL_CAP];
        const uint8_t fetchRadiusKm = normalizeRadiusKm(g_radar.radiusKm.load());
        double centerLat = 0.0;
        double centerLon = 0.0;
        if (!loadRadarCenter(centerLat, centerLon))
        {
            setStatus(RadarStatus::Error);
            LOG_WARNING("FlightRadar: kein gültiger Wetterstandort gespeichert");
            return true;
        }

        buildOpenSkyUrl(url, sizeof(url), fetchRadiusKm, centerLat, centerLon);

        g_radar.abort.store(false);
        HttpResponse response;
        const bool credentialsConfigured = hasOpenSkyCredentials();
        const bool authenticated = refreshBearerToken();
        if (credentialsConfigured && !authenticated)
        {
            if (g_radar.abort.load() || !g_radar.active.load())
                return false;

            LOG_WARNING("FlightRadar: OpenSky OAuth fehlgeschlagen, anonymer Abruf wird versucht");
        }

        if (authenticated)
        {
            char authValue[AUTH_HEADER_CAP];
            stringutils::safeFormat(authValue, sizeof(authValue), "Bearer %s", g_radar.bearerToken);
            const HttpHeader headers[] = {
                {"Authorization", authValue}
            };
            response = HTTP_SERVICE.getWithHeadersCopy(url, headers, 1,
                                                       g_openSkyResponseCopy,
                                                       OPENSKY_RESPONSE_COPY_BYTES,
                                                       &g_radar.abort);
        }
        else
        {
            response = HTTP_SERVICE.getWithHeadersCopy(url, nullptr, 0,
                                                       g_openSkyResponseCopy,
                                                       OPENSKY_RESPONSE_COPY_BYTES,
                                                       &g_radar.abort);
        }

        if (g_radar.abort.load() || !g_radar.active.load())
            return false;

        if (fetchRadiusKm != normalizeRadiusKm(g_radar.radiusKm.load()))
            return false;

        LOG_INFO_F("FlightRadar: OpenSky %s, HTTP %d, %u Bytes, erwartet %u, abgeschnitten %s",
                   authenticated ? "OAuth" : "anonym",
                   response.httpCode,
                   response.length,
                   response.expectedLength,
                   response.truncated ? "ja" : "nein");

        if (!response.isSuccess() || !response.hasData())
        {
            LOG_WARNING_F("FlightRadar: OpenSky-Abruf fehlgeschlagen, Code %d", response.httpCode);
            setStatus(RadarStatus::Error);
            return true;
        }

        const bool parsed = parseOpenSky(response.data, fetchRadiusKm, centerLat, centerLon);
        LOG_INFO_F("FlightRadar: Parse %s, Worker-Stack frei %u",
                   parsed ? "ok" : "fehlerhaft",
                   static_cast<unsigned>(uxTaskGetStackHighWaterMark(nullptr)));
        if (!parsed)
            setStatus(RadarStatus::Error);

        return true;
    }

    /**
     * @brief Worker-Rumpf für regelmäßige OpenSky-Abrufe.
     */
    void workerTask(void *)
    {
        LOG_INFO("FlightRadar: Worker gestartet");

        while (g_radar.active.load())
        {
            const uint32_t now = millis();
            const uint32_t nextFetch = g_radar.nextFetchMs.load();
            const bool due = (nextFetch == 0) ||
                             (static_cast<int32_t>(now - nextFetch) >= 0);

            if (due)
            {
                const bool completed = fetchOnce();
                if (!g_radar.active.load())
                    break;
                if (completed)
                {
                    const uint32_t cycleStart = millis();
                    g_radar.fetchCycleStartMs.store(cycleStart);
                    g_radar.nextFetchMs.store(cycleStart + fetchIntervalMs());
                }
            }

            vTaskDelay(pdMS_TO_TICKS(WORKER_SLEEP_MS));
        }

        g_radar.abort.store(false);
        g_radar.task = nullptr;
        LOG_INFO("FlightRadar: Worker beendet");
        vTaskDelete(nullptr);
    }

    /**
     * @brief Sortiert die lokale Kopie nach Entfernung.
     */
    void sortByDistance(Aircraft *items, int count)
    {
        for (int i = 0; i < count - 1; ++i)
        {
            for (int j = i + 1; j < count; ++j)
            {
                if (items[j].distanceKm < items[i].distanceKm)
                {
                    Aircraft tmp = items[i];
                    items[i] = items[j];
                    items[j] = tmp;
                }
            }
        }
    }

    /**
     * @brief Kopiert den aktuellen Schnappschuss für den Zeichenpfad.
     * @param items Zielpuffer mit Platz für MAX_AIRCRAFT Einträge
     * @param count Anzahl kopierter Einträge, bleibt bei Sperrfehler unverändert
     * @param status Radarstatus, bleibt bei Sperrfehler unverändert
     * @param dataVersion Optional die Datenversion, bleibt bei Sperrfehler unverändert
     * @param lockTimeoutMs Maximale Wartezeit auf die Sperre
     * @return false wenn die Sperre nicht rechtzeitig frei wurde
     */
    bool copySnapshot(Aircraft *items, int &count, RadarStatus &status, uint32_t *dataVersion, uint32_t lockTimeoutMs)
    {
        if (!lockRadar(lockTimeoutMs))
            return false;

        count = 0;
        status = RadarStatus::Idle;
        if (dataVersion)
            *dataVersion = 0;

        double centerLat = 0.0;
        double centerLon = 0.0;
        if (!loadRadarCenter(centerLat, centerLon))
        {
            unlockRadar();
            return true;
        }

        const uint32_t now = millis();
        for (int i = 0; i < g_radar.aircraftCount; ++i)
            tickAircraft(g_radar.aircraft[i], now, centerLat, centerLon);

        count = g_radar.aircraftCount;
        if (count > MAX_AIRCRAFT)
            count = MAX_AIRCRAFT;

        for (int i = 0; i < count; ++i)
            items[i] = g_radar.aircraft[i];

        status = g_radar.status;
        if (dataVersion)
            *dataVersion = g_radar.dataVersion;
        unlockRadar();

        sortByDistance(items, count);
        return true;
    }

    /**
     * @brief Liefert eine geglättete Abfallkurve von 1.0 nach 0.0.
     */
    float smoothFade(float value)
    {
        const float t = clampf(value, 0.0f, 1.0f);
        const float smooth = t * t * (3.0f - 2.0f * t);
        return 1.0f - smooth;
    }

    /**
     * @brief Liefert den aktuellen Sweep-Winkel.
     */
    float currentSweepAngleDeg()
    {
        const uint32_t now = millis();
        if (g_sweepLastMs == 0)
        {
            g_sweepLastMs = now;
            return g_sweepAngleDeg;
        }

        const uint32_t elapsedMs = now - g_sweepLastMs;
        g_sweepLastMs = now;

        const float anglePerMs = FULL_CIRCLE_DEG / SCAN_PERIOD_MS;
        g_sweepAngleDeg += static_cast<float>(elapsedMs) * anglePerMs;
        while (g_sweepAngleDeg >= FULL_CIRCLE_DEG)
            g_sweepAngleDeg -= FULL_CIRCLE_DEG;

        return g_sweepAngleDeg;
    }

    /**
     * @brief Zeichnet die Radarringe.
     */
    void drawRadarGrid(Gfx_Sprite *sprite, int cx, int cy, int radius)
    {
        const float northY = static_cast<float>(cy - radius + COMPASS_INSET);
        const float eastX = static_cast<float>(cx + radius - COMPASS_INSET);
        const float southY = static_cast<float>(cy + radius - COMPASS_INSET);
        const float westX = static_cast<float>(cx - radius + COMPASS_INSET);
        sprite->drawWideLine(static_cast<float>(cx), static_cast<float>(cy),
                             static_cast<float>(cx), northY + COMPASS_GLYPH_HALF + COMPASS_AXIS_LABEL_GAP,
                             RADAR_AXIS_THICKNESS, C_RADAR_AXIS);
        sprite->drawWideLine(static_cast<float>(cx), static_cast<float>(cy),
                             eastX - COMPASS_GLYPH_HALF - COMPASS_AXIS_LABEL_GAP, static_cast<float>(cy),
                             RADAR_AXIS_THICKNESS, C_RADAR_AXIS);
        sprite->drawWideLine(static_cast<float>(cx), static_cast<float>(cy),
                             static_cast<float>(cx), southY - COMPASS_GLYPH_HALF - COMPASS_AXIS_LABEL_GAP,
                             RADAR_AXIS_THICKNESS, C_RADAR_AXIS);
        sprite->drawWideLine(static_cast<float>(cx), static_cast<float>(cy),
                             westX + COMPASS_GLYPH_HALF + COMPASS_AXIS_LABEL_GAP, static_cast<float>(cy),
                             RADAR_AXIS_THICKNESS, C_RADAR_AXIS);
        sprite->drawSmoothCircle(cx, cy, static_cast<float>(radius), RADAR_OUTER_RING_THICKNESS, C_RADAR_GRID, C_RADAR_BG);
        sprite->drawSmoothCircle(cx, cy, static_cast<float>((radius / 3) * 2), RADAR_RING_THICKNESS, C_RADAR_GRID_MID, C_RADAR_BG);
        sprite->drawSmoothCircle(cx, cy, static_cast<float>(radius / 3), RADAR_RING_THICKNESS, C_RADAR_GRID_DIM, C_RADAR_BG);
        sprite->fillSmoothCircle(cx, cy, RADAR_CENTER_DOT_R, C_RADAR_COMPASS);
    }

    /**
     * @brief Zeichnet Richtungszeichen in den Radarschirm.
     */
    void drawCompassLabels(Gfx_Sprite *sprite, int cx, int cy, int radius)
    {
        if (!sprite || radius <= 0)
            return;

        sprite->loadFont(ComfortaaBold16);
        sprite->setTextDatum(MC_DATUM);
        sprite->setTextColor(C_RADAR_COMPASS);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_FLIGHT_RADAR_COMPASS_NORTH), cx, cy - radius + COMPASS_INSET);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_FLIGHT_RADAR_COMPASS_EAST), cx + radius - COMPASS_INSET, cy);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_FLIGHT_RADAR_COMPASS_SOUTH), cx, cy + radius - COMPASS_INSET);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_FLIGHT_RADAR_COMPASS_WEST), cx - radius + COMPASS_INSET, cy);
        sprite->unloadFont();
    }

    /**
     * @brief Kopiert einen 16-Bit-Sprite zeilenweise per memcpy in den Ziel-Sprite. Umgeht den pixelweisen Kopierpfad, den LovyanGFX für Sprites im PSRAM verwendet.
     * @param dst Ziel-Sprite
     * @param src Quell-Sprite
     * @param x Linke Kante im Ziel-Sprite
     * @param y Obere Kante im Ziel-Sprite
     * @return false wenn ein Puffer fehlt oder der Quellbereich nicht vollständig im Ziel liegt
     */
    bool blitSpriteRows(Gfx_Sprite *dst, Gfx_Sprite *src, int x, int y)
    {
        if (!dst || !src)
            return false;

        uint16_t *dstBuf = static_cast<uint16_t *>(dst->getBuffer());
        const uint16_t *srcBuf = static_cast<const uint16_t *>(src->getBuffer());
        if (!dstBuf || !srcBuf)
            return false;

        const int dstW = dst->width();
        const int dstH = dst->height();
        const int srcW = src->width();
        const int srcH = src->height();
        if (x < 0 || y < 0 || x + srcW > dstW || y + srcH > dstH)
            return false;

        const size_t rowBytes = static_cast<size_t>(srcW) * sizeof(uint16_t);
        for (int row = 0; row < srcH; ++row)
            memcpy(dstBuf + (y + row) * dstW + x, srcBuf + row * srcW, rowBytes);

        return true;
    }

    /**
     * @brief Prüft, ob ein Rechteck leer ist.
     */
    bool rectEmpty(const RadarRect &rect)
    {
        return rect.w <= 0 || rect.h <= 0;
    }

    /**
     * @brief Begrenzt ein Rechteck auf einen Bereich, leer wenn kein Überlapp besteht.
     */
    RadarRect clipRadarRect(int x, int y, int w, int h, int clipX, int clipY, int clipW, int clipH)
    {
        RadarRect rect;
        const int left = (x > clipX) ? x : clipX;
        const int top = (y > clipY) ? y : clipY;
        const int right = ((x + w) < (clipX + clipW)) ? (x + w) : (clipX + clipW);
        const int bottom = ((y + h) < (clipY + clipH)) ? (y + h) : (clipY + clipH);
        if (right <= left || bottom <= top)
            return rect;

        rect.x = static_cast<int16_t>(left);
        rect.y = static_cast<int16_t>(top);
        rect.w = static_cast<int16_t>(right - left);
        rect.h = static_cast<int16_t>(bottom - top);
        return rect;
    }

    /**
     * @brief Liefert das umschließende Rechteck zweier Rechtecke, leere Rechtecke werden ignoriert.
     */
    RadarRect unionRadarRect(const RadarRect &a, const RadarRect &b)
    {
        if (rectEmpty(a))
            return b;
        if (rectEmpty(b))
            return a;

        const int left = (a.x < b.x) ? a.x : b.x;
        const int top = (a.y < b.y) ? a.y : b.y;
        const int right = ((a.x + a.w) > (b.x + b.w)) ? (a.x + a.w) : (b.x + b.w);
        const int bottom = ((a.y + a.h) > (b.y + b.h)) ? (a.y + a.h) : (b.y + b.h);
        RadarRect rect;
        rect.x = static_cast<int16_t>(left);
        rect.y = static_cast<int16_t>(top);
        rect.w = static_cast<int16_t>(right - left);
        rect.h = static_cast<int16_t>(bottom - top);
        return rect;
    }

    /**
     * @brief Nimmt ein Rechteck in die Push-Liste auf. Bei voller Liste wird es mit dem letzten Eintrag vereinigt, damit kein Bereich verloren geht.
     */
    void addPushRect(const RadarRect &rect)
    {
        if (rectEmpty(rect))
            return;

        if (g_pushRectCount < PUSH_RECT_CAP)
        {
            g_pushRects[g_pushRectCount++] = rect;
            return;
        }

        g_pushRects[PUSH_RECT_CAP - 1] = unionRadarRect(g_pushRects[PUSH_RECT_CAP - 1], rect);
    }

    /**
     * @brief Erweitert die Grenzen um einen Punkt auf dem Sweep-Kreis.
     */
    void includeArcPoint(float &minX, float &maxX, float &minY, float &maxY, float cx, float cy, float r, float deg)
    {
        const float rad = deg * DEG_TO_RAD;
        const float x = cx + cosf(rad) * r;
        const float y = cy + sinf(rad) * r;
        if (x < minX)
            minX = x;
        if (x > maxX)
            maxX = x;
        if (y < minY)
            minY = y;
        if (y > maxY)
            maxY = y;
    }

    /**
     * @brief Liefert das umschließende Rechteck von Sweep-Keil und Frontlinie, begrenzt auf die Radarbox. Der Keil deckt die Winkel von angleDeg - SCAN_SWEEP_DEG bis angleDeg ab, Achsenschnitte dazwischen bilden die Extrempunkte.
     */
    RadarRect sweepWedgeRect(int cx, int cy, int sweepRadius, float angleDeg, int clipX, int clipY, int clipW, int clipH)
    {
        const float r = static_cast<float>(sweepRadius + SCAN_SWEEP_SPRITE_PAD);
        const float cxF = static_cast<float>(cx);
        const float cyF = static_cast<float>(cy);
        const float a1 = angleDeg;
        const float a0 = angleDeg - SCAN_SWEEP_DEG;
        float minX = cxF;
        float maxX = cxF;
        float minY = cyF;
        float maxY = cyF;
        includeArcPoint(minX, maxX, minY, maxY, cxF, cyF, r, a0);
        includeArcPoint(minX, maxX, minY, maxY, cxF, cyF, r, a1);

        constexpr float AXIS_ANGLES[4] = {0.0f, 90.0f, 180.0f, 270.0f};
        for (float axis : AXIS_ANGLES)
        {
            if (axis >= a0 && axis <= a1)
                includeArcPoint(minX, maxX, minY, maxY, cxF, cyF, r, axis);
        }

        const int left = static_cast<int>(floorf(minX)) - RADAR_DIRTY_PAD;
        const int top = static_cast<int>(floorf(minY)) - RADAR_DIRTY_PAD;
        const int right = static_cast<int>(ceilf(maxX)) + RADAR_DIRTY_PAD + 1;
        const int bottom = static_cast<int>(ceilf(maxY)) + RADAR_DIRTY_PAD + 1;
        return clipRadarRect(left, top, right - left, bottom - top, clipX, clipY, clipW, clipH);
    }

    /**
     * @brief Stellt einen Bereich der Radarbox zeilenweise aus dem Hintergrund-Sprite wieder her.
     * @param sprite Work-Sprite des Workshop-Moduls
     * @param background Hintergrund-Sprite in Radarbox-Größe
     * @param boxX Linke Kante der Radarbox im Work-Sprite
     * @param boxY Obere Kante der Radarbox im Work-Sprite
     * @param rect Bereich im Work-Sprite
     */
    void restoreRadarRect(Gfx_Sprite *sprite, Gfx_Sprite *background, int boxX, int boxY, const RadarRect &rect)
    {
        if (rectEmpty(rect) || !sprite || !background)
            return;

        uint16_t *dst = static_cast<uint16_t *>(sprite->getBuffer());
        const uint16_t *srcBuf = static_cast<const uint16_t *>(background->getBuffer());
        if (!dst || !srcBuf)
            return;

        const int spriteW = sprite->width();
        const int spriteH = sprite->height();
        const int bgW = background->width();
        const int bgH = background->height();
        const int srcX = rect.x - boxX;
        const int srcY = rect.y - boxY;
        if (rect.x < 0 || rect.y < 0 || rect.x + rect.w > spriteW || rect.y + rect.h > spriteH ||
            srcX < 0 || srcY < 0 || srcX + rect.w > bgW || srcY + rect.h > bgH)
            return;

        const size_t rowBytes = static_cast<size_t>(rect.w) * sizeof(uint16_t);
        for (int row = 0; row < rect.h; ++row)
            memcpy(dst + (rect.y + row) * spriteW + rect.x, srcBuf + (srcY + row) * bgW + srcX, rowBytes);
    }

    /**
     * @brief Baut die Push-Liste aus den dynamischen Bereichen des vorherigen und des aktuellen Frames. Bereiche desselben Flugzeugs werden vereinigt, verschwundene Flugzeuge liefern ihre alten Bereiche.
     */
    void buildPushRects()
    {
        g_pushRectCount = 0;
        addPushRect(unionRadarRect(g_prevFrameRects.sweep, g_frameRects.sweep));

        bool prevUsed[MAX_AIRCRAFT] = {};
        for (int i = 0; i < g_frameRects.aircraftCount; ++i)
        {
            const AircraftRects &current = g_frameRects.aircraft[i];
            int prevIndex = -1;
            for (int j = 0; j < g_prevFrameRects.aircraftCount; ++j)
            {
                if (!prevUsed[j] && strcmp(g_prevFrameRects.aircraft[j].icao24, current.icao24) == 0)
                {
                    prevIndex = j;
                    break;
                }
            }

            if (prevIndex >= 0)
            {
                prevUsed[prevIndex] = true;
                addPushRect(unionRadarRect(g_prevFrameRects.aircraft[prevIndex].marker, current.marker));
                addPushRect(unionRadarRect(g_prevFrameRects.aircraft[prevIndex].label, current.label));
            }
            else
            {
                addPushRect(current.marker);
                addPushRect(current.label);
            }
        }

        for (int j = 0; j < g_prevFrameRects.aircraftCount; ++j)
        {
            if (prevUsed[j])
                continue;

            addPushRect(g_prevFrameRects.aircraft[j].marker);
            addPushRect(g_prevFrameRects.aircraft[j].label);
        }
    }

    /**
     * @brief Zeichnet die abgerundete linke untere Ecke des Arbeitsbereichs in den Hintergrund-Sprite. Wird verwendet, wenn die Radarbox bis in diese Ecke reicht, weil der Hintergrund dort sonst den schwarzen Eckbereich außerhalb der Rundung überschreibt.
     * @param background Hintergrund-Sprite in Radarbox-Größe
     */
    void drawWorkCornerMask(Gfx_Sprite *background)
    {
        constexpr int CORNER = WorkshopCfg::WORK_CORNER_R;
        if (!background)
            return;

        const int bottom = background->height();
        background->fillRect(0, bottom - CORNER, CORNER, CORNER, WorkshopCfg::C_BG);
        background->fillSmoothRoundRect(0, bottom - CORNER * 2, CORNER * 2, CORNER * 2, CORNER, WorkshopCfg::C_WORK_BG);
    }

    /**
     * @brief Zeichnet die statische Radarfläche.
     */
    void drawRadarBackground(Gfx_Sprite *sprite, int cx, int cy, int radius)
    {
        if (!sprite || radius <= 0)
            return;

        sprite->fillRect(0, 0, sprite->width(), sprite->height(), WorkshopCfg::C_WORK_BG);
        sprite->fillSmoothCircle(cx, cy, radius, C_RADAR_BG);
    }

    /**
     * @brief Zeichnet das statische Radaroverlay.
     */
    void drawRadarOverlay(Gfx_Sprite *sprite, int cx, int cy, int radius)
    {
        if (!sprite || radius <= 0)
            return;

        sprite->fillSprite(C_RADAR_BG);
        drawRadarGrid(sprite, cx, cy, radius);
        drawCompassLabels(sprite, cx, cy, radius);
    }

    /**
     * @brief Liefert die Alpha-Deckung einer geglätteten Keillinie.
     */
    float wedgeLineAlpha(float px, float py, float x1, float y1, float x2, float y2, float r1, float r2)
    {
        const float vx = x2 - x1;
        const float vy = y2 - y1;
        const float wx = px - x1;
        const float wy = py - y1;
        const float len2 = vx * vx + vy * vy;
        float t = (len2 > 0.0f) ? ((wx * vx + wy * vy) / len2) : 0.0f;
        t = clampf(t, 0.0f, 1.0f);
        const float dx = px - (x1 + t * vx);
        const float dy = py - (y1 + t * vy);
        const float dist = sqrtf(dx * dx + dy * dy);
        const float radius = r1 + (r2 - r1) * t;
        return clampf(radius + 0.5f - dist, 0.0f, 1.0f);
    }

    /**
     * @brief Wandelt eine Alpha-Deckung in die Markermaske.
     */
    uint8_t markerAlphaCode(float alpha)
    {
        const float clipped = clampf(alpha, 0.0f, 1.0f);
        uint8_t value = static_cast<uint8_t>(clipped * AIRCRAFT_MARKER_ALPHA_MASK + 0.5f);
        if (value == 0)
            return 0;
        return value;
    }

    /**
     * @brief Mischt zwei RGB565-Farben mit 7-Bit-Alpha.
     */
    uint16_t blendColorAlpha7(uint16_t fg, uint16_t bg, uint8_t alpha7)
    {
        if (alpha7 >= AIRCRAFT_MARKER_ALPHA_MASK)
            return fg;
        if (alpha7 == 0)
            return bg;

        const uint32_t alpha = static_cast<uint32_t>(alpha7) << 1;
        const uint32_t invAlpha = 256U - alpha;

        const uint32_t r1 = (fg >> 11) & 0x1F;
        const uint32_t g1 = (fg >> 5) & 0x3F;
        const uint32_t b1 = fg & 0x1F;
        const uint32_t r2 = (bg >> 11) & 0x1F;
        const uint32_t g2 = (bg >> 5) & 0x3F;
        const uint32_t b2 = bg & 0x1F;

        const uint32_t r = (r1 * alpha + r2 * invAlpha) >> 8;
        const uint32_t g = (g1 * alpha + g2 * invAlpha) >> 8;
        const uint32_t b = (b1 * alpha + b2 * invAlpha) >> 8;
        return static_cast<uint16_t>((r << 11) | (g << 5) | b);
    }

    /**
     * @brief Tauscht die Byte-Reihenfolge eines RGB565-Werts.
     */
    uint16_t swapRgb565(uint16_t value)
    {
        return static_cast<uint16_t>((value << 8) | (value >> 8));
    }

    /**
     * @brief Liefert die Helligkeit eines RGB565-Pixels als 8-Bit-Wert.
     */
    uint8_t rgb565Luma8(uint16_t color)
    {
        const uint8_t r = static_cast<uint8_t>(((color >> 11) & 0x1F) << 3);
        const uint8_t g = static_cast<uint8_t>(((color >> 5) & 0x3F) << 2);
        const uint8_t b = static_cast<uint8_t>((color & 0x1F) << 3);
        return static_cast<uint8_t>((static_cast<uint16_t>(r) * 30U +
                                     static_cast<uint16_t>(g) * 59U +
                                     static_cast<uint16_t>(b) * 11U) / 100U);
    }

    /**
     * @brief Stellt die Renderfläche für Beschriftungsmasken bereit.
     */
    bool ensureLabelRenderSprite(LGFX *tft)
    {
        if (!tft)
            return false;

        if (g_labelRenderSprite &&
            g_labelRenderSprite->width() == AIRCRAFT_LABEL_W &&
            g_labelRenderSprite->height() == AIRCRAFT_LABEL_H)
            return true;

        if (!tft->safeCreateSprite(g_labelRenderSprite, AIRCRAFT_LABEL_W, AIRCRAFT_LABEL_H))
        {
            LOG_WARNING("FlightRadar: Label-Render-Sprite konnte nicht erstellt werden");
            return false;
        }

        return true;
    }

    /**
     * @brief Stellt den PSRAM-Puffer einer Beschriftungsmaske bereit.
     */
    bool ensureLabelMask(LabelCacheEntry &entry)
    {
        if (entry.mask)
            return true;

        entry.mask = static_cast<uint8_t *>(heap_caps_malloc(AIRCRAFT_LABEL_PIXELS,
                                                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!entry.mask)
            entry.mask = static_cast<uint8_t *>(malloc(AIRCRAFT_LABEL_PIXELS));

        if (!entry.mask)
        {
            LOG_WARNING("FlightRadar: Labelmaske konnte nicht erstellt werden");
            return false;
        }

        memset(entry.mask, 0, AIRCRAFT_LABEL_PIXELS);
        return true;
    }

    /**
     * @brief Entfernt alle gepufferten Flugzeugbeschriftungen.
     */
    void deleteAircraftLabelCache(LGFX *tft)
    {
        for (int i = 0; i < MAX_AIRCRAFT; ++i)
        {
            if (g_labelCache[i].mask)
            {
                free(g_labelCache[i].mask);
                g_labelCache[i].mask = nullptr;
            }
            g_labelCache[i] = LabelCacheEntry();
        }

        if (tft && g_labelRenderSprite)
        {
            tft->safeDeleteSprite(g_labelRenderSprite);
            return;
        }

        if (g_labelRenderSprite)
        {
            g_labelRenderSprite->unloadFont();
            g_labelRenderSprite->deleteSprite();
            delete g_labelRenderSprite;
            g_labelRenderSprite = nullptr;
        }
    }

    /**
     * @brief Übernimmt die sichtbaren Pixel der Renderfläche in die Alpha-Maske.
     */
    void captureLabelMask(LabelCacheEntry &entry, bool detailColor)
    {
        uint16_t *src = static_cast<uint16_t *>(g_labelRenderSprite ? g_labelRenderSprite->getBuffer() : nullptr);
        if (!src || !entry.mask)
            return;

        for (int y = 0; y < AIRCRAFT_LABEL_H; ++y)
        {
            for (int x = 0; x < AIRCRAFT_LABEL_W; ++x)
            {
                const uint16_t color = swapRgb565(src[y * AIRCRAFT_LABEL_W + x]);
                const uint8_t luma = rgb565Luma8(color);
                if (luma == 0)
                    continue;

                const uint8_t alpha = static_cast<uint8_t>((static_cast<uint16_t>(luma) * AIRCRAFT_LABEL_ALPHA_MASK + 127U) / 255U);
                if (alpha == 0)
                    continue;

                const uint8_t code = static_cast<uint8_t>(alpha | (detailColor ? AIRCRAFT_LABEL_DETAIL_FLAG : 0));
                uint8_t &dst = entry.mask[y * AIRCRAFT_LABEL_W + x];
                if ((code & AIRCRAFT_LABEL_ALPHA_MASK) > (dst & AIRCRAFT_LABEL_ALPHA_MASK))
                    dst = code;

                if (x + 1 > entry.width)
                    entry.width = x + 1;
                if (y + 1 > entry.height)
                    entry.height = y + 1;
            }
        }
    }

    /**
     * @brief Rendert eine geglättete Beschriftung in die Alpha-Maske.
     */
    bool renderLabelMask(LGFX *tft, LabelCacheEntry &entry)
    {
        if (!ensureLabelMask(entry) || !ensureLabelRenderSprite(tft))
            return false;

        memset(entry.mask, 0, AIRCRAFT_LABEL_PIXELS);
        entry.width = 0;
        entry.height = 0;

        g_labelRenderSprite->fillSprite(TFT_BLACK);
        g_labelRenderSprite->loadFont(AtkinsonHyperlegibleNextMedium11);
        g_labelRenderSprite->setTextDatum(TL_DATUM);
        g_labelRenderSprite->setTextColor(TFT_WHITE, TFT_BLACK);
        g_labelRenderSprite->drawString(entry.callsign, 0, AIRCRAFT_LABEL_NAME_Y);
        g_labelRenderSprite->unloadFont();
        captureLabelMask(entry, false);

        if (entry.labelMode != LABEL_MODE_NAME)
        {
            g_labelRenderSprite->fillSprite(TFT_BLACK);
            g_labelRenderSprite->loadFont(AtkinsonHyperlegibleNextMedium11);
            g_labelRenderSprite->setTextDatum(TL_DATUM);
            g_labelRenderSprite->setTextColor(TFT_WHITE, TFT_BLACK);
            if (entry.detail1[0] != '\0')
                g_labelRenderSprite->drawString(entry.detail1, 0, AIRCRAFT_LABEL_DETAIL1_Y);
            if (entry.detail2[0] != '\0')
                g_labelRenderSprite->drawString(entry.detail2, 0, AIRCRAFT_LABEL_DETAIL2_Y);
            g_labelRenderSprite->unloadFont();
            captureLabelMask(entry, true);
        }

        entry.ready = entry.width > 0 && entry.height > 0;
        return entry.ready;
    }

    /**
     * @brief Stellt den Speicher für den Markercache bereit.
     */
    bool ensureAircraftMarkerCache()
    {
        if (g_aircraftMarkerCache)
            return true;
        if (g_aircraftMarkerCacheFailed)
            return false;

        g_aircraftMarkerCache = static_cast<uint8_t *>(heap_caps_malloc(AIRCRAFT_MARKER_CACHE_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!g_aircraftMarkerCache)
            g_aircraftMarkerCache = static_cast<uint8_t *>(malloc(AIRCRAFT_MARKER_CACHE_BYTES));

        if (!g_aircraftMarkerCache)
        {
            g_aircraftMarkerCacheFailed = true;
            LOG_WARNING("FlightRadar: Markercache konnte nicht erstellt werden");
            return false;
        }

        memset(g_aircraftMarkerCache, 0, AIRCRAFT_MARKER_CACHE_BYTES);
        return true;
    }

    /**
     * @brief Bereitet die Markermasken für feste Kurswinkel vor.
     */
    bool buildAircraftMarkerCache()
    {
        if (g_aircraftMarkerCacheReady)
            return true;

        if (!ensureAircraftMarkerCache())
            return false;

        constexpr float BODY_X = 0.0f;
        constexpr float NOSE_Y = -9.0f;
        constexpr float BODY_TAIL_Y = 8.0f;
        constexpr float MAIN_WING_Y = -3.0f;
        constexpr float MAIN_WING_LEFT_X = -8.5f;
        constexpr float MAIN_WING_RIGHT_X = 8.5f;
        constexpr float MAIN_WING_TIP_Y = 2.2f;
        constexpr float TAIL_WING_Y = 5.0f;
        constexpr float TAIL_WING_LEFT_X = -5.5f;
        constexpr float TAIL_WING_RIGHT_X = 5.5f;
        constexpr float TAIL_WING_TIP_Y = 9.0f;
        constexpr float BODY_NOSE_R = 0.55f;
        constexpr float BODY_TAIL_R = 1.55f;
        constexpr float MAIN_WING_ROOT_R = 1.7f;
        constexpr float MAIN_WING_TIP_R = 0.18f;
        constexpr float TAIL_WING_ROOT_R = 1.7f;
        constexpr float TAIL_WING_TIP_R = 0.32f;
        constexpr int MARKER_HALF = AIRCRAFT_MARKER_DRAW_SIZE / 2;

        for (int step = 0; step < AIRCRAFT_MARKER_STEPS; ++step)
        {
            const float angle = static_cast<float>(step) * AIRCRAFT_MARKER_STEP_DEG * DEG_TO_RAD;
            const float c = cosf(angle);
            const float s = sinf(angle);
            uint8_t *mask = g_aircraftMarkerCache + step * AIRCRAFT_MARKER_PIXELS;

            for (int sy = -MARKER_HALF; sy <= MARKER_HALF; ++sy)
            {
                for (int sx = -MARKER_HALF; sx <= MARKER_HALF; ++sx)
                {
                    const float lx = static_cast<float>(sx) * c + static_cast<float>(sy) * s;
                    const float ly = -static_cast<float>(sx) * s + static_cast<float>(sy) * c;

                    float brightAlpha = wedgeLineAlpha(lx, ly, BODY_X, NOSE_Y, BODY_X, BODY_TAIL_Y, BODY_NOSE_R, BODY_TAIL_R);
                    brightAlpha = fmaxf(brightAlpha, wedgeLineAlpha(lx, ly, BODY_X, MAIN_WING_Y, MAIN_WING_LEFT_X, MAIN_WING_TIP_Y, MAIN_WING_ROOT_R, MAIN_WING_TIP_R));
                    brightAlpha = fmaxf(brightAlpha, wedgeLineAlpha(lx, ly, BODY_X, MAIN_WING_Y, MAIN_WING_RIGHT_X, MAIN_WING_TIP_Y, MAIN_WING_ROOT_R, MAIN_WING_TIP_R));

                    float tailAlpha = wedgeLineAlpha(lx, ly, BODY_X, TAIL_WING_Y, TAIL_WING_LEFT_X, TAIL_WING_TIP_Y, TAIL_WING_ROOT_R, TAIL_WING_TIP_R);
                    tailAlpha = fmaxf(tailAlpha, wedgeLineAlpha(lx, ly, BODY_X, TAIL_WING_Y, TAIL_WING_RIGHT_X, TAIL_WING_TIP_Y, TAIL_WING_ROOT_R, TAIL_WING_TIP_R));

                    const float alpha = fmaxf(brightAlpha, tailAlpha);
                    const int index = (sy + MARKER_HALF) * AIRCRAFT_MARKER_DRAW_SIZE + (sx + MARKER_HALF);
                    mask[index] = markerAlphaCode(alpha);
                }
            }
        }

        g_aircraftMarkerCacheReady = true;
        return true;
    }

    /**
     * @brief Liefert den vorberechneten Kursindex für einen Flugzeugmarker.
     */
    int aircraftMarkerStep(float trackDeg)
    {
        float angle = trackDeg;
        while (angle < 0.0f)
            angle += FULL_CIRCLE_DEG;
        while (angle >= FULL_CIRCLE_DEG)
            angle -= FULL_CIRCLE_DEG;

        int step = static_cast<int>((angle + AIRCRAFT_MARKER_STEP_DEG * 0.5f) / AIRCRAFT_MARKER_STEP_DEG);
        if (step >= AIRCRAFT_MARKER_STEPS)
            step = 0;
        return step;
    }

    /**
     * @brief Zeichnet den Verlaufskeil in den Sweep-Sprite. Die Keilspitze liegt links unten bei (SCAN_SWEEP_SPRITE_PAD, Höhe - 1 - SCAN_SWEEP_SPRITE_PAD), der Keil öffnet sich nach rechts oben über SCAN_SWEEP_DEG. Alle übrigen Pixel bleiben in der Transparenzfarbe C_RADAR_BG.
     * @param sprite Keil-Sprite mit passender Größe
     * @param sweepRadius Länge des Sweeps in Pixeln
     */
    void renderSweepSprite(Gfx_Sprite *sprite, int sweepRadius)
    {
        if (!sprite || sweepRadius <= 0)
            return;

        const int apexX = SCAN_SWEEP_SPRITE_PAD;
        const int apexY = sprite->height() - 1 - SCAN_SWEEP_SPRITE_PAD;
        const float radiusF = static_cast<float>(sweepRadius);
        const float radius2 = radiusF * radiusF;
        const float sweepRad = SCAN_SWEEP_DEG * DEG_TO_RAD;

        sprite->fillScreen(C_RADAR_BG);

        for (int y = 0; y < sprite->height(); ++y)
        {
            const float dy = static_cast<float>(y - apexY);
            for (int x = 0; x < sprite->width(); ++x)
            {
                const float dx = static_cast<float>(x - apexX);
                const float dist2 = dx * dx + dy * dy;
                if (dist2 > radius2 || dist2 <= 1.0f)
                    continue;

                float angle = atan2f(dy, dx);
                if (angle > 0.0f)
                    angle -= FULL_CIRCLE_RAD;

                if (angle < -sweepRad)
                    continue;

                const float angularT = -angle / sweepRad;
                const float radialT = sqrtf(dist2) / radiusF;
                const float fade = smoothFade(angularT);
                const float radialLift = 0.35f + 0.65f * radialT;
                const uint8_t brightness = static_cast<uint8_t>(
                    SCAN_SWEEP_MIN_BRIGHTNESS +
                    (SCAN_SWEEP_MAX_BRIGHTNESS - SCAN_SWEEP_MIN_BRIGHTNESS) * fade * radialLift);

                if (brightness > SCAN_SWEEP_MIN_BRIGHTNESS)
                    sprite->drawPixel(x, y, lgfx::color888(0, brightness, 0));
            }
        }
    }

    /**
     * @brief Zeichnet einen Flugzeugmarker im Radar.
     */
    void drawAircraftMarker(Gfx_Sprite *sprite, const Aircraft &aircraft, int cx, int cy, int radius, float searchRadiusKm,
                            int clipX, int clipY, int clipW, int clipH, RadarRect *outRect)
    {
        if (outRect)
            *outRect = RadarRect();

        if (aircraft.onGround)
            return;

        if (!sprite || clipW <= 0 || clipH <= 0)
            return;

        uint16_t *buffer = static_cast<uint16_t *>(sprite->getBuffer());
        if (!buffer)
            return;

        const int spriteW = sprite->width();
        const int spriteH = sprite->height();
        const int clipRight = ((clipX + clipW) < spriteW) ? (clipX + clipW) : spriteW;
        const int clipBottom = ((clipY + clipH) < spriteH) ? (clipY + clipH) : spriteH;
        if (clipX < 0)
            clipX = 0;
        if (clipY < 0)
            clipY = 0;
        if (clipX >= clipRight || clipY >= clipBottom)
            return;

        const float xNorm = clampf(aircraft.eastKm / searchRadiusKm, -1.0f, 1.0f);
        const float yNorm = clampf(aircraft.northKm / searchRadiusKm, -1.0f, 1.0f);
        const int px = cx + static_cast<int>(xNorm * radius);
        const int py = cy - static_cast<int>(yNorm * radius);

        if (aircraft.distanceKm > searchRadiusKm)
            return;

        if (!buildAircraftMarkerCache())
            return;

        constexpr int MARKER_HALF = AIRCRAFT_MARKER_DRAW_SIZE / 2;
        const uint8_t *mask = g_aircraftMarkerCache + aircraftMarkerStep(aircraft.trackDeg) * AIRCRAFT_MARKER_PIXELS;
        if (outRect)
            *outRect = clipRadarRect(px - MARKER_HALF, py - MARKER_HALF, AIRCRAFT_MARKER_DRAW_SIZE, AIRCRAFT_MARKER_DRAW_SIZE,
                                     clipX, clipY, clipRight - clipX, clipBottom - clipY);

        for (int sy = -MARKER_HALF; sy <= MARKER_HALF; ++sy)
        {
            for (int sx = -MARKER_HALF; sx <= MARKER_HALF; ++sx)
            {
                const int maskIndex = (sy + MARKER_HALF) * AIRCRAFT_MARKER_DRAW_SIZE + (sx + MARKER_HALF);
                const uint8_t code = mask[maskIndex];
                if (code == 0)
                    continue;

                const int tx = px + sx;
                const int ty = py + sy;
                if (tx < clipX || ty < clipY || tx >= clipRight || ty >= clipBottom)
                    continue;

                const uint16_t fg = C_AIRCRAFT_MARKER;
                const uint8_t alphaCode = code & AIRCRAFT_MARKER_ALPHA_MASK;
                uint16_t &dst = buffer[ty * spriteW + tx];
                if (alphaCode >= AIRCRAFT_MARKER_ALPHA_MASK)
                    dst = swapRgb565(fg);
                else
                    dst = swapRgb565(blendColorAlpha7(fg, swapRgb565(dst), alphaCode));
            }
        }
    }

    /**
     * @brief Findet eine Beschriftung anhand der ICAO24-Adresse.
     */
    LabelCacheEntry *findLabelCacheEntry(const char *icao24, bool activeOnly)
    {
        if (!icao24 || icao24[0] == '\0')
            return nullptr;

        for (int i = 0; i < MAX_AIRCRAFT; ++i)
        {
            if (activeOnly && !g_labelCache[i].active)
                continue;

            if (g_labelCache[i].icao24[0] != '\0' &&
                strcmp(g_labelCache[i].icao24, icao24) == 0)
                return &g_labelCache[i];
        }

        return nullptr;
    }

    /**
     * @brief Liefert einen freien Beschriftungseintrag.
     */
    LabelCacheEntry *freeLabelCacheEntry()
    {
        for (int i = 0; i < MAX_AIRCRAFT; ++i)
        {
            if (!g_labelCache[i].active && g_labelCache[i].icao24[0] == '\0')
                return &g_labelCache[i];
        }

        for (int i = 0; i < MAX_AIRCRAFT; ++i)
        {
            if (!g_labelCache[i].active)
                return &g_labelCache[i];
        }

        return nullptr;
    }

    /**
     * @brief Formatiert die Detailzeilen einer Flugzeugbeschriftung.
     */
    void formatAircraftLabelDetails(const Aircraft &aircraft, char *detail1, size_t detail1Size,
                                    char *detail2, size_t detail2Size)
    {
        if (detail1 && detail1Size > 0)
            stringutils::safeFormat(detail1, detail1Size, "%.0f km/h", aircraft.velocityMps * MS_TO_KMH);

        if (!detail2 || detail2Size == 0)
            return;

        detail2[0] = '\0';
        if (aircraft.altitudeM >= 0)
            stringutils::safeFormat(detail2, detail2Size, "%d m", aircraft.altitudeM);
    }

    /**
     * @brief Aktualisiert die gepufferten Beschriftungsmasken.
     */
    void updateAircraftLabelCache(LGFX *tft, const Aircraft *items, int count, uint8_t labelMode)
    {
        for (int i = 0; i < MAX_AIRCRAFT; ++i)
            g_labelCache[i].active = false;

        if (!tft || !items || count <= 0 || labelMode == LABEL_MODE_SYMBOL)
            return;

        for (int i = 0; i < count; ++i)
        {
            const Aircraft &aircraft = items[i];
            if (aircraft.onGround || aircraft.icao24[0] == '\0')
                continue;

            char detail1[24] = {};
            char detail2[24] = {};
            if (labelMode != LABEL_MODE_NAME)
                formatAircraftLabelDetails(aircraft, detail1, sizeof(detail1), detail2, sizeof(detail2));

            LabelCacheEntry *entry = findLabelCacheEntry(aircraft.icao24, false);
            if (!entry)
                entry = freeLabelCacheEntry();
            if (!entry)
                continue;

            const bool textChanged =
                strcmp(entry->callsign, aircraft.callsign) != 0 ||
                strcmp(entry->detail1, detail1) != 0 ||
                strcmp(entry->detail2, detail2) != 0 ||
                entry->labelMode != labelMode ||
                !entry->ready;

            entry->active = true;
            stringutils::safeCopy(entry->icao24, sizeof(entry->icao24), aircraft.icao24);

            if (!textChanged)
                continue;

            stringutils::safeCopy(entry->callsign, sizeof(entry->callsign), aircraft.callsign);
            stringutils::safeCopy(entry->detail1, sizeof(entry->detail1), detail1);
            stringutils::safeCopy(entry->detail2, sizeof(entry->detail2), detail2);
            entry->labelMode = labelMode;
            entry->ready = false;
            renderLabelMask(tft, *entry);
        }
    }

    /**
     * @brief Zeichnet eine gepufferte Flugzeugbeschriftung auf den aktuellen Radarinhalt.
     */
    void drawAircraftLabel(Gfx_Sprite *sprite, const Aircraft &aircraft,
                           int cx, int cy, int radius, float searchRadiusKm,
                           int clipX, int clipY, int clipW, int clipH, RadarRect *outRect)
    {
        if (outRect)
            *outRect = RadarRect();

        if (aircraft.onGround || aircraft.distanceKm > searchRadiusKm || !sprite)
            return;

        LabelCacheEntry *entry = findLabelCacheEntry(aircraft.icao24, true);
        if (!entry || !entry->ready || !entry->mask)
            return;

        uint16_t *buffer = static_cast<uint16_t *>(sprite->getBuffer());
        if (!buffer)
            return;

        const int spriteW = sprite->width();
        const int spriteH = sprite->height();
        const int clipRight = ((clipX + clipW) < spriteW) ? (clipX + clipW) : spriteW;
        const int clipBottom = ((clipY + clipH) < spriteH) ? (clipY + clipH) : spriteH;
        if (clipX < 0)
            clipX = 0;
        if (clipY < 0)
            clipY = 0;
        if (clipX >= clipRight || clipY >= clipBottom)
            return;

        const float xNorm = clampf(aircraft.eastKm / searchRadiusKm, -1.0f, 1.0f);
        const float yNorm = clampf(aircraft.northKm / searchRadiusKm, -1.0f, 1.0f);
        const int px = cx + static_cast<int>(xNorm * radius) + AIRCRAFT_LABEL_OFFSET_X;
        const int py = cy - static_cast<int>(yNorm * radius) + 5;
        if (outRect)
            *outRect = clipRadarRect(px, py, entry->width, entry->height, clipX, clipY, clipRight - clipX, clipBottom - clipY);

        for (int sy = 0; sy < entry->height; ++sy)
        {
            const int ty = py + sy;
            if (ty < clipY || ty >= clipBottom)
                continue;

            for (int sx = 0; sx < entry->width; ++sx)
            {
                const uint8_t code = entry->mask[sy * AIRCRAFT_LABEL_W + sx];
                const uint8_t alphaCode = code & AIRCRAFT_LABEL_ALPHA_MASK;
                if (alphaCode == 0)
                    continue;

                const int tx = px + sx;
                if (tx < clipX || tx >= clipRight)
                    continue;

                const uint16_t fg = (code & AIRCRAFT_LABEL_DETAIL_FLAG) ? C_AIRCRAFT_DETAIL : C_AIRCRAFT_CALLSIGN;
                uint16_t &dst = buffer[ty * spriteW + tx];
                if (alphaCode >= AIRCRAFT_LABEL_ALPHA_MASK)
                    dst = swapRgb565(fg);
                else
                    dst = swapRgb565(blendColorAlpha7(fg, swapRgb565(dst), alphaCode));
            }
        }
    }

    /**
     * @brief Zeichnet die rechte Flugzeugliste.
     */
    void drawAircraftList(Gfx_Sprite *sprite, const Aircraft *items, int count,
                          int x, int y, int w, int h, RadarStatus status, bool authenticated)
    {
        constexpr int CARD_R = 8;
        constexpr int PAD_X = 12;
        constexpr int AUTH_BADGE_PAD_X = 7;
        constexpr int AUTH_BADGE_PAD_Y = 2;
        constexpr int AUTH_BADGE_R = 4;
        constexpr int AUTH_BADGE_GAP = 8;
        constexpr int ROW_DETAIL_Y = 20;
        constexpr int DIVIDER_PAD_X = 10;
        constexpr int DIVIDER_OFFSET_Y = 2;
        constexpr uint16_t C_LIST_DIVIDER = 0x4208;
        constexpr uint16_t C_AUTH_ACCOUNT_BG = 0x0320;
        constexpr uint16_t C_AUTH_ACCOUNT_TEXT = 0x07E0;
        constexpr uint16_t C_AUTH_GUEST_BG = 0x4208;
        constexpr uint16_t C_AUTH_GUEST_TEXT = C_PANEL_DIM;
        constexpr int BADGE_PAD_X = 5;
        constexpr int BADGE_PAD_Y = 2;
        constexpr int BADGE_R = 4;
        constexpr int BADGE_OUTSET_X = 7;
        constexpr int BADGE_OUTSET_Y = 6;
        constexpr uint16_t C_BADGE_BG = 0x50A5;
        constexpr uint16_t C_BADGE_TEXT = WorkshopCfg::C_TEXT_BRIGHT;

        sprite->fillSmoothRoundRect(x, y, w, h, CARD_R, C_PANEL_BG);

        sprite->loadFont(ComfortaaBold14);
        sprite->setTextDatum(TL_DATUM);
        sprite->setTextColor(C_RADAR_COMPASS, C_PANEL_BG);
        char labelBuf[24];
        I18n::format(labelBuf, sizeof(labelBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_LABEL_COUNT);
        char countBuf[8];
        stringutils::safeFormat(countBuf, sizeof(countBuf), "%d", count);
        char countLabelBuf[40];
        stringutils::safeFormat(countLabelBuf, sizeof(countLabelBuf), "%s %s", countBuf, labelBuf);
        sprite->drawString(countLabelBuf, x + PAD_X, y + 8);
        char authBuf[16];
        I18n::format(authBuf, sizeof(authBuf),
                     authenticated ? WorkshopKey::WORKSHOP_FLIGHT_RADAR_AUTH_ACCOUNT
                                   : WorkshopKey::WORKSHOP_FLIGHT_RADAR_AUTH_GUEST);
        const int countLabelW = sprite->textWidth(countLabelBuf);
        const int authTextW = sprite->textWidth(authBuf);
        const int authBadgeW = authTextW + AUTH_BADGE_PAD_X * 2;
        const int authBadgeH = sprite->fontHeight() + AUTH_BADGE_PAD_Y * 2;
        const int authBadgeX = x + w - PAD_X - authBadgeW;
        const int authBadgeY = y + 6;
        if (authBadgeX > x + PAD_X + countLabelW + AUTH_BADGE_GAP)
        {
            sprite->fillSmoothRoundRect(authBadgeX, authBadgeY, authBadgeW, authBadgeH,
                                        AUTH_BADGE_R,
                                        authenticated ? C_AUTH_ACCOUNT_BG : C_AUTH_GUEST_BG);
            sprite->setTextDatum(MC_DATUM);
            sprite->setTextColor(authenticated ? C_AUTH_ACCOUNT_TEXT : C_AUTH_GUEST_TEXT,
                                 authenticated ? C_AUTH_ACCOUNT_BG : C_AUTH_GUEST_BG);
            sprite->drawString(authBuf, authBadgeX + authBadgeW / 2, authBadgeY + authBadgeH / 2);
            sprite->setTextDatum(TL_DATUM);
        }
        sprite->setTextDatum(TL_DATUM);
        sprite->unloadFont();

        if (status == RadarStatus::Offline)
        {
            char titleBuf[40];
            I18n::format(titleBuf, sizeof(titleBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_OFFLINE_TITLE);

            const int msgX = x + w / 2;
            const int msgY = y + LIST_HEADER_H + (h - LIST_HEADER_H) / 2;

            sprite->loadFont(ComfortaaBold14);
            sprite->setTextDatum(MC_DATUM);
            sprite->setTextColor(WorkshopCfg::C_TEXT_BRIGHT, C_PANEL_BG);
            sprite->drawString(titleBuf, msgX, msgY);
            sprite->setTextDatum(TL_DATUM);
            sprite->unloadFont();
            return;
        }

        int rowY = y + LIST_HEADER_H;
        const int visible = (count < LIST_ROWS) ? count : LIST_ROWS;

        sprite->loadFont(ComfortaaBold16);
        for (int i = 0; i < visible; ++i)
        {
            const Aircraft &a = items[i];

            if (i > 0)
                sprite->drawFastHLine(x + DIVIDER_PAD_X, rowY - DIVIDER_OFFSET_Y, w - DIVIDER_PAD_X * 2, C_LIST_DIVIDER);

            sprite->setTextColor(C_AIRCRAFT_CALLSIGN, C_PANEL_BG);
            sprite->drawString(a.callsign, x + PAD_X, rowY);

            char distBuf[16];
            stringutils::safeFormat(distBuf, sizeof(distBuf), "%.1f km", a.distanceKm);
            sprite->setTextDatum(TR_DATUM);
            sprite->setTextColor(C_AIRCRAFT_DETAIL, C_PANEL_BG);
            sprite->drawString(distBuf, x + w - PAD_X, rowY);
            sprite->setTextDatum(TL_DATUM);
            rowY += LIST_ROW_H;
        }
        sprite->unloadFont();

        sprite->loadFont(ComfortaaBold14);
        rowY = y + LIST_HEADER_H + ROW_DETAIL_Y;
        for (int i = 0; i < visible; ++i)
        {
            const Aircraft &a = items[i];

            char detailBuf[48];
            if (a.onGround)
            {
                I18n::format(detailBuf, sizeof(detailBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_LABEL_GROUND);
            }
            else if (a.altitudeM >= 0)
            {
                stringutils::safeFormat(detailBuf, sizeof(detailBuf), "%d m  %.0f km/h",
                                        a.altitudeM, a.velocityMps * MS_TO_KMH);
            }
            else
            {
                stringutils::safeFormat(detailBuf, sizeof(detailBuf), "%.0f km/h", a.velocityMps * MS_TO_KMH);
            }

            sprite->setTextColor(a.onGround ? C_AIRCRAFT_MARKER : C_AIRCRAFT_DETAIL, C_PANEL_BG);
            sprite->drawString(detailBuf, x + PAD_X, rowY);
            rowY += LIST_ROW_H;
        }

        const int remaining = count - visible;
        if (remaining > 0)
        {
            char badgeBuf[24];
            I18n::format(badgeBuf, sizeof(badgeBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_FMT_MORE, static_cast<unsigned>(remaining));

            const int badgeTextW = sprite->textWidth(badgeBuf);
            const int badgeTextH = sprite->fontHeight();
            const int badgeW = badgeTextW + BADGE_PAD_X * 2;
            const int badgeH = badgeTextH + BADGE_PAD_Y * 2;
            const int badgeX = x + w - badgeW + BADGE_OUTSET_X;
            const int badgeY = y + h - badgeH + BADGE_OUTSET_Y;

            sprite->fillSmoothRoundRect(badgeX, badgeY, badgeW, badgeH, BADGE_R, C_BADGE_BG);
            sprite->setTextDatum(MC_DATUM);
            sprite->setTextColor(C_BADGE_TEXT, C_BADGE_BG);
            sprite->drawString(badgeBuf, badgeX + badgeW / 2, badgeY + badgeH / 2);
            sprite->setTextDatum(TL_DATUM);
        }
        sprite->unloadFont();
    }

}

// ===================================================================
// Lebenszyklus
// ===================================================================

/**
 * @brief Gibt interne Zeichenressourcen frei.
 */
WorkshopTool_FlightRadar::~WorkshopTool_FlightRadar()
{
    deleteProgressSprite();
    deleteRadarBackgroundSprite();
    deleteSweepSprite();
    deleteAircraftLabelCache(m_tft);
}

/**
 * @brief Startet den Radar-Worker.
 */
void WorkshopTool_FlightRadar::onEnter(LGFX *tft)
{
    m_tft = tft;
    m_searchRadiusKm = normalizeRadiusKm(APP_STATE.getWorkshopFlightRadarRadiusKm());
    m_labelMode = LabelMode::Full;
    configureSharedRadiusKm(m_searchRadiusKm);
    initFooterButtons(tft);
    if (!m_msg)
        m_msg.reset(new MessageBox(tft));
    m_nextFrameMs = 0;
    m_framePending = true;
    m_lastProgressUpdateMs = 0;
    resetSweepAnimation();
    buildAircraftMarkerCache();

    if (m_tft && !m_progressSprite &&
        !m_tft->safeCreateSprite(m_progressSprite, PROGRESS_BAR_W, m_tft->height()))
        LOG_WARNING("FlightRadar: Fortschrittsbalken-Sprite konnte nicht erstellt werden");

    if (!ensureMutex())
        return;

    const uint32_t now = millis();
    g_radar.active.store(true);
    g_radar.abort.store(false);
    g_radar.fetchCycleStartMs.store(0);
    g_radar.nextFetchMs.store(now + INITIAL_FETCH_DELAY_MS);

    if (!g_radar.task)
    {
        BaseType_t ok = xTaskCreate(workerTask, "flight_radar", WORKER_STACK_BYTES,
                                    nullptr, 1, &g_radar.task);
        if (ok != pdPASS)
        {
            g_radar.task = nullptr;
            setStatus(RadarStatus::Error);
            LOG_ERROR("FlightRadar: Worker konnte nicht gestartet werden");
        }
    }
}

/**
 * @brief Stoppt den Radar-Worker asynchron.
 */
void WorkshopTool_FlightRadar::onExit()
{
    if (owner())
        owner()->setWorkContentPaused(false);

    g_radar.active.store(false);
    g_radar.abort.store(true);
    m_msg.reset();
    m_buttonsInitialized = false;
    m_labelMode = LabelMode::Full;
    deleteProgressSprite();
    deleteRadarBackgroundSprite();
    deleteSweepSprite();
    deleteAircraftLabelCache(m_tft);
    m_tft = nullptr;
    m_radarBoxValid = false;
    m_aircraftListX = 0;
    m_aircraftListY = 0;
    m_aircraftListW = 0;
    m_aircraftListH = 0;
    m_aircraftListVisible = 0;
    memset(g_listRowIcao, 0, sizeof(g_listRowIcao));
    m_listRedrawDueMs = 0;
    m_listPushBands = 0;
    m_lastProgressUpdateMs = 0;
    m_nextFrameMs = 0;
    m_framePending = false;
}

/**
 * @brief Gibt den Fortschrittsbalken-Sprite frei.
 */
void WorkshopTool_FlightRadar::deleteProgressSprite()
{
    if (m_tft && m_progressSprite)
    {
        m_progressSprite->drawFastVLine(0, 0, m_tft->height(), Theme::ColorMainBackGround);
        m_progressSprite->pushSprite(m_tft->width() - PROGRESS_BAR_W, 0);
        m_tft->safeDeleteSprite(m_progressSprite);
        m_progressSprite = nullptr;
        return;
    }

    if (m_progressSprite)
    {
        m_progressSprite->unloadFont();
        m_progressSprite->deleteSprite();
        delete m_progressSprite;
        m_progressSprite = nullptr;
    }
}

/**
 * @brief Gibt den Verlaufssprite frei.
 */
void WorkshopTool_FlightRadar::deleteSweepSprite()
{
    if (m_tft)
    {
        m_tft->safeDeleteSprite(m_sweepSprite);
        m_sweepSprite = nullptr;
    }
    else if (m_sweepSprite)
    {
        m_sweepSprite->unloadFont();
        m_sweepSprite->deleteSprite();
        delete m_sweepSprite;
        m_sweepSprite = nullptr;
    }

    m_sweepSpriteRadius = 0;
}

/**
 * @brief Erstellt den Keil-Sprite passend zur aktuellen Sweep-Länge. Der Sprite umfasst nur das Rechteck des Keils, der Drehpunkt liegt auf der Keilspitze.
 */
bool WorkshopTool_FlightRadar::ensureSweepSprite(int sweepRadius)
{
    if (sweepRadius <= 0 || !m_tft)
        return false;

    if (m_sweepSprite && m_sweepSpriteRadius == sweepRadius)
        return true;

    deleteSweepSprite();

    const float sweepRad = SCAN_SWEEP_DEG * DEG_TO_RAD;
    const int width = sweepRadius + 1 + SCAN_SWEEP_SPRITE_PAD * 2;
    const int height = static_cast<int>(ceilf(static_cast<float>(sweepRadius) * sinf(sweepRad))) + 1 + SCAN_SWEEP_SPRITE_PAD * 2;
    if (!m_tft->safeCreateSprite(m_sweepSprite, width, height))
    {
        LOG_WARNING("FlightRadar: Sweep-Sprite konnte nicht erstellt werden");
        return false;
    }

    m_sweepSprite->setPivot(static_cast<float>(SCAN_SWEEP_SPRITE_PAD),
                            static_cast<float>(height - 1 - SCAN_SWEEP_SPRITE_PAD));
    renderSweepSprite(m_sweepSprite, sweepRadius);
    m_sweepSpriteRadius = sweepRadius;
    return true;
}

/**
 * @brief Gibt die statischen Radarhintergrund-Sprites frei.
 */
void WorkshopTool_FlightRadar::deleteRadarBackgroundSprite()
{
    deleteOverlayPixelList();
    if (m_tft)
    {
        m_tft->safeDeleteSprite(m_radarBackgroundSprite);
        m_radarBackgroundSprite = nullptr;
        m_tft->safeDeleteSprite(m_radarOverlaySprite);
        m_radarOverlaySprite = nullptr;
    }
    else
    {
        if (m_radarBackgroundSprite)
        {
            m_radarBackgroundSprite->unloadFont();
            m_radarBackgroundSprite->deleteSprite();
            delete m_radarBackgroundSprite;
            m_radarBackgroundSprite = nullptr;
        }

        if (m_radarOverlaySprite)
        {
            m_radarOverlaySprite->unloadFont();
            m_radarOverlaySprite->deleteSprite();
            delete m_radarOverlaySprite;
            m_radarOverlaySprite = nullptr;
        }
    }

    m_radarBackgroundW = 0;
    m_radarBackgroundH = 0;
    m_radarBackgroundCx = 0;
    m_radarBackgroundCy = 0;
    m_radarBackgroundRadius = 0;
    m_radarBackgroundCorner = false;
}

/**
 * @brief Erstellt die statischen Radarsprites passend zur aktuellen Geometrie. Der Hintergrund-Sprite enthält Arbeitsbereichsfarbe, Radarfläche, Raster, Richtungszeichen und bei Bedarf die Rundung der linken unteren Ecke. Das Overlay bleibt zusätzlich als Sprite und als Pixelliste erhalten, damit es nach dem Sweep bereichsweise erneut gezeichnet werden kann.
 * @param w Breite der Radarbox
 * @param h Höhe der Radarbox
 * @param cx Mittelpunkt X innerhalb der Radarbox
 * @param cy Mittelpunkt Y innerhalb der Radarbox
 * @param radius Radarradius
 * @param roundBottomLeft true wenn die Radarbox bis in die linke untere Ecke des Work-Sprites reicht
 */
bool WorkshopTool_FlightRadar::ensureRadarBackgroundSprite(int w, int h, int cx, int cy, int radius, bool roundBottomLeft)
{
    if (w <= 0 || h <= 0 || radius <= 0 || !m_tft)
        return false;

    if (m_radarBackgroundSprite &&
        m_radarOverlaySprite &&
        m_radarBackgroundW == w &&
        m_radarBackgroundH == h &&
        m_radarBackgroundCx == cx &&
        m_radarBackgroundCy == cy &&
        m_radarBackgroundRadius == radius &&
        m_radarBackgroundCorner == roundBottomLeft)
        return true;

    deleteRadarBackgroundSprite();

    if (!m_tft->safeCreateSprite(m_radarBackgroundSprite, w, h))
    {
        LOG_WARNING("FlightRadar: Hintergrund-Sprite konnte nicht erstellt werden");
        return false;
    }

    if (!m_tft->safeCreateSprite(m_radarOverlaySprite, w, h))
    {
        LOG_WARNING("FlightRadar: Overlay-Sprite konnte nicht erstellt werden");
        deleteRadarBackgroundSprite();
        return false;
    }

    drawRadarBackground(m_radarBackgroundSprite, cx, cy, radius);
    drawRadarOverlay(m_radarOverlaySprite, cx, cy, radius);
    buildOverlayPixelList();
    m_radarOverlaySprite->pushSprite(m_radarBackgroundSprite, 0, 0, C_RADAR_BG);
    if (roundBottomLeft)
        drawWorkCornerMask(m_radarBackgroundSprite);

    m_radarBackgroundW = w;
    m_radarBackgroundH = h;
    m_radarBackgroundCx = cx;
    m_radarBackgroundCy = cy;
    m_radarBackgroundRadius = radius;
    m_radarBackgroundCorner = roundBottomLeft;
    return true;
}

/**
 * @brief Gibt die Overlay-Pixelliste frei.
 */
void WorkshopTool_FlightRadar::deleteOverlayPixelList()
{
    if (m_overlayPixels)
    {
        free(m_overlayPixels);
        m_overlayPixels = nullptr;
    }

    m_overlayPixelCount = 0;
}

/**
 * @brief Extrahiert alle Pixel des Overlay-Sprites, die nicht in der Transparenzfarbe stehen, in eine kompakte Liste aus X, Y und Farbe. Der Zeichenpfad schreibt diese Pixel direkt in den Work-Sprite, statt den Sprite pixelweise mit Transparenzprüfung zu pushen.
 * @return false wenn kein Overlay-Sprite vorliegt oder der Speicher fehlt
 */
bool WorkshopTool_FlightRadar::buildOverlayPixelList()
{
    deleteOverlayPixelList();
    if (!m_radarOverlaySprite)
        return false;

    const uint16_t *src = static_cast<const uint16_t *>(m_radarOverlaySprite->getBuffer());
    if (!src)
        return false;

    const int w = m_radarOverlaySprite->width();
    const int h = m_radarOverlaySprite->height();
    const int total = w * h;
    const uint16_t transparent = swapRgb565(C_RADAR_BG);
    int count = 0;
    for (int i = 0; i < total; ++i)
    {
        if (src[i] != transparent)
            ++count;
    }

    const size_t entries = (count > 0) ? static_cast<size_t>(count) : 1U;
    const size_t bytes = entries * OVERLAY_PIXEL_WORDS * sizeof(uint16_t);
    m_overlayPixels = static_cast<uint16_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!m_overlayPixels)
        m_overlayPixels = static_cast<uint16_t *>(malloc(bytes));

    if (!m_overlayPixels)
    {
        LOG_WARNING("FlightRadar: Overlay-Pixelliste konnte nicht erstellt werden");
        return false;
    }

    uint16_t *out = m_overlayPixels;
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const uint16_t color = src[y * w + x];
            if (color == transparent)
                continue;

            out[0] = static_cast<uint16_t>(x);
            out[1] = static_cast<uint16_t>(y);
            out[2] = color;
            out += OVERLAY_PIXEL_WORDS;
        }
    }

    m_overlayPixelCount = count;
    return true;
}

/**
 * @brief Zeichnet die Overlay-Pixel innerhalb eines Bereichs über den aktuellen Radarinhalt. Ohne gültigen Bereich wird das gesamte Overlay gezeichnet, ohne Pixelliste ersatzweise per Sprite-Push mit Transparenz.
 * @param sprite Work-Sprite des Workshop-Moduls
 * @param limitX Linke Kante des Bereichs im Work-Sprite
 * @param limitY Obere Kante des Bereichs im Work-Sprite
 * @param limitW Breite des Bereichs, 0 oder kleiner für das gesamte Overlay
 * @param limitH Höhe des Bereichs, 0 oder kleiner für das gesamte Overlay
 */
void WorkshopTool_FlightRadar::drawRadarOverlayLayer(Gfx_Sprite *sprite, int limitX, int limitY, int limitW, int limitH)
{
    if (!sprite || !m_radarOverlaySprite)
        return;

    uint16_t *buffer = static_cast<uint16_t *>(sprite->getBuffer());
    const bool boxInside = m_radarBoxX >= 0 && m_radarBoxY >= 0 &&
                           m_radarBoxX + m_radarOverlaySprite->width() <= sprite->width() &&
                           m_radarBoxY + m_radarOverlaySprite->height() <= sprite->height();
    if (!m_overlayPixels || !buffer || !boxInside)
    {
        m_radarOverlaySprite->pushSprite(sprite, m_radarBoxX, m_radarBoxY, C_RADAR_BG);
        return;
    }

    const bool limited = limitW > 0 && limitH > 0;
    const int minX = limitX - m_radarBoxX;
    const int minY = limitY - m_radarBoxY;
    const int maxX = minX + limitW;
    const int maxY = minY + limitH;
    const int spriteW = sprite->width();
    const uint16_t *entry = m_overlayPixels;
    for (int i = 0; i < m_overlayPixelCount; ++i, entry += OVERLAY_PIXEL_WORDS)
    {
        if (limited && (entry[0] < minX || entry[0] >= maxX || entry[1] < minY || entry[1] >= maxY))
            continue;

        buffer[(m_radarBoxY + entry[1]) * spriteW + m_radarBoxX + entry[0]] = entry[2];
    }
}

/**
 * @brief Zeichnet den Sweep in den Ziel-Sprite.
 * @return Verwendeter Sweep-Winkel in Grad, -1.0 wenn nichts gezeichnet wurde
 */
float WorkshopTool_FlightRadar::drawSweep(Gfx_Sprite *sprite, int cx, int cy, int radius)
{
    if (!sprite || radius <= 0)
        return -1.0f;

    const int sweepRadius = radius - SCAN_SWEEP_INSET;
    if (sweepRadius <= 0 || !ensureSweepSprite(sweepRadius))
        return -1.0f;

    const float sweepAngleDeg = currentSweepAngleDeg();
    m_sweepSprite->pushRotateZoom(sprite, cx, cy, sweepAngleDeg, 1.0f, 1.0f, C_RADAR_BG);

    const float sweepRadiusF = static_cast<float>(sweepRadius);
    const float angleRad = sweepAngleDeg * DEG_TO_RAD;
    const float endX = static_cast<float>(cx) + cosf(angleRad) * sweepRadiusF;
    const float endY = static_cast<float>(cy) + sinf(angleRad) * sweepRadiusF;
    sprite->drawWideLine(static_cast<float>(cx),
                         static_cast<float>(cy),
                         endX,
                         endY,
                         SCAN_FRONT_THICKNESS,
                         lgfx::color888(0, SCAN_FRONT_BRIGHTNESS, 0));
    return sweepAngleDeg;
}

/**
 * @brief Zeichnet den Abrufbalken am rechten Displayrand.
 */
void WorkshopTool_FlightRadar::updateProgressBar(bool force)
{
    if (!m_tft || !m_progressSprite)
        return;

    const uint32_t now = millis();
    if (!force && m_lastProgressUpdateMs != 0 &&
        static_cast<int32_t>(now - m_lastProgressUpdateMs) < static_cast<int32_t>(PROGRESS_UPDATE_INTERVAL_MS))
        return;

    m_lastProgressUpdateMs = now;

    const uint32_t cycleStart = g_radar.fetchCycleStartMs.load();
    const uint32_t nextFetch = g_radar.nextFetchMs.load();
    float progress = 0.0f;

    if (cycleStart != 0 && nextFetch != 0 &&
        static_cast<int32_t>(nextFetch - cycleStart) > 0 &&
        static_cast<int32_t>(nextFetch - now) > 0)
    {
        uint32_t interval = nextFetch - cycleStart;
        const uint32_t remaining = nextFetch - now;
        progress = static_cast<float>(remaining) / static_cast<float>(interval);
        progress = constrain(progress, 0.0f, 1.0f);
    }

    const int displayH = m_tft->height();
    const int barHeight = static_cast<int>(displayH * progress);
    const int startY = displayH - barHeight;

    m_progressSprite->drawFastVLine(0, 0, startY, Theme::ColorMainBackGround);
    m_progressSprite->drawFastVLine(0, startY, barHeight, C_PROGRESS_BAR);
    m_progressSprite->pushSprite(m_tft->width() - PROGRESS_BAR_W, 0);
}

/**
 * @brief Initialisiert die Footer-Buttons.
 */
void WorkshopTool_FlightRadar::initFooterButtons(LGFX *tft)
{
    if (!tft || m_buttonsInitialized)
        return;

    const int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    int left = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN;

    for (uint8_t i = 0; i < 4; ++i)
    {
        const int cx = left + RADIUS_BTN_W / 2;
        m_radiusButtons[i].initButton(tft, cx, cy, RADIUS_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R, C_BUTTON_INACTIVE_OUTLINE, C_BUTTON_INACTIVE_FILL, C_BUTTON_INACTIVE_TEXT, RADIUS_LABELS[i], nullptr);
        m_radiusButtons[i].setFont(LeagueGothicCond34);
        left += RADIUS_BTN_W + FOOTER_BTN_GAP;
    }

    const int labelModeCx = left + LABEL_MODE_BTN_W / 2;
    m_labelModeButton.initButton(tft, labelModeCx, cy, LABEL_MODE_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R, C_BUTTON_INACTIVE_OUTLINE, C_BUTTON_INACTIVE_FILL, C_BUTTON_INACTIVE_TEXT, "3", nullptr);
    left += LABEL_MODE_BTN_W + FOOTER_BTN_GAP;

    const int cx = left + HELP_BTN_W / 2;
    m_helpButton.initButton(tft, cx, cy, HELP_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R, C_BUTTON_INACTIVE_OUTLINE, C_BUTTON_INACTIVE_FILL, C_BUTTON_INACTIVE_TEXT, "?", nullptr);
    m_helpButton.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
    updateFooterButtonColors();
}

/**
 * @brief Aktualisiert die Footer-Buttonfarben.
 */
void WorkshopTool_FlightRadar::updateFooterButtonColors()
{
    if (!m_buttonsInitialized)
        return;

    const uint8_t activeIndex = radiusIndex(m_searchRadiusKm);
    for (uint8_t i = 0; i < 4; ++i)
    {
        const bool active = i == activeIndex;
        m_radiusButtons[i].setOutlineColor(active ? C_BUTTON_ACTIVE_OUTLINE : C_BUTTON_INACTIVE_OUTLINE);
        m_radiusButtons[i].setFillColor(active ? C_BUTTON_ACTIVE_FILL : C_BUTTON_INACTIVE_FILL);
        m_radiusButtons[i].setTextColor(active ? C_BUTTON_ACTIVE_TEXT : C_BUTTON_INACTIVE_TEXT);
    }

    m_labelModeButton.setOutlineColor(C_BUTTON_INACTIVE_OUTLINE);
    m_labelModeButton.setFillColor(C_BUTTON_INACTIVE_FILL);
    m_labelModeButton.setTextColor(C_BUTTON_INACTIVE_TEXT);

    m_helpButton.setOutlineColor(C_BUTTON_INACTIVE_OUTLINE);
    m_helpButton.setFillColor(C_BUTTON_INACTIVE_FILL);
    m_helpButton.setTextColor(C_BUTTON_INACTIVE_TEXT);
}

/**
 * @brief Zeichnet den Label-Modus-Button.
 */
void WorkshopTool_FlightRadar::drawLabelModeButtonSymbol(bool pressed)
{
    constexpr uint16_t C_RING_ACTIVE = C_AIRCRAFT_CALLSIGN; ///< Aktive Symbolringe
    constexpr uint16_t C_RING_DIM = C_BUTTON_INACTIVE_OUTLINE; ///< Gedimmte Symbolringe
    constexpr uint16_t C_DOT = 0xFFE0;           ///< Mittlerer Symbolpunkt
    constexpr float INNER_R = 4.0f;              ///< Radius des mittleren Punkts
    constexpr float MIDDLE_R = 10.0f;            ///< Radius des mittleren Rings
    constexpr float OUTER_R = 16.0f;             ///< Radius des äußeren Rings
    constexpr float RING_THICKNESS = 2.4f;       ///< Linienstärke der Symbolringe

    if (!m_tft)
        return;

    m_labelModeButton.setLabelText("");
    m_labelModeButton.drawButton(pressed);

    const int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    const int labelButtonLeft = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN + (4 * (RADIUS_BTN_W + FOOTER_BTN_GAP));
    const int cx = labelButtonLeft + LABEL_MODE_BTN_W / 2;
    uint16_t middleRingColor = C_RING_DIM;
    uint16_t outerRingColor = C_RING_DIM;

    switch (m_labelMode)
    {
    case LabelMode::Full:
        middleRingColor = C_RING_ACTIVE;
        outerRingColor = C_RING_ACTIVE;
        break;
    case LabelMode::Name:
        middleRingColor = C_RING_ACTIVE;
        break;
    case LabelMode::Symbol:
    default:
        break;
    }

    m_tft->startWrite();
    m_tft->drawSmoothCircle(cx, cy, OUTER_R, RING_THICKNESS, outerRingColor, C_BUTTON_INACTIVE_FILL);
    m_tft->drawSmoothCircle(cx, cy, MIDDLE_R, RING_THICKNESS, middleRingColor, C_BUTTON_INACTIVE_FILL);
    m_tft->fillSmoothCircle(cx, cy, INNER_R, C_DOT);
    m_tft->endWrite();
}

/**
 * @brief Schaltet den Umfang der Radar-Beschriftung weiter.
 */
void WorkshopTool_FlightRadar::toggleLabelMode()
{
    switch (m_labelMode)
    {
    case LabelMode::Full:
        m_labelMode = LabelMode::Name;
        break;
    case LabelMode::Name:
        m_labelMode = LabelMode::Symbol;
        break;
    case LabelMode::Symbol:
    default:
        m_labelMode = LabelMode::Full;
        break;
    }

    m_framePending = true;
}

/**
 * @brief Setzt den Suchradius.
 */
void WorkshopTool_FlightRadar::setRadiusKm(uint8_t radiusKm)
{
    const uint8_t normalized = normalizeRadiusKm(radiusKm);
    if (m_searchRadiusKm == normalized)
        return;

    m_searchRadiusKm = normalized;
    APP_STATE.setWorkshopFlightRadarRadiusKm(m_searchRadiusKm);
    configureSharedRadiusKm(m_searchRadiusKm);
    m_lastAircraftCount = -1;
    m_lastDataVersion = 0;
    m_lastProgressUpdateMs = 0;
    m_framePending = true;
    updateFooterButtonColors();
}

/**
 * @brief Liefert die berührte Zeile der Flugzeugliste.
 */
int WorkshopTool_FlightRadar::aircraftListRowAt(int32_t x, int32_t y) const
{
    if (m_aircraftListVisible <= 0 || m_aircraftListW <= 0 || m_aircraftListH <= 0)
        return -1;

    if (x < m_aircraftListX || x >= m_aircraftListX + m_aircraftListW ||
        y < m_aircraftListY + LIST_HEADER_H || y >= m_aircraftListY + m_aircraftListH)
        return -1;

    const int row = static_cast<int>((y - (m_aircraftListY + LIST_HEADER_H)) / LIST_ROW_H);
    return (row >= 0 && row < m_aircraftListVisible) ? row : -1;
}

/**
 * @brief Öffnet den Detaildialog zu einer Flugzeugzeile.
 */
void WorkshopTool_FlightRadar::showAircraftDetailDialog(uint8_t rowIndex)
{
    if (!m_msg || rowIndex >= LIST_ROWS || g_listRowIcao[rowIndex][0] == '\0')
        return;

    Aircraft *items = g_uiAircraftScratch;
    int count = 0;
    RadarStatus status = RadarStatus::Idle;
    if (!copySnapshot(items, count, status, nullptr, DIALOG_LOCK_MS))
        return;

    int aircraftIndex = -1;
    for (int i = 0; i < count; ++i)
    {
        if (strcmp(items[i].icao24, g_listRowIcao[rowIndex]) == 0)
        {
            aircraftIndex = i;
            break;
        }
    }

    if (aircraftIndex < 0)
        return;

    const Aircraft &aircraft = items[aircraftIndex];
    const char *titleName = aircraft.callsign[0] ? aircraft.callsign : aircraft.icao24;
    I18n::format(m_aircraftDetailTitleBuf, sizeof(m_aircraftDetailTitleBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_DETAIL_TITLE_FMT, titleName);

    char unknownBuf[24];
    I18n::format(unknownBuf, sizeof(unknownBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_VALUE_UNKNOWN);

    char groundLabel[24];
    I18n::format(groundLabel, sizeof(groundLabel), WorkshopKey::WORKSHOP_FLIGHT_RADAR_LABEL_GROUND);

    char altitudeBuf[32];
    if (aircraft.onGround)
        stringutils::safeCopy(altitudeBuf, sizeof(altitudeBuf), groundLabel);
    else if (aircraft.altitudeM >= 0)
        stringutils::safeFormat(altitudeBuf, sizeof(altitudeBuf), "%d m", aircraft.altitudeM);
    else
        stringutils::safeCopy(altitudeBuf, sizeof(altitudeBuf), unknownBuf);

    char lastSeenBuf[32];
    if (aircraft.lastSeenMs == 0)
    {
        stringutils::safeCopy(lastSeenBuf, sizeof(lastSeenBuf), unknownBuf);
    }
    else
    {
        const uint32_t ageSec = (millis() - aircraft.lastSeenMs) / 1000UL;
        if (ageSec < 2)
            I18n::format(lastSeenBuf, sizeof(lastSeenBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_VALUE_JUST_NOW);
        else if (ageSec < 60)
            I18n::format(lastSeenBuf, sizeof(lastSeenBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_FMT_SECONDS_AGO, static_cast<unsigned>(ageSec));
        else
            I18n::format(lastSeenBuf, sizeof(lastSeenBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_FMT_MINUTES_AGO, static_cast<unsigned>(ageSec / 60UL));
    }

    char labelBufs[8][24];
    I18n::format(labelBufs[0], sizeof(labelBufs[0]), WorkshopKey::WORKSHOP_FLIGHT_RADAR_DETAIL_CALLSIGN);
    I18n::format(labelBufs[1], sizeof(labelBufs[1]), WorkshopKey::WORKSHOP_FLIGHT_RADAR_DETAIL_ICAO24);
    I18n::format(labelBufs[2], sizeof(labelBufs[2]), WorkshopKey::WORKSHOP_FLIGHT_RADAR_DETAIL_COUNTRY);
    I18n::format(labelBufs[3], sizeof(labelBufs[3]), WorkshopKey::WORKSHOP_FLIGHT_RADAR_DETAIL_DISTANCE);
    I18n::format(labelBufs[4], sizeof(labelBufs[4]), WorkshopKey::WORKSHOP_FLIGHT_RADAR_DETAIL_ALTITUDE);
    I18n::format(labelBufs[5], sizeof(labelBufs[5]), WorkshopKey::WORKSHOP_FLIGHT_RADAR_DETAIL_SPEED);
    I18n::format(labelBufs[6], sizeof(labelBufs[6]), WorkshopKey::WORKSHOP_FLIGHT_RADAR_DETAIL_TRACK);
    I18n::format(labelBufs[7], sizeof(labelBufs[7]), WorkshopKey::WORKSHOP_FLIGHT_RADAR_DETAIL_LAST_SEEN);

    char callsignValue[24];
    char icaoValue[16];
    char countryValue[40];
    char distanceValue[24];
    char speedValue[24];
    char trackValue[24];
    stringutils::safeCopy(callsignValue, sizeof(callsignValue), aircraft.callsign[0] ? aircraft.callsign : unknownBuf);
    stringutils::safeCopy(icaoValue, sizeof(icaoValue), aircraft.icao24[0] ? aircraft.icao24 : unknownBuf);
    stringutils::safeCopy(countryValue, sizeof(countryValue), aircraft.originCountry[0] ? aircraft.originCountry : unknownBuf);
    stringutils::safeFormat(distanceValue, sizeof(distanceValue), "%.1f km", aircraft.distanceKm);
    stringutils::safeFormat(speedValue, sizeof(speedValue), "%.0f km/h", aircraft.velocityMps * MS_TO_KMH);
    I18n::format(trackValue, sizeof(trackValue), WorkshopKey::WORKSHOP_FLIGHT_RADAR_FMT_TRACK, static_cast<double>(aircraft.trackDeg));

    const char *labels[] = {
        labelBufs[0],
        labelBufs[1],
        labelBufs[2],
        labelBufs[3],
        labelBufs[4],
        labelBufs[5],
        labelBufs[6],
        labelBufs[7]
    };
    const char *values[] = {
        callsignValue,
        icaoValue,
        countryValue,
        distanceValue,
        altitudeBuf,
        speedValue,
        trackValue,
        lastSeenBuf
    };

    I18n::format(m_dialogCloseBuf, sizeof(m_dialogCloseBuf), WorkshopKey::WORKSHOP_DIALOG_CLOSE);
    m_msg->setTitle(m_aircraftDetailTitleBuf);
    m_msg->setDetailRows(labels, values, 8);
    m_msg->setButtonLabel(m_dialogCloseBuf);
    m_msg->setTimeout(0);
    m_msg->setAutoBounds(DIALOG_AIRCRAFT_X, 0, DIALOG_AIRCRAFT_W, WorkshopCfg::SCREEN_H);
    m_msg->setCallback([this](bool confirmed)
                       { onMsgResult(confirmed); });

    if (owner())
        owner()->setWorkContentPaused(true);

    m_msg->start();
}

/**
 * @brief Zeichnet die Footer-Buttons.
 */
void WorkshopTool_FlightRadar::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    updateFooterButtonColors();
    for (uint8_t i = 0; i < 4; ++i)
        m_radiusButtons[i].drawButton();

    drawLabelModeButtonSymbol(false);
    m_helpButton.drawButton();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
 */
bool WorkshopTool_FlightRadar::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    if (m_msg && m_msg->isVisible())
        return false;

    bool onRadius[4] = {};
    bool wasRadius[4] = {};
    for (uint8_t i = 0; i < 4; ++i)
    {
        onRadius[i] = touched && m_radiusButtons[i].contains(tx, ty);
        wasRadius[i] = m_radiusButtons[i].isPressed();
        m_radiusButtons[i].press(onRadius[i]);
    }

    const bool onHelp = touched && m_helpButton.contains(tx, ty);
    const bool wasHelp = m_helpButton.isPressed();
    m_helpButton.press(onHelp);

    const bool onLabelMode = touched && m_labelModeButton.contains(tx, ty);
    m_labelModeButton.press(onLabelMode);

    for (uint8_t i = 0; i < 4; ++i)
    {
        if (m_radiusButtons[i].justReleased())
        {
            setRadiusKm(RADIUS_VALUES[i]);
            return true;
        }
    }

    if (m_helpButton.justReleased())
    {
        showHelpDialog();
        return false;
    }

    if (m_labelModeButton.justReleased())
    {
        toggleLabelMode();
        drawLabelModeButtonSymbol(false);
        return false;
    }

    for (uint8_t i = 0; i < 4; ++i)
    {
        if (onRadius[i] != wasRadius[i])
            m_radiusButtons[i].drawButton(onRadius[i]);
    }

    if (onHelp != wasHelp)
        m_helpButton.drawButton(onHelp);

    return false;
}

/**
 * @brief Liefert den Untertitel mit aktuellem Suchradius.
 */
const char *WorkshopTool_FlightRadar::getSubtitle() const
{
    char locationBuf[64];
    copyRadarLocationName(locationBuf, sizeof(locationBuf));
    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_FMT_SUBTITLE, static_cast<unsigned>(m_searchRadiusKm), locationBuf);
    return m_subtitleBuf;
}

/**
 * @brief Öffnet die Kurzanleitung.
 */
void WorkshopTool_FlightRadar::showHelpDialog()
{
    if (!m_msg)
        return;

    I18n::format(m_helpTitleBuf, sizeof(m_helpTitleBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_HELP_TITLE);
    I18n::format(m_helpTextBuf, sizeof(m_helpTextBuf), WorkshopKey::WORKSHOP_FLIGHT_RADAR_HELP_TEXT);
    I18n::format(m_dialogCloseBuf, sizeof(m_dialogCloseBuf), WorkshopKey::WORKSHOP_DIALOG_CLOSE);

    m_msg->setTitle(m_helpTitleBuf);
    m_msg->setMessage(m_helpTextBuf);
    m_msg->setButtonLabel(m_dialogCloseBuf);
    m_msg->setTimeout(0);
    m_msg->setAutoBounds(DIALOG_HELP_X, 0, DIALOG_HELP_W, WorkshopCfg::SCREEN_H);
    m_msg->setCallback([this](bool confirmed)
                       { onMsgResult(confirmed); });

    if (owner())
        owner()->setWorkContentPaused(true);

    m_msg->start();
}

/**
 * @brief Callback der MessageBox.
 */
void WorkshopTool_FlightRadar::onMsgResult(bool confirmed)
{
    (void)confirmed;

    if (m_buttonsInitialized)
    {
        for (uint8_t i = 0; i < 4; ++i)
            m_radiusButtons[i].press(false);

        m_labelModeButton.press(false);
        m_helpButton.press(false);
        updateFooterButtonColors();
        for (uint8_t i = 0; i < 4; ++i)
            m_radiusButtons[i].drawButton();

        drawLabelModeButtonSymbol(false);
        m_helpButton.drawButton();
    }

    if (owner())
        owner()->setWorkContentPaused(false);
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet Sweep, Overlay-Ausschnitt, Marker und Beschriftungen in die Radarbox und hält die gezeichneten Bereiche in g_frameRects fest. Die Flugzeugdaten stammen aus g_uiAircraftScratch.
 * @param sprite Work-Sprite des Workshop-Moduls
 * @param count Anzahl gültiger Einträge in g_uiAircraftScratch
 * @param hasBackground true wenn Hintergrund-Sprite und Overlay vorliegen, sonst werden Raster und Richtungszeichen direkt gezeichnet
 */
void WorkshopTool_FlightRadar::renderRadarLayers(Gfx_Sprite *sprite, int count, bool hasBackground)
{
    const Aircraft *items = g_uiAircraftScratch;
    g_frameRects.sweep = RadarRect();
    g_frameRects.aircraftCount = 0;
    if (!sprite)
        return;

    const int sweepRadius = m_radarRadius - SCAN_SWEEP_INSET;
    const float sweepAngleDeg = drawSweep(sprite, m_radarCx, m_radarCy, m_radarRadius);
    if (sweepAngleDeg >= 0.0f)
        g_frameRects.sweep = sweepWedgeRect(m_radarCx, m_radarCy, sweepRadius, sweepAngleDeg, m_radarBoxX, m_radarBoxY, m_radarBoxW, m_radarBoxH);

    if (hasBackground)
    {
        drawRadarOverlayLayer(sprite, g_frameRects.sweep.x, g_frameRects.sweep.y, g_frameRects.sweep.w, g_frameRects.sweep.h);
    }
    else
    {
        drawRadarGrid(sprite, m_radarCx, m_radarCy, m_radarRadius);
        drawCompassLabels(sprite, m_radarCx, m_radarCy, m_radarRadius);
    }

    if (count > MAX_AIRCRAFT)
        count = MAX_AIRCRAFT;

    const float searchRadiusKm = static_cast<float>(m_searchRadiusKm);
    for (int i = count - 1; i >= 0; --i)
        drawAircraftMarker(sprite, items[i], m_radarCx, m_radarCy, m_radarRadius, searchRadiusKm, m_radarBoxX, m_radarBoxY, m_radarBoxW, m_radarBoxH, &g_frameRects.aircraft[i].marker);

    const uint8_t labelMode = static_cast<uint8_t>(m_labelMode);
    updateAircraftLabelCache(m_tft, items, count, labelMode);
    for (int i = 0; i < count; ++i)
    {
        drawAircraftLabel(sprite, items[i], m_radarCx, m_radarCy, m_radarRadius, searchRadiusKm, m_radarBoxX, m_radarBoxY, m_radarBoxW, m_radarBoxH, &g_frameRects.aircraft[i].label);
        stringutils::safeCopy(g_frameRects.aircraft[i].icao24, items[i].icao24);
    }

    g_frameRects.aircraftCount = count;
}

/**
 * @brief Zeichnet Radar und Liste.
 */
void WorkshopTool_FlightRadar::draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
{
    if (!sprite)
        return;

    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);
        updateProgressBar(false);
        return;
    }

    const uint32_t frameStartMs = millis();

    Aircraft *items = g_uiAircraftScratch;
    int count = (m_lastAircraftCount > 0) ? m_lastAircraftCount : 0;
    RadarStatus status = static_cast<RadarStatus>(m_lastRadarStatus);
    uint32_t dataVersion = m_lastDataVersion;
    copySnapshot(items, count, status, &dataVersion, SNAPSHOT_LOCK_MS);
    if (WiFi.status() != WL_CONNECTED)
    {
        status = RadarStatus::Offline;
        count = 0;
    }

    constexpr int RADAR_MARGIN = 10;

    const int radarLeft = originX;
    const int radarTop = WorkshopCfg::WORK_CONTENT_Y;
    const int radarBottom = originY + h;
    const int radarDiameter = radarBottom - radarTop;
    const int radarR = radarDiameter / 2;
    const int radarCx = radarLeft + radarR;
    const int radarCy = radarTop + radarR;
    const int radarRight = radarLeft + radarDiameter;
    const int panelX = radarRight + RADAR_MARGIN;
    const int panelY = radarTop;
    const int panelW = originX + w - panelX;
    const int panelH = radarDiameter;

    if (radarR <= 0 || panelW <= 0 || panelH <= 0)
    {
        m_aircraftListVisible = 0;
        return;
    }

    m_aircraftListX = panelX;
    m_aircraftListY = panelY;
    m_aircraftListW = panelW;
    m_aircraftListH = panelH;
    m_aircraftListVisible = (count < LIST_ROWS) ? count : LIST_ROWS;
    for (int i = 0; i < LIST_ROWS; ++i)
        stringutils::safeCopy(g_listRowIcao[i], (i < m_aircraftListVisible) ? items[i].icao24 : "");

    constexpr int RADAR_PUSH_PAD = AIRCRAFT_SPRITE_SIZE / 2 + 3;
    constexpr int RADAR_PUSH_PAD_TOP = WorkshopCfg::TITLE_MARGIN;
    constexpr int RADAR_LABEL_PUSH_PAD_Y = 36;
    m_radarBoxX = (radarLeft > RADAR_PUSH_PAD) ? (radarLeft - RADAR_PUSH_PAD) : 0;
    m_radarBoxY = (radarTop > RADAR_PUSH_PAD_TOP) ? (radarTop - RADAR_PUSH_PAD_TOP) : 0;
    const int radarBoxRight = (panelX < sprite->width()) ? panelX : sprite->width();
    const int radarBoxBottomRaw = radarTop + radarDiameter + RADAR_LABEL_PUSH_PAD_Y;
    const int radarBoxBottom = (radarBoxBottomRaw < sprite->height()) ? radarBoxBottomRaw : sprite->height();
    m_radarBoxW = radarBoxRight - m_radarBoxX;
    m_radarBoxH = radarBoxBottom - m_radarBoxY;
    m_radarCx = radarCx;
    m_radarCy = radarCy;
    m_radarRadius = radarR;
    m_radarBoxValid = true;
    m_lastAircraftCount = count;
    m_lastDataVersion = dataVersion;
    m_lastRadarStatus = visibleStatusCode(status, m_lastRadarStatus);

    const int bgCx = radarCx - m_radarBoxX;
    const int bgCy = radarCy - m_radarBoxY;
    const bool cornerBottomLeft = (m_radarBoxX == 0 && m_radarBoxY + m_radarBoxH >= sprite->height());
    const bool hasBackground = ensureRadarBackgroundSprite(m_radarBoxW, m_radarBoxH, bgCx, bgCy, radarR, cornerBottomLeft);

    sprite->setClipRect(m_radarBoxX, m_radarBoxY, m_radarBoxW, m_radarBoxH);
    if (hasBackground)
    {
        if (!blitSpriteRows(sprite, m_radarBackgroundSprite, m_radarBoxX, m_radarBoxY))
            m_radarBackgroundSprite->pushSprite(sprite, m_radarBoxX, m_radarBoxY);
    }
    else
    {
        sprite->fillRect(m_radarBoxX, m_radarBoxY, m_radarBoxW, m_radarBoxH, WorkshopCfg::C_WORK_BG);
        sprite->fillSmoothCircle(radarCx, radarCy, radarR, C_RADAR_BG);
    }

    renderRadarLayers(sprite, count, hasBackground);
    sprite->clearClipRect();
    g_prevFrameRects = g_frameRects;
    g_pushRectCount = 0;
    m_pushWholeBox = true;
    m_listRedrawDueMs = 0;
    m_listPushBands = 0;

    drawAircraftList(sprite, items, count, panelX, panelY, panelW, panelH, status, hasValidOpenSkyBearerToken());
    updateProgressBar(true);
    m_nextFrameMs = frameStartMs + ANIMATION_FRAME_MS;
    m_framePending = false;
}

/**
 * @brief Zeichnet nur die geänderten Bereiche des Radars. Die dynamischen Bereiche des vorherigen Frames werden aus dem Hintergrund wiederhergestellt, danach werden Sweep, Marker und Beschriftungen neu gezeichnet und die betroffenen Bereiche für den Push gesammelt. Neue Daten zeichnen die Liste erst nach LIST_REDRAW_DELAY_MS neu, die Übertragung der Liste wird danach auf LIST_PUSH_BANDS Frames verteilt.
 */
void WorkshopTool_FlightRadar::drawAnimationFrame(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
{
    (void)originX;
    (void)originY;
    (void)w;
    (void)h;

    if (!sprite || !m_radarBoxValid || m_radarRadius <= 0)
        return;

    const uint32_t frameStartMs = millis();

    Aircraft *items = g_uiAircraftScratch;
    int count = (m_lastAircraftCount > 0) ? m_lastAircraftCount : 0;
    RadarStatus status = static_cast<RadarStatus>(m_lastRadarStatus);
    uint32_t dataVersion = m_lastDataVersion;
    copySnapshot(items, count, status, &dataVersion, SNAPSHOT_LOCK_MS);
    if (WiFi.status() != WL_CONNECTED)
    {
        status = RadarStatus::Offline;
        count = 0;
    }

    const uint8_t statusCode = visibleStatusCode(status, m_lastRadarStatus);
    const bool dataChanged = dataVersion != m_lastDataVersion ||
                             count != m_lastAircraftCount ||
                             statusCode != m_lastRadarStatus;
    if (dataChanged)
    {
        m_lastAircraftCount = count;
        m_lastDataVersion = dataVersion;
        m_lastRadarStatus = statusCode;
        m_listRedrawDueMs = frameStartMs + LIST_REDRAW_DELAY_MS;
        if (m_listRedrawDueMs == 0)
            m_listRedrawDueMs = 1;
    }

    const int bgCx = m_radarCx - m_radarBoxX;
    const int bgCy = m_radarCy - m_radarBoxY;
    const bool cornerBottomLeft = (m_radarBoxX == 0 && m_radarBoxY + m_radarBoxH >= sprite->height());
    const bool hasBackground = ensureRadarBackgroundSprite(m_radarBoxW, m_radarBoxH, bgCx, bgCy, m_radarRadius, cornerBottomLeft);

    g_pushRectCount = 0;
    m_pushWholeBox = !hasBackground;

    sprite->setClipRect(m_radarBoxX, m_radarBoxY, m_radarBoxW, m_radarBoxH);
    if (hasBackground)
    {
        restoreRadarRect(sprite, m_radarBackgroundSprite, m_radarBoxX, m_radarBoxY, g_prevFrameRects.sweep);
        for (int i = 0; i < g_prevFrameRects.aircraftCount; ++i)
        {
            restoreRadarRect(sprite, m_radarBackgroundSprite, m_radarBoxX, m_radarBoxY, g_prevFrameRects.aircraft[i].marker);
            restoreRadarRect(sprite, m_radarBackgroundSprite, m_radarBoxX, m_radarBoxY, g_prevFrameRects.aircraft[i].label);
        }
    }
    else
    {
        sprite->fillRect(m_radarBoxX, m_radarBoxY, m_radarBoxW, m_radarBoxH, WorkshopCfg::C_WORK_BG);
        sprite->fillSmoothCircle(m_radarCx, m_radarCy, m_radarRadius, C_RADAR_BG);
    }

    renderRadarLayers(sprite, count, hasBackground);
    sprite->clearClipRect();
    if (hasBackground)
        buildPushRects();

    if (m_listRedrawDueMs != 0 && static_cast<int32_t>(frameStartMs - m_listRedrawDueMs) >= 0)
    {
        m_listRedrawDueMs = 0;
        m_aircraftListVisible = (count < LIST_ROWS) ? count : LIST_ROWS;
        for (int i = 0; i < LIST_ROWS; ++i)
            stringutils::safeCopy(g_listRowIcao[i], (i < m_aircraftListVisible) ? items[i].icao24 : "");

        drawAircraftList(sprite, items, count, m_aircraftListX, m_aircraftListY, m_aircraftListW, m_aircraftListH, status, hasValidOpenSkyBearerToken());
        m_listPushBand = 0;
        m_listPushBands = LIST_PUSH_BANDS;
    }
    else if (m_listPushBands > 0)
    {
        const int bandH = (m_aircraftListH + m_listPushBands - 1) / m_listPushBands;
        const int bandY = m_aircraftListY + bandH * m_listPushBand;
        addPushRect(clipRadarRect(m_aircraftListX, bandY, m_aircraftListW, bandH, 0, 0, sprite->width(), sprite->height()));
        if (++m_listPushBand >= m_listPushBands)
            m_listPushBands = 0;
    }

    g_prevFrameRects = g_frameRects;
    updateProgressBar(false);
    m_nextFrameMs = frameStartMs + ANIMATION_FRAME_MS;
    m_framePending = false;
}

/**
 * @brief Überträgt die gesammelten Bereiche des Animationsframes einzeln zum Display.
 */
bool WorkshopTool_FlightRadar::pushAnimationFrame(LGFX *tft, Gfx_Sprite *sprite, int spriteX, int spriteY)
{
    if (!tft || !sprite || m_pushWholeBox)
        return false;

    for (int i = 0; i < g_pushRectCount; ++i)
    {
        const RadarRect &rect = g_pushRects[i];
        tft->setClipRect(spriteX + rect.x, spriteY + rect.y, rect.w, rect.h);
        sprite->pushSprite(tft, spriteX, spriteY);
    }
    tft->clearClipRect();
    g_pushRectCount = 0;
    return true;
}

// ===================================================================
// Touch und Redraw
// ===================================================================

/**
 * @brief Verarbeitet Tap ohne Umgehen des Abrufbudgets.
 */
bool WorkshopTool_FlightRadar::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);

        m_touchActive = false;
        return false;
    }

    if (touched)
    {
        m_touchActive = true;
        m_touchStartX = tx;
        m_touchStartY = ty;
        return false;
    }

    if (!m_touchActive)
        return false;

    m_touchActive = false;
    const int32_t workX = m_touchStartX - WorkshopCfg::WORK_X;
    const int32_t workY = m_touchStartY - WorkshopCfg::WORK_Y;
    const int rowIndex = aircraftListRowAt(workX, workY);
    if (rowIndex >= 0)
    {
        showAircraftDetailDialog(static_cast<uint8_t>(rowIndex));
        return false;
    }

    const uint32_t now = millis();
    const uint32_t nextFetch = g_radar.nextFetchMs.load();
    if (nextFetch == 0 || static_cast<int32_t>(now - nextFetch) >= 0)
    {
        g_radar.fetchCycleStartMs.store(0);
        g_radar.nextFetchMs.store(0);
        m_lastProgressUpdateMs = 0;
    }

    return true;
}

/**
 * @brief Fordert Animationsframes im begrenzten Takt an.
 */
bool WorkshopTool_FlightRadar::needsAnimationFrame() const
{
    if (!m_radarBoxValid)
        return true;

    if (m_framePending)
        return true;

    const uint32_t now = millis();
    if (m_nextFrameMs == 0 || static_cast<int32_t>(now - m_nextFrameMs) >= 0)
    {
        m_framePending = true;
        return true;
    }

    return false;
}

/**
 * @brief Liefert den Displaybereich für den animierten Radarbereich.
 */
bool WorkshopTool_FlightRadar::getPushBoundingBox(int &x, int &y, int &w, int &h) const
{
    if (!m_radarBoxValid || m_radarBoxW <= 0 || m_radarBoxH <= 0)
        return false;

    x = m_radarBoxX;
    y = m_radarBoxY;
    w = m_radarBoxW;
    h = m_radarBoxH;
    return true;
}
