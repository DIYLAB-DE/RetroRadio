#ifndef STRINGS_BREATHER_H
#define STRINGS_BREATHER_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte des Breather-Moduls
 *
 * Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_breather.cpp` erweitern. Der `static_assert` dort verhindert eine Drift zwischen Enum und Tabelle.
 *
 * Sitzungs- und Übungstexte liegen als parallele Tabellen `SESSIONS_DE`/`SESSIONS_EN` und `EX_*_DE`/`EX_*_EN` im anonymen Namespace von `Breather.cpp`.
 */
enum class BreatherKey : uint16_t
{
    // ===================================================================
    // Auswahl-Bildschirm
    // ===================================================================
    BREATHER_HEADER_TITLE = 0,    ///< "AUSZEIT" / "TIME OUT"
    BREATHER_HEADER_SUBTITLE,     ///< "Was tut Dir gerade gut?" / "What do you need right now?"

    // ===================================================================
    // Button-Beschriftungen
    // ===================================================================
    BREATHER_BTN_BACK,            ///< "zurück" / "back"
    BREATHER_BTN_PAUSE,           ///< "pause" / "pause"
    BREATHER_BTN_RESUME,          ///< "weiter" (Pause aufheben) / "resume"
    BREATHER_BTN_DONE,            ///< "fertig" / "done"

    // ===================================================================
    // Statische Labels in laufender Sitzung und Visualisierung
    // ===================================================================
    BREATHER_REMAINING_LABEL,     ///< "Verbleibend:" / "Remaining:"
    BREATHER_EYE_CLOSED,          ///< Augen-Palming-Symbol: "Augen zu" / "close eyes"
    BREATHER_PHASE_INHALE,        ///< Atem-Phase: "Einatmen" / "Inhale"
    BREATHER_PHASE_HOLD,          ///< Atem-Phase: "Halten" / "Hold"
    BREATHER_PHASE_EXHALE,        ///< Atem-Phase: "Ausatmen" / "Exhale"
    BREATHER_EYE_FAR,             ///< Augenwechsel Fern: "fern" / "far"
    BREATHER_EYE_NEAR,            ///< Augenwechsel Nah: "nah" / "near"
    BREATHER_AXIS_HEAD,           ///< Pendulum-Achse Nacken: "Kopf" / "Head"
    BREATHER_AXIS_TWIST,          ///< Pendulum-Achse Wirbelsäule: "Drehung" / "Twist"

    // ===================================================================
    // Done-Bildschirm
    // ===================================================================
    BREATHER_DONE_TITLE,          ///< "Geschafft." / "Well done."
    BREATHER_DONE_HINT,           ///< "So geht es gut weiter." / "Carry on from there."

    // ===================================================================
    // Format-Strings
    // ===================================================================
    BREATHER_DURATION_MIN_INT_FMT, ///< "{0:%u} Min." / "{0:%u} min" (Auswahl-Kachel, glatte Minuten)
    BREATHER_DURATION_MIN_STR_FMT, ///< "{0} Min." / "{0} min" (Auswahl-Kachel, M:SS vorformatiert)
    BREATHER_EXERCISE_X_OF_Y_FMT,  ///< "Übung {0:%u} von {1:%u}" / "Exercise {0:%u} of {1:%u}"
    BREATHER_DONE_DURATION_FMT,    ///< "{0} genommen" / "Took {0}" (Done-Bildschirm)

    BREATHER_KEY_COUNT ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_BREATHER_H
