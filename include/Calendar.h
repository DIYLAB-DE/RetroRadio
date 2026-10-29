#ifndef CALENDAR_H
#define CALENDAR_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <atomic>

#include "BaseModule.h"
#include "Button.h"
#include "Clocks.h"
#include "DS1307_RTC.h"
#include "Gfx.h"
#include "Logger.h"
#include "Symbols.h"

// Kalenderkonfiguration
static constexpr int MAX_UPCOMING_DAYS = 14;     ///< 2 Wochen vorausschauen
static constexpr int MAX_EVENTS_PER_DAY = 5;     ///< Max 5 Events pro Tag
static constexpr int MAX_UPCOMING_EVENTS = 25;   ///< Gesamt-Limit für alle Events
static constexpr int MAX_CONSECUTIVE_ERRORS = 5; ///< Max Fehler vor Abbruch
// ================================================

/**
 * @brief Struktur für einen einzelnen Kalendereintrag
 * @details Enthält alle relevanten Daten für die Darstellung eines Termins
 */
struct CalendarEvent
{
    char startTime[12]; ///< "HH:MM" oder "Ganztägig"
    char summary[64];   ///< Titel des Termins
    bool isAllDay;      ///< Flag für ganztägige Termine
    bool isPast;        ///< Flag für abgelaufene Termine
    char eventType[10]; ///< Event-Typ: "default", "birthday", "holiday"
};

/**
 * @brief Datenstruktur für einen Tag mit Events
 * @details Repräsentiert einen einzelnen Tag in der Upcoming-Ansicht
 */
struct DayWithEvents
{
    char date[11];                            ///< ISO-Datum "YYYY-MM-DD"
    char dayName[4];                          ///< Wochentag-Abkürzung "Mo", "Di", etc.
    int dayOfMonth;                           ///< Tag des Monats (1-31)
    int month;                                ///< Monat (1-12)
    int year;                                 ///< Jahr
    bool isToday;                             ///< Flag, ob es heute ist
    CalendarEvent events[MAX_EVENTS_PER_DAY]; ///< Max [MAX_EVENTS_PER_DAY] Events pro Tag
    int eventCount;                           ///< Anzahl Events an diesem Tag
};

/**
 * @brief Datenstruktur für kommende Events
 * @details Enthält gruppierte Events für die nächsten Tage
 */
struct UpcomingEventsData
{
    DayWithEvents days[MAX_UPCOMING_DAYS]; ///< Max [MAX_UPCOMING_DAYS] Tage mit Events
    int dayCount;                          ///< Anzahl Tage mit Events
    int totalEventCount;                   ///< Gesamtzahl der Events
};

/**
 * @brief Datenstruktur für einen Monat mit Event-Informationen
 * @details Speichert, welche Tage Events haben
 */
struct MonthData
{
    int month;         ///< Monat (1-12)
    int year;          ///< Jahr
    bool hasEvent[32]; ///< Indizes 1-31 für Tage mit Terminen
};

/**
 * @brief Datenstruktur für das aktuelle Datum
 * @details Enthält nur noch die Datumsinformationen vom Server
 */
struct TodayData
{
    int currentDay;   ///< Aktueller Tag (vom Server)
    int currentMonth; ///< Aktueller Monat (vom Server)
    int currentYear;  ///< Aktuelles Jahr (vom Server)
};

/**
 * @brief Thread-sichere Datenstruktur für alle Kalenderdaten
 * @details Wird über Queue zwischen Worker-Task und UI-Thread übertragen
 */
struct CalendarData
{
    TodayData today;                   ///< Heutige Termine und Datumsinformationen
    MonthData currentMonth;            ///< Aktueller Monat mit Event-Markierungen
    MonthData nextMonth;               ///< Nächster Monat mit Event-Markierungen
    UpcomingEventsData upcomingEvents; ///< Kommende Events gruppiert nach Tagen
    unsigned long timestamp;           ///< Zeitstempel der Daten für Validierung
};

/**
 * @brief Callbacks für das Calendar-Modul
 */
struct CalendarCallbacks
{
    void (*onExitRequested)() = nullptr; ///< Wird beim Verlassen aufgerufen
};

/**
 * @brief Thread-sicheres Kalender-Modul mit asynchronem Datenabruf
 * @details Implementiert robuste Kommunikation zwischen UI und Worker-Task
 *
 * @note Nicht kopierbar wegen Task-Handle und atomaren Membern
 */
class Calendar : public BaseModule
{
public:
    /**
     * @brief Konstruktor
     */
    Calendar();

    /**
     * @brief Destruktor
     */
    ~Calendar();

    // Keine Kopien erlauben
    Calendar(const Calendar &) = delete;
    Calendar &operator=(const Calendar &) = delete;

    // Keine Move-Semantik
    Calendar(Calendar &&) = delete;
    Calendar &operator=(Calendar &&) = delete;

    /**
     * @brief Initialisiert das Kalender-Modul
     * @param tft Referenz auf das Display-Objekt
     * @param callbacks Callback-Funktionen für Events
     * @return true bei erfolgreicher Initialisierung
     *
     * @note HTTP-Requests werden über den zentralen HttpService abgewickelt.
     *       HttpService muss vorher initialisiert sein (HTTP_SERVICE.begin()).
     */
    bool init(LGFX &tft, const CalendarCallbacks &callbacks);

    /**
     * @brief Hauptzeichenfunktion - wird zyklisch aufgerufen
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück
     * @return AppModule::CALENDAR
     */
    AppModule type() const override;

    /**
     * @brief Triggert eine nichtblockierende Datenaktualisierung
     * @param force true = ignoriert Rate-Limiting
     */
    void requestRefresh(bool force = false);

private:
    /**
     * @brief Scroll-Status für die Upcoming Events Ansicht
     * @details Verwaltet alle Scroll-bezogenen Zustände
     */
    struct ScrollState
    {
        int scrollOffset = 0;             ///< Aktuelle Scroll-Position in Pixeln
        int contentHeight = 0;            ///< Gesamthöhe des Inhalts
        int viewportHeight = 480;         ///< Sichtbare Höhe
        int maxScroll = 0;                ///< Maximale Scroll-Position
        unsigned long lastScrollTime = 0; ///< Zeitstempel für Smooth-Scrolling
        float scrollVelocity = 0.0f;      ///< Geschwindigkeit für Momentum-Scrolling (float für Präzision)
        bool isDragging = false;          ///< Touch-Drag aktiv
        int dragStartY = 0;               ///< Start-Y beim Drag
        int dragStartOffset = 0;          ///< Scroll-Offset beim Drag-Start
        bool isSnappingBack = false;      ///< Flag für Zurückschnapp-Animation

        /**
         * @brief Setzt alle Scroll-Zustände zurück
         */
        void reset()
        {
            scrollOffset = 0;
            scrollVelocity = 0.0f;
            isDragging = false;
            isSnappingBack = false;
        }
    };

    /**
     * @brief Eintrittsvektor für den Worker-Task
     * @param arg Zeiger auf Calendar-Instanz
     */
    static void TaskEntry(void *arg);

    /**
     * @brief Haupt Arbeitsschleife (läuft in separatem Task)
     */
    void workerLoop();

    /**
     * @brief Core-Funktion für HTTP-Datenabruf (ohne UI)
     * @param outData Zeiger auf empfangene Daten (muss bereits allokiert sein!)
     * @return HTTP-Code oder negativer Wert bei Fehler
     */
    int fetchCalendarDataCore(CalendarData *outData);

    /**
     * @brief Generiert Demo-Daten für Präsentationen
     * @param outData Zeiger auf Zielstruktur für die generierten Daten
     *
     * Generiert realistische deutsche Kalender-Events:
     * - Normale Termine mit Uhrzeiten
     * - Ganztägige Events (Urlaub, Homeoffice, etc.)
     * - Geburtstage
     * - Garantiert mindestens einen Termin für heute
     */
    void generateDemoData(CalendarData *outData);

    /**
     * @brief Füllt Datumsdaten thread-sicher direkt ins Ziel
     * @param out Ziel-Struktur für das aktuelle Datum
     *
     * Statische Hilfsfunktion, die das aktuelle Datum von der RTC holt und direkt in die übergebene Struktur schreibt. Wird vom Worker verwendet, um Thread-Safety-Probleme zu vermeiden.
     */
    static void fillCurrentTime(TodayData &out);

    /**
     * @brief Validiert empfangene Event-Daten
     * @param start Startzeit
     * @param summary Event-Titel
     * @return true wenn Daten gültig sind
     */
    bool isValidEventData(const char *start, const char *summary) const;

    /**
     * @brief Aktualisiert lokale Zeit basierend auf RTC (nur UI-Thread!)
     */
    void updateCurrentTime();

    /**
     * @brief Beendet Worker-Task
     * @param fast true für sofortiges Beenden
     */
    void shutdownWorker(bool fast = false);

    /**
     * @brief Vollredraw nach Sprachwechsel (alle statischen Texte + Sprite-Caches)
     *
     * Wird aus draw() aufgerufen, wenn I18n::consumeLanguageChanged() true liefert. Zeichnet je nach Zustand entweder den Loading-Screen oder die Hauptansicht (Termin-Liste, beide Monatskalender, Buttons, Digitaluhr) komplett neu. Im Demo-Modus werden die Demo-Daten vorher in der neuen Sprache neu erzeugt.
     */
    void redrawAll();

    /**
     * @brief Prüft ob ein bestimmter Tag Events hat (nur für aktuelle Monate)
     * @param day Tag des Monats (1-31)
     * @param month Monat (1-12)
     * @param year Jahr
     * @param checkOnlyCurrentMonths true = nur aktueller/nächster Monat
     * @return true wenn Events vorhanden sind
     */
    bool hasEventForDay(int day, int month, int year, bool checkOnlyCurrentMonths = false) const;

    /**
     * @brief Bestimmt die Farbe basierend auf Event-Typ
     * @param eventType Event-Typ
     * @param isAllDay true für ganztägige Events
     * @return TFT Farbwert
     */
    uint16_t getEventColor(const char *eventType, bool isAllDay) const;

    // UI-Zeichenfunktionen
    void drawEvents();
    void truncateTextToWidth(const char *text, int maxWidth, LGFX_Sprite *sprite, char *output, size_t outputSize) const;
    void drawMonthCalendars(int month, int year, int startY, bool isCurrentMonth = true);

    /**
     * @brief Berechnet die Pixelhöhe eines gerenderten Monatskalenders
     * @param month Monat (1-12)
     * @param year  Jahr
     * @return Höhe in Pixeln (Header + Wochentag-Zeile + alle Zeilen des Grids)
     *
     * Die Berechnung spiegelt exakt die Geometrie von drawMonthCalendars() wider: 25 px Header + 30 px Grid-Offset + rowsNeeded × CELL_HEIGHT. Wird verwendet, um die Y-Position des nachfolgenden Kalenders dynamisch zu bestimmen, anstatt sie hart zu kodieren.
     */
    int calculateMonthCalendarHeight(int month, int year) const;

    void drawMonthHeader(int month, int year, int centerX, int y);
    int getDaysInMonth(int month, int year) const;
    int getStartDayOfWeek(int month, int year) const;
    int getCalendarWeek(int day, int month, int year) const;
    void drawDigitalClock(int32_t x, int32_t y);

    // Scroll-Funktionen
    /**
     * @brief Verarbeitet Touch-Input für Scrolling
     * @param tx Touch X-Koordinate
     * @param ty Touch Y-Koordinate
     * @param touched True wenn gerade berührt wird
     */
    void handleScrollTouch(int32_t tx, int32_t ty, bool touched);

    /**
     * @brief Aktualisiert Scroll-Physics (Momentum, Dämpfung)
     */
    void updateScrollPhysics();

    /**
     * @brief Zeichnet die Scrollbar
     */
    void drawScrollbar();

    /**
     * @brief Berechnet die Gesamthöhe des scrollbaren Contents
     * @return Höhe in Pixeln
     */
    int calculateContentHeight() const;

    /**
     * @brief Begrenzt die Scroll-Position auf gültige Werte mit Elastic-Effekt
     */
    void constrainScroll();

    // ========================================================================
    // MEMBER-VARIABLEN
    // ========================================================================

    // Core-Member
    LGFX *m_tft = nullptr;
    CalendarCallbacks m_callbacks;
    bool m_initialized = false;
    Button m_exitButton;
    Button m_cancelLoadButton;

    // Aktuelle Kalenderdaten (UI-Thread) - als Pointer für Heap-Allokierung
    CalendarData *m_currentData = nullptr;
    bool m_hasValidData = false;

    // Scroll-State
    ScrollState m_scrollState;
    int32_t m_lastTouchY = -1;         ///< Letzte Touch-Position für Velocity-Berechnung
    unsigned long m_lastTouchTime = 0; ///< Zeitstempel letzter Touch

    // Threading und Synchronisation (alle thread-shared als atomic!)
    std::atomic<TaskHandle_t> m_taskHandle{nullptr}; ///< Handle des Worker-Tasks
    QueueHandle_t m_dataQueue = nullptr;
    std::atomic<bool> m_shutdownRequested{false};
    std::atomic<bool> m_refreshRequested{false};
    std::atomic<bool> m_inFlight{false};

    // Timing und Rate-Limiting
    static constexpr unsigned long API_CALL_INTERVAL = 15UL * 60UL * 1000UL; ///< 15 Minuten
    static constexpr unsigned long MIN_RETRY_INTERVAL = 30000UL;             ///< 30 Sekunden min. Retry
    static constexpr unsigned long MAX_BACKOFF_TIME = 480000UL;              ///< 8 Minuten max. Backoff

    unsigned long m_lastApiCall = 0;
    unsigned long m_backoffMs = 0;
    std::atomic<unsigned long> m_nextAllowedFetch{0}; ///< Frühester erlaubter nächster Fetch (thread-safe)
    int m_consecutiveErrors = 0;

    // Sprite für Upcoming Events
    Gfx_Sprite *m_upcomingSprite = nullptr;  ///< Sprite für die Termine-Liste
    Gfx_Sprite *m_scrollbarSprite = nullptr; ///< Sprite für die Scrollbar
    bool m_upcomingSpriteValid = false;      ///< Flag ob Sprite-Inhalt aktuell ist
    int m_lastScrollOffset = -1;             ///< Letzter gezeichneter Scroll-Offset

    // Digitaluhr
    Gfx_Sprite *m_clockSprite = nullptr; ///< Sprite für die Digitaluhr
    int m_lastDrawnSecond = -1;          ///< Letzte gezeichnete Sekunde

    // UI-State
    bool m_layoutDrawn = false;
    Gfx_Sprite *m_progressSprite = nullptr;
    unsigned long m_lastProgressUpdate = 0;
    unsigned long m_workerLastSeen = 0; ///< Watchdog für Worker-Task

    const char *m_appsScriptUrl = nullptr; ///< URL zum Google Apps Script (aus AppState)
    bool m_demoMode = false;               ///< Runtime Demo-Modus (aktiv wenn keine gültige URL)
};

#endif // CALENDAR_H
