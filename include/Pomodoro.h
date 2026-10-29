#ifndef POMODORO_H
#define POMODORO_H

#include <stdint.h>

#include "BaseModule.h"
#include "Button.h"
#include "DotMatrix.h"
#include "Gfx.h"

// ===========================================================================
// Pomodoro-Konfiguration
// ===========================================================================

static constexpr int POMODORO_FOCUS_SEC = 25 * 60;      ///< Fokus-Phase: 25 Minuten
static constexpr int POMODORO_SHORT_BREAK_SEC = 5 * 60; ///< Kurze Pause: 5 Minuten
static constexpr int POMODORO_LONG_BREAK_SEC = 15 * 60; ///< Lange Pause: 15 Minuten
static constexpr int POMODORO_SESSIONS_CYCLE = 4;       ///< Fokus-Sessions bis zur langen Pause

/**
 * @brief Betriebsmodus des Pomodoro-Moduls
 */
enum class PomodoroMode : uint8_t
{
    POMODORO, ///< Klassischer Pomodoro-Zyklus (25/5/15)
    TIMER     ///< Freier Countdown-Timer (1-60 Min)
};

/**
 * @brief Aktuelle Phase im Pomodoro-Zyklus bzw. Timer-Modus
 */
enum class PomodoroPhase : uint8_t
{
    IDLE,        ///< Bereit zum Starten
    FOCUS,       ///< Fokus-Phase (25 Min)
    SHORT_BREAK, ///< Kurze Pause (5 Min)
    LONG_BREAK,  ///< Lange Pause (15 Min)
    TIMER_ACTIVE ///< Freier Timer läuft
};

/**
 * @brief Callbacks für das Pomodoro-Modul
 */
struct PomodoroCallbacks
{
    void (*onExitRequested)() = nullptr;             ///< Wird beim Verlassen aufgerufen
    void (*onPlaySound)(const char *path) = nullptr; ///< MP3 von SD-Karte abspielen
};

/**
 * @brief Pomodoro-Timer-Modul mit Zifferblatt und Countdown-Arc
 *
 * Architektur:
 * - Rein lokales Modul ohne Backend, millis()-basiert
 * - Analoges Zifferblatt ohne Zeiger mit äußerem Arc
 * - DotMatrix-Anzeige mit monospaced Zeitdarstellung im Zentrum
 * - Persistierung des Tages-Zählers über Preferences
 *
 * UI-Layout (800x480):
 * - Links: Zifferblatt mit proportionalem Arc und DotMatrix MM:SS im Zentrum
 * - Rechts: Session-Checkboxen, Steuerbuttons, Modus-Umschaltung und Tages-Statistik
 * - Unten rechts: Exit-Button und Digitaluhr
 */
class Pomodoro : public BaseModule
{
public:
    /**
     * @brief Erstellt ein Pomodoro-Modul
     */
    Pomodoro();

    /**
     * @brief Gibt Sprites und lokale Ressourcen frei
     */
    ~Pomodoro() override;

    // Keine Kopien oder Moves
    Pomodoro(const Pomodoro &) = delete;
    Pomodoro &operator=(const Pomodoro &) = delete;
    Pomodoro(Pomodoro &&) = delete;
    Pomodoro &operator=(Pomodoro &&) = delete;

    /**
     * @brief Initialisiert das Pomodoro-Modul
     * @param tft Referenz auf das Display-Objekt
     * @param callbacks Callback-Funktionen
     * @return true bei Erfolg
     */
    bool init(LGFX &tft, const PomodoroCallbacks &callbacks);

    /**
     * @brief Hauptzeichenfunktion, die zyklisch aufgerufen wird
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück
     * @return AppModule::POMODORO
     */
    AppModule type() const override;

private:
    // =======================================================================
    // Timer-Steuerung
    // =======================================================================

    void startTimer();   ///< Startet den Timer aus dem IDLE-Zustand
    void pauseTimer();   ///< Pausiert den laufenden Timer
    void resumeTimer();  ///< Setzt den pausierten Timer fort
    void stopTimer();    ///< Stoppt den Timer und kehrt zu IDLE zurück
    void advancePhase(); ///< Wechselt zur nächsten Phase im Pomodoro-Zyklus

    // =======================================================================
    // Timer-Berechnung
    // =======================================================================

    /**
     * @brief Berechnet die verbleibenden Sekunden der aktuellen Phase
     * @return Verbleibende Sekunden, im IDLE-Zustand die Startdauer
     */
    int getRemainingSeconds() const;

    /**
     * @brief Gibt die Gesamtdauer der aktuellen Phase zurück
     * @return Gesamtdauer in Sekunden
     */
    int getTotalSeconds() const;

    // =======================================================================
    // Persistierung
    // =======================================================================

    void loadStats();      ///< Lädt Tages-Statistik und Timer-Preset aus Preferences
    void saveStats();      ///< Speichert Tages-Statistik und Timer-Preset
    void checkDateReset(); ///< Prüft auf Tageswechsel und setzt den Zähler zurück

    // =======================================================================
    // UI-Zeichenfunktionen
    // =======================================================================

    void drawStaticLayout();                     ///< Zeichnet einmalige Layout-Elemente
    void redrawAll();                            ///< Vollredraw nach Sprachwechsel
    void drawGauge();                            ///< Zeichnet Zifferblatt und Countdown-Arc
    void drawCenterText();                       ///< Zeichnet MM:SS und Phasenname im Zentrum
    void drawPhaseInfo();                        ///< Zeichnet Session-Checkboxen und Phasen-Info
    void updateControls();                       ///< Aktualisiert Buttons je nach Zustand
    void drawDailyStats();                       ///< Zeichnet die Tages-Statistik
    void drawDigitalClock(int32_t x, int32_t y); ///< Zeichnet die Digitaluhr

    /**
     * @brief Zeichnet eine Checkbox mit optionalem Haken
     * @param x X-Position links oben
     * @param y Y-Position links oben
     * @param size Seitenlänge
     * @param checked true, wenn die Checkbox abgehakt ist
     * @param active true, wenn die Checkbox die aktuelle Session markiert
     */
    void drawCheckbox(int x, int y, int size, bool checked, bool active);

    // =======================================================================
    // Touch-Handling
    // =======================================================================

    void handleTouch(int32_t tx, int32_t ty, bool touched);

    // =======================================================================
    // Hilfsfunktionen
    // =======================================================================

    uint16_t getPhaseColor() const;                  ///< Farbe für den Countdown-Arc
    const __FlashStringHelper *getPhaseName() const; ///< Anzeigename der aktuellen Phase
    void playBell();                                  ///< Spielt den Phasenwechsel-Sound

    // =======================================================================
    // Member-Variablen
    // =======================================================================

    // Core
    LGFX *m_tft = nullptr;         ///< Display-Referenz
    PomodoroCallbacks m_callbacks; ///< Callback-Funktionen
    bool m_initialized = false;    ///< Modul erfolgreich initialisiert

    // Buttons
    Button m_exitButton;         ///< Exit unten rechts
    Button m_startPauseButton;   ///< Start / Pause / Weiter
    Button m_stopButton;         ///< Stop / Reset
    Button m_modeButton;         ///< Modus-Umschaltung Pomodoro/Timer
    Button m_durationUpButton;   ///< Timer-Dauer erhöhen, nur im Timer-Modus und nur in IDLE
    Button m_durationDownButton; ///< Timer-Dauer verringern

    // Zustand
    PomodoroMode m_mode = PomodoroMode::POMODORO; ///< Aktueller Betriebsmodus
    PomodoroPhase m_phase = PomodoroPhase::IDLE;  ///< Aktuelle Phase
    bool m_paused = false;                        ///< Timer ist pausiert

    // Timer, millis()-basiert
    unsigned long m_phaseStartMs = 0;  ///< millis() bei Phasenstart
    unsigned long m_pauseStartMs = 0;  ///< millis() bei Pausenbeginn
    unsigned long m_totalPausedMs = 0; ///< Kumulierte Pausenzeit in der aktuellen Phase
    int m_phaseDurationSec = 0;        ///< Dauer der aktuellen Phase in Sekunden

    // Pomodoro-Zyklus
    int m_cyclePosition = 0;   ///< Position im 4er-Zyklus (0-3)
    int m_totalFocusToday = 0; ///< Abgeschlossene Fokus-Sessions heute

    // Timer-Modus
    int m_timerPresetIndex = 12; ///< Index des aktiven Timer-Presets, Default: 25 Min

    // Persistenz
    char m_lastDate[11] = {}; ///< YYYY-MM-DD für Tageswechsel-Erkennung

    // Zeitanzeige
    DotMatrix m_dotMatrix; ///< HD44780-Punktmatrix für MM:SS

    // Sprites
    Gfx_Sprite *m_gaugeSprite = nullptr; ///< Sprite für Zifferblatt und Arc
    Gfx_Sprite *m_clockSprite = nullptr; ///< Sprite für die Digitaluhr
    Gfx_Sprite *m_startPauseSprite = nullptr; ///< Sprite für den Start/Pause-Button (flimmerfrei)
    Gfx_Sprite *m_stopSprite = nullptr;       ///< Sprite für den Stop-Button (flimmerfrei)
    Gfx_Sprite *m_modeSprite = nullptr;       ///< Sprite für den Modus-Button (flimmerfrei)
    int m_lastDrawnSecond = -1;          ///< Letzte gezeichnete Sekunde der Digitaluhr

    // UI-State
    bool m_layoutDrawn = false;                           ///< Statisches Layout bereits gezeichnet
    int m_lastDisplayedRemaining = -1;                    ///< Letzter angezeigter Sekundenwert
    unsigned long m_lastPausedAnimationTick = 0;          ///< Letzter Animations-Takt für den Pause-Puls
    bool m_controlsNeedRedraw = true;                     ///< Buttons müssen aktualisiert werden
    bool m_statsNeedRedraw = true;                        ///< Statistik muss aktualisiert werden
    PomodoroPhase m_lastDrawnPhase = PomodoroPhase::IDLE; ///< Letzte gezeichnete Phase
};

#endif // POMODORO_H
