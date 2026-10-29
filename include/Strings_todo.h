#ifndef STRINGS_TODO_H
#define STRINGS_TODO_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte des ToDo-Moduls
 *
 * Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_todo.cpp` erweitern. Der `static_assert` dort verhindert eine Drift zwischen Enum und Tabelle.
 */
enum class ToDoKey : uint16_t
{
    // ===================================================================
    // Ladescreen
    // ===================================================================
    TODO_LOADING = 0,           ///< "- lade aufgaben -" / "- loading tasks -"
    TODO_LOADING_CANCEL,        ///< "- abbruch -" / "- cancel -"

    // ===================================================================
    // Buttons
    // ===================================================================
    TODO_BUTTON_REFRESH,        ///< "aktualisieren" / "refresh"

    // ===================================================================
    // Stats-Panel
    // ===================================================================
    TODO_STATS_TITLE,           ///< "Aufgaben" / "Tasks"
    TODO_STATS_TOTAL,           ///< "Gesamt:" / "Total:"
    TODO_STATS_OPEN,            ///< "Offen:" / "Open:"
    TODO_STATS_TODAY,           ///< "Heute:" / "Today:"
    TODO_STATS_OVERDUE,         ///< "Überfällig:" / "Overdue:"

    // ===================================================================
    // Toggle-Switch und Platzhalter
    // ===================================================================
    TODO_TOGGLE_LABEL,          ///< "Erledigte" / "Completed"
    TODO_EMPTY_LIST,            ///< "Keine Aufgaben vorhanden" / "No tasks available"

    // ===================================================================
    // Backend-Default (Default-Listenname bei fehlendem JSON-Feld)
    // ===================================================================
    TODO_LIST_DEFAULT_NAME,     ///< "Aufgaben" / "Tasks"

    // ===================================================================
    // ProgressBox-Texte
    // ===================================================================
    TODO_PROGRESS_TITLE,        ///< "Aktualisierung" / "Refresh"
    TODO_PROGRESS_MESSAGE,      ///< "Aktualisierung läuft..." / "Refresh in progress..."
    TODO_PROGRESS_MANUAL_MSG,   ///< "Aktualisierung wird ausgeführt..." / "Performing refresh..."

    // ===================================================================
    // MessageBox-Texte
    // ===================================================================
    TODO_DLG_TITLE_COMPLETE,    ///< "Aufgabe erledigt?" / "Task completed?"
    TODO_DLG_TITLE_REOPEN,      ///< "Aufgabe wieder öffnen?" / "Reopen task?"
    TODO_DLG_LOADING,           ///< "Wird aktualisiert..." / "Updating..."

    TODO_KEY_COUNT              ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_TODO_H
