#ifndef STRINGS_OFF_H
#define STRINGS_OFF_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte des Off-Moduls
 *
 * Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_off.cpp` erweitern. Der `static_assert` dort verhindert eine Drift zwischen Enum und Tabelle.
 */
enum class OffKey : uint16_t
{
    OFF_HINT = 0,     ///< Projekt-Hinweis, wird mehrzeilig umgebrochen
    OFF_LINK_LABEL,   ///< "Projektseite:" / "Project page:"
    OFF_BTN_BACK,     ///< "zurück" / "back"
    OFF_BTN_SHUTDOWN, ///< "ausschalten" / "switch off"

    OFF_KEY_COUNT ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_OFF_H
