#include "MainScreen.h"

#include <cmath> // für cosf, sinf in der Analog-Uhr
#include <time.h>

#include "RetroRadio.h"
#include "SDPlayer.h"
#include "Workshop.h"
#include "AppState.h"
#include "Calendar.h"
#include "ToDo.h"
#include "Weather.h"
#include "DS1307_RTC.h"
#include "Contacts.h"
#include "Pomodoro.h"
#include "Breather.h"
#include "DayView.h"
#include "Settings.h"
#include "Off.h"
#include "StationManager.h"
#include "I18n.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Theme.h"
#include "IconShapes.h"
#include "SystemStats.h"

#include <WiFi.h> // Für IP-Anzeige in der Uhr

// Fonts
#include "fonts/ComfortaaBold30.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold10.h"

// Icons

// Audio-Steuerungs-Callbacks (extern)
extern void setNewStation(uint8_t id);
extern void stopCurrentStation();
extern void handleButtonPressed(uint16_t btnIdx, bool buttonState);
extern void handleVolumeChanged(uint8_t volume);
extern void handleVolumeBeforeMute(uint8_t volume);
extern void handleMuteChanged(bool mute);
extern void handleBalanceChanged(int8_t balance);
extern void handleGainLowPassChanged(int8_t gainLowPass);
extern void handleGainBandPassChanged(int8_t gainBandPass);
extern void handleGainHighPassChanged(int8_t gainHighPass);

// SDPlayer spezifische Callbacks
extern void handlePlayFileRequested(const char *path);
extern void handleStopRequested();
extern void handlePauseResumeRequested();
extern void handleSeekOffsetRequested(int sec);

extern void setActiveSdPlayer(SdPlayer *player);

// ===================================================================
// Layout-Konstanten
// ===================================================================
namespace Layout
{
    // Display-Grundmaße
    constexpr int DISPLAY_WIDTH = 800;  ///< Display-Breite in Pixel
    constexpr int DISPLAY_HEIGHT = 480; ///< Display-Höhe in Pixel

    constexpr int EDGE_MARGIN = 22; ///< Abstand des Button-Grids zu den Display-Kanten

    // Rechte Seite (Buttons)
    constexpr int BUTTON_SIZE = 100;   ///< Quadratische Buttons
    constexpr int BUTTON_SPACING = 12; ///< Abstand zwischen Buttons
    constexpr int BUTTON_COLS = 3;     ///< Anzahl Button-Spalten
    constexpr int BUTTON_ROWS = 4;     ///< Anzahl Button-Zeilen

    // Berechnete Grid-Dimensionen
    constexpr int BUTTON_GRID_W = BUTTON_COLS * BUTTON_SIZE + (BUTTON_COLS - 1) * BUTTON_SPACING; ///< Gesamtbreite des Button-Grids
    constexpr int BUTTON_GRID_H = BUTTON_ROWS * BUTTON_SIZE + (BUTTON_ROWS - 1) * BUTTON_SPACING; ///< Gesamthöhe des Button-Grids

    // Button-Startpositionen
    constexpr int BUTTON_START_Y = EDGE_MARGIN;                                 ///< Abstand des Button-Grids zum oberen Rand
    constexpr int BUTTON_START_X = DISPLAY_WIDTH - EDGE_MARGIN - BUTTON_GRID_W; ///< Abstand des Button-Grids zum rechten Rand

    // Linke Seite: Uhr + Datum
    constexpr int MARK_INSET = 10; ///< Abstand der Stundenmarkierungen vom Uhr-Rand

    // Datum-Geometrie
    constexpr int DATE_CACHE_H = 36;      ///< Höhe der Datumszeile
    constexpr int CLOCK_DATE_OFFSET = 10; ///< Abstand Uhr -> Datum

    // Uhr-Radius aus sichtbarer Fläche berechnen
    constexpr int CLOCK_DIAMETER = BUTTON_GRID_H + MARK_INSET - CLOCK_DATE_OFFSET - DATE_CACHE_H;
    constexpr float CLOCK_RADIUS = CLOCK_DIAMETER / 2.0f;

    // Sprite etwas höher als Button-Grid (wegen 'MARK_INSET' am oberen Rand)
    constexpr int SPRITE_WIDTH = CLOCK_DIAMETER + 30;                                ///< Sprite-Breite (etwas größer als Uhr-Durchmesser für Status-Indikatoren)
    constexpr int SPRITE_HEIGHT = CLOCK_DIAMETER + CLOCK_DATE_OFFSET + DATE_CACHE_H; ///< Sprite-Höhe (Uhr + Abstand + Datum)
    constexpr int SPRITE_X = 12;                                                     ///< Abstand des Sprites zum linken Rand
    constexpr int SPRITE_Y = BUTTON_START_Y - MARK_INSET;                            ///< 10px über Button-Top

    // Analog-Uhr
    constexpr int CLOCK_CENTER_X = SPRITE_WIDTH / 2;   ///< X-Koordinate des Uhr-Zentrums im Sprite
    constexpr int CLOCK_CENTER_Y = CLOCK_DIAMETER / 2; ///< Oberkante Kreis bei Y=0
    constexpr int DATE_CACHE_W = SPRITE_WIDTH;         ///< Breite des Datum-Cache-Sprites

    // WiFi-Signalbalken
    constexpr int SIGNAL_BAR_COUNT = 5;            ///< Anzahl der Signalbalken
    constexpr int SIGNAL_BAR_WIDTH = 4;            ///< Breite eines einzelnen Signalbalkens
    constexpr int SIGNAL_BAR_GAP = 4;              ///< Abstand zwischen den Signalbalken
    constexpr int SIGNAL_BAR_MIN_H = 6;            ///< Höhe des kleinsten Signalbalkens
    constexpr int SIGNAL_BAR_STEP_H = 5;           ///< Zusätzliche Höhe pro Signalstärke-Level
    constexpr float SIGNAL_BAR_RADIAL_POS = 0.36f; ///< Radiale Position der Balken (höher als zuvor wegen IP-Zeile)
    constexpr int SSID_OFFSET_Y = 6;               ///< Abstand WiFi-Balken -> SSID-Text
    constexpr int IP_OFFSET_Y = 13;                ///< Abstand SSID-Baseline -> IP-Text

    // NTP/DST Label-Cache-Sprites
    constexpr int LABEL_CACHE_W = 42;         ///< Breite der NTP/DST-Label-Caches
    constexpr int LABEL_CACHE_H = 18;         ///< Höhe der NTP/DST-Label-Caches
    constexpr float LABEL_RADIAL_POS = 0.68f; ///< Radiale Position der Labels (0=Zentrum, 1=Rand)

    // Kalender-Button
    constexpr int CALENDAR_LABEL_OFFSET_Y = 14; ///< Vertikale Verschiebung der Tageszahl gegenüber der Button-Mitte
} // namespace Layout

// Platzhalter, wenn die Systemzeit noch nicht plausibel ist (kein NTP/keine gepufferte RTC).
namespace
{
    constexpr const char *DATE_PLACEHOLDER = "--. ---. ----";
    constexpr const char *DAY_PLACEHOLDER = "--";
}

// ===================================================================
// Button-Index-Zuordnung (4 Zeilen x 3 Spalten)
// ===================================================================
//
//  Zeile 0:  [0] Radio      [ 1] SD-Player  [ 2] DayView
//  Zeile 1:  [3] Kalender   [ 4] ToDo       [ 5] Kontakte
//  Zeile 2:  [6] Wetter     [ 7] Pomodoro   [ 8] Zeiterfassung
//  Zeile 3:  [9] Settings   [10] Werkstatt  [11] Aus
//

/**
 * @brief Konstruktor
 */
MainScreen::MainScreen()
{
}

/**
 * @brief Destruktor
 */
MainScreen::~MainScreen()
{
    // Sprites nur löschen wenn m_tft gültig ist
    if (m_tft)
    {
        // Datum-Cache-Sprite freigeben
        m_tft->safeDeleteSprite(m_sprDateCache);

        // Label-Cache-Sprites freigeben
        m_tft->safeDeleteSprite(m_sprLabelNtp);
        m_tft->safeDeleteSprite(m_sprLabelDst);

        if (m_sprMainScreen)
            m_tft->safeDeleteSprite(m_sprMainScreen);
    }

    // Tinted-Icon-Puffer in PSRAM freigeben
    for (int i = 0; i < ICON_COUNT; ++i)
    {
        if (m_tintedIcons[i])
        {
            free(m_tintedIcons[i]);
            m_tintedIcons[i] = nullptr;
        }
    }
}

/**
 * @brief Initialisierung
 */
bool MainScreen::init(LGFX &tft, DS1307_RTC &rtc, StationManager &stationManager, Audio &audio)
{
    // Referenzen speichern
    m_tft = &tft;
    m_rtc = &rtc;
    m_stationManager = &stationManager;
    m_audio = &audio;

    if (!m_tft->safeCreateSprite(m_sprMainScreen, Layout::SPRITE_WIDTH, Layout::SPRITE_HEIGHT))
    {
        LOG_ERROR("Konnte Sprite für das Hauptdisplay nicht erstellen!");
        return false;
    }

    // Zeichenzustand initialisieren
    m_drawStep = 0;
    m_drawState = IDLE;
    m_needsRedraw = true;   // Initial brauchen wir ein Redraw
    m_lastDrawnSecond = -1; // Initialisierung für Uhr-Update

    // Status-Cache initialisieren (erzwingt Updates beim ersten Durchlauf)
    m_lastNtpStatus = false;
    m_lastDstStatus = false;
    m_lastWifiConnected = false;
    m_lastWifiPercent = 255; // Ungültiger Wert erzwingt erstes Update
    m_lastUpdatedDay = 0;
    m_dateCacheValid = false;   // Datum-Cache beim ersten Frame neu rendern
    m_labelCachesValid = false; // NTP/DST-Labels beim ersten Frame rendern

    // Sprite-Puffer initial schwarz leeren
    m_sprMainScreen->clear(TFT_BLACK);

    // Buttons initialisieren und zeichnen
    initButtons();

    // Sperrbildschirm initialisieren und als Boot-Gate aktivieren
    m_lockScreen.init(tft);
    m_lockScreen.activate();
    m_lockScreenActive = true;

    m_initialized = true;
    LOG_SUCCESS("MainScreen wurde erfolgreich initialisiert.");

    return true;
}

// ===================================================================
// Haupt-Update-Logik
// ===================================================================

/**
 * @brief Haupt-Update-Funktion mit Modul-Management
 */
void MainScreen::update()
{
    if (!m_initialized)
        return;

    if (updateTimeWindowSleep())
        return;

    // Display-Aufwecken prüfen
    if (!APP_STATE.isDisplayOn())
    {
        int32_t tx, ty;
        const bool touched = m_tft->getTouch(&tx, &ty);

        // Wurde mit noch liegendem Finger ausgeschaltet (langer Druck auf Off), erst ein echtes Loslassen abwarten, sonst weckt derselbe Touch sofort wieder.
        if (m_wakeRequiresRelease)
        {
            if (!touched)
                m_wakeRequiresRelease = false;

            return;
        }

        if (touched)
        {
            uint8_t day = TIME_WINDOW_SLOT_INVALID;
            uint8_t hour = TIME_WINDOW_SLOT_INVALID;
            if (getCurrentTimeWindowSlot(day, hour) && !APP_STATE.isTimeWindowEnabled(day, hour))
                rememberManualTimeWindowWake(day, hour);

            extern void setBacklight(bool on);
            setBacklight(true);
            m_timeWindowSleepActive = false;
            m_wakeRequiresRelease = false;
            APP_STATE.setBlockGFX(false);

            if (isManualTimeWindowWake(day, hour))
            {
                triggerLockScreen();
            }
            else if (m_lockScreenActive)
            {
                m_lockScreen.activate();
            }
            else
            {
                requestRedraw(true);
                drawButtons();
            }
        }

        return;
    }

    // Sperrbildschirm hat Vorrang vor allem anderen
    if (m_lockScreenActive)
    {
        if (m_lockScreen.update())
        {
            m_lockScreenActive = false;
            m_tft->clear(TFT_BLACK);
            requestRedraw(true);

            // Erst den Zustand setzen, dann zeichnen: Andernfalls zeichnet drawButtons die vom WLAN abhängigen Kacheln aktiv und disable sie unmittelbar danach ein zweites Mal ab.
            updateButtonStates(false);
            drawButtons();
        }

        return;
    }

    // Angeforderten Modul-Wechsel verarbeiten
    if (m_requestedModule != AppModule::NONE)
    {
        switchToModule(m_requestedModule);
        m_requestedModule = AppModule::NONE;
    }

    // Angefordertes Modul schließen
    if (m_moduleCloseRequested)
    {
        closeActiveModule();
        m_moduleCloseRequested = false;

        // Nach dem Schließen prüfen ob Sleep-Mode
        if (APP_STATE.getSleepModeTriggered())
        {
            APP_STATE.setSleepModeTriggered(false);

            // Beim nächsten Aufwachen direkt den Sperrbildschirm zeigen
            m_lockScreenActive = true;

            extern void setBacklight(bool on);
            setBacklight(false);

            return;
        }
    }

    // Sperrbildschirm-Anforderung global prüfen
    if (APP_STATE.consumeLockRequest())
    {
        triggerLockScreen();
        return;
    }

    // Modul oder MainScreen updaten
    if (m_activeModule)
    {
        // Sub-Modul ist aktiv und hat das alleinige Touch-Handling. Active-Corner-Geste ist hier deaktiviert, damit das Modul nicht unbeendet aus dem Speicher gerissen wird. Ein Lock muss bei aktivem Modul über APP_STATE.requestLock() angestoßen werden, das oben vor diesem Branch ausgewertet wird.
        m_activeModule->draw();
    }
    else
    {
        // Touch einmal pro Frame lesen
        m_frameTouched = m_tft->getTouch(&m_frameTouchX, &m_frameTouchY);

        // Active-Corner-Geste prüfen
        if (checkActiveCorner())
            return;

        // Während einer laufenden Geste kein Button-Handling
        if (!m_cornerTouchActive)
            handleMainScreenTouch();

        updateMainScreen();
    }
}

/**
 * @brief Touch-Handling für MainScreen
 */
void MainScreen::handleMainScreenTouch()
{
    if (handleOffButton())
        return;

    int8_t pressedButton = getPressedButton();
    if (pressedButton != -1)
    {
        LOG_INFO_F("Button %d gedrückt", pressedButton);

        // Über Mapping-Funktion das Modul ermitteln
        AppModule module = buttonIndexToModule(pressedButton);
        if (module != AppModule::NONE)
            m_requestedModule = module;
    }
}

/**
 * @brief Sonderbehandlung des Off-Buttons (kurzer Tipp öffnet, langes Halten schaltet aus)
 */
bool MainScreen::handleOffButton()
{
    Button *offBtn = m_buttons[BUTTON_INDEX_OFF].get();
    if (!offBtn)
        return false;

    const bool onOffButton = m_frameTouched && offBtn->contains(m_frameTouchX, m_frameTouchY) && offBtn->isEnabled();

    const unsigned long now = millis();

    if (onOffButton)
    {
        if (!m_offButtonHeld)
        {
            m_offButtonHeld = true;
            m_offLongPressFired = false;
            m_offPressStartMs = now;
            offBtn->drawButton(true);
        }
        else if (!m_offLongPressFired && (now - m_offPressStartMs >= OFF_LONG_PRESS_MS))
        {
            m_offLongPressFired = true;
            LOG_INFO("Off-Button lang gedrückt -> sofort ausschalten");
            m_wakeRequiresRelease = true; // Finger liegt beim Ausschalten noch auf dem Glas
            requestPowerOff();
        }

        return true;
    }

    if (m_offButtonHeld)
    {
        m_offButtonHeld = false;
        offBtn->drawButton(false);

        if (!m_offLongPressFired)
            m_requestedModule = AppModule::OFF; // kurzer Tipp öffnet den Off-Bildschirm

        m_offLongPressFired = false;
        return true;
    }

    return false;
}

/**
 * @brief Schaltet das Gerät aus (Sleep aktivieren, aktives Modul schließen)
 */
void MainScreen::requestPowerOff()
{
    APP_STATE.setSleepModeTriggered(true);
    m_moduleCloseRequested = true;
}

/**
 * @brief Prüft die Zeitfenster-Logik und schaltet bei Bedarf Display und Module aus.
 */
bool MainScreen::updateTimeWindowSleep()
{
    uint8_t day = TIME_WINDOW_SLOT_INVALID;
    uint8_t hour = TIME_WINDOW_SLOT_INVALID;

    if (!getCurrentTimeWindowSlot(day, hour))
    {
        clearManualTimeWindowWake();
        if (!APP_STATE.isDisplayOn() && m_timeWindowSleepActive)
            wakeFromTimeWindowSleep();
        m_timeWindowSleepActive = false;
        return false;
    }

    if (day != m_timeWindowLastDay || hour != m_timeWindowLastHour)
    {
        m_timeWindowLastDay = day;
        m_timeWindowLastHour = hour;

        if (!isManualTimeWindowWake(day, hour))
            clearManualTimeWindowWake();
    }

    if (APP_STATE.isTimeWindowEnabled(day, hour))
    {
        clearManualTimeWindowWake();
        if (!APP_STATE.isDisplayOn() && m_timeWindowSleepActive)
            wakeFromTimeWindowSleep();
        m_timeWindowSleepActive = false;
        return false;
    }

    if (!APP_STATE.isDisplayOn())
        return false;

    if (isManualTimeWindowWake(day, hour))
        return false;

    enterTimeWindowSleep();
    return true;
}

/**
 * @brief Ermittelt den aktuellen Zeitfenster-Slot.
 */
bool MainScreen::getCurrentTimeWindowSlot(uint8_t &day, uint8_t &hour) const
{
    day = TIME_WINDOW_SLOT_INVALID;
    hour = TIME_WINDOW_SLOT_INVALID;

    if (!DS1307_RTC::isSystemTimeValid())
        return false;

    struct tm localTime = DS1307_RTC::getCurrentLocalTime();
    if (localTime.tm_wday < 0 || localTime.tm_wday > 6 || localTime.tm_hour < 0 || localTime.tm_hour > 23)
        return false;

    day = (localTime.tm_wday == 0) ? 6 : static_cast<uint8_t>(localTime.tm_wday - 1);
    hour = static_cast<uint8_t>(localTime.tm_hour);

    return true;
}

/**
 * @brief Prüft, ob die aktuelle inaktive Stunde manuell aufgeweckt wurde.
 */
bool MainScreen::isManualTimeWindowWake(uint8_t day, uint8_t hour) const
{
    return m_timeWindowWakeDay == day && m_timeWindowWakeHour == hour;
}

/**
 * @brief Merkt ein manuelles Aufwecken für die aktuelle inaktive Stunde.
 */
void MainScreen::rememberManualTimeWindowWake(uint8_t day, uint8_t hour)
{
    m_timeWindowWakeDay = day;
    m_timeWindowWakeHour = hour;
}

/**
 * @brief Löscht das manuelle Zeitfenster-Aufwecken.
 */
void MainScreen::clearManualTimeWindowWake()
{
    m_timeWindowWakeDay = TIME_WINDOW_SLOT_INVALID;
    m_timeWindowWakeHour = TIME_WINDOW_SLOT_INVALID;
}

/**
 * @brief Schaltet wegen inaktivem Zeitfenster in den Schlafmodus.
 */
void MainScreen::enterTimeWindowSleep()
{
    m_timeWindowSleepActive = true;
    m_requestedModule = AppModule::NONE;
    m_moduleCloseRequested = false;
    APP_STATE.setSleepModeTriggered(false);

    closeActiveModule();
    APP_STATE.setBlockGFX(false);
    triggerLockScreen();
    m_wakeRequiresRelease = true;

    extern void setBacklight(bool on);
    setBacklight(false);
}

/**
 * @brief Weckt das Gerät am Beginn eines aktiven Zeitfensters.
 */
void MainScreen::wakeFromTimeWindowSleep()
{
    extern void setBacklight(bool on);
    setBacklight(true);
    m_timeWindowSleepActive = false;
    m_wakeRequiresRelease = false;
    APP_STATE.setBlockGFX(false);
    triggerLockScreen();
}

/**
 * @brief Update-Logik für MainScreen
 */
void MainScreen::updateMainScreen()
{
    // Aktuellen WiFi-Status prüfen
    bool currentWifiStatus = APP_STATE.getWifiConnected();

    // Hat sich der WiFi-Connect-Status geändert?
    bool wifiStatusChanged = (currentWifiStatus != m_lastWifiConnected);
    if (wifiStatusChanged)
    {
        updateButtonStates(); // Buttons anpassen
        m_lastWifiConnected = currentWifiStatus;
    }

    // Aktuelle Statuswerte lesen
    bool currentNtpStatus = APP_STATE.getNtpSynced();
    bool currentDstStatus = APP_STATE.getDstActive();

    // Prüfen, ob sich einer der Werte für die Statusanzeige geändert hat
    if (currentNtpStatus != m_lastNtpStatus || currentDstStatus != m_lastDstStatus || wifiStatusChanged)
    {
        m_lastNtpStatus = currentNtpStatus;
        m_lastDstStatus = currentDstStatus;
        m_needsRedraw = true; // Status-Marker in der Uhr aktualisieren
    }

    // WiFi-Signal bei Prozentänderung aktualisieren
    uint8_t curPct = APP_STATE.getWifiSignalPercent();
    if (curPct != m_lastWifiPercent)
    {
        m_lastWifiPercent = curPct;
        m_needsRedraw = true; // WiFi-Balken in der Uhr aktualisieren
    }

    // Systemzeit einmalig für Kalender-Button und Sekunden-Check holen
    time_t nowEpoch = time(nullptr);
    struct tm nowLocal;
    localtime_r(&nowEpoch, &nowLocal);
    bool timeValid = DS1307_RTC::isSystemTimeValid();

    // Kalender-Button (Index 3) aktualisieren wenn sich der Tag ändert. Bei ungültiger Systemzeit wird der Platzhalter angezeigt, bis NTP oder RTC die Zeit setzt.
    if (m_buttons[BUTTON_INDEX_CALENDAR])
    {
        int currentDay = timeValid ? nowLocal.tm_mday : 0;

        if (currentDay != m_lastUpdatedDay)
        {
            char dayStr[4];
            if (timeValid)
                stringutils::safeFormat(dayStr, "%d", nowLocal.tm_mday);
            else
                stringutils::safeCopy(dayStr, DAY_PLACEHOLDER);

            m_buttons[BUTTON_INDEX_CALENDAR]->setLabelText(dayStr);
            m_buttons[BUTTON_INDEX_CALENDAR]->setLabelOffset(0, Layout::CALENDAR_LABEL_OFFSET_Y);
            m_buttons[BUTTON_INDEX_CALENDAR]->drawButton();

            m_lastUpdatedDay = currentDay;
            m_dateCacheValid = false; // Datum-Cache bei Tageswechsel neu rendern
        }
    }

    unsigned long currentTime = millis();

    // Sekundenwechsel erzwingt Redraw der Uhr
    if (m_lastDrawnSecond != nowLocal.tm_sec)
    {
        m_lastDrawnSecond = nowLocal.tm_sec;
        m_needsRedraw = true;
    }

    // Prüfen ob genug Zeit vergangen ist für den nächsten Zeichenschritt
    if (currentTime - m_lastDrawTime < m_drawInterval)
        return;

    m_lastDrawTime = currentTime;

    // Wenn GFX blockiert ist, den gesamten Zeichen- und Swap-Vorgang überspringen
    if (APP_STATE.getBlockGFX())
        return;

    // Je nach Status verschiedene Aktionen
    switch (m_drawState)
    {
    case IDLE:
        if (m_needsRedraw)
        {
            m_drawState = DRAWING;
            m_drawStep = 0;
            m_needsRedraw = false;
        }
        break;

    case DRAWING:
        if (drawIncremental())
        {
            m_drawState = READY_TO_SHOW;
        }
        break;

    case READY_TO_SHOW:
        display();
        m_drawState = IDLE;
        break;
    }
}

// ===================================================================
// Modul-Management
// ===================================================================

/**
 * @brief Ordnet einen Button-Index dem zugehörigen AppModule zu.
 */
AppModule MainScreen::buttonIndexToModule(int8_t buttonIndex) const
{
    switch (buttonIndex)
    {
    case BUTTON_INDEX_RADIO:
        return AppModule::RETRO_RADIO;
    case BUTTON_INDEX_SD_PLAYER:
        return AppModule::SD_PLAYER;
    case BUTTON_INDEX_DAYVIEW:
        return AppModule::DAYVIEW;
    case BUTTON_INDEX_CALENDAR:
        return AppModule::CALENDAR;
    case BUTTON_INDEX_TODO:
        return AppModule::TODO;
    case BUTTON_INDEX_CONTACTS:
        return AppModule::CONTACTS;
    case BUTTON_INDEX_WEATHER:
        return AppModule::WEATHER;
    case BUTTON_INDEX_POMODORO:
        return AppModule::POMODORO;
    case BUTTON_INDEX_BREATHER:
        return AppModule::BREATHER;
    case BUTTON_INDEX_SETTINGS:
        return AppModule::SETTINGS;
    case BUTTON_INDEX_WORKSHOP:
        return AppModule::WORKSHOP;
    case BUTTON_INDEX_OFF:
        return AppModule::OFF;
    default:
        return AppModule::NONE;
    }
}

/**
 * @brief Wechselt zu einem anderen Modul
 */
void MainScreen::switchToModule(AppModule module)
{
    // Altes Modul freigeben (unique_ptr erledigt delete automatisch)
    m_activeModule.reset();

    // Display schwarz füllen
    m_tft->clear(TFT_BLACK);

    LOG_INFO_F("Wechsle zu Modul: %d", static_cast<int>(module));

    // Statischer Zeiger auf MainScreen für Lambda-Callbacks. Muss außerhalb des switch-Blocks stehen, damit er für alle Module sichtbar ist.
    static MainScreen *mainScreenPtr = this;

    switch (module)
    {
    case AppModule::RETRO_RADIO:
    {
        RetroRadio *radio = new RetroRadio();
        RetroRadioCallbacks callbacks;

        // Callbacks für Start/Stopp durch das Modul
        callbacks.requestStopPlayback = stopCurrentStation;
        callbacks.requestPlayStation = setNewStation;

        // Exit-Callback
        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        // Station-Callbacks
        static RetroRadio *currentRadio = nullptr;
        currentRadio = radio;

        callbacks.onStationPressed = [](uint16_t staIdx)
        {
            if (currentRadio)
                currentRadio->setStation(staIdx);

            setNewStation(staIdx);
        };

        callbacks.onButtonPressed = [](uint16_t btnIdx, bool buttonState)
        {
            handleButtonPressed(btnIdx, buttonState);
            if (currentRadio && (btnIdx == 1 || btnIdx == 2))
                currentRadio->setStation(APP_STATE.getCurrentRetroStation());
        };

        // Restliche Callbacks
        callbacks.onVolumeChanged = handleVolumeChanged;
        callbacks.onVolumeBeforeMute = handleVolumeBeforeMute;
        callbacks.onMuteChanged = handleMuteChanged;
        callbacks.onBalanceChanged = handleBalanceChanged;
        callbacks.onGainLowPassChanged = handleGainLowPassChanged;
        callbacks.onGainBandPassChanged = handleGainBandPassChanged;
        callbacks.onGainHighPassChanged = handleGainHighPassChanged;

        if (radio->init(*m_tft, *m_rtc, *m_stationManager, callbacks))
        {
            m_activeModule.reset(radio);
            m_activeModuleType = AppModule::RETRO_RADIO;
            APP_STATE.setActiveModule(AppModule::RETRO_RADIO);
            radio->setStation(APP_STATE.getCurrentRetroStation());
            LOG_SUCCESS("RetroRadio gestartet");
        }
        else
        {
            delete radio;
            currentRadio = nullptr;
            LOG_ERROR("RetroRadio Start fehlgeschlagen");
        }

        break;
    }

    case AppModule::SD_PLAYER:
    {
        SdPlayer *sdPlayer = new SdPlayer(*m_tft, *m_audio);
        SdPlayerCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        callbacks.onVolumeChanged = handleVolumeChanged;
        callbacks.onBalanceChanged = handleBalanceChanged;
        callbacks.onGainLowPassChanged = handleGainLowPassChanged;
        callbacks.onGainBandPassChanged = handleGainBandPassChanged;
        callbacks.onGainHighPassChanged = handleGainHighPassChanged;
        callbacks.onPlayFileRequested = handlePlayFileRequested;
        callbacks.onStopRequested = handleStopRequested;
        callbacks.onPauseResumeRequested = handlePauseResumeRequested;
        callbacks.onSeekOffsetRequested = handleSeekOffsetRequested;

        if (sdPlayer->init(*m_rtc, callbacks))
        {
            m_activeModule.reset(sdPlayer);
            m_activeModuleType = AppModule::SD_PLAYER;
            APP_STATE.setActiveModule(AppModule::SD_PLAYER);

            // Globalen Zeiger setzen
            setActiveSdPlayer(sdPlayer);

            LOG_SUCCESS("SD-Player-Modul gestartet");
        }
        else
        {
            delete sdPlayer;
            LOG_ERROR("Start des SD-Player-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::WORKSHOP:
    {
        Workshop *workshop = new Workshop();
        WorkshopCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        if (workshop->init(*m_tft, callbacks))
        {
            m_activeModule.reset(workshop);
            m_activeModuleType = AppModule::WORKSHOP;
            APP_STATE.setActiveModule(AppModule::WORKSHOP);
            LOG_SUCCESS("Werkstatt-Modul gestartet");
        }
        else
        {
            delete workshop;
            LOG_ERROR("Start des Werkstatt-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::WEATHER:
    {
        Weather *weather = new Weather();
        WeatherCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        if (weather->init(*m_tft, callbacks))
        {
            m_activeModule.reset(weather);
            m_activeModuleType = AppModule::WEATHER;
            APP_STATE.setActiveModule(AppModule::WEATHER);
            LOG_SUCCESS("Wetter-Modul gestartet");
        }
        else
        {
            delete weather;
            LOG_ERROR("Start des Wetter-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::CALENDAR:
    {
        Calendar *calendar = new Calendar();
        CalendarCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        if (calendar->init(*m_tft, callbacks))
        {
            m_activeModule.reset(calendar);
            m_activeModuleType = AppModule::CALENDAR;
            APP_STATE.setActiveModule(AppModule::CALENDAR);
            LOG_SUCCESS("Kalender-Modul gestartet");
        }
        else
        {
            delete calendar;
            LOG_ERROR("Start des Kalender-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::TODO:
    {
        ToDo *todo = new ToDo();
        ToDoCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        if (todo->init(*m_tft, callbacks))
        {
            m_activeModule.reset(todo);
            m_activeModuleType = AppModule::TODO;
            APP_STATE.setActiveModule(AppModule::TODO);
            LOG_SUCCESS("ToDo-Modul gestartet");
        }
        else
        {
            delete todo;
            LOG_ERROR("Start des ToDo-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::CONTACTS:
    {
        Contacts *contacts = new Contacts();
        ContactsCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        if (contacts->init(*m_tft, callbacks))
        {
            m_activeModule.reset(contacts);
            m_activeModuleType = AppModule::CONTACTS;
            APP_STATE.setActiveModule(AppModule::CONTACTS);
            LOG_SUCCESS("Contacts-Modul gestartet");
        }
        else
        {
            delete contacts;
            LOG_ERROR("Start des Contacts-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::SETTINGS:
    {
        Settings *settings = new Settings();
        SettingsCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };
        callbacks.requestPlayStation = setNewStation;
        callbacks.requestStopPlayback = stopCurrentStation;
        callbacks.onVolumeChanged = handleVolumeChanged;

        if (settings->init(*m_tft, callbacks))
        {
            m_activeModule.reset(settings);
            m_activeModuleType = AppModule::SETTINGS;
            APP_STATE.setActiveModule(AppModule::SETTINGS);
            LOG_SUCCESS("Einstellungen-Modul gestartet");
        }
        else
        {
            delete settings;
            LOG_ERROR("Start des Einstellungen-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::OFF:
    {
        Off *off = new Off();
        OffCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        callbacks.onPowerOffRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestPowerOff();
        };

        if (off->init(*m_tft, callbacks))
        {
            m_activeModule.reset(off);
            m_activeModuleType = AppModule::OFF;
            APP_STATE.setActiveModule(AppModule::OFF);
            LOG_SUCCESS("Off-Modul gestartet");
        }
        else
        {
            delete off;
            LOG_ERROR("Start des Off-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::POMODORO:
    {
        Pomodoro *pomodoro = new Pomodoro();
        PomodoroCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        callbacks.onPlaySound = [](const char *path)
        {
            extern bool playFileFromSd(const char *path);
            playFileFromSd(path);
        };

        if (pomodoro->init(*m_tft, callbacks))
        {
            m_activeModule.reset(pomodoro);
            m_activeModuleType = AppModule::POMODORO;
            APP_STATE.setActiveModule(AppModule::POMODORO);
            LOG_SUCCESS("Pomodoro-Modul gestartet");
        }
        else
        {
            delete pomodoro;
            LOG_ERROR("Start des Pomodoro-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::BREATHER:
    {
        Breather *breather = new Breather();
        BreatherCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        callbacks.onPlaySound = [](const char *path)
        {
            extern bool playFileFromSd(const char *path);
            playFileFromSd(path);
        };

        if (breather->init(*m_tft, callbacks))
        {
            m_activeModule.reset(breather);
            m_activeModuleType = AppModule::BREATHER;
            APP_STATE.setActiveModule(AppModule::BREATHER);
            LOG_SUCCESS("Breather-Modul gestartet");
        }
        else
        {
            delete breather;
            LOG_ERROR("Start des Breather-Moduls fehlgeschlagen");
        }

        break;
    }

    case AppModule::DAYVIEW:
    {
        DayView *dayView = new DayView();
        DayViewCallbacks callbacks;

        callbacks.onExitRequested = []()
        {
            if (mainScreenPtr)
                mainScreenPtr->requestModuleClose();
        };

        // Radio-Callbacks
        callbacks.requestPlayStation = setNewStation;
        callbacks.requestStopPlayback = stopCurrentStation;
        callbacks.onVolumeChanged = handleVolumeChanged;
        callbacks.requestPlayFile = handlePlayFileRequested;
        callbacks.requestPauseResume = handlePauseResumeRequested;

        if (dayView->init(*m_tft, *m_stationManager, *m_audio, callbacks))
        {
            m_activeModule.reset(dayView);
            m_activeModuleType = AppModule::DAYVIEW;
            APP_STATE.setActiveModule(AppModule::DAYVIEW);
            LOG_SUCCESS("DayView-Modul gestartet");
        }
        else
        {
            delete dayView;
            LOG_ERROR("Start des DayView-Moduls fehlgeschlagen");
        }

        break;
    }

    default:
        LOG_ERROR_F("Unbekanntes Modul: %d", static_cast<int>(module));
        break;
    }
}

/**
 * @brief Schließt das aktive Modul und kehrt zum MainScreen zurück
 */
void MainScreen::closeActiveModule()
{
    if (m_activeModule)
    {
        LOG_INFO_F("Schließe Modul: %d", static_cast<int>(m_activeModuleType));

        // AppState über Modulwechsel informieren
        APP_STATE.setActiveModule(AppModule::NONE);

        // Bei SDPlayer globalen Zeiger zurücksetzen
        if (m_activeModuleType == AppModule::SD_PLAYER)
        {
            handleStopRequested();
            setActiveSdPlayer(nullptr);
        }

        // Modul freigeben
        m_activeModule.reset();
        m_activeModuleType = AppModule::NONE;
        APP_STATE.setActiveModule(AppModule::NONE);

        // Display schwarz füllen
        m_tft->clear(TFT_BLACK);

        // Textausrichtung zurücksetzen
        m_tft->setTextDatum(TL_DATUM);

        // MainScreen neu zeichnen
        requestRedraw(true);

        // Buttons neu zeichnen
        drawButtons();
    }
}

// ===================================================================
// Sperrbildschirm
// ===================================================================

/**
 * @brief Prüft auf Active-Corner-Geste in den linken Ecken
 */
bool MainScreen::checkActiveCorner()
{
    if (!m_frameTouched)
    {
        m_cornerTouchActive = false;
        m_cornerStartedInCorner = false;
        return false;
    }

    if (!m_cornerTouchActive)
    {
        const bool inUpperStartZone = (m_frameTouchX < 150) && (m_frameTouchY < 150);
        const bool inLowerStartZone = (m_frameTouchX < 150) && (m_frameTouchY > Layout::DISPLAY_HEIGHT - 150);
        const bool inUpperCorner = (m_frameTouchX < 32) && (m_frameTouchY < 32);
        const bool inLowerCorner = (m_frameTouchX < 32) && (m_frameTouchY > Layout::DISPLAY_HEIGHT - 32);

        if (inUpperStartZone)
        {
            m_cornerTouchActive = true;
            m_cornerStartX = m_frameTouchX;
            m_cornerStartY = m_frameTouchY;
            m_cornerIsUpperLeft = true;
            m_cornerStartedInCorner = inUpperCorner;
        }
        else if (inLowerStartZone)
        {
            m_cornerTouchActive = true;
            m_cornerStartX = m_frameTouchX;
            m_cornerStartY = m_frameTouchY;
            m_cornerIsUpperLeft = false;
            m_cornerStartedInCorner = inLowerCorner;
        }

        return false;
    }

    const int32_t dx = m_frameTouchX - m_cornerStartX;
    const int32_t dy = m_frameTouchY - m_cornerStartY;
    bool triggered = false;

    if (m_cornerIsUpperLeft)
    {
        const bool reachedCorner = (m_frameTouchX < 32) && (m_frameTouchY < 32);
        const bool inwardSwipe = reachedCorner && (dx < -50) && (dy < -50);
        const bool outwardSwipe = m_cornerStartedInCorner && (dx > 50) && (dy > 50);
        triggered = inwardSwipe || outwardSwipe;
    }
    else
    {
        const bool reachedCorner = (m_frameTouchX < 32) && (m_frameTouchY > Layout::DISPLAY_HEIGHT - 32);
        const bool inwardSwipe = reachedCorner && (dx < -50) && (dy > 50);
        const bool outwardSwipe = m_cornerStartedInCorner && (dx > 50) && (dy < -50);
        triggered = inwardSwipe || outwardSwipe;
    }

    if (!triggered)
        return false;

    m_cornerTouchActive = false;
    m_cornerStartedInCorner = false;
    triggerLockScreen();
    return true;
}

/**
 * @brief Aktiviert den Sperrbildschirm
 */
void MainScreen::triggerLockScreen()
{
    m_cornerTouchActive = false;
    m_cornerStartedInCorner = false;
    m_touchProcessed = false;
    m_lastPressedButton = -1;
    m_frameTouched = false;
    m_frameTouchX = 0;
    m_frameTouchY = 0;

    m_lockScreen.activate();
    m_lockScreenActive = true;
}

// ===================================================================
// Sprite-Hilfsfunktionen
// ===================================================================

/**
 * @brief Prüft ob Sprites für Zeichenoperationen bereit sind
 */
bool MainScreen::spritesReady() const
{
    return (m_tft != nullptr) && (m_sprMainScreen != nullptr);
}

/**
 * @brief Gibt den Sprite-Puffer des Hauptbildschirms zurück
 */
Gfx_Sprite *MainScreen::getBuffer()
{
    if (!spritesReady())
        return nullptr;

    return m_sprMainScreen;
}

/**
 * @brief Zeigt den Puffer auf dem Display an
 */
void MainScreen::display()
{
    if (!spritesReady())
        return;

    m_sprMainScreen->pushSprite(Layout::SPRITE_X, Layout::SPRITE_Y);
}

// ===================================================================
// Zeichenfunktionen
// ===================================================================

/**
 * @brief Zeichnet den Hauptbildschirm sofort komplett (blockierend)
 */
void MainScreen::drawImmediate()
{
    Gfx_Sprite *puffer = getBuffer();
    if (!puffer)
        return;

    drawBackground(puffer);
    drawAnalogClock(puffer, Layout::CLOCK_CENTER_X, Layout::CLOCK_CENTER_Y, Layout::CLOCK_RADIUS);

    display();
}

/**
 * @brief Zeichnet den Hauptbildschirm inkrementell (nicht blockierend)
 */
bool MainScreen::drawIncremental()
{
    if (!m_initialized)
        return true;

    Gfx_Sprite *puffer = getBuffer();
    if (!puffer)
        return true;

    switch (m_drawStep)
    {
    case 0:
        drawBackground(puffer);
        m_drawStep++;
        return false;

    case 1:
        drawAnalogClock(puffer, Layout::CLOCK_CENTER_X, Layout::CLOCK_CENTER_Y, Layout::CLOCK_RADIUS);
        m_drawStep++;
        return false;

    case 2:
        m_drawStep = 0;
        return true;

    default:
        m_drawStep = 0;
        return true;
    }
}

/**
 * @brief Fordert ein Neuzeichnen an
 */
void MainScreen::requestRedraw(bool immediate)
{
    if (!m_initialized)
        return;

    if (immediate)
    {
        drawImmediate();
        m_drawState = IDLE;
        m_drawStep = 0;
        m_needsRedraw = false;
    }
    else
    {
        m_needsRedraw = true;
    }
}

/**
 * @brief Zeichnet den Hintergrund
 */
void MainScreen::drawBackground(Gfx_Sprite *buffer)
{
    if (!buffer)
        return;

    buffer->clear(TFT_BLACK);
}

/**
 * @brief Zeichnet eine analoge Uhr mit integrierten Status-Indikatoren und Datum.
 *
 * Status-Indikatoren innerhalb der Uhr:
 * - 11-Uhr-Marker: NTP-Status
 * - 1-Uhr-Marker: DST-Status
 * - Zwischen Zentrum und 6 Uhr: WiFi-Signalbalken, darunter SSID + IP
 * - Rotierte Labels "NTP" und "DST" neben den jeweiligen Markern
 *
 * Bei ungültiger Systemzeit (kein NTP-Sync, keine gepufferte RTC) werden die Zeiger weggelassen, damit der Anwender sofort sieht, dass aktuell keine Uhrzeit verfügbar ist.
 */
void MainScreen::drawAnalogClock(Gfx_Sprite *buffer, int centerX, int centerY, float radius)
{
    if (!buffer || !APP_STATE.isDisplayOn() || APP_STATE.getBlockGFX())
        return;

    // Kurzreferenz auf aktives Theme
    const auto &T = Theme::ActiveMainScreen;

    // Geometrie-Konstanten
    const float HOUR_HAND_RATIO = 0.55f;
    const float MINUTE_HAND_RATIO = 0.75f;
    const float SECOND_HAND_RATIO = 0.95f;
    const float HOUR_MARK_LENGTH = radius * 0.15f;
    const float MINOR_MARK_LENGTH = radius * 0.08f;
    const float HAND_WIDTH = radius * 0.025f;
    const float CENTER_CIRCLE_RADIUS = radius * 0.08f;
    const int DEGREES_PER_HOUR = 30;
    const int DEGREES_PER_MINUTE = 6;

    // Status-Werte lesen
    bool ntpSynced = APP_STATE.getNtpSynced();
    bool dstActive = APP_STATE.getDstActive();
    bool wifiConnected = APP_STATE.getWifiConnected();
    uint8_t wifiPercent = APP_STATE.getWifiSignalPercent();

    // Lambda für Koordinatenberechnung
    auto getCoord = [](float cx, float cy, float angleDeg, float length) -> std::pair<float, float>
    {
        const float rad = (angleDeg - 90.0f) * 0.0174532925f; // DEG2RAD
        return {cx + cosf(rad) * length, cy + sinf(rad) * length};
    };

    buffer->setTextDatum(TL_DATUM);

    // Minuten-Markierungen
    for (int minute = 0; minute < 60; minute++)
    {
        if (minute % 5 != 0)
        {
            const float angleDeg = minute * DEGREES_PER_MINUTE;
            auto outer = getCoord(centerX, centerY, angleDeg, radius - 10);
            auto inner = getCoord(centerX, centerY, angleDeg, radius - 10 - MINOR_MARK_LENGTH);
            buffer->drawWideLine(roundf(inner.first), roundf(inner.second), roundf(outer.first), roundf(outer.second), 1.0f, T.clockMinorMarks);
        }
    }

    // Stunden-Markierungen (mit Status-Farben für 11 Uhr und 1 Uhr)
    for (int hour = 0; hour < 12; hour++)
    {
        const float angleDeg = hour * DEGREES_PER_HOUR;
        auto outer = getCoord(centerX, centerY, angleDeg, radius - 10);
        auto inner = getCoord(centerX, centerY, angleDeg, radius - 10 - HOUR_MARK_LENGTH);

        // Standardfarbe und -dicke
        float lineWidth = (hour % 3 == 0) ? 3.0f : 2.0f;
        uint16_t markColor = (hour % 3 == 0) ? T.clockMarks : T.clockMinorMarks;

        // Status-Marker für 11 Uhr (NTP) und 1 Uhr (DST)
        if (hour == 11)
        {
            markColor = ntpSynced ? T.statusNtpActive : T.statusOff;
            lineWidth = 3.0f;
        }
        else if (hour == 1)
        {
            markColor = dstActive ? T.statusDstActive : T.statusOff;
            lineWidth = 3.0f;
        }

        buffer->drawWideLine(roundf(inner.first), roundf(inner.second), roundf(outer.first), roundf(outer.second), lineWidth, markColor);
    }

    // Rotierte NTP/DST-Labels
    if (!m_labelCachesValid)
        renderLabelCaches();

    if (m_sprLabelNtp)
    {
        // NTP-Label bei 11 Uhr (330°), radial innerhalb der Markierung
        constexpr float NTP_ANGLE = 330.0f;
        auto ntpPos = getCoord(centerX, centerY, NTP_ANGLE, radius * Layout::LABEL_RADIAL_POS);
        m_sprLabelNtp->pushRotateZoomWithAA(buffer, roundf(ntpPos.first), roundf(ntpPos.second), NTP_ANGLE + 90.0f, 1.0f, 1.0f, TFT_BLACK);
    }

    if (m_sprLabelDst)
    {
        // DST-Label bei 1 Uhr (30°), radial innerhalb der Markierung
        constexpr float DST_ANGLE = 30.0f;
        auto dstPos = getCoord(centerX, centerY, DST_ANGLE, radius * Layout::LABEL_RADIAL_POS);
        m_sprLabelDst->pushRotateZoomWithAA(buffer, roundf(dstPos.first), roundf(dstPos.second), DST_ANGLE + 90.0f, 1.0f, 1.0f, TFT_BLACK);
    }

    // WiFi-Signalbalken
    {
        constexpr int barCount = Layout::SIGNAL_BAR_COUNT;
        constexpr int barW = Layout::SIGNAL_BAR_WIDTH;
        constexpr int gap = Layout::SIGNAL_BAR_GAP;
        constexpr int minH = Layout::SIGNAL_BAR_MIN_H;
        constexpr int stepH = Layout::SIGNAL_BAR_STEP_H;
        constexpr int maxH = minH + (barCount - 1) * stepH;

        // Gesamtbreite aller Balken
        constexpr int totalBarW = barCount * barW + (barCount - 1) * gap;

        // Position: zwischen Zentrum und 6-Uhr-Marker
        int barCenterY = centerY + int(radius * Layout::SIGNAL_BAR_RADIAL_POS);
        int x0 = centerX - totalBarW / 2;
        int yBottom = barCenterY + maxH / 2;

        // Aktive Balken aus Signalstärke berechnen
        int activeBars = 0;
        if (wifiConnected && wifiPercent > 0)
            activeBars = (wifiPercent - 1) * barCount / 100 + 1;

        for (int i = 0; i < barCount; ++i)
        {
            int barH = minH + i * stepH;
            int bx = x0 + i * (barW + gap);
            int by = yBottom - barH;

            uint16_t color;
            if (!wifiConnected)
                color = T.statusOff;
            else
                color = (i < activeBars) ? T.statusWifiActive : T.statusOff;

            buffer->fillRect(bx, by, barW, barH, color);
        }

        // SSID + IP unter den Balken anzeigen
        const char *ssid = SYSTEM_STATS.getWifiSSID();
        if (wifiConnected && ssid[0] != '\0')
        {
            buffer->loadFont(ComfortaaBold10);
            buffer->setTextDatum(TC_DATUM);
            buffer->setTextColor(T.ssidText);

            // Verfügbare Breite: Uhrenradius begrenzt den Platz
            constexpr int maxTextW = 120;
            int ssidY = yBottom + Layout::SSID_OFFSET_Y;

            // SSID zeichnen und bei Bedarf mit "..." kürzen
            if (buffer->textWidth(ssid) <= maxTextW)
            {
                buffer->drawString(ssid, centerX, ssidY);
            }
            else
            {
                char truncated[34];
                size_t len = strlen(ssid);
                // Zeichen von hinten kürzen bis es passt
                for (size_t n = len; n > 0; --n)
                {
                    stringutils::safeFormat(truncated, "%.*s\xE2\x80\xA6", (int)n, ssid); // UTF-8-Ellipse

                    if (buffer->textWidth(truncated) <= maxTextW)
                        break;
                }

                buffer->drawString(truncated, centerX, ssidY);
            }

            // IP-Adresse direkt unter SSID (gleiche Font, gleiches Datum, eigene Farbe)
            IPAddress ip = WiFi.localIP();
            if (static_cast<uint32_t>(ip) != 0)
            {
                char ipBuf[16]; // "255.255.255.255\0" = 16 Bytes
                stringutils::safeFormat(ipBuf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
                int ipY = ssidY + Layout::IP_OFFSET_Y;
                buffer->loadFont(ComfortaaBold14);
                buffer->setTextColor(T.ipText);
                buffer->drawString(ipBuf, centerX, ipY);
            }

            buffer->unloadFont();
        }
    }

    // Zeiger nur zeichnen wenn die Systemzeit plausibel ist. Ohne NTP-Sync und ohne gepufferte RTC bleibt das Zifferblatt leer - so sieht der Anwender sofort, dass keine aktuelle Uhrzeit verfügbar ist.
    if (DS1307_RTC::isSystemTimeValid())
    {
        time_t nowEpoch = time(nullptr);
        struct tm localTime;
        localtime_r(&nowEpoch, &localTime);

        const float hourLength = radius * HOUR_HAND_RATIO;
        const float minuteLength = radius * MINUTE_HAND_RATIO;
        const float secondLength = radius * SECOND_HAND_RATIO;

        const float hourAngle = (localTime.tm_hour % 12 + localTime.tm_min / 60.0f) * DEGREES_PER_HOUR;
        const float minuteAngle = (localTime.tm_min + localTime.tm_sec / 60.0f) * DEGREES_PER_MINUTE;
        const float secondAngle = localTime.tm_sec * DEGREES_PER_MINUTE;

        // Stundenzeiger
        auto hourPos = getCoord(centerX, centerY, hourAngle, hourLength);
        buffer->drawWideLine(centerX, centerY, hourPos.first, hourPos.second, HAND_WIDTH, T.clockHands);

        // Minutenzeiger
        auto minPos = getCoord(centerX, centerY, minuteAngle, minuteLength);
        buffer->drawWideLine(centerX, centerY, minPos.first, minPos.second, HAND_WIDTH, T.clockHands);

        // Mittelpunkt
        buffer->fillSmoothCircle(centerX, centerY, CENTER_CIRCLE_RADIUS, T.clockCenter);

        // Sekundenzeiger
        const float counterWeightLength = radius * 0.2f;
        const float secHandWidth = radius * 0.015f;

        auto secTip = getCoord(centerX, centerY, secondAngle, secondLength);
        auto counterWeightTip = getCoord(centerX, centerY, secondAngle + 180, counterWeightLength);

        buffer->drawWedgeLine(roundf(counterWeightTip.first), roundf(counterWeightTip.second), roundf(secTip.first), roundf(secTip.second), secHandWidth, 0.5f, T.clockSecondHand);

        // Innerer Akzent im Mittelpunkt
        buffer->fillSmoothCircle(centerX, centerY, CENTER_CIRCLE_RADIUS * 0.8f, T.clockCenterAccent);
    }

    // Datum unter der Uhr
    if (!m_dateCacheValid)
        renderDateCache();

    if (m_sprDateCache)
    {
        int dateX = centerX - Layout::DATE_CACHE_W / 2;
        int dateY = centerY + int(radius) + Layout::CLOCK_DATE_OFFSET;
        m_sprDateCache->pushSprite(buffer, dateX, dateY, TFT_BLACK);
    }
}

/**
 * @brief Rendert die NTP/DST-Label-Cache-Sprites (einmalig).
 */
void MainScreen::renderLabelCaches()
{
    if (!m_tft)
        return;

    constexpr int w = Layout::LABEL_CACHE_W;
    constexpr int h = Layout::LABEL_CACHE_H;

    // NTP-Label
    if (!m_sprLabelNtp)
    {
        if (!m_tft->safeCreateSprite(m_sprLabelNtp, w, h))
        {
            LOG_ERROR("NTP-Label-Sprite konnte nicht erstellt werden!");
            return;
        }
    }

    // DST-Label
    if (!m_sprLabelDst)
    {
        if (!m_tft->safeCreateSprite(m_sprLabelDst, w, h))
        {
            LOG_ERROR("DST-Label-Sprite konnte nicht erstellt werden!");
            return;
        }
    }

    // Label-Farbe aus aktivem Theme
    const uint16_t labelColor = Theme::ActiveMainScreen.statusLabels;

    // NTP rendern
    m_sprLabelNtp->clear(TFT_BLACK);
    m_sprLabelNtp->loadFont(ComfortaaBold14);
    m_sprLabelNtp->setTextDatum(MC_DATUM);
    m_sprLabelNtp->setTextColor(labelColor, TFT_BLACK);
    m_sprLabelNtp->drawString("NTP", w / 2, h / 2);
    m_sprLabelNtp->unloadFont();

    // DST rendern
    m_sprLabelDst->clear(TFT_BLACK);
    m_sprLabelDst->loadFont(ComfortaaBold14);
    m_sprLabelDst->setTextDatum(MC_DATUM);
    m_sprLabelDst->setTextColor(labelColor, TFT_BLACK);
    m_sprLabelDst->drawString("DST", w / 2, h / 2);
    m_sprLabelDst->unloadFont();

    m_labelCachesValid = true;
    LOG_INFO("NTP/DST-Label-Caches erstellt.");
}

/**
 * @brief Rendert das Datum-Cache-Sprite neu.
 *
 * Quelle ist die Systemzeit. Bei ungültiger Zeit wird ein Platzhalter gezeichnet, damit der Anwender sieht, dass gerade kein Datum verfügbar ist.
 */
void MainScreen::renderDateCache()
{
    if (!m_tft)
        return;

    // Sprite bei Bedarf erstellen
    if (!m_sprDateCache)
    {
        if (!m_tft->safeCreateSprite(m_sprDateCache, Layout::DATE_CACHE_W, Layout::DATE_CACHE_H))
        {
            LOG_ERROR("Datum-Cache-Sprite konnte nicht erstellt werden!");
            return;
        }
    }

    // Datum-String aus Systemzeit bilden oder Platzhalter setzen
    char buf[64];
    if (DS1307_RTC::isSystemTimeValid())
    {
        time_t nowEpoch = time(nullptr);
        struct tm nowLocal;
        localtime_r(&nowEpoch, &nowLocal);

        char wdBuf[16];
        char monBuf[16];
        I18n::copyWeekday(wdBuf, sizeof(wdBuf), nowLocal.tm_wday, /*longForm=*/false);
        I18n::copyMonth(monBuf, sizeof(monBuf), nowLocal.tm_mon, /*longForm=*/true);
        stringutils::safeFormat(buf, "%s., %d. %s %d",
                                wdBuf,
                                nowLocal.tm_mday,
                                monBuf,
                                nowLocal.tm_year + 1900);
    }
    else
    {
        stringutils::safeCopy(buf, DATE_PLACEHOLDER);
    }

    // In Cache-Sprite rendern
    const uint16_t dateColor = Theme::ActiveMainScreen.dateText;

    m_sprDateCache->clear(TFT_BLACK);
    m_sprDateCache->loadFont(ComfortaaBold30);
    m_sprDateCache->setTextDatum(BC_DATUM);
    m_sprDateCache->setTextColor(dateColor, TFT_BLACK);
    m_sprDateCache->drawString(buf, Layout::DATE_CACHE_W / 2, Layout::DATE_CACHE_H);
    m_sprDateCache->unloadFont();

    m_dateCacheValid = true;

    LOG_INFO_F("Datum-Cache aktualisiert: %s", buf);
}

// ===================================================================
// Button-Management
// ===================================================================

/**
 * @brief Initialisiert die 12 Hauptmenü-Buttons (4 Zeilen x 3 Spalten)
 */
void MainScreen::initButtons()
{
    // Kurzreferenz auf aktives Theme
    const auto &T = Theme::ActiveMainScreen;

    // Symbole einmalig in der Theme-Farbe zeichnen (PSRAM). Der Knopf schiebt sie ohne Transparenz auf, deshalb gehört die schwarze Fläche zum Bild.
    constexpr uint16_t ICON_BACKGROUND = 0x0000;

    const IconId iconIds[ICON_COUNT] = {IconId::Radio, IconId::Player, IconId::Calendar, IconId::ToDo,
                                        IconId::Contacts, IconId::Weather, IconId::Config, IconId::Workshop,
                                        IconId::Off, IconId::Breather, IconId::Pomodoro, IconId::DayView};

    for (int i = 0; i < ICON_COUNT; ++i)
    {
        m_tintedIcons[i] = IconShapes::draw(iconIds[i], T.iconTints[i], ICON_BACKGROUND);
        if (!m_tintedIcons[i])
            LOG_ERROR_F("Symbol %d konnte nicht gezeichnet werden", i);
    }


    // Tinted-Icon-Referenzen mit Fallback auf das Original
    const uint16_t *ic_radio = m_tintedIcons[0];
    const uint16_t *ic_player = m_tintedIcons[1];
    const uint16_t *ic_calendar = m_tintedIcons[2];
    const uint16_t *ic_todo = m_tintedIcons[3];
    const uint16_t *ic_contacts = m_tintedIcons[4];
    const uint16_t *ic_wheater = m_tintedIcons[5];
    const uint16_t *ic_config = m_tintedIcons[6];
    const uint16_t *ic_workshop = m_tintedIcons[7];
    const uint16_t *ic_off = m_tintedIcons[8];
    const uint16_t *ic_breather = m_tintedIcons[9];
    const uint16_t *ic_pomodoro = m_tintedIcons[10];
    const uint16_t *ic_dayview = m_tintedIcons[11];

    // Button-Definitionen für das 4x3 Grid
    struct ButtonDef
    {
        const char *label;
        uint16_t outlineColor;
        uint16_t fillColor;
        const uint16_t *icon;
    };

    const ButtonDef buttonDefs[BUTTON_COUNT] = {
        // Zeile 0
        {"", T.buttonBorder, T.buttonFill, ic_radio},   // [0] Radio
        {"", T.buttonBorder, T.buttonFill, ic_player},  // [1] SD-Player
        {"", T.buttonBorder, T.buttonFill, ic_dayview}, // [2] DayView

        // Zeile 1
        {"", T.buttonBorder, T.buttonFill, ic_calendar}, // [3] Kalender
        {"", T.buttonBorder, T.buttonFill, ic_todo},     // [4] ToDo
        {"", T.buttonBorder, T.buttonFill, ic_contacts}, // [5] Kontakte

        // Zeile 2
        {"", T.buttonBorder, T.buttonFill, ic_wheater},  // [6] Wetter
        {"", T.buttonBorder, T.buttonFill, ic_pomodoro}, // [7] Pomodoro
        {"", T.buttonBorder, T.buttonFill, ic_breather}, // [8] Breather

        // Zeile 3
        {"", T.buttonBorder, T.buttonFill, ic_config},   // [9] Einstellungen
        {"", T.buttonBorder, T.buttonFill, ic_workshop}, // [10] Werkstatt
        {"", T.buttonBorder, T.buttonFill, ic_off}       // [11] Ausschalten
    };

    // Buttons erstellen und positionieren
    for (int i = 0; i < BUTTON_COUNT; ++i)
    {
        std::unique_ptr<Button> btn(new Button());

        // Position berechnen (3 Spalten, 4 Zeilen)
        int col = i % Layout::BUTTON_COLS;
        int row = i / Layout::BUTTON_COLS;

        int x = Layout::BUTTON_START_X + col * (Layout::BUTTON_SIZE + Layout::BUTTON_SPACING) + Layout::BUTTON_SIZE / 2;
        int y = Layout::BUTTON_START_Y + row * (Layout::BUTTON_SIZE + Layout::BUTTON_SPACING) + Layout::BUTTON_SIZE / 2;

        btn->initButton(
            m_tft,                      // Display-Objekt
            x,                          // X-Position
            y,                          // Y-Position
            Layout::BUTTON_SIZE,        // Breite
            Layout::BUTTON_SIZE,        // Höhe
            10,                         // Eckenradius
            buttonDefs[i].outlineColor, // Umrandungsfarbe
            buttonDefs[i].fillColor,    // Füllfarbe
            uint16_t(0xFFFF),           // Textfarbe
            buttonDefs[i].label,        // Beschriftung
            buttonDefs[i].icon,         // Icon
            1.0f,                       // Icon-Scale (1.0 = Originalgröße)
            1.0f                        // Text-Scale (1.0 = Originalgröße
        );

        if (i == 3)
            btn->setFont(ComfortaaBold30);

        btn->setTextColor(T.buttonText);

        // Icon-Größe
        btn->setIconSize(80, 80);
        btn->setIconPosition(Button::IconPosition::Center);

        m_buttons[i] = std::move(btn);
    }

    // Initiales Datum für den Kalender-Button (Index 3) aus der Systemzeit setzen. Bei ungültiger Zeit zeigt der Button den Platzhalter.
    if (m_buttons[BUTTON_INDEX_CALENDAR])
    {
        char dayStr[4];
        if (DS1307_RTC::isSystemTimeValid())
        {
            time_t nowEpoch = time(nullptr);
            struct tm nowLocal;
            localtime_r(&nowEpoch, &nowLocal);
            stringutils::safeFormat(dayStr, "%d", nowLocal.tm_mday);
        }
        else
        {
            stringutils::safeCopy(dayStr, DAY_PLACEHOLDER);
        }

        m_buttons[BUTTON_INDEX_CALENDAR]->setLabelText(dayStr);
        m_buttons[BUTTON_INDEX_CALENDAR]->setLabelOffset(0, Layout::CALENDAR_LABEL_OFFSET_Y);
    }
}

/**
 * @brief Zeichnet die Buttons
 */
void MainScreen::drawButtons()
{
    for (int i = 0; i < BUTTON_COUNT; ++i)
        m_buttons[i]->drawButton(false);
}

/**
 * @brief Prüft welcher Button gedrückt wurde
 */
int8_t MainScreen::getPressedButton()
{
    if (m_frameTouched)
    {
        if (!m_touchProcessed)
        {
            for (int i = 0; i < BUTTON_COUNT; ++i)
            {
                if (i == BUTTON_INDEX_OFF)
                    continue; // Off-Button läuft über handleOffButton() (langer Druck)

                if (m_buttons[i] && m_buttons[i]->contains(m_frameTouchX, m_frameTouchY) && m_buttons[i]->isEnabled())
                {
                    m_touchProcessed = true;
                    m_lastPressedButton = i;

                    // Visuelles Feedback
                    m_buttons[i]->drawButton(true);

                    return i;
                }
            }
        }
    }
    else
    {
        // Touch losgelassen
        if (m_touchProcessed)
        {
            if (m_lastPressedButton >= 0 && m_lastPressedButton < BUTTON_COUNT)
            {
                if (m_buttons[m_lastPressedButton])
                    m_buttons[m_lastPressedButton]->drawButton(false);
            }

            m_touchProcessed = false;
            m_lastPressedButton = -1;
        }
    }

    return -1;
}

/**
 * @brief Aktualisiert den aktiv/deaktiviert Status der Buttons
 */
void MainScreen::updateButtonStates(bool redraw)
{
    bool wifiReady = APP_STATE.getWifiConnected();

    // Radio-Button (Index 0)
    if (m_buttons[BUTTON_INDEX_RADIO])
        m_buttons[BUTTON_INDEX_RADIO]->setEnabled(wifiReady, redraw);

    // DayView-Button (Index 2)
    if (m_buttons[BUTTON_INDEX_DAYVIEW])
        m_buttons[BUTTON_INDEX_DAYVIEW]->setEnabled(wifiReady, redraw);

    // Kalender-Button (Index 3)
    if (m_buttons[BUTTON_INDEX_CALENDAR])
        m_buttons[BUTTON_INDEX_CALENDAR]->setEnabled(wifiReady, redraw);

    // ToDo-Button (Index 4)
    if (m_buttons[BUTTON_INDEX_TODO])
        m_buttons[BUTTON_INDEX_TODO]->setEnabled(wifiReady, redraw);

    // Kontakte-Button (Index 5)
    if (m_buttons[BUTTON_INDEX_CONTACTS])
        m_buttons[BUTTON_INDEX_CONTACTS]->setEnabled(wifiReady, redraw);

    // Wetter-Button (Index 6)
    if (m_buttons[BUTTON_INDEX_WEATHER])
        m_buttons[BUTTON_INDEX_WEATHER]->setEnabled(wifiReady, redraw);
}
