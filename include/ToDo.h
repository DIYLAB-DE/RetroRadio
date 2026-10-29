#ifndef TODO_H
#define TODO_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <atomic>
#include <memory>

#include "BaseModule.h"
#include "Button.h"
#include "Clocks.h"
#include "DS1307_RTC.h"
#include "Gfx.h"
#include "Logger.h"
#include "MessageBox.h"
#include "ProgressBox.h"
#include "Symbols.h"


// ============================================================================
// TODO-KONFIGURATION
// ============================================================================

namespace ToDoCfg
{
    constexpr int MAX_TASKS = 30;           ///< Maximale Anzahl Tasks
    constexpr int DEMO_TASK_COUNT = 15;     ///< Anzahl Demo-Tasks im Demo-Modus
    constexpr int MAX_TASK_TITLE_LEN = 256; ///< Maximale Titellänge (für 2-Zeilen-Umbruch)
    constexpr int MAX_TASK_NOTES_LEN = 96;  ///< Maximale Notizlänge
    constexpr int MAX_LIST_NAME_LEN = 64;   ///< Maximale Länge des Listennamens
    constexpr int BUFFER_COUNT = 2;         ///< Anzahl der Double-Buffer

    // Timing-Konstanten
    constexpr unsigned long API_CALL_INTERVAL_MS = 15UL * 60UL * 1000UL; ///< 15 Minuten
    constexpr unsigned long MIN_RETRY_INTERVAL_MS = 30000UL;             ///< 30 Sekunden
    constexpr unsigned long MAX_BACKOFF_TIME_MS = 480000UL;              ///< 8 Minuten
}

// ============================================================================
// DATENSTRUKTUREN
// ============================================================================

/**
 * @brief Struktur für einen einzelnen Task
 * @details Enthält alle relevanten Daten für die Darstellung eines Tasks
 */
struct TaskItem
{
    char id[48];                               ///< Google Tasks ID
    char listId[48];                           ///< ID der zugehörigen Taskliste
    char listName[ToDoCfg::MAX_LIST_NAME_LEN]; ///< Name der Taskliste (für Gruppierung)
    char title[ToDoCfg::MAX_TASK_TITLE_LEN];   ///< Titel des Tasks
    char notes[ToDoCfg::MAX_TASK_NOTES_LEN];   ///< Optionale Notizen
    char dueDateFormatted[12];                 ///< Formatiertes Datum "DD.MM.YYYY"
    bool isCompleted = false;                  ///< Task erledigt?
    bool isOverdue = false;                    ///< Task überfällig?
    bool isDueToday = false;                   ///< Task heute fällig?
    int daysOverdue = 0;                       ///< Tage seit Fälligkeit (wenn überfällig)
    uint8_t priority = 0;                      ///< 0=normal, 1=heute, 2=überfällig, 3=stark überfällig
};

/**
 * @brief Statistiken über die Tasks
 */
struct TaskStats
{
    int total = 0;     ///< Gesamtanzahl Tasks
    int completed = 0; ///< Anzahl erledigter Tasks
    int overdue = 0;   ///< Anzahl überfälliger Tasks
    int dueToday = 0;  ///< Anzahl heute fälliger Tasks
};

/**
 * @brief Datenstruktur für das aktuelle Datum
 */
struct ToDoTodayData
{
    int currentDay = 0;   ///< Aktueller Tag (vom Server)
    int currentMonth = 0; ///< Aktueller Monat (vom Server)
    int currentYear = 0;  ///< Aktuelles Jahr (vom Server)
};

/**
 * @brief Thread-sichere Datenstruktur für alle Task-Daten
 * @details Wird im Double-Buffer-Verfahren zwischen Worker-Task und UI-Thread verwendet.
 *          Keine dynamische Allokation zur Laufzeit - vermeidet Heap-Fragmentierung.
 */
struct ToDoData
{
    TaskItem tasks[ToDoCfg::MAX_TASKS]; ///< Array aller Tasks
    int taskCount = 0;                  ///< Anzahl der Tasks
    TaskStats stats;                    ///< Statistiken
    ToDoTodayData today;                ///< Heutige Datumsinformationen
    unsigned long timestamp = 0;        ///< Zeitstempel der Daten für Validierung

    /**
     * @brief Setzt alle Daten auf Initialwerte zurück
     */
    void reset()
    {
        taskCount = 0;
        stats = TaskStats{};
        today = ToDoTodayData{};
        timestamp = 0;
    }
};

/**
 * @brief Callbacks für das ToDo-Modul
 */
struct ToDoCallbacks
{
    void (*onExitRequested)() = nullptr; ///< Wird beim Verlassen aufgerufen
};

// ============================================================================
// HAUPTKLASSE
// ============================================================================

/**
 * @brief Thread-sicheres ToDo-Modul mit asynchronem Datenabruf
 * @details Implementiert robuste Kommunikation zwischen UI und Worker-Task
 *          zur Anzeige und Verwaltung von Google Tasks.
 *
 *          Verwendet Double-Buffering für ToDoData:
 *          - Zwei statische Buffer als Member (keine Runtime-Allokation)
 *          - Worker schreibt in inaktiven Buffer
 *          - Atomarer Index-Swap nach erfolgreichem Fetch
 *          - Vermeidet Heap-Fragmentierung bei Langzeitbetrieb
 */
class ToDo : public BaseModule
{
public:
    ToDo();
    ~ToDo();

    // Keine Kopien erlauben (wegen Task-Handle und Queue)
    ToDo(const ToDo &) = delete;
    ToDo &operator=(const ToDo &) = delete;

    /**
     * @brief Initialisiert das ToDo-Modul
     * @param tft Referenz auf das Display-Objekt
     * @param callbacks Callback-Funktionen für Events
     * @return true bei erfolgreicher Initialisierung
     * 
     * @note HTTP-Requests werden über den zentralen HttpService abgewickelt.
     *       HttpService muss vorher initialisiert sein (HTTP_SERVICE.begin()).
     */
    bool init(LGFX &tft, const ToDoCallbacks &callbacks);

    /**
     * @brief Hauptzeichenfunktion - wird zyklisch aufgerufen
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück
     * @return AppModule::TODO
     */
    AppModule type() const override;

    /**
     * @brief Triggert eine nichtblockierende Datenaktualisierung
     * @param force true = ignoriert Rate-Limiting
     */
    void requestRefresh(bool force = false);

private:
    // ========================================================================
    // INNERE STRUKTUREN
    // ========================================================================

    /**
     * @brief Scroll-Status für die Task-Liste
     */
    struct ScrollState
    {
        int scrollOffset = 0;             ///< Aktuelle Scroll-Position in Pixeln
        int contentHeight = 0;            ///< Gesamthöhe des Inhalts
        int viewportHeight = 480;         ///< Sichtbare Höhe
        int maxScroll = 0;                ///< Maximale Scroll-Position
        unsigned long lastScrollTime = 0; ///< Zeitstempel für Smooth-Scrolling
        float scrollVelocity = 0.0f;      ///< Geschwindigkeit für Momentum-Scrolling
        bool isDragging = false;          ///< Touch-Drag aktiv
        int dragStartY = 0;               ///< Start-Y beim Drag
        int dragStartOffset = 0;          ///< Scroll-Offset beim Drag-Start
        bool isSnappingBack = false;      ///< Flag für Zurückschnapp-Animation

        void reset()
        {
            scrollOffset = 0;
            scrollVelocity = 0.0f;
            isDragging = false;
            isSnappingBack = false;
        }
    };

    /**
     * @brief Toggle-Switch Status
     */
    struct ToggleState
    {
        bool enabled = true; ///< Toggle-State (true = erledigte Tasks anzeigen)
    };

    /**
     * @brief Tap-Erkennung Status
     */
    struct TapState
    {
        int32_t startX = 0;                  ///< X-Position bei Touch-Start
        int32_t startY = 0;                  ///< Y-Position bei Touch-Start
        bool isTap = false;                  ///< True wenn es ein Tap (kein Drag) ist
        bool toggleTapStarted = false;       ///< Toggle-Tap in Bearbeitung
        static constexpr int THRESHOLD = 15; ///< Max. Bewegung für Tap-Erkennung

        void reset()
        {
            startX = 0;
            startY = 0;
            isTap = false;
            toggleTapStarted = false;
        }

        bool exceedsThreshold(int32_t tx, int32_t ty) const
        {
            return (abs(tx - startX) > THRESHOLD || abs(ty - startY) > THRESHOLD);
        }
    };

    // ========================================================================
    // WORKER-TASK FUNKTIONEN
    // ========================================================================

    static void taskEntry(void *arg);
    void workerLoop();
    int fetchToDoDataCore(ToDoData *outData);
    
    void generateDemoData(ToDoData *outData);
    
    bool sendTaskUpdate(const char *taskId, const char *listId, bool completed);
    bool isValidTaskData(const char *title) const;
    void shutdownWorker(bool fast = false);

    /**
     * @brief Füllt Datumsdaten thread-sicher direkt ins Ziel
     * @param out Ziel-Struktur für das aktuelle Datum
     *
     * Statische Hilfsfunktion, die das aktuelle Datum von der RTC holt und direkt in die übergebene Struktur schreibt. Wird vom Worker verwendet, um Thread-Safety-Probleme zu vermeiden.
     */
    static void fillCurrentTime(ToDoTodayData &out);

    /**
     * @brief Aktualisiert das aktuelle Datum im aktiven Buffer
     * @note Nur im UI-Thread verwenden (Konstruktor)!
     */
    void updateCurrentTime();

    // ========================================================================
    // DOUBLE-BUFFER FUNKTIONEN
    // ========================================================================

    /**
     * @brief Gibt Zeiger auf den aktiven (UI-sichtbaren) Buffer zurück
     * @return Pointer auf den aktuell aktiven ToDoData Buffer
     */
    ToDoData *getActiveBuffer();

    /**
     * @brief Gibt Zeiger auf den inaktiven (Worker-beschreibbaren) Buffer zurück
     * @return Pointer auf den inaktiven ToDoData Buffer
     */
    ToDoData *getInactiveBuffer();

    /**
     * @brief Swappt die Buffer (macht inaktiven zum aktiven)
     * @details Atomare Operation - thread-safe zwischen Worker und UI
     */
    void swapBuffers();

    // ========================================================================
    // UI-ZEICHENFUNKTIONEN
    // ========================================================================

    /**
     * @brief Zeichnet die komplette Hauptansicht neu (nach Sprachwechsel)
     *
     * Räumt das Display, regeneriert im Demo-Modus die Demo-Daten in der jetzt aktiven Sprache, invalidiert die Sprite-Caches und zeichnet alle Sektionen über die regulären draw*-Funktionen neu. Wird ausschließlich aus draw() nach einem positiven consumeLanguageChanged() aufgerufen.
     */
    void redrawAll();

    void drawTasks();
    void drawStats();
    void drawToggleSwitch();
    void drawRefreshButton();
    void drawLastUpdateTime();
    void drawDigitalClock(int32_t x, int32_t y);
    void drawCheckbox(int x, int y, bool checked, uint16_t color);
    void drawScrollbar();
    void truncateTextToWidth(const char *text, int maxWidth, LGFX_Sprite *sprite,
                             char *output, size_t outputSize) const;

    // ========================================================================
    // TOUCH-HANDLING
    // ========================================================================

    bool isToggleTouched(int32_t tx, int32_t ty) const;
    void handleToggleTap();
    bool isRefreshButtonTouched(int32_t tx, int32_t ty) const;
    void handleRefreshButtonTap();
    void handleScrollTouch(int32_t tx, int32_t ty, bool touched);
    void updateScrollPhysics();
    void constrainScroll();
    int calculateContentHeight() const;
    int getTaskAtPosition(int32_t tx, int32_t ty) const;
    void handleCheckboxTap(int taskIndex);
    void processMessageBoxConfirmation();
    void onMessageBoxResult(bool confirmed);

    // ========================================================================
    // HILFSFUNKTIONEN
    // ========================================================================

    uint16_t getCheckboxColor(const TaskItem &task) const;
    bool isTaskVisible(const TaskItem &task) const;

    /**
     * @brief Sucht einen Task anhand seiner ID
     * @param taskId Die zu suchende Task-ID
     * @return Index im tasks-Array oder -1 wenn nicht gefunden
     */
    int findTaskIndexById(const char *taskId) const;

    // ========================================================================
    // MEMBER-VARIABLEN
    // ========================================================================

    // Core
    LGFX *m_tft = nullptr;
    ToDoCallbacks m_callbacks{};
    bool m_initialized = false;

    // Buttons
    Button m_exitButton;
    Button m_cancelLoadButton;
    Button m_refreshButton;

    // ========================================================================
    // DOUBLE-BUFFER FÜR TASK-DATEN
    // ========================================================================
    // Zwei statische Buffer vermeiden Runtime-Allokation und Heap-Fragmentierung. Worker schreibt in inaktiven Buffer, dann wird Index atomar getauscht.
    // ========================================================================

    ToDoData m_dataBuffers[ToDoCfg::BUFFER_COUNT]; ///< Double-Buffer Array
    std::atomic<int> m_activeBufferIndex{0};       ///< Index des aktiven Buffers (0 oder 1)
    std::atomic<bool> m_dataReady{false};          ///< Signal: Neue Daten verfügbar
    bool m_hasValidData = false;                   ///< UI hat gültige Daten empfangen

    // UI-State
    ScrollState m_scrollState;
    ToggleState m_toggleState;
    TapState m_tapState;

    int32_t m_lastTouchY = -1;
    unsigned long m_lastTouchTime = 0;

    // Threading und Synchronisation
    std::atomic<TaskHandle_t> m_taskHandle{nullptr}; ///< Handle des Worker-Tasks (thread-safe)
    std::atomic<bool> m_shutdownRequested{false};
    std::atomic<bool> m_refreshRequested{false};
    std::atomic<bool> m_inFlight{false};
    std::atomic<bool> m_fetchCompleted{false};

    // Timing und Rate-Limiting
    unsigned long m_lastApiCall = 0;
    unsigned long m_backoffMs = 0;
    std::atomic<unsigned long> m_nextAllowedFetch{0}; ///< Frühester erlaubter nächster Fetch (thread-safe)
    int m_consecutiveErrors = 0;

    // Zeitstempel der letzten Aktualisierung
    char m_lastUpdateTimestamp[20] = "";

    // Sprites
    Gfx_Sprite *m_taskSprite = nullptr;
    Gfx_Sprite *m_scrollbarSprite = nullptr;
    Gfx_Sprite *m_clockSprite = nullptr;
    Gfx_Sprite *m_progressSprite = nullptr;

    bool m_taskSpriteValid = false;
    int m_lastScrollOffset = -1;
    int m_lastDrawnSecond = -1;

    // UI-State
    bool m_layoutDrawn = false;
    unsigned long m_lastProgressUpdate = 0;
    unsigned long m_workerLastSeen = 0;

    // ========================================================================
    // ID-BASIERTE TASK-AUSWAHL (für Refresh-Stabilität)
    // ========================================================================
    char m_messageBoxTaskId[48] = "";  ///< ID des Tasks für MessageBox (statt Index)
    bool m_messageBoxNewStatus = false;

    // Dialoge
    std::unique_ptr<MessageBox> m_messageBox;
    std::unique_ptr<ProgressBox> m_progressBox;

    // API-Endpoint
    const char *m_appsScriptUrl = nullptr; ///< URL zum Google Apps Script (aus AppState)
    bool m_demoMode = false;               ///< Runtime Demo-Modus (aktiv wenn keine gültige URL)
};

#endif // TODO_H
