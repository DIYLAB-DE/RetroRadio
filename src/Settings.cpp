#include "Settings.h"
#include "main.h"

#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>
#include <WiFi.h>

#include "hal/usb_serial_jtag_ll.h"
#include "soc/usb_serial_jtag_reg.h"

#include <cctype>
#include <cstring>
#include <ctime>

#include "AppState.h"
#include "Clocks.h"
#include "DS1307_RTC.h"
#include "HttpService.h"
#include "I18n.h"
#include "Keyboard.h"
#include "MessageBox.h"
#include "Strings_common.h"
#include "SystemStats.h"
#include "Theme.h"
#include "RetroRadio.h" // Maße, Farben und Aufzählung der Abstimmröhren

#include "WifiList.h"

#include <ArduinoJson.h>

#include "fonts/ComfortaaBold10.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold30.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Layout- und Farbkonstanten
// ===================================================================

namespace
{
    // Bildschirm und Grobaufteilung
    constexpr int SCREEN_W = 800;                   ///< Display-Breite
    constexpr int SCREEN_H = 480;                   ///< Display-Höhe
    constexpr int FOOTER_H = 56;                    ///< Höhe der Fußzeile
    constexpr int VIEWPORT_H = SCREEN_H - FOOTER_H; ///< Höhe über der Fußzeile
    constexpr int FOOTER_Y = VIEWPORT_H;            ///< Obere Kante der Fußzeile

    // Menü (links)
    constexpr int MENU_X = 2;                      ///< Linke Kante der Menüliste
    constexpr int MENU_W = 170;                    ///< Breite der Menüliste
    constexpr int MENU_SPRITE_H = SCREEN_H;        ///< Höhe des Menü-Sprites (volle Displayhöhe)
    constexpr int SECTION_ROW_H = 28;              ///< Zeilenhöhe einer Sektionsüberschrift
    constexpr int ITEM_ROW_H = 36;                 ///< Zeilenhöhe eines Panel-Eintrags; drei Sektionsköpfe und elf Einträge müssen zusammen in MENU_SPRITE_H passen
    constexpr int ITEM_ACCENT_W = 4;               ///< Breite des Kategorie-Farbstreifens
    constexpr int ITEM_TEXT_X = ITEM_ACCENT_W + 6; ///< Texteinzug nach dem Akzent-Balken

    // Scrollbar (zwischen Menü und Arbeitsbereich)
    constexpr int SCROLLBAR_X = MENU_X + MENU_W + 4;     ///< Linke Kante der Scrollbar (4 px Abstand nach Menü)
    constexpr int SCROLLBAR_W = 6;                       ///< Breite der Scrollbar
    constexpr int DIVIDER_X = SCROLLBAR_X + SCROLLBAR_W; ///< Grenze Menübereich Arbeitsbereich

    // Arbeitsbereich (rechts)
    constexpr int WORK_X = SCROLLBAR_X + SCROLLBAR_W + 10; ///< Linke Kante des Arbeitsbereichs (10 px Abstand nach Scrollbar)
    constexpr int WORK_Y = 5;                              ///< Obere Kante des Arbeitsbereichs
    constexpr int WORK_W = SCREEN_W - WORK_X - 10;         ///< Breite des Arbeitsbereichs
    constexpr int WORK_H = VIEWPORT_H - WORK_Y;            ///< Höhe des Arbeitsbereichs
    constexpr int WORK_R = 12;                             ///< Eckradius des Arbeitsbereichs

    // Titelbox im Arbeitsbereich (sprite-relativ)
    constexpr int TITLE_BOX_X = 10;          ///< Linke Kante der Titelbox
    constexpr int TITLE_BOX_Y = 10;          ///< Obere Kante der Titelbox
    constexpr int TITLE_BOX_W = WORK_W - 20; ///< Breite der Titelbox
    constexpr int TITLE_BOX_H = 69;          ///< Höhe der Titelbox
    constexpr int TITLE_BOX_R = 10;          ///< Eckradius der Titelbox
    constexpr int TITLE_TEXT_X = 28;         ///< X-Einzug von Titel und Untertitel
    constexpr int TITLE_TEXT_Y = 18;         ///< Y-Position des Titels
    constexpr int SUB_TEXT_Y = 50;           ///< Y-Position des Untertitels

    // Inhaltsbereich unterhalb der Titelbox (sprite-relativ)
    constexpr int CONTENT_X = TITLE_BOX_X;                    ///< Linke Kante des Inhalts
    constexpr int CONTENT_Y = TITLE_BOX_Y + TITLE_BOX_H + 12; ///< Obere Kante des Inhalts
    constexpr int CONTENT_W = WORK_W - 2 * TITLE_BOX_X;       ///< Breite des Inhalts
    constexpr int CONTENT_H = WORK_H - CONTENT_Y - 10;        ///< Höhe des Inhalts

    // WLAN-Profilzeilen
    constexpr int WIFI_ROW_H = 46;   ///< Höhe einer Profilzeile
    constexpr int WIFI_ROW_GAP = 8;  ///< Abstand zwischen Profilzeilen
    constexpr int WIFI_ACT_H = 44;   ///< Höhe der Aktionsknöpfe
    constexpr int WIFI_ACT_GAP = 10; ///< Abstand zwischen Aktionsknöpfen

    // Geräte-Netzwerkname (Feld am unteren Rand des Arbeitsbereichs)
    constexpr int HOSTNAME_FIELD_H = 64;     ///< Höhe des Netzwerknamen-Feldes
    constexpr int HOSTNAME_BOTTOM_GAP = 10;  ///< Abstand der Feld-Unterkante zur Unterkante des Arbeitsbereichs
    constexpr int HOSTNAME_PAD_X = 14;       ///< Innenabstand links/rechts im Feld
    constexpr int HOSTNAME_LINE_GAP = 4;     ///< Vertikaler Abstand zwischen Beschriftung und Name
    constexpr int HOSTNAME_EDIT_BTN_W = 130; ///< Breite des ändern-Knopfes im Feld
    constexpr int HOSTNAME_EDIT_BTN_H = 44;  ///< Höhe des ändern-Knopfes im Feld

    // Umschalter mit Rahmen, dunkler Füllung und Statusfarbe
    constexpr int TOGGLE_W = 55;      ///< Breite des Umschalters
    constexpr int TOGGLE_H = 29;      ///< Höhe des Umschalters
    constexpr int TOGGLE_MARGIN = 14; ///< Abstand des Umschalters vom rechten Zeilenrand
    constexpr int TOGGLE_INSET = 2;   ///< Randbreite des Gehäuses
    constexpr int TOGGLE_KNOB_R = 10; ///< Radius des Schaltknopfes

    // Sprachzeilen
    constexpr int LANG_ROW_H = 54;   ///< Höhe einer Sprachzeile
    constexpr int LANG_ROW_GAP = 12; ///< Abstand zwischen Sprachzeilen

    // Info-Knöpfe
    constexpr int FOOTER_ACTION_BTN_MIN_W = 120;  ///< Mindestbreite eines Textknopfes im Footer
    constexpr int FOOTER_ACTION_BTN_PAD_X = 24;   ///< Horizontaler Innenabstand eines Textknopfes im Footer
    constexpr int FOOTER_ACTION_BTN_GAP = 12;     ///< Abstand zwischen Textknöpfen im Footer
    constexpr int FOOTER_ACTION_BTN_X0 = WORK_X;  ///< Linker Anfang des Textknopf-Blocks im Footer
    constexpr int FOOTER_ACTION_CLOCK_GAP = 16;   ///< Mindestabstand zwischen Textknöpfen und Uhr

    // PIN-Punkte
    constexpr int PIN_DOT_R = 13;   ///< Radius eines PIN-Punktes
    constexpr int PIN_DOT_GAP = 26; ///< Abstand zwischen PIN-Punkten

    // Fußzeile und Buttons
    constexpr int FOOTER_CY = FOOTER_Y + FOOTER_H / 2;   ///< Vertikale Mitte der Fußzeile
    constexpr int BTN_H = 44;                            ///< Standard-Button-Höhe
    constexpr int BTN_R = 6;                             ///< Standard-Button-Eckradius
    constexpr int SAVE_BTN_W = 150;                      ///< Breite des Speichern-Knopfes
    constexpr int SAVE_BTN_CX = WORK_X + SAVE_BTN_W / 2; ///< X-Mitte des Speichern-Knopfes

    constexpr int EXIT_BTN_SIZE = 44;                              ///< Kantenlänge des Exit-Knopfes
    constexpr int EXIT_BTN_CX = SCREEN_W - EXIT_BTN_SIZE / 2 - 10; ///< X-Mitte (10 px vom rechten Rand)
    constexpr int EXIT_BTN_CY = FOOTER_CY;                         ///< Y-Mitte des Exit-Knopfes

    constexpr int CLOCK_W = 104;                                                 ///< Uhr-Sprite-Breite
    constexpr int CLOCK_H = 45;                                                  ///< Uhr-Sprite-Höhe
    constexpr int CLOCK_PUSH_X = EXIT_BTN_CX - EXIT_BTN_SIZE / 2 - 14 - CLOCK_W; ///< X-Startposition der Uhr (14 px Abstand zum Exit)
    constexpr int CLOCK_PUSH_Y = FOOTER_Y + (FOOTER_H - CLOCK_H) / 2 + 1;        ///< Y-Startposition der Uhr

    // Farben
    constexpr uint16_t C_BG = Theme::ColorMainBackGround; ///< Hintergrundfarbe
    constexpr uint16_t C_WORK_BG = 0x1082;                ///< Hintergrund des Arbeitsbereichs
    constexpr uint16_t C_TITLE_BG = 0x2104;               ///< Hintergrund der Titelbox
    constexpr uint16_t C_TEXT = 0xDEDB;                   ///< Standardtext
    constexpr uint16_t C_TEXT_BRIGHT = 0xFFFF;            ///< Heller Text für gewählten Eintrag
    constexpr uint16_t C_SUB = 0xB5F7;                    ///< Untertitel und Hilfetext
    constexpr uint16_t C_SEC_HDR_TXT = 0xB5F7;            ///< Sektionskopf-Text
    constexpr uint16_t C_SEP_LINE = 0x2945;               ///< Trennlinie zwischen Listeneinträgen
    constexpr uint16_t C_ROW_BG = 0x18C3;                 ///< Hintergrund einer Zeile
    constexpr uint16_t C_ROW_SEL_BG = 0x2945;             ///< Hintergrund der gewählten Zeile
    constexpr uint16_t C_DOT_EMPTY = 0x4208;              ///< Leerer PIN-Punkt

    // Scrollbar (zwischen Menü und Arbeitsbereich)
    constexpr uint16_t C_SCROLLBAR_TRACK = 0x2104;  ///< Scrollbar-Track
    constexpr uint16_t C_SCROLLBAR_NORMAL = 0x5ACB; ///< Scrollbar-Thumb

    // Sektionsfarben
    constexpr uint16_t C_SEC_CONNECTION = 0x07F4; ///< Petrol
    constexpr uint16_t C_SEC_SERVICES = 0xC1AC;   ///< Pflaume
    constexpr uint16_t C_SEC_SYSTEM = 0x07E0;     ///< Grün

    // Status-Farben (WLAN-Profilzeile)
    constexpr uint16_t C_ST_INACTIVE = 0x8410; ///< Status: inaktiv
    constexpr uint16_t C_ST_EMPTY = 0x6B4D;    ///< Status: kein Netz

    // Button-Farben mit dunkler Füllung und heller Kontur
    constexpr uint16_t BTN_OUTLINE = 0x39E8;     ///< Rahmen im Normalzustand
    constexpr uint16_t BTN_FILL = 0x0000;        ///< Füllung im Normalzustand
    constexpr uint16_t BTN_TEXT = 0xAD75;        ///< Text im Normalzustand
    constexpr uint16_t BTN_ACTIVE_FILL = 0x328A; ///< Füllung im aktiven Zustand
    constexpr uint16_t BTN_ACTIVE_TEXT = 0xFFFF; ///< Text im aktiven Zustand

    // Die Firmware-Kennung wird über das Build-Flag FW_VERSION eingespeist.
#ifndef FW_VERSION
#define FW_VERSION "1.0"
#endif

    // Wetter-Panel: PLZ-Eingabe (5 große Ziffernfelder)
    constexpr int PLZ_BOX_W = 50;   ///< Breite einer Ziffer-Box
    constexpr int PLZ_BOX_H = 56;   ///< Höhe einer Ziffer-Box
    constexpr int PLZ_BOX_GAP = 12; ///< Abstand zwischen zwei Ziffer-Boxen
    constexpr int PLZ_DIGITS = 5;   ///< Anzahl Ziffern der Postleitzahl

    // Cloud-Panel: fünf tappbare Zeilen mit Label und Wert, plus Lösch-Knopf
    constexpr int CLOUD_TOP_GAP = 10;      ///< Abstand zwischen Titelbox und erster Cloud-Zeile
    constexpr int CLOUD_ROW_H = 48;        ///< Höhe einer Cloud-Zeile (zwei Textzeilen)
    constexpr int CLOUD_ROW_GAP = 6;       ///< Abstand zwischen Zeilen
    constexpr int CLOUD_ROW_PAD_X = 14;    ///< Innenabstand der URL-Zeile links/rechts
    constexpr int CLOUD_ROW_LABEL_Y = 5;   ///< Y der Beschriftung relativ zum Zeilen-Anfang
    constexpr int CLOUD_ROW_VALUE_Y = 25;  ///< Y der Wertanzeige relativ zum Zeilen-Anfang
    constexpr int CLOUD_VIS_BTN_SIZE = 44; ///< Größe des Sichtbarkeits-Knopfes
    constexpr int CLOUD_VIS_BTN_MARGIN = 12; ///< Abstand des Sichtbarkeits-Knopfes zum Titelbox-Rand
    constexpr int CLOUD_ROW_COUNT = 5;     ///< Fünf Cloud-Werte

    // Zeit-Panel: drei Wahl-Zeilen oben, fünf Stepper-Spalten plus Set-Knopf unten
    constexpr int TIME_FIELD_H = 50;       ///< Höhe einer Wahl-Zeile
    constexpr int TIME_FIELD_GAP = 6;      ///< Abstand zwischen Wahl-Zeilen
    constexpr int TIME_FIELD_PAD_X = 14;   ///< Innenabstand der Wahl-Zeile links/rechts
    constexpr int TIME_FIELD_LABEL_Y = 6;  ///< Y der Beschriftung relativ zum Zeilen-Anfang
    constexpr int TIME_FIELD_VALUE_Y = 26; ///< Y des Wertes relativ zum Zeilen-Anfang
    constexpr int TIME_FIELD_COUNT = 3;    ///< Zeitzone, NTP-Server, Sync-Intervall

    constexpr int TIME_STEPPER_BLOCK_Y = TIME_FIELD_COUNT * (TIME_FIELD_H + TIME_FIELD_GAP) + 6;
    constexpr int TIME_STEPPER_COL_W = 70;  ///< Breite einer Stepper-Spalte
    constexpr int TIME_STEPPER_COL_GAP = 6; ///< Abstand zwischen Stepper-Spalten
    constexpr int TIME_STEPPER_TOTAL_W = 5 * TIME_STEPPER_COL_W + 4 * TIME_STEPPER_COL_GAP;
    constexpr int TIME_STEPPER_BTN_H = 26; ///< Höhe eines Stepper-Knopfes (oben/unten)
    constexpr int TIME_STEPPER_VAL_H = 32; ///< Höhe der Wert-Anzeige in der Mitte
    constexpr int TIME_STEPPER_TOTAL_H = TIME_STEPPER_BTN_H * 2 + TIME_STEPPER_VAL_H + 4;
    constexpr int TIME_SET_BTN_H = 45;  ///< Höhe des Zeit-setzen-Knopfes
    constexpr int TIME_SET_BTN_W = 160; ///< Breite des Zeit-setzen-Knopfes

    // Zeitfenster-Panel: 7 Wochentage x 24 volle Stunden
    constexpr int TIME_WINDOW_DAYS = AppState::TIME_WINDOW_DAY_COUNT;   ///< Anzahl Wochentage
    constexpr int TIME_WINDOW_HOURS = AppState::TIME_WINDOW_HOUR_COUNT; ///< Anzahl voller Stunden
    constexpr int TIME_WINDOW_DAY_LABEL_W = 28; ///< Breite der Wochentags-Spalte
    constexpr int TIME_WINDOW_GRID_W = CONTENT_W - TIME_WINDOW_DAY_LABEL_W; ///< Breite der Stunden-Spalten
    constexpr int TIME_WINDOW_CELL_H = 27;     ///< Höhe einer Wochentags-Zeile
    constexpr int TIME_WINDOW_CHECK_SIZE = 15; ///< Kantenlänge einer Checkbox
    constexpr int TIME_WINDOW_HEADER_H = 26;   ///< Höhe der Stunden-Kopfzeile
    constexpr int TIME_WINDOW_TOP = CONTENT_Y + 10; ///< Obere Kante der Matrix
    constexpr int TIME_WINDOW_DAY_TEXT_X = 8;  ///< Texteinzug der Wochentagskürzel
    constexpr int TIME_WINDOW_HOUR_LABEL_BUF = 3; ///< Puffergröße für zweistellige Stunden
    constexpr int TIME_WINDOW_FRAME_R = 6;     ///< Eckradius des Matrix-Rahmens
    constexpr int TIME_WINDOW_INNER_R = 5;     ///< Eckradius der Innenfläche
    constexpr int TIME_WINDOW_FRAME_INSET = 1; ///< Innenversatz der Matrix-Füllung
    constexpr int TIME_WINDOW_FRAME_PAD_Y = 2; ///< Vertikaler Außenabstand des Matrix-Rahmens
    constexpr int TIME_WINDOW_CHECK_LINE_W = 2; ///< Strichbreite des Hakens
    constexpr int TIME_WINDOW_HINT_GAP_Y = 16; ///< Abstand der Erklärung unter der Matrix
    constexpr int TIME_WINDOW_HINT_LINE_H = 19; ///< Zeilenhöhe der Erklärung
    constexpr int TIME_WINDOW_HINT_X = CONTENT_X + 4; ///< Linke Textkante der Erklärung
    constexpr uint32_t TIME_WINDOW_FULL_MASK = 0x00FFFFFFUL; ///< Alle 24 Stunden aktiv

    // Klang-Panel: fünf Klangwerte und Lautsprecher-Freigabe, je Label links, dann Minus, Wert, Plus
    constexpr int AUDIO_ROW_COUNT = 5;       ///< Tiefen, Mitten, Höhen, Basisbreite, Loudness
    constexpr int AUDIO_WIDTH_ROW = 3;       ///< Zeile für die Basisbreite
    constexpr int AUDIO_ROW_H = 44;          ///< Höhe einer Klang-Zeile
    constexpr int AUDIO_ROW_GAP = 5;         ///< Abstand zwischen den Zeilen
    constexpr int AUDIO_ROW_FIRST_Y = 4;     ///< Y der ersten Zeile relativ zum Inhalt
    constexpr int AUDIO_STEP_BTN_W = 58;     ///< Breite eines Plus/Minus-Knopfes
    constexpr int AUDIO_STEP_VAL_W = 150;    ///< Breite der Wert-Anzeige in der Mitte
    constexpr int AUDIO_STEP_GAP = 10;       ///< Abstand zwischen Knopf und Wert
    constexpr int AUDIO_FREQ_STEP_HZ = 100;  ///< Schrittweite eines Tastendrucks in Hz
    constexpr int AUDIO_WIDTH_STEP_PCT = 10; ///< Schrittweite der Basisbreite in Prozent
    constexpr int AUDIO_LOUDNESS_STEP_PCT = 10; ///< Schrittweite der Loudness in Prozent

    // Eckfrequenz-Grenzen je Band in Hz (konservativ)
    constexpr uint16_t AUDIO_LOW_MIN = 100; // 100 statt 200: erlaubt einen echten Bass-Shelf, der Stimmen unangetastet lässt
    constexpr uint16_t AUDIO_LOW_MAX = 800;
    constexpr uint16_t AUDIO_PEAK_MIN = 1500;
    constexpr uint16_t AUDIO_PEAK_MAX = 6000;
    constexpr uint16_t AUDIO_HIGH_MIN = 4000;
    constexpr uint16_t AUDIO_HIGH_MAX = 12000;
    constexpr uint16_t AUDIO_WIDTH_MIN = 0;
    constexpr uint16_t AUDIO_WIDTH_MAX = 200;
    constexpr uint16_t AUDIO_LOUDNESS_MIN = 0;
    constexpr uint16_t AUDIO_LOUDNESS_MAX = 100;

    // Klang-Footer: Hörprobe-Knopf und Lautstärke (Play, Minus, DotMatrix, Plus)
    constexpr int AUDIO_FT_BTN_SIZE = 44;    ///< Quadratische Footer-Knöpfe
    constexpr int AUDIO_FT_BTN_R = 6;        ///< Eckradius der Footer-Knöpfe
    constexpr int AUDIO_FT_GAP = 8;          ///< Abstand zwischen den Footer-Elementen
    constexpr int AUDIO_FT_VOL_STEP = 5;     ///< Schrittweite der Lautstärke pro Tastendruck
    constexpr int AUDIO_FT_DOT_W = 3;        ///< DotMatrix Punktbreite
    constexpr int AUDIO_FT_DOT_H = 3;        ///< DotMatrix Punkthöhe
    constexpr int AUDIO_FT_DOT_SPACE = 1;    ///< Abstand zwischen Punkten
    constexpr int AUDIO_FT_CHAR_SPACE = 5;   ///< Abstand zwischen Ziffern
    constexpr int AUDIO_FT_CHAR_W = 5 * AUDIO_FT_DOT_W + 4 * AUDIO_FT_DOT_SPACE; ///< Breite einer Ziffer
    constexpr int AUDIO_FT_DISP_W = 2 * AUDIO_FT_CHAR_W + AUDIO_FT_CHAR_SPACE;   ///< Breite der Zahl (zweistellig)
    constexpr int AUDIO_FT_DISP_H = 7 * AUDIO_FT_DOT_H + 6 * AUDIO_FT_DOT_SPACE; ///< Höhe der Zahl
    constexpr int AUDIO_FT_FRAME_PAD = 8;    ///< Innenabstand im Zahlenrahmen
    constexpr int AUDIO_FT_FRAME_W = AUDIO_FT_DISP_W + 2 * AUDIO_FT_FRAME_PAD;   ///< Breite des Zahlenrahmens

    // Klang-Footer: Block aus Play, Vol-, Zahl, Vol+ rechtsbündig vor der Uhr
    constexpr int AUDIO_FT_BLOCK_W = AUDIO_FT_BTN_SIZE * 3 + AUDIO_FT_FRAME_W + AUDIO_FT_GAP * 3; ///< Gesamtbreite des Blocks
    constexpr int AUDIO_FT_BLOCK_RIGHT = CLOCK_PUSH_X - 16;                  ///< Rechte Kante des Blocks (16 px vor der Uhr)
    constexpr int AUDIO_FT_PLAY_CX = AUDIO_FT_BLOCK_RIGHT - AUDIO_FT_BLOCK_W + AUDIO_FT_BTN_SIZE / 2; ///< Play-Knopf-Mitte
    constexpr int AUDIO_FT_VDOWN_CX = AUDIO_FT_PLAY_CX + AUDIO_FT_BTN_SIZE + AUDIO_FT_GAP;            ///< Vol-Minus-Mitte
    constexpr int AUDIO_FT_FRAME_X = AUDIO_FT_VDOWN_CX + AUDIO_FT_BTN_SIZE / 2 + AUDIO_FT_GAP;        ///< Zahlenrahmen linke Kante
    constexpr int AUDIO_FT_DISP_X = AUDIO_FT_FRAME_X + AUDIO_FT_FRAME_PAD;                            ///< DotMatrix linke Kante
    constexpr int AUDIO_FT_VUP_CX = AUDIO_FT_FRAME_X + AUDIO_FT_FRAME_W + AUDIO_FT_GAP + AUDIO_FT_BTN_SIZE / 2; ///< Vol-Plus-Mitte

    // Anzeige-Panel: eine Zeile mit Label links, dann Minus, Wert-Text, Plus
    // Anzeige-Panel: zwei Abschnitte, oben die Röhrenwahl mit Vorschau, unten das VU-Meter.
    // Alle Y-Werte sind Abstände zur Oberkante des Inhaltsbereichs.
    constexpr int APPEARANCE_HINT_H = 23;       ///< Zeilenhöhe von ComfortaaBold16: Aufstrich 18, Unterlänge 5
    constexpr int APPEARANCE_SEC_LINE_DY = 24;  ///< Trennlinie unter dem Text der Überschrift
    constexpr int APPEARANCE_SEC1_Y = 0;        ///< Überschrift des ersten Abschnitts
    constexpr int APPEARANCE_TILE_Y = 29;       ///< Obere Kante der Röhrenkacheln
    constexpr int APPEARANCE_TILE_W = 134;      ///< Breite einer Röhrenkachel
    constexpr int APPEARANCE_TILE_H = 94;       ///< Höhe einer Röhrenkachel
    constexpr int APPEARANCE_TILE_GAP = 14;     ///< Abstand zwischen den Kacheln
    constexpr int APPEARANCE_EYE_BOX = 70;      ///< Kantenlänge der Röhrendarstellung, wie im Radio
    constexpr int APPEARANCE_HINT1_Y = 128;     ///< Hinweiszeile des ersten Abschnitts
    constexpr int APPEARANCE_SEC2_Y = 159;      ///< Überschrift des zweiten Abschnitts
    constexpr int APPEARANCE_ROW1_Y = 189;      ///< Zeile der Zeigerträgheit
    constexpr int APPEARANCE_ROW2_Y = 256;      ///< Zeile des Bezugspegels
    constexpr int APPEARANCE_ROW_H = 36;        ///< Höhe einer Wahlzeile
    constexpr int APPEARANCE_HINT_DY = 39;      ///< Abstand der Hinweiszeile unter ihrer Wahlzeile

    // Der Inhaltsbereich endet ohnehin 10 Bildpunkte über der Panelkante. Endet die letzte
    // Hinweiszeile genau auf seiner Unterkante, stimmt dieser Abstand; der Rest verteilt sich
    // auf die Lücken darüber. Wer eine Zeile verschiebt, muss die Rechnung wieder aufgehen lassen.
    static_assert(APPEARANCE_ROW2_Y + APPEARANCE_HINT_DY + APPEARANCE_HINT_H == CONTENT_H,
                  "Die letzte Hinweiszeile muss genau auf der Unterkante des Inhalts enden");
    constexpr int APPEARANCE_LABEL_W = 150;     ///< Breite der Beschriftung links neben den Segmenten
    constexpr int APPEARANCE_SEG_GAP = 6;       ///< Abstand zwischen zwei Segmenten
    constexpr int APPEARANCE_LEVEL_W = 220;     ///< Breite der Pegelanzeige rechts in der Abschnittszeile
    constexpr unsigned long APPEARANCE_LEVEL_MS = 1000; ///< Auffrischung der Pegelanzeige
    constexpr float APPEARANCE_LEVEL_SMOOTH = 0.05f;    ///< Glättung je Durchlauf, ergibt rund eine Sekunde
    constexpr uint8_t APPEARANCE_EYE_MIN = 0;   ///< Kleinster Stilwert des magischen Auges (EM11)
    constexpr uint8_t APPEARANCE_EYE_MAX = 3;   ///< Größter Stilwert des magischen Auges (EM84), muss zur Aufzählung MagicEyeStyle in include/RetroRadio.h passen
    constexpr uint8_t APPEARANCE_VU_MIN = 0;    ///< Kleinste Trägheit
    constexpr uint8_t APPEARANCE_VU_MAX = 2;    ///< Größte Trägheit, muss zur Aufzählung VuBallistics in include/Visualizer.h passen
    constexpr uint8_t APPEARANCE_SENS_MIN = 0;  ///< Kleinster Bezugspegel
    constexpr uint8_t APPEARANCE_SENS_MAX = 4;  ///< Größter Bezugspegel, muss zur Tabelle VU_ALIGNMENT_DB in src/Visualizer.cpp passen

    /**
     * @brief Zeitzonen-Tabelle: Region-Bezeichner -> POSIX-Zeichenkette.
     *
     * Die Region-Namen folgen der IANA-Schreibweise und gelten sprachübergreifend gleichermaßen. POSIX-Strings codieren den UTC-Versatz und gegebenenfalls die Sommerzeitregeln, sodass die Zeitumstellung ohne weiteren Umschalter korrekt funktioniert.
     */
    struct TimezoneEntry
    {
        const char *region;
        const char *posix;
    };

    const TimezoneEntry TIMEZONES[] = {
        {"Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Vienna", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Zurich", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Brussels", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Rome", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0"},
        {"Europe/Dublin", "IST-1GMT0,M10.5.0,M3.5.0/1"},
        {"Europe/Lisbon", "WET0WEST,M3.5.0/1,M10.5.0"},
        {"Europe/Athens", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
        {"Europe/Helsinki", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
        {"Europe/Stockholm", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Oslo", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Copenhagen", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Warsaw", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Prague", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Budapest", "CET-1CEST,M3.5.0,M10.5.0/3"},
        {"Europe/Moscow", "MSK-3"},
        {"Europe/Kiev", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
        {"Europe/Istanbul", "TRT-3"},
        {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
        {"America/Chicago", "CST6CDT,M3.2.0,M11.1.0"},
        {"America/Denver", "MST7MDT,M3.2.0,M11.1.0"},
        {"America/Los_Angeles", "PST8PDT,M3.2.0,M11.1.0"},
        {"America/Anchorage", "AKST9AKDT,M3.2.0,M11.1.0"},
        {"America/Phoenix", "MST7"},
        {"America/Toronto", "EST5EDT,M3.2.0,M11.1.0"},
        {"America/Vancouver", "PST8PDT,M3.2.0,M11.1.0"},
        {"America/Mexico_City", "CST6"},
        {"America/Sao_Paulo", "<-03>3"},
        {"America/Buenos_Aires", "<-03>3"},
        {"Asia/Tokyo", "JST-9"},
        {"Asia/Seoul", "KST-9"},
        {"Asia/Shanghai", "CST-8"},
        {"Asia/Hong_Kong", "HKT-8"},
        {"Asia/Singapore", "SGT-8"},
        {"Asia/Bangkok", "ICT-7"},
        {"Asia/Dubai", "GST-4"},
        {"Asia/Kolkata", "IST-5:30"},
        {"Asia/Jakarta", "WIB-7"},
        {"Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
        {"Australia/Melbourne", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
        {"Australia/Perth", "AWST-8"},
        {"Pacific/Auckland", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
        {"Pacific/Honolulu", "HST10"},
        {"Africa/Cairo", "EET-2EEST,M4.5.5/0,M10.5.4/24"},
        {"Africa/Johannesburg", "SAST-2"},
        {"UTC", "UTC0"},
    };

    constexpr int TIMEZONES_COUNT = sizeof(TIMEZONES) / sizeof(TIMEZONES[0]);

    /**
     * @brief Sync-Intervall-Stufen: Millisekunden und i18n-Label-Schlüssel.
     *
     * Reihenfolge von lang nach kurz, damit die Standardwahl oben liegt und die kurzen Werte am Ende warnend stehen.
     */
    struct SyncIntervalEntry
    {
        unsigned long ms;
        SettingsKey labelKey;
    };

    const SyncIntervalEntry SYNC_INTERVALS[] = {
        {86400000UL, SettingsKey::SETTINGS_TIME_SYNC_24H},
        {43200000UL, SettingsKey::SETTINGS_TIME_SYNC_12H},
        {21600000UL, SettingsKey::SETTINGS_TIME_SYNC_6H},
        {10800000UL, SettingsKey::SETTINGS_TIME_SYNC_3H},
        {3600000UL, SettingsKey::SETTINGS_TIME_SYNC_1H},
        {1800000UL, SettingsKey::SETTINGS_TIME_SYNC_30M},
        {600000UL, SettingsKey::SETTINGS_TIME_SYNC_10M},
        {60000UL, SettingsKey::SETTINGS_TIME_SYNC_1M},
    };

    constexpr int SYNC_INTERVALS_COUNT = sizeof(SYNC_INTERVALS) / sizeof(SYNC_INTERVALS[0]);

    /**
     * @brief Sucht den Index eines POSIX-Zeitzonen-Strings in TIMEZONES.
     * @return -1 wenn nicht gefunden
     */
    int findTimezoneIndex(const char *region)
    {
        if (!region || !*region)
            return -1;
        for (int i = 0; i < TIMEZONES_COUNT; ++i)
        {
            if (strcmp(region, TIMEZONES[i].region) == 0)
                return i;
        }
        return -1;
    }

    /**
     * @brief Sucht den Index eines Sync-Intervalls in SYNC_INTERVALS.
     * @return Index der nächstliegenden Stufe; mindestens 0, höchstens letzte Position
     */
    int findSyncIntervalIndex(unsigned long ms)
    {
        for (int i = 0; i < SYNC_INTERVALS_COUNT; ++i)
        {
            if (SYNC_INTERVALS[i].ms == ms)
                return i;
        }
        return 4; // Default: 1 Stunde
    }

    /**
     * @brief Liefert die Anzahl Tage in einem Monat (gregorianisch, mit Schaltjahr).
     */
    int daysInMonth(int year, int month)
    {
        switch (month)
        {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31;
        case 4:
        case 6:
        case 9:
        case 11:
            return 30;
        case 2:
        {
            const bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            return leap ? 29 : 28;
        }
        default:
            return 30;
        }
    }

    // Felder im Schrittgeberblock. Die Reihenfolge in der Spalte ist sprachabhängig: Deutsch zeigt Tag-Monat-Jahr, Englisch zeigt Jahr-Monat-Tag.
    constexpr int FIELD_YEAR = 0;
    constexpr int FIELD_MONTH = 1;
    constexpr int FIELD_DAY = 2;
    constexpr int FIELD_HOUR = 3;
    constexpr int FIELD_MINUTE = 4;
    constexpr int FIELD_COUNT = 5;

    /**
     * @brief Liefert das Feld, das in einer Spalte angezeigt wird.
     *
     * Die ersten drei Spalten (Datum) folgen der Sprache: DE = Tag/Monat/Jahr, EN = Jahr/Monat/Tag. Stunde und Minute bleiben in beiden Sprachen rechts.
     */
    int stepperColToField(int col)
    {
        const bool german = (I18n::currentLang() == LangIndex::DE);
        if (german)
        {
            switch (col)
            {
            case 0:
                return FIELD_DAY;
            case 1:
                return FIELD_MONTH;
            case 2:
                return FIELD_YEAR;
            case 3:
                return FIELD_HOUR;
            case 4:
                return FIELD_MINUTE;
            }
        }
        else
        {
            switch (col)
            {
            case 0:
                return FIELD_YEAR;
            case 1:
                return FIELD_MONTH;
            case 2:
                return FIELD_DAY;
            case 3:
                return FIELD_HOUR;
            case 4:
                return FIELD_MINUTE;
            }
        }
        return FIELD_YEAR;
    }

    // Geocoding (Nominatim/OpenStreetMap) für die PLZ-Auflösung
    constexpr int GEOCODE_CITY_BUF = 64;         ///< Puffergröße für den Ortsnamen
    constexpr int GEOCODE_URL_BUF = 192;         ///< Puffergröße für die Such-URL
    constexpr int GEOCODE_JSON_DOC_BYTES = 8192; ///< Größe des JSON-Dokuments
    constexpr double DE_BBOX_LAT_MIN = 47.2;     ///< Untere Breite der Deutschland-Box
    constexpr double DE_BBOX_LAT_MAX = 55.1;     ///< Obere Breite der Deutschland-Box
    constexpr double DE_BBOX_LON_MIN = 5.8;      ///< Westliche Länge der Deutschland-Box
    constexpr double DE_BBOX_LON_MAX = 15.1;     ///< Östliche Länge der Deutschland-Box

    /**
     * @brief Tabelle: Bundesland-Name aus der Nominatim-Antwort -> Region-ID (1..16).
     *
     * Enthält die Primärnamen sowie die Varianten, die Nominatim häufig liefert ("Land X", "Freistaat X", Schreibweisen ohne Bindestrich). Die Zuordnung muss mit der Weboberfläche übereinstimmen.
     */
    struct StateRegionEntry
    {
        const char *name;
        int region;
    };

    const StateRegionEntry STATE_REGION_MAP[] = {
        // Primärnamen
        {"Baden-Württemberg", 1},
        {"Bayern", 2},
        {"Berlin", 3},
        {"Brandenburg", 4},
        {"Bremen", 5},
        {"Hamburg", 6},
        {"Hessen", 7},
        {"Mecklenburg-Vorpommern", 8},
        {"Niedersachsen", 9},
        {"Nordrhein-Westfalen", 10},
        {"Rheinland-Pfalz", 11},
        {"Saarland", 12},
        {"Sachsen", 13},
        {"Sachsen-Anhalt", 14},
        {"Schleswig-Holstein", 15},
        {"Thüringen", 16},
        // Nominatim-Varianten
        {"Land Berlin", 3},
        {"Land Brandenburg", 4},
        {"Freistaat Bayern", 2},
        {"Freistaat Sachsen", 13},
        {"Freistaat Thüringen", 16},
        {"Hansestadt Hamburg", 6},
        {"Freie Hansestadt Bremen", 5},
        {"Nordrhein Westfalen", 10},
        {"Rheinland Pfalz", 11},
    };

    /**
     * @brief Liefert die Nachrichten-Region (1..16) zu einem Bundesland-Namen.
     * @return 0, wenn der Name nicht in der Mapping-Tabelle steht.
     */
    int stateNameToRegion(const char *name)
    {
        if (!name || !*name)
            return 0;
        for (const auto &e : STATE_REGION_MAP)
        {
            if (strcmp(name, e.name) == 0)
                return e.region;
        }
        return 0;
    }

    // News-Panel: Layout der Ressort-Chips und des Bundesland-Selektors
    constexpr int NEWS_CHIP_H = 40;       ///< Höhe eines Ressort-Chips
    constexpr int NEWS_CHIP_GAP_X = 8;    ///< Horizontaler Abstand zwischen Chips
    constexpr int NEWS_CHIP_GAP_Y = 8;    ///< Vertikaler Abstand zwischen Chip-Reihen
    constexpr int NEWS_CHIPS_PER_ROW = 4; ///< Chips pro Reihe
    constexpr int NEWS_RESSORT_COUNT = 8; ///< Anzahl Ressort-Chips
    constexpr int NEWS_SELECTOR_H = 44;   ///< Höhe des Bundesland-Wahlknopfes

    // Listen-Picker (Modal, zentriert über dem Hauptlayout)
    constexpr int PICK_W = 460; ///< Breite des Picker-Rahmens
    constexpr int PICK_H = 400; ///< Höhe des Picker-Rahmens
    constexpr int PICK_X = (SCREEN_W - PICK_W) / 2;
    constexpr int PICK_Y = (SCREEN_H - PICK_H) / 2;
    constexpr int PICK_R = 12;                   ///< Eckradius des Rahmens
    constexpr int PICK_PADDING = 16;             ///< Innenabstand
    constexpr int PICK_TITLE_H = 36;             ///< Höhe der Titelzeile
    constexpr int PICK_CANCEL_H = 44;            ///< Höhe des Abbruch-Knopfes
    constexpr int PICK_ITEM_H = 36;              ///< Höhe eines Listeneintrags
    constexpr int PICK_ITEM_GAP = 2;             ///< Abstand zwischen Listeneinträgen
    constexpr int PICK_DRAG_THRESH = 5;          ///< Pixel-Schwelle: Tipp vs. Wisch
    constexpr uint16_t C_PICK_BG = 0x18C3;       ///< Hintergrund des Pickers
    constexpr uint16_t C_PICK_BORDER = 0x39E8;   ///< Rahmen des Pickers
    constexpr uint16_t C_PICK_ITEM_BG = 0x0000;  ///< Eintrag im Normalzustand
    constexpr uint16_t C_PICK_ITEM_SEL = 0x2945; ///< Eintrag im gewählten Zustand

    /**
     * @brief Tabelle der Ressort-Werte und ihrer Anzeigetexte.
     *
     * Erste Spalte ist der String, der in AppState landet (leer = kein Filter).
     */
    struct NewsRessortEntry
    {
        const char *value;
        SettingsKey labelKey;
    };

    const NewsRessortEntry NEWS_RESSORTS[NEWS_RESSORT_COUNT] = {
        {"", SettingsKey::SETTINGS_NEWS_RESSORT_ALL},
        {"inland", SettingsKey::SETTINGS_NEWS_RESSORT_INLAND},
        {"ausland", SettingsKey::SETTINGS_NEWS_RESSORT_AUSLAND},
        {"wirtschaft", SettingsKey::SETTINGS_NEWS_RESSORT_WIRTSCHAFT},
        {"sport", SettingsKey::SETTINGS_NEWS_RESSORT_SPORT},
        {"video", SettingsKey::SETTINGS_NEWS_RESSORT_VIDEO},
        {"investigativ", SettingsKey::SETTINGS_NEWS_RESSORT_INVESTIGATIV},
        {"wissen", SettingsKey::SETTINGS_NEWS_RESSORT_WISSEN},
    };

    /**
     * @brief Liefert den Anzeige-Key für eine Bundesland-ID (1..16).
     *
     * Bei ungültiger ID fällt die Funktion auf Brandenburg zurück.
     */
    SettingsKey newsRegionKey(int region)
    {
        switch (region)
        {
        case 1:
            return SettingsKey::SETTINGS_NEWS_BL_BW;
        case 2:
            return SettingsKey::SETTINGS_NEWS_BL_BY;
        case 3:
            return SettingsKey::SETTINGS_NEWS_BL_BE;
        case 4:
            return SettingsKey::SETTINGS_NEWS_BL_BB;
        case 5:
            return SettingsKey::SETTINGS_NEWS_BL_HB;
        case 6:
            return SettingsKey::SETTINGS_NEWS_BL_HH;
        case 7:
            return SettingsKey::SETTINGS_NEWS_BL_HE;
        case 8:
            return SettingsKey::SETTINGS_NEWS_BL_MV;
        case 9:
            return SettingsKey::SETTINGS_NEWS_BL_NI;
        case 10:
            return SettingsKey::SETTINGS_NEWS_BL_NW;
        case 11:
            return SettingsKey::SETTINGS_NEWS_BL_RP;
        case 12:
            return SettingsKey::SETTINGS_NEWS_BL_SL;
        case 13:
            return SettingsKey::SETTINGS_NEWS_BL_SN;
        case 14:
            return SettingsKey::SETTINGS_NEWS_BL_ST;
        case 15:
            return SettingsKey::SETTINGS_NEWS_BL_SH;
        case 16:
            return SettingsKey::SETTINGS_NEWS_BL_TH;
        default:
            return SettingsKey::SETTINGS_NEWS_BL_BB;
        }
    }

    /** @brief Prüft, ob ein Punkt in einem Rechteck liegt. */
    bool pointIn(int px, int py, int x, int y, int w, int h)
    {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
} // namespace

// ===================================================================
// Statische Menüliste
// ===================================================================

// Reihenfolge bestimmt die Anzeige von oben nach unten.
const Settings::MenuEntry Settings::s_menu[] = {
    {EntryType::SECTION_HEADER, Section::CONNECTION, Panel::WIFI, SettingsKey::SETTINGS_SEC_CONNECTION},
    {EntryType::PANEL_ITEM, Section::CONNECTION, Panel::WIFI, SettingsKey::SETTINGS_MENU_WIFI},
    {EntryType::PANEL_ITEM, Section::CONNECTION, Panel::TIME, SettingsKey::SETTINGS_MENU_TIME},
    {EntryType::PANEL_ITEM, Section::CONNECTION, Panel::TIME_WINDOW, SettingsKey::SETTINGS_MENU_TIME_WINDOW},

    {EntryType::SECTION_HEADER, Section::SERVICES, Panel::WEATHER, SettingsKey::SETTINGS_SEC_SERVICES},
    {EntryType::PANEL_ITEM, Section::SERVICES, Panel::WEATHER, SettingsKey::SETTINGS_MENU_WEATHER},
    {EntryType::PANEL_ITEM, Section::SERVICES, Panel::NEWS, SettingsKey::SETTINGS_MENU_NEWS},
    {EntryType::PANEL_ITEM, Section::SERVICES, Panel::CLOUD, SettingsKey::SETTINGS_MENU_CLOUD},

    {EntryType::SECTION_HEADER, Section::SYSTEM, Panel::LANGUAGE, SettingsKey::SETTINGS_SEC_SYSTEM},
    {EntryType::PANEL_ITEM, Section::SYSTEM, Panel::LANGUAGE, SettingsKey::SETTINGS_MENU_LANGUAGE},
    {EntryType::PANEL_ITEM, Section::SYSTEM, Panel::SECURITY, SettingsKey::SETTINGS_MENU_SECURITY},
    {EntryType::PANEL_ITEM, Section::SYSTEM, Panel::AUDIO, SettingsKey::SETTINGS_MENU_AUDIO},
    {EntryType::PANEL_ITEM, Section::SYSTEM, Panel::APPEARANCE, SettingsKey::SETTINGS_MENU_APPEARANCE},
    {EntryType::PANEL_ITEM, Section::SYSTEM, Panel::INFO, SettingsKey::SETTINGS_MENU_INFO},
};

const int Settings::s_menuCount = sizeof(Settings::s_menu) / sizeof(Settings::s_menu[0]);

// ===================================================================
// Lebenszyklus
// ===================================================================

/**
 * @brief Konstruktor, setzt alle Profile auf leer.
 */
Settings::Settings()
{
    for (auto &p : m_profiles)
    {
        p.ssid[0] = '\0';
        p.password[0] = '\0';
        p.enabled = false;
        p.modified = false;
        p.tested_ok = false;
    }
    for (auto &b : m_baseline)
    {
        b.ssid[0] = '\0';
        b.password[0] = '\0';
        b.enabled = false;
    }
    m_newsRessort[0] = '\0';
    m_newsRessortBaseline[0] = '\0';
    m_newsRegion = 4;
    m_newsRegionBaseline = 4;
    m_plzEntry[0] = '\0';
    m_plzBaseline[0] = '\0';
    m_plzLen = 0;
    m_cloudCalendarUrl[0] = '\0';
    m_cloudTodoUrl[0] = '\0';
    m_cloudContactsUrl[0] = '\0';
    m_cloudOpenSkyClientId[0] = '\0';
    m_cloudOpenSkyClientSecret[0] = '\0';
    m_cloudCalendarBaseline[0] = '\0';
    m_cloudTodoBaseline[0] = '\0';
    m_cloudContactsBaseline[0] = '\0';
    m_cloudOpenSkyClientIdBaseline[0] = '\0';
    m_cloudOpenSkyClientSecretBaseline[0] = '\0';
    m_cloudValuesVisible = false;
    m_tzIndex = -1;
    m_tzBaseline = -1;
    m_ntpServer[0] = '\0';
    m_ntpServerBaseline[0] = '\0';
    m_syncIntervalMs = 3600000UL;
    m_syncIntervalBaselineMs = 3600000UL;
    m_manualYear = 2025;
    m_manualMonth = 1;
    m_manualDay = 1;
    m_manualHour = 12;
    m_manualMinute = 0;
    m_manualInit = false;
    for (int day = 0; day < TIME_WINDOW_DAYS; ++day)
    {
        for (int hour = 0; hour < TIME_WINDOW_HOURS; ++hour)
            m_timeWindow[day][hour] = true;
        m_timeWindowBaselineMasks[day] = TIME_WINDOW_FULL_MASK;
    }
}

/**
 * @brief Destruktor, gibt Dialoge und Sprites frei.
 */
Settings::~Settings()
{
    // Beim Verlassen die Soundausgabe stoppen, damit kein inaktives Modul Ton hält
    if (m_callbacks.requestStopPlayback)
        m_callbacks.requestStopPlayback();
    m_previewPlaying = false;
    applySpeakerOutput(APP_STATE.getSpeakersEnabled());

    m_activeMessageBox.reset();
    m_activeProgressBox.reset();
    deleteSprites();
}

/**
 * @brief Initialisiert das Modul, lädt Profile und erstellt die Sprites.
 */
bool Settings::init(LGFX &tft, const SettingsCallbacks &callbacks)
{
    m_tft = &tft;
    m_callbacks = callbacks;

    // Profile aus AppState laden
    for (int i = 0; i < MAX_PROFILES; ++i)
    {
        char ssid[65] = {0};
        char password[65] = {0};
        if (APP_STATE.getWiFiProfile(i, ssid, sizeof(ssid), password, sizeof(password)))
        {
            stringutils::safeCopy(m_profiles[i].ssid, ssid);
            stringutils::safeCopy(m_profiles[i].password, password);
            m_profiles[i].enabled = APP_STATE.isWiFiProfileEnabled(i);
        }
        else
        {
            m_profiles[i].ssid[0] = '\0';
            m_profiles[i].password[0] = '\0';
            m_profiles[i].enabled = false;
        }
        m_profiles[i].modified = false;
        m_profiles[i].tested_ok = false;
    }

    // Geräte-Netzwerkname aus AppState laden, Baseline = aktueller Wert
    stringutils::safeCopy(m_hostname, APP_STATE.getHostname());
    stringutils::safeCopy(m_hostnameBaseline, m_hostname);

    // News-Filter aus AppState laden, Baseline = aktueller Wert
    stringutils::safeCopy(m_newsRessort, APP_STATE.getNewsRessort());
    stringutils::safeCopy(m_newsRessortBaseline, m_newsRessort);
    m_newsRegion = APP_STATE.getNewsRegion();
    if (m_newsRegion < 1 || m_newsRegion > 16)
        m_newsRegion = 4;
    m_newsRegionBaseline = m_newsRegion;

    // Postleitzahl aus AppState laden, Baseline = aktueller Wert
    stringutils::safeCopy(m_plzEntry, APP_STATE.getPlz());
    stringutils::safeCopy(m_plzBaseline, m_plzEntry);
    m_plzLen = static_cast<int>(strlen(m_plzEntry));
    if (m_plzLen > 5)
        m_plzLen = 5;

    // Cloud-Anbindung aus AppState laden, Baselines = aktueller Stand
    stringutils::safeCopy(m_cloudCalendarUrl, APP_STATE.getCalendarScriptUrl());
    stringutils::safeCopy(m_cloudTodoUrl, APP_STATE.getTodoScriptUrl());
    stringutils::safeCopy(m_cloudContactsUrl, APP_STATE.getContactsScriptUrl());
    stringutils::safeCopy(m_cloudOpenSkyClientId, APP_STATE.getOpenSkyClientId());
    stringutils::safeCopy(m_cloudOpenSkyClientSecret, APP_STATE.getOpenSkyClientSecret());
    stringutils::safeCopy(m_cloudCalendarBaseline, m_cloudCalendarUrl);
    stringutils::safeCopy(m_cloudTodoBaseline, m_cloudTodoUrl);
    stringutils::safeCopy(m_cloudContactsBaseline, m_cloudContactsUrl);
    stringutils::safeCopy(m_cloudOpenSkyClientIdBaseline, m_cloudOpenSkyClientId);
    stringutils::safeCopy(m_cloudOpenSkyClientSecretBaseline, m_cloudOpenSkyClientSecret);

    // Zeit-Einstellungen aus AppState übernehmen und Baselines setzen
    m_tzIndex = findTimezoneIndex(APP_STATE.getTimezoneRegion());
    m_tzBaseline = m_tzIndex;
    stringutils::safeCopy(m_ntpServer, APP_STATE.getNtpServer());
    stringutils::safeCopy(m_ntpServerBaseline, m_ntpServer);
    m_syncIntervalMs = APP_STATE.getNtpSyncInterval();
    if (m_syncIntervalMs < 60000UL)
        m_syncIntervalMs = 60000UL;
    m_syncIntervalBaselineMs = m_syncIntervalMs;
    m_manualInit = false;

    // Zeitfenster aus AppState laden, Baseline = aktueller Stand
    loadTimeWindowFromAppState();

    // Klangregelung: Eckfrequenzen aus AppState laden, Baselines = aktueller Stand
    m_eqFreqLow = APP_STATE.getEqFreqLow();
    m_eqFreqPeak = APP_STATE.getEqFreqPeak();
    m_eqFreqHigh = APP_STATE.getEqFreqHigh();
    m_stereoWidth = APP_STATE.getStereoWidth();
    m_loudness = APP_STATE.getLoudness();
    m_speakersEnabled = APP_STATE.getSpeakersEnabled();
    m_eqFreqLowBaseline = m_eqFreqLow;
    m_eqFreqPeakBaseline = m_eqFreqPeak;
    m_eqFreqHighBaseline = m_eqFreqHigh;
    m_stereoWidthBaseline = m_stereoWidth;
    m_loudnessBaseline = m_loudness;
    m_speakersEnabledBaseline = m_speakersEnabled;

    // Anzeige: Stil des magischen Auges aus AppState laden, Baseline = aktueller Stand
    m_magicEyeStyle = APP_STATE.getMagicEyeStyle();
    if (m_magicEyeStyle > APPEARANCE_EYE_MAX)
        m_magicEyeStyle = APPEARANCE_EYE_MIN;
    m_magicEyeStyleBaseline = m_magicEyeStyle;

    // Anzeige: Trägheit des VU-Zeigers aus AppState laden, Baseline = aktueller Stand
    m_vuBallistics = APP_STATE.getVuBallistics();
    if (m_vuBallistics > APPEARANCE_VU_MAX)
        m_vuBallistics = APPEARANCE_VU_MIN;
    m_vuBallisticsBaseline = m_vuBallistics;

    // Anzeige: Empfindlichkeit des VU aus AppState laden, Baseline = aktueller Stand
    m_vuSensitivity = APP_STATE.getVuSensitivity();
    if (m_vuSensitivity > APPEARANCE_SENS_MAX)
        m_vuSensitivity = APPEARANCE_SENS_MIN;
    m_vuSensitivityBaseline = m_vuSensitivity;

    snapshotBaseline();
    recomputeDirtyFlags();

    if (!createSprites())
    {
        LOG_ERROR("Settings: Sprite-Erzeugung fehlgeschlagen");
        return false;
    }

    initButtons();

    m_panel = Panel::WIFI;
    m_selWifi = 0;
    resetPinEntry();

    m_tft->clear(C_BG);
    m_state = State::IDLE;
    m_testState = TestState::NOT_TESTING;

    invalidateAll();

    m_requireTouchRelease = true;
    m_touchBlockUntil = millis() + 150;
    m_touchProcessed = false;

    m_lastObservedWifi = APP_STATE.getWifiConnected();

    m_initialized = true;
    LOG_SUCCESS("Settings-Modul initialisiert");
    return true;
}

/**
 * @brief Gibt den Modultyp zurück.
 */
AppModule Settings::type() const
{
    return AppModule::SETTINGS;
}

/**
 * @brief Erstellt die PSRAM-Sprites für Menü und Arbeitsbereich.
 */
bool Settings::createSprites()
{
    if (!m_tft)
        return false;

    if (!m_tft->safeCreateSprite(m_menuSprite, MENU_W, MENU_SPRITE_H))
    {
        deleteSprites();
        return false;
    }
    if (!m_tft->safeCreateSprite(m_scrollbarSprite, SCROLLBAR_W, MENU_SPRITE_H))
    {
        deleteSprites();
        return false;
    }
    if (!m_tft->safeCreateSprite(m_workSprite, WORK_W, WORK_H))
    {
        deleteSprites();
        return false;
    }
    if (!m_tft->safeCreateSprite(m_clockSprite, CLOCK_W, CLOCK_H))
    {
        deleteSprites();
        return false;
    }
    if (!m_tft->safeCreateSprite(m_audioVolSprite, AUDIO_FT_DISP_W, AUDIO_FT_DISP_H))
        LOG_WARNING("Settings: Lautstärke-Sprite konnte nicht erstellt werden");

    m_menuSprite->clear(C_BG);
    m_scrollbarSprite->clear(C_SCROLLBAR_TRACK);
    m_workSprite->clear(C_BG);
    m_clockSprite->clear(C_BG);
    return true;
}

/**
 * @brief Gibt die Sprites frei.
 */
void Settings::deleteSprites()
{
    if (!m_tft)
        return;

    m_tft->safeDeleteSprite(m_menuSprite);
    m_tft->safeDeleteSprite(m_scrollbarSprite);
    m_tft->safeDeleteSprite(m_workSprite);
    m_tft->safeDeleteSprite(m_clockSprite);
    m_tft->safeDeleteSprite(m_audioVolSprite);
}

/**
 * @brief Initialisiert einen ins Work-Sprite zeichnenden Werkzeug-Button.
 * @param btn Zu initialisierender Button
 * @param spriteRect Geometrie sprite-relativ (Ursprung = Work-Sprite oben links)
 */
void Settings::initToolButton(Button &btn, const Rect &spriteRect)
{
    const int cx = WORK_X + spriteRect.x + spriteRect.w / 2;
    const int cy = WORK_Y + spriteRect.y + spriteRect.h / 2;
    btn.initButton(m_tft, cx, cy, spriteRect.w, spriteRect.h, BTN_R,
                   BTN_OUTLINE, BTN_FILL, BTN_TEXT, "", nullptr);
    btn.setFont(LeagueGothicCond34);
    btn.setDrawTarget(m_workSprite, WORK_X, WORK_Y);
}

/**
 * @brief Initialisiert die Fußzeilen- und Werkzeug-Buttons.
 */
void Settings::initButtons()
{
    m_exitButton.initButton(
        m_tft,
        EXIT_BTN_CX, EXIT_BTN_CY,
        EXIT_BTN_SIZE, EXIT_BTN_SIZE,
        BTN_R,
        BTN_OUTLINE,
        BTN_FILL,
        uint16_t(TFT_DARKGREY),
        "^POWER:32:D206^",
        nullptr);
    m_exitButton.setFont(ComfortaaBold30);

    m_saveButton.initButton(
        m_tft,
        SAVE_BTN_CX, FOOTER_CY,
        SAVE_BTN_W, BTN_H,
        BTN_R,
        BTN_OUTLINE,
        BTN_FILL,
        BTN_TEXT,
        "",
        nullptr);
    m_saveButton.setFont(LeagueGothicCond34);

    initToolButton(m_btnScan, wifiActionRect(0));
    initToolButton(m_btnPassword, wifiActionRect(1));
    initToolButton(m_btnTest, wifiActionRect(2));
    initToolButton(m_btnDelete, wifiActionRect(3));
    initToolButton(m_cloudVisibilityButton, cloudVisibilityButtonRect());
    // Info-Knöpfe liegen im Footer und zeichnen direkt aufs Display.
    {
        const Rect r = footerInfoButtonRect(0);
        m_btnRestart.initButton(m_tft,
                                r.x + r.w / 2, r.y + r.h / 2,
                                r.w, r.h, BTN_R,
                                BTN_OUTLINE, BTN_FILL, BTN_TEXT,
                                "", nullptr);
        m_btnRestart.setFont(LeagueGothicCond34);
    }
    {
        const Rect r = footerInfoButtonRect(1);
        m_btnReset.initButton(m_tft,
                              r.x + r.w / 2, r.y + r.h / 2,
                              r.w, r.h, BTN_R,
                              BTN_OUTLINE, BTN_FILL, BTN_TEXT,
                              "", nullptr);
        m_btnReset.setFont(LeagueGothicCond34);
    }

    // Klang-Footer: Hörprobe-Knopf und Lautstärke-Knöpfe (Icon-Beschriftung)
    m_audioPlayBtn.initButton(m_tft,
                              AUDIO_FT_PLAY_CX, FOOTER_CY,
                              AUDIO_FT_BTN_SIZE, AUDIO_FT_BTN_SIZE, AUDIO_FT_BTN_R,
                              BTN_OUTLINE, BTN_FILL, BTN_TEXT,
                              "^PLAY:32:5C44^", nullptr);
    m_audioPlayBtn.setFont(ComfortaaBold30);

    m_audioVolDownBtn.initButton(m_tft,
                                 AUDIO_FT_VDOWN_CX, FOOTER_CY,
                                 AUDIO_FT_BTN_SIZE, AUDIO_FT_BTN_SIZE, AUDIO_FT_BTN_R,
                                 BTN_OUTLINE, BTN_FILL, BTN_TEXT,
                                 "^MINUS:32:7BEF^", nullptr);
    m_audioVolDownBtn.setFont(ComfortaaBold30);

    m_audioVolUpBtn.initButton(m_tft,
                               AUDIO_FT_VUP_CX, FOOTER_CY,
                               AUDIO_FT_BTN_SIZE, AUDIO_FT_BTN_SIZE, AUDIO_FT_BTN_R,
                               BTN_OUTLINE, BTN_FILL, BTN_TEXT,
                               "^PLUS:32:7BEF^", nullptr);
    m_audioVolUpBtn.setFont(ComfortaaBold30);

    m_audioVolDots.init(AUDIO_FT_DOT_W, AUDIO_FT_DOT_H, AUDIO_FT_DOT_SPACE, AUDIO_FT_CHAR_SPACE);
    m_audioVolDots.setOnColor(0x7BEF, TFT_BLACK);
    m_audioVolDots.setOffColor(0x2104, TFT_BLACK);
    m_audioVolDots.setBackgroundColor(TFT_BLACK);
    m_audioVolDots.setShowOffDots(true);
    m_audioVolDots.setGradientEnabled(false);
    m_audioVolDots.setDotShape(DotMatrix::DotShape::SQUARE);
    m_audioVolDots.reserveText(4);
}

// ===================================================================
// Zeichensteuerung
// ===================================================================

/**
 * @brief Markiert alle Bereiche zum Neuzeichnen.
 */
void Settings::invalidateAll()
{
    m_menuDirty = true;
    m_workDirty = true;
    m_footerDirty = true;
    m_numpadLayoutValid = false;
}

/**
 * @brief Vollredraw nach Sprachwechsel oder Dialog.
 */
void Settings::redrawAll()
{
    if (m_tft)
        m_tft->clear(C_BG);
    invalidateAll();
}

/**
 * @brief Setzt den UI-Zustand nach einem modalen Dialog zurück.
 */
void Settings::afterModalRedraw()
{
    I18n::consumeLanguageChanged();
    redrawAll();
    m_lastDrawnSecond = -1;
    m_requireTouchRelease = true;
    m_touchBlockUntil = millis() + 150;
    m_touchProcessed = false;
}

/**
 * @brief Zyklische Update- und Zeichenfunktion.
 */
void Settings::draw()
{
    if (!m_initialized)
        return;

    if (m_state == State::IDLE && I18n::consumeLanguageChanged())
        redrawAll();

    switch (m_state)
    {
    case State::TESTING:
        updateTestingState();
        break;
    case State::SHOWING_MESSAGE:
        updateMessageState();
        break;
    case State::SAVING:
        updateSavingState();
        break;
    case State::IDLE:
    default:
        break;
    }

    if (handleDeferredActions())
        return; // Modul kann nach Exit/Restart bereits abgebaut sein

    if (m_state == State::IDLE)
    {
        // Wenn der WLAN-Zustand sich ändert, das Wetter-Panel auffrischen (steuert die Sichtbarkeit der dezenten Hinweiszeile)
        const bool wifiNow = APP_STATE.getWifiConnected();
        if (wifiNow != m_lastObservedWifi)
        {
            m_lastObservedWifi = wifiNow;
            if (m_panel == Panel::WEATHER)
                m_workDirty = true;
        }

        // Info-Panel zeigt laufende Werte (Uptime, Heap, Signal, Sync). Alle zwei Sekunden neu zeichnen lassen, solange das Panel sichtbar ist.
        if (m_panel == Panel::INFO)
        {
            const unsigned long now = millis();
            if (now - m_lastInfoRefreshMs >= 2000UL)
            {
                m_lastInfoRefreshMs = now;
                m_workDirty = true;
            }
        }

        if (m_menuDirty)
        {
            drawMenu();
            drawScrollbar();
        }
        if (m_workDirty)
            drawWorkPanel();
        if (m_footerDirty)
            drawFooter();

        // Die Anzeigeseite zeigt den Senderpegel laufend mit; das geht nur nach dem Panel,
        // weil ein Neuzeichnen des Panels die Zahl sonst wieder überdecken würde.
        drawAppearanceLevel();

        handleTouch();
    }

    drawClock();
}

// ===================================================================
// Menü zeichnen
// ===================================================================

/**
 * @brief Liefert die Akzentfarbe einer Sektion.
 */
uint16_t Settings::sectionColor(Section s) const
{
    switch (s)
    {
    case Section::CONNECTION:
        return C_SEC_CONNECTION;
    case Section::SERVICES:
        return C_SEC_SERVICES;
    case Section::SYSTEM:
    default:
        return C_SEC_SYSTEM;
    }
}

/**
 * @brief Y-Position eines Menü-Eintrags (sprite-relativ).
 */
int Settings::menuEntryY(int index) const
{
    int y = 0;
    for (int i = 0; i < index && i < s_menuCount; ++i)
        y += (s_menu[i].type == EntryType::SECTION_HEADER) ? SECTION_ROW_H : ITEM_ROW_H;
    return y;
}

/**
 * @brief Liefert das Panel an einer Menü-Y-Position oder -1.
 */
int Settings::menuEntryAtY(int y) const
{
    const int contentY = y + m_scrollOffset;
    for (int i = 0; i < s_menuCount; ++i)
    {
        const int rowY = menuEntryY(i);
        const int rowH = (s_menu[i].type == EntryType::SECTION_HEADER) ? SECTION_ROW_H : ITEM_ROW_H;
        if (contentY >= rowY && contentY < rowY + rowH)
        {
            if (s_menu[i].type == EntryType::PANEL_ITEM)
                return static_cast<int>(s_menu[i].panel);
            return -1;
        }
    }
    return -1;
}

/**
 * @brief Gesamthöhe der Menüliste in Pixeln.
 */
int Settings::getTotalListHeight() const
{
    int h = 0;
    for (int i = 0; i < s_menuCount; ++i)
        h += (s_menu[i].type == EntryType::SECTION_HEADER) ? SECTION_ROW_H : ITEM_ROW_H;
    return h;
}

/**
 * @brief Zeichnet die gesamte Menüliste und pusht sie.
 */
void Settings::drawMenu()
{
    if (!m_menuSprite)
        return;

    m_menuSprite->clear(C_BG);
    m_menuSprite->loadFont(ComfortaaBold16);

    for (int i = 0; i < s_menuCount; ++i)
    {
        const MenuEntry &e = s_menu[i];
        const int rowH = (e.type == EntryType::SECTION_HEADER) ? SECTION_ROW_H : ITEM_ROW_H;
        const int y = menuEntryY(i) - m_scrollOffset;

        if (y + rowH <= 0 || y >= MENU_SPRITE_H)
            continue;

        const uint16_t catColor = sectionColor(e.section);

        if (e.type == EntryType::SECTION_HEADER)
        {
            const uint16_t tintColor = scaleColor(catColor, 0.45f);
            m_menuSprite->fillRect(0, y, MENU_W, rowH, tintColor);
            m_menuSprite->fillRect(0, y, ITEM_ACCENT_W, rowH, catColor);

            m_menuSprite->setTextDatum(ML_DATUM);
            m_menuSprite->setTextColor(C_SEC_HDR_TXT, tintColor);
            m_menuSprite->drawString(I18n::str(e.label), ITEM_TEXT_X, y + rowH / 2);
        }
        else
        {
            const bool selected = (e.panel == m_panel);
            const uint16_t selBg = scaleColor(catColor, 0.25f);
            if (selected)
                m_menuSprite->fillRect(0, y, MENU_W, rowH, selBg);

            const uint16_t accentCol = selected ? catColor : scaleColor(catColor, 0.45f);
            m_menuSprite->fillRect(0, y, ITEM_ACCENT_W, rowH, accentCol);

            m_menuSprite->setTextDatum(ML_DATUM);
            m_menuSprite->setTextColor(
                selected ? C_TEXT_BRIGHT : C_TEXT,
                selected ? selBg : C_BG);
            m_menuSprite->drawString(I18n::str(e.label), ITEM_TEXT_X, y + rowH / 2);

            m_menuSprite->drawFastHLine(ITEM_ACCENT_W, y + rowH - 1, MENU_W - ITEM_ACCENT_W, C_SEP_LINE);
        }
    }

    m_menuSprite->unloadFont();
    m_menuSprite->pushSprite(MENU_X, 0);
    m_menuDirty = false;
}

/**
 * @brief Zeichnet die Scrollbar und pusht sie.
 *
 * Passt die Inhaltshöhe in die Menühöhe, wird nur der Track gezeichnet. Sobald spätere Panels mehr Einträge bringen, erscheint zusätzlich der Thumb.
 */
void Settings::drawScrollbar()
{
    if (!m_scrollbarSprite)
        return;

    m_scrollbarSprite->clear(C_SCROLLBAR_TRACK);

    const int totalH = getTotalListHeight();
    const int maxScroll = (totalH > MENU_SPRITE_H) ? (totalH - MENU_SPRITE_H) : 0;
    if (maxScroll <= 0)
    {
        m_scrollbarSprite->pushSprite(SCROLLBAR_X, 0);
        return;
    }

    const float viewRatio = static_cast<float>(MENU_SPRITE_H) / static_cast<float>(totalH);
    int thumbH = static_cast<int>(MENU_SPRITE_H * viewRatio);
    if (thumbH < 20)
        thumbH = 20;

    int clamped = m_scrollOffset;
    if (clamped < 0)
        clamped = 0;
    if (clamped > maxScroll)
        clamped = maxScroll;

    const float scrollRatio = static_cast<float>(clamped) / static_cast<float>(maxScroll);
    const int thumbY = static_cast<int>(scrollRatio * (MENU_SPRITE_H - thumbH));

    m_scrollbarSprite->fillRoundRect(0, thumbY, SCROLLBAR_W, thumbH, 2, C_SCROLLBAR_NORMAL);
    m_scrollbarSprite->pushSprite(SCROLLBAR_X, 0);
}

// ===================================================================
// Arbeitsbereich zeichnen
// ===================================================================

/**
 * @brief Liefert den Titel-Schlüssel eines Panels.
 */
SettingsKey Settings::panelTitleKey(Panel p) const
{
    switch (p)
    {
    case Panel::WIFI:
        return SettingsKey::SETTINGS_WIFI_TITLE;
    case Panel::LANGUAGE:
        return SettingsKey::SETTINGS_LANG_TITLE;
    case Panel::SECURITY:
        return SettingsKey::SETTINGS_SEC_TITLE;
    case Panel::INFO:
        return SettingsKey::SETTINGS_INFO_TITLE;
    case Panel::TIME:
        return SettingsKey::SETTINGS_TIME_TITLE;
    case Panel::TIME_WINDOW:
        return SettingsKey::SETTINGS_TIME_WINDOW_TITLE;
    case Panel::WEATHER:
        return SettingsKey::SETTINGS_WEATHER_TITLE;
    case Panel::NEWS:
        return SettingsKey::SETTINGS_NEWS_TITLE;
    case Panel::CLOUD:
        return SettingsKey::SETTINGS_CLOUD_TITLE;
    case Panel::AUDIO:
        return SettingsKey::SETTINGS_AUDIO_TITLE;
    case Panel::APPEARANCE:
        return SettingsKey::SETTINGS_APPEARANCE_TITLE;
    default:
        return SettingsKey::SETTINGS_EMPTY;
    }
}

/**
 * @brief Liefert den Untertitel-Schlüssel eines Panels.
 */
SettingsKey Settings::panelSubtitleKey(Panel p) const
{
    switch (p)
    {
    case Panel::WIFI:
        return SettingsKey::SETTINGS_WIFI_SUBTITLE;
    case Panel::LANGUAGE:
        return SettingsKey::SETTINGS_LANG_SUBTITLE;
    case Panel::SECURITY:
        return SettingsKey::SETTINGS_SEC_SUBTITLE;
    case Panel::INFO:
        return SettingsKey::SETTINGS_INFO_SUBTITLE;
    case Panel::NEWS:
        return SettingsKey::SETTINGS_NEWS_SUBTITLE;
    case Panel::WEATHER:
        return SettingsKey::SETTINGS_WEATHER_SUBTITLE;
    case Panel::CLOUD:
        return SettingsKey::SETTINGS_CLOUD_SUBTITLE;
    case Panel::TIME:
        return SettingsKey::SETTINGS_TIME_SUBTITLE;
    case Panel::TIME_WINDOW:
        return SettingsKey::SETTINGS_TIME_WINDOW_SUBTITLE;
    case Panel::AUDIO:
        return SettingsKey::SETTINGS_AUDIO_SUBTITLE;
    case Panel::APPEARANCE:
        return SettingsKey::SETTINGS_APPEARANCE_SUBTITLE;
    default:
        return SettingsKey::SETTINGS_PLACEHOLDER_TITLE;
    }
}

/**
 * @brief Zeichnet die Titelbox und das gewählte Panel, dann Push.
 */
void Settings::drawWorkPanel()
{
    if (!m_workSprite)
        return;

    m_workSprite->clear(C_BG);
    m_workSprite->fillSmoothRoundRect(0, 0, WORK_W, WORK_H, WORK_R, C_WORK_BG);
    m_workSprite->fillSmoothRoundRect(TITLE_BOX_X, TITLE_BOX_Y, TITLE_BOX_W, TITLE_BOX_H, TITLE_BOX_R, C_TITLE_BG);

    m_workSprite->setTextDatum(TL_DATUM);
    m_workSprite->loadFont(ComfortaaBold30);
    m_workSprite->setTextColor(Theme::ColorGreen);
    m_workSprite->drawString(I18n::str(panelTitleKey(m_panel)), TITLE_TEXT_X, TITLE_TEXT_Y);
    m_workSprite->unloadFont();

    m_workSprite->loadFont(ComfortaaBold16);
    m_workSprite->setTextColor(C_SUB);
    m_workSprite->drawString(I18n::str(panelSubtitleKey(m_panel)), TITLE_TEXT_X, SUB_TEXT_Y);
    m_workSprite->unloadFont();

    switch (m_panel)
    {
    case Panel::WIFI:
        drawPanelWifi(m_workSprite);
        break;
    case Panel::LANGUAGE:
        drawPanelLanguage(m_workSprite);
        break;
    case Panel::SECURITY:
        drawPanelSecurity(m_workSprite);
        break;
    case Panel::INFO:
        drawPanelInfo(m_workSprite);
        break;
    case Panel::NEWS:
        drawPanelNews(m_workSprite);
        break;
    case Panel::WEATHER:
        drawPanelWeather(m_workSprite);
        break;
    case Panel::CLOUD:
        drawPanelCloud(m_workSprite);
        break;
    case Panel::TIME:
        drawPanelTime(m_workSprite);
        break;
    case Panel::TIME_WINDOW:
        drawPanelTimeWindow(m_workSprite);
        break;
    case Panel::AUDIO:
        drawPanelAudio(m_workSprite);
        break;
    case Panel::APPEARANCE:
        drawPanelAppearance(m_workSprite);
        break;
    default:
        drawPanelPlaceholder(m_workSprite);
        break;
    }

    m_workSprite->pushSprite(WORK_X, WORK_Y);
    m_workDirty = false;
}

/**
 * @brief Zeichnet ein Platzhalter-Panel (Dienste in Vorbereitung).
 */
void Settings::drawPanelPlaceholder(Gfx_Sprite *spr)
{
    spr->loadFont(ComfortaaBold16);
    spr->setTextColor(C_SUB);
    spr->setTextDatum(MC_DATUM);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_PLACEHOLDER_MESSAGE),
                    CONTENT_X + CONTENT_W / 2, CONTENT_Y + CONTENT_H / 2);
    spr->unloadFont();
}

// ===================================================================
// WLAN-Panel
// ===================================================================

/**
 * @brief Geometrie einer Profilzeile (sprite-relativ).
 */
Settings::Rect Settings::wifiRowRect(int idx) const
{
    const int y = CONTENT_Y + idx * (WIFI_ROW_H + WIFI_ROW_GAP);
    return {CONTENT_X, y, CONTENT_W, WIFI_ROW_H};
}

/**
 * @brief Geometrie des Umschalters einer Profilzeile (sprite-relativ).
 */
Settings::Rect Settings::wifiToggleRect(int idx) const
{
    const Rect r = wifiRowRect(idx);
    const int x = r.x + r.w - TOGGLE_W - TOGGLE_MARGIN;
    const int y = r.y + (r.h - TOGGLE_H) / 2;
    return {x, y, TOGGLE_W, TOGGLE_H};
}

/**
 * @brief Geometrie eines Aktionsknopfes (sprite-relativ).
 */
Settings::Rect Settings::wifiActionRect(int slot) const
{
    const int y = CONTENT_Y + MAX_PROFILES * (WIFI_ROW_H + WIFI_ROW_GAP) + 6;
    const int w = (CONTENT_W - 3 * WIFI_ACT_GAP) / 4;
    const int x = CONTENT_X + slot * (w + WIFI_ACT_GAP);
    return {x, y, w, WIFI_ACT_H};
}

/**
 * @brief Geometrie des Netzwerknamen-Feldes am unteren Rand des Arbeitsbereichs (sprite-relativ).
 */
Settings::Rect Settings::hostnameFieldRect() const
{
    const int y = WORK_H - HOSTNAME_BOTTOM_GAP - HOSTNAME_FIELD_H;
    return {CONTENT_X, y, CONTENT_W, HOSTNAME_FIELD_H};
}

/**
 * @brief Liefert den Status-Schlüssel eines Profils.
 */
SettingsKey Settings::wifiStatusKey(int profileIndex) const
{
    if (!isProfileValid(profileIndex))
        return SettingsKey::SETTINGS_WIFI_STATUS_EMPTY;
    if (m_profiles[profileIndex].tested_ok)
        return SettingsKey::SETTINGS_WIFI_STATUS_CONNECTED;
    if (m_profiles[profileIndex].enabled)
        return SettingsKey::SETTINGS_WIFI_STATUS_ACTIVE;
    return SettingsKey::SETTINGS_WIFI_STATUS_INACTIVE;
}

/**
 * @brief Liefert die Statusfarbe eines Profils.
 */
uint16_t Settings::wifiStatusColor(int profileIndex) const
{
    if (!isProfileValid(profileIndex))
        return C_ST_EMPTY;
    if (m_profiles[profileIndex].tested_ok)
        return C_SEC_SYSTEM;
    if (m_profiles[profileIndex].enabled)
        return RetroColors::Olive;
    return C_ST_INACTIVE;
}

/**
 * @brief Zeichnet eine WLAN-Profilzeile inklusive Umschalter.
 */
void Settings::drawWifiRow(Gfx_Sprite *spr, int idx, const Rect &r, bool selected)
{
    const uint16_t rowBg = selected ? C_ROW_SEL_BG : C_ROW_BG;
    spr->fillSmoothRoundRect(r.x, r.y, r.w, r.h, 8, rowBg);
    if (selected)
        spr->fillRect(r.x, r.y + 6, ITEM_ACCENT_W, r.h - 12, C_SEC_CONNECTION);

    // Statuspunkt
    const uint16_t stColor = wifiStatusColor(idx);
    spr->fillSmoothCircle(r.x + 22, r.y + r.h / 2, 6, stColor);

    // Profilname
    char nameBuf[40];
    if (isProfileValid(idx))
    {
        stringutils::safeCopy(nameBuf, m_profiles[idx].ssid);
    }
    else
    {
        STRF(nameBuf, SettingsKey::SETTINGS_WIFI_PROFILE_FMT, idx + 1);
    }

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(isProfileValid(idx) ? C_TEXT_BRIGHT : C_SUB);
    spr->drawString(nameBuf, r.x + 40, r.y + 16);
    spr->unloadFont();

    // Statustext
    spr->loadFont(ComfortaaBold14);
    spr->setTextColor(stColor);
    spr->drawString(I18n::str(wifiStatusKey(idx)), r.x + 40, r.y + r.h - 14);
    spr->unloadFont();

    drawToggle(spr, wifiToggleRect(idx), m_profiles[idx].enabled, rowBg);
}

/**
 * @brief Zeichnet einen Umschalter mit Statusfarbe in den angegebenen Sprite.
 */
void Settings::drawToggle(Gfx_Sprite *spr, const Rect &t, bool on, uint16_t background)
{
    const uint16_t housing = on ? uint16_t(Theme::ColorButtonBorder) : uint16_t(TFT_DARKGREY);
    const uint16_t knobCol = on ? uint16_t(RetroColors::Olive) : uint16_t(Theme::ColorRetroRed);

    spr->fillSmoothRoundRect(t.x, t.y, t.w, t.h, t.h / 2, housing);
    spr->fillSmoothRoundRect(t.x + TOGGLE_INSET, t.y + TOGGLE_INSET,
                             t.w - 2 * TOGGLE_INSET, t.h - 2 * TOGGLE_INSET,
                             (t.h - 2 * TOGGLE_INSET) / 2, background);
    const int knobCx = on ? (t.x + t.w - TOGGLE_INSET - TOGGLE_KNOB_R - 1)
                          : (t.x + TOGGLE_INSET + TOGGLE_KNOB_R + 1);
    spr->fillSmoothCircle(knobCx, t.y + t.h / 2, TOGGLE_KNOB_R, knobCol);
}

/**
 * @brief Zeichnet das gesamte WLAN-Panel.
 */
void Settings::drawPanelWifi(Gfx_Sprite *spr)
{
    for (int i = 0; i < MAX_PROFILES; ++i)
        drawWifiRow(spr, i, wifiRowRect(i), i == m_selWifi);

    // Aktionsleiste für das gewählte Profil
    const bool configured = isProfileValid(m_selWifi);
    Button *abtn[4] = {&m_btnScan, &m_btnPassword, &m_btnTest, &m_btnDelete};
    const SettingsKey labels[4] = {
        SettingsKey::SETTINGS_WIFI_ACT_SCAN,
        SettingsKey::SETTINGS_WIFI_ACT_PASSWORD,
        SettingsKey::SETTINGS_WIFI_ACT_TEST,
        SettingsKey::SETTINGS_WIFI_ACT_DELETE};
    // Scannen ist immer möglich; Passwort/Test/Löschen brauchen eine SSID
    const bool enabled[4] = {true, configured, configured, configured};

    for (int k = 0; k < 4; ++k)
    {
        abtn[k]->setLabelText(reinterpret_cast<const char *>(I18n::str(labels[k])));
        if (enabled[k])
            abtn[k]->enable();
        else
            abtn[k]->disable();
        abtn[k]->drawButton();
    }

    // Geräte-Netzwerkname als tippbares Feld
    const Rect hf = hostnameFieldRect();
    spr->fillSmoothRoundRect(hf.x, hf.y, hf.w, hf.h, 8, BTN_OUTLINE);
    spr->fillSmoothRoundRect(hf.x + 1, hf.y + 1, hf.w - 2, hf.h - 2, 7, C_ROW_BG);

    if (hostnamePending())
        spr->fillRect(hf.x + 1, hf.y + 8, ITEM_ACCENT_W, hf.h - 16, C_SEC_CONNECTION);

    const int editX = hf.x + hf.w - HOSTNAME_PAD_X - HOSTNAME_EDIT_BTN_W;
    const int editY = hf.y + (hf.h - HOSTNAME_EDIT_BTN_H) / 2;

    spr->loadFont(ComfortaaBold16);
    const int lineH = spr->fontHeight();
    const int blockH = lineH * 2 + HOSTNAME_LINE_GAP;
    const int labelY = hf.y + (hf.h - blockH) / 2;
    const int valueY = labelY + lineH + HOSTNAME_LINE_GAP;

    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(C_SUB);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_WIFI_HOSTNAME_LABEL),
                    hf.x + HOSTNAME_PAD_X, labelY);
    spr->setTextColor(C_TEXT_BRIGHT);
    spr->drawString(m_hostname, hf.x + HOSTNAME_PAD_X, valueY);
    spr->unloadFont();

    // ändern-Knopf rechts im Feld als Aufforderung zum Bearbeiten
    spr->fillSmoothRoundRect(editX, editY, HOSTNAME_EDIT_BTN_W, HOSTNAME_EDIT_BTN_H, 6, BTN_OUTLINE);
    spr->fillSmoothRoundRect(editX + 1, editY + 1, HOSTNAME_EDIT_BTN_W - 2, HOSTNAME_EDIT_BTN_H - 2, 5, BTN_FILL);

    spr->loadFont(LeagueGothicCond34);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(BTN_TEXT);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_WIFI_HOSTNAME_EDIT),
                    editX + HOSTNAME_EDIT_BTN_W / 2, editY + HOSTNAME_EDIT_BTN_H / 2);
    spr->unloadFont();
}

// ===================================================================
// Sprache-Panel
// ===================================================================

/**
 * @brief Geometrie einer Sprachzeile (sprite-relativ).
 */
Settings::Rect Settings::langRowRect(int idx) const
{
    const int y = CONTENT_Y + idx * (LANG_ROW_H + LANG_ROW_GAP);
    return {CONTENT_X, y, CONTENT_W, LANG_ROW_H};
}

/**
 * @brief Zeichnet das Sprach-Panel.
 */
void Settings::drawPanelLanguage(Gfx_Sprite *spr)
{
    const int selectedIdx = (m_pendingLangIndex >= 0) ? m_pendingLangIndex : currentLangIndex();
    const int activeSysIdx = currentLangIndex();
    const SettingsKey names[2] = {SettingsKey::SETTINGS_LANG_NAME_DE, SettingsKey::SETTINGS_LANG_NAME_EN};

    for (int i = 0; i < 2; ++i)
    {
        const Rect r = langRowRect(i);
        const bool selected = (i == selectedIdx);
        spr->fillSmoothRoundRect(r.x, r.y, r.w, r.h, 10, selected ? C_ROW_SEL_BG : C_ROW_BG);
        if (selected)
            spr->fillRect(r.x, r.y + 8, ITEM_ACCENT_W, r.h - 16, C_SEC_SYSTEM);

        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(selected ? C_TEXT_BRIGHT : C_TEXT);
        spr->drawString(I18n::str(names[i]), r.x + 24, r.y + r.h / 2);
        spr->unloadFont();

        if (i == activeSysIdx)
        {
            spr->loadFont(ComfortaaBold14);
            spr->setTextDatum(MR_DATUM);
            spr->setTextColor(C_SEC_SYSTEM);
            spr->drawString(I18n::str(SettingsKey::SETTINGS_LANG_ACTIVE), r.x + r.w - 20, r.y + r.h / 2);
            spr->unloadFont();
        }
    }
}

// ===================================================================
// Sicherheit-Panel
// ===================================================================

/**
 * @brief Zeichnet das Sicherheit-Panel mit PIN-Anzeige und Nummernblock.
 */
void Settings::drawPanelSecurity(Gfx_Sprite *spr)
{
    if (!m_numpadLayoutValid)
    {
        m_numpad.computeLayout(CONTENT_W, CONTENT_H - WorkshopNumpad::REQUIRED_H);
        m_numpadLayoutValid = true;
    }

    // Hinweistext je nach Eingabephase
    SettingsKey hintKey = SettingsKey::SETTINGS_SEC_PIN_HINT;
    uint16_t hintColor = C_SUB;
    if (m_pinConfirming)
    {
        hintKey = SettingsKey::SETTINGS_SEC_PIN_REPEAT;
    }
    else if (m_pinMismatch)
    {
        hintKey = SettingsKey::SETTINGS_SEC_PIN_MISMATCH;
        hintColor = uint16_t(Theme::ColorRetroRed);
    }
    else if (pinPending() && m_pinLen == 0)
    {
        hintKey = SettingsKey::SETTINGS_SEC_PIN_READY;
        hintColor = C_SEC_SYSTEM;
    }

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(hintColor);
    spr->drawString(I18n::str(hintKey), CONTENT_X + CONTENT_W / 2, CONTENT_Y + 18);
    spr->unloadFont();

    // PIN-Punkte
    const int dotsY = CONTENT_Y + 64;
    const int totalW = 3 * PIN_DOT_GAP;
    const int startX = CONTENT_X + (CONTENT_W - totalW) / 2;
    for (int i = 0; i < 4; ++i)
    {
        const int cx = startX + i * PIN_DOT_GAP;
        if (i < m_pinLen)
            spr->fillSmoothCircle(cx, dotsY, PIN_DOT_R, C_TEXT);
        else
            spr->fillSmoothCircle(cx, dotsY, PIN_DOT_R, C_DOT_EMPTY);
    }

    // Nummernblock am unteren Rand
    m_numpad.draw(spr, CONTENT_X, CONTENT_Y, -1);
}

// ===================================================================
// Info-Panel
// ===================================================================

/**
 * @brief Geometrie eines der beiden Info-Knöpfe im Footer.
 */
Settings::Rect Settings::footerInfoButtonRect(int slot) const
{
    int buttonW = footerButtonWidthForLabels(SettingsKey::SETTINGS_INFO_RESTART_BTN, SettingsKey::SETTINGS_INFO_RESET_BTN);
    const int availableW = CLOCK_PUSH_X - FOOTER_ACTION_CLOCK_GAP - FOOTER_ACTION_BTN_X0;
    const int maxButtonW = (availableW - FOOTER_ACTION_BTN_GAP) / 2;
    if (buttonW > maxButtonW)
        buttonW = maxButtonW;

    const int x = FOOTER_ACTION_BTN_X0 + slot * (buttonW + FOOTER_ACTION_BTN_GAP);
    const int y = FOOTER_Y + (FOOTER_H - BTN_H) / 2;
    return {x, y, buttonW, BTN_H};
}

/**
 * @brief Geometrie des Zurücksetzen-Knopfs im Zeitfenster-Footer.
 */
Settings::Rect Settings::footerTimeWindowResetButtonRect() const
{
    const int x = WORK_X + SAVE_BTN_W + FOOTER_ACTION_BTN_GAP;
    const int y = FOOTER_Y + (FOOTER_H - BTN_H) / 2;
    int buttonW = footerButtonWidthForLabel(SettingsKey::SETTINGS_INFO_RESET_BTN);
    const int maxButtonW = CLOCK_PUSH_X - FOOTER_ACTION_CLOCK_GAP - x;
    if (buttonW > maxButtonW)
        buttonW = maxButtonW;

    return {x, y, buttonW, BTN_H};
}

/**
 * @brief Geometrie des Cloud-Löschknopfs im Footer.
 */
Settings::Rect Settings::footerCloudClearButtonRect() const
{
    const int x = WORK_X + SAVE_BTN_W + FOOTER_ACTION_BTN_GAP;
    const int y = FOOTER_Y + (FOOTER_H - BTN_H) / 2;
    int buttonW = footerButtonWidthForLabel(SettingsKey::SETTINGS_CLOUD_CLEAR_BTN);
    const int maxButtonW = CLOCK_PUSH_X - FOOTER_ACTION_CLOCK_GAP - x;
    if (buttonW > maxButtonW)
        buttonW = maxButtonW;

    return {x, y, buttonW, BTN_H};
}

/**
 * @brief Berechnet eine Footer-Buttonbreite aus Labelbreite und Innenabstand.
 */
int Settings::footerButtonWidthForLabel(SettingsKey labelKey) const
{
    if (!m_tft)
        return FOOTER_ACTION_BTN_MIN_W;

    const char *label = reinterpret_cast<const char *>(I18n::str(labelKey));
    m_tft->loadFont(LeagueGothicCond34);
    const int textW = m_tft->textWidth(label ? label : "");
    m_tft->unloadFont();

    const int wantedW = textW + 2 * FOOTER_ACTION_BTN_PAD_X;
    if (wantedW < FOOTER_ACTION_BTN_MIN_W)
        return FOOTER_ACTION_BTN_MIN_W;

    return wantedW;
}

/**
 * @brief Berechnet eine gemeinsame Breite für zwei Footer-Buttons.
 */
int Settings::footerButtonWidthForLabels(SettingsKey firstKey, SettingsKey secondKey) const
{
    const int firstW = footerButtonWidthForLabel(firstKey);
    const int secondW = footerButtonWidthForLabel(secondKey);
    if (firstW > secondW)
        return firstW;

    return secondW;
}

/**
 * @brief Initialisiert einen Textbutton im Footer mit aktueller Geometrie.
 */
void Settings::initFooterTextButton(Button &btn, const Rect &rect, SettingsKey labelKey)
{
    btn.initButton(m_tft,
                   rect.x + rect.w / 2,
                   rect.y + rect.h / 2,
                   rect.w,
                   rect.h,
                   BTN_R,
                   BTN_OUTLINE,
                   BTN_FILL,
                   BTN_TEXT,
                   reinterpret_cast<const char *>(I18n::str(labelKey)),
                   nullptr);
    btn.setFont(LeagueGothicCond34);
}

/**
 * @brief Zeichnet das Info-Panel mit Netzwerk- und System-Werten in zwei Spalten.
 *
 * Linke Spalte zeigt netzwerkbezogene Werte (SSID, Host-Name, IP-Adresse, MAC-Adresse, Kanal, Signal, Zeitserver, Sync), rechte Spalte System-Werte (Version, FW-Belegung, MCU, RAM, UART, Innenklima, Heap, PSRAM, Uptime). Die zwei Aktions-Knöpfe (Neustart, Zurücksetzen) liegen nicht im Panel selbst, sondern in der Fußzeile und werden von `drawFooter()` gezeichnet.
 */
void Settings::drawPanelInfo(Gfx_Sprite *spr)
{
    constexpr int COL_GAP = 14;
    constexpr int COL_W = (CONTENT_W - COL_GAP) / 2;
    constexpr int LEFT_X = CONTENT_X;
    constexpr int RIGHT_X = CONTENT_X + COL_W + COL_GAP;
    constexpr int HDR_H = 26;
    constexpr int HDR_PAD_X = 10;
    constexpr int ROW_H = 28;
    constexpr int ROW_PAD_X = 8;
    constexpr int I2C_LABEL_GAP = 10; ///< Abstand zwischen I2C-Badge und Adressliste
    constexpr int I2C_BOTTOM_GAP = 10; ///< Abstand der I2C-Zeile zum unteren Rand
    constexpr float I2C_BADGE_COLOR_SCALE = 0.35f; ///< Abdunkelung der I2C-Badge-Farbe
    constexpr int SIGNAL_BAR_W = 80;
    constexpr int SIGNAL_BAR_H = 8;

    // Gedämpfte Headerfarben, getrennt von den hellen Akzent- Streifen im Menü, weil eine vollflächige Sättigung dort grell wirkt.
    constexpr uint16_t C_INFO_HDR_NET = 0x60A6; ///< Gedämpftes Burgunder-Pflaume für den Netzwerk-Header
    constexpr uint16_t C_INFO_HDR_SYS = 0x6A82; ///< Warmes Messing für den System-Header

    // Sammeln der Werte einmalig oben, dann einheitlich zeichnen.
    char ssidBuf[40] = {0};
    char hostBuf[40] = {0};
    char ipBuf[24] = {0};
    char gatewayBuf[24] = {0};
    char macBuf[24] = {0};
    char channelBuf[24] = {0};
    char ntpBuf[40] = {0};
    char syncBuf[32] = {0};
    char fwBuf[32] = {0};
    char fwSizeBuf[24] = {0};
    char mcuBuf[40] = {0};
    char hardwareBuf[40] = {0};
    char heapBuf[40] = {0};
    char psramFreeBuf[32] = {0};
    char uptimeBuf[32] = {0};
    char uartBuf[24] = {0};
    char i2cBuf[96] = {0};

    const bool wifiOn = WiFi.status() == WL_CONNECTED;

    if (wifiOn)
    {
        const String ssid = WiFi.SSID();
        stringutils::safeCopy(ssidBuf, ssid.c_str());
        const char *host = WiFi.getHostname();
        if (host && *host)
            stringutils::safeCopy(hostBuf, host);
        else
            stringutils::safeCopy(hostBuf, reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_INFO_NOT_CONNECTED)));
        const IPAddress ip = WiFi.localIP();
        stringutils::safeFormat(ipBuf, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
        const IPAddress gw = WiFi.gatewayIP();
        stringutils::safeFormat(gatewayBuf, "%u.%u.%u.%u", gw[0], gw[1], gw[2], gw[3]);
        STRF(channelBuf, SettingsKey::SETTINGS_INFO_CHANNEL_FMT,
             static_cast<int>(WiFi.channel()));
    }
    else
    {
        const char *notConn = reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_INFO_NOT_CONNECTED));
        stringutils::safeCopy(ssidBuf, notConn);
        stringutils::safeCopy(hostBuf, notConn);
        stringutils::safeCopy(ipBuf, notConn);
        stringutils::safeCopy(gatewayBuf, notConn);
        stringutils::safeCopy(channelBuf, notConn);
    }

    // MAC-Adresse liegt im eFuse und ist auch ohne WLAN-Verbindung lesbar.
    {
        uint8_t mac[6] = {0};
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        stringutils::safeFormat(macBuf, "%02X:%02X:%02X:%02X:%02X:%02X",
                                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    // Zeitserver-Name aus AppState; falls leer, dezenter Platzhalter
    const char *ntpServer = APP_STATE.getNtpServer();
    if (ntpServer && *ntpServer)
        stringutils::safeCopy(ntpBuf, ntpServer);
    else
        stringutils::safeCopy(ntpBuf, reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_INFO_NOT_CONNECTED)));

    // Letzter Zeitabgleich kompakt: TT.MM.JJ HH:MM (Nx)
    {
        const char *lastSync = SYSTEM_STATS.getLastSync();
        const unsigned long syncCount = static_cast<unsigned long>(SYSTEM_STATS.getNtpSyncCount());
        const bool hasWebFormat = lastSync && strlen(lastSync) >= 16 &&
                                  isdigit(static_cast<unsigned char>(lastSync[0])) &&
                                  isdigit(static_cast<unsigned char>(lastSync[1])) &&
                                  lastSync[2] == '.' &&
                                  isdigit(static_cast<unsigned char>(lastSync[3])) &&
                                  isdigit(static_cast<unsigned char>(lastSync[4])) &&
                                  lastSync[5] == '.' &&
                                  isdigit(static_cast<unsigned char>(lastSync[8])) &&
                                  isdigit(static_cast<unsigned char>(lastSync[9])) &&
                                  lastSync[10] == ' ' &&
                                  isdigit(static_cast<unsigned char>(lastSync[11])) &&
                                  isdigit(static_cast<unsigned char>(lastSync[12])) &&
                                  lastSync[13] == ':' &&
                                  isdigit(static_cast<unsigned char>(lastSync[14])) &&
                                  isdigit(static_cast<unsigned char>(lastSync[15]));

        if (hasWebFormat)
        {
            stringutils::safeFormat(syncBuf, "%c%c.%c%c.%c%c %c%c:%c%c (%lux)",
                                    lastSync[0], lastSync[1],
                                    lastSync[3], lastSync[4],
                                    lastSync[8], lastSync[9],
                                    lastSync[11], lastSync[12],
                                    lastSync[14], lastSync[15],
                                    syncCount);
        }
        else
        {
            stringutils::safeFormat(syncBuf, "%s (%lux)",
                                    reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_INFO_NEVER)),
                                    syncCount);
        }
    }

    // Firmware-Kennung kommt aus dem Build-Flag FW_VERSION.
    stringutils::safeCopy(fwBuf, FW_VERSION);

    // Firmware-Belegung: Sketch-Größe in Byte und Auslastung der laufenden Partition.
    {
        const size_t sketchSize = ESP.getSketchSize();
        const esp_partition_t *partition = esp_ota_get_running_partition();
        const size_t partitionSize = partition ? partition->size : 0;
        if (partitionSize > 0)
        {
            const int usedPercent = static_cast<int>((sketchSize * 100UL) / partitionSize);
            stringutils::safeFormat(fwSizeBuf, "%lu Byte (%d%%)",
                                    static_cast<unsigned long>(sketchSize), usedPercent);
        }
        else
        {
            stringutils::safeFormat(fwSizeBuf, "%lu Byte",
                                    static_cast<unsigned long>(sketchSize));
        }
    }

    stringutils::safeFormat(mcuBuf, "%s @ %u MHz",
                            ESP.getChipModel(),
                            static_cast<unsigned>(ESP.getCpuFreqMHz()));

    STRF(hardwareBuf, SettingsKey::SETTINGS_INFO_HARDWARE_FMT,
         static_cast<int>(ESP.getFlashChipSize() / (1024UL * 1024UL)),
         static_cast<int>(ESP.getPsramSize() / (1024UL * 1024UL)));

    // Heap-Fragmentierung: (1 - groesster_freier_Block / freie_gesamt) * 100
    {
        const size_t freeTotal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        const size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        int frag = 0;
        if (freeTotal > 0 && largest <= freeTotal)
            frag = static_cast<int>(100UL - (largest * 100UL) / freeTotal);
        if (frag < 0)
            frag = 0;
        if (frag > 100)
            frag = 100;

        STRF(heapBuf, SettingsKey::SETTINGS_INFO_HEAP_FMT,
             static_cast<int>(ESP.getFreeHeap() / 1024UL), frag);
    }

    {
        const uint32_t psramFree = ESP.getFreePsram();
        const uint32_t psramTotal = ESP.getPsramSize();
        int psramPercent = 0;
        if (psramTotal > 0)
            psramPercent = static_cast<int>((psramFree * 100UL) / psramTotal);
        STRF(psramFreeBuf, SettingsKey::SETTINGS_INFO_PSRAM_FMT,
             static_cast<int>(psramFree / 1024UL), psramPercent);
    }

    const uint32_t secs = millis() / 1000UL;
    STRF(uptimeBuf, SettingsKey::SETTINGS_INFO_UPTIME_FMT,
         static_cast<int>(secs / 86400UL),
         static_cast<int>((secs % 86400UL) / 3600UL),
         static_cast<int>((secs % 3600UL) / 60UL),
         static_cast<int>(SYSTEM_STATS.getWifiReconnects()));

    stringutils::safeFormat(uartBuf, "%u bit/s", static_cast<unsigned>(Serial.baudRate()));

    if (APP_STATE.wasI2cScanBusBlocked())
        STRF(i2cBuf, SettingsKey::SETTINGS_INFO_I2C_BUS_BLOCKED);
    else if (!APP_STATE.wasI2cScanCompleted())
        STRF(i2cBuf, SettingsKey::SETTINGS_INFO_I2C_NOT_CHECKED);
    else if (APP_STATE.getI2cScanResult()[0] == '\0')
        STRF(i2cBuf, SettingsKey::SETTINGS_INFO_I2C_NONE);
    else
        stringutils::safeCopy(i2cBuf, APP_STATE.getI2cScanResult());

    // Datenstrukturen für Standardzeilen, die Signalzeile wird separat gezeichnet
    struct InfoRow
    {
        SettingsKey labelKey;
        const char *value;
    };

    // Linke Spalte: SSID, Host, IP, Gateway, MAC, [Signal], NTP, Sync
    const InfoRow leftRowsTop[] = {
        {SettingsKey::SETTINGS_INFO_SSID, ssidBuf},
        {SettingsKey::SETTINGS_INFO_HOSTNAME, hostBuf},
        {SettingsKey::SETTINGS_INFO_IP, ipBuf},
        {SettingsKey::SETTINGS_INFO_GATEWAY, gatewayBuf},
        {SettingsKey::SETTINGS_INFO_MAC, macBuf},
        {SettingsKey::SETTINGS_INFO_CHANNEL, channelBuf},
    };
    const InfoRow leftRowsBot[] = {
        {SettingsKey::SETTINGS_INFO_NTP_SERVER, ntpBuf},
        {SettingsKey::SETTINGS_INFO_SYNC_STATUS, syncBuf},
    };

    // Systemdaten in der rechten Spalte
    const InfoRow rightRows[] = {
        {SettingsKey::SETTINGS_INFO_FIRMWARE, fwBuf},
        {SettingsKey::SETTINGS_INFO_FIRMWARE_SIZE, fwSizeBuf},
        {SettingsKey::SETTINGS_INFO_MCU, mcuBuf},
        {SettingsKey::SETTINGS_INFO_HARDWARE, hardwareBuf},
        {SettingsKey::SETTINGS_INFO_UART, uartBuf},
        {SettingsKey::SETTINGS_INFO_HEAP_FREE, heapBuf},
        {SettingsKey::SETTINGS_INFO_PSRAM_FREE, psramFreeBuf},
        {SettingsKey::SETTINGS_INFO_UPTIME, uptimeBuf},
    };

    // Hilfs-Lambda zum Zeichnen einer Standard-Zeile (Label links, Wert rechts).
    auto drawInfoRow = [&](int colX, int colW, int yTop, const InfoRow &row)
    {
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_SUB);
        spr->drawString(I18n::str(row.labelKey), colX + ROW_PAD_X, yTop + ROW_H / 2);
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_TEXT_BRIGHT);
        spr->drawString(row.value, colX + colW - ROW_PAD_X, yTop + ROW_H / 2);
        spr->drawFastHLine(colX + ROW_PAD_X, yTop + ROW_H - 1,
                           colW - 2 * ROW_PAD_X, C_SEP_LINE);
    };

    spr->loadFont(ComfortaaBold16);

    // Gedämpfte Sektionsüberschriften in warmem und kühlem Farbton
    spr->fillSmoothRoundRect(LEFT_X, CONTENT_Y, COL_W, HDR_H, 6, C_INFO_HDR_NET);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_TEXT_BRIGHT);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_INFO_SECTION_NETWORK),
                    LEFT_X + HDR_PAD_X, CONTENT_Y + HDR_H / 2);

    spr->fillSmoothRoundRect(RIGHT_X, CONTENT_Y, COL_W, HDR_H, 6, C_INFO_HDR_SYS);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_TEXT_BRIGHT);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_INFO_SECTION_SYSTEM),
                    RIGHT_X + HDR_PAD_X, CONTENT_Y + HDR_H / 2);

    // Linke Spalte: obere Standard-Zeilen
    int leftY = CONTENT_Y + HDR_H + 4;
    for (const InfoRow &row : leftRowsTop)
    {
        drawInfoRow(LEFT_X, COL_W, leftY, row);
        leftY += ROW_H;
    }

    // Signal-Zeile mit Balken
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_SUB);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_INFO_SIGNAL),
                    LEFT_X + ROW_PAD_X, leftY + ROW_H / 2);

    if (wifiOn)
    {
        const int32_t rssi = WiFi.RSSI();
        int percent = 0;
        if (rssi >= -50)
            percent = 100;
        else if (rssi <= -90)
            percent = 0;
        else
            percent = (rssi + 90) * 100 / 40;
        if (percent < 0)
            percent = 0;
        if (percent > 100)
            percent = 100;

        uint16_t barColor;
        if (percent >= 60)
            barColor = C_SEC_SYSTEM;
        else if (percent >= 30)
            barColor = 0xFE60;
        else
            barColor = 0xF800;

        const int textRightX = LEFT_X + COL_W - ROW_PAD_X;
        const int barX = textRightX - SIGNAL_BAR_W;
        const int barY = leftY + ROW_H / 2 - SIGNAL_BAR_H / 2;
        spr->fillRoundRect(barX, barY, SIGNAL_BAR_W, SIGNAL_BAR_H, 2, BTN_OUTLINE);
        const int fillW = SIGNAL_BAR_W * percent / 100;
        if (fillW > 0)
            spr->fillRoundRect(barX, barY, fillW, SIGNAL_BAR_H, 2, barColor);

        char sigText[20];
        stringutils::safeFormat(sigText, "%d dB (%d%%)",
                                static_cast<int>(rssi), percent);
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_TEXT_BRIGHT);
        spr->drawString(sigText, barX - 6, leftY + ROW_H / 2);
    }
    else
    {
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_SUB);
        spr->drawString(I18n::str(SettingsKey::SETTINGS_INFO_NOT_CONNECTED),
                        LEFT_X + COL_W - ROW_PAD_X, leftY + ROW_H / 2);
    }
    spr->drawFastHLine(LEFT_X + ROW_PAD_X, leftY + ROW_H - 1,
                       COL_W - 2 * ROW_PAD_X, C_SEP_LINE);
    leftY += ROW_H;

    // Letzte zwei Zeilen links: NTP-Server und Sync
    for (const InfoRow &row : leftRowsBot)
    {
        drawInfoRow(LEFT_X, COL_W, leftY, row);
        leftY += ROW_H;
    }

    // Rechte Spalte: System-Werte untereinander
    int rightY = CONTENT_Y + HDR_H + 4;
    for (const InfoRow &row : rightRows)
    {
        drawInfoRow(RIGHT_X, COL_W, rightY, row);
        rightY += ROW_H;
    }

    const int i2cY = WORK_H - I2C_BOTTOM_GAP - ROW_H;
    const char *i2cLabel = reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_INFO_I2C));
    const int i2cLabelW = spr->textWidth(i2cLabel) + 2 * HDR_PAD_X;
    spr->fillSmoothRoundRect(CONTENT_X, i2cY + 1, i2cLabelW, HDR_H, 6, scaleColor(C_SEC_CONNECTION, I2C_BADGE_COLOR_SCALE));
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_TEXT_BRIGHT);
    spr->drawString(i2cLabel, CONTENT_X + HDR_PAD_X, i2cY + 1 + HDR_H / 2);
    spr->drawString(i2cBuf, CONTENT_X + i2cLabelW + I2C_LABEL_GAP, i2cY + ROW_H / 2);
    spr->drawFastHLine(CONTENT_X + ROW_PAD_X, i2cY + ROW_H - 1, CONTENT_W - 2 * ROW_PAD_X, C_SEP_LINE);

    spr->unloadFont();
}

// ===================================================================
// Fußzeile
// ===================================================================

/**
 * @brief Zeichnet die Fußzeile: löscht den Bereich bis zur Uhr, zeichnet Exit
 *        und den Speichern-Knopf nur, wenn es ungespeicherte Änderungen gibt.
 */
void Settings::drawFooter()
{
    if (!m_tft)
        return;

    m_tft->fillRect(DIVIDER_X + 1, FOOTER_Y, CLOCK_PUSH_X - DIVIDER_X - 1, FOOTER_H, C_BG);
    m_exitButton.drawButton();

    if (m_panel == Panel::INFO)
    {
        initFooterTextButton(m_btnRestart, footerInfoButtonRect(0), SettingsKey::SETTINGS_INFO_RESTART_BTN);
        m_btnRestart.enable();
        m_btnRestart.drawButton();

        initFooterTextButton(m_btnReset, footerInfoButtonRect(1), SettingsKey::SETTINGS_INFO_RESET_BTN);
        m_btnReset.enable();
        m_btnReset.drawButton();
    }
    else if (m_panel == Panel::CLOUD)
    {
        if (cloudPending())
        {
            m_saveButton.setLabelText(reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_FOOTER_SAVE)));
            m_saveButton.enable();
            m_saveButton.setFillColor(BTN_ACTIVE_FILL);
            m_saveButton.setOutlineColor(BTN_ACTIVE_FILL);
            m_saveButton.setTextColor(BTN_ACTIVE_TEXT);
            m_saveButton.drawButton();
        }

        initFooterTextButton(m_btnReset, footerCloudClearButtonRect(), SettingsKey::SETTINGS_CLOUD_CLEAR_BTN);
        m_btnReset.enable();
        m_btnReset.drawButton();
    }
    else if (m_panel == Panel::TIME_WINDOW)
    {
        const bool dirty = timeWindowPending();

        m_saveButton.setLabelText(reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_FOOTER_SAVE)));
        m_saveButton.setFillColor(BTN_ACTIVE_FILL);
        m_saveButton.setOutlineColor(BTN_ACTIVE_FILL);
        m_saveButton.setTextColor(BTN_ACTIVE_TEXT);
        if (dirty)
            m_saveButton.enable();
        else
            m_saveButton.disable();
        m_saveButton.drawButton();

        initFooterTextButton(m_btnReset, footerTimeWindowResetButtonRect(), SettingsKey::SETTINGS_INFO_RESET_BTN);
        m_btnReset.enable();
        m_btnReset.drawButton();
    }
    else if (panelDirty(m_panel))
    {
        m_saveButton.setLabelText(reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_FOOTER_SAVE)));
        m_saveButton.enable();
        m_saveButton.setFillColor(BTN_ACTIVE_FILL);
        m_saveButton.setOutlineColor(BTN_ACTIVE_FILL);
        m_saveButton.setTextColor(BTN_ACTIVE_TEXT);
        m_saveButton.drawButton();
    }

    if (m_panel == Panel::AUDIO)
        drawAudioFooterControls();

    m_footerDirty = false;
}

/**
 * @brief Zeichnet Hörprobe-Knopf und Lautstärke-Knöpfe im Footer (nur Klang-Panel).
 */
void Settings::drawAudioFooterControls()
{
    m_audioPlayBtn.setLabelText(m_previewPlaying ? "^STOP:32:A944^" : "^PLAY:32:5C44^");
    m_audioPlayBtn.drawButton(false);
    m_audioVolDownBtn.drawButton(false);
    m_audioVolUpBtn.drawButton(false);

    // Rahmen um die Lautstärke-Zahl
    const int frameY = FOOTER_CY - AUDIO_FT_BTN_SIZE / 2;
    m_tft->fillSmoothRoundRect(AUDIO_FT_FRAME_X, frameY, AUDIO_FT_FRAME_W, AUDIO_FT_BTN_SIZE, AUDIO_FT_BTN_R, BTN_OUTLINE);
    m_tft->fillSmoothRoundRect(AUDIO_FT_FRAME_X + 1, frameY + 1, AUDIO_FT_FRAME_W - 2, AUDIO_FT_BTN_SIZE - 2, AUDIO_FT_BTN_R, TFT_BLACK);

    drawAudioVolumeValue();
}

/**
 * @brief Aktualisiert die Lautstärke-DotMatrix im Footer.
 */
void Settings::drawAudioVolumeValue()
{
    if (!m_audioVolSprite)
        return;

    const uint8_t vol = constrain(APP_STATE.getCurrentVolume(), 0, 99);
    char volStr[4];
    stringutils::safeFormat(volStr, "%2d", vol);
    m_audioVolDots.setText(volStr);

    m_audioVolSprite->clear(TFT_BLACK);
    m_audioVolDots.draw(m_audioVolSprite, 0, 0);

    const int sprY = FOOTER_CY - AUDIO_FT_DISP_H / 2;
    m_audioVolSprite->pushSprite(AUDIO_FT_DISP_X, sprY);
}

/**
 * @brief Zeichnet die Digitaluhr (interner Redraw nur bei Sekundenwechsel).
 */
void Settings::drawClock()
{
    if (!m_clockSprite)
        return;

    DS1307_RTC *rtc = &APP_STATE.getRTC();
    Clocks::drawDigitalClockPlain(m_clockSprite, rtc, m_lastDrawnSecond, CLOCK_PUSH_X, CLOCK_PUSH_Y, false, true);
}

// ===================================================================
// Touch-Verarbeitung
// ===================================================================

/**
 * @brief Verarbeitet Touch-Eingaben und verteilt sie auf die Bereiche.
 */
bool Settings::handleTouch()
{
    int32_t tx, ty;

    if (m_requireTouchRelease)
    {
        bool touching = m_tft->getTouch(&tx, &ty);
        if (!touching && (m_touchBlockUntil == 0 || millis() >= m_touchBlockUntil))
            m_requireTouchRelease = false;
        return false;
    }

    if (m_tft->getTouch(&tx, &ty))
    {
        if (!m_touchProcessed)
        {
            m_touchProcessed = true;

            if (pointIn(tx, ty, MENU_X, 0, MENU_W, MENU_SPRITE_H))
                handleMenuTouch(tx, ty);
            else if (pointIn(tx, ty, WORK_X, WORK_Y, WORK_W, WORK_H))
                handleWorkTouch(tx, ty);
            else if (ty >= FOOTER_Y)
            {
                bool handled = false;

                if (m_panel == Panel::INFO)
                {
                    const Rect restart = footerInfoButtonRect(0);
                    const Rect reset = footerInfoButtonRect(1);
                    if (pointIn(tx, ty, restart.x, restart.y, restart.w, restart.h))
                    {
                        startRestartDialog();
                        handled = true;
                    }
                    else if (pointIn(tx, ty, reset.x, reset.y, reset.w, reset.h))
                    {
                        startResetDialog();
                        handled = true;
                    }
                }
                else if (m_panel == Panel::CLOUD)
                {
                    const Rect clear = footerCloudClearButtonRect();
                    if (cloudPending() && m_saveButton.contains(tx, ty))
                    {
                        startSaving();
                        handled = true;
                    }
                    else if (pointIn(tx, ty, clear.x, clear.y, clear.w, clear.h))
                    {
                        startCloudClearDialog();
                        handled = true;
                    }
                }
                else if (m_panel == Panel::TIME_WINDOW)
                {
                    const Rect reset = footerTimeWindowResetButtonRect();
                    if (m_saveButton.contains(tx, ty))
                    {
                        if (timeWindowPending())
                            startSaving();
                        handled = true;
                    }
                    else if (pointIn(tx, ty, reset.x, reset.y, reset.w, reset.h))
                    {
                        startTimeWindowResetDialog();
                        handled = true;
                    }
                }

                if (m_panel == Panel::AUDIO && !handled)
                {
                    if (m_audioPlayBtn.contains(tx, ty))
                    {
                        if (m_previewPlaying)
                        {
                            if (m_callbacks.requestStopPlayback)
                                m_callbacks.requestStopPlayback();
                            m_previewPlaying = false;
                        }
                        else
                        {
                            if (m_callbacks.requestPlayStation)
                                m_callbacks.requestPlayStation(APP_STATE.getCurrentRetroStation());
                            m_previewPlaying = true;
                        }
                        m_footerDirty = true;
                        handled = true;
                    }
                    else if (m_audioVolUpBtn.contains(tx, ty))
                    {
                        int vol = APP_STATE.getCurrentVolume();
                        vol = (vol + AUDIO_FT_VOL_STEP > 100) ? 100 : vol + AUDIO_FT_VOL_STEP;
                        APP_STATE.setCurrentVolume(static_cast<uint8_t>(vol));
                        if (m_callbacks.onVolumeChanged)
                            m_callbacks.onVolumeChanged(static_cast<uint8_t>(vol));
                        extern void applyAudioTone(uint16_t freqLow, uint16_t freqPeak, uint16_t freqHigh, uint8_t loudness);
                        applyAudioTone(m_eqFreqLow, m_eqFreqPeak, m_eqFreqHigh, static_cast<uint8_t>(m_loudness));
                        drawAudioVolumeValue();
                        handled = true;
                    }
                    else if (m_audioVolDownBtn.contains(tx, ty))
                    {
                        int vol = APP_STATE.getCurrentVolume();
                        vol = (vol < AUDIO_FT_VOL_STEP) ? 0 : vol - AUDIO_FT_VOL_STEP;
                        APP_STATE.setCurrentVolume(static_cast<uint8_t>(vol));
                        if (m_callbacks.onVolumeChanged)
                            m_callbacks.onVolumeChanged(static_cast<uint8_t>(vol));
                        extern void applyAudioTone(uint16_t freqLow, uint16_t freqPeak, uint16_t freqHigh, uint8_t loudness);
                        applyAudioTone(m_eqFreqLow, m_eqFreqPeak, m_eqFreqHigh, static_cast<uint8_t>(m_loudness));
                        drawAudioVolumeValue();
                        handled = true;
                    }
                }

                if (!handled)
                {
                    if (m_saveButton.contains(tx, ty))
                    {
                        if (panelDirty(m_panel))
                            startSaving();
                    }
                    else if (m_exitButton.contains(tx, ty))
                    {
                        m_exitButton.press(true);
                        m_exitButton.drawButton(true);
                    }
                }
            }
            return true;
        }
    }
    else
    {
        if (m_exitButton.isPressed())
        {
            m_exitButton.press(false);
            m_exitButton.drawButton(false);
            startExitDialog();
            m_touchProcessed = false;
            return true;
        }

        m_touchProcessed = false;
    }
    return false;
}

/**
 * @brief Behandelt einen Tap im Menü.
 */
void Settings::handleMenuTouch(int32_t tx, int32_t ty)
{
    (void)tx;
    const int panelIdx = menuEntryAtY(static_cast<int>(ty));
    if (panelIdx < 0)
        return;
    selectPanel(static_cast<Panel>(panelIdx));
}

/**
 * @brief Wechselt das angezeigte Panel.
 */
void Settings::selectPanel(Panel p)
{
    if (p == m_panel)
        return;
    m_panel = p;
    if (p == Panel::SECURITY)
    {
        resetPinEntry();
        m_numpadLayoutValid = false;
    }
    if (p == Panel::CLOUD)
        m_cloudValuesVisible = false;
    m_menuDirty = true;
    m_workDirty = true;
    m_footerDirty = true;
}

/**
 * @brief Verteilt einen Tap im Arbeitsbereich an das aktive Panel.
 */
void Settings::handleWorkTouch(int32_t tx, int32_t ty)
{
    const int sx = static_cast<int>(tx) - WORK_X;
    const int sy = static_cast<int>(ty) - WORK_Y;

    switch (m_panel)
    {
    case Panel::WIFI:
        handleWifiTouch(sx, sy);
        break;
    case Panel::LANGUAGE:
        handleLanguageTouch(sx, sy);
        break;
    case Panel::SECURITY:
        handleSecurityTouch(sx, sy);
        break;
    case Panel::NEWS:
        handleNewsTouch(sx, sy);
        break;
    case Panel::WEATHER:
        handleWeatherTouch(sx, sy);
        break;
    case Panel::CLOUD:
        handleCloudTouch(sx, sy);
        break;
    case Panel::TIME:
        handleTimeTouch(sx, sy);
        break;
    case Panel::TIME_WINDOW:
        handleTimeWindowTouch(sx, sy);
        break;
    case Panel::AUDIO:
        handleAudioTouch(sx, sy);
        break;
    case Panel::APPEARANCE:
        handleAppearanceTouch(sx, sy);
        break;
    default:
        break;
    }
}

/**
 * @brief Behandelt Taps im WLAN-Panel.
 */
void Settings::handleWifiTouch(int sx, int sy)
{
    for (int i = 0; i < MAX_PROFILES; ++i)
    {
        const Rect t = wifiToggleRect(i);
        if (pointIn(sx, sy, t.x, t.y, t.w, t.h))
        {
            onToggleWifi(i);
            return;
        }
        const Rect r = wifiRowRect(i);
        if (pointIn(sx, sy, r.x, r.y, r.w, r.h))
        {
            if (m_selWifi != i)
            {
                m_selWifi = i;
                m_workDirty = true;
            }
            return;
        }
    }

    const bool configured = isProfileValid(m_selWifi);
    for (int k = 0; k < 4; ++k)
    {
        const Rect a = wifiActionRect(k);
        if (!pointIn(sx, sy, a.x, a.y, a.w, a.h))
            continue;

        if (k != 0 && !configured)
            return; // Passwort/Test/Löschen brauchen eine SSID

        switch (k)
        {
        case 0:
            startScan(m_selWifi);
            break;
        case 1:
            startPasswordInput(m_selWifi);
            break;
        case 2:
            startConnectionTest(m_selWifi);
            break;
        case 3:
            startClearProfileDialog(m_selWifi);
            break;
        default:
            break;
        }
        return;
    }

    const Rect hf = hostnameFieldRect();
    if (pointIn(sx, sy, hf.x, hf.y, hf.w, hf.h))
        startHostnameInput();
}

/**
 * @brief Behandelt Taps im Sprach-Panel.
 */
void Settings::handleLanguageTouch(int sx, int sy)
{
    for (int i = 0; i < 2; ++i)
    {
        const Rect r = langRowRect(i);
        if (pointIn(sx, sy, r.x, r.y, r.w, r.h))
        {
            applyLanguage(i);
            return;
        }
    }
}

/**
 * @brief Behandelt Taps im Sicherheit-Panel (Nummernblock).
 */
void Settings::handleSecurityTouch(int sx, int sy)
{
    const int cx = sx - CONTENT_X;
    const int cy = sy - CONTENT_Y;
    const int key = m_numpad.hitTest(cx, cy);
    if (key < 0)
        return;

    handlePinKey(WorkshopNumpad::keyAction(key));
    if (m_state == State::IDLE)
        m_workDirty = true;
}

// ===================================================================
// Sprache und PIN
// ===================================================================

/**
 * @brief Liefert den Index der aktuell aktiven Sprache (0=de, 1=en).
 */
int Settings::currentLangIndex() const
{
    return (I18n::currentLang() == LangIndex::EN) ? 1 : 0;
}

/**
 * @brief Prüft, ob eine abweichende Sprache vorgemerkt ist.
 */
bool Settings::languagePending() const
{
    return m_pendingLangIndex >= 0 && m_pendingLangIndex != currentLangIndex();
}

/**
 * @brief Merkt die gewählte Sprache vor (wird erst beim Speichern angewendet).
 */
void Settings::applyLanguage(int langIndex)
{
    m_pendingLangIndex = (langIndex == currentLangIndex()) ? -1 : langIndex;
    recomputeDirtyFlags();
    m_workDirty = true;
    m_footerDirty = true;
}

/**
 * @brief Setzt die laufende PIN-Eingabe zurück (eine vorgemerkte PIN bleibt erhalten).
 */
void Settings::resetPinEntry()
{
    m_pinLen = 0;
    m_pinEntry[0] = '\0';
    m_pinConfirming = false;
    m_pinFirst[0] = '\0';
    m_pinMismatch = false;
}

/**
 * @brief Verarbeitet eine Nummernblock-Taste für die PIN.
 *
 * Erst werden vier Ziffern eingegeben, dann zur Kontrolle wiederholt. Stimmen beide überein, wird die PIN nur vorgemerkt und der Speichern-Knopf aktiv; bei Abweichung erscheint ein Hinweis und die Eingabe beginnt von vorn.
 */
void Settings::handlePinKey(char action)
{
    if (action >= '0' && action <= '9')
    {
        m_pinMismatch = false;
        if (m_pinLen < 4)
        {
            m_pinEntry[m_pinLen++] = action;
            m_pinEntry[m_pinLen] = '\0';
        }
        if (m_pinLen < 4)
            return;

        if (!m_pinConfirming)
        {
            stringutils::safeCopy(m_pinFirst, m_pinEntry);
            m_pinConfirming = true;
            m_pinLen = 0;
            m_pinEntry[0] = '\0';
            return;
        }

        if (strcmp(m_pinEntry, m_pinFirst) == 0)
            stringutils::safeCopy(m_pinPending, m_pinEntry);
        else
            m_pinMismatch = true;

        m_pinConfirming = false;
        m_pinFirst[0] = '\0';
        m_pinLen = 0;
        m_pinEntry[0] = '\0';
        recomputeDirtyFlags();
        return;
    }

    if (action == 'B')
    {
        if (m_pinLen > 0)
        {
            m_pinLen--;
            m_pinEntry[m_pinLen] = '\0';
        }
        return;
    }

    if (action == 'C')
        resetPinEntry();
}

/**
 * @brief Setzt eine vorgemerkte PIN tatsächlich als Sperr-PIN.
 * @return true wenn eine PIN vorgemerkt war und gesetzt wurde
 */
bool Settings::applyPendingPin()
{
    if (m_pinPending[0] == '\0')
        return false;

    APP_STATE.setLockPin(m_pinPending);
    m_pinPending[0] = '\0';
    m_pinMismatch = false;
    LOG_SUCCESS("Neue Sperr-PIN gespeichert");
    return true;
}

/**
 * @brief Prüft, ob eine PIN vorgemerkt, aber noch nicht gespeichert ist.
 */
bool Settings::pinPending() const
{
    return m_pinPending[0] != '\0';
}

// ===================================================================
// State-Machine für Dialoge und Test
// ===================================================================

/**
 * @brief Wechselt den State-Machine-Zustand.
 */
void Settings::changeState(State newState)
{
    m_state = newState;
    m_operationStartTime = millis();
}

/**
 * @brief Aktualisiert den Zustand TESTING.
 */
void Settings::updateTestingState()
{
    if (!m_activeProgressBox)
    {
        char title[40];
        STRF(title, SettingsKey::SETTINGS_WIFI_TEST_BOX_TITLE);
        char msg[64];
        STRF(msg, SettingsKey::SETTINGS_WIFI_TEST_RUNNING);
        m_activeProgressBox.reset(new ProgressBox(*m_tft));
        m_activeProgressBox->setAutoCenter(true);
        m_activeProgressBox->setTitle(title);
        m_activeProgressBox->start(msg, 0);
    }

    const uint32_t elapsed = millis() - m_operationStartTime;
    const int progress = (elapsed >= TEST_TIMEOUT) ? 100 : static_cast<int>((elapsed * 100UL) / TEST_TIMEOUT);
    m_activeProgressBox->updateProgress(progress);

    yield();

    if (checkTestStatus())
    {
        if (m_activeProgressBox)
        {
            m_activeProgressBox->finish();
            m_activeProgressBox.reset();
        }
        showTestResult();
    }
}

/**
 * @brief Aktualisiert den Zustand SHOWING_MESSAGE.
 */
void Settings::updateMessageState()
{
    if (!m_activeMessageBox)
    {
        changeState(State::IDLE);
        afterModalRedraw();
        return;
    }

    m_activeMessageBox->update();

    if (m_activeMessageBox->isBusy())
        m_activeMessageBox->completeConfirmation(true);

    if (!m_activeMessageBox->isActive())
    {
        m_activeMessageBox.reset();
        changeState(State::IDLE);
        afterModalRedraw();
    }
}

/**
 * @brief Aktualisiert den Zustand SAVING.
 */
void Settings::updateSavingState()
{
    if (!m_activeMessageBox)
    {
        changeState(State::IDLE);
        afterModalRedraw();
        return;
    }

    m_activeMessageBox->update();

    if (m_activeMessageBox->isBusy())
        m_activeMessageBox->completeConfirmation(true);

    if (!m_activeMessageBox->isActive())
    {
        m_activeMessageBox.reset();
        changeState(State::IDLE);
        afterModalRedraw();
    }
}

/**
 * @brief Behandelt verzögerte kritische Aktionen (Neustart/Exit).
 * @return true, wenn das Modul danach möglicherweise abgebaut ist
 */
bool Settings::handleDeferredActions()
{
    if (m_restartRequested)
    {
        m_restartRequested = false;
        performRestart();
        return true;
    }

    if (m_exitRequested)
    {
        m_exitRequested = false;
        if (m_callbacks.onExitRequested)
        {
            m_callbacks.onExitRequested();
            return true;
        }
    }
    return false;
}

/**
 * @brief Führt einen robusten Neustart aus.
 */
void Settings::performRestart()
{
    LOG_INFO("Neustart wird durchgeführt...");

    if (m_callbacks.onRestartRequested)
        m_callbacks.onRestartRequested();

    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);

    Serial.flush();
    delay(100);

    // USB Serial/JTAG D+ Pull-up deaktivieren, sonst hängt esp_restart() gelegentlich
    usb_serial_jtag_ll_disable_intr_mask(USB_SERIAL_JTAG_LL_INTR_MASK);
    *(volatile uint32_t *)USB_SERIAL_JTAG_CONF0_REG &= ~(1 << 9);
    delay(100);

    esp_restart();

    while (true)
        delay(1000);
}

// ===================================================================
// WLAN-Aktionen
// ===================================================================

/**
 * @brief Schaltet ein Profil aktiv/inaktiv.
 */
void Settings::onToggleWifi(int profileIndex)
{
    if (profileIndex < 0 || profileIndex >= MAX_PROFILES)
        return;

    if (!m_profiles[profileIndex].enabled && !isProfileValid(profileIndex))
    {
        showInfoBox(SettingsKey::SETTINGS_WIFI_NEED_SSID_TITLE, SettingsKey::SETTINGS_WIFI_NEED_SSID_MSG);
        return;
    }

    m_profiles[profileIndex].enabled = !m_profiles[profileIndex].enabled;
    recomputeDirtyFlags();
    m_workDirty = true;
    m_footerDirty = true;

    LOG_INFO_F("Profil %d: %s", profileIndex + 1, m_profiles[profileIndex].enabled ? "aktiviert" : "deaktiviert");
}

/**
 * @brief Startet den WLAN-Scan (blockierender Modal-Dialog).
 */
void Settings::startScan(int profileIndex)
{
    if (profileIndex < 0 || profileIndex >= MAX_PROFILES)
        return;

    m_currentProfileIndex = profileIndex;

    char label[48];
    STRF(label, SettingsKey::SETTINGS_WIFI_SCAN_TITLE);

    WifiList wifiList(m_tft);
    char selectedSSID[65] = {0};
    wifiList.setPosition(150, 90);
    wifiList.setSize(500, 300);

    if (wifiList.show(label, selectedSSID) && strlen(selectedSSID) > 0)
    {
        stringutils::safeCopy(m_profiles[profileIndex].ssid, selectedSSID);
        m_profiles[profileIndex].tested_ok = false;
        recomputeDirtyFlags();
        LOG_SUCCESS_F("SSID gewählt: %s", selectedSSID);
    }

    afterModalRedraw();
}

/**
 * @brief Startet die Passwort-Eingabe (blockierender Modal-Dialog).
 */
void Settings::startPasswordInput(int profileIndex)
{
    if (profileIndex < 0 || profileIndex >= MAX_PROFILES)
        return;

    if (!isProfileValid(profileIndex))
    {
        showInfoBox(SettingsKey::SETTINGS_WIFI_NEED_SSID_TITLE, SettingsKey::SETTINGS_WIFI_NEED_SSID_MSG);
        return;
    }

    m_currentProfileIndex = profileIndex;

    Keyboard keyboard(m_tft);
    keyboard.setPasswordMode(true);

    char password[65] = {0};
    stringutils::safeCopy(password, m_profiles[profileIndex].password);

    char label[96];
    STRF(label, SettingsKey::SETTINGS_WIFI_PW_TITLE_FMT, m_profiles[profileIndex].ssid);

    if (keyboard.show(label, password, sizeof(password), password))
    {
        char normalized[65] = {0};
        SettingsKey errorKey = SettingsKey::SETTINGS_EMPTY;

        if (!validateAndNormalizePassword(password, normalized, sizeof(normalized), errorKey))
        {
            afterModalRedraw();
            showInfoBox(SettingsKey::SETTINGS_WIFI_PW_BAD_TITLE, errorKey);
            return;
        }

        stringutils::safeCopy(m_profiles[profileIndex].password, normalized);
        m_profiles[profileIndex].tested_ok = false;
        recomputeDirtyFlags();
        LOG_SUCCESS("Passwort aktualisiert");
    }

    afterModalRedraw();
}

/**
 * @brief Startet den Verbindungstest für ein Profil.
 */
void Settings::startConnectionTest(int profileIndex)
{
    if (profileIndex < 0 || profileIndex >= MAX_PROFILES)
        return;

    if (!isProfileConfigured(profileIndex))
    {
        showInfoBox(SettingsKey::SETTINGS_WIFI_NEED_SSID_TITLE, SettingsKey::SETTINGS_WIFI_NEED_SSID_MSG);
        return;
    }

    m_currentProfileIndex = profileIndex;
    m_testState = TestState::CONNECTING;

    char title[40];
    STRF(title, SettingsKey::SETTINGS_WIFI_TEST_BOX_TITLE);
    char prep[64];
    STRF(prep, SettingsKey::SETTINGS_WIFI_TEST_PREPARE);

    m_activeProgressBox.reset(new ProgressBox(*m_tft));
    m_activeProgressBox->setAutoCenter(true);
    m_activeProgressBox->setTitle(title);
    m_activeProgressBox->start(prep, 0);

    delay(50);

    WiFi.disconnect(false);
    delay(250);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(m_profiles[profileIndex].ssid, m_profiles[profileIndex].password);

    if (m_activeProgressBox)
    {
        char run[64];
        STRF(run, SettingsKey::SETTINGS_WIFI_TEST_RUNNING);
        m_activeProgressBox->updateMessage(run);
    }

    changeState(State::TESTING);
}

/**
 * @brief Prüft den Status des laufenden Verbindungstests.
 */
bool Settings::checkTestStatus()
{
    yield();

    if (millis() - m_operationStartTime > TEST_TIMEOUT)
    {
        m_testState = TestState::TIMEOUT;
        return true;
    }

    const wl_status_t status = WiFi.status();
    if (status == WL_CONNECTED)
    {
        m_testState = TestState::CONNECTED;
        return true;
    }

    const uint32_t elapsed = millis() - m_operationStartTime;
    if (elapsed > 5000)
    {
        if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL || status == WL_CONNECTION_LOST)
        {
            m_testState = TestState::FAILED;
            return true;
        }
        if (status == WL_DISCONNECTED && elapsed > 7000)
        {
            m_testState = TestState::FAILED;
            return true;
        }
    }

    return false;
}

/**
 * @brief Zeigt das Ergebnis des Verbindungstests an.
 */
void Settings::showTestResult()
{
    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 140, 400, 200);

    if (m_testState == TestState::CONNECTED)
    {
        const IPAddress ip = WiFi.localIP();
        const int32_t rssi = WiFi.RSSI();

        char title[40];
        STRF(title, SettingsKey::SETTINGS_WIFI_TEST_OK_TITLE);
        char msg[160];
        STRF(msg, SettingsKey::SETTINGS_WIFI_TEST_OK_FMT, ip.toString().c_str(), static_cast<int>(rssi));

        m_activeMessageBox->setTitle(title);
        m_activeMessageBox->setMessage(msg);

        if (m_currentProfileIndex >= 0 && m_currentProfileIndex < MAX_PROFILES)
        {
            m_profiles[m_currentProfileIndex].tested_ok = true;
            if (!m_profiles[m_currentProfileIndex].enabled)
            {
                m_profiles[m_currentProfileIndex].enabled = true;
                recomputeDirtyFlags();
            }
        }
        LOG_SUCCESS_F("Verbindungstest erfolgreich: %s", ip.toString().c_str());
    }
    else
    {
        char title[40];
        STRF(title, SettingsKey::SETTINGS_WIFI_TEST_FAIL_TITLE);
        char msg[128];
        STRF(msg, (m_testState == TestState::TIMEOUT)
                      ? SettingsKey::SETTINGS_WIFI_TEST_TIMEOUT_MSG
                      : SettingsKey::SETTINGS_WIFI_TEST_FAIL_MSG);

        m_activeMessageBox->setTitle(title);
        m_activeMessageBox->setMessage(msg);

        if (m_currentProfileIndex >= 0 && m_currentProfileIndex < MAX_PROFILES)
            m_profiles[m_currentProfileIndex].tested_ok = false;
        LOG_ERROR("WLAN-Verbindungstest fehlgeschlagen");
    }

    m_activeMessageBox->setButtonLabels(reinterpret_cast<const char *>(I18n::str(CommonKey::COMMON_BTN_OK)), "");
    m_activeMessageBox->start();

    if (m_testState != TestState::CONNECTED)
        WiFi.disconnect(false);

    changeState(State::SHOWING_MESSAGE);
    m_testState = TestState::NOT_TESTING;
}

/**
 * @brief Startet den Bestätigungs-Dialog für die Profil-Löschung.
 */
void Settings::startClearProfileDialog(int profileIndex)
{
    if (profileIndex < 0 || profileIndex >= MAX_PROFILES)
        return;

    char title[40];
    STRF(title, SettingsKey::SETTINGS_WIFI_DELETE_TITLE);
    char msg[96];
    STRF(msg, SettingsKey::SETTINGS_WIFI_DELETE_MSG);

    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 140, 400, 200);
    m_activeMessageBox->setTitle(title);
    m_activeMessageBox->setMessage(msg);
    m_activeMessageBox->setButtonLabels(
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_WIFI_ACT_DELETE)),
        reinterpret_cast<const char *>(I18n::str(CommonKey::COMMON_BTN_CANCEL)));

    m_activeMessageBox->setCallback([this, profileIndex](bool confirmed)
                                    {
        if (confirmed)
        {
            m_profiles[profileIndex].ssid[0] = '\0';
            m_profiles[profileIndex].password[0] = '\0';
            m_profiles[profileIndex].enabled = false;
            m_profiles[profileIndex].tested_ok = false;
            recomputeDirtyFlags();
            LOG_INFO_F("Profil %d zum Löschen vorgemerkt", profileIndex + 1);
        } });

    m_activeMessageBox->start();
    changeState(State::SHOWING_MESSAGE);
}

/**
 * @brief Validiert und normalisiert ein WLAN-Passwort (WPA/WPA2/WPA3).
 */
bool Settings::validateAndNormalizePassword(const char *input, char *output, size_t outSize,
                                            SettingsKey &errorKey) const
{
    if (!input || !output || outSize == 0)
    {
        errorKey = SettingsKey::SETTINGS_WIFI_PW_ERR_INTERNAL;
        return false;
    }

    const size_t len = strlen(input);

    // Offenes Netzwerk: leeres Passwort erlaubt
    if (len == 0)
    {
        output[0] = '\0';
        return true;
    }

    // 64-stelliger Hex-PSK
    if (len == 64)
    {
        for (size_t i = 0; i < len; ++i)
        {
            if (!std::isxdigit(static_cast<unsigned char>(input[i])))
            {
                errorKey = SettingsKey::SETTINGS_WIFI_PW_ERR_HEX;
                return false;
            }
        }
        if (outSize <= len)
        {
            errorKey = SettingsKey::SETTINGS_WIFI_PW_ERR_INTERNAL;
            return false;
        }
        for (size_t i = 0; i < len; ++i)
            output[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(input[i])));
        output[len] = '\0';
        return true;
    }

    // Normale Passphrase: 8..63 druckbare ASCII-Zeichen
    if (len < 8 || len > 63)
    {
        errorKey = SettingsKey::SETTINGS_WIFI_PW_ERR_LEN;
        return false;
    }
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(input[i]);
        if (c < 0x20 || c > 0x7E)
        {
            errorKey = SettingsKey::SETTINGS_WIFI_PW_ERR_CHARS;
            return false;
        }
    }
    if (outSize <= len)
    {
        errorKey = SettingsKey::SETTINGS_WIFI_PW_ERR_INTERNAL;
        return false;
    }

    stringutils::safeCopy(output, outSize, input);
    return true;
}

/**
 * @brief Validiert einen Netzwerknamen nach den Hostname-Regeln (RFC 1123).
 *
 * Erlaubt sind Buchstaben, Ziffern und der Bindestrich, 1 bis 32 Zeichen. Der Name darf nicht mit einem Bindestrich beginnen oder enden. Bei Erfolg wird der Name unverändert nach output kopiert.
 */
bool Settings::validateHostname(const char *input, char *output, size_t outSize,
                                SettingsKey &errorKey) const
{
    if (!input || !output || outSize == 0)
    {
        errorKey = SettingsKey::SETTINGS_WIFI_HOSTNAME_ERR;
        return false;
    }

    const size_t len = strlen(input);
    if (len < 1 || len > 32 || outSize <= len)
    {
        errorKey = SettingsKey::SETTINGS_WIFI_HOSTNAME_ERR;
        return false;
    }

    if (input[0] == '-' || input[len - 1] == '-')
    {
        errorKey = SettingsKey::SETTINGS_WIFI_HOSTNAME_ERR;
        return false;
    }

    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(input[i]);
        const bool ok = std::isalnum(c) || c == '-';
        if (!ok)
        {
            errorKey = SettingsKey::SETTINGS_WIFI_HOSTNAME_ERR;
            return false;
        }
    }

    stringutils::safeCopy(output, outSize, input);
    return true;
}

/**
 * @brief Öffnet die Tastatur zur Eingabe des Geräte-Netzwerknamens.
 */
void Settings::startHostnameInput()
{
    if (!m_tft)
        return;

    Keyboard keyboard(m_tft);
    keyboard.setPasswordMode(false);

    char label[40];
    STRF(label, SettingsKey::SETTINGS_WIFI_HOSTNAME_PROMPT);

    char buffer[33] = {0};
    stringutils::safeCopy(buffer, m_hostname);

    if (keyboard.show(label, buffer, sizeof(buffer), buffer))
    {
        char normalized[33] = {0};
        SettingsKey errorKey = SettingsKey::SETTINGS_EMPTY;

        if (!validateHostname(buffer, normalized, sizeof(normalized), errorKey))
        {
            afterModalRedraw();
            showInfoBox(SettingsKey::SETTINGS_WIFI_HOSTNAME_BAD_TITLE, errorKey);
            return;
        }

        stringutils::safeCopy(m_hostname, normalized);
        recomputeDirtyFlags();
        LOG_SUCCESS_F("Hostname vorgemerkt: %s", m_hostname);
    }

    afterModalRedraw();
}

// ===================================================================
// Speichern, Verlassen, Neustart, Zurücksetzen
// ===================================================================

/**
 * @brief Speichert die geänderten WLAN-Profile und den Netzwerknamen in AppState.
 */
void Settings::saveProfiles()
{
    for (int i = 0; i < MAX_PROFILES; ++i)
    {
        if (m_profiles[i].modified)
        {
            APP_STATE.setWiFiProfile(i, m_profiles[i].ssid, m_profiles[i].password, m_profiles[i].enabled);
            m_profiles[i].modified = false;
        }
    }

    if (hostnamePending())
    {
        APP_STATE.setHostname(m_hostname);
        stringutils::safeCopy(m_hostnameBaseline, m_hostname);
    }

    snapshotBaseline();
    recomputeDirtyFlags();
}

/**
 * @brief Warnt vor dem für eine Sprachänderung nötigen Neustart.
 *
 * Bei Bestätigung werden die Profile gesichert, die Sprache persistiert und sofort neu gestartet; nur so übernehmen alle Module die neue Sprache. Bei Abbruch wird die Vormerkung verworfen, die Sprache bleibt unverändert.
 */
void Settings::startLanguageRestartDialog()
{
    char title[40];
    STRF(title, SettingsKey::SETTINGS_LANG_RESTART_TITLE);
    char msg[128];
    STRF(msg, SettingsKey::SETTINGS_LANG_RESTART_MSG);

    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 140, 400, 200);
    m_activeMessageBox->setTitle(title);
    m_activeMessageBox->setMessage(msg);
    m_activeMessageBox->setButtonLabels(
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_INFO_RESTART_BTN)),
        reinterpret_cast<const char *>(I18n::str(CommonKey::COMMON_BTN_CANCEL)));

    m_activeMessageBox->setCallback([this](bool restart)
                                    {
        if (restart)
        {
            saveProfiles();
            APP_STATE.setUiLanguage(m_pendingLangIndex == 1 ? "en" : "de");
            m_pendingLangIndex = -1;
            m_restartRequested = true;
        }
        else
        {
            m_pendingLangIndex = -1;
            recomputeDirtyFlags();
            m_workDirty = true;
            m_footerDirty = true;
        } });

    m_activeMessageBox->start();
    changeState(State::SHOWING_MESSAGE);
}

/**
 * @brief Startet den Speichervorgang mit Bestätigungs-Dialog.
 */
/**
 * @brief Speichert die Änderungen des aktuellen Panels.
 *
 * Save wirkt nur auf das Panel, auf dem es eingeblendet ist: WLAN sichert die Profile, Sprache löst den Neustart-Dialog aus, Security setzt die PIN.
 */
void Settings::startSaving()
{
    switch (m_panel)
    {
    case Panel::LANGUAGE:
        startLanguageRestartDialog();
        return;

    case Panel::SECURITY:
        if (applyPendingPin())
        {
            recomputeDirtyFlags();
            showInfoBox(SettingsKey::SETTINGS_SEC_SAVED_TITLE, SettingsKey::SETTINGS_SEC_SAVED_MSG);
        }
        return;

    case Panel::NEWS:
        saveNews();
        return;

    case Panel::WEATHER:
        saveWeather();
        return;

    case Panel::CLOUD:
        saveCloud();
        return;

    case Panel::TIME:
        saveTime();
        return;

    case Panel::TIME_WINDOW:
        saveTimeWindow();
        return;

    case Panel::AUDIO:
        saveAudio();
        return;

    case Panel::APPEARANCE:
        saveAppearance();
        return;

    case Panel::WIFI:
    {
        saveProfiles();

        char title[40];
        STRF(title, SettingsKey::SETTINGS_SAVED_TITLE);
        char msg[128];
        STRF(msg, SettingsKey::SETTINGS_SAVED_MSG);

        m_activeMessageBox.reset(new MessageBox(m_tft));
        m_activeMessageBox->setBounds(200, 140, 400, 220);
        m_activeMessageBox->setTitle(title);
        m_activeMessageBox->setMessage(msg);
        m_activeMessageBox->setButtonLabels(
            reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_SAVED_RESTART_BTN)),
            reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_SAVED_LATER_BTN)));

        m_activeMessageBox->setCallback([this](bool restart)
                                        {
            if (restart)
                m_restartRequested = true; });

        m_activeMessageBox->start();
        changeState(State::SAVING);
        return;
    }

    default:
        return;
    }
}

/**
 * @brief Startet den Exit-Dialog mit optionalem Speichern.
 */
void Settings::startExitDialog()
{
    recomputeDirtyFlags();

    if (!m_changesMade)
    {
        m_exitRequested = true;
        return;
    }

    char title[40];
    STRF(title, SettingsKey::SETTINGS_EXIT_TITLE);
    char msg[64];
    STRF(msg, SettingsKey::SETTINGS_EXIT_MSG);

    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 140, 400, 200);
    m_activeMessageBox->setTitle(title);
    m_activeMessageBox->setMessage(msg);
    m_activeMessageBox->setButtonLabels(
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_FOOTER_SAVE)),
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_EXIT_DISCARD_BTN)));

    m_activeMessageBox->setCallback([this](bool save)
                                    {
        if (save)
        {
            // Alle Panels still sichern. Eine Bestätigungsbox darf hier niemand öffnen, weil der Rückruf aus der noch laufenden Meldungsbox heraus läuft und eine zweite sie unter sich selbst wegräumen würde.
            saveProfiles();
            applyPendingPin();

            // Nur eine wirklich geänderte Postleitzahl schreiben; weatherPending meldet auch die unveränderte, und AppState::setPlz leert dabei das Nachrichten-Ressort im Arbeitsspeicher. Aus demselben Grund steht die Postleitzahl vor den Nachrichten.
            if (m_plzLen == PLZ_DIGITS && strcmp(m_plzEntry, m_plzBaseline) != 0)
                persistPlz();
            if (newsPending())
                persistNews(false);

            if (cloudPending())
                persistCloud(false);
            if (timePending())
                persistTime(false);
            if (timeWindowPending())
                persistTimeWindow(false);
            if (audioPending())
                persistAudio(false);
            if (appearancePending())
            {
                APP_STATE.setMagicEyeStyle(m_magicEyeStyle);
                APP_STATE.setVuBallistics(m_vuBallistics);
                APP_STATE.setVuSensitivity(m_vuSensitivity);

                // SSE-Event an Web-Clients senden
                extern void sendRadioSSEEvent();
                sendRadioSSEEvent();
            }

            // Eine geänderte Sprache greift erst nach einem Neustart in allen Modulen; performRestart kehrt nicht zurück.
            if (languagePending())
            {
                APP_STATE.setUiLanguage(m_pendingLangIndex == 1 ? "en" : "de");
                m_pendingLangIndex = -1;
                m_restartRequested = true;
            }
        }
        m_exitRequested = true; });

    m_activeMessageBox->start();
    changeState(State::SHOWING_MESSAGE);
}

/**
 * @brief Startet den Bestätigungs-Dialog für den Neustart.
 */
void Settings::startRestartDialog()
{
    char title[40];
    STRF(title, SettingsKey::SETTINGS_INFO_RESTART_TITLE);
    char msg[128];
    STRF(msg, SettingsKey::SETTINGS_INFO_RESTART_MSG);

    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 140, 400, 200);
    m_activeMessageBox->setTitle(title);
    m_activeMessageBox->setMessage(msg);
    m_activeMessageBox->setButtonLabels(
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_INFO_RESTART_BTN)),
        reinterpret_cast<const char *>(I18n::str(CommonKey::COMMON_BTN_CANCEL)));

    m_activeMessageBox->setCallback([this](bool restart)
                                    {
        if (restart)
            m_restartRequested = true; });

    m_activeMessageBox->start();
    changeState(State::SHOWING_MESSAGE);
}

/**
 * @brief Startet den Bestätigungs-Dialog für das Zurücksetzen.
 */
void Settings::startResetDialog()
{
    char title[40];
    STRF(title, SettingsKey::SETTINGS_INFO_RESET_TITLE);
    char msg[160];
    STRF(msg, SettingsKey::SETTINGS_INFO_RESET_MSG);

    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 130, 420, 220);
    m_activeMessageBox->setTitle(title);
    m_activeMessageBox->setMessage(msg);
    m_activeMessageBox->setButtonLabels(
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_INFO_RESET_BTN)),
        reinterpret_cast<const char *>(I18n::str(CommonKey::COMMON_BTN_CANCEL)));

    m_activeMessageBox->setCallback([this](bool confirmed)
                                    {
        if (confirmed)
        {
            APP_STATE.resetToDefaults(true, true);
            m_restartRequested = true;
        } });

    m_activeMessageBox->start();
    changeState(State::SHOWING_MESSAGE);
}

// ===================================================================
// Hilfsdialoge und Dirty-Tracking
// ===================================================================

/**
 * @brief Zeigt eine einfache Hinweis-Box mit OK-Knopf.
 */
void Settings::showInfoBox(SettingsKey titleKey, SettingsKey messageKey)
{
    char msg[160];
    STRF(msg, messageKey);
    showInfoBoxText(titleKey, msg);
}

/**
 * @brief Zeigt eine Hinweis-Box mit vorformatiertem Text.
 */
void Settings::showInfoBoxText(SettingsKey titleKey, const char *message)
{
    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 140, 420, 200);
    m_activeMessageBox->setTitle(reinterpret_cast<const char *>(I18n::str(titleKey)));
    m_activeMessageBox->setMessage(message);
    m_activeMessageBox->setButtonLabels(reinterpret_cast<const char *>(I18n::str(CommonKey::COMMON_BTN_OK)), "");
    m_activeMessageBox->start();
    changeState(State::SHOWING_MESSAGE);
}

/**
 * @brief Speichert den aktuellen Profilzustand als Vergleichsbasis.
 */
void Settings::snapshotBaseline()
{
    for (int i = 0; i < MAX_PROFILES; ++i)
    {
        stringutils::safeCopy(m_baseline[i].ssid, m_profiles[i].ssid);
        stringutils::safeCopy(m_baseline[i].password, m_profiles[i].password);
        m_baseline[i].enabled = m_profiles[i].enabled;
    }
}

/**
 * @brief Berechnet die Dirty-Flags der Profile neu.
 */
void Settings::recomputeDirtyFlags()
{
    bool any = false;
    for (int i = 0; i < MAX_PROFILES; ++i)
    {
        const bool mod =
            (strcmp(m_profiles[i].ssid, m_baseline[i].ssid) != 0) ||
            (strcmp(m_profiles[i].password, m_baseline[i].password) != 0) ||
            (m_profiles[i].enabled != m_baseline[i].enabled);
        m_profiles[i].modified = mod;
        any |= mod;
    }
    // Hinweis: weatherPending() ist absichtlich "true" auch bei unveränderter PLZ (damit der Save-Knopf im Panel auch ohne Eingabewechsel den Orts-Check anstoßen kann). Für die globale Prüfung auf ungespeicherte Änderungen zählt aber nur eine tatsächliche Abweichung vom Ausgangswert.
    const bool weatherDiffersFromBaseline = strcmp(m_plzEntry, m_plzBaseline) != 0;

    m_changesMade = any || pinPending() || languagePending() || newsPending() || weatherDiffersFromBaseline || cloudPending() || timePending() || timeWindowPending() || audioPending() || appearancePending() || hostnamePending();
    m_footerDirty = true;
}

/**
 * @brief Prüft, ob mindestens ein Profil von der Baseline abweicht.
 */
bool Settings::hasProfileChanges() const
{
    for (int i = 0; i < MAX_PROFILES; ++i)
    {
        if (m_profiles[i].modified)
            return true;
    }
    return false;
}

/**
 * @brief Prüft, ob der vorgemerkte Netzwerkname von der Baseline abweicht.
 */
bool Settings::hostnamePending() const
{
    return strcmp(m_hostname, m_hostnameBaseline) != 0;
}

/**
 * @brief Prüft, ob das angegebene Panel ungespeicherte Änderungen hat.
 *
 * Jedes Panel bringt seinen Speichern-Knopf nur dann in die Fußzeile, wenn genau dieses Panel etwas zu speichern hat; Save wirkt nur auf das aktuelle Panel.
 */
bool Settings::panelDirty(Panel p) const
{
    switch (p)
    {
    case Panel::WIFI:
        return hasProfileChanges() || hostnamePending();
    case Panel::LANGUAGE:
        return languagePending();
    case Panel::SECURITY:
        return pinPending();
    case Panel::NEWS:
        return newsPending();
    case Panel::WEATHER:
        return weatherPending();
    case Panel::CLOUD:
        return cloudPending();
    case Panel::TIME:
        return timePending();
    case Panel::TIME_WINDOW:
        return timeWindowPending();
    case Panel::AUDIO:
        return audioPending();
    case Panel::APPEARANCE:
        return appearancePending();
    default:
        return false;
    }
}

/**
 * @brief Prüft, ob ein Profil eine SSID hat.
 */
bool Settings::isProfileValid(int profileIndex) const
{
    if (profileIndex < 0 || profileIndex >= MAX_PROFILES)
        return false;
    return strlen(m_profiles[profileIndex].ssid) > 0;
}

/**
 * @brief Prüft, ob ein Profil ausreichend konfiguriert ist (SSID genügt).
 */
bool Settings::isProfileConfigured(int profileIndex) const
{
    return isProfileValid(profileIndex);
}

// ===================================================================
// News-Panel
// ===================================================================

/**
 * @brief Liefert das Rechteck eines Ressort-Chips (sprite-relativ).
 *
 * Chips liegen in zwei Reihen á NEWS_CHIPS_PER_ROW Spalten, die Breite teilt sich gleichmäßig auf die verfügbare Inhaltsbreite auf.
 */
Settings::Rect Settings::newsRessortChipRect(int idx) const
{
    constexpr int chipW =
        (CONTENT_W - (NEWS_CHIPS_PER_ROW - 1) * NEWS_CHIP_GAP_X) / NEWS_CHIPS_PER_ROW;
    constexpr int yBase = CONTENT_Y + 30;

    const int row = idx / NEWS_CHIPS_PER_ROW;
    const int col = idx % NEWS_CHIPS_PER_ROW;
    const int x = CONTENT_X + col * (chipW + NEWS_CHIP_GAP_X);
    const int y = yBase + row * (NEWS_CHIP_H + NEWS_CHIP_GAP_Y);
    return Rect{x, y, chipW, NEWS_CHIP_H};
}

/**
 * @brief Liefert das Rechteck des Bundesland-Wahlknopfes (sprite-relativ).
 */
Settings::Rect Settings::newsRegionSelectorRect() const
{
    constexpr int chipsBottom =
        CONTENT_Y + 30 + 2 * NEWS_CHIP_H + NEWS_CHIP_GAP_Y;
    constexpr int labelGap = 20;
    constexpr int labelToBtn = 26;
    constexpr int y = chipsBottom + labelGap + labelToBtn;
    return Rect{CONTENT_X, y, CONTENT_W, NEWS_SELECTOR_H};
}

/**
 * @brief Zeichnet das Nachrichten-Panel: Themen-Chips und Bundesland-Wahl.
 */
void Settings::drawPanelNews(Gfx_Sprite *spr)
{
    if (!spr)
        return;

    constexpr int labelLeft = CONTENT_X + 2;
    constexpr int chipsBottom =
        CONTENT_Y + 30 + 2 * NEWS_CHIP_H + NEWS_CHIP_GAP_Y;
    constexpr int regionLabelY = chipsBottom + 20;

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(C_SUB);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_NEWS_RESSORT_LABEL),
                    labelLeft, CONTENT_Y + 4);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_NEWS_REGION_LABEL),
                    labelLeft, regionLabelY);
    spr->unloadFont();

    // Ressort-Chips
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(MC_DATUM);
    for (int i = 0; i < NEWS_RESSORT_COUNT; ++i)
    {
        const Rect r = newsRessortChipRect(i);
        const bool active = strcmp(m_newsRessort, NEWS_RESSORTS[i].value) == 0;
        const uint16_t fill = active ? C_SEC_SERVICES : BTN_FILL;
        const uint16_t txt = active ? BTN_ACTIVE_TEXT : BTN_TEXT;

        if (active)
        {
            spr->fillSmoothRoundRect(r.x, r.y, r.w, r.h, 6, fill);
        }
        else
        {
            spr->fillSmoothRoundRect(r.x, r.y, r.w, r.h, 6, BTN_OUTLINE);
            spr->fillSmoothRoundRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, 5, fill);
        }
        spr->setTextColor(txt);
        spr->drawString(I18n::str(NEWS_RESSORTS[i].labelKey),
                        r.x + r.w / 2, r.y + r.h / 2);
    }
    spr->unloadFont();

    // Bundesland-Wahlknopf mit aktuellem Namen und Caret rechts
    const Rect sel = newsRegionSelectorRect();
    spr->fillSmoothRoundRect(sel.x, sel.y, sel.w, sel.h, 6, BTN_OUTLINE);
    spr->fillSmoothRoundRect(sel.x + 1, sel.y + 1, sel.w - 2, sel.h - 2, 5, BTN_FILL);

    char regionLabel[40];
    STRF(regionLabel, newsRegionKey(m_newsRegion));

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_TEXT);
    spr->drawString(regionLabel, sel.x + 18, sel.y + sel.h / 2);

    spr->setTextDatum(MR_DATUM);
    spr->setTextColor(C_SUB);
    spr->drawString(">", sel.x + sel.w - 18, sel.y + sel.h / 2);
    spr->unloadFont();
}

/**
 * @brief Verarbeitet Taps im Nachrichten-Panel.
 */
void Settings::handleNewsTouch(int sx, int sy)
{
    // Themen-Chips
    for (int i = 0; i < NEWS_RESSORT_COUNT; ++i)
    {
        const Rect r = newsRessortChipRect(i);
        if (pointIn(sx, sy, r.x, r.y, r.w, r.h))
        {
            if (strcmp(m_newsRessort, NEWS_RESSORTS[i].value) != 0)
            {
                stringutils::safeCopy(m_newsRessort, NEWS_RESSORTS[i].value);
                recomputeDirtyFlags();
                m_workDirty = true;
                m_footerDirty = true;
            }
            return;
        }
    }

    // Bundesland-Wahlknopf
    const Rect sel = newsRegionSelectorRect();
    if (pointIn(sx, sy, sel.x, sel.y, sel.w, sel.h))
    {
        const int chosen = runListPicker(
            SettingsKey::SETTINGS_NEWS_REGION_PICK_TITLE,
            16,
            [](int idx, char *buf, size_t bufSize)
            {
                I18n::format(buf, bufSize, newsRegionKey(idx + 1));
            },
            m_newsRegion - 1);

        if (chosen >= 0 && chosen + 1 != m_newsRegion)
        {
            m_newsRegion = chosen + 1;
            recomputeDirtyFlags();
        }
        afterModalRedraw();
        return;
    }
}

/**
 * @brief Prüft, ob das Nachrichten-Panel ungespeicherte Änderungen hat.
 */
bool Settings::newsPending() const
{
    return strcmp(m_newsRessort, m_newsRessortBaseline) != 0 || m_newsRegion != m_newsRegionBaseline;
}

/**
 * @brief Schreibt die vorgemerkten Nachrichten-Filter nach AppState.
 * @param showMessage true blendet die Bestätigung ein, false schreibt still
 *
 * Still schreiben muss, wer aus dem Rückruf des Verlassen-Dialogs kommt: Dort läuft noch die Meldungsbox, die den Rückruf ausgelöst hat, und eine zweite würde sie unter sich selbst wegräumen.
 */
void Settings::persistNews(bool showMessage)
{
    APP_STATE.setNewsRessort(m_newsRessort);
    APP_STATE.setNewsRegion(m_newsRegion);

    stringutils::safeCopy(m_newsRessortBaseline, m_newsRessort);
    m_newsRegionBaseline = m_newsRegion;

    recomputeDirtyFlags();
    m_workDirty = true;

    if (showMessage)
        showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, SettingsKey::SETTINGS_NEWS_SAVED_MSG);
    LOG_SUCCESS("Settings: News-Filter gespeichert");
}

/**
 * @brief Speichert die Nachrichten-Filter mit Hinweis.
 */
void Settings::saveNews()
{
    persistNews(true);
}

// ===================================================================
// Wetter-Panel
// ===================================================================

/**
 * @brief Zeichnet das Wetter-Panel: PLZ-Eingabe, Status, Nummernblock.
 */
void Settings::drawPanelWeather(Gfx_Sprite *spr)
{
    if (!spr)
        return;

    if (!m_numpadLayoutValid)
    {
        m_numpad.computeLayout(CONTENT_W, CONTENT_H - WorkshopNumpad::REQUIRED_H);
        m_numpadLayoutValid = true;
    }

    constexpr int labelY = CONTENT_Y + 12;
    constexpr int boxesY = CONTENT_Y + 28;
    constexpr int totalBoxW = PLZ_DIGITS * PLZ_BOX_W + (PLZ_DIGITS - 1) * PLZ_BOX_GAP;
    constexpr int boxesX = CONTENT_X + (CONTENT_W - totalBoxW) / 2;
    constexpr int statusY = boxesY + PLZ_BOX_H + 14;
    constexpr int statusLineH = 26;
    constexpr int statusLabelX = CONTENT_X + 16;
    constexpr int statusValueX = CONTENT_X + 152;

    // Label "Postleitzahl" (zentriert oberhalb der Eingabe-Boxen)
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_SUB);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_WEATHER_PLZ_LABEL),
                    CONTENT_X + CONTENT_W / 2, labelY);
    spr->unloadFont();

    // Fünf Ziffer-Boxen mit Rahmen und Innenfüllung
    for (int i = 0; i < PLZ_DIGITS; ++i)
    {
        const int bx = boxesX + i * (PLZ_BOX_W + PLZ_BOX_GAP);
        spr->fillSmoothRoundRect(bx, boxesY, PLZ_BOX_W, PLZ_BOX_H, 8, BTN_OUTLINE);

        const bool active = (i == m_plzLen && m_plzLen < PLZ_DIGITS);
        const uint16_t inner = active ? C_ROW_SEL_BG : C_ROW_BG;
        spr->fillSmoothRoundRect(bx + 2, boxesY + 2,
                                 PLZ_BOX_W - 4, PLZ_BOX_H - 4, 6, inner);
    }

    // Ziffern in die ausgefüllten Boxen schreiben
    spr->loadFont(ComfortaaBold30);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_TEXT_BRIGHT);
    for (int i = 0; i < m_plzLen; ++i)
    {
        const int bx = boxesX + i * (PLZ_BOX_W + PLZ_BOX_GAP);
        char dig[2] = {m_plzEntry[i], '\0'};
        spr->drawString(dig, bx + PLZ_BOX_W / 2, boxesY + PLZ_BOX_H / 2);
    }
    spr->unloadFont();

    // Status: Ort und Koordinaten zeigen den zuletzt gespeicherten Stand. Solange die Eingabe oben nicht zur gespeicherten PLZ passt (leer, unvollständig oder abweichend), werden die Werte ausgegraut, damit der Anwender sieht: was hier steht, gehört nicht zur aktuell vorbereiteten Eingabe.
    const bool entryMatchesSaved = (strcmp(m_plzEntry, m_plzBaseline) == 0) && (m_plzBaseline[0] != '\0');
    const uint16_t valueColor = entryMatchesSaved ? C_TEXT : C_SUB;

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(TL_DATUM);

    spr->setTextColor(C_SUB);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_WEATHER_LOCATION_LABEL),
                    statusLabelX, statusY);

    char locBuf[64] = {0};
    APP_STATE.getWeatherLocation(locBuf, sizeof(locBuf));
    if (APP_STATE.hasWeatherLocation() && locBuf[0] != '\0')
    {
        spr->setTextColor(valueColor);
        spr->drawString(locBuf, statusValueX, statusY);
    }
    else
    {
        spr->setTextColor(C_SUB);
        spr->drawString(I18n::str(SettingsKey::SETTINGS_WEATHER_NOT_RESOLVED),
                        statusValueX, statusY);
    }

    const int coordY = statusY + statusLineH;
    spr->setTextColor(C_SUB);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_WEATHER_COORDS_LABEL),
                    statusLabelX, coordY);

    if (APP_STATE.hasWeatherCoordinates())
    {
        char coordsBuf[40];
        stringutils::safeFormat(coordsBuf, "%.4f, %.4f",
                                APP_STATE.getWeatherLatitude(),
                                APP_STATE.getWeatherLongitude());
        spr->setTextColor(valueColor);
        spr->drawString(coordsBuf, statusValueX, coordY);
    }
    else
    {
        spr->setTextColor(C_SUB);
        spr->drawString(I18n::str(SettingsKey::SETTINGS_WEATHER_NOT_RESOLVED),
                        statusValueX, coordY);
    }

    // Dezenter Hinweis nur bei fehlender WLAN-Verbindung
    if (!APP_STATE.getWifiConnected())
    {
        const int hintY = coordY + statusLineH + 6;
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_SUB);
        spr->drawString(I18n::str(SettingsKey::SETTINGS_WEATHER_NO_WIFI_HINT),
                        CONTENT_X + CONTENT_W / 2, hintY);
    }

    spr->unloadFont();

    // Nummernblock am unteren Rand
    m_numpad.draw(spr, CONTENT_X, CONTENT_Y, -1);
}

/**
 * @brief Verarbeitet Taps im Wetter-Panel.
 */
void Settings::handleWeatherTouch(int sx, int sy)
{
    const int cx = sx - CONTENT_X;
    const int cy = sy - CONTENT_Y;
    const int key = m_numpad.hitTest(cx, cy);
    if (key < 0)
        return;

    handlePlzKey(WorkshopNumpad::keyAction(key));
    if (m_state == State::IDLE)
        m_workDirty = true;
}

/**
 * @brief Verarbeitet einen Nummernblock-Tastendruck im Wetter-Panel.
 *
 * Ziffern werden angehängt (max. fünf), Backspace entfernt die letzte Stelle, Clear leert die Eingabe. Dezimalpunkt und Vorzeichen werden ignoriert, da eine Postleitzahl nur aus Ziffern besteht.
 */
void Settings::handlePlzKey(char action)
{
    bool changed = false;

    if (action >= '0' && action <= '9')
    {
        if (m_plzLen < PLZ_DIGITS)
        {
            m_plzEntry[m_plzLen] = action;
            m_plzLen++;
            m_plzEntry[m_plzLen] = '\0';
            changed = true;
        }
    }
    else if (action == 'B')
    {
        if (m_plzLen > 0)
        {
            m_plzLen--;
            m_plzEntry[m_plzLen] = '\0';
            changed = true;
        }
    }
    else if (action == 'C')
    {
        if (m_plzLen > 0)
        {
            m_plzEntry[0] = '\0';
            m_plzLen = 0;
            changed = true;
        }
    }

    if (changed)
        recomputeDirtyFlags();
}

/**
 * @brief Prüft, ob eine speicherreife Postleitzahl vorgemerkt ist.
 *
 * Speicherreif ist jede vollständige Eingabe aus genau fünf Ziffern, unabhängig davon, ob sie vom zuletzt gespeicherten Wert abweicht. Dadurch kann derselbe Wert erneut bestätigt werden, etwa um die Orts-Auflösung manuell anzustoßen, wenn beim letzten Versuch kein WLAN bestand oder der Ort verloren ging.
 */
bool Settings::weatherPending() const
{
    return m_plzLen == PLZ_DIGITS;
}

/**
 * @brief Speichert die vorgemerkte Postleitzahl und ermittelt den zugehörigen Ort.
 *
 * Der Speichervorgang läuft in drei Phasen ab: die Postleitzahl wird zuerst nach AppState geschrieben, dann blockierend über die Nominatim-API in einen Ort und Koordinaten aufgelöst, und schließlich erscheint eine Bestätigungs-Box mit dem Ergebnis. Während der Auflösung sieht der Anwender eine Wartebox.
 *
 * Schlägt die Auflösung fehl (kein WLAN, Nominatim antwortet nicht, PLZ unbekannt), bleibt die Postleitzahl gespeichert, Ort und Koordinaten werden geleert; das Wetter-Modul wird bei der nächsten Gelegenheit selbst eine neue Auflösung versuchen.
 */
void Settings::saveWeather()
{
    persistPlz();

    // Wartebox einblenden
    char title[40];
    STRF(title, SettingsKey::SETTINGS_WEATHER_TITLE);
    char waitMsg[64];
    STRF(waitMsg, SettingsKey::SETTINGS_WEATHER_RESOLVING);

    m_activeProgressBox.reset(new ProgressBox(*m_tft));
    m_activeProgressBox->setAutoCenter(true);
    m_activeProgressBox->setTitle(title);
    m_activeProgressBox->start(waitMsg, 0);
    delay(50);

    // Auflösung anstoßen
    char resolvedCity[GEOCODE_CITY_BUF] = {0};
    double resolvedLat = 0.0;
    double resolvedLon = 0.0;
    int resolvedRegion = 0;

    const PlzLookupStatus rc = resolvePlz(m_plzEntry,
                                          resolvedCity, sizeof(resolvedCity),
                                          resolvedLat, resolvedLon, resolvedRegion);

    if (m_activeProgressBox)
    {
        m_activeProgressBox->finish();
        m_activeProgressBox.reset();
    }

    recomputeDirtyFlags();
    m_workDirty = true;

    if (rc == PlzLookupStatus::SUCCESS)
    {
        APP_STATE.setWeatherLocation(resolvedCity);
        APP_STATE.setWeatherLatitude(resolvedLat);
        APP_STATE.setWeatherLongitude(resolvedLon);

        // Nachrichten-Region nachziehen, wenn ein Bundesland erkannt wurde
        if (resolvedRegion >= 1 && resolvedRegion <= 16)
        {
            APP_STATE.setNewsRegion(resolvedRegion);
            m_newsRegion = resolvedRegion;
            m_newsRegionBaseline = resolvedRegion;
        }

        char msg[96];
        STRF(msg, SettingsKey::SETTINGS_WEATHER_SAVED_FMT, resolvedCity);
        showInfoBoxText(SettingsKey::SETTINGS_SAVED_TITLE, msg);

        LOG_SUCCESS_F("Settings: PLZ %s -> %s (%.5f, %.5f), Region %d",
                      m_plzEntry, resolvedCity, resolvedLat, resolvedLon, resolvedRegion);
        return;
    }

    // Fehlerpfad: PLZ ist gespeichert, Ort wird geleert damit das Wetter-Modul bei der nächsten Auflösungs-Gelegenheit nicht den alten Ort weiterzeigt
    APP_STATE.setWeatherLocation("");

    SettingsKey errorKey;
    switch (rc)
    {
    case PlzLookupStatus::NO_WIFI:
        errorKey = SettingsKey::SETTINGS_WEATHER_NO_WIFI;
        break;
    case PlzLookupStatus::NOT_FOUND:
        errorKey = SettingsKey::SETTINGS_WEATHER_NOT_FOUND;
        break;
    case PlzLookupStatus::OUTSIDE_DE:
        errorKey = SettingsKey::SETTINGS_WEATHER_OUTSIDE_DE;
        break;
    default:
        errorKey = SettingsKey::SETTINGS_WEATHER_LOOKUP_FAILED;
        break;
    }

    showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, errorKey);
    LOG_WARNING_F("Settings: PLZ %s konnte nicht aufgelöst werden (Status %d)",
                  m_plzEntry, static_cast<int>(rc));
}

/**
 * @brief Löst eine 5-stellige PLZ über Nominatim auf und parst das erste Ergebnis.
 *
 * Die Anfrage entspricht inhaltlich dem, was das Webinterface im Browser tut: Aufruf von `https://nominatim.openstreetmap.org/search?postalcode=...&countrycodes=DE` mit `format=json`, `limit=5` und `addressdetails=1`, anschließend wird der erste Treffer geprüft (Deutschland-Bounding-Box, Ortsname, Bundesland-Mapping).
 */
Settings::PlzLookupStatus Settings::resolvePlz(const char *plz,
                                               char *outCity, size_t outCitySize,
                                               double &outLat, double &outLon,
                                               int &outRegion)
{
    if (outCity && outCitySize > 0)
        outCity[0] = '\0';
    outLat = 0.0;
    outLon = 0.0;
    outRegion = 0;

    if (!plz || strlen(plz) != PLZ_DIGITS)
        return PlzLookupStatus::REQUEST_FAILED;

    if (!APP_STATE.getWifiConnected())
        return PlzLookupStatus::NO_WIFI;

    if (!HTTP_SERVICE.isInitialized())
        return PlzLookupStatus::REQUEST_FAILED;

    char url[GEOCODE_URL_BUF];
    stringutils::safeFormat(url,
                            "https://nominatim.openstreetmap.org/search"
                            "?postalcode=%s&countrycodes=DE&format=json&limit=5&addressdetails=1",
                            plz);

    LOG_INFO_F("Settings: PLZ-Aufloesung %s", url);

    HttpResponse response = HTTP_SERVICE.get(url);
    if (!response.isSuccess() || !response.hasData())
    {
        LOG_WARNING_F("Settings: Nominatim HTTP %d", response.httpCode);
        return PlzLookupStatus::REQUEST_FAILED;
    }

    // Nominatim liefert ein Array mit Treffern auf der obersten Ebene
    JsonDocument filter;
    filter[0]["lat"] = true;
    filter[0]["lon"] = true;
    filter[0]["display_name"] = true;
    filter[0]["address"]["city"] = true;
    filter[0]["address"]["town"] = true;
    filter[0]["address"]["village"] = true;
    filter[0]["address"]["municipality"] = true;
    filter[0]["address"]["state"] = true;

    JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
    const DeserializationError err = deserializeJson(doc, response.data,
                                                     DeserializationOption::Filter(filter));
    if (err)
    {
        LOG_WARNING_F("Settings: Nominatim JSON-Fehler: %s", err.c_str());
        return PlzLookupStatus::REQUEST_FAILED;
    }

    if (!doc.is<JsonArray>())
        return PlzLookupStatus::REQUEST_FAILED;

    JsonArray results = doc.as<JsonArray>();
    if (results.size() == 0)
        return PlzLookupStatus::NOT_FOUND;

    JsonObject hit = results[0];
    const char *latStr = hit["lat"] | "";
    const char *lonStr = hit["lon"] | "";
    if (!*latStr || !*lonStr)
        return PlzLookupStatus::REQUEST_FAILED;

    const double lat = atof(latStr);
    const double lon = atof(lonStr);

    // Deutschland-Begrenzungsrahmen für die Plausibilitätsprüfung
    if (lat < DE_BBOX_LAT_MIN || lat > DE_BBOX_LAT_MAX || lon < DE_BBOX_LON_MIN || lon > DE_BBOX_LON_MAX)
    {
        return PlzLookupStatus::OUTSIDE_DE;
    }

    // Ortsnamen über die abgestufte Ersatzwertfolge ermitteln
    JsonObject addr = hit["address"];
    const char *cityName = nullptr;
    if (!addr.isNull())
    {
        const char *city = addr["city"] | "";
        const char *town = addr["town"] | "";
        const char *village = addr["village"] | "";
        const char *municipality = addr["municipality"] | "";
        if (*city)
            cityName = city;
        else if (*town)
            cityName = town;
        else if (*village)
            cityName = village;
        else if (*municipality)
            cityName = municipality;
    }

    // Letzter Fallback: erstes Komma-Segment aus display_name
    char fallbackCity[GEOCODE_CITY_BUF] = {0};
    if (!cityName)
    {
        const char *displayName = hit["display_name"] | "";
        const char *comma = strchr(displayName, ',');
        size_t copyLen = comma ? static_cast<size_t>(comma - displayName) : strlen(displayName);
        if (copyLen >= sizeof(fallbackCity))
            copyLen = sizeof(fallbackCity) - 1;
        memcpy(fallbackCity, displayName, copyLen);
        fallbackCity[copyLen] = '\0';
        cityName = fallbackCity;
    }

    if (!*cityName)
        return PlzLookupStatus::NOT_FOUND;

    if (outCity && outCitySize > 0)
        stringutils::safeCopy(outCity, outCitySize, cityName);
    outLat = lat;
    outLon = lon;

    const char *stateName = addr.isNull() ? "" : (addr["state"] | "");
    outRegion = stateNameToRegion(stateName);

    return PlzLookupStatus::SUCCESS;
}

// ===================================================================
// Cloud-Panel
// ===================================================================

/**
 * @brief Geometrie einer Cloud-Zeile (sprite-relativ).
 */
Settings::Rect Settings::cloudRowRect(int idx) const
{
    const int y = TITLE_BOX_Y + TITLE_BOX_H + CLOUD_TOP_GAP + idx * (CLOUD_ROW_H + CLOUD_ROW_GAP);
    return Rect{CONTENT_X, y, CONTENT_W, CLOUD_ROW_H};
}

/**
 * @brief Geometrie des zentralen Sichtbarkeits-Knopfes im Cloud-Panel.
 */
Settings::Rect Settings::cloudVisibilityButtonRect() const
{
    const int x = TITLE_BOX_X + TITLE_BOX_W - CLOUD_VIS_BTN_MARGIN - CLOUD_VIS_BTN_SIZE;
    const int y = TITLE_BOX_Y + (TITLE_BOX_H - CLOUD_VIS_BTN_SIZE) / 2;
    return Rect{x, y, CLOUD_VIS_BTN_SIZE, CLOUD_VIS_BTN_SIZE};
}

namespace
{
    /**
     * @brief Kürzt eine lange URL für die Anzeige in eine Zeile mit Auslassungszeichen.
     *
     * Liegt die Breite des Originaltextes innerhalb des verfügbaren Platzes, wird er unverändert übernommen. Sonst wird in der Mitte gekürzt, sodass der Aufbau und das Endsegment (typischerweise die ID am Ende der Google-Apps-Script-URL) sichtbar bleiben.
     */
    void formatCloudUrlForDisplay(Gfx_Sprite *spr, const char *src,
                                  int maxWidth, char *out, size_t outSize)
    {
        if (!out || outSize == 0)
            return;
        out[0] = '\0';

        if (!src || !*src)
            return;

        const int srcLen = static_cast<int>(strlen(src));
        if (spr->textWidth(src) <= maxWidth)
        {
            stringutils::safeCopy(out, outSize, src);
            return;
        }

        const char ellipsis[] = "...";
        const int ellipsisW = spr->textWidth(ellipsis);
        if (ellipsisW >= maxWidth)
        {
            stringutils::safeCopy(out, outSize, ellipsis);
            return;
        }

        // Wir bauen den Anzeigetext aus head + "..." + tail. head und tail werden schrittweise verkleinert, bis die Gesamtbreite passt.
        int headLen = srcLen / 2;
        int tailLen = srcLen - headLen;
        char buf[80];

        while (headLen > 0 || tailLen > 0)
        {
            if (static_cast<size_t>(headLen + 3 + tailLen + 1) > sizeof(buf))
            {
                if (headLen > 0)
                    --headLen;
                if (tailLen > 0)
                    --tailLen;
                continue;
            }
            memcpy(buf, src, headLen);
            memcpy(buf + headLen, ellipsis, 3);
            memcpy(buf + headLen + 3, src + srcLen - tailLen, tailLen);
            buf[headLen + 3 + tailLen] = '\0';

            if (spr->textWidth(buf) <= maxWidth)
            {
                stringutils::safeCopy(out, outSize, buf);
                return;
            }
            // Abwechselnd am Ende und am Anfang weiter wegnehmen, damit der Endteil (die ID am URL-Ende) länger sichtbar bleibt als der Anfang.
            if (headLen >= tailLen && headLen > 0)
                --headLen;
            else if (tailLen > 0)
                --tailLen;
            else
                --headLen;
        }

        stringutils::safeCopy(out, outSize, ellipsis);
    }

    /**
     * @brief Schreibt eine verdeckte Wertanzeige.
     */
    void formatCloudHiddenValue(char *out, size_t outSize)
    {
        stringutils::safeCopy(out, outSize, "********");
    }
} // namespace

/**
 * @brief Zeichnet das Cloud-Panel mit Apps-Script-Adressen und OpenSky-Zugangsdaten.
 */
void Settings::drawPanelCloud(Gfx_Sprite *spr)
{
    if (!spr)
        return;

    struct RowInfo
    {
        SettingsKey labelKey;
        const char *value;
        const char *baseline;
    };

    const RowInfo rows[CLOUD_ROW_COUNT] = {
        {SettingsKey::SETTINGS_CLOUD_CALENDAR_LABEL, m_cloudCalendarUrl, m_cloudCalendarBaseline},
        {SettingsKey::SETTINGS_CLOUD_TODO_LABEL, m_cloudTodoUrl, m_cloudTodoBaseline},
        {SettingsKey::SETTINGS_CLOUD_CONTACTS_LABEL, m_cloudContactsUrl, m_cloudContactsBaseline},
        {SettingsKey::SETTINGS_CLOUD_OPENSKY_ID_LABEL, m_cloudOpenSkyClientId, m_cloudOpenSkyClientIdBaseline},
        {SettingsKey::SETTINGS_CLOUD_OPENSKY_SECRET_LABEL, m_cloudOpenSkyClientSecret, m_cloudOpenSkyClientSecretBaseline},
    };

    m_cloudVisibilityButton.setLabelText(m_cloudValuesVisible ? "^EYEON:20^" : "^EYEOFF:20^");
    m_cloudVisibilityButton.drawButton(false);

    for (int i = 0; i < CLOUD_ROW_COUNT; ++i)
    {
        const Rect r = cloudRowRect(i);

        // Zeilen-Hintergrund mit dezentem Rahmen
        spr->fillSmoothRoundRect(r.x, r.y, r.w, r.h, 8, BTN_OUTLINE);
        spr->fillSmoothRoundRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, 7, C_ROW_BG);

        // Zeile gilt als ungespeichert, wenn ihr Wert von der Baseline abweicht
        const bool dirty = strcmp(rows[i].value, rows[i].baseline) != 0;
        if (dirty)
            spr->fillRect(r.x + 1, r.y + 8, ITEM_ACCENT_W, r.h - 16, C_SEC_SERVICES);

        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(TL_DATUM);
        spr->setTextColor(C_SEC_CONNECTION);
        spr->drawString(I18n::str(rows[i].labelKey),
                        r.x + CLOUD_ROW_PAD_X, r.y + CLOUD_ROW_LABEL_Y);

        const int valueX = r.x + CLOUD_ROW_PAD_X;
        const int valueMaxW = r.w - 2 * CLOUD_ROW_PAD_X;
        if (rows[i].value[0] != '\0')
        {
            char display[96];
            if (m_cloudValuesVisible)
                formatCloudUrlForDisplay(spr, rows[i].value, valueMaxW, display, sizeof(display));
            else
                formatCloudHiddenValue(display, sizeof(display));
            spr->setTextColor(C_TEXT);
            spr->drawString(display, valueX, r.y + CLOUD_ROW_VALUE_Y);
        }
        else
        {
            spr->setTextColor(C_SUB);
            spr->drawString(I18n::str(SettingsKey::SETTINGS_CLOUD_NOT_SET),
                            valueX, r.y + CLOUD_ROW_VALUE_Y);
        }
        spr->unloadFont();
    }

}

/**
 * @brief Verarbeitet Taps im Cloud-Panel.
 */
void Settings::handleCloudTouch(int sx, int sy)
{
    const Rect visibility = cloudVisibilityButtonRect();
    if (pointIn(sx, sy, visibility.x, visibility.y, visibility.w, visibility.h))
    {
        m_cloudValuesVisible = !m_cloudValuesVisible;
        m_workDirty = true;
        return;
    }

    for (int i = 0; i < CLOUD_ROW_COUNT; ++i)
    {
        const Rect r = cloudRowRect(i);
        if (pointIn(sx, sy, r.x, r.y, r.w, r.h))
        {
            startCloudUrlInput(i);
            return;
        }
    }

}

/**
 * @brief Öffnet die Tastatur für einen Cloud-Wert.
 *
 * Die Eingabe erfolgt im Klartext, damit die Werte direkt bearbeitet werden können.
 */
void Settings::startCloudUrlInput(int slot)
{
    if (slot < 0 || slot >= CLOUD_ROW_COUNT || !m_tft)
        return;

    SettingsKey promptKey;
    char *target;
    size_t targetSize;
    switch (slot)
    {
    case 0:
        promptKey = SettingsKey::SETTINGS_CLOUD_CALENDAR_PROMPT;
        target = m_cloudCalendarUrl;
        targetSize = sizeof(m_cloudCalendarUrl);
        break;
    case 1:
        promptKey = SettingsKey::SETTINGS_CLOUD_TODO_PROMPT;
        target = m_cloudTodoUrl;
        targetSize = sizeof(m_cloudTodoUrl);
        break;
    case 2:
        promptKey = SettingsKey::SETTINGS_CLOUD_CONTACTS_PROMPT;
        target = m_cloudContactsUrl;
        targetSize = sizeof(m_cloudContactsUrl);
        break;
    case 3:
        promptKey = SettingsKey::SETTINGS_CLOUD_OPENSKY_ID_PROMPT;
        target = m_cloudOpenSkyClientId;
        targetSize = sizeof(m_cloudOpenSkyClientId);
        break;
    case 4:
        promptKey = SettingsKey::SETTINGS_CLOUD_OPENSKY_SECRET_PROMPT;
        target = m_cloudOpenSkyClientSecret;
        targetSize = sizeof(m_cloudOpenSkyClientSecret);
        break;
    default:
        return;
    }

    Keyboard keyboard(m_tft);
    keyboard.setPasswordMode(false);

    char label[40];
    STRF(label, promptKey);

    char buffer[256] = {0};
    stringutils::safeCopy(buffer, target);

    const bool ok = keyboard.show(label, buffer, targetSize < sizeof(buffer) ? targetSize : sizeof(buffer), buffer);

    if (ok)
    {
        stringutils::safeCopy(target, targetSize, buffer);
        recomputeDirtyFlags();
    }

    afterModalRedraw();
}

/**
 * @brief Startet den Bestätigungs-Dialog für die Lösch-Aktion.
 *
 * Wird der Dialog bestätigt, werden alle Werte in der vorgemerkten Eingabe geleert. Erst beim nächsten Speichern landet die Löschung in AppState (und ruft dann `resetPreferencesGoogleApi()` auf).
 */
void Settings::startCloudClearDialog()
{
    if (!m_tft)
        return;

    char title[40];
    STRF(title, SettingsKey::SETTINGS_CLOUD_CLEAR_TITLE);
    char msg[140];
    STRF(msg, SettingsKey::SETTINGS_CLOUD_CLEAR_MSG);

    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 140, 400, 200);
    m_activeMessageBox->setTitle(title);
    m_activeMessageBox->setMessage(msg);
    m_activeMessageBox->setButtonLabels(
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_CLOUD_CLEAR_BTN)),
        reinterpret_cast<const char *>(I18n::str(CommonKey::COMMON_BTN_CANCEL)));

    m_activeMessageBox->setCallback([this](bool confirmed)
                                    {
        if (confirmed)
        {
            m_cloudCalendarUrl[0] = '\0';
            m_cloudTodoUrl[0]     = '\0';
            m_cloudContactsUrl[0] = '\0';
            m_cloudOpenSkyClientId[0] = '\0';
            m_cloudOpenSkyClientSecret[0] = '\0';
            recomputeDirtyFlags();
            m_workDirty = true;
            LOG_INFO("Settings: Cloud-Anbindung zum Löschen vorgemerkt");
        } });

    m_activeMessageBox->start();
    changeState(State::SHOWING_MESSAGE);
}

/**
 * @brief Prüft, ob mindestens ein Cloud-Wert abweicht.
 */
bool Settings::cloudPending() const
{
    return strcmp(m_cloudCalendarUrl, m_cloudCalendarBaseline) != 0 ||
           strcmp(m_cloudTodoUrl, m_cloudTodoBaseline) != 0 ||
           strcmp(m_cloudContactsUrl, m_cloudContactsBaseline) != 0 ||
           strcmp(m_cloudOpenSkyClientId, m_cloudOpenSkyClientIdBaseline) != 0 ||
           strcmp(m_cloudOpenSkyClientSecret, m_cloudOpenSkyClientSecretBaseline) != 0;
}

/**
 * @brief Prüft eine Cloud-Adresse auf das erwartete Schema.
 */
Settings::CloudUrlIssue Settings::validateCloudUrl(const char *url) const
{
    if (!url || url[0] == '\0')
        return CloudUrlIssue::NONE;

    constexpr char PREFIX[] = "https://script.google.com/";
    constexpr size_t PREFIX_LEN = sizeof(PREFIX) - 1;

    if (strncmp(url, PREFIX, PREFIX_LEN) != 0)
        return CloudUrlIssue::PREFIX_MISSING;

    const size_t urlLen = strlen(url);
    constexpr char SUFFIX[] = "/exec";
    constexpr size_t SUFFIX_LEN = sizeof(SUFFIX) - 1;

    if (urlLen < SUFFIX_LEN || strcmp(url + urlLen - SUFFIX_LEN, SUFFIX) != 0)
        return CloudUrlIssue::SUFFIX_MISSING;

    return CloudUrlIssue::NONE;
}

/**
 * @brief Schreibt allein die vorgemerkte Postleitzahl nach AppState.
 *
 * Die Auflösung in Ort und Koordinaten bleibt hier aussen vor, weil sie blockierend über das Netz läuft und eine eigene Box zeigt. Aus dem Verlassen-Dialog heraus ist beides nicht möglich; das Wetter-Modul holt die Auflösung bei nächster Gelegenheit selbst nach.
 */
void Settings::persistPlz()
{
    APP_STATE.setPlz(m_plzEntry);
    stringutils::safeCopy(m_plzBaseline, m_plzEntry);
}

/**
 * @brief Schreibt die vorgemerkte Cloud-Anbindung nach AppState.
 *
 * Vor dem Speichern werden die Apps-Script-Adressen auf das erwartete Format geprüft. Hat mindestens eine ein Problem, erscheint statt der Speicher-Bestätigung eine Sammel-Fehlermeldung, die alle problematischen Adressen mit ihrem jeweiligen Grund nennt; gespeichert wird in diesem Fall nichts. Sind alle Werte leer, wird `resetPreferencesGoogleApi()` aufgerufen, um die zugehörigen Preferences auch in den Defaultzustand zu bringen.
 */
bool Settings::persistCloud(bool showMessage)
{
    struct RowInfo
    {
        SettingsKey labelKey;
        const char *url;
        bool validateAsScriptUrl;
    };

    const RowInfo rows[CLOUD_ROW_COUNT] = {
        {SettingsKey::SETTINGS_CLOUD_CALENDAR_LABEL, m_cloudCalendarUrl, true},
        {SettingsKey::SETTINGS_CLOUD_TODO_LABEL, m_cloudTodoUrl, true},
        {SettingsKey::SETTINGS_CLOUD_CONTACTS_LABEL, m_cloudContactsUrl, true},
        {SettingsKey::SETTINGS_CLOUD_OPENSKY_ID_LABEL, m_cloudOpenSkyClientId, false},
        {SettingsKey::SETTINGS_CLOUD_OPENSKY_SECRET_LABEL, m_cloudOpenSkyClientSecret, false},
    };

    // Schritt 1: alle drei prüfen, problematische Zeilen sammeln
    char errorMsg[320];
    errorMsg[0] = '\0';
    bool anyIssue = false;

    for (int i = 0; i < CLOUD_ROW_COUNT; ++i)
    {
        if (!rows[i].validateAsScriptUrl)
            continue;

        const CloudUrlIssue issue = validateCloudUrl(rows[i].url);
        if (issue == CloudUrlIssue::NONE)
            continue;

        SettingsKey reasonKey = (issue == CloudUrlIssue::PREFIX_MISSING)
                                    ? SettingsKey::SETTINGS_CLOUD_INVALID_PREFIX
                                    : SettingsKey::SETTINGS_CLOUD_INVALID_SUFFIX;

        if (anyIssue)
            stringutils::safeAppend(errorMsg, "\n\n");

        char labelBuf[40];
        STRF(labelBuf, rows[i].labelKey);
        char reasonBuf[80];
        STRF(reasonBuf, reasonKey);

        char line[160];
        stringutils::safeFormat(line, "%s: %s", labelBuf, reasonBuf);
        stringutils::safeAppend(errorMsg, line);
        anyIssue = true;
    }

    if (anyIssue)
    {
        if (showMessage)
            showInfoBoxText(SettingsKey::SETTINGS_CLOUD_INVALID_TITLE, errorMsg);
        LOG_WARNING("Settings: Cloud-Adressen abgewiesen (Format-Prüfung)");
        return false;
    }

    // Schritt 2: alles in Ordnung, schreiben
    const bool allEmpty = m_cloudCalendarUrl[0] == '\0' &&
                          m_cloudTodoUrl[0] == '\0' &&
                          m_cloudContactsUrl[0] == '\0' &&
                          m_cloudOpenSkyClientId[0] == '\0' &&
                          m_cloudOpenSkyClientSecret[0] == '\0';

    if (allEmpty)
    {
        APP_STATE.resetPreferencesGoogleApi();
    }
    else
    {
        APP_STATE.setCalendarScriptUrl(m_cloudCalendarUrl);
        APP_STATE.setTodoScriptUrl(m_cloudTodoUrl);
        APP_STATE.setContactsScriptUrl(m_cloudContactsUrl);
        APP_STATE.setOpenSkyClientId(m_cloudOpenSkyClientId);
        APP_STATE.setOpenSkyClientSecret(m_cloudOpenSkyClientSecret);
    }

    stringutils::safeCopy(m_cloudCalendarBaseline, m_cloudCalendarUrl);
    stringutils::safeCopy(m_cloudTodoBaseline, m_cloudTodoUrl);
    stringutils::safeCopy(m_cloudContactsBaseline, m_cloudContactsUrl);
    stringutils::safeCopy(m_cloudOpenSkyClientIdBaseline, m_cloudOpenSkyClientId);
    stringutils::safeCopy(m_cloudOpenSkyClientSecretBaseline, m_cloudOpenSkyClientSecret);

    recomputeDirtyFlags();
    m_workDirty = true;

    if (showMessage)
        showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, SettingsKey::SETTINGS_CLOUD_SAVED_MSG);
    LOG_SUCCESS("Settings: Cloud-Anbindung gespeichert");
    return true;
}

/**
 * @brief Speichert die Cloud-Anbindung mit Hinweis.
 */
void Settings::saveCloud()
{
    persistCloud(true);
}

// ===================================================================
// Zeit-Panel
// ===================================================================

/**
 * @brief Geometrie einer der drei Auswahl-Zeilen oben im Zeit-Panel (sprite-relativ).
 *
 * Index 0 = Zeitzone, 1 = Zeitserver, 2 = Sync-Intervall.
 */
Settings::Rect Settings::timeFieldRect(int idx) const
{
    const int y = CONTENT_Y + idx * (TIME_FIELD_H + TIME_FIELD_GAP);
    return Rect{CONTENT_X, y, CONTENT_W, TIME_FIELD_H};
}

/**
 * @brief Geometrie einer der fünf Stepper-Spalten (sprite-relativ).
 *
 * Die ersten drei Spalten zeigen das Datum in der jeweiligen Landes-Reihenfolge (DE: Tag-Monat-Jahr, EN: Jahr-Monat-Tag). Die letzten beiden Spalten zeigen Stunde und Minute in beiden Sprachen rechts.
 */
Settings::Rect Settings::timeStepperColumnRect(int col) const
{
    const int totalW = TIME_STEPPER_TOTAL_W;
    const int startX = CONTENT_X + (CONTENT_W - totalW) / 2;
    const int x = startX + col * (TIME_STEPPER_COL_W + TIME_STEPPER_COL_GAP);
    const int y = CONTENT_Y + TIME_STEPPER_BLOCK_Y;
    return Rect{x, y, TIME_STEPPER_COL_W, TIME_STEPPER_TOTAL_H};
}

/**
 * @brief Geometrie des oberen Stepper-Knopfes einer Spalte (sprite-relativ).
 */
Settings::Rect Settings::timeStepperUpRect(int col) const
{
    const Rect c = timeStepperColumnRect(col);
    return Rect{c.x, c.y, c.w, TIME_STEPPER_BTN_H};
}

/**
 * @brief Geometrie des unteren Stepper-Knopfes einer Spalte (sprite-relativ).
 */
Settings::Rect Settings::timeStepperDownRect(int col) const
{
    const Rect c = timeStepperColumnRect(col);
    return Rect{c.x, c.y + TIME_STEPPER_BTN_H + TIME_STEPPER_VAL_H + 4, c.w, TIME_STEPPER_BTN_H};
}

/**
 * @brief Geometrie der Wert-Anzeige in einer Spalte (sprite-relativ).
 */
Settings::Rect Settings::timeStepperValueRect(int col) const
{
    const Rect c = timeStepperColumnRect(col);
    return Rect{c.x, c.y + TIME_STEPPER_BTN_H + 2, c.w, TIME_STEPPER_VAL_H};
}

/**
 * @brief Geometrie des "Zeit setzen"-Knopfes (sprite-relativ).
 */
Settings::Rect Settings::timeSetButtonRect() const
{
    const int x = CONTENT_X + (CONTENT_W - TIME_SET_BTN_W) / 2;
    const int y = CONTENT_Y + TIME_STEPPER_BLOCK_Y + TIME_STEPPER_TOTAL_H + 11;
    return Rect{x, y, TIME_SET_BTN_W, TIME_SET_BTN_H};
}

/**
 * @brief Begrenzt den Tag-Wert auf die in Monat/Jahr gültige Spanne.
 */
void Settings::clampManualDay()
{
    const int maxDay = daysInMonth(m_manualYear, m_manualMonth);
    if (m_manualDay > maxDay)
        m_manualDay = maxDay;
    if (m_manualDay < 1)
        m_manualDay = 1;
}

/**
 * @brief Liest die aktuelle RTC-Zeit in die Stepper-Werte ein (einmalig beim Eintritt).
 */
void Settings::initManualFromRtc()
{
    DS1307_RTC *rtc = &APP_STATE.getRTC();
    DateTime now{};
    if (rtc && rtc->getDateTime(now) && now.year >= 2000)
    {
        m_manualYear = static_cast<int>(now.year);
        m_manualMonth = static_cast<int>(now.month);
        m_manualDay = static_cast<int>(now.day);
        m_manualHour = static_cast<int>(now.hour);
        m_manualMinute = static_cast<int>(now.minute);
    }
    clampManualDay();
    m_manualInit = true;
}

/**
 * @brief Zeichnet das Zeit-Panel: Auswahl-Zeilen oben, Stepper-Block unten.
 */
void Settings::drawPanelTime(Gfx_Sprite *spr)
{
    if (!spr)
        return;

    if (!m_manualInit)
        initManualFromRtc();

    // Wert-Texte für die drei Auswahl-Zeilen vorbereiten
    const char *fieldLabels[TIME_FIELD_COUNT] = {
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_TIME_TZ_LABEL)),
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_TIME_NTP_LABEL)),
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_TIME_SYNC_LABEL)),
    };

    char tzValue[40] = {0};
    if (m_tzIndex >= 0 && m_tzIndex < TIMEZONES_COUNT)
        stringutils::safeCopy(tzValue, TIMEZONES[m_tzIndex].region);

    char ntpValue[64] = {0};
    stringutils::safeCopy(ntpValue, m_ntpServer);

    char syncValue[40] = {0};
    const int syncIdx = findSyncIntervalIndex(m_syncIntervalMs);
    if (syncIdx >= 0 && syncIdx < SYNC_INTERVALS_COUNT)
    {
        const char *lbl = reinterpret_cast<const char *>(I18n::str(SYNC_INTERVALS[syncIdx].labelKey));
        stringutils::safeCopy(syncValue, lbl);
    }

    const char *fieldValues[TIME_FIELD_COUNT] = {tzValue, ntpValue, syncValue};
    const SettingsKey notSetKeys[TIME_FIELD_COUNT] = {
        SettingsKey::SETTINGS_TIME_TZ_NOT_SET,
        SettingsKey::SETTINGS_TIME_NTP_NOT_SET,
        SettingsKey::SETTINGS_TIME_TZ_NOT_SET, // Sync hat immer einen Default
    };

    // Pending-Detektion pro Zeile für den linken Akzentstreifen
    const bool tzDirty = (m_tzIndex != m_tzBaseline);
    const bool ntpDirty = strcmp(m_ntpServer, m_ntpServerBaseline) != 0;
    const bool syncDirty = (m_syncIntervalMs != m_syncIntervalBaselineMs);
    const bool dirty[TIME_FIELD_COUNT] = {tzDirty, ntpDirty, syncDirty};

    for (int i = 0; i < TIME_FIELD_COUNT; ++i)
    {
        const Rect r = timeFieldRect(i);
        spr->fillSmoothRoundRect(r.x, r.y, r.w, r.h, 8, BTN_OUTLINE);
        spr->fillSmoothRoundRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, 7, C_ROW_BG);
        if (dirty[i])
            spr->fillRect(r.x + 1, r.y + 8, ITEM_ACCENT_W, r.h - 16, C_SEC_CONNECTION);

        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(TL_DATUM);
        spr->setTextColor(C_SUB);
        spr->drawString(fieldLabels[i], r.x + TIME_FIELD_PAD_X, r.y + TIME_FIELD_LABEL_Y);

        const bool hasValue = fieldValues[i][0] != '\0';
        spr->setTextColor(hasValue ? C_TEXT : C_SUB);
        const char *valueText = hasValue
                                    ? fieldValues[i]
                                    : reinterpret_cast<const char *>(I18n::str(notSetKeys[i]));
        spr->drawString(valueText, r.x + TIME_FIELD_PAD_X, r.y + TIME_FIELD_VALUE_Y);
        spr->unloadFont();
    }

    // Schrittgeberblock mit fünf Spalten in sprachabhängiger Datums- und Zeitreihenfolge
    clampManualDay();
    const int manualValues[FIELD_COUNT] = {
        m_manualYear, m_manualMonth, m_manualDay, m_manualHour, m_manualMinute};

    spr->loadFont(ComfortaaBold16);
    for (int col = 0; col < 5; ++col)
    {
        const Rect up = timeStepperUpRect(col);
        spr->fillSmoothRoundRect(up.x, up.y, up.w, up.h, 6, BTN_OUTLINE);
        spr->fillSmoothRoundRect(up.x + 1, up.y + 1, up.w - 2, up.h - 2, 5, BTN_FILL);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(BTN_TEXT);
        spr->drawString("+", up.x + up.w / 2, up.y + up.h / 2);

        const Rect down = timeStepperDownRect(col);
        spr->fillSmoothRoundRect(down.x, down.y, down.w, down.h, 6, BTN_OUTLINE);
        spr->fillSmoothRoundRect(down.x + 1, down.y + 1, down.w - 2, down.h - 2, 5, BTN_FILL);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(BTN_TEXT);
        spr->drawString("-", down.x + down.w / 2, down.y + down.h / 2);
    }
    spr->unloadFont();

    // Wert-Anzeigen: Jahr vierstellig, Rest zweistellig mit führender Null. Datum (Spalten links) hell, Uhrzeit (Spalten rechts) in Petrol, damit beide Gruppen visuell auseinanderzuhalten sind. Y-Offset gleicht die optische Asymmetrie der Ziffern aus (keine Unterlängen).
    constexpr int VALUE_Y_OFFSET = 3;
    spr->loadFont(ComfortaaBold30);
    spr->setTextDatum(MC_DATUM);
    for (int col = 0; col < 5; ++col)
    {
        const int field = stepperColToField(col);
        const Rect v = timeStepperValueRect(col);

        char buf[8];
        if (field == FIELD_YEAR)
            stringutils::safeFormat(buf, "%d", manualValues[field]);
        else
            stringutils::safeFormat(buf, "%02d", manualValues[field]);

        const bool isTime = (field == FIELD_HOUR) || (field == FIELD_MINUTE);
        spr->setTextColor(isTime ? C_SEC_CONNECTION : C_TEXT_BRIGHT);
        spr->drawString(buf, v.x + v.w / 2, v.y + v.h / 2 + VALUE_Y_OFFSET);
    }
    spr->unloadFont();

    // "Zeit setzen"-Knopf
    const Rect btn = timeSetButtonRect();
    spr->fillSmoothRoundRect(btn.x, btn.y, btn.w, btn.h, 6, BTN_OUTLINE);
    spr->fillSmoothRoundRect(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2, 5, BTN_FILL);
    spr->loadFont(LeagueGothicCond34);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(BTN_TEXT);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_TIME_MANUAL_BTN),
                    btn.x + btn.w / 2, btn.y + btn.h / 2);
    spr->unloadFont();
}

/**
 * @brief Verarbeitet Taps im Zeit-Panel.
 */
void Settings::handleTimeTouch(int sx, int sy)
{
    // Auswahl-Zeilen
    for (int i = 0; i < TIME_FIELD_COUNT; ++i)
    {
        const Rect r = timeFieldRect(i);
        if (pointIn(sx, sy, r.x, r.y, r.w, r.h))
        {
            if (i == 0)
                startTimezonePicker();
            else if (i == 1)
                startNtpServerInput();
            else if (i == 2)
                startSyncIntervalPicker();
            return;
        }
    }

    // Schrittgebertasten: Spalte zur Feldidentität auflösen, dann das passende Memberfeld erhöhen oder verringern. Der Tageshöchstwert ist datumsabhängig und wird abschließend durch clampManualDay() begrenzt.
    int *fieldPtrs[FIELD_COUNT] = {
        &m_manualYear, &m_manualMonth, &m_manualDay, &m_manualHour, &m_manualMinute};
    int fieldMin[FIELD_COUNT] = {2000, 1, 1, 0, 0};
    int fieldMax[FIELD_COUNT] = {2099, 12, 31, 23, 59};

    for (int col = 0; col < 5; ++col)
    {
        const int field = stepperColToField(col);
        int *value = fieldPtrs[field];
        const int minVal = fieldMin[field];
        const int maxVal = fieldMax[field];

        const Rect up = timeStepperUpRect(col);
        if (pointIn(sx, sy, up.x, up.y, up.w, up.h))
        {
            (*value)++;
            if (field == FIELD_DAY)
            {
                const int maxDay = daysInMonth(m_manualYear, m_manualMonth);
                if (*value > maxDay)
                    *value = minVal;
            }
            else if (*value > maxVal)
            {
                *value = minVal;
            }
            clampManualDay();
            m_workDirty = true;
            return;
        }

        const Rect down = timeStepperDownRect(col);
        if (pointIn(sx, sy, down.x, down.y, down.w, down.h))
        {
            (*value)--;
            if (field == FIELD_DAY)
            {
                if (*value < minVal)
                    *value = daysInMonth(m_manualYear, m_manualMonth);
            }
            else if (*value < minVal)
            {
                *value = maxVal;
            }
            clampManualDay();
            m_workDirty = true;
            return;
        }
    }

    // "Zeit setzen"-Knopf
    const Rect btn = timeSetButtonRect();
    if (pointIn(sx, sy, btn.x, btn.y, btn.w, btn.h))
    {
        startSetTimeDialog();
        return;
    }
}

/**
 * @brief Öffnet den Listen-Picker zur Wahl einer Zeitzone.
 */
void Settings::startTimezonePicker()
{
    const int selected = runListPicker(
        SettingsKey::SETTINGS_TIME_TZ_PICK_TITLE,
        TIMEZONES_COUNT,
        [](int idx, char *out, size_t outSize)
        {
            stringutils::safeCopy(out, outSize, TIMEZONES[idx].region);
        },
        m_tzIndex);

    if (selected >= 0 && selected < TIMEZONES_COUNT)
    {
        m_tzIndex = selected;
        recomputeDirtyFlags();
    }

    afterModalRedraw();
}

/**
 * @brief Öffnet den Listen-Picker zur Wahl des NTP-Sync-Intervalls.
 */
void Settings::startSyncIntervalPicker()
{
    const int currentIdx = findSyncIntervalIndex(m_syncIntervalMs);
    const int selected = runListPicker(
        SettingsKey::SETTINGS_TIME_SYNC_PICK_TITLE,
        SYNC_INTERVALS_COUNT,
        [](int idx, char *out, size_t outSize)
        {
            const char *lbl = reinterpret_cast<const char *>(I18n::str(SYNC_INTERVALS[idx].labelKey));
            stringutils::safeCopy(out, outSize, lbl);
        },
        currentIdx);

    if (selected >= 0 && selected < SYNC_INTERVALS_COUNT)
    {
        m_syncIntervalMs = SYNC_INTERVALS[selected].ms;
        recomputeDirtyFlags();
    }

    afterModalRedraw();
}

/**
 * @brief Öffnet die Tastatur zur Eingabe des NTP-Server-Hostnamens.
 */
void Settings::startNtpServerInput()
{
    if (!m_tft)
        return;

    Keyboard keyboard(m_tft);
    keyboard.setPasswordMode(false);

    char label[40];
    STRF(label, SettingsKey::SETTINGS_TIME_NTP_PROMPT);

    char buffer[64] = {0};
    stringutils::safeCopy(buffer, m_ntpServer);

    if (keyboard.show(label, buffer, sizeof(buffer), buffer))
    {
        stringutils::safeCopy(m_ntpServer, sizeof(m_ntpServer), buffer);
        recomputeDirtyFlags();
    }

    afterModalRedraw();
}

/**
 * @brief Startet die Bestätigungs-Box vor dem Setzen der Uhr.
 */
void Settings::startSetTimeDialog()
{
    if (!m_tft)
        return;

    char title[40];
    STRF(title, SettingsKey::SETTINGS_TIME_MANUAL_CONFIRM_TITLE);
    char msg[200];
    STRF(msg, SettingsKey::SETTINGS_TIME_MANUAL_CONFIRM_MSG);

    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 140, 400, 200);
    m_activeMessageBox->setTitle(title);
    m_activeMessageBox->setMessage(msg);
    m_activeMessageBox->setButtonLabels(
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_TIME_MANUAL_BTN)),
        reinterpret_cast<const char *>(I18n::str(CommonKey::COMMON_BTN_CANCEL)));

    m_activeMessageBox->setCallback([this](bool confirmed)
                                    {
        if (confirmed)
            applyManualTime(); });

    m_activeMessageBox->start();
    changeState(State::SHOWING_MESSAGE);
}

/**
 * @brief Schreibt die im Stepper eingestellte Zeit in die RTC.
 *
 * Der Wochentag wird aus dem Datum hergeleitet. mktime füllt tm.tm_wday, danach Umrechnung von 0=Sonntag..6=Samstag in die ISO-Konvention 1=Montag..7=Sonntag).
 */
void Settings::applyManualTime()
{
    clampManualDay();

    // Wochentag über mktime ableiten
    std::tm t{};
    t.tm_year = m_manualYear - 1900;
    t.tm_mon = m_manualMonth - 1;
    t.tm_mday = m_manualDay;
    t.tm_hour = m_manualHour;
    t.tm_min = m_manualMinute;
    t.tm_sec = 0;
    t.tm_isdst = -1;
    if (mktime(&t) == static_cast<time_t>(-1))
    {
        showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, SettingsKey::SETTINGS_TIME_MANUAL_FAIL);
        return;
    }

    // tm_wday: 0=Sonntag..6=Samstag -> ISO 1=Montag..7=Sonntag
    const uint8_t isoDow = (t.tm_wday == 0) ? 7 : static_cast<uint8_t>(t.tm_wday);

    DateTime dt{};
    dt.year = static_cast<uint16_t>(m_manualYear);
    dt.month = static_cast<uint8_t>(m_manualMonth);
    dt.day = static_cast<uint8_t>(m_manualDay);
    dt.hour = static_cast<uint8_t>(m_manualHour);
    dt.minute = static_cast<uint8_t>(m_manualMinute);
    dt.second = 0;
    dt.dayOfWeek = isoDow;

    DS1307_RTC *rtc = &APP_STATE.getRTC();
    const bool ok = rtc && rtc->setDateTime(dt);

    if (ok)
    {
        showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, SettingsKey::SETTINGS_TIME_MANUAL_OK);
        LOG_SUCCESS_F("Settings: Uhr auf %04d-%02d-%02d %02d:%02d gesetzt",
                      m_manualYear, m_manualMonth, m_manualDay, m_manualHour, m_manualMinute);
    }
    else
    {
        showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, SettingsKey::SETTINGS_TIME_MANUAL_FAIL);
        LOG_WARNING("Settings: Uhr konnte nicht gesetzt werden");
    }
}

/**
 * @brief Prüft, ob mindestens eine Zeit-Einstellung abweicht.
 *
 * Die manuelle Zeit wird hier nicht berücksichtigt, da sie eine Sofortaktion ist über den eigenen Knopf, kein gestaged-und-gespeichertes Feld.
 */
bool Settings::timePending() const
{
    return (m_tzIndex != m_tzBaseline) || strcmp(m_ntpServer, m_ntpServerBaseline) != 0 || (m_syncIntervalMs != m_syncIntervalBaselineMs);
}

/**
 * @brief Schreibt die vorgemerkten Zeit-Einstellungen nach AppState.
 *
 * Geschrieben werden Zeitzone (Region und POSIX-String als Paar), NTP-Server und Sync-Intervall. Die manuelle Zeit ist hier nicht enthalten.
 */
void Settings::persistTime(bool showMessage)
{
    if (m_tzIndex >= 0 && m_tzIndex < TIMEZONES_COUNT)
    {
        APP_STATE.setTimezoneRegion(TIMEZONES[m_tzIndex].region);
        APP_STATE.setTimezoneStr(TIMEZONES[m_tzIndex].posix);
    }
    APP_STATE.setNtpServer(m_ntpServer);
    APP_STATE.setNtpSyncInterval(m_syncIntervalMs);

    m_tzBaseline = m_tzIndex;
    stringutils::safeCopy(m_ntpServerBaseline, m_ntpServer);
    m_syncIntervalBaselineMs = m_syncIntervalMs;

    recomputeDirtyFlags();
    m_workDirty = true;

    if (showMessage)
        showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, SettingsKey::SETTINGS_TIME_SAVED_MSG);
    LOG_SUCCESS("Settings: Zeit-Einstellungen gespeichert");
}

/**
 * @brief Speichert die Zeit-Einstellungen mit Hinweis.
 */
void Settings::saveTime()
{
    persistTime(true);
}

// ===================================================================
// Zeitfenster-Panel
// ===================================================================

/**
 * @brief Bildet die 24-Bit-Maske eines Zeitfenster-Tages.
 */
uint32_t Settings::timeWindowDayMask(int day) const
{
    if (day < 0 || day >= TIME_WINDOW_DAYS)
        return TIME_WINDOW_FULL_MASK;

    uint32_t mask = 0;
    for (int hour = 0; hour < TIME_WINDOW_HOURS; ++hour)
    {
        if (m_timeWindow[day][hour])
            mask |= (1UL << hour);
    }
    return mask & TIME_WINDOW_FULL_MASK;
}

/**
 * @brief Lädt die Zeitfenster aus AppState und setzt die Vergleichsbasis.
 */
void Settings::loadTimeWindowFromAppState()
{
    for (int day = 0; day < TIME_WINDOW_DAYS; ++day)
    {
        const uint32_t mask = APP_STATE.getTimeWindowDayMask(static_cast<uint8_t>(day)) & TIME_WINDOW_FULL_MASK;
        m_timeWindowBaselineMasks[day] = mask;
        for (int hour = 0; hour < TIME_WINDOW_HOURS; ++hour)
            m_timeWindow[day][hour] = (mask & (1UL << hour)) != 0;
    }
}

/**
 * @brief Prüft, ob die Zeitfenster-Matrix von der Baseline abweicht.
 */
bool Settings::timeWindowPending() const
{
    for (int day = 0; day < TIME_WINDOW_DAYS; ++day)
    {
        if (timeWindowDayMask(day) != m_timeWindowBaselineMasks[day])
            return true;
    }
    return false;
}

/**
 * @brief Speichert die Zeitfenster in AppState.
 */
void Settings::persistTimeWindow(bool showMessage)
{
    for (int day = 0; day < TIME_WINDOW_DAYS; ++day)
    {
        const uint32_t mask = timeWindowDayMask(day);
        APP_STATE.setTimeWindowDayMask(static_cast<uint8_t>(day), mask);
        m_timeWindowBaselineMasks[day] = mask;
    }

    recomputeDirtyFlags();
    m_workDirty = true;

    if (showMessage)
        showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, SettingsKey::SETTINGS_TIME_WINDOW_SAVED_MSG);
    LOG_SUCCESS("Settings: Zeitfenster gespeichert");
}

/**
 * @brief Speichert die aktuelle Zeitfenster-Matrix mit Hinweis.
 */
void Settings::saveTimeWindow()
{
    persistTimeWindow(true);
}

/**
 * @brief Fragt vor dem Zurücksetzen der Zeitfenster nach.
 */
void Settings::startTimeWindowResetDialog()
{
    if (!m_tft)
        return;

    char title[48];
    STRF(title, SettingsKey::SETTINGS_TIME_WINDOW_RESET_TITLE);
    char msg[96];
    STRF(msg, SettingsKey::SETTINGS_TIME_WINDOW_RESET_MSG);

    m_activeMessageBox.reset(new MessageBox(m_tft));
    m_activeMessageBox->setBounds(200, 140, 400, 200);
    m_activeMessageBox->setTitle(title);
    m_activeMessageBox->setMessage(msg);
    m_activeMessageBox->setButtonLabels(
        reinterpret_cast<const char *>(I18n::str(SettingsKey::SETTINGS_INFO_RESET_BTN)),
        reinterpret_cast<const char *>(I18n::str(CommonKey::COMMON_BTN_CANCEL)));

    m_activeMessageBox->setCallback([this](bool confirmed)
                                    {
        if (confirmed)
            resetTimeWindow(); });

    m_activeMessageBox->start();
    changeState(State::SHOWING_MESSAGE);
}

/**
 * @brief Setzt alle Zeitfenster auf aktiv und speichert den Standard.
 */
void Settings::resetTimeWindow()
{
    for (int day = 0; day < TIME_WINDOW_DAYS; ++day)
    {
        for (int hour = 0; hour < TIME_WINDOW_HOURS; ++hour)
            m_timeWindow[day][hour] = true;
        m_timeWindowBaselineMasks[day] = TIME_WINDOW_FULL_MASK;
    }

    APP_STATE.resetPreferencesTimeWindow();
    recomputeDirtyFlags();
    m_workDirty = true;
    m_footerDirty = true;
}

/**
 * @brief Geometrie einer Zeitfenster-Zelle (sprite-relativ).
 */
Settings::Rect Settings::timeWindowCellRect(int day, int hour) const
{
    const int startX = CONTENT_X;
    const int gridX = startX + TIME_WINDOW_DAY_LABEL_W;
    const int cellX = gridX + (hour * TIME_WINDOW_GRID_W) / TIME_WINDOW_HOURS;
    const int nextCellX = gridX + ((hour + 1) * TIME_WINDOW_GRID_W) / TIME_WINDOW_HOURS;
    const int rowY = TIME_WINDOW_TOP + TIME_WINDOW_HEADER_H + day * TIME_WINDOW_CELL_H;
    return Rect{cellX, rowY, nextCellX - cellX, TIME_WINDOW_CELL_H};
}

/**
 * @brief Zeichnet das Zeitfenster-Panel mit Wochentagen und vollen Stunden.
 */
void Settings::drawPanelTimeWindow(Gfx_Sprite *spr)
{
    if (!spr)
        return;

    const char *daysDe[TIME_WINDOW_DAYS] = {"Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"};
    const char *daysEn[TIME_WINDOW_DAYS] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    const char **days = (I18n::currentLang() == LangIndex::EN) ? daysEn : daysDe;

    const int totalW = TIME_WINDOW_DAY_LABEL_W + TIME_WINDOW_GRID_W;
    const int startX = CONTENT_X;
    const int gridX = startX + TIME_WINDOW_DAY_LABEL_W;
    const int headerY = TIME_WINDOW_TOP;
    const int rowsY = TIME_WINDOW_TOP + TIME_WINDOW_HEADER_H;
    const int rowsH = TIME_WINDOW_DAYS * TIME_WINDOW_CELL_H;

    spr->fillSmoothRoundRect(startX, rowsY - TIME_WINDOW_FRAME_PAD_Y,
                             totalW, rowsH + 2 * TIME_WINDOW_FRAME_PAD_Y,
                             TIME_WINDOW_FRAME_R, BTN_OUTLINE);
    spr->fillSmoothRoundRect(startX + TIME_WINDOW_FRAME_INSET, rowsY - TIME_WINDOW_FRAME_INSET,
                             totalW - 2 * TIME_WINDOW_FRAME_INSET, rowsH + 2 * TIME_WINDOW_FRAME_INSET,
                             TIME_WINDOW_INNER_R, C_ROW_BG);

    spr->loadFont(ComfortaaBold10);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_SUB);
    for (int hour = 0; hour < TIME_WINDOW_HOURS; ++hour)
    {
        char label[TIME_WINDOW_HOUR_LABEL_BUF];
        stringutils::safeFormat(label, "%02d", hour);
        const Rect cell = timeWindowCellRect(0, hour);
        spr->drawString(label,
                        cell.x + cell.w / 2,
                        headerY + TIME_WINDOW_HEADER_H / 2);
    }

    for (int day = 0; day < TIME_WINDOW_DAYS; ++day)
    {
        const int rowY = rowsY + day * TIME_WINDOW_CELL_H;

        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_TEXT_BRIGHT);
        spr->drawString(days[day], startX + TIME_WINDOW_DAY_TEXT_X, rowY + TIME_WINDOW_CELL_H / 2);

        for (int hour = 0; hour < TIME_WINDOW_HOURS; ++hour)
        {
            const Rect cell = timeWindowCellRect(day, hour);
            const int boxX = cell.x + (cell.w - TIME_WINDOW_CHECK_SIZE) / 2;
            const int boxY = cell.y + (cell.h - TIME_WINDOW_CHECK_SIZE) / 2;
            const bool checked = m_timeWindow[day][hour];
            const uint16_t color = checked ? C_SEC_SYSTEM : uint16_t(Theme::ColorButtonBorder);

            spr->drawRect(boxX, boxY, TIME_WINDOW_CHECK_SIZE, TIME_WINDOW_CHECK_SIZE, color);
            if (checked)
            {
                const int cx = boxX + TIME_WINDOW_CHECK_SIZE / 2;
                const int cy = boxY + TIME_WINDOW_CHECK_SIZE / 2;
                spr->drawWideLine(cx - 3, cy - 1, cx - 1, cy + 2, TIME_WINDOW_CHECK_LINE_W, color);
                spr->drawWideLine(cx - 1, cy + 2, cx + 4, cy - 4, TIME_WINDOW_CHECK_LINE_W, color);
            }
        }

        if (day + 1 < TIME_WINDOW_DAYS)
            spr->drawFastHLine(startX + TIME_WINDOW_FRAME_INSET,
                               rowY + TIME_WINDOW_CELL_H - TIME_WINDOW_FRAME_INSET,
                               totalW - 2 * TIME_WINDOW_FRAME_INSET,
                               C_SEP_LINE);
    }

    for (int hour = 0; hour <= TIME_WINDOW_HOURS; ++hour)
    {
        const int x = gridX + (hour * TIME_WINDOW_GRID_W) / TIME_WINDOW_HOURS;
        spr->drawFastVLine(x, rowsY, rowsH, C_SEP_LINE);
    }
    spr->unloadFont();

    const int hintY = rowsY + rowsH + TIME_WINDOW_HINT_GAP_Y;
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(C_SUB);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_TIME_WINDOW_HINT_ACTIVE), TIME_WINDOW_HINT_X, hintY);
    spr->drawString(I18n::str(SettingsKey::SETTINGS_TIME_WINDOW_HINT_SLEEP), TIME_WINDOW_HINT_X, hintY + TIME_WINDOW_HINT_LINE_H);
    spr->unloadFont();
}

/**
 * @brief Schaltet eine Zeitfenster-Checkbox um.
 */
void Settings::handleTimeWindowTouch(int sx, int sy)
{
    for (int day = 0; day < TIME_WINDOW_DAYS; ++day)
    {
        for (int hour = 0; hour < TIME_WINDOW_HOURS; ++hour)
        {
            const Rect cell = timeWindowCellRect(day, hour);
            if (!pointIn(sx, sy, cell.x, cell.y, cell.w, cell.h))
                continue;

            m_timeWindow[day][hour] = !m_timeWindow[day][hour];
            recomputeDirtyFlags();
            m_workDirty = true;
            return;
        }
    }
}

// ===================================================================
// Klang-Panel
// ===================================================================

/**
 * @brief Geometrie einer Klang-Zeile (sprite-relativ).
 */
Settings::Rect Settings::audioStepperColumnRect(int row) const
{
    const int y = CONTENT_Y + AUDIO_ROW_FIRST_Y + row * (AUDIO_ROW_H + AUDIO_ROW_GAP);
    return Rect{CONTENT_X, y, CONTENT_W, AUDIO_ROW_H};
}

/**
 * @brief Geometrie des Minus-Knopfes einer Klang-Zeile (sprite-relativ).
 */
Settings::Rect Settings::audioStepperDownRect(int row) const
{
    const Rect c = audioStepperColumnRect(row);
    const int blockW = AUDIO_STEP_BTN_W * 2 + AUDIO_STEP_VAL_W + AUDIO_STEP_GAP * 2;
    const int x = c.x + c.w - blockW;
    return Rect{x, c.y, AUDIO_STEP_BTN_W, AUDIO_ROW_H};
}

/**
 * @brief Geometrie der Wert-Anzeige einer Klang-Zeile (sprite-relativ).
 */
Settings::Rect Settings::audioStepperValueRect(int row) const
{
    const Rect down = audioStepperDownRect(row);
    return Rect{down.x + AUDIO_STEP_BTN_W + AUDIO_STEP_GAP, down.y, AUDIO_STEP_VAL_W, AUDIO_ROW_H};
}

/**
 * @brief Geometrie des Plus-Knopfes einer Klang-Zeile (sprite-relativ).
 */
Settings::Rect Settings::audioStepperUpRect(int row) const
{
    const Rect v = audioStepperValueRect(row);
    return Rect{v.x + AUDIO_STEP_VAL_W + AUDIO_STEP_GAP, v.y, AUDIO_STEP_BTN_W, AUDIO_ROW_H};
}

/**
 * @brief Zeichnet die Klangregler und den Lautsprecher-Umschalter.
 */
void Settings::drawPanelAudio(Gfx_Sprite *spr)
{
    const SettingsKey labels[AUDIO_ROW_COUNT + 1] = {
        SettingsKey::SETTINGS_AUDIO_BASS,
        SettingsKey::SETTINGS_AUDIO_MID,
        SettingsKey::SETTINGS_AUDIO_HIGH,
        SettingsKey::SETTINGS_AUDIO_WIDTH,
        SettingsKey::SETTINGS_AUDIO_LOUDNESS,
        SettingsKey::SETTINGS_AUDIO_SPEAKERS};
    const SettingsKey valueFormats[AUDIO_ROW_COUNT] = {
        SettingsKey::SETTINGS_AUDIO_HZ_FMT,
        SettingsKey::SETTINGS_AUDIO_HZ_FMT,
        SettingsKey::SETTINGS_AUDIO_HZ_FMT,
        SettingsKey::SETTINGS_AUDIO_PCT_FMT,
        SettingsKey::SETTINGS_AUDIO_PCT_FMT};
    const uint16_t values[AUDIO_ROW_COUNT] = {m_eqFreqLow, m_eqFreqPeak, m_eqFreqHigh, m_stereoWidth, m_loudness};

    for (int row = 0; row <= AUDIO_ROW_COUNT; ++row)
    {
        const Rect c = audioStepperColumnRect(row);

        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_TEXT_BRIGHT);
        spr->drawString(I18n::str(labels[row]), c.x + 4, c.y + AUDIO_ROW_H / 2);
        spr->unloadFont();

        const Rect down = audioStepperDownRect(row);
        spr->fillSmoothRoundRect(down.x, down.y, down.w, down.h, 6, BTN_OUTLINE);
        spr->fillSmoothRoundRect(down.x + 1, down.y + 1, down.w - 2, down.h - 2, 5, BTN_FILL);
        spr->loadFont(ComfortaaBold30);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(BTN_TEXT);
        spr->drawString("-", down.x + down.w / 2, down.y + down.h / 2);
        spr->unloadFont();

        const Rect up = audioStepperUpRect(row);
        spr->fillSmoothRoundRect(up.x, up.y, up.w, up.h, 6, BTN_OUTLINE);
        spr->fillSmoothRoundRect(up.x + 1, up.y + 1, up.w - 2, up.h - 2, 5, BTN_FILL);
        spr->loadFont(ComfortaaBold30);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(BTN_TEXT);
        spr->drawString("+", up.x + up.w / 2, up.y + up.h / 2);
        spr->unloadFont();

        const Rect v = audioStepperValueRect(row);
        char buf[16];
        if (row == AUDIO_ROW_COUNT)
            STRF(buf, m_speakersEnabled ? SettingsKey::SETTINGS_AUDIO_SPEAKERS_ON : SettingsKey::SETTINGS_AUDIO_SPEAKERS_OFF);
        else
            STRF(buf, valueFormats[row], static_cast<int>(values[row]));
        spr->loadFont(ComfortaaBold30);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_SEC_SYSTEM);
        spr->drawString(buf, v.x + v.w / 2, v.y + v.h / 2 + 3);
        spr->unloadFont();
    }
}

/**
 * @brief Verarbeitet Taps im Klang-Panel und wendet Änderungen sofort an.
 */
void Settings::handleAudioTouch(int sx, int sy)
{
    const Rect speakersUp = audioStepperUpRect(AUDIO_ROW_COUNT);
    const Rect speakersDown = audioStepperDownRect(AUDIO_ROW_COUNT);
    const bool enableSpeakers = pointIn(sx, sy, speakersUp.x, speakersUp.y, speakersUp.w, speakersUp.h);
    const bool disableSpeakers = pointIn(sx, sy, speakersDown.x, speakersDown.y, speakersDown.w, speakersDown.h);
    if (enableSpeakers || disableSpeakers)
    {
        if (m_speakersEnabled != enableSpeakers)
        {
            m_speakersEnabled = enableSpeakers;
            applySpeakerOutput(m_speakersEnabled);
            m_workDirty = true;
            m_footerDirty = true;
        }
        return;
    }

    extern void applyAudioTone(uint16_t freqLow, uint16_t freqPeak, uint16_t freqHigh, uint8_t loudness);
    extern void applyStereoWidth(uint8_t width);

    uint16_t *values[AUDIO_ROW_COUNT] = {&m_eqFreqLow, &m_eqFreqPeak, &m_eqFreqHigh, &m_stereoWidth, &m_loudness};
    const uint16_t mins[AUDIO_ROW_COUNT] = {AUDIO_LOW_MIN, AUDIO_PEAK_MIN, AUDIO_HIGH_MIN, AUDIO_WIDTH_MIN, AUDIO_LOUDNESS_MIN};
    const uint16_t maxs[AUDIO_ROW_COUNT] = {AUDIO_LOW_MAX, AUDIO_PEAK_MAX, AUDIO_HIGH_MAX, AUDIO_WIDTH_MAX, AUDIO_LOUDNESS_MAX};
    const uint16_t steps[AUDIO_ROW_COUNT] = {AUDIO_FREQ_STEP_HZ, AUDIO_FREQ_STEP_HZ, AUDIO_FREQ_STEP_HZ, AUDIO_WIDTH_STEP_PCT, AUDIO_LOUDNESS_STEP_PCT};

    for (int row = 0; row < AUDIO_ROW_COUNT; ++row)
    {
        const Rect up = audioStepperUpRect(row);
        if (pointIn(sx, sy, up.x, up.y, up.w, up.h))
        {
            if (*values[row] + steps[row] <= maxs[row])
                *values[row] += steps[row];
            if (row == AUDIO_WIDTH_ROW)
                applyStereoWidth(static_cast<uint8_t>(m_stereoWidth));
            else
                applyAudioTone(m_eqFreqLow, m_eqFreqPeak, m_eqFreqHigh, static_cast<uint8_t>(m_loudness));
            m_workDirty = true;
            m_footerDirty = true;
            return;
        }

        const Rect down = audioStepperDownRect(row);
        if (pointIn(sx, sy, down.x, down.y, down.w, down.h))
        {
            if (*values[row] >= mins[row] + steps[row])
                *values[row] -= steps[row];
            if (row == AUDIO_WIDTH_ROW)
                applyStereoWidth(static_cast<uint8_t>(m_stereoWidth));
            else
                applyAudioTone(m_eqFreqLow, m_eqFreqPeak, m_eqFreqHigh, static_cast<uint8_t>(m_loudness));
            m_workDirty = true;
            m_footerDirty = true;
            return;
        }
    }
}

/**
 * @brief Meldet, ob die Klangwerte vom gespeicherten Stand abweichen.
 */
bool Settings::audioPending() const
{
    return m_eqFreqLow != m_eqFreqLowBaseline || m_eqFreqPeak != m_eqFreqPeakBaseline || m_eqFreqHigh != m_eqFreqHighBaseline || m_stereoWidth != m_stereoWidthBaseline || m_loudness != m_loudnessBaseline || m_speakersEnabled != m_speakersEnabledBaseline;
}

/**
 * @brief Schreibt die eingestellten Klangwerte dauerhaft in AppState.
 */
void Settings::persistAudio(bool showMessage)
{
    APP_STATE.setEqFreqLow(m_eqFreqLow);
    APP_STATE.setEqFreqPeak(m_eqFreqPeak);
    APP_STATE.setEqFreqHigh(m_eqFreqHigh);
    APP_STATE.setStereoWidth(static_cast<uint8_t>(m_stereoWidth));
    APP_STATE.setLoudness(static_cast<uint8_t>(m_loudness));
    APP_STATE.setSpeakersEnabled(m_speakersEnabled);

    m_eqFreqLowBaseline = m_eqFreqLow;
    m_eqFreqPeakBaseline = m_eqFreqPeak;
    m_eqFreqHighBaseline = m_eqFreqHigh;
    m_stereoWidthBaseline = m_stereoWidth;
    m_loudnessBaseline = m_loudness;
    m_speakersEnabledBaseline = m_speakersEnabled;

    recomputeDirtyFlags();
    m_workDirty = true;

    if (showMessage)
        showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, SettingsKey::SETTINGS_AUDIO_SAVED_MSG);
    LOG_SUCCESS_F("Settings: Klang gespeichert (%u/%u/%u Hz, Breite %u%%, Loudness %u%%, Lautsprecher %s)",
                  m_eqFreqLow, m_eqFreqPeak, m_eqFreqHigh, m_stereoWidth, m_loudness, m_speakersEnabled ? "an" : "aus");
}

/**
 * @brief Speichert die Klangwerte mit Hinweis.
 */
void Settings::saveAudio()
{
    persistAudio(true);
}

// ===================================================================
// Anzeige-Panel
// ===================================================================

/**
 * @brief Geometrie einer Röhrenkachel (sprite-relativ).
 */
Settings::Rect Settings::appearanceTileRect(int index) const
{
    const int x = CONTENT_X + index * (APPEARANCE_TILE_W + APPEARANCE_TILE_GAP);
    return Rect{x, CONTENT_Y + APPEARANCE_TILE_Y, APPEARANCE_TILE_W, APPEARANCE_TILE_H};
}

/**
 * @brief Geometrie eines Segments einer Wahlzeile (sprite-relativ).
 * @param rowY Y der Zeile relativ zum Inhalt
 * @param index Nummer des Segments von links
 * @param count Anzahl der Segmente in dieser Zeile
 *
 * Die Segmente teilen sich den Platz rechts der Beschriftung zu gleichen Teilen.
 */
Settings::Rect Settings::appearanceSegmentRect(int rowY, int index, int count) const
{
    const int x0 = CONTENT_X + APPEARANCE_LABEL_W;
    const int platz = CONTENT_W - APPEARANCE_LABEL_W;
    const int w = (platz - (count - 1) * APPEARANCE_SEG_GAP) / count;
    return Rect{x0 + index * (w + APPEARANCE_SEG_GAP), CONTENT_Y + rowY, w, APPEARANCE_ROW_H};
}

/**
 * @brief Zeichnet eine Abschnittsüberschrift mit Trennlinie darunter.
 */
void Settings::drawAppearanceSectionTitle(Gfx_Sprite *spr, int y, SettingsKey titleKey)
{
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(C_SEC_SYSTEM);
    spr->drawString(I18n::str(titleKey), CONTENT_X + 2, CONTENT_Y + y);
    spr->unloadFont();

    spr->drawFastHLine(CONTENT_X, CONTENT_Y + y + APPEARANCE_SEC_LINE_DY, CONTENT_W, BTN_OUTLINE);
}

/**
 * @brief Zeichnet eine Hinweiszeile in gedämpfter Farbe.
 */
void Settings::drawAppearanceHint(Gfx_Sprite *spr, int y, SettingsKey key)
{
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(C_SUB);
    spr->drawString(I18n::str(key), CONTENT_X + 2, CONTENT_Y + y);
    spr->unloadFont();
}

/**
 * @brief Zeichnet die vier Röhrenkacheln samt Vorschau des Schattenbildes.
 *
 * Die Vorschau benutzt dieselben Zeichenroutinen und dieselbe Größe wie das Radio, ist also kein
 * Abbild, sondern die Sache selbst. Ein mittlerer Pegel zeigt das Muster jeder Bauart am besten.
 */
void Settings::drawAppearanceEyeTiles(Gfx_Sprite *spr)
{
    static const SettingsKey NAMEN[] = {
        SettingsKey::SETTINGS_APPEARANCE_EYE_EM11,
        SettingsKey::SETTINGS_APPEARANCE_EYE_EM34,
        SettingsKey::SETTINGS_APPEARANCE_EYE_EM80,
        SettingsKey::SETTINGS_APPEARANCE_EYE_EM84,
    };
    static_assert(sizeof(NAMEN) / sizeof(NAMEN[0]) == APPEARANCE_EYE_MAX + 1,
                  "Zu jeder Röhre muss eine Beschriftung gehören");

    constexpr float VORSCHAU_PEGEL = 0.55f;
    constexpr float RING_DICKE = 5.0f;

    for (int i = 0; i <= APPEARANCE_EYE_MAX; ++i)
    {
        const Rect t = appearanceTileRect(i);
        const bool aktiv = (i == m_magicEyeStyle);
        const uint16_t grund = aktiv ? C_ROW_SEL_BG : C_ROW_BG;

        spr->fillSmoothRoundRect(t.x, t.y, t.w, t.h, 8, aktiv ? C_SEC_SYSTEM : BTN_OUTLINE);
        spr->fillSmoothRoundRect(t.x + 1, t.y + 1, t.w - 2, t.h - 2, 7, grund);

        const int cx = t.x + t.w / 2;
        const int cy = t.y + 4 + APPEARANCE_EYE_BOX / 2;

        switch (static_cast<MagicEyeStyle>(i))
        {
        case MagicEyeStyle::Em80:
            spr->drawEm80MagicEye(cx, cy, RRLayout::MagicEye::EM80_WIDTH / 2.0f,
                                  RRLayout::MagicEye::EM80_HEIGHT / 2.0f,
                                  VORSCHAU_PEGEL, RRColors::MAGIC_EYE_RING, grund);
            break;

        case MagicEyeStyle::Em84:
            spr->drawEm84MagicEye(cx, cy, RRLayout::MagicEye::EM84_WIDTH / 2.0f,
                                  RRLayout::MagicEye::EM84_HEIGHT / 2.0f,
                                  VORSCHAU_PEGEL, RRColors::MAGIC_EYE_RING, grund);
            break;

        default:
        {
            const float r = APPEARANCE_EYE_BOX / 2.0f - 1.0f;
            spr->drawSmoothCircle(cx, cy, r, RING_DICKE, RRColors::MAGIC_EYE_RING, grund);

            if (static_cast<MagicEyeStyle>(i) == MagicEyeStyle::Em34)
                spr->drawEm34MagicEye(cx, cy, r - RING_DICKE, VORSCHAU_PEGEL, grund);
            else
                spr->drawEm11MagicEye(cx, cy, r - RING_DICKE, VORSCHAU_PEGEL, grund);
            break;
        }
        }

        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(aktiv ? C_TEXT_BRIGHT : C_TEXT);
        spr->drawString(I18n::str(NAMEN[i]), cx, t.y + t.h - 13);
        spr->unloadFont();
    }
}

/**
 * @brief Frischt die Pegelanzeige rechts in der Abschnittszeile des VU-Meters auf.
 *
 * Angezeigt wird der Effektivwert, den der Sender gerade liefert, in Dezibel unter
 * Vollaussteuerung. Wer den Bezugspegel ungefähr auf diese Zahl stellt, bekommt einen Zeiger,
 * der um 0 VU pendelt. Damit erübrigt sich das Ausprobieren.
 *
 * Gezeichnet wird unmittelbar auf das Display, nicht in den Arbeits-Sprite: die vier
 * Röhrenvorschauen jedes Mal neu zu rechnen wäre für eine Zahl zu teuer.
 */
void Settings::drawAppearanceLevel()
{
    if (!m_tft || m_panel != Panel::APPEARANCE || m_state != State::IDLE)
        return;

    // Steht die Wiedergabe, liefert der AudioManager keine neuen Werte mehr, sondern behält
    // seinen letzten. Ohne diese Abfrage bliebe eine eingefrorene Zahl stehen, als käme noch
    // etwas. Der SD-Player stoppt beim Verlassen seines Moduls, das Radio läuft weiter.
    const bool laeuft = audio.isRunning();

    // Der Momentanwert springt bei Musik um viele Dezibel. Gemittelt wird deshalb in jedem
    // Durchlauf, geschrieben nur einmal je Sekunde und nur, wenn sich die Zahl wirklich ändert.
    const float rms = fmaxf(g_audioManager.getRMSLeft(), g_audioManager.getRMSRight());
    const float db = (laeuft && rms > 1e-5f) ? 20.0f * log10f(rms) : -60.0f;
    m_appearanceLevelDb += (db - m_appearanceLevelDb) * APPEARANCE_LEVEL_SMOOTH;

    const unsigned long now = millis();
    if (now - m_appearanceLevelMs < APPEARANCE_LEVEL_MS)
        return;

    m_appearanceLevelMs = now;

    char wert[12];
    if (laeuft)
        stringutils::safeFormat(wert, "%d dB", (int)lroundf(m_appearanceLevelDb));
    else
        stringutils::safeCopy(wert, sizeof(wert), "--");

    if (strcmp(wert, m_appearanceLevelText) == 0)
        return;

    stringutils::safeCopy(m_appearanceLevelText, sizeof(m_appearanceLevelText), wert);

    const int x = WORK_X + CONTENT_X + CONTENT_W;
    const int y = WORK_Y + CONTENT_Y + APPEARANCE_SEC2_Y;

    m_tft->fillRect(x - APPEARANCE_LEVEL_W, y, APPEARANCE_LEVEL_W, APPEARANCE_HINT_H, C_WORK_BG);
    m_tft->loadFont(ComfortaaBold16);
    m_tft->setTextDatum(TR_DATUM);
    m_tft->setTextColor(C_TEXT_BRIGHT, C_WORK_BG);
    m_tft->drawString(wert, x, y);
    m_tft->setTextColor(C_SUB, C_WORK_BG);
    m_tft->drawString(I18n::str(SettingsKey::SETTINGS_APPEARANCE_LEVEL_NOW),
                      x - m_tft->textWidth(wert) - 8, y);
    m_tft->unloadFont();
}

/**
 * @brief Zeichnet eine Wahlzeile aus nebeneinanderliegenden Segmenten.
 * @param spr Ziel-Sprite
 * @param rowY Y der Zeile relativ zum Inhalt
 * @param labelKey Beschriftung am linken Rand
 * @param values Beschriftungen der Segmente
 * @param count Anzahl der Segmente
 * @param selected Nummer des gewählten Segments
 *
 * Alle Möglichkeiten stehen gleichzeitig da; ein Druck wählt unmittelbar aus, ohne Zählen.
 */
void Settings::drawAppearanceSegments(Gfx_Sprite *spr, int rowY, SettingsKey labelKey,
                                      const char *const *values, int count, int selected)
{
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_TEXT_BRIGHT);
    spr->drawString(I18n::str(labelKey), CONTENT_X + 2, CONTENT_Y + rowY + APPEARANCE_ROW_H / 2);
    spr->unloadFont();

    for (int i = 0; i < count; ++i)
    {
        const Rect r = appearanceSegmentRect(rowY, i, count);
        const bool aktiv = (i == selected);

        spr->fillSmoothRoundRect(r.x, r.y, r.w, r.h, 6, aktiv ? C_SEC_SYSTEM : BTN_OUTLINE);
        spr->fillSmoothRoundRect(r.x + 1, r.y + 1, r.w - 2, r.h - 2, 5, aktiv ? BTN_ACTIVE_FILL : BTN_FILL);

        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(aktiv ? BTN_ACTIVE_TEXT : BTN_TEXT);
        spr->drawString(values[i], r.x + r.w / 2, r.y + r.h / 2);
        spr->unloadFont();
    }
}

/**
 * @brief Zeichnet das Anzeige-Panel: oben die Röhrenwahl, unten die Einstellungen des VU-Meters.
 */
void Settings::drawPanelAppearance(Gfx_Sprite *spr)
{
    drawAppearanceSectionTitle(spr, APPEARANCE_SEC1_Y, SettingsKey::SETTINGS_APPEARANCE_MAGIC_EYE);
    drawAppearanceEyeTiles(spr);
    drawAppearanceHint(spr, APPEARANCE_HINT1_Y, SettingsKey::SETTINGS_APPEARANCE_HINT);

    drawAppearanceSectionTitle(spr, APPEARANCE_SEC2_Y, SettingsKey::SETTINGS_APPEARANCE_SECTION_VU);

    // Angegeben ist die Einschwingzeit bis 99 Prozent; Reihenfolge wie VuBallistics.
    static const char *const BALLISTIK[] = {"150 ms", "300 ms", "600 ms"};
    static_assert(sizeof(BALLISTIK) / sizeof(BALLISTIK[0]) == APPEARANCE_VU_MAX + 1,
                  "Zu jeder Trägheit muss eine Zeitangabe gehören");

    drawAppearanceSegments(spr, APPEARANCE_ROW1_Y, SettingsKey::SETTINGS_APPEARANCE_VU_BALLISTICS,
                           BALLISTIK, APPEARANCE_VU_MAX + 1, m_vuBallistics);
    drawAppearanceHint(spr, APPEARANCE_ROW1_Y + APPEARANCE_HINT_DY,
                       SettingsKey::SETTINGS_APPEARANCE_BALLISTICS_HINT);

    // Pegel unter Vollaussteuerung, bei dem der Zeiger 0 VU erreicht; Reihenfolge wie VU_ALIGNMENT_DB.
    static const char *const PEGEL[] = {"-2 dB", "-6 dB", "-10 dB", "-14 dB", "-18 dB"};
    static_assert(sizeof(PEGEL) / sizeof(PEGEL[0]) == APPEARANCE_SENS_MAX + 1,
                  "Zu jedem Bezugspegel muss eine Beschriftung gehören");

    drawAppearanceSegments(spr, APPEARANCE_ROW2_Y, SettingsKey::SETTINGS_APPEARANCE_VU_LEVEL,
                           PEGEL, APPEARANCE_SENS_MAX + 1, m_vuSensitivity);
    drawAppearanceHint(spr, APPEARANCE_ROW2_Y + APPEARANCE_HINT_DY,
                       SettingsKey::SETTINGS_APPEARANCE_LEVEL_HINT);

    // Das Panel hat die Pegelanzeige gerade überdeckt. Den gemerkten Text löschen, damit der
    // nächste Durchlauf sie wieder hinschreibt, auch wenn sich die Zahl nicht geändert hat.
    m_appearanceLevelText[0] = 0;
}

/**
 * @brief Verarbeitet Taps im Anzeige-Panel.
 */
void Settings::handleAppearanceTouch(int sx, int sy)
{
    for (int i = 0; i <= APPEARANCE_EYE_MAX; ++i)
    {
        const Rect t = appearanceTileRect(i);
        if (pointIn(sx, sy, t.x, t.y, t.w, t.h))
        {
            m_magicEyeStyle = static_cast<uint8_t>(i);
            m_workDirty = true;
            m_footerDirty = true;
            return;
        }
    }

    for (int i = 0; i <= APPEARANCE_VU_MAX; ++i)
    {
        const Rect r = appearanceSegmentRect(APPEARANCE_ROW1_Y, i, APPEARANCE_VU_MAX + 1);
        if (pointIn(sx, sy, r.x, r.y, r.w, r.h))
        {
            m_vuBallistics = static_cast<uint8_t>(i);
            m_workDirty = true;
            m_footerDirty = true;
            return;
        }
    }

    for (int i = 0; i <= APPEARANCE_SENS_MAX; ++i)
    {
        const Rect r = appearanceSegmentRect(APPEARANCE_ROW2_Y, i, APPEARANCE_SENS_MAX + 1);
        if (pointIn(sx, sy, r.x, r.y, r.w, r.h))
        {
            m_vuSensitivity = static_cast<uint8_t>(i);
            m_workDirty = true;
            m_footerDirty = true;
            return;
        }
    }
}

/**
 * @brief Meldet, ob eine der drei Einstellungen vom gespeicherten Stand abweicht.
 */
bool Settings::appearancePending() const
{
    return m_magicEyeStyle != m_magicEyeStyleBaseline || m_vuBallistics != m_vuBallisticsBaseline ||
           m_vuSensitivity != m_vuSensitivityBaseline;
}

/**
 * @brief Schreibt die Einstellungen der Anzeigeseite dauerhaft in AppState.
 */
void Settings::saveAppearance()
{
    APP_STATE.setMagicEyeStyle(m_magicEyeStyle);
    APP_STATE.setVuBallistics(m_vuBallistics);
    APP_STATE.setVuSensitivity(m_vuSensitivity);

    // SSE-Event an Web-Clients senden
    extern void sendRadioSSEEvent();
    sendRadioSSEEvent();

    m_magicEyeStyleBaseline = m_magicEyeStyle;
    m_vuBallisticsBaseline = m_vuBallistics;
    m_vuSensitivityBaseline = m_vuSensitivity;

    recomputeDirtyFlags();
    m_workDirty = true;

    showInfoBox(SettingsKey::SETTINGS_SAVED_TITLE, SettingsKey::SETTINGS_APPEARANCE_SAVED_MSG);
    LOG_SUCCESS_F("Settings: Anzeige gespeichert (magisches Auge %u, Zeigerträgheit %u, Empfindlichkeit %u)",
                  static_cast<unsigned>(m_magicEyeStyle), static_cast<unsigned>(m_vuBallistics),
                  static_cast<unsigned>(m_vuSensitivity));
}

// ===================================================================
// Listen-Picker (modal, blockierend)
// ===================================================================

/**
 * @brief Zeigt einen modalen Listen-Picker und liefert den gewählten Index.
 *
 * Blockiert wie Keyboard und WifiList, bis der Anwender einen Eintrag wählt oder abbricht. Während der Anzeige liegt der Picker mittig über dem Hauptlayout; nach der Rückkehr ist es Aufgabe des Aufrufers, afterModalRedraw() aufzurufen.
 */
int Settings::runListPicker(SettingsKey titleKey,
                            int itemCount,
                            ListPickerLabelFn labelFn,
                            int initialIndex)
{
    if (!m_tft || itemCount <= 0 || !labelFn)
        return -1;

    if (initialIndex < 0 || initialIndex >= itemCount)
        initialIndex = 0;

    int32_t tx = 0, ty = 0;

    // Auslöser-Touch sauber loslassen lassen
    while (m_tft->getTouch(&tx, &ty))
    {
        delay(10);
        yield();
    }
    delay(40);

    // Rahmen, Titel und Abbruch-Knopf einmalig direkt aufs Display zeichnen
    m_tft->fillSmoothRoundRect(PICK_X, PICK_Y, PICK_W, PICK_H, PICK_R, C_PICK_BORDER);
    m_tft->fillSmoothRoundRect(PICK_X + 2, PICK_Y + 2, PICK_W - 4, PICK_H - 4,
                               PICK_R - 2, C_PICK_BG);

    m_tft->loadFont(ComfortaaBold16);
    m_tft->setTextDatum(MC_DATUM);
    m_tft->setTextColor(C_TEXT, C_PICK_BG);
    m_tft->drawString(I18n::str(titleKey),
                      PICK_X + PICK_W / 2,
                      PICK_Y + PICK_PADDING + PICK_TITLE_H / 2);
    m_tft->unloadFont();

    constexpr int CANCEL_W = 160;
    const int cancelY = PICK_Y + PICK_H - PICK_PADDING - PICK_CANCEL_H;
    const int cancelX = PICK_X + (PICK_W - CANCEL_W) / 2;

    m_tft->fillSmoothRoundRect(cancelX, cancelY, CANCEL_W, PICK_CANCEL_H, 6, BTN_OUTLINE);
    m_tft->fillSmoothRoundRect(cancelX + 1, cancelY + 1, CANCEL_W - 2, PICK_CANCEL_H - 2,
                               5, BTN_FILL);
    m_tft->loadFont(LeagueGothicCond34);
    m_tft->setTextDatum(MC_DATUM);
    m_tft->setTextColor(BTN_TEXT, BTN_FILL);
    m_tft->drawString(I18n::str(CommonKey::COMMON_BTN_CANCEL),
                      cancelX + CANCEL_W / 2,
                      cancelY + PICK_CANCEL_H / 2);
    m_tft->unloadFont();

    // Listenbereich (zwischen Titel und Abbruch-Knopf)
    const int listX = PICK_X + PICK_PADDING;
    const int listY = PICK_Y + PICK_PADDING + PICK_TITLE_H + 8;
    const int listW = PICK_W - 2 * PICK_PADDING;
    const int listH = cancelY - listY - 12;

    Gfx_Sprite *listSpr = nullptr;
    if (!m_tft->safeCreateSprite(listSpr, listW, listH))
    {
        LOG_ERROR("Listen-Picker: Zeichenfläche konnte nicht angelegt werden");
        return -1;
    }
    listSpr->setTextDatum(ML_DATUM);

    const int itemStep = PICK_ITEM_H + PICK_ITEM_GAP;
    const int contentH = itemCount * itemStep - PICK_ITEM_GAP;
    int maxScroll = contentH - listH;
    if (maxScroll < 0)
        maxScroll = 0;

    int scrollOff = initialIndex * itemStep - listH / 2 + PICK_ITEM_H / 2;
    if (scrollOff < 0)
        scrollOff = 0;
    if (scrollOff > maxScroll)
        scrollOff = maxScroll;

    bool dirty = true;
    bool touching = false;
    int touchStartY = 0;
    int scrollStart = 0;
    bool moved = false;
    int lastTx = 0, lastTy = 0;
    int result = -1;
    bool finished = false;

    while (!finished)
    {
        if (dirty)
        {
            listSpr->clear(C_PICK_BG);
            listSpr->loadFont(ComfortaaBold16);
            listSpr->setTextDatum(ML_DATUM);

            int firstIdx = scrollOff / itemStep;
            if (firstIdx < 0)
                firstIdx = 0;
            int lastIdx = (scrollOff + listH) / itemStep;
            if (lastIdx > itemCount - 1)
                lastIdx = itemCount - 1;

            char buf[40];
            for (int i = firstIdx; i <= lastIdx; ++i)
            {
                const int yInList = i * itemStep - scrollOff;
                const bool sel = (i == initialIndex);
                const uint16_t bg = sel ? C_PICK_ITEM_SEL : C_PICK_ITEM_BG;
                listSpr->fillSmoothRoundRect(0, yInList, listW, PICK_ITEM_H, 5, bg);

                buf[0] = '\0';
                labelFn(i, buf, sizeof(buf));
                listSpr->setTextColor(sel ? C_TEXT_BRIGHT : C_TEXT);
                listSpr->drawString(buf, 14, yInList + PICK_ITEM_H / 2);
            }
            listSpr->unloadFont();
            listSpr->pushSprite(listX, listY);
            dirty = false;
        }

        const bool isDown = m_tft->getTouch(&tx, &ty);
        if (isDown)
        {
            lastTx = tx;
            lastTy = ty;
            if (!touching)
            {
                touching = true;
                touchStartY = ty;
                scrollStart = scrollOff;
                moved = false;
            }
            else
            {
                int dy = ty - touchStartY;
                if (dy < 0)
                    dy = -dy;
                if (!moved && dy > PICK_DRAG_THRESH)
                    moved = true;
                if (moved)
                {
                    int newScroll = scrollStart - (ty - touchStartY);
                    if (newScroll < 0)
                        newScroll = 0;
                    if (newScroll > maxScroll)
                        newScroll = maxScroll;
                    if (newScroll != scrollOff)
                    {
                        scrollOff = newScroll;
                        dirty = true;
                    }
                }
            }
        }
        else if (touching)
        {
            touching = false;
            if (!moved)
            {
                if (pointIn(lastTx, lastTy, listX, listY, listW, listH))
                {
                    const int relY = (lastTy - listY) + scrollOff;
                    const int hitIdx = relY / itemStep;
                    const int hitYInItem = relY % itemStep;
                    if (hitIdx >= 0 && hitIdx < itemCount && hitYInItem < PICK_ITEM_H)
                    {
                        result = hitIdx;
                        finished = true;
                    }
                }
                else if (pointIn(lastTx, lastTy, cancelX, cancelY, CANCEL_W, PICK_CANCEL_H))
                {
                    result = -1;
                    finished = true;
                }
            }
        }

        delay(15);
        yield();
    }

    m_tft->safeDeleteSprite(listSpr);

    // Abschließenden Release abwarten, damit der Hauptloop nicht den Picker-Tap erbt
    while (m_tft->getTouch(&tx, &ty))
    {
        delay(10);
        yield();
    }

    return result;
}
