#include "Contacts.h"

#include <ArduinoJson.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>

#include "AppState.h"
#include "HttpService.h"    // Zentraler HTTP-Service mit PSRAM-Puffern
#include "I18n.h"
#include "ProgressBox.h"
#include "Strings_contacts.h"
#include "StringUtils.h"
#include "Theme.h"

// Fonts
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold30.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// KONSTANTEN
// ===================================================================

namespace
{

    // Fehler-Codes
    constexpr int ERR_SHUTDOWN = -100;        ///< Shutdown angefordert
    constexpr int ERR_JSON_PARSE = -101;      ///< JSON-Parsing fehlgeschlagen
    constexpr int ERR_EMPTY_RESPONSE = -102;  ///< Leere Server-Antwort
    constexpr int ERR_HTTP_NOT_INIT = -103;   ///< HttpService nicht initialisiert
    constexpr int ERR_SCHEMA = -104;         ///< Erforderliche Datenstruktur fehlt

    // Task Konfiguration
    constexpr size_t WORKER_STACK_SIZE = 16384; ///< Stack-Größe des Worker-Tasks
    constexpr UBaseType_t WORKER_PRIORITY = 1;  ///< Priorität des Worker-Tasks
    constexpr BaseType_t WORKER_CORE = 1;       ///< Core auf dem der Worker läuft

    // ===================================================================
    // LAYOUT-KONSTANTEN
    // ===================================================================

    // Viewport
    constexpr int VIEWPORT_HEIGHT = 480;

    // Alphabet-Leiste
    constexpr int ALPHABET_BAR_X = 0;
    constexpr int ALPHABET_BAR_WIDTH = 40;
    constexpr int ALPHABET_ENTRY_HEIGHT = 18;
    constexpr int ALPHABET_TOTAL_HEIGHT = ContactsCfg::ALPHABET_ENTRIES * ALPHABET_ENTRY_HEIGHT;
    constexpr int ALPHABET_Y_OFFSET = (VIEWPORT_HEIGHT - ALPHABET_TOTAL_HEIGHT) / 2;

    // Kontaktliste
    constexpr int LIST_X = ALPHABET_BAR_WIDTH + 5;
    constexpr int LIST_WIDTH = 280;
    constexpr int CONTACT_HEIGHT = 40;
    constexpr int LIST_PADDING = 5;
    constexpr int SCROLLBAR_X = LIST_X + LIST_WIDTH + 5;
    constexpr int SCROLLBAR_WIDTH = 5;

    // Detail-Bereich
    constexpr int DETAIL_X = SCROLLBAR_X + SCROLLBAR_WIDTH + 10;
    constexpr int DETAIL_Y = 5;
    constexpr int DETAIL_WIDTH = 800 - DETAIL_X - 10;
    constexpr int DETAIL_PADDING = 15;
    constexpr int DETAIL_LINE_HEIGHT = 32;
    constexpr int DETAIL_LABEL_WIDTH = 130;
    constexpr int DETAIL_SPRITE_HEIGHT = 420;

    // Exit-Button Layout
    constexpr int16_t EXIT_BUTTON_X = 765;
    constexpr int16_t EXIT_BUTTON_Y = 457;
    constexpr uint16_t BUTTON_SIZE = 45;
    constexpr uint16_t BUTTON_RADIUS = 4;

    // Action-Buttons
    constexpr uint16_t ACTION_BTN_HEIGHT = 45;
    constexpr int16_t ACTION_BTN_Y = 457;

    // Search-Button
    constexpr uint16_t SEARCH_BTN_WIDTH = 70;
    constexpr int16_t SEARCH_BTN_X = DETAIL_X + SEARCH_BTN_WIDTH / 2;

    // Reset-Button
    constexpr uint16_t RESET_BTN_WIDTH = 55;
    constexpr int16_t RESET_BTN_X = DETAIL_X + SEARCH_BTN_WIDTH + 10 + RESET_BTN_WIDTH / 2;

    // Refresh-Button
    constexpr uint16_t REFRESH_BTN_WIDTH = 120;
    constexpr int16_t REFRESH_BTN_X = DETAIL_X + SEARCH_BTN_WIDTH + 10 + RESET_BTN_WIDTH + 10 + REFRESH_BTN_WIDTH / 2;

    // Birthday Badge
    constexpr int BIRTHDAY_BADGE_SIZE = 23;
    constexpr uint16_t COLOR_BIRTHDAY = TFT_YELLOW;
    constexpr uint16_t COLOR_BIRTHDAY_BG = 0xFC00;

    // Alter-Badge
    constexpr int AGE_BADGE_PADDING_H = 10;
    constexpr int AGE_BADGE_PADDING_V = 4;
    constexpr int AGE_BADGE_RADIUS = 8;
    constexpr uint16_t COLOR_AGE_BADGE_BG = 0x2104;

    // Farben
    constexpr uint16_t COLOR_BUTTON = 0x39E8;
    constexpr uint16_t COLOR_BUTTON_TEXT = TFT_BLACK;
    constexpr uint16_t COLOR_BUTTON_BORDER = TFT_DARKGREY;
    constexpr uint16_t COLOR_LOADING_TEXT = 0xD69A;
    constexpr uint16_t COLOR_SELECTED = Theme::ColorSelectedRow;
    constexpr uint16_t COLOR_CONTACT_NAME = 0xD69A;
    constexpr uint16_t COLOR_LABEL = 0x9CF3;
    constexpr uint16_t COLOR_VALUE = 0xD69A;
    constexpr uint16_t COLOR_SEPARATOR = 0x4208;
    constexpr uint16_t COLOR_ALPHABET_BG = 0x18C3;
    constexpr uint16_t COLOR_ALPHABET_TEXT = 0x9CF3;
    constexpr uint16_t COLOR_ALPHABET_ACTIVE = 0xFFFF;
    constexpr uint16_t COLOR_SCROLLBAR_TRACK = 0x2104;
    constexpr uint16_t COLOR_SCROLLBAR_ACTIVE = 0x8410;
    constexpr uint16_t COLOR_SCROLLBAR_NORMAL = 0x5ACB;
    constexpr uint16_t COLOR_NO_DATA = TFT_DARKGREY;
    constexpr uint16_t COLOR_PROGRESS_BAR = 0x8010;
    constexpr uint16_t COLOR_STATS = 0x9CF3;

    // Fehlerbehandlung
    constexpr int MAX_CONSECUTIVE_ERRORS = 5;

    // Buffer-Größe für Textkürzung
    constexpr size_t LINE_BUFFER_SIZE = 128;
}

// ===================================================================
// KONSTRUKTOR / DESTRUKTOR
// ===================================================================

/**
 * @brief Konstruktor
 */
Contacts::Contacts()
{
    m_dataBuffers[0].reset();
    m_dataBuffers[1].reset();
    m_activeBufferIndex.store(0);
    m_dataReady.store(false);

    updateCurrentTime();
}

/**
 * @brief Destruktor
 */
Contacts::~Contacts()
{
    shutdownWorker();

    if (m_tft)
    {
        m_tft->safeDeleteSprite(m_listSprite);
        m_tft->safeDeleteSprite(m_detailSprite);
        m_tft->safeDeleteSprite(m_alphabetSprite);
        m_tft->safeDeleteSprite(m_scrollbarSprite);
        m_tft->safeDeleteSprite(m_clockSprite);
        m_tft->safeDeleteSprite(m_progressSprite);
    }
}

/**
 * @brief Beendet den Worker-Task sauber
 */
void Contacts::shutdownWorker(bool fast)
{
    TaskHandle_t handle = m_taskHandle.load();
    if (!handle)
        return;

    m_shutdownRequested.store(true);

    if (m_inFlight.load())
    {
        LOG_WARNING("Shutdown während des HTTP-Requests - warte auf Abschluss");

        int waited = 0;
        constexpr int HTTP_TIMEOUT_MS = 15000;

        while (m_inFlight.load() && waited < HTTP_TIMEOUT_MS)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            waited += 100;
        }

        if (m_inFlight.load())
            LOG_ERROR("HTTP-Timeout überschritten - erzwinge Shutdown");
    }

    handle = m_taskHandle.load();
    if (handle)
        xTaskNotifyGive(handle);

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
        LOG_WARNING("Worker reagiert nicht - erzwinge Löschen");
        vTaskDelete(handle);
        m_taskHandle.store(nullptr);
    }
    else
    {
        LOG_SUCCESS("Worker-Task beendet");
    }
}

/**
 * @brief Gibt den Modultyp zurück
 */
AppModule Contacts::type() const
{
    return AppModule::CONTACTS;
}

// ===================================================================
// DOUBLE-BUFFER FUNKTIONEN
// ===================================================================

/**
 * @brief Liefert den aktiven Buffer (für UI-Thread)
 */
ContactsData *Contacts::getActiveBuffer()
{
    return &m_dataBuffers[m_activeBufferIndex.load()];
}

/**
 * @brief Liefert den inaktiven Buffer (für Worker-Thread)
 */
ContactsData *Contacts::getInactiveBuffer()
{
    return &m_dataBuffers[1 - m_activeBufferIndex.load()];
}

/**
 * @brief Tauscht atomisch aktiven und inaktiven Buffer
 */
void Contacts::swapBuffers()
{
    const int current = m_activeBufferIndex.load();
    m_activeBufferIndex.store(1 - current);
    LOG_INFO_F("Buffer-Swap: %d -> %d", current, 1 - current);
}

// ===================================================================
// INITIALISIERUNG
// ===================================================================

/**
 * @brief Initialisiert das Contacts-Modul
 * 
 * GEÄNDERT: WifiManager-Parameter entfernt, verwendet jetzt HTTP_SERVICE
 */
bool Contacts::init(LGFX &tft, const ContactsCallbacks &callbacks)
{
    if (m_taskHandle.load())
    {
        LOG_WARNING("Re-Initialisierung: Beende alten Worker-Task");
        shutdownWorker(true);
    }

    m_initialized = false;
    m_tft = &tft;
    m_callbacks = callbacks;

    // Google Apps Script URL aus AppState laden
    m_appsScriptUrl = APP_STATE.getContactsScriptUrl();

    // URL validieren: muss vorhanden sein und mit "/exec" enden
    const size_t urlLen = m_appsScriptUrl ? strlen(m_appsScriptUrl) : 0;
    m_demoMode = (urlLen < 5 || strcmp(m_appsScriptUrl + urlLen - 5, "/exec") != 0);

    if (m_demoMode && urlLen > 0)
        LOG_WARNING("Contacts: URL endet nicht mit /exec → Demo-Modus");
    else if (m_demoMode)
        LOG_INFO("Contacts: Keine API-URL konfiguriert → Demo-Modus aktiv");
    else
        LOG_INFO_F("Contacts: API-URL geladen (%d Zeichen)", strlen(m_appsScriptUrl));

    m_shutdownRequested.store(false);
    m_refreshRequested.store(false);
    m_inFlight.store(false);
    m_fetchCompleted.store(false);
    m_activeBufferIndex.store(0);
    m_dataReady.store(false);

    m_hasValidData = false;
    m_scrollState.reset();
    m_tapState.reset();
    m_searchState.reset();
    m_selectedContactIndex = -1;
    m_selectedContactId[0] = '\0';
    m_lastTouchY = -1;
    m_lastTouchTime = 0;
    m_alphabetTouchActive = false;
    m_selectedLetter = '?';

    m_lastApiCall = 0;
    m_backoffMs = 0;
    m_nextAllowedFetch.store(0);
    m_consecutiveErrors = 0;

    m_layoutDrawn = false;
    m_listSpriteValid = false;
    m_detailSpriteValid = false;
    m_lastScrollOffset = -1;
    m_lastDrawnSecond = -1;
    m_lastUpdateTimestamp[0] = '\0';

    m_dataBuffers[0].reset();
    m_dataBuffers[1].reset();

    if (!m_tft->safeCreateSprite(m_listSprite, LIST_WIDTH, VIEWPORT_HEIGHT) ||
        !m_tft->safeCreateSprite(m_detailSprite, DETAIL_WIDTH, DETAIL_SPRITE_HEIGHT) ||
        !m_tft->safeCreateSprite(m_alphabetSprite, ALPHABET_BAR_WIDTH, VIEWPORT_HEIGHT) ||
        !m_tft->safeCreateSprite(m_scrollbarSprite, SCROLLBAR_WIDTH, VIEWPORT_HEIGHT) ||
        !m_tft->safeCreateSprite(m_clockSprite, 103, 45) ||
        !m_tft->safeCreateSprite(m_progressSprite, 1, VIEWPORT_HEIGHT))
    {
        LOG_ERROR("Konnte Sprites nicht erstellen");
        return false;
    }

    m_clockSprite->clear(Theme::ColorMainBackGround);

    m_keyboard = std::make_unique<Keyboard>(m_tft);
    m_keyboard->setPosition();

    m_progressBox = std::make_unique<ProgressBox>(*m_tft);
    m_progressBox->setAutoCenter(true);
    m_progressBox->setTitle(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_PROGRESS_TITLE)));
    m_progressBox->setMessage(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_PROGRESS_MESSAGE)));

    m_exitButton.initButton(m_tft, EXIT_BUTTON_X, EXIT_BUTTON_Y,
        BUTTON_SIZE, BUTTON_SIZE, BUTTON_RADIUS,
        COLOR_BUTTON, COLOR_BUTTON_TEXT, COLOR_BUTTON_BORDER,
        "^POWER:32:D206^", nullptr);
    m_exitButton.setFont(LeagueGothicCond34);

    m_tft->clear(Theme::ColorMainBackGround);
    m_tft->loadFont(LeagueGothicCond34);
    m_tft->setTextColor(COLOR_LOADING_TEXT, Theme::ColorMainBackGround);
    m_tft->setTextDatum(MC_DATUM);
    m_tft->drawString(STR(ContactsKey::CONTACTS_LOADING_TITLE), m_tft->width() / 2, (m_tft->height() / 2) - 25);
    m_tft->unloadFont();

    m_cancelLoadButton.initButton(m_tft, m_tft->width() / 2, m_tft->height() / 2 + 25,
        BUTTON_SIZE, BUTTON_SIZE, BUTTON_RADIUS,
        COLOR_BUTTON, COLOR_BUTTON_TEXT, COLOR_BUTTON_BORDER,
        "^DEL:32:F800^", nullptr);
    m_cancelLoadButton.drawButton(false);

    m_searchButton.initButton(m_tft, SEARCH_BTN_X, ACTION_BTN_Y,
        SEARCH_BTN_WIDTH, ACTION_BTN_HEIGHT, BUTTON_RADIUS,
        COLOR_BUTTON, COLOR_BUTTON_TEXT, COLOR_BUTTON_BORDER,
        reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_BTN_SEARCH)), nullptr);
    m_searchButton.setFont(LeagueGothicCond34);

    m_resetButton.initButton(m_tft, RESET_BTN_X, ACTION_BTN_Y,
        RESET_BTN_WIDTH, ACTION_BTN_HEIGHT, BUTTON_RADIUS,
        COLOR_BUTTON, COLOR_BUTTON_TEXT, COLOR_BUTTON_BORDER,
        reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_BTN_RESET)), nullptr);
    m_resetButton.setFont(LeagueGothicCond34);

    m_refreshButton.initButton(m_tft, REFRESH_BTN_X, ACTION_BTN_Y,
        REFRESH_BTN_WIDTH, ACTION_BTN_HEIGHT, BUTTON_RADIUS,
        COLOR_BUTTON, COLOR_BUTTON_TEXT, COLOR_BUTTON_BORDER,
        reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_BTN_REFRESH)), nullptr);
    m_refreshButton.setFont(LeagueGothicCond34);

    m_messageBox = std::make_unique<MessageBox>(m_tft);
    m_messageBox->setBounds(200, 150, 400, 180);

    TaskHandle_t taskHandle = nullptr;
    BaseType_t result = xTaskCreatePinnedToCore(
        &Contacts::taskEntry, "ContactsWorker", WORKER_STACK_SIZE,
        this, WORKER_PRIORITY, &taskHandle, WORKER_CORE);

    if (result != pdPASS)
    {
        LOG_ERROR("Konnte Contacts Worker-Task nicht starten");
        return false;
    }

    m_taskHandle.store(taskHandle);
    m_initialized = true;

    LOG_SUCCESS("Contacts-Modul erfolgreich initialisiert.");
    return true;
}

// ===================================================================
// REFRESH REQUEST
// ===================================================================

/**
 * @brief Fordert einen Daten-Refresh an
 */
void Contacts::requestRefresh(bool force)
{
    if (!m_initialized || m_shutdownRequested.load())
        return;

    if (!force && millis() < m_nextAllowedFetch.load())
        return;

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
 * @brief Statischer Task-Einstiegspunkt
 */
void Contacts::taskEntry(void *arg)
{
    auto *self = static_cast<Contacts *>(arg);
    if (self)
        self->workerLoop();
}

/**
 * @brief Hauptschleife des Worker-Tasks
 */
void Contacts::workerLoop()
{
    LOG_INFO("Contacts Worker-Task gestartet");

    constexpr TickType_t WAIT_TICKS = pdMS_TO_TICKS(30000);

    while (!m_shutdownRequested.load())
    {
        ulTaskNotifyTake(pdTRUE, WAIT_TICKS);

        if (m_shutdownRequested.load())
        {
            LOG_INFO("Shutdown-Signal empfangen - beende Worker-Task");
            break;
        }

        if (!m_refreshRequested.exchange(false))
            continue;

        m_inFlight.store(true);
        LOG_INFO("Worker: Starte Datenabruf");

        ContactsData *fetchBuffer = getInactiveBuffer();
        fetchBuffer->reset();

        const int httpCode = fetchContactsDataCore(fetchBuffer);

        if (m_shutdownRequested.load())
        {
            m_inFlight.store(false);
            break;
        }

        if (httpCode == HTTP_CODE_OK)
        {
            sortContacts(fetchBuffer);
            buildAlphabetIndex(fetchBuffer);
            swapBuffers();
            m_dataReady.store(true);

            LOG_INFO("Double-Buffer Swap erfolgreich - neue Daten verfügbar");

            m_consecutiveErrors = 0;
            m_backoffMs = 0;
            m_nextAllowedFetch.store(millis());
        }
        else
        {
            m_fetchCompleted.store(true);

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

                m_backoffMs = (m_backoffMs == 0)
                                  ? ContactsCfg::MIN_RETRY_INTERVAL_MS
                                  : std::min(m_backoffMs * 2, ContactsCfg::MAX_BACKOFF_TIME_MS);

                m_nextAllowedFetch.store(millis() + m_backoffMs);

                LOG_WARNING_F("Worker: Fehler %d/%d, Backoff: %lu ms (HTTP: %d)", 
                              m_consecutiveErrors, MAX_CONSECUTIVE_ERRORS, m_backoffMs, httpCode);

                if (m_consecutiveErrors >= MAX_CONSECUTIVE_ERRORS)
                {
                    LOG_WARNING("Zu viele transiente Fehler - wechsle in Demo-Modus");
                    m_demoMode = true;
                }
            }

            // Demo-Modus aktiviert -> sofort Demo-Daten bereitstellen
            if (m_demoMode)
            {
                int backIdx = 1 - m_activeBufferIndex.load();
                m_dataBuffers[backIdx].reset();
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
    }

    LOG_SUCCESS("Der Worker-Task beendet sich");
    m_taskHandle.store(nullptr);
    vTaskDelete(nullptr);
}

// ===================================================================
// API-FUNKTIONEN (MIT HTTP_SERVICE UND PSRAM)
// ===================================================================

/**
 * @brief Führt den HTTP-Request und JSON-Parsing durch
 */
int Contacts::fetchContactsDataCore(ContactsData *outData)
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

        LOG_INFO("Demo-Modus aktiv - fiktive Kontakte generiert");
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
        LOG_WARNING("Contacts: Apps Script URL ist leer");
        return -1;
    }

    if (!HTTP_SERVICE.isInitialized())
    {
        LOG_ERROR("Contacts: HttpService nicht initialisiert!");
        return ERR_HTTP_NOT_INIT;
    }

    if (!APP_STATE.getWifiConnected())
    {
        LOG_WARNING("Contacts: Kein WiFi verfügbar");
        return -2;
    }

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

    if (!doc["contacts"].is<JsonArray>() || !doc["error"].isNull())
    {
        LOG_ERROR("Erforderliche Datenstruktur fehlt in der Server-Antwort");
        return ERR_SCHEMA;
    }

    // ===================================================================
    // DATEN VERARBEITEN
    // ===================================================================

    JsonObject today = doc["today"];
    if (today)
    {
        outData->today.currentDay = today["day"] | outData->today.currentDay;
        outData->today.currentMonth = today["month"] | outData->today.currentMonth;
        outData->today.currentYear = today["year"] | outData->today.currentYear;
        LOG_INFO("Server-Datum übernommen");
    }

    JsonArray contacts = doc["contacts"];
    outData->contactCount = 0;
    outData->stats = ContactStats{};

    if (contacts)
    {
        for (JsonObject contact : contacts)
        {
            if (outData->contactCount >= ContactsCfg::MAX_CONTACTS)
                break;

            ContactItem &item = outData->contacts[outData->contactCount];
            item.reset();

            stringutils::safeCopy(item.id, contact["id"] | "");
            stringutils::safeCopy(item.firstName, contact["firstName"] | "");
            stringutils::safeCopy(item.lastName, contact["lastName"] | "");
            stringutils::safeCopy(item.phoneHome, contact["phoneHome"] | "");
            stringutils::safeCopy(item.phoneMobile, contact["phoneMobile"] | "");
            stringutils::safeCopy(item.phoneWork, contact["phoneWork"] | "");
            stringutils::safeCopy(item.email, contact["email"] | "");

            stringutils::safeCopy(item.address.street, contact["addrStreet"] | "");
            stringutils::safeCopy(item.address.postalCode, contact["addrPostalCode"] | "");
            stringutils::safeCopy(item.address.city, contact["addrCity"] | "");
            stringutils::safeCopy(item.address.country, contact["addrCountry"] | "");

            stringutils::safeCopy(item.company, contact["company"] | "");

            item.birthdayDay = contact["birthdayDay"] | 0;
            item.birthdayMonth = contact["birthdayMonth"] | 0;
            item.birthdayYear = contact["birthdayYear"] | 0;

            formatDisplayName(item);
            item.sortKey = getSortKey(item.lastName[0] ? item.lastName : item.firstName);
            item.hasBirthdayToday = checkBirthdayToday(item, outData->today);

            outData->stats.total++;

            if (item.birthdayDay > 0 && item.birthdayMonth > 0)
                outData->stats.withBirthday++;

            if (item.hasBirthdayToday)
                outData->stats.birthdaysToday++;

            outData->contactCount++;
        }
    }

    LOG_INFO_F("Kontakte geladen: %d von max %d",
               outData->contactCount, ContactsCfg::MAX_CONTACTS);

    if (!m_shutdownRequested.load())
        LOG_SUCCESS("Kontakt-Daten erfolgreich verarbeitet");

    return m_shutdownRequested.load() ? ERR_SHUTDOWN : HTTP_CODE_OK;
}

// ===================================================================
// DEMO-DATEN GENERIERUNG
// ===================================================================

/**
 * @brief Generiert fiktive Demo-Kontakte für Präsentationen
 */
void Contacts::generateDemoData(ContactsData *outData)
{
    // Arrays im Flash (nicht auf dem Stack!)
    static constexpr const char *const VORNAMEN_DE[] = {
        "Max", "Anna", "Felix", "Laura", "Paul", "Sophie", "Leon", "Marie",
        "Jonas", "Emma", "Lukas", "Mia", "Tim", "Hannah", "Ben", "Lea",
        "Finn", "Lena", "Noah", "Julia", "Elias", "Sarah", "Luis", "Lisa",
        "Julian", "Katharina", "David", "Jana", "Moritz", "Melanie",
        "Niklas", "Christina", "Tom", "Stefanie", "Erik", "Nicole",
        "Alexander", "Sandra", "Maximilian", "Claudia", "Sebastian", "Andrea",
        "Florian", "Petra", "Daniel", "Monika", "Michael", "Sabine",
        "Thomas", "Martina"};
    static constexpr const char *const VORNAMEN_EN[] = {
        "Oliver", "Amelia", "George", "Olivia", "Noah", "Isla", "Arthur", "Emily",
        "Leo", "Ava", "Oscar", "Sophia", "Charlie", "Mia", "Jack", "Grace",
        "Theo", "Lily", "Henry", "Ella", "Harry", "Freya", "Thomas", "Sienna",
        "Jacob", "Charlotte", "William", "Poppy", "Alexander", "Evie",
        "James", "Daisy", "Tommy", "Phoebe", "Edward", "Alice",
        "Lucas", "Maya", "Mason", "Willow", "Daniel", "Rosie",
        "Logan", "Harper", "Joshua", "Ivy", "Michael", "Sophie",
        "Benjamin", "Florence"};
    static constexpr int VORNAMEN_COUNT = 50;

    static constexpr const char *const NACHNAMEN_DE[] = {
        "Mueller", "Schmidt", "Schneider", "Fischer", "Weber", "Meyer", "Wagner",
        "Becker", "Hoffmann", "Richter", "Klein", "Wolf", "Schroeder", "Neumann",
        "Schwarz", "Zimmermann", "Braun", "Krueger", "Hofmann", "Lange",
        "Schmitt", "Werner", "Schmitz", "Krause", "Meier", "Lehmann", "Schmid",
        "Schulze", "Maier", "Koehler", "Herrmann", "Koenig", "Walter", "Mayer",
        "Huber", "Kaiser", "Fuchs", "Peters", "Lang", "Scholz"};
    static constexpr const char *const NACHNAMEN_EN[] = {
        "Smith", "Jones", "Williams", "Brown", "Taylor", "Davies", "Wilson",
        "Evans", "Thomas", "Roberts", "Johnson", "Walker", "Robinson", "Wood",
        "Thompson", "White", "Watson", "Jackson", "Wright", "Green",
        "Harris", "Cooper", "King", "Lee", "Martin", "Clarke", "James",
        "Morgan", "Hughes", "Edwards", "Hill", "Moore", "Clark", "Harrison",
        "Lewis", "Turner", "Hall", "Patel", "Mitchell", "Wells"};
    static constexpr int NACHNAMEN_COUNT = 40;

    static constexpr const char *const STAEDTE_DE[] = {
        "Berlin", "Hamburg", "Muenchen", "Koeln", "Frankfurt",
        "Stuttgart", "Duesseldorf", "Leipzig", "Dortmund", "Essen",
        "Bremen", "Dresden", "Hannover", "Nuernberg", "Duisburg",
        "Bochum", "Wuppertal", "Bielefeld", "Bonn", "Muenster"};
    static constexpr const char *const STAEDTE_EN[] = {
        "London", "Manchester", "Birmingham", "Liverpool", "Leeds",
        "Sheffield", "Bristol", "Newcastle", "Edinburgh", "Glasgow",
        "Cardiff", "Belfast", "Brighton", "Oxford", "Cambridge",
        "Nottingham", "Leicester", "Coventry", "Reading", "Aberdeen"};
    static constexpr int STAEDTE_COUNT = 20;

    static constexpr const char *const PLZ_PREFIX_DE[] = {
        "10", "20", "80", "50", "60", "70", "40", "04", "44", "45",
        "28", "01", "30", "90", "47", "44", "42", "33", "53", "48"};
    static constexpr const char *const PLZ_PREFIX_EN[] = {
        "SW1", "M1", "B1", "L1", "LS1", "S1", "BS1", "NE1", "EH1", "G1",
        "CF1", "BT1", "BN1", "OX1", "CB1", "NG1", "LE1", "CV1", "RG1", "AB1"};

    static constexpr const char *const STRASSEN_DE[] = {
        "Hauptstrasse", "Bahnhofstrasse", "Schulstrasse", "Gartenstrasse",
        "Berliner Strasse", "Dorfstrasse", "Kirchstrasse", "Waldstrasse",
        "Ringstrasse", "Bergstrasse", "Lindenstrasse", "Muehlweg",
        "Am Markt", "Feldstrasse", "Industriestrasse", "Rosenweg",
        "Schillerstrasse", "Goethestrasse", "Mozartstrasse", "Parkstrasse"};
    static constexpr const char *const STRASSEN_EN[] = {
        "High Street", "Church Road", "Station Road", "Park Lane",
        "Victoria Street", "Mill Lane", "Queens Road", "King Street",
        "Manor Road", "Albert Road", "George Street", "Green Lane",
        "The Avenue", "Main Street", "London Road", "School Lane",
        "New Road", "North Street", "Oxford Road", "York Street"};
    static constexpr int STRASSEN_COUNT = 20;

    static constexpr const char *const FIRMEN_DE[] = {
        "Siemens AG", "Deutsche Bank", "Bosch GmbH", "SAP SE", "Volkswagen AG",
        "Allianz SE", "BASF SE", "BMW AG", "Telekom AG", "Daimler AG",
        "Bayer AG", "Continental AG", "Henkel AG", "Adidas AG", "Infineon AG",
        "TechStart GmbH", "DataSoft AG", "MediCare GmbH", "AutoService GmbH",
        "Elektro Klein"};
    static constexpr const char *const FIRMEN_EN[] = {
        "Tesco PLC", "Barclays Bank", "Rolls-Royce Ltd", "BT Group", "BP PLC",
        "HSBC Holdings", "Unilever PLC", "Vodafone Group", "Shell PLC", "Diageo PLC",
        "GlaxoSmithKline", "Aviva PLC", "Sainsbury's", "BAE Systems", "ARM Holdings",
        "TechStart Ltd", "DataSoft Ltd", "MediCare Ltd", "AutoService Ltd",
        "Smith Electrics"};
    static constexpr int FIRMEN_COUNT = 20;

    // Sprachabhängige Auswahl
    const bool en = (I18n::currentLang() == LangIndex::EN);
    const char *const *VORNAMEN    = en ? VORNAMEN_EN    : VORNAMEN_DE;
    const char *const *NACHNAMEN   = en ? NACHNAMEN_EN   : NACHNAMEN_DE;
    const char *const *STAEDTE     = en ? STAEDTE_EN     : STAEDTE_DE;
    const char *const *STRASSEN    = en ? STRASSEN_EN    : STRASSEN_DE;
    const char *const *FIRMEN      = en ? FIRMEN_EN      : FIRMEN_DE;
    const char *const *PLZ_PREFIX  = en ? PLZ_PREFIX_EN  : PLZ_PREFIX_DE;

    // Sprachabhängige Formatzeichenfolgen und Werte
    const char *const phoneHomeFmt   = en ? "+44 %d %07d"        : "+49 %d %07d";
    const char *const phoneMobileFmt = en ? "+44 7%d %08d"       : "+49 1%d %08d";
    const char *const phoneWorkFmt   = en ? "+44 %d %07d ext %d" : "+49 %d %07d-%d";
    const char *const emailFmt       = en ? "%s.%s@example.com"  : "%s.%s@beispiel.de";
    const char *const postalFmt      = en ? "%s %03d"            : "%s%03d";
    const char *const countryName    = en ? "United Kingdom"     : "Deutschland";

    // Aktuelles Datum direkt ins Ziel schreiben (thread-sicher)
    fillCurrentTime(outData->today);

    outData->timestamp = millis();
    outData->contactCount = 0;
    outData->stats = ContactStats{};

    // Demo-Kontakte generieren (max. 100)
    constexpr int demoCount = (ContactsCfg::DEMO_CONTACT_COUNT > 100) ? 100 : ContactsCfg::DEMO_CONTACT_COUNT;
    for (int i = 0; i < demoCount; ++i)
    {
        ContactItem &item = outData->contacts[outData->contactCount];
        item.reset();

        // Indizes berechnen
        const int vi = i % VORNAMEN_COUNT;
        const int ni = (i * 7) % NACHNAMEN_COUNT;
        const int si = (i * 3) % STAEDTE_COUNT;
        const int sti = (i * 11) % STRASSEN_COUNT;
        const int fi = (i * 13) % FIRMEN_COUNT;

        // ID
        stringutils::safeFormat(item.id, "demo-%03d", i);

        // Name
        stringutils::safeCopy(item.firstName, VORNAMEN[vi]);
        if (i == 0)
        {
            // Geburtstagskind bekommt "A"-Nachname für erste Position in sortierter Liste
            stringutils::safeCopy(item.lastName, "Adam");
        }
        else
        {
            stringutils::safeCopy(item.lastName, NACHNAMEN[ni]);
        }
        formatDisplayName(item);

        // Firma
        stringutils::safeCopy(item.company, FIRMEN[fi]);

        // Telefonnummern
        stringutils::safeFormat(item.phoneHome, phoneHomeFmt, 30 + (i % 70), 1000000 + i * 7919);
        stringutils::safeFormat(item.phoneMobile, phoneMobileFmt, 50 + (i % 30), 10000000 + i * 6131);
        stringutils::safeFormat(item.phoneWork, phoneWorkFmt, 30 + ((i + 5) % 70), 2000000 + i * 3571, 100 + (i % 900));

        // E-Mail
        stringutils::safeFormat(item.email, emailFmt, VORNAMEN[vi], NACHNAMEN[ni]);

        // Lowercase
        for (char *c = item.email; *c; ++c)
        {
            if (*c >= 'A' && *c <= 'Z')
                *c = static_cast<char>(*c + 32);
        }

        // Adresse
        stringutils::safeFormat(item.address.street, "%s %d", STRASSEN[sti], 1 + (i * 3) % 150);
        stringutils::safeFormat(item.address.postalCode, postalFmt, PLZ_PREFIX[si], 100 + (i * 7) % 900);
        stringutils::safeCopy(item.address.city, STAEDTE[si]);
        stringutils::safeCopy(item.address.country, countryName);

        // Geburtstag
        if (i == 0)
        {
            item.birthdayDay = static_cast<uint8_t>(outData->today.currentDay);
            item.birthdayMonth = static_cast<uint8_t>(outData->today.currentMonth);
            item.birthdayYear = 1985;
        }
        else
        {
            item.birthdayDay = static_cast<uint8_t>(1 + (i * 7) % 28);
            item.birthdayMonth = static_cast<uint8_t>(1 + (i * 3) % 12);
            item.birthdayYear = static_cast<uint16_t>(1955 + (i * 41) % 50);
        }

        // Sort-Key und Geburtstags-Check
        item.sortKey = getSortKey(item.lastName);
        item.hasBirthdayToday = checkBirthdayToday(item, outData->today);

        // Statistiken
        outData->stats.total++;
        outData->stats.withBirthday++;

        if (item.hasBirthdayToday)
            outData->stats.birthdaysToday++;

        outData->contactCount++;
    }

    LOG_INFO_F("Demo-Daten generiert: %d Kontakte, %d Geburtstage heute", outData->contactCount, outData->stats.birthdaysToday);
}

// ===================================================================
// HILFSFUNKTIONEN
// ===================================================================

/**
 * @brief Füllt Datumsdaten thread-sicher direkt ins Ziel
 */
void Contacts::fillCurrentTime(ContactsTodayData& out)
{
    struct tm timeinfo = DS1307_RTC::getCurrentLocalTime();
    out.currentYear = timeinfo.tm_year + 1900;
    out.currentMonth = timeinfo.tm_mon + 1;
    out.currentDay = timeinfo.tm_mday;
}

/**
 * @brief Aktualisiert das aktuelle Datum von der RTC
 */
void Contacts::updateCurrentTime()
{
    ContactsData *data = getActiveBuffer();
    if (!data)
        return;

    fillCurrentTime(data->today);
}

/**
 * @brief Ermittelt den Sortierschlüssel für einen Namen
 */
char Contacts::getSortKey(const char *lastName) const
{
    if (!lastName || lastName[0] == '\0')
        return '?';

    unsigned char c = static_cast<unsigned char>(lastName[0]);

    // UTF-8 Umlaute behandeln
    if (c == 0xC3 && lastName[1])
    {
        switch (static_cast<unsigned char>(lastName[1]))
        {
        case 0x84: // Ä
        case 0xA4: // ä
            return 'A';
        case 0x96: // Ö
        case 0xB6: // ö
            return 'O';
        case 0x9C: // Ü
        case 0xBC: // ü
            return 'U';
        }
    }

    // kleine Buchstaben in Großbuchstaben umwandeln
    if (c >= 'a' && c <= 'z')
        return static_cast<char>(c - 32);

    // Großbuchstaben unverändert zurückgeben
    if (c >= 'A' && c <= 'Z')
        return static_cast<char>(c);

    return '?';
}

/**
 * @brief Formatiert den Anzeigenamen im Format "Nachname, Vorname"
 */
void Contacts::formatDisplayName(ContactItem &contact)
{
    if (contact.lastName[0] != '\0' && contact.firstName[0] != '\0')
        stringutils::safeFormat(contact.displayName, "%s, %s", contact.lastName, contact.firstName);
    else if (contact.lastName[0] != '\0')
        stringutils::safeCopy(contact.displayName, contact.lastName);
    else if (contact.firstName[0] != '\0')
        stringutils::safeCopy(contact.displayName, contact.firstName);
    else
        stringutils::safeCopy(contact.displayName, "(Unbekannt)");
}

/**
 * @brief Prüft ob der Kontakt heute Geburtstag hat
 */
bool Contacts::checkBirthdayToday(ContactItem &contact, const ContactsTodayData &today) const
{
    if (contact.birthdayDay == 0 || contact.birthdayMonth == 0)
        return false;

    // Normaler Vergleich
    if (contact.birthdayDay == today.currentDay && contact.birthdayMonth == today.currentMonth)
        return true;

    // Sonderfall: Geburtstag am 29. Februar
    if (contact.birthdayDay == 29 && contact.birthdayMonth == 2)
    {
        const int year = today.currentYear;
        const bool isLeapYear = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));

        // In Nicht-Schaltjahren am 28. Februar feiern
        if (!isLeapYear && today.currentDay == 28 && today.currentMonth == 2)
            return true;
    }

    return false;
}

/**
 * @brief Sortiert die Kontakte alphabetisch
 */
void Contacts::sortContacts(ContactsData *data)
{
    if (!data || data->contactCount <= 1)
        return;

    std::sort(data->contacts, data->contacts + data->contactCount,
              [](const ContactItem &a, const ContactItem &b)
              {
                  if (a.sortKey != b.sortKey)
                      return a.sortKey < b.sortKey;
                  return strcasecmp(a.displayName, b.displayName) < 0;
              });

    LOG_INFO("Kontakte sortiert");
}

/**
 * @brief Erstellt den Alphabet-Index für Schnellnavigation
 */
void Contacts::buildAlphabetIndex(ContactsData *data)
{
    if (!data)
        return;

    for (int i = 0; i < ContactsCfg::ALPHABET_ENTRIES; ++i)
        data->alphabetIndex[i] = -1;

    for (int i = 0; i < data->contactCount; ++i)
    {
        const char key = data->contacts[i].sortKey;
        int index = -1;

        if (key >= 'A' && key <= 'Z')
            index = key - 'A';

        if (index >= 0 && index < ContactsCfg::ALPHABET_ENTRIES)
        {
            if (data->alphabetIndex[index] == -1)
                data->alphabetIndex[index] = i;
        }
    }

    LOG_INFO("Alphabet-Index aufgebaut");
}

/**
 * @brief Konvertiert Alphabet-Index zu Buchstabe
 */
char Contacts::getLetterFromAlphabetIndex(int index) const
{
    if (index < 0 || index >= ContactsCfg::ALPHABET_ENTRIES)
        return '?';

    return static_cast<char>('A' + index);
}

/**
 * @brief Ermittelt Alphabet-Index aus Y-Position
 */
int Contacts::getAlphabetIndexFromY(int32_t y) const
{
    if (y < ALPHABET_Y_OFFSET || y >= ALPHABET_Y_OFFSET + ALPHABET_TOTAL_HEIGHT)
        return -1;

    return (y - ALPHABET_Y_OFFSET) / ALPHABET_ENTRY_HEIGHT;
}

// ===================================================================
// WILDCARD-SUCHE
// ===================================================================

/**
 * @brief Prüft ob Text zum Wildcard-Muster passt (* und ? unterstützt)
 */
bool Contacts::matchesWildcard(const char *text, const char *pattern) const
{
    if (!text || !pattern)
        return false;

    if (pattern[0] == '\0')
        return true;

    // Sicherer tolower: Cast auf unsigned char verhindert UB bei UTF-8
    auto lower = [](char c) { return static_cast<char>(tolower(static_cast<unsigned char>(c))); };

    const char *t = text;
    const char *p = pattern;
    const char *starPos = nullptr;
    const char *matchPos = nullptr;

    while (*t)
    {
        if (*p && (lower(*p) == lower(*t) || *p == '?'))
        {
            ++t;
            ++p;
        }
        else if (*p == '*')
        {
            starPos = p++;
            matchPos = t;
        }
        else if (starPos)
        {
            p = starPos + 1;
            t = ++matchPos;
        }
        else
        {
            return false;
        }
    }

    while (*p == '*')
        ++p;

    return *p == '\0';
}

/**
 * @brief Prüft ob ein Kontakt nach aktuellem Filter sichtbar ist
 */
bool Contacts::isContactVisible(const ContactItem &contact) const
{
    if (!m_searchState.isActive || m_searchState.searchPattern[0] == '\0')
        return true;

    return matchesWildcard(contact.lastName, m_searchState.searchPattern);
}

/**
 * @brief Baut die Zuordnung der sichtbaren Kontakte neu auf.
 */
void Contacts::rebuildVisibleIndices()
{
    m_visibleCount = 0;

    // Alphabet-Index für sichtbare Kontakte initialisieren
    for (int i = 0; i < ContactsCfg::ALPHABET_ENTRIES; ++i)
        m_visibleAlphabetIndex[i] = -1;

    const int bufferIndex = m_activeBufferIndex.load();
    const ContactsData *data = &m_dataBuffers[bufferIndex];

    if (!data || !m_hasValidData)
        return;

    // Sichtbare Kontakte sammeln und Alphabet-Index aufbauen
    for (int i = 0; i < data->contactCount; ++i)
    {
        if (isContactVisible(data->contacts[i]))
        {
            m_visibleIndices[m_visibleCount] = i;

            // Alphabet-Index für sichtbare Kontakte (erste Zeile je Buchstabe)
            const char sortKey = data->contacts[i].sortKey;
            if (sortKey >= 'A' && sortKey <= 'Z')
            {
                const int letterIndex = sortKey - 'A';
                if (m_visibleAlphabetIndex[letterIndex] == -1)
                    m_visibleAlphabetIndex[letterIndex] = m_visibleCount;
            }

            ++m_visibleCount;
        }
    }

    // Scroll-Parameter aktualisieren
    m_scrollState.contentHeight = m_visibleCount * CONTACT_HEIGHT;
    m_scrollState.maxScroll = std::max(0, m_scrollState.contentHeight - VIEWPORT_HEIGHT);

    // Scroll-Position begrenzen
    if (m_scrollState.scrollOffset > m_scrollState.maxScroll)
        m_scrollState.scrollOffset = m_scrollState.maxScroll;

    LOG_INFO_F("Visible-Mapping: %d von %d Kontakten sichtbar", m_visibleCount, data->contactCount);
}

/**
 * @brief Scrollt die Liste zu einem bestimmten Buchstaben (O(1) via visibleAlphabetIndex)
 */
void Contacts::scrollToLetter(char letter)
{
    if (m_visibleCount == 0)
        return;

    // Buchstabe normalisieren
    char normalizedLetter = letter;
    if (letter >= 'a' && letter <= 'z')
        normalizedLetter = static_cast<char>(letter - 32);

    m_selectedLetter = normalizedLetter;

    // Ungültiger Buchstabe
    if (normalizedLetter < 'A' || normalizedLetter > 'Z')
        return;

    // Suche exakten Buchstaben oder nächsten verfügbaren (O(26) worst case)
    int visibleRow = -1;
    for (int i = normalizedLetter - 'A'; i < ContactsCfg::ALPHABET_ENTRIES; ++i)
    {
        if (m_visibleAlphabetIndex[i] >= 0)
        {
            visibleRow = m_visibleAlphabetIndex[i];
            break;
        }
    }

    if (visibleRow >= 0)
    {
        m_scrollState.scrollOffset = visibleRow * CONTACT_HEIGHT;
        constrainScroll();
        m_listSpriteValid = false;
        LOG_INFO_F("Springe zu Buchstabe '%c' (Zeile %d)", normalizedLetter, visibleRow);
    }
}

// ===================================================================
// HAUPTZEICHENFUNKTION
// ===================================================================

/**
 * @brief Hauptzeichenfunktion - wird zyklisch aufgerufen
 */
void Contacts::draw()
{
    if (!m_initialized)
        return;

    // Sprachwechsel? -> komplette Anzeige neu aufbauen
    if (I18n::consumeLanguageChanged())
        redrawAll();

    // ProgressBox Animation
    if (m_progressBox && m_progressBox->isActive())
    {
        m_progressBox->updateAnimation();

        if (m_fetchCompleted.exchange(false))
        {
            m_progressBox->finish();
            LOG_WARNING("Refresh fehlgeschlagen - ProgressBox geschlossen");
        }
    }

    // Touch-Handling
    int32_t tx, ty;
    const bool isTouched = m_tft->getTouch(&tx, &ty);

    // Prüfe auf neue Daten vom Worker
    if (m_dataReady.exchange(false))
    {
        m_hasValidData = true;
        m_listSpriteValid = false;
        m_detailSpriteValid = false;

        DS1307_RTC *rtc = &APP_STATE.getRTC();
        DateTime dt;
        if (rtc->getDateTime(dt))
            stringutils::safeFormat(m_lastUpdateTimestamp, "%02d:%02d", dt.hour, dt.minute);

        m_lastApiCall = millis();

        if (m_progressBox && m_progressBox->isActive())
            m_progressBox->finish();

        // Sichtbarkeits-Mapping aufbauen (setzt auch contentHeight und maxScroll)
        rebuildVisibleIndices();
        constrainScroll();

        // Versuche vorherige Auswahl via ID wiederherzustellen
        const int bufferIndex = m_activeBufferIndex.load();
        const ContactsData *data = &m_dataBuffers[bufferIndex];

        int restoredIndex = -1;
        if (m_selectedContactId[0] != '\0')
        {
            // Suche Kontakt mit gespeicherter ID in sichtbaren Kontakten
            for (int row = 0; row < m_visibleCount; ++row)
            {
                const int idx = m_visibleIndices[row];
                if (strcmp(data->contacts[idx].id, m_selectedContactId) == 0)
                {
                    restoredIndex = idx;
                    break;
                }
            }
        }

        if (restoredIndex >= 0)
        {
            m_selectedContactIndex = restoredIndex;
            LOG_INFO_F("Auswahl wiederhergestellt: '%s'", data->contacts[restoredIndex].displayName);
        }
        else if (m_visibleCount > 0)
        {
            // Fallback: ersten sichtbaren Kontakt auswählen
            m_selectedContactIndex = m_visibleIndices[0];
            stringutils::safeCopy(m_selectedContactId, data->contacts[m_selectedContactIndex].id);
        }
        else
        {
            m_selectedContactIndex = -1;
            m_selectedContactId[0] = '\0';
        }

        LOG_INFO("Neue Kontakt-Daten von Worker übernommen");
    }

    // Periodische Aktualisierung
    const unsigned long now = millis();
    const bool intervalDue = (m_lastApiCall == 0) || (now - m_lastApiCall > ContactsCfg::API_CALL_INTERVAL_MS);
    // Blocke Trigger solange neue Daten pending sind (verhindert Buffer-Overwrite-Race)
    const bool canTrigger = !m_dataReady.load() &&
                            !m_refreshRequested.load() &&
                            !m_inFlight.load() &&
                            (now >= m_nextAllowedFetch.load());

    if (intervalDue && canTrigger)
        requestRefresh(false);

    if (!m_layoutDrawn)
    {
        // Ladescreen: Cancel-Button
        if (m_cancelLoadButton.isPressed() && !isTouched)
        {
            m_cancelLoadButton.press(false);

            const int textY = (m_tft->height() / 2) - 25;
            m_tft->fillRect(0, textY - 20, m_tft->width(), 40, Theme::ColorMainBackGround);
            m_tft->loadFont(LeagueGothicCond34);
            m_tft->setTextColor(0xD69A, Theme::ColorMainBackGround);
            m_tft->setTextDatum(MC_DATUM);
            m_tft->drawString(STR(ContactsKey::CONTACTS_LOADING_CANCEL), m_tft->width() / 2, textY);
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

        if (m_hasValidData)
        {
            drawLayout();
            m_layoutDrawn = true;
        }
    }
    else
    {
        // ===================================================================
        // HAUPTANSICHT
        // ===================================================================

        // MessageBox aktualisieren
        if (m_messageBox && (m_messageBox->isActive() || m_messageBox->isBusy()))
        {
            m_messageBox->update();

            if (m_messageBox->isBusy())
            {
                m_messageBox->completeConfirmation(true);
                return;
            }

            return;
        }

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

        // Suchen-Button
        if (m_searchButton.isPressed() && !isTouched)
        {
            m_searchButton.press(false);
            handleSearchButtonTap();
        }
        else if (isTouched && isSearchButtonTouched(tx, ty))
        {
            if (!m_searchButton.isPressed())
            {
                m_searchButton.press(true);
                m_searchButton.drawButton(true);
            }
        }
        else if (m_searchButton.isPressed())
        {
            m_searchButton.press(false);
            m_searchButton.drawButton(false);
        }

        // Reset-Button (nur wenn aktiviert)
        if (m_resetButton.isEnabled())
        {
            if (m_resetButton.isPressed() && !isTouched)
            {
                m_resetButton.press(false);
                handleResetButtonTap();
            }
            else if (isTouched && isResetButtonTouched(tx, ty))
            {
                if (!m_resetButton.isPressed())
                {
                    m_resetButton.press(true);
                    m_resetButton.drawButton(true);
                }
            }
            else if (m_resetButton.isPressed())
            {
                m_resetButton.press(false);
                m_resetButton.drawButton(false);
            }
        }

        // Refresh-Button
        if (m_refreshButton.isPressed() && !isTouched)
        {
            m_refreshButton.press(false);
            m_refreshButton.drawButton(false); // Button visuell zurücksetzen
            handleRefreshButtonTap();
        }
        else if (isTouched && isRefreshButtonTouched(tx, ty))
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

        // Tap auf die Alphabet-Leiste
        if (isAlphabetBarTouched(tx, ty))
        {
            if (isTouched && !m_alphabetTouchActive)
            {
                m_alphabetTouchActive = true;
                handleAlphabetTap(tx, ty);
            }
        }

        if (!isTouched)
            m_alphabetTouchActive = false;

        // Kontaktliste Touch/Scroll
        const bool touchOnSearchBtn = isSearchButtonTouched(tx, ty);
        const bool touchOnResetBtn = isResetButtonTouched(tx, ty);
        const bool touchOnRefreshBtn = isRefreshButtonTouched(tx, ty);
        const bool touchOnExitBtn = m_exitButton.contains(tx, ty);
        const bool touchOnAlphabet = isAlphabetBarTouched(tx, ty);
        const bool touchInList = (tx >= LIST_X && tx < LIST_X + LIST_WIDTH);

        if (isTouched && touchInList && !touchOnExitBtn && !touchOnSearchBtn &&
            !touchOnResetBtn && !touchOnRefreshBtn && !touchOnAlphabet)
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

            if (m_tapState.isTap && m_tapState.startX > 0)
            {
                int contactIndex = getContactAtPosition(m_tapState.startX, m_tapState.startY);
                if (contactIndex >= 0)
                {
                    LOG_INFO_F("Tap erkannt bei (%ld, %ld) -> Kontakt %d", m_tapState.startX, m_tapState.startY, contactIndex);
                    handleContactTap(contactIndex);
                }
            }
            m_tapState.reset();
        }
        else if (!isTouched)
        {
            if (m_tapState.isTap && m_tapState.startX > 0)
            {
                int contactIndex = getContactAtPosition(m_tapState.startX, m_tapState.startY);
                if (contactIndex >= 0)
                {
                    LOG_INFO_F("Tap erkannt bei (%ld, %ld) -> Kontakt %d", m_tapState.startX, m_tapState.startY, contactIndex);
                    handleContactTap(contactIndex);
                }
            }

            m_tapState.reset();
        }

        updateScrollPhysics();

        // Sprites aktualisieren
        if (!m_listSpriteValid || m_scrollState.scrollOffset != m_lastScrollOffset)
        {
            drawContactList();
            drawScrollbar();
            drawAlphabetBar();
        }

        if (!m_detailSpriteValid)
            drawContactDetail();

        drawDigitalClock(625, (m_tft->height() - m_clockSprite->height()) + 1);

        // Progress-Bar für nächsten Refresh
        if (m_progressSprite)
        {
            const unsigned long elapsedTime = millis() - m_lastApiCall;
            float progress = 1.0f - (static_cast<float>(elapsedTime) / static_cast<float>(ContactsCfg::API_CALL_INTERVAL_MS));
            progress = std::max(0.0f, std::min(1.0f, progress));

            const int barHeight = static_cast<int>(m_tft->height() * progress);
            const int startY = m_tft->height() - barHeight;

            m_progressSprite->drawFastVLine(0, 0, startY, Theme::ColorMainBackGround);
            m_progressSprite->drawFastVLine(0, startY, barHeight, COLOR_PROGRESS_BAR);
            m_progressSprite->pushSprite(m_tft->width() - 1, 0);
        }
    }
}

// ===================================================================
// LAYOUT-ZEICHENFUNKTIONEN
// ===================================================================

/**
 * @brief Zeichnet das komplette Layout
 */
void Contacts::drawLayout()
{
    m_tft->clear(Theme::ColorMainBackGround);

    // Reset-Button Status basierend auf Suchfilter
    if (m_searchState.isActive)
        m_resetButton.enable();
    else
        m_resetButton.disable();

    m_exitButton.drawButton(false);
    m_searchButton.drawButton(false);
    m_resetButton.drawButton(false);
    m_refreshButton.drawButton(false);

    drawAlphabetBar();
    drawContactList();
    drawScrollbar();
    drawContactDetail();

    LOG_INFO("Layout gezeichnet");
}

/**
 * @brief Zeichnet die gesamte Anzeige nach einem Sprachwechsel neu
 *
 * Aktualisiert die Beschriftungen der Aktions-Buttons und erzeugt im Demomodus die Demokontakte einschließlich Telefonvorwahl und E-Mail-Domain in der aktiven Sprache neu. Anschließend wird im Ladezustand der zentrierte Ladetext mit Abbruch-Button oder andernfalls das vollständige Hauptlayout gezeichnet.
 */
void Contacts::redrawAll()
{
    if (!m_tft)
        return;

    // Schaltflächenbeschriftungen vor dem Zeichnen in der neuen Sprache übernehmen
    m_searchButton.setLabelText(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_BTN_SEARCH)));
    m_resetButton.setLabelText(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_BTN_RESET)));
    m_refreshButton.setLabelText(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_BTN_REFRESH)));

    // ProgressBox-Texte ebenfalls aktualisieren
    if (m_progressBox)
    {
        m_progressBox->setTitle(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_PROGRESS_TITLE)));
        m_progressBox->setMessage(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_PROGRESS_MESSAGE)));
    }

    // Im Demomodus müssen die Demodaten in der neuen Sprache neu erzeugt werden. Der Worker schreibt in den inaktiven Buffer, sortiert, baut den Alphabet-Index und tauscht threadsicher. Direkt aus dem UI-Thread wäre eine Race-Condition mit einem laufenden Worker-Fetch. requestRefresh(true) ignoriert das Rate-Limit.
    if (m_demoMode)
        requestRefresh(true);

    if (m_layoutDrawn)
    {
        // Hauptlayout: Sprite-Caches invalidieren, dann drawLayout()
        m_listSpriteValid = false;
        m_detailSpriteValid = false;
        m_lastScrollOffset = -1;
        m_lastDrawnSecond = -1;

        drawLayout();
    }
    else
    {
        // Ladescreen: zentrierter Lade-Text plus Cancel-Button
        m_tft->clear(Theme::ColorMainBackGround);
        m_tft->loadFont(LeagueGothicCond34);
        m_tft->setTextColor(COLOR_LOADING_TEXT, Theme::ColorMainBackGround);
        m_tft->setTextDatum(MC_DATUM);
        m_tft->drawString(STR(ContactsKey::CONTACTS_LOADING_TITLE), m_tft->width() / 2, (m_tft->height() / 2) - 25);
        m_tft->unloadFont();
        m_cancelLoadButton.drawButton(false);
    }
}

/**
 * @brief Zeichnet die Alphabet-Leiste für Schnellnavigation
 */
void Contacts::drawAlphabetBar()
{
    if (!m_alphabetSprite)
        return;

    m_alphabetSprite->clear(COLOR_ALPHABET_BG);
    m_alphabetSprite->loadFont(ComfortaaBold14);

    // Buffer-Index einmalig lesen
    const int bufferIndex = m_activeBufferIndex.load();
    const ContactsData *data = &m_dataBuffers[bufferIndex];

    // Aktiven Buchstaben ermitteln (über Sichtbarkeits-Mapping)
    char scrollKey = '?';
    if (data && m_visibleCount > 0 && m_scrollState.contentHeight > 0)
    {
        // topRow ist Index im sichtbaren Array, nicht im Original
        const int topRow = m_scrollState.scrollOffset / CONTACT_HEIGHT;
        if (topRow >= 0 && topRow < m_visibleCount)
        {
            const int contactIndex = m_visibleIndices[topRow];
            scrollKey = data->contacts[contactIndex].sortKey;
        }
    }

    char activeKey = scrollKey;
    if (m_selectedLetter >= 'A' && m_selectedLetter <= 'Z')
    {
        if (m_selectedLetter >= scrollKey)
            activeKey = m_selectedLetter;
    }

    for (int i = 0; i < ContactsCfg::ALPHABET_ENTRIES; ++i)
    {
        const int y = ALPHABET_Y_OFFSET + i * ALPHABET_ENTRY_HEIGHT;
        const char letter = getLetterFromAlphabetIndex(i);
        char letterStr[2] = {letter, '\0'};

        // hasContacts basiert auf SICHTBAREN Kontakten (nicht ungefilterten)
        bool hasContacts = (m_visibleAlphabetIndex[i] >= 0);
        bool isActive = (letter == activeKey);

        uint16_t textColor = hasContacts ? COLOR_ALPHABET_TEXT : 0x4208;

        if (isActive && hasContacts)
        {
            textColor = COLOR_ALPHABET_ACTIVE;
            m_alphabetSprite->fillRect(0, y, ALPHABET_BAR_WIDTH, ALPHABET_ENTRY_HEIGHT, 0x2945);
        }

        m_alphabetSprite->setTextColor(textColor);
        m_alphabetSprite->setTextDatum(MC_DATUM);
        m_alphabetSprite->drawString(letterStr, ALPHABET_BAR_WIDTH / 2,
                                     y + ALPHABET_ENTRY_HEIGHT / 2);
    }

    m_alphabetSprite->unloadFont();
    m_alphabetSprite->pushSprite(ALPHABET_BAR_X, 0);
}

/**
 * @brief Zeichnet die Kontaktliste
 */
void Contacts::drawContactList()
{
    if (!m_listSprite || !m_hasValidData)
        return;

    m_listSprite->clear(Theme::ColorMainBackGround);
    m_listSprite->loadFont(ComfortaaBold16);

    const int bufferIndex = m_activeBufferIndex.load();
    const ContactsData *data = &m_dataBuffers[bufferIndex];

    // Berechne erste sichtbare Zeile basierend auf Scroll-Offset Clamp auf >= 0 um Array-Out-of-Bounds bei Overscroll nach oben zu verhindern
    const int rawFirstRow = m_scrollState.scrollOffset / CONTACT_HEIGHT;
    const int firstVisibleRow = std::max(0, rawFirstRow);
    // yOffset berechnen: Bei Overscroll (scrollOffset < 0) wird die Liste nach unten verschoben
    const int yOffset = -m_scrollState.scrollOffset + firstVisibleRow * CONTACT_HEIGHT;

    // Iteriere nur über sichtbare Kontakte via Mapping
    for (int row = firstVisibleRow; row < m_visibleCount; ++row)
    {
        const int yPos = yOffset + (row - firstVisibleRow) * CONTACT_HEIGHT;

        // Außerhalb des Viewports -> fertig
        if (yPos >= VIEWPORT_HEIGHT)
            break;

        const int contactIndex = m_visibleIndices[row];
        const ContactItem &contact = data->contacts[contactIndex];

        // Highlight für ausgewählten Kontakt
        if (contactIndex == m_selectedContactIndex)
            m_listSprite->fillRect(0, yPos, LIST_WIDTH, CONTACT_HEIGHT, COLOR_SELECTED);

        // Geburtstags-Badge
        if (contact.hasBirthdayToday)
            drawBirthdayBadge(LIST_WIDTH - BIRTHDAY_BADGE_SIZE - 8, yPos + (CONTACT_HEIGHT - BIRTHDAY_BADGE_SIZE) / 2);

        // Name mit Ellipse kürzen falls nötig
        int maxNameWidth = LIST_WIDTH;
        if (contact.hasBirthdayToday)
            maxNameWidth -= BIRTHDAY_BADGE_SIZE + 10;

        char truncatedName[LINE_BUFFER_SIZE];
        truncateTextToWidth(contact.displayName, maxNameWidth, m_listSprite, truncatedName, sizeof(truncatedName));

        m_listSprite->setTextColor(COLOR_CONTACT_NAME);
        m_listSprite->setTextDatum(ML_DATUM);
        m_listSprite->drawString(truncatedName, LIST_PADDING, yPos + CONTACT_HEIGHT / 2);

        // Trennlinie
        m_listSprite->drawFastHLine(0, yPos + CONTACT_HEIGHT - 1, LIST_WIDTH, COLOR_SEPARATOR);
    }

    // Leere Liste anzeigen
    if (m_visibleCount == 0)
    {
        m_listSprite->setTextColor(COLOR_NO_DATA);
        m_listSprite->setTextDatum(MC_DATUM);

        if (m_searchState.isActive)
            m_listSprite->drawString(STR(ContactsKey::CONTACTS_NO_RESULTS), LIST_WIDTH / 2, VIEWPORT_HEIGHT / 2);
        else
            m_listSprite->drawString(STR(ContactsKey::CONTACTS_NO_CONTACTS), LIST_WIDTH / 2, VIEWPORT_HEIGHT / 2);
    }

    m_listSprite->unloadFont();
    m_listSprite->pushSprite(LIST_X, 0);

    m_listSpriteValid = true;
    m_lastScrollOffset = m_scrollState.scrollOffset;
}

/**
 * @brief Zeichnet das Geburtstags-Badge
 */
void Contacts::drawBirthdayBadge(int x, int y)
{
    if (!m_listSprite)
        return;

    m_listSprite->fillSmoothCircle(x + BIRTHDAY_BADGE_SIZE / 2, y + BIRTHDAY_BADGE_SIZE / 2, BIRTHDAY_BADGE_SIZE / 2, COLOR_BIRTHDAY_BG);

    const int cx = x + BIRTHDAY_BADGE_SIZE / 2;
    const int cy = y + BIRTHDAY_BADGE_SIZE / 2;

    m_listSprite->fillRect(cx - 1, cy - 1, 3, 9, TFT_WHITE);
    m_listSprite->fillSmoothCircle(cx, cy - 6, 2, COLOR_BIRTHDAY);
}

/**
 * @brief Zeichnet die Kontaktdetails
 */
void Contacts::drawContactDetail()
{
    if (!m_detailSprite)
        return;

    m_detailSprite->clear(Theme::ColorMainBackGround);
    m_detailSprite->fillSmoothRoundRect(0, 0, m_detailSprite->width(), m_detailSprite->height(), 8, 0x1082);

    // Lambda für Statistik-Block
    auto drawStatsBlock = [this]()
    {
        if (!m_hasValidData)
            return;

        const int bufferIndex = m_activeBufferIndex.load();
        const ContactsData *data = &m_dataBuffers[bufferIndex];

        char statsText[64];
        if (m_searchState.isActive)
            STRF(statsText, ContactsKey::CONTACTS_STATS_FILTERED, m_visibleCount, data->contactCount);
        else
            STRF(statsText, ContactsKey::CONTACTS_STATS_TOTAL, data->contactCount);

        const int statsRectY = DETAIL_SPRITE_HEIGHT - 10 - 38;
        const int statsRectHeight = 38;
        m_detailSprite->fillSmoothRoundRect(10, statsRectY, m_detailSprite->width() - 20, statsRectHeight, 8, 0x2104);

        m_detailSprite->loadFont(ComfortaaBold16);
        m_detailSprite->setTextColor(COLOR_STATS);
        m_detailSprite->setTextDatum(MC_DATUM);
        m_detailSprite->drawString(statsText, m_detailSprite->width() / 2, statsRectY + statsRectHeight / 2);
        m_detailSprite->unloadFont();
    };

    if (!m_hasValidData || m_selectedContactIndex < 0)
    {
        m_detailSprite->loadFont(ComfortaaBold16);
        m_detailSprite->setTextColor(COLOR_NO_DATA);
        m_detailSprite->setTextDatum(MC_DATUM);
        m_detailSprite->drawString(STR(ContactsKey::CONTACTS_NO_CONTACTS), DETAIL_WIDTH / 2, m_detailSprite->height() / 2 - 20);
        m_detailSprite->unloadFont();

        drawStatsBlock();

        m_detailSprite->pushSprite(DETAIL_X, DETAIL_Y);
        m_detailSpriteValid = true;
        return;
    }

    const int bufferIndex = m_activeBufferIndex.load();
    const ContactsData *data = &m_dataBuffers[bufferIndex];

    if (m_selectedContactIndex >= data->contactCount)
    {
        m_selectedContactIndex = -1;
        m_selectedContactId[0] = '\0';
        m_detailSpriteValid = true;
        return;
    }

    const ContactItem &contact = data->contacts[m_selectedContactIndex];

    int yPos = 20;

    // Name
    m_detailSprite->fillSmoothRoundRect(10, 10, m_detailSprite->width() - 20, 53, 8, 0x2104);

    m_detailSprite->loadFont(ComfortaaBold30);
    m_detailSprite->setTextColor(Theme::ColorGreen);
    m_detailSprite->setTextDatum(TL_DATUM);

    char truncatedName[LINE_BUFFER_SIZE];
    const int maxNameWidth = DETAIL_WIDTH - 20;
    truncateTextToWidth(contact.displayName, maxNameWidth, m_detailSprite, truncatedName, sizeof(truncatedName));
    m_detailSprite->drawString(truncatedName, DETAIL_PADDING + 3, yPos);

    m_detailSprite->unloadFont();
    yPos += 62;

    m_detailSprite->loadFont(ComfortaaBold16);

    // Firma
    if (contact.company[0] != '\0')
    {
        m_detailSprite->setTextColor(COLOR_LABEL);
        m_detailSprite->drawString(STR(ContactsKey::CONTACTS_LABEL_COMPANY), DETAIL_PADDING, yPos);
        m_detailSprite->setTextColor(COLOR_VALUE);
        m_detailSprite->drawString(contact.company, DETAIL_PADDING + DETAIL_LABEL_WIDTH, yPos);
        yPos += DETAIL_LINE_HEIGHT;
    }

    // Telefon privat
    if (contact.phoneHome[0] != '\0')
    {
        m_detailSprite->setTextColor(COLOR_LABEL);
        m_detailSprite->drawString(STR(ContactsKey::CONTACTS_LABEL_PHONE_HOME), DETAIL_PADDING, yPos);
        m_detailSprite->setTextColor(COLOR_VALUE);
        m_detailSprite->drawString(contact.phoneHome, DETAIL_PADDING + DETAIL_LABEL_WIDTH, yPos);
        yPos += DETAIL_LINE_HEIGHT;
    }

    // Telefon mobil
    if (contact.phoneMobile[0] != '\0')
    {
        m_detailSprite->setTextColor(COLOR_LABEL);
        m_detailSprite->drawString(STR(ContactsKey::CONTACTS_LABEL_PHONE_MOBILE), DETAIL_PADDING, yPos);
        m_detailSprite->setTextColor(COLOR_VALUE);
        m_detailSprite->drawString(contact.phoneMobile, DETAIL_PADDING + DETAIL_LABEL_WIDTH, yPos);
        yPos += DETAIL_LINE_HEIGHT;
    }

    // Telefon dienstlich
    if (contact.phoneWork[0] != '\0')
    {
        m_detailSprite->setTextColor(COLOR_LABEL);
        m_detailSprite->drawString(STR(ContactsKey::CONTACTS_LABEL_PHONE_WORK), DETAIL_PADDING, yPos);
        m_detailSprite->setTextColor(COLOR_VALUE);
        m_detailSprite->drawString(contact.phoneWork, DETAIL_PADDING + DETAIL_LABEL_WIDTH, yPos);
        yPos += DETAIL_LINE_HEIGHT;
    }

    // E-Mail
    if (contact.email[0] != '\0')
    {
        m_detailSprite->setTextColor(COLOR_LABEL);
        m_detailSprite->drawString(STR(ContactsKey::CONTACTS_LABEL_EMAIL), DETAIL_PADDING, yPos);
        m_detailSprite->setTextColor(COLOR_VALUE);

        char truncatedEmail[LINE_BUFFER_SIZE];
        truncateTextToWidth(contact.email, DETAIL_WIDTH - DETAIL_PADDING * 2 - DETAIL_LABEL_WIDTH,
                            m_detailSprite, truncatedEmail, sizeof(truncatedEmail));
        m_detailSprite->drawString(truncatedEmail, DETAIL_PADDING + DETAIL_LABEL_WIDTH, yPos);
        yPos += DETAIL_LINE_HEIGHT;
    }

    // Adresse
    if (contact.address.hasData())
    {
        m_detailSprite->setTextColor(COLOR_LABEL);
        m_detailSprite->drawString(STR(ContactsKey::CONTACTS_LABEL_ADDRESS), DETAIL_PADDING, yPos);

        m_detailSprite->setTextColor(COLOR_VALUE);
        const int addrValueX = DETAIL_PADDING + DETAIL_LABEL_WIDTH;
        const int addrMaxWidth = DETAIL_WIDTH - DETAIL_PADDING - DETAIL_LABEL_WIDTH;

        if (contact.address.street[0] != '\0')
        {
            char truncatedStreet[LINE_BUFFER_SIZE];
            truncateTextToWidth(contact.address.street, addrMaxWidth, m_detailSprite, truncatedStreet, sizeof(truncatedStreet));
            m_detailSprite->drawString(truncatedStreet, addrValueX, yPos);
        }
        yPos += DETAIL_LINE_HEIGHT - 6;

        if (contact.address.postalCode[0] != '\0' || contact.address.city[0] != '\0')
        {
            char plzOrt[80];
            if (contact.address.postalCode[0] != '\0' && contact.address.city[0] != '\0')
                stringutils::safeFormat(plzOrt, "%s %s",
                                        contact.address.postalCode, contact.address.city);
            else if (contact.address.city[0] != '\0')
                stringutils::safeCopy(plzOrt, contact.address.city);
            else
                stringutils::safeCopy(plzOrt, contact.address.postalCode);

            m_detailSprite->drawString(plzOrt, addrValueX, yPos);
            yPos += DETAIL_LINE_HEIGHT - 6;
        }

        if (contact.address.country[0] != '\0')
        {
            m_detailSprite->drawString(contact.address.country, addrValueX, yPos);
            yPos += DETAIL_LINE_HEIGHT - 6;
        }

        yPos += 6;
    }

    // Geburtstag mit Alter-Badge
    if (contact.birthdayDay > 0 && contact.birthdayMonth > 0)
    {
        char birthdayStr[32];
        int age = -1;

        if (contact.birthdayYear > 0)
        {
            // Alter berechnen
            age = data->today.currentYear - contact.birthdayYear;

            // Effektiven Geburtstag für Vergleich ermitteln (Feb-29-Geburtstage werden in Nicht-Schaltjahren am 28.02 gefeiert)
            int effectiveBirthdayDay = contact.birthdayDay;
            if (contact.birthdayDay == 29 && contact.birthdayMonth == 2)
            {
                const int year = data->today.currentYear;
                const bool isLeapYear = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
                if (!isLeapYear)
                    effectiveBirthdayDay = 28;
            }

            // Prüfen ob Geburtstag dieses Jahr noch nicht war
            if (data->today.currentMonth < contact.birthdayMonth ||
                (data->today.currentMonth == contact.birthdayMonth && data->today.currentDay < effectiveBirthdayDay))
                age--;

            STRF(birthdayStr, ContactsKey::CONTACTS_DATE_FULL_FMT, contact.birthdayDay, contact.birthdayMonth, contact.birthdayYear);
        }
        else
        {
            STRF(birthdayStr, ContactsKey::CONTACTS_DATE_SHORT_FMT, contact.birthdayDay, contact.birthdayMonth);
        }

        // Label zeichnen
        m_detailSprite->setTextColor(COLOR_LABEL);
        m_detailSprite->drawString(STR(ContactsKey::CONTACTS_LABEL_BIRTHDAY), DETAIL_PADDING, yPos);

        // Datum zeichnen
        const int valueX = DETAIL_PADDING + DETAIL_LABEL_WIDTH;
        m_detailSprite->setTextColor(contact.hasBirthdayToday ? COLOR_BIRTHDAY : COLOR_VALUE);
        m_detailSprite->drawString(birthdayStr, valueX, yPos);

        // Age-Badge zeichnen, wenn das Alter bekannt ist
        if (age >= 0)
        {
            char ageStr[8];
            stringutils::safeFormat(ageStr, "%d", age);

            const int dateWidth = m_detailSprite->textWidth(birthdayStr);
            const int ageWidth = m_detailSprite->textWidth(ageStr);
            const int fontHeight = m_detailSprite->fontHeight();

            const int badgeWidth = ageWidth + AGE_BADGE_PADDING_H * 2;
            const int badgeHeight = fontHeight + AGE_BADGE_PADDING_V * 2 - 3;
            const int badgeX = valueX + dateWidth + 10;
            const int badgeY = yPos - AGE_BADGE_PADDING_V + 2;

            // Badge-Hintergrund
            m_detailSprite->fillSmoothRoundRect(badgeX, badgeY - 2, badgeWidth, badgeHeight, AGE_BADGE_RADIUS, COLOR_AGE_BADGE_BG);

            // Alter zentriert im Badge
            m_detailSprite->setTextDatum(MC_DATUM);
            m_detailSprite->setTextColor(contact.hasBirthdayToday ? Theme::ColorRetroRed : COLOR_VALUE, COLOR_AGE_BADGE_BG);
            m_detailSprite->drawString(ageStr, badgeX + badgeWidth / 2, badgeY + badgeHeight / 2);
            m_detailSprite->setTextDatum(TL_DATUM);
        }
    }

    m_detailSprite->unloadFont();

    drawStatsBlock();

    m_detailSprite->pushSprite(DETAIL_X, DETAIL_Y);
    m_detailSpriteValid = true;
}

/**
 * @brief Zeichnet den Scrollbalken
 */
void Contacts::drawScrollbar()
{
    if (!m_scrollbarSprite)
        return;

    m_scrollbarSprite->clear(COLOR_SCROLLBAR_TRACK);

    if (m_scrollState.contentHeight <= VIEWPORT_HEIGHT || m_scrollState.maxScroll <= 0)
    {
        m_scrollbarSprite->pushSprite(SCROLLBAR_X, 0);
        return;
    }

    const float viewRatio = static_cast<float>(VIEWPORT_HEIGHT) / m_scrollState.contentHeight;
    const int thumbHeight = std::max(20, static_cast<int>(VIEWPORT_HEIGHT * viewRatio));

    // Geclampter Offset für Scrollbar (verhindert Thumb außerhalb bei Gummiband-Overscroll)
    const int clampedOffset = std::max(0, std::min(m_scrollState.scrollOffset, m_scrollState.maxScroll));
    const float scrollRatio = static_cast<float>(clampedOffset) / m_scrollState.maxScroll;
    const int thumbY = static_cast<int>(scrollRatio * (VIEWPORT_HEIGHT - thumbHeight));

    uint16_t thumbColor = m_scrollState.isDragging ? COLOR_SCROLLBAR_ACTIVE : COLOR_SCROLLBAR_NORMAL;
    m_scrollbarSprite->fillRoundRect(0, thumbY, SCROLLBAR_WIDTH, thumbHeight, 2, thumbColor);

    m_scrollbarSprite->pushSprite(SCROLLBAR_X, 0);
}

/**
 * @brief Zeichnet die Digitaluhr
 */
void Contacts::drawDigitalClock(int32_t x, int32_t y)
{
    if (!m_clockSprite || !m_hasValidData)
        return;

    DS1307_RTC *rtc = &APP_STATE.getRTC();
    Clocks::drawDigitalClockPlain(m_clockSprite, rtc, m_lastDrawnSecond, x, y, false, true);
}

// ===================================================================
// TOUCH-HANDLING
// ===================================================================

/**
 * @brief Verarbeitet Touch-Events für Scrolling
 */
void Contacts::handleScrollTouch(int32_t tx, int32_t ty, bool touched)
{
    if (touched)
    {
        if (!m_scrollState.isDragging)
        {
            m_scrollState.isDragging = true;
            m_scrollState.dragStartY = ty;
            m_scrollState.dragStartOffset = m_scrollState.scrollOffset;
            m_scrollState.scrollVelocity = 0;
            m_scrollState.isSnappingBack = false;
            m_lastTouchY = ty;
            m_lastTouchTime = millis();
            m_selectedLetter = '?';
        }
        else
        {
            const int deltaY = ty - m_scrollState.dragStartY;
            m_scrollState.scrollOffset = m_scrollState.dragStartOffset - static_cast<int>(deltaY * 1.5f);

            // Velocity für Momentum
            const unsigned long now = millis();
            const int timeDelta = static_cast<int>(now - m_lastTouchTime);

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
        m_scrollState.scrollVelocity = constrain(m_scrollState.scrollVelocity, -2500.0f, 2500.0f);

        if (m_scrollState.scrollOffset < 0 || m_scrollState.scrollOffset > m_scrollState.maxScroll)
            m_scrollState.isSnappingBack = true;
    }
}

/**
 * @brief Aktualisiert die Scroll-Physik
 */
void Contacts::updateScrollPhysics()
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

        constrainScroll();
    }
    else
    {
        m_scrollState.isSnappingBack = false;
    }
}

/**
 * @brief Begrenzt Scroll-Position auf gültigen Bereich
 */
void Contacts::constrainScroll()
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
 * @brief Berechnet die Gesamthöhe der sichtbaren Kontakte
 */
int Contacts::calculateContentHeight() const
{
    // Nutzt gecachten visibleCount aus rebuildVisibleIndices()
    return m_visibleCount * CONTACT_HEIGHT;
}

/**
 * @brief Ermittelt den Kontakt-Index an einer Touch-Position (O(1))
 */
int Contacts::getContactAtPosition(int32_t tx, int32_t ty) const
{
    if (!m_hasValidData || m_visibleCount == 0)
        return -1;

    // Relative Y-Position im Viewport
    const int relativeY = ty + m_scrollState.scrollOffset;
    if (relativeY < 0)
        return -1;

    // Direkte Berechnung der Zeile
    const int row = relativeY / CONTACT_HEIGHT;

    // Bounds-Check und Rückgabe des Original-Index
    if (row >= 0 && row < m_visibleCount)
        return m_visibleIndices[row];

    return -1;
}

/**
 * @brief Verarbeitet den Tap auf einen Kontakt
 */
void Contacts::handleContactTap(int contactIndex)
{
    if (contactIndex < 0)
        return;

    const int bufferIndex = m_activeBufferIndex.load();
    const ContactsData *data = &m_dataBuffers[bufferIndex];

    if (contactIndex >= data->contactCount)
        return;

    m_selectedContactIndex = contactIndex;

    // ID speichern für Refresh-Stabilität
    stringutils::safeCopy(m_selectedContactId, data->contacts[contactIndex].id);

    m_listSpriteValid = false;
    m_detailSpriteValid = false;

    LOG_INFO_F("Kontakt ausgewählt: %d '%s'", contactIndex, data->contacts[contactIndex].displayName);
}

/**
 * @brief Verarbeitet Tap auf die Alphabet-Leiste
 */
void Contacts::handleAlphabetTap(int32_t tx, int32_t ty)
{
    int index = getAlphabetIndexFromY(ty);
    if (index < 0 || index >= ContactsCfg::ALPHABET_ENTRIES)
        return;

    char letter = getLetterFromAlphabetIndex(index);
    scrollToLetter(letter);
}

/**
 * @brief Prüft ob Alphabet-Leiste berührt wird
 */
bool Contacts::isAlphabetBarTouched(int32_t tx, int32_t ty) const
{
    return (tx >= ALPHABET_BAR_X && tx < ALPHABET_BAR_X + ALPHABET_BAR_WIDTH && ty >= 0 && ty < VIEWPORT_HEIGHT);
}

/**
 * @brief Prüft ob Suchen-Button berührt wird
 */
bool Contacts::isSearchButtonTouched(int32_t tx, int32_t ty) const
{
    return m_searchButton.contains(tx, ty);
}

/**
 * @brief Prüft ob Reset-Button berührt wird
 */
bool Contacts::isResetButtonTouched(int32_t tx, int32_t ty) const
{
    return m_resetButton.contains(tx, ty);
}

/**
 * @brief Prüft ob Refresh-Button berührt wird
 */
bool Contacts::isRefreshButtonTouched(int32_t tx, int32_t ty) const
{
    return m_refreshButton.contains(tx, ty);
}

/**
 * @brief Öffnet das Suchfeld
 */
void Contacts::handleSearchButtonTap()
{
    if (!m_keyboard)
        return;

    char searchBuffer[ContactsCfg::MAX_SEARCH_LEN] = "";
    if (m_searchState.isActive)
        stringutils::safeCopy(searchBuffer, m_searchState.searchPattern);

    m_keyboard->setOkLabel(reinterpret_cast<const char *>(STR(CommonKey::COMMON_BTN_OK)));
    bool result = m_keyboard->show(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_SEARCH_PROMPT)), searchBuffer, sizeof(searchBuffer), searchBuffer);

    if (result)
    {
        stringutils::safeCopy(m_searchState.searchPattern, searchBuffer);
        m_searchState.isActive = (searchBuffer[0] != '\0');

        if (m_searchState.isActive)
            m_resetButton.enable();
        else
            m_resetButton.disable();

        m_scrollState.reset();
        rebuildVisibleIndices(); // Aktualisiert auch contentHeight und maxScroll

        // Versuche vorherige Auswahl beizubehalten, sonst ersten sichtbaren wählen
        const int bufferIndex = m_activeBufferIndex.load();
        const ContactsData *data = &m_dataBuffers[bufferIndex];

        int restoredIndex = -1;
        if (m_selectedContactId[0] != '\0')
        {
            for (int row = 0; row < m_visibleCount; ++row)
            {
                const int idx = m_visibleIndices[row];
                if (strcmp(data->contacts[idx].id, m_selectedContactId) == 0)
                {
                    restoredIndex = idx;
                    break;
                }
            }
        }

        if (restoredIndex >= 0)
        {
            m_selectedContactIndex = restoredIndex;
        }
        else if (m_visibleCount > 0)
        {
            m_selectedContactIndex = m_visibleIndices[0];
            stringutils::safeCopy(m_selectedContactId, data->contacts[m_selectedContactIndex].id);
        }
        else
        {
            m_selectedContactIndex = -1;
            m_selectedContactId[0] = '\0';
        }

        LOG_INFO_F("Suche: '%s' (aktiv: %s)", m_searchState.searchPattern, m_searchState.isActive ? "ja" : "nein");
    }

    m_layoutDrawn = false;
    m_listSpriteValid = false;
    m_detailSpriteValid = false;
    drawLayout();
    m_layoutDrawn = true;
}

/**
 * @brief Setzt den Suchfilter zurück
 */
void Contacts::handleResetButtonTap()
{
    if (!m_searchState.isActive)
    {
        LOG_INFO("Kein Filter aktiv - nichts zu resetten");
        return;
    }

    if (!m_messageBox)
        return;

    m_messageBox->setTitle(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_RESET_TITLE)));
    m_messageBox->setMessage(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_RESET_MESSAGE)));
    m_messageBox->setButtonLabels(
        reinterpret_cast<const char *>(STR(CommonKey::COMMON_BTN_YES)),
        reinterpret_cast<const char *>(STR(CommonKey::COMMON_BTN_NO)));
    m_messageBox->setCallback([this](bool confirmed)
                              {
        if (confirmed)
        {
            m_searchState.reset();
            m_scrollState.reset();
            rebuildVisibleIndices(); // Aktualisiert auch contentHeight und maxScroll

            // Versuche vorherige Auswahl beizubehalten
            const int bufferIndex = m_activeBufferIndex.load();
            const ContactsData *data = &m_dataBuffers[bufferIndex];

            int restoredIndex = -1;
            if (m_selectedContactId[0] != '\0')
            {
                for (int row = 0; row < m_visibleCount; ++row)
                {
                    const int idx = m_visibleIndices[row];
                    if (strcmp(data->contacts[idx].id, m_selectedContactId) == 0)
                    {
                        restoredIndex = idx;
                        break;
                    }
                }
            }

            if (restoredIndex >= 0)
            {
                m_selectedContactIndex = restoredIndex;
            }
            else if (m_visibleCount > 0)
            {
                m_selectedContactIndex = m_visibleIndices[0];
                stringutils::safeCopy(m_selectedContactId, data->contacts[m_selectedContactIndex].id);
            }
            else
            {
                m_selectedContactIndex = -1;
                m_selectedContactId[0] = '\0';
            }

            m_resetButton.disable();

            LOG_INFO("Suchfilter zurückgesetzt");

            m_layoutDrawn = false;
            m_listSpriteValid = false;
            m_detailSpriteValid = false;
            drawLayout();
            m_layoutDrawn = true;
        } });

    m_messageBox->start();
}

/**
 * @brief Startet einen manuellen Refresh
 */
void Contacts::handleRefreshButtonTap()
{
    LOG_INFO("Manueller Refresh getriggert");

    m_fetchCompleted.store(false);

    if (m_progressBox)
    {
        m_progressBox->setMessage(reinterpret_cast<const char *>(STR(ContactsKey::CONTACTS_PROGRESS_MESSAGE)));
        m_progressBox->start(nullptr, -1);
    }

    requestRefresh(true);
}

// ===================================================================
// TEXT-HILFSFUNKTIONEN
// ===================================================================

/**
 * @brief Kürzt Text auf maximale Pixelbreite mit Ellipse
 */
void Contacts::truncateTextToWidth(const char *text, int maxWidth, LGFX_Sprite *sprite, char *output, size_t outputSize) const
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
    auto isUtf8Continuation = [](char c) {
        return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
    };

    // Binary Search: Finde größte Länge die noch passt textWidth() ist monoton steigend mit der Textlänge
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
            bestFit = mid;  // Diese Länge passt, versuche längeren Text
            low = mid + 1;
        }
        else
        {
            if (mid == 0)
                break;
            high = mid - 1;  // Zu lang, versuche kürzeren Text
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
