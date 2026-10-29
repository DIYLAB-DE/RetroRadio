#ifndef STRINGS_CONTACTS_H
#define STRINGS_CONTACTS_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte des Contacts-Moduls
 *
 * Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_contacts.cpp` erweitern. Der `static_assert` dort verhindert eine Drift zwischen Enum und Tabelle.
 *
 * Format-Platzhalter-Konvention:
 *   CONTACTS_STATS_FILTERED:   {0:%d} = sichtbar, {1:%d} = gesamt
 *   CONTACTS_STATS_TOTAL:      {0:%d} = gesamt
 *   CONTACTS_DATE_FULL_FMT:    {0:%02d} = Tag, {1:%02d} = Monat, {2:%04d} = Jahr
 *   CONTACTS_DATE_SHORT_FMT:   {0:%02d} = Tag, {1:%02d} = Monat
 */
enum class ContactsKey : uint16_t
{
    // ===================================================================
    // Ladescreen
    // ===================================================================
    CONTACTS_LOADING_TITLE = 0,     ///< "- lade kontakte -" / "- loading contacts -"
    CONTACTS_LOADING_CANCEL,        ///< "- abbruch -" / "- cancelled -"

    // ===================================================================
    // ProgressBox (Aktualisierungs-Dialog)
    // ===================================================================
    CONTACTS_PROGRESS_TITLE,        ///< "Kontakte laden" / "Loading contacts"
    CONTACTS_PROGRESS_MESSAGE,      ///< "Aktualisierung läuft..." / "Update in progress..."

    // ===================================================================
    // Action-Buttons
    // ===================================================================
    CONTACTS_BTN_SEARCH,            ///< "suchen" / "search"
    CONTACTS_BTN_RESET,             ///< "reset" / "reset"
    CONTACTS_BTN_REFRESH,           ///< "aktualisieren" / "refresh"

    // ===================================================================
    // Platzhalter-Texte (leere Liste / kein ausgewählter Kontakt)
    // ===================================================================
    CONTACTS_NO_RESULTS,            ///< "Keine Treffer" / "No matches"
    CONTACTS_NO_CONTACTS,           ///< "Keine Kontakte" / "No contacts"

    // ===================================================================
    // Detail-Labels
    // ===================================================================
    CONTACTS_LABEL_COMPANY,         ///< "Firma:" / "Company:"
    CONTACTS_LABEL_PHONE_HOME,      ///< "Privat:" / "Home:"
    CONTACTS_LABEL_PHONE_MOBILE,    ///< "Mobil:" / "Mobile:"
    CONTACTS_LABEL_PHONE_WORK,      ///< "Dienst:" / "Work:"
    CONTACTS_LABEL_EMAIL,           ///< "E-Mail:" / "Email:"
    CONTACTS_LABEL_ADDRESS,         ///< "Adresse:" / "Address:"
    CONTACTS_LABEL_BIRTHDAY,        ///< "Geburtstag:" / "Birthday:"

    // ===================================================================
    // Statistik (Format-Strings)
    // ===================================================================
    CONTACTS_STATS_FILTERED,        ///< "{0:%d} von {1:%d} Kontakten" / "{0:%d} of {1:%d} contacts"
    CONTACTS_STATS_TOTAL,           ///< "{0:%d} Kontakte" / "{0:%d} contacts"

    // ===================================================================
    // Geburtstags-Datum (Format-Strings)
    // ===================================================================
    CONTACTS_DATE_FULL_FMT,         ///< "{0:%02d}.{1:%02d}.{2:%04d}" / "{0:%02d}/{1:%02d}/{2:%04d}"
    CONTACTS_DATE_SHORT_FMT,        ///< "{0:%02d}.{1:%02d}." / "{0:%02d}/{1:%02d}"

    // ===================================================================
    // Suchfunktion
    // ===================================================================
    CONTACTS_SEARCH_PROMPT,         ///< Tastatur-Aufforderung beim Such-Dialog

    // ===================================================================
    // Reset-Bestätigungsdialog (MessageBox)
    // ===================================================================
    CONTACTS_RESET_TITLE,           ///< "Filter zurücksetzen" / "Reset filter"
    CONTACTS_RESET_MESSAGE,         ///< Suchfilter wirklich zurücksetzen?

    CONTACTS_KEY_COUNT ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_CONTACTS_H
