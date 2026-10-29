#include "Pomodoro.h"

#include <Preferences.h>

#include <cmath>
#include <cstring>
#include <time.h>
#include <utility>

#include "AppState.h"
#include "Clocks.h"
#include "I18n.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Strings_pomodoro.h"
#include "Theme.h"

// Fonts
#include "fonts/ComfortaaBold16.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// KONSTANTEN
// ===================================================================

namespace
{
    // Timer-Voreinstellungen
    constexpr int TIMER_PRESETS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 25, 30, 45, 60};
    constexpr int TIMER_PRESET_COUNT = sizeof(TIMER_PRESETS) / sizeof(TIMER_PRESETS[0]);
    constexpr int DEFAULT_TIMER_PRESET_INDEX = 12; ///< 25 Minuten
    constexpr int SECONDS_PER_MINUTE = 60;         ///< Sekunden pro Minute
    constexpr int HOURS_ON_DIAL = 12;              ///< Stundenmarkierungen auf dem Zifferblatt
    constexpr int MINUTES_ON_DIAL = 60;            ///< Minutenmarkierungen auf dem Zifferblatt

    // Sound-Pfad für das Glocken-Signal am Ende einer Phase
    constexpr const char *BELL_SOUND_PATH = "/system/sounds/pomodoro.mp3";

    // Preferences
    constexpr const char *PREF_NAMESPACE = "Pomodoro";
    constexpr const char *PREF_KEY_COUNT = "count";
    constexpr const char *PREF_KEY_DATE = "date";
    constexpr const char *PREF_KEY_TIMER_IDX = "timerIdx";

    // ===================================================================
    // Layout
    // ===================================================================

    constexpr int DISPLAY_W = 800; ///< Display-Breite
    constexpr int DISPLAY_H = 480; ///< Display-Höhe

    // Zifferblatt-Geometrie
    constexpr float DIAL_RADIUS = 195.0f;                                                      ///< Radius des Zifferblatts (für Stunden- und Minutenmarkierungen)
    constexpr float ARC_THICKNESS = 14.0f;                                                     ///< Dicke der Fortschrittsanzeige (Arc) um das Zifferblatt
    constexpr float ARC_RADIUS = DIAL_RADIUS + ARC_THICKNESS + 6.0f;                           ///< Radius der Fortschrittsanzeige
    constexpr int DIAL_CENTER_X = 235;                                                         ///< X-Koordinate des Zifferblatt-Zentrums
    constexpr int DIAL_CENTER_Y = 240;                                                         ///< Y-Koordinate des Zifferblatt-Zentrums
    constexpr int GAUGE_SPRITE_SIZE = static_cast<int>(2 * (ARC_RADIUS + ARC_THICKNESS)) + 20; ///< Größe des Sprites für die Fortschrittsanzeige
    constexpr int GAUGE_SPRITE_X = DIAL_CENTER_X - GAUGE_SPRITE_SIZE / 2;                      ///< X-Koordinate des Gauge-Sprites (zentriert um das Zifferblatt)
    constexpr int GAUGE_SPRITE_Y = DIAL_CENTER_Y - GAUGE_SPRITE_SIZE / 2;                      ///< Y-Koordinate des Gauge-Sprites (zentriert um das Zifferblatt)
    constexpr float HOUR_MARK_LENGTH = DIAL_RADIUS * 0.15f;                                    ///< Länge der Stundenmarkierungen
    constexpr float MINOR_MARK_LENGTH = DIAL_RADIUS * 0.08f;                                   ///< Länge der Minutenmarkierungen
    constexpr int DEGREES_PER_HOUR = 30;                                                       ///< Grad pro Stunde auf dem Zifferblatt (360° / 12 Stunden)
    constexpr int DEGREES_PER_MINUTE = 6;                                                      ///< Grad pro Minute auf dem Zifferblatt (360° / 60 Minuten)
    constexpr float MARK_INSET = 10.0f;                                                        ///< Abstand der Markierungen vom äußeren Rand des Zifferblatts (für bessere Lesbarkeit)

    // DotMatrix Konfiguration
    constexpr int DOT_WIDTH = 6;      ///< Breite eines Punktes
    constexpr int DOT_HEIGHT = 6;     ///< Höhe eines Punktes
    constexpr int DOT_SPACE = 2;      ///< Abstand zwischen Punkten
    constexpr int DOT_CHAR_SPACE = 5; ///< Abstand zwischen Zeichen

    // Rechtes Panel
    constexpr int PANEL_X = 490;        ///< X-Koordinate des rechten Panels (linke Wisch-Kante)
    constexpr int PANEL_CENTER_X = 630; ///< X-Koordinate für zentrierte Elemente im rechten Panel

    // Exit-Button
    constexpr int16_t EXIT_BTN_X = 765;     ///< X-Koordinate des Exit-Buttons
    constexpr int16_t EXIT_BTN_Y = 457;     ///< Y-Koordinate des Exit-Buttons
    constexpr uint16_t EXIT_BTN_SIZE = 45;  ///< Größe des quadratischen Exit-Buttons
    constexpr uint16_t EXIT_BTN_RADIUS = 4; ///< Eckenradius des Exit-Buttons

    // Start/Pause-Button
    constexpr int16_t START_BTN_X = PANEL_CENTER_X; ///< X-Koordinate des Start/Pause-Buttons
    constexpr int16_t START_BTN_Y = 230;            ///< Y-Koordinate des Start/Pause-Buttons
    constexpr uint16_t START_BTN_W = 260;           ///< Breite des Start/Pause-Buttons
    constexpr uint16_t START_BTN_H = 56;            ///< Höhe des Start/Pause-Buttons
    constexpr uint16_t START_BTN_RADIUS = 4;        ///< Eckenradius des Start/Pause-Buttons

    // Header-Geometrie (Modul-Titel, Trennlinie, Panel-Labels, Checkboxen) an der linken und rechten Außenkante der großen Buttons ausgerichtet
    constexpr int16_t HEADER_LEFT_X = PANEL_CENTER_X - START_BTN_W / 2; ///< Linke Kante des Header-Bereichs (= linke Button-Kante)
    constexpr uint16_t HEADER_W = START_BTN_W;                          ///< Breite des Header-Bereichs (= Button-Breite)

    // Stop-Button
    constexpr int16_t STOP_BTN_X = PANEL_CENTER_X; ///< X-Koordinate des Stop-Buttons
    constexpr int16_t STOP_BTN_Y = 305;            ///< Y-Koordinate des Stop-Buttons
    constexpr uint16_t STOP_BTN_W = 260;           ///< Breite des Stop-Buttons
    constexpr uint16_t STOP_BTN_H = 56;            ///< Höhe des Stop-Buttons
    constexpr uint16_t STOP_BTN_RADIUS = 4;        ///< Eckenradius des Stop-Buttons

    // Modus-Button
    constexpr int16_t MODE_BTN_X = PANEL_CENTER_X; ///< X-Koordinate des Modus-Buttons
    constexpr int16_t MODE_BTN_Y = 380;            ///< Y-Koordinate des Modus-Buttons
    constexpr uint16_t MODE_BTN_W = 260;           ///< Breite des Modus-Buttons
    constexpr uint16_t MODE_BTN_H = 46;            ///< Höhe des Modus-Buttons
    constexpr uint16_t MODE_BTN_RADIUS = 4;        ///< Eckenradius des Modus-Buttons

    // +/- Buttons im Timer-Modus
    constexpr int16_t DUR_UP_X = 720;      ///< X-Koordinate des "+"-Buttons
    constexpr int16_t DUR_DOWN_X = 540;    ///< X-Koordinate des "-"-Buttons
    constexpr int16_t DUR_BTN_Y = 140;     ///< Y-Koordinate der +/- Buttons
    constexpr uint16_t DUR_BTN_SIZE = 44;  ///< Größe der quadratischen +/- Buttons
    constexpr uint16_t DUR_BTN_RADIUS = 4; ///< Eckenradius der +/- Buttons

    // Session-Checkboxen
    constexpr int16_t SESSION_LABEL_Y = 74;  ///< Y-Position des "Session"-Labels
    constexpr int16_t SESSION_CB_Y = 110;    ///< Y-Position der Checkboxen
    constexpr int SESSION_CB_SIZE = 37;      ///< Checkbox-Größe
    constexpr int SESSION_CB_SPACING = 74;   ///< Abstand zwischen Checkboxen (Linke Kante zu linke Kante)

    // Tages-Statistik
    constexpr int16_t STATS_CENTER_X = PANEL_CENTER_X; ///< X-Koordinate für zentrierte Elemente in der Statistik
    constexpr int16_t STATS_Y = 162;                   ///< Y-Koordinate für die Statistik

    // Digitaluhr unten rechts
    constexpr int16_t CLOCK_X = 625;                            ///< X-Koordinate der Digitaluhr
    constexpr int16_t CLOCK_SPRITE_W = 103;                     ///< Breite des Sprites für die Digitaluhr
    constexpr int16_t CLOCK_SPRITE_H = 45;                      ///< Höhe des Sprites für die Digitaluhr
    constexpr int16_t CLOCK_Y = DISPLAY_H - CLOCK_SPRITE_H + 1; ///< Y-Koordinate der Digitaluhr

    // ===================================================================
    // Farben
    // ===================================================================

    constexpr uint16_t COLOR_FOCUS = 0xDCC4;       ///< Hauptfarbe für aktive Elemente
    constexpr uint16_t COLOR_SHORT_BREAK = 0x5C44; ///< Farbe für kurze Pausen
    constexpr uint16_t COLOR_LONG_BREAK = 0x5DDA;  ///< Farbe für lange Pausen
    constexpr uint16_t COLOR_TIMER = 0x7D7A;       ///< Farbe für den Timer-Modus

    constexpr uint16_t COLOR_TRACK = 0x18E3;      ///< Farbe der Arc-Spur außerhalb des aktuellen Fortschritts
    constexpr uint16_t COLOR_PAUSED_DIM = 0x4208; ///< Farbe für pausierten Fortschritt

    // DotMatrix-Farben (Amber-Töne passend zum Theme)
    constexpr uint16_t DOT_ON_COLOR = 0xDCC4;   ///< Farbe für aktive Punkte
    constexpr uint16_t DOT_ON_SHADOW = 0xDCC4;  ///< Schattenfarbe für aktive Punkte
    constexpr uint16_t DOT_OFF_COLOR = 0x0000;  ///< Farbe für inaktive Punkte
    constexpr uint16_t DOT_OFF_SHADOW = 0x0000; ///< Schattenfarbe für inaktive Punkte

    constexpr uint16_t COLOR_BTN_START_BORDER = 0x3666;    ///< Randfarbe für den Start-Button
    constexpr uint16_t COLOR_BTN_START_FILL = 0x0320;      ///< Füllfarbe für den Start-Button
    constexpr uint16_t COLOR_BTN_PAUSE_BORDER = 0xDCC4;    ///< Randfarbe für den Pause-Button
    constexpr uint16_t COLOR_BTN_PAUSE_FILL = 0x6200;      ///< Füllfarbe für den Pause-Button
    constexpr uint16_t COLOR_BTN_STOP_BORDER = 0xF800;     ///< Randfarbe für den Stop-Button
    constexpr uint16_t COLOR_BTN_STOP_FILL = 0x4000;       ///< Füllfarbe für den Stop-Button
    constexpr uint16_t COLOR_BTN_MODE_BORDER = 0x39E8;     ///< Randfarbe für den Modus-Button
    constexpr uint16_t COLOR_BTN_DISABLED_BORDER = 0x2104; ///< Randfarbe für deaktivierte Buttons

    constexpr uint16_t COLOR_TEXT_BRIGHT = 0xFFFF; ///< Helle Textfarbe
    constexpr uint16_t COLOR_TEXT_NORMAL = 0xD69A; ///< Normale Textfarbe
    constexpr uint16_t COLOR_TEXT_DIM = 0x8410;    ///< Gedimmte Textfarbe

    constexpr uint16_t COLOR_CHECK_GREEN = Theme::ColorGreen;        ///< Farbe für Rahmen und Haken einer abgehakten Session-Checkbox
    constexpr uint16_t COLOR_CB_BORDER = Theme::ColorButtonBorder;   ///< Randfarbe einer leeren Session-Checkbox
    constexpr uint16_t COLOR_CB_ACTIVE = 0xDCC4;                     ///< Randfarbe und Mittelpunkt einer Session-Checkbox, die die aktuell laufende Session markiert

    constexpr uint16_t BUTTON_COLOR = 0x39E8; ///< Gemeinsame Randfarbe für alle Buttons

    constexpr uint16_t CLOCK_IDLE_SHADOW = 0x1082;               ///< Schattenfarbe der inaktiven Zeitanzeige
    constexpr uint16_t SEPARATOR_COLOR = 0x2104;                 ///< Farbe der Panel-Trennlinie
    constexpr unsigned long PAUSED_ANIMATION_PERIOD_MS = 1500UL; ///< Dauer des Pause-Pulses
    constexpr unsigned long PAUSED_ANIMATION_TICK_MS = 100UL;    ///< Aktualisierungsintervall des Pause-Pulses

    constexpr float DEG2RAD = 0.0174532925f; ///< Faktor zur Umrechnung von Grad in Bogenmaß
}

// ===================================================================
// KONSTRUKTOR / DESTRUKTOR
// ===================================================================

/**
 * @brief Konstruktor
 */
Pomodoro::Pomodoro()
{
}

/**
 * @brief Destruktor
 */
Pomodoro::~Pomodoro()
{
    // Buttons vom Sprite-Target lösen, bevor die Sprites freigegeben werden
    m_startPauseButton.clearDrawTarget();
    m_stopButton.clearDrawTarget();
    m_modeButton.clearDrawTarget();

    if (m_tft)
    {
        m_tft->safeDeleteSprite(m_gaugeSprite);
        m_tft->safeDeleteSprite(m_clockSprite);
        m_tft->safeDeleteSprite(m_startPauseSprite);
        m_tft->safeDeleteSprite(m_stopSprite);
        m_tft->safeDeleteSprite(m_modeSprite);
    }
}

// ===================================================================
// INITIALISIERUNG
// ===================================================================

/**
 * @brief Initialisiert das Pomodoro-Modul
 */
bool Pomodoro::init(LGFX &tft, const PomodoroCallbacks &callbacks)
{
    m_tft = &tft;
    m_callbacks = callbacks;

    // Gauge-Sprite erstellen
    if (!m_tft->safeCreateSprite(m_gaugeSprite, GAUGE_SPRITE_SIZE, GAUGE_SPRITE_SIZE))
    {
        LOG_ERROR("Pomodoro: Gauge-Sprite konnte nicht erstellt werden");
        return false;
    }

    // Clock-Sprite erstellen
    if (!m_tft->safeCreateSprite(m_clockSprite, CLOCK_SPRITE_W, CLOCK_SPRITE_H))
    {
        LOG_ERROR("Pomodoro: Clock-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_gaugeSprite);
        return false;
    }
    m_clockSprite->clear(Theme::ColorMainBackGround);

    // Button-Sprites für flimmerfreies Rendern der großen Buttons
    if (!m_tft->safeCreateSprite(m_startPauseSprite, START_BTN_W, START_BTN_H))
    {
        LOG_ERROR("Pomodoro: Start-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_gaugeSprite);
        m_tft->safeDeleteSprite(m_clockSprite);
        return false;
    }
    if (!m_tft->safeCreateSprite(m_stopSprite, STOP_BTN_W, STOP_BTN_H))
    {
        LOG_ERROR("Pomodoro: Stop-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_gaugeSprite);
        m_tft->safeDeleteSprite(m_clockSprite);
        m_tft->safeDeleteSprite(m_startPauseSprite);
        return false;
    }
    if (!m_tft->safeCreateSprite(m_modeSprite, MODE_BTN_W, MODE_BTN_H))
    {
        LOG_ERROR("Pomodoro: Mode-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_gaugeSprite);
        m_tft->safeDeleteSprite(m_clockSprite);
        m_tft->safeDeleteSprite(m_startPauseSprite);
        m_tft->safeDeleteSprite(m_stopSprite);
        return false;
    }

    // DotMatrix für Zeitanzeige initialisieren
    m_dotMatrix.init(DOT_WIDTH, DOT_HEIGHT, DOT_SPACE, DOT_CHAR_SPACE);
    m_dotMatrix.setOnColor(DOT_ON_COLOR, DOT_ON_SHADOW);
    m_dotMatrix.setOffColor(DOT_OFF_COLOR, DOT_OFF_SHADOW);
    m_dotMatrix.setBackgroundColor(Theme::ColorMainBackGround);
    m_dotMatrix.setShowOffDots(false);
    m_dotMatrix.setGradientEnabled(false);
    m_dotMatrix.setDotShape(DotMatrix::DotShape::SQUARE);
    m_dotMatrix.reserveText(8);
    m_dotMatrix.setText("25:00");

    m_exitButton.initButton(
        m_tft,                               ///< Display-Objekt
        EXIT_BTN_X,                          ///< X-Position des Exit-Buttons
        EXIT_BTN_Y,                          ///< Y-Position des Exit-Buttons
        EXIT_BTN_SIZE,                       ///< Breite des Exit-Buttons
        EXIT_BTN_SIZE,                       ///< Höhe des Exit-Buttons
        EXIT_BTN_RADIUS,                     ///< Radius des Exit-Buttons
        BUTTON_COLOR,                        ///< Randfarbe des Exit-Buttons
        static_cast<uint16_t>(TFT_BLACK),    ///< Füllfarbe des Exit-Buttons
        static_cast<uint16_t>(TFT_DARKGREY), ///< Schattenfarbe des Exit-Buttons
        "^POWER:32:D206^",                   ///< Text des Exit-Buttons
        nullptr                              ///< kein Icon
    );

    m_exitButton.setFont(LeagueGothicCond34);

    m_startPauseButton.initButton(
        m_tft,                  ///< Display-Objekt
        START_BTN_X,            ///< X-Position des Start/Pause-Buttons
        START_BTN_Y,            ///< Y-Position des Start/Pause-Buttons
        START_BTN_W,            ///< Breite des Start/Pause-Buttons
        START_BTN_H,            ///< Höhe des Start/Pause-Buttons
        START_BTN_RADIUS,       ///< Radius des Start/Pause-Buttons
        COLOR_BTN_START_BORDER, ///< Randfarbe des Start/Pause-Buttons
        COLOR_BTN_START_FILL,   ///< Füllfarbe des Start/Pause-Buttons
        COLOR_TEXT_BRIGHT,      ///< Textfarbe des Start/Pause-Buttons
        reinterpret_cast<const char *>(STR(PomodoroKey::POMODORO_BTN_START)), ///< Text des Start/Pause-Buttons
        nullptr                 ///< kein Icon
    );

    m_startPauseButton.setFont(LeagueGothicCond34);

    m_stopButton.initButton(
        m_tft,                            ///< Display-Objekt
        STOP_BTN_X,                       ///< X-Position des Stop-Buttons
        STOP_BTN_Y,                       ///< Y-Position des Stop-Buttons
        STOP_BTN_W,                       ///< Breite des Stop-Buttons
        STOP_BTN_H,                       ///< Höhe des Stop-Buttons
        STOP_BTN_RADIUS,                  ///< Radius des Stop-Buttons
        COLOR_BTN_DISABLED_BORDER,        ///< Randfarbe des Stop-Buttons
        static_cast<uint16_t>(TFT_BLACK), ///< Füllfarbe des Stop-Buttons
        COLOR_TEXT_DIM,                   ///< Textfarbe des Stop-Buttons
        reinterpret_cast<const char *>(STR(PomodoroKey::POMODORO_BTN_STOP)), ///< Text des Stop-Buttons
        nullptr                           ///< kein Icon
    );

    m_stopButton.setFont(LeagueGothicCond34);
    m_stopButton.disable();

    m_modeButton.initButton(
        m_tft,                            ///< Display-Objekt
        MODE_BTN_X,                       ///< X-Position des Mode-Buttons
        MODE_BTN_Y,                       ///< Y-Position des Mode-Buttons
        MODE_BTN_W,                       ///< Breite des Mode-Buttons
        MODE_BTN_H,                       ///< Höhe des Mode-Buttons
        MODE_BTN_RADIUS,                  ///< Radius des Mode-Buttons
        COLOR_BTN_MODE_BORDER,            ///< Randfarbe des Mode-Buttons
        static_cast<uint16_t>(TFT_BLACK), ///< Füllfarbe des Mode-Buttons
        COLOR_TEXT_NORMAL,                ///< Textfarbe des Mode-Buttons
        reinterpret_cast<const char *>(STR(PomodoroKey::POMODORO_TITLE_TIMER)), ///< Text des Mode-Buttons
        nullptr                           ///< kein Icon
    );

    m_modeButton.setFont(LeagueGothicCond34);

    m_durationUpButton.initButton(
        m_tft,                            ///< Display-Objekt
        DUR_UP_X,                         ///< X-Position des Duration-Up-Buttons
        DUR_BTN_Y,                        ///< Y-Position des Duration-Up-Buttons
        DUR_BTN_SIZE,                     ///< Breite des Duration-Up-Buttons
        DUR_BTN_SIZE,                     ///< Höhe des Duration-Up-Buttons
        DUR_BTN_RADIUS,                   ///< Radius des Duration-Up-Buttons
        COLOR_BTN_MODE_BORDER,            ///< Randfarbe des Duration-Up-Buttons
        static_cast<uint16_t>(TFT_BLACK), ///< Füllfarbe des Duration-Up-Buttons
        COLOR_TEXT_NORMAL,                ///< Textfarbe des Duration-Up-Buttons
        "+",                              ///< Text des Duration-Up-Buttons
        nullptr                           ///< kein Icon
    );

    m_durationUpButton.setFont(LeagueGothicCond34);
    m_durationUpButton.disable();

    m_durationDownButton.initButton(
        m_tft,                            ///< Display-Objekt
        DUR_DOWN_X,                       ///< X-Position des Duration-Down-Buttons
        DUR_BTN_Y,                        ///< Y-Position des Duration-Down-Buttons
        DUR_BTN_SIZE,                     ///< Breite des Duration-Down-Buttons
        DUR_BTN_SIZE,                     ///< Höhe des Duration-Down-Buttons
        DUR_BTN_RADIUS,                   ///< Radius des Duration-Down-Buttons
        COLOR_BTN_MODE_BORDER,            ///< Randfarbe des Duration-Down-Buttons
        static_cast<uint16_t>(TFT_BLACK), ///< Füllfarbe des Duration-Down-Buttons
        COLOR_TEXT_NORMAL,                ///< Textfarbe des Duration-Down-Buttons
        "-",                              ///< Text des Duration-Down-Buttons
        nullptr                           ///< kein Icon
    );

    m_durationDownButton.setFont(LeagueGothicCond34);
    m_durationDownButton.disable();

    // Sprite-Ziel für die großen Buttons: Jeder Button rendert in sein eigenes PSRAM-Sprite, das beim Übertragen atomar auf dem Display landet. Dies verhindert das kurze Aufblitzen durch die zwei fillSmoothRoundRect-Phasen beim direkten Zeichnen, zuerst vollflächig mit der Konturfarbe und dann innen mit der Füllfarbe.
    m_startPauseButton.setDrawTarget(m_startPauseSprite, START_BTN_X - START_BTN_W / 2, START_BTN_Y - START_BTN_H / 2);
    m_stopButton.setDrawTarget(m_stopSprite, STOP_BTN_X - STOP_BTN_W / 2, STOP_BTN_Y - STOP_BTN_H / 2);
    m_modeButton.setDrawTarget(m_modeSprite, MODE_BTN_X - MODE_BTN_W / 2, MODE_BTN_Y - MODE_BTN_H / 2);

    // Tages-Statistik laden
    loadStats();
    checkDateReset();

    // Display vorbereiten
    m_tft->clear(Theme::ColorMainBackGround);
    m_layoutDrawn = false;
    m_controlsNeedRedraw = true;
    m_statsNeedRedraw = true;
    m_lastDisplayedRemaining = -1;
    m_lastPausedAnimationTick = 0;
    m_lastDrawnSecond = -1;

    m_initialized = true;
    LOG_SUCCESS("Pomodoro-Modul erfolgreich initialisiert");
    return true;
}

/**
 * @brief Gibt den Modultyp zurück
 */
AppModule Pomodoro::type() const
{
    return AppModule::POMODORO;
}

// ===================================================================
// HAUPTZEICHENFUNKTION
// ===================================================================

/**
 * @brief Hauptzeichenfunktion - wird zyklisch aufgerufen
 */
void Pomodoro::draw()
{
    if (!m_initialized)
        return;

    // Sprachwechsel? -> kompletten Bildschirm neu zeichnen
    if (I18n::consumeLanguageChanged())
        redrawAll();

    // GFX blockiert?
    const bool gfxBlocked = APP_STATE.getBlockGFX();

    int32_t tx, ty;
    bool touched = !gfxBlocked && m_tft->getTouch(&tx, &ty);

    handleTouch(tx, ty, touched);

    // Während eines Screenshots keine weiteren Zeichen-Operationen
    if (gfxBlocked)
        return;

    // Timer-Update (nur wenn aktiv und nicht pausiert)
    if (m_phase != PomodoroPhase::IDLE && !m_paused)
    {
        int remaining = getRemainingSeconds();

        if (remaining <= 0)
        {
            playBell();
            advancePhase();
            m_controlsNeedRedraw = true;
            m_statsNeedRedraw = true;
        }
    }

    // Statisches Layout beim ersten Aufruf
    if (!m_layoutDrawn)
    {
        drawStaticLayout();
        m_layoutDrawn = true;
    }

    // Gauge aktualisieren bei Sekundenwechsel, Phasenwechsel oder pausierter Puls-Animation
    int currentRemaining = getRemainingSeconds();
    bool phaseChanged = (m_phase != m_lastDrawnPhase);
    unsigned long pausedAnimationTick = 0;
    bool pausedAnimationChanged = false;

    if (m_paused)
    {
        pausedAnimationTick = millis() / PAUSED_ANIMATION_TICK_MS;
        pausedAnimationChanged = (pausedAnimationTick != m_lastPausedAnimationTick);
    }

    if (currentRemaining != m_lastDisplayedRemaining || phaseChanged || pausedAnimationChanged)
    {
        m_lastDisplayedRemaining = currentRemaining;
        m_lastDrawnPhase = m_phase;
        m_lastPausedAnimationTick = pausedAnimationTick;
        drawGauge();
    }

    // Controls und Stats bei Zustandsänderung
    if (m_controlsNeedRedraw)
    {
        updateControls();
        drawPhaseInfo();
        m_controlsNeedRedraw = false;
    }

    if (m_statsNeedRedraw)
    {
        drawDailyStats();
        m_statsNeedRedraw = false;
    }

    // Digitaluhr aktualisieren
    drawDigitalClock(CLOCK_X, CLOCK_Y);
}

// ===================================================================
// TIMER-STEUERUNG
// ===================================================================

/**
 * @brief Startet den Timer
 */
void Pomodoro::startTimer()
{
    if (m_mode == PomodoroMode::POMODORO)
    {
        m_phase = PomodoroPhase::FOCUS;
        m_phaseDurationSec = POMODORO_FOCUS_SEC;
    }
    else
    {
        m_phase = PomodoroPhase::TIMER_ACTIVE;
        m_phaseDurationSec = TIMER_PRESETS[m_timerPresetIndex] * SECONDS_PER_MINUTE;
    }

    m_phaseStartMs = millis();
    m_pauseStartMs = 0;
    m_totalPausedMs = 0;
    m_paused = false;
    m_lastPausedAnimationTick = 0;
    m_controlsNeedRedraw = true;

    LOG_INFO_F("Pomodoro: %s gestartet (%d Sek.)", m_mode == PomodoroMode::POMODORO ? "Fokus" : "Timer", m_phaseDurationSec);
}

/**
 * @brief Pausiert den Timer
 */
void Pomodoro::pauseTimer()
{
    if (m_phase == PomodoroPhase::IDLE || m_paused)
        return;

    m_paused = true;
    m_pauseStartMs = millis();
    m_lastPausedAnimationTick = 0;
    m_controlsNeedRedraw = true;
    LOG_INFO("Pomodoro: Pausiert");
}

/**
 * @brief Setzt den Timer fort
 */
void Pomodoro::resumeTimer()
{
    if (!m_paused)
        return;

    m_totalPausedMs += (millis() - m_pauseStartMs);
    m_paused = false;
    m_pauseStartMs = 0;
    m_lastPausedAnimationTick = 0;
    m_controlsNeedRedraw = true;
    LOG_INFO("Pomodoro: Fortgesetzt");
}

/**
 * @brief Stoppt den Timer komplett
 */
void Pomodoro::stopTimer()
{
    m_phase = PomodoroPhase::IDLE;
    m_paused = false;
    m_lastPausedAnimationTick = 0;
    m_phaseStartMs = 0;
    m_pauseStartMs = 0;
    m_totalPausedMs = 0;
    m_phaseDurationSec = 0;
    m_controlsNeedRedraw = true;
    m_statsNeedRedraw = true;
    LOG_INFO("Pomodoro: Gestoppt");
}

/**
 * @brief Wechselt zur nächsten Phase
 */
void Pomodoro::advancePhase()
{
    if (m_mode == PomodoroMode::TIMER)
    {
        LOG_INFO("Pomodoro: Timer abgelaufen");
        stopTimer();
        return;
    }

    switch (m_phase)
    {
    case PomodoroPhase::FOCUS:
    {
        m_cyclePosition++;
        m_totalFocusToday++;
        saveStats();

        if (m_cyclePosition >= POMODORO_SESSIONS_CYCLE)
        {
            m_phase = PomodoroPhase::LONG_BREAK;
            m_phaseDurationSec = POMODORO_LONG_BREAK_SEC;
            m_cyclePosition = 0;
            LOG_INFO_F("Pomodoro: Lange Pause (nach %d Sessions)", POMODORO_SESSIONS_CYCLE);
        }
        else
        {
            m_phase = PomodoroPhase::SHORT_BREAK;
            m_phaseDurationSec = POMODORO_SHORT_BREAK_SEC;
            LOG_INFO_F("Pomodoro: Kurze Pause (%d/%d)", m_cyclePosition, POMODORO_SESSIONS_CYCLE);
        }
        break;
    }

    case PomodoroPhase::SHORT_BREAK:
    case PomodoroPhase::LONG_BREAK:
        m_phase = PomodoroPhase::FOCUS;
        m_phaseDurationSec = POMODORO_FOCUS_SEC;
        LOG_INFO("Pomodoro: Fokus-Phase gestartet");
        break;

    default:
        stopTimer();
        return;
    }

    m_phaseStartMs = millis();
    m_totalPausedMs = 0;
    m_pauseStartMs = 0;
    m_paused = false;
    m_lastPausedAnimationTick = 0;
    m_controlsNeedRedraw = true;
    m_statsNeedRedraw = true;
}

// ===================================================================
// TIMER-BERECHNUNG
// ===================================================================

/**
 * @brief Berechnet die verbleibenden Sekunden
 */
int Pomodoro::getRemainingSeconds() const
{
    if (m_phase == PomodoroPhase::IDLE || m_phaseDurationSec <= 0)
        return m_phase == PomodoroPhase::IDLE ? getTotalSeconds() : 0;

    unsigned long elapsed;
    if (m_paused)
        elapsed = (m_pauseStartMs - m_phaseStartMs) - m_totalPausedMs;
    else
        elapsed = (millis() - m_phaseStartMs) - m_totalPausedMs;

    int elapsedSec = static_cast<int>(elapsed / 1000UL);
    int remaining = m_phaseDurationSec - elapsedSec;

    return remaining > 0 ? remaining : 0;
}

/**
 * @brief Gibt die Gesamtdauer der aktuellen Phase zurück
 */
int Pomodoro::getTotalSeconds() const
{
    if (m_phase == PomodoroPhase::IDLE)
    {
        if (m_mode == PomodoroMode::POMODORO)
            return POMODORO_FOCUS_SEC;
        return TIMER_PRESETS[m_timerPresetIndex] * SECONDS_PER_MINUTE;
    }

    return m_phaseDurationSec > 0 ? m_phaseDurationSec : 1;
}

// ===================================================================
// PERSISTIERUNG
// ===================================================================

/**
 * @brief Lädt Statistik aus Preferences
 */
void Pomodoro::loadStats()
{
    Preferences prefs;
    if (!prefs.begin(PREF_NAMESPACE, true))
    {
        LOG_WARNING("Pomodoro: Preferences nicht lesbar");
        return;
    }

    m_totalFocusToday = prefs.getUShort(PREF_KEY_COUNT, 0);

    m_lastDate[0] = '\0';
    prefs.getString(PREF_KEY_DATE, m_lastDate, sizeof(m_lastDate));

    m_timerPresetIndex = prefs.getUChar(PREF_KEY_TIMER_IDX, DEFAULT_TIMER_PRESET_INDEX);
    if (m_timerPresetIndex >= TIMER_PRESET_COUNT)
        m_timerPresetIndex = DEFAULT_TIMER_PRESET_INDEX;

    prefs.end();

    LOG_INFO_F("Pomodoro: Stats geladen - Heute: %d, Datum: %s, Timer: %d Min", m_totalFocusToday, m_lastDate, TIMER_PRESETS[m_timerPresetIndex]);
}

/**
 * @brief Speichert Statistik in Preferences
 */
void Pomodoro::saveStats()
{
    checkDateReset();

    Preferences prefs;
    if (!prefs.begin(PREF_NAMESPACE, false))
    {
        LOG_WARNING("Pomodoro: Preferences nicht schreibbar");
        return;
    }

    prefs.putUShort(PREF_KEY_COUNT, static_cast<uint16_t>(m_totalFocusToday));
    prefs.putString(PREF_KEY_DATE, m_lastDate);
    prefs.putUChar(PREF_KEY_TIMER_IDX, static_cast<uint8_t>(m_timerPresetIndex));

    prefs.end();
}

/**
 * @brief Prüft auf Tageswechsel
 */
void Pomodoro::checkDateReset()
{
    struct tm localTime = APP_STATE.getRTC().getCurrentLocalTime();
    char today[11];
    strftime(today, sizeof(today), "%Y-%m-%d", &localTime);

    if (strcmp(m_lastDate, today) != 0)
    {
        if (m_lastDate[0] != '\0')
        {
            LOG_INFO_F("Pomodoro: Tageswechsel %s -> %s", m_lastDate, today);
            m_totalFocusToday = 0;
        }

        stringutils::safeCopy(m_lastDate, today);
    }
}

// ===================================================================
// UI: STATISCHES LAYOUT
// ===================================================================

/**
 * @brief Zeichnet einmalige Layout-Elemente
 */
void Pomodoro::drawStaticLayout()
{
    m_tft->clear(Theme::ColorMainBackGround);

    // Modul-Titel
    m_tft->setFont(&fonts::Orbitron_Light_24);
    m_tft->setTextColor(TFT_SILVER, Theme::ColorMainBackGround);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->drawString(STR(m_mode == PomodoroMode::POMODORO ? PomodoroKey::POMODORO_TITLE_POMODORO : PomodoroKey::POMODORO_TITLE_TIMER), HEADER_LEFT_X, 20);

    // Trennlinie
    m_tft->drawFastHLine(HEADER_LEFT_X, 52, HEADER_W, SEPARATOR_COLOR);

    // Buttons zeichnen
    m_exitButton.drawButton(false);

    m_startPauseSprite->fillSprite(Theme::ColorMainBackGround);
    m_startPauseButton.drawButton(false);
    m_startPauseSprite->pushSprite(START_BTN_X - START_BTN_W / 2, START_BTN_Y - START_BTN_H / 2);

    m_stopSprite->fillSprite(Theme::ColorMainBackGround);
    m_stopButton.drawButton(false);
    m_stopSprite->pushSprite(STOP_BTN_X - STOP_BTN_W / 2, STOP_BTN_Y - STOP_BTN_H / 2);

    m_modeSprite->fillSprite(Theme::ColorMainBackGround);
    m_modeButton.drawButton(false);
    m_modeSprite->pushSprite(MODE_BTN_X - MODE_BTN_W / 2, MODE_BTN_Y - MODE_BTN_H / 2);

    if (m_mode == PomodoroMode::TIMER && m_phase == PomodoroPhase::IDLE)
    {
        m_durationUpButton.enable();
        m_durationDownButton.enable();
        m_durationUpButton.drawButton(false);
        m_durationDownButton.drawButton(false);
    }

    drawPhaseInfo();
    drawDailyStats();
}

// ===================================================================
// UI: VOLLREDRAW NACH SPRACHWECHSEL
// ===================================================================

/**
 * @brief Zeichnet den kompletten Bildschirm nach einem Sprachwechsel neu
 *
 * Initialisiert zuerst die zustandsabhängigen Buttons (START/PAUSE/WEITER/STOP/Modus) mit ihren aktuellen Labels in der neuen Sprache und zeichnet dann das statische Layout neu. Gauge und Digitaluhr werden über zurückgesetzte Change-Detection-Marker im nächsten draw()-Frame neu gezeichnet.
 */
void Pomodoro::redrawAll()
{
    if (!m_tft)
        return;

    // Buttons mit zustandsabhängigen Labels in der aktuellen Sprache neu initialisieren
    updateControls();

    // Statisches Layout neu zeichnen (clear, Titel, Trennlinie, Buttons, Panel-Labels, Stats)
    drawStaticLayout();
    m_layoutDrawn = true;

    // Change-Detection invalidieren - Gauge und Digitaluhr im nächsten Frame neu zeichnen
    m_lastDisplayedRemaining = -1;
    m_lastDrawnSecond = -1;

    // updateControls und drawStaticLayout haben Panels bereits gezeichnet
    m_controlsNeedRedraw = false;
    m_statsNeedRedraw = false;
}

// ===================================================================
// UI: ZIFFERBLATT + GAUGE
// ===================================================================

/**
 * @brief Zeichnet Zifferblatt, Minuten-/Stundenmarkierungen und Fortschrittsanzeige im Gauge-Sprite
 */
void Pomodoro::drawGauge()
{
    if (!m_gaugeSprite)
        return;

    const int cx = GAUGE_SPRITE_SIZE / 2;
    const int cy = GAUGE_SPRITE_SIZE / 2;
    const auto &T = Theme::ActiveMainScreen;

    m_gaugeSprite->clear(Theme::ColorMainBackGround);

    // Koordinaten für die Markierungen berechnen
    auto getCoord = [](float centerX, float centerY, float angleDeg, float length)
        -> std::pair<float, float>
    {
        const float rad = (angleDeg - 90.0f) * DEG2RAD;
        return {centerX + cosf(rad) * length, centerY + sinf(rad) * length};
    };

    // Minuten-Markierungen
    for (int minute = 0; minute < MINUTES_ON_DIAL; minute++)
    {
        if (minute % 5 != 0)
        {
            const float angleDeg = minute * DEGREES_PER_MINUTE;
            auto outer = getCoord(cx, cy, angleDeg, DIAL_RADIUS - MARK_INSET);
            auto inner = getCoord(cx, cy, angleDeg, DIAL_RADIUS - MARK_INSET - MINOR_MARK_LENGTH);
            m_gaugeSprite->drawWideLine(roundf(inner.first), roundf(inner.second), roundf(outer.first), roundf(outer.second), 1.0f, T.clockMinorMarks);
        }
    }

    // Stunden-Markierungen
    for (int hour = 0; hour < HOURS_ON_DIAL; hour++)
    {
        const float angleDeg = hour * DEGREES_PER_HOUR;
        auto outer = getCoord(cx, cy, angleDeg, DIAL_RADIUS - MARK_INSET);
        auto inner = getCoord(cx, cy, angleDeg, DIAL_RADIUS - MARK_INSET - HOUR_MARK_LENGTH);

        float lineWidth = (hour % 3 == 0) ? 3.0f : 2.0f;
        uint16_t markColor = (hour % 3 == 0) ? T.clockMarks : T.clockMinorMarks;

        m_gaugeSprite->drawWideLine(roundf(inner.first), roundf(inner.second), roundf(outer.first), roundf(outer.second), lineWidth, markColor);
    }

    // Countdown-Arc proportional zur Phasendauer
    int totalSec = getTotalSeconds();
    int remaining = getRemainingSeconds();
    uint16_t arcColor = getPhaseColor();

    if (m_paused)
        arcColor = getPulsatingColor(COLOR_PAUSED_DIM, arcColor, PAUSED_ANIMATION_PERIOD_MS);

    // Arc-Spanne: Gesamtminuten / 60 * 360°
    float totalMinutes = totalSec / static_cast<float>(SECONDS_PER_MINUTE);
    float arcAngleSpan = (totalMinutes / static_cast<float>(MINUTES_ON_DIAL)) * 360.0f;
    if (arcAngleSpan > 360.0f)
        arcAngleSpan = 360.0f;

    if (m_phase == PomodoroPhase::IDLE)
    {
        // IDLE: Nur Track als Vorschau
        m_gaugeSprite->drawSmoothGauge(
            cx, cy, ARC_RADIUS, ARC_THICKNESS,
            0.0f, arcAngleSpan,
            0.0f, 1.0f, 0.0f,
            arcColor, COLOR_TRACK, true,
            Theme::ColorMainBackGround);
    }
    else
    {
        // Aktiv: Arc schrumpft mit verbleibender Zeit
        m_gaugeSprite->drawSmoothGauge(
            cx, cy, ARC_RADIUS, ARC_THICKNESS,
            0.0f, arcAngleSpan,
            0.0f, static_cast<float>(totalSec), static_cast<float>(remaining),
            arcColor, COLOR_TRACK, true,
            Theme::ColorMainBackGround);
    }

    // Zentrumstext mit DotMatrix MM:SS und Phasenname
    drawCenterText();

    m_gaugeSprite->pushSprite(GAUGE_SPRITE_X, GAUGE_SPRITE_Y);
}

/**
 * @brief Zeichnet MM:SS (DotMatrix) und Phasenname im Gauge-Zentrum
 */
void Pomodoro::drawCenterText()
{
    if (!m_gaugeSprite)
        return;

    const int cx = GAUGE_SPRITE_SIZE / 2;
    const int cy = GAUGE_SPRITE_SIZE / 2;

    int remaining = getRemainingSeconds();
    int minutes = remaining / SECONDS_PER_MINUTE;
    int seconds = remaining % SECONDS_PER_MINUTE;

    // MM:SS in DotMatrix setzen
    char timeStr[8];
    stringutils::safeFormat(timeStr, "%02d:%02d", minutes, seconds);
    m_dotMatrix.setText(timeStr);

    // DotMatrix-Farbe je nach Phase anpassen
    uint16_t dotColor = (m_phase == PomodoroPhase::IDLE) ? COLOR_TEXT_DIM : getPhaseColor();
    uint16_t dotShadow = (m_phase == PomodoroPhase::IDLE) ? CLOCK_IDLE_SHADOW : DOT_ON_SHADOW;

    if (m_paused)
    {
        dotColor = getPulsatingColor(COLOR_TEXT_DIM, getPhaseColor(), PAUSED_ANIMATION_PERIOD_MS);
        dotShadow = DOT_ON_SHADOW;
    }

    m_dotMatrix.setOnColor(dotColor, dotShadow);

    // DotMatrix zentriert zeichnen
    int textW = m_dotMatrix.getTextWidth();
    int textH = m_dotMatrix.getHeight();
    int textX = cx - textW / 2;
    int textY = cy - textH / 2 - 12; // Etwas nach oben, Platz für Phasenname

    m_dotMatrix.draw(m_gaugeSprite, textX, textY);

    // Phasenname darunter (LeagueGothicCond34)
    const __FlashStringHelper *phaseName = getPhaseName();
    uint16_t phaseColor = (m_phase == PomodoroPhase::IDLE) ? COLOR_TEXT_DIM : getPhaseColor();

    m_gaugeSprite->loadFont(LeagueGothicCond34);
    m_gaugeSprite->setTextDatum(TC_DATUM);
    m_gaugeSprite->setTextColor(phaseColor, Theme::ColorMainBackGround);
    m_gaugeSprite->drawString(phaseName, cx, textY + textH + 6);
    m_gaugeSprite->unloadFont();
}

// ===================================================================
// UI: PHASEN-INFO & SESSION-CHECKBOXEN
// ===================================================================

/**
 * @brief Zeichnet Phasen-Info und Session-Checkboxen
 */
void Pomodoro::drawPhaseInfo()
{
    if (!m_tft)
        return;

    // Bereich löschen
    m_tft->fillRect(PANEL_X, 56, DISPLAY_W - PANEL_X, 110, Theme::ColorMainBackGround);

    if (m_mode == PomodoroMode::POMODORO)
    {
        // Wieviele Boxen als abgehakt zeichnen
        int boxesCompleted = m_cyclePosition;
        if (m_phase == PomodoroPhase::LONG_BREAK)
            boxesCompleted = POMODORO_SESSIONS_CYCLE;

        // Counter-Zahl: aktuelle Session-Nummer in FOCUS, sonst abgeschlossene Sessions
        int displayCount;
        if (m_phase == PomodoroPhase::FOCUS)
            displayCount = m_cyclePosition + 1;
        else
            displayCount = boxesCompleted;

        // Label "Session X/Y"
        char sessionStr[24];
        STRF(sessionStr, PomodoroKey::POMODORO_LABEL_SESSION_FMT, displayCount, POMODORO_SESSIONS_CYCLE);

        m_tft->loadFont(ComfortaaBold16);
        m_tft->setTextDatum(TL_DATUM);
        m_tft->setTextColor(COLOR_TEXT_NORMAL, Theme::ColorMainBackGround);
        m_tft->drawString(sessionStr, HEADER_LEFT_X, SESSION_LABEL_Y);
        m_tft->unloadFont();

        // Checkboxen zeichnen
        for (int i = 0; i < POMODORO_SESSIONS_CYCLE; i++)
        {
            int cbX = HEADER_LEFT_X + i * SESSION_CB_SPACING;
            int cbY = SESSION_CB_Y;

            bool checked = (i < boxesCompleted);
            bool active = (i == m_cyclePosition && m_phase == PomodoroPhase::FOCUS);

            drawCheckbox(cbX, cbY, SESSION_CB_SIZE, checked, active);
        }
    }
    else
    {
        // Timer-Modus: Dauer-Anzeige
        m_tft->loadFont(ComfortaaBold16);
        m_tft->setTextDatum(TL_DATUM);
        m_tft->setTextColor(COLOR_TEXT_NORMAL, Theme::ColorMainBackGround);
        m_tft->drawString(STR(PomodoroKey::POMODORO_LABEL_DURATION), HEADER_LEFT_X, SESSION_LABEL_Y);
        m_tft->unloadFont();

        char durStr[12];
        STRF(durStr, PomodoroKey::POMODORO_DURATION_FMT, TIMER_PRESETS[m_timerPresetIndex]);

        m_tft->loadFont(LeagueGothicCond34);
        m_tft->setTextDatum(MC_DATUM);
        m_tft->setTextColor(COLOR_TIMER, Theme::ColorMainBackGround);
        m_tft->drawString(durStr, PANEL_CENTER_X, DUR_BTN_Y);
        m_tft->unloadFont();

        if (m_phase == PomodoroPhase::IDLE)
        {
            m_durationUpButton.enable();
            m_durationDownButton.enable();
            m_durationUpButton.drawButton(false);
            m_durationDownButton.drawButton(false);
        }
        else
        {
            m_durationUpButton.disable();
            m_durationDownButton.disable();
        }
    }
}

/**
 * @brief Zeichnet eine Checkbox mit optionalem Haken
 */
void Pomodoro::drawCheckbox(int x, int y, int size, bool checked, bool active)
{
    if (!m_tft)
        return;

    uint16_t color;
    if (checked)
        color = COLOR_CHECK_GREEN;
    else if (active)
        color = COLOR_CB_ACTIVE;
    else
        color = COLOR_CB_BORDER;

    m_tft->drawRect(x, y, size, size, color);

    if (checked)
    {
        const int cx = x + size / 2;
        const int cy = y + size / 2;
        m_tft->drawWideLine(cx - 5, cy - 1, cx - 1, cy + 3, 2.0f, color);
        m_tft->drawWideLine(cx - 1, cy + 3, cx + 6, cy - 4, 2.0f, color);
    }
    else if (active)
    {
        const int cx = x + size / 2;
        const int cy = y + size / 2;
        m_tft->fillSmoothCircle(cx, cy, size / 4, COLOR_CB_ACTIVE);
    }
}

// ===================================================================
// UI: BUTTON-AKTUALISIERUNG
// ===================================================================

/**
 * @brief Aktualisiert Button-Labels und Farben je nach Zustand
 */
void Pomodoro::updateControls()
{
    // Start/Pause-Button
    if (m_phase == PomodoroPhase::IDLE)
    {
        m_startPauseButton.initButton(
            m_tft,                  ///< Display-Objekt
            START_BTN_X,            ///< X-Position
            START_BTN_Y,            ///< Y-Position
            START_BTN_W,            ///< Breite
            START_BTN_H,            ///< Höhe
            START_BTN_RADIUS,       ///< Radius
            COLOR_BTN_START_BORDER, ///< Randfarbe
            COLOR_BTN_START_FILL,   ///< Füllfarbe
            COLOR_TEXT_BRIGHT,      ///< Textfarbe
            reinterpret_cast<const char *>(STR(PomodoroKey::POMODORO_BTN_START)), ///< Text
            nullptr                 ///< kein Icon
        );

        m_startPauseButton.setFont(LeagueGothicCond34);
        m_startPauseButton.enable();
    }
    else if (m_paused)
    {
        m_startPauseButton.initButton(
            m_tft,                  ///< Display-Objekt
            START_BTN_X,            ///< X-Position
            START_BTN_Y,            ///< Y-Position
            START_BTN_W,            ///< Breite
            START_BTN_H,            ///< Höhe
            START_BTN_RADIUS,       ///< Radius
            COLOR_BTN_START_BORDER, ///< Randfarbe
            COLOR_BTN_START_FILL,   ///< Füllfarbe
            COLOR_TEXT_BRIGHT,      ///< Textfarbe
            reinterpret_cast<const char *>(STR(PomodoroKey::POMODORO_BTN_RESUME)), ///< Text
            nullptr                 ///< kein Icon
        );

        m_startPauseButton.setFont(LeagueGothicCond34);
        m_startPauseButton.enable();
    }
    else
    {
        m_startPauseButton.initButton(
            m_tft,                  ///< Display-Objekt
            START_BTN_X,            ///< X-Position
            START_BTN_Y,            ///< Y-Position
            START_BTN_W,            ///< Breite
            START_BTN_H,            ///< Höhe
            START_BTN_RADIUS,       ///< Radius
            COLOR_BTN_PAUSE_BORDER, ///< Randfarbe
            COLOR_BTN_PAUSE_FILL,   ///< Füllfarbe
            COLOR_TEXT_BRIGHT,      ///< Textfarbe
            reinterpret_cast<const char *>(STR(PomodoroKey::POMODORO_BTN_PAUSE)), ///< Text
            nullptr                 ///< kein Icon
        );

        m_startPauseButton.setFont(LeagueGothicCond34);
        m_startPauseButton.enable();
    }

    m_startPauseSprite->fillSprite(Theme::ColorMainBackGround);
    m_startPauseButton.drawButton(false);
    m_startPauseSprite->pushSprite(START_BTN_X - START_BTN_W / 2, START_BTN_Y - START_BTN_H / 2);

    // Stop-Button
    if (m_phase != PomodoroPhase::IDLE)
    {
        m_stopButton.initButton(
            m_tft,                 ///< Display-Objekt
            STOP_BTN_X,            ///< X-Position
            STOP_BTN_Y,            ///< Y-Position
            STOP_BTN_W,            ///< Breite
            STOP_BTN_H,            ///< Höhe
            STOP_BTN_RADIUS,       ///< Radius
            COLOR_BTN_STOP_BORDER, ///< Randfarbe
            COLOR_BTN_STOP_FILL,   ///< Füllfarbe
            COLOR_TEXT_BRIGHT,     ///< Textfarbe
            reinterpret_cast<const char *>(STR(PomodoroKey::POMODORO_BTN_STOP)), ///< Text
            nullptr                ///< kein Icon
        );

        m_stopButton.setFont(LeagueGothicCond34);
        m_stopButton.enable();
    }
    else
    {
        m_stopButton.initButton(
            m_tft,                            ///< Display-Objekt
            STOP_BTN_X,                       ///< X-Position
            STOP_BTN_Y,                       ///< Y-Position
            STOP_BTN_W,                       ///< Breite
            STOP_BTN_H,                       ///< Höhe
            STOP_BTN_RADIUS,                  ///< Radius
            COLOR_BTN_DISABLED_BORDER,        ///< Randfarbe
            static_cast<uint16_t>(TFT_BLACK), ///< Füllfarbe
            COLOR_TEXT_DIM,                   ///< Textfarbe
            reinterpret_cast<const char *>(STR(PomodoroKey::POMODORO_BTN_STOP)), ///< Text
            nullptr                           ///< kein Icon
        );

        m_stopButton.setFont(LeagueGothicCond34);
        m_stopButton.disable();
    }
    m_stopSprite->fillSprite(Theme::ColorMainBackGround);
    m_stopButton.drawButton(false);
    m_stopSprite->pushSprite(STOP_BTN_X - STOP_BTN_W / 2, STOP_BTN_Y - STOP_BTN_H / 2);

    // Modus-Button
    if (m_phase == PomodoroPhase::IDLE)
    {
        const PomodoroKey modeKey = (m_mode == PomodoroMode::POMODORO) ? PomodoroKey::POMODORO_TITLE_TIMER : PomodoroKey::POMODORO_TITLE_POMODORO;
        m_modeButton.initButton(
            m_tft,                            ///< Display-Objekt
            MODE_BTN_X,                       ///< X-Position
            MODE_BTN_Y,                       ///< Y-Position
            MODE_BTN_W,                       ///< Breite
            MODE_BTN_H,                       ///< Höhe
            MODE_BTN_RADIUS,                  ///< Radius
            COLOR_BTN_MODE_BORDER,            ///< Randfarbe
            static_cast<uint16_t>(TFT_BLACK), ///< Füllfarbe
            COLOR_TEXT_NORMAL,                ///< Textfarbe
            reinterpret_cast<const char *>(STR(modeKey)), ///< Text
            nullptr                           ///< kein Icon
        );

        m_modeButton.setFont(LeagueGothicCond34);
        m_modeButton.enable();
    }
    else
    {
        m_modeButton.disable();
    }

    m_modeSprite->fillSprite(Theme::ColorMainBackGround);
    m_modeButton.drawButton(false);
    m_modeSprite->pushSprite(MODE_BTN_X - MODE_BTN_W / 2, MODE_BTN_Y - MODE_BTN_H / 2);

    // Modul-Titel aktualisieren
    m_tft->fillRect(HEADER_LEFT_X, 14, HEADER_W, 34, Theme::ColorMainBackGround);
    m_tft->setFont(&fonts::Orbitron_Light_24);
    m_tft->setTextColor(TFT_SILVER, Theme::ColorMainBackGround);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->drawString(STR(m_mode == PomodoroMode::POMODORO ? PomodoroKey::POMODORO_TITLE_POMODORO : PomodoroKey::POMODORO_TITLE_TIMER), HEADER_LEFT_X, 20);
}

// ===================================================================
// UI: TAGES-STATISTIK
// ===================================================================

/**
 * @brief Zeichnet die Tages-Statistik im Pomodoro-Modus
 */
void Pomodoro::drawDailyStats()
{
    if (!m_tft)
        return;

    // Bereich unter dem Zifferblatt löschen
    m_tft->fillRect(STATS_CENTER_X - 100, STATS_Y + 1, 200, 17, Theme::ColorMainBackGround);

    // Nur im Pomodoro-Modus anzeigen
    if (m_mode != PomodoroMode::POMODORO)
        return;

    char statsStr[32];
    const PomodoroKey statsKey = (m_totalFocusToday == 1)
                                     ? PomodoroKey::POMODORO_STATS_SINGULAR
                                     : PomodoroKey::POMODORO_STATS_PLURAL;
    STRF(statsStr, statsKey, m_totalFocusToday);

    m_tft->loadFont(ComfortaaBold16);
    m_tft->setTextDatum(TC_DATUM);
    m_tft->setTextColor(COLOR_TEXT_DIM, Theme::ColorMainBackGround);
    m_tft->drawString(statsStr, STATS_CENTER_X, STATS_Y);
    m_tft->unloadFont();
}

// ===================================================================
// TOUCH-HANDLING
// ===================================================================

/**
 * @brief Verarbeitet Touch-Eingaben
 */
void Pomodoro::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    // Exit-Button
    if (m_exitButton.isPressed() && !touched)
    {
        m_exitButton.press(false);
        if (m_callbacks.onExitRequested)
            m_callbacks.onExitRequested();
        return;
    }
    if (touched && m_exitButton.contains(tx, ty))
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

    // Start/Pause-Button
    if (m_startPauseButton.isPressed() && !touched)
    {
        m_startPauseButton.press(false);
        m_startPauseSprite->fillSprite(Theme::ColorMainBackGround);
        m_startPauseButton.drawButton(false);
        m_startPauseSprite->pushSprite(START_BTN_X - START_BTN_W / 2, START_BTN_Y - START_BTN_H / 2);

        if (m_phase == PomodoroPhase::IDLE)
            startTimer();
        else if (m_paused)
            resumeTimer();
        else
            pauseTimer();

        return;
    }
    if (touched && m_startPauseButton.contains(tx, ty) && m_startPauseButton.isEnabled())
    {
        if (!m_startPauseButton.isPressed())
        {
            m_startPauseButton.press(true);
            m_startPauseSprite->fillSprite(Theme::ColorMainBackGround);
            m_startPauseButton.drawButton(true);
            m_startPauseSprite->pushSprite(START_BTN_X - START_BTN_W / 2, START_BTN_Y - START_BTN_H / 2);
        }
    }
    else if (m_startPauseButton.isPressed())
    {
        m_startPauseButton.press(false);
        m_startPauseSprite->fillSprite(Theme::ColorMainBackGround);
        m_startPauseButton.drawButton(false);
        m_startPauseSprite->pushSprite(START_BTN_X - START_BTN_W / 2, START_BTN_Y - START_BTN_H / 2);
    }

    // Stop-Button
    if (m_stopButton.isPressed() && !touched)
    {
        m_stopButton.press(false);
        stopTimer();
        return;
    }
    if (touched && m_stopButton.contains(tx, ty) && m_stopButton.isEnabled())
    {
        if (!m_stopButton.isPressed())
        {
            m_stopButton.press(true);
            m_stopSprite->fillSprite(Theme::ColorMainBackGround);
            m_stopButton.drawButton(true);
            m_stopSprite->pushSprite(STOP_BTN_X - STOP_BTN_W / 2, STOP_BTN_Y - STOP_BTN_H / 2);
        }
    }
    else if (m_stopButton.isPressed())
    {
        m_stopButton.press(false);
        m_stopSprite->fillSprite(Theme::ColorMainBackGround);
        m_stopButton.drawButton(false);
        m_stopSprite->pushSprite(STOP_BTN_X - STOP_BTN_W / 2, STOP_BTN_Y - STOP_BTN_H / 2);
    }

    // Modus-Button
    if (m_modeButton.isPressed() && !touched)
    {
        m_modeButton.press(false);

        if (m_phase == PomodoroPhase::IDLE)
        {
            m_mode = (m_mode == PomodoroMode::POMODORO)
                         ? PomodoroMode::TIMER
                         : PomodoroMode::POMODORO;

            m_controlsNeedRedraw = true;
            m_statsNeedRedraw = true;
            m_lastDisplayedRemaining = -1;
            LOG_INFO_F("Pomodoro: Modus -> %s", m_mode == PomodoroMode::POMODORO ? "Pomodoro" : "Timer");
        }
        return;
    }
    if (touched && m_modeButton.contains(tx, ty) && m_modeButton.isEnabled())
    {
        if (!m_modeButton.isPressed())
        {
            m_modeButton.press(true);
            m_modeSprite->fillSprite(Theme::ColorMainBackGround);
            m_modeButton.drawButton(true);
            m_modeSprite->pushSprite(MODE_BTN_X - MODE_BTN_W / 2, MODE_BTN_Y - MODE_BTN_H / 2);
        }
    }
    else if (m_modeButton.isPressed())
    {
        m_modeButton.press(false);
        m_modeSprite->fillSprite(Theme::ColorMainBackGround);
        m_modeButton.drawButton(false);
        m_modeSprite->pushSprite(MODE_BTN_X - MODE_BTN_W / 2, MODE_BTN_Y - MODE_BTN_H / 2);
    }

    // Timer-Dauer erhöhen
    if (m_durationUpButton.isPressed() && !touched)
    {
        m_durationUpButton.press(false);
        m_durationUpButton.drawButton(false);

        if (m_mode == PomodoroMode::TIMER && m_phase == PomodoroPhase::IDLE)
        {
            if (m_timerPresetIndex < TIMER_PRESET_COUNT - 1)
            {
                m_timerPresetIndex++;
                saveStats();
                m_controlsNeedRedraw = true;
                m_lastDisplayedRemaining = -1;
            }
        }
        return;
    }
    if (touched && m_durationUpButton.contains(tx, ty) && m_durationUpButton.isEnabled())
    {
        if (!m_durationUpButton.isPressed())
        {
            m_durationUpButton.press(true);
            m_durationUpButton.drawButton(true);
        }
    }
    else if (m_durationUpButton.isPressed())
    {
        m_durationUpButton.press(false);
        m_durationUpButton.drawButton(false);
    }

    // Timer-Dauer verringern
    if (m_durationDownButton.isPressed() && !touched)
    {
        m_durationDownButton.press(false);
        m_durationDownButton.drawButton(false);

        if (m_mode == PomodoroMode::TIMER && m_phase == PomodoroPhase::IDLE)
        {
            if (m_timerPresetIndex > 0)
            {
                m_timerPresetIndex--;
                saveStats();
                m_controlsNeedRedraw = true;
                m_lastDisplayedRemaining = -1;
            }
        }
        return;
    }
    if (touched && m_durationDownButton.contains(tx, ty) && m_durationDownButton.isEnabled())
    {
        if (!m_durationDownButton.isPressed())
        {
            m_durationDownButton.press(true);
            m_durationDownButton.drawButton(true);
        }
    }
    else if (m_durationDownButton.isPressed())
    {
        m_durationDownButton.press(false);
        m_durationDownButton.drawButton(false);
    }
}

// ===================================================================
// HILFSFUNKTIONEN
// ===================================================================

/**
 * @brief Phasenfarbe für den Gauge-Arc
 */
uint16_t Pomodoro::getPhaseColor() const
{
    switch (m_phase)
    {
    case PomodoroPhase::FOCUS:
        return COLOR_FOCUS;
    case PomodoroPhase::SHORT_BREAK:
        return COLOR_SHORT_BREAK;
    case PomodoroPhase::LONG_BREAK:
        return COLOR_LONG_BREAK;
    case PomodoroPhase::TIMER_ACTIVE:
        return COLOR_TIMER;
    case PomodoroPhase::IDLE:
    default:
        return (m_mode == PomodoroMode::TIMER) ? COLOR_TIMER : COLOR_FOCUS;
    }
}

/**
 * @brief Anzeigename der Phase
 */
const __FlashStringHelper *Pomodoro::getPhaseName() const
{
    switch (m_phase)
    {
    case PomodoroPhase::FOCUS:
        return STR(PomodoroKey::POMODORO_PHASE_FOCUS);
    case PomodoroPhase::SHORT_BREAK:
        return STR(PomodoroKey::POMODORO_PHASE_SHORT_BREAK);
    case PomodoroPhase::LONG_BREAK:
        return STR(PomodoroKey::POMODORO_PHASE_LONG_BREAK);
    case PomodoroPhase::TIMER_ACTIVE:
        return STR(PomodoroKey::POMODORO_PHASE_TIMER);
    case PomodoroPhase::IDLE:
    default:
        if (m_paused)
            return STR(PomodoroKey::POMODORO_PHASE_PAUSED);
        return STR((m_mode == PomodoroMode::POMODORO) ? PomodoroKey::POMODORO_PHASE_READY : PomodoroKey::POMODORO_PHASE_TIMER);
    }
}

/**
 * @brief Spielt den Phasenwechsel-Sound
 */
void Pomodoro::playBell()
{
    if (m_callbacks.onPlaySound)
    {
        m_callbacks.onPlaySound(BELL_SOUND_PATH);
        LOG_INFO("Pomodoro: Bell-Sound abgespielt");
    }
}

// ===================================================================
// DIGITALUHR
// ===================================================================

/**
 * @brief Zeichnet die Digitaluhr
 */
void Pomodoro::drawDigitalClock(int32_t x, int32_t y)
{
    if (!m_clockSprite)
        return;

    DS1307_RTC *rtc = &APP_STATE.getRTC();
    Clocks::drawDigitalClockPlain(m_clockSprite, rtc, m_lastDrawnSecond, x, y, false, true);
}
