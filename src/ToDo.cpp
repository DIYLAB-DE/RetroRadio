#include "ToDo.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <cstdio>
#include <ctime>

#include "AppState.h"
#include "HttpService.h"
#include "I18n.h"
#include "ProgressBox.h"
#include "StringUtils.h"
#include "Strings_todo.h"
#include "Theme.h"

// Fonts
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// KONSTANTEN
// ===================================================================

namespace
{

    // Fehler-Codes
    constexpr int ERR_SHUTDOWN = -100;       ///< Shutdown angefordert
    constexpr int ERR_JSON_PARSE = -101;     ///< JSON-Parsing fehlgeschlagen
    constexpr int ERR_EMPTY_RESPONSE = -102; ///< Leere Server-Antwort
    constexpr int ERR_HTTP_NOT_INIT = -103;  ///< HttpService nicht initialisiert
    constexpr int ERR_SCHEMA = -104;         ///< Erforderliche Datenstruktur fehlt

    // Task Konfiguration
    constexpr size_t WORKER_STACK_SIZE = 12288; ///< Stack-Größe des Worker-Tasks
    constexpr UBaseType_t WORKER_PRIORITY = 1;  ///< Priorität des Worker-Tasks
    constexpr BaseType_t WORKER_CORE = 1;       ///< Core auf dem der Worker läuft

    // Watchdog Timeouts
    constexpr unsigned long WATCHDOG_IDLE_MS = 90000UL; ///< Idle Timeout
    constexpr unsigned long WATCHDOG_BUSY_MS = 45000UL; ///< Busy Timeout


    // Layout-Konstanten
    constexpr int TASK_HEIGHT = 48;          ///< Höhe eines Task-Eintrags in Pixeln
    constexpr int TASK_LINE_HEIGHT = 20;     ///< Zeilenhöhe für Task-Titel
    constexpr int LIST_HEADER_HEIGHT = 37;   ///< Höhe der Listentrennzeile
    constexpr int LIST_SEPARATOR_HEIGHT = 6; ///< Höhe des Listentrenners
    constexpr int TASK_INDENT = 10;          ///< Einrückung des Task-Inhalts
    constexpr int CHECKBOX_SIZE = 37;        ///< Größe der Checkbox
    constexpr int PRIORITY_RECT_WIDTH = 4;   ///< Breite des Prioritätsrechtecks
    constexpr int PRIORITY_RECT_MARGIN = 10; ///< Abstand des Prioritätsrechtecks
    constexpr int DATE_COLUMN_X = 490;       ///< X-Position der Datumsspalte
    constexpr int LIST_WIDTH = 585;          ///< Breite der Task-Liste
    constexpr int SCROLLBAR_X = 590;         ///< X-Position der Scrollbar
    constexpr int STATS_PANEL_X = 630;       ///< X-Position des Statistik-Panels
    constexpr int STATS_PANEL_WIDTH = 133;   ///< Breite des Statistik-Panels
    constexpr int VIEWPORT_HEIGHT = 480;     ///< Höhe des sichtbaren Bereichs

    // Exit-Button Layout
    constexpr int16_t EXIT_BUTTON_X = 765; ///< X-Position des Exit-Buttons
    constexpr int16_t EXIT_BUTTON_Y = 457; ///< Y-Position des Exit-Buttons
    constexpr uint16_t BUTTON_SIZE = 45;   ///< Größe des quadratischen Buttons
    constexpr uint16_t BUTTON_RADIUS = 4;  ///< Eckradius der Buttons

    // Toggle-Switch Layout
    constexpr int TOGGLE_X = STATS_PANEL_X;          ///< X-Position des Toggle-Switch
    constexpr int TOGGLE_Y = 230;                    ///< Y-Position des Toggle-Switch
    constexpr int TOGGLE_WIDTH = 55;                 ///< Breite des Toggle-Switch
    constexpr int TOGGLE_HEIGHT = 29;                ///< Höhe des Toggle-Switch
    constexpr int TOGGLE_RADIUS = TOGGLE_HEIGHT / 2; ///< Radius des Toggle-Switch
    constexpr int KNOB_SIZE = 21;                    ///< Größe des Toggle-Knobs
    constexpr int KNOB_MARGIN = 4;                   ///< Abstand des Knobs zum Gehäuserand
    constexpr int TOGGLE_TOUCH_MARGIN = 5;           ///< Zusätzlicher Touch-Bereich

    // Refresh-Button Layout
    constexpr int16_t REFRESH_BTN_X = STATS_PANEL_X + 65; ///< X-Position des Refresh-Buttons
    constexpr int16_t REFRESH_BTN_Y = 306;                ///< Y-Position des Refresh-Buttons
    constexpr uint16_t REFRESH_BTN_WIDTH = 130;           ///< Breite des Buttons
    constexpr uint16_t REFRESH_BTN_HEIGHT = 45;           ///< Höhe des Buttons

    // Timestamp Layout
    constexpr int TIMESTAMP_Y = 350; ///< Y-Position des Zeitstempels

    // Badge Layout für Stats
    constexpr int BADGE_PADDING_H = 10;         ///< Horizontales Padding im Badge
    constexpr int BADGE_PADDING_V = 4;          ///< Vertikales Padding im Badge
    constexpr int BADGE_RADIUS = 8;             ///< Eckenradius der Badges
    constexpr int BADGE_MIN_WIDTH = 36;         ///< Minimale Badge-Breite
    constexpr uint16_t COLOR_BADGE_BG = 0x2104; ///< Badge Hintergrundfarbe

    // Farben
    constexpr uint16_t COLOR_BUTTON = 0x39E8;                            ///< Farbe Button Hintergrund
    constexpr uint16_t COLOR_BUTTON_TEXT = TFT_BLACK;                    ///< Farbe Button Text
    constexpr uint16_t COLOR_BUTTON_BORDER = TFT_DARKGREY;               ///< Farbe Button Umrandung
    constexpr uint16_t COLOR_PROGRESS_BAR = 0x8010;                      ///< Farbe Fortschrittsbalken
    constexpr uint16_t COLOR_LOADING_TEXT = 0xD69A;                      ///< Farbe Lade-Text
    constexpr uint16_t COLOR_OVERDUE = Theme::ColorRetroRed;             ///< Farbe überfällige Tasks
    constexpr uint16_t COLOR_OVERDUE_STRONG = TFT_RED;                   ///< Farbe stark überfällige Tasks
    constexpr uint16_t COLOR_DUE_TODAY = 0xFD20;                         ///< Farbe heute fällige Tasks
    constexpr uint16_t COLOR_NORMAL = 0xD69A;                            ///< Farbe normale Tasks
    constexpr uint16_t COLOR_COMPLETED = TFT_DARKGREY;                   ///< Farbe erledigte Tasks
    constexpr uint16_t COLOR_CHECKBOX_DONE = Theme::ColorGreen;          ///< Farbe Checkbox erledigt
    constexpr uint16_t COLOR_CHECKBOX_BORDER = Theme::ColorButtonBorder; ///< Farbe Checkbox Rand
    constexpr uint16_t COLOR_CHECKBOX_PENDING = 0xFD20;                  ///< Farbe Checkbox offen
    constexpr uint16_t COLOR_LABEL = 0x9CF3;                             ///< Farbe Labels
    constexpr uint16_t COLOR_SEPARATOR = 0x4208;                         ///< Farbe Listentrenner
    constexpr uint16_t COLOR_SCROLLBAR_TRACK = 0x2104;                   ///< Farbe Scrollbar Hintergrund
    constexpr uint16_t COLOR_SCROLLBAR_ACTIVE = 0x8410;                  ///< Farbe Scrollbar aktiv
    constexpr uint16_t COLOR_SCROLLBAR_ANIM = 0x6B6D;                    ///< Farbe Scrollbar Animation
    constexpr uint16_t COLOR_SCROLLBAR_NORMAL = 0x5ACB;                  ///< Farbe Scrollbar normal
    constexpr uint16_t COLOR_TIMESTAMP = 0x7BEF;                         ///< Farbe Zeitstempel

    // Maximale Fehler bevor längere Pause einsetzt
    constexpr int MAX_CONSECUTIVE_ERRORS = 5;

    // Buffer-Größe für Zeilenumbruch
    constexpr size_t LINE_BUFFER_SIZE = 128;
}

// ===================================================================
// KONSTRUKTOR / DESTRUKTOR
// ===================================================================

/**
 * @brief Konstruktor
 */
ToDo::ToDo()
{
    // Double-Buffer für Daten initialisieren
    m_dataBuffers[0].reset();
    m_dataBuffers[1].reset();
    m_activeBufferIndex.store(0);
    m_dataReady.store(false);

    updateCurrentTime();
}

/**
 * @brief Destruktor
 */
ToDo::~ToDo()
{
    // Worker-Task beenden
    shutdownWorker();

    // Sprites nur löschen wenn m_tft gültig ist
    if (m_tft)
    {
        // Font entladen bevor Sprite gelöscht wird
        if (m_taskSprite)
            m_taskSprite->unloadFont();

        m_tft->safeDeleteSprite(m_taskSprite);
        m_tft->safeDeleteSprite(m_progressSprite);
        m_tft->safeDeleteSprite(m_scrollbarSprite);
        m_tft->safeDeleteSprite(m_clockSprite);
    }
}

/**
 * @brief Beendet den Worker-Task sauber
 */
void ToDo::shutdownWorker(bool fast)
{
    // Handle lokal speichern - Thread-safe lesen
    TaskHandle_t handle = m_taskHandle.load();
    if (!handle)
        return;

    m_shutdownRequested.store(true);

    // KRITISCH: Bei aktivem HTTP-Request MÜSSEN wir warten! vTaskDelete während HTTP = Memory Leak (WiFiClientSecure ~16KB)
    if (m_inFlight.load())
    {
        LOG_WARNING("Shutdown während HTTP-Request - warte auf Abschluss...");

        int waited = 0;

        while (m_inFlight.load() && waited < HttpServiceCfg::HTTP_TIMEOUT_MS)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            waited += 100;
        }

        if (m_inFlight.load())
            LOG_ERROR("HTTP-Timeout überschritten - Force-Shutdown (mögliches Memory Leak!)");
    }

    // Handle nochmal lesen - könnte sich während Warten geändert haben
    handle = m_taskHandle.load();
    if (handle)
        xTaskNotifyGive(handle);

    // Graceful warten
    const int maxWaitMs = fast ? 1000 : 5000;
    int waited = 0;

    while (m_taskHandle.load() != nullptr && waited < maxWaitMs)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        waited += 100;
    }

    // Handle nochmal lesen vor potentiellem Delete
    handle = m_taskHandle.load();
    if (handle)
    {
        LOG_WARNING("Worker reagiert nicht - Force Delete");
        vTaskDelete(handle);
        m_taskHandle.store(nullptr);
    }
    else
    {
        LOG_SUCCESS("Worker-Task graceful beendet");
    }
}

/**
 * @brief Gibt den Modultyp zurück
 */
AppModule ToDo::type() const
{
    return AppModule::TODO;
}

// ===================================================================
// DOUBLE-BUFFER FUNKTIONEN
// ===================================================================

/**
 * @brief Gibt Zeiger auf den aktiven Buffer zurück
 */
ToDoData *ToDo::getActiveBuffer()
{
    return &m_dataBuffers[m_activeBufferIndex.load()];
}

/**
 * @brief Gibt Zeiger auf den inaktiven Buffer zurück
 */
ToDoData *ToDo::getInactiveBuffer()
{
    // Inaktiver Buffer ist der jeweils andere (0->1, 1->0)
    return &m_dataBuffers[1 - m_activeBufferIndex.load()];
}

/**
 * @brief Swappt die Buffer atomar
 */
void ToDo::swapBuffers()
{
    // Atomarer Wechsel: 0->1 oder 1->0
    const int current = m_activeBufferIndex.load();
    m_activeBufferIndex.store(1 - current);
    LOG_INFO_F("Buffer-Swap: %d -> %d", current, 1 - current);
}

// ===================================================================
// ZEIT-FUNKTIONEN (Thread-Safe)
// ===================================================================

/**
 * @brief Füllt Datumsdaten thread-sicher direkt ins Ziel
 */
void ToDo::fillCurrentTime(ToDoTodayData &out)
{
    struct tm timeinfo = DS1307_RTC::getCurrentLocalTime();
    out.currentYear = timeinfo.tm_year + 1900;
    out.currentMonth = timeinfo.tm_mon + 1;
    out.currentDay = timeinfo.tm_mday;
}

/**
 * @brief Aktualisiert das aktuelle Datum im aktiven Buffer
 * @note Nur im UI-Thread/Konstruktor verwenden!
 */
void ToDo::updateCurrentTime()
{
    ToDoData *data = getActiveBuffer();
    if (!data)
        return;

    fillCurrentTime(data->today);
}

// ===================================================================
// INITIALISIERUNG
// ===================================================================

/**
 * @brief Initialisiert das ToDo-Modul
 *
 * GEÄNDERT: WifiManager-Parameter entfernt, verwendet jetzt HTTP_SERVICE
 */
bool ToDo::init(LGFX &tft, const ToDoCallbacks &callbacks)
{
    // Bei Re-Initialisierung -> alten Worker beenden
    if (m_taskHandle.load())
    {
        LOG_WARNING("Re-Initialisierung: Beende alten Worker-Task");
        shutdownWorker(true);
    }

    m_tft = &tft;
    m_callbacks = callbacks;
    m_initialized = false;

    // Google Apps Script URL aus AppState laden
    m_appsScriptUrl = APP_STATE.getTodoScriptUrl();

    // URL validieren: muss vorhanden sein und mit "/exec" enden
    const size_t urlLen = m_appsScriptUrl ? strlen(m_appsScriptUrl) : 0;
    m_demoMode = (urlLen < 5 || strcmp(m_appsScriptUrl + urlLen - 5, "/exec") != 0);

    if (m_demoMode && urlLen > 0)
        LOG_WARNING("ToDo: URL endet nicht mit /exec -> Demo-Modus");
    else if (m_demoMode)
        LOG_INFO("ToDo: Keine API-URL konfiguriert -> Demo-Modus aktiv");
    else
        LOG_INFO_F("ToDo: API-URL geladen (%d Zeichen)", strlen(m_appsScriptUrl));

    // Kompletten State zurücksetzen
    m_shutdownRequested.store(false);
    m_refreshRequested.store(false);
    m_inFlight.store(false);
    m_fetchCompleted.store(false);
    m_activeBufferIndex.store(0);
    m_dataReady.store(false);

    m_hasValidData = false;
    m_scrollState.reset();
    m_tapState.reset();

    m_lastApiCall = 0;
    m_backoffMs = 0;
    m_nextAllowedFetch.store(0);
    m_consecutiveErrors = 0;

    m_layoutDrawn = false;
    m_taskSpriteValid = false;
    m_lastScrollOffset = -1;
    m_lastDrawnSecond = -1;
    m_lastUpdateTimestamp[0] = '\0';

    m_messageBoxTaskId[0] = '\0';
    m_messageBoxNewStatus = false;

    m_dataBuffers[0].reset();
    m_dataBuffers[1].reset();

    // Sprites erstellen
    if (!m_tft->safeCreateSprite(m_progressSprite, 1, m_tft->height()) ||
        !m_tft->safeCreateSprite(m_taskSprite, LIST_WIDTH, VIEWPORT_HEIGHT) ||
        !m_tft->safeCreateSprite(m_scrollbarSprite, 5, VIEWPORT_HEIGHT) ||
        !m_tft->safeCreateSprite(m_clockSprite, 103, 45))
    {
        LOG_ERROR("Konnte Sprites nicht erstellen");
        return false;
    }

    // Font für TaskSprite einmalig laden (wird im Destruktor entladen)
    m_taskSprite->loadFont(ComfortaaBold16);

    m_clockSprite->clear(Theme::ColorMainBackGround);
    m_lastDrawnSecond = -1;

    // Dialoge erstellen
    m_messageBox = std::make_unique<MessageBox>(m_tft);
    m_progressBox = std::make_unique<ProgressBox>(*m_tft);
    m_progressBox->setAutoCenter(true);
    m_progressBox->setTitle(reinterpret_cast<const char *>(STR(ToDoKey::TODO_PROGRESS_TITLE)));
    m_progressBox->setMessage(reinterpret_cast<const char *>(STR(ToDoKey::TODO_PROGRESS_MESSAGE)));

    // Exit-Button initialisieren
    m_exitButton.initButton(
        m_tft,               // TFT-Zeichenobjekt
        EXIT_BUTTON_X,       // X-Position
        EXIT_BUTTON_Y,       // Y-Position
        BUTTON_SIZE,         // Breite
        BUTTON_SIZE,         // Höhe
        BUTTON_RADIUS,       // Eckradius
        COLOR_BUTTON,        // Hintergrundfarbe
        COLOR_BUTTON_TEXT,   // Textfarbe
        COLOR_BUTTON_BORDER, // Rahmenfarbe
        "^POWER:32:D206^",   // Beschriftung
        nullptr              // Icon
    );

    m_exitButton.setFont(LeagueGothicCond34);

    // Ladescreen anzeigen
    m_tft->clear(Theme::ColorMainBackGround);
    m_tft->loadFont(LeagueGothicCond34);
    m_tft->setTextColor(COLOR_LOADING_TEXT, Theme::ColorMainBackGround);
    m_tft->setTextDatum(MC_DATUM);
    m_tft->drawString(reinterpret_cast<const char *>(STR(ToDoKey::TODO_LOADING)),
                      m_tft->width() / 2, (m_tft->height() / 2) - 25);
    m_tft->unloadFont();

    // Cancel-Button
    m_cancelLoadButton.initButton(
        m_tft,                    // TFT-Zeichenobjekt
        m_tft->width() / 2,       // X-Position
        m_tft->height() / 2 + 25, // Y-Position
        BUTTON_SIZE,              // Breite
        BUTTON_SIZE,              // Höhe
        BUTTON_RADIUS,            // Eckradius
        COLOR_BUTTON,             // Hintergrundfarbe
        COLOR_BUTTON_TEXT,        // Textfarbe
        COLOR_BUTTON_BORDER,      // Rahmenfarbe
        "^DEL:32:F800^",          // Beschriftung
        nullptr                   // Icon
    );

    m_cancelLoadButton.drawButton(false);

    // Refresh-Button
    m_refreshButton.initButton(
        m_tft,               // TFT-Zeichenobjekt
        REFRESH_BTN_X,       // X-Position
        REFRESH_BTN_Y,       // Y-Position
        REFRESH_BTN_WIDTH,   // Breite
        REFRESH_BTN_HEIGHT,  // Höhe
        BUTTON_RADIUS,       // Eckradius
        COLOR_BUTTON,        // Hintergrundfarbe
        COLOR_BUTTON_TEXT,   // Textfarbe
        COLOR_BUTTON_BORDER, // Rahmenfarbe
        reinterpret_cast<const char *>(STR(ToDoKey::TODO_BUTTON_REFRESH)), // Beschriftung
        nullptr              // Icon
    );

    m_refreshButton.setFont(LeagueGothicCond34);

    // Toggle-Status aus NVRAM laden
    m_toggleState.enabled = APP_STATE.getToDoShowCompleted();
    LOG_INFO_F("ToDo Toggle-Status aus NVRAM geladen: %s", m_toggleState.enabled ? "erledigte anzeigen" : "erledigte ausblenden");

    m_layoutDrawn = false;

    // Worker-Task starten
    TaskHandle_t tempHandle = nullptr;
    BaseType_t result = xTaskCreatePinnedToCore(
        &ToDo::taskEntry,  // Task-Funktion
        "ToDoWorker",      // Task-Name
        WORKER_STACK_SIZE, // Stack-Größe
        this,              // Task-Parameter
        WORKER_PRIORITY,   // Task-Priorität
        &tempHandle,       // Task-Handle
        WORKER_CORE);      // Core-ID (Core 1!)

    if (result != pdPASS)
    {
        LOG_ERROR("Konnte ToDo Worker-Task nicht starten");
        return false;
    }

    m_taskHandle.store(tempHandle);
    m_initialized = true;

    LOG_SUCCESS("ToDo-Modul erfolgreich initialisiert.");
    return true;
}

// ===================================================================
// HAUPTZEICHENFUNKTION
// ===================================================================

/**
 * @brief Hauptzeichenfunktion - verarbeitet UI und Touch-Events
 */
void ToDo::draw()
{
    if (!m_initialized || !m_tft)
        return;

    // Sprachwechsel? -> kompletten Bildschirm neu zeichnen
    if (I18n::consumeLanguageChanged())
        redrawAll();

    // MessageBox: Warte auf Completion -> HTTP-Update durchführen
    if (m_messageBox && m_messageBox->isBusy())
    {
        processMessageBoxConfirmation();
        return;
    }

    // MessageBox hat Priorität
    if (m_messageBox && m_messageBox->isActive())
    {
        m_messageBox->update();
        return;
    }

    // ProgressBox Animation
    if (m_progressBox && m_progressBox->isActive())
    {
        m_progressBox->updateAnimation();

        // Fetch fehlgeschlagen -> schließen
        if (m_fetchCompleted.exchange(false))
        {
            m_progressBox->finish();
            LOG_WARNING("Refresh fehlgeschlagen - ProgressBox geschlossen");
        }
    }

    // Touch-Handling
    int32_t tx, ty;
    const bool isTouched = m_tft->getTouch(&tx, &ty);

    if (!m_layoutDrawn)
    {
        // Ladescreen: Cancel-Button
        if (m_cancelLoadButton.isPressed() && !isTouched)
        {
            m_cancelLoadButton.press(false);

            // Benutzer über Abbruch informieren
            const int textY = (m_tft->height() / 2) - 25;
            m_tft->fillRect(0, textY - 20, m_tft->width(), 40, Theme::ColorMainBackGround);
            m_tft->loadFont(LeagueGothicCond34);
            m_tft->setTextColor(COLOR_LOADING_TEXT, Theme::ColorMainBackGround);
            m_tft->setTextDatum(MC_DATUM);
            m_tft->drawString(reinterpret_cast<const char *>(STR(ToDoKey::TODO_LOADING_CANCEL)),
                              m_tft->width() / 2, textY);
            m_tft->unloadFont();

            shutdownWorker(true);

            if (m_callbacks.onExitRequested)
                m_callbacks.onExitRequested();

            return;
        }

        if (isTouched && m_cancelLoadButton.contains(tx, ty))
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
    else
    {
        // Hauptansicht
        bool touchOnExitButton = false;

        // Exit-Button
        if (m_exitButton.isPressed() && !isTouched)
        {
            m_exitButton.press(false);
            shutdownWorker(true);

            if (m_callbacks.onExitRequested)
                m_callbacks.onExitRequested();

            return;
        }

        if (isTouched && m_exitButton.contains(tx, ty))
        {
            touchOnExitButton = true;
            if (!m_exitButton.isPressed())
            {
                m_exitButton.press(true);
                m_exitButton.drawButton(true);
            }
        }
        else if (m_exitButton.isPressed())
        {
            m_exitButton.press(false);
            m_exitButton.drawButton(false);
        }

        // Toggle-Switch
        if (isTouched && isToggleTouched(tx, ty))
        {
            if (m_tapState.startX == 0 && m_tapState.startY == 0)
            {
                m_tapState.startX = tx;
                m_tapState.startY = ty;
                m_tapState.isTap = true;
                m_tapState.toggleTapStarted = true;
            }

            if (m_tapState.exceedsThreshold(tx, ty))
            {
                m_tapState.isTap = false;
                m_tapState.toggleTapStarted = false;
            }
        }
        else if (!isTouched && m_tapState.toggleTapStarted && m_tapState.isTap && m_tapState.startX > 0)
        {
            if (isToggleTouched(m_tapState.startX, m_tapState.startY))
            {
                handleToggleTap();
                drawStats();
            }

            m_tapState.reset();
        }
        else if (!isTouched)
        {
            m_tapState.toggleTapStarted = false;
        }

        // Refresh-Button
        if (m_refreshButton.isPressed() && !isTouched)
        {
            m_refreshButton.press(false);
            handleRefreshButtonTap();
            m_refreshButton.drawButton(false);
        }

        if (isTouched && m_refreshButton.contains(tx, ty))
        {
            if (!m_refreshButton.isPressed())
            {
                m_refreshButton.press(true);
                m_refreshButton.drawButton(true);
            }
        }
        else if (m_refreshButton.isPressed())
        {
            m_refreshButton.press(false);
            m_refreshButton.drawButton(false);
        }

        // Scroll-Logik
        const bool touchOnRefresh = m_refreshButton.contains(tx, ty);
        if (isTouched && !touchOnExitButton && !m_tapState.toggleTapStarted && !touchOnRefresh)
        {
            if (m_tapState.startX == 0 && m_tapState.startY == 0)
            {
                m_tapState.startX = tx;
                m_tapState.startY = ty;
                m_tapState.isTap = true;
            }

            handleScrollTouch(tx, ty, true);

            if (m_tapState.exceedsThreshold(tx, ty))
                m_tapState.isTap = false;
        }
        else if (!isTouched && m_scrollState.isDragging)
        {
            handleScrollTouch(0, 0, false);

            // Tap-Erkennung für Checkbox
            if (m_tapState.isTap && m_tapState.startX > 0)
            {
                int taskIdx = getTaskAtPosition(m_tapState.startX, m_tapState.startY);
                if (taskIdx >= 0)
                {
                    LOG_INFO_F("Tap erkannt bei (%ld, %ld) -> Task %d", m_tapState.startX, m_tapState.startY, taskIdx);
                    handleCheckboxTap(taskIdx);
                }
            }
            m_tapState.reset();
        }
        else if (!isTouched)
        {
            if (m_tapState.isTap && m_tapState.startX > 0)
            {
                int taskIdx = getTaskAtPosition(m_tapState.startX, m_tapState.startY);
                if (taskIdx >= 0)
                {
                    LOG_INFO_F("Tap erkannt bei (%ld, %ld) -> Task %d",
                               m_tapState.startX, m_tapState.startY, taskIdx);
                    handleCheckboxTap(taskIdx);
                }
            }
            m_tapState.reset();
        }
    }

    // Scrollphysik aktualisieren
    updateScrollPhysics();

    // Neue Daten vom Worker empfangen
    if (m_dataReady.exchange(false))
    {
        // ProgressBox schließen
        if (m_progressBox && m_progressBox->isActive())
            m_progressBox->finish();

        // Buffer wurde bereits im Worker geswappt - jetzt ist neuer Buffer aktiv
        m_hasValidData = true;
        m_lastApiCall = millis();

        // Zeitstempel aus Systemzeit formatieren. Bei ungültiger Zeit bleibt das Feld leer, sodass drawLastUpdateTime() nichts zeichnet.
        if (DS1307_RTC::isSystemTimeValid())
        {
            time_t nowEpoch = time(nullptr);
            struct tm nowLocal;
            localtime_r(&nowEpoch, &nowLocal);
            stringutils::safeFormat(m_lastUpdateTimestamp, "%02d.%02d.%04d %02d:%02d:%02d",
                                    nowLocal.tm_mday, nowLocal.tm_mon + 1, nowLocal.tm_year + 1900,
                                    nowLocal.tm_hour, nowLocal.tm_min, nowLocal.tm_sec);
        }
        else
        {
            m_lastUpdateTimestamp[0] = '\0';
        }

        // Scroll-State zurücksetzen
        m_scrollState.reset();
        m_scrollState.contentHeight = calculateContentHeight();
        m_scrollState.viewportHeight = VIEWPORT_HEIGHT;
        m_scrollState.maxScroll = std::max(0, m_scrollState.contentHeight - VIEWPORT_HEIGHT);

        m_taskSpriteValid = false;
        m_lastScrollOffset = -1;

        // UI komplett neu zeichnen
        m_tft->clear(Theme::ColorMainBackGround);

        if (!m_layoutDrawn)
            m_layoutDrawn = true;

        drawStats();
        drawRefreshButton();
        drawLastUpdateTime();
        drawTasks();
        m_exitButton.drawButton(false);
        drawDigitalClock(625, (m_tft->height() - m_clockSprite->height()) + 1);

        LOG_INFO("UI mit neuen Task-Daten aktualisiert (Double-Buffer)");
    }

    // nur scrollbaren Bereich neu zeichnen
    if (m_hasValidData && (m_scrollState.isDragging || std::abs(m_scrollState.scrollVelocity) > 0.01f || m_scrollState.isSnappingBack))
        drawTasks();

    // periodische Aktualisierung
    const bool intervalDue = (m_lastApiCall == 0) || (millis() - m_lastApiCall > ToDoCfg::API_CALL_INTERVAL_MS);
    const bool canTrigger = !m_refreshRequested.load() && !m_inFlight.load();

    if (intervalDue && canTrigger)
        requestRefresh(false);

    // Worker-Task Watchdog
    if (!m_taskHandle.load() && !m_shutdownRequested.load())
    {
        LOG_WARNING("Worker-Task nicht aktiv - starte neu");
        m_consecutiveErrors = 0;
        m_backoffMs = 0;

        TaskHandle_t tempHandle = nullptr;
        BaseType_t result = xTaskCreatePinnedToCore(&ToDo::taskEntry, "ToDoWorker", WORKER_STACK_SIZE, this, WORKER_PRIORITY, &tempHandle, WORKER_CORE);

        if (result == pdPASS)
        {
            m_taskHandle.store(tempHandle);
            LOG_SUCCESS("Worker-Task erfolgreich neu gestartet");
            m_refreshRequested.store(true);
            xTaskNotifyGive(tempHandle);
        }
        else
        {
            LOG_ERROR("Konnte Worker-Task nicht neu starten!");
        }
    }

    // zustandsabhängiger Watchdog
    if (m_taskHandle.load() && m_workerLastSeen > 0)
    {
        const unsigned long now = millis();
        const unsigned long idle = now - m_workerLastSeen;
        const bool busy = m_inFlight.load();
        const unsigned long threshold = busy ? WATCHDOG_BUSY_MS : WATCHDOG_IDLE_MS;

        if (idle > threshold)
        {
            LOG_ERROR_F("Worker-Task Watchdog: idle=%lu ms (busy=%d) -> Neustart", idle, busy ? 1 : 0);
            TaskHandle_t handle = m_taskHandle.load();
            if (handle)
            {
                vTaskDelete(handle);
                m_taskHandle.store(nullptr);
            }
            m_workerLastSeen = 0;
            m_inFlight.store(false);
            m_refreshRequested.store(false);
            m_shutdownRequested.store(false);
        }
    }

    // Progressbar und Digitaluhr sekündlich aktualisieren
    if (m_hasValidData && millis() - m_lastProgressUpdate > 1000)
    {
        m_lastProgressUpdate = millis();

        if (m_progressSprite)
        {
            const unsigned long elapsedTime = millis() - m_lastApiCall;
            float progress = 1.0f - (static_cast<float>(elapsedTime) / static_cast<float>(ToDoCfg::API_CALL_INTERVAL_MS));
            progress = constrain(progress, 0.0f, 1.0f);

            const int barHeight = static_cast<int>(m_tft->height() * progress);
            const int startY = m_tft->height() - barHeight;

            m_progressSprite->drawFastVLine(0, 0, startY, Theme::ColorMainBackGround);
            m_progressSprite->drawFastVLine(0, startY, barHeight, COLOR_PROGRESS_BAR);
            m_progressSprite->pushSprite(m_tft->width() - 1, 0);
        }

        drawDigitalClock(625, (m_tft->height() - m_clockSprite->height()) + 1);
    }
}

/**
 * @brief Triggert Datenaktualisierung
 */
void ToDo::requestRefresh(bool force)
{
    if (!m_initialized || m_shutdownRequested.load())
        return;

    if (!force && millis() < m_nextAllowedFetch.load())
    {
        // Log-Spam vermeiden: Nur alle 5 Sekunden loggen
        static unsigned long lastBackoffLog = 0;
        if (millis() - lastBackoffLog > 5000)
        {
            LOG_INFO("Request refresh ignoriert - Backoff aktiv");
            lastBackoffLog = millis();
        }
        return;
    }

    m_refreshRequested.store(true);

    TaskHandle_t handle = m_taskHandle.load();
    if (handle)
        xTaskNotifyGive(handle);

    LOG_INFO("Refresh-Request gesendet");
}

// ===================================================================
// WORKER-TASK
// ===================================================================

/**
 * @brief Eintrittsvektor für den Worker-Task
 */
void ToDo::taskEntry(void *arg)
{
    auto *self = static_cast<ToDo *>(arg);
    if (self)
        self->workerLoop();
}

/**
 * @brief Haupt-Arbeitsschleife
 */
void ToDo::workerLoop()
{
    LOG_INFO("ToDo Worker-Task gestartet (Double-Buffer Modus)");

    constexpr TickType_t WAIT_TICKS = pdMS_TO_TICKS(30000);

    while (!m_shutdownRequested.load())
    {
        ulTaskNotifyTake(pdTRUE, WAIT_TICKS);
        m_workerLastSeen = millis();

        if (m_shutdownRequested.load())
        {
            LOG_INFO("Shutdown-Signal empfangen - beende Worker graceful");
            break;
        }

        if (!m_refreshRequested.exchange(false))
            continue;

        m_inFlight.store(true);
        LOG_INFO("Worker: Starte Datenabruf (Double-Buffer)");

        // In den inaktiven Buffer schreiben (keine Allokation!)
        ToDoData *fetchBuffer = getInactiveBuffer();
        fetchBuffer->reset();

        const int httpCode = fetchToDoDataCore(fetchBuffer);

        if (m_shutdownRequested.load())
        {
            m_inFlight.store(false);
            break;
        }

        if (httpCode == HTTP_CODE_OK)
        {
            // Atomarer Buffer-Swap - macht fetchBuffer zum neuen aktiven Buffer
            swapBuffers();

            // Signal an UI: Neue Daten verfügbar
            m_dataReady.store(true);

            LOG_INFO("Double-Buffer Swap erfolgreich - neue Daten verfügbar");

            m_consecutiveErrors = 0;
            m_backoffMs = 0;
            m_nextAllowedFetch.store(millis());
        }
        else
        {
            // Bei Fehler: Buffer nicht swappen, Daten verwerfen
            m_fetchCompleted.store(true); // Signal für UI: Fehler

            // HTTP 4xx = Client-Fehler (falscher Key, URL ungültig) -> sofort Demo-Modus Andere Fehler (Timeout, 5xx, kein WiFi) -> Retry mit Backoff
            const bool clientError = (httpCode >= 400 && httpCode < 500);

            if (clientError)
            {
                LOG_WARNING_F("HTTP %d (Client-Fehler) - API-URL ungültig, wechsle in Demo-Modus", httpCode);
                m_demoMode = true;
            }
            else
            {
                m_consecutiveErrors++;
                m_backoffMs = (m_backoffMs == 0) ? ToDoCfg::MIN_RETRY_INTERVAL_MS
                                                 : std::min(m_backoffMs * 2, ToDoCfg::MAX_BACKOFF_TIME_MS);
                m_nextAllowedFetch.store(millis() + m_backoffMs);

                LOG_WARNING_F("Worker: Fehler %d/%d, Backoff: %lu ms (HTTP: %d)", m_consecutiveErrors, MAX_CONSECUTIVE_ERRORS, m_backoffMs, httpCode);

                if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
                {
                    LOG_WARNING("Zu viele transiente Fehler - wechsle in Demo-Modus");
                    m_demoMode = true;
                }
            }

            // Demo-Modus aktiviert -> sofort Demo-Daten bereitstellen
            if (m_demoMode)
            {
                // Back-Buffer direkt mit Demo-Daten füllen
                int backIdx = 1 - m_activeBufferIndex.load();
                memset(&m_dataBuffers[backIdx], 0, sizeof(ToDoData));
                generateDemoData(&m_dataBuffers[backIdx]);
                m_activeBufferIndex.store(backIdx);
                m_dataReady.store(true);
                m_fetchCompleted.store(true);

                LOG_INFO("Demo-Daten nach API-Fehler bereitgestellt");

                m_consecutiveErrors = 0;
                m_backoffMs = 0;
            }
        }

        m_inFlight.store(false);
        m_workerLastSeen = millis();
    }

    LOG_SUCCESS("ToDo Worker-Task beendet sich graceful");
    m_taskHandle.store(nullptr);
    vTaskDelete(nullptr);
}

// ===================================================================
// API-FUNKTIONEN
// ===================================================================

/**
 * @brief HTTP-Datenabruf vom Google Apps Script
 */
int ToDo::fetchToDoDataCore(ToDoData *outData)
{
    if (!outData)
        return -1;

    // ===================================================================
    // DEMO-MODUS: Wenn keine gültige URL konfiguriert
    // ===================================================================
    if (m_demoMode)
    {
        if (m_shutdownRequested.load())
            return ERR_SHUTDOWN;

        generateDemoData(outData);

        LOG_INFO("Demo-Modus aktiv - fiktive Tasks generiert");
        return HTTP_CODE_OK;
    }

    // ===================================================================
    // ECHTER API-MODUS MIT HTTP_SERVICE
    // ===================================================================

    if (m_shutdownRequested.load())
    {
        LOG_INFO("HTTP-Request abgebrochen - Shutdown angefordert");
        return ERR_SHUTDOWN;
    }

    if (!m_appsScriptUrl || m_appsScriptUrl[0] == '\0')
    {
        LOG_WARNING("ToDo: Apps Script URL ist leer");
        return -1;
    }

    if (!HTTP_SERVICE.isInitialized())
    {
        LOG_ERROR("ToDo: HttpService nicht initialisiert!");
        return ERR_HTTP_NOT_INIT;
    }

    if (!APP_STATE.getWifiConnected())
    {
        LOG_WARNING("ToDo: Kein WiFi verfügbar");
        return -2;
    }

    // Aktuelles Datum direkt ins Ziel schreiben (thread-sicher)
    fillCurrentTime(outData->today);

    outData->timestamp = millis();

    LOG_INFO("Sende Anfrage an Google Apps Script...");

    // ===================================================================
    // HTTP REQUEST ÜBER ZENTRALEN SERVICE
    // ===================================================================

    HttpResponse response = HTTP_SERVICE.get(m_appsScriptUrl, &m_shutdownRequested);

    LOG_INFO_F("HTTP Response nach %lu ms, Code: %d",
               response.requestTimeMs, response.httpCode);

    if (m_shutdownRequested.load())
    {
        LOG_INFO("HTTP-Request abgeschlossen aber Shutdown angefordert");
        return ERR_SHUTDOWN;
    }

    if (!response.isSuccess())
    {
        if (response.httpCode == -200)
            return ERR_SHUTDOWN;
        return response.httpCode;
    }

    if (!response.hasData())
    {
        LOG_ERROR("Leere Antwort vom Server!");
        return ERR_EMPTY_RESPONSE;
    }

    LOG_INFO_F("Payload empfangen: %u bytes", response.length);

    if (m_shutdownRequested.load())
    {
        LOG_INFO("JSON-Parsing übersprungen - Shutdown angefordert");
        return ERR_SHUTDOWN;
    }

    // ===================================================================
    // JSON PARSING MIT PSRAM-ALLOCATOR
    // ===================================================================

    JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
    DeserializationError error = deserializeJson(doc, response.data);

    if (error)
    {
        LOG_ERROR_F("JSON Parse Error: %s", error.c_str());
        return ERR_JSON_PARSE;
    }

    if (!doc["tasks"].is<JsonArray>() || !doc["error"].isNull())
    {
        LOG_ERROR("Erforderliche Datenstruktur fehlt in der Server-Antwort");
        return ERR_SCHEMA;
    }

    // ===================================================================
    // DATEN VERARBEITEN
    // ===================================================================

    // Datum vom Server
    JsonObject today = doc["today"];
    if (today)
    {
        outData->today.currentDay = today["day"] | outData->today.currentDay;
        outData->today.currentMonth = today["month"] | outData->today.currentMonth;
        outData->today.currentYear = today["year"] | outData->today.currentYear;
        LOG_INFO("Server-Datum übernommen");
    }

    // Statistiken
    JsonObject stats = doc["stats"];
    if (stats)
    {
        outData->stats.total = stats["total"] | 0;
        outData->stats.completed = stats["completed"] | 0;
        outData->stats.overdue = stats["overdue"] | 0;
        outData->stats.dueToday = stats["dueToday"] | 0;
    }

    // Tasks verarbeiten
    JsonArray tasks = doc["tasks"];
    outData->taskCount = 0;

    // Default-Listenname einmalig in lokalen Puffer kopieren, damit die Sprache pro Fetch stabil bleibt, falls sie während des Parsings wechselt.
    char defaultListNameBuf[ToDoCfg::MAX_LIST_NAME_LEN];
    strncpy_P(defaultListNameBuf,
              reinterpret_cast<PGM_P>(STR(ToDoKey::TODO_LIST_DEFAULT_NAME)),
              sizeof(defaultListNameBuf) - 1);
    defaultListNameBuf[sizeof(defaultListNameBuf) - 1] = '\0';

    if (tasks)
    {
        for (JsonObject task : tasks)
        {
            if (outData->taskCount >= ToDoCfg::MAX_TASKS)
                break;

            const char *title = task["title"];
            if (!isValidTaskData(title))
                continue;

            TaskItem &item = outData->tasks[outData->taskCount];

            stringutils::safeCopy(item.id, task["id"] | "");
            stringutils::safeCopy(item.listId, task["listId"] | "");
            stringutils::safeCopy(item.listName, task["listName"] | defaultListNameBuf);
            stringutils::safeCopy(item.title, title);
            stringutils::safeCopy(item.notes, task["notes"] | "");
            stringutils::safeCopy(item.dueDateFormatted, task["dueDateFormatted"] | "");

            item.isCompleted = task["isCompleted"] | false;
            item.isOverdue = task["isOverdue"] | false;
            item.isDueToday = task["isDueToday"] | false;
            item.daysOverdue = task["daysOverdue"] | 0;
            item.priority = task["priority"] | 0;

            outData->taskCount++;
        }
    }

    LOG_INFO_F("Tasks geladen: %d von max %d", outData->taskCount, ToDoCfg::MAX_TASKS);

    if (!m_shutdownRequested.load())
        LOG_SUCCESS("Task-Daten erfolgreich verarbeitet");

    return m_shutdownRequested.load() ? ERR_SHUTDOWN : HTTP_CODE_OK;
}

/**
 * @brief Generiert Demo-Tasks für Präsentationen
 */
void ToDo::generateDemoData(ToDoData *outData)
{
    // ===================================================================
    // DEMO-DATEN IM FLASH (NICHT auf dem Stack -> verhindert Stack-Overflow!)
    // ===================================================================

    const bool en = (I18n::currentLang() == LangIndex::EN);

    // Listennamen (Inhalt sprachabhängig)
    static constexpr const char *const LISTE_WICHTIG_DE  = "Wichtig";
    static constexpr const char *const LISTE_WICHTIG_EN  = "Important";
    static constexpr const char *const LISTE_AUFGABEN_DE = "Aufgaben";
    static constexpr const char *const LISTE_AUFGABEN_EN = "Tasks";

    const char *const LISTE_WICHTIG  = en ? LISTE_WICHTIG_EN  : LISTE_WICHTIG_DE;
    const char *const LISTE_AUFGABEN = en ? LISTE_AUFGABEN_EN : LISTE_AUFGABEN_DE;

    // Realistische deutsche Aufgabentitel - gruppiert nach Wichtigkeit
    static constexpr const char *const AUFGABEN_WICHTIG_DE[] = {
        "Quartalsbericht für Q4 mit allen Abteilungszahlen fertigstellen",
        "Kundenpräsentation für Jahresmeeting vorbereiten und abstimmen",
        "Steuererklärung mit allen Belegen beim Finanzamt einreichen",
        "Vertrag mit externem Dienstleister prüfen und unterschreiben",
        "Projektdokumentation vor Abgabetermin finalisieren",
        "Budgetplanung für nächstes Geschäftsjahr erstellen",
        "Mitarbeitergespräche für alle Teammitglieder terminieren",
        "Angebot für Großkunden mit Sonderkonditionen ausarbeiten"};
    static constexpr const char *const AUFGABEN_WICHTIG_EN[] = {
        "Finalise Q4 quarterly report with all departmental figures",
        "Prepare and align customer presentation for annual meeting",
        "Submit tax return with all receipts to the tax office",
        "Review and sign contract with external service provider",
        "Finalise project documentation before submission deadline",
        "Draw up budget plan for the next financial year",
        "Schedule appraisal meetings for all team members",
        "Work out tender for major client with special conditions"};
    static constexpr int AUFGABEN_WICHTIG_COUNT = 8;

    static constexpr const char *const AUFGABEN_NORMAL_DE[] = {
        "E-Mails der letzten Woche durchgehen und beantworten",
        "Technische Dokumentation auf aktuellem Stand halten",
        "Code-Review für neue Feature-Branch durchführen",
        "Wocheneinkauf im Supermarkt erledigen",
        "Termin beim Zahnarzt für Kontrolle vereinbaren",
        "Auto zur Inspektion in die Werkstatt bringen",
        "Winterreifen im Keller einlagern und beschriften",
        "Alle Fenster im Erdgeschoss gründlich reinigen",
        "Balkonpflanzen umtopfen und düngen",
        "Garage entrümpeln und Regale neu sortieren",
        "Schreibtisch aufräumen und Ablage sortieren",
        "Automatisches Backup auf externer Festplatte prüfen",
        "Alle Online-Passwörter im Passwortmanager aktualisieren",
        "Geburtstagsgeschenk für Oma rechtzeitig besorgen",
        "Reiseunterlagen für Sommerurlaub zusammenstellen"};
    static constexpr const char *const AUFGABEN_NORMAL_EN[] = {
        "Go through and reply to last week's emails",
        "Keep technical documentation up to date",
        "Carry out code review for new feature branch",
        "Do the weekly shop at the supermarket",
        "Arrange dental check-up appointment",
        "Take the car in for its service",
        "Store and label winter tyres in the cellar",
        "Give all ground-floor windows a thorough clean",
        "Repot and feed the balcony plants",
        "Clear out the garage and reorganise the shelves",
        "Tidy the desk and sort the filing tray",
        "Check the automatic backup on the external hard drive",
        "Update all online passwords in the password manager",
        "Pick up birthday present for grandma in good time",
        "Put together travel documents for the summer holiday"};
    static constexpr int AUFGABEN_NORMAL_COUNT = 15;

    // Optionale Notizen
    static constexpr const char *const NOTIZEN_DE[] = {
        "", // Viele Tasks ohne Notiz
        "",
        "",
        "",
        "Wichtig: Deadline beachten!",
        "Mit Max besprechen",
        "Siehe E-Mail vom 15.",
        "Budget: ca. 50 EUR"};
    static constexpr const char *const NOTIZEN_EN[] = {
        "",
        "",
        "",
        "",
        "Important: mind the deadline!",
        "Discuss with Max",
        "See email from the 15th.",
        "Budget: approx. 50 EUR"};
    static constexpr int NOTIZEN_COUNT = 8;

    const char *const *AUFGABEN_WICHTIG = en ? AUFGABEN_WICHTIG_EN : AUFGABEN_WICHTIG_DE;
    const char *const *AUFGABEN_NORMAL  = en ? AUFGABEN_NORMAL_EN  : AUFGABEN_NORMAL_DE;
    const char *const *NOTIZEN          = en ? NOTIZEN_EN          : NOTIZEN_DE;

    // ===================================================================
    // Aktuelles Datum thread-sicher ins Ziel schreiben
    // ===================================================================
    fillCurrentTime(outData->today);

    // ===================================================================
    // TASKS GRUPPIERT GENERIEREN
    // ===================================================================

    const int totalTasks = (ToDoCfg::DEMO_TASK_COUNT > ToDoCfg::MAX_TASKS)
                               ? ToDoCfg::MAX_TASKS
                               : ToDoCfg::DEMO_TASK_COUNT;

    // Verteilung: ~30% wichtig, ~70% normal
    const int wichtigCount = (totalTasks * 3) / 10;
    const int normalCount = totalTasks - wichtigCount;

    outData->taskCount = 0;
    outData->stats = TaskStats{};
    outData->timestamp = millis();

    int taskIndex = 0;

    // ===================================================================
    // ERSTE GRUPPE: "Wichtig" Tasks
    // ===================================================================
    for (int i = 0; i < wichtigCount && outData->taskCount < ToDoCfg::MAX_TASKS; i++)
    {
        TaskItem &task = outData->tasks[outData->taskCount];

        // IDs und Liste
        stringutils::safeFormat(task.id, "demo_task_%02d", taskIndex);
        stringutils::safeCopy(task.listId, "demo_list_wichtig");
        stringutils::safeCopy(task.listName, LISTE_WICHTIG);

        // Titel und Notizen
        stringutils::safeCopy(task.title, AUFGABEN_WICHTIG[i % AUFGABEN_WICHTIG_COUNT]);
        stringutils::safeCopy(task.notes, NOTIZEN[i % NOTIZEN_COUNT]);

        // Erste 3 wichtige Tasks sind offen mit speziellem Status
        task.isCompleted = (i >= 3) && ((i % 4) == 0);

        // Initialisierung
        task.isOverdue = false;
        task.isDueToday = false;
        task.daysOverdue = 0;
        task.priority = 0;
        task.dueDateFormatted[0] = '\0';

        // Spezielle Demo-Tasks für Präsentation
        if (i == 0 && !task.isCompleted)
        {
            // Erster Task: HEUTE fällig (gelb)
            task.isDueToday = true;
            task.priority = 1;
            stringutils::safeFormat(task.dueDateFormatted, "%02d.%02d.%04d",
                                    outData->today.currentDay,
                                    outData->today.currentMonth,
                                    outData->today.currentYear);
            outData->stats.dueToday++;
        }
        else if (i == 1 && !task.isCompleted)
        {
            // Zweiter Task: 3 Tage überfällig (rot) - mit korrekter Datumsarithmetik
            task.isOverdue = true;
            task.daysOverdue = 3;
            task.priority = 2;

            // Korrekte Datumsberechnung mit mktime
            struct tm date = {};
            date.tm_year = outData->today.currentYear - 1900;
            date.tm_mon = outData->today.currentMonth - 1;
            date.tm_mday = outData->today.currentDay - 3;
            date.tm_hour = 12;
            mktime(&date); // Normalisiert das Datum

            stringutils::safeFormat(task.dueDateFormatted, "%02d.%02d.%04d",
                                    date.tm_mday, date.tm_mon + 1, date.tm_year + 1900);
            outData->stats.overdue++;
        }
        else if (i == 2 && !task.isCompleted)
        {
            // Dritter Task: 10 Tage stark überfällig (dunkelrot)
            task.isOverdue = true;
            task.daysOverdue = 10;
            task.priority = 3;

            struct tm date = {};
            date.tm_year = outData->today.currentYear - 1900;
            date.tm_mon = outData->today.currentMonth - 1;
            date.tm_mday = outData->today.currentDay - 10;
            date.tm_hour = 12;
            mktime(&date);

            stringutils::safeFormat(task.dueDateFormatted, "%02d.%02d.%04d",
                                    date.tm_mday, date.tm_mon + 1, date.tm_year + 1900);
            outData->stats.overdue++;
        }

        // Statistiken
        outData->stats.total++;
        if (task.isCompleted)
            outData->stats.completed++;

        outData->taskCount++;
        taskIndex++;
    }

    // ===================================================================
    // ZWEITE GRUPPE: "Aufgaben" Tasks
    // ===================================================================
    for (int i = 0; i < normalCount && outData->taskCount < ToDoCfg::MAX_TASKS; i++)
    {
        TaskItem &task = outData->tasks[outData->taskCount];

        // IDs und Liste
        stringutils::safeFormat(task.id, "demo_task_%02d", taskIndex);
        stringutils::safeCopy(task.listId, "demo_list_aufgaben");
        stringutils::safeCopy(task.listName, LISTE_AUFGABEN);

        // Titel und Notizen
        stringutils::safeCopy(task.title, AUFGABEN_NORMAL[i % AUFGABEN_NORMAL_COUNT]);
        stringutils::safeCopy(task.notes, NOTIZEN[(i + 3) % NOTIZEN_COUNT]);

        // Ca. 30% erledigt
        task.isCompleted = (i % 10) < 3;

        // Initialisierung
        task.isOverdue = false;
        task.isDueToday = false;
        task.daysOverdue = 0;
        task.priority = 0;
        task.dueDateFormatted[0] = '\0';

        // Einige mit Fälligkeitsdatum in der Zukunft
        if (!task.isCompleted && (i % 5) == 0)
        {
            struct tm date = {};
            date.tm_year = outData->today.currentYear - 1900;
            date.tm_mon = outData->today.currentMonth - 1;
            date.tm_mday = outData->today.currentDay + (i % 7) + 2;
            date.tm_hour = 12;
            mktime(&date);

            stringutils::safeFormat(task.dueDateFormatted, "%02d.%02d.%04d",
                                    date.tm_mday, date.tm_mon + 1, date.tm_year + 1900);
        }

        // Statistiken
        outData->stats.total++;
        if (task.isCompleted)
            outData->stats.completed++;

        outData->taskCount++;
        taskIndex++;
    }

    LOG_INFO_F("Demo-Daten generiert: %d Tasks (%d wichtig, %d normal), %d erledigt, %d überfällig, %d heute",
               outData->stats.total, wichtigCount, normalCount,
               outData->stats.completed, outData->stats.overdue, outData->stats.dueToday);
}

/**
 * @brief Sendet eine Aufgabenaktualisierung an den Server.
 */
bool ToDo::sendTaskUpdate(const char *taskId, const char *listId, bool completed)
{
    if (!taskId || !listId)
        return false;

    if (m_demoMode)
    {
        // Demo-Modus: Simuliere erfolgreiche Aktualisierung
        LOG_INFO_F("Demo-Modus: Task-Update simuliert (taskId=%s, completed=%s)",
                   taskId, completed ? "true" : "false");
        return true;
    }

    if (!APP_STATE.getWifiConnected())
    {
        LOG_WARNING("ToDo: Kein WiFi für Task-Update");
        return false;
    }

    if (!HTTP_SERVICE.isInitialized())
    {
        LOG_ERROR("ToDo: HttpService nicht initialisiert!");
        return false;
    }

    // URL mit Query-Parametern aufbauen
    char url[512];
    stringutils::safeFormat(url, "%s?action=update&taskId=%s&listId=%s&completed=%s",
                            m_appsScriptUrl, taskId, listId, completed ? "true" : "false");

    LOG_INFO_F("Sende Task-Update: %s", url);

    // HTTP Request über zentralen Service
    HttpResponse response = HTTP_SERVICE.get(url, nullptr);
    bool success = false;

    if (response.isSuccess() && response.hasData())
    {
        LOG_INFO_F("Task-Update Response: %s", response.data);

        JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
        if (deserializeJson(doc, response.data) == DeserializationError::Ok)
            success = doc["success"] | false;
    }
    else
    {
        LOG_ERROR_F("Task-Update fehlgeschlagen: HTTP %d", response.httpCode);
    }

    return success;
}

/**
 * @brief Validiert Task-Daten
 */
bool ToDo::isValidTaskData(const char *title) const
{
    return title && title[0] != '\0';
}

/**
 * @brief Bestimmt Checkbox-Farbe
 */
uint16_t ToDo::getCheckboxColor(const TaskItem &task) const
{
    return task.isCompleted ? COLOR_CHECKBOX_DONE : COLOR_CHECKBOX_BORDER;
}

/**
 * @brief Prüft ob Task bei aktuellem Filter sichtbar ist
 */
bool ToDo::isTaskVisible(const TaskItem &task) const
{
    return !task.isCompleted || m_toggleState.enabled;
}

/**
 * @brief Sucht einen Task anhand seiner ID
 */
int ToDo::findTaskIndexById(const char *taskId) const
{
    if (!taskId || taskId[0] == '\0')
        return -1;

    const ToDoData *data = &m_dataBuffers[m_activeBufferIndex.load()];

    for (int i = 0; i < data->taskCount; ++i)
    {
        if (strcmp(data->tasks[i].id, taskId) == 0)
            return i;
    }

    return -1;
}

// ===================================================================
// UI-ZEICHENFUNKTIONEN
// ===================================================================

/**
 * @brief Zeichnet die komplette Hauptansicht neu (nach Sprachwechsel)
 *
 * Räumt das Display, regeneriert im Demo-Modus die Demo-Daten in der jetzt aktiven Sprache, invalidiert die Sprite-Caches und zeichnet alle Sektionen über die regulären draw*-Funktionen neu. Im Ladescreen-State (vor dem ersten erfolgreichen Fetch) tut die Funktion nichts, weil der nächste Daten-Empfang den Vollredraw ohnehin in der neuen Sprache ausführt.
 */
void ToDo::redrawAll()
{
    if (!m_tft || !m_hasValidData)
        return;

    // Demo-Daten in der jetzt aktiven Sprache neu erzeugen
    if (m_demoMode)
    {
        ToDoData *data = getActiveBuffer();
        if (data)
            generateDemoData(data);
    }

    // Sprite-Caches invalidieren, damit Change-Detection einen Vollredraw macht
    m_taskSpriteValid = false;
    m_lastScrollOffset = -1;
    m_lastDrawnSecond = -1;

    m_tft->clear(Theme::ColorMainBackGround);

    drawStats();
    drawRefreshButton();
    drawLastUpdateTime();
    drawTasks();
    m_exitButton.drawButton(false);
    drawDigitalClock(625, (m_tft->height() - m_clockSprite->height()) + 1);
}

/**
 * @brief Zeichnet Stats-Panel
 */
void ToDo::drawStats()
{
    if (!m_tft || !m_hasValidData)
        return;

    ToDoData *data = getActiveBuffer();
    if (!data)
        return;

    m_tft->setTextDatum(TL_DATUM);
    m_tft->setFont(&fonts::Orbitron_Light_24);
    m_tft->setTextColor(TFT_SILVER, Theme::ColorMainBackGround);
    m_tft->drawString(reinterpret_cast<const char *>(STR(ToDoKey::TODO_STATS_TITLE)),
                      STATS_PANEL_X, 20);
    m_tft->loadFont(ComfortaaBold16);

    int yPos = 80;
    constexpr int lineHeight = 35;
    char numStr[8];

    // Statistik-Zeilen
    const struct
    {
        const char *label;
        int value;
        uint16_t valueColor;
    } rows[] = {
        {reinterpret_cast<const char *>(STR(ToDoKey::TODO_STATS_TOTAL)),
         data->stats.total, COLOR_NORMAL},
        {reinterpret_cast<const char *>(STR(ToDoKey::TODO_STATS_OPEN)),
         data->stats.total - data->stats.completed, COLOR_NORMAL},
        {reinterpret_cast<const char *>(STR(ToDoKey::TODO_STATS_TODAY)),
         data->stats.dueToday, COLOR_DUE_TODAY},
        {reinterpret_cast<const char *>(STR(ToDoKey::TODO_STATS_OVERDUE)),
         data->stats.overdue,
         data->stats.overdue > 0 ? COLOR_OVERDUE : COLOR_NORMAL}};

    // Fontmetrik für Badge-Höhe
    const int fontHeight = m_tft->fontHeight();
    const int badgeHeight = fontHeight + BADGE_PADDING_V * 2;

    for (const auto &row : rows)
    {
        // Label zeichnen
        m_tft->setTextColor(COLOR_LABEL, Theme::ColorMainBackGround);
        m_tft->drawString(row.label, STATS_PANEL_X, yPos);

        // Wert als String formatieren
        stringutils::safeFormat(numStr, "%d", row.value);

        // Badge-Dimensionen berechnen
        const int textWidth = m_tft->textWidth(numStr);
        const int badgeWidth = std::max(textWidth + BADGE_PADDING_H * 2, BADGE_MIN_WIDTH);
        const int badgeX = STATS_PANEL_X + STATS_PANEL_WIDTH - badgeWidth;
        const int badgeY = yPos - BADGE_PADDING_V + 2;

        // Badge-Hintergrund zeichnen
        m_tft->fillSmoothRoundRect(badgeX, badgeY, badgeWidth, badgeHeight, BADGE_RADIUS, COLOR_BADGE_BG);

        // Wert zentriert im Badge zeichnen
        m_tft->setTextColor(row.valueColor, COLOR_BADGE_BG);
        m_tft->setTextDatum(MC_DATUM);
        m_tft->drawString(numStr, badgeX + badgeWidth / 2, badgeY + badgeHeight / 2);
        m_tft->setTextDatum(TL_DATUM);

        yPos += lineHeight;
    }

    m_tft->unloadFont();

    drawToggleSwitch();
}

/**
 * @brief Zeichnet Toggle-Switch
 */
void ToDo::drawToggleSwitch()
{
    if (!m_tft)
        return;

    const bool isOn = m_toggleState.enabled;
    const uint16_t bgColor = isOn ? Theme::ColorButtonBorder : TFT_DARKGREY;

    m_tft->fillSmoothRoundRect(TOGGLE_X, TOGGLE_Y, TOGGLE_WIDTH, TOGGLE_HEIGHT, TOGGLE_RADIUS, bgColor);
    m_tft->fillSmoothRoundRect(TOGGLE_X + 2, TOGGLE_Y + 2, TOGGLE_WIDTH - 4, TOGGLE_HEIGHT - 4, TOGGLE_RADIUS - 2, Theme::ColorMainBackGround);

    const int knobX = isOn ? (TOGGLE_X + TOGGLE_WIDTH - KNOB_SIZE - KNOB_MARGIN)
                           : (TOGGLE_X + KNOB_MARGIN);

    const int knobY = TOGGLE_Y + (TOGGLE_HEIGHT - KNOB_SIZE) / 2;

    m_tft->fillSmoothCircle(knobX + KNOB_SIZE / 2, knobY + KNOB_SIZE / 2, KNOB_SIZE / 2, isOn ?  RetroColors::Olive : Theme::ColorRetroRed);

    m_tft->loadFont(ComfortaaBold14);
    m_tft->setTextColor(COLOR_LABEL, Theme::ColorMainBackGround);
    m_tft->setTextDatum(ML_DATUM);
    m_tft->drawString(reinterpret_cast<const char *>(STR(ToDoKey::TODO_TOGGLE_LABEL)),
                      TOGGLE_X + TOGGLE_WIDTH + 12, TOGGLE_Y + TOGGLE_HEIGHT / 2);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->unloadFont();
}

/**
 * @brief Zeichnet den Refresh-Button
 */
void ToDo::drawRefreshButton()
{
    m_refreshButton.drawButton(false);
}

/**
 * @brief Zeichnet den Zeitstempel der letzten Aktualisierung
 */
void ToDo::drawLastUpdateTime()
{
    if (!m_tft || m_lastUpdateTimestamp[0] == '\0')
        return;

    m_tft->loadFont(ComfortaaBold14);
    m_tft->setTextColor(COLOR_TIMESTAMP, Theme::ColorMainBackGround);
    m_tft->setTextDatum(TC_DATUM);
    m_tft->drawString(m_lastUpdateTimestamp, REFRESH_BTN_X, TIMESTAMP_Y);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->unloadFont();
}

/**
 * @brief Zeichnet Task-Liste (optimiert: Font nur 1x laden)
 */
void ToDo::drawTasks()
{
    ToDoData *data = getActiveBuffer();
    if (!data || !m_taskSprite)
        return;

    // Basis-Geometrie
    constexpr int baseTextX = TASK_INDENT + PRIORITY_RECT_MARGIN + PRIORITY_RECT_WIDTH + 8 + CHECKBOX_SIZE + 10;
    constexpr int availableTextWidth = DATE_COLUMN_X - baseTextX - 10;

    // Content-Höhe und maxScroll berechnen
    m_scrollState.contentHeight = calculateContentHeight();
    m_scrollState.viewportHeight = VIEWPORT_HEIGHT;
    m_scrollState.maxScroll = std::max(0, m_scrollState.contentHeight - VIEWPORT_HEIGHT);

    constrainScroll();
    drawScrollbar();

    // Prüfen ob Neuzeichnen nötig
    const bool needsRedraw = !m_taskSpriteValid ||
                             (m_lastScrollOffset != m_scrollState.scrollOffset) ||
                             !m_layoutDrawn;

    if (!needsRedraw)
        return;

    m_taskSprite->clear(Theme::ColorMainBackGround);
    m_taskSprite->setTextDatum(TL_DATUM);

    int yPos = -m_scrollState.scrollOffset;
    const char *lastListName = "";
    bool isFirstVisibleList = true;

    // Buffer für Zeilenumbruch
    char line1[LINE_BUFFER_SIZE];
    char line2[LINE_BUFFER_SIZE];
    char truncatedTitle[LINE_BUFFER_SIZE + 4];

    for (int i = 0; i < data->taskCount; ++i)
    {
        const TaskItem &task = data->tasks[i];

        // Erledigte Aufgaben überspringen wenn der ToggleSwitch aus ist
        if (!isTaskVisible(task))
            continue;

        // Neue Liste erkannt?
        if (strcmp(task.listName, lastListName) != 0)
        {
            // Trennstrich (nur wenn es nicht die erste sichtbare Liste ist)
            if (!isFirstVisibleList)
            {
                if (yPos > -LIST_SEPARATOR_HEIGHT && yPos < VIEWPORT_HEIGHT)
                {
                    m_taskSprite->fillRect(0, yPos, LIST_WIDTH, LIST_SEPARATOR_HEIGHT, Theme::ColorMainBackGround);
                    m_taskSprite->drawFastHLine(10, yPos + LIST_SEPARATOR_HEIGHT / 2, LIST_WIDTH - 20, COLOR_SEPARATOR);
                }

                yPos += LIST_SEPARATOR_HEIGHT;
            }

            // Listenüberschrift
            if (yPos > -LIST_HEADER_HEIGHT && yPos < VIEWPORT_HEIGHT)
            {
                m_taskSprite->fillRect(0, yPos, LIST_WIDTH, LIST_HEADER_HEIGHT, Theme::ColorMainBackGround);
                m_taskSprite->setTextColor(COLOR_LABEL, Theme::ColorMainBackGround);
                m_taskSprite->setTextDatum(ML_DATUM);
                m_taskSprite->drawString(task.listName, 10, yPos + LIST_HEADER_HEIGHT / 2);
                m_taskSprite->setTextDatum(TL_DATUM);
            }

            yPos += LIST_HEADER_HEIGHT;

            lastListName = task.listName;
            isFirstVisibleList = false;
        }

        // Skip wenn komplett außerhalb
        if (yPos > VIEWPORT_HEIGHT + TASK_HEIGHT)
            break;

        if (yPos + TASK_HEIGHT < -TASK_HEIGHT)
        {
            yPos += TASK_HEIGHT;
            continue;
        }

        // Task-Hintergrund
        m_taskSprite->fillRect(0, yPos, LIST_WIDTH, TASK_HEIGHT, Theme::ColorMainBackGround);

        // Prioritäts-Farbe bestimmen
        uint16_t priorityColor;
        if (task.isCompleted)
            priorityColor = COLOR_COMPLETED;
        else if (task.isOverdue)
            priorityColor = (task.daysOverdue >= 7) ? COLOR_OVERDUE_STRONG : COLOR_OVERDUE;
        else if (task.isDueToday)
            priorityColor = COLOR_DUE_TODAY;
        else
            priorityColor = Theme::ColorMarker;

        // Prioritäts-Rechteck
        const int rectX = TASK_INDENT + PRIORITY_RECT_MARGIN;
        const int rectHeight = CHECKBOX_SIZE;
        const int rectY = yPos + (TASK_HEIGHT - rectHeight) / 2;
        m_taskSprite->fillRect(rectX, rectY, PRIORITY_RECT_WIDTH, rectHeight, priorityColor);

        // Checkbox - Pending-Status anhand der gespeicherten ID prüfen
        const int checkboxX = TASK_INDENT + PRIORITY_RECT_MARGIN + PRIORITY_RECT_WIDTH + 8;
        const int checkboxY = rectY + (rectHeight - CHECKBOX_SIZE) / 2;
        const bool isPending = (m_messageBoxTaskId[0] != '\0' && strcmp(task.id, m_messageBoxTaskId) == 0);
        const uint16_t checkboxColor = isPending ? COLOR_CHECKBOX_PENDING : getCheckboxColor(task);
        drawCheckbox(checkboxX, checkboxY, task.isCompleted, checkboxColor);

        // Task-Titel (Font ist bereits geladen!)
        const uint16_t textColor = task.isCompleted ? COLOR_COMPLETED : COLOR_NORMAL;
        m_taskSprite->setTextColor(textColor);

        const int textX = checkboxX + CHECKBOX_SIZE + 10;
        const int fullTextWidth = m_taskSprite->textWidth(task.title);

        if (fullTextWidth <= availableTextWidth)
        {
            // Eine Zeile
            const int textY = yPos + (TASK_HEIGHT - TASK_LINE_HEIGHT) / 2;
            m_taskSprite->drawString(task.title, textX, textY);

            if (task.isCompleted)
            {
                const int lineY = textY + TASK_LINE_HEIGHT / 2;
                m_taskSprite->drawFastHLine(textX, lineY, fullTextWidth, COLOR_COMPLETED);
            }
        }
        else
        {
            // Zeilenumbruch nötig
            line1[0] = '\0';
            line2[0] = '\0';

            // Wort für Wort einfügen
            char tempBuffer[LINE_BUFFER_SIZE];
            strlcpy(tempBuffer, task.title, sizeof(tempBuffer));

            char testLine[LINE_BUFFER_SIZE] = "";
            char *word = strtok(tempBuffer, " ");

            while (word)
            {
                char tryLine[LINE_BUFFER_SIZE];
                if (testLine[0] == '\0')
                    strlcpy(tryLine, word, sizeof(tryLine));
                else
                    stringutils::safeFormat(tryLine, "%s %s", testLine, word);

                if (m_taskSprite->textWidth(tryLine) <= availableTextWidth)
                {
                    strlcpy(testLine, tryLine, sizeof(testLine));
                }
                else
                {
                    if (testLine[0] != '\0')
                        strlcpy(line1, testLine, sizeof(line1));

                    // Rest für Zeile 2
                    strlcpy(line2, word, sizeof(line2));
                    word = strtok(nullptr, " ");
                    while (word)
                    {
                        strlcat(line2, " ", sizeof(line2));
                        strlcat(line2, word, sizeof(line2));
                        word = strtok(nullptr, " ");
                    }
                    break;
                }
                word = strtok(nullptr, " ");
            }

            if (line1[0] == '\0' && testLine[0] != '\0')
                strlcpy(line1, testLine, sizeof(line1));

            // Zeile 2 kürzen falls nötig
            if (line2[0] != '\0' && m_taskSprite->textWidth(line2) > availableTextWidth)
            {
                truncateTextToWidth(line2, availableTextWidth, m_taskSprite, truncatedTitle, sizeof(truncatedTitle));
                strlcpy(line2, truncatedTitle, sizeof(line2));
            }

            // Zeichnen
            const int textY1 = yPos + (TASK_HEIGHT - 2 * TASK_LINE_HEIGHT) / 2;
            const int textY2 = textY1 + TASK_LINE_HEIGHT;

            if (line1[0] != '\0')
            {
                m_taskSprite->drawString(line1, textX, textY1);
                if (task.isCompleted)
                {
                    const int lineWidth = m_taskSprite->textWidth(line1);
                    m_taskSprite->drawFastHLine(textX, textY1 + TASK_LINE_HEIGHT / 2, lineWidth, COLOR_COMPLETED);
                }
            }

            if (line2[0] != '\0')
            {
                m_taskSprite->drawString(line2, textX, textY2);
                if (task.isCompleted)
                {
                    const int lineWidth = m_taskSprite->textWidth(line2);
                    m_taskSprite->drawFastHLine(textX, textY2 + TASK_LINE_HEIGHT / 2, lineWidth, COLOR_COMPLETED);
                }
            }
        }

        // Trennlinie für Datumsspalte
        m_taskSprite->drawFastVLine(DATE_COLUMN_X, rectY, CHECKBOX_SIZE, Theme::ColorButtonBorder);

        // Datum
        if (task.dueDateFormatted[0] != '\0')
        {
            uint16_t dateColor = COLOR_NORMAL;
            if (task.isOverdue)
                dateColor = COLOR_OVERDUE;
            else if (task.isDueToday)
                dateColor = COLOR_DUE_TODAY;

            m_taskSprite->setTextColor(dateColor);
            m_taskSprite->setTextDatum(MC_DATUM);

            const int dateCenterX = DATE_COLUMN_X + (LIST_WIDTH - DATE_COLUMN_X) / 2;
            const int dateCenterY = yPos + TASK_HEIGHT / 2;
            m_taskSprite->drawString(task.dueDateFormatted, dateCenterX, dateCenterY);
            m_taskSprite->setTextDatum(TL_DATUM);
        }

        yPos += TASK_HEIGHT;
    }

    // Keine Tasks
    if (data->taskCount == 0)
    {
        m_taskSprite->setTextColor(TFT_DARKGREY, Theme::ColorMainBackGround);
        m_taskSprite->setTextDatum(MC_DATUM);
        m_taskSprite->drawString(reinterpret_cast<const char *>(STR(ToDoKey::TODO_EMPTY_LIST)),
                                 LIST_WIDTH / 2, VIEWPORT_HEIGHT / 2);
    }

    m_taskSprite->pushSprite(0, 0);

    m_taskSpriteValid = true;
    m_lastScrollOffset = m_scrollState.scrollOffset;
}

/**
 * @brief Zeichnet Checkbox
 */
void ToDo::drawCheckbox(int x, int y, bool checked, uint16_t color)
{
    if (!m_taskSprite)
        return;

    m_taskSprite->drawRect(x, y, CHECKBOX_SIZE, CHECKBOX_SIZE, color);

    if (checked)
    {
        const int cx = x + CHECKBOX_SIZE / 2;
        const int cy = y + CHECKBOX_SIZE / 2;
        m_taskSprite->drawWideLine(cx - 5, cy - 1, cx - 1, cy + 3, 2, color);
        m_taskSprite->drawWideLine(cx - 1, cy + 3, cx + 6, cy - 4, 2, color);
    }
}

/**
 * @brief Kürzt Text auf maximale Breite (UTF-8 sicher mit Binary Search)
 */
void ToDo::truncateTextToWidth(const char *text, int maxWidth, LGFX_Sprite *sprite, char *output, size_t outputSize) const
{
    if (!text || !sprite || !output || outputSize == 0)
    {
        if (output)
            output[0] = '\0';

        return;
    }

    if (sprite->textWidth(text) <= maxWidth)
    {
        strlcpy(output, text, outputSize);
        return;
    }

    const int ellipsisWidth = sprite->textWidth("...");
    const int targetWidth = maxWidth - ellipsisWidth;

    const size_t len = strlen(text);

    // Binary Search für optimale Länge
    size_t low = 0;
    size_t high = len;
    size_t bestFit = 0;

    char tempBuffer[LINE_BUFFER_SIZE];

    while (low <= high && high <= len)
    {
        size_t mid = (low + high) / 2;

        // UTF-8 sicher: Nicht mitten im Multi-Byte Zeichen schneiden Continuation bytes haben das Format 10xxxxxx (0x80-0xBF)
        while (mid > 0 && (static_cast<unsigned char>(text[mid]) & 0xC0) == 0x80)
            mid--;

        const size_t copyLen = std::min<size_t>(mid, sizeof(tempBuffer) - 1);
        memcpy(tempBuffer, text, copyLen);
        tempBuffer[copyLen] = '\0';

        if (sprite->textWidth(tempBuffer) <= targetWidth)
        {
            bestFit = mid;
            low = mid + 1;
        }
        else
        {
            if (mid == 0)
                break;
            high = mid - 1;
        }
    }

    // UTF-8 safe: Finale Position auf gültiges Zeichen prüfen
    size_t finalLen = bestFit;
    while (finalLen > 0 && (static_cast<unsigned char>(text[finalLen]) & 0xC0) == 0x80)
        finalLen--;

    // Kopieren und Ellipse anhängen
    const size_t maxCopy = std::min<size_t>(finalLen, outputSize - 4);
    memcpy(output, text, maxCopy);
    output[maxCopy] = '\0';
    strlcat(output, "...", outputSize);
}

/**
 * @brief Berechnet Content-Höhe (mit Filter-Berücksichtigung)
 */
int ToDo::calculateContentHeight() const
{
    // Const-Version: Direkter Zugriff auf aktiven Buffer
    const ToDoData *data = &m_dataBuffers[m_activeBufferIndex.load()];
    if (!m_hasValidData || data->taskCount == 0)
        return 0;

    int height = 0;
    const char *lastListName = "";
    bool isFirstVisibleList = true;

    for (int i = 0; i < data->taskCount; ++i)
    {
        const TaskItem &task = data->tasks[i];

        if (!isTaskVisible(task))
            continue;

        // Neue Liste?
        if (strcmp(task.listName, lastListName) != 0)
        {
            if (!isFirstVisibleList)
                height += LIST_SEPARATOR_HEIGHT;

            height += LIST_HEADER_HEIGHT;
            lastListName = task.listName;
            isFirstVisibleList = false;
        }

        height += TASK_HEIGHT;
    }

    return height;
}

// ===================================================================
// SCROLL-FUNKTIONEN
// ===================================================================

/**
 * @brief Verarbeitet Touch für Scrolling
 */
void ToDo::handleScrollTouch(int32_t tx, int32_t ty, bool touched)
{
    if (touched)
    {
        if (tx > SCROLLBAR_X)
            return;

        if (!m_scrollState.isDragging)
        {
            m_scrollState.isDragging = true;
            m_scrollState.dragStartY = ty;
            m_scrollState.dragStartOffset = m_scrollState.scrollOffset;
            m_scrollState.scrollVelocity = 0;
            m_scrollState.isSnappingBack = false;
            m_lastTouchY = ty;
            m_lastTouchTime = millis();
        }
        else
        {
            const int deltaY = ty - m_scrollState.dragStartY;
            m_scrollState.scrollOffset = m_scrollState.dragStartOffset - static_cast<int>(deltaY * 1.5f);

            // Velocity für Momentum
            const unsigned long now = millis();
            const int timeDelta = now - m_lastTouchTime;

            if (timeDelta > 0 && timeDelta < 100)
            {
                const float newVelocity = (m_lastTouchY - ty) * 1000.0f / timeDelta;
                m_scrollState.scrollVelocity = (newVelocity * 0.7f) + (m_scrollState.scrollVelocity * 0.3f);
                m_scrollState.scrollVelocity *= 1.3f;
            }

            m_lastTouchY = ty;
            m_lastTouchTime = now;

            // Gummiband-Effekt
            if (m_scrollState.scrollOffset < -50)
            {
                m_scrollState.scrollOffset = -50 + static_cast<int>((m_scrollState.scrollOffset + 50) * 0.3f);
            }
            else if (m_scrollState.scrollOffset > m_scrollState.maxScroll + 50)
            {
                const float excess = m_scrollState.scrollOffset - m_scrollState.maxScroll - 50;
                m_scrollState.scrollOffset = m_scrollState.maxScroll + 50 + static_cast<int>(excess * 0.3f);
            }
        }
    }
    else if (m_scrollState.isDragging)
    {
        m_scrollState.isDragging = false;

        // Velocity begrenzen
        m_scrollState.scrollVelocity = constrain(m_scrollState.scrollVelocity, -2500.0f, 2500.0f);

        // Snap-Animation
        if (m_scrollState.scrollOffset < 0 || m_scrollState.scrollOffset > m_scrollState.maxScroll)
            m_scrollState.isSnappingBack = true;
    }
}

/**
 * @brief Aktualisiert Scroll-Physik
 */
void ToDo::updateScrollPhysics()
{
    if (m_scrollState.isDragging)
        return;

    if (m_scrollState.scrollOffset < 0)
    {
        const float diff = -m_scrollState.scrollOffset;
        m_scrollState.scrollOffset += static_cast<int>(diff * 0.35f);

        if (std::abs(m_scrollState.scrollOffset) < 1)
        {
            m_scrollState.scrollOffset = 0;
            m_scrollState.scrollVelocity = 0;
            m_scrollState.isSnappingBack = false;
        }
    }
    else if (m_scrollState.scrollOffset > m_scrollState.maxScroll)
    {
        const float diff = m_scrollState.maxScroll - m_scrollState.scrollOffset;
        m_scrollState.scrollOffset += static_cast<int>(diff * 0.35f);

        if (std::abs(m_scrollState.scrollOffset - m_scrollState.maxScroll) < 1)
        {
            m_scrollState.scrollOffset = m_scrollState.maxScroll;
            m_scrollState.scrollVelocity = 0;
            m_scrollState.isSnappingBack = false;
        }
    }
    else if (std::abs(m_scrollState.scrollVelocity) > 0.5f)
    {
        m_scrollState.scrollOffset += static_cast<int>(m_scrollState.scrollVelocity / 60.0f);
        m_scrollState.scrollVelocity *= 0.92f;

        if (std::abs(m_scrollState.scrollVelocity) < 1)
            m_scrollState.scrollVelocity = 0;

        if (m_scrollState.scrollOffset < 0)
        {
            m_scrollState.scrollOffset = 0;
            m_scrollState.scrollVelocity = 0;
        }
        else if (m_scrollState.scrollOffset > m_scrollState.maxScroll)
        {
            m_scrollState.scrollOffset = m_scrollState.maxScroll;
            m_scrollState.scrollVelocity = 0;
        }
    }
    else
    {
        m_scrollState.isSnappingBack = false;
    }
}

/**
 * @brief Begrenzt Scroll-Position
 */
void ToDo::constrainScroll()
{
    if (m_scrollState.isDragging)
        return;

    if (m_scrollState.scrollOffset < 0)
    {
        m_scrollState.scrollOffset = 0;
        m_scrollState.scrollVelocity = 0;
    }
    else if (m_scrollState.scrollOffset > m_scrollState.maxScroll)
    {
        m_scrollState.scrollOffset = m_scrollState.maxScroll;
        m_scrollState.scrollVelocity = 0;
    }
}

/**
 * @brief Zeichnet Scrollbar
 */
void ToDo::drawScrollbar()
{
    if (!m_scrollbarSprite)
        return;

    constexpr int barWidth = 3;

    m_scrollbarSprite->clear(Theme::ColorMainBackGround);
    m_scrollbarSprite->fillRect(0, 0, barWidth, VIEWPORT_HEIGHT, COLOR_SCROLLBAR_TRACK);

    if (m_scrollState.contentHeight > VIEWPORT_HEIGHT)
    {
        const float visibleRatio = static_cast<float>(VIEWPORT_HEIGHT) / m_scrollState.contentHeight;
        int barHeight = static_cast<int>(VIEWPORT_HEIGHT * visibleRatio);
        barHeight = std::clamp(barHeight, 30, VIEWPORT_HEIGHT);

        float scrollRatio = 0;
        if (m_scrollState.maxScroll > 0)
        {
            const int clampedOffset = std::clamp(m_scrollState.scrollOffset, 0, m_scrollState.maxScroll);
            scrollRatio = static_cast<float>(clampedOffset) / m_scrollState.maxScroll;
        }

        const int barY = static_cast<int>(scrollRatio * (VIEWPORT_HEIGHT - barHeight));

        uint16_t barColor;
        if (m_scrollState.isDragging)
            barColor = COLOR_SCROLLBAR_ACTIVE;
        else if (m_scrollState.isSnappingBack || std::abs(m_scrollState.scrollVelocity) > 10)
            barColor = COLOR_SCROLLBAR_ANIM;
        else
            barColor = COLOR_SCROLLBAR_NORMAL;

        m_scrollbarSprite->fillRect(0, barY + 1, barWidth, barHeight - 2, barColor);
    }

    m_scrollbarSprite->pushSprite(SCROLLBAR_X, 0);
}

// ===================================================================
// TASK-INTERAKTION
// ===================================================================

/**
 * @brief Ermittelt Task an Touch-Position (mit Filter-Berücksichtigung)
 */
int ToDo::getTaskAtPosition(int32_t tx, int32_t ty) const
{
    // Const-Version: Direkter Zugriff auf aktiven Buffer
    const ToDoData *data = &m_dataBuffers[m_activeBufferIndex.load()];
    if (!m_hasValidData || data->taskCount == 0)
        return -1;

    // Checkbox-Bereich prüfen
    constexpr int checkboxX = TASK_INDENT + PRIORITY_RECT_MARGIN + PRIORITY_RECT_WIDTH + 8;
    if (tx > checkboxX + CHECKBOX_SIZE + 30)
        return -1;

    const int relativeY = ty + m_scrollState.scrollOffset;
    if (relativeY < 0)
        return -1;

    int yPos = 0;
    const char *lastListName = "";
    bool isFirstVisibleList = true;

    for (int i = 0; i < data->taskCount; ++i)
    {
        const TaskItem &task = data->tasks[i];

        // Filter wie bei drawTasks
        if (!isTaskVisible(task))
            continue;

        // Neue Liste?
        if (strcmp(task.listName, lastListName) != 0)
        {
            if (!isFirstVisibleList)
                yPos += LIST_SEPARATOR_HEIGHT;

            yPos += LIST_HEADER_HEIGHT;
            lastListName = task.listName;
            isFirstVisibleList = false;
        }

        if (relativeY >= yPos && relativeY < yPos + TASK_HEIGHT)
            return i;

        yPos += TASK_HEIGHT;
    }

    return -1;
}

/**
 * @brief Verarbeitet Checkbox-Tap
 */
void ToDo::handleCheckboxTap(int taskIndex)
{
    ToDoData *data = getActiveBuffer();
    if (!data || taskIndex < 0 || taskIndex >= data->taskCount)
        return;

    // Sperre wenn Update oder Dialog läuft
    if (m_messageBoxTaskId[0] != '\0' || (m_messageBox && m_messageBox->isActive()))
    {
        LOG_INFO("Checkbox-Tap ignoriert - Update oder Dialog läuft bereits");
        return;
    }

    TaskItem &task = data->tasks[taskIndex];
    const bool newStatus = !task.isCompleted;

    LOG_INFO_F("Checkbox getappt: Task %d '%s' -> %s", taskIndex, task.title, newStatus ? "erledigt" : "offen");

    // Task-ID speichern (statt Index für Refresh-Stabilität)
    stringutils::safeCopy(m_messageBoxTaskId, task.id);
    m_messageBoxNewStatus = newStatus;

    m_messageBox->setTitle(newStatus
                               ? reinterpret_cast<const char *>(STR(ToDoKey::TODO_DLG_TITLE_COMPLETE))
                               : reinterpret_cast<const char *>(STR(ToDoKey::TODO_DLG_TITLE_REOPEN)));
    m_messageBox->setMessage(task.title);

    const int boxWidth = 340;
    const int boxX = (m_tft->width() - boxWidth) / 2;
    m_messageBox->setAutoBounds(boxX, 0, boxWidth, m_tft->height());

    m_messageBox->setButtonLabels(
        reinterpret_cast<const char *>(STR(CommonKey::COMMON_BTN_CONTINUE)),
        reinterpret_cast<const char *>(STR(CommonKey::COMMON_BTN_CANCEL)));
    m_messageBox->setLoadingText(reinterpret_cast<const char *>(STR(ToDoKey::TODO_DLG_LOADING)));
    m_messageBox->setTimeout(10000);

    m_messageBox->setCallback([this](bool confirmed)
                              { onMessageBoxResult(confirmed); });
    m_messageBox->start();
}

/**
 * @brief MessageBox-Callback bei Abbruch
 */
void ToDo::onMessageBoxResult(bool)
{
    m_messageBoxTaskId[0] = '\0';
    m_messageBoxNewStatus = false;
    m_taskSpriteValid = false;
    drawTasks();
}

/**
 * @brief Verarbeitet MessageBox-Bestätigung
 */
void ToDo::processMessageBoxConfirmation()
{
    // Refresh während Update blockieren - verhindert Inkonsistenzen
    if (m_inFlight.load())
    {
        LOG_WARNING("Update übersprungen - Refresh läuft");
        m_messageBox->completeConfirmation(false);
        m_messageBoxTaskId[0] = '\0';
        m_messageBoxNewStatus = false;
        return;
    }

    // Task anhand der ID finden (nicht Index - für Refresh-Stabilität)
    const int taskIndex = findTaskIndexById(m_messageBoxTaskId);

    ToDoData *data = getActiveBuffer();
    if (taskIndex < 0 || !data || taskIndex >= data->taskCount)
    {
        LOG_WARNING_F("Task mit ID '%s' nicht mehr gefunden", m_messageBoxTaskId);
        m_messageBox->completeConfirmation(false);
        m_messageBoxTaskId[0] = '\0';
        m_messageBoxNewStatus = false;

        return;
    }

    TaskItem &task = data->tasks[taskIndex];
    const bool newStatus = m_messageBoxNewStatus;
    const bool success = sendTaskUpdate(task.id, task.listId, newStatus);

    m_messageBox->completeConfirmation(success);

    if (success)
    {
        task.isCompleted = newStatus;

        if (newStatus)
        {
            // Erledigt
            const bool wasOverdue = task.isOverdue;
            const bool wasDueToday = task.isDueToday;

            task.isOverdue = false;
            task.priority = 0;

            data->stats.completed++;

            if (wasOverdue)
                data->stats.overdue--;

            if (wasDueToday)
                data->stats.dueToday--;
        }
        else
        {
            // Wieder geöffnet
            data->stats.completed--;

            if (task.dueDateFormatted[0] != '\0')
            {
                int dueDay = 0, dueMonth = 0, dueYear = 0;
                if (sscanf(task.dueDateFormatted, "%d.%d.%d", &dueDay, &dueMonth, &dueYear) == 3)
                {
                    const int todayValue = data->today.currentYear * 10000 + data->today.currentMonth * 100 + data->today.currentDay;
                    const int dueValue = dueYear * 10000 + dueMonth * 100 + dueDay;

                    if (dueValue < todayValue)
                    {
                        task.isOverdue = true;
                        task.isDueToday = false;

                        struct tm dueDate = {};
                        dueDate.tm_year = dueYear - 1900;
                        dueDate.tm_mon = dueMonth - 1;
                        dueDate.tm_mday = dueDay;
                        const time_t dueTime = mktime(&dueDate);

                        struct tm todayDate = {};
                        todayDate.tm_year = data->today.currentYear - 1900;
                        todayDate.tm_mon = data->today.currentMonth - 1;
                        todayDate.tm_mday = data->today.currentDay;
                        const time_t todayTime = mktime(&todayDate);

                        task.daysOverdue = static_cast<int>((todayTime - dueTime) / (60 * 60 * 24));
                        task.priority = (task.daysOverdue >= 7) ? 3 : 2;

                        data->stats.overdue++;
                        LOG_INFO_F("Task wieder geöffnet - %d Tage überfällig", task.daysOverdue);
                    }
                    else if (dueValue == todayValue)
                    {
                        task.isOverdue = false;
                        task.isDueToday = true;
                        task.daysOverdue = 0;
                        task.priority = 1;

                        data->stats.dueToday++;
                        LOG_INFO("Task wieder geöffnet - heute fällig");
                    }
                    else
                    {
                        task.isOverdue = false;
                        task.isDueToday = false;
                        task.daysOverdue = 0;
                        task.priority = 0;
                    }
                }
            }
        }

        drawStats();
        LOG_SUCCESS_F("Task '%s' erfolgreich aktualisiert", task.title);
    }
    else
    {
        LOG_WARNING("Task-Update fehlgeschlagen - Status nicht geändert");
    }

    m_messageBoxTaskId[0] = '\0';
    m_messageBoxNewStatus = false;
    m_taskSpriteValid = false;
    drawTasks();
}

/**
 * @brief Zeichnet Digitaluhr
 */
void ToDo::drawDigitalClock(int32_t x, int32_t y)
{
    if (!m_clockSprite || !m_hasValidData)
        return;

    DS1307_RTC *rtc = &APP_STATE.getRTC();
    Clocks::drawDigitalClockPlain(m_clockSprite, rtc, m_lastDrawnSecond, x, y, false, true);
}

// ===================================================================
// TOGGLE-SWITCH FUNKTIONEN
// ===================================================================

/**
 * @brief Prüft ob Toggle-Switch berührt wurde
 */
bool ToDo::isToggleTouched(int32_t tx, int32_t ty) const
{
    return (tx >= TOGGLE_X - TOGGLE_TOUCH_MARGIN &&
            tx <= TOGGLE_X + TOGGLE_WIDTH + TOGGLE_TOUCH_MARGIN &&
            ty >= TOGGLE_Y - TOGGLE_TOUCH_MARGIN &&
            ty <= TOGGLE_Y + TOGGLE_HEIGHT + TOGGLE_TOUCH_MARGIN);
}

/**
 * @brief Verarbeitet Toggle-Tap
 */
void ToDo::handleToggleTap()
{
    m_toggleState.enabled = !m_toggleState.enabled;
    APP_STATE.setToDoShowCompleted(m_toggleState.enabled);

    LOG_INFO_F("Toggle umgeschaltet: Erledigte Tasks %s (gespeichert in NVRAM)", m_toggleState.enabled ? "anzeigen" : "ausblenden");

    m_scrollState.reset();
    m_taskSpriteValid = false;
    m_lastScrollOffset = -1;
    drawTasks();
}

/**
 * @brief Prüft ob Refresh-Button berührt wurde
 */
bool ToDo::isRefreshButtonTouched(int32_t tx, int32_t ty) const
{
    return m_refreshButton.contains(tx, ty);
}

/**
 * @brief Verarbeitet Refresh-Button-Tap
 */
void ToDo::handleRefreshButtonTap()
{
    LOG_INFO("Manueller Refresh getriggert");

    m_fetchCompleted.store(false);

    if (m_progressBox)
    {
        m_progressBox->setMessage(reinterpret_cast<const char *>(STR(ToDoKey::TODO_PROGRESS_MANUAL_MSG)));
        m_progressBox->start(nullptr, -1);
    }

    requestRefresh(true);
}
