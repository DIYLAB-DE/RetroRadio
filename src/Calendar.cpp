#include "Calendar.h"

#include <ArduinoJson.h>
#include <time.h>

#include <algorithm>
#include <cmath>

#include "AppState.h"
#include "HttpService.h"
#include "I18n.h"
#include "Strings_calendar.h"
#include "StringUtils.h"
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
    constexpr BaseType_t WORKER_CORE = 1;       ///< Core 1 (Core 0 ist oft mit WiFi/BT belegt)

    // UI Button-Konstanten
    constexpr int16_t EXIT_BUTTON_X = 765;
    constexpr int16_t EXIT_BUTTON_Y = 457;
    constexpr uint16_t BUTTON_SIZE = 45;
    constexpr uint16_t BUTTON_RADIUS = 4;
    constexpr int BUTTON_COLOR = 0x39E8;

    constexpr int LEFT_PANEL_WIDTH = 440;
    constexpr int RIGHT_PANEL_X = 480;
    constexpr int VIEWPORT_HEIGHT = 480;
    constexpr int LEFT_MARGIN = 20;
    constexpr int CELL_WIDTH = 38;
    constexpr int CELL_HEIGHT = 28;
    constexpr int WEEK_COLUMN_WIDTH = 25;

    // Watchdog Timeouts
    constexpr unsigned long WATCHDOG_IDLE_MS = 90000UL;
    constexpr unsigned long WATCHDOG_BUSY_MS = 45000UL;

    // Farben
    constexpr uint16_t COLOR_PROGRESS_BAR = 0x8010;
    constexpr uint16_t COLOR_LOADING_TEXT = 0xD69A;
    constexpr uint16_t COLOR_BIRTHDAY = 0xF800;
    constexpr uint16_t COLOR_HOLIDAY = 0xFD20;
    constexpr uint16_t COLOR_ALLDAY = 0x3666;
    constexpr uint16_t COLOR_NORMAL_EVENT = 0x5370;

    // Buffer-Größe für Textkürzung
    constexpr size_t LINE_BUFFER_SIZE = 128;

    /**
     * @brief Übersetzt den vom Backend gelieferten Wochentags-Kurznamen in die aktuelle UI-Sprache
     * @param backendName Deutsche Kurzform aus Backend oder Demo ("So", "Mo", ..., "Sa")
     * @param outBuf Stack-Puffer, in den die EN-Form geschrieben wird (nur bei Sprache != DE genutzt)
     * @param outBufSize Größe des Puffers in Bytes
     * @return Pointer auf den anzuzeigenden String (entweder outBuf oder backendName)
     *
     * Bei UI-Sprache DE wird der Backend-String direkt zurückgegeben (kein Kopieraufwand). Bei EN wird der Index 0..6 über strcmp gegen ein fixes DE-Array bestimmt und dann I18n::copyWeekday() in den Puffer geschrieben.
     */
    const char *localizedDayShort(const char *backendName, char *outBuf, size_t outBufSize)
    {
        if (I18n::currentLang() == LangIndex::DE)
            return backendName;

        static const char *const DE_SHORT[7] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
        for (int i = 0; i < 7; i++)
        {
            if (strcmp(backendName, DE_SHORT[i]) == 0)
            {
                I18n::copyWeekday(outBuf, outBufSize, i, false);
                return outBuf;
            }
        }
        return backendName;
    }
}

// ===================================================================
// KONSTRUKTOR / DESTRUKTOR
// ===================================================================

/**
 * @brief Konstruktor
 */
Calendar::Calendar()
{
    // Heap-Allokierung für CalendarData
    m_currentData = new CalendarData();
    if (m_currentData)
        memset(m_currentData, 0, sizeof(CalendarData));

    // Mit aktueller lokaler Zeit initialisieren
    updateCurrentTime();
}

/**
 * @brief Destruktor
 */
Calendar::~Calendar()
{
    // Graceful Shutdown des Worker-Tasks
    shutdownWorker();

    // Queue leeren bevor löschen - verhindert Memory Leak!
    if (m_dataQueue != nullptr)
    {
        CalendarData *leftover = nullptr;
        while (xQueueReceive(m_dataQueue, &leftover, 0) == pdTRUE)
        {
            if (leftover)
            {
                delete leftover;
                leftover = nullptr;
            }
        }
        vQueueDelete(m_dataQueue);
        m_dataQueue = nullptr;
    }

    // CalendarData freigeben
    delete m_currentData;
    m_currentData = nullptr;

    // Sprites löschen
    if (m_tft)
    {
        m_tft->safeDeleteSprite(m_progressSprite);
        m_tft->safeDeleteSprite(m_upcomingSprite);
        m_tft->safeDeleteSprite(m_scrollbarSprite);
        m_tft->safeDeleteSprite(m_clockSprite);
    }
}

/**
 * @brief Beendet den Worker-Task sauber
 */
void Calendar::shutdownWorker(bool fast)
{
    // Handle lokal speichern
    TaskHandle_t handle = m_taskHandle.load();
    if (!handle)
        return;

    m_shutdownRequested.store(true);

    // Bei aktivem HTTP-Request warten
    if (m_inFlight.load())
    {
        LOG_WARNING("Shutdown während HTTP-Request - warte auf Abschluss...");

        constexpr int MAX_HTTP_WAIT_MS = 20000;
        int waited = 0;

        while (m_inFlight.load() && waited < MAX_HTTP_WAIT_MS)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            waited += 100;
        }

        if (m_inFlight.load())
            LOG_ERROR("HTTP-Timeout überschritten - Force-Shutdown (mögliches Memory Leak!)");
    }

    // Handle nochmal lesen
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
 * @brief Vollredraw nach Sprachwechsel
 *
 * Wird bei einem Sprachwechsel aufgerufen, damit alle statischen Texte mit den neuen Übersetzungen überschrieben werden. Verzweigt nach m_layoutDrawn: vor dem ersten Datensatz wird der Ladescreen neu in der aktuellen Sprache gezeichnet; danach wird die Hauptansicht (Termin-Liste, beide Monatskalender, Buttons, Digitaluhr) komplett neu aufgebaut. Im Demo-Modus werden die Demo-Daten vorher mit der neuen Sprache neu materialisiert, damit die Event-Titel in der gewünschten Sprache erscheinen.
 */
void Calendar::redrawAll()
{
    if (!m_tft)
        return;

    if (!m_layoutDrawn)
    {
        // Ladescreen neu zeichnen
        m_tft->clear(Theme::ColorMainBackGround);
        m_tft->loadFont(LeagueGothicCond34);
        m_tft->setTextColor(COLOR_LOADING_TEXT, Theme::ColorMainBackGround);
        m_tft->setTextDatum(MC_DATUM);
        m_tft->drawString(STR(CalendarKey::CALENDAR_LOADING), m_tft->width() / 2, (m_tft->height() / 2) - 25);
        m_tft->unloadFont();
        m_cancelLoadButton.drawButton(m_cancelLoadButton.isPressed());
        return;
    }

    if (!m_currentData)
        return;

    // Im Demo-Modus: Demo-Daten in der jetzt aktiven Sprache neu erzeugen. Die deterministische Seed-Wahl in generateDemoData() sorgt dafür, dass dieselben Indizes in die DE/EN-Tabellen gewählt werden, sodass der Anwender exakt dieselben Events sieht, nur in der anderen Sprache.
    if (m_demoMode)
        generateDemoData(m_currentData);

    // Sprite-Caches invalidieren, damit drawEvents() das Sprite neu aufbaut
    m_upcomingSpriteValid = false;
    m_lastScrollOffset = -1;
    m_lastDrawnSecond = -1;

    // Benutzeroberfläche mit der bestehenden Geometrie vollständig neu zeichnen
    m_tft->clear(Theme::ColorMainBackGround);

    drawEvents();

    constexpr int FIRST_CALENDAR_Y = 10;
    constexpr int CALENDAR_GAP = 0;
    drawMonthCalendars(m_currentData->today.currentMonth, m_currentData->today.currentYear, FIRST_CALENDAR_Y, true);

    int nextMonth = m_currentData->today.currentMonth + 1;
    int nextYear = m_currentData->today.currentYear;
    if (nextMonth > 12)
    {
        nextMonth = 1;
        nextYear++;
    }

    const int secondCalendarY = FIRST_CALENDAR_Y + calculateMonthCalendarHeight(m_currentData->today.currentMonth, m_currentData->today.currentYear) + CALENDAR_GAP;
    drawMonthCalendars(nextMonth, nextYear, secondCalendarY, false);
    m_exitButton.drawButton(false);

    drawDigitalClock(625, (m_tft->height() - m_clockSprite->height()) + 1);
}

/**
 * @brief Gibt den Modultyp zurück
 */
AppModule Calendar::type() const
{
    return AppModule::CALENDAR;
}

/**
 * @brief Initialisiert das Kalender-Modul
 */
bool Calendar::init(LGFX &tft, const CalendarCallbacks &callbacks)
{
    m_tft = &tft;
    m_callbacks = callbacks;

    // Google Apps Script URL aus AppState laden
    m_appsScriptUrl = APP_STATE.getCalendarScriptUrl();

    // URL validieren: muss vorhanden sein und mit "/exec" enden
    const size_t urlLen = m_appsScriptUrl ? strlen(m_appsScriptUrl) : 0;
    m_demoMode = (urlLen < 5 || strcmp(m_appsScriptUrl + urlLen - 5, "/exec") != 0);

    if (m_demoMode && urlLen > 0)
        LOG_WARNING("Calendar: URL endet nicht mit /exec -> Demo-Modus");
    else if (m_demoMode)
        LOG_INFO("Calendar: Keine API-URL konfiguriert -> Demo-Modus aktiv");
    else
        LOG_INFO_F("Calendar: API-URL geladen (%d Zeichen)", strlen(m_appsScriptUrl));

    // Progressbar-Sprite erstellen
    if (!m_tft->safeCreateSprite(m_progressSprite, 1, m_tft->height()))
    {
        LOG_WARNING("Konnte Progressbar-Sprite nicht erstellen");
        return false;
    }

    // Sprite für Upcoming Events erstellen (440x480)
    if (!m_tft->safeCreateSprite(m_upcomingSprite, 440, 480))
    {
        LOG_WARNING("Konnte Upcoming-Events-Sprite nicht erstellen");
        return false;
    }

    // Scrollbar-Sprite erstellen
    if (!m_tft->safeCreateSprite(m_scrollbarSprite, 5, 480))
    {
        LOG_WARNING("Konnte Scrollbar-Sprite nicht erstellen");
        return false;
    }

    // Sprite für die Digitaluhr erstellen
    if (!m_tft->safeCreateSprite(m_clockSprite, 103, 45))
    {
        LOG_WARNING("Konnte Clock-Sprite nicht erstellen");
        return false;
    }
    m_clockSprite->clear(Theme::ColorMainBackGround);
    m_lastDrawnSecond = -1;

    // Exit-Button initialisieren
    m_exitButton.initButton(
        m_tft,
        EXIT_BUTTON_X,
        EXIT_BUTTON_Y,
        BUTTON_SIZE,
        BUTTON_SIZE,
        BUTTON_RADIUS,
        BUTTON_COLOR,
        TFT_BLACK,
        TFT_DARKGREY,
        "^POWER:32:D206^",
        nullptr);

    m_exitButton.setFont(LeagueGothicCond34);

    // Ladescreen anzeigen
    m_tft->clear(Theme::ColorMainBackGround);
    m_tft->loadFont(LeagueGothicCond34);
    m_tft->setTextColor(COLOR_LOADING_TEXT, Theme::ColorMainBackGround);
    m_tft->setTextDatum(MC_DATUM);
    m_tft->drawString(STR(CalendarKey::CALENDAR_LOADING), m_tft->width() / 2, (m_tft->height() / 2) - 25);

    // Cancel-Button zentriert unter dem Text positionieren
    const int16_t cancelX = m_tft->width() / 2;
    const int16_t cancelY = (m_tft->height() / 2) + 25;

    m_cancelLoadButton.initButton(
        m_tft,
        cancelX,
        cancelY,
        BUTTON_SIZE,
        BUTTON_SIZE,
        BUTTON_RADIUS,
        BUTTON_COLOR,
        TFT_BLACK,
        TFT_DARKGREY,
        "^DEL:32:F800^",
        nullptr);

    m_tft->unloadFont();
    m_cancelLoadButton.drawButton(false);

    m_layoutDrawn = false;

    // Queue für threadsichere Datenübertragung erstellen
    m_dataQueue = xQueueCreate(2, sizeof(CalendarData *));
    if (m_dataQueue == nullptr)
    {
        LOG_ERROR("Konnte Calendar Data Queue nicht erstellen");
        return false;
    }

    // Worker-Task starten
    TaskHandle_t tempHandle = nullptr;
    BaseType_t result = xTaskCreatePinnedToCore(
        &Calendar::TaskEntry,
        "CalendarWorker",
        WORKER_STACK_SIZE,
        this,
        WORKER_PRIORITY,
        &tempHandle,
        WORKER_CORE);

    if (result != pdPASS)
    {
        LOG_ERROR("Konnte Calendar Worker-Task nicht starten");
        vQueueDelete(m_dataQueue);
        m_dataQueue = nullptr;
        return false;
    }

    m_taskHandle.store(tempHandle);

    // Reset für ersten Aufruf
    m_lastApiCall = 0;
    m_consecutiveErrors = 0;
    m_initialized = true;

    LOG_SUCCESS("Kalender-Modul erfolgreich initialisiert");
    return true;
}

// ===================================================================
// HAUPTZEICHENFUNKTION
// ===================================================================

/**
 * @brief Hauptzeichenfunktion - wird zyklisch aufgerufen
 */
void Calendar::draw()
{
    if (!m_initialized || !m_currentData)
        return;

    // Sprachwechsel? -> kompletten Bildschirm neu zeichnen
    if (I18n::consumeLanguageChanged())
        redrawAll();

    // Touch-Handling
    int32_t tx, ty;
    bool is_touched = m_tft->getTouch(&tx, &ty);

    if (!m_layoutDrawn)
    {
        // LadeScreen Cancel-Button behandeln
        if (m_cancelLoadButton.isPressed() && !is_touched)
        {
            m_cancelLoadButton.press(false);

            // Benutzer über Abbruch informieren
            const int textY = (m_tft->height() / 2) - 25;
            m_tft->fillRect(0, textY - 20, m_tft->width(), 40, Theme::ColorMainBackGround);
            m_tft->loadFont(LeagueGothicCond34);
            m_tft->setTextColor(COLOR_LOADING_TEXT, Theme::ColorMainBackGround);
            m_tft->setTextDatum(MC_DATUM);
            m_tft->drawString(STR(CalendarKey::CALENDAR_LOADING_CANCELLED), m_tft->width() / 2, textY);
            m_tft->unloadFont();

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
    else
    {
        // Hauptansicht
        bool touch_on_exit_button = false;

        // Exit-Button behandeln
        if (m_exitButton.isPressed() && !is_touched)
        {
            m_exitButton.press(false);
            shutdownWorker(true);

            if (m_callbacks.onExitRequested)
                m_callbacks.onExitRequested();

            return;
        }
        if (is_touched && m_exitButton.contains(tx, ty))
        {
            touch_on_exit_button = true;

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

        // Scroll-Logik (nur ausführen, wenn nicht der Exit-Button berührt wird)
        if (is_touched && !touch_on_exit_button)
            handleScrollTouch(tx, ty, is_touched);
        else if (!is_touched && m_scrollState.isDragging)
            handleScrollTouch(0, 0, false);
    }

    // Scroll-Physics aktualisieren
    updateScrollPhysics();

    // Prüfe auf neue Daten vom Worker-Task
    CalendarData *newDataPtr = nullptr;
    if (xQueueReceive(m_dataQueue, &newDataPtr, 0) == pdTRUE)
    {
        // Speicher des alten Datensatzes freigeben
        delete m_currentData;

        // Neuen Pointer als aktuellen Datensatz übernehmen
        m_currentData = newDataPtr;
        m_hasValidData = true;

        // UI-Zustand für komplettes Neuzeichnen zurücksetzen
        m_scrollState.reset();
        m_upcomingSpriteValid = false;
        m_lastScrollOffset = -1;

        // UI komplett neu zeichnen
        m_tft->clear(Theme::ColorMainBackGround);

        if (!m_layoutDrawn)
            m_layoutDrawn = true;

        drawEvents();
        constexpr int FIRST_CALENDAR_Y = 10;
        constexpr int CALENDAR_GAP = 0; ///< Abstand zwischen erstem und zweitem Kalender
        drawMonthCalendars(m_currentData->today.currentMonth, m_currentData->today.currentYear, FIRST_CALENDAR_Y, true);

        int nextMonth = m_currentData->today.currentMonth + 1;
        int nextYear = m_currentData->today.currentYear;
        if (nextMonth > 12)
        {
            nextMonth = 1;
            nextYear++;
        }

        const int secondCalendarY = FIRST_CALENDAR_Y + calculateMonthCalendarHeight(m_currentData->today.currentMonth, m_currentData->today.currentYear) + CALENDAR_GAP;
        drawMonthCalendars(nextMonth, nextYear, secondCalendarY, false);
        m_exitButton.drawButton(false);

        // Digitaluhr unter die Monatskalendern zeichnen
        drawDigitalClock(625, (m_tft->height() - m_clockSprite->height()) + 1);

        LOG_INFO("UI mit neuen Kalenderdaten aktualisiert");
    }

    // Bei Scroll-Bewegung: nur den scrollbaren Bereich neu zeichnen
    if (m_hasValidData && (m_scrollState.isDragging || std::abs(m_scrollState.scrollVelocity) > 0.5f || m_scrollState.isSnappingBack))
        drawEvents();

    // Periodische Aktualisierung triggern
    const bool intervalDue = (m_lastApiCall == 0) || (millis() - m_lastApiCall > API_CALL_INTERVAL);
    const bool canTrigger = !m_refreshRequested.load() && !m_inFlight.load();

    if (intervalDue && canTrigger)
    {
        requestRefresh(false);
        m_lastApiCall = millis();
    }

    // Prüfen, ob Worker-Task läuft, ggf. neu starten
    if (m_taskHandle.load() == nullptr && !m_shutdownRequested.load())
    {
        LOG_WARNING("Worker-Task nicht aktiv - starte neu");

        // Fehlerzähler/Backoff zurücksetzen
        m_consecutiveErrors = 0;
        m_backoffMs = 0;

        TaskHandle_t tempHandle = nullptr;
        BaseType_t result = xTaskCreatePinnedToCore(
            &Calendar::TaskEntry,
            "CalendarWorker",
            WORKER_STACK_SIZE,
            this,
            WORKER_PRIORITY,
            &tempHandle,
            WORKER_CORE);

        if (result == pdPASS)
        {
            m_taskHandle.store(tempHandle);
            LOG_SUCCESS("Worker-Task erfolgreich neu gestartet");
            // Sofortiger Refresh
            m_refreshRequested.store(true);
            xTaskNotifyGive(tempHandle);
        }
        else
        {
            LOG_ERROR("Konnte Worker-Task nicht neu starten!");
        }
    }

    // Zustandsabhängiger Watchdog (busy = 45s, idle = 90s)
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

            // Zustände defensiv zurücksetzen
            m_workerLastSeen = 0;
            m_inFlight.store(false);
            m_refreshRequested.store(false);
            m_shutdownRequested.store(false);
        }
    }

    // Progressbar und Digitaluhr sekündlich zeichnen (nur bei gültigen Daten)
    if (m_hasValidData && millis() - m_lastProgressUpdate > 1000)
    {
        m_lastProgressUpdate = millis();

        if (m_progressSprite)
        {
            unsigned long elapsedTime = millis() - m_lastApiCall;
            float progress = 1.0f - (static_cast<float>(elapsedTime) / static_cast<float>(API_CALL_INTERVAL));
            progress = constrain(progress, 0.0f, 1.0f);

            int barHeight = static_cast<int>(m_tft->height() * progress);
            int startY = m_tft->height() - barHeight;

            // Progressbar zeichnen
            m_progressSprite->drawFastVLine(0, 0, startY, Theme::ColorMainBackGround);
            m_progressSprite->drawFastVLine(0, startY, barHeight, COLOR_PROGRESS_BAR);

            // Sprite ganz rechts positionieren
            m_progressSprite->pushSprite(m_tft->width() - 1, 0);
        }

        // Digitaluhr aktualisieren
        drawDigitalClock(625, (m_tft->height() - m_clockSprite->height()) + 1);
    }
}

/**
 * @brief Fordert einen Refresh der Kalenderdaten an
 */
void Calendar::requestRefresh(bool force)
{
    if (!m_initialized || m_shutdownRequested.load())
        return;

    if (!force && millis() < m_nextAllowedFetch.load())
    {
        // Log-Spam vermeiden
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
void Calendar::TaskEntry(void *arg)
{
    auto *self = static_cast<Calendar *>(arg);
    if (self)
        self->workerLoop();
}

/**
 * @brief Haupt-Arbeitsschleife des Worker-Tasks
 */
void Calendar::workerLoop()
{
    LOG_INFO("Calendar Worker-Task gestartet");

    constexpr TickType_t WAIT_TICKS = pdMS_TO_TICKS(30000); // 30 s warten auf Refresh/Signal

    while (!m_shutdownRequested.load())
    {
        // Bis zu 30 s auf ein Signal warten (Refresh / Shutdown)
        ulTaskNotifyTake(pdTRUE, WAIT_TICKS);

        // Heartbeat nach dem Warten setzen (wichtig für den Watchdog)
        m_workerLastSeen = millis();

        // Shutdown-Check
        if (m_shutdownRequested.load())
        {
            LOG_INFO("Shutdown-Signal empfangen - beende Worker graceful");
            break;
        }

        // Wurde wirklich ein Refresh angefordert?
        if (!m_refreshRequested.exchange(false))
        {
            // Nein -> nächste Runde (weiter warten). Heartbeat ist frisch.
            continue;
        }

        // Jetzt arbeiten: Daten holen
        m_inFlight.store(true);
        LOG_INFO("Worker: Starte Datenabruf");

        // Heap-Objekt für jeden neuen Datensatz erstellen
        CalendarData *fetchedData = new CalendarData();
        if (!fetchedData)
        {
            LOG_ERROR("Konnte CalendarData nicht allokieren, überspringe diesen Abruf");
            m_inFlight.store(false);
            vTaskDelay(pdMS_TO_TICKS(5000)); // Kurz warten vor nächstem Versuch
            continue;
        }

        const int httpCode = fetchCalendarDataCore(fetchedData);

        if (m_shutdownRequested.load())
        {
            delete fetchedData; // Aufräumen, da Daten nicht übergeben werden
            m_inFlight.store(false);
            break;
        }

        if (httpCode == HTTP_CODE_OK)
        {
            // Ownership-Übergabe an UI-Thread via Queue
            if (xQueueSend(m_dataQueue, &fetchedData, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                LOG_INFO("Kalenderdaten-Pointer erfolgreich an UI übertragen");
                fetchedData = nullptr; // Ownership liegt jetzt beim UI
            }
            else
            {
                LOG_WARNING("Queue voll - UI-Update verpasst. Lösche ungenutzte Daten");
                delete fetchedData; // Worker behält Ownership und muss aufräumen
            }

            // Fehlerzähler/Backoff zurücksetzen
            m_consecutiveErrors = 0;
            m_backoffMs = 0;
            m_nextAllowedFetch.store(millis());
        }
        else
        {
            // Fehler: HTTP-Request fehlgeschlagen. Wir müssen selbst aufräumen.
            delete fetchedData;

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

                if (m_backoffMs == 0)
                    m_backoffMs = MIN_RETRY_INTERVAL;
                else
                    m_backoffMs = min(m_backoffMs * 2, MAX_BACKOFF_TIME);

                m_nextAllowedFetch.store(millis() + m_backoffMs);

                LOG_WARNING_F("Worker: Fehler %d/%d, Backoff: %lu ms (HTTP: %d)", m_consecutiveErrors, MAX_CONSECUTIVE_ERRORS, m_backoffMs, httpCode);

                if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
                {
                    LOG_WARNING("Zu viele transiente Fehler - wechsle in Demo-Modus");
                    m_demoMode = true;
                }
            }

            // Demo-Modus aktiviert -> sofort Demo-Daten an UI senden
            if (m_demoMode)
            {
                CalendarData *demoData = new CalendarData();
                if (demoData)
                {
                    memset(demoData, 0, sizeof(CalendarData));
                    generateDemoData(demoData);

                    if (xQueueSend(m_dataQueue, &demoData, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        LOG_INFO("Demo-Daten nach API-Fehler an UI übertragen");
                        demoData = nullptr;
                    }
                    else
                    {
                        delete demoData;
                    }
                }

                m_consecutiveErrors = 0;
                m_backoffMs = 0;
            }
        }

        // Arbeit vorbei: Status & Heartbeat setzen
        m_inFlight.store(false);
        m_workerLastSeen = millis();
    }

    LOG_SUCCESS("Calendar Worker-Task beendet sich graceful");

    m_taskHandle.store(nullptr);
    vTaskDelete(nullptr);
}

// ===================================================================
// API-FUNKTIONEN
// ===================================================================

/**
 * @brief Core-Funktion für HTTP-Datenabruf
 */
int Calendar::fetchCalendarDataCore(CalendarData *outData)
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

        LOG_INFO("Demo-Modus aktiv - fiktive Kalender-Events generiert");
        return HTTP_CODE_OK;
    }

    // ===================================================================
    // ECHTER API-MODUS MIT HTTP_SERVICE
    // ===================================================================

    // Früher Shutdown-Check
    if (m_shutdownRequested.load())
    {
        LOG_INFO("HTTP-Request abgebrochen - Shutdown angefordert");
        return ERR_SHUTDOWN;
    }

    // Vorbedingungen prüfen
    if (!m_appsScriptUrl || strlen(m_appsScriptUrl) == 0)
    {
        LOG_WARNING("Calendar: Apps Script URL ist leer");
        return -1;
    }

    if (!HTTP_SERVICE.isInitialized())
    {
        LOG_ERROR("Calendar: HttpService nicht initialisiert!");
        return ERR_HTTP_NOT_INIT;
    }

    if (!APP_STATE.getWifiConnected())
    {
        LOG_WARNING("Calendar: Kein WiFi verfügbar");
        return -2;
    }

    // Ausgabestruktur initialisieren
    memset(outData, 0, sizeof(CalendarData));

    // Aktuelles Datum thread-sicher ins Ziel schreiben
    fillCurrentTime(outData->today);
    outData->timestamp = millis();

    LOG_INFO("Sende Anfrage an Google Apps Script...");

    // ===================================================================
    // HTTP REQUEST ÜBER ZENTRALEN SERVICE
    // ===================================================================

    HttpResponse response = HTTP_SERVICE.get(m_appsScriptUrl, &m_shutdownRequested);

    LOG_INFO_F("HTTP Response nach %lu ms, Code: %d", response.requestTimeMs, response.httpCode);

    // Shutdown-Check nach HTTP-Request
    if (m_shutdownRequested.load())
    {
        LOG_INFO("HTTP-Request abgeschlossen aber Shutdown angefordert");
        return ERR_SHUTDOWN;
    }

    if (!response.isSuccess())
    {
        if (response.httpCode == -200)
            return ERR_SHUTDOWN;

        LOG_ERROR_F("HTTP-Fehler: %d", response.httpCode);
        return response.httpCode;
    }

    if (!response.hasData())
    {
        LOG_ERROR("Leere Antwort vom Server!");
        return ERR_EMPTY_RESPONSE;
    }

    LOG_INFO_F("Payload empfangen: %u bytes", response.length);

    // Shutdown-Check vor JSON-Parsing
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

    bool success = false;

    if (!error && (!doc["today"].is<JsonObject>() || !doc["upcomingEvents"]["days"].is<JsonArray>() || !doc["currentMonth"]["daysWithEvents"].is<JsonArray>() || !doc["nextMonth"]["daysWithEvents"].is<JsonArray>() || !doc["error"].isNull()))
    {
        LOG_ERROR("Erforderliche Kalenderstruktur fehlt in der Server-Antwort");
        return ERR_SCHEMA;
    }

    if (!error)
    {
        // Nur Datum vom Server übernehmen
        JsonObject today = doc["today"];
        if (today)
        {
            outData->today.currentDay = today["day"] | outData->today.currentDay;
            outData->today.currentMonth = today["month"] | outData->today.currentMonth;
            outData->today.currentYear = today["year"] | outData->today.currentYear;
            LOG_INFO("Server-Datum übernommen");
        }

        // Upcoming Events verarbeiten
        JsonObject upcoming = doc["upcomingEvents"];
        if (upcoming)
        {
            JsonArray days = upcoming["days"];
            outData->upcomingEvents.dayCount = 0;

            // Serverwert lesen, aber lokal deckeln
            int serverTotal = upcoming["totalEventCount"] | 0;
            outData->upcomingEvents.totalEventCount = min(serverTotal, MAX_UPCOMING_EVENTS);
            int totalAdded = 0;

            if (days)
            {
                for (JsonObject day : days)
                {
                    if (outData->upcomingEvents.dayCount >= MAX_UPCOMING_DAYS)
                        break;

                    if (totalAdded >= MAX_UPCOMING_EVENTS)
                        break;

                    DayWithEvents &dayData = outData->upcomingEvents.days[outData->upcomingEvents.dayCount];

                    // Tag-Informationen
                    const char *date = day["date"] | "";
                    const char *dayName = day["dayName"] | "";
                    stringutils::safeCopy(dayData.date, date);
                    stringutils::safeCopy(dayData.dayName, dayName);
                    dayData.dayOfMonth = day["dayOfMonth"] | 0;
                    dayData.month = day["month"] | 0;
                    dayData.year = day["year"] | 0;
                    dayData.isToday = day["isToday"] | false;

                    // Events des Tages
                    dayData.eventCount = 0;
                    JsonArray dayEvents = day["events"];
                    if (dayEvents)
                    {
                        for (JsonObject event : dayEvents)
                        {
                            if (dayData.eventCount >= MAX_EVENTS_PER_DAY)
                                break;

                            if (totalAdded >= MAX_UPCOMING_EVENTS)
                                break;

                            const char *start = event["start"];
                            const char *summary = event["summary"];
                            const char *type = event["eventType"] | "default";

                            if (!isValidEventData(start, summary))
                                continue;

                            CalendarEvent &evt = dayData.events[dayData.eventCount];
                            stringutils::safeCopy(evt.startTime, start);
                            stringutils::safeCopy(evt.summary, summary);
                            stringutils::safeCopy(evt.eventType, type);
                            evt.isAllDay = event["isAllDay"] | false;
                            evt.isPast = event["isPast"] | false;

                            dayData.eventCount++;
                            totalAdded++;
                        }
                    }

                    // Nur Tage mit verbleibenden Events zählen
                    if (dayData.eventCount > 0)
                        outData->upcomingEvents.dayCount++;

                    if (totalAdded >= MAX_UPCOMING_EVENTS)
                        break;
                }
            }

            LOG_INFO_F("Kommende Termine: %d Tage mit %d Terminen geladen", outData->upcomingEvents.dayCount, totalAdded);
        }

        // Aktueller Monat verarbeiten
        JsonObject currentMonth = doc["currentMonth"];
        if (currentMonth)
        {
            JsonArray daysWithEvents = currentMonth["daysWithEvents"];
            if (daysWithEvents)
            {
                for (int day : daysWithEvents)
                {
                    if (day >= 1 && day <= 31)
                        outData->currentMonth.hasEvent[day] = true;
                }
            }
        }

        // Nächster Monat verarbeiten
        JsonObject nextMonth = doc["nextMonth"];
        if (nextMonth)
        {
            outData->nextMonth.month = nextMonth["month"] | 0;
            outData->nextMonth.year = nextMonth["year"] | 0;
            JsonArray daysWithEvents = nextMonth["daysWithEvents"];

            if (daysWithEvents)
            {
                for (int day : daysWithEvents)
                {
                    if (day >= 1 && day <= 31)
                        outData->nextMonth.hasEvent[day] = true;
                }
            }
        }

        success = !m_shutdownRequested.load();
        if (success)
        {
            LOG_SUCCESS("Kalenderdaten erfolgreich verarbeitet");
        }
    }
    else
    {
        LOG_ERROR_F("JSON Parse Error: %s", error.c_str());

        switch (error.code())
        {
        case DeserializationError::InvalidInput:
            LOG_ERROR("Response ist kein gültiges JSON");

            if (response.length > 0)
                LOG_INFO_F("Response-Anfang: %.200s", response.data);

            break;

        case DeserializationError::NoMemory:
            LOG_ERROR_F("JSON zu groß - PSRAM sollte genug haben!");
            break;

        default:
            LOG_ERROR_F("JSON-Fehler: %d", static_cast<int>(error.code()));
        }

        return ERR_JSON_PARSE;
    }

    return success ? HTTP_CODE_OK : ERR_SHUTDOWN;
}

// ===================================================================
// DEMO-DATEN GENERIERUNG
// ===================================================================

/**
 * @brief Generiert fiktive Demo-Daten für Präsentationen
 */
void Calendar::generateDemoData(CalendarData *outData)
{
    // Wochentags-Abkürzungen (sprachneutral im dayName, wird beim Rendern lokalisiert)
    static constexpr const char *const WOCHENTAGE[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};

    // Demo-Event-Titel - realistische Termine (NEUTRAL - keine Feiertage!)
    static constexpr const char *const DEMO_EVENTS_TIMED_DE[] = {
        "Team-Meeting",
        "Projektbesprechung",
        "Kundentermin",
        "Zahnarzt",
        "Arzttermin",
        "Elternabend",
        "Yoga-Kurs",
        "Friseur",
        "Handwerker",
        "Paketabholung",
        "Videocall",
        "Mittagessen",
        "Steuerberater",
        "Sport-Training",
        "Online-Kurs",
        "Telefonkonferenz",
        "Bewerbungsgespräch",
        "Vorstellungsgespräch"};
    static constexpr const char *const DEMO_EVENTS_TIMED_EN[] = {
        "Team meeting",
        "Project review",
        "Client meeting",
        "Dentist appointment",
        "Doctor's appointment",
        "Parents' evening",
        "Yoga class",
        "Hairdresser",
        "Tradesperson visit",
        "Parcel pickup",
        "Video call",
        "Lunch",
        "Accountant",
        "Fitness training",
        "Online course",
        "Conference call",
        "Job interview",
        "Interview"};
    static constexpr int DEMO_EVENTS_TIMED_COUNT = 18;

    // Ganztägige Events (NEUTRAL - keine Feiertage!)
    static constexpr const char *const DEMO_EVENTS_ALLDAY_DE[] = {
        "Urlaub",
        "Homeoffice",
        "Dienstreise",
        "Messe",
        "Fortbildung",
        "Seminar",
        "Workshop",
        "Konferenz",
        "Betriebsausflug",
        "Inventur"};
    static constexpr const char *const DEMO_EVENTS_ALLDAY_EN[] = {
        "Holiday",
        "Working from home",
        "Business trip",
        "Trade fair",
        "Training course",
        "Seminar",
        "Workshop",
        "Conference",
        "Team outing",
        "Stocktaking"};
    static constexpr int DEMO_EVENTS_ALLDAY_COUNT = 10;

    // Geburtstage
    static constexpr const char *const DEMO_BIRTHDAYS_DE[] = {
        "Geburtstag von Max",
        "Geburtstag von Anna",
        "Geburtstag von Felix",
        "Geburtstag von Laura",
        "Geburtstag von Paul",
        "Geburtstag von Sophie",
        "Geburtstag von Leon",
        "Geburtstag von Marie"};
    static constexpr const char *const DEMO_BIRTHDAYS_EN[] = {
        "Max's birthday",
        "Anna's birthday",
        "Felix's birthday",
        "Laura's birthday",
        "Paul's birthday",
        "Sophie's birthday",
        "Leon's birthday",
        "Marie's birthday"};
    static constexpr int DEMO_BIRTHDAYS_COUNT = 8;

    // Sprachabhängige Pointer-Auswahl (Inhalte sind in beiden Tabellen positionsidentisch)
    const bool en = (I18n::currentLang() == LangIndex::EN);
    const char *const *DEMO_EVENTS_TIMED  = en ? DEMO_EVENTS_TIMED_EN  : DEMO_EVENTS_TIMED_DE;
    const char *const *DEMO_EVENTS_ALLDAY = en ? DEMO_EVENTS_ALLDAY_EN : DEMO_EVENTS_ALLDAY_DE;
    const char *const *DEMO_BIRTHDAYS    = en ? DEMO_BIRTHDAYS_EN    : DEMO_BIRTHDAYS_DE;

    // Uhrzeiten für normale Termine
    static constexpr const char *const DEMO_TIMES[] = {
        "08:00", "08:30", "09:00", "09:30", "10:00", "10:30",
        "11:00", "11:30", "12:00", "13:00", "14:00", "14:30",
        "15:00", "15:30", "16:00", "16:30", "17:00", "18:00",
        "18:30", "19:00", "19:30", "20:00"};
    static constexpr int DEMO_TIMES_COUNT = 22;

    // Aktuelles Datum thread-sicher holen
    fillCurrentTime(outData->today);

    const int currentYear = outData->today.currentYear;
    const int currentMonth = outData->today.currentMonth;
    const int currentDay = outData->today.currentDay;

    outData->timestamp = millis();

    // Nächster Monat berechnen
    int nextMonth = currentMonth + 1;
    int nextYear = currentYear;
    if (nextMonth > 12)
    {
        nextMonth = 1;
        nextYear++;
    }

    outData->currentMonth.month = currentMonth;
    outData->currentMonth.year = currentYear;
    outData->nextMonth.month = nextMonth;
    outData->nextMonth.year = nextYear;

    // Initialisiere hasEvent-Arrays
    memset(outData->currentMonth.hasEvent, 0, sizeof(outData->currentMonth.hasEvent));
    memset(outData->nextMonth.hasEvent, 0, sizeof(outData->nextMonth.hasEvent));

    // Upcoming Events generieren
    outData->upcomingEvents.dayCount = 0;
    outData->upcomingEvents.totalEventCount = 0;

    // Hilfsfunktion um Tage im Monat zu berechnen
    auto daysInMonth = [](int m, int y) -> int
    {
        if (m == 2)
            return (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 29 : 28;
        else if (m == 4 || m == 6 || m == 9 || m == 11)
            return 30;
        return 31;
    };

    // Hilfsfunktion für Wochentag (0=So, 1=Mo, ..., 6=Sa)
    auto getWeekday = [](int day, int month, int year) -> int
    {
        struct tm t = {0, 0, 0, day, month - 1, year - 1900, 0, 0, 0};
        time_t time_temp = mktime(&t);
        struct tm const *time_out = localtime(&time_temp);
        return time_out->tm_wday;
    };

    // Seed für Pseudo-Zufallsgenerator basierend auf aktuellem Datum
    unsigned int seed = static_cast<unsigned int>(currentYear * 10000 + currentMonth * 100 + currentDay);

    // Einfacher LCG für deterministische "Zufalls"-Zahlen
    auto nextRandom = [&seed]() -> unsigned int
    {
        seed = seed * 1103515245 + 12345;
        return (seed / 65536) % 32768;
    };

    // Aktuelle Stunde für "vergangene" Termine
    struct tm timeinfo = DS1307_RTC::getCurrentLocalTime();
    const int currentHour = timeinfo.tm_hour;

    int totalEvents = 0;
    int day = currentDay;
    int month = currentMonth;
    int year = currentYear;

    // Generiere Events für die nächsten MAX_UPCOMING_DAYS Tage
    for (int dayOffset = 0; dayOffset < MAX_UPCOMING_DAYS && totalEvents < MAX_UPCOMING_EVENTS; dayOffset++)
    {
        // Aktuellen Tag berechnen
        int daysThisMonth = daysInMonth(month, year);
        if (day > daysThisMonth)
        {
            day = 1;
            month++;
            if (month > 12)
            {
                month = 1;
                year++;
            }
        }

        DayWithEvents &dayData = outData->upcomingEvents.days[outData->upcomingEvents.dayCount];
        memset(&dayData, 0, sizeof(DayWithEvents));

        // Datum formatieren
        stringutils::safeFormat(dayData.date, "%04d-%02d-%02d", year, month, day);

        // Wochentag
        int weekday = getWeekday(day, month, year);
        stringutils::safeCopy(dayData.dayName, WOCHENTAGE[weekday]);

        dayData.dayOfMonth = day;
        dayData.month = month;
        dayData.year = year;
        dayData.isToday = (dayOffset == 0);
        dayData.eventCount = 0;

        // Anzahl Events für diesen Tag bestimmen (0-3)
        // Wochentage (Mo-Fr) haben höhere Chance
        int eventChance = (weekday >= 1 && weekday <= 5) ? 70 : 40;
        int numEvents = 0;

        // Heute immer mindestens 1 Event
        if (dayOffset == 0)
        {
            numEvents = 1 + static_cast<int>(nextRandom() % 3); // 1-3 Events heute
        }
        else if (static_cast<int>(nextRandom() % 100) < eventChance)
        {
            numEvents = 1 + static_cast<int>(nextRandom() % 3); // 1-3 Events
        }

        // Events hinzufügen
        for (int e = 0; e < numEvents && dayData.eventCount < MAX_EVENTS_PER_DAY && totalEvents < MAX_UPCOMING_EVENTS; e++)
        {
            CalendarEvent &evt = dayData.events[dayData.eventCount];
            memset(&evt, 0, sizeof(CalendarEvent));

            // Event-Typ bestimmen
            int typeRoll = static_cast<int>(nextRandom() % 100);

            if (typeRoll < 15)
            {
                // 15% Geburtstag (ganztägig)
                stringutils::safeCopy(evt.eventType, "birthday");
                stringutils::safeCopy(evt.startTime, "Ganztagig");
                stringutils::safeCopy(evt.summary, DEMO_BIRTHDAYS[nextRandom() % DEMO_BIRTHDAYS_COUNT]);
                evt.isAllDay = true;
                evt.isPast = false;
            }
            else if (typeRoll < 30)
            {
                // 15% ganztägiger normaler Termin
                stringutils::safeCopy(evt.eventType, "default");
                stringutils::safeCopy(evt.startTime, "Ganztagig");
                stringutils::safeCopy(evt.summary, DEMO_EVENTS_ALLDAY[nextRandom() % DEMO_EVENTS_ALLDAY_COUNT]);
                evt.isAllDay = true;
                evt.isPast = false;
            }
            else
            {
                // 70% normaler Termin mit Uhrzeit
                stringutils::safeCopy(evt.eventType, "default");
                stringutils::safeCopy(evt.startTime, DEMO_TIMES[nextRandom() % DEMO_TIMES_COUNT]);
                stringutils::safeCopy(evt.summary, DEMO_EVENTS_TIMED[nextRandom() % DEMO_EVENTS_TIMED_COUNT]);
                evt.isAllDay = false;

                // Termine heute vor aktueller Uhrzeit als "vergangen" markieren
                if (dayOffset == 0)
                {
                    int evtHour = (evt.startTime[0] - '0') * 10 + (evt.startTime[1] - '0');
                    evt.isPast = (evtHour < currentHour);
                }
                else
                {
                    evt.isPast = false;
                }
            }

            dayData.eventCount++;
            totalEvents++;
        }

        // Tag nur hinzufügen wenn Events vorhanden (außer heute - immer anzeigen)
        if (dayData.eventCount > 0 || dayOffset == 0)
        {
            // hasEvent-Markierung für Monatskalender setzen
            if (month == currentMonth && year == currentYear)
            {
                outData->currentMonth.hasEvent[day] = (dayData.eventCount > 0);
            }
            else if (month == nextMonth && year == nextYear)
            {
                outData->nextMonth.hasEvent[day] = (dayData.eventCount > 0);
            }

            outData->upcomingEvents.dayCount++;
        }

        // Zum nächsten Tag
        day++;
    }

    outData->upcomingEvents.totalEventCount = totalEvents;

    // Zusätzliche Event-Markierungen für den Monatskalender generieren
    for (int d = 1; d <= daysInMonth(currentMonth, currentYear); d++)
    {
        if (!outData->currentMonth.hasEvent[d] && (static_cast<int>(nextRandom() % 100) < 30))
        {
            outData->currentMonth.hasEvent[d] = true;
        }
    }

    for (int d = 1; d <= daysInMonth(nextMonth, nextYear); d++)
    {
        if (!outData->nextMonth.hasEvent[d] && (static_cast<int>(nextRandom() % 100) < 25))
        {
            outData->nextMonth.hasEvent[d] = true;
        }
    }

    LOG_INFO_F("Demo-Daten generiert: %d Tage, %d Events", outData->upcomingEvents.dayCount, totalEvents);
}

// ===================================================================
// HILFSFUNKTIONEN
// ===================================================================

/**
 * @brief Füllt Datumsdaten thread-sicher direkt ins Ziel
 */
void Calendar::fillCurrentTime(TodayData &out)
{
    struct tm timeinfo = DS1307_RTC::getCurrentLocalTime();
    out.currentYear = timeinfo.tm_year + 1900;
    out.currentMonth = timeinfo.tm_mon + 1;
    out.currentDay = timeinfo.tm_mday;
}

/**
 * @brief Aktualisiert das aktuelle Datum von der RTC (nur UI-Thread!)
 */
void Calendar::updateCurrentTime()
{
    if (!m_currentData)
        return;

    fillCurrentTime(m_currentData->today);
}

/**
 * @brief Validiert empfangene Event-Daten
 */
bool Calendar::isValidEventData(const char *start, const char *summary) const
{
    if (!start || !summary)
        return false;

    if (start[0] == '\0' || summary[0] == '\0')
        return false;

    if (strlen(start) >= sizeof(CalendarEvent::startTime))
        return false;

    return true;
}

/**
 * @brief Bestimmt die Farbe basierend auf Event-Typ
 */
uint16_t Calendar::getEventColor(const char *eventType, bool isAllDay) const
{
    if (strcmp(eventType, "birthday") == 0)
        return COLOR_BIRTHDAY; // Rot für Geburtstage
    else if (strcmp(eventType, "holiday") == 0)
        return COLOR_HOLIDAY; // Orange für Feiertage
    else if (isAllDay)
        return COLOR_ALLDAY; // Grün für ganztägige Termine
    else
        return COLOR_NORMAL_EVENT; // Blau für normale Termine
}

// ===================================================================
// UI-ZEICHENFUNKTIONEN
// ===================================================================

/**
 * @brief Zeichnet die kommenden Termine mit Scroll-Funktionalität
 */
void Calendar::drawEvents()
{
    if (!m_currentData)
        return;

    // Bereich für Upcoming Events
    const int leftMargin = 20;
    const int rightMargin = 440;
    const int viewStartY = 0;
    const int viewHeight = 480;

    // Buffer für gekürzte Texte
    char truncatedSummary[68]; // 64 + Platz für "..."

    // Verfügbare Breite für Text berechnen
    const int eventBarWidth = 4;
    const int eventBarMargin = 10;
    const int availableWidth = rightMargin - (leftMargin + eventBarMargin + eventBarWidth + 8) - 10;

    // Berechne Content-Höhe und wende finale Begrenzung an
    m_scrollState.contentHeight = calculateContentHeight();
    m_scrollState.maxScroll = max(0, m_scrollState.contentHeight - viewHeight);

    constrainScroll(); // Finale Sicherung vor dem Zeichnen

    // Prüfe ob Neuzeichnen nötig ist
    bool needsRedraw = !m_upcomingSpriteValid || (m_lastScrollOffset != m_scrollState.scrollOffset) || !m_layoutDrawn;

    if (!needsRedraw)
        return; // Nichts zu tun - verhindert Flackern!

    // SPRITE-BASIERTES ZEICHNEN

    m_upcomingSprite->clear(Theme::ColorMainBackGround);
    m_upcomingSprite->setTextDatum(TL_DATUM);

    // Events in Sprite zeichnen
    int yPos = -m_scrollState.scrollOffset; // Negativ für Scroll-Offset
    const int lineSpacing = 3;

    m_upcomingSprite->loadFont(ComfortaaBold16);

    int safeDayCount = min(m_currentData->upcomingEvents.dayCount, MAX_UPCOMING_DAYS);

    for (int dayIdx = 0; dayIdx < safeDayCount; dayIdx++)
    {
        const DayWithEvents &day = m_currentData->upcomingEvents.days[dayIdx];

        // Skip wenn komplett außerhalb
        if (yPos > viewHeight + 100)
            break;

        // Trennlinie
        if (dayIdx > 0)
        {
            yPos += lineSpacing;
            if (yPos >= -10 && yPos <= viewHeight)
                m_upcomingSprite->drawFastHLine(leftMargin, yPos, rightMargin - leftMargin, 0x4208);

            yPos += lineSpacing;
        }

        // Datum-Header
        yPos += 8;
        if (yPos >= -30 && yPos <= viewHeight)
        {
            m_upcomingSprite->setTextColor(0x9CF3, Theme::ColorMainBackGround);

            char dateHeader[64];
            char monBuf[16];
            char wdBuf[8];
            I18n::copyMonth(monBuf, sizeof(monBuf), day.month - 1, /*longForm=*/false);
            const char *wd = localizedDayShort(day.dayName, wdBuf, sizeof(wdBuf));

            if (day.isToday)
                STRF(dateHeader, CalendarKey::CALENDAR_DATE_HEADER_TODAY_FMT, wd, monBuf, day.dayOfMonth);
            else
                STRF(dateHeader, CalendarKey::CALENDAR_DATE_HEADER_FMT, wd, monBuf, day.dayOfMonth);

            m_upcomingSprite->drawString(dateHeader, leftMargin, yPos);
        }
        yPos += 22;

        int safeEventCount = min(day.eventCount, MAX_EVENTS_PER_DAY);

        // Events
        if (safeEventCount == 0)
        {
            if (yPos >= -20 && yPos <= viewHeight)
            {
                m_upcomingSprite->setTextColor(TFT_DARKGREY, Theme::ColorMainBackGround);
                const __FlashStringHelper *noEventsText = day.isToday
                    ? STR(CalendarKey::CALENDAR_NO_EVENTS_TODAY)
                    : STR(CalendarKey::CALENDAR_NO_EVENTS);
                m_upcomingSprite->drawString(noEventsText, leftMargin + eventBarMargin + eventBarWidth + 5, yPos);
            }
            yPos += 22;
        }
        else
        {
            for (int eventIdx = 0; eventIdx < safeEventCount; eventIdx++)
            {
                if (yPos >= -25 && yPos <= viewHeight)
                {
                    const CalendarEvent &event = day.events[eventIdx];

                    // Farbiger Balken
                    uint16_t barColor = getEventColor(event.eventType, event.isAllDay);
                    m_upcomingSprite->fillRect(leftMargin + eventBarMargin, yPos + 3, eventBarWidth, 15, barColor);

                    // Event-Text
                    uint16_t textColor = event.isPast ? TFT_DARKGREY : 0xD69A;
                    m_upcomingSprite->setTextColor(textColor, Theme::ColorMainBackGround);

                    int textX = leftMargin + eventBarMargin + eventBarWidth + 8;

                    if (event.isAllDay || strcmp(event.startTime, "Ganztagig") == 0)
                    {
                        // Ganztägige Events - volle Breite verfügbar
                        truncateTextToWidth(event.summary, availableWidth, m_upcomingSprite, truncatedSummary, sizeof(truncatedSummary));
                        m_upcomingSprite->drawString(truncatedSummary, textX, yPos);
                    }
                    else
                    {
                        // Zeit in einer anderen Farbe zeichnen
                        uint16_t timeColor = event.isPast ? TFT_DARKGREY : 0xE300;
                        m_upcomingSprite->setTextColor(timeColor, Theme::ColorMainBackGround);
                        m_upcomingSprite->drawString(event.startTime, textX, yPos);

                        // Summary wieder in normaler Farbe
                        m_upcomingSprite->setTextColor(textColor, Theme::ColorMainBackGround);

                        int timeWidth = 50; // Feste Breite für Zeit-Spalte
                        truncateTextToWidth(event.summary, availableWidth - timeWidth - 5, m_upcomingSprite, truncatedSummary, sizeof(truncatedSummary));
                        m_upcomingSprite->drawString(truncatedSummary, textX + timeWidth, yPos);
                    }
                }
                yPos += 22;
            }
        }

        yPos += 5;
    }

    // Falls keine Events
    if (m_currentData->upcomingEvents.dayCount == 0)
    {
        m_upcomingSprite->setTextColor(TFT_DARKGREY, Theme::ColorMainBackGround);
        m_upcomingSprite->drawString(STR(CalendarKey::CALENDAR_NO_UPCOMING), leftMargin + 10, 20);
    }

    m_upcomingSprite->unloadFont();

    // Sprite auf Display pushen
    m_upcomingSprite->pushSprite(0, viewStartY);

    // Scrollbar immer zeichnen (entscheidet selbst ob nötig)
    drawScrollbar();

    // Status updaten
    m_upcomingSpriteValid = true;
    m_lastScrollOffset = m_scrollState.scrollOffset;
}

/**
 * @brief Kürzt Text auf maximale Pixelbreite mit Ellipsis (Binary Search + UTF-8 safe)
 */
void Calendar::truncateTextToWidth(const char *text, int maxWidth, LGFX_Sprite *sprite, char *output, size_t outputSize) const
{
    if (!text || !sprite || !output || outputSize == 0)
    {
        if (output)
            output[0] = '\0';
        return;
    }

    // Schneller Pfad: Text passt komplett
    if (sprite->textWidth(text) <= maxWidth)
    {
        stringutils::safeCopy(output, outputSize, text);
        return;
    }

    const int ellipsisWidth = sprite->textWidth("...");
    const int targetWidth = maxWidth - ellipsisWidth;

    // Fallback wenn selbst Ellipse nicht passt
    if (targetWidth <= 0)
    {
        stringutils::safeCopy(output, outputSize, "...");
        return;
    }

    char tempBuffer[LINE_BUFFER_SIZE];
    const size_t len = strlen(text);
    const size_t maxLen = std::min<size_t>(len, sizeof(tempBuffer) - 1);

    // Hilfsfunktion: Prüft ob Byte ein UTF-8 Continuation Byte ist (10xxxxxx)
    auto isUtf8Continuation = [](char c)
    {
        return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
    };

    // Binary Search: Finde größte Länge die noch passt
    size_t low = 0;
    size_t high = maxLen;
    size_t bestFit = 0;

    while (low <= high)
    {
        const size_t mid = low + (high - low) / 2;

        memcpy(tempBuffer, text, mid);
        tempBuffer[mid] = '\0';

        if (sprite->textWidth(tempBuffer) <= targetWidth)
        {
            bestFit = mid; // Diese Länge passt, versuche längeren Text
            low = mid + 1;
        }
        else
        {
            if (mid == 0)
                break;
            high = mid - 1; // Zu lang, versuche kürzeren Text
        }
    }

    // UTF-8 Korrektur: Falls wir mitten in einem Multi-Byte Zeichen sind, zurückgehen bis zum Start des Codepoints (z.B. "Mü" -> "M" statt "M�")
    while (bestFit > 0 && isUtf8Continuation(text[bestFit]))
        --bestFit;

    // Ergebnis zusammenbauen
    if (bestFit > 0)
    {
        memcpy(tempBuffer, text, bestFit);
        tempBuffer[bestFit] = '\0';
        stringutils::safeCopy(output, outputSize, tempBuffer);
        stringutils::safeAppend(output, outputSize, "...");
    }
    else
    {
        stringutils::safeCopy(output, outputSize, "...");
    }
}

/**
 * @brief Berechnet die Gesamthöhe des scrollbaren Contents
 */
int Calendar::calculateContentHeight() const
{
    if (!m_hasValidData || !m_currentData)
        return 0;

    int height = 0;
    const int lineSpacing = 3;

    int safeDayCount = min(m_currentData->upcomingEvents.dayCount, MAX_UPCOMING_DAYS);

    for (int dayIdx = 0; dayIdx < safeDayCount; dayIdx++)
    {
        const DayWithEvents &day = m_currentData->upcomingEvents.days[dayIdx];

        if (dayIdx > 0)
            height += lineSpacing * 2; // Trennlinie

        height += 8;  // Padding vor Header
        height += 22; // Datum-Header

        int eventCount = min(day.eventCount, MAX_EVENTS_PER_DAY);
        if (eventCount == 0)
            height += 22; // "Keine Ereignisse"
        else
            height += eventCount * 22; // Pro Event

        height += 6; // Abstand zum nächsten Tag (5 + 1 Korrektur)
    }

    return height;
}

// ===================================================================
// SCROLL-FUNKTIONEN
// ===================================================================

/**
 * @brief Verarbeitet Touch-Input für Scrolling
 */
void Calendar::handleScrollTouch(int32_t tx, int32_t ty, bool touched)
{
    // Nur im linken Bereich scrollen (Scrollbar endet bei 456)
    if (tx > 456)
        return;

    const int viewStartY = 0;
    const int viewHeight = 480;

    // Nur im scrollbaren Bereich
    if (ty < viewStartY || ty > viewStartY + viewHeight)
        return;

    if (touched)
    {
        if (!m_scrollState.isDragging)
        {
            // Drag starten
            m_scrollState.isDragging = true;
            m_scrollState.dragStartY = ty;
            m_scrollState.dragStartOffset = m_scrollState.scrollOffset;
            m_scrollState.scrollVelocity = 0.0f;
            m_scrollState.isSnappingBack = false; // Snap-Animation abbrechen
            m_lastTouchY = ty;
            m_lastTouchTime = millis();
        }
        else
        {
            // Drag fortsetzen mit verstärktem Scroll-Faktor (1.8x)
            int deltaY = ty - m_scrollState.dragStartY;
            m_scrollState.scrollOffset = m_scrollState.dragStartOffset - static_cast<int>(deltaY * 1.8f);

            // Velocity für Momentum berechnen - mit Glättung über mehrere Samples
            unsigned long now = millis();
            int timeDelta = static_cast<int>(now - m_lastTouchTime);

            if (timeDelta > 0 && timeDelta < 100)
            {
                // Neue Velocity mit 70% Gewichtung, alte mit 30% für Glättung
                float newVelocity = static_cast<float>(m_lastTouchY - ty) * 1000.0f / static_cast<float>(timeDelta);
                m_scrollState.scrollVelocity = (newVelocity * 0.7f) + (m_scrollState.scrollVelocity * 0.3f);

                // Velocity verstärken für besseres Momentum
                m_scrollState.scrollVelocity *= 1.5f;
            }

            m_lastTouchY = ty;
            m_lastTouchTime = now;

            // Soft-Begrenzung während des Ziehens (Gummiband-Effekt)
            if (m_scrollState.scrollOffset < -50)
            {
                m_scrollState.scrollOffset = -50 + static_cast<int>((m_scrollState.scrollOffset + 50) * 0.3f);
            }
            else if (m_scrollState.scrollOffset > m_scrollState.maxScroll + 50)
            {
                float excess = static_cast<float>(m_scrollState.scrollOffset - m_scrollState.maxScroll - 50);
                m_scrollState.scrollOffset = m_scrollState.maxScroll + 50 + static_cast<int>(excess * 0.3f);
            }
        }
    }
    else if (m_scrollState.isDragging)
    {
        // Drag beenden
        m_scrollState.isDragging = false;

        // Velocity-Cap für kontrolliertes Momentum
        if (m_scrollState.scrollVelocity > 3000.0f)
            m_scrollState.scrollVelocity = 3000.0f;
        else if (m_scrollState.scrollVelocity < -3000.0f)
            m_scrollState.scrollVelocity = -3000.0f;

        // Prüfen ob das Zurückschnappen überhaupt nötig ist
        if (m_scrollState.scrollOffset < 0 || m_scrollState.scrollOffset > m_scrollState.maxScroll)
            m_scrollState.isSnappingBack = true;
    }
}

/**
 * @brief Aktualisiert Scroll-Physics (Momentum und Reibung)
 */
void Calendar::updateScrollPhysics()
{
    if (m_scrollState.isDragging)
        return;

    // Gummiband-Zurückschnappen wenn außerhalb der Grenzen
    if (m_scrollState.scrollOffset < 0)
    {
        float targetOffset = 0.0f;
        float diff = targetOffset - static_cast<float>(m_scrollState.scrollOffset);
        m_scrollState.scrollOffset += static_cast<int>(diff * 0.35f); // Spring-Konstante

        if (std::abs(m_scrollState.scrollOffset) < 1)
        {
            m_scrollState.scrollOffset = 0;
            m_scrollState.scrollVelocity = 0.0f;
            m_scrollState.isSnappingBack = false;
        }
    }
    else if (m_scrollState.scrollOffset > m_scrollState.maxScroll)
    {
        float targetOffset = static_cast<float>(m_scrollState.maxScroll);
        float diff = targetOffset - static_cast<float>(m_scrollState.scrollOffset);
        m_scrollState.scrollOffset += static_cast<int>(diff * 0.35f); // Spring-Konstante

        if (std::abs(m_scrollState.scrollOffset - m_scrollState.maxScroll) < 1)
        {
            m_scrollState.scrollOffset = m_scrollState.maxScroll;
            m_scrollState.scrollVelocity = 0.0f;
            m_scrollState.isSnappingBack = false;
        }
    }
    // Momentum-Scrolling nur wenn innerhalb der Grenzen
    else if (std::abs(m_scrollState.scrollVelocity) > 0.5f)
    {
        // Geschwindigkeit direkt anwenden
        m_scrollState.scrollOffset += static_cast<int>(m_scrollState.scrollVelocity / 60.0f);

        // Sanftere Reibung für längeres Gleiten
        m_scrollState.scrollVelocity *= 0.92f;

        // Geschwindigkeit nullen wenn sehr klein
        if (std::abs(m_scrollState.scrollVelocity) < 1.0f)
            m_scrollState.scrollVelocity = 0.0f;

        // Harte Grenzen beim Momentum
        if (m_scrollState.scrollOffset < 0)
        {
            m_scrollState.scrollOffset = 0;
            m_scrollState.scrollVelocity = 0.0f;
        }
        else if (m_scrollState.scrollOffset > m_scrollState.maxScroll)
        {
            m_scrollState.scrollOffset = m_scrollState.maxScroll;
            m_scrollState.scrollVelocity = 0.0f;
        }
    }
    else
    {
        // Wenn keine Velocity aber Snap-Flag gesetzt, Animation beenden
        m_scrollState.isSnappingBack = false;
    }
}

/**
 * @brief Begrenzt die Scroll-Position auf gültige Werte
 */
void Calendar::constrainScroll()
{
    // Harte Grenzen nur für finale Positionen (nicht während Drag)
    if (!m_scrollState.isDragging)
    {
        if (m_scrollState.scrollOffset < 0)
        {
            m_scrollState.scrollOffset = 0;
            m_scrollState.scrollVelocity = 0.0f;
        }
        else if (m_scrollState.scrollOffset > m_scrollState.maxScroll)
        {
            m_scrollState.scrollOffset = m_scrollState.maxScroll;
            m_scrollState.scrollVelocity = 0.0f;
        }
    }
}

/**
 * @brief Zeichnet die Scrollbar mit Sprite für flackerfreies Rendering
 */
void Calendar::drawScrollbar()
{
    if (!m_scrollbarSprite)
        return;

    const int barX = 453;
    const int barWidth = 3;
    const int viewHeight = 480;

    // Nur zeichnen wenn Scrolling nötig ist
    if (m_scrollState.contentHeight <= viewHeight)
    {
        // Kein Scrollbar nötig - Bereich schwarz übermalen
        m_tft->fillRect(barX, 0, 5, viewHeight, Theme::ColorMainBackGround);
        return;
    }

    // Scrollbar-Höhe proportional zum sichtbaren Anteil
    float visibleRatio = static_cast<float>(viewHeight) / static_cast<float>(m_scrollState.contentHeight);
    int barHeight = static_cast<int>(viewHeight * visibleRatio);
    barHeight = max(30, min(barHeight, viewHeight)); // Min 30px für bessere Greifbarkeit

    // Position basierend auf Scroll-Offset (mit Clamping für Overscroll)
    float scrollRatio = 0.0f;
    if (m_scrollState.maxScroll > 0)
    {
        int clampedOffset = max(0, min(m_scrollState.scrollOffset, m_scrollState.maxScroll));
        scrollRatio = static_cast<float>(clampedOffset) / static_cast<float>(m_scrollState.maxScroll);
    }
    int barY = static_cast<int>(scrollRatio * (viewHeight - barHeight));

    // In Sprite zeichnen
    m_scrollbarSprite->clear(Theme::ColorMainBackGround);

    // Track (Hintergrund) - sehr dunkelgrau
    m_scrollbarSprite->fillRect(0, 0, barWidth, viewHeight, 0x2104);

    // Thumb (Scrollbar selbst)
    uint16_t barColor;
    if (m_scrollState.isDragging)
        barColor = 0x8410; // Heller wenn aktiv
    else if (m_scrollState.isSnappingBack || std::abs(m_scrollState.scrollVelocity) > 10.0f)
        barColor = 0x6B6D; // Mittelhell während Animation
    else
        barColor = 0x5ACB; // Normal

    // Scrollbar mit abgerundeten Ecken
    m_scrollbarSprite->fillRect(0, barY + 1, barWidth, barHeight - 2, barColor);

    // Sprite auf Display pushen
    m_scrollbarSprite->pushSprite(barX, 0);
}

/**
 * @brief Prüft ob ein bestimmter Tag Events hat
 */
bool Calendar::hasEventForDay(int day, int month, int year, bool checkOnlyCurrentMonths) const
{
    if (!m_hasValidData || !m_currentData)
        return false;

    // Bounds-Check
    if (day < 1 || day > 31)
        return false;

    // Aktueller Monat
    if (month == m_currentData->today.currentMonth && year == m_currentData->today.currentYear)
        return m_currentData->currentMonth.hasEvent[day];

    // Nächster Monat
    if (month == m_currentData->nextMonth.month && year == m_currentData->nextMonth.year)
        return m_currentData->nextMonth.hasEvent[day];

    // Für Lückenfüller: keine Events anzeigen wenn checkOnlyCurrentMonths = true
    if (checkOnlyCurrentMonths)
        return false;

    return false;
}

// ===================================================================
// MONATSKALENDER-FUNKTIONEN
// ===================================================================

/**
 * @brief Berechnet die Pixelhöhe eines Monatskalenders
 */
int Calendar::calculateMonthCalendarHeight(int month, int year) const
{
    // Geometrie-Konstanten aus drawMonthCalendars() spiegeln
    constexpr int HEADER_H = 25;    ///< Höhe des Monats-Headers (gridStartY = startY + 25)
    constexpr int GRID_OFFSET = 30; ///< Offset der ersten Zelle unterhalb gridStartY

    int daysInMonth = getDaysInMonth(month, year);
    int startDayOfWeek = getStartDayOfWeek(month, year);

    // Anzahl der für das Monatsraster benötigten Zeilen
    int totalDaysToShow = startDayOfWeek + daysInMonth;
    int rowsNeeded = (totalDaysToShow + 6) / 7;

    // Letzte Zeile wegfallen lassen, wenn sie vollständig zum Folgemonat gehört
    const int lastRowStartCell = (rowsNeeded - 1) * 7;
    const bool lastRowIsAllNextMonth = (lastRowStartCell >= startDayOfWeek + daysInMonth);
    if (lastRowIsAllNextMonth)
        rowsNeeded--;

    return HEADER_H + GRID_OFFSET + rowsNeeded * CELL_HEIGHT;
}

/**
 * @brief Zeichnet Monatskalender
 */
void Calendar::drawMonthCalendars(int month, int year, int startY, bool isCurrentMonth)
{
    const int rightPanelX = 480;
    const int gridStartX = rightPanelX;
    const int cellWidth = 38;
    const int cellHeight = 28;
    const int weekColumnWidth = 25;

    // Berechne das tatsächliche Zentrum des Kalender-Grids (ohne Wochenspalte)
    const int gridCenterX = gridStartX + weekColumnWidth + (7 * cellWidth) / 2;

    // Header mit Monatsname und Jahr
    drawMonthHeader(month, year, gridCenterX, startY);

    // Kalender-Grid Parameter
    const int gridStartY = startY + 25;

    m_tft->loadFont(ComfortaaBold14);

    // Wochentag-Header: "#" für KW-Spalte, dann Mo..So in tm_wday-Konvention
    static constexpr int WD_INDICES[7] = {1, 2, 3, 4, 5, 6, 0};
    char wdBuf[8];
    m_tft->setTextDatum(MC_DATUM);
    m_tft->setTextColor(0x9CF3, Theme::ColorMainBackGround);

    for (int i = 0; i < 8; i++)
    {
        int x = gridStartX + (i == 0 ? weekColumnWidth / 2 : weekColumnWidth + (i - 1) * cellWidth + cellWidth / 2);
        const char *txt;
        if (i == 0)
            txt = "#";
        else
        {
            I18n::copyWeekday(wdBuf, sizeof(wdBuf), WD_INDICES[i - 1], false);
            txt = wdBuf;
        }
        m_tft->drawString(txt, x, gridStartY + 12);
    }

    // Trennlinie unter Header
    m_tft->drawFastHLine(gridStartX, gridStartY + 22, weekColumnWidth + 7 * cellWidth, 0x6B4D);

    int daysInMonth = getDaysInMonth(month, year);
    int startDayOfWeek = getStartDayOfWeek(month, year);

    // Vorheriger und nächster Monat für Lückenfüllung
    int prevMonth = month - 1;
    int prevYear = year;
    if (prevMonth < 1)
    {
        prevMonth = 12;
        prevYear--;
    }

    int nextMonthCalc = month + 1;
    int nextYearCalc = year;
    if (nextMonthCalc > 12)
    {
        nextMonthCalc = 1;
        nextYearCalc++;
    }

    int daysInPrevMonth = getDaysInMonth(prevMonth, prevYear);

    // Berechnen, wie viele Zeilen benötigt werden
    int totalDaysToShow = startDayOfWeek + daysInMonth;
    int rowsNeeded = (totalDaysToShow + 6) / 7;

    // Prüfen, ob die letzte Zeile komplett zum nächsten Monat gehört
    int lastRowStartCell = (rowsNeeded - 1) * 7;
    bool lastRowIsAllNextMonth = (lastRowStartCell >= startDayOfWeek + daysInMonth);

    if (lastRowIsAllNextMonth)
        rowsNeeded--;

    int totalCells = rowsNeeded * 7;

    // Kalender-Grid zeichnen
    for (int cellIndex = 0; cellIndex < totalCells; cellIndex++)
    {
        int weekday = cellIndex % 7;
        int row = cellIndex / 7;

        // Berechne welcher Tag dargestellt wird
        int displayDay;
        int displayMonth = month;
        int displayYear = year;
        bool isCurrentMonthDay = false;

        if (cellIndex < startDayOfWeek)
        {
            // Tage vom vorherigen Monat
            displayDay = daysInPrevMonth - (startDayOfWeek - cellIndex - 1);
            displayMonth = prevMonth;
            displayYear = prevYear;
        }
        else if (cellIndex < startDayOfWeek + daysInMonth)
        {
            // Tage vom aktuellen Monat
            displayDay = cellIndex - startDayOfWeek + 1;
            isCurrentMonthDay = true;
        }
        else
        {
            // Tage vom nächsten Monat
            displayDay = cellIndex - startDayOfWeek - daysInMonth + 1;
            displayMonth = nextMonthCalc;
            displayYear = nextYearCalc;
        }

        int cellX = gridStartX + weekColumnWidth + weekday * cellWidth;
        int cellY = gridStartY + 30 + row * cellHeight;

        // Kalenderwoche nur in der ersten Spalte jeder Zeile
        if (weekday == 0)
        {
            int kw = getCalendarWeek(displayDay, displayMonth, displayYear);
            m_tft->setTextColor(0x7BEF, Theme::ColorMainBackGround);
            m_tft->setTextDatum(MC_DATUM);
            char kwStr[4];
            stringutils::safeFormat(kwStr, "%d", kw);
            m_tft->drawString(kwStr, gridStartX + weekColumnWidth / 2, cellY + cellHeight / 2 - 2);
        }

        // Hintergrund und Textfarbe bestimmen
        bool isToday = isCurrentMonth && isCurrentMonthDay && (displayDay == m_currentData->today.currentDay);
        uint16_t textColor;

        if (isToday)
        {
            m_tft->fillRect(cellX + 4, cellY, cellWidth - 8, cellHeight - 2, 0x0398);
            textColor = TFT_WHITE;
        }
        else if (isCurrentMonthDay)
        {
            textColor = 0xD69A; // aktuelle Monatstage
        }
        else
        {
            textColor = 0x8C51; // Vor-/Nachmonatstage
        }

        // Tag-Nummer
        m_tft->setTextColor(textColor, isToday ? 0x0398 : Theme::ColorMainBackGround);
        m_tft->setTextDatum(MC_DATUM);
        char dayStr[3];
        stringutils::safeFormat(dayStr, "%d", displayDay);
        m_tft->drawString(dayStr, cellX + cellWidth / 2, cellY + cellHeight / 2 - 2);

        // Event-Indikator nur für aktuelle Monatstage
        if (isCurrentMonthDay && hasEventForDay(displayDay, displayMonth, displayYear, true))
        {
            uint16_t indicatorColor = isToday ? TFT_WHITE : COLOR_NORMAL_EVENT;
            int lineY = cellY + cellHeight - 6;
            int lineStartX = cellX + 8;
            int lineEndX = cellX + cellWidth - 8;
            m_tft->drawFastHLine(lineStartX, lineY, lineEndX - lineStartX, indicatorColor);
        }
    }

    m_tft->unloadFont();
}

/**
 * @brief Zeichnet Monats-Header
 */
void Calendar::drawMonthHeader(int month, int year, int centerX, int y)
{
    m_tft->loadFont(ComfortaaBold16);
    m_tft->setTextColor(COLOR_ALLDAY, Theme::ColorMainBackGround);
    m_tft->setTextDatum(MC_DATUM);

    char monthYearStr[20];
    char monBuf[16];
    I18n::copyMonth(monBuf, sizeof(monBuf), month - 1, /*longForm=*/false);
    stringutils::safeFormat(monthYearStr, "%s %d", monBuf, year);
    m_tft->drawString(monthYearStr, centerX, y + 12);
    m_tft->unloadFont();
}

/**
 * @brief Berechnet Kalenderwoche nach ISO 8601
 */
int Calendar::getCalendarWeek(int day, int month, int year) const
{
    struct tm time_in = {0, 0, 0, day, month - 1, year - 1900, 0, 0, 0};
    time_t time_temp = mktime(&time_in);
    struct tm const *time_out = localtime(&time_temp);

    int dayOfYear = time_out->tm_yday + 1; // 1-basiert
    int weekday = time_out->tm_wday;
    if (weekday == 0)
        weekday = 7; // Sonntag = 7

    // Montag der aktuellen Woche
    int mondayDayOfYear = dayOfYear - weekday + 1;

    // 4. Januar bestimmt die erste Woche
    struct tm jan4 = {0, 0, 0, 4, 0, year - 1900, 0, 0, 0};
    time_t jan4_time = mktime(&jan4);
    struct tm const *jan4_out = localtime(&jan4_time);
    int jan4_weekday = jan4_out->tm_wday;
    if (jan4_weekday == 0)
        jan4_weekday = 7;

    int firstMondayOfYear = 4 - jan4_weekday + 1;

    if (mondayDayOfYear < firstMondayOfYear)
        return getCalendarWeek(31, 12, year - 1);

    int weekNumber = ((mondayDayOfYear - firstMondayOfYear) / 7) + 1;
    return (weekNumber > 52) ? 1 : weekNumber;
}

/**
 * @brief Gibt Anzahl der Tage im Monat zurück
 */
int Calendar::getDaysInMonth(int month, int year) const
{
    if (month == 2)
        return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 29 : 28;
    else if (month == 4 || month == 6 || month == 9 || month == 11)
        return 30;

    return 31;
}

/**
 * @brief Gibt den Wochentag des ersten Tag des Monats zurück
 */
int Calendar::getStartDayOfWeek(int month, int year) const
{
    struct tm time_in = {0, 0, 0, 1, month - 1, year - 1900, 0, 0, 0};
    time_t time_temp = mktime(&time_in);
    struct tm const *time_out = localtime(&time_temp);
    int startDay = time_out->tm_wday - 1;
    if (startDay < 0)
        startDay = 6;

    return startDay;
}

/**
 * @brief Zeichnet Digitaluhr
 */
void Calendar::drawDigitalClock(int32_t x, int32_t y)
{
    if (!m_clockSprite || !m_hasValidData)
        return;

    DS1307_RTC *rtc = &APP_STATE.getRTC();
    Clocks::drawDigitalClockPlain(m_clockSprite, rtc, m_lastDrawnSecond, x, y, false, true);
}
