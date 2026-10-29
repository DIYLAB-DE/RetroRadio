#include "DayView.h"

#include <ArduinoJson.h> // für JSON-Verarbeitung
#include <cmath>         // für std::ceil
#include <time.h>        // für time_t, struct tm, localtime_r

#include <cstring> // für std::strncpy, std::strlen
#include <cstdlib> // für std::strtol, std::strtof
#include <cctype>  // für std::isdigit, std::isspace

#include <algorithm>       // für std::min, std::max, std::clamp
#include <esp_heap_caps.h> // für heap_caps_malloc, MALLOC_CAP_SPIRAM
#include <type_traits>     // für std::is_same_v, std::enable_if_t

#include "AppState.h"         // zentraler Plexus für App-Zustand und -Daten
#include "Audio.h"            // für die Audio-Klasse (isRunning, Wiedergabesteuerung)
#include "AudioManager.h"     // für den globalen AudioManager (Event-Ring, Metadaten)
#include "HttpService.h"      // für HttpService (API-Aufrufe, z.B. Kalender, Wetter, News)
#include "I18n.h"             // für I18n Internationalisierung
#include "StationManager.h"   // für StationManager (Verwaltung der Radio-Senderliste)
#include "Strings_dayview.h"  // für Strings_dayview (z.B. Fehlermeldungen, Statusmeldungen)
#include "Strings_sdplayer.h" // für Strings_sdplayer (z.B. Fehlermeldungen, Statusmeldungen)
#include "StringUtils.h"      // für StringUtils (Utility-Funktionen für Strings)
#include "Theme.h"            // für Theme (Farbdefinitionen, MainScreenColors-Struktur, etc.)

// Fonts
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"

// ===================================================================
// Konstanten
// ===================================================================

namespace
{
    // Fehler-Codes
    constexpr int ERR_SHUTDOWN = -100;       ///< Interner Fehler: Task soll herunterfahren (kein echter Fehler, sondern Steuerungssignal)
    constexpr int ERR_JSON_PARSE = -101;     ///< Fehler beim Parsen der JSON-Antwort (ungültiges Format)
    constexpr int ERR_EMPTY_RESPONSE = -102; ///< Leere Antwort vom Server (kein JSON, Verbindungsfehler, etc.)
    constexpr int ERR_NO_URL = -103;         ///< Keine URL konfiguriert (z.B. kein Kalender-Script angegeben)
    constexpr int ERR_SCHEMA = -104;         ///< Unerwartetes JSON-Schema (erwartete Felder fehlen oder falscher Typ)

    // Display-Dimensionen
    constexpr int DISPLAY_W = 800; ///< Display-Breite
    constexpr int DISPLAY_H = 480; ///< Display-Höhe

    // Exit-Button
    constexpr int16_t EXIT_BTN_X = 765; ///< X-Mitte des Power-Buttons
    constexpr int16_t EXIT_BTN_Y = 457; ///< Y-Mitte des Power-Buttons
    constexpr uint16_t BTN_SIZE = 45;   ///< Quadratischer Button
    constexpr uint16_t BTN_RADIUS = 4;  ///< Abgerundete Ecken
    constexpr int BTN_COLOR = 0x39E8;   ///< Farbe für Button-Rahmen

    // Layout-Bereiche
    constexpr int LEFT_MARGIN = 10; ///< Linker Rand für alle Bereiche
    constexpr int TOP_MARGIN = 10;  ///< Oberer Rand für den Header-Bereich
    constexpr int COL_GAP = 8;      ///< Abstand zwischen linker und rechter Spalte
    constexpr int ROW_GAP = 8;      ///< Vertikaler Abstand zwischen Sektionen

    // Oberer Bereich: Wetter + Uhr
    constexpr int HEADER_Y = TOP_MARGIN; ///< Y-Position des Header-Bereichs
    constexpr int HEADER_H = 130;        ///< Höhe des Header-Bereichs (Uhr + Wetter)

    // Wetter-Icon
    constexpr int ICON_SIZE = 180; ///< Maximalgröße des Wetter-Icons (quadratisch, wird ggf. skaliert)

    // Header-Sprite
    constexpr int HEADER_SPR_W = DISPLAY_W - 2 * LEFT_MARGIN; ///< Header-Sprite ist so breit wie der verfügbare Platz zwischen den Rändern (780px)
    constexpr int HEADER_SPR_H = HEADER_H;                    ///< Header-Sprite ist so hoch wie der Header-Bereich (130px)
    constexpr int ICON_SPR_X = HEADER_SPR_W - ICON_SIZE;      ///< Icon rechtsbündig im Header-Sprite
    constexpr int ICON_SPR_Y = -5;                            ///< Leicht nach oben versetzt, damit es optisch mit der Uhr auf einer Linie ist
    constexpr int CLOCK_GAP = 10;                             ///< Abstand zwischen Wetter-Icon und Uhrzeit

    // Header-DotMatrix (Uhrzeit + Temperatur)
    constexpr int HDR_DOT_W = 5;                                        ///< DotMatrix Dot-Breite
    constexpr int HDR_DOT_H = 6;                                        ///< DotMatrix Dot-Höhe
    constexpr int HDR_DOT_SPACE = 1;                                    ///< Dot-Abstand
    constexpr int HDR_CHAR_SPACE = 3;                                   ///< Zeichen-Abstand
    constexpr int HDR_CHAR_W = 5 * HDR_DOT_W + 4 * HDR_DOT_SPACE;       ///< Zeichen-Breite (5 Dots + 4 Spaces)
    constexpr int HDR_DISP_H = 7 * HDR_DOT_H + 6 * HDR_DOT_SPACE;       ///< Display-Höhe (7 Dots + 6 Spaces)
    constexpr int HDR_CLOCK_Y = 5;                                      ///< Y-Position Uhrzeit im Header-Sprite
    constexpr int HDR_LINE_GAP = 10;                                    ///< Abstand zwischen Uhrzeit und Temperatur
    constexpr int HDR_TEMP_Y = HDR_CLOCK_Y + HDR_DISP_H + HDR_LINE_GAP; ///< Y-Position Temperatur
    constexpr uint16_t HDR_DOT_ON = 0xC618;                             ///< Leuchtende Dots
    constexpr uint16_t HDR_DOT_OFF = 0x2104;                            ///< Dunkle Dots
    constexpr int DETAIL_COL_GAP = 130;                                 ///< Abstand Detail-Spalte 1 -> Spalte 2
    constexpr int DETAIL_SEP_PAD = 10;                                  ///< Abstand Trenner -> Spalte 2
    constexpr uint16_t COLOR_DETAIL_SEP = 0x5345;                       ///< Farbe für vertikalen Detail-Trenner

    // Mittlerer Bereich: Events links, Tasks rechts
    constexpr int CONTENT_Y = HEADER_Y + HEADER_H;                     ///< Y-Position des Content-Bereichs (direkt unter dem Header)
    constexpr int COL_LEFT_X = LEFT_MARGIN;                            ///< X-Position der linken Spalte für Events
    constexpr int COL_LEFT_W = 380;                                    ///< Breite der linken Spalte (fester Wert, damit die rechte Spalte flexibel den Rest der Breite einnimmt)
    constexpr int COL_RIGHT_X = COL_LEFT_X + COL_LEFT_W + COL_GAP;     ///< X-Position der rechten Spalte für Tasks (direkt nach linker Spalte + Abstand)
    constexpr int COL_RIGHT_W = DISPLAY_W - COL_RIGHT_X - LEFT_MARGIN; ///< Breite der rechten Spalte (Restbreite zwischen rechter Spalte und rechtem Rand)

    // Info-Bar (Radio/SD-Player/News-Zeile zwischen Content und Radio-Buttons)
    constexpr int BOTTOM_H = 30;                        ///< Höhe der Info-Bar am unteren Rand
    constexpr int BOTTOM_Y = 427 - BOTTOM_H;            ///< Y-Position der Info-Bar am unteren Rand
    constexpr int CONTENT_H = BOTTOM_Y - 4 - CONTENT_Y; ///< Höhe des Content-Bereichs

    // Sprite-Dimensionen
    constexpr int EVENTS_SPR_W = COL_LEFT_W;                           ///< Breite des Events-Sprites
    constexpr int EVENTS_SPR_H = CONTENT_H;                            ///< Höhe des Events-Sprites
    constexpr int TASKS_DIV_X = COL_LEFT_X + COL_LEFT_W + COL_GAP / 2; ///< X-Position der vertikalen Trennlinie zwischen Events und Tasks
    constexpr int TASKS_SPR_X = TASKS_DIV_X + 1;                       ///< X-Position des Tasks-Sprites
    constexpr int TASKS_SPR_W = DISPLAY_W - LEFT_MARGIN - TASKS_SPR_X; ///< Breite des Tasks-Sprites
    constexpr int TASKS_SPR_H = CONTENT_H;                             ///< Höhe des Tasks-Sprites
    constexpr int BOTTOM_SPR_W = DISPLAY_W - 2 * LEFT_MARGIN;          ///< Breite des Bottom-Sprites (Info-Bar)
    constexpr int BOTTOM_SPR_H = BOTTOM_H;                             ///< Höhe des Bottom-Sprites (Info-Bar)

    // QR-Code Overlay
    constexpr int QR_BOX_SIZE = 180;                            ///< Größe des quadratischen QR-Code-Overlays
    constexpr int QR_X = DISPLAY_W - LEFT_MARGIN - QR_BOX_SIZE; ///< X-Position des QR-Code-Overlays
    constexpr int QR_Y = BOTTOM_Y - QR_BOX_SIZE - 4;            ///< Y-Position des QR-Code-Overlays

    // Touch-Hitbox der News-Zeile
    constexpr int NEWS_TOUCH_Y1 = BOTTOM_Y;            ///< Y-Position der News-Zeile (oberer Rand)
    constexpr int NEWS_TOUCH_Y2 = BOTTOM_Y + BOTTOM_H; ///< Y-Position der News-Zeile (unterer Rand)

    // Radio-Bar
    constexpr int RADIO_Y = BOTTOM_Y + BOTTOM_H + 4; ///< Oberkante für Trennlinie/Clearing
    constexpr int RADIO_H = DISPLAY_H - RADIO_Y;     ///< Resthöhe für Radio-Bar

    // Radio-Buttons
    constexpr int RADIO_BTN_SIZE = 45;                                         ///< Quadratische Buttons
    constexpr int RADIO_BTN_R = 4;                                             ///< Abgerundete Ecken
    constexpr int RADIO_BTN_GAP = 10;                                          ///< Abstand zwischen den Buttons
    constexpr int RADIO_BTN_Y = EXIT_BTN_Y;                                    ///< Y-Mitte = Exit-Button
    constexpr int RADIO_LEFT = 10;                                             ///< Linker Rand 10px
    constexpr int RADIO_FAV_X = RADIO_LEFT + RADIO_BTN_SIZE / 2;               ///< Favoriten-Mitte
    constexpr int RADIO_PREV_X = RADIO_FAV_X + RADIO_BTN_SIZE + RADIO_BTN_GAP; ///< Prev-Mitte

    // Sender-Index DotMatrix Display
    constexpr int STN_DOT_W = 3;                                                   ///< DotMatrix Dot-Breite
    constexpr int STN_DOT_H = 3;                                                   ///< DotMatrix Dot-Höhe
    constexpr int STN_DOT_SPACE = 1;                                               ///< Dot-Abstand
    constexpr int STN_CHAR_SPACE = 5;                                              ///< Zeichen-Abstand
    constexpr int STN_CHAR_W = 5 * STN_DOT_W + 4 * STN_DOT_SPACE;                  ///< Zeichen-Breite
    constexpr int STN_DISP_W = 2 * STN_CHAR_W + STN_CHAR_SPACE;                    ///< Breite für 2 Zeichen + Abstand (z.B. "01"-"99")
    constexpr int STN_DISP_H = 7 * STN_DOT_H + 6 * STN_DOT_SPACE;                  ///< Display-Höhe
    constexpr int STN_FRAME_PAD = 8;                                               ///< Innenabstand Rahmen
    constexpr int STN_FRAME_W = STN_DISP_W + 2 * STN_FRAME_PAD;                    ///< Rahmenbreite
    constexpr int STN_FRAME_H = RADIO_BTN_SIZE;                                    ///< Rahmenhöhe
    constexpr int STN_FRAME_X = RADIO_PREV_X + RADIO_BTN_SIZE / 2 + RADIO_BTN_GAP; ///< Rahmen linke Kante
    constexpr int STN_DISP_X = STN_FRAME_X + STN_FRAME_PAD;                        ///< DotMatrix linke Kante

    constexpr int RADIO_PLAY_X = STN_FRAME_X + STN_FRAME_W + RADIO_BTN_GAP + RADIO_BTN_SIZE / 2; ///< Play/Pause-Mitte
    constexpr int RADIO_NEXT_X = RADIO_PLAY_X + RADIO_BTN_SIZE + RADIO_BTN_GAP;                  ///< Next-Mitte

    // Radio-Button Repeat
    constexpr unsigned long RADIO_REPEAT_DELAY_MS = 800;    ///< Verzögerung vor Repeat-Start
    constexpr unsigned long RADIO_REPEAT_INTERVAL_MS = 300; ///< Intervall zwischen Repeat-Schritten

    // Lautstärke-Sektion
    constexpr int VOL_DOWN_X = RADIO_NEXT_X + RADIO_BTN_SIZE + RADIO_BTN_GAP;                ///< Vol-Button Mitte
    constexpr int VOL_DOT_W = 3;                                                             ///< DotMatrix Dot-Breite
    constexpr int VOL_DOT_H = 3;                                                             ///< DotMatrix Dot-Höhe
    constexpr int VOL_DOT_SPACE = 1;                                                         ///< Dot-Abstand
    constexpr int VOL_CHAR_SPACE = 5;                                                        ///< Zeichen-Abstand
    constexpr int VOL_CHAR_W = 5 * VOL_DOT_W + 4 * VOL_DOT_SPACE;                            ///< Zeichen-Breite
    constexpr int VOL_DISP_W = 2 * VOL_CHAR_W + VOL_CHAR_SPACE;                              ///< Breite für 2 Zeichen + Abstand (z.B. "00"-"99")
    constexpr int VOL_DISP_H = 7 * VOL_DOT_H + 6 * VOL_DOT_SPACE;                            ///< Display-Höhe
    constexpr int VOL_FRAME_PAD = 8;                                                         ///< Innenabstand Rahmen
    constexpr int VOL_FRAME_W = VOL_DISP_W + 2 * VOL_FRAME_PAD;                              ///< Rahmenbreite
    constexpr int VOL_FRAME_H = RADIO_BTN_SIZE;                                              ///< Rahmenhöhe
    constexpr int VOL_FRAME_X = VOL_DOWN_X + RADIO_BTN_SIZE / 2 + RADIO_BTN_GAP;             ///< Rahmen linke Kante
    constexpr int VOL_DISP_X = VOL_FRAME_X + VOL_FRAME_PAD;                                  ///< DotMatrix linke Kante
    constexpr int VOL_UP_X = VOL_FRAME_X + VOL_FRAME_W + RADIO_BTN_GAP + RADIO_BTN_SIZE / 2; ///< Vol+ Button Mitte

    // SD-Player Buttons
    constexpr int SD_GAP_LEFT = VOL_UP_X + RADIO_BTN_SIZE / 2;                                 ///< Linke Grenze des SD-Button-Gruppierungsbereichs
    constexpr int SD_GAP_RIGHT = EXIT_BTN_X - BTN_SIZE / 2;                                    ///< Rechte Grenze des SD-Button-Gruppierungsbereichs
    constexpr int SD_GROUP_W = 4 * RADIO_BTN_SIZE + 3 * RADIO_BTN_GAP;                         ///< Gesamtbreite der 4 SD-Buttons inkl. Abstände
    constexpr int SD_GROUP_LEFT = SD_GAP_LEFT + (SD_GAP_RIGHT - SD_GAP_LEFT - SD_GROUP_W) / 2; ///< Linke Grenze der SD-Buttons
    constexpr int SD_BROWSE_X = SD_GROUP_LEFT + RADIO_BTN_SIZE / 2;                            ///< Browse-Mitte
    constexpr int SD_PLAY_X = SD_BROWSE_X + RADIO_BTN_SIZE + RADIO_BTN_GAP;                    ///< Play/Pause-Mitte
    constexpr int SD_NEXT_X = SD_PLAY_X + RADIO_BTN_SIZE + RADIO_BTN_GAP;                      ///< Next-Mitte
    constexpr int SD_CLEAR_X = SD_NEXT_X + RADIO_BTN_SIZE + RADIO_BTN_GAP;                     ///< Clear-Mitte

    // Rahmenfarben
    constexpr uint16_t RADIO_BORDER_COLOR = RetroColors::Petrol;  ///< Radio: Petrol (0x1B4E)
    constexpr uint16_t SD_BORDER_COLOR = RetroColors::Terracotta; ///< SD: Terrakotta (0xD346)

    // SD-Playback Timeouts
    constexpr unsigned long SD_START_TIMEOUT_MS = 5000; ///< Max. Wartezeit auf Playback-Start

    // Farben
    constexpr uint16_t COLOR_HEADER = TFT_SILVER; ///< Überschriften
    constexpr uint16_t COLOR_TEXT = 0xC618;       ///< Hellgrau für Text
    constexpr uint16_t COLOR_DIM = 0x7BEF;        ///< Gedimmter Text
    constexpr uint16_t COLOR_ACCENT = 0x04FF;     ///< Cyan-Akzent
    constexpr uint16_t COLOR_OVERDUE = 0xF800;    ///< Rot für überfällige Aufgaben
    constexpr uint16_t COLOR_TODAY = 0xFE60;      ///< Orange für "heute fällig"
    constexpr uint16_t COLOR_BIRTHDAY = 0xF81F;   ///< Magenta für Geburtstage

    // Info-Bar Farben
    constexpr uint16_t COLOR_INFOBAR_BG = 0x1082;      ///< Dunkler Hintergrund
    constexpr uint16_t COLOR_INFOBAR_BORDER = 0x39E8;  ///< Horizontale Trennlinien
    constexpr uint16_t COLOR_INFOBAR_STATION = 0x5370; ///< Sendername (Akzent)
    constexpr uint16_t COLOR_INFOBAR_BITRATE = 0x6C64; ///< Bitrate (gedimmt)
    constexpr uint16_t COLOR_INFOBAR_SEP = 0x7BEF;     ///< Separator " | " (Dunkelgrau)

    // Breaking News Farben + Konfiguration
    constexpr uint16_t COLOR_NEWS_BG = 0xF800;                 ///< Roter Hintergrund für Eilmeldungen
    constexpr uint16_t COLOR_NEWS_TEXT = TFT_WHITE;            ///< Weiße Schrift für die News-Zeile
    constexpr uint16_t COLOR_NEWS_BG_TOP1 = COLOR_NEWS_BG;     ///< Gleiche Hintergrundfarbe wie Eilmeldung
    constexpr uint16_t COLOR_NEWS_TEXT_TOP1 = COLOR_NEWS_TEXT; ///< Gleiche Textfarbe wie Eilmeldung
    constexpr bool NEWS_UPPERCASE = false;                     ///< true = Alle News-Headline in Großbuchstaben, false = Originaltext (je nach API-Antwort)

    constexpr const char *TAGESSCHAU_API_URL = "https://www.tagesschau.de/api2u/news"; ///< API-Endpoint für Tagesschau-News

    // Breaking News Test-Modus: auf true setzen zum Testen.
    constexpr bool NEWS_TEST_MODE = false;
    constexpr uint16_t COLOR_DIVIDER = 0x2945;         ///< Farbe für Trennlinien (z.B. zwischen Events, vertikaler Trenner zwischen Events und Tasks)
    constexpr uint16_t COLOR_LOADING = 0x39E8;         ///< Farbe für Lade-Spinner (Petrol)
    constexpr uint16_t COLOR_PROGRESS = 0x8010;        ///< Farbe für Fortschrittsanzeige (dunkles Lila)
    constexpr uint16_t COLOR_WEATHER_ICN = TFT_SILVER; ///< Farbe für Wetter-Icon (Silber)

    // Event-Farben
    constexpr uint16_t COLOR_EVT_BAR_NORMAL = 0x5370;      ///< Blau für normale Termine
    constexpr uint16_t COLOR_EVT_BAR_ALLDAY = 0x3666;      ///< Dunkelblau für Ganztagestermine (ohne Uhrzeit)
    constexpr uint16_t COLOR_EVT_BAR_HOLIDAY = 0xFD20;     ///< Orange für Feiertags-Balken (eventType "holiday")
    constexpr uint16_t COLOR_EVT_SUMMARY_HOLIDAY = 0x7F13; ///< Dunkelrot für Feiertags-Titel (eventType "holiday")
    constexpr uint16_t COLOR_EVT_TIME = 0xE300;            ///< Rotton für die Uhrzeit-Spalte
    constexpr uint16_t COLOR_EVT_SUMMARY = 0xD69A;         ///< Hellgrau für Termintitel
    constexpr uint16_t COLOR_EVT_DAY_HEADER = 0x9CF3;      ///< Retro-Grau für Tages-Header (z.B. "Heute", "Mo, 24. Feb")
    constexpr uint16_t COLOR_EVT_SEPARATOR = 0x2945;       ///< Farbe für horizontale Trennlinien zwischen Terminen

    // Badge für "+x weitere"
    constexpr int BADGE_MARGIN_RIGHT = 8;         ///< Abstand vom rechten Rand der Spalte
    constexpr int BADGE_MARGIN_BOTTOM = 6;        ///< Abstand von der unteren Trennlinie
    constexpr int BADGE_PAD_H = 4;                ///< Horizontales Padding im Badge
    constexpr int BADGE_PAD_V = 2;                ///< Vertikales Padding im Badge
    constexpr uint16_t COLOR_BADGE_BG = 0x2945;   ///< Badge-Hintergrund (dunkelgrau)
    constexpr uint16_t COLOR_BADGE_TEXT = 0x9CF3; ///< Badge-Text (retro-grau)

    // Aufgaben-Layout
    constexpr int TASK_BULLET_SIZE = 6;            ///< Durchmesser der Aufgabensymbole (Kreis oder Quadrat, je nach Status)
    constexpr int TASK_BULLET_GAP = 8;             ///< Abstand zwischen Aufzählungszeichen und Text
    constexpr int TASK_LINE_H = 22;                ///< Zeilenhöhe pro Aufgabe (inkl. vertikalem Abstand)
    constexpr uint16_t COLOR_TASK_BULLET = 0x5370; ///< Farbe der Aufgabensymbole (Blau)

    // Rechte Spalte: Sub-Sektion (Geburtstage unter Aufgaben)
    constexpr int TASK_LABEL_H = 18;   ///< Höhe der Überschrift "AUFGABEN"
    constexpr int TASK_START_PAD = 6;  ///< Abstand zwischen Überschrift "AUFGABEN" und der ersten Aufgabe
    constexpr int SUB_SECTION_GAP = 8; ///< Vertikaler Abstand zwischen Aufgaben-Sektion und der Sub-Sektion (Geburtstage)
    constexpr int SUB_LABEL_H = 16;    ///< Höhe der Sub-Überschrift "GEBURTSTAGE"
    constexpr int SUB_LABEL_PAD = 4;   ///< Abstand zwischen Sub-Überschrift und dem ersten Eintrag der Sub-Sektion
    constexpr int SUB_TEXT_H = 20;     ///< Zeilenhöhe pro Eintrag in der Sub-Sektion (inkl. vertikalem Abstand)
    constexpr int BOTTOM_PAD = 4;      ///< Abstand zwischen letzter Zeile der Sub-Sektion und der Info-Bar am unteren Rand

    // Berechnete Platzreservierung für Geburtstage
    constexpr int BDAY_BLOCK_H = SUB_LABEL_H + SUB_LABEL_PAD + SUB_TEXT_H;         ///< Höhe des Geburtstags-Blocks (Überschrift + Abstand + 1 Zeile für ersten Geburtstag)
    constexpr int FIXED_BELOW_TASKS = SUB_SECTION_GAP + BDAY_BLOCK_H + BOTTOM_PAD; ///< Fest reservierte Höhe unterhalb der Aufgaben-Sektion (Abstand + Geburtstage + Abstand zum unteren Rand)
    constexpr int TASK_AREA_START = TASK_LABEL_H + TASK_START_PAD;                 ///< Höhe der festen Bereiche über den Aufgaben (Überschrift "AUFGABEN" + Abstand) - alles darunter ist für die Aufgaben reserviert

    /// Max. sichtbare Aufgaben (Rest = Geburtstage)
    constexpr int MAX_VISIBLE_TASKS = (TASKS_SPR_H - TASK_AREA_START - FIXED_BELOW_TASKS) / TASK_LINE_H; ///< Max. Anzahl sichtbarer Aufgaben (abhängig von der Höhe des Tasks-Sprites und der reservierten Bereiche für Überschrift, Geburtstage und Abstände)

    // URL-Buffer
    constexpr size_t URL_BUFFER_SIZE = 512; ///< Max. Länge für API-URLs

    // Sektionsweise Vergleichsfunktionen
    static_assert(std::is_trivially_copyable<DayViewData>::value, "DayViewData muss trivially copyable sein (Queue by-value, memcpy, memcmp)");
    static_assert(std::is_trivially_copyable<DayViewEvent>::value, "DayViewEvent muss trivially copyable sein (memcmp in eventsChanged)");
    static_assert(std::is_trivially_copyable<DayViewTask>::value, "DayViewTask muss trivially copyable sein (memcmp in tasksChanged)");
    static_assert(std::is_trivially_copyable<DayViewWeather>::value, "DayViewWeather muss trivially copyable sein (memcmp in weatherChanged)");
    static_assert(std::is_trivially_copyable<DayViewBirthday>::value, "DayViewBirthday muss trivially copyable sein (memcmp in birthdaysChanged)");

    // Wetter-Änderungen: memcmp auf das Wetter-Struct
    /**
     * @brief Prüft, ob sich die Wetterdaten geändert haben.
     */
    bool weatherChanged(const DayViewData *cur, const DayViewData *prev)
    {
        if (!prev)
            return true;

        return memcmp(&cur->weather, &prev->weather, sizeof(DayViewWeather)) != 0;
    }

    // Termin-Änderungen: Zähler, Demo-Kennung und memcmp auf das Events-Array
    /**
     * @brief Prüft, ob sich die Termine geändert haben.
     */
    bool eventsChanged(const DayViewData *cur, const DayViewData *prev)
    {
        if (!prev)
            return true;

        if (cur->eventCount != prev->eventCount || cur->eventTotalCount != prev->eventTotalCount || cur->calendarIsDemo != prev->calendarIsDemo)
            return true;

        const int safeEvents = std::min(cur->eventCount, DayViewCfg::MAX_EVENTS);
        return memcmp(cur->events, prev->events, safeEvents * sizeof(DayViewEvent)) != 0;
    }

    // Geburtstags-Änderungen: Zähler, Demo-Kennung und memcmp auf das Birthdays-Array. Geburtstage stehen in der Aufgabenspalte.
    /**
     * @brief Prüft, ob sich die Geburtstage geändert haben.
     */
    bool birthdaysChanged(const DayViewData *cur, const DayViewData *prev)
    {
        if (!prev)
            return true;

        if (cur->birthdayCount != prev->birthdayCount || cur->birthdayTotalCount != prev->birthdayTotalCount || cur->calendarIsDemo != prev->calendarIsDemo)
            return true;

        const int safeBdays = std::min(cur->birthdayCount, DayViewCfg::MAX_BIRTHDAYS);
        return memcmp(cur->birthdays, prev->birthdays, safeBdays * sizeof(DayViewBirthday)) != 0;
    }

    // Aufgaben-Änderungen: memcmp auf die Tasks-Arrays
    /**
     * @brief Prüft, ob sich die Aufgabendaten geändert haben.
     */
    bool tasksChanged(const DayViewData *cur, const DayViewData *prev)
    {
        if (!prev)
            return true;

        if (cur->taskCount != prev->taskCount || cur->taskTotalCount != prev->taskTotalCount || cur->tasksIsDemo != prev->tasksIsDemo)
            return true;

        const int safeTasks = std::min(cur->taskCount, DayViewCfg::MAX_TASKS);
        return memcmp(cur->tasks, prev->tasks, safeTasks * sizeof(DayViewTask)) != 0;
    }

    // Prüft ob eine API-URL tatsächlich nutzbar ist
    /**
     * @brief Prüft, ob eine verwendbare URL konfiguriert ist.
     */
    bool isUrlConfigured(const char *url)
    {
        return url && strlen(url) >= 5;
    }

    /**
     * @brief Mappt einen WMO-Wettercode auf den passenden DayViewKey
     *
     * Wird beim Zeichnen aufgerufen, damit ein Sprachwechsel ohne erneutes Fetch greift. Unbekannte Codes liefern DAYVIEW_WMO_UNKNOWN.
     */
    DayViewKey wmoToKey(int wmoCode)
    {
        switch (wmoCode)
        {
        case 0:
            return DayViewKey::DAYVIEW_WMO_CLEAR;
        case 1:
            return DayViewKey::DAYVIEW_WMO_MOSTLY_SUNNY;
        case 2:
            return DayViewKey::DAYVIEW_WMO_PARTLY_CLOUDY;
        case 3:
            return DayViewKey::DAYVIEW_WMO_OVERCAST;
        case 45:
            return DayViewKey::DAYVIEW_WMO_FOG;
        case 48:
            return DayViewKey::DAYVIEW_WMO_RIME_FOG;
        case 51:
            return DayViewKey::DAYVIEW_WMO_DRIZZLE_LIGHT;
        case 53:
            return DayViewKey::DAYVIEW_WMO_DRIZZLE;
        case 55:
            return DayViewKey::DAYVIEW_WMO_DRIZZLE_HEAVY;
        case 56:
            return DayViewKey::DAYVIEW_WMO_FREEZING_RAIN;
        case 57:
            return DayViewKey::DAYVIEW_WMO_FREEZING_RAIN_HEAVY;
        case 61:
            return DayViewKey::DAYVIEW_WMO_RAIN_LIGHT;
        case 63:
            return DayViewKey::DAYVIEW_WMO_RAIN;
        case 65:
            return DayViewKey::DAYVIEW_WMO_RAIN_HEAVY;
        case 66:
            return DayViewKey::DAYVIEW_WMO_FREEZING_RAIN;
        case 67:
            return DayViewKey::DAYVIEW_WMO_FREEZING_RAIN_HEAVY;
        case 71:
            return DayViewKey::DAYVIEW_WMO_SNOW_LIGHT;
        case 73:
            return DayViewKey::DAYVIEW_WMO_SNOW;
        case 75:
            return DayViewKey::DAYVIEW_WMO_SNOW_HEAVY;
        case 77:
            return DayViewKey::DAYVIEW_WMO_SNOW_GRAINS;
        case 80:
            return DayViewKey::DAYVIEW_WMO_SHOWERS_LIGHT;
        case 81:
            return DayViewKey::DAYVIEW_WMO_SHOWERS;
        case 82:
            return DayViewKey::DAYVIEW_WMO_SHOWERS_HEAVY;
        case 85:
            return DayViewKey::DAYVIEW_WMO_SNOW_SHOWERS;
        case 86:
            return DayViewKey::DAYVIEW_WMO_SNOW_SHOWERS_HEAVY;
        case 95:
            return DayViewKey::DAYVIEW_WMO_THUNDER;
        case 96:
            return DayViewKey::DAYVIEW_WMO_THUNDER_HAIL;
        case 99:
            return DayViewKey::DAYVIEW_WMO_THUNDER_HEAVY;
        default:
            return DayViewKey::DAYVIEW_WMO_UNKNOWN;
        }
    }

    /**
     * @brief Mappt den deutschen Backend-Wochentag-String auf einen tm_wday-Index
     *
     * Das Kalender-Backend liefert dayName immer in deutscher Kurzform ("So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"). Bei unbekanntem oder leerem String wird -1 zurückgegeben, in dem Fall wird zur Zeichenzeit kein Wochentag-Header ausgegeben.
     */
    int8_t weekdayIdxFromBackendName(const char *dn)
    {
        if (!dn || dn[0] == '\0')
            return -1;
        static const char *const WD[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
        for (int8_t i = 0; i < 7; ++i)
        {
            if (strcmp(dn, WD[i]) == 0)
                return i;
        }
        return -1;
    }

} // namespace

// ===================================================================
// Datei-Scope-Hilfsfunktionen
// ===================================================================

/**
 * @brief Wählt die beste verfügbare Artikel-URL aus einem JSON-Item.
 */
static const char *pickArticleUrl(JsonObject item)
{
    auto hasArticlePath = [](const char *url) -> bool
    {
        if (!url || url[0] == '\0')
            return false;

        const char *afterProto = strstr(url, "://");
        if (!afterProto)
            return false;

        const char *slash1 = strchr(afterProto + 3, '/');
        if (!slash1 || slash1[1] == '\0')
            return false;

        const char *pathStart = slash1 + 1;

        // Zweites Segment vorhanden und nicht leer?
        const char *slash2 = strchr(pathStart, '/');
        if (slash2 && slash2[1] != '\0')
            return true;

        // Kein zweites Segment, aber Dateiname mit Punkt?
        return strchr(pathStart, '.') != nullptr;
    };

    const char *share = item["shareURL"] | "";
    const char *detail = item["detailsweb"] | "";

    LOG_INFO_F("DV pickArticleUrl: shareURL='%.80s' detailsweb='%.80s'", share, detail);

    if (hasArticlePath(share))
        return share;

    if (hasArticlePath(detail))
        return detail;

    LOG_WARNING("DV pickArticleUrl: keine verwertbare Artikel-URL gefunden");
    return "";
}

// ===================================================================
// KONSTRUKTOR / DESTRUKTOR
// ===================================================================

/**
 * @brief Konstruktor
 */
DayView::DayView()
{
    void *mem = heap_caps_malloc(sizeof(DayViewData), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!mem) // Fallback auf internen Heap
        mem = malloc(sizeof(DayViewData));

    if (mem)
    {
        memset(mem, 0, sizeof(DayViewData));
        m_currentData = static_cast<DayViewData *>(mem);
    }

    // Vorherige Daten für Change-Detection (PSRAM)
    void *prevMem = heap_caps_malloc(sizeof(DayViewData), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!prevMem)
        prevMem = malloc(sizeof(DayViewData));

    if (prevMem)
    {
        memset(prevMem, 0, sizeof(DayViewData));
        m_previousData = static_cast<DayViewData *>(prevMem);
    }
}

/**
 * @brief Destruktor - Worker beenden, Ressourcen freigeben
 */
DayView::~DayView()
{
    // Audio stoppen beim Verlassen (Radio oder SD-Player)
    if (m_callbacks.requestStopPlayback)
        m_callbacks.requestStopPlayback();

    // Modale Dialoge vor Worker beenden (halten 'this' als Raw-Pointer)
    m_messageBox.reset();
    m_fileExplorer.reset();

    shutdownWorker();

    // Queue löschen
    if (m_dataQueue != nullptr)
    {
        vQueueDelete(m_dataQueue);
        m_dataQueue = nullptr;
    }

    // Daten freigeben
    free(m_currentData);
    m_currentData = nullptr;

    // Vorherige Daten freigeben
    free(m_previousData);
    m_previousData = nullptr;

    // SD-Playlist freigeben
    m_sdPlaylist.clear();

    // UI-Ressourcen freigeben
    cleanupInitResources();
}

// ===================================================================
// SHUTDOWN
// ===================================================================

/**
 * @brief Beendet den Worker-Task
 */
void DayView::shutdownWorker(bool fast)
{
    TaskHandle_t handle = m_taskHandle.load();
    if (!handle)
        return;

    // Kooperativer Shutdown: Flag setzen -> HTTP-Abbruch + Worker-Exit. HTTP_SERVICE.getThrottled() prüft m_shutdownRequested und bricht ab, workerLoop() prüft es nach jedem Fetch und vor jeder Wartezeit.
    m_shutdownRequested.store(true);
    xTaskNotifyGive(handle);

    // Warten mit periodischem Nachfragen (Notify wiederholen für den Fall, dass der Worker nach HTTP-Return erneut in ulTaskNotifyTake landet). Fast-Pfad: kurz warten, dann Force Delete. Normal-Pfad: großzügig warten (HTTP-Timeout + Cleanup).
    const int maxWaitMs = fast ? 3000 : 20000;
    int waited = 0;
    while (m_taskHandle.load() != nullptr && waited < maxWaitMs)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        waited += 100;

        // Alle 500ms Notify wiederholen
        if (waited % 500 == 0)
        {
            handle = m_taskHandle.load();
            if (handle)
                xTaskNotifyGive(handle);
        }
    }

    handle = m_taskHandle.load();
    if (handle)
    {
        // Worker hat nicht rechtzeitig reagiert.
        LOG_ERROR_F("DV: Worker nach %d ms nicht beendet - Force Delete! (fast=%s)", waited, fast ? "true" : "false");
        vTaskDelete(handle);
        m_taskHandle.store(nullptr);
    }
    else
    {
        LOG_SUCCESS_F("DV: Worker sauber beendet (%d ms)", waited);
    }
}

/**
 * @brief UI-Ressourcen freigeben
 *
 * Zentrale Cleanup-Funktion für alle Fehlerpfade in init() und den Destruktor.
 */
void DayView::cleanupInitResources()
{
    if (m_tft)
    {
        m_tft->safeDeleteSprite(m_headerSprite);
        m_tft->safeDeleteSprite(m_progressSprite);
        m_tft->safeDeleteSprite(m_eventsSprite);
        m_tft->safeDeleteSprite(m_tasksSprite);
        m_tft->safeDeleteSprite(m_bottomSprite);
        m_tft->safeDeleteSprite(m_volSprite);
        m_tft->safeDeleteSprite(m_stnSprite);
    }

    delete m_symbols;
    m_symbols = nullptr;
}

/**
 * @brief Gibt den Modultyp zurück
 */
AppModule DayView::type() const
{
    return AppModule::DAYVIEW;
}

// ===================================================================
// INITIALISIERUNG
// ===================================================================

/**
 * @brief Initialisiert das DayView-Modul
 */
bool DayView::init(LGFX &tft, StationManager &stationManager, Audio &audio, const DayViewCallbacks &callbacks)
{
    m_tft = &tft;
    m_stationManager = &stationManager;
    m_audio = &audio;
    m_callbacks = callbacks;

    // Konstruktor-Allokation prüfen
    if (!m_currentData || !m_previousData)
    {
        LOG_ERROR_F("DV: Daten-Allokation fehlgeschlagen (currentData=%p, previousData=%p)", m_currentData, m_previousData);
        return false;
    }

    // Alt-Events des vorherigen Moduls verwerfen
    drainStaleAudioEvents();

    // Wetter-Symbole erstellen
    m_symbols = new Symbols(m_tft);
    if (!m_symbols)
        LOG_WARNING("DV: Symbols-Allokation fehlgeschlagen (kein Wetter-Icon)");

    // API-URLs aus AppState laden
    m_calendarUrl = APP_STATE.getCalendarScriptUrl();
    m_todoUrl = APP_STATE.getTodoScriptUrl();

    // Demo-Modus: aktiv wenn keine der URLs konfiguriert ist
    auto isValidUrl = [](const char *url) -> bool
    {
        const size_t len = url ? strlen(url) : 0;
        return (len >= 5 && strcmp(url + len - 5, "/exec") == 0);
    };

    const bool hasCalendar = isValidUrl(m_calendarUrl);
    const bool hasTodo = isValidUrl(m_todoUrl);

    // Wetter-Koordinaten: gültig wenn nicht NaN und im zulässigen Bereich
    const double wLat = APP_STATE.getWeatherLatitude();
    const double wLon = APP_STATE.getWeatherLongitude();
    const bool hasWeather = !std::isnan(wLat) && !std::isnan(wLon) && (wLat >= -90.0 && wLat <= 90.0) && (wLon >= -180.0 && wLon <= 180.0);

    m_demoMode = (!hasCalendar && !hasTodo && !hasWeather);

    if (m_demoMode)
        LOG_INFO("DV: Keine API-URLs konfiguriert -> Demo-Modus");
    else
        LOG_INFO_F("DV: APIs: Cal:%d Todo:%d Wetter:%d", hasCalendar, hasTodo, hasWeather);

    // Header-DotMatrix für Uhrzeit + Temperatur konfigurieren
    auto initHeaderDot = [](DotMatrix &dm)
    {
        dm.init(HDR_DOT_W, HDR_DOT_H, HDR_DOT_SPACE, HDR_CHAR_SPACE);
        dm.setOnColor(HDR_DOT_ON, HDR_DOT_ON);    // Kein Schatten
        dm.setOffColor(HDR_DOT_OFF, HDR_DOT_OFF); // Kein Schatten
        dm.setBackgroundColor(Theme::ColorMainBackGround);
        dm.setShowOffDots(true);
        dm.setGradientEnabled(false);
        dm.setDotShape(DotMatrix::DotShape::SQUARE);
    };

    initHeaderDot(m_clockDotMatrix);
    m_clockDotMatrix.reserveText(8); // "HH:MM"

    initHeaderDot(m_tempDotMatrix);
    m_tempDotMatrix.reserveText(10); // "-00.0°C"

    // m_headerTextX aus DotMatrix-Breite berechnen
    m_tempDotMatrix.setText("-00.0°");
    const int maxTempW = m_tempDotMatrix.getTextWidth();
    m_clockDotMatrix.setText("00:00");
    const int clockW = m_clockDotMatrix.getTextWidth();
    m_headerTextX = std::max(clockW, maxTempW) + CLOCK_GAP;

    // Header-Sprite erstellen
    if (!m_tft->safeCreateSprite(m_headerSprite, HEADER_SPR_W, HEADER_SPR_H))
    {
        LOG_WARNING("DV: Header-Sprite fehlgeschlagen");
        cleanupInitResources();
        return false;
    }
    m_headerSprite->clear(Theme::ColorMainBackGround);
    m_lastDrawnMinute = -1;

    // Progressbar-Sprite (1px breit, volle Höhe)
    if (!m_tft->safeCreateSprite(m_progressSprite, 1, m_tft->height()))
        LOG_WARNING("DV: Progressbar-Sprite fehlgeschlagen");

    // Sektions-Sprites
    if (!m_tft->safeCreateSprite(m_eventsSprite, EVENTS_SPR_W, EVENTS_SPR_H))
        LOG_WARNING("DV: Events-Sprite fehlgeschlagen");
    if (!m_tft->safeCreateSprite(m_tasksSprite, TASKS_SPR_W, TASKS_SPR_H))
        LOG_WARNING("DV: Tasks-Sprite fehlgeschlagen");
    if (!m_tft->safeCreateSprite(m_bottomSprite, BOTTOM_SPR_W, BOTTOM_SPR_H))
        LOG_WARNING("DV: Bottom-Sprite fehlgeschlagen");

    // Exit-Button
    m_exitButton.initButton(
        m_tft,             // Display-Objekt
        EXIT_BTN_X,        // X-Position (Mitte des Buttons)
        EXIT_BTN_Y,        // Y-Position (Mitte des Buttons)
        BTN_SIZE,          // Breite des Buttons
        BTN_SIZE,          // Höhe des Buttons
        BTN_RADIUS,        // Radius des Buttons
        BTN_COLOR,         // Hintergrundfarbe des Buttons
        TFT_BLACK,         // Textfarbe (wird nicht verwendet, da ein Symbol gezeichnet wird)
        TFT_DARKGREY,      // Rahmenfarbe des Buttons
        "^POWER:32:D206^", // Text auf dem Button (hier ein Power-Symbol)
        nullptr            // kein Icon
    );

    // Stationen laden bevor Layout gezeichnet wird
    if (m_stationManager)
        m_stationManager->loadStations();

    // Layout sofort zeichnen (inkl. Radio-Bar mit geladenen Sendern)
    m_tft->clear(Theme::ColorMainBackGround);
    drawLayout();

    // Demo-Modus: Sofort alles anzeigen
    if (m_demoMode)
    {
        generateDemoData(m_currentData);
        m_currentData->availableSections = SECTION_ALL;
        drawHeaderSection();
        drawEventsSection();
        drawTasksSection();
        m_qrCodeOverlay.redraw(); // Tasks-Sprite überdeckt QR-Bereich -> sofort wiederherstellen
    }
    else
    {
        // Header sofort zeichnen
        drawHeaderSection();

        // Lade-Hinweise anzeigen
        drawSectionLoading(m_eventsSprite, EVENTS_SPR_W, EVENTS_SPR_H, COL_LEFT_X, CONTENT_Y, reinterpret_cast<const char *>(STR(DayViewKey::DAYVIEW_LOADING_EVENTS)));
        drawSectionLoading(m_tasksSprite, TASKS_SPR_W, TASKS_SPR_H, TASKS_SPR_X, CONTENT_Y, reinterpret_cast<const char *>(STR(DayViewKey::DAYVIEW_LOADING_TASKS)));
    }

    // Queue erstellen
    m_dataQueue = xQueueCreate(DayViewCfg::QUEUE_SIZE, sizeof(DayViewData));
    if (m_dataQueue == nullptr)
    {
        LOG_ERROR("DV: Queue-Erstellung fehlgeschlagen");
        cleanupInitResources();
        return false;
    }

    // Initialen Refresh anfordern
    m_refreshRequested.store(true);
    m_lastRefresh = millis();

    // Worker-Task starten
    TaskHandle_t handle = nullptr;
    const BaseType_t result = xTaskCreatePinnedToCore(
        workerEntry,
        "DayViewWorker",
        WORKER_STACK_SIZE,
        this,
        WORKER_PRIORITY,
        &handle,
        WORKER_CORE);

    if (result != pdPASS || !handle)
    {
        LOG_ERROR("DV: Worker-Task konnte nicht erstellt werden");
        vQueueDelete(m_dataQueue);
        m_dataQueue = nullptr;
        cleanupInitResources();
        return false;
    }

    m_taskHandle.store(handle);

    // Worker sofort aufwecken
    xTaskNotifyGive(handle);

    LOG_SUCCESS("DV: Modul initialisiert, Worker gestartet");

    return true;
}

// ===================================================================
// DRAW - Progressive Updates + Radio + Clock + Progressbar
// ===================================================================

/**
 * @brief Hauptzeichenfunktion
 */
void DayView::draw()
{
    if (!m_tft || !m_dataQueue)
        return;

    // Sprachwechsel? -> kompletten Bildschirm neu zeichnen
    if (I18n::consumeLanguageChanged())
        redrawAll();

    // GFX blockiert?
    const bool gfxBlocked = APP_STATE.getBlockGFX();

    // Audio-Events auch bei offenem Dialog abbauen, sonst läuft der Ring über und EOF/Metadaten gehen verloren; gezeichnet wird dabei nur ohne Dialog
    processAudioEvents();

    // PresetManager-Dialog aktiv? -> eigene Touch-Verarbeitung
    if (m_presetDlg && m_presetDlg->isActive())
    {
        m_presetDlg->update();
        return;
    }

    // StationPicker-Dialog aktiv? -> eigene Touch-Verarbeitung
    if (m_stationPicker && m_stationPicker->isActive())
    {
        m_stationPicker->update();
        return;
    }

    // FileExplorer-Dialog aktiv? -> eigene Touch-Verarbeitung
    if (m_fileExplorer && m_fileExplorer->isActive())
    {
        m_fileExplorer->update();
        return;
    }

    // MessageBox-Dialog aktiv? -> eigene Touch-Verarbeitung
    if (m_messageBox && (m_messageBox->isActive() || m_messageBox->isBusy()))
    {
        m_messageBox->update();

        // Completion sofort abschließen (einfacher Dialog, keine externe Verarbeitung)
        if (m_messageBox->isBusy())
        {
            m_messageBox->completeConfirmation(true);
            return;
        }

        return;
    }

    // Touch verarbeiten
    int16_t tx, ty;
    const unsigned long now = millis();
    const bool touching = !gfxBlocked && m_tft->getTouch(&tx, &ty);

    // Frischtouch-Flanke: nur ein frisch aufgesetzter Finger macht Aktionen scharf, hereinwandernde Gesten nicht
    const bool freshTouch = touching && !m_wasTouching;
    m_wasTouching = touching;

    // Rising-Edge-Detect: m_newsBarTouchConsumed verhindert Dauerfeuer bei gehaltenem Touch
    if (!touching)
    {
        m_newsBarTouchConsumed = false;
        m_volHeldDir = 0;
    }

    // Volume-Halten nur beim frischen Aufsetzen direkt auf dem Button scharf machen
    if (freshTouch)
    {
        if (m_volDownBtn.contains(tx, ty))
            m_volHeldDir = -1;
        else if (m_volUpBtn.contains(tx, ty))
            m_volHeldDir = +1;
        else
            m_volHeldDir = 0;
    }

    // Touch auf aktive News-Zeile
    if (freshTouch && m_newsActive && !m_newsBarTouchConsumed)
    {
        const int NEWS_TOUCH_MARGIN = 5; // Toleranzbereich über/unter der News-Zeile
        const bool onNewsBar = (ty >= (NEWS_TOUCH_Y1 - NEWS_TOUCH_MARGIN) && ty <= (NEWS_TOUCH_Y2 + NEWS_TOUCH_MARGIN));

        if (onNewsBar)
        {
            // Erstes Touch-Down auf News-Zeile -> QR-Code umschalten (ein/aus)
            m_newsBarTouchConsumed = true; // weitere Frames ignorieren bis Finger weg
            if (m_qrCodeOverlay.isVisible())
                m_qrCodeOverlay.hide();
            else if (m_newsShareUrl[0] != '\0')
                m_qrCodeOverlay.show(m_tft, m_newsShareUrl, QR_X, QR_Y, QR_BOX_SIZE);

            // Meldungszeile bleibt sichtbar
        }
        else
        {
            // Touch außerhalb der News-Zeile -> Meldung + QR verwerfen
            m_newsActive = false;
            m_newsBarTouchConsumed = false;
            m_qrCodeOverlay.hide();

            drawBottomSection();
            return;
        }
    }

    // Radio Prev/Next
    if (touching && (m_radioPrevBtn.contains(tx, ty) || m_radioNextBtn.contains(tx, ty)))
    {
        const int8_t dir = m_radioPrevBtn.contains(tx, ty) ? -1 : +1;

        if (m_radioRepeatDir == 0)
        {
            // Erster Druck nur bei frischem Aufsetzen; ein hereinwandernder Finger bleibt wirkungslos
            if (freshTouch)
            {
                m_radioRepeatDir = dir;
                m_radioRepeatStart = now;
                m_radioRepeatLast = now;

                if (dir < 0)
                {
                    m_radioPrevBtn.drawButton(true);
                    radioPrevStation();
                }
                else
                {
                    m_radioNextBtn.drawButton(true);
                    radioNextStation();
                }

                drawRadioBar();
            }
        }
        else if (m_radioRepeatDir == dir)
        {
            // Finger gehalten -> Repeat nach Verzögerung
            const unsigned long held = now - m_radioRepeatStart;
            if (held >= RADIO_REPEAT_DELAY_MS && (now - m_radioRepeatLast >= RADIO_REPEAT_INTERVAL_MS))
            {
                m_radioRepeatLast = now;

                if (dir < 0)
                    radioPrevStation();
                else
                    radioNextStation();

                drawRadioBar();
            }
        }

        // Richtungswechsel (Finger gleitet) -> ignorieren bis zum loslassen
    }
    else
    {
        // Kein Touch auf Prev/Next -> Repeat-State zurücksetzen
        if (m_radioRepeatDir != 0)
            m_radioRepeatDir = 0;

        // Restliche Buttons mit Debounce
        if (touching && (now - m_lastTouchMs >= 250))
        {
            m_lastTouchMs = now;

            // Exit-Button
            if (freshTouch && m_exitButton.contains(tx, ty))
            {
                m_exitButton.drawButton(true);

                if (m_callbacks.onExitRequested)
                    m_callbacks.onExitRequested();

                return;
            }

            // Favoriten-Button -> PresetManager-Dialog öffnen
            if (freshTouch && m_radioFavBtn.contains(tx, ty) && m_presetDlg && m_stationManager)
            {
                m_radioFavBtn.drawButton(true);
                APP_STATE.setBlockGFX(true);

                m_presetDlg->setBounds((m_tft->width() - 600) / 2, (m_tft->height() - 440) / 2, 600, 440);
                m_presetDlg->setTitle(reinterpret_cast<const char *>(STR(DayViewKey::DAYVIEW_PRESET_TITLE)));
                m_presetDlg->setTimeout(10000);
                m_presetDlg->setCallback([this](int selectedIndex)
                                         {
                    if (selectedIndex >= 0)
                    {
                        // Laufende Quelle stoppen + Alt-Events verwerfen, dann Sender starten
                        stopCurrentSource();

                        if (m_callbacks.requestPlayStation)
                            m_callbacks.requestPlayStation(static_cast<uint8_t>(selectedIndex));

                        m_audioSource = AudioSource::RADIO;
                    }

                    APP_STATE.setBlockGFX(false); });
                m_presetDlg->start(m_stationManager->getStations(), m_stationManager->getStationCount());
                return;
            }

            // Sender-DotMatrix Touch -> StationPicker-Dialog öffnen
            {
                const int stnFrameY = RADIO_BTN_Y - STN_FRAME_H / 2;
                if (freshTouch && m_stationPicker && m_stationManager &&
                    tx >= STN_FRAME_X && tx <= STN_FRAME_X + STN_FRAME_W &&
                    ty >= stnFrameY && ty <= stnFrameY + STN_FRAME_H)
                {
                    APP_STATE.setBlockGFX(true);

                    m_stationPicker->setTitle(
                        reinterpret_cast<const char *>(STR(DayViewKey::DAYVIEW_STATIONPICKER_TITLE)));
                    m_stationPicker->setCancelLabel(
                        reinterpret_cast<const char *>(STR(DayViewKey::DAYVIEW_STATIONPICKER_CANCEL)));
                    m_stationPicker->setTimeout(15000);
                    m_stationPicker->setCallback([this](int selectedIndex)
                                                 {
                        if (selectedIndex >= 0)
                        {
                            // Laufende Quelle stoppen + Alt-Events verwerfen, dann Sender starten
                            stopCurrentSource();

                            if (m_callbacks.requestPlayStation)
                                m_callbacks.requestPlayStation(static_cast<uint8_t>(selectedIndex));

                            m_audioSource = AudioSource::RADIO;
                        }

                        APP_STATE.setBlockGFX(false); });
                    m_stationPicker->start(m_stationManager->getStations(), m_stationManager->getStationCount(), APP_STATE.getCurrentRetroStation());
                    return;
                }
            }

            // Radio Play/Stop
            if (freshTouch && m_radioPlayBtn.contains(tx, ty))
            {
                m_radioPlayBtn.drawButton(true);
                radioPlayStop();
                drawRadioBar();
            }

            // Volume-Buttons
            handleVolumeTouches(tx, ty);

            // SD-Player Buttons
            if (freshTouch && m_sdBrowseBtn.contains(tx, ty))
            {
                m_sdBrowseBtn.drawButton(true);
                sdBrowse();
                return;
            }

            if (freshTouch && m_sdPlayBtn.contains(tx, ty))
            {
                m_sdPlayBtn.drawButton(true);
                sdPlayStop();
                drawRadioBar();
            }

            if (freshTouch && m_sdNextBtn.contains(tx, ty))
            {
                m_sdNextBtn.drawButton(true);
                sdNextTrack();
            }

            if (freshTouch && m_sdClearBtn.contains(tx, ty))
            {
                m_sdClearBtn.drawButton(true);
                sdClearPlaylist();
                return;
            }
        }
    }

    // Phase 1: Queue drainieren -> Daten mergen + Dirty-Flags vormerken. Gezeichnet wird erst in Phase 2, frühestens REDRAW_DELAY_MS nach dem letzten Dateneingang.
    DayViewData newData;
    while (xQueueReceive(m_dataQueue, &newData, 0) == pdTRUE)
    {
        const uint8_t newSections = newData.availableSections;

        // Sektionsweise mergen: nur neue Bereiche übernehmen
        if (newSections & SECTION_CALENDAR)
        {
            memcpy(m_currentData->events, newData.events, sizeof(newData.events));
            m_currentData->eventCount = newData.eventCount;
            m_currentData->eventTotalCount = newData.eventTotalCount;
            memcpy(m_currentData->birthdays, newData.birthdays, sizeof(newData.birthdays));
            m_currentData->birthdayCount = newData.birthdayCount;
            m_currentData->birthdayTotalCount = newData.birthdayTotalCount;
            m_currentData->calendarIsDemo = newData.calendarIsDemo;
        }

        if (newSections & SECTION_TASKS)
        {
            memcpy(m_currentData->tasks, newData.tasks, sizeof(newData.tasks));
            m_currentData->taskCount = newData.taskCount;
            m_currentData->taskTotalCount = newData.taskTotalCount;
            m_currentData->tasksIsDemo = newData.tasksIsDemo;
        }

        if (newSections & SECTION_WEATHER)
            m_currentData->weather = newData.weather;

        m_currentData->availableSections |= newSections;
        m_currentData->timestamp = newData.timestamp;

        // Change-Detection -> Dirty-Flags setzen (hier kein Zeichnen!)
        if ((newSections & SECTION_WEATHER) && weatherChanged(m_currentData, m_previousData))
            m_dirtyHeader = true;

        if ((newSections & SECTION_CALENDAR) && eventsChanged(m_currentData, m_previousData))
            m_dirtyEvents = true;

        // Geburtstage stehen in der Aufgabenspalte und lösen nur dort ein Neuzeichnen aus
        if ((newSections & SECTION_CALENDAR) && birthdaysChanged(m_currentData, m_previousData))
            m_dirtyTasks = true;

        if ((newSections & SECTION_TASKS) && tasksChanged(m_currentData, m_previousData))
            m_dirtyTasks = true;

        // Breaking News: Eilmeldung aktivieren/deaktivieren
        if (newSections & SECTION_NEWS)
        {
            stringutils::safeCopy(m_currentData->newsHeadline, newData.newsHeadline);
            stringutils::safeCopy(m_currentData->newsId, newData.newsId);
            stringutils::safeCopy(m_currentData->newsShareUrl, newData.newsShareUrl);
            m_currentData->newsAvailable = newData.newsAvailable;

            if (newData.newsAvailable && newData.newsId[0] != '\0' &&
                strcmp(newData.newsId, m_lastNewsId) != 0)
            {
                // Neue Meldung erkannt -> Meldungszeile aktivieren
                stringutils::safeCopy(m_lastNewsId, newData.newsId);
                m_newsIsBreaking = newData.newsIsBreaking;

                // Meldungsformat: vorhandene Zier-Zeichen und Whitespace am Anfang/Ende entfernen, dann einheitlich mit "+++ ... +++" umschliessen.
                {
                    const char *hl = newData.newsHeadline;

                    // Anfang: überspringen, solange '+' oder Whitespace
                    while (*hl == '+' || *hl == ' ' || *hl == '\t')
                        ++hl;

                    // Sanitisierte Headline in Stack-Puffer, um Trailing-Trim zu machen
                    char clean[256];
                    stringutils::safeCopy(clean, hl);

                    // Ende: '+' und Whitespace abschneiden
                    size_t cleanLen = strlen(clean);
                    while (cleanLen > 0 &&
                           (clean[cleanLen - 1] == '+' ||
                            clean[cleanLen - 1] == ' ' ||
                            clean[cleanLen - 1] == '\t'))
                    {
                        clean[--cleanLen] = '\0';
                    }

                    STRF(m_newsTickerText, DayViewKey::DAYVIEW_NEWS_FMT, clean);
                }

                // Meldungstext in Großbuchstaben wandeln, falls gewünscht
                if (NEWS_UPPERCASE)
                {
                    for (char *p = m_newsTickerText; *p; ++p)
                    {
                        // Nur ASCII-Zeichen wandeln, UTF-8-Continuation-Bytes (0x80-0xBF) ignorieren
                        if (static_cast<unsigned char>(*p) < 0x80)
                            *p = toupper(static_cast<unsigned char>(*p));

                        // Deutsche Umlaute: ä->Ä (0xC3 0xA4->0xC3 0x84), ö->Ö, ü->Ü
                        else if (static_cast<unsigned char>(*p) == 0xC3 && *(p + 1))
                        {
                            unsigned char next = static_cast<unsigned char>(*(p + 1));
                            if (next == 0xA4)
                                *(p + 1) = 0x84; // ä -> Ä
                            else if (next == 0xB6)
                                *(p + 1) = 0x96; // ö -> Ö
                            else if (next == 0xBC)
                                *(p + 1) = 0x9C; // ü -> Ü
                        }
                    }
                }

                m_newsActive = true;

                // Audio nur bei Eilmeldung stoppen
                if (m_newsIsBreaking && m_audioSource != AudioSource::NONE)
                    stopCurrentSource();

                // Share-URL für QR-Code merken; alten QR ausblenden (neue Meldung = neuer Code)
                stringutils::safeCopy(m_newsShareUrl, newData.newsShareUrl);
                m_qrCodeOverlay.hide();

                m_dirtyBottom = true;
            }

            // Meldung bleibt bis zum User-Touch
        }

        m_lastRefresh = millis();
    }

    // Phase 2: höchstens eine vorgemerkte Sektion pro Frame rendern, frühestens REDRAW_DELAY_MS nach dem letzten Dateneingang. So fällt pro Frame nur ein Sprite-Schub an, und keiner davon liegt im WLAN-Nachlauf des Abrufs, der das Panel aus dem Takt bringt. previousData wird je Sektion erst beim Zeichnen nachgezogen.
    const bool redrawPending = m_dirtyHeader || m_dirtyEvents || m_dirtyTasks || m_dirtyBottom;

    if (redrawPending && millis() - m_lastRefresh >= REDRAW_DELAY_MS)
    {
        if (m_dirtyHeader)
        {
            m_dirtyHeader = false;
            drawHeaderSection();

            if (m_previousData)
                m_previousData->weather = m_currentData->weather;
        }
        else if (m_dirtyEvents)
        {
            m_dirtyEvents = false;
            drawEventsSection();

            if (m_previousData)
            {
                memcpy(m_previousData->events, m_currentData->events, sizeof(m_currentData->events));
                m_previousData->eventCount = m_currentData->eventCount;
                m_previousData->eventTotalCount = m_currentData->eventTotalCount;
                m_previousData->calendarIsDemo = m_currentData->calendarIsDemo;
            }
        }
        else if (m_dirtyTasks)
        {
            m_dirtyTasks = false;
            drawTasksSection();
            m_qrCodeOverlay.redraw(); // Tasks-Sprite überdeckt QR-Bereich -> sofort wiederherstellen

            if (m_previousData)
            {
                memcpy(m_previousData->tasks, m_currentData->tasks, sizeof(m_currentData->tasks));
                m_previousData->taskCount = m_currentData->taskCount;
                m_previousData->taskTotalCount = m_currentData->taskTotalCount;
                m_previousData->tasksIsDemo = m_currentData->tasksIsDemo;
                memcpy(m_previousData->birthdays, m_currentData->birthdays, sizeof(m_currentData->birthdays));
                m_previousData->birthdayCount = m_currentData->birthdayCount;
                m_previousData->birthdayTotalCount = m_currentData->birthdayTotalCount;
                m_previousData->calendarIsDemo = m_currentData->calendarIsDemo;
            }
        }
        else
        {
            m_dirtyBottom = false;
            drawBottomSection();
        }
    }

    // Sekündliche Updates + Audio-Events nur wenn GFX nicht blockiert
    if (!gfxBlocked)
    {
        if (millis() - m_lastProgressUpdate > 1000)
        {
            m_lastProgressUpdate = millis();

            // Uhr-Update: nur bei Minutenwechsel
            {
                const struct tm lt = DS1307_RTC::getCurrentLocalTime();
                const int curMinute = lt.tm_hour * 60 + lt.tm_min;

                if (curMinute != m_lastDrawnMinute)
                    drawHeaderSection();
            }

            // Progressbar
            drawProgressBar();

            // Radio-Status prüfen
            const uint8_t curStation = APP_STATE.getCurrentRetroStation();
            const bool curPlaying = m_audio ? m_audio->isRunning() : false;
            const uint8_t curVolume = APP_STATE.getCurrentVolume();

            if (curStation != m_lastRadioStation || curPlaying != m_lastRadioPlaying)
            {
                m_lastRadioStation = curStation;
                m_lastRadioPlaying = curPlaying;
                drawRadioBar();
            }
            else if (curVolume != m_lastVolume)
            {
                // Nur Lautstärke-DotMatrix neu zeichnen
                drawVolumeDisplay();
            }
        }

    }
}

// ===================================================================
// Worker-Task
// ===================================================================

/**
 * @brief FreeRTOS Task-Entry
 */
void DayView::workerEntry(void *arg)
{
    auto *self = static_cast<DayView *>(arg);
    if (self)
        self->workerLoop();
}

/**
 * @brief Worker-Hauptschleife: holt APIs sequentiell, sendet nach jedem Fetch
 */
void DayView::workerLoop()
{
    LOG_INFO("DV Worker-Task gestartet");

    constexpr TickType_t WAIT_TICKS = pdMS_TO_TICKS(30000);

    while (!m_shutdownRequested.load())
    {
        ulTaskNotifyTake(pdTRUE, WAIT_TICKS);

        if (m_shutdownRequested.load())
            break;

        const bool forceAll = m_refreshRequested.exchange(false);
        const unsigned long now = millis();

        // Jeder Datentyp hat seinen eigenen Takt; forceAll (Init und Backoff) erzwingt einen kompletten Durchlauf.
        const bool dueWeather = forceAll || (now - m_lastWeatherFetch >= WEATHER_INTERVAL);
        const bool dueCalendar = isUrlConfigured(m_calendarUrl) && (forceAll || (now - m_lastCalendarFetch >= CALENDAR_INTERVAL));
        const bool dueTasks = isUrlConfigured(m_todoUrl) && (forceAll || (now - m_lastTodoFetch >= TODO_INTERVAL));
        const bool dueNews = forceAll || (now - m_lastNewsFetch >= NEWS_INTERVAL);

        if (!dueWeather && !dueCalendar && !dueTasks && !dueNews)
            continue;

        LOG_INFO("DV Worker: Starte Datenabruf");

        // Akkumulierte Daten auf dem Stack
        DayViewData data;
        memset(&data, 0, sizeof(DayViewData));

        // Demo-Modus: alles auf einmal
        if (m_demoMode)
        {
            generateDemoData(&data);
            data.calendarIsDemo = true;
            data.tasksIsDemo = true;
            sendPartialUpdate(&data);

            const unsigned long t = millis();
            m_lastWeatherFetch = t;
            m_lastCalendarFetch = t;
            m_lastTodoFetch = t;
            m_lastNewsFetch = t;
            continue;
        }

        bool anyAttempted = false;
        bool anySuccess = false;
        bool calendarOk = false;
        bool tasksOk = false;
        bool didFetch = false;

        // 1. Wetter (schnellste API -> sofort sichtbar)
        if (dueWeather && !m_shutdownRequested.load())
        {
            if (didFetch)
                pauseBetweenFetches();
            didFetch = true;
            anyAttempted = true;

            const int rc = fetchWeather(&data);
            if (rc == HTTP_CODE_OK)
            {
                data.availableSections |= SECTION_WEATHER;
                sendPartialUpdate(&data);
                anySuccess = true;
                m_lastWeatherFetch = millis();
            }
            else if (rc == ERR_NO_URL)
            {
                m_lastWeatherFetch = millis(); // kein Ort konfiguriert, erst im nächsten Intervall erneut versuchen
            }
            else
            {
                LOG_WARNING_F("DV: Wetter-Fehler %d", rc);
            }
        }

        // 2. Kalender (inkl. Geburtstage)
        if (dueCalendar && !m_shutdownRequested.load())
        {
            if (didFetch)
                pauseBetweenFetches();
            didFetch = true;
            anyAttempted = true;

            const int rc = fetchCalendar(&data);
            if (rc == HTTP_CODE_OK)
            {
                data.availableSections |= SECTION_CALENDAR;
                sendPartialUpdate(&data);
                anySuccess = true;
                calendarOk = true;
                m_lastCalendarFetch = millis();
            }
            else if (rc != ERR_NO_URL)
            {
                LOG_WARNING_F("DV: Kalender-Fehler %d", rc);
            }
        }

        // 3. Aufgaben
        if (dueTasks && !m_shutdownRequested.load())
        {
            if (didFetch)
                pauseBetweenFetches();
            didFetch = true;
            anyAttempted = true;

            const int rc = fetchTasks(&data);
            if (rc == HTTP_CODE_OK)
            {
                data.availableSections |= SECTION_TASKS;
                sendPartialUpdate(&data);
                anySuccess = true;
                tasksOk = true;
                m_lastTodoFetch = millis();
            }
            else if (rc != ERR_NO_URL)
            {
                LOG_WARNING_F("DV: Todo-Fehler %d", rc);
            }
        }

        // 4. Breaking News (Tagesschau Eilmeldung)
        if (dueNews && !m_shutdownRequested.load())
        {
            if (didFetch)
                pauseBetweenFetches();
            didFetch = true;
            anyAttempted = true;

            const int rc = fetchBreakingNews(&data);
            if (rc == HTTP_CODE_OK)
            {
                data.availableSections |= SECTION_NEWS;
                sendPartialUpdate(&data);
                anySuccess = true;
                m_lastNewsFetch = millis();
            }
            else if (rc != ERR_SHUTDOWN)
            {
                LOG_WARNING_F("DV: News-Fehler %d", rc);
            }
        }

        // Per-Sektion Demo-Fallback: nur wenn keine URL konfiguriert ist. Bei temporären Fehlern (URL vorhanden, Fetch fehlgeschlagen) werden keine Demo-Daten eingespeist - die UI behält die vorherigen Daten oder den "Lade..."-Platzhalter bis zum nächsten erfolgreichen Fetch.
        if (!m_shutdownRequested.load())
        {
            bool needDemoUpdate = false;
            const bool needCalDemo = (!isUrlConfigured(m_calendarUrl) && !(data.availableSections & SECTION_CALENDAR));
            const bool needTaskDemo = (!isUrlConfigured(m_todoUrl) && !(data.availableSections & SECTION_TASKS));

            if (needCalDemo || needTaskDemo)
            {
                void *mem = heap_caps_malloc(sizeof(DayViewData), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (!mem)
                    mem = malloc(sizeof(DayViewData));

                if (mem)
                {
                    DayViewData *demoData = static_cast<DayViewData *>(mem);
                    memset(demoData, 0, sizeof(DayViewData));
                    generateDemoData(demoData);

                    if (needCalDemo)
                    {
                        LOG_INFO("DV: Keine Kalender-URL -> Demo-Termine");
                        memcpy(data.events, demoData->events, sizeof(demoData->events));
                        data.eventCount = demoData->eventCount;
                        data.eventTotalCount = demoData->eventTotalCount;
                        memcpy(data.birthdays, demoData->birthdays, sizeof(demoData->birthdays));
                        data.birthdayCount = demoData->birthdayCount;
                        data.birthdayTotalCount = demoData->birthdayTotalCount;
                        data.availableSections |= SECTION_CALENDAR;
                        data.calendarIsDemo = true;
                        needDemoUpdate = true;
                    }

                    if (needTaskDemo)
                    {
                        LOG_INFO("DV: Keine Todo-URL -> Demo-Aufgaben");
                        memcpy(data.tasks, demoData->tasks, sizeof(demoData->tasks));
                        data.taskCount = demoData->taskCount;
                        data.taskTotalCount = demoData->taskTotalCount;
                        data.availableSections |= SECTION_TASKS;
                        data.tasksIsDemo = true;
                        needDemoUpdate = true;
                    }

                    free(demoData);
                }
                else
                {
                    LOG_ERROR("DV: Demo-Allokation fehlgeschlagen");
                }
            }

            if (dueCalendar && !calendarOk)
                LOG_WARNING("DV: Kalender-Fetch fehlgeschlagen, vorherige Daten behalten");

            if (dueTasks && !tasksOk)
                LOG_WARNING("DV: Todo-Fetch fehlgeschlagen, vorherige Daten behalten");

            if (needDemoUpdate)
                sendPartialUpdate(&data);
        }

        // Konsolidiertes Final-Update: falls ein Partial-Send fehlgeschlagen ist, werden hier alle gesammelten Daten nochmal als Ganzes gesendet
        if (data.availableSections != 0 && !m_shutdownRequested.load())
            sendPartialUpdate(&data);

        // Fehler-Handling
        if (m_shutdownRequested.load())
            break;

        if (anySuccess)
        {
            m_consecutiveErrors = 0;
            m_backoffMs = 0;
        }
        else if (anyAttempted)
        {
            m_consecutiveErrors++;

            if (m_backoffMs == 0)
                m_backoffMs = MIN_RETRY_INTERVAL;
            else
                m_backoffMs = std::min(m_backoffMs * 2, MAX_BACKOFF_TIME);

            LOG_WARNING_F("DV: Alle APIs fehlgeschlagen (%d), Backoff: %lu ms", m_consecutiveErrors, m_backoffMs);

            // Interruptibler Backoff: in kleinen Stücken warten, damit Shutdown den Worker jederzeit aufwecken kann
            constexpr unsigned long BACKOFF_SLICE_MS = 500;
            unsigned long remaining = m_backoffMs;

            while (remaining > 0 && !m_shutdownRequested.load())
            {
                const unsigned long slice = std::min(remaining, BACKOFF_SLICE_MS);
                ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(slice));
                remaining -= slice;
            }

            // Nach dem Backoff sofort erneut aufwecken; die noch fälligen (fehlgeschlagenen) Typen werden dann erneut versucht
            if (!m_shutdownRequested.load() && !m_demoMode)
            {
                TaskHandle_t self = m_taskHandle.load();
                if (self)
                    xTaskNotifyGive(self);
            }
        }
    }

    LOG_INFO("DV Worker-Task beendet");
    m_taskHandle.store(nullptr);
    vTaskDelete(nullptr);
}

/**
 * @brief Kurze, abbrechbare Pause zwischen zwei API-Abrufen
 */
void DayView::pauseBetweenFetches()
{
    // ulTaskNotifyTake kehrt vorzeitig zurück, wenn der Worker benachrichtigt wird (z.B. Shutdown)
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(INTER_FETCH_GAP));
}

/**
 * @brief Sendet Kopie der akkumulierten Daten an UI-Queue (by-value)
 */
bool DayView::sendPartialUpdate(const DayViewData *data)
{
    DayViewData copy;
    memcpy(&copy, data, sizeof(DayViewData));
    copy.timestamp = millis();

    if (xQueueSend(m_dataQueue, &copy, pdMS_TO_TICKS(50)) == pdTRUE)
        return true;

    // Queue voll: ältesten Eintrag in copy empfangen, dadurch wird ein Slot frei
    if (xQueueReceive(m_dataQueue, &copy, 0) == pdTRUE)
    {
        memcpy(&copy, data, sizeof(DayViewData));
        copy.timestamp = millis();

        if (xQueueSend(m_dataQueue, &copy, pdMS_TO_TICKS(50)) == pdTRUE)
            return true;
    }

    LOG_WARNING_F("DV: Queue-Send fehlgeschlagen (sections=0x%02X)", data->availableSections);
    return false;
}

// ===================================================================
// DATENABFRAGE - KALENDER
// ===================================================================

/**
 * @brief Holt Kalender-Daten (Events + Geburtstage)
 */
int DayView::fetchCalendar(DayViewData *outData)
{
    if (!m_calendarUrl || strlen(m_calendarUrl) < 5)
        return ERR_NO_URL;

    char url[URL_BUFFER_SIZE];
    stringutils::safeFormat(url, "%s?action=getEvents", m_calendarUrl);

    HttpResponse response = HTTP_SERVICE.getThrottled(url, &m_shutdownRequested);
    if (m_shutdownRequested.load())
        return ERR_SHUTDOWN;

    if (!response.isSuccess())
        return response.httpCode;

    if (!response.hasData())
        return ERR_EMPTY_RESPONSE;

    // JSON-Filter: nur benötigte Felder parsen
    JsonDocument filter;

    JsonObject fDays = filter["upcomingEvents"]["days"][0].to<JsonObject>();
    fDays["isToday"] = true;
    fDays["dayName"] = true;
    fDays["dayOfMonth"] = true;
    fDays["month"] = true;

    JsonObject fEvt = fDays["events"][0].to<JsonObject>();
    fEvt["summary"] = true;
    fEvt["start"] = true;
    fEvt["eventType"] = true;
    fEvt["isAllDay"] = true;
    fEvt["isPast"] = true;

    JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
    if (deserializeJson(doc, response.data, DeserializationOption::Filter(filter)))
        return ERR_JSON_PARSE;

    // Schema-Validierung
    JsonObject upcoming = doc["upcomingEvents"];
    if (!upcoming)
    {
        LOG_WARNING("DV: Kalender-Schema ungültig (kein 'upcomingEvents')");
        return ERR_SCHEMA;
    }

    JsonArray days = upcoming["days"];
    if (!days)
    {
        LOG_WARNING("DV: Kalender-Schema ungültig (kein 'days' Array)");
        return ERR_SCHEMA;
    }

    outData->eventCount = 0;
    outData->eventTotalCount = 0;
    outData->birthdayCount = 0;
    outData->birthdayTotalCount = 0;

    // Letzte Header-Identität als (weekdayIdx, monthIdx, dayOfMonth) merken, isTodayHeader separat - so erkennen wir einen Tageswechsel ohne String-Vergleich.
    int8_t lastWdIdx = -2; // -2 = noch nichts geschrieben (gültige Werte: -1..6)
    int8_t lastMonIdx = -2;
    int8_t lastDayOfMonth = -2;
    bool lastWasToday = false;

    for (JsonObject day : days)
    {
        if (m_shutdownRequested.load())
            return ERR_SHUTDOWN;

        const bool isToday = day["isToday"] | false;
        const char *dayName = day["dayName"] | "";
        const int dayOfMonth = day["dayOfMonth"] | 0;
        const int month = day["month"] | 0;

        // Header-Identität für diesen Tag bestimmen
        int8_t curWdIdx = -1;
        int8_t curMonIdx = -1;
        int8_t curDayOfMonth = -1;
        if (!isToday && dayOfMonth > 0 && month >= 1 && month <= 12)
        {
            curWdIdx = weekdayIdxFromBackendName(dayName);
            curMonIdx = static_cast<int8_t>(month - 1);
            curDayOfMonth = static_cast<int8_t>(dayOfMonth);
        }

        // Hat sich der Tag im Vergleich zur letzten Iteration geändert?
        const bool dayChanged = (isToday != lastWasToday) ||
                                (curWdIdx != lastWdIdx) ||
                                (curMonIdx != lastMonIdx) ||
                                (curDayOfMonth != lastDayOfMonth);

        JsonArray dayEvents = day["events"];
        if (!dayEvents)
            continue;

        bool headerForFirstEventPending = dayChanged;

        for (JsonObject event : dayEvents)
        {
            if (m_shutdownRequested.load())
                return ERR_SHUTDOWN;

            const char *summary = event["summary"];
            const char *start = event["start"];
            const char *evtType = event["eventType"] | "default";
            const bool isAllDay = event["isAllDay"] | false;
            const bool isPast = event["isPast"] | false;

            if (!summary || strlen(summary) == 0)
                continue;

            // Heutige Geburtstage
            if (strcmp(evtType, "birthday") == 0)
            {
                if (isToday)
                {
                    outData->birthdayTotalCount++;
                    if (outData->birthdayCount < DayViewCfg::MAX_BIRTHDAYS)
                    {
                        const char *name = summary;
                        if (strncmp(name, "Geburtstag von ", 15) == 0)
                            name += 15;

                        stringutils::safeCopy(outData->birthdays[outData->birthdayCount].name, name);
                        outData->birthdayCount++;
                    }
                }

                continue;
            }

            // Reguläre Termine
            outData->eventTotalCount++;

            if (outData->eventCount < DayViewCfg::MAX_EVENTS)
            {
                DayViewEvent &evt = outData->events[outData->eventCount];
                stringutils::safeCopy(evt.summary, summary);
                stringutils::safeCopy(evt.time, start ? start : "");
                stringutils::safeCopy(evt.eventType, evtType);
                evt.isAllDay = isAllDay;
                evt.isPast = isPast;
                evt.isToday = isToday;

                // Header nur beim ersten Event eines neuen Tages setzen
                if (headerForFirstEventPending)
                {
                    evt.hasDayHeader = true;
                    evt.isTodayHeader = isToday;
                    evt.weekdayIdx = isToday ? -1 : curWdIdx;
                    evt.monthIdx = isToday ? -1 : curMonIdx;
                    evt.dayOfMonth = isToday ? -1 : curDayOfMonth;
                    headerForFirstEventPending = false;
                }
                else
                {
                    evt.hasDayHeader = false;
                    evt.isTodayHeader = false;
                    evt.weekdayIdx = -1;
                    evt.monthIdx = -1;
                    evt.dayOfMonth = -1;
                }

                outData->eventCount++;
            }
        }

        // Status für nächste Iteration merken
        lastWasToday = isToday;
        lastWdIdx = curWdIdx;
        lastMonIdx = curMonIdx;
        lastDayOfMonth = curDayOfMonth;
    }

    LOG_INFO_F("DV Cal: %d/%d Termine, %d/%d Geburtstage", outData->eventCount, outData->eventTotalCount, outData->birthdayCount, outData->birthdayTotalCount);
    return HTTP_CODE_OK;
}

// ===================================================================
// DATENABFRAGE - AUFGABEN
// ===================================================================

/**
 * @brief Holt offene Aufgaben (erledigte werden gefiltert)
 */
int DayView::fetchTasks(DayViewData *outData)
{
    if (!m_todoUrl || strlen(m_todoUrl) < 5)
        return ERR_NO_URL;

    char url[URL_BUFFER_SIZE];
    stringutils::safeFormat(url, "%s?action=getTasks", m_todoUrl);

    HttpResponse response = HTTP_SERVICE.getThrottled(url, &m_shutdownRequested);
    if (m_shutdownRequested.load())
        return ERR_SHUTDOWN;

    if (!response.isSuccess())
        return response.httpCode;

    if (!response.hasData())
        return ERR_EMPTY_RESPONSE;

    // JSON-Filter: nur benötigte Felder parsen
    JsonDocument filter;
    JsonObject fTask = filter["tasks"][0].to<JsonObject>();
    fTask["title"] = true;
    fTask["isCompleted"] = true;
    fTask["isOverdue"] = true;
    fTask["isDueToday"] = true;

    JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
    if (deserializeJson(doc, response.data, DeserializationOption::Filter(filter)))
        return ERR_JSON_PARSE;

    JsonArray tasks = doc["tasks"];
    if (!tasks)
    {
        LOG_WARNING("DV: Todo-Schema ungültig (kein 'tasks' Array)");
        return ERR_SCHEMA;
    }

    outData->taskCount = 0;
    outData->taskTotalCount = 0;

    for (JsonObject task : tasks)
    {
        // Erledigte Aufgaben nicht anzeigen
        if (task["isCompleted"] | false)
            continue;

        const char *title = task["title"];
        if (!title || strlen(title) == 0)
            continue;

        // Gesamtzahl mitzählen
        outData->taskTotalCount++;

        // Array nur bis MAX_TASKS füllen
        if (outData->taskCount >= DayViewCfg::MAX_TASKS)
            continue;

        DayViewTask &item = outData->tasks[outData->taskCount];
        stringutils::safeCopy(item.title, title);
        item.isOverdue = task["isOverdue"] | false;
        item.isDueToday = task["isDueToday"] | false;
        outData->taskCount++;
    }

    LOG_INFO_F("DV Todo: %d/%d offene Aufgaben", outData->taskCount, outData->taskTotalCount);
    return HTTP_CODE_OK;
}

// ===================================================================
// DATENABFRAGE - WETTER
// ===================================================================

/**
 * @brief Holt aktuelle Wetterdaten von Open-Meteo
 */
int DayView::fetchWeather(DayViewData *outData)
{
    const double lat = APP_STATE.getWeatherLatitude();
    const double lon = APP_STATE.getWeatherLongitude();

    // Koordinaten müssen konfiguriert (nicht NaN) und im zulässigen Bereich sein
    if (std::isnan(lat) || std::isnan(lon) || lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
        return ERR_NO_URL;

    char url[URL_BUFFER_SIZE];
    stringutils::safeFormat(url,
                            "https://api.open-meteo.com/v1/forecast?"
                            "latitude=%.5f&longitude=%.5f"
                            "&current=temperature_2m,apparent_temperature,weather_code,is_day,"
                            "wind_speed_10m,relative_humidity_2m"
                            "&timezone=Europe%%2FBerlin&forecast_days=1",
                            lat, lon);

    HttpResponse response = HTTP_SERVICE.getThrottled(url, &m_shutdownRequested);
    if (m_shutdownRequested.load())
        return ERR_SHUTDOWN;

    if (!response.isSuccess())
        return response.httpCode;

    if (!response.hasData())
        return ERR_EMPTY_RESPONSE;

    // JSON-Filter: nur benötigte Felder parsen
    JsonDocument filter;
    filter["current"]["temperature_2m"] = true;
    filter["current"]["apparent_temperature"] = true;
    filter["current"]["weather_code"] = true;
    filter["current"]["is_day"] = true;
    filter["current"]["wind_speed_10m"] = true;
    filter["current"]["relative_humidity_2m"] = true;

    JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
    if (deserializeJson(doc, response.data, DeserializationOption::Filter(filter)))
        return ERR_JSON_PARSE;

    const JsonObject cur = doc["current"];
    if (!cur)
    {
        LOG_WARNING("DV: Wetter-Schema ungültig (kein 'current' Objekt)");
        return ERR_SCHEMA;
    }

    const float temp = cur["temperature_2m"] | NAN;
    const float feels = cur["apparent_temperature"] | NAN;
    const float wind = cur["wind_speed_10m"] | NAN;
    const int humid = cur["relative_humidity_2m"] | 0;

    // Temperatur: Zehntelgrad für Hauptanzeige, Ganzzahl für Gefühlt/Wind
    outData->weather.temperature = isnan(temp) ? 0.0f : roundf(temp * 10.0f) / 10.0f;
    outData->weather.feelsLike = isnan(feels) ? 0 : lroundf(feels);
    outData->weather.windSpeed = isnan(wind) ? 0 : lroundf(wind);
    outData->weather.humidity = humid;

    outData->weather.weatherCode = cur["weather_code"] | 0;
    outData->weather.isDay = (cur["is_day"] | 1) == 1;

    LOG_INFO_F("DV Wetter: %.1f °C, WMO %d, Wind %d km/h", outData->weather.temperature, outData->weather.weatherCode, outData->weather.windSpeed);
    return HTTP_CODE_OK;
}

// ===================================================================
// DATENABFRAGE - Tagesschau Eilmeldungen
// ===================================================================

/**
 * @brief Holt aktuelle Meldungen von der Tagesschau API
 *
 * Nutzt den offiziellen /api2u/news Endpoint (max. 60 Abrufe/Stunde). Stufen (erste Übereinstimmung gewinnt):
 *   1a. Top-Level-breakingNews-Array -> newsIsBreaking = true
 *   1b. Boolean-Flag / sophoraId / topline in news[]-Items -> newsIsBreaking = true
 *   2.  Kein Eilmeldungs-Kriterium -> news[0] als Top-Meldung -> newsIsBreaking = false
 */
int DayView::fetchBreakingNews(DayViewData *outData)
{
    // Test-Modus: Fake-Eilmeldung beim ersten Aufruf
    if (NEWS_TEST_MODE)
    {
        m_newsTestCycle++;
        if (m_newsTestCycle == 1)
        {
            stringutils::safeCopy(outData->newsHeadline,
                                  "Internationaler Astronomie-Kongress meldet Entdeckung eines erdähnlichen Planeten in nur 12 Lichtjahren Entfernung"
                                  " - Wissenschaftler sprechen von einem historischen Durchbruch in der Exoplanetenforschung");

            stringutils::safeCopy(outData->newsId, "test-eilmeldung-001");
            outData->newsAvailable = true;
            outData->newsIsBreaking = true;
            LOG_WARNING("DV News TEST: Fake-Eilmeldung injiziert");
            return HTTP_CODE_OK;
        }
    }

    char newsUrl[160];
    const char *ressort = APP_STATE.getNewsRessort();
    if (ressort && ressort[0] != '\0')
        stringutils::safeFormat(newsUrl, "%s?regions=%d&ressort=%s", TAGESSCHAU_API_URL, APP_STATE.getNewsRegion(), ressort);
    else
        stringutils::safeFormat(newsUrl, "%s?regions=%d", TAGESSCHAU_API_URL, APP_STATE.getNewsRegion());

    HttpResponse response = HTTP_SERVICE.getCompressed(newsUrl, &m_shutdownRequested);
    if (m_shutdownRequested.load())
        return ERR_SHUTDOWN;

    if (!response.isSuccess())
        return response.httpCode;

    if (!response.hasData())
        return ERR_EMPTY_RESPONSE;

    // JSON-Filter: nur die für die Erkennung nötigen Felder
    JsonDocument filter;
    filter["breakingNews"][0]["headline"] = true;
    filter["breakingNews"][0]["id"] = true;
    filter["news"][0]["breakingNews"] = true;
    filter["news"][0]["title"] = true;
    filter["news"][0]["sophoraId"] = true;
    filter["news"][0]["topline"] = true;
    filter["news"][0]["shareURL"] = true;   // Bevorzugte Artikel-URL
    filter["news"][0]["detailsweb"] = true; // Fallback wenn shareURL fehlt/generisch

    JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
    if (deserializeJson(doc, response.data, DeserializationOption::Filter(filter)))
        return ERR_JSON_PARSE;

    // Stufe 1a: Top-Level-breakingNews-Array
    JsonArray breakingArr = doc["breakingNews"];
    if (breakingArr)
    {
        for (JsonObject item : breakingArr)
        {
            const char *headline = item["headline"] | "";
            if (headline[0] == '\0')
                continue;

            const char *id = item["id"] | "";
            char derivedId[64];
            if (id[0] != '\0')
                stringutils::safeCopy(derivedId, id);
            else
                stringutils::safeCopy(derivedId, sizeof(derivedId), headline);

            stringutils::safeCopy(outData->newsHeadline, headline);
            stringutils::safeCopy(outData->newsId, derivedId);
            outData->newsShareUrl[0] = '\0'; // breakingNews-Array liefert keine URL-Felder
            outData->newsAvailable = true;
            outData->newsIsBreaking = true;

            LOG_INFO_F("DV News: Eilmeldung (breakingNews-Array): \"%s\"", headline);
            return HTTP_CODE_OK;
        }
    }

    // Stufe 1b: Boolean-Flag / sophoraId / topline in news[]-Items
    JsonArray newsArr = doc["news"];
    if (!newsArr)
    {
        LOG_WARNING("DV News: Kein 'news' Array im Schema");
        return ERR_SCHEMA;
    }

    for (JsonObject item : newsArr)
    {
        const bool flagBreaking = item["breakingNews"] | false;
        const char *id = item["sophoraId"] | "";
        const char *topline = item["topline"] | "";
        const bool idIsEilmeldung = (strstr(id, "eilmeldung") != nullptr);
        const bool toplineIsEilmeldung = (strstr(topline, "Eilmeldung") != nullptr);

        if (flagBreaking || idIsEilmeldung || toplineIsEilmeldung)
        {
            const char *title = item["title"] | "";
            if (title[0] == '\0')
                continue;

            stringutils::safeCopy(outData->newsHeadline, title);
            stringutils::safeCopy(outData->newsId, id);
            stringutils::safeCopy(outData->newsShareUrl, pickArticleUrl(item));
            outData->newsAvailable = true;
            outData->newsIsBreaking = true;

            LOG_INFO_F("DV News: Eilmeldung (news-Flag) [flag=%d, id=%d, topline=%d]: \"%s\"", flagBreaking, idIsEilmeldung, toplineIsEilmeldung, title);
            return HTTP_CODE_OK;
        }
    }

    // Stufe 2: Keine Eilmeldung -> Top-Meldung (news[0])
    JsonObject top1 = newsArr[0];
    if (top1)
    {
        const char *title = top1["title"] | "";
        const char *id = top1["sophoraId"] | "";

        if (title[0] != '\0')
        {
            char top1Id[64];
            if (id[0] != '\0')
                stringutils::safeFormat(top1Id, "top1:%s", id);
            else
                stringutils::safeCopy(top1Id, sizeof(top1Id), title);

            stringutils::safeCopy(outData->newsHeadline, title);
            stringutils::safeCopy(outData->newsId, top1Id);
            stringutils::safeCopy(outData->newsShareUrl, pickArticleUrl(top1));
            outData->newsAvailable = true;
            outData->newsIsBreaking = false;

            LOG_INFO_F("DV News: Top-Meldung: \"%s\"", title);
            return HTTP_CODE_OK;
        }
    }

    // Keine Meldung verfügbar
    outData->newsAvailable = false;
    outData->newsIsBreaking = false;
    outData->newsHeadline[0] = '\0';
    outData->newsId[0] = '\0';
    outData->newsShareUrl[0] = '\0';
    return HTTP_CODE_OK;
}

// ===================================================================
// DEMO-DATEN
// ===================================================================

/**
 * @brief Generiert Demo-Daten mit korrekten Datumsangaben
 *
 * Ereignis- und Aufgabentitel sowie Fälligkeitsbeschriftungen liegen in zwei Sprachtabellen vor. Die Auswahl erfolgt zur Generierungszeit über I18n::currentLang(), sodass redrawAll() bei einem Sprachwechsel die Demo-Daten in der neuen Sprache erzeugen kann.
 */
void DayView::generateDemoData(DayViewData *outData)
{
    const bool en = (I18n::currentLang() == LangIndex::EN);

    // ===================================================================
    // Demo-Texte (Inhalt sprachabhängig, Reihenfolge identisch)
    // ===================================================================
    static const char *const DEMO_EVT_SUMMARY_DE[10] = {
        "Teammeeting Labor",
        "Review Messdaten",
        "Seminar: Detektorphysik",
        "Homeoffice",
        "Sprint Planning",
        "Code-Review ESP32",
        "Standup Detektor-Team",
        "Workshop Sensorik",
        "Brückentag",
        "Aquafitness"};
    static const char *const DEMO_EVT_SUMMARY_EN[10] = {
        "Team meeting lab",
        "Review measurement data",
        "Seminar: detector physics",
        "Home office",
        "Sprint planning",
        "ESP32 code review",
        "Standup detector team",
        "Sensor workshop",
        "Bank holiday bridge day",
        "Aquafitness"};

    static const char *const DEMO_TASK_TITLE_DE[10] = {
        "Doku Firmware-Update",
        "Bestellung Bauteile Reichelt",
        "PR Review #42 - MQTT Refactoring",
        "Testprotokoll Sensorkalibrierung",
        "Schaltplan Eagle aktualisieren",
        "Laborbericht Q1 fertigstellen",
        "3D-Druck Gehäuse bestellen",
        "Review Meeting-Notizen archivieren",
        "CI/CD Pipeline ESP32 einrichten",
        "Inventur Elektroniklager vorbereiten"};
    static const char *const DEMO_TASK_TITLE_EN[10] = {
        "Doc firmware update",
        "Order Reichelt parts",
        "PR review #42 - MQTT refactoring",
        "Test report sensor calibration",
        "Update Eagle schematic",
        "Finish Q1 lab report",
        "Order 3D printed enclosure",
        "Archive meeting notes",
        "Set up ESP32 CI/CD pipeline",
        "Prepare lab inventory"};

    static const char *const DEMO_BIRTHDAY_DE[2] = {
        "Nina Musterfrau", "Max Mustermann"};
    static const char *const DEMO_BIRTHDAY_EN[2] = {
        "Jane Smith", "John Doe"};

    const char *const *EVT_SUMMARY = en ? DEMO_EVT_SUMMARY_EN : DEMO_EVT_SUMMARY_DE;
    const char *const *TASK_TITLE = en ? DEMO_TASK_TITLE_EN : DEMO_TASK_TITLE_DE;
    const char *const *BIRTHDAY_NAME = en ? DEMO_BIRTHDAY_EN : DEMO_BIRTHDAY_DE;

    // ===================================================================
    // Sprachneutrale Meta-Felder pro Demo-Event
    // ===================================================================
    struct DemoEventMeta
    {
        int dayOffset;    // 0=heute, 1=morgen, 2=in 2 Tagen, 4=in 4 Tagen
        const char *time; // "HH:MM" oder "" bei allDay
        const char *type; // "default" oder "holiday"
        bool isAllDay;
        bool isPast;
    };

    static constexpr DemoEventMeta DEMO_EVENTS[10] = {
        // Heute (3 Termine, idx 0 trägt den "Heute"-Header)
        {0, "09:00", "default", false, true},
        {0, "11:30", "default", false, false},
        {0, "14:00", "default", false, false},
        // Morgen (3 Termine, idx 3 trägt den Datums-Header)
        {1, "", "default", true, false},
        {1, "10:00", "default", false, false},
        {1, "16:00", "default", false, false},
        // Übermorgen (2 Termine, idx 6 trägt den Datums-Header)
        {2, "09:30", "default", false, false},
        {2, "15:00", "default", false, false},
        // In 4 Tagen (2 Termine, idx 8 trägt den Datums-Header)
        {4, "", "holiday", true, false},
        {4, "19:00", "default", false, false},
    };

    // Erste Demo-Event-Indizes pro Tag (an diesen Indizes wird der Header gesetzt)
    static constexpr bool DEMO_HEADER_AT[10] = {
        true, false, false, // Tag 0
        true, false, false, // Tag 1
        true, false,        // Tag 2
        true, false         // Tag 4
    };

    // Aktuelle Zeit als Basis für die Header-Datumsfelder
    time_t now = time(nullptr);
    struct tm tmBase;
    localtime_r(&now, &tmBase);

    outData->eventCount = 10;
    outData->eventTotalCount = 14; // 4 weitere nicht angezeigte -> Badge

    for (int i = 0; i < 10; ++i)
    {
        DayViewEvent &evt = outData->events[i];
        const DemoEventMeta &meta = DEMO_EVENTS[i];

        stringutils::safeCopy(evt.time, meta.time);
        stringutils::safeCopy(evt.summary, EVT_SUMMARY[i]);
        stringutils::safeCopy(evt.eventType, meta.type);
        evt.isAllDay = meta.isAllDay;
        evt.isPast = meta.isPast;
        evt.isToday = (meta.dayOffset == 0);

        if (DEMO_HEADER_AT[i])
        {
            evt.hasDayHeader = true;
            if (meta.dayOffset == 0)
            {
                evt.isTodayHeader = true;
                evt.weekdayIdx = -1;
                evt.monthIdx = -1;
                evt.dayOfMonth = -1;
            }
            else
            {
                struct tm tmDay = tmBase;
                tmDay.tm_mday += meta.dayOffset;
                mktime(&tmDay); // Normalisiert Überlauf
                evt.isTodayHeader = false;
                evt.weekdayIdx = static_cast<int8_t>(tmDay.tm_wday % 7);
                evt.monthIdx = static_cast<int8_t>(tmDay.tm_mon % 12);
                evt.dayOfMonth = static_cast<int8_t>(tmDay.tm_mday);
            }
        }
        else
        {
            evt.hasDayHeader = false;
            evt.isTodayHeader = false;
            evt.weekdayIdx = -1;
            evt.monthIdx = -1;
            evt.dayOfMonth = -1;
        }
    }

    // Geburtstage (mehrere, für Komma-Darstellung)
    outData->birthdayCount = 2;
    outData->birthdayTotalCount = 2;
    stringutils::safeCopy(outData->birthdays[0].name, BIRTHDAY_NAME[0]);
    stringutils::safeCopy(outData->birthdays[1].name, BIRTHDAY_NAME[1]);

    // ===================================================================
    // Aufgaben (10 gespeichert, Gesamtzahl 13 -> Badge "+3 weitere")
    // ===================================================================
    static constexpr bool DEMO_TASK_OVERDUE[10] = {
        false, false, true, false, false, true, false, false, false, false};
    static constexpr bool DEMO_TASK_TODAY[10] = {
        true, false, false, true, false, false, false, false, false, false};

    const int storedTasks = std::min(10, DayViewCfg::MAX_TASKS);
    for (int i = 0; i < storedTasks; ++i)
    {
        stringutils::safeCopy(outData->tasks[i].title, TASK_TITLE[i]);
        outData->tasks[i].isOverdue = DEMO_TASK_OVERDUE[i];
        outData->tasks[i].isDueToday = DEMO_TASK_TODAY[i];
    }

    // Gesamtzahl höher als gespeichert -> erzwingt Badge
    outData->taskCount = storedTasks;
    outData->taskTotalCount = 13;

    // Wetter (Beschreibung wird zur Zeichenzeit aus weatherCode aufgelöst)
    outData->weather.temperature = 7.3f;
    outData->weather.feelsLike = 5;
    outData->weather.windSpeed = 17;
    outData->weather.humidity = 82;
    outData->weather.weatherCode = 2;
    outData->weather.isDay = true;

    outData->availableSections = SECTION_ALL;
    LOG_INFO("DV: Demo-Daten generiert");
}

// ===================================================================
// UI - LAYOUT (einmalig)
// ===================================================================

/**
 * @brief Zeichnet die vertikale Trennlinie zwischen den Sektions-Spalten
 *
 * Wird sowohl beim Erstaufbau in drawLayout() als auch bei redrawAll() (Sprachwechsel) aufgerufen. Die Sektions-Überschriften zeichnen die Sektions-Sprites selbst.
 */
void DayView::drawStaticLabels()
{
    // Vertikale Trennlinie zwischen Spalten (endet an Badge-Unterkante)
    const int vDivX = COL_LEFT_X + COL_LEFT_W + COL_GAP / 2;
    m_tft->drawFastVLine(vDivX, CONTENT_Y, CONTENT_H - BADGE_MARGIN_BOTTOM, COLOR_DIVIDER);
}

/**
 * @brief Zeichnet statisches Layout
 */
void DayView::drawLayout()
{
    drawStaticLabels();

    // Buttons zeichnen
    m_exitButton.drawButton(false);

    // Radio-Buttons initialisieren
    constexpr uint16_t RB_FILL = static_cast<uint16_t>(TFT_BLACK);
    constexpr uint16_t RB_TEXT = static_cast<uint16_t>(TFT_DARKGREY);

    // Favoriten-Button
    m_radioFavBtn.initButton(
        m_tft,              // Display-Objekt
        RADIO_FAV_X,        // X-Position in Buttonmitte
        RADIO_BTN_Y,        // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,     // Breite des Buttons
        RADIO_BTN_SIZE,     // Höhe des Buttons
        RADIO_BTN_R,        // Eckenradius
        RADIO_BORDER_COLOR, // Rahmenfarbe
        RB_FILL,            // Füllfarbe
        RB_TEXT,            // Textfarbe
        "^STAR:32:FC60^",   // Beschriftung
        nullptr             // kein Icon
    );

    // PresetManager-Dialog erstellen
    m_presetDlg = std::make_unique<PresetManager>(m_tft);

    // StationPicker-Dialog erstellen (Sender-Direktwahl)
    m_stationPicker = std::make_unique<StationPicker>(m_tft);

    // "Sender zurück" Button
    m_radioPrevBtn.initButton(
        m_tft,              // Display-Objekt
        RADIO_PREV_X,       // X-Position in Buttonmitte
        RADIO_BTN_Y,        // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,     // Breite des Buttons
        RADIO_BTN_SIZE,     // Höhe des Buttons
        RADIO_BTN_R,        // Eckenradius
        RADIO_BORDER_COLOR, // Rahmenfarbe
        RB_FILL,            // Füllfarbe
        RB_TEXT,            // Textfarbe
        "^BCKW:32:7BEF^",   // Beschriftung
        nullptr             // kein Icon
    );

    // Abspielen Button
    m_radioPlayBtn.initButton(
        m_tft,              // Display-Objekt
        RADIO_PLAY_X,       // X-Position in Buttonmitte
        RADIO_BTN_Y,        // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,     // Breite des Buttons
        RADIO_BTN_SIZE,     // Höhe des Buttons
        RADIO_BTN_R,        // Eckenradius
        RADIO_BORDER_COLOR, // Rahmenfarbe
        RB_FILL,            // Füllfarbe
        RB_TEXT,            // Textfarbe
        "^PLAY:32:5C44^",   // Beschriftung
        nullptr             // kein Icon
    );

    // "Sender vor" Button
    m_radioNextBtn.initButton(
        m_tft,              // Display-Objekt
        RADIO_NEXT_X,       // X-Position in Buttonmitte
        RADIO_BTN_Y,        // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,     // Breite des Buttons
        RADIO_BTN_SIZE,     // Höhe des Buttons
        RADIO_BTN_R,        // Eckenradius
        RADIO_BORDER_COLOR, // Rahmenfarbe
        RB_FILL,            // Füllfarbe
        RB_TEXT,            // Textfarbe
        "^FORW:32:7BEF^",   // Beschriftung
        nullptr             // kein Icon
    );

    // "Lautstärke verringern" Button
    m_volDownBtn.initButton(
        m_tft,              // Display-Objekt
        VOL_DOWN_X,         // X-Position in Buttonmitte
        RADIO_BTN_Y,        // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,     // Breite des Buttons
        RADIO_BTN_SIZE,     // Höhe des Buttons
        RADIO_BTN_R,        // Eckenradius
        RADIO_BORDER_COLOR, // Rahmenfarbe
        RB_FILL,            // Füllfarbe
        RB_TEXT,            // Textfarbe
        "^MINUS:32:7BEF^",  // Beschriftung
        nullptr             // kein Icon
    );

    // "Lautstärke erhöhen" Button
    m_volUpBtn.initButton(
        m_tft,              // Display-Objekt
        VOL_UP_X,           // X-Position in Buttonmitte
        RADIO_BTN_Y,        // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,     // Breite des Buttons
        RADIO_BTN_SIZE,     // Höhe des Buttons
        RADIO_BTN_R,        // Eckenradius
        RADIO_BORDER_COLOR, // Rahmenfarbe
        RB_FILL,            // Füllfarbe
        RB_TEXT,            // Textfarbe
        "^PLUS:32:7BEF^",   // Beschriftung
        nullptr             // kein Icon
    );

    // DotMatrix für Lautstärke-Anzeige (2 Ziffern)
    constexpr uint16_t DOT_ON_COLOR = 0x7BEF;      // leuchtende Segmente (neutrales Grau)
    constexpr uint16_t DOT_ON_SHADOW = TFT_BLACK;  // Schatten
    constexpr uint16_t DOT_OFF_COLOR = 0x2104;     // dunkle Segmente
    constexpr uint16_t DOT_OFF_SHADOW = TFT_BLACK; // Schatten

    m_volDotMatrix.init(VOL_DOT_W, VOL_DOT_H, VOL_DOT_SPACE, VOL_CHAR_SPACE);
    m_volDotMatrix.setOnColor(DOT_ON_COLOR, DOT_ON_SHADOW);
    m_volDotMatrix.setOffColor(DOT_OFF_COLOR, DOT_OFF_SHADOW);
    m_volDotMatrix.setBackgroundColor(TFT_BLACK);
    m_volDotMatrix.setShowOffDots(true);
    m_volDotMatrix.setGradientEnabled(false);
    m_volDotMatrix.setDotShape(DotMatrix::DotShape::SQUARE);
    m_volDotMatrix.reserveText(4);

    // Sprite für Volume-DotMatrix
    if (!m_tft->safeCreateSprite(m_volSprite, VOL_DISP_W, VOL_DISP_H))
        LOG_WARNING("DV: Volume-Sprite fehlgeschlagen");

    // DotMatrix für die zweistellige Senderindex-Anzeige
    m_stnDotMatrix.init(STN_DOT_W, STN_DOT_H, STN_DOT_SPACE, STN_CHAR_SPACE);
    m_stnDotMatrix.setOnColor(DOT_ON_COLOR, DOT_ON_SHADOW);
    m_stnDotMatrix.setOffColor(DOT_OFF_COLOR, DOT_OFF_SHADOW);
    m_stnDotMatrix.setBackgroundColor(TFT_BLACK);
    m_stnDotMatrix.setShowOffDots(true);
    m_stnDotMatrix.setGradientEnabled(false);
    m_stnDotMatrix.setDotShape(DotMatrix::DotShape::SQUARE);
    m_stnDotMatrix.reserveText(4);

    // Sprite für Sender-Index DotMatrix
    if (!m_tft->safeCreateSprite(m_stnSprite, STN_DISP_W, STN_DISP_H))
        LOG_WARNING("DV: Station-Sprite fehlgeschlagen");

    // ===================================================================
    // SD-Player Buttons
    // ===================================================================

    // FileExplorer erstellen
    m_fileExplorer = std::make_unique<FileExplorer>(m_tft);

    // MessageBox erstellen
    m_messageBox = std::make_unique<MessageBox>(m_tft);

    // Browse-Button (Ordner-Symbol)
    m_sdBrowseBtn.initButton(
        m_tft,              // Display-Objekt
        SD_BROWSE_X,        // X-Position in Buttonmitte
        RADIO_BTN_Y,        // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,     // Breite des Buttons
        RADIO_BTN_SIZE,     // Höhe des Buttons
        RADIO_BTN_R,        // Eckenradius
        SD_BORDER_COLOR,    // Rahmenfarbe
        RB_FILL,            // Füllfarbe
        RB_TEXT,            // Textfarbe
        "^FOLDER:32:7BEF^", // Beschriftung
        nullptr             // kein Icon
    );

    // SD Play/Stop Button (anfangs Play)
    m_sdPlayBtn.initButton(
        m_tft,            // Display-Objekt
        SD_PLAY_X,        // X-Position in Buttonmitte
        RADIO_BTN_Y,      // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,   // Breite des Buttons
        RADIO_BTN_SIZE,   // Höhe des Buttons
        RADIO_BTN_R,      // Eckenradius
        SD_BORDER_COLOR,  // Rahmenfarbe
        RB_FILL,          // Füllfarbe
        RB_TEXT,          // Textfarbe
        "^PLAY:32:5C44^", // Beschriftung
        nullptr           // kein Icon
    );

    // SD Next Button
    m_sdNextBtn.initButton(
        m_tft,            // Display-Objekt
        SD_NEXT_X,        // X-Position in Buttonmitte
        RADIO_BTN_Y,      // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,   // Breite des Buttons
        RADIO_BTN_SIZE,   // Höhe des Buttons
        RADIO_BTN_R,      // Eckenradius
        SD_BORDER_COLOR,  // Rahmenfarbe
        RB_FILL,          // Füllfarbe
        RB_TEXT,          // Textfarbe
        "^FORW:32:7BEF^", // Beschriftung
        nullptr           // kein Icon
    );

    // SD Clear/Delete Button
    m_sdClearBtn.initButton(
        m_tft,           // Display-Objekt
        SD_CLEAR_X,      // X-Position in Buttonmitte
        RADIO_BTN_Y,     // Y-Position in Buttonmitte
        RADIO_BTN_SIZE,  // Breite des Buttons
        RADIO_BTN_SIZE,  // Höhe des Buttons
        RADIO_BTN_R,     // Eckenradius
        SD_BORDER_COLOR, // Rahmenfarbe
        RB_FILL,         // Füllfarbe
        RB_TEXT,         // Textfarbe
        "^DEL:32:7BEF^", // Beschriftung
        nullptr          // kein Icon
    );

    // Initiales Zeichnen der Radio-Bar + Info-Bar
    drawRadioBar();
    drawBottomSection();
}

// ===================================================================
// VOLLREDRAW (nach Sprachwechsel)
// ===================================================================

/**
 * @brief Zeichnet die komplette Anzeige neu (Bildschirm leeren + alle Sektionen)
 *
 * Wird bei einem Sprachwechsel aufgerufen, damit alle statischen Labels und gerenderten Sprites mit den neuen Texten überschrieben werden. Die Buttons werden nicht neu initialisiert, nur die zugehörigen Sprites/Labels neu gezeichnet. Im Demo-Modus werden auch die Demo-Daten in der gerade aktiven Sprache neu erzeugt.
 */
void DayView::redrawAll()
{
    if (!m_tft)
        return;

    m_tft->clear(Theme::ColorMainBackGround);

    drawStaticLabels();
    m_exitButton.drawButton(false);

    // Demo-Daten in der jetzt aktiven Sprache neu erzeugen
    if (m_demoMode && m_currentData)
        generateDemoData(m_currentData);

    // Header-Sprite hat keinen eigenen Change-Detection-Mechanismus, nur die Minute. Zurücksetzen erzwingt das Neuzeichnen im nächsten draw()-Frame.
    m_lastDrawnMinute = -1;
    drawHeaderSection();

    // Sektionen nur zeichnen, wenn Daten vorliegen, sonst Lade-Hinweis
    if (m_currentData && (m_currentData->availableSections & SECTION_CALENDAR))
        drawEventsSection();
    else
        drawSectionLoading(m_eventsSprite, EVENTS_SPR_W, EVENTS_SPR_H, COL_LEFT_X, CONTENT_Y, reinterpret_cast<const char *>(STR(DayViewKey::DAYVIEW_LOADING_EVENTS)));

    if (m_currentData && (m_currentData->availableSections & SECTION_TASKS))
        drawTasksSection();
    else
        drawSectionLoading(m_tasksSprite, TASKS_SPR_W, TASKS_SPR_H, TASKS_SPR_X, CONTENT_Y, reinterpret_cast<const char *>(STR(DayViewKey::DAYVIEW_LOADING_TASKS)));

    // Bottom + Radio-Bar: erzwinge Neuzeichnung
    m_lastRadioStation = 0xFF;
    m_lastVolume = 0xFF;
    drawRadioBar();
    drawBottomSection();

    // Offenes QR-Overlay wiederherstellen (Sektions-Sprites übermalen den Bereich)
    m_qrCodeOverlay.redraw();
}

// ===================================================================
// HEADER-SEKTION
// ===================================================================

/**
 * @brief Zeichnet Uhr, Ort, Datum, Temperatur und Wetter-Details in Header-Sprite
 */
void DayView::drawHeaderSection()
{
    if (!m_headerSprite)
        return;

    auto *spr = m_headerSprite;
    spr->clear(Theme::ColorMainBackGround);

    // Systemzeit einmalig holen und auf Plausibilität prüfen. Ohne NTP/gepufferte RTC wird der Platzhalter angezeigt.
    const bool timeValid = DS1307_RTC::isSystemTimeValid();
    const struct tm lt = DS1307_RTC::getCurrentLocalTime();

    // Uhrzeit "HH:MM" als DotMatrix (oder Platzhalter)
    char timeBuf[8];
    if (timeValid)
        stringutils::safeFormat(timeBuf, "%02d:%02d", lt.tm_hour, lt.tm_min);
    else
        stringutils::safeCopy(timeBuf, "--:--");

    m_clockDotMatrix.setText(timeBuf);
    m_clockDotMatrix.setShowOffDots(false);
    m_clockDotMatrix.setGradientEnabled(false);
    m_clockDotMatrix.draw(spr, 0, HDR_CLOCK_Y);

    // Ort
    spr->setFont(&fonts::Orbitron_Light_24);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(COLOR_HEADER, Theme::ColorMainBackGround);

    const char *location = APP_STATE.getWeatherLocationDirect();
    if (location && *location)
        spr->drawString(location, m_headerTextX, 0);

    // Datum (oder Platzhalter)
    spr->setTextColor(COLOR_DIM, Theme::ColorMainBackGround);
    char dateBuf[48];
    if (timeValid)
    {
        char wdBuf[16];
        char monBuf[16];
        I18n::copyWeekday(wdBuf, sizeof(wdBuf), lt.tm_wday % 7, /*longForm=*/false);
        I18n::copyMonth(monBuf, sizeof(monBuf), lt.tm_mon % 12, /*longForm=*/false);
        STRF(dateBuf, DayViewKey::DAYVIEW_DATE_HEADER_FMT, wdBuf, monBuf, lt.tm_mday, lt.tm_year + 1900);
    }
    else
    {
        STRF(dateBuf, CommonKey::COMMON_DT_PLACEHOLDER_DATE);
    }

    spr->drawString(dateBuf, m_headerTextX, 28);

    // Wetter-Daten
    if (m_currentData && (m_currentData->availableSections & SECTION_WEATHER))
    {
        const DayViewWeather &w = m_currentData->weather;

        // Große Temperatur als DotMatrix
        char tempStr[16];
        if (w.temperature <= -10.0f || w.temperature >= 100.0f)
            stringutils::safeFormat(tempStr, "%d°", static_cast<int>(w.temperature));
        else
            stringutils::safeFormat(tempStr, "%.1f°", w.temperature);

        m_tempDotMatrix.setText(tempStr);
        m_tempDotMatrix.setShowOffDots(false);
        m_tempDotMatrix.setGradientEnabled(false);
        m_tempDotMatrix.draw(spr, 0, HDR_TEMP_Y);

        // Details
        const int detailX1 = m_headerTextX;
        const int detailX2 = detailX1 + DETAIL_COL_GAP;
        constexpr int DETAIL_Y1 = HDR_TEMP_Y + 10;
        constexpr int DETAIL_Y2 = DETAIL_Y1 + 21; // 26;
        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(TL_DATUM);
        char buf[32];
        { // Zeile 1
            STRF(buf, DayViewKey::DAYVIEW_WEATHER_FEELS_LIKE, w.feelsLike);
            spr->setTextColor(COLOR_DIM, Theme::ColorMainBackGround);
            spr->drawString(buf, detailX1, DETAIL_Y1);
            STRF(buf, DayViewKey::DAYVIEW_WEATHER_HUMIDITY, w.humidity);
            spr->setTextColor(COLOR_DIM, Theme::ColorMainBackGround);
            spr->drawString(buf, detailX2, DETAIL_Y1);
        }
        { // Zeile 2
            STRF(buf, DayViewKey::DAYVIEW_WEATHER_WIND, w.windSpeed);
            spr->setTextColor(COLOR_TEXT, Theme::ColorMainBackGround);
            spr->drawString(buf, detailX1, DETAIL_Y2);
            spr->setTextColor(COLOR_TEXT, Theme::ColorMainBackGround);
            spr->drawString(STR(wmoToKey(w.weatherCode)), detailX2, DETAIL_Y2);
        }
        spr->unloadFont();

        // Vertikaler 1px-Trenner zwischen Detail-Spalten
        const int sepX = detailX2 - DETAIL_SEP_PAD;
        spr->drawFastVLine(sepX, DETAIL_Y1, DETAIL_Y2 + 16 - DETAIL_Y1, COLOR_DETAIL_SEP);

        // Wetter-Icon
        drawWeatherIcon(w.weatherCode, w.isDay, ICON_SPR_X, ICON_SPR_Y, ICON_SIZE, COLOR_WEATHER_ICN, spr);
    }

    // Atomar auf Display pushen
    spr->pushSprite(LEFT_MARGIN, HEADER_Y);

    // Minute merken
    m_lastDrawnMinute = lt.tm_hour * 60 + lt.tm_min;
}

// ===================================================================
// UI - LADEHINWEIS
// ===================================================================

/**
 * @brief Zeichnet zentrierten Ladehinweis in ein Sektions-Sprite
 */
void DayView::drawSectionLoading(Gfx_Sprite *spr, int sprW, int sprH, int displayX, int displayY, const char *text)
{
    if (!spr)
        return;

    spr->clear(Theme::ColorMainBackGround);
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(COLOR_DIM, Theme::ColorMainBackGround);
    spr->drawString(text, sprW / 2, sprH / 2);
    spr->unloadFont();
    spr->pushSprite(displayX, displayY);
}

// ===================================================================
// UI - TERMINE
// ===================================================================

/**
 * @brief Zeichnet Kalender-Termine links (Calendar-Stil mit Farbbalken)
 */
void DayView::drawEventsSection()
{
    if (!m_currentData || !m_eventsSprite)
        return;

    // Sprite als Zeichenziel setzen und Hintergrund füllen
    auto *spr = m_eventsSprite;
    spr->clear(Theme::ColorMainBackGround);

    // Label (ggf. mit DEMO-Suffix)
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(COLOR_ACCENT, Theme::ColorMainBackGround);
    spr->drawString(STR(m_currentData->calendarIsDemo
                            ? DayViewKey::DAYVIEW_LABEL_EVENTS_DEMO
                            : DayViewKey::DAYVIEW_LABEL_EVENTS),
                    0, 0);
    spr->unloadFont();

    constexpr int startY = 18; // Platz für Label

    if (m_currentData->eventCount == 0)
    {
        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(TL_DATUM);
        spr->setTextColor(COLOR_DIM, Theme::ColorMainBackGround);
        spr->drawString(STR(DayViewKey::DAYVIEW_NO_EVENTS), 0, startY + 8);
        spr->unloadFont();
        spr->pushSprite(COL_LEFT_X, CONTENT_Y);
        return;
    }

    // Layout-Konstanten
    constexpr int EVENT_BAR_W = 4;       ///< Breite des Farbbalkens
    constexpr int EVENT_BAR_MARGIN = 10; ///< Abstand links zum Balken
    constexpr int EVENT_BAR_H = 15;      ///< Höhe des Farbbalkens
    constexpr int TEXT_OFFSET = 8;       ///< Abstand Balken -> Text
    constexpr int TIME_COL_W = 50;       ///< Feste Breite für Zeitangabe
    constexpr int LINE_H = 20;           ///< Zeilenhöhe pro Event
    constexpr int DAY_HEADER_PAD = 4;    ///< Padding über Tages-Header
    constexpr int DAY_HEADER_H = 16;     ///< Höhe des Tages-Headers
    constexpr int SEPARATOR_PAD = 2;     ///< Padding um Trennlinien

    constexpr int barX = EVENT_BAR_MARGIN;                                                         ///< X-Position des Farbbalkens
    constexpr int textX = barX + EVENT_BAR_W + TEXT_OFFSET;                                        ///< X-Position des Texts (Zeit + Zusammenfassung)
    constexpr int availableW = EVENTS_SPR_W - (EVENT_BAR_MARGIN + EVENT_BAR_W + TEXT_OFFSET) - 10; ///< Verfügbare Breite für Zeit + Zusammenfassung (inkl. etwas Puffer rechts)
    constexpr int maxY = EVENTS_SPR_H - 4;                                                         ///< Maximale Y-Position (mit etwas Puffer unten)

    char truncBuf[64];
    int y = startY;
    int drawn = 0;
    bool fontIs16 = false;

    for (int i = 0; i < m_currentData->eventCount && i < DayViewCfg::MAX_EVENTS; ++i)
    {
        const DayViewEvent &evt = m_currentData->events[i];

        // Tages-Header bei Tageswechsel
        if (evt.hasDayHeader)
        {
            if (y + DAY_HEADER_PAD + DAY_HEADER_H + LINE_H > maxY)
                break;

            // Tages-Label aus Roh-Feldern bauen: "Heute" oder "Mo, 24. Feb"
            char dayLabelBuf[24];
            if (evt.isTodayHeader)
            {
                strncpy_P(dayLabelBuf, reinterpret_cast<PGM_P>(STR(CommonKey::COMMON_TODAY)), sizeof(dayLabelBuf) - 1);
                dayLabelBuf[sizeof(dayLabelBuf) - 1] = '\0';
            }
            else if (evt.weekdayIdx >= 0 && evt.monthIdx >= 0 && evt.dayOfMonth > 0)
            {
                char wdBuf[16];
                char monBuf[16];
                I18n::copyWeekday(wdBuf, sizeof(wdBuf), evt.weekdayIdx, /*longForm=*/false);
                I18n::copyMonth(monBuf, sizeof(monBuf), evt.monthIdx, /*longForm=*/false);
                STRF(dayLabelBuf, DayViewKey::DAYVIEW_DAYLABEL_FMT, wdBuf, monBuf, static_cast<int>(evt.dayOfMonth));
            }
            else
            {
                dayLabelBuf[0] = '\0';
            }

            // Trennlinie vor neuem Tag (nicht vor dem ersten)
            if (i > 0)
            {
                y += SEPARATOR_PAD;
                spr->drawFastHLine(0, y, EVENTS_SPR_W, COLOR_EVT_SEPARATOR);
                y += SEPARATOR_PAD;
            }

            y += DAY_HEADER_PAD;

            if (fontIs16)
            {
                spr->unloadFont();
                fontIs16 = false;
            }
            spr->loadFont(ComfortaaBold14);
            spr->setTextDatum(TL_DATUM);
            spr->setTextColor(COLOR_EVT_DAY_HEADER, Theme::ColorMainBackGround);
            spr->drawString(dayLabelBuf, 0, y);
            spr->unloadFont();
            y += DAY_HEADER_H + 2;
        }

        if (y + LINE_H > maxY)
            break;

        if (!fontIs16)
        {
            spr->loadFont(ComfortaaBold16);
            fontIs16 = true;
        }

        // Farbiger Balken
        const uint16_t barColor = getEventBarColor(evt.eventType, evt.isAllDay);
        spr->fillRect(barX, y + 3, EVENT_BAR_W, EVENT_BAR_H, barColor);

        const bool isHoliday = (strcmp(evt.eventType, "holiday") == 0);
        const uint16_t timeColor = evt.isPast ? TFT_DARKGREY : COLOR_EVT_TIME;
        const uint16_t textColor = evt.isPast  ? TFT_DARKGREY
                                   : isHoliday ? COLOR_EVT_SUMMARY_HOLIDAY
                                               : COLOR_EVT_SUMMARY;

        spr->setTextDatum(TL_DATUM);

        if (evt.isAllDay)
        {
            spr->setTextColor(textColor, Theme::ColorMainBackGround);
            truncateTextToWidth(spr, evt.summary, availableW, truncBuf, sizeof(truncBuf));
            spr->drawString(truncBuf, textX, y);
        }
        else
        {
            spr->setTextColor(timeColor, Theme::ColorMainBackGround);
            spr->drawString(evt.time, textX, y);
            spr->setTextColor(textColor, Theme::ColorMainBackGround);
            truncateTextToWidth(spr, evt.summary, availableW - TIME_COL_W - 5, truncBuf, sizeof(truncBuf));
            spr->drawString(truncBuf, textX + TIME_COL_W, y);
        }

        y += LINE_H;
        drawn++;
    }

    if (fontIs16)
        spr->unloadFont();

    // Badge "+X weitere" (basierend auf Gesamtzahl)
    const int remaining = m_currentData->eventTotalCount - drawn;
    if (remaining > 0)
    {
        char hint[24];
        STRF(hint, DayViewKey::DAYVIEW_BADGE_MORE, remaining);

        spr->loadFont(ComfortaaBold14);

        const int badgeTextW = spr->textWidth(hint);
        const int badgeTextH = spr->fontHeight();
        const int badgeW = badgeTextW + 2 * BADGE_PAD_H;
        const int badgeH = badgeTextH + 2 * BADGE_PAD_V;

        const int badgeX = EVENTS_SPR_W - badgeW - BADGE_MARGIN_RIGHT;
        const int badgeY = EVENTS_SPR_H - badgeH - BADGE_MARGIN_BOTTOM;

        spr->fillRect(badgeX, badgeY, badgeW, badgeH, COLOR_BADGE_BG);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(COLOR_BADGE_TEXT, COLOR_BADGE_BG);
        spr->drawString(hint, badgeX + badgeW / 2, badgeY + badgeH / 2);

        spr->unloadFont();
    }

    // Atomar auf Display pushen
    spr->pushSprite(COL_LEFT_X, CONTENT_Y);
}

// ===================================================================
// HILFSFUNKTIONEN FÜR TERMINE
// ===================================================================

/**
 * @brief Bestimmt die Balkenfarbe basierend auf Event-Typ
 */
uint16_t DayView::getEventBarColor(const char *eventType, bool isAllDay) const
{
    if (eventType && strcmp(eventType, "holiday") == 0)
        return COLOR_EVT_BAR_HOLIDAY;

    if (isAllDay)
        return COLOR_EVT_BAR_ALLDAY;

    return COLOR_EVT_BAR_NORMAL;
}

/**
 * @brief Textkürzung als Template
 */
template <typename GfxTarget>
static void truncateImpl(GfxTarget *gfx, const char *text, int maxWidth, char *output, size_t outputSize)
{
    if (!text || !output || outputSize == 0 || !gfx)
    {
        if (output)
            output[0] = '\0';

        return;
    }

    // Schneller Pfad: Text passt komplett
    if (gfx->textWidth(text) <= maxWidth)
    {
        stringutils::safeCopy(output, outputSize, text);
        return;
    }

    const int ellipsisWidth = gfx->textWidth("...");
    const int targetWidth = maxWidth - ellipsisWidth;

    if (targetWidth <= 0)
    {
        stringutils::safeCopy(output, outputSize, "...");
        return;
    }

    // UTF-8 Continuation Byte erkennen
    auto isUtf8Cont = [](char c)
    { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; };

    // Lineare Suche von hinten
    const size_t len = strlen(text);
    const size_t maxLen = std::min<size_t>(len, outputSize - 4);

    char tempBuf[128];
    size_t bestFit = 0;

    for (size_t i = std::min<size_t>(maxLen, sizeof(tempBuf) - 1); i > 0; --i)
    {
        if (isUtf8Cont(text[i]))
            continue;

        memcpy(tempBuf, text, i);
        tempBuf[i] = '\0';

        if (gfx->textWidth(tempBuf) <= targetWidth)
        {
            bestFit = i;
            break;
        }
    }

    if (bestFit > 0)
    {
        memcpy(output, text, bestFit);
        output[bestFit] = '\0';
        stringutils::safeAppend(output, outputSize, "...");
    }
    else
    {
        stringutils::safeCopy(output, outputSize, "...");
    }
}

/**
 * @brief Kürzt Text auf maximale Pixelbreite mit "..." (UTF-8 sicher)
 */
void DayView::truncateTextToWidth(const char *text, int maxWidth, char *output, size_t outputSize) const
{
    truncateImpl(m_tft, text, maxWidth, output, outputSize);
}

/**
 * @brief Kürzt Text auf maximale Pixelbreite mit "..." (Sprite-Variante)
 */
void DayView::truncateTextToWidth(Gfx_Sprite *sprite, const char *text, int maxWidth, char *output, size_t outputSize) const
{
    truncateImpl(sprite, text, maxWidth, output, outputSize);
}

// ===================================================================
// UI - RECHTE SPALTE (AUFGABEN + GEBURTSTAGE)
// ===================================================================

/**
 * @brief Zeichnet rechte Spalte: Aufgaben und Geburtstage
 */
void DayView::drawTasksSection()
{
    if (!m_currentData || !m_tasksSprite)
        return;

    auto *spr = m_tasksSprite;
    spr->clear(Theme::ColorMainBackGround);

    // Lokale Positionen
    constexpr int TASK_X = 9;
    constexpr int BULLET_X = TASK_X;
    constexpr int TEXT_X = TASK_X + TASK_BULLET_SIZE + TASK_BULLET_GAP;
    const int availableW = TASKS_SPR_W - TEXT_X - 10;

    // ===================================================================
    // 1. AUFGABEN
    // ===================================================================

    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(COLOR_ACCENT, Theme::ColorMainBackGround);
    spr->drawString(STR(m_currentData->tasksIsDemo
                            ? DayViewKey::DAYVIEW_LABEL_TASKS_DEMO
                            : DayViewKey::DAYVIEW_LABEL_TASKS),
                    TASK_X, 0);
    spr->unloadFont();

    int y = TASK_AREA_START;
    int drawn = 0;

    if (m_currentData->taskCount == 0)
    {
        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(TL_DATUM);
        spr->setTextColor(COLOR_DIM, Theme::ColorMainBackGround);
        spr->drawString(STR(DayViewKey::DAYVIEW_NO_TASKS), TEXT_X, y + 2);
        spr->unloadFont();
        y += SUB_TEXT_H + 4;
    }
    else
    {
        spr->loadFont(ComfortaaBold16);
        char truncBuf[64];

        const int maxTasks = std::min({m_currentData->taskCount, DayViewCfg::MAX_TASKS, MAX_VISIBLE_TASKS});

        for (int i = 0; i < maxTasks; ++i)
        {
            const DayViewTask &task = m_currentData->tasks[i];

            uint16_t titleColor = COLOR_TEXT;
            uint16_t bulletColor = COLOR_TASK_BULLET;

            if (task.isOverdue)
            {
                titleColor = COLOR_OVERDUE;
                bulletColor = COLOR_OVERDUE;
            }
            else if (task.isDueToday)
            {
                titleColor = COLOR_TODAY;
                bulletColor = COLOR_TODAY;
            }

            // Aufzählungszeichen
            const int bulletY = y + 10 - TASK_BULLET_SIZE / 2;
            spr->fillRect(BULLET_X, bulletY, TASK_BULLET_SIZE, TASK_BULLET_SIZE, bulletColor);

            // Aufgaben-Titel
            spr->setTextDatum(TL_DATUM);
            spr->setTextColor(titleColor, Theme::ColorMainBackGround);
            truncateTextToWidth(spr, task.title, availableW, truncBuf, sizeof(truncBuf));
            spr->drawString(truncBuf, TEXT_X, y);

            y += TASK_LINE_H;
            drawn++;
        }

        spr->unloadFont();

        // Badge "+X weitere" (basierend auf Gesamtzahl)
        const int remaining = m_currentData->taskTotalCount - drawn;
        if (remaining > 0)
        {
            char hint[24];
            STRF(hint, DayViewKey::DAYVIEW_BADGE_MORE, remaining);

            spr->loadFont(ComfortaaBold14);
            const int badgeTextW = spr->textWidth(hint);
            const int badgeTextH = spr->fontHeight();
            const int badgeW = badgeTextW + 2 * BADGE_PAD_H;
            const int badgeH = badgeTextH + 2 * BADGE_PAD_V;
            const int badgeX = TASKS_SPR_W - badgeW - BADGE_MARGIN_RIGHT;
            const int badgeY = y - TASK_LINE_H + (TASK_LINE_H - badgeH) / 2;

            spr->fillRect(badgeX, badgeY, badgeW, badgeH, COLOR_BADGE_BG);
            spr->setTextDatum(MC_DATUM);
            spr->setTextColor(COLOR_BADGE_TEXT, COLOR_BADGE_BG);
            spr->drawString(hint, badgeX + badgeW / 2, badgeY + badgeH / 2);
            spr->unloadFont();
        }
    }

    // ===================================================================
    // 2. GEBURTSTAGE
    // ===================================================================

    y += SUB_SECTION_GAP;

    // Trennlinie (zentriert im Gap)
    spr->drawFastHLine(TASK_X, y - SUB_SECTION_GAP / 2, TASKS_SPR_W - TASK_X - 8, COLOR_DIVIDER);

    // Label
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(COLOR_ACCENT, Theme::ColorMainBackGround);
    spr->drawString(STR(m_currentData->calendarIsDemo
                            ? DayViewKey::DAYVIEW_LABEL_BIRTHDAYS_DEMO
                            : DayViewKey::DAYVIEW_LABEL_BIRTHDAYS),
                    TASK_X, y);
    spr->unloadFont();
    y += SUB_LABEL_H + SUB_LABEL_PAD;

    // Geburtstags-Namen
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(TL_DATUM);

    if (m_currentData->birthdayCount > 0 && (m_currentData->availableSections & SECTION_CALENDAR))
    {
        // Alle Namen kommagetrennt zusammenbauen
        char bdayStr[128] = {0};
        for (int i = 0; i < m_currentData->birthdayCount && i < DayViewCfg::MAX_BIRTHDAYS; ++i)
        {
            if (i > 0)
                stringutils::safeAppend(bdayStr, sizeof(bdayStr), ", ");

            stringutils::safeAppend(bdayStr, sizeof(bdayStr), m_currentData->birthdays[i].name);
        }

        // Truncate, falls zu breit
        char truncBuf[128];
        truncateTextToWidth(spr, bdayStr, availableW, truncBuf, sizeof(truncBuf));

        spr->setTextColor(COLOR_BIRTHDAY, Theme::ColorMainBackGround);
        spr->drawString(truncBuf, TEXT_X, y);
    }
    else
    {
        spr->setTextColor(COLOR_DIM, Theme::ColorMainBackGround);
        spr->drawString(STR(DayViewKey::DAYVIEW_NO_BIRTHDAYS), TEXT_X, y);
    }

    spr->unloadFont();

    // Atomar auf Display pushen
    spr->pushSprite(TASKS_SPR_X, CONTENT_Y);
}

// ===================================================================
// UI - INFO-BAR (Station | Titel | Bitrate im Bottom-Sprite)
// ===================================================================

/**
 * @brief Zeichnet die Radio-Info-Bar: Station | Titel | Bitrate
 */
void DayView::drawBottomSection()
{
    if (!m_bottomSprite)
        return;

    auto *spr = m_bottomSprite;

    // ===================================================================
    // News-Modus: Eilmeldung (rot) oder Top-Meldung (dunkelblau)
    // ===================================================================
    if (m_newsActive && m_newsTickerText[0] != '\0')
    {
        const uint16_t bgColor = m_newsIsBreaking ? COLOR_NEWS_BG : COLOR_NEWS_BG_TOP1;
        const uint16_t textColor = m_newsIsBreaking ? COLOR_NEWS_TEXT : COLOR_NEWS_TEXT_TOP1;

        spr->clear(bgColor);
        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(MC_DATUM);
        spr->setTextWrap(false);
        spr->setTextColor(textColor, bgColor);

        // Statisch zentriert; eine zu breite Schlagzeile wird gekürzt, der volle Artikel bleibt über den QR-Code erreichbar
        char truncBuf[sizeof(m_newsTickerText)];
        truncateTextToWidth(spr, m_newsTickerText, BOTTOM_SPR_W - 8, truncBuf, sizeof(truncBuf));
        spr->drawString(truncBuf, BOTTOM_SPR_W / 2, BOTTOM_SPR_H / 2);
        spr->unloadFont();

        spr->pushSprite(LEFT_MARGIN, BOTTOM_Y);
        return;
    }

    // ===================================================================
    // SD-Player Info-Modus: "3/12 | Artist - Title | 320 kbps"
    // ===================================================================
    if (m_audioSource == AudioSource::SD_PLAYER && !m_sdPlaylist.empty())
    {
        spr->clear(COLOR_INFOBAR_BG);
        spr->drawFastHLine(0, 0, BOTTOM_SPR_W, COLOR_INFOBAR_BORDER);
        spr->drawFastHLine(0, BOTTOM_SPR_H - 1, BOTTOM_SPR_W, COLOR_INFOBAR_BORDER);

        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(ML_DATUM);
        spr->setTextWrap(false);

        constexpr const char *SEP = " | ";
        const int sepW = spr->textWidth(SEP);
        const int centerY = BOTTOM_SPR_H / 2;
        const int availableW = BOTTOM_SPR_W - 8;

        // Track-Index: "3/12"
        char trackIdx[12];
        stringutils::safeFormat(trackIdx, "%d/%d", m_sdCurrentTrack + 1, static_cast<int>(m_sdPlaylist.size()));

        // Titel: "Artist - Title" oder nur Title/Dateiname
        char titleBuf[196] = {};
        if (m_sdArtist[0] != '\0' && m_sdTitle[0] != '\0')
            stringutils::safeFormat(titleBuf, sizeof(titleBuf), "%s - %s", m_sdArtist, m_sdTitle);
        else if (m_sdTitle[0] != '\0')
            stringutils::safeCopy(titleBuf, m_sdTitle);

        const bool hasTitle = titleBuf[0] != '\0';
        const bool hasBitrate = m_sdBitrate[0] != '\0';

        // Feste Breiten berechnen
        const int idxW = spr->textWidth(trackIdx);
        const int bitrateW = hasBitrate ? spr->textWidth(m_sdBitrate) : 0;

        int fixedW = idxW;
        if (hasTitle)
            fixedW += sepW;
        if (hasBitrate)
            fixedW += sepW + bitrateW;

        // Titel kürzen, falls nötig (UTF-8-sicher)
        char truncTitle[196] = {};
        if (hasTitle)
        {
            const int availForTitle = availableW - fixedW;
            if (availForTitle > 0)
                truncateTextToWidth(spr, titleBuf, availForTitle, truncTitle, sizeof(truncTitle));
        }

        // Gesamtbreite + zentriert zeichnen
        int totalW = idxW;
        if (truncTitle[0] != '\0')
            totalW += sepW + spr->textWidth(truncTitle);

        if (hasBitrate)
            totalW += sepW + bitrateW;

        int curX = (BOTTOM_SPR_W - totalW) / 2;

        // Track-Index (Akzentfarbe)
        spr->setTextColor(COLOR_INFOBAR_STATION, COLOR_INFOBAR_BG);
        spr->drawString(trackIdx, curX, centerY);
        curX += idxW;

        // Separator + Titel
        if (truncTitle[0] != '\0')
        {
            spr->setTextColor(COLOR_INFOBAR_SEP, COLOR_INFOBAR_BG);
            spr->drawString(SEP, curX, centerY);
            curX += sepW;
            spr->setTextColor(TFT_SILVER, COLOR_INFOBAR_BG);
            spr->drawString(truncTitle, curX, centerY);
            curX += spr->textWidth(truncTitle);
        }

        // Separator + Bitrate
        if (hasBitrate)
        {
            spr->setTextColor(COLOR_INFOBAR_SEP, COLOR_INFOBAR_BG);
            spr->drawString(SEP, curX, centerY);
            curX += sepW;
            spr->setTextColor(COLOR_INFOBAR_BITRATE, COLOR_INFOBAR_BG);
            spr->drawString(m_sdBitrate, curX, centerY);
        }

        spr->unloadFont();
        spr->pushSprite(LEFT_MARGIN, BOTTOM_Y);
        return;
    }

    // ===================================================================
    // Normaler Radio-Info-Modus
    // ===================================================================

    // Hintergrund + Rahmenlinien
    spr->clear(COLOR_INFOBAR_BG);
    spr->drawFastHLine(0, 0, BOTTOM_SPR_W, COLOR_INFOBAR_BORDER);
    spr->drawFastHLine(0, BOTTOM_SPR_H - 1, BOTTOM_SPR_W, COLOR_INFOBAR_BORDER);

    // Kein Sender -> leere Bar
    if (m_radioStation[0] == '\0')
    {
        spr->pushSprite(LEFT_MARGIN, BOTTOM_Y);
        return;
    }

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);
    spr->setTextWrap(false);

    constexpr const char *SEP = " | ";
    const int sepW = spr->textWidth(SEP);
    const int centerY = BOTTOM_SPR_H / 2;
    const int availableW = BOTTOM_SPR_W - 8; // 4px Rand links + rechts
    const bool hasStation = m_radioStation[0] != '\0';
    const bool hasTitle = m_radioTitle[0] != '\0';
    const bool hasBitrate = m_radioBitrate[0] != '\0';

    // Feste Breiten berechnen (Station + Bitrate + Separatoren)
    const int stationW = hasStation ? spr->textWidth(m_radioStation) : 0;
    const int bitrateW = hasBitrate ? spr->textWidth(m_radioBitrate) : 0;

    int fixedW = stationW;
    if (hasTitle && hasStation)
        fixedW += sepW;

    if (hasBitrate)
    {
        if (hasTitle || hasStation)
            fixedW += sepW;

        fixedW += bitrateW;
    }

    // Titel-Text vorbereiten (ggf. kürzen, UTF-8-sicher)
    char truncTitle[256] = {};
    if (hasTitle)
    {
        const int availForTitle = availableW - fixedW;

        if (availForTitle > 0)
            truncateTextToWidth(spr, m_radioTitle, availForTitle, truncTitle, sizeof(truncTitle));
    }

    // Gesamtbreite berechnen + zentriert zeichnen
    auto calcWidth = [&](const char *sta, const char *ttl, const char *brt) -> int
    {
        int w = 0;
        bool prev = false;

        if (sta[0] != '\0')
        {
            w += spr->textWidth(sta);
            prev = true;
        }

        if (ttl[0] != '\0')
        {
            if (prev)
                w += sepW;
            w += spr->textWidth(ttl);
            prev = true;
        }

        if (brt[0] != '\0')
        {
            if (prev)
                w += sepW;
            w += spr->textWidth(brt);
        }

        return w;
    };

    const int totalW = calcWidth(m_radioStation, truncTitle, m_radioBitrate);
    int curX = (BOTTOM_SPR_W - totalW) / 2;

    // Station
    if (hasStation)
    {
        spr->setTextColor(COLOR_INFOBAR_STATION, COLOR_INFOBAR_BG);
        spr->drawString(m_radioStation, curX, centerY);
        curX += stationW;
    }

    // Separator + Titel
    if (truncTitle[0] != '\0')
    {
        if (hasStation)
        {
            spr->setTextColor(COLOR_INFOBAR_SEP, COLOR_INFOBAR_BG);
            spr->drawString(SEP, curX, centerY);
            curX += sepW;
        }

        spr->setTextColor(TFT_SILVER, COLOR_INFOBAR_BG);
        spr->drawString(truncTitle, curX, centerY);
        curX += spr->textWidth(truncTitle);
    }

    // Separator + Bitrate
    if (hasBitrate)
    {
        if (hasStation || truncTitle[0] != '\0')
        {
            spr->setTextColor(COLOR_INFOBAR_SEP, COLOR_INFOBAR_BG);
            spr->drawString(SEP, curX, centerY);
            curX += sepW;
        }

        spr->setTextColor(COLOR_INFOBAR_BITRATE, COLOR_INFOBAR_BG);
        spr->drawString(m_radioBitrate, curX, centerY);
    }

    spr->unloadFont();
    spr->pushSprite(LEFT_MARGIN, BOTTOM_Y);
}

// ===================================================================
// AUDIO-EVENT-VERARBEITUNG (Metadaten für Info-Bar)
// ===================================================================

/**
 * @brief Prüft, ob direkt aufs Display gezeichnet werden darf
 *
 * false, solange ein modaler Dialog (Favoriten, Senderwahl, Dateiauswahl, MessageBox) aktiv ist oder GFX global blockiert ist - Zeichnen würde den Dialog übermalen. Zustandsänderungen setzen dann nur ihre Dirty-Flags, nachgezeichnet wird nach dem Schließen.
 */
bool DayView::uiDrawAllowed() const
{
    if (APP_STATE.getBlockGFX())
        return false;

    if (m_presetDlg && m_presetDlg->isActive())
        return false;

    if (m_stationPicker && m_stationPicker->isActive())
        return false;

    if (m_fileExplorer && m_fileExplorer->isActive())
        return false;

    if (m_messageBox && (m_messageBox->isActive() || m_messageBox->isBusy()))
        return false;

    return true;
}

/**
 * @brief Verwirft alle noch gepufferten Audio-Events
 *
 * Nach einem synchronen Stopp bzw. vor einem Neustart gehören alle Ring-Events (Titel, Bitrate, EOF) dem alten Titel oder Sender und würden sonst am neuen landen.
 */
void DayView::drainStaleAudioEvents()
{
    AudioManagerEventType staleType;
    while (g_audioManager.dequeueEvent(staleType, m_radioEventText, sizeof(m_radioEventText)))
        continue;
}

/**
 * @brief Verarbeitet Audio-Events aus der globalen Queue
 *
 * Liest STATION_NAME, STREAM_TITLE und BITRATE Events und aktualisiert die Info-Bar bei Änderungen. Läuft auch bei offenem Dialog (sonst läuft der Ring über); gezeichnet wird dann nicht, die Dirty-Flags bleiben bis zum nächsten erlaubten Durchlauf stehen.
 */
void DayView::processAudioEvents()
{
    const bool drawAllowed = uiDrawAllowed();
    AudioManagerEventType eventType;
    int eventCount = 0;
    constexpr int MAX_EVENTS_PER_FRAME = 3;

    while (eventCount < MAX_EVENTS_PER_FRAME && g_audioManager.dequeueEvent(eventType, m_radioEventText, sizeof(m_radioEventText)))
    {
        eventCount++;

        // SD-Player Events
        if (m_audioSource == AudioSource::SD_PLAYER)
        {
            switch (eventType)
            {
            case AudioManagerEventType::EOF_AUDIO:
                sdHandleEof();
                break;

            case AudioManagerEventType::ID3_ARTIST:
                if (strcmp(m_sdArtist, m_radioEventText) != 0)
                {
                    stringutils::safeCopy(m_sdArtist, m_radioEventText);
                    m_sdInfoDirty = true;
                }
                break;

            case AudioManagerEventType::ID3_TITLE:
                if (strcmp(m_sdTitle, m_radioEventText) != 0)
                {
                    stringutils::safeCopy(m_sdTitle, m_radioEventText);
                    m_sdInfoDirty = true;
                }
                break;

            case AudioManagerEventType::BITRATE:
                if (m_radioEventText[0] != '\0' && strcmp(m_radioEventText, "0") != 0)
                {
                    int bps = atoi(m_radioEventText);
                    int kbps = (bps > 1000) ? bps / 1000 : bps;
                    char newBitrate[16];
                    stringutils::safeFormat(newBitrate, "%d kbps", kbps);

                    if (strcmp(m_sdBitrate, newBitrate) != 0)
                    {
                        stringutils::safeCopy(m_sdBitrate, newBitrate);
                        m_sdInfoDirty = true;
                    }
                }
                break;

            default:
                break;
            }

            continue; // SD-Events nicht an Radio-Logik weiterleiten
        }

        // Radio Events (bestehende Logik)
        switch (eventType)
        {
        case AudioManagerEventType::STATION_NAME:
            stringutils::safeCopy(m_radioEventStation, m_radioEventText);
            {
                // Bester Sendername: Event-Station wenn länger als Preset
                Station preset;
                const char *presetName = "";

                if (m_stationManager &&
                    m_stationManager->getStation(APP_STATE.getCurrentRetroStation(), preset))
                    presetName = preset.name;

                const char *best = (m_radioEventStation[0] != '\0' && strlen(m_radioEventStation) >= strlen(presetName))
                                       ? m_radioEventStation
                                       : presetName;

                if (strcmp(m_radioStation, best) != 0)
                {
                    stringutils::safeCopy(m_radioStation, best);
                    m_radioInfoDirty = true;
                }
            }

            break;

        case AudioManagerEventType::STREAM_TITLE:
            if (strcmp(m_radioTitle, m_radioEventText) != 0)
            {
                stringutils::safeCopy(m_radioTitle, m_radioEventText);
                m_radioInfoDirty = true;
            }

            break;

        case AudioManagerEventType::BITRATE:
            if (m_radioEventText[0] != '\0' && strcmp(m_radioEventText, "0") != 0)
            {
                int bps = atoi(m_radioEventText);
                int kbps = (bps > 1000) ? bps / 1000 : bps;
                char newBitrate[16];

                stringutils::safeFormat(newBitrate, "%d kbps", kbps);

                if (strcmp(m_radioBitrate, newBitrate) != 0)
                {
                    stringutils::safeCopy(m_radioBitrate, newBitrate);
                    m_radioInfoDirty = true;
                }
            }

            break;

        default:
            break;
        }
    }

    // Info-Bar nur bei Änderungen neu zeichnen (nicht wenn Eilmeldung aktiv oder ein Dialog offen ist)
    if ((m_radioInfoDirty || m_sdInfoDirty) && drawAllowed)
    {
        m_radioInfoDirty = false;
        m_sdInfoDirty = false;

        if (!m_newsActive)
            drawBottomSection();
    }

    // SD-Playback-Start-Erkennung (Timeout-Schutz); bei offenem Dialog zeichnet der nächste Radio-Status-Check nach
    if (m_sdWaitingForStart)
    {
        const bool running = m_audio ? m_audio->isRunning() : false;
        if (running)
        {
            m_sdWaitingForStart = false;
            m_sdPlaying = true;

            if (drawAllowed)
                drawSdButtons();
        }
        else if (millis() - m_sdStartWaitMs > SD_START_TIMEOUT_MS)
        {
            LOG_WARNING("DV SD: Playback-Start Timeout");
            m_sdWaitingForStart = false;
            m_sdPlaying = false;

            if (drawAllowed)
                drawSdButtons();
        }
    }
}

// ===================================================================
// UI - RADIO-BAR
// ===================================================================

/**
 * @brief Zeichnet die Radio-Steuerung
 */
void DayView::drawRadioBar()
{
    const bool playing = m_audio ? m_audio->isRunning() : false;

    // Play/Stop-Symbol: Grün wenn spielend, Standard wenn gestoppt
    m_radioPlayBtn.setLabelText(playing ? "^STOP:32:A944^" : "^PLAY:32:5C44^");

    // Transport-Buttons zeichnen
    m_radioFavBtn.drawButton(false);
    m_radioPrevBtn.drawButton(false);
    m_radioPlayBtn.drawButton(false);
    m_radioNextBtn.drawButton(false);

    // Sender-Index + Volume DotMatrix-Anzeigen
    drawStationDisplay();
    m_volDownBtn.drawButton(false);
    m_volUpBtn.drawButton(false);
    drawVolumeDisplay();

    // SD-Player Buttons zeichnen
    drawSdButtons();

    // Senderwechsel erkennen -> Metadaten zurücksetzen + Info-Bar aktualisieren
    const uint8_t stationIdx = APP_STATE.getCurrentRetroStation();
    if (stationIdx != m_lastRadioStation)
    {
        Station station;
        if (m_stationManager && m_stationManager->getStation(stationIdx, station))
            stringutils::safeCopy(m_radioStation, station.name);
        else
            stringutils::safeCopy(m_radioStation, "---");

        // Alte Metadaten löschen bei Senderwechsel
        m_radioTitle[0] = '\0';
        m_radioBitrate[0] = '\0';
        m_radioEventStation[0] = '\0';
        m_radioInfoDirty = true;
    }

    m_lastRadioStation = stationIdx;
    m_lastRadioPlaying = playing;
}

/**
 * @brief Zeichnet die DotMatrix-Senderanzeige im Rahmen (zweistellig, 01-99)
 */
void DayView::drawStationDisplay()
{
    if (!m_stnSprite)
        return;

    const uint8_t stnIdx = APP_STATE.getCurrentRetroStation();
    const int displayNum = constrain(stnIdx + 1, 1, 99); // 0-basiert -> 1-basiert, Anzeige ist zweistellig

    // Rahmen zeichnen
    const int frameY = RADIO_BTN_Y - STN_FRAME_H / 2;
    m_tft->fillSmoothRoundRect(STN_FRAME_X, frameY, STN_FRAME_W, STN_FRAME_H, RADIO_BTN_R, RADIO_BORDER_COLOR);
    m_tft->fillSmoothRoundRect(STN_FRAME_X + 1, frameY + 1, STN_FRAME_W - 2, STN_FRAME_H - 2, RADIO_BTN_R, TFT_BLACK);

    // Text setzen (rechtsbündig formatiert)
    char stnStr[4];
    stringutils::safeFormat(stnStr, "%2d", displayNum);
    m_stnDotMatrix.setText(stnStr);

    // Sprite füllen + DotMatrix zeichnen
    m_stnSprite->clear(TFT_BLACK);
    m_stnDotMatrix.draw(m_stnSprite, 0, 0);

    // Sprite vertikal zentriert im Rahmen ausgeben
    const int sprY = RADIO_BTN_Y - STN_DISP_H / 2;
    m_stnSprite->pushSprite(STN_DISP_X, sprY);
}

/**
 * @brief Zeichnet die DotMatrix-Lautstärkeanzeige im Button-Rahmen
 */
void DayView::drawVolumeDisplay()
{
    if (!m_volSprite)
        return;

    const uint8_t vol = constrain(APP_STATE.getCurrentVolume(), 0, 99);

    // Rahmen zeichnen
    const int frameY = RADIO_BTN_Y - VOL_FRAME_H / 2; ///< Y-Position des Rahmens (zentriert um Button-Y)
    m_tft->fillSmoothRoundRect(VOL_FRAME_X, frameY, VOL_FRAME_W, VOL_FRAME_H, RADIO_BTN_R, RADIO_BORDER_COLOR);
    m_tft->fillSmoothRoundRect(VOL_FRAME_X + 1, frameY + 1, VOL_FRAME_W - 2, VOL_FRAME_H - 2, RADIO_BTN_R, TFT_BLACK);

    // Text setzen (rechtsbündig formatiert)
    char volStr[4];
    stringutils::safeFormat(volStr, "%2d", vol);
    m_volDotMatrix.setText(volStr);

    // Sprite füllen + DotMatrix zeichnen
    m_volSprite->clear(TFT_BLACK);
    m_volDotMatrix.draw(m_volSprite, 0, 0);

    // Sprite vertikal zentriert im Rahmen ausgeben
    const int sprY = RADIO_BTN_Y - VOL_DISP_H / 2;
    m_volSprite->pushSprite(VOL_DISP_X, sprY);

    m_lastVolume = vol;
}

/**
 * @brief Touch-Handling für Volume-Buttons (+/-)
 *
 * Wiederholt beim Halten im Debounce-Takt; scharf ist nur der Button, auf dem der Finger frisch aufgesetzt hat (m_volHeldDir), ein hereinwandernder Finger bleibt wirkungslos.
 */
void DayView::handleVolumeTouches(int16_t tx, int16_t ty)
{
    if (m_volHeldDir < 0 && m_volDownBtn.contains(tx, ty))
    {
        m_volDownBtn.drawButton(true);
        uint8_t vol = APP_STATE.getCurrentVolume();
        if (vol > 0)
        {
            --vol;
            APP_STATE.setCurrentVolume(vol);

            if (m_callbacks.onVolumeChanged)
                m_callbacks.onVolumeChanged(vol);
        }

        drawVolumeDisplay();
        m_volDownBtn.drawButton(false);
        return;
    }

    if (m_volHeldDir > 0 && m_volUpBtn.contains(tx, ty))
    {
        m_volUpBtn.drawButton(true);
        uint8_t vol = APP_STATE.getCurrentVolume();

        if (vol < 99)
        {
            ++vol;
            APP_STATE.setCurrentVolume(vol);
            if (m_callbacks.onVolumeChanged)
                m_callbacks.onVolumeChanged(vol);
        }

        drawVolumeDisplay();
        m_volUpBtn.drawButton(false);
        return;
    }
}

/**
 * @brief Play/Stop Toggle
 */
void DayView::radioPlayStop()
{
    const bool playing = m_audio ? m_audio->isRunning() : false;

    if (playing && m_audioSource != AudioSource::SD_PLAYER)
    {
        // Radio läuft (auch extern gestartete Wiedergabe, z.B. über die Weboberfläche) -> stoppen
        stopCurrentSource();
        LOG_INFO("DV Radio: Stop");
    }
    else
    {
        // Laufende Quelle stoppen + Alt-Events verwerfen (es kann nur einen geben)
        stopCurrentSource();

        if (m_callbacks.requestPlayStation)
            m_callbacks.requestPlayStation(APP_STATE.getCurrentRetroStation());

        m_audioSource = AudioSource::RADIO;
        LOG_INFO("DV Radio: Play");
    }
}

/**
 * @brief Nächster Sender
 */
void DayView::radioNextStation()
{
    if (!m_stationManager)
        return;

    const int count = m_stationManager->getStationCount();
    if (count <= 0)
        return;

    // Laufende Quelle stoppen + Alt-Events verwerfen (Abspielen nur über PLAY-Button)
    const bool playing = m_audio ? m_audio->isRunning() : false;
    stopCurrentSource();

    uint8_t next = (APP_STATE.getCurrentRetroStation() + 1) % count;
    APP_STATE.setCurrentRetroStation(next);

    LOG_INFO_F("DV Radio: Next -> %d%s", next, playing ? " (stopped)" : "");
}

/**
 * @brief Vorheriger Sender
 */
void DayView::radioPrevStation()
{
    if (!m_stationManager)
        return;

    const int count = m_stationManager->getStationCount();
    if (count <= 0)
        return;

    // Laufende Quelle stoppen + Alt-Events verwerfen (Abspielen nur über PLAY-Button)
    const bool playing = m_audio ? m_audio->isRunning() : false;
    stopCurrentSource();

    uint8_t cur = APP_STATE.getCurrentRetroStation();
    uint8_t prev = (cur == 0) ? (count - 1) : (cur - 1);
    APP_STATE.setCurrentRetroStation(prev);

    LOG_INFO_F("DV Radio: Prev -> %d%s", prev, playing ? " (stopped)" : "");
}

// ===================================================================
// SD-PLAYER STEUERUNG
// ===================================================================

/**
 * @brief Öffnet den FileExplorer zur Datei-/Verzeichnisauswahl
 */
void DayView::sdBrowse()
{
    if (!m_fileExplorer)
        return;

    APP_STATE.setBlockGFX(true);

    FileExplorerConfig config;
    config.title = reinterpret_cast<const char *>(STR(SDPlayerKey::FILEEXPLORER_TITLE));
    config.setExtensions({".mp3", ".aac", ".flac", ".m4a", ".ogg"});

    m_fileExplorer->setBounds(
        (m_tft->width() - 550) / 2,
        (m_tft->height() - 350) / 2,
        550, 350);

    m_fileExplorer->start(config, [this](std::vector<FileResult> selectedFiles)
                          {
        APP_STATE.setBlockGFX(false);

        if (selectedFiles.empty())
        {
            LOG_INFO("DV SD: FileExplorer ohne Auswahl geschlossen");
            return;
        }

        int addedCount = 0;
        int duplicateCount = 0;

        // Duplikat-Prüfung + Hinzufügen
        for (const auto &file : selectedFiles)
        {
            bool duplicate = false;
            for (const auto &existing : m_sdPlaylist)
            {
                if (strcmp(existing.path, file.path) == 0)
                {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate)
            {
                m_sdPlaylist.push_back(file);
                addedCount++;
            }
            else
            {
                duplicateCount++;
            }
        }

        if (addedCount > 0)
        {
            LOG_INFO_F("DV SD: %d Datei(en) hinzugefügt (%d Duplikate)", addedCount, duplicateCount);
            m_sdPlaylistFinished = false;

            // Info-Bar aktualisieren falls SD-Quelle aktiv ist
            if (m_audioSource == AudioSource::SD_PLAYER)
            {
                m_sdInfoDirty = true;
                drawSdButtons();
            }
        }

        if (duplicateCount > 0)
            LOG_INFO_F("DV SD: %d Datei(en) übersprungen (Duplikate)", duplicateCount); });
}

/**
 * @brief SD Play/Pause Toggle
 *
 * - Playlist leer -> Browse öffnen
 * - Gestoppt -> Play (Radio stoppen, Track starten)
 * - Spielend -> Pause
 * - Pausiert -> Resume
 */
void DayView::sdPlayStop()
{
    if (m_sdPlaylist.empty())
    {
        LOG_INFO("DV SD: Keine Playlist -> Browse öffnen");
        sdBrowse();
        return;
    }

    if (m_sdPlaying && !m_sdPaused)
    {
        // Spielend -> Pause
        if (m_callbacks.requestPauseResume)
            m_callbacks.requestPauseResume();

        m_sdPaused = true;
        LOG_INFO_F("DV SD: Pause (Track %d/%d)", m_sdCurrentTrack + 1, static_cast<int>(m_sdPlaylist.size()));
        drawSdButtons();
        return;
    }

    if (m_sdPaused)
    {
        // Pausiert -> Resume
        if (m_callbacks.requestPauseResume)
            m_callbacks.requestPauseResume();

        m_sdPaused = false;
        LOG_INFO_F("DV SD: Resume (Track %d/%d)", m_sdCurrentTrack + 1, static_cast<int>(m_sdPlaylist.size()));
        drawSdButtons();
        return;
    }

    // Gestoppt -> Play: laufende Quelle stoppen + Alt-Events verwerfen
    stopCurrentSource();

    m_audioSource = AudioSource::SD_PLAYER;
    m_sdPaused = false;

    // Fortsetzen wo wir waren, oder von vorne
    if (m_sdCurrentTrack < 0 || m_sdPlaylistFinished)
    {
        m_sdCurrentTrack = 0;
        m_sdPlaylistFinished = false;
    }

    sdStartTrack(m_sdCurrentTrack);
    LOG_INFO_F("DV SD: Play Track %d/%d", m_sdCurrentTrack + 1, static_cast<int>(m_sdPlaylist.size()));
}

/**
 * @brief Nächster SD-Track (Skip)
 */
void DayView::sdNextTrack()
{
    if (m_sdPlaylist.empty() || m_audioSource != AudioSource::SD_PLAYER)
        return;

    // Pause aufheben bei Track-Wechsel
    m_sdPaused = false;

    if (m_sdCurrentTrack < static_cast<int>(m_sdPlaylist.size()) - 1)
    {
        m_sdCurrentTrack++;
        sdStartTrack(m_sdCurrentTrack);
        LOG_INFO_F("DV SD: Next -> Track %d/%d", m_sdCurrentTrack + 1, static_cast<int>(m_sdPlaylist.size()));
    }
    else
    {
        LOG_INFO("DV SD: Ende der Playlist (Next)");
        m_sdPlaylistFinished = true;
        stopCurrentSource();

        if (uiDrawAllowed())
            drawSdButtons();
    }
}

/**
 * @brief Playlist leeren + SD stoppen
 */
void DayView::sdClearPlaylist()
{
    // Leere Playlist -> nichts zu tun
    if (m_sdPlaylist.empty())
        return;

    // MessageBox-Bestätigung anzeigen
    if (!m_messageBox)
        return;

    m_messageBox->setBounds((m_tft->width() - 300) / 2, (m_tft->height() - 200) / 2, 300, 200);
    m_messageBox->setTitle(reinterpret_cast<const char *>(STR(DayViewKey::DAYVIEW_CLEAR_TITLE)));

    char msg[128];
    STRF(msg, DayViewKey::DAYVIEW_CLEAR_MESSAGE, static_cast<int>(m_sdPlaylist.size()));
    m_messageBox->setMessage(msg);
    m_messageBox->setButtonLabels(reinterpret_cast<const char *>(STR(CommonKey::COMMON_BTN_CONTINUE)), reinterpret_cast<const char *>(STR(CommonKey::COMMON_BTN_CANCEL)));
    m_messageBox->setTimeout(10000);
    m_messageBox->setCallback([this](bool confirmed)
                              {
        if (!confirmed)
        {
            drawRadioBar();
            return;
        }

        // Wiedergabe stoppen falls SD aktiv (inkl. Verwerfen der Alt-Events)
        if (m_audioSource == AudioSource::SD_PLAYER)
            stopCurrentSource();

        m_sdPlaylist.clear();
        m_sdCurrentTrack = -1;
        m_sdPlaying = false;
        m_sdPaused = false;
        m_sdPlaylistFinished = false;
        m_sdWaitingForStart = false;

        // Metadaten zurücksetzen
        m_sdArtist[0] = '\0';
        m_sdTitle[0] = '\0';
        m_sdBitrate[0] = '\0';
        m_sdInfoDirty = true;

        LOG_INFO("DV SD: Playlist geleert");

        // UI aktualisieren
        drawRadioBar();

        if (!m_newsActive)
            drawBottomSection(); });

    m_messageBox->start();
}

/**
 * @brief Startet einen bestimmten Track
 */
void DayView::sdStartTrack(int index)
{
    if (index < 0 || index >= static_cast<int>(m_sdPlaylist.size()))
        return;

    m_sdCurrentTrack = index;
    m_sdPaused = false;

    // Alt-Events des vorigen Titels verwerfen, Metadaten zurücksetzen (warten auf ID3-Events)
    drainStaleAudioEvents();
    m_sdArtist[0] = '\0';
    m_sdBitrate[0] = '\0';
    g_audioManager.resetMetadata();

    // Dateiname als Fallback-Titel
    const char *fullPath = m_sdPlaylist[index].path;
    const char *filename = strrchr(fullPath, '/');
    filename = filename ? filename + 1 : fullPath;

    // Extension entfernen für saubere Anzeige
    stringutils::safeCopy(m_sdTitle, filename);
    char *dot = strrchr(m_sdTitle, '.');
    if (dot)
        *dot = '\0';

    m_sdInfoDirty = true;

    // Playback starten
    m_sdWaitingForStart = true;
    m_sdStartWaitMs = millis();

    if (m_callbacks.requestPlayFile)
        m_callbacks.requestPlayFile(fullPath);

    if (uiDrawAllowed())
    {
        drawSdButtons();

        if (!m_newsActive)
            drawBottomSection();
    }
}

/**
 * @brief EOF-Handler: nächster Track oder Playlist-Ende
 */
void DayView::sdHandleEof()
{
    if (m_sdPlaylist.empty())
        return;

    if (m_sdCurrentTrack < static_cast<int>(m_sdPlaylist.size()) - 1)
    {
        // Nächster Track
        m_sdCurrentTrack++;
        sdStartTrack(m_sdCurrentTrack);
        LOG_INFO_F("DV SD: EOF -> Track %d/%d", m_sdCurrentTrack + 1, static_cast<int>(m_sdPlaylist.size()));
    }
    else
    {
        // Playlist beendet
        LOG_INFO("DV SD: Playlist komplett abgespielt");
        m_sdPlaylistFinished = true;
        m_sdPlaying = false;
        m_sdPaused = false;
        m_sdWaitingForStart = false;
        m_audioSource = AudioSource::NONE;
        m_sdInfoDirty = true;

        if (uiDrawAllowed())
        {
            drawSdButtons();

            if (!m_newsActive)
                drawBottomSection();
        }
    }
}

/**
 * @brief Stoppt die aktuell aktive Audio-Quelle
 */
void DayView::stopCurrentSource()
{
    if (m_callbacks.requestStopPlayback)
        m_callbacks.requestStopPlayback();

    // Der Stopp läuft synchron durch; alles, was danach noch im Ring liegt, gehört der alten Quelle
    drainStaleAudioEvents();

    if (m_audioSource == AudioSource::SD_PLAYER)
    {
        m_sdPlaying = false;
        m_sdPaused = false;
        m_sdWaitingForStart = false;
        m_sdInfoDirty = true;
    }

    m_audioSource = AudioSource::NONE;
}

/**
 * @brief Zeichnet die SD-Player Buttons (Play/Stop-Symbol aktualisieren)
 */
void DayView::drawSdButtons()
{
    // Play/Pause/Stop-Symbol:
    // - Spielend -> Pause-Symbol (Orange)
    // - Pausiert -> Play-Symbol (Gelb/Orange als "Resume"-Hinweis)
    // - Gestoppt -> Play-Symbol (Grün)
    if (m_sdPlaying && !m_sdPaused)
        m_sdPlayBtn.setLabelText("^PAUSE:32:A944^");
    else if (m_sdPaused)
        m_sdPlayBtn.setLabelText("^PLAY:32:FBE0^");
    else
        m_sdPlayBtn.setLabelText("^PLAY:32:5C44^");

    m_sdBrowseBtn.drawButton(false);
    m_sdPlayBtn.drawButton(false);
    m_sdNextBtn.drawButton(false);
    m_sdClearBtn.drawButton(false);
}

// ===================================================================
// UI - PROGRESSBAR
// ===================================================================

/**
 * @brief Refresh-Countdown am rechten Rand (1px lila Linie)
 */
void DayView::drawProgressBar()
{
    if (!m_progressSprite)
        return;

    unsigned long elapsed = millis() - m_lastRefresh;
    float progress = 1.0f - (static_cast<float>(elapsed) / static_cast<float>(NEWS_INTERVAL));
    progress = constrain(progress, 0.0f, 1.0f);

    int barHeight = static_cast<int>(m_tft->height() * progress);
    int startY = m_tft->height() - barHeight;

    m_progressSprite->drawFastVLine(0, 0, startY, Theme::ColorMainBackGround);
    m_progressSprite->drawFastVLine(0, startY, barHeight, COLOR_PROGRESS);
    m_progressSprite->pushSprite(m_tft->width() - 1, 0);
}

// ===================================================================
// WETTER-ICON (delegiert an Symbols-Klasse)
// ===================================================================

/**
 * @brief Zeichnet Wetter-Icon basierend auf WMO-Code (optional in Sprite)
 */
void DayView::drawWeatherIcon(int wmoCode, bool isDay, int x, int y, int width, uint16_t color, Gfx_Sprite *target)
{
    if (!m_symbols)
        return;

    switch (wmoCode)
    {
    case 0:
        isDay ? m_symbols->showSun(x, y, width, color, Theme::ColorMainBackGround, target)
              : m_symbols->showMoon(x, y, width, color, Theme::ColorMainBackGround, target);
        break;
    case 1:
    case 2:
    case 3:
        isDay ? m_symbols->showSunAndCloud(x, y, width, color, Theme::ColorMainBackGround, target)
              : m_symbols->showMoonAndCloud(x, y, width, color, Theme::ColorMainBackGround, target);
        break;
    case 45:
    case 48:
        m_symbols->showCloudAndFog(x, y, width, color, Theme::ColorMainBackGround, target);
        break;
    case 51:
    case 53:
    case 55:
    case 56:
    case 57:
    case 61:
    case 63:
    case 65:
    case 80:
    case 81:
    case 82:
        isDay ? m_symbols->showCloudAndRain(x, y, width, color, Theme::ColorMainBackGround, target)
              : m_symbols->showMoonAndCloudAndRain(x, y, width, color, Theme::ColorMainBackGround, target);
        break;
    case 66:
    case 67:
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
        m_symbols->showCloudAndSnow(x, y, width, color, Theme::ColorMainBackGround, target);
        break;
    case 95:
    case 96:
    case 99:
        m_symbols->showCloudAndThunderstorm(x, y, width, color, Theme::ColorMainBackGround, target);
        break;
    default:
        m_symbols->showCloud(x, y, width, color, Theme::ColorMainBackGround, target);
        break;
    }
}
