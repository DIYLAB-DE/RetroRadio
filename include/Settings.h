#ifndef SETTINGS_H
#define SETTINGS_H

#include <array>
#include <functional>
#include <memory>

#include "AppState.h"
#include "BaseModule.h"
#include "Button.h"
#include "DotMatrix.h"
#include "Gfx.h"
#include "Logger.h"
#include "ProgressBox.h"
#include "StringUtils.h"
#include "Strings_settings.h"
#include "WorkshopNumpad.h"

class MessageBox;
class Keyboard;
class WifiList;

/**
 * @brief Callback-Struktur für Settings-Events
 *
 * Funktionszeiger, die das Settings-Modul nach aussen meldet. Beide dürfen nullptr sein; das Modul prüft vor jedem Aufruf.
 */
struct SettingsCallbacks
{
    void (*onExitRequested)() = nullptr;    ///< Wird beim Verlassen des Moduls aufgerufen
    void (*onRestartRequested)() = nullptr; ///< Wird bei angefordertem Neustart aufgerufen
    void (*requestPlayStation)(uint8_t id) = nullptr; ///< Sender starten (Hörprobe im Klang-Panel)
    void (*requestStopPlayback)() = nullptr;          ///< Wiedergabe stoppen
    void (*onVolumeChanged)(uint8_t volume) = nullptr; ///< Lautstärke geändert
};

/**
 * @brief Einstellungs-Modul im Split-Screen-Layout.
 *
 * Links eine feste Liste aus Sektionsköpfen und Panel-Einträgen, rechts der Arbeitsbereich des gewählten Panels. Voll bedienbar sind WLAN, Nachrichten, Wetter, Sprache, Sicherheit und Info; die übrigen Dienste-Panels sind Platzhalter. Alle Bereiche werden in PSRAM-Sprites gezeichnet und atomar gepusht.
 *
 * @note Änderungen an WLAN-Profilen, Nachrichten-Filtern und Wetter-Standort
 *       werden über AppState persistent gespeichert.
 */
class Settings : public BaseModule
{
public:
    /** Maximale Anzahl der WLAN-Profile */
    static constexpr int MAX_PROFILES = 3;

    /** Timeout für den Verbindungstest in Millisekunden */
    static constexpr uint32_t TEST_TIMEOUT = 10000;

    Settings();
    ~Settings();

    Settings(const Settings &) = delete;
    Settings &operator=(const Settings &) = delete;
    Settings(Settings &&) = delete;
    Settings &operator=(Settings &&) = delete;

    /**
     * @brief Initialisiert das Modul, lädt Profile und erstellt die Sprites.
     * @param tft Display-Referenz
     * @param callbacks Event-Callbacks für Exit und Neustart
     * @return true bei erfolgreicher Sprite-Erstellung
     */
    bool init(LGFX &tft, const SettingsCallbacks &callbacks);

    /**
     * @brief Zyklische Update- und Zeichenfunktion (nicht blockierend).
     *
     * Verarbeitet die State-Machine für Verbindungstest und Dialoge, führt verzögerte Exit-/Neustart-Aktionen aus und pusht im Leerlauf nur die Sprites mit gesetztem Dirty-Flag.
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück.
     * @return AppModule::SETTINGS
     */
    AppModule type() const override;

private:
    // ========================================================================
    // TYPEN
    // ========================================================================

    /** @brief Zustände der internen State-Machine. */
    enum class State : uint8_t
    {
        IDLE,            ///< Normaler Betrieb
        TESTING,         ///< Verbindungstest läuft
        SHOWING_MESSAGE, ///< MessageBox aktiv
        SAVING           ///< Speicher-Dialog aktiv
    };

    /** @brief Status des Verbindungstests. */
    enum class TestState : uint8_t
    {
        NOT_TESTING, ///< Kein Test aktiv
        CONNECTING,  ///< Verbindung wird hergestellt
        CONNECTED,   ///< Erfolgreich verbunden
        FAILED,      ///< Verbindung fehlgeschlagen
        TIMEOUT      ///< Zeitüberschreitung
    };

    /** @brief Auswählbare Panels (Reihenfolge unabhängig vom Menü). */
    enum class Panel : uint8_t
    {
        WIFI,
        TIME,
        TIME_WINDOW,
        WEATHER,
        NEWS,
        CLOUD,
        LANGUAGE,
        SECURITY,
        AUDIO,
        APPEARANCE,
        INFO,
        COUNT
    };

    /** @brief Menü-Sektionen. */
    enum class Section : uint8_t
    {
        CONNECTION,
        SERVICES,
        SYSTEM
    };

    /** @brief Zeilentyp in der Menüliste. */
    enum class EntryType : uint8_t
    {
        SECTION_HEADER, ///< Sektionsüberschrift (nicht wählbar)
        PANEL_ITEM      ///< Panel-Eintrag
    };

    /** @brief Ein Eintrag der flachen Menüliste. */
    struct MenuEntry
    {
        EntryType type;     ///< SECTION_HEADER oder PANEL_ITEM
        Section section;    ///< Zugehörige Sektion (für Akzentfarbe)
        Panel panel;        ///< Zielpanel (bei SECTION_HEADER irrelevant)
        SettingsKey label;  ///< Schlüssel für den Anzeigetext
    };

    /** @brief Ein WLAN-Profil im Bearbeitungszustand. */
    struct WiFiProfile
    {
        char ssid[65];     ///< SSID (max. 64 Zeichen + Nullbyte)
        char password[65]; ///< Passwort (max. 64 Zeichen + Nullbyte)
        bool enabled;      ///< Profil aktiv
        bool modified;     ///< Seit letztem Speichern geändert
        bool tested_ok;    ///< Letzter Verbindungstest erfolgreich
    };

    /** @brief Vergleichsbasis für das Dirty-Tracking. */
    struct WiFiProfileBaseline
    {
        char ssid[65];
        char password[65];
        bool enabled;
    };

    /** @brief Achsenparalleles Rechteck (sprite-relativ). */
    struct Rect
    {
        int x; ///< Linke Kante
        int y; ///< Obere Kante
        int w; ///< Breite
        int h; ///< Höhe
    };

    /**
     * @brief Funktionsobjekt, das Anzeigetexte für die Listen-Auswahl liefert.
     *
     * Wird vom Listen-Picker pro sichtbarem Eintrag aufgerufen. Der Index läuft von 0 bis itemCount-1. Der Aufrufer muss den Puffer null-terminiert füllen; bei leerem Puffer bleibt die Zeile leer.
     */
    using ListPickerLabelFn = std::function<void(int index, char *buf, size_t bufSize)>;

    static const MenuEntry s_menu[]; ///< Statische Menü-Definition (oben nach unten)
    static const int s_menuCount;    ///< Anzahl der Menü-Einträge
    static constexpr int TIME_WINDOW_DAY_COUNT = AppState::TIME_WINDOW_DAY_COUNT;   ///< Anzahl Wochentage im Zeitfenster
    static constexpr int TIME_WINDOW_HOUR_COUNT = AppState::TIME_WINDOW_HOUR_COUNT; ///< Anzahl voller Stunden im Zeitfenster

    // ========================================================================
    // MEMBER
    // ========================================================================

    LGFX *m_tft = nullptr;          ///< Display-Objekt
    SettingsCallbacks m_callbacks;  ///< Event-Callbacks
    bool m_initialized = false;     ///< Initialisierungs-Status

    State m_state = State::IDLE;             ///< Aktueller State-Machine-Zustand
    TestState m_testState = TestState::NOT_TESTING; ///< Verbindungstest-Status

    Gfx_Sprite *m_menuSprite = nullptr;      ///< Menüliste (MENU_W x MENU_SPRITE_H)
    Gfx_Sprite *m_scrollbarSprite = nullptr; ///< Scrollbar (SCROLLBAR_W x MENU_SPRITE_H)
    Gfx_Sprite *m_workSprite = nullptr;      ///< Arbeitsbereich (WORK_W x WORK_H)
    Gfx_Sprite *m_clockSprite = nullptr;     ///< Digitaluhr (CLOCK_W x CLOCK_H)
    DotMatrix m_audioVolDots;                ///< Klang-Footer: Lautstärke-Zahl
    Gfx_Sprite *m_audioVolSprite = nullptr;  ///< Sprite für die Lautstärke-DotMatrix
    int m_scrollOffset = 0;                  ///< Vertikaler Scroll-Versatz der Menüliste
    int m_lastDrawnSecond = -1;              ///< Letzte gezeichnete Sekunde der Uhr

    Button m_saveButton; ///< Fußzeile: Speichern
    Button m_exitButton; ///< Fußzeile: Verlassen (Power-Symbol)
    Button m_btnScan;     ///< WLAN: Scannen
    Button m_btnPassword; ///< WLAN: Passwort
    Button m_btnTest;     ///< WLAN: Test
    Button m_btnDelete;   ///< WLAN: Löschen
    Button m_btnRestart;  ///< Info: Neustart
    Button m_btnReset;    ///< Info: Zurücksetzen
    Button m_cloudVisibilityButton; ///< Cloud: Werte anzeigen oder verstecken
    Button m_audioPlayBtn;   ///< Klang-Footer: Hörprobe Start/Stopp
    Button m_audioVolDownBtn; ///< Klang-Footer: Lautstärke leiser
    Button m_audioVolUpBtn;   ///< Klang-Footer: Lautstärke lauter

    bool m_menuDirty = true;   ///< Menüliste neu zeichnen
    bool m_workDirty = true;   ///< Arbeitsbereich neu zeichnen
    bool m_footerDirty = true; ///< Fußzeile neu zeichnen

    Panel m_panel = Panel::WIFI; ///< Aktuell gewähltes Panel

    std::array<WiFiProfile, MAX_PROFILES> m_profiles;          ///< Profile (Bearbeitung)
    std::array<WiFiProfileBaseline, MAX_PROFILES> m_baseline;  ///< Dirty-Vergleichsbasis
    int m_selWifi = 0;              ///< Im WLAN-Panel ausgewähltes Profil
    int m_currentProfileIndex = -1; ///< Profil des laufenden Tests
    bool m_changesMade = false;     ///< Mindestens ein Profil abweichend von der Baseline

    char m_hostname[33] = {0};         ///< Vorgemerkter Geräte-Netzwerkname
    char m_hostnameBaseline[33] = {0}; ///< Stand beim letzten Speichern oder Init

    char m_pinEntry[5] = {0}; ///< Eingegebene PIN-Ziffern (max. 4 + Nullbyte)
    int m_pinLen = 0;         ///< Anzahl eingegebener Ziffern
    char m_pinFirst[5] = {0};   ///< Erste PIN-Eingabe während der Wiederholung
    char m_pinPending[5] = {0}; ///< Vorgemerkte PIN (leer = keine), erst beim Speichern gesetzt
    bool m_pinConfirming = false; ///< true während der Wiederholungseingabe
    bool m_pinMismatch = false;   ///< true nach abweichender Wiederholung, bis zur nächsten Eingabe
    int m_pendingLangIndex = -1; ///< Vorgemerkte Sprache (0=de, 1=en), -1 = keine Auswahl
    WorkshopNumpad m_numpad;  ///< Nummernblock für die PIN-Eingabe
    bool m_numpadLayoutValid = false; ///< Numpad-Layout für aktuelle Größe berechnet

    char m_newsRessort[16] = {0};         ///< Aktuell vorgemerktes Nachrichten-Ressort (leer = kein Filter)
    int  m_newsRegion = 4;                ///< Aktuell vorgemerktes Bundesland (1..16)
    char m_newsRessortBaseline[16] = {0}; ///< Stand beim letzten Speichern oder Init
    int  m_newsRegionBaseline = 4;        ///< Stand beim letzten Speichern oder Init

    char m_plzEntry[8] = {0};    ///< Aktuell eingetippte Postleitzahl (max. 5 Ziffern + Nullbyte)
    int  m_plzLen = 0;           ///< Anzahl eingegebener Ziffern
    char m_plzBaseline[8] = {0}; ///< Stand beim letzten Speichern oder Init
    bool m_lastObservedWifi = false; ///< Letzter beobachteter WLAN-Zustand für Panel-Updates
    unsigned long m_lastInfoRefreshMs = 0; ///< Zeitpunkt des letzten Info-Panel-Refreshs

    char m_cloudCalendarUrl[256] = {0};         ///< Vorgemerkte Kalender-Adresse
    char m_cloudTodoUrl[256] = {0};             ///< Vorgemerkte Aufgaben-Adresse
    char m_cloudContactsUrl[256] = {0};         ///< Vorgemerkte Kontakte-Adresse
    char m_cloudOpenSkyClientId[128] = {0};     ///< Vorgemerkte OpenSky Client-ID
    char m_cloudOpenSkyClientSecret[192] = {0}; ///< Vorgemerktes OpenSky Client-Secret
    char m_cloudCalendarBaseline[256] = {0};    ///< Stand beim letzten Speichern oder Init
    char m_cloudTodoBaseline[256] = {0};        ///< Stand beim letzten Speichern oder Init
    char m_cloudContactsBaseline[256] = {0};    ///< Stand beim letzten Speichern oder Init
    char m_cloudOpenSkyClientIdBaseline[128] = {0}; ///< Stand beim letzten Speichern oder Init
    char m_cloudOpenSkyClientSecretBaseline[192] = {0}; ///< Stand beim letzten Speichern oder Init
    bool m_cloudValuesVisible = false; ///< true wenn Cloud-Werte im Panel sichtbar sind

    int  m_tzIndex = -1;                        ///< Aktuell vorgemerkter Zeitzonen-Index (-1 = keiner)
    int  m_tzBaseline = -1;                     ///< Stand beim letzten Speichern oder Init
    char m_ntpServer[64] = {0};                 ///< Vorgemerkter NTP-Server
    char m_ntpServerBaseline[64] = {0};         ///< Stand beim letzten Speichern oder Init
    unsigned long m_syncIntervalMs = 3600000UL; ///< Vorgemerktes NTP-Sync-Intervall
    unsigned long m_syncIntervalBaselineMs = 3600000UL; ///< Stand beim letzten Speichern oder Init
    int  m_manualYear  = 2025;                  ///< Stepper-Wert: Jahr
    int  m_manualMonth = 1;                     ///< Stepper-Wert: Monat (1-12)
    int  m_manualDay   = 1;                     ///< Stepper-Wert: Tag (1-31)
    int  m_manualHour  = 12;                    ///< Stepper-Wert: Stunde (0-23)
    int  m_manualMinute = 0;                    ///< Stepper-Wert: Minute (0-59)
    bool m_manualInit = false;                  ///< true sobald die Stepper aus der RTC vorinitialisiert wurden
    bool m_timeWindow[TIME_WINDOW_DAY_COUNT][TIME_WINDOW_HOUR_COUNT] = {}; ///< Zeitfenster-Matrix: Wochentag x Stunde, Standard aktiv
    uint32_t m_timeWindowBaselineMasks[TIME_WINDOW_DAY_COUNT] = {}; ///< Stand beim letzten Speichern oder Init

    uint16_t m_eqFreqLow = 500;           ///< Stepper-Wert: Eckfrequenz Tiefen in Hz
    uint16_t m_eqFreqPeak = 2000;         ///< Stepper-Wert: Mittenfrequenz in Hz
    uint16_t m_eqFreqHigh = 7000;         ///< Stepper-Wert: Eckfrequenz Höhen in Hz
    uint16_t m_stereoWidth = 100;         ///< Stepper-Wert: Basisbreite in Prozent
    uint16_t m_loudness = 0;              ///< Stepper-Wert: Loudness-Stärke in Prozent
    bool m_speakersEnabled = AppState::DEFAULT_SPEAKERS_ENABLED; ///< Sofort wirksame Lautsprecher-Freigabe
    uint16_t m_eqFreqLowBaseline = 500;   ///< Stand beim letzten Speichern oder Init
    uint16_t m_eqFreqPeakBaseline = 2000; ///< Stand beim letzten Speichern oder Init
    uint16_t m_eqFreqHighBaseline = 7000; ///< Stand beim letzten Speichern oder Init
    uint16_t m_stereoWidthBaseline = 100; ///< Stand beim letzten Speichern oder Init
    uint16_t m_loudnessBaseline = 0;      ///< Stand beim letzten Speichern oder Init
    bool m_speakersEnabledBaseline = AppState::DEFAULT_SPEAKERS_ENABLED; ///< Lautsprecher-Freigabe beim letzten Speichern oder Init
    bool m_previewPlaying = false;        ///< true, solange die Hörprobe im Klang-Panel läuft

    uint8_t m_magicEyeStyle = 0;         ///< Stepper-Wert: Stil des magischen Auges (0 = EM11, 1 = EM34, 2 = EM80, 3 = EM84)
    uint8_t m_magicEyeStyleBaseline = 0; ///< Stand beim letzten Speichern oder Init
    uint8_t m_vuBallistics = 1;          ///< Gewählte Trägheit des VU-Zeigers (0 = 150 ms bis 2 = 600 ms)
    uint8_t m_vuBallisticsBaseline = 1;  ///< Stand beim letzten Speichern oder Init
    uint8_t m_vuSensitivity = 2;         ///< Gewählter Bezugspegel des VU (0 = -2 dBFS bis 4 = -18 dBFS)
    uint8_t m_vuSensitivityBaseline = 2; ///< Stand beim letzten Speichern oder Init
    unsigned long m_appearanceLevelMs = 0; ///< Zeitpunkt der letzten Auffrischung der Pegelanzeige
    float m_appearanceLevelDb = -60.0f;    ///< Geglätteter Senderpegel in dBFS
    char m_appearanceLevelText[12] = {0};  ///< Zuletzt geschriebener Text, verhindert unnötiges Neuzeichnen

    bool m_requireTouchRelease = true; ///< Erst vollständiges Loslassen nach Screenwechsel abwarten
    uint32_t m_touchBlockUntil = 0;    ///< Bis zu diesem Zeitpunkt keine Touch-Verarbeitung (ms)
    bool m_touchProcessed = false;     ///< Verhindert Mehrfachauslösung pro Berührung

    uint32_t m_operationStartTime = 0; ///< Startzeit für zeitbasierte Operationen

    std::unique_ptr<MessageBox> m_activeMessageBox;   ///< Aktive MessageBox
    std::unique_ptr<ProgressBox> m_activeProgressBox; ///< Aktive ProgressBox

    bool m_exitRequested = false;    ///< Exit verzögert anfordern
    bool m_restartRequested = false; ///< Neustart verzögert anfordern

    // ========================================================================
    // METHODEN
    // ========================================================================

    bool createSprites();
    void deleteSprites();
    void initButtons();
    void initToolButton(Button &btn, const Rect &spriteRect);

    void redrawAll();
    void invalidateAll();

    void drawMenu();
    void drawScrollbar();
    int getTotalListHeight() const;
    void drawWorkPanel();
    void drawFooter();
    void drawClock();

    void drawPanelWifi(Gfx_Sprite *spr);
    void drawWifiRow(Gfx_Sprite *spr, int idx, const Rect &r, bool selected);
    void drawPanelLanguage(Gfx_Sprite *spr);
    void drawPanelSecurity(Gfx_Sprite *spr);
    void drawPanelInfo(Gfx_Sprite *spr);
    void drawPanelNews(Gfx_Sprite *spr);
    void drawPanelWeather(Gfx_Sprite *spr);
    void drawPanelCloud(Gfx_Sprite *spr);
    void drawPanelTime(Gfx_Sprite *spr);
    void drawPanelTimeWindow(Gfx_Sprite *spr);
    void drawPanelAudio(Gfx_Sprite *spr);
    void drawPanelAppearance(Gfx_Sprite *spr);
    void drawPanelPlaceholder(Gfx_Sprite *spr);

    SettingsKey panelTitleKey(Panel p) const;
    SettingsKey panelSubtitleKey(Panel p) const;
    uint16_t sectionColor(Section s) const;

    int menuEntryY(int index) const;
    int menuEntryAtY(int y) const;
    void selectPanel(Panel p);

    Rect wifiRowRect(int idx) const;
    Rect wifiToggleRect(int idx) const;
    void drawToggle(Gfx_Sprite *spr, const Rect &rect, bool on, uint16_t background); ///< Zeichnet einen Umschalter in einen vorhandenen Sprite
    Rect wifiActionRect(int slot) const;
    Rect hostnameFieldRect() const;
    Rect langRowRect(int idx) const;
    Rect footerInfoButtonRect(int slot) const;
    Rect footerTimeWindowResetButtonRect() const;
    Rect footerCloudClearButtonRect() const;
    int footerButtonWidthForLabel(SettingsKey labelKey) const;
    int footerButtonWidthForLabels(SettingsKey firstKey, SettingsKey secondKey) const;
    void initFooterTextButton(Button &btn, const Rect &rect, SettingsKey labelKey);
    Rect newsRessortChipRect(int idx) const;
    Rect newsRegionSelectorRect() const;

    bool handleTouch();
    void handleMenuTouch(int32_t tx, int32_t ty);
    void handleWorkTouch(int32_t tx, int32_t ty);
    void handleWifiTouch(int sx, int sy);
    void handleLanguageTouch(int sx, int sy);
    void handleSecurityTouch(int sx, int sy);
    void handleNewsTouch(int sx, int sy);
    void handleWeatherTouch(int sx, int sy);
    void handleCloudTouch(int sx, int sy);
    void handleTimeTouch(int sx, int sy);
    void handleTimeWindowTouch(int sx, int sy);
    void handleAudioTouch(int sx, int sy);
    void handleAppearanceTouch(int sx, int sy);

    Rect cloudRowRect(int idx) const;
    Rect cloudVisibilityButtonRect() const;
    void startCloudUrlInput(int slot);
    void startCloudClearDialog();

    Rect timeFieldRect(int idx) const;
    Rect timeStepperColumnRect(int col) const;
    Rect timeStepperUpRect(int col) const;
    Rect timeStepperDownRect(int col) const;
    Rect timeStepperValueRect(int col) const;
    Rect timeSetButtonRect() const;
    Rect timeWindowCellRect(int day, int hour) const;
    uint32_t timeWindowDayMask(int day) const;
    void loadTimeWindowFromAppState();
    void saveTimeWindow();
    void persistTimeWindow(bool showMessage);
    void startTimeWindowResetDialog();
    void resetTimeWindow();

    Rect audioStepperColumnRect(int row) const;
    Rect audioStepperUpRect(int row) const;
    Rect audioStepperDownRect(int row) const;
    Rect audioStepperValueRect(int row) const;
    void drawAudioFooterControls(); ///< Zeichnet Hörprobe-Knopf und Lautstärke im Footer (nur Klang-Panel)
    void drawAudioVolumeValue();    ///< Aktualisiert die Lautstärke-DotMatrix im Footer

    Rect appearanceTileRect(int index) const;
    Rect appearanceSegmentRect(int rowY, int index, int count) const;
    void drawAppearanceSectionTitle(Gfx_Sprite *spr, int y, SettingsKey titleKey);
    void drawAppearanceHint(Gfx_Sprite *spr, int y, SettingsKey key);
    void drawAppearanceEyeTiles(Gfx_Sprite *spr);
    void drawAppearanceLevel();
    void drawAppearanceSegments(Gfx_Sprite *spr, int rowY, SettingsKey labelKey,
                               const char *const *values, int count, int selected);

    void startTimezonePicker();
    void startSyncIntervalPicker();
    void startNtpServerInput();
    void startSetTimeDialog();
    void applyManualTime();
    void clampManualDay();
    void initManualFromRtc();

    void updateTestingState();
    void updateMessageState();
    void updateSavingState();
    void changeState(State newState);
    bool handleDeferredActions();
    void performRestart();

    void onToggleWifi(int profileIndex);
    void startScan(int profileIndex);
    void startPasswordInput(int profileIndex);
    void startConnectionTest(int profileIndex);
    void startClearProfileDialog(int profileIndex);
    void startHostnameInput();
    bool checkTestStatus();
    void showTestResult();
    bool validateAndNormalizePassword(const char *input, char *output, size_t outSize,
                                      SettingsKey &errorKey) const;
    bool validateHostname(const char *input, char *output, size_t outSize,
                          SettingsKey &errorKey) const;

    void startSaving();
    void startExitDialog();
    void startRestartDialog();
    void startResetDialog();
    void startLanguageRestartDialog();
    void saveProfiles();
    void saveNews();
    void persistNews(bool showMessage);
    void saveWeather();
    void persistPlz();
    void saveCloud();
    bool persistCloud(bool showMessage);
    void saveTime();
    void persistTime(bool showMessage);
    void saveAudio();
    void persistAudio(bool showMessage);
    void saveAppearance();

    /** @brief Mögliche Probleme einer Cloud-Adresse. */
    enum class CloudUrlIssue : uint8_t
    {
        NONE,            ///< Adresse ist leer oder gültig
        PREFIX_MISSING,  ///< Beginnt nicht mit "https://script.google.com/"
        SUFFIX_MISSING   ///< Endet nicht auf "/exec"
    };

    /**
     * @brief Prüft eine Cloud-Adresse auf das erwartete Schema.
     *
     * Leere Adressen gelten als gültig (NONE). Andernfalls muss die Adresse mit `https://script.google.com/` beginnen und auf `/exec` enden. Die Prüfung deckt nur das grobe Format ab; ob die Adresse tatsächlich erreichbar ist, sagt sie nicht.
     */
    CloudUrlIssue validateCloudUrl(const char *url) const;

    /** @brief Ergebnis eines PLZ-Auflösungs-Versuchs. */
    enum class PlzLookupStatus : uint8_t
    {
        SUCCESS,        ///< Ort und Koordinaten erfolgreich ermittelt
        NO_WIFI,        ///< Keine WLAN-Verbindung
        NOT_FOUND,      ///< Postleitzahl unbekannt
        OUTSIDE_DE,     ///< Treffer liegt außerhalb der Deutschland-Bounding-Box
        REQUEST_FAILED, ///< HTTP-Fehler, JSON-Fehler oder leere Antwort
    };

    /**
     * @brief Löst eine 5-stellige deutsche Postleitzahl auf.
     * @param plz Postleitzahl (genau 5 Ziffern, sonst REQUEST_FAILED)
     * @param outCity Puffer für den Ortsnamen (mindestens 64 Byte empfohlen)
     * @param outCitySize Größe des Ortsnamen-Puffers
     * @param outLat Ausgabe: Breitengrad
     * @param outLon Ausgabe: Längengrad
     * @param outRegion Ausgabe: Bundesland-ID nach Settings-Konvention (1..16, 0 bei unbekannt)
     * @return Ergebnis-Status
     *
     * Spricht die Nominatim-API von OpenStreetMap an (`/search?postalcode=...&countrycodes=DE`), wertet das erste Ergebnis aus, prüft die Deutschland-Bounding-Box und leitet aus dem Bundesland eine Nachrichten-Region ab. Bleibt blockierend bis zur Antwort oder zum Fehler; der Aufrufer rahmt die Wartezeit mit einer ProgressBox ein.
     */
    PlzLookupStatus resolvePlz(const char *plz,
                               char *outCity, size_t outCitySize,
                               double &outLat, double &outLon,
                               int &outRegion);

    void snapshotBaseline();
    void recomputeDirtyFlags();
    bool panelDirty(Panel p) const;
    bool hasProfileChanges() const;
    bool hostnamePending() const;
    bool newsPending() const;
    bool weatherPending() const;
    bool cloudPending() const;
    bool timePending() const;
    bool timeWindowPending() const;
    bool audioPending() const;
    bool appearancePending() const;

    bool isProfileValid(int profileIndex) const;
    bool isProfileConfigured(int profileIndex) const;
    SettingsKey wifiStatusKey(int profileIndex) const;
    uint16_t wifiStatusColor(int profileIndex) const;

    void applyLanguage(int langIndex);
    int currentLangIndex() const;
    bool languagePending() const;
    void handlePinKey(char action);
    bool applyPendingPin();
    bool pinPending() const;
    void resetPinEntry();

    void handlePlzKey(char action);

    /**
     * @brief Zeigt einen modalen Listen-Picker und liefert den gewählten Index.
     * @param titleKey Überschrift des Pickers (anwender-sichtbar)
     * @param itemCount Anzahl der Einträge (>= 1)
     * @param labelFn Liefert den Anzeigetext für einen gegebenen Index in einen Puffer
     * @param initialIndex Vorausgewählter Eintrag (0..itemCount-1, sonst geklemmt)
     * @return Gewählter Index oder -1 bei Abbruch
     *
     * Blockiert bis zur Auswahl oder zum Abbruch (wie Keyboard/WifiList). Nach der Rückkehr ruft der Aufrufer afterModalRedraw() auf, damit das Hauptlayout wiederhergestellt wird.
     */
    int runListPicker(SettingsKey titleKey,
                      int itemCount,
                      ListPickerLabelFn labelFn,
                      int initialIndex);

    void showInfoBox(SettingsKey titleKey, SettingsKey messageKey);
    void showInfoBoxText(SettingsKey titleKey, const char *message);
    void afterModalRedraw();
};

#endif // SETTINGS_H
