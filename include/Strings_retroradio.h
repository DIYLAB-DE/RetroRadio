#ifndef STRINGS_RETRORADIO_H
#define STRINGS_RETRORADIO_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte im RetroRadio-Modul
 *
 * Enthält Dialog-Titel, Dialog-Nachrichten und Control-Labels des Radio-Hauptscreens sowie die Labels der Visualizer-Sub-Komponente (Goniometer-Info). Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_retroradio.cpp` erweitern. Der `static_assert` dort verhindert eine Drift.
 *
 * Sub-Komponenten-Präfixe:
 *   RETRO_      Hauptscreen (Dialoge, Knob, Equalizer)
 *   RETRO_VIS_  Visualizer (Goniometer-Info-Sprite)
 */
enum class RetroRadioKey : uint16_t
{
    // ------------------------------------------------------------------------
    // Dialog-Texte (Hauptscreen)
    // ------------------------------------------------------------------------
    RETRO_PRESET_DLG_TITLE,    ///< "FAVORITEN" / "FAVOURITES"
    RETRO_RESET_DLG_TITLE,     ///< "ZURÜCKSETZEN?" / "RESET?"
    RETRO_RESET_DLG_MESSAGE,   ///< Reset-Bestätigungs-Text (Lautstärke, Ton, Visualisierungen)
    RETRO_SLEEP_DLG_TITLE,     ///< "AUSSCHALTEN" / "SWITCH OFF"
    RETRO_SLEEP_DLG_MESSAGE,   ///< "Radio ausschalten nach:" / "Switch off radio after:"

    // ------------------------------------------------------------------------
    // Control-Labels (Hauptscreen)
    // ------------------------------------------------------------------------
    RETRO_KNOB_LABEL,          ///< "Lautstärke" / "Volume"
    RETRO_TRACK_BALANCE,       ///< "Balance" / "Balance"
    RETRO_TRACK_LOW,           ///< "Tiefen" / "Bass"
    RETRO_TRACK_MID,           ///< "Mitten" / "Mid"
    RETRO_TRACK_HIGH,          ///< "Höhen" / "Treble"

    // ------------------------------------------------------------------------
    // Visualizer (Goniometer-Info-Sprite)
    // ------------------------------------------------------------------------
    RETRO_VIS_GAIN_LABEL,      ///< "Verstärkungsfaktor" / "Gain factor"
    RETRO_VIS_GONIO_LEFT,      ///< "links" / "left"
    RETRO_VIS_GONIO_RIGHT,     ///< "rechts" / "right"

    RETRORADIO_KEY_COUNT       ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_RETRORADIO_H
