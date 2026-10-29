#ifndef STRINGS_SDPLAYER_H
#define STRINGS_SDPLAYER_H

#include <stdint.h>

/**
 * @brief Schlüssel für anwender-sichtbare Texte des SD-Player-Moduls
 *
 * Beim Hinzufügen eines Keys auch die Pointer-Tabelle in `Strings_sdplayer.cpp` erweitern. Der `static_assert` dort verhindert eine Drift zwischen Enum und Tabelle.
 */
enum class SDPlayerKey : uint16_t
{
    // ===================================================================
    // Knob- und Slider-Labels
    // ===================================================================
    SDPLAYER_KNOB_LABEL = 0,    ///< "Lautstärke" / "Volume"
    SDPLAYER_SLIDER_BALANCE,    ///< "Balance" / "Balance"
    SDPLAYER_SLIDER_LOW,        ///< "Tiefen" / "Bass"
    SDPLAYER_SLIDER_MID,        ///< "Mitten" / "Mid"
    SDPLAYER_SLIDER_HIGH,       ///< "Höhen" / "Treble"

    // ===================================================================
    // Statuszeile und Playlist-Platzhalter
    // ===================================================================
    SDPLAYER_PLAYLIST_EMPTY,      ///< "Keine Titel" / "No tracks"
    SDPLAYER_PLAYLIST_HINT_1,     ///< "Tippe auf 'durchs.' und" / "Tap 'browse' and pick"
    SDPLAYER_PLAYLIST_HINT_2,     ///< "wähle Ordner oder Titel" / "folders or tracks"
    SDPLAYER_STATUS_NO_PLAYBACK,  ///< "keine Wiedergabe" / "no playback"
    SDPLAYER_STATUS_PLAYLIST_END, ///< "Playlist beendet" / "Playlist ended"

    // ===================================================================
    // Cover-Fallback und Album-Platzhalter
    // ===================================================================
    SDPLAYER_COVER_NOCOVER, ///< "Kein Cover" / "No cover"
    SDPLAYER_ALBUM_UNKNOWN, ///< "Album unbekannt" / "Unknown album"

    // ===================================================================
    // Control-Buttons (Browse und Clear sind übersetzbar)
    // ===================================================================
    SDPLAYER_BTN_BROWSE, ///< "durchs." / "browse"
    SDPLAYER_BTN_CLEAR,  ///< "leeren" / "clear"

    // ===================================================================
    // Format-Strings (Platzhalter siehe Doku am jeweiligen Key)
    // ===================================================================
    SDPLAYER_TRACK_INFO_FORMAT, ///< "Track: {0:%d}/{1:%d}" - {0}=aktueller Track, {1}=Gesamtanzahl
    SDPLAYER_SD_CAPACITY_FORMAT,          ///< "SD: {0:%d},{1:%d} GB frei von {2:%d},{3:%d} GB" - Ganzzahl- und Zehntelteile, Dezimaltrennzeichen je Sprache
    SDPLAYER_SD_NONE,                     ///< "Keine SD-Karte" / "No SD card"
    SDPLAYER_PLAYLIST_SUMMARY_FORMAT,     ///< "{0:%d} Titel | {1}" - {0}=Anzahl, {1}=Ordnername
    SDPLAYER_PLAYLIST_SUMMARY_ONE_FORMAT, ///< "1 Titel | {0}" - {0}=Ordnername
    SDPLAYER_HINT_ADD_TRACKS,             ///< "Titel mit 'durchs.' hinzufügen" / "Add tracks with 'browse'"

    // ===================================================================
    // MessageBox-Texte
    // ===================================================================
    SDPLAYER_MSG_CLEAR_TITLE,   ///< "LISTE LEEREN?" / "CLEAR LIST?"
    SDPLAYER_MSG_CLEAR_MESSAGE, ///< "Die gesamte Abspielliste wird gelöscht." / "The whole playlist will be removed."
    SDPLAYER_MSG_RESET_TITLE,   ///< "ZURÜCKSETZEN?" / "RESET?"
    SDPLAYER_MSG_RESET_MESSAGE, ///< Reset-Bestätigungstext

    // ===================================================================
    // SleepTimer-Texte
    // ===================================================================
    SDPLAYER_SLEEP_TITLE,   ///< "AUSSCHALTEN" / "TURN OFF"
    SDPLAYER_SLEEP_MESSAGE, ///< "Player verlassen nach:" / "Quit player after:"

    // ===================================================================
    // FileExplorer-Texte (Sub-Komponente des SDPlayer-Moduls)
    // ===================================================================
    FILEEXPLORER_TITLE,                    ///< "Musikdatei(en) auswählen:" / "Select music file(s):"
    FILEEXPLORER_COL_NAME,                 ///< "Dateiname" / "File name"
    FILEEXPLORER_COL_TYPE,                 ///< "Typ" / "Type"
    FILEEXPLORER_COL_SIZE,                 ///< "Größe" / "Size"
    FILEEXPLORER_EMPTY,                    ///< "Verzeichnis ist leer" / "Directory is empty"
    FILEEXPLORER_STATUS_OVERVIEW,          ///< "{0:%d} Dateien, {1:%d} Ordner ges." - {0}=Dateien, {1}=Ordner
    FILEEXPLORER_STATUS_FOLDERS_SELECTED,  ///< "{0:%d} Ordner ausgewählt" - {0}=Anzahl
    FILEEXPLORER_STATUS_FILES_SELECTED,    ///< "{0} in {1:%d} Dateien ausgew." - {0}=Größentext, {1}=Anzahl
    FILEEXPLORER_STATUS_NOTHING_SELECTED,  ///< "- nichts ausgewählt -" / "- nothing selected -"

    SDPLAYER_KEY_COUNT ///< Endmarker für Sanity-Checks
};

#endif // STRINGS_SDPLAYER_H
