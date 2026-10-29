#include "Breather.h"

#include <cmath>
#include <string.h>
#include "AppState.h"
#include "Clocks.h"
#include "I18n.h"
#include "Logger.h"
#include "Strings_breather.h"
#include "StringUtils.h"

// Fonts
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Layout-Konstanten und Farben
// ===================================================================

namespace
{
    // Display
    constexpr int DISPLAY_W = 800;

    // Sound
    constexpr const char *PHASE_SOUND_PATH = "/system/sounds/breather.mp3";

    // Auswahl-Bildschirm
    constexpr int SEL_HEADER_Y = 18;                                                                   ///< Y-Position der Hauptüberschrift
    constexpr int SEL_TILES_Y = 80;                                                                    ///< Obere Kante der Kachel-Reihe
    constexpr int SEL_TILE_W = 240;                                                                    ///< Kachelbreite
    constexpr int SEL_TILE_H = 160;                                                                    ///< Kachelhöhe
    constexpr int SEL_TILE_GAP_X = 15;                                                                 ///< Horizontaler Kachelabstand
    constexpr int SEL_TILE_GAP_Y = 15;                                                                 ///< Vertikaler Kachelabstand
    constexpr int SEL_TILE_COLS = 3;                                                                   ///< Spaltenanzahl
    constexpr int SEL_TILE_ROWS = 2;                                                                   ///< Zeilenanzahl
    constexpr int SEL_TILE_RADIUS = 10;                                                                ///< Eckenradius der Kacheln
    constexpr int SEL_TILE_ACCENT_W = 4;                                                               ///< Breite des Akzentstreifens am linken Kachelrand
    constexpr int SEL_TILE_TEXT_X = 16;                                                                ///< Linker Texteinzug innerhalb der Kachel (nach Akzentstreifen plus Padding)
    constexpr int SEL_TILE_TITLE_Y = 14;                                                               ///< Y-Position des Haupttitels innerhalb der Kachel
    constexpr int SEL_TILE_SUB_Y = 60;                                                                 ///< Y-Position des Stimmungs-Untertitels innerhalb der Kachel
    constexpr int SEL_TILE_PAD_R = 14;                                                                 ///< Rechter Innenabstand für die Dauer-Anzeige
    constexpr int SEL_TILE_PAD_B = 12;                                                                 ///< Unterer Innenabstand für die Dauer-Anzeige
    constexpr int SEL_TILE_GRID_W = SEL_TILE_COLS * SEL_TILE_W + (SEL_TILE_COLS - 1) * SEL_TILE_GAP_X; ///< Gesamte Breite des Kachelgitters
    constexpr int SEL_TILE_GRID_X = (DISPLAY_W - SEL_TILE_GRID_W) / 2;                                 ///< X-Startposition des Kachelgitters
    constexpr int SEL_TILE_COUNT = SEL_TILE_COLS * SEL_TILE_ROWS;                                      ///< Anzahl der Kacheln im Auswahl-Raster, muss gleich oder größer SESSION_COUNT sein

    // Laufende Sitzung
    constexpr int RUN_HEADER_Y = 14;                                 ///< Y-Position des Sitzungs-Titels
    constexpr int RUN_EX_INFO_Y = 50;                                ///< Y-Position der Übungs-Info "Übung x von y"
    constexpr int RUN_HEADER_SPRITE_H = 82;                          ///< Höhe des Header-Sprites
    constexpr int RUN_PROGRESS_Y = 84;                               ///< Y-Position des Fortschrittsbalkens
    constexpr int RUN_PROGRESS_X = 30;                               ///< X-Startposition Fortschrittsbalken
    constexpr int RUN_PROGRESS_W = DISPLAY_W - 2 * RUN_PROGRESS_X;   ///< Breite Fortschrittsbalken
    constexpr int RUN_PROGRESS_H = 6;                                ///< Höhe Fortschrittsbalken
    constexpr int RUN_VIZ_X = 65;                                    ///< Visualisierungs-Sprite links
    constexpr int RUN_VIZ_Y = 130;                                   ///< Visualisierungs-Sprite oben
    constexpr int RUN_VIZ_W = 280;                                   ///< Visualisierungs-Sprite Breite
    constexpr int RUN_VIZ_H = 280;                                   ///< Visualisierungs-Sprite Höhe
    constexpr int RUN_INSTR_X = 410;                                 ///< Anweisungstext rechts
    constexpr int RUN_INSTR_Y = 110;                                 ///< Anweisungstext oben
    constexpr int RUN_INSTR_W = 365;                                 ///< Anweisungstext Breite
    constexpr int RUN_INSTR_H = 250;                                 ///< Anweisungstext Höhe
    constexpr int RUN_TITLE_H = 36;                                  ///< Höhe des Titels im Anweisungs-Sprite
    constexpr int RUN_INSTR_TEXT_Y = 44;                             ///< Y-Position des Anweisungstextes im Anweisungs-Sprite
    constexpr int RUN_INSTR_TEXT_H = RUN_INSTR_H - RUN_INSTR_TEXT_Y; ///< Höhe des Anweisungstextes im Anweisungs-Sprite
    constexpr int RUN_REMAIN_X = RUN_INSTR_X;                        ///< Verbleibende Sekunden oben in der rechten Spalte
    constexpr int RUN_REMAIN_Y = 370;                                ///< Y-Position der verbleibenden Sekunden
    constexpr int RUN_REMAIN_W = 365;                                ///< Breite der Anzeige der verbleibenden Sekunden
    constexpr int RUN_REMAIN_H = 50;                                 ///< Höhe der Anzeige der verbleibenden Sekunden

    // Footer-Buttons im laufenden Bildschirm, drei gleich große Aktions-Buttons links gruppiert.
    constexpr int RUN_BTN_Y = 457;       ///< Y-Position der Buttons (Mitte)
    constexpr int RUN_BTN_H = 45;        ///< Höhe der Buttons
    constexpr int RUN_BTN_W = 130;       ///< Breite der Buttons
    constexpr int RUN_BTN_RADIUS = 4;    ///< Eckenradius der Buttons
    constexpr int RUN_BTN_GAP = 8;       ///< Horizontaler Abstand zwischen benachbarten Buttons
    constexpr int RUN_BTN_GROUP_X = 20;  ///< Linke Kante der Button-Gruppe
    constexpr int RUN_BTN_BACK_X = RUN_BTN_GROUP_X + RUN_BTN_W / 2;                                       ///< X-Position des Zurück-Buttons (Mitte)
    constexpr int RUN_BTN_PAUSE_X = RUN_BTN_BACK_X + RUN_BTN_W + RUN_BTN_GAP;                             ///< X-Position des Pause/Weiter-Buttons (Mitte)
    constexpr int RUN_BTN_SKIP_X = RUN_BTN_PAUSE_X + RUN_BTN_W + RUN_BTN_GAP;                             ///< X-Position des Skip-Buttons (Mitte)

    // Exit-Button
    constexpr int16_t EXIT_BTN_X = 765;     ///< X-Position des Exit-Buttons
    constexpr int16_t EXIT_BTN_Y = 457;     ///< Y-Position des Exit-Buttons
    constexpr uint16_t EXIT_BTN_SIZE = 45;  ///< Breite und Höhe des quadratischen Exit-Buttons
    constexpr uint16_t EXIT_BTN_RADIUS = 4; ///< Eckenradius des Exit-Buttons

    // Done-Bildschirm
    constexpr int DONE_BTN_X = DISPLAY_W / 2;           ///< X-Position des Done-Buttons
    constexpr int DONE_BTN_Y = 360;                     ///< Y-Position des Done-Buttons
    constexpr int DONE_BTN_W = 220;                     ///< Breite des Done-Buttons
    constexpr int DONE_BTN_H = 45;                      ///< Höhe des Done-Buttons
    constexpr int DONE_BTN_RADIUS = 4;                  ///< Eckenradius des Done-Buttons
    constexpr unsigned long DONE_AUTO_RETURN_MS = 5000; ///< Automatischer Rücksprung zur Auswahl

    // Digitaluhr - Position rechts unten, links vom Exit-Button
    constexpr int CLOCK_X = 625;           ///< X-Position der Digitaluhr
    constexpr int CLOCK_Y_FROM_BOTTOM = 1; ///< Y-Position der Digitaluhr vom unteren Bildschirmrand aus
    constexpr int CLOCK_W = 103;           ///< Breite der Digitaluhr
    constexpr int CLOCK_H = 45;            ///< Höhe der Digitaluhr

    // Visualisierung im Sprite
    constexpr int VIZ_CENTER_X = RUN_VIZ_W / 2;      ///< X-Position des Visualisierungs-Zentrums im Sprite
    constexpr int VIZ_CENTER_Y = RUN_VIZ_H / 2;      ///< Y-Position des Visualisierungs-Zentrums im Sprite
    constexpr float VIZ_BREATH_RADIUS_MIN = 50.0f;     ///< Minimaler Radius des Atemkreises (bei Einatmen)
    constexpr float VIZ_BREATH_RADIUS_MAX = 128.0f;    ///< Maximaler Radius des Atemkreises (bei Ausatmen)
    constexpr float VIZ_BREATH_MARKER_RADIUS = 128.0f; ///< Radius des Markierungs-Rings am äußeren Atemkreis
    constexpr float VIZ_PENDULUM_AMPLITUDE = 110.0f; ///< Amplitude der Pendelbewegung (links/rechts) bei den Bewegungsübungen
    constexpr float VIZ_BOB_AMPLITUDE = 100.0f;      ///< Amplitude der Auf/Ab-Bewegung bei den Bewegungsübungen
    constexpr float VIZ_ROT_RADIUS = 100.0f;         ///< Radius der rotierenden Punkte bei den Bewegungsübungen
    constexpr float VIZ_DOT_RADIUS = 22.0f;          ///< Radius der Punkte bei den Bewegungsübungen
    constexpr float VIZ_EYE_NEAR_RADIUS = 28.0f;     ///< Radius des inneren Kreises bei der Augenübung "Fern und Nah"
    constexpr float VIZ_EYE_FAR_RADIUS = 110.0f;     ///< Radius des äußeren Kreises bei der Augenübung "Fern und Nah"
    constexpr float VIZ_EYE_PALMING_RADIUS = 80.0f;  ///< Radius des Kreises bei der Übung "Augen abdunkeln"
    constexpr float VIZ_REST_RADIUS = 38.0f;         ///< Radius des Kreises bei Ruhephasen zwischen den Übungen

    // Animierte Kugel als eigenes Sprite, einmal pro Übung mit AA gezeichnet
    constexpr int DOT_SPRITE_SIZE = 52;            ///< Kantenlänge des Kugel-Sprites in Pixeln
    constexpr int DOT_SPRITE_HALF = DOT_SPRITE_SIZE / 2; ///< Halbe Kantenlänge, also Mittelpunkt des Sprites
    constexpr float DOT_SMOOTH_THICKNESS = 1.5f;   ///< Dicke des AA-Rings beim Vorzeichnen der Kugel

    // Atemphasen
    constexpr uint16_t BOX_PHASE_SECS[4] = {4, 4, 4, 4};                                      ///< Dauer jeder Phase in Sekunden
    constexpr const char *BOX_PHASE_LABELS[4] = {"Einatmen", "Halten", "Ausatmen", "Halten"}; ///< Beschriftung der Phasen
    constexpr uint16_t T478_PHASE_SECS[3] = {4, 7, 8};                                        ///< Dauer jeder Phase in Sekunden
    constexpr const char *T478_PHASE_LABELS[3] = {"Einatmen", "Halten", "Ausatmen"};          ///< Beschriftung der Phasen

    // Mathematik
    constexpr float TWO_PI_F = 6.28318530718f; ///< 2 * Pi als Float

    // Farben - Hintergrund und Text, abgestimmt auf die Workshop-Palette
    constexpr uint16_t COLOR_BG = 0x1082;              ///< Hintergrund, Workshop-Anthrazit
    constexpr uint16_t COLOR_TILE_BG = 0x2104;         ///< Karten-Hintergrund auf dem Auswahl-Bildschirm
    constexpr uint16_t COLOR_TILE_BG_PRESSED = 0x39E7; ///< Karten-Hintergrund während eines Tap-Drucks (heller)
    constexpr uint16_t COLOR_TEXT_BRIGHT = 0xFFFF;     ///< Heller Text für Haupttitel
    constexpr uint16_t COLOR_TEXT = 0xDEDB;            ///< Standard-Textfarbe
    constexpr uint16_t COLOR_TEXT_DIM = 0xAD75;        ///< Gedimmter Text für Untertitel und Zusatzinfo
    constexpr uint16_t COLOR_SEP_LINE = 0x2945;        ///< Trennlinien und gedämpfte Konturen
    constexpr uint16_t COLOR_BUTTON_BORDER = 0x39E7;   ///< Rahmenfarbe der Footer-Buttons

    // Akzentfarben pro Sitzung, übernommen aus den Workshop-Sektionsfarben
    constexpr uint16_t COLOR_ACCENT_PLUM = 0xC1AC;   ///< Pflaume, "Bin angespannt"
    constexpr uint16_t COLOR_ACCENT_GOLD = 0xFEA0;   ///< Gold, "Bin müde"
    constexpr uint16_t COLOR_ACCENT_PETROL = 0x07F4; ///< Petrol, "Bin zappelig"
    constexpr uint16_t COLOR_ACCENT_CYAN = 0x04FF;   ///< Cyan-Blau, "Augen-Pause"
    constexpr uint16_t COLOR_ACCENT_GREEN = 0x07E0;  ///< Grün, "Quadratatmung"
    constexpr uint16_t COLOR_ACCENT_ORANGE = 0xFD20; ///< Akzent-Orange, "Atemberuhigung"

    // Visualisierungs-Farben für die Übungs-Animation
    constexpr uint16_t COLOR_VIZ_PRIMARY = 0xC52E;   ///< Hauptfarbe (Brass) für Atemkreise
    constexpr uint16_t COLOR_VIZ_SECONDARY = 0x74D0; ///< Sekundärfarbe (TealMid) für bewegte Kugel und Augenwechsel
    constexpr uint16_t COLOR_VIZ_REST = 0x5C44;      ///< Avocado für Ruhephasen
    constexpr uint16_t COLOR_VIZ_PALM = 0x18C3;      ///< Sehr dunkel für Palming-Symbol

    // ===================================================================
    // Übungs-Bibliothek
    // ===================================================================

    // Reihenfolge: pro Sitzung ein eigenes Array, gefolgt von einem Eintrag in s_sessions. Die Reihenfolge in SESSIONS bestimmt die Position im 3x2-Auswahl-Raster: obere Reihe (Indizes 0..2) sind nach Stimmung gegliedert, untere Reihe (3..5) enthält gezielte Übungen.
    //
    // Pro Sprache ein paralleles _DE/_EN-Paar. Layout-Felder (kind, durationSec, accentColor, totalDurationSec) sind identisch, nur die anwender-sichtbaren Strings unterscheiden sich. `activeSessions()` wählt zur Laufzeit anhand der UI-Sprache den passenden Pointer.

    constexpr BreatherExercise EX_TENSE_DE[] = {
        {ExerciseKind::BREATHING_478, "Tief einatmen", "4 Sekunden einatmen.\n7 Sekunden halten.\n8 Sekunden ausatmen.\nDie Schultern sinken.", 60},
        {ExerciseKind::NECK_TILT, "Nacken seitlich", "Kopf langsam zur Seite neigen.\nSchulter unten lassen.\nNicht ziehen, nur fallen lassen.", 45},
        {ExerciseKind::SHOULDER_ROLL, "Schulterkreisen", "Schultern locker kreisen.\nVorwärts und rückwärts wechseln.\nMitatmen.", 45},
        {ExerciseKind::BREATHING_478, "Atem zum Ausklang", "Noch einmal ruhig\nim 4-7-8-Takt atmen.\nDem Kreis folgen.", 30},
    };

    constexpr BreatherExercise EX_TIRED_DE[] = {
        {ExerciseKind::BREATHING_BOX, "Atem ankommen lassen", "4 Sekunden einatmen.\n4 Sekunden halten.\n4 Sekunden ausatmen.\n4 Sekunden halten.", 45},
        {ExerciseKind::STAND_AND_REACH, "Aufstehen und strecken", "Aufstehen und die Arme\nweit nach oben strecken.\nFingerspitzen zur Decke.", 60},
        {ExerciseKind::SHOULDER_LIFT, "Schultern wecken", "Schultern Richtung Ohren ziehen.\nKurz halten.\nDann loslassen.", 15},
    };

    constexpr BreatherExercise EX_RESTLESS_DE[] = {
        {ExerciseKind::BREATHING_478, "Atem ankommen lassen", "4 Sekunden einatmen.\n7 Sekunden halten.\n8 Sekunden ausatmen.", 60},
        {ExerciseKind::SPINE_TWIST, "Wirbelsäule drehen", "Im Sitzen sanft\nzur Seite drehen.\nLinks und rechts wechseln.", 60},
        {ExerciseKind::BREATHING_BOX, "Atem zum Ausklang", "Ruhig im 4-4-4-4-Takt atmen.\nDem Kreis folgen.", 60},
    };

    constexpr BreatherExercise EX_EYES_DE[] = {
        {ExerciseKind::EYE_PALMING, "Augen abdunkeln", "Hände aneinander reiben\nund die warmen Handflächen\nvor die geschlossenen Augen legen.", 30},
        {ExerciseKind::EYE_FAR_NEAR, "Fern und Nah", "Erst in die Ferne schauen,\ndann auf einen nahen Punkt,\nz. B. den Daumen.", 30},
        {ExerciseKind::EYE_CIRCLE, "Augen kreisen", "Augen langsam kreisen lassen.\nEinmal links herum,\neinmal rechts herum.", 30},
        {ExerciseKind::EYE_PALMING, "Augen abdunkeln", "Noch einmal die warmen Handflächen\nvor die Augen legen.\nTief durchatmen.", 30},
    };

    constexpr BreatherExercise EX_BOX_DE[] = {
        {ExerciseKind::BREATHING_BOX, "Quadratatmung", "4 Sekunden einatmen.\n4 Sekunden halten.\n4 Sekunden ausatmen.\n4 Sekunden halten.\n\nDem Kreis folgen.", 180},
    };

    constexpr BreatherExercise EX_478_DE[] = {
        {ExerciseKind::BREATHING_478, "Atemberuhigung", "4 Sekunden einatmen.\n7 Sekunden halten.\n8 Sekunden ausatmen.\n\nDem Kreis folgen.", 180},
    };

    constexpr BreatherExercise EX_TENSE_EN[] = {
        {ExerciseKind::BREATHING_478, "Deep breathing", "Breathe in for 4 seconds.\nHold for 7 seconds.\nBreathe out for 8 seconds.\nLet your shoulders drop.", 60},
        {ExerciseKind::NECK_TILT, "Side neck tilt", "Tilt your head slowly to the side.\nKeep your shoulder low.\nDon't pull, just let it fall.", 45},
        {ExerciseKind::SHOULDER_ROLL, "Shoulder rolls", "Roll your shoulders loosely.\nAlternate forwards and backwards.\nBreathe along.", 45},
        {ExerciseKind::BREATHING_478, "Closing breath", "One more calm round\nin the 4-7-8 rhythm.\nFollow the circle.", 30},
    };

    constexpr BreatherExercise EX_TIRED_EN[] = {
        {ExerciseKind::BREATHING_BOX, "Settle the breath", "Breathe in for 4 seconds.\nHold for 4 seconds.\nBreathe out for 4 seconds.\nHold for 4 seconds.", 45},
        {ExerciseKind::STAND_AND_REACH, "Stand and reach", "Stand up and stretch\nyour arms high overhead.\nFingertips to the ceiling.", 60},
        {ExerciseKind::SHOULDER_LIFT, "Wake the shoulders", "Pull your shoulders up to your ears.\nHold briefly.\nThen let go.", 15},
    };

    constexpr BreatherExercise EX_RESTLESS_EN[] = {
        {ExerciseKind::BREATHING_478, "Settle the breath", "Breathe in for 4 seconds.\nHold for 7 seconds.\nBreathe out for 8 seconds.", 60},
        {ExerciseKind::SPINE_TWIST, "Spine twist", "Sitting upright, gently\nturn to the side.\nAlternate left and right.", 60},
        {ExerciseKind::BREATHING_BOX, "Closing breath", "Breathe calmly\nin the 4-4-4-4 rhythm.\nFollow the circle.", 60},
    };

    constexpr BreatherExercise EX_EYES_EN[] = {
        {ExerciseKind::EYE_PALMING, "Cover the eyes", "Rub your hands together\nand place the warm palms\nover your closed eyes.", 30},
        {ExerciseKind::EYE_FAR_NEAR, "Far and near", "First look into the distance,\nthen at a close point,\ne.g. your thumb.", 30},
        {ExerciseKind::EYE_CIRCLE, "Eye circles", "Slowly circle your eyes.\nOnce to the left,\nonce to the right.", 30},
        {ExerciseKind::EYE_PALMING, "Cover the eyes", "Once more place the warm palms\nover your eyes.\nBreathe deeply.", 30},
    };

    constexpr BreatherExercise EX_BOX_EN[] = {
        {ExerciseKind::BREATHING_BOX, "Box breathing", "Breathe in for 4 seconds.\nHold for 4 seconds.\nBreathe out for 4 seconds.\nHold for 4 seconds.\n\nFollow the circle.", 180},
    };

    constexpr BreatherExercise EX_478_EN[] = {
        {ExerciseKind::BREATHING_478, "Calming breath", "Breathe in for 4 seconds.\nHold for 7 seconds.\nBreathe out for 8 seconds.\n\nFollow the circle.", 180},
    };

    static_assert(sizeof(EX_TENSE_EN) / sizeof(EX_TENSE_EN[0])
                  == sizeof(EX_TENSE_DE) / sizeof(EX_TENSE_DE[0]),
                  "EX_TENSE_EN length mismatch");
    static_assert(sizeof(EX_TIRED_EN) / sizeof(EX_TIRED_EN[0])
                  == sizeof(EX_TIRED_DE) / sizeof(EX_TIRED_DE[0]),
                  "EX_TIRED_EN length mismatch");
    static_assert(sizeof(EX_RESTLESS_EN) / sizeof(EX_RESTLESS_EN[0])
                  == sizeof(EX_RESTLESS_DE) / sizeof(EX_RESTLESS_DE[0]),
                  "EX_RESTLESS_EN length mismatch");
    static_assert(sizeof(EX_EYES_EN) / sizeof(EX_EYES_EN[0])
                  == sizeof(EX_EYES_DE) / sizeof(EX_EYES_DE[0]),
                  "EX_EYES_EN length mismatch");
    static_assert(sizeof(EX_BOX_EN) / sizeof(EX_BOX_EN[0])
                  == sizeof(EX_BOX_DE) / sizeof(EX_BOX_DE[0]),
                  "EX_BOX_EN length mismatch");
    static_assert(sizeof(EX_478_EN) / sizeof(EX_478_EN[0])
                  == sizeof(EX_478_DE) / sizeof(EX_478_DE[0]),
                  "EX_478_EN length mismatch");

    constexpr BreatherSession SESSIONS_DE[] = {
        {"Bin angespannt", "Atem und Schultern lockern", 180, COLOR_ACCENT_PLUM, EX_TENSE_DE, sizeof(EX_TENSE_DE) / sizeof(EX_TENSE_DE[0])},
        {"Bin müde", "Wach werden, locker bleiben", 120, COLOR_ACCENT_GOLD, EX_TIRED_DE, sizeof(EX_TIRED_DE) / sizeof(EX_TIRED_DE[0])},
        {"Bin zappelig", "Zur Ruhe kommen", 180, COLOR_ACCENT_PETROL, EX_RESTLESS_DE, sizeof(EX_RESTLESS_DE) / sizeof(EX_RESTLESS_DE[0])},
        {"Augen-Pause", "Die Augen entlasten", 120, COLOR_ACCENT_CYAN, EX_EYES_DE, sizeof(EX_EYES_DE) / sizeof(EX_EYES_DE[0])},
        {"Quadratatmung", "Atem im 4-4-4-4-Takt", 180, COLOR_ACCENT_GREEN, EX_BOX_DE, sizeof(EX_BOX_DE) / sizeof(EX_BOX_DE[0])},
        {"Atemberuhigung", "Atem im 4-7-8-Takt", 180, COLOR_ACCENT_ORANGE, EX_478_DE, sizeof(EX_478_DE) / sizeof(EX_478_DE[0])},
    };

    constexpr BreatherSession SESSIONS_EN[] = {
        {"I'm tense", "Loosen breath and shoulders", 180, COLOR_ACCENT_PLUM, EX_TENSE_EN, sizeof(EX_TENSE_EN) / sizeof(EX_TENSE_EN[0])},
        {"I'm tired", "Wake up, stay relaxed", 120, COLOR_ACCENT_GOLD, EX_TIRED_EN, sizeof(EX_TIRED_EN) / sizeof(EX_TIRED_EN[0])},
        {"I'm restless", "Settle down", 180, COLOR_ACCENT_PETROL, EX_RESTLESS_EN, sizeof(EX_RESTLESS_EN) / sizeof(EX_RESTLESS_EN[0])},
        {"Eye break", "Rest your eyes", 120, COLOR_ACCENT_CYAN, EX_EYES_EN, sizeof(EX_EYES_EN) / sizeof(EX_EYES_EN[0])},
        {"Box breathing", "Breath in 4-4-4-4 rhythm", 180, COLOR_ACCENT_GREEN, EX_BOX_EN, sizeof(EX_BOX_EN) / sizeof(EX_BOX_EN[0])},
        {"Calming breath", "Breath in 4-7-8 rhythm", 180, COLOR_ACCENT_ORANGE, EX_478_EN, sizeof(EX_478_EN) / sizeof(EX_478_EN[0])},
    };

    constexpr uint8_t SESSION_COUNT = sizeof(SESSIONS_DE) / sizeof(SESSIONS_DE[0]);

    static_assert(SESSION_COUNT <= SEL_TILE_COUNT, "Mehr Sessions als Kacheln im Auswahl-Raster - nicht alle Sessions wären erreichbar.");
    static_assert(sizeof(SESSIONS_EN) / sizeof(SESSIONS_EN[0]) == SESSION_COUNT,
                  "SESSIONS_EN length mismatch");

    /**
     * @brief Liefert die zur aktuell aktiven UI-Sprache passende Sitzungs-Tabelle.
     *
     * Beide Tabellen sind strukturell identisch, nur die anwender-sichtbaren Strings (Sitzungs-Titel, Untertitel, Übungs-Titel, Anweisungstexte) unterscheiden sich. Dauer, Akzentfarbe, Übungs-Bauart und Übungs-Anzahl sind in beiden Sprachen gleich. Für Logger-Ausgaben (immer deutsch) wird direkt `SESSIONS_DE` verwendet.
     */
    const BreatherSession *activeSessions()
    {
        return (I18n::currentLang() == LangIndex::EN) ? SESSIONS_EN : SESSIONS_DE;
    }

    /**
     * @brief Liefert das übersetzte Phasen-Label zu einem deutschen Phasen-String.
     *
     * Wird in `renderBreathingCircle` aufgerufen, um die Tabellen-Einträge aus `BOX_PHASE_LABELS` / `T478_PHASE_LABELS` (immer deutsch) in die aktuell aktive Sprache zu übersetzen, bevor das Label im Atemkreis-Zentrum gezeichnet wird. Fallback bei unbekanntem Eingangstext ist der Eingangstext selbst.
     */
    const char *localizedPhaseLabel(const char *deLabel)
    {
        if (!deLabel || !deLabel[0])
            return "";
        if (strcmp(deLabel, "Einatmen") == 0)
            return reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_PHASE_INHALE));
        if (strcmp(deLabel, "Halten") == 0)
            return reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_PHASE_HOLD));
        if (strcmp(deLabel, "Ausatmen") == 0)
            return reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_PHASE_EXHALE));
        return deLabel;
    }

    // ===================================================================
    // Hilfsfunktionen für die Visualisierung
    // ===================================================================

    /**
     * @brief Bestimmt für eine Phasen-Sequenz die aktive Phase und den Phasenfortschritt.
     * @param elapsedMs Verstrichene Millisekunden seit Übungsstart
     * @param phaseSecsArr Phasen-Längen in Sekunden
     * @param phaseCount Anzahl Phasen
     * @param outPhase Ausgabe: aktive Phase im Zyklus
     * @param outPhaseProgress Ausgabe: Fortschritt 0.0..1.0 innerhalb der Phase
     */
    void resolveCyclicPhase(uint32_t elapsedMs, const uint16_t *phaseSecsArr, uint8_t phaseCount, uint8_t &outPhase, float &outPhaseProgress)
    {
        uint32_t cycleMs = 0;
        for (uint8_t i = 0; i < phaseCount; ++i)
            cycleMs += static_cast<uint32_t>(phaseSecsArr[i]) * 1000UL;

        if (cycleMs == 0)
        {
            outPhase = 0;
            outPhaseProgress = 0.0f;
            return;
        }

        const uint32_t inCycleMs = elapsedMs % cycleMs;
        uint32_t accum = 0;

        for (uint8_t i = 0; i < phaseCount; ++i)
        {
            const uint32_t phaseMs = static_cast<uint32_t>(phaseSecsArr[i]) * 1000UL;

            if (inCycleMs < accum + phaseMs)
            {
                outPhase = i;
                outPhaseProgress = static_cast<float>(inCycleMs - accum) / static_cast<float>(phaseMs);
                return;
            }

            accum += phaseMs;
        }

        outPhase = phaseCount - 1;
        outPhaseProgress = 1.0f;
    }

    /**
     * @brief Liefert die nächste Zeile eines mehrzeiligen Anweisungstextes und kopiert sie in einen Puffer. Liefert false, wenn keine weitere Zeile vorliegt.
     */
    bool nextLine(const char *&cursor, char *out, size_t outSize)
    {
        if (!cursor || *cursor == '\0' || outSize == 0)
            return false;

        size_t i = 0;

        while (*cursor != '\0' && *cursor != '\n' && i + 1 < outSize)
            out[i++] = *cursor++;

        out[i] = '\0';

        if (*cursor == '\n')
            cursor++;

        return true;
    }
}

// ===================================================================
// Konstruktor / Destruktor
// ===================================================================

/**
 * @brief Konstruktor
 */
Breather::Breather()
{
}

/**
 * @brief Destruktor
 */
Breather::~Breather()
{
    unloadVizFont();

    if (m_tft)
    {
        m_tft->safeDeleteSprite(m_vizSprite);
        m_tft->safeDeleteSprite(m_dotSprite);
        m_tft->safeDeleteSprite(m_headerSprite);
        m_tft->safeDeleteSprite(m_progressSprite);
        m_tft->safeDeleteSprite(m_titleSprite);
        m_tft->safeDeleteSprite(m_instructionSprite);
        m_tft->safeDeleteSprite(m_remainingSprite);
        m_tft->safeDeleteSprite(m_clockSprite);
    }
}

// ===================================================================
// Initialisierung
// ===================================================================

/**
 * @brief Initialisiert das Breather-Modul
 */
bool Breather::init(LGFX &tft, const BreatherCallbacks &callbacks)
{
    m_tft = &tft;
    m_callbacks = callbacks;

    // Visualisierungs-Sprite für den statischen Hintergrund der animierten Übung
    if (!m_tft->safeCreateSprite(m_vizSprite, RUN_VIZ_W, RUN_VIZ_H))
    {
        LOG_ERROR("Breather: Visualisierungs-Sprite konnte nicht erstellt werden");
        return false;
    }

    // Kugel-Sprite für den animierten Punkt
    if (!m_tft->safeCreateSprite(m_dotSprite, DOT_SPRITE_SIZE, DOT_SPRITE_SIZE))
    {
        LOG_ERROR("Breather: Kugel-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_vizSprite);
        return false;
    }

    if (!m_tft->safeCreateSprite(m_headerSprite, DISPLAY_W, RUN_HEADER_SPRITE_H))
    {
        LOG_ERROR("Breather: Header-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_vizSprite);
        m_tft->safeDeleteSprite(m_dotSprite);
        return false;
    }

    if (!m_tft->safeCreateSprite(m_progressSprite, RUN_PROGRESS_W + 2, RUN_PROGRESS_H + 2))
    {
        LOG_ERROR("Breather: Fortschritts-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_vizSprite);
        m_tft->safeDeleteSprite(m_dotSprite);
        m_tft->safeDeleteSprite(m_headerSprite);
        return false;
    }

    if (!m_tft->safeCreateSprite(m_titleSprite, RUN_INSTR_W, RUN_TITLE_H))
    {
        LOG_ERROR("Breather: Titel-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_vizSprite);
        m_tft->safeDeleteSprite(m_dotSprite);
        m_tft->safeDeleteSprite(m_headerSprite);
        m_tft->safeDeleteSprite(m_progressSprite);
        return false;
    }

    if (!m_tft->safeCreateSprite(m_instructionSprite, RUN_INSTR_W, RUN_INSTR_TEXT_H))
    {
        LOG_ERROR("Breather: Anweisungs-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_vizSprite);
        m_tft->safeDeleteSprite(m_dotSprite);
        m_tft->safeDeleteSprite(m_headerSprite);
        m_tft->safeDeleteSprite(m_progressSprite);
        m_tft->safeDeleteSprite(m_titleSprite);
        return false;
    }

    if (!m_tft->safeCreateSprite(m_remainingSprite, RUN_REMAIN_W, RUN_REMAIN_H))
    {
        LOG_ERROR("Breather: Restzeit-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_vizSprite);
        m_tft->safeDeleteSprite(m_dotSprite);
        m_tft->safeDeleteSprite(m_headerSprite);
        m_tft->safeDeleteSprite(m_progressSprite);
        m_tft->safeDeleteSprite(m_titleSprite);
        m_tft->safeDeleteSprite(m_instructionSprite);
        return false;
    }

    if (!m_tft->safeCreateSprite(m_clockSprite, CLOCK_W, CLOCK_H))
    {
        LOG_ERROR("Breather: Uhr-Sprite konnte nicht erstellt werden");
        m_tft->safeDeleteSprite(m_vizSprite);
        m_tft->safeDeleteSprite(m_dotSprite);
        m_tft->safeDeleteSprite(m_headerSprite);
        m_tft->safeDeleteSprite(m_progressSprite);
        m_tft->safeDeleteSprite(m_titleSprite);
        m_tft->safeDeleteSprite(m_instructionSprite);
        m_tft->safeDeleteSprite(m_remainingSprite);
        return false;
    }

    m_clockSprite->clear(COLOR_BG);

    // Exit-Button ist immer sichtbar
    m_exitButton.initButton(
        m_tft,                  // Display-Objekt
        EXIT_BTN_X,             // X-Position in Button Mitte
        EXIT_BTN_Y,             // Y-Position in Button Mitte
        EXIT_BTN_SIZE,          // Breite
        EXIT_BTN_SIZE,          // Höhe
        EXIT_BTN_RADIUS,        // Eckenradius
        COLOR_BUTTON_BORDER,    // Rahmenfarbe
        COLOR_BG,               // Hintergrundfarbe
        COLOR_TEXT_DIM,         // Textfarbe
        "^POWER:32:D206^",      // Beschriftung
        nullptr                 // kein Icon
    );

    m_exitButton.setFont(LeagueGothicCond34);

    // Zurück zur Sitzungs-Auswahl
    m_backButton.initButton(
        m_tft,               // Display-Objekt
        RUN_BTN_BACK_X,      // X-Position in Button Mitte
        RUN_BTN_Y,           // Y-Position in Button Mitte
        RUN_BTN_W,           // Breite
        RUN_BTN_H,           // Höhe
        RUN_BTN_RADIUS,      // Eckenradius
        COLOR_BUTTON_BORDER, // Rahmenfarbe
        COLOR_BG,            // Hintergrundfarbe
        COLOR_TEXT,          // Textfarbe
        reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_BTN_BACK)), // Beschriftung
        nullptr              // kein Icon
    );

    m_backButton.setFont(LeagueGothicCond34);

    // Pause / Weiter
    m_pauseButton.initButton(
        m_tft,               // Display-Objekt
        RUN_BTN_PAUSE_X,     // X-Position in Button Mitte
        RUN_BTN_Y,           // Y-Position in Button Mitte
        RUN_BTN_W,           // Breite
        RUN_BTN_H,           // Höhe
        RUN_BTN_RADIUS,      // Eckenradius
        COLOR_BUTTON_BORDER, // Rahmenfarbe
        COLOR_BG,            // Hintergrundfarbe
        COLOR_TEXT,          // Textfarbe
        reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_BTN_PAUSE)), // Beschriftung
        nullptr              // kein Icon
    );

    m_pauseButton.setFont(LeagueGothicCond34);

    // Übung überspringen
    m_skipButton.initButton(
        m_tft,               // Display-Objekt
        RUN_BTN_SKIP_X,      // X-Position in Button Mitte
        RUN_BTN_Y,           // Y-Position in Button Mitte
        RUN_BTN_W,           // Breite
        RUN_BTN_H,           // Höhe
        RUN_BTN_RADIUS,      // Eckenradius
        COLOR_BUTTON_BORDER, // Rahmenfarbe
        COLOR_BG,            // Hintergrundfarbe
        COLOR_TEXT,          // Textfarbe
        reinterpret_cast<const char *>(STR(CommonKey::COMMON_BTN_CONTINUE)), // Beschriftung
        nullptr              // kein Icon
    );

    m_skipButton.setFont(LeagueGothicCond34);

    // Done
    m_doneButton.initButton(
        m_tft,               // Display-Objekt
        DONE_BTN_X,          // X-Position in Button Mitte
        DONE_BTN_Y,          // Y-Position in Button Mitte
        DONE_BTN_W,          // Breite
        DONE_BTN_H,          // Höhe
        DONE_BTN_RADIUS,     // Eckenradius
        COLOR_BUTTON_BORDER, // Rahmenfarbe
        COLOR_BG,            // Hintergrundfarbe
        COLOR_TEXT_BRIGHT,   // Textfarbe
        reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_BTN_DONE)), // Beschriftung
        nullptr              // kein Icon
    );

    m_doneButton.setFont(LeagueGothicCond34);

    // Display vorbereiten
    m_tft->clear(COLOR_BG);
    m_screen = BreatherScreen::SELECTION;
    m_screenChanged = true;
    m_lastDrawnSecond = -1;
    m_touchHandover = true; // Bei jedem Modul-Eintritt scharfschalten, sonst kann ein gehaltener Touch vom MainScreen sofort eine Kachel auswählen.

    m_initialized = true;
    LOG_SUCCESS("Breather-Modul erfolgreich initialisiert");
    return true;
}

/**
 * @brief Modultyp
 */
AppModule Breather::type() const
{
    return AppModule::BREATHER;
}

// ===================================================================
// Sprachwechsel
// ===================================================================

/**
 * @brief Vollredraw nach Sprachwechsel
 *
 * Wird vom `consumeLanguageChanged()`-Hook in `draw()` aufgerufen. Setzt alle Button-Labels neu, invalidiert die Change-Detection-Caches und erzwingt einen vollständigen Bildschirm-Aufbau über `m_screenChanged`. Der bestehende `m_screenChanged`-Pfad in `draw()` erledigt anschließend das eigentliche Neuzeichnen.
 *
 * Die Atemphasen-Labels in den Visualisierungs-Sprites werden nicht direkt neu gezeichnet, sondern über den invalidierten `m_lastBreathPhase`-Cache beim nächsten Atemkreis-Frame.
 */
void Breather::redrawAll()
{
    if (!m_tft)
        return;

    // Button-Labels in der neuen Sprache neu setzen
    m_backButton.setLabelText(reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_BTN_BACK)));
    m_pauseButton.setLabelText(reinterpret_cast<const char *>(
        m_pauseLabelIsResume ? STR(BreatherKey::BREATHER_BTN_RESUME)
                             : STR(BreatherKey::BREATHER_BTN_PAUSE)));
    m_skipButton.setLabelText(reinterpret_cast<const char *>(STR(CommonKey::COMMON_BTN_CONTINUE)));
    m_doneButton.setLabelText(reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_BTN_DONE)));

    // Bildschirm-Aufbau forcieren - der bestehende m_screenChanged-Pfad in draw() macht den Rest
    m_screenChanged = true;

    // Change-Detection-Caches invalidieren, damit alle Sprite-Inhalte neu gezeichnet werden
    m_lastDrawnPressedTile = -1;
    m_lastDisplayedExercise = 0xFF;
    m_lastDisplayedRemaining = -1;
    m_lastDisplayedProgress = -1;
    m_vizStaticDrawn = false;
    m_vizStaticInitialized = false;
    m_lastBreathRadius = -1;
    m_lastBreathPhase = 0xFF;
    m_eyeFarNearDrawn = false;
    m_lastDrawnSecond = -1;
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Wird zyklisch aufgerufen
 */
void Breather::draw()
{
    if (!m_initialized)
        return;

    // Sprachwechsel? -> Buttons neu beschriften, kompletten Bildschirm neu aufbauen
    if (I18n::consumeLanguageChanged())
        redrawAll();

    // GFX blockiert?
    const bool gfxBlocked = APP_STATE.getBlockGFX();

    int32_t tx, ty;
    bool touched = !gfxBlocked && m_tft->getTouch(&tx, &ty);

    handleTouch(tx, ty, touched);

    // Während eines Screenshots keine weiteren Zeichen-Operationen
    if (gfxBlocked)
        return;

    // Bildschirm-Wechsel: Hintergrund aufbauen
    if (m_screenChanged)
    {
        m_tft->clear(COLOR_BG);

        switch (m_screen)
        {
        case BreatherScreen::SELECTION:
            drawSelectionScreen();
            break;

        case BreatherScreen::RUNNING:
            m_lastDisplayedRemaining = -1;
            m_lastDisplayedProgress = -1;
            m_vizStaticDrawn = false;
            m_vizStaticInitialized = false;
            m_lastBreathRadius = -1;
            m_lastBreathPhase = 0xFF;
            m_eyeFarNearDrawn = false;
            drawRunningStaticLayout();
            drawRunningHeader();
            drawProgressBar();
            drawExerciseTitle();
            drawInstruction();
            m_lastDisplayedExercise = m_exerciseIndex;
            updateRunningControls();
            break;

        case BreatherScreen::DONE:
            drawDoneScreen();
            break;
        }

        m_exitButton.drawButton(false);

        if (m_screen == BreatherScreen::RUNNING)
        {
            m_backButton.drawButton(false);
            m_pauseButton.drawButton(false);
            m_skipButton.drawButton(false);
        }
        else if (m_screen == BreatherScreen::DONE)
        {
            m_doneButton.drawButton(false);
        }

        m_screenChanged = false;
        m_lastDrawnPressedTile = m_pressedSelectionTile;
    }

    // Pro Frame
    if (m_screen == BreatherScreen::SELECTION)
    {
        // Druck-Optik nur dort neu zeichnen, wo sich der Zustand geändert hat
        if (m_pressedSelectionTile != m_lastDrawnPressedTile)
        {
            if (m_lastDrawnPressedTile >= 0 && m_lastDrawnPressedTile < SESSION_COUNT)
                drawSelectionTile(static_cast<uint8_t>(m_lastDrawnPressedTile));

            if (m_pressedSelectionTile >= 0 && m_pressedSelectionTile < SESSION_COUNT)
                drawSelectionTile(static_cast<uint8_t>(m_pressedSelectionTile));

            m_lastDrawnPressedTile = m_pressedSelectionTile;
        }
    }
    else if (m_screen == BreatherScreen::RUNNING && m_currentSession >= 0)
    {
        const BreatherSession &session = SESSIONS_DE[m_currentSession];

        // Prüfen ob die aktuelle Übung beendet ist
        if (!m_paused && m_exerciseIndex < session.exerciseCount)
        {
            const uint16_t elapsedSec = getElapsedSecondsInExercise();
            const BreatherExercise &ex = session.exercises[m_exerciseIndex];

            if (elapsedSec >= ex.durationSec)
            {
                advanceExercise();
                return;
            }
        }

        // Übungs-Wechsel erkennen
        if (m_lastDisplayedExercise != m_exerciseIndex)
        {
            drawRunningHeader();
            drawExerciseTitle();
            drawInstruction();
            m_lastDisplayedExercise = m_exerciseIndex;
            m_lastDisplayedRemaining = -1;
            m_vizStaticDrawn = false;
            m_vizStaticInitialized = false;
            m_lastBreathRadius = -1;
            m_lastBreathPhase = 0xFF;
            m_eyeFarNearDrawn = false;
        }

        drawVisualization();
        drawRemainingTime();
        drawProgressBar();
    }
    else if (m_screen == BreatherScreen::DONE)
    {
        if (millis() - m_doneShownAtMs >= DONE_AUTO_RETURN_MS)
            returnToSelection();
    }

    drawDigitalClock(CLOCK_X, m_tft->height() - CLOCK_H + CLOCK_Y_FROM_BOTTOM);
}

// ===================================================================
// Ablaufsteuerung
// ===================================================================

/**
 * @brief Startet eine Sitzung
 */
void Breather::startSession(uint8_t sessionIndex)
{
    if (sessionIndex >= SESSION_COUNT)
        return;

    m_currentSession = static_cast<int8_t>(sessionIndex);
    m_exerciseIndex = 0;
    m_paused = false;
    m_exerciseStartMs = millis();
    m_pauseStartMs = 0;
    m_totalPausedMs = 0;
    m_lastDisplayedExercise = 0xFF;
    m_lastDisplayedRemaining = -1;
    m_lastDisplayedProgress = -1;
    m_pauseLabelIsResume = false;
    m_pressedSelectionTile = -1;

    m_screen = BreatherScreen::RUNNING;
    m_screenChanged = true;

    LOG_INFO_F("Breather: Sitzung '%s' gestartet", SESSIONS_DE[sessionIndex].title);
}

/**
 * @brief Geht zur nächsten Übung oder beendet die Sitzung
 */
void Breather::advanceExercise()
{
    if (m_currentSession < 0)
        return;

    const BreatherSession &session = SESSIONS_DE[m_currentSession];

    m_exerciseIndex++;

    if (m_exerciseIndex >= session.exerciseCount)
    {
        finishSession();
        return;
    }

    m_exerciseStartMs = millis();
    m_pauseStartMs = 0;
    m_totalPausedMs = 0;

    playPhaseSound();
}

/**
 * @brief Sitzung abgeschlossen, in den DONE-Bildschirm wechseln
 */
void Breather::finishSession()
{
    if (m_currentSession < 0)
        return;

    LOG_INFO_F("Breather: Sitzung '%s' beendet", SESSIONS_DE[m_currentSession].title);

    m_paused = false;
    m_doneShownAtMs = millis();
    m_screen = BreatherScreen::DONE;
    m_screenChanged = true;

    playPhaseSound();
}

/**
 * @brief Zurück zum Auswahl-Bildschirm
 */
void Breather::returnToSelection()
{
    m_currentSession = -1;
    m_exerciseIndex = 0;
    m_paused = false;
    m_pressedSelectionTile = -1;
    m_screen = BreatherScreen::SELECTION;
    m_screenChanged = true;
}

/**
 * @brief Pausiert oder setzt die Sitzung fort
 */
void Breather::togglePause()
{
    if (m_screen != BreatherScreen::RUNNING)
        return;

    if (m_paused)
    {
        // Pause beenden, Pausenzeit kumulieren
        m_totalPausedMs += millis() - m_pauseStartMs;
        m_pauseStartMs = 0;
        m_paused = false;
        m_pauseLabelIsResume = false;
        updateRunningControls();
    }
    else
    {
        m_pauseStartMs = millis();
        m_paused = true;
        m_pauseLabelIsResume = true;
        updateRunningControls();
    }
}

/**
 * @brief Springt zur nächsten Übung
 */
void Breather::skipExercise()
{
    if (m_screen != BreatherScreen::RUNNING || m_currentSession < 0)
        return;

    if (m_paused)
        togglePause();

    advanceExercise();
}

// ===================================================================
// Zeitberechnung
// ===================================================================

/**
 * @brief Verstrichene Millisekunden seit Übungsstart unter Berücksichtigung der Pausen
 */
uint32_t Breather::getElapsedMsInExercise() const
{
    if (m_exerciseStartMs == 0)
        return 0;

    const unsigned long now = millis();
    unsigned long pausedMs = m_totalPausedMs;

    if (m_paused && m_pauseStartMs > 0)
        pausedMs += now - m_pauseStartMs;

    // Erst die Roh-Differenz zum Übungsstart bilden, dann die Pausenzeit abziehen. So bleibt die Rechnung über millis()-Rollover hinweg robust.
    const unsigned long rawElapsed = now - m_exerciseStartMs;

    if (rawElapsed < pausedMs)
        return 0;

    return static_cast<uint32_t>(rawElapsed - pausedMs);
}

/**
 * @brief Verstrichene Sekunden in der aktuellen Übung
 */
uint16_t Breather::getElapsedSecondsInExercise() const
{
    return static_cast<uint16_t>(getElapsedMsInExercise() / 1000UL);
}

/**
 * @brief Verstrichene Sekunden insgesamt in der Sitzung
 */
uint16_t Breather::getElapsedSecondsInSession() const
{
    if (m_currentSession < 0)
        return 0;

    const BreatherSession &session = SESSIONS_DE[m_currentSession];
    uint16_t total = 0;

    for (uint8_t i = 0; i < m_exerciseIndex && i < session.exerciseCount; ++i)
        total += session.exercises[i].durationSec;

    total += getElapsedSecondsInExercise();

    if (total > session.totalDurationSec)
        total = session.totalDurationSec;

    return total;
}

// ===================================================================
// Auswahl-Bildschirm
// ===================================================================

/**
 * @brief Vollständiger Aufbau der Sitzungs-Auswahl
 */
void Breather::drawSelectionScreen()
{
    // Hauptüberschrift im Stil der Modul-Titel der übrigen Anwendung
    m_tft->setFont(&fonts::Orbitron_Light_24);
    m_tft->setTextDatum(TC_DATUM);
    m_tft->setTextColor(COLOR_TEXT_BRIGHT, COLOR_BG);
    m_tft->drawString(STR(BreatherKey::BREATHER_HEADER_TITLE), DISPLAY_W / 2, SEL_HEADER_Y);

    // Untertitel
    m_tft->loadFont(ComfortaaBold16);
    m_tft->setTextDatum(TC_DATUM);
    m_tft->setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    m_tft->drawString(STR(BreatherKey::BREATHER_HEADER_SUBTITLE), DISPLAY_W / 2, SEL_HEADER_Y + 28);
    m_tft->unloadFont();

    for (uint8_t i = 0; i < SEL_TILE_COUNT && i < SESSION_COUNT; ++i)
        drawSelectionTile(i);
}

/**
 * @brief Zeichnet eine einzelne Sitzungs-Kachel
 *
 * Die Karte hat einen 4 px breiten Akzentstreifen am linken Rand in der Sitzungs-Akzentfarbe. Titel oben links nach dem Streifen, kurzer Stimmungs-Untertitel darunter, Dauer unten rechts dezent in der Akzentfarbe. Wird die Kachel gerade gedrückt gehalten, hellt sich der Kartenhintergrund leicht auf.
 */
void Breather::drawSelectionTile(uint8_t sessionIndex)
{
    if (sessionIndex >= SESSION_COUNT)
        return;

    const int col = sessionIndex % SEL_TILE_COLS;
    const int row = sessionIndex / SEL_TILE_COLS;
    const int x = SEL_TILE_GRID_X + col * (SEL_TILE_W + SEL_TILE_GAP_X);
    const int y = SEL_TILES_Y + row * (SEL_TILE_H + SEL_TILE_GAP_Y);

    const BreatherSession &session = activeSessions()[sessionIndex];

    const bool pressed = (m_pressedSelectionTile == static_cast<int8_t>(sessionIndex));
    const uint16_t cardBg = pressed ? COLOR_TILE_BG_PRESSED : COLOR_TILE_BG;

    // Karte und Akzentstreifen
    m_tft->fillRoundRect(x, y, SEL_TILE_W, SEL_TILE_H, SEL_TILE_RADIUS, cardBg);
    m_tft->fillRect(x, y, SEL_TILE_ACCENT_W, SEL_TILE_H, session.accentColor);

    // Haupttitel oben links nach dem Akzentstreifen
    m_tft->loadFont(LeagueGothicCond34);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->setTextColor(COLOR_TEXT_BRIGHT, cardBg);
    m_tft->drawString(session.title, x + SEL_TILE_TEXT_X, y + SEL_TILE_TITLE_Y);
    m_tft->unloadFont();

    // Stimmungs-Untertitel etwas darunter
    m_tft->loadFont(ComfortaaBold14);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->setTextColor(COLOR_TEXT_DIM, cardBg);
    m_tft->drawString(session.subtitle, x + SEL_TILE_TEXT_X, y + SEL_TILE_SUB_Y);
    m_tft->unloadFont();

    // Dauer unten rechts in Akzentfarbe
    char dauerBuf[16];
    if (session.totalDurationSec % 60 == 0)
    {
        STRF(dauerBuf, BreatherKey::BREATHER_DURATION_MIN_INT_FMT, session.totalDurationSec / 60);
    }
    else
    {
        char timeBuf[12];
        formatDurationShort(session.totalDurationSec, timeBuf, sizeof(timeBuf));
        STRF(dauerBuf, BreatherKey::BREATHER_DURATION_MIN_STR_FMT, timeBuf);
    }

    m_tft->loadFont(ComfortaaBold18);
    m_tft->setTextDatum(BR_DATUM);
    m_tft->setTextColor(session.accentColor, cardBg);
    m_tft->drawString(dauerBuf, x + SEL_TILE_W - SEL_TILE_PAD_R, y + SEL_TILE_H - SEL_TILE_PAD_B);
    m_tft->unloadFont();
}

/**
 * @brief Liefert den Index der Kachel an der Position oder -1
 */
int8_t Breather::findSelectionTileAt(int32_t x, int32_t y) const
{
    for (uint8_t i = 0; i < SEL_TILE_COUNT && i < SESSION_COUNT; ++i)
    {
        const int col = i % SEL_TILE_COLS;
        const int row = i / SEL_TILE_COLS;
        const int tx = SEL_TILE_GRID_X + col * (SEL_TILE_W + SEL_TILE_GAP_X);
        const int ty = SEL_TILES_Y + row * (SEL_TILE_H + SEL_TILE_GAP_Y);

        if (x >= tx && x < tx + SEL_TILE_W && y >= ty && y < ty + SEL_TILE_H)
            return static_cast<int8_t>(i);
    }

    return -1;
}

// ===================================================================
// Laufende Sitzung - Layout und Header
// ===================================================================

/**
 * @brief Statische Elemente einmalig zeichnen
 */
void Breather::drawRunningStaticLayout()
{
    // Trennlinie mittig zwischen rechtem Rand der Visualisierung und linkem Rand des Anweisungs-Bereichs
    constexpr int sepX = (RUN_VIZ_X + RUN_VIZ_W + RUN_INSTR_X) / 2;
    m_tft->drawLine(sepX, RUN_VIZ_Y + 20, sepX, RUN_VIZ_Y + RUN_VIZ_H - 20, COLOR_TEXT_DIM);
}

/**
 * @brief Zeichnet Sitzungs-Titel und Übungsfortschritt
 */
void Breather::drawRunningHeader()
{
    if (m_currentSession < 0 || !m_headerSprite)
        return;

    const BreatherSession &session = activeSessions()[m_currentSession];

    m_headerSprite->clear(COLOR_BG);

    m_headerSprite->loadFont(LeagueGothicCond34);
    m_headerSprite->setTextDatum(TC_DATUM);
    m_headerSprite->setTextColor(COLOR_TEXT_BRIGHT, COLOR_BG);
    m_headerSprite->drawString(session.title, DISPLAY_W / 2, RUN_HEADER_Y);
    m_headerSprite->unloadFont();

    char buf[64];
    if (m_exerciseIndex < session.exerciseCount)
        STRF(buf, BreatherKey::BREATHER_EXERCISE_X_OF_Y_FMT, m_exerciseIndex + 1, session.exerciseCount);
    else
        stringutils::safeCopy(buf, "");

    m_headerSprite->loadFont(ComfortaaBold16);
    m_headerSprite->setTextDatum(TC_DATUM);
    m_headerSprite->setTextColor(COLOR_TEXT, COLOR_BG);
    m_headerSprite->drawString(buf, DISPLAY_W / 2, RUN_EX_INFO_Y);
    m_headerSprite->unloadFont();

    m_headerSprite->pushSprite(0, 0);
}

/**
 * @brief Zeichnet den Gesamt-Fortschrittsbalken
 */
void Breather::drawProgressBar()
{
    if (m_currentSession < 0 || !m_progressSprite)
        return;

    const BreatherSession &session = SESSIONS_DE[m_currentSession];
    const uint16_t elapsed = getElapsedSecondsInSession();
    const int filledW = (session.totalDurationSec > 0)
                            ? (RUN_PROGRESS_W * elapsed) / session.totalDurationSec
                            : 0;

    if (filledW == m_lastDisplayedProgress)
        return;

    m_progressSprite->clear(COLOR_BG);
    m_progressSprite->drawRect(0, 0, RUN_PROGRESS_W + 2, RUN_PROGRESS_H + 2, COLOR_SEP_LINE);
    m_progressSprite->fillRect(1, 1, filledW, RUN_PROGRESS_H, session.accentColor);
    m_progressSprite->pushSprite(RUN_PROGRESS_X - 1, RUN_PROGRESS_Y - 1);

    m_lastDisplayedProgress = filledW;
}

/**
 * @brief Übungs-Titel über dem Anweisungstext
 */
void Breather::drawExerciseTitle()
{
    if (m_currentSession < 0 || !m_titleSprite)
        return;

    const BreatherSession &session = activeSessions()[m_currentSession];

    if (m_exerciseIndex >= session.exerciseCount)
        return;

    const BreatherExercise &ex = session.exercises[m_exerciseIndex];

    m_titleSprite->clear(COLOR_BG);
    m_titleSprite->loadFont(ComfortaaBold18);
    m_titleSprite->setTextDatum(TL_DATUM);
    m_titleSprite->setTextColor(COLOR_VIZ_PRIMARY, COLOR_BG);
    m_titleSprite->drawString(ex.title, 0, 0);
    m_titleSprite->unloadFont();
    m_titleSprite->pushSprite(RUN_INSTR_X, RUN_INSTR_Y);
}

/**
 * @brief Mehrzeiliger Anweisungstext
 */
void Breather::drawInstruction()
{
    if (m_currentSession < 0 || !m_instructionSprite)
        return;

    const BreatherSession &session = activeSessions()[m_currentSession];

    if (m_exerciseIndex >= session.exerciseCount)
        return;

    const BreatherExercise &ex = session.exercises[m_exerciseIndex];

    m_instructionSprite->clear(COLOR_BG);
    m_instructionSprite->loadFont(ComfortaaBold16);
    m_instructionSprite->setTextDatum(TL_DATUM);
    m_instructionSprite->setTextColor(COLOR_TEXT, COLOR_BG);

    constexpr int LINE_HEIGHT = 24;
    int yLine = 0;

    char lineBuf[128];
    const char *cursor = ex.instruction;

    while (nextLine(cursor, lineBuf, sizeof(lineBuf)))
    {
        m_instructionSprite->drawString(lineBuf, 0, yLine);
        yLine += LINE_HEIGHT;

        if (yLine + LINE_HEIGHT > RUN_INSTR_TEXT_H)
            break;
    }

    m_instructionSprite->unloadFont();
    m_instructionSprite->pushSprite(RUN_INSTR_X, RUN_INSTR_Y + RUN_INSTR_TEXT_Y);
}

/**
 * @brief Verbleibende Sekunden der aktuellen Übung
 */
void Breather::drawRemainingTime()
{
    if (m_currentSession < 0 || !m_remainingSprite)
        return;

    const BreatherSession &session = SESSIONS_DE[m_currentSession];

    if (m_exerciseIndex >= session.exerciseCount)
        return;

    const BreatherExercise &ex = session.exercises[m_exerciseIndex];
    const uint16_t elapsed = getElapsedSecondsInExercise();
    int remaining = static_cast<int>(ex.durationSec) - static_cast<int>(elapsed);

    if (remaining < 0)
        remaining = 0;

    if (remaining == m_lastDisplayedRemaining)
        return;

    m_lastDisplayedRemaining = remaining;

    char buf[16];
    formatSeconds(static_cast<uint16_t>(remaining), buf, sizeof(buf));

    m_remainingSprite->clear(COLOR_BG);
    m_remainingSprite->loadFont(ComfortaaBold18);
    m_remainingSprite->setTextDatum(TL_DATUM);
    m_remainingSprite->setTextColor(COLOR_TEXT, COLOR_BG);

    const char *label = reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_REMAINING_LABEL));
    constexpr int labelGap = 12;
    m_remainingSprite->drawString(label, 0, 0);
    const int valueX = m_remainingSprite->textWidth(label) + labelGap;

    m_remainingSprite->setTextColor(COLOR_VIZ_PRIMARY, COLOR_BG);
    m_remainingSprite->drawString(buf, valueX, 0);
    m_remainingSprite->unloadFont();
    m_remainingSprite->pushSprite(RUN_REMAIN_X, RUN_REMAIN_Y);
}

/**
 * @brief Aktualisiert die Beschriftung vom Pause-Button
 */
void Breather::updateRunningControls()
{
    m_pauseButton.setLabelText(reinterpret_cast<const char *>(
        m_pauseLabelIsResume ? STR(BreatherKey::BREATHER_BTN_RESUME)
                             : STR(BreatherKey::BREATHER_BTN_PAUSE)));
    m_pauseButton.drawButton(false);
}

// ===================================================================
// Visualisierung
// ===================================================================

/**
 * @brief Hauptverteiler für die Visualisierung der aktuellen Übung
 */
void Breather::drawVisualization()
{
    if (m_currentSession < 0 || !m_vizSprite)
        return;

    const BreatherSession &session = SESSIONS_DE[m_currentSession];

    if (m_exerciseIndex >= session.exerciseCount)
        return;

    const BreatherExercise &ex = session.exercises[m_exerciseIndex];

    // Statische Visualisierungen nur einmal pro Übung zeichnen
    const bool isStatic = (ex.kind == ExerciseKind::EYE_PALMING || ex.kind == ExerciseKind::REST);

    if (isStatic && m_vizStaticDrawn)
        return;

    // EyeFarNear wechselt nur alle 3 Sekunden den Zustand, dazwischen kein Redraw nötig
    if (ex.kind == ExerciseKind::EYE_FAR_NEAR)
    {
        const uint32_t elapsedMs = getElapsedMsInExercise();
        constexpr uint32_t PERIOD_MS = 6000;
        const bool farPhase = (elapsedMs % PERIOD_MS) < PERIOD_MS / 2;

        if (m_eyeFarNearDrawn && farPhase == m_lastEyeFarPhase)
            return;

        m_lastEyeFarPhase = farPhase;
        m_eyeFarNearDrawn = true;
    }

    m_vizFont = BreatherFontId::NONE;
    m_vizPushClipped = false;

    bool drawn = true;

    switch (ex.kind)
    {
    case ExerciseKind::BREATHING_BOX:
        drawn = renderBreathingBox();
        break;
    case ExerciseKind::BREATHING_478:
        drawn = renderBreathing478();
        break;
    case ExerciseKind::NECK_TILT:
        drawn = renderPendulum(4.0f, reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_AXIS_HEAD)));
        break;
    case ExerciseKind::SPINE_TWIST:
        drawn = renderPendulum(5.0f, reinterpret_cast<const char *>(STR(BreatherKey::BREATHER_AXIS_TWIST)));
        break;
    case ExerciseKind::SHOULDER_ROLL:
        drawn = renderRotation(4.0f);
        break;
    case ExerciseKind::WRIST_CIRCLE:
        drawn = renderRotation(3.0f);
        break;
    case ExerciseKind::EYE_CIRCLE:
        drawn = renderRotation(6.0f);
        break;
    case ExerciseKind::SHOULDER_LIFT:
        drawn = renderVerticalBob(3.0f);
        break;
    case ExerciseKind::HIP_OPENER:
        drawn = renderVerticalBob(4.0f);
        break;
    case ExerciseKind::STAND_AND_REACH:
        drawn = renderVerticalBob(5.0f);
        break;
    case ExerciseKind::EYE_FAR_NEAR:
        renderEyeFarNear();
        break;
    case ExerciseKind::EYE_PALMING:
        renderEyePalming();
        break;
    case ExerciseKind::REST:
        renderRest();
        break;
    }

    if (drawn)
    {
        unloadVizFont();

        if (m_vizPushClipped)
        {
            m_tft->setClipRect(RUN_VIZ_X + m_vizPushX, RUN_VIZ_Y + m_vizPushY, m_vizPushW, m_vizPushH);
            m_vizSprite->pushSprite(RUN_VIZ_X, RUN_VIZ_Y);
            m_tft->clearClipRect();
        }
        else
        {
            m_vizSprite->pushSprite(RUN_VIZ_X, RUN_VIZ_Y);
        }
    }

    if (isStatic)
        m_vizStaticDrawn = true;
}

/**
 * @brief Generischer Atemkreis mit Phasen-Sequenz und Change-Detection
 */
bool Breather::renderBreathingCircle(const uint16_t *phaseSecsArr, const char *const *phaseLabels, uint8_t phaseCount)
{
    const uint32_t elapsedMs = getElapsedMsInExercise();
    uint8_t phase = 0;
    float phaseProgress = 0.0f;
    resolveCyclicPhase(elapsedMs, phaseSecsArr, phaseCount, phase, phaseProgress);

    float radius = VIZ_BREATH_RADIUS_MIN;
    const char *label = (phase < phaseCount) ? phaseLabels[phase] : "";

    if (phaseCount == 4)
    {
        // Box: ein, halten oben, aus, halten unten
        switch (phase)
        {
        case 0:
            radius = VIZ_BREATH_RADIUS_MIN + (VIZ_BREATH_RADIUS_MAX - VIZ_BREATH_RADIUS_MIN) * phaseProgress;
            break;
        case 1:
            radius = VIZ_BREATH_RADIUS_MAX;
            break;
        case 2:
            radius = VIZ_BREATH_RADIUS_MAX - (VIZ_BREATH_RADIUS_MAX - VIZ_BREATH_RADIUS_MIN) * phaseProgress;
            break;
        case 3:
            radius = VIZ_BREATH_RADIUS_MIN;
            break;
        }
    }
    else if (phaseCount == 3)
    {
        // 4-7-8: ein, halten, aus
        switch (phase)
        {
        case 0:
            radius = VIZ_BREATH_RADIUS_MIN + (VIZ_BREATH_RADIUS_MAX - VIZ_BREATH_RADIUS_MIN) * phaseProgress;
            break;
        case 1:
            radius = VIZ_BREATH_RADIUS_MAX;
            break;
        case 2:
            radius = VIZ_BREATH_RADIUS_MAX - (VIZ_BREATH_RADIUS_MAX - VIZ_BREATH_RADIUS_MIN) * phaseProgress;
            break;
        }
    }

    const int r = static_cast<int>(radius);

    if (r == m_lastBreathRadius && phase == m_lastBreathPhase)
        return false;

    m_lastBreathRadius = r;
    m_lastBreathPhase = phase;

    m_vizSprite->clear(COLOR_BG);

    // Äußerer Markierungs-Kreis mit AA-Rand
    m_vizSprite->drawSmoothCircle(VIZ_CENTER_X, VIZ_CENTER_Y, VIZ_BREATH_MARKER_RADIUS, 2.0f, COLOR_TEXT_DIM, COLOR_BG);

    // Gefüllter Innenkreis mit AA-Kante
    m_vizSprite->fillSmoothCircle(VIZ_CENTER_X, VIZ_CENTER_Y, r, COLOR_VIZ_PRIMARY);

    // Phasen-Label im Zentrum
    useVizFont(BreatherFontId::LEAGUE_GOTHIC34);
    m_vizSprite->setTextDatum(MC_DATUM);
    m_vizSprite->setTextColor(COLOR_BG, COLOR_VIZ_PRIMARY);
    m_vizSprite->drawString(localizedPhaseLabel(label), VIZ_CENTER_X, VIZ_CENTER_Y);

    return true;
}

/**
 * @brief 4-4-4-4 Atemkreis
 */
bool Breather::renderBreathingBox()
{
    return renderBreathingCircle(BOX_PHASE_SECS, BOX_PHASE_LABELS, 4);
}

/**
 * @brief 4-7-8 Atemkreis
 */
bool Breather::renderBreathing478()
{
    return renderBreathingCircle(T478_PHASE_SECS, T478_PHASE_LABELS, 3);
}

/**
 * @brief Zeichnet die animierte Kugel einmalig in m_dotSprite
 *
 * Schwarz als BG-Quadrat, Kugel mit AA-Rand. Beim Push aufs Display überdeckt der schwarze Rest die Bahn-Linie unter der Kugel. Beim Wegnehmen wird der komplette Quadrat-Bereich aus dem statischen Hintergrund-Sprite zurückgeholt.
 */
void Breather::prepareDotSprite()
{
    if (!m_dotSprite)
        return;

    m_dotSprite->clear(COLOR_BG);
    m_dotSprite->drawSmoothCircle(DOT_SPRITE_HALF, DOT_SPRITE_HALF, VIZ_DOT_RADIUS, DOT_SMOOTH_THICKNESS, COLOR_VIZ_SECONDARY, COLOR_BG);
    m_dotSprite->fillCircle(DOT_SPRITE_HALF, DOT_SPRITE_HALF, static_cast<int>(VIZ_DOT_RADIUS) - 1, COLOR_VIZ_SECONDARY);
}

/**
 * @brief Pendelnde Bewegung links und rechts
 */
bool Breather::renderPendulum(float periodSec, const char *axisLabel)
{
    const uint32_t elapsedMs = getElapsedMsInExercise();
    const float t = static_cast<float>(elapsedMs) / 1000.0f;
    const float angle = TWO_PI_F * t / periodSec;
    const float xOffset = sinf(angle) * VIZ_PENDULUM_AMPLITUDE;

    // Sprite-Koordinaten der neuen Kugel-Mittelpunkt-Position
    const int newX = VIZ_CENTER_X + static_cast<int>(xOffset);
    const int newY = VIZ_CENTER_Y;

    // Sprite-interne Koordinaten des statischen Hintergrunds
    const int trackXLeft = VIZ_CENTER_X - static_cast<int>(VIZ_PENDULUM_AMPLITUDE);
    const int trackXRight = VIZ_CENTER_X + static_cast<int>(VIZ_PENDULUM_AMPLITUDE);

    if (!m_vizStaticInitialized)
    {
        // Statischer Hintergrund einmalig komplett ins Sprite zeichnen
        m_vizSprite->clear(COLOR_BG);
        m_vizSprite->drawLine(trackXLeft, VIZ_CENTER_Y, trackXRight, VIZ_CENTER_Y, COLOR_TEXT_DIM);
        m_vizSprite->fillCircle(trackXLeft, VIZ_CENTER_Y, 5, COLOR_TEXT_DIM);
        m_vizSprite->fillCircle(trackXRight, VIZ_CENTER_Y, 5, COLOR_TEXT_DIM);

        if (axisLabel && axisLabel[0] != '\0')
        {
            useVizFont(BreatherFontId::COMFORTAA_BOLD14);
            m_vizSprite->setTextDatum(MC_DATUM);
            m_vizSprite->setTextColor(COLOR_TEXT_DIM, COLOR_BG);
            m_vizSprite->drawString(axisLabel, VIZ_CENTER_X, VIZ_CENTER_Y - 80);
        }

        // Kugel-Sprite vorbereiten und an Startposition ins große Sprite pushen
        prepareDotSprite();
        m_dotSprite->pushSprite(m_vizSprite, newX - DOT_SPRITE_HALF, newY - DOT_SPRITE_HALF);

        m_lastDotX = newX;
        m_lastDotY = newY;
        m_vizStaticInitialized = true;
        return true;
    }

    // Wenn die Pixelposition gleich geblieben ist, kein Redraw
    if (newX == m_lastDotX && newY == m_lastDotY)
        return false;

    // Folge-Frame: Bounding-Box aus alter und neuer Kugel-Position
    const int xMin = (m_lastDotX < newX ? m_lastDotX : newX) - DOT_SPRITE_HALF - 1;
    const int xMax = (m_lastDotX > newX ? m_lastDotX : newX) + DOT_SPRITE_HALF + 1;
    const int yMin = newY - DOT_SPRITE_HALF - 1;
    const int yMax = newY + DOT_SPRITE_HALF + 1;

    const int boxX = (xMin < 0) ? 0 : xMin;
    const int boxY = (yMin < 0) ? 0 : yMin;
    const int boxW = ((xMax >= RUN_VIZ_W) ? RUN_VIZ_W - 1 : xMax) - boxX + 1;
    const int boxH = ((yMax >= RUN_VIZ_H) ? RUN_VIZ_H - 1 : yMax) - boxY + 1;

    // Im Sprite die Box mit BG füllen und statische Elemente in der Box rekonstruieren
    m_vizSprite->setClipRect(boxX, boxY, boxW, boxH);
    m_vizSprite->fillRect(boxX, boxY, boxW, boxH, COLOR_BG);
    m_vizSprite->drawLine(trackXLeft, VIZ_CENTER_Y, trackXRight, VIZ_CENTER_Y, COLOR_TEXT_DIM);
    m_vizSprite->fillCircle(trackXLeft, VIZ_CENTER_Y, 5, COLOR_TEXT_DIM);
    m_vizSprite->fillCircle(trackXRight, VIZ_CENTER_Y, 5, COLOR_TEXT_DIM);
    // AA-Kugel in die Box pushen, der Sprite-Clip beschränkt den Push automatisch
    m_dotSprite->pushSprite(m_vizSprite, newX - DOT_SPRITE_HALF, newY - DOT_SPRITE_HALF);
    m_vizSprite->clearClipRect();

    m_vizPushClipped = true;
    m_vizPushX = boxX;
    m_vizPushY = boxY;
    m_vizPushW = boxW;
    m_vizPushH = boxH;

    m_lastDotX = newX;
    m_lastDotY = newY;
    return true;
}

/**
 * @brief Kreisende Bewegung
 */
bool Breather::renderRotation(float periodSec)
{
    const uint32_t elapsedMs = getElapsedMsInExercise();
    const float t = static_cast<float>(elapsedMs) / 1000.0f;
    const float angle = TWO_PI_F * t / periodSec;
    const float dotX = cosf(angle) * VIZ_ROT_RADIUS;
    const float dotY = sinf(angle) * VIZ_ROT_RADIUS;

    // Sprite-Koordinaten der neuen Kugel-Mittelpunkt-Position
    const int newX = VIZ_CENTER_X + static_cast<int>(dotX);
    const int newY = VIZ_CENTER_Y + static_cast<int>(dotY);

    if (!m_vizStaticInitialized)
    {
        // Statischer Hintergrund einmalig komplett ins Sprite zeichnen
        m_vizSprite->clear(COLOR_BG);
        m_vizSprite->drawCircle(VIZ_CENTER_X, VIZ_CENTER_Y, static_cast<int>(VIZ_ROT_RADIUS), COLOR_TEXT_DIM);

        // Kugel-Sprite vorbereiten und an Startposition ins große Sprite pushen
        prepareDotSprite();
        m_dotSprite->pushSprite(m_vizSprite, newX - DOT_SPRITE_HALF, newY - DOT_SPRITE_HALF);

        m_lastDotX = newX;
        m_lastDotY = newY;
        m_vizStaticInitialized = true;
        return true;
    }

    // Wenn die Pixelposition gleich geblieben ist, kein Redraw
    if (newX == m_lastDotX && newY == m_lastDotY)
        return false;

    // Folge-Frame: Bounding-Box aus alter und neuer Kugel-Position
    const int xMin = (m_lastDotX < newX ? m_lastDotX : newX) - DOT_SPRITE_HALF - 1;
    const int xMax = (m_lastDotX > newX ? m_lastDotX : newX) + DOT_SPRITE_HALF + 1;
    const int yMin = (m_lastDotY < newY ? m_lastDotY : newY) - DOT_SPRITE_HALF - 1;
    const int yMax = (m_lastDotY > newY ? m_lastDotY : newY) + DOT_SPRITE_HALF + 1;

    const int boxX = (xMin < 0) ? 0 : xMin;
    const int boxY = (yMin < 0) ? 0 : yMin;
    const int boxW = ((xMax >= RUN_VIZ_W) ? RUN_VIZ_W - 1 : xMax) - boxX + 1;
    const int boxH = ((yMax >= RUN_VIZ_H) ? RUN_VIZ_H - 1 : yMax) - boxY + 1;

    m_vizSprite->setClipRect(boxX, boxY, boxW, boxH);
    m_vizSprite->fillRect(boxX, boxY, boxW, boxH, COLOR_BG);
    m_vizSprite->drawCircle(VIZ_CENTER_X, VIZ_CENTER_Y, static_cast<int>(VIZ_ROT_RADIUS), COLOR_TEXT_DIM);
    m_dotSprite->pushSprite(m_vizSprite, newX - DOT_SPRITE_HALF, newY - DOT_SPRITE_HALF);
    m_vizSprite->clearClipRect();

    m_vizPushClipped = true;
    m_vizPushX = boxX;
    m_vizPushY = boxY;
    m_vizPushW = boxW;
    m_vizPushH = boxH;

    m_lastDotX = newX;
    m_lastDotY = newY;
    return true;
}

/**
 * @brief Wechsel zwischen Fern- und Nahpunkt
 */
void Breather::renderEyeFarNear()
{
    // Phasenzustand wurde bereits in drawVisualization() ermittelt und in m_lastEyeFarPhase abgelegt
    const float radius = m_lastEyeFarPhase ? VIZ_EYE_FAR_RADIUS : VIZ_EYE_NEAR_RADIUS;

    m_vizSprite->clear(COLOR_BG);
    m_vizSprite->fillCircle(VIZ_CENTER_X, VIZ_CENTER_Y, static_cast<int>(radius), COLOR_VIZ_SECONDARY);

    useVizFont(BreatherFontId::LEAGUE_GOTHIC34);
    m_vizSprite->setTextDatum(MC_DATUM);
    m_vizSprite->setTextColor(COLOR_BG, COLOR_VIZ_SECONDARY);
    m_vizSprite->drawString(m_lastEyeFarPhase ? STR(BreatherKey::BREATHER_EYE_FAR)
                                              : STR(BreatherKey::BREATHER_EYE_NEAR),
                            VIZ_CENTER_X, VIZ_CENTER_Y);
}

/**
 * @brief Statisches abgedunkeltes Symbol
 */
void Breather::renderEyePalming()
{
    m_vizSprite->clear(COLOR_BG);
    m_vizSprite->fillCircle(VIZ_CENTER_X, VIZ_CENTER_Y, static_cast<int>(VIZ_EYE_PALMING_RADIUS), COLOR_VIZ_PALM);

    useVizFont(BreatherFontId::LEAGUE_GOTHIC34);
    m_vizSprite->setTextDatum(MC_DATUM);
    m_vizSprite->setTextColor(COLOR_TEXT_DIM, COLOR_VIZ_PALM);
    m_vizSprite->drawString(STR(BreatherKey::BREATHER_EYE_CLOSED), VIZ_CENTER_X, VIZ_CENTER_Y);
}

/**
 * @brief Auf- und abwärts pendelnde Bewegung
 */
bool Breather::renderVerticalBob(float periodSec)
{
    const uint32_t elapsedMs = getElapsedMsInExercise();
    const float t = static_cast<float>(elapsedMs) / 1000.0f;
    const float angle = TWO_PI_F * t / periodSec;
    const float yOffset = sinf(angle) * VIZ_BOB_AMPLITUDE;

    // Sprite-Koordinaten der neuen Kugel-Mittelpunkt-Position
    const int newX = VIZ_CENTER_X;
    const int newY = VIZ_CENTER_Y + static_cast<int>(yOffset);

    // Sprite-interne Koordinaten des statischen Hintergrunds
    const int trackYTop = VIZ_CENTER_Y - static_cast<int>(VIZ_BOB_AMPLITUDE);
    const int trackYBottom = VIZ_CENTER_Y + static_cast<int>(VIZ_BOB_AMPLITUDE);

    if (!m_vizStaticInitialized)
    {
        // Statischer Hintergrund einmalig komplett ins Sprite zeichnen
        m_vizSprite->clear(COLOR_BG);
        m_vizSprite->drawLine(VIZ_CENTER_X, trackYTop, VIZ_CENTER_X, trackYBottom, COLOR_TEXT_DIM);
        m_vizSprite->fillCircle(VIZ_CENTER_X, trackYTop, 5, COLOR_TEXT_DIM);
        m_vizSprite->fillCircle(VIZ_CENTER_X, trackYBottom, 5, COLOR_TEXT_DIM);

        // Kugel-Sprite vorbereiten und an Startposition ins große Sprite pushen
        prepareDotSprite();
        m_dotSprite->pushSprite(m_vizSprite, newX - DOT_SPRITE_HALF, newY - DOT_SPRITE_HALF);

        m_lastDotX = newX;
        m_lastDotY = newY;
        m_vizStaticInitialized = true;
        return true;
    }

    // Wenn die Pixelposition gleich geblieben ist, kein Redraw
    if (newX == m_lastDotX && newY == m_lastDotY)
        return false;

    // Folge-Frame: Bounding-Box aus alter und neuer Kugel-Position
    const int xMin = newX - DOT_SPRITE_HALF - 1;
    const int xMax = newX + DOT_SPRITE_HALF + 1;
    const int yMin = (m_lastDotY < newY ? m_lastDotY : newY) - DOT_SPRITE_HALF - 1;
    const int yMax = (m_lastDotY > newY ? m_lastDotY : newY) + DOT_SPRITE_HALF + 1;

    const int boxX = (xMin < 0) ? 0 : xMin;
    const int boxY = (yMin < 0) ? 0 : yMin;
    const int boxW = ((xMax >= RUN_VIZ_W) ? RUN_VIZ_W - 1 : xMax) - boxX + 1;
    const int boxH = ((yMax >= RUN_VIZ_H) ? RUN_VIZ_H - 1 : yMax) - boxY + 1;

    m_vizSprite->setClipRect(boxX, boxY, boxW, boxH);
    m_vizSprite->fillRect(boxX, boxY, boxW, boxH, COLOR_BG);
    m_vizSprite->drawLine(VIZ_CENTER_X, trackYTop, VIZ_CENTER_X, trackYBottom, COLOR_TEXT_DIM);
    m_vizSprite->fillCircle(VIZ_CENTER_X, trackYTop, 5, COLOR_TEXT_DIM);
    m_vizSprite->fillCircle(VIZ_CENTER_X, trackYBottom, 5, COLOR_TEXT_DIM);
    m_dotSprite->pushSprite(m_vizSprite, newX - DOT_SPRITE_HALF, newY - DOT_SPRITE_HALF);
    m_vizSprite->clearClipRect();

    m_vizPushClipped = true;
    m_vizPushX = boxX;
    m_vizPushY = boxY;
    m_vizPushW = boxW;
    m_vizPushH = boxH;

    m_lastDotX = newX;
    m_lastDotY = newY;
    return true;
}

/**
 * @brief Ruhiger statischer Punkt
 */
void Breather::renderRest()
{
    m_vizSprite->clear(COLOR_BG);
    m_vizSprite->fillCircle(VIZ_CENTER_X, VIZ_CENTER_Y, static_cast<int>(VIZ_REST_RADIUS), COLOR_VIZ_REST);
}

// ===================================================================
// Abschluss-Bildschirm
// ===================================================================

/**
 * @brief Bestätigung "Geschafft" mit Sitzungs-Daten
 */
void Breather::drawDoneScreen()
{
    if (m_currentSession < 0)
        return;

    const BreatherSession &session = activeSessions()[m_currentSession];

    // Titel
    m_tft->loadFont(LeagueGothicCond34);
    m_tft->setTextDatum(TC_DATUM);
    m_tft->setTextColor(session.accentColor, COLOR_BG);
    m_tft->drawString(STR(BreatherKey::BREATHER_DONE_TITLE), DISPLAY_W / 2, 140);

    // Sitzungs-Name
    m_tft->setTextDatum(TC_DATUM);
    m_tft->setTextColor(COLOR_TEXT_BRIGHT, COLOR_BG);
    m_tft->drawString(session.title, DISPLAY_W / 2, 200);
    m_tft->unloadFont();

    // Dauer
    char buf[32];
    char durBuf[8];
    formatDurationShort(session.totalDurationSec, durBuf, sizeof(durBuf));
    STRF(buf, BreatherKey::BREATHER_DONE_DURATION_FMT, durBuf);

    m_tft->loadFont(ComfortaaBold16);
    m_tft->setTextDatum(TC_DATUM);
    m_tft->setTextColor(COLOR_TEXT, COLOR_BG);
    m_tft->drawString(buf, DISPLAY_W / 2, 250);
    m_tft->unloadFont();

    // Hinweis
    m_tft->loadFont(ComfortaaBold14);
    m_tft->setTextDatum(TC_DATUM);
    m_tft->setTextColor(COLOR_TEXT_DIM, COLOR_BG);
    m_tft->drawString(STR(BreatherKey::BREATHER_DONE_HINT), DISPLAY_W / 2, 290);
    m_tft->unloadFont();
}

// ===================================================================
// Digitaluhr
// ===================================================================

/**
 * @brief Zeichnet die Digitaluhr
 */
void Breather::drawDigitalClock(int32_t x, int32_t y)
{
    if (!m_clockSprite)
        return;

    DS1307_RTC *rtc = &APP_STATE.getRTC();
    Clocks::drawDigitalClockPlain(m_clockSprite, rtc, m_lastDrawnSecond, x, y, false, true);
}

// ===================================================================
// Font-Helfer
// ===================================================================

/**
 * @brief Lädt einen Visualisierungs-Font nur bei echtem Wechsel
 */
void Breather::useVizFont(BreatherFontId fontId)
{
    if (!m_vizSprite || m_vizFont == fontId)
        return;

    if (m_vizFont != BreatherFontId::NONE)
        m_vizSprite->unloadFont();

    switch (fontId)
    {
    case BreatherFontId::LEAGUE_GOTHIC34:
        m_vizSprite->loadFont(LeagueGothicCond34);
        break;
    case BreatherFontId::COMFORTAA_BOLD14:
        m_vizSprite->loadFont(ComfortaaBold14);
        break;
    case BreatherFontId::COMFORTAA_BOLD16:
        m_vizSprite->loadFont(ComfortaaBold16);
        break;
    case BreatherFontId::NONE:
        break;
    }

    m_vizFont = fontId;
}

/**
 * @brief Entlädt den aktiven Visualisierungs-Font
 */
void Breather::unloadVizFont()
{
    if (!m_vizSprite || m_vizFont == BreatherFontId::NONE)
        return;

    m_vizSprite->unloadFont();
    m_vizFont = BreatherFontId::NONE;
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Top-Level-Verteiler
 */
void Breather::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    // Beim Modulstart kann der Finger vom Breather-Button im MainScreen noch auf dem Glas liegen. Erst nach dem Loslassen ist das Modul scharf, sonst würde der gehaltene Touch sofort eine Kachel auswählen.
    if (m_touchHandover)
    {
        if (!touched)
            m_touchHandover = false;

        return;
    }

    // Exit ist auf jedem Bildschirm aktiv
    if (m_exitButton.isPressed() && !touched)
    {
        m_exitButton.press(false);
        if (m_callbacks.onExitRequested)
            m_callbacks.onExitRequested();

        return;
    }

    if (touched && m_exitButton.contains(tx, ty))
    {
        if (!m_exitButton.isPressed())
        {
            m_exitButton.press(true);
            m_exitButton.drawButton(true);
        }
    }
    else if (m_exitButton.isPressed())
    {
        m_exitButton.press(false);
        m_exitButton.drawButton(false);
    }

    // Bildschirm-spezifische Verarbeitung
    switch (m_screen)
    {
    case BreatherScreen::SELECTION:
        handleSelectionTouch(tx, ty, touched);
        break;

    case BreatherScreen::RUNNING:
        handleRunningTouch(tx, ty, touched);
        break;

    case BreatherScreen::DONE:
        handleDoneTouch(tx, ty, touched);
        break;
    }
}

/**
 * @brief Touch im Auswahl-Bildschirm
 */
void Breather::handleSelectionTouch(int32_t tx, int32_t ty, bool touched)
{
    // Beim Loslassen liefert der Touch-Treiber keine verlässlichen Koordinaten, deshalb wird die beim Press-Down gemerkte Kachel verwendet.
    if (!touched)
    {
        if (m_pressedSelectionTile >= 0)
        {
            const int8_t toStart = m_pressedSelectionTile;
            m_pressedSelectionTile = -1;
            startSession(static_cast<uint8_t>(toStart));
        }

        return;
    }

    const int8_t hit = findSelectionTileAt(tx, ty);

    if (m_pressedSelectionTile == -1)
        m_pressedSelectionTile = hit;
    else if (hit != m_pressedSelectionTile)
        m_pressedSelectionTile = -1; // Finger aus der ursprünglich gedrückten Kachel rausgeglitten, Tap wird verworfen.
}

/**
 * @brief Touch in laufender Sitzung
 */
void Breather::handleRunningTouch(int32_t tx, int32_t ty, bool touched)
{
    // Zurück-Button: bei Loslassen Sitzung beenden und zur Auswahl springen
    if (m_backButton.isPressed() && !touched)
    {
        m_backButton.press(false);
        m_backButton.drawButton(false);
        returnToSelection();
        return;
    }

    if (touched && m_backButton.contains(tx, ty) && m_backButton.isEnabled())
    {
        if (!m_backButton.isPressed())
        {
            m_backButton.press(true);
            m_backButton.drawButton(true);
        }
    }
    else if (m_backButton.isPressed())
    {
        m_backButton.press(false);
        m_backButton.drawButton(false);
    }

    // Pause-Button
    if (m_pauseButton.isPressed() && !touched)
    {
        m_pauseButton.press(false);
        m_pauseButton.drawButton(false);
        togglePause();
        return;
    }

    if (touched && m_pauseButton.contains(tx, ty) && m_pauseButton.isEnabled())
    {
        if (!m_pauseButton.isPressed())
        {
            m_pauseButton.press(true);
            m_pauseButton.drawButton(true);
        }
    }
    else if (m_pauseButton.isPressed())
    {
        m_pauseButton.press(false);
        m_pauseButton.drawButton(false);
    }

    // Skip-Button
    if (m_skipButton.isPressed() && !touched)
    {
        m_skipButton.press(false);
        m_skipButton.drawButton(false);
        skipExercise();
        return;
    }

    if (touched && m_skipButton.contains(tx, ty) && m_skipButton.isEnabled())
    {
        if (!m_skipButton.isPressed())
        {
            m_skipButton.press(true);
            m_skipButton.drawButton(true);
        }
    }
    else if (m_skipButton.isPressed())
    {
        m_skipButton.press(false);
        m_skipButton.drawButton(false);
    }
}

/**
 * @brief Touch im Abschluss-Bildschirm
 */
void Breather::handleDoneTouch(int32_t tx, int32_t ty, bool touched)
{
    if (m_doneButton.isPressed() && !touched)
    {
        m_doneButton.press(false);
        m_doneButton.drawButton(false);
        returnToSelection();
        return;
    }

    if (touched && m_doneButton.contains(tx, ty) && m_doneButton.isEnabled())
    {
        if (!m_doneButton.isPressed())
        {
            m_doneButton.press(true);
            m_doneButton.drawButton(true);
        }
    }
    else if (m_doneButton.isPressed())
    {
        m_doneButton.press(false);
        m_doneButton.drawButton(false);
    }
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Spielt den Phasenwechsel-Sound
 */
void Breather::playPhaseSound()
{
    if (m_callbacks.onPlaySound)
        m_callbacks.onPlaySound(PHASE_SOUND_PATH);
}

/**
 * @brief Formatiert eine Dauer als "M:SS" oder "MM:SS"
 */
void Breather::formatDurationShort(uint16_t totalSec, char *out, size_t outSize)
{
    const uint16_t mm = totalSec / 60;
    const uint16_t ss = totalSec % 60;
    stringutils::safeFormat(out, outSize, "%u:%02u", mm, ss);
}

/**
 * @brief Formatiert Sekunden als "SS" oder "M:SS"
 */
void Breather::formatSeconds(uint16_t sec, char *out, size_t outSize)
{
    if (sec < 60)
    {
        stringutils::safeFormat(out, outSize, "%u s", sec);
        return;
    }

    const uint16_t mm = sec / 60;
    const uint16_t ss = sec % 60;
    stringutils::safeFormat(out, outSize, "%u:%02u", mm, ss);
}
