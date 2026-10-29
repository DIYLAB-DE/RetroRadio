#include "Strings_sdplayer.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    // Knob- und Slider-Labels
    const char DE_KNOB_LABEL[]    PROGMEM = "Lautstärke";
    const char DE_SLIDER_BALANCE[] PROGMEM = "Balance";
    const char DE_SLIDER_LOW[]    PROGMEM = "Tiefen";
    const char DE_SLIDER_MID[]    PROGMEM = "Mitten";
    const char DE_SLIDER_HIGH[]   PROGMEM = "Höhen";

    // Statuszeile und Playlist-Platzhalter
    const char DE_PLAYLIST_EMPTY[]      PROGMEM = "Keine Titel";
    const char DE_PLAYLIST_HINT_1[]     PROGMEM = "Tippe auf 'durchs.' und";
    const char DE_PLAYLIST_HINT_2[]     PROGMEM = "wähle Ordner oder Titel";
    const char DE_STATUS_NO_PLAYBACK[]  PROGMEM = "keine Wiedergabe";
    const char DE_STATUS_PLAYLIST_END[] PROGMEM = "Playlist beendet";

    // Cover- und Album-Platzhalter
    const char DE_COVER_NOCOVER[] PROGMEM = "Kein Cover";
    const char DE_ALBUM_UNKNOWN[] PROGMEM = "Album unbekannt";

    // Control-Buttons (übersetzbar)
    const char DE_BTN_BROWSE[] PROGMEM = "durchs.";
    const char DE_BTN_CLEAR[]  PROGMEM = "leeren";

    // Format-Strings
    const char DE_TRACK_INFO_FORMAT[] PROGMEM = "Track: {0:%d}/{1:%d}";
    const char DE_SD_CAPACITY_FORMAT[] PROGMEM = "SD: {0:%d},{1:%d} GB frei von {2:%d},{3:%d} GB";
    const char DE_SD_NONE[] PROGMEM = "Keine SD-Karte";
    const char DE_PLAYLIST_SUMMARY_FORMAT[] PROGMEM = "{0:%d} Titel | {1}";
    const char DE_PLAYLIST_SUMMARY_ONE_FORMAT[] PROGMEM = "1 Titel | {0}";
    const char DE_HINT_ADD_TRACKS[] PROGMEM = "Titel mit 'durchs.' hinzufügen";

    // MessageBox-Texte
    const char DE_MSG_CLEAR_TITLE[]   PROGMEM = "LISTE LEEREN?";
    const char DE_MSG_CLEAR_MESSAGE[] PROGMEM = "Die gesamte Abspielliste wird gelöscht.";
    const char DE_MSG_RESET_TITLE[]   PROGMEM = "ZURÜCKSETZEN?";
    const char DE_MSG_RESET_MESSAGE[] PROGMEM = "Die Lautstärke, der Ton und die Visualisierungen werden zurückgesetzt.";

    // SleepTimer-Texte
    const char DE_SLEEP_TITLE[]   PROGMEM = "AUSSCHALTEN";
    const char DE_SLEEP_MESSAGE[] PROGMEM = "Player verlassen nach:";

    // FileExplorer-Texte (Sub-Komponente)
    const char DE_FE_TITLE[]                   PROGMEM = "Musikdatei(en) auswählen:";
    const char DE_FE_COL_NAME[]                PROGMEM = "Dateiname";
    const char DE_FE_COL_TYPE[]                PROGMEM = "Typ";
    const char DE_FE_COL_SIZE[]                PROGMEM = "Größe";
    const char DE_FE_EMPTY[]                   PROGMEM = "Verzeichnis ist leer";
    const char DE_FE_STATUS_OVERVIEW[]         PROGMEM = "{0:%d} Dateien, {1:%d} Ordner ges.";
    const char DE_FE_STATUS_FOLDERS_SELECTED[] PROGMEM = "{0:%d} Ordner ausgewählt";
    const char DE_FE_STATUS_FILES_SELECTED[]   PROGMEM = "{0} in {1:%d} Dateien ausgew.";
    const char DE_FE_STATUS_NOTHING_SELECTED[] PROGMEM = "- nichts ausgewählt -";

    // ===================================================================
    // PROGMEM-Literale (Englisch, British)
    // ===================================================================

    const char EN_KNOB_LABEL[]    PROGMEM = "Volume";
    const char EN_SLIDER_BALANCE[] PROGMEM = "Balance";
    const char EN_SLIDER_LOW[]    PROGMEM = "Bass";
    const char EN_SLIDER_MID[]    PROGMEM = "Mid";
    const char EN_SLIDER_HIGH[]   PROGMEM = "Treble";

    const char EN_PLAYLIST_EMPTY[]      PROGMEM = "No tracks";
    const char EN_PLAYLIST_HINT_1[]     PROGMEM = "Tap 'browse' and pick";
    const char EN_PLAYLIST_HINT_2[]     PROGMEM = "folders or tracks";
    const char EN_STATUS_NO_PLAYBACK[]  PROGMEM = "no playback";
    const char EN_STATUS_PLAYLIST_END[] PROGMEM = "Playlist ended";

    const char EN_COVER_NOCOVER[] PROGMEM = "No cover";
    const char EN_ALBUM_UNKNOWN[] PROGMEM = "Unknown album";

    const char EN_BTN_BROWSE[] PROGMEM = "browse";
    const char EN_BTN_CLEAR[]  PROGMEM = "clear";

    const char EN_TRACK_INFO_FORMAT[] PROGMEM = "Track: {0:%d}/{1:%d}";
    const char EN_SD_CAPACITY_FORMAT[] PROGMEM = "SD: {0:%d}.{1:%d} GB free of {2:%d}.{3:%d} GB";
    const char EN_SD_NONE[] PROGMEM = "No SD card";
    const char EN_PLAYLIST_SUMMARY_FORMAT[] PROGMEM = "{0:%d} tracks | {1}";
    const char EN_PLAYLIST_SUMMARY_ONE_FORMAT[] PROGMEM = "1 track | {0}";
    const char EN_HINT_ADD_TRACKS[] PROGMEM = "Add tracks with 'browse'";

    const char EN_MSG_CLEAR_TITLE[]   PROGMEM = "CLEAR LIST?";
    const char EN_MSG_CLEAR_MESSAGE[] PROGMEM = "The whole playlist will be removed.";
    const char EN_MSG_RESET_TITLE[]   PROGMEM = "RESET?";
    const char EN_MSG_RESET_MESSAGE[] PROGMEM = "Volume, audio and visualisations will be reset.";

    const char EN_SLEEP_TITLE[]   PROGMEM = "TURN OFF";
    const char EN_SLEEP_MESSAGE[] PROGMEM = "Quit player after:";

    // FileExplorer-Texte (Sub-Komponente)
    const char EN_FE_TITLE[]                   PROGMEM = "Select music file(s):";
    const char EN_FE_COL_NAME[]                PROGMEM = "File name";
    const char EN_FE_COL_TYPE[]                PROGMEM = "Type";
    const char EN_FE_COL_SIZE[]                PROGMEM = "Size";
    const char EN_FE_EMPTY[]                   PROGMEM = "Directory is empty";
    const char EN_FE_STATUS_OVERVIEW[]         PROGMEM = "{0:%d} files, {1:%d} folders total";
    const char EN_FE_STATUS_FOLDERS_SELECTED[] PROGMEM = "{0:%d} folders selected";
    const char EN_FE_STATUS_FILES_SELECTED[]   PROGMEM = "{0} in {1:%d} files selected";
    const char EN_FE_STATUS_NOTHING_SELECTED[] PROGMEM = "- nothing selected -";

    // ===================================================================
    // Pointer-Tabelle [Key][Sprache]
    // ===================================================================

    PGM_P const SDPLAYER_TABLE[static_cast<size_t>(SDPlayerKey::SDPLAYER_KEY_COUNT)]
                              [static_cast<size_t>(LangIndex::COUNT)] PROGMEM =
    {
        // Knob- und Slider-Labels
        { DE_KNOB_LABEL,     EN_KNOB_LABEL     },
        { DE_SLIDER_BALANCE, EN_SLIDER_BALANCE },
        { DE_SLIDER_LOW,     EN_SLIDER_LOW     },
        { DE_SLIDER_MID,     EN_SLIDER_MID     },
        { DE_SLIDER_HIGH,    EN_SLIDER_HIGH    },

        // Statuszeile und Playlist-Platzhalter
        { DE_PLAYLIST_EMPTY,      EN_PLAYLIST_EMPTY      },
        { DE_PLAYLIST_HINT_1,     EN_PLAYLIST_HINT_1     },
        { DE_PLAYLIST_HINT_2,     EN_PLAYLIST_HINT_2     },
        { DE_STATUS_NO_PLAYBACK,  EN_STATUS_NO_PLAYBACK  },
        { DE_STATUS_PLAYLIST_END, EN_STATUS_PLAYLIST_END },

        // Cover- und Album-Platzhalter
        { DE_COVER_NOCOVER, EN_COVER_NOCOVER },
        { DE_ALBUM_UNKNOWN, EN_ALBUM_UNKNOWN },

        // Control-Buttons
        { DE_BTN_BROWSE, EN_BTN_BROWSE },
        { DE_BTN_CLEAR,  EN_BTN_CLEAR  },

        // Format-Strings
        { DE_TRACK_INFO_FORMAT, EN_TRACK_INFO_FORMAT },
        { DE_SD_CAPACITY_FORMAT, EN_SD_CAPACITY_FORMAT },
        { DE_SD_NONE, EN_SD_NONE },
        { DE_PLAYLIST_SUMMARY_FORMAT, EN_PLAYLIST_SUMMARY_FORMAT },
        { DE_PLAYLIST_SUMMARY_ONE_FORMAT, EN_PLAYLIST_SUMMARY_ONE_FORMAT },
        { DE_HINT_ADD_TRACKS, EN_HINT_ADD_TRACKS },

        // MessageBox-Texte
        { DE_MSG_CLEAR_TITLE,   EN_MSG_CLEAR_TITLE   },
        { DE_MSG_CLEAR_MESSAGE, EN_MSG_CLEAR_MESSAGE },
        { DE_MSG_RESET_TITLE,   EN_MSG_RESET_TITLE   },
        { DE_MSG_RESET_MESSAGE, EN_MSG_RESET_MESSAGE },

        // SleepTimer-Texte
        { DE_SLEEP_TITLE,   EN_SLEEP_TITLE   },
        { DE_SLEEP_MESSAGE, EN_SLEEP_MESSAGE },

        // FileExplorer-Texte
        { DE_FE_TITLE,                   EN_FE_TITLE                   },
        { DE_FE_COL_NAME,                EN_FE_COL_NAME                },
        { DE_FE_COL_TYPE,                EN_FE_COL_TYPE                },
        { DE_FE_COL_SIZE,                EN_FE_COL_SIZE                },
        { DE_FE_EMPTY,                   EN_FE_EMPTY                   },
        { DE_FE_STATUS_OVERVIEW,         EN_FE_STATUS_OVERVIEW         },
        { DE_FE_STATUS_FOLDERS_SELECTED, EN_FE_STATUS_FOLDERS_SELECTED },
        { DE_FE_STATUS_FILES_SELECTED,   EN_FE_STATUS_FILES_SELECTED   },
        { DE_FE_STATUS_NOTHING_SELECTED, EN_FE_STATUS_NOTHING_SELECTED }
    };

    static_assert(sizeof(SDPLAYER_TABLE) / sizeof(SDPLAYER_TABLE[0])
                  == static_cast<size_t>(SDPlayerKey::SDPLAYER_KEY_COUNT),
                  "SDPLAYER_TABLE / SDPlayerKey out of sync");

    // Fallback für den unwahrscheinlichen Fall eines fehlenden Eintrags
    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten SD-Player-Text für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getSDPlayer(SDPlayerKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(SDPlayerKey::SDPLAYER_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&SDPLAYER_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&SDPLAYER_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}
