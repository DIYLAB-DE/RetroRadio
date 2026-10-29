#ifndef STRINGS_POMODORO_H
#define STRINGS_POMODORO_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte des Pomodoro-Moduls
 *
 * Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_pomodoro.cpp` erweitern. Der `static_assert` dort verhindert eine Drift zwischen Enum und Tabelle.
 */
enum class PomodoroKey : uint16_t
{
    // ===================================================================
    // Modul-Titel (oben im rechten Panel) und Modus-Umschalt-Button
    // ===================================================================
    POMODORO_TITLE_POMODORO = 0, ///< "POMODORO" / "POMODORO"
    POMODORO_TITLE_TIMER,        ///< "TIMER" / "TIMER"

    // ===================================================================
    // Button-Labels
    // ===================================================================
    POMODORO_BTN_START,  ///< "START" / "START"
    POMODORO_BTN_STOP,   ///< "STOP" / "STOP"
    POMODORO_BTN_PAUSE,  ///< "PAUSE" / "PAUSE"
    POMODORO_BTN_RESUME, ///< "WEITER" / "RESUME"

    // ===================================================================
    // Panel-Labels
    // ===================================================================
    POMODORO_LABEL_SESSION_FMT, ///< "Session {0:%d}/{1:%d}" / "Session {0:%d}/{1:%d}"
    POMODORO_LABEL_DURATION,    ///< "Timer-Dauer" / "Timer duration"

    // ===================================================================
    // Phasen-Namen im Gauge-Zentrum
    // ===================================================================
    POMODORO_PHASE_FOCUS,       ///< "FOKUS" / "FOCUS"
    POMODORO_PHASE_SHORT_BREAK, ///< "PAUSE" / "BREAK"
    POMODORO_PHASE_LONG_BREAK,  ///< "LANGE PAUSE" / "LONG BREAK"
    POMODORO_PHASE_TIMER,       ///< "TIMER" / "TIMER" (TIMER_ACTIVE)
    POMODORO_PHASE_PAUSED,      ///< "PAUSIERT" / "PAUSED"
    POMODORO_PHASE_READY,       ///< "BEREIT" / "READY" (IDLE im Pomodoro-Modus)

    // ===================================================================
    // Format-Strings
    // ===================================================================
    POMODORO_DURATION_FMT,    ///< "{0:%d} Min" / "{0:%d} min"
    POMODORO_STATS_SINGULAR,  ///< "Heute: {0:%d} Pomodoro" / "Today: {0:%d} pomodoro"
    POMODORO_STATS_PLURAL,    ///< "Heute: {0:%d} Pomodoros" / "Today: {0:%d} pomodoros"

    POMODORO_KEY_COUNT ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_POMODORO_H
