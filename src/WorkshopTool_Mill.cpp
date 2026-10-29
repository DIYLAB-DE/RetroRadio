// ===================================================================
// WorkshopTool_Mill.cpp - Brettspiel Mühle gegen einen Bot-Gegner
// ===================================================================

#include "WorkshopTool_Mill.h"

#include <Arduino.h>
#include <Preferences.h>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"

#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold30.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Anonymer Namespace - file-local Konstanten und Helfer
// ===================================================================

namespace
{
    // ===================================================================
    // Inhaltsbereich-Layout
    // ===================================================================
    // Das 320 px große Brett und die Seitenleiste beginnen bei y=7 und schließen dadurch bündig mit dem 327 px hohen Inhaltsbereich ab.
    constexpr int TOP_GAP = 7;

    constexpr int BOARD_SIZE = 320; ///< Äußere Kantenlänge des Spielbretts
    constexpr int BOARD_PAD  = 20;  ///< Innen-Rand zwischen Spielfeld und Brettrand

    // Rechte Sidebar
    constexpr int SIDE_GAP   = 12;
    constexpr int SIDE_X_OFF = BOARD_SIZE + SIDE_GAP;
    constexpr int SIDE_W     = 215; ///< 547 - 320 - 12

    // ===================================================================
    // Sidebar-Karten (von oben nach unten, total = 320 px inkl. Lücken)
    // ===================================================================
    constexpr int CARD_R       = 8;
    constexpr int CARD_GAP     = 6;
    constexpr int CARD_PAD_X   = 12;
    constexpr int PHASE_H      = 40;
    constexpr int PLAYER_H     = 40;
    constexpr int STONES_H     = 72;
    constexpr int HINT_H       = 56;
    constexpr int STATS_H      = 88;

    // ===================================================================
    // Steine
    // ===================================================================
    constexpr int STONE_R           = 16; ///< Stein-Außenradius
    constexpr int POINT_DOT_R       = 4;  ///< Marker für freie Punkte
    constexpr int SELECT_RING_R     = 21; ///< Auswahl-Ring um eigenen Stein
    constexpr int SELECT_RING_THICK = 3;
    constexpr int CAPTURE_RING_R    = 21; ///< Roter Ring um schlagbare Bot-Steine
    constexpr int CAPTURE_RING_THICK = 3;
    constexpr int TARGET_HALO_R     = 11; ///< Marker für gültige Zielfelder

    constexpr int TOUCH_HIT_RADIUS  = 28; ///< Maximaler Abstand für Nearest-Point-Hittest

    // ===================================================================
    // Brett-Geometrie auf der 280×280 Innenfläche
    // ===================================================================
    // Sieben Stützstellen pro Achse: 0, 47, 93, 140, 187, 233, 280
    // Symmetrisch um 140, jedes Quadrat ~46 px voneinander entfernt.
    constexpr uint16_t POINT_X[24] = {
        0, 140, 280,         // Reihe 0  (oberer Rand)
        47, 140, 233,        // Reihe 1
        93, 140, 187,        // Reihe 2
        0,  47, 93,          // Reihe 3 (linke Hälfte der Mittelreihe)
        187, 233, 280,       // Reihe 3 (rechte Hälfte)
        93, 140, 187,        // Reihe 4
        47, 140, 233,        // Reihe 5
        0, 140, 280};        // Reihe 6 (unterer Rand)

    constexpr uint16_t POINT_Y[24] = {
        0, 0, 0,
        47, 47, 47,
        93, 93, 93,
        140, 140, 140,
        140, 140, 140,
        187, 187, 187,
        233, 233, 233,
        280, 280, 280};

    // ===================================================================
    // Adjazenz-Tabelle: pro Punkt bis zu vier benachbarte Punkte (-1 = leer)
    // ===================================================================
    constexpr int8_t ADJACENCY[24][4] = {
        /*  0 */ { 1,  9, -1, -1},
        /*  1 */ { 0,  2,  4, -1},
        /*  2 */ { 1, 14, -1, -1},
        /*  3 */ { 4, 10, -1, -1},
        /*  4 */ { 1,  3,  5,  7},
        /*  5 */ { 4, 13, -1, -1},
        /*  6 */ { 7, 11, -1, -1},
        /*  7 */ { 4,  6,  8, -1},
        /*  8 */ { 7, 12, -1, -1},
        /*  9 */ { 0, 10, 21, -1},
        /* 10 */ { 3,  9, 11, 18},
        /* 11 */ { 6, 10, 15, -1},
        /* 12 */ { 8, 13, 17, -1},
        /* 13 */ { 5, 12, 14, 20},
        /* 14 */ { 2, 13, 23, -1},
        /* 15 */ {11, 16, -1, -1},
        /* 16 */ {15, 17, 19, -1},
        /* 17 */ {12, 16, -1, -1},
        /* 18 */ {10, 19, -1, -1},
        /* 19 */ {16, 18, 20, 22},
        /* 20 */ {13, 19, -1, -1},
        /* 21 */ { 9, 22, -1, -1},
        /* 22 */ {19, 21, 23, -1},
        /* 23 */ {14, 22, -1, -1}};

    // ===================================================================
    // Mühle-Tabelle: 16 Drei-in-einer-Reihe-Konstellationen
    // ===================================================================
    constexpr uint8_t MILLS[16][3] = {
        // Acht horizontale Reihen
        { 0,  1,  2}, { 3,  4,  5}, { 6,  7,  8},
        { 9, 10, 11}, {12, 13, 14},
        {15, 16, 17}, {18, 19, 20}, {21, 22, 23},
        // Acht vertikale Spalten
        { 0,  9, 21}, { 3, 10, 18}, { 6, 11, 15},
        { 1,  4,  7}, {16, 19, 22}, { 8, 12, 17},
        { 5, 13, 20}, { 2, 14, 23}};

    // Pro Punkt die Indizes der zwei Mühlen, in denen er Mitglied ist
    constexpr uint8_t POINT_MILLS[24][2] = {
        /*  0 */ { 0,  8}, /*  1 */ { 0, 11}, /*  2 */ { 0, 15},
        /*  3 */ { 1,  9}, /*  4 */ { 1, 11}, /*  5 */ { 1, 14},
        /*  6 */ { 2, 10}, /*  7 */ { 2, 11}, /*  8 */ { 2, 13},
        /*  9 */ { 3,  8}, /* 10 */ { 3,  9}, /* 11 */ { 3, 10},
        /* 12 */ { 4, 13}, /* 13 */ { 4, 14}, /* 14 */ { 4, 15},
        /* 15 */ { 5, 10}, /* 16 */ { 5, 12}, /* 17 */ { 5, 13},
        /* 18 */ { 6,  9}, /* 19 */ { 6, 12}, /* 20 */ { 6, 14},
        /* 21 */ { 7,  8}, /* 22 */ { 7, 12}, /* 23 */ { 7, 15}};

    // ===================================================================
    // Footer-Buttons (5 Buttons: leicht | mittel | neu | reset | ?)
    // ===================================================================
    constexpr int FOOTER_BTN_W      = 72;
    constexpr int FOOTER_BTN_H      = 44;
    constexpr int FOOTER_BTN_R      = 6;
    constexpr int FOOTER_BTN_GAP    = 8;
    constexpr int FOOTER_BTN_MARGIN = 2;

    constexpr uint16_t TOGGLE_ACTIVE_FILL    = 0x328A;
    constexpr uint16_t TOGGLE_ACTIVE_TEXT    = 0xFFFF;
    constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;
    constexpr uint16_t TOGGLE_INACTIVE_FILL    = 0x0000;
    constexpr uint16_t TOGGLE_INACTIVE_TEXT    = 0x7BEF;
    constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8;

    // ===================================================================
    // Allgemeine Farbpalette
    // ===================================================================
    constexpr uint16_t C_BOARD_BG       = 0xC52E; ///< Brett-Hintergrund (helles Holz)
    constexpr uint16_t C_BOARD_FRAME    = 0x4124; ///< Brett-Außenrahmen (dunkles Holz)
    constexpr uint16_t C_LINE           = 0x6A88; ///< Brettlinien (mittleres Holzbraun)
    constexpr uint16_t C_POINT_DOT      = 0x6A88; ///< Marker auf freien Punkten
    constexpr uint16_t C_STONE_WHITE    = 0xEF7D; ///< Spieler-Stein hell
    constexpr uint16_t C_STONE_WHITE_O  = 0x4124; ///< Dunkler Outline-Ring für hellen Stein
    constexpr uint16_t C_STONE_BLACK    = 0x18C3; ///< Bot-Stein dunkel
    constexpr uint16_t C_STONE_BLACK_O  = 0x39E8; ///< Dunkler Outline-Ring
    constexpr uint16_t C_SELECT_RING    = 0x0451; ///< Dunkles Petrol für Auswahl-Highlight
    constexpr uint16_t C_TARGET         = 0x0408; ///< Dunkles Grün für gültige Zielfelder
    constexpr uint16_t C_CAPTURE_RING   = 0x9000; ///< Bordeaux-Rot für schlagbare Bot-Steine

    constexpr uint16_t C_CARD_BG        = 0x2945; ///< Sidebar-Karten-Hintergrund
    constexpr uint16_t C_CARD_LABEL     = 0xAD75; ///< Karten-Label gedimmt
    constexpr uint16_t C_CARD_VALUE     = 0xFFFF; ///< Karten-Hauptwert
    constexpr uint16_t C_CARD_VALUE_DIM = 0xDEDB; ///< Karten-Sekundärwert
    constexpr uint16_t C_HINT_BG        = 0x2104; ///< Hinweis-Karten-Hintergrund
    constexpr uint16_t C_HINT_FG        = 0xDEDB; ///< Hinweis-Text

    constexpr uint16_t C_OVERLAY_BG         = 0x0841; ///< Overlay-Innenfläche
    constexpr uint16_t C_OVERLAY_TEXT       = 0xFFFF;
    constexpr uint16_t C_OVERLAY_FRAME_WIN  = 0xFD20; ///< Orange für Sieg
    constexpr uint16_t C_OVERLAY_FRAME_LOSE = 0xC9C7; ///< Rot-Braun für Niederlage
    constexpr uint16_t C_OVERLAY_FRAME_DRAW = 0x7BEF; ///< Grau für Remis

    // ===================================================================
    // Dialog-Geometrie
    // ===================================================================
    constexpr int DIALOG_HELP_W   = 700;
    constexpr int DIALOG_HELP_X   = (800 - DIALOG_HELP_W) / 2;
    constexpr int DIALOG_RESET_W  = 480;
    constexpr int DIALOG_RESET_H  = 220;
    constexpr int DIALOG_RESET_X  = (800 - DIALOG_RESET_W) / 2;
    constexpr int DIALOG_RESET_Y  = (480 - DIALOG_RESET_H) / 2;

    // ===================================================================
    // Preferences
    // ===================================================================
    const char *const PREFS_NS       = "wsmill";
    const char *const PREFS_KEY_WON  = "won";
    const char *const PREFS_KEY_LOST = "lost";
    const char *const PREFS_KEY_DRAW = "draw";

    // ===================================================================
    // Bot-Suchtiefen pro Schwierigkeit und Phase
    // ===================================================================
    constexpr int DEPTH_EASY_PLACE = 2;
    constexpr int DEPTH_EASY_MOVE  = 3;
    constexpr int DEPTH_MED_PLACE  = 3;
    constexpr int DEPTH_MED_MOVE   = 4;

    // Künstliche Mindest-Denkzeit des Bots in Millisekunden
    constexpr uint32_t BOT_MIN_THINK_MS = 500;

    // 50-Züge-Regel: Anzahl Halbzüge ohne Mühle bis Remis
    constexpr int DRAW_NO_MILL_LIMIT = 50;

    // Maximalzahl der Züge in einem Brettzustand (wird vom Bot benötigt)
    constexpr int MAX_MOVES = 256;

    // ===================================================================
    // Bot: Kompakte Move-Repräsentation
    // ===================================================================
    /**
     * @brief Ein Halbzug für die Bot-Suche.
     *
     * @c from ist -1 in der Setzphase. @c cap ist der Index eines geschlagenen gegnerischen Steins, oder -1 wenn keine Mühle entstand.
     */
    struct Move
    {
        int8_t from;
        int8_t to;
        int8_t cap;
    };

    // ===================================================================
    // Zobrist-Hash (Lazy-Init beim ersten Aufruf)
    // ===================================================================
    uint64_t s_zobrist[24][2] = {};
    uint64_t s_zobristSide    = 0;
    bool     s_zobristReady   = false;

    /**
     * @brief Initialisiert die Zobrist-Tabelle einmalig per LCG.
     */
    void ensureZobristReady()
    {
        if (s_zobristReady)
            return;
        uint64_t state = 0xC0FFEE12345678ULL;
        for (int i = 0; i < 24; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                state            = state * 6364136223846793005ULL + 1442695040888963407ULL;
                s_zobrist[i][c]  = state;
            }
        }
        state          = state * 6364136223846793005ULL + 1442695040888963407ULL;
        s_zobristSide  = state;
        s_zobristReady = true;
    }

    // ===================================================================
    // Hilfen für Move-Generation und Mühle-Erkennung
    // ===================================================================

    /**
     * @brief Liefert den gegnerischen Farb-Tag.
     */
    inline WorkshopTool_Mill::Cell opponentOf(WorkshopTool_Mill::Cell c)
    {
        return (c == WorkshopTool_Mill::Cell::WHITE)
                   ? WorkshopTool_Mill::Cell::BLACK
                   : WorkshopTool_Mill::Cell::WHITE;
    }

    /**
     * @brief Prüft, ob die drei Punkte einer Mühle alle dem Eigentümer
     *        gehören.
     */
    inline bool millCompleted(const WorkshopTool_Mill::Cell cells[24],
                              int millIdx,
                              WorkshopTool_Mill::Cell owner)
    {
        return cells[MILLS[millIdx][0]] == owner
            && cells[MILLS[millIdx][1]] == owner
            && cells[MILLS[millIdx][2]] == owner;
    }

    /**
     * @brief Prüft, ob das Setzen eines Steins der Farbe @p owner auf
     *        @p idx eine neue Mühle schließt (idx muss bereits besetzt
     *        sein, wenn diese Funktion gerufen wird).
     */
    inline bool didFormMill(const WorkshopTool_Mill::Cell cells[24],
                            int idx,
                            WorkshopTool_Mill::Cell owner)
    {
        const uint8_t *mills = POINT_MILLS[idx];
        return millCompleted(cells, mills[0], owner)
            || millCompleted(cells, mills[1], owner);
    }

} // namespace

// ===================================================================
// Konstruktor
// ===================================================================

/**
 * @brief Default-Konstruktor - Spielzustand wird in onEnter() initialisiert.
 */
WorkshopTool_Mill::WorkshopTool_Mill() = default;

// ===================================================================
// Lebenszyklus
// ===================================================================

/**
 * @brief Initialisiert Spielzustand, Statistik, MessageBox und Footer-Buttons.
 */
void WorkshopTool_Mill::onEnter(LGFX *tft)
{
    ensureZobristReady();
    loadStats();

    m_difficulty = Difficulty::EASY; // Stufe nicht persistiert: immer auf leicht starten

    resetGame();
    initFooterButtons(tft);

    m_msg = std::make_unique<MessageBox>(tft);
    m_msgPurpose = MsgPurpose::NONE;
}

/**
 * @brief Räumt den Werkzeugzustand auf und gibt die MessageBox frei.
 */
void WorkshopTool_Mill::onExit()
{
    // Pause ggf. aufheben, sonst bliebe der Inhalt des nächsten Werkzeugs eingefroren.
    if (owner())
        owner()->setWorkContentPaused(false);

    m_buttonsInitialized = false;
    m_touchActive        = false;
    m_botPending         = false;
    m_msgPurpose         = MsgPurpose::NONE;
    m_msg.reset();
}

// ===================================================================
// Untertitel
// ===================================================================

/**
 * @brief Liefert einen statusabhängigen Untertitel als statischen Pointer.
 */
const char *WorkshopTool_Mill::getSubtitle() const
{
    WorkshopKey key;
    switch (m_state)
    {
    case GameState::WON:
        key = WorkshopKey::WORKSHOP_MILL_SUB_WON;
        break;
    case GameState::LOST:
        key = WorkshopKey::WORKSHOP_MILL_SUB_LOST;
        break;
    case GameState::DRAW:
        key = WorkshopKey::WORKSHOP_MILL_SUB_DRAW;
        break;
    default:
        if (m_botPending)
            key = WorkshopKey::WORKSHOP_MILL_SUB_BOT;
        else if (m_capture == CaptureMode::HUMAN_CHOOSE)
            key = WorkshopKey::WORKSHOP_MILL_SUB_CAPTURE;
        else
        {
            const Phase ph = phaseOf(m_pos, Cell::WHITE);
            switch (ph)
            {
            case Phase::PLACING:
                key = WorkshopKey::WORKSHOP_MILL_SUB_PLACING;
                break;
            case Phase::FLYING:
                key = WorkshopKey::WORKSHOP_MILL_SUB_FLYING;
                break;
            default:
                key = WorkshopKey::WORKSHOP_MILL_SUB_MOVING;
                break;
            }
        }
        break;
    }

    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), key);
    return m_subtitleBuf;
}

// ===================================================================
// Spiellogik - Phasen, Mühlen, Beweglichkeit
// ===================================================================

/**
 * @brief Setzt das Brett auf den Startzustand zurück.
 */
void WorkshopTool_Mill::resetGame()
{
    for (int i = 0; i < 24; ++i)
        m_pos.cells[i] = Cell::EMPTY;

    m_pos.whiteInHand   = 9;
    m_pos.blackInHand   = 9;
    m_pos.whiteOnBoard  = 0;
    m_pos.blackOnBoard  = 0;
    m_pos.sideToMove    = Cell::WHITE; // Spieler beginnt

    m_state           = GameState::PLAYING;
    m_capture         = CaptureMode::NONE;
    m_selected        = -1;
    m_movesSinceMill  = 0;
    m_botPending      = false;
    m_botMoveAt       = 0;
    m_resultBooked    = false;

    clearHistory();
}

/**
 * @brief Ermittelt die Phase eines Spielers (Setzen / Ziehen / Springen).
 */
WorkshopTool_Mill::Phase
WorkshopTool_Mill::phaseOf(const Position &p, Cell color) const
{
    const uint8_t inHand   = (color == Cell::WHITE) ? p.whiteInHand : p.blackInHand;
    const uint8_t onBoard  = (color == Cell::WHITE) ? p.whiteOnBoard : p.blackOnBoard;

    if (inHand > 0)
        return Phase::PLACING;
    if (onBoard <= 3)
        return Phase::FLYING;
    return Phase::MOVING;
}

/**
 * @brief Prüft, ob der Stein an idx Teil einer geschlossenen Mühle ist.
 */
bool WorkshopTool_Mill::isInMill(const Position &p, int idx) const
{
    const Cell owner = p.cells[idx];
    if (owner == Cell::EMPTY)
        return false;

    const uint8_t *mills = POINT_MILLS[idx];
    return millCompleted(p.cells, mills[0], owner)
        || millCompleted(p.cells, mills[1], owner);
}

/**
 * @brief Prüft, ob die Verteidigerseite Steine außerhalb von Mühlen hat.
 */
bool WorkshopTool_Mill::hasNonMillStones(const Position &p, Cell defender) const
{
    for (int i = 0; i < 24; ++i)
    {
        if (p.cells[i] == defender && !isInMill(p, i))
            return true;
    }
    return false;
}

/**
 * @brief Prüft, ob ein Stein an idx aus Sicht des Angreifers schlagbar ist.
 */
bool WorkshopTool_Mill::canCapture(const Position &p, int idx, Cell attacker) const
{
    const Cell defender = opponentOf(attacker);
    if (p.cells[idx] != defender)
        return false;

    // Steine in Mühlen sind tabu, außer alle gegnerischen Steine sind in Mühlen
    if (isInMill(p, idx) && hasNonMillStones(p, defender))
        return false;

    return true;
}

/**
 * @brief Prüft, ob die angegebene Farbe in der gegebenen Stellung
 *        überhaupt einen legalen Halbzug ausführen kann.
 */
bool WorkshopTool_Mill::hasAnyMove(const Position &p, Cell color) const
{
    const Phase ph = phaseOf(p, color);

    if (ph == Phase::PLACING)
    {
        for (int i = 0; i < 24; ++i)
            if (p.cells[i] == Cell::EMPTY)
                return true;
        return false;
    }

    if (ph == Phase::FLYING)
    {
        // Mindestens ein leeres Feld genügt - der Spieler darf überall hin
        for (int i = 0; i < 24; ++i)
            if (p.cells[i] == Cell::EMPTY)
                return true;
        return false;
    }

    // MOVING: Mindestens ein eigener Stein muss einen freien Nachbarn haben
    for (int i = 0; i < 24; ++i)
    {
        if (p.cells[i] != color)
            continue;
        for (int n = 0; n < 4; ++n)
        {
            const int8_t nb = ADJACENCY[i][n];
            if (nb < 0)
                break;
            if (p.cells[nb] == Cell::EMPTY)
                return true;
        }
    }
    return false;
}

// ===================================================================
// Spiellogik - Aktionen auf dem Brett
// ===================================================================

/**
 * @brief Setzt einen Stein der Farbe @p color auf @p idx.
 */
bool WorkshopTool_Mill::placeStone(Position &p, int idx, Cell color) const
{
    p.cells[idx] = color;
    if (color == Cell::WHITE)
    {
        if (p.whiteInHand > 0) p.whiteInHand--;
        p.whiteOnBoard++;
    }
    else
    {
        if (p.blackInHand > 0) p.blackInHand--;
        p.blackOnBoard++;
    }
    return didFormMill(p.cells, idx, color);
}

/**
 * @brief Verschiebt einen eigenen Stein nach @p to (Zug- oder Sprungphase).
 */
bool WorkshopTool_Mill::moveStone(Position &p, int from, int to) const
{
    const Cell color = p.cells[from];
    p.cells[from] = Cell::EMPTY;
    p.cells[to]   = color;
    return didFormMill(p.cells, to, color);
}

/**
 * @brief Entfernt einen Stein vom Brett.
 */
void WorkshopTool_Mill::removeStone(Position &p, int idx) const
{
    const Cell c = p.cells[idx];
    p.cells[idx] = Cell::EMPTY;
    if (c == Cell::WHITE)
    {
        if (p.whiteOnBoard > 0) p.whiteOnBoard--;
    }
    else if (c == Cell::BLACK)
    {
        if (p.blackOnBoard > 0) p.blackOnBoard--;
    }
}

/**
 * @brief Wechselt die am Zug befindliche Farbe.
 */
void WorkshopTool_Mill::switchSide(Position &p) const
{
    p.sideToMove = opponentOf(p.sideToMove);
}
// ===================================================================
// Spielzustands-Update und Statistik-Buchung
// ===================================================================

/**
 * @brief Aktualisiert den globalen Spielzustand nach einem Halbzug.
 */
void WorkshopTool_Mill::updateGameState()
{
    // Sieg-/Niederlage-Erkennung anhand der Steineanzahl
    if (m_pos.whiteOnBoard + m_pos.whiteInHand < 3)
    {
        m_state = GameState::LOST;
        bookResultIfNeeded();
        return;
    }
    if (m_pos.blackOnBoard + m_pos.blackInHand < 3)
    {
        m_state = GameState::WON;
        bookResultIfNeeded();
        return;
    }

    // Wer am Zug ist und nicht ziehen kann, hat verloren
    if (!hasAnyMove(m_pos, m_pos.sideToMove))
    {
        m_state = (m_pos.sideToMove == Cell::WHITE) ? GameState::LOST
                                                    : GameState::WON;
        bookResultIfNeeded();
        return;
    }

    // 50-Züge-Regel (nur in Zug-/Sprungphase relevant)
    if (m_movesSinceMill >= DRAW_NO_MILL_LIMIT)
    {
        m_state = GameState::DRAW;
        bookResultIfNeeded();
        return;
    }

    // Dreifache Stellungswiederholung (nur in Zug-/Sprungphase aktiv)
    if (isThreefoldRepetition())
    {
        m_state = GameState::DRAW;
        bookResultIfNeeded();
        return;
    }
}

/**
 * @brief Bucht das aktuelle Endergebnis genau einmal in der Statistik.
 */
void WorkshopTool_Mill::bookResultIfNeeded()
{
    if (m_resultBooked)
        return;

    switch (m_state)
    {
    case GameState::WON:  m_stWon++;  break;
    case GameState::LOST: m_stLost++; break;
    case GameState::DRAW: m_stDraw++; break;
    case GameState::PLAYING:
        return;
    }
    m_resultBooked = true;
    saveStats();
}

// ===================================================================
// Mensch-Aktionen (Tap auf Punkt, Schlag-Auswahl)
// ===================================================================

/**
 * @brief Wendet einen menschlichen Tap auf einen Brettpunkt an.
 */
bool WorkshopTool_Mill::handleHumanTapOnPoint(int idx)
{
    if (m_state != GameState::PLAYING)
        return false;
    if (m_pos.sideToMove != Cell::WHITE)
        return false; // Bot ist am Zug
    if (m_capture == CaptureMode::HUMAN_CHOOSE)
        return handleHumanCaptureTap(idx);

    const Phase ph = phaseOf(m_pos, Cell::WHITE);

    // Setzphase
    if (ph == Phase::PLACING)
    {
        if (m_pos.cells[idx] != Cell::EMPTY)
            return false;

        const bool millFormed = placeStone(m_pos, idx, Cell::WHITE);

        if (millFormed)
        {
            // Auf der Setzphase bleibt der 50-Züge-Zähler bei 0
            m_movesSinceMill = 0;
            // Es gibt mindestens einen schlagbaren Stein, sonst ist der Spieler ohnehin in einer Sondersituation; in der Setzphase kann der Bot anfangs noch keine Mühlen bilden, daher fast immer schlagbar.
            bool anyCapture = false;
            for (int i = 0; i < 24; ++i)
                if (canCapture(m_pos, i, Cell::WHITE)) { anyCapture = true; break; }

            if (anyCapture)
            {
                m_capture = CaptureMode::HUMAN_CHOOSE;
                return true;
            }
            // Kein Schlag möglich (z.B. nur Steine in gegnerischen Mühlen wenn auch alle nicht schlagbar wären) - direkt weitermachen
        }

        switchSide(m_pos);
        clearHistory(); // Setzphase verändert die Stellung monoton
        updateGameState();
        if (m_state == GameState::PLAYING)
            scheduleBotMove();
        return true;
    }

    // Zug- oder Sprungphase Tap auf eigenen Stein -> Selektion (oder Selektions-Wechsel)
    if (m_pos.cells[idx] == Cell::WHITE)
    {
        m_selected = idx;
        return true;
    }

    // Tap auf gegnerischen Stein -> Selektion aufheben
    if (m_pos.cells[idx] == Cell::BLACK)
    {
        if (m_selected >= 0)
        {
            m_selected = -1;
            return true;
        }
        return false;
    }

    // Tap auf leeren Punkt - nur sinnvoll, wenn ein eigener Stein selektiert ist
    if (m_selected < 0)
        return false;

    // Legalität prüfen
    bool legal = false;
    if (ph == Phase::FLYING)
    {
        legal = true;
    }
    else
    {
        for (int n = 0; n < 4; ++n)
        {
            const int8_t nb = ADJACENCY[m_selected][n];
            if (nb < 0)
                break;
            if (nb == idx) { legal = true; break; }
        }
    }
    if (!legal)
        return false;

    // Zug ausführen
    const bool millFormed = moveStone(m_pos, m_selected, idx);
    m_selected = -1;

    if (millFormed)
    {
        m_movesSinceMill = 0;
        clearHistory();

        bool anyCapture = false;
        for (int i = 0; i < 24; ++i)
            if (canCapture(m_pos, i, Cell::WHITE)) { anyCapture = true; break; }

        if (anyCapture)
        {
            m_capture = CaptureMode::HUMAN_CHOOSE;
            return true;
        }
    }
    else
    {
        m_movesSinceMill++;
    }

    switchSide(m_pos);
    recordHistory();
    updateGameState();
    if (m_state == GameState::PLAYING)
        scheduleBotMove();
    return true;
}

/**
 * @brief Wendet einen Schlag-Tap im CaptureMode::HUMAN_CHOOSE an.
 */
bool WorkshopTool_Mill::handleHumanCaptureTap(int idx)
{
    if (!canCapture(m_pos, idx, Cell::WHITE))
        return false;

    removeStone(m_pos, idx);
    m_capture        = CaptureMode::NONE;
    m_movesSinceMill = 0;

    switchSide(m_pos);
    // Nach dem Schlag ist der Verlauf für Wiederholungen auf null (Schläge sind irreversible Stellungs-Verbesserungen)
    clearHistory();
    updateGameState();
    if (m_state == GameState::PLAYING)
        scheduleBotMove();
    return true;
}

// ===================================================================
// Bot - Planung und Ausführung
// ===================================================================

/**
 * @brief Plant einen Bot-Zug ein und setzt die Mindest-Denkzeit.
 */
void WorkshopTool_Mill::scheduleBotMove()
{
    if (m_pos.sideToMove != Cell::BLACK)
        return;
    m_botPending = true;
    m_botMoveAt  = millis() + BOT_MIN_THINK_MS;
}

/**
 * @brief Führt den eingeplanten Bot-Zug aus, sobald die Wartezeit um ist.
 */
bool WorkshopTool_Mill::tickBot()
{
    if (!m_botPending)
        return false;
    if (m_state != GameState::PLAYING)
    {
        m_botPending = false;
        return false;
    }
    if (static_cast<int32_t>(millis() - m_botMoveAt) < 0)
        return false;

    int from = -1;
    int to   = -1;
    int cap  = -1;
    computeBotMove(from, to, cap);

    if (to < 0)
    {
        // Sollte nicht vorkommen - wenn doch, gilt der Bot als bewegungslos
        m_botPending = false;
        m_state      = GameState::WON;
        bookResultIfNeeded();
        return true;
    }

    bool millFormed;
    if (from < 0)
    {
        millFormed = placeStone(m_pos, to, Cell::BLACK);
    }
    else
    {
        millFormed = moveStone(m_pos, from, to);
    }

    if (millFormed && cap >= 0)
    {
        removeStone(m_pos, cap);
        m_movesSinceMill = 0;
        clearHistory();
    }
    else
    {
        if (from >= 0) // Setzphase zählt nicht für die 50-Züge-Regel
            m_movesSinceMill++;
    }

    switchSide(m_pos);
    if (from >= 0 && !millFormed)
        recordHistory();
    else if (from < 0)
        clearHistory(); // Setzphase verändert Stellung monoton

    m_botPending = false;
    updateGameState();
    return true;
}

// ===================================================================
// Bot - Minimax mit Alpha-Beta
// ===================================================================

/**
 * @brief Liefert die Suchtiefe abhängig von Schwierigkeit und Phase.
 */
int WorkshopTool_Mill::searchDepth(const Position &p) const
{
    const bool placing = (p.whiteInHand > 0 || p.blackInHand > 0);
    if (m_difficulty == Difficulty::EASY)
        return placing ? DEPTH_EASY_PLACE : DEPTH_EASY_MOVE;
    return placing ? DEPTH_MED_PLACE : DEPTH_MED_MOVE;
}

namespace
{
    /**
     * @brief Generiert alle legalen Halbzüge für die am Zug befindliche Seite.
     * @return Anzahl der erzeugten Züge
     */
    int generateMoves(const WorkshopTool_Mill::Position &p,
                      Move *outMoves, int maxMoves)
    {
        using Cell = WorkshopTool_Mill::Cell;
        const Cell side = p.sideToMove;
        const Cell opp  = (side == Cell::WHITE) ? Cell::BLACK : Cell::WHITE;
        int count = 0;

        const bool placing = (side == Cell::WHITE) ? (p.whiteInHand > 0)
                                                   : (p.blackInHand > 0);
        const uint8_t onBoard = (side == Cell::WHITE) ? p.whiteOnBoard
                                                      : p.blackOnBoard;
        const bool flying = (!placing) && (onBoard <= 3);

        // Hilfslambda: Move + ggf. Schlag-Aufzählung emittieren
        auto emitWithCaptures = [&](int from, int to)
        {
            // Stein temporär setzen, um Mühlen-Erkennung durchzuführen
            WorkshopTool_Mill::Position np = p;
            if (from < 0)
            {
                np.cells[to] = side;
            }
            else
            {
                np.cells[from] = Cell::EMPTY;
                np.cells[to]   = side;
            }
            // Mühle entstanden?
            const uint8_t *mills = POINT_MILLS[to];
            const bool millFormed = millCompleted(np.cells, mills[0], side)
                                 || millCompleted(np.cells, mills[1], side);
            if (!millFormed)
            {
                if (count < maxMoves)
                    outMoves[count++] = Move{ static_cast<int8_t>(from),
                                              static_cast<int8_t>(to),
                                              -1 };
                return;
            }

            // "Hat die Verteidigerseite Steine außerhalb von Mühlen?"
            // Wir müssen die Mühlen-Information in np berechnen.
            bool defenderHasNonMill = false;
            for (int i = 0; i < 24 && !defenderHasNonMill; ++i)
            {
                if (np.cells[i] != opp)
                    continue;
                const uint8_t *m = POINT_MILLS[i];
                const bool inMill = millCompleted(np.cells, m[0], opp)
                                 || millCompleted(np.cells, m[1], opp);
                if (!inMill)
                    defenderHasNonMill = true;
            }

            // Schlagkandidaten sammeln
            for (int i = 0; i < 24; ++i)
            {
                if (np.cells[i] != opp)
                    continue;
                if (defenderHasNonMill)
                {
                    const uint8_t *m = POINT_MILLS[i];
                    const bool inMill = millCompleted(np.cells, m[0], opp)
                                     || millCompleted(np.cells, m[1], opp);
                    if (inMill)
                        continue;
                }
                if (count < maxMoves)
                    outMoves[count++] = Move{ static_cast<int8_t>(from),
                                              static_cast<int8_t>(to),
                                              static_cast<int8_t>(i) };
            }
        };

        if (placing)
        {
            for (int to = 0; to < 24; ++to)
                if (p.cells[to] == Cell::EMPTY)
                    emitWithCaptures(-1, to);
        }
        else if (flying)
        {
            for (int from = 0; from < 24; ++from)
            {
                if (p.cells[from] != side)
                    continue;
                for (int to = 0; to < 24; ++to)
                    if (p.cells[to] == Cell::EMPTY)
                        emitWithCaptures(from, to);
            }
        }
        else // MOVING
        {
            for (int from = 0; from < 24; ++from)
            {
                if (p.cells[from] != side)
                    continue;
                for (int n = 0; n < 4; ++n)
                {
                    const int8_t nb = ADJACENCY[from][n];
                    if (nb < 0) break;
                    if (p.cells[nb] == Cell::EMPTY)
                        emitWithCaptures(from, nb);
                }
            }
        }

        return count;
    }

    /**
     * @brief Wendet einen Move auf eine Position an (in-place).
     *
     * Aktualisiert auch Steine-Zähler und wechselt die am Zug befindliche Farbe. Der Aufrufer muss die ursprüngliche Position selbst sichern, wenn ein Undo nötig ist (typisch: lokale Kopie pro Knoten).
     */
    void applyMove(WorkshopTool_Mill::Position &p, const Move &m)
    {
        using Cell = WorkshopTool_Mill::Cell;
        const Cell side = p.sideToMove;
        if (m.from < 0)
        {
            p.cells[m.to] = side;
            if (side == Cell::WHITE) { p.whiteInHand--; p.whiteOnBoard++; }
            else                     { p.blackInHand--; p.blackOnBoard++; }
        }
        else
        {
            p.cells[m.from] = Cell::EMPTY;
            p.cells[m.to]   = side;
        }
        if (m.cap >= 0)
        {
            const Cell capCol = p.cells[m.cap];
            p.cells[m.cap] = Cell::EMPTY;
            if (capCol == Cell::WHITE) { if (p.whiteOnBoard > 0) p.whiteOnBoard--; }
            else                       { if (p.blackOnBoard > 0) p.blackOnBoard--; }
        }
        p.sideToMove = (side == Cell::WHITE) ? Cell::BLACK : Cell::WHITE;
    }
} // namespace

/**
 * @brief Statische Stellungsbewertung aus Sicht des Bots (BLACK).
 */
int WorkshopTool_Mill::evaluate(const Position &p) const
{
    const int blackTotal = p.blackOnBoard + p.blackInHand;
    const int whiteTotal = p.whiteOnBoard + p.whiteInHand;

    // Frühe Endbedingungen
    if (whiteTotal < 3) return  10000;
    if (blackTotal < 3) return -10000;

    int score = 0;
    score += (p.blackOnBoard - p.whiteOnBoard) * 100;
    score += (p.blackInHand  - p.whiteInHand)  * 30;

    // Mühlen zählen
    int blackMills = 0;
    int whiteMills = 0;
    for (int i = 0; i < 16; ++i)
    {
        const uint8_t a = MILLS[i][0], b = MILLS[i][1], c = MILLS[i][2];
        if (p.cells[a] == Cell::BLACK && p.cells[b] == Cell::BLACK && p.cells[c] == Cell::BLACK)
            blackMills++;
        else if (p.cells[a] == Cell::WHITE && p.cells[b] == Cell::WHITE && p.cells[c] == Cell::WHITE)
            whiteMills++;
    }
    score += (blackMills - whiteMills) * 40;

    // Mobilität: vereinfacht über Anzahl direkter Bewegungs-Möglichkeiten
    int blackMob = 0;
    int whiteMob = 0;
    for (int i = 0; i < 24; ++i)
    {
        if (p.cells[i] == Cell::EMPTY)
            continue;
        for (int n = 0; n < 4; ++n)
        {
            const int8_t nb = ADJACENCY[i][n];
            if (nb < 0) break;
            if (p.cells[nb] == Cell::EMPTY)
            {
                if (p.cells[i] == Cell::BLACK) blackMob++;
                else                           whiteMob++;
            }
        }
    }
    score += (blackMob - whiteMob) * 5;

    // Capture-Threat-Bonus: für jede Mühle, die eine Seite im nächsten Halbzug tatsächlich schließen könnte, wird ein Bonus vergeben. Das verhindert, dass der Bot zwischen zwei gleich bewerteten Stellungen schaukelt, statt eine bereitstehende Mühle wirklich zu schließen. Gewicht knapp unter dem statischen Mühlen-Score (×40), damit eine reale Mühle immer schwerer wiegt als eine drohende.
    constexpr int CAPTURE_THREAT_WEIGHT = 30;

    auto wouldCloseMillAt = [&](int target, Cell color, int excludeFrom) -> bool
    {
        // target ist aktuell EMPTY oder wird beim Zug durch excludeFrom frei. Die Prüfung ermittelt, ob color eine der beiden Mühlenlinien durch target schließt. excludeFrom wird als leer betrachtet, weil der ziehende Stein dieses Feld im gleichen Halbzug verlässt.
        const uint8_t *ml = POINT_MILLS[target];
        for (int k = 0; k < 2; ++k)
        {
            const uint8_t millIdx = ml[k];
            const uint8_t a = MILLS[millIdx][0];
            const uint8_t b = MILLS[millIdx][1];
            const uint8_t c = MILLS[millIdx][2];
            int own = 0;
            if (a != target && a != excludeFrom && p.cells[a] == color) own++;
            if (b != target && b != excludeFrom && p.cells[b] == color) own++;
            if (c != target && c != excludeFrom && p.cells[c] == color) own++;
            if (own == 2) return true;
        }
        return false;
    };

    auto countCaptureThreats = [&](Cell color) -> int
    {
        const Phase ph         = phaseOf(p, color);
        const int   onBoard    = (color == Cell::BLACK) ? p.blackOnBoard : p.whiteOnBoard;
        const int   inHand     = (color == Cell::BLACK) ? p.blackInHand  : p.whiteInHand;
        int threats = 0;

        if (ph == Phase::PLACING)
        {
            // In Setzphase nur wenn noch Steine in Hand sind
            if (inHand <= 0) return 0;
            for (int t = 0; t < 24; ++t)
                if (p.cells[t] == Cell::EMPTY && wouldCloseMillAt(t, color, -1))
                    threats++;
            return threats;
        }

        if (ph == Phase::FLYING)
        {
            // Springen: jeder eigene Stein kann auf jedes freie Feld. Der springende Stein verlässt sein Feld - das zählen wir konservativ nicht mit (würde eine Obergrenze an Threats liefern; da hier mehrere Quellen möglich sind, ist die genaue Zählung für den Heuristik-Bonus unnötig).
            if (onBoard <= 0) return 0;
            for (int t = 0; t < 24; ++t)
                if (p.cells[t] == Cell::EMPTY && wouldCloseMillAt(t, color, -1))
                    threats++;
            return threats;
        }

        // MOVING: eigene Steine entlang der Adjazenzen auf freie Nachbarn
        for (int from = 0; from < 24; ++from)
        {
            if (p.cells[from] != color) continue;
            for (int n = 0; n < 4; ++n)
            {
                const int8_t to = ADJACENCY[from][n];
                if (to < 0) break;
                if (p.cells[to] != Cell::EMPTY) continue;
                if (wouldCloseMillAt(to, color, from))
                    threats++;
            }
        }
        return threats;
    };

    const int blackThreats = countCaptureThreats(Cell::BLACK);
    const int whiteThreats = countCaptureThreats(Cell::WHITE);
    score += (blackThreats - whiteThreats) * CAPTURE_THREAT_WEIGHT;

    return score;
}

/**
 * @brief Rekursive Minimax-Suche mit Alpha-Beta-Pruning.
 */
int WorkshopTool_Mill::minimax(Position &p, int depth, int alpha, int beta, Cell maximizer) const
{
    // Materielle Spielende-Prüfung zuerst (weniger als 3 Steine = verloren)
    if (p.whiteOnBoard + p.whiteInHand < 3) return  10000 - (10 - depth);
    if (p.blackOnBoard + p.blackInHand < 3) return -10000 + (10 - depth);

    // Zugerzeugung VOR dem Tiefenabbruch: eine Stellung ohne legale Züge ist auch am Suchhorizont terminal (Blockade-Verlust für die Seite am Zug)
    Move moves[MAX_MOVES];
    const int n = generateMoves(p, moves, MAX_MOVES);
    if (n == 0)
    {
        // Wer am Zug ist und nicht ziehen kann, hat verloren
        return (p.sideToMove == maximizer) ? (-10000 + (10 - depth))
                                           : ( 10000 - (10 - depth));
    }

    // Erst jetzt darf die Heuristik übernehmen
    if (depth <= 0)
        return evaluate(p);

    const bool maxNode = (p.sideToMove == maximizer);

    if (maxNode)
    {
        int best = -32000;
        for (int i = 0; i < n; ++i)
        {
            Position np = p;
            applyMove(np, moves[i]);
            const int v = minimax(np, depth - 1, alpha, beta, maximizer);
            if (v > best) best = v;
            if (best > alpha) alpha = best;
            if (alpha >= beta) break;
        }
        return best;
    }
    else
    {
        int best = 32000;
        for (int i = 0; i < n; ++i)
        {
            Position np = p;
            applyMove(np, moves[i]);
            const int v = minimax(np, depth - 1, alpha, beta, maximizer);
            if (v < best) best = v;
            if (best < beta) beta = best;
            if (alpha >= beta) break;
        }
        return best;
    }
}

/**
 * @brief Berechnet den besten Bot-Zug per Minimax mit Alpha-Beta.
 */
void WorkshopTool_Mill::computeBotMove(int &outFrom, int &outTo, int &outCap)
{
    outFrom = -1;
    outTo   = -1;
    outCap  = -1;

    Move moves[MAX_MOVES];
    const int n = generateMoves(m_pos, moves, MAX_MOVES);
    if (n == 0)
        return;

    const int depth = searchDepth(m_pos);
    int bestScore = -32000;
    int bestIdx   = 0;

    for (int i = 0; i < n; ++i)
    {
        Position np = m_pos;
        applyMove(np, moves[i]);
        const int v = minimax(np, depth - 1, -32000, 32000, Cell::BLACK);
        if (v > bestScore)
        {
            bestScore = v;
            bestIdx   = i;
        }
    }

    outFrom = moves[bestIdx].from;
    outTo   = moves[bestIdx].to;
    outCap  = moves[bestIdx].cap;
}

// ===================================================================
// Zobrist-Hash und Wiederholungs-Erkennung
// ===================================================================

/**
 * @brief Berechnet einen 64-Bit-Hash der aktuellen Stellung.
 */
uint64_t WorkshopTool_Mill::hashPosition(const Position &p) const
{
    uint64_t h = 0;
    for (int i = 0; i < 24; ++i)
    {
        if (p.cells[i] == Cell::WHITE)
            h ^= s_zobrist[i][0];
        else if (p.cells[i] == Cell::BLACK)
            h ^= s_zobrist[i][1];
    }
    if (p.sideToMove == Cell::BLACK)
        h ^= s_zobristSide;
    return h;
}

/**
 * @brief Trägt die aktuelle Stellung in den Wiederholungs-Verlauf ein.
 */
void WorkshopTool_Mill::recordHistory()
{
    // In der Setzphase macht Wiederholungserkennung keinen Sinn
    if (m_pos.whiteInHand > 0 || m_pos.blackInHand > 0)
        return;

    if (m_historyCount >= HISTORY_CAP)
    {
        // Ältesten Eintrag verwerfen, um Platz zu schaffen
        for (int i = 1; i < HISTORY_CAP; ++i)
            m_history[i - 1] = m_history[i];
        m_historyCount = HISTORY_CAP - 1;
    }
    m_history[m_historyCount++] = hashPosition(m_pos);
}

/**
 * @brief Setzt den Wiederholungs-Verlauf zurück.
 */
void WorkshopTool_Mill::clearHistory()
{
    m_historyCount = 0;
}

/**
 * @brief Prüft, ob der zuletzt eingetragene Hash dreimal vorkommt.
 */
bool WorkshopTool_Mill::isThreefoldRepetition() const
{
    if (m_historyCount == 0)
        return false;
    const uint64_t last = m_history[m_historyCount - 1];
    int count = 0;
    for (int i = 0; i < m_historyCount; ++i)
        if (m_history[i] == last)
            ++count;
    return count >= 3;
}

// ===================================================================
// Persistenz
// ===================================================================

/**
 * @brief Liest die Statistik aus dem Preferences-Namespace "wsmill".
 */
void WorkshopTool_Mill::loadStats()
{
    Preferences prefs;
    if (prefs.begin(PREFS_NS, /*readOnly=*/true))
    {
        m_stWon  = prefs.getUInt(PREFS_KEY_WON,  0);
        m_stLost = prefs.getUInt(PREFS_KEY_LOST, 0);
        m_stDraw = prefs.getUInt(PREFS_KEY_DRAW, 0);
        prefs.end();
    }
    else
    {
        m_stWon = m_stLost = m_stDraw = 0;
    }
}

/**
 * @brief Schreibt die Statistik in den Preferences-Namespace "wsmill".
 */
void WorkshopTool_Mill::saveStats()
{
    Preferences prefs;
    if (prefs.begin(PREFS_NS, /*readOnly=*/false))
    {
        prefs.putUInt(PREFS_KEY_WON,  m_stWon);
        prefs.putUInt(PREFS_KEY_LOST, m_stLost);
        prefs.putUInt(PREFS_KEY_DRAW, m_stDraw);
        prefs.end();
    }
}

// ===================================================================
// Touch-Handling im Spielfeld
// ===================================================================

/**
 * @brief Verarbeitet Touch-Ereignisse auf dem Spielbrett (Touch-up-Semantik).
 *
 * Workshop liefert beim Loslassen handleTouch(0, 0, false), die Release- Koordinaten sind also unbrauchbar. Wir merken uns daher die zuletzt gesehene Position aus jedem touched=true-Frame in m_touchLastX/Y und werten den Tap beim Release gegen diesen gespeicherten Endpunkt aus.
 */
bool WorkshopTool_Mill::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    // Solange die MessageBox sichtbar ist, schluckt sie alle Brett-Touches.
    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();

        // Mill hat keine externe Verarbeitung - Bestätigung sofort abschließen.
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);

        m_touchActive = false;
        return false;
    }

    // Mindest-Denkzeit kann zwischen Touch-Frames ablaufen
    bool botTriggered = tickBot();

    if (touched)
    {
        m_touchActive = true;
        m_touchLastX  = tx;
        m_touchLastY  = ty;
        return botTriggered;
    }

    // touched=false ohne vorheriges touched=true -> ignorieren
    if (!m_touchActive)
        return botTriggered;

    m_touchActive = false;

    // Während eines Overlays oder während der Bot rechnet, kein Spieler-Tap
    if (m_state != GameState::PLAYING)
        return botTriggered;
    if (m_botPending)
        return botTriggered;
    if (m_pos.sideToMove != Cell::WHITE)
        return botTriggered;

    // Display-Koordinaten in tool-relative Koordinaten umrechnen
    const int cx = static_cast<int>(m_touchLastX) - WorkshopCfg::WORK_X - m_originX;
    const int cy = static_cast<int>(m_touchLastY) - WorkshopCfg::WORK_Y - m_originY;

    // Nearest-Point-Hittest auf das Brett (Brett beginnt bei tool-Y = TOP_GAP)
    const int idx = hitTestPoint(cx, cy, 0, TOP_GAP);
    if (idx < 0)
        return botTriggered;

    return handleHumanTapOnPoint(idx) || botTriggered;
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Brett und Sidebar in den Work-Sprite.
 */
void WorkshopTool_Mill::draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h)
{
    (void)w;
    (void)h;

    if (!sprite)
        return;

    // Solange die MessageBox sichtbar ist, nicht ins Sprite zeichnen - sonst würde der nächste Workshop-Push die Box überschreiben.
    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);
        return;
    }

    m_originX = ox;
    m_originY = oy;

    tickBot();

    const int boardX = ox;
    const int boardY = oy + TOP_GAP;
    const int sideX  = ox + SIDE_X_OFF;
    const int sideY  = oy + TOP_GAP;

    drawBoard(sprite, boardX, boardY);
    drawSidebar(sprite, sideX, sideY);

    if (m_state != GameState::PLAYING)
        drawOverlay(sprite, boardX, boardY);
}

// ===================================================================
// Footer-Buttons
// ===================================================================

/**
 * @brief Initialisiert die fünf Footer-Buttons mit Position und Stil.
 */
void WorkshopTool_Mill::initFooterButtons(LGFX *tft)
{
    constexpr int cy  = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int x0  = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN;
    constexpr int cx1 = x0 + FOOTER_BTN_W / 2;
    constexpr int cx2 = cx1 + FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int cx3 = cx2 + FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int cx4 = cx3 + FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int cx5 = cx4 + FOOTER_BTN_W + FOOTER_BTN_GAP;

    I18n::format(m_btnEasyLabel, sizeof(m_btnEasyLabel), WorkshopKey::WORKSHOP_MILL_BTN_EASY);
    I18n::format(m_btnMediumLabel, sizeof(m_btnMediumLabel), WorkshopKey::WORKSHOP_MILL_BTN_MEDIUM);
    I18n::format(m_btnNewLabel, sizeof(m_btnNewLabel), WorkshopKey::WORKSHOP_MILL_BTN_NEW);
    I18n::format(m_btnResetLabel, sizeof(m_btnResetLabel), WorkshopKey::WORKSHOP_MILL_BTN_RESET);

    m_btnEasy.initButton(
        tft, cx1, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
        TOGGLE_ACTIVE_OUTLINE, TOGGLE_ACTIVE_FILL, TOGGLE_ACTIVE_TEXT,
        m_btnEasyLabel, nullptr);
    m_btnEasy.setFont(LeagueGothicCond34);

    m_btnMedium.initButton(
        tft, cx2, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
        TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
        m_btnMediumLabel, nullptr);
    m_btnMedium.setFont(LeagueGothicCond34);

    m_btnNew.initButton(
        tft, cx3, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
        TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
        m_btnNewLabel, nullptr);
    m_btnNew.setFont(LeagueGothicCond34);

    m_btnReset.initButton(
        tft, cx4, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
        TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
        m_btnResetLabel, nullptr);
    m_btnReset.setFont(LeagueGothicCond34);

    m_btnHelp.initButton(
        tft, cx5, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
        TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
        "?", nullptr);
    m_btnHelp.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
}

/**
 * @brief Aktualisiert die Footer-Button-Farben nach einem Stufen-Wechsel.
 */
void WorkshopTool_Mill::updateDifficultyButtons()
{
    if (!m_buttonsInitialized)
        return;

    const bool easy = (m_difficulty == Difficulty::EASY);

    m_btnEasy.setOutlineColor(easy ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);
    m_btnEasy.setFillColor   (easy ? TOGGLE_ACTIVE_FILL    : TOGGLE_INACTIVE_FILL);
    m_btnEasy.setTextColor   (easy ? TOGGLE_ACTIVE_TEXT    : TOGGLE_INACTIVE_TEXT);

    m_btnMedium.setOutlineColor(easy ? TOGGLE_INACTIVE_OUTLINE : TOGGLE_ACTIVE_OUTLINE);
    m_btnMedium.setFillColor   (easy ? TOGGLE_INACTIVE_FILL    : TOGGLE_ACTIVE_FILL);
    m_btnMedium.setTextColor   (easy ? TOGGLE_INACTIVE_TEXT    : TOGGLE_ACTIVE_TEXT);
}

/**
 * @brief Zeichnet die Footer-Buttons abhängig vom Spielzustand.
 */
void WorkshopTool_Mill::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft; (void)x; (void)y; (void)w; (void)h;

    if (!m_buttonsInitialized)
        return;

    const bool playing = (m_state == GameState::PLAYING);

    if (playing)
    {
        m_btnEasy.drawButton();
        m_btnMedium.drawButton();
    }
    m_btnNew.drawButton();
    if (playing)
        m_btnReset.drawButton();
    m_btnHelp.drawButton();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
 */
bool WorkshopTool_Mill::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    // Solange die MessageBox sichtbar ist, ignoriert der Footer alle Touches
    if (m_msg && m_msg->isVisible())
        return false;

    const bool playing = (m_state == GameState::PLAYING);

    const bool onEasy   = playing && touched && m_btnEasy.contains(tx, ty);
    const bool onMedium = playing && touched && m_btnMedium.contains(tx, ty);
    const bool onNew    = touched && m_btnNew.contains(tx, ty);
    const bool onReset  = playing && touched && m_btnReset.contains(tx, ty);
    const bool onHelp   = touched && m_btnHelp.contains(tx, ty);

    const bool wasEasy   = m_btnEasy.isPressed();
    const bool wasMedium = m_btnMedium.isPressed();
    const bool wasNew    = m_btnNew.isPressed();
    const bool wasReset  = m_btnReset.isPressed();
    const bool wasHelp   = m_btnHelp.isPressed();

    if (playing) m_btnEasy.press(onEasy);
    if (playing) m_btnMedium.press(onMedium);
    m_btnNew.press(onNew);
    if (playing) m_btnReset.press(onReset);
    m_btnHelp.press(onHelp);

    // Aktion bei Loslassen
    if (playing && m_btnEasy.justReleased())
    {
        if (m_difficulty != Difficulty::EASY)
        {
            m_difficulty = Difficulty::EASY;
            updateDifficultyButtons();
            m_btnEasy.drawButton();
            m_btnMedium.drawButton();
        }
        return true;
    }

    if (playing && m_btnMedium.justReleased())
    {
        if (m_difficulty != Difficulty::MEDIUM)
        {
            m_difficulty = Difficulty::MEDIUM;
            updateDifficultyButtons();
            m_btnEasy.drawButton();
            m_btnMedium.drawButton();
        }
        return true;
    }

    if (m_btnNew.justReleased())
    {
        resetGame();
        return true;
    }

    if (playing && m_btnReset.justReleased())
    {
        showResetDialog();
        return false;
    }

    if (m_btnHelp.justReleased())
    {
        showHelpDialog();
        return false;
    }

    // Visuelle Press-Rückmeldung ohne Aktion
    if (playing && onEasy   != wasEasy)   m_btnEasy.drawButton(onEasy);
    if (playing && onMedium != wasMedium) m_btnMedium.drawButton(onMedium);
    if (onNew != wasNew)                  m_btnNew.drawButton(onNew);
    if (playing && onReset  != wasReset)  m_btnReset.drawButton(onReset);
    if (onHelp != wasHelp)                m_btnHelp.drawButton(onHelp);

    return false;
}

// ===================================================================
// MessageBox-Dialoge
// ===================================================================

/**
 * @brief Öffnet die Spielanleitung in einer modalen MessageBox.
 */
void WorkshopTool_Mill::showHelpDialog()
{
    if (!m_msg)
        return;

    m_msgPurpose = MsgPurpose::HELP;

    I18n::format(m_helpTitleBuf, sizeof(m_helpTitleBuf), WorkshopKey::WORKSHOP_MILL_HELP_TITLE);
    I18n::format(m_helpTextBuf, sizeof(m_helpTextBuf), WorkshopKey::WORKSHOP_MILL_HELP_TEXT);
    I18n::format(m_dialogCloseBuf, sizeof(m_dialogCloseBuf), WorkshopKey::WORKSHOP_DIALOG_CLOSE);

    m_msg->setTitle(m_helpTitleBuf);
    m_msg->setMessage(m_helpTextBuf);
    m_msg->setButtonLabel(m_dialogCloseBuf);
    m_msg->setTimeout(0);
    m_msg->setAutoBounds(DIALOG_HELP_X, 0, DIALOG_HELP_W, 480);
    m_msg->setCallback([this](bool confirmed) { onMsgResult(confirmed); });

    if (owner())
        owner()->setWorkContentPaused(true);

    m_msg->start();
}

/**
 * @brief Öffnet den Bestätigungsdialog zum Zurücksetzen der Statistik.
 */
void WorkshopTool_Mill::showResetDialog()
{
    if (!m_msg)
        return;

    m_msgPurpose = MsgPurpose::RESET_STATS;

    I18n::format(m_resetTitleBuf, sizeof(m_resetTitleBuf), WorkshopKey::WORKSHOP_MILL_RESET_TITLE);
    I18n::format(m_resetTextBuf, sizeof(m_resetTextBuf), WorkshopKey::WORKSHOP_MILL_RESET_TEXT);
    I18n::format(m_dialogOkBuf, sizeof(m_dialogOkBuf), WorkshopKey::WORKSHOP_DIALOG_OK);
    I18n::format(m_dialogCancelBuf, sizeof(m_dialogCancelBuf), WorkshopKey::WORKSHOP_DIALOG_CANCEL);

    m_msg->setBounds(DIALOG_RESET_X, DIALOG_RESET_Y, DIALOG_RESET_W, DIALOG_RESET_H);
    m_msg->setTitle(m_resetTitleBuf);
    m_msg->setMessage(m_resetTextBuf);
    m_msg->setButtonLabels(m_dialogOkBuf, m_dialogCancelBuf);
    m_msg->setTimeout(15000);

    m_msg->setCallback([this](bool confirmed) { onMsgResult(confirmed); });

    if (owner())
        owner()->setWorkContentPaused(true);

    m_msg->start();
}

/**
 * @brief Callback der MessageBox - wertet das Ergebnis je nach Zweck aus.
 */
void WorkshopTool_Mill::onMsgResult(bool confirmed)
{
    const MsgPurpose purpose = m_msgPurpose;
    m_msgPurpose = MsgPurpose::NONE;

    if (purpose == MsgPurpose::RESET_STATS && confirmed)
    {
        m_stWon = m_stLost = m_stDraw = 0;
        saveStats();
    }

    // Footer-Buttons aus dem gedrückten Zustand lösen. Während die Box offen war, hat Workshop die Touch-Events gesperrt - das justReleased() wurde nie ausgelöst, die Buttons stecken intern noch auf pressed=true.
    if (m_buttonsInitialized)
    {
        m_btnEasy.press(false);
        m_btnMedium.press(false);
        m_btnNew.press(false);
        m_btnReset.press(false);
        m_btnHelp.press(false);

        const bool playing = (m_state == GameState::PLAYING);
        if (playing)
        {
            m_btnEasy.drawButton();
            m_btnMedium.drawButton();
            m_btnReset.drawButton();
        }
        m_btnNew.drawButton();
        m_btnHelp.drawButton();
    }

    if (owner())
        owner()->setWorkContentPaused(false);
}

// ===================================================================
// Brett-Geometrie
// ===================================================================

/**
 * @brief Liefert die sprite-relativen Pixel-Koordinaten eines Brettpunkts.
 */
void WorkshopTool_Mill::pointPixel(int idx, int boardX, int boardY,
                                   int &outX, int &outY) const
{
    outX = boardX + BOARD_PAD + POINT_X[idx];
    outY = boardY + BOARD_PAD + POINT_Y[idx];
}

/**
 * @brief Sucht den nächstgelegenen Brettpunkt zu einem Touch-Punkt.
 */
int WorkshopTool_Mill::hitTestPoint(int cx, int cy, int boardX, int boardY) const
{
    int bestIdx  = -1;
    int bestDist = TOUCH_HIT_RADIUS * TOUCH_HIT_RADIUS;
    for (int i = 0; i < 24; ++i)
    {
        int px, py;
        pointPixel(i, boardX, boardY, px, py);
        const int dx = cx - px;
        const int dy = cy - py;
        const int d2 = dx * dx + dy * dy;
        if (d2 < bestDist)
        {
            bestDist = d2;
            bestIdx  = i;
        }
    }
    return bestIdx;
}

// ===================================================================
// Zeichnen - Spielbrett
// ===================================================================

/**
 * @brief Zeichnet das komplette Spielbrett inklusive Steine und Markierungen.
 */
void WorkshopTool_Mill::drawBoard(Gfx_Sprite *sprite, int boardX, int boardY) const
{
    // Brett-Hintergrund
    //sprite->fillSmoothRoundRect(boardX, boardY, BOARD_SIZE, BOARD_SIZE, 10, C_BOARD_FRAME);
    //sprite->fillSmoothRoundRect(boardX + 3, boardY + 3, BOARD_SIZE - 6, BOARD_SIZE - 6, 8, C_BOARD_BG);

    sprite->fillSmoothRoundRect(boardX, boardY, BOARD_SIZE, BOARD_SIZE, 10, C_BOARD_BG);

    // Brettlinien
    // Drei verschachtelte Quadrate
    auto drawSquare = [&](int p_topLeft, int p_topRight,
                          int p_bottomLeft, int p_bottomRight)
    {
        int x1, y1, x2, y2, x3, y3, x4, y4;
        pointPixel(p_topLeft,    boardX, boardY, x1, y1);
        pointPixel(p_topRight,   boardX, boardY, x2, y2);
        pointPixel(p_bottomLeft, boardX, boardY, x3, y3);
        pointPixel(p_bottomRight,boardX, boardY, x4, y4);
        sprite->drawWideLine(x1, y1, x2, y2, 2.0f, C_LINE);
        sprite->drawWideLine(x3, y3, x4, y4, 2.0f, C_LINE);
        sprite->drawWideLine(x1, y1, x3, y3, 2.0f, C_LINE);
        sprite->drawWideLine(x2, y2, x4, y4, 2.0f, C_LINE);
    };
    drawSquare( 0,  2, 21, 23); // Außen
    drawSquare( 3,  5, 18, 20); // Mittel
    drawSquare( 6,  8, 15, 17); // Innen

    // Vier Mittelverbindungen
    auto drawSegment = [&](int a, int b)
    {
        int x1, y1, x2, y2;
        pointPixel(a, boardX, boardY, x1, y1);
        pointPixel(b, boardX, boardY, x2, y2);
        sprite->drawWideLine(x1, y1, x2, y2, 2.0f, C_LINE);
    };
    drawSegment( 1,  4); // Oben mitte
    drawSegment( 4,  7);
    drawSegment( 9, 10); // Links mitte
    drawSegment(10, 11);
    drawSegment(12, 13); // Rechts mitte
    drawSegment(13, 14);
    drawSegment(16, 19); // Unten mitte
    drawSegment(19, 22);

    // Selektion: Zielfeld-Marker Gültige Ziele für den selektierten eigenen Stein hervorheben
    if (m_selected >= 0 && m_state == GameState::PLAYING
        && m_pos.sideToMove == Cell::WHITE
        && m_capture == CaptureMode::NONE)
    {
        const Phase ph = phaseOf(m_pos, Cell::WHITE);
        for (int i = 0; i < 24; ++i)
        {
            if (m_pos.cells[i] != Cell::EMPTY)
                continue;
            bool isTarget = false;
            if (ph == Phase::FLYING)
            {
                isTarget = true;
            }
            else
            {
                for (int n = 0; n < 4; ++n)
                {
                    const int8_t nb = ADJACENCY[m_selected][n];
                    if (nb < 0) break;
                    if (nb == i) { isTarget = true; break; }
                }
            }
            if (isTarget)
            {
                int px, py;
                pointPixel(i, boardX, boardY, px, py);
                sprite->drawSmoothCircle(px, py, TARGET_HALO_R, 2.0f,
                                         C_TARGET, C_BOARD_BG);
            }
        }
    }

    // Punkt-Marker auf freien Punkten
    for (int i = 0; i < 24; ++i)
    {
        if (m_pos.cells[i] != Cell::EMPTY)
            continue;
        int px, py;
        pointPixel(i, boardX, boardY, px, py);
        sprite->fillSmoothCircle(px, py, POINT_DOT_R, C_POINT_DOT);
    }

    // Steine
    // Steinfläche
    for (int i = 0; i < 24; ++i)
    {
        if (m_pos.cells[i] == Cell::EMPTY)
            continue;
        int px, py;
        pointPixel(i, boardX, boardY, px, py);
        sprite->fillSmoothCircle(px, py, STONE_R, (m_pos.cells[i] == Cell::WHITE) ?  C_STONE_WHITE : C_STONE_BLACK);
    }

    // Auswahl-Ring um selektierten eigenen Stein
    if (m_selected >= 0 && m_pos.cells[m_selected] == Cell::WHITE
        && m_capture == CaptureMode::NONE)
    {
        int px, py;
        pointPixel(m_selected, boardX, boardY, px, py);
        sprite->drawSmoothCircle(px, py, SELECT_RING_R,
                                 static_cast<float>(SELECT_RING_THICK),
                                 C_SELECT_RING, C_BOARD_BG);
    }

    // Schlag-Markierungen für Bot-Steine
    if (m_capture == CaptureMode::HUMAN_CHOOSE)
    {
        for (int i = 0; i < 24; ++i)
        {
            if (!canCapture(m_pos, i, Cell::WHITE))
                continue;
            int px, py;
            pointPixel(i, boardX, boardY, px, py);
            sprite->drawSmoothCircle(px, py, CAPTURE_RING_R,
                                     static_cast<float>(CAPTURE_RING_THICK),
                                     C_CAPTURE_RING, C_BOARD_BG);
        }
    }
}

// ===================================================================
// Zeichnen - Sidebar
// ===================================================================

/**
 * @brief Zeichnet die Sidebar-Karten an der angegebenen Position.
 */
void WorkshopTool_Mill::drawSidebar(Gfx_Sprite *sprite, int sideX, int sideY) const
{
    // Karten-Hintergründe
    int y = sideY;
    const int yPhase  = y; y += PHASE_H + CARD_GAP;
    const int yPlayer = y; y += PLAYER_H + CARD_GAP;
    const int yStones = y; y += STONES_H + CARD_GAP;
    const int yHint   = y; y += HINT_H + CARD_GAP;
    const int yStats  = y;

    sprite->fillSmoothRoundRect(sideX, yPhase,  SIDE_W, PHASE_H,  CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, yPlayer, SIDE_W, PLAYER_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, yStones, SIDE_W, STONES_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, yHint,   SIDE_W, HINT_H,   CARD_R, C_HINT_BG);
    sprite->fillSmoothRoundRect(sideX, yStats,  SIDE_W, STATS_H,  CARD_R, C_CARD_BG);

    // Texte mit Bold14 (Labels und Hinweise)
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);

    // Phase-Label - vertikal mittig in der Karte
    sprite->setTextDatum(ML_DATUM);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_MILL_LABEL_PHASE), sideX + CARD_PAD_X, yPhase + PHASE_H / 2);

    // Spieler-Label - vertikal mittig in der Karte
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_MILL_LABEL_TURN), sideX + CARD_PAD_X, yPlayer + PLAYER_H / 2);

    // Steine-Label - oben in der Karte (mehrzeiliger Inhalt folgt darunter)
    sprite->setTextDatum(TL_DATUM);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_MILL_LABEL_STONES), sideX + CARD_PAD_X, yStones + 6);

    // Hinweis-Text
    {
        const __FlashStringHelper *hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_DEFAULT);
        if (m_state == GameState::WON)       hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_WON);
        else if (m_state == GameState::LOST) hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_LOST);
        else if (m_state == GameState::DRAW) hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_DRAW);
        else if (m_capture == CaptureMode::HUMAN_CHOOSE)
                                             hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_CAPTURE);
        else if (m_botPending)               hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_BOT_THINKING);
        else
        {
            const Phase ph = phaseOf(m_pos, Cell::WHITE);
            if (ph == Phase::PLACING)        hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_TAP_FREE);
            else if (ph == Phase::FLYING)    hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_JUMP_FREE);
            else if (m_selected >= 0)        hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_PICK_TARGET);
            else                             hint = I18n::str(WorkshopKey::WORKSHOP_MILL_HINT_PICK_OWN);
        }
        sprite->setTextColor(C_HINT_FG, C_HINT_BG);
        sprite->setTextDatum(ML_DATUM);
        sprite->drawString(hint, sideX + CARD_PAD_X, yHint + HINT_H / 2);
    }

    // Steine-Karte: zwei Zeilen mit Label-Text
    {
        char buf[40];
        sprite->setTextColor(C_CARD_VALUE_DIM, C_CARD_BG);
        sprite->setTextDatum(TL_DATUM);

        // Zeile 1: Du
        I18n::format(buf, sizeof(buf), WorkshopKey::WORKSHOP_MILL_FMT_STONES_YOU,
                     static_cast<unsigned>(m_pos.whiteInHand),
                     static_cast<unsigned>(m_pos.whiteOnBoard));
        sprite->drawString(buf, sideX + CARD_PAD_X, yStones + 28);

        // Zeile 2: Bot
        I18n::format(buf, sizeof(buf), WorkshopKey::WORKSHOP_MILL_FMT_STONES_BOT,
                     static_cast<unsigned>(m_pos.blackInHand),
                     static_cast<unsigned>(m_pos.blackOnBoard));
        sprite->drawString(buf, sideX + CARD_PAD_X, yStones + 48);
    }

    // Statistik-Karte: Werte (Bold30) + Sub-Labels (Bold14) als vertikal zentrierter Block. Bold30 Höhe ~30, Gap 2, Bold14 Höhe ~14 -> total 46. Zentriert in STATS_H=88: Block beginnt bei yStats + 21. Werte-Mitte (MC_DATUM): yStats + 21 + 15 = yStats + 36 Label-Oberkante (TC_DATUM): yStats + 21 + 32 = yStats + 53
    {
        const int colW = SIDE_W / 3;
        const int sx0  = sideX + colW / 2;
        const int sx1  = sideX + colW + colW / 2;
        const int sx2  = sideX + 2 * colW + colW / 2;

        sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
        sprite->setTextDatum(TC_DATUM);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_MILL_LABEL_WINS),   sx0, yStats + 53);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_MILL_LABEL_LOSSES), sx1, yStats + 53);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_MILL_LABEL_DRAWS),  sx2, yStats + 53);
    }

    sprite->unloadFont();

    // Phase-Wert (Bold16) - vertikal mittig, rechts
    sprite->loadFont(ComfortaaBold16);
    sprite->setTextColor(C_CARD_VALUE, C_CARD_BG);
    sprite->setTextDatum(MR_DATUM);
    {
        const Phase phW = phaseOf(m_pos, Cell::WHITE);
        const __FlashStringHelper *phaseText = I18n::str(WorkshopKey::WORKSHOP_MILL_PHASE_PLACING);
        if (phW == Phase::FLYING)      phaseText = I18n::str(WorkshopKey::WORKSHOP_MILL_PHASE_FLYING);
        else if (phW == Phase::MOVING) phaseText = I18n::str(WorkshopKey::WORKSHOP_MILL_PHASE_MOVING);
        sprite->drawString(phaseText, sideX + SIDE_W - CARD_PAD_X,
                           yPhase + PHASE_H / 2);
    }

    // Spieler am Zug (Bold16) - vertikal mittig; rechts haben die Spielsteine einen äußeren Radius von 11 px und einen inneren Radius von 9 px.
    {
        const bool whiteToMove = (m_pos.sideToMove == Cell::WHITE)
                                  && (m_state == GameState::PLAYING);
        const __FlashStringHelper *whoText = whiteToMove
                                                 ? I18n::str(WorkshopKey::WORKSHOP_MILL_TURN_YOU)
                                                 : I18n::str(WorkshopKey::WORKSHOP_MILL_TURN_BOT);

        // Pin rechts, dann Text links daneben
        const int pinX = sideX + SIDE_W - CARD_PAD_X - 11;
        const int pinY = yPlayer + PLAYER_H / 2;

        sprite->setTextColor(C_CARD_VALUE, C_CARD_BG);
        sprite->setTextDatum(MR_DATUM);
        sprite->drawString(whoText, pinX - 16, pinY);

        if (whiteToMove)
        {
            sprite->fillSmoothCircle(pinX, pinY, 11, C_STONE_WHITE_O);
            sprite->fillSmoothCircle(pinX, pinY,  9, C_STONE_WHITE);
        }
        else
        {
            sprite->fillSmoothCircle(pinX, pinY, 11, C_STONE_BLACK_O);
            sprite->fillSmoothCircle(pinX, pinY,  9, C_STONE_BLACK);
        }
    }
    sprite->unloadFont();

    // Statistik-Werte (Bold30) - über den drei Label-Spalten
    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(C_CARD_VALUE, C_CARD_BG);
    {
        const int colW = SIDE_W / 3;
        const int sx0  = sideX + colW / 2;
        const int sx1  = sideX + colW + colW / 2;
        const int sx2  = sideX + 2 * colW + colW / 2;

        char buf[12];
        stringutils::safeFormat(buf, sizeof(buf), "%lu", static_cast<unsigned long>(m_stWon));
        sprite->drawString(buf, sx0, yStats + 36);
        stringutils::safeFormat(buf, sizeof(buf), "%lu", static_cast<unsigned long>(m_stLost));
        sprite->drawString(buf, sx1, yStats + 36);
        stringutils::safeFormat(buf, sizeof(buf), "%lu", static_cast<unsigned long>(m_stDraw));
        sprite->drawString(buf, sx2, yStats + 36);
    }
    sprite->unloadFont();
}

// ===================================================================
// Zeichnen - Overlay (Sieg / Niederlage / Remis)
// ===================================================================

/**
 * @brief Zeichnet ein zentriertes Overlay über dem Spielbrett.
 */
void WorkshopTool_Mill::drawOverlay(Gfx_Sprite *sprite, int boardX, int boardY) const
{
    constexpr int OV_W = 260;
    constexpr int OV_H = 120;

    const int ovX = boardX + (BOARD_SIZE - OV_W) / 2;
    const int ovY = boardY + (BOARD_SIZE - OV_H) / 2;

    uint16_t frameColor = C_OVERLAY_FRAME_DRAW;
    const __FlashStringHelper *title = I18n::str(WorkshopKey::WORKSHOP_MILL_OVERLAY_DRAW_TITLE);
    const __FlashStringHelper *line2 = I18n::str(WorkshopKey::WORKSHOP_MILL_OVERLAY_DRAW_LINE2);
    if (m_state == GameState::WON)
    {
        frameColor = C_OVERLAY_FRAME_WIN;
        title      = I18n::str(WorkshopKey::WORKSHOP_MILL_OVERLAY_WON_TITLE);
        line2      = I18n::str(WorkshopKey::WORKSHOP_MILL_OVERLAY_WON_LINE2);
    }
    else if (m_state == GameState::LOST)
    {
        frameColor = C_OVERLAY_FRAME_LOSE;
        title      = I18n::str(WorkshopKey::WORKSHOP_MILL_OVERLAY_LOST_TITLE);
        line2      = I18n::str(WorkshopKey::WORKSHOP_MILL_OVERLAY_LOST_LINE2);
    }

    // Outline-Trick: zwei fillSmoothRoundRect (außen Rahmen, innen Fläche)
    sprite->fillSmoothRoundRect(ovX, ovY, OV_W, OV_H, 12, frameColor);
    sprite->fillSmoothRoundRect(ovX + 3, ovY + 3, OV_W - 6, OV_H - 6, 10, C_OVERLAY_BG);

    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(C_OVERLAY_TEXT, C_OVERLAY_BG);
    sprite->drawString(title, ovX + OV_W / 2, ovY + 38);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold16);
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(C_OVERLAY_TEXT, C_OVERLAY_BG);
    sprite->drawString(line2, ovX + OV_W / 2, ovY + 80);
    sprite->unloadFont();
}
