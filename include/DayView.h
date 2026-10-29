#ifndef DAY_VIEW_H
#define DAY_VIEW_H

// ============================================================================
// DayView.h - Tages-Dashboard für RetroRadio
// ============================================================================
// Zeigt alle wichtigen Tagesinformationen auf einen Blick: Kalender-Termine, offene Aufgaben, Wetter mit Icon, Geburtstage und Radio-Steuerung. Die Daten kommen asynchron und jede Sektion wird sofort gezeichnet, sobald ihre Daten verfügbar sind.
// ============================================================================

#include <freertos/FreeRTOS.h> // FreeRTOS-Kern
#include <freertos/queue.h>    // FreeRTOS-Queues für Thread-Kommunikation
#include <freertos/task.h>     // FreeRTOS-Tasks für Worker-Thread

#include <atomic>  // std::atomic für Thread-sichere Flags
#include <cstdint> // für uint8_t, uint16_t, etc.
#include <memory>  // std::unique_ptr für automatische Speicherverwaltung
#include <vector>  // std::vector für flexible Arrays (z.B. Audio-Events)

#include "BaseModule.h"    // Basisklasse für Module
#include "Button.h"        // Button-Definitionen
#include "DotMatrix.h"     // DotMatrix-Display für Lautstärke und Senderanzeige
#include "DS1307_RTC.h"    // RTC für Uhrzeit
#include "FileExplorer.h"  // Dateisystem-Zugriff für SD-Player
#include "Gfx.h"           // Grafische Hilfsfunktionen und Sprite-Management
#include "Logger.h"        // Logging-Funktionen
#include "MessageBox.h"    // MessageBox für Fehlermeldungen
#include "PresetManager.h" // Preset-Verwaltung
#include "StationPicker.h" // Sender-Auswahl-Dialog
#include "Symbols.h"       // Vektor-Symbole
#include "QRCodeOverlay.h" // QR-Code Overlay für News-Links

// Forwärtsdeklarationen
class StationManager;
class Audio;

// ============================================================================
// DayView-Konfiguration
// ============================================================================
namespace DayViewCfg
{
    constexpr int MAX_EVENTS = 10;   ///< Max angezeigte Kalender-Termine (über mehrere Tage)
    constexpr int MAX_BIRTHDAYS = 3; ///< Max angezeigte Geburtstage
    constexpr int MAX_TASKS = 10;    ///< Max angezeigte Aufgaben
    constexpr int QUEUE_SIZE = 4;    ///< Queue-Slots (partielle Updates + Final)
} // namespace DayViewCfg

// ============================================================================
// Datenstrukturen
// ============================================================================

/**
 * @brief Kompakter Kalender-Termin für DayView
 *
 * Tages-Header werden zur Zeichenzeit aus den Roh-Feldern materialisiert, damit ein Sprachwechsel keine Worker-Aktualisierung erzwingt.
 */
struct DayViewEvent
{
    char time[6];       ///< "HH:MM" oder leer bei Ganztags-Events
    char summary[48];   ///< Titel (gekürzt)
    char eventType[10]; ///< Event-Typ: "default", "holiday" (für Balkenfarbe)
    bool isAllDay;      ///< Ganztags-Event
    bool isPast;        ///< Event bereits vorbei
    bool isToday;       ///< Termin ist heute
    bool hasDayHeader;  ///< true = vor diesem Termin neuen Tages-Header zeichnen
    bool isTodayHeader; ///< nur gültig wenn hasDayHeader: true = "Heute", false = Datums-Header
    int8_t weekdayIdx;  ///< 0=So..6=Sa, oder -1 (nur gesetzt bei !isTodayHeader)
    int8_t monthIdx;    ///< 0=Jan..11=Dez, oder -1 (nur gesetzt bei !isTodayHeader)
    int8_t dayOfMonth;  ///< 1..31, oder -1 (nur gesetzt bei !isTodayHeader)
};

/**
 * @brief Geburtstags-Eintrag für DayView
 */
struct DayViewBirthday
{
    char name[48]; ///< Name der Person
};

/**
 * @brief Kompakte Aufgabe für DayView
 */
struct DayViewTask
{
    char title[48]; ///< Aufgaben-Titel (gekürzt)
    bool isOverdue; ///< Überfällig
    bool isDueToday; ///< Heute fällig
};

/**
 * @brief Kompakte Wetterdaten für DayView
 *
 * Die Beschreibung wird zur Zeichenzeit aus `weatherCode` über `wmoToKey()` und `STR(...)` aufgelöst, damit ein Sprachwechsel kein erneutes Fetch erfordert.
 */
struct DayViewWeather
{
    float temperature; ///< Aktuelle Temperatur mit Zehntelgrad (°C)
    int feelsLike;     ///< Gefühlte Temperatur gerundet (°C)
    int windSpeed;     ///< Windgeschwindigkeit gerundet (km/h)
    int humidity;      ///< Luftfeuchtigkeit (%)
    int weatherCode;   ///< WMO-Wettercode
    bool isDay;        ///< Tag/Nacht
};

/**
 * @brief Bitmask für verfügbare Daten-Sektionen
 *
 * Ermöglicht progressive Anzeige: Jede Sektion wird gezeichnet, sobald ihr Flag gesetzt ist.
 */
enum DayViewSection : uint8_t
{
    SECTION_NONE = 0x00,
    SECTION_CALENDAR = 0x01, ///< Termine + Geburtstage verfügbar
    SECTION_TASKS = 0x02,    ///< Aufgaben verfügbar
    SECTION_WEATHER = 0x04,  ///< Wetterdaten verfügbar
    SECTION_NEWS = 0x08,     ///< Breaking News (Eilmeldung) verfügbar
    SECTION_ALL = 0x0F       ///< Alle Sektionen komplett
};

/**
 * @brief Gesamtdaten-Struktur für den DayView Worker
 *
 * Wird nach jedem erfolgreichen API-Fetch per Queue gesendet. Das Feld `availableSections` zeigt an, welche Bereiche gültig sind.
 */
struct DayViewData
{
    // Kalender-Termine
    DayViewEvent events[DayViewCfg::MAX_EVENTS];
    int eventCount;      ///< Gespeicherte Termine (<= MAX_EVENTS)
    int eventTotalCount; ///< Gesamtzahl von API (für Badge "+X weitere")

    // Geburtstage (nur heute)
    DayViewBirthday birthdays[DayViewCfg::MAX_BIRTHDAYS];
    int birthdayCount;      ///< Gespeicherte Geburtstage (<= MAX_BIRTHDAYS)
    int birthdayTotalCount; ///< Gesamtzahl von API

    // Offene Aufgaben
    DayViewTask tasks[DayViewCfg::MAX_TASKS];
    int taskCount;      ///< Gespeicherte Aufgaben (<= MAX_TASKS)
    int taskTotalCount; ///< Gesamtzahl von API (für Badge "+X weitere")

    // Wetter
    DayViewWeather weather;

    // Meta
    uint8_t availableSections; ///< Bitmask: welche Sektionen gültig sind
    unsigned long timestamp;   ///< millis() zum Zeitpunkt der Datenerhebung
    bool calendarIsDemo;       ///< true wenn Kalender-Daten aus Demo stammen
    bool tasksIsDemo;          ///< true wenn Aufgaben-Daten aus Demo stammen

    // Breaking News (Tagesschau Eilmeldung)
    char newsHeadline[256]; ///< Headline der aktuellen Meldung
    char newsId[64];        ///< Meldungs-ID (Duplikat-Erkennung)
    char newsShareUrl[200]; ///< Artikel-URL für QR-Code (shareURL, Fallback detailsweb; leer wenn kein Link)
    bool newsAvailable;     ///< true wenn eine anzuzeigende Meldung vorliegt
    bool newsIsBreaking;    ///< true = Eilmeldung, false = Top-Meldung (news[0])
};

/**
 * @brief Audio-Quelle für gegenseitigen Ausschluss Radio/SD-Player
 */
enum class AudioSource : uint8_t
{
    NONE,     ///< Keine Wiedergabe aktiv
    RADIO,    ///< Radio-Stream aktiv
    SD_PLAYER ///< SD-Datei aktiv
};

/**
 * @brief Callbacks für das DayView-Modul
 */
struct DayViewCallbacks
{
    void (*onExitRequested)() = nullptr;                 ///< Wird beim Verlassen aufgerufen
    void (*requestPlayStation)(uint8_t id) = nullptr;    ///< Sender starten (-> setNewStation)
    void (*requestStopPlayback)() = nullptr;             ///< Wiedergabe stoppen (-> stopCurrentStation)
    void (*onVolumeChanged)(uint8_t volume) = nullptr;   ///< Lautstärke geändert (-> handleVolumeChanged)
    void (*requestPlayFile)(const char *path) = nullptr; ///< SD-Datei abspielen (-> connecttoFS)
    void (*requestPauseResume)() = nullptr;              ///< Pause/Resume umschalten (-> audio.pauseResume)
};

/**
 * @brief Tages-Dashboard-Modul mit progressiver Datenanzeige
 *
 * Architektur:
 * - Layout wird sofort bei init() gezeichnet (kein Ladescreen)
 * - Worker-Task auf Core 1 holt APIs sequentiell ab
 * - Nach jedem Fetch -> partielles Update per Queue -> Sektion erscheint
 * - Radio-Steuerung direkt im UI-Thread (kein HTTP nötig)
 * - Progress-Bar am rechten Rand zeigt Refresh-Countdown
 */
class DayView : public BaseModule
{
public:
    DayView();
    ~DayView();

    // Keine Kopien / Moves (atomare Member, Task-Handles)
    DayView(const DayView &) = delete;
    DayView &operator=(const DayView &) = delete;
    DayView(DayView &&) = delete;
    DayView &operator=(DayView &&) = delete;

    /**
     * @brief Initialisiert das DayView-Modul
     * @param tft Referenz auf das Display-Objekt
     * @param stationManager Referenz auf den Station Manager (für Sendernamen)
     * @param audio Referenz auf die Audio-Instanz (für isRunning-Abfrage)
     * @param callbacks Callback-Funktionen
     * @return true bei Erfolg
     *
     * Zeichnet Layout sofort, erstellt Sprites und startet Worker-Task. Kein Ladescreen - Daten füllen die Sektionen progressiv.
     */
    bool init(LGFX &tft, StationManager &stationManager, Audio &audio, const DayViewCallbacks &callbacks);

    /**
     * @brief Hauptzeichenfunktion - wird zyklisch aufgerufen
     *
     * Empfängt partielle Daten-Updates aus der Queue, zeichnet nur die frisch geladenen Sektionen. Aktualisiert Uhr, Radio-Status und Progress-Bar jede Sekunde.
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück
     */
    AppModule type() const override;

private:
    // ========================================================================
    // Worker-Task
    // ========================================================================

    static void workerEntry(void *arg); ///< Statischer Entry-Point für den Worker-Task
    void workerLoop();                  ///< Haupt-Arbeitsschleife

    /**
     * @brief Kurze, abbrechbare Pause zwischen zwei aufeinanderfolgenden API-Abrufen
     *
     * Wartet INTER_FETCH_GAP Millisekunden (vorzeitig beendet durch eine Task-Notification, z.B. beim Shutdown), damit die LCD-DMA zwischen zwei Abrufen wieder Frames aus dem Framebuffer nachholen kann und nicht mehrere PSRAM-intensive Abrufe direkt aufeinandertreffen.
     */
    void pauseBetweenFetches();

    int fetchCalendar(DayViewData *outData);     ///< Kalender + Geburtstage
    int fetchTasks(DayViewData *outData);        ///< Offene Aufgaben
    int fetchWeather(DayViewData *outData);      ///< Open-Meteo Wetter
    int fetchBreakingNews(DayViewData *outData); ///< Tagesschau Eilmeldung

    /**
     * @brief Sendet partielles Update an UI-Queue (Kopie!)
     * @param data Aktuelle akkumulierte Daten
     * @return true wenn erfolgreich eingereiht
     */
    bool sendPartialUpdate(const DayViewData *data);

    void generateDemoData(DayViewData *outData); ///< Demo-Daten generieren
    void shutdownWorker(bool fast = false);      ///< Worker sauber beenden
    void cleanupInitResources();                 ///< Sprites + Symbols freigeben (für Fehlerpfade + Destruktor)

    // ========================================================================
    // UI-Zeichenfunktionen
    // ========================================================================

    void drawLayout();                                                                                                    ///< Statisches Layout (Rahmen, Labels, Buttons)
    void drawStaticLabels();                                                                                              ///< Vertikale Trennlinie zwischen den Spalten (für drawLayout und redrawAll)
    void redrawAll();                                                                                                     ///< Vollredraw nach Sprachwechsel (clear + alle Sektionen neu)
    void drawHeaderSection();                                                                                             ///< Uhr + Wetter + Ort/Datum
    void drawEventsSection();                                                                                             ///< Kalender-Termine links
    void drawTasksSection();                                                                                              ///< Aufgaben rechts
    void drawBottomSection();                                                                                             ///< Geburtstage
    void drawSectionLoading(Gfx_Sprite *spr, int sprW, int sprH, int displayX, int displayY, const char *text);           ///< Lade-Hinweis
    void drawRadioBar();                                                                                                  ///< Radio-Steuerung unten
    void drawVolumeDisplay();                                                                                             ///< DotMatrix-Lautstärkeanzeige aktualisieren
    void drawStationDisplay();                                                                                            ///< DotMatrix-Senderanzeige aktualisieren
    void processAudioEvents();                                                                                            ///< Audio-Events verarbeiten (Metadaten für Info-Bar)
    void drainStaleAudioEvents();                                                                                         ///< Verwirft alle gepufferten Audio-Events (gehören dem alten Titel/Sender)
    bool uiDrawAllowed() const;                                                                                           ///< true wenn kein modaler Dialog aktiv und GFX nicht blockiert ist
    void drawProgressBar();                                                                                               ///< Refresh-Countdown am rechten Rand
    void drawWeatherIcon(int wmoCode, bool isDay, int x, int y, int width, uint16_t color, Gfx_Sprite *target = nullptr); ///< WMO-Icon

    /**
     * @brief Bestimmt die Balkenfarbe basierend auf Event-Typ
     * @param eventType Event-Typ ("default", "holiday")
     * @param isAllDay true für ganztägige Events
     * @return TFT Farbwert
     */
    uint16_t getEventBarColor(const char *eventType, bool isAllDay) const;

    /**
     * @brief Kürzt Text auf maximale Pixelbreite mit "..." (UTF-8 sicher)
     * @param text Eingabetext
     * @param maxWidth Max. Breite in Pixeln
     * @param output Ausgabe-Buffer
     * @param outputSize Größe des Ausgabe-Buffers
     */
    void truncateTextToWidth(const char *text, int maxWidth, char *output, size_t outputSize) const;
    void truncateTextToWidth(Gfx_Sprite *sprite, const char *text, int maxWidth, char *output, size_t outputSize) const;

    // ========================================================================
    // Radio-Steuerung
    // ========================================================================

    void handleVolumeTouches(int16_t tx, int16_t ty);
    void radioPlayStop();
    void radioNextStation();
    void radioPrevStation();

    // ========================================================================
    // SD-Player Steuerung
    // ========================================================================

    void sdBrowse();              ///< FileExplorer öffnen (Dateien/Verzeichnisse wählen)
    void sdPlayStop();            ///< SD-Wiedergabe starten/stoppen
    void sdNextTrack();           ///< Nächsten Track abspielen
    void sdClearPlaylist();       ///< Playlist leeren + Wiedergabe stoppen
    void sdStartTrack(int index); ///< Track an Index abspielen
    void sdHandleEof();           ///< EOF-Event: nächster Track oder Playlist-Ende
    void stopCurrentSource();     ///< Aktive Audio-Quelle stoppen (Radio oder SD) und Alt-Events verwerfen
    void drawSdButtons();         ///< SD-Buttons zeichnen

    // ========================================================================
    // Member-Variablen
    // ========================================================================

    // Core
    LGFX *m_tft = nullptr;                          ///< Display-Referenz
    DayViewCallbacks m_callbacks;                   ///< Exit + Radio Callbacks
    StationManager *m_stationManager = nullptr;     ///< Station Manager für Sendernamen
    Audio *m_audio = nullptr;                       ///< Audio-Instanz für isRunning()
    Symbols *m_symbols = nullptr;                   ///< Wetter-Symbol-Renderer
    Button m_exitButton;                            ///< Power-Button unten rechts
    Button m_radioFavBtn;                           ///< Radio: Favoriten-Dialog
    Button m_radioPrevBtn;                          ///< Radio: vorheriger Sender
    Button m_radioPlayBtn;                          ///< Radio: Play/Stop Toggle
    Button m_radioNextBtn;                          ///< Radio: nächster Sender
    Button m_volDownBtn;                            ///< Lautstärke: leiser
    Button m_volUpBtn;                              ///< Lautstärke: lauter
    DotMatrix m_volDotMatrix;                       ///< DotMatrix-Anzeige für Lautstärke
    Gfx_Sprite *m_volSprite = nullptr;              ///< Sprite für DotMatrix-Rendering (Lautstärke)
    DotMatrix m_stnDotMatrix;                       ///< DotMatrix-Anzeige für Sender-Index
    Gfx_Sprite *m_stnSprite = nullptr;              ///< Sprite für DotMatrix-Rendering (Sender)
    DotMatrix m_clockDotMatrix;                     ///< DotMatrix-Anzeige für Uhrzeit im Header
    DotMatrix m_tempDotMatrix;                      ///< DotMatrix-Anzeige für Temperatur im Header
    std::unique_ptr<PresetManager> m_presetDlg;     ///< Favoriten-Auswahldialog
    std::unique_ptr<StationPicker> m_stationPicker; ///< Sender-Direktwahl Grid-Dialog

    // Daten (UI-Thread)
    DayViewData *m_currentData = nullptr;  ///< Aktuelle Anzeigedaten
    DayViewData *m_previousData = nullptr; ///< Vorherige Daten (Change-Detection)

    // Threading
    std::atomic<TaskHandle_t> m_taskHandle{nullptr};
    QueueHandle_t m_dataQueue = nullptr;
    std::atomic<bool> m_shutdownRequested{false};
    std::atomic<bool> m_refreshRequested{false};

    // Timing
    static constexpr unsigned long NEWS_INTERVAL = 600UL * 1000UL;           ///< News-Takt (auch Skala der Fortschrittsanzeige)
    static constexpr unsigned long WEATHER_INTERVAL = 20UL * 60UL * 1000UL;  ///< Wetter-Takt
    static constexpr unsigned long CALENDAR_INTERVAL = 35UL * 60UL * 1000UL; ///< Kalender-Takt
    static constexpr unsigned long TODO_INTERVAL = 50UL * 60UL * 1000UL;     ///< Aufgaben-Takt
    static constexpr unsigned long INTER_FETCH_GAP = 750UL;                  ///< Pause zwischen zwei Abrufen im selben Durchlauf
    static constexpr unsigned long REDRAW_DELAY_MS = 300UL;                 ///< Wartezeit zwischen Dateneingang und Neuzeichnen, damit der Sprite-Schub nicht in den WLAN-Nachlauf des Abrufs fällt
    static constexpr unsigned long MIN_RETRY_INTERVAL = 10000UL;
    static constexpr unsigned long MAX_BACKOFF_TIME = 120000UL;
    static constexpr size_t WORKER_STACK_SIZE = 16384;
    static constexpr UBaseType_t WORKER_PRIORITY = 1;
    static constexpr BaseType_t WORKER_CORE = 1;

    unsigned long m_lastRefresh = 0;
    unsigned long m_lastWeatherFetch = 0;  ///< Zeitpunkt des letzten erfolgreichen Wetter-Abrufs
    unsigned long m_lastCalendarFetch = 0; ///< Zeitpunkt des letzten erfolgreichen Kalender-Abrufs
    unsigned long m_lastTodoFetch = 0;     ///< Zeitpunkt des letzten erfolgreichen Aufgaben-Abrufs
    unsigned long m_lastNewsFetch = 0;     ///< Zeitpunkt des letzten erfolgreichen News-Abrufs
    unsigned long m_backoffMs = 0;
    int m_consecutiveErrors = 0;

    // Sprites
    Gfx_Sprite *m_headerSprite = nullptr;   ///< Uhr+Wetter-Text
    Gfx_Sprite *m_progressSprite = nullptr; ///< 1px Progressbar rechter Rand
    Gfx_Sprite *m_eventsSprite = nullptr;   ///< Termine-Sektion
    Gfx_Sprite *m_tasksSprite = nullptr;    ///< Aufgaben-Sektion
    Gfx_Sprite *m_bottomSprite = nullptr;   ///< Info-Bar (Radio/SD/News-Zeile)
    int m_lastDrawnMinute = -1;             ///< Letzte gezeichnete Minute
    bool m_dirtyHeader = false;             ///< Header nach Wetter-Eingang neu zu zeichnen
    bool m_dirtyEvents = false;             ///< Terminspalte nach Kalender-Eingang neu zu zeichnen
    bool m_dirtyTasks = false;              ///< Aufgabenspalte (inkl. Geburtstage) nach Dateneingang neu zu zeichnen
    bool m_dirtyBottom = false;             ///< Info-Bar nach neuer Meldung neu zu zeichnen
    int m_headerTextX = 0;                  ///< Feste X-Position für Ort/Datum
    unsigned long m_lastProgressUpdate = 0;

    // UI-State
    bool m_demoMode = false;
    uint8_t m_lastRadioStation = 0xFF; ///< Letzte angezeigte Station (Redraw-Check)
    bool m_lastRadioPlaying = false;   ///< Letzter bekannter Play-Status
    uint8_t m_lastVolume = 0xFF;       ///< Letzte angezeigte Lautstärke (Redraw-Check)
    unsigned long m_lastTouchMs = 0;   ///< Debounce-Zeitstempel für Touch-Events
    bool m_wasTouching = false;        ///< Touch-Zustand des Vorframes (Frischtouch-Flanke)
    int8_t m_volHeldDir = 0;           ///< Gehaltener Volume-Button: -1=leiser, +1=lauter, 0=keiner

    // Radio-Metadaten (Info-Bar im Bottom-Sprite)
    static constexpr size_t RADIO_EVENT_BUF_SIZE = 256; ///< Entkoppelt von AudioManager::MAX_EVENT_STRING_LENGTH
    char m_radioStation[64] = {};                       ///< Aktueller Sendername (bester aus Event/Preset)
    char m_radioTitle[256] = {};                        ///< Stream-Titel (ICY Metadata)
    char m_radioBitrate[16] = {};                       ///< Bitrate-String ("320 kbps")
    char m_radioEventText[RADIO_EVENT_BUF_SIZE] = {};   ///< Event-Buffer
    char m_radioEventStation[64] = {};                  ///< Sendername aus STATION_NAME-Event
    bool m_radioInfoDirty = false;                      ///< true wenn Info-Bar neu gezeichnet werden muss

    // Radio-Button Repeat-State (Vor/Zurück gedrückt halten)
    int8_t m_radioRepeatDir = 0;          ///< 0=kein Repeat, -1=Prev, +1=Next
    unsigned long m_radioRepeatStart = 0; ///< Zeitpunkt des ersten Drückens
    unsigned long m_radioRepeatLast = 0;  ///< Zeitpunkt der letzten Repeat-Aktion

    // Breaking News (Tagesschau Eilmeldung)
    bool m_newsActive = false;                ///< Info-Bar zeigt Meldung statt Radio
    bool m_newsIsBreaking = false;            ///< true = Eilmeldung (rot), false = Top-Meldung
    char m_lastNewsId[64] = {};               ///< ID der zuletzt verarbeiteten Eilmeldung
    char m_newsTickerText[320] = {};          ///< Formatierte Meldungszeile "+++ headline +++"
    int m_newsTestCycle = 0;                  ///< Test-Modus Zykluszähler (nur bei NEWS_TEST_MODE)
    char m_newsShareUrl[200] = {};            ///< Artikel-URL der aktuellen Meldung (für QR-Code-Overlay)
    QRCodeOverlay m_qrCodeOverlay;            ///< QR-Code Overlay für News-Links
    bool m_newsBarTouchConsumed = false;      ///< Rising-Edge-Detect: verhindert Dauerfeuer bei gehaltenem Touch

    // API-URLs (aus AppState, nicht freigeben!)
    const char *m_calendarUrl = nullptr; ///< Kalender-Script URL
    const char *m_todoUrl = nullptr;     ///< ToDo-Script URL

    // ========================================================================
    // SD-Player
    // ========================================================================

    AudioSource m_audioSource = AudioSource::NONE; ///< Aktive Audio-Quelle

    // SD-Buttons
    Button m_sdBrowseBtn; ///< SD: Dateien/Verzeichnisse auswählen
    Button m_sdPlayBtn;   ///< SD: Play/Stop Toggle
    Button m_sdNextBtn;   ///< SD: nächster Track
    Button m_sdClearBtn;  ///< SD: Playlist leeren

    // FileExplorer (modal, auf Abruf)
    std::unique_ptr<FileExplorer> m_fileExplorer; ///< Dateiauswahl-Dialog

    // MessageBox (modal, auf Abruf)
    std::unique_ptr<MessageBox> m_messageBox; ///< Bestätigungsdialog (SD-Clear)

    // Playlist (flüchtig, kein Persist)
    std::vector<FileResult> m_sdPlaylist; ///< SD-Playlist (Dateipfade)
    int m_sdCurrentTrack = -1;            ///< Aktueller Track-Index (-1 = keiner)
    bool m_sdPlaying = false;             ///< SD-Wiedergabe aktiv
    bool m_sdPaused = false;              ///< SD-Wiedergabe pausiert
    bool m_sdPlaylistFinished = false;    ///< Playlist komplett abgespielt

    // SD-Metadaten (Info-Bar)
    char m_sdArtist[64] = {};          ///< ID3-Artist
    char m_sdTitle[128] = {};          ///< ID3-Title oder Dateiname-Fallback
    char m_sdBitrate[16] = {};         ///< Bitrate-String ("320 kbps")
    bool m_sdInfoDirty = false;        ///< Info-Bar muss aktualisiert werden
    bool m_sdWaitingForStart = false;  ///< Warten auf Playback-Start nach requestPlayFile
    unsigned long m_sdStartWaitMs = 0; ///< Zeitstempel für Playback-Start-Timeout
};

#endif // DAY_VIEW_H
