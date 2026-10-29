#ifndef CONTACTS_H
#define CONTACTS_H

// ============================================================================

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <atomic>
#include <memory>

#include "BaseModule.h"
#include "Button.h"
#include "Clocks.h"
#include "DS1307_RTC.h"
#include "Gfx.h"
#include "Keyboard.h"
#include "Logger.h"
#include "MessageBox.h"
#include "ProgressBox.h"


// ============================================================================
// CONTACTS-KONFIGURATION
// ============================================================================

/**
 * @brief Konfigurationskonstanten für das Contacts-Modul
 */
namespace ContactsCfg
{
    constexpr int MAX_CONTACTS = 100;    ///< Maximale Anzahl Kontakte
    constexpr int DEMO_CONTACT_COUNT = 30; ///< Anzahl Demo-Kontakte im Demo-Modus
    constexpr int MAX_NAME_LEN = 48;     ///< Maximale Namenslänge
    constexpr int MAX_PHONE_LEN = 24;    ///< Maximale Telefonnummerlänge
    constexpr int MAX_EMAIL_LEN = 48;    ///< Maximale E-Mail-Länge
    constexpr int MAX_COMPANY_LEN = 32;  ///< Maximale Firmenlänge
    constexpr int MAX_SEARCH_LEN = 32;   ///< Maximale Suchlänge
    constexpr int BUFFER_COUNT = 2;      ///< Anzahl der Double-Buffer
    constexpr int ALPHABET_ENTRIES = 26; ///< Nur A-Z (keine Zahlen)

    // Adress-Feldlängen
    constexpr int MAX_STREET_LEN = 48;  ///< Straße inkl. Hausnummer
    constexpr int MAX_POSTAL_LEN = 10;  ///< PLZ
    constexpr int MAX_CITY_LEN = 32;    ///< Stadt/Ort
    constexpr int MAX_COUNTRY_LEN = 32; ///< Land (oder Ländercode)

    // Timing-Konstanten
    constexpr unsigned long API_CALL_INTERVAL_MS = 30UL * 60UL * 1000UL; ///< 30 Minuten
    constexpr unsigned long MIN_RETRY_INTERVAL_MS = 30000UL;             ///< 30 Sekunden
    constexpr unsigned long MAX_BACKOFF_TIME_MS = 480000UL;              ///< 8 Minuten
}

// ============================================================================
// DATENSTRUKTUREN
// ============================================================================

/**
 * @brief Struktur für Adressdaten eines Kontakts
 *
 * Enthält die wichtigsten Adresskomponenten gemäß Google People API.
 */
struct ContactAddress
{
    char street[ContactsCfg::MAX_STREET_LEN];     ///< Straße inkl. Hausnummer
    char postalCode[ContactsCfg::MAX_POSTAL_LEN]; ///< PLZ
    char city[ContactsCfg::MAX_CITY_LEN];         ///< Stadt/Ort
    char country[ContactsCfg::MAX_COUNTRY_LEN];   ///< Land (oder Ländercode)

    /**
     * @brief Initialisiert alle Adressfelder auf leere Strings
     */
    void reset()
    {
        street[0] = '\0';
        postalCode[0] = '\0';
        city[0] = '\0';
        country[0] = '\0';
    }

    /**
     * @brief Prüft ob mindestens ein Adressfeld gefüllt ist
     * @return true wenn Adressdaten vorhanden sind
     */
    bool hasData() const
    {
        return (street[0] != '\0' || city[0] != '\0' || postalCode[0] != '\0');
    }
};

/**
 * @brief Struktur für einen einzelnen Kontakt
 *
 * Enthält alle relevanten Daten für die Darstellung eines Kontakts inklusive Kontaktdaten, Adresse und Geburtstagsinformationen.
 */
struct ContactItem
{
    char id[48] = "";                                         ///< Google Contacts ID
    char firstName[ContactsCfg::MAX_NAME_LEN] = "";           ///< Vorname
    char lastName[ContactsCfg::MAX_NAME_LEN] = "";            ///< Nachname
    char displayName[ContactsCfg::MAX_NAME_LEN * 2 + 4] = ""; ///< "Nachname, Vorname"
    char phoneHome[ContactsCfg::MAX_PHONE_LEN] = "";          ///< Telefon privat
    char phoneMobile[ContactsCfg::MAX_PHONE_LEN] = "";        ///< Telefon mobil
    char phoneWork[ContactsCfg::MAX_PHONE_LEN] = "";          ///< Telefon dienstlich
    char email[ContactsCfg::MAX_EMAIL_LEN] = "";              ///< E-Mail-Adresse
    ContactAddress address{};                                 ///< Strukturierte Adressdaten
    char company[ContactsCfg::MAX_COMPANY_LEN] = "";          ///< Firma/Organisation
    uint8_t birthdayDay = 0;                                  ///< Geburtstag: Tag (1-31, 0=unbekannt)
    uint8_t birthdayMonth = 0;                                ///< Geburtstag: Monat (1-12, 0=unbekannt)
    uint16_t birthdayYear = 0;                                ///< Geburtstag: Jahr (0=unbekannt)
    char sortKey = '?';                                       ///< Sortier-Schlüssel (A-Z, uppercase)
    bool hasBirthdayToday = false;                            ///< Hat heute Geburtstag?

    /**
     * @brief Setzt alle Felder auf Initialwerte zurück
     */
    void reset()
    {
        id[0] = '\0';
        firstName[0] = '\0';
        lastName[0] = '\0';
        displayName[0] = '\0';
        phoneHome[0] = '\0';
        phoneMobile[0] = '\0';
        phoneWork[0] = '\0';
        email[0] = '\0';
        address.reset();
        company[0] = '\0';
        birthdayDay = 0;
        birthdayMonth = 0;
        birthdayYear = 0;
        sortKey = '?';
        hasBirthdayToday = false;
    }
};

/**
 * @brief Statistiken über die geladenen Kontakte
 */
struct ContactStats
{
    int total = 0;          ///< Gesamtanzahl Kontakte
    int withBirthday = 0;   ///< Anzahl mit Geburtstag
    int birthdaysToday = 0; ///< Anzahl Geburtstage heute
};

/**
 * @brief Datenstruktur für das aktuelle Datum
 *
 * Wird für Altersberechnung und Geburtstagserkennung verwendet.
 */
struct ContactsTodayData
{
    int currentDay = 0;   ///< Aktueller Tag (1-31)
    int currentMonth = 0; ///< Aktueller Monat (1-12)
    int currentYear = 0;  ///< Aktuelles Jahr (z.B. 2025)
};

/**
 * @brief Thread-sichere Datenstruktur für alle Kontakt-Daten
 *
 * Wird im Double-Buffer-Verfahren zwischen Worker-Task und UI-Thread verwendet. Der Worker schreibt in den inaktiven Buffer, nach erfolgreichem Fetch werden die Buffer atomar getauscht.
 */
struct ContactsData
{
    ContactItem contacts[ContactsCfg::MAX_CONTACTS]; ///< Array aller Kontakte
    int contactCount = 0;                            ///< Anzahl der Kontakte
    ContactStats stats{};                            ///< Statistiken
    ContactsTodayData today{};                       ///< Heutige Datumsinformationen
    unsigned long timestamp = 0;                     ///< Zeitstempel der Daten (millis)

    /**
     * @brief Alphabet-Index: Position des ersten Kontakts für jeden Buchstaben
     *
     * Index 0-25 für A-Z. Wert -1 bedeutet kein Kontakt mit diesem Anfangsbuchstaben vorhanden.
     */
    int alphabetIndex[ContactsCfg::ALPHABET_ENTRIES];

    /**
     * @brief Setzt alle Daten auf Initialwerte zurück
     */
    void reset()
    {
        contactCount = 0;
        stats = ContactStats{};
        today = ContactsTodayData{};
        timestamp = 0;

        for (int i = 0; i < ContactsCfg::ALPHABET_ENTRIES; ++i)
        {
            alphabetIndex[i] = -1;
        }
    }
};

/**
 * @brief Callbacks für das Contacts-Modul
 */
struct ContactsCallbacks
{
    void (*onExitRequested)() = nullptr; ///< Wird beim Verlassen des Moduls aufgerufen
};

// ============================================================================
// HAUPTKLASSE
// ============================================================================

/**
 * @brief Thread-sicheres Contacts-Modul mit asynchronem Datenabruf
 *
 * Implementiert robuste Kommunikation zwischen UI und Worker-Task zur Anzeige von Google Contacts mit folgenden Features:
 *
 * - Double-Buffering für ContactsData (keine Runtime-Allokation)
 * - Worker schreibt in inaktiven Buffer
 * - Atomarer Index-Swap nach erfolgreichem Fetch
 * - Alphabet-Navigation für schnellen Zugriff
 * - Wildcard-Suche nach Nachnamen
 * - Geburtstagserkennung mit Altersanzeige
 *
 * @note Nicht kopierbar wegen Task-Handle und atomaren Membern
 */
class Contacts : public BaseModule
{
public:
    /**
     * @brief Konstruktor
     */
    Contacts();

    /**
     * @brief Destruktor
     */
    ~Contacts();

    // Keine Kopien erlauben
    Contacts(const Contacts &) = delete;
    Contacts &operator=(const Contacts &) = delete;

    // Keine Move-Semantik
    Contacts(Contacts &&) = delete;
    Contacts &operator=(Contacts &&) = delete;

    /**
     * @brief Initialisiert das Contacts-Modul
     * @param tft Referenz auf das Display-Objekt
     * @param callbacks Callback-Funktionen für Events
     * @return true bei erfolgreicher Initialisierung
     *
     * Erstellt alle Sprites, initialisiert Buttons und startet den Worker-Task für den asynchronen Datenabruf.
     * 
     * @note HTTP-Requests werden über den zentralen HttpService abgewickelt.
     *       HttpService muss vorher initialisiert sein (HTTP_SERVICE.begin()).
     */
    bool init(LGFX &tft, const ContactsCallbacks &callbacks);

    /**
     * @brief Hauptzeichenfunktion - wird zyklisch aufgerufen
     *
     * Verarbeitet Touch-Events, aktualisiert UI-Komponenten und prüft auf neue Daten vom Worker-Task.
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück
     * @return AppModule::CONTACTS
     */
    AppModule type() const override;

    /**
     * @brief Fordert einen Refresh der Kontaktdaten an
     * @param force true um Rate-Limiting zu ignorieren
     */
    void requestRefresh(bool force = false);

private:
    // ========================================================================
    // INNERE STRUKTUREN
    // ========================================================================

    /**
     * @brief Scroll-Status für die Kontakt-Liste
     *
     * Verwaltet Scroll-Position, Momentum-Scrolling und Gummiband-Effekt am Listenrand.
     */
    struct ScrollState
    {
        int scrollOffset = 0;        ///< Aktuelle Scroll-Position in Pixeln
        int contentHeight = 0;       ///< Gesamthöhe des Inhalts
        int maxScroll = 0;           ///< Maximale Scroll-Position
        float scrollVelocity = 0.0f; ///< Geschwindigkeit für Momentum-Scrolling
        bool isDragging = false;     ///< Touch-Drag aktiv
        int dragStartY = 0;          ///< Start-Y beim Drag
        int dragStartOffset = 0;     ///< Scroll-Offset beim Drag-Start
        bool isSnappingBack = false; ///< Flag für Zurückschnapp-Animation

        void reset()
        {
            scrollOffset = 0;
            scrollVelocity = 0.0f;
            isDragging = false;
            isSnappingBack = false;
        }
    };

    /**
     * @brief Tap-Erkennung Status
     *
     * Unterscheidet zwischen Tap (kurzer Touch) und Drag (Scroll-Geste).
     */
    struct TapState
    {
        int32_t startX = 0;                  ///< X-Position bei Touch-Start
        int32_t startY = 0;                  ///< Y-Position bei Touch-Start
        bool isTap = false;                  ///< True wenn Tap (kein Drag)
        static constexpr int THRESHOLD = 15; ///< Max. Bewegung für Tap-Erkennung

        void reset()
        {
            startX = 0;
            startY = 0;
            isTap = false;
        }

        bool exceedsThreshold(int32_t tx, int32_t ty) const
        {
            return (abs(tx - startX) > THRESHOLD || abs(ty - startY) > THRESHOLD);
        }
    };

    /**
     * @brief Such-Status für Wildcard-Filterung
     */
    struct SearchState
    {
        char searchPattern[ContactsCfg::MAX_SEARCH_LEN] = ""; ///< Aktuelles Suchmuster
        bool isActive = false;                                ///< Suche aktiv?
        int resultCount = 0;                                  ///< Anzahl gefundener Kontakte

        void reset()
        {
            searchPattern[0] = '\0';
            isActive = false;
            resultCount = 0;
        }
    };

    // ========================================================================
    // WORKER-TASK FUNKTIONEN
    // ========================================================================

    static void taskEntry(void *arg);                 ///< Statischer Task-Einstiegspunkt
    void workerLoop();                                ///< Hauptschleife des Worker-Tasks
    int fetchContactsDataCore(ContactsData *outData); ///< HTTP-Abruf und JSON-Parsing
    
    void generateDemoData(ContactsData *outData);     ///< Generiert Demo-Kontakte
    
    void updateCurrentTime();                         ///< Aktualisiert Datum vom RTC (UI-Thread)
    void shutdownWorker(bool fast = false);           ///< Beendet Worker-Task sauber

    /**
     * @brief Füllt Datumsdaten thread-sicher direkt ins Ziel
     * @param out Ziel-Struktur für das aktuelle Datum
     *
     * Statische Hilfsfunktion, die das aktuelle Datum von der RTC holt und direkt in die übergebene Struktur schreibt. Wird vom Worker verwendet, um Thread-Safety-Probleme zu vermeiden.
     */
    static void fillCurrentTime(ContactsTodayData &out);

    // ========================================================================
    // DOUBLE-BUFFER FUNKTIONEN
    // ========================================================================

    ContactsData *getActiveBuffer();   ///< Liefert aktiven Buffer (für UI)
    ContactsData *getInactiveBuffer(); ///< Liefert inaktiven Buffer (für Worker)
    void swapBuffers();                ///< Tauscht aktiven/inaktiven Buffer

    // ========================================================================
    // UI-ZEICHENFUNKTIONEN
    // ========================================================================

    void drawLayout();                           ///< Zeichnet komplettes Layout
    void redrawAll();                            ///< Vollredraw nach Sprachwechsel (Layout + Buttons mit neuen Labels)
    void drawAlphabetBar();                      ///< Zeichnet Alphabet-Navigation
    void drawContactList();                      ///< Zeichnet Kontaktliste
    void drawContactDetail();                    ///< Zeichnet Kontaktdetails
    void drawDigitalClock(int32_t x, int32_t y); ///< Zeichnet Digitaluhr
    void drawScrollbar();                        ///< Zeichnet Scrollbalken
    void drawBirthdayBadge(int x, int y);        ///< Zeichnet Geburtstags-Symbol

    /**
     * @brief Kürzt Text auf maximale Pixelbreite mit Ellipse
     * @param text Eingabetext
     * @param maxWidth Maximale Breite in Pixeln
     * @param sprite Sprite für Textbreitenberechnung
     * @param output Ausgabepuffer
     * @param outputSize Größe des Ausgabepuffers
     */
    void truncateTextToWidth(const char *text, int maxWidth, LGFX_Sprite *sprite, char *output, size_t outputSize) const;

    // ========================================================================
    // TOUCH-HANDLING
    // ========================================================================

    void handleScrollTouch(int32_t tx, int32_t ty, bool touched); ///< Verarbeitet Scroll-Touch
    void updateScrollPhysics();                                   ///< Aktualisiert Momentum-Scrolling
    void constrainScroll();                                       ///< Begrenzt Scroll auf gültigen Bereich
    int calculateContentHeight() const;                           ///< Berechnet Gesamthöhe
    int getContactAtPosition(int32_t tx, int32_t ty) const;       ///< Kontakt an Position
    void handleContactTap(int contactIndex);                      ///< Verarbeitet Kontakt-Auswahl
    void handleAlphabetTap(int32_t tx, int32_t ty);               ///< Verarbeitet Alphabet-Touch
    void handleSearchButtonTap();                                 ///< Öffnet Suchfeld
    void handleResetButtonTap();                                  ///< Setzt Filter zurück
    void handleRefreshButtonTap();                                ///< Startet manuellen Refresh
    bool isSearchButtonTouched(int32_t tx, int32_t ty) const;     ///< Prüft Such-Button Touch
    bool isResetButtonTouched(int32_t tx, int32_t ty) const;      ///< Prüft Reset-Button Touch
    bool isRefreshButtonTouched(int32_t tx, int32_t ty) const;    ///< Prüft Refresh-Button Touch
    bool isAlphabetBarTouched(int32_t tx, int32_t ty) const;      ///< Prüft Alphabet-Leiste Touch

    // ========================================================================
    // SUCH-FUNKTIONEN
    // ========================================================================

    bool matchesWildcard(const char *text, const char *pattern) const;
    bool isContactVisible(const ContactItem &contact) const;
    void scrollToLetter(char letter);
    int getAlphabetIndexFromY(int32_t y) const;
    char getLetterFromAlphabetIndex(int index) const;

    /**
     * @brief Baut das Sichtbarkeits-Mapping neu auf
     *
     * Aktualisiert m_visibleIndices[] und m_visibleCount basierend auf dem aktuellen Suchfilter. Muss bei Datenübernahme und Filteränderung aufgerufen werden. Ermöglicht O(1) Zugriff auf sichtbare Kontakte.
     */
    void rebuildVisibleIndices();

    // ========================================================================
    // HILFSFUNKTIONEN
    // ========================================================================

    void buildAlphabetIndex(ContactsData *data);                                         ///< Erstellt Alphabet-Index
    void sortContacts(ContactsData *data);                                               ///< Sortiert Kontakte
    char getSortKey(const char *lastName) const;                                         ///< Ermittelt Sortierschlüssel
    void formatDisplayName(ContactItem &contact);                                        ///< Formatiert Anzeigenamen
    bool checkBirthdayToday(ContactItem &contact, const ContactsTodayData &today) const; ///< Prüft Geburtstag

    // ========================================================================
    // MEMBER-VARIABLEN
    // ========================================================================

    // Core-Komponenten
    LGFX *m_tft = nullptr;               ///< Display-Objekt
    ContactsCallbacks m_callbacks{};     ///< Callback-Funktionen
    bool m_initialized = false;          ///< Modul initialisiert?

    // UI-Buttons
    Button m_exitButton;       ///< Beenden-Button
    Button m_cancelLoadButton; ///< Abbrechen-Button (Ladescreen)
    Button m_searchButton;     ///< Suchen-Button
    Button m_resetButton;      ///< Reset-Button (Filter zurücksetzen)
    Button m_refreshButton;    ///< Aktualisieren-Button

    // Keyboard für Suchfeld
    std::unique_ptr<Keyboard> m_keyboard;

    // ========================================================================
    // DOUBLE-BUFFER FÜR KONTAKT-DATEN
    // ========================================================================

    ContactsData m_dataBuffers[ContactsCfg::BUFFER_COUNT]; ///< Double-Buffer Array
    std::atomic<int> m_activeBufferIndex{0};               ///< Index des aktiven Buffers (0 oder 1)
    std::atomic<bool> m_dataReady{false};                  ///< Signal: Neue Daten verfügbar
    bool m_hasValidData = false;                           ///< UI hat gültige Daten empfangen

    // UI-State
    ScrollState m_scrollState; ///< Scroll-Zustand
    TapState m_tapState;       ///< Tap-Erkennung
    SearchState m_searchState; ///< Such-Zustand

    // ========================================================================
    // SICHTBARKEITS-MAPPING
    // ========================================================================

    int m_visibleIndices[ContactsCfg::MAX_CONTACTS];           ///< Indizes der sichtbaren Kontakte im Original-Array
    int m_visibleCount = 0;                                    ///< Anzahl aktuell sichtbarer Kontakte
    int m_visibleAlphabetIndex[ContactsCfg::ALPHABET_ENTRIES]; ///< Erste sichtbare Zeile je Buchstabe (-1 = keine)

    int m_selectedContactIndex = -1;    ///< Aktuell ausgewählter Kontakt (-1 = keiner)
    char m_selectedContactId[48] = "";  ///< ID des ausgewählten Kontakts (für Refresh-Stabilität)
    int32_t m_lastTouchY = -1;          ///< Letzte Touch-Y-Position
    unsigned long m_lastTouchTime = 0;  ///< Zeitstempel letzter Touch
    bool m_alphabetTouchActive = false; ///< Verhindert Dauerfeuer bei Alphabet-Touch
    char m_selectedLetter = '?';        ///< Zuletzt ausgewählter Buchstabe

    // Threading und Synchronisation
    std::atomic<TaskHandle_t> m_taskHandle{nullptr}; ///< Handle des Worker-Tasks
    std::atomic<bool> m_shutdownRequested{false};    ///< Shutdown angefordert?
    std::atomic<bool> m_refreshRequested{false};     ///< Refresh angefordert?
    std::atomic<bool> m_inFlight{false};             ///< HTTP-Request läuft?
    std::atomic<bool> m_fetchCompleted{false};       ///< Fetch abgeschlossen (für UI)

    // Timing und Rate-Limiting
    unsigned long m_lastApiCall = 0;                  ///< Zeitpunkt letzter API-Aufruf
    unsigned long m_backoffMs = 0;                    ///< Aktuelle Backoff-Zeit
    std::atomic<unsigned long> m_nextAllowedFetch{0}; ///< Frühester erlaubter nächster Fetch (thread-safe)
    int m_consecutiveErrors = 0;                      ///< Aufeinanderfolgende Fehler

    // Zeitstempel-Anzeige
    char m_lastUpdateTimestamp[20] = ""; ///< Formatierter Zeitstempel "HH:MM"

    // Sprites für flüssiges Rendering
    Gfx_Sprite *m_listSprite = nullptr;      ///< Sprite für Kontaktliste
    Gfx_Sprite *m_detailSprite = nullptr;    ///< Sprite für Detailansicht
    Gfx_Sprite *m_alphabetSprite = nullptr;  ///< Sprite für Alphabet-Leiste
    Gfx_Sprite *m_scrollbarSprite = nullptr; ///< Sprite für Scrollbalken
    Gfx_Sprite *m_clockSprite = nullptr;     ///< Sprite für Digitaluhr
    Gfx_Sprite *m_progressSprite = nullptr;  ///< 1px Fortschrittsbalken (rechter Rand)

    // Sprite-Validierung (Dirty-Flags)
    bool m_listSpriteValid = false;   ///< Liste muss neu gezeichnet werden
    bool m_detailSpriteValid = false; ///< Details müssen neu gezeichnet werden
    int m_lastScrollOffset = -1;      ///< Letzter Scroll-Offset (für Vergleich)
    int m_lastDrawnSecond = -1;       ///< Letzte gezeichnete Sekunde (Digitaluhr)

    // UI-State
    bool m_layoutDrawn = false; ///< Hauptlayout wurde gezeichnet

    // Dialoge
    std::unique_ptr<ProgressBox> m_progressBox; ///< Lade-Dialog
    std::unique_ptr<MessageBox> m_messageBox;   ///< Bestätigungs-Dialog

    // API-Endpoint (Google Apps Script)
    const char *m_appsScriptUrl = nullptr; ///< URL zum Google Apps Script (aus AppState)
    bool m_demoMode = false;               ///< Runtime Demo-Modus (aktiv wenn keine gültige URL)
};

#endif // CONTACTS_H
