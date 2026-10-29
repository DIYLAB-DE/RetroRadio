#include "WorkshopTool_LightsOut.h"

#include <Preferences.h>
#include <string.h>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold30.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================
namespace
{

    // Layout
    constexpr int TOP_GAP = 10;                                                        ///< Abstand von Tool-Top zum Brett-Top
    constexpr int BOARD_CELL = 58;                                                     ///< Größe einer Zelle (quadratisch)
    constexpr int BOARD_GAP = 4;                                                       ///< Abstand zwischen Zellen
    constexpr int BOARD_GRID = 5;                                                      ///< Anzahl Zellen pro Zeile/Spalte
    constexpr int BOARD_SIZE = BOARD_GRID * BOARD_CELL + (BOARD_GRID - 1) * BOARD_GAP; ///< Gesamtgröße des Bretts (quadratisch)
    constexpr int CELL_R = 8;                                                          ///< Radius der abgerundeten Ecken der Zellen
    constexpr int COLUMN_GAP = 14;                                                     ///< Abstand zwischen Brett und Sidebar

    constexpr int SIDE_W = 227;    ///< Breite der Sidebar rechts neben dem Brett
    constexpr int CARD_R = 8;      ///< Radius der abgerundeten Ecken der Info-Cards in der Sidebar
    constexpr int CARD_GAP = 6;    ///< Vertikaler Abstand zwischen den Info-Cards in der Sidebar
    constexpr int CARD_PAD_X = 12; ///< Horizontaler Innenabstand der Info-Cards (für Text und Zahlen)

    constexpr int CARD_MOVES_H = 56;                                                                     ///< Höhe der Info-Card für die Anzahl gespielter Züge
    constexpr int CARD_TIME_H = 56;                                                                      ///< Höhe der Info-Card für die verstrichene Zeit
    constexpr int CARD_STATUS_H = 44;                                                                    ///< Höhe der Status-Card, die anzeigt, ob die aktuelle Partie gelöst ist (grün) oder nicht (rot)
    constexpr int CARD_STATS_H = BOARD_SIZE - CARD_MOVES_H - CARD_TIME_H - CARD_STATUS_H - 3 * CARD_GAP; ///< Höhe der Card, die die Statistik anzeigt (restliche Höhe der Sidebar)

    // Farben
    constexpr uint16_t C_CELL_OFF_FILL = 0x18C3;    ///< Inaktive Zelle (dunkles Grün)
    constexpr uint16_t C_CELL_OFF_OUTLINE = 0x4208; ///< Inaktive Zelle Umriss (dunkles Grün mit etwas Blauanteil)
    constexpr uint16_t C_CELL_ON_FILL = 0xFD20;     ///< Aktive Zelle (leuchtendes Gelb)
    constexpr uint16_t C_CELL_ON_OUTLINE = 0xFEE0;  ///< Aktive Zelle Umriss (leuchtendes Gelb mit etwas Rotanteil)

    constexpr uint16_t C_CARD_BG = 0x2945;        ///< Hintergrundfarbe der Info-Cards in der Sidebar (dunkles Blaugrün)
    constexpr uint16_t C_CARD_LABEL = 0xAD75;     ///< Farbe der Labels in den Info-Cards (helles Grün)
    constexpr uint16_t C_CARD_VALUE = 0xFFFF;     ///< Farbe der Werte in den Info-Cards (Weiß)
    constexpr uint16_t C_CARD_VALUE_DIM = 0xDEDB; ///< Farbe der Werte in den Info-Cards, wenn die Partie gelöst ist (leicht gedimmt, damit die grüne Status-Card stärker hervortritt)
    constexpr uint16_t C_STATUS_SOLVED = 0x07E0;  ///< Farbe der Status-Card, wenn die Partie gelöst ist (leuchtendes Grün)

    constexpr uint16_t TOGGLE_ACTIVE_FILL = 0x328A;      ///< Füllfarbe eines aktiven Difficulty-Buttons (TealMid)
    constexpr uint16_t TOGGLE_ACTIVE_TEXT = 0xFFFF;      ///< Textfarbe eines aktiven Difficulty-Buttons (Weiß)
    constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;   ///< Umrissfarbe eines aktiven Difficulty-Buttons (TealMid)
    constexpr uint16_t TOGGLE_INACTIVE_FILL = 0x0000;    ///< Füllfarbe eines inaktiven Difficulty-Buttons (Schwarz)
    constexpr uint16_t TOGGLE_INACTIVE_TEXT = 0x7BEF;    ///< Textfarbe eines inaktiven Difficulty-Buttons (Hellgrau)
    constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8; ///< Umrissfarbe eines inaktiven Difficulty-Buttons (Dunkelgrau)

    // Footer-Buttons
    constexpr int FOOTER_BTN_W = 62;     ///< Breite der Footer-Buttons (leicht, mittel, schwer, neu, reset, ?)
    constexpr int FOOTER_BTN_H = 44;     ///< Höhe der Footer-Buttons (leicht, mittel, schwer, neu, reset, ?)
    constexpr int FOOTER_BTN_R = 6;      ///< Radius der abgerundeten Ecken der Footer-Buttons (leicht, mittel, schwer, neu, reset, ?)
    constexpr int FOOTER_BTN_GAP = 4;    ///< Horizontaler Abstand zwischen den Footer-Buttons (leicht, mittel, schwer, neu, reset, ?)
    constexpr int FOOTER_BTN_MARGIN = 2; ///< Abstand der ersten Footer-Button zum linken Rand des Tools

    // Spiel
    constexpr int SCRAMBLE_EASY = 3;   ///< Anzahl zufälliger Taps für die Verwürfelungsphase auf der Stufe "leicht"
    constexpr int SCRAMBLE_MEDIUM = 7; ///< Anzahl zufälliger Taps für die Verwürfelungsphase auf der Stufe "mittel"
    constexpr int SCRAMBLE_HARD = 12;  ///< Anzahl zufälliger Taps für die Verwürfelungsphase auf der Stufe "schwer"

    // Preferences
    const char *const PREFS_NS = "wslout";  ///< Namespace für die Preferences-Schlüssel
    const char *const PREFS_KEY_E_S = "es"; ///< Anzahl gelöster Partien auf Stufe "leicht"
    const char *const PREFS_KEY_E_B = "eb"; ///< Bestzeit (in Sekunden) auf Stufe "leicht"
    const char *const PREFS_KEY_M_S = "ms"; ///< Anzahl gelöster Partien auf Stufe "mittel"
    const char *const PREFS_KEY_M_B = "mb"; ///< Bestzeit (in Sekunden) auf Stufe "mittel"
    const char *const PREFS_KEY_H_S = "hs"; ///< Anzahl gelöster Partien auf Stufe "schwer"
    const char *const PREFS_KEY_H_B = "hb"; ///< Bestzeit (in Sekunden) auf Stufe "schwer"

    // Reset-Dialog
    constexpr int DIALOG_RESET_W = 480;                        ///< Breite der Reset-Bestätigungs-Dialogbox
    constexpr int DIALOG_RESET_H = 220;                        ///< Höhe der Reset-Bestätigungs-Dialogbox
    constexpr int DIALOG_RESET_X = (800 - DIALOG_RESET_W) / 2; ///< X-Position der Reset-Bestätigungs-Dialogbox (zentriert)
    constexpr int DIALOG_RESET_Y = (480 - DIALOG_RESET_H) / 2; ///< Y-Position der Reset-Bestätigungs-Dialogbox (zentriert)

    // Help-Dialog
    constexpr int DIALOG_HELP_W = 700;                       ///< Breite der Help-Dialogbox
    constexpr int DIALOG_HELP_X = (800 - DIALOG_HELP_W) / 2; ///< X-Position der Help-Dialogbox (zentriert)

    // Hilfsfunktionen

    /**
     * @brief Formatiert Sekunden als mm:ss in @p buf.
     */
    void formatTime(char *buf, size_t bufSize, uint32_t seconds)
    {
        if (seconds > 5999)
            seconds = 5999; // 99:59 cap

        stringutils::safeFormat(buf, bufSize, "%lu:%02lu", (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
    }

    /**
     * @brief Formatiert eine Bestzeit, "-:--" wenn noch nicht gesetzt.
     */
    void formatBest(char *buf, size_t bufSize, uint16_t bestSeconds)
    {
        if (bestSeconds == 0)
        {
            stringutils::safeCopy(buf, bufSize, "-:--");
            return;
        }

        formatTime(buf, bufSize, bestSeconds);
    }

} // namespace

// ===================================================================
// Konstruktor
// ===================================================================
WorkshopTool_LightsOut::WorkshopTool_LightsOut() = default;

// ===================================================================
// Lifecycle
// ===================================================================

/**
 * @brief Initialisiert Spielzustand, Statistik und Footer-Buttons.
 */
void WorkshopTool_LightsOut::onEnter(LGFX *tft)
{
    loadStats();

    // Einmaliges Seeding der Arduino-random()-Schnittstelle pro Tool-Sitzung. Quelle ist esp_random() kombiniert mit millis(), damit aufeinanderfolgende Verwürfelungen unterschiedliche Verläufe nehmen.
    randomSeed((uint32_t)millis() ^ (uint32_t)esp_random());

    m_difficulty = Difficulty::EASY;
    m_state = GameState::PLAYING;
    m_resultBooked = false;
    m_moves = 0;
    m_startMs = 0;
    m_solvedMs = 0;
    m_lastShownSeconds = 0;

    // Touch-Zustand sauber initialisieren
    m_touchActive = false;
    m_touchLastX = 0;
    m_touchLastY = 0;

    memset(m_cells, 0, sizeof(m_cells));
    resetGame();

    // MessageBox
    if (!m_msg)
        m_msg.reset(new MessageBox(tft));

    // Footer-Buttons
    if (!m_buttonsInitialized)
    {
        constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
        constexpr int x0 = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN;
        constexpr int cx1 = x0 + FOOTER_BTN_W / 2;
        constexpr int cx2 = cx1 + FOOTER_BTN_W + FOOTER_BTN_GAP;
        constexpr int cx3 = cx2 + FOOTER_BTN_W + FOOTER_BTN_GAP;
        constexpr int cx4 = cx3 + FOOTER_BTN_W + FOOTER_BTN_GAP;
        constexpr int cx5 = cx4 + FOOTER_BTN_W + FOOTER_BTN_GAP;
        constexpr int cx6 = cx5 + FOOTER_BTN_W + FOOTER_BTN_GAP;

        I18n::format(m_diffEasyLabel, sizeof(m_diffEasyLabel), WorkshopKey::WORKSHOP_LIGHTS_OUT_DIFF_EASY);
        I18n::format(m_diffMediumLabel, sizeof(m_diffMediumLabel), WorkshopKey::WORKSHOP_LIGHTS_OUT_DIFF_MEDIUM);
        I18n::format(m_diffHardLabel, sizeof(m_diffHardLabel), WorkshopKey::WORKSHOP_LIGHTS_OUT_DIFF_HARD);
        I18n::format(m_btnNewLabel, sizeof(m_btnNewLabel), WorkshopKey::WORKSHOP_LIGHTS_OUT_BTN_NEW);
        I18n::format(m_btnResetLabel, sizeof(m_btnResetLabel), WorkshopKey::WORKSHOP_LIGHTS_OUT_BTN_RESET);

        m_btnEasy.initButton(
            tft,                   // Display-Objekt
            cx1,                   // X-Position des Button-Zentrums
            cy,                    // Y-Position des Button-Zentrums
            FOOTER_BTN_W,          // Breite des Buttons
            FOOTER_BTN_H,          // Höhe des Buttons
            FOOTER_BTN_R,          // Radius der abgerundeten Ecken
            TOGGLE_ACTIVE_OUTLINE, // Umrissfarbe
            TOGGLE_ACTIVE_FILL,    // Füllfarbe
            TOGGLE_ACTIVE_TEXT,    // Textfarbe
            m_diffEasyLabel,       // Button-Label
            nullptr                // kein Icon
        );

        m_btnEasy.setFont(LeagueGothicCond34);

        m_btnMedium.initButton(
            tft,                     // Display-Objekt
            cx2,                     // X-Position des Button-Zentrums
            cy,                      // Y-Position des Button-Zentrums
            FOOTER_BTN_W,            // Breite des Buttons
            FOOTER_BTN_H,            // Höhe des Buttons
            FOOTER_BTN_R,            // Radius der abgerundeten Ecken
            TOGGLE_INACTIVE_OUTLINE, // Umrissfarbe
            TOGGLE_INACTIVE_FILL,    // Füllfarbe
            TOGGLE_INACTIVE_TEXT,    // Textfarbe
            m_diffMediumLabel,       // Button-Label
            nullptr                  // kein Icon
        );

        m_btnMedium.setFont(LeagueGothicCond34);

        m_btnHard.initButton(
            tft,                     // Display-Objekt
            cx3,                     // X-Position des Button-Zentrums
            cy,                      // Y-Position des Button-Zentrums
            FOOTER_BTN_W,            // Breite des Buttons
            FOOTER_BTN_H,            // Höhe des Buttons
            FOOTER_BTN_R,            // Radius der abgerundeten Ecken
            TOGGLE_INACTIVE_OUTLINE, // Umrissfarbe
            TOGGLE_INACTIVE_FILL,    // Füllfarbe
            TOGGLE_INACTIVE_TEXT,    // Textfarbe
            m_diffHardLabel,         // Button-Label
            nullptr                  // kein Icon
        );

        m_btnHard.setFont(LeagueGothicCond34);

        m_btnNew.initButton(
            tft,                     // Display-Objekt
            cx4,                     // X-Position des Button-Zentrums
            cy,                      // Y-Position des Button-Zentrums
            FOOTER_BTN_W,            // Breite des Buttons
            FOOTER_BTN_H,            // Höhe des Buttons
            FOOTER_BTN_R,            // Radius der abgerundeten Ecken
            TOGGLE_INACTIVE_OUTLINE, // Umrissfarbe
            TOGGLE_INACTIVE_FILL,    // Füllfarbe
            TOGGLE_INACTIVE_TEXT,    // Textfarbe
            m_btnNewLabel,           // Button-Label
            nullptr                  // kein Icon
        );

        m_btnNew.setFont(LeagueGothicCond34);

        m_btnReset.initButton(
            tft,                     // Display-Objekt
            cx5,                     // X-Position des Button-Zentrums
            cy,                      // Y-Position des Button-Zentrums
            FOOTER_BTN_W,            // Breite des Buttons
            FOOTER_BTN_H,            // Höhe des Buttons
            FOOTER_BTN_R,            // Radius der abgerundeten Ecken
            TOGGLE_INACTIVE_OUTLINE, // Umrissfarbe
            TOGGLE_INACTIVE_FILL,    // Füllfarbe
            TOGGLE_INACTIVE_TEXT,    // Textfarbe
            m_btnResetLabel,         // Button-Label
            nullptr                  // kein Icon
        );

        m_btnReset.setFont(LeagueGothicCond34);

        m_btnHelp.initButton(
            tft,                     ///< Display-Objekt
            cx6,                     ///< X-Position des Button-Zentrums
            cy,                      ///< Y-Position des Button-Zentrums
            FOOTER_BTN_W,            ///< Breite des Buttons
            FOOTER_BTN_H,            ///< Höhe des Buttons
            FOOTER_BTN_R,            ///< Radius der abgerundeten Ecken
            TOGGLE_INACTIVE_OUTLINE, ///< Umrissfarbe
            TOGGLE_INACTIVE_FILL,    ///< Füllfarbe
            TOGGLE_INACTIVE_TEXT,    ///< Textfarbe
            "?",                     ///< Button-Label
            nullptr                  ///< kein Icon
        );

        m_btnHelp.setFont(LeagueGothicCond34);
        m_buttonsInitialized = true;
    }

    updateDifficultyButtons();
}

/**
 * @brief Räumt den Werkzeugzustand auf.
 */
void WorkshopTool_LightsOut::onExit()
{
    m_msg.reset();
    m_msgPurpose = MsgPurpose::NONE;
    m_buttonsInitialized = false;

    // Touch-Zustand verwerfen, damit keine alte Tap-Sequenz beim erneuten Eintritt ins Tool fälschlich als gültig gewertet wird.
    m_touchActive = false;
    m_touchLastX = 0;
    m_touchLastY = 0;
}

// ===================================================================
// Spiel-Logik
// ===================================================================

/**
 * @brief Setzt das Brett zurück und verwürfelt für die aktuelle Stufe.
 */
void WorkshopTool_LightsOut::resetGame()
{
    memset(m_cells, 0, sizeof(m_cells));
    m_moves = 0;
    m_state = GameState::PLAYING;
    m_resultBooked = false;
    m_startMs = 0;
    m_solvedMs = 0;
    m_lastShownSeconds = 0;

    // Touch-Zustand bei jedem Neustart verwerfen, damit keine alte Tap-Sequenz über den Reset hinweg gültig bleibt.
    m_touchActive = false;
    m_touchLastX = 0;
    m_touchLastY = 0;

    // Verwürfelung durch zufällige Tap-Operationen auf paarweise verschiedenen Zellen. Jede so erzeugte Stellung ist garantiert lösbar, und die effektive Schwierigkeit entspricht exakt der Verwürfelungslänge, weil sich keine zwei Operationen gegenseitig neutralisieren können.
    const int n = scrambleLengthForDifficulty(m_difficulty);
    bool used[25] = {};
    int applied = 0;
    while (applied < n)
    {
        const int idx = static_cast<int>(random(0, 25));
        if (used[idx])
            continue;

        used[idx] = true;
        toggleCellAndNeighbors(idx / 5, idx % 5);
        applied++;
    }

    // Sonderfall: Kombination der Operationen ergibt die Nullstellung. In diesem Fall einen zusätzlichen Mittel-Tap ausführen, damit das Spiel nicht direkt als gelöst startet.
    if (isSolved())
        toggleCellAndNeighbors(2, 2);
}

/**
 * @brief Invertiert die Zelle (r,c) und ihre orthogonalen Nachbarn.
 */
void WorkshopTool_LightsOut::toggleCellAndNeighbors(int row, int col)
{
    auto flip = [this](int r, int c)
    {
        if (r < 0 || r >= BOARD_GRID || c < 0 || c >= BOARD_GRID)
            return;

        const int idx = r * BOARD_GRID + c;
        m_cells[idx] = !m_cells[idx];
    };

    flip(row, col);
    flip(row - 1, col);
    flip(row + 1, col);
    flip(row, col - 1);
    flip(row, col + 1);
}

/**
 * @brief Liefert true, wenn alle 25 Zellen aus sind.
 */
bool WorkshopTool_LightsOut::isSolved() const
{
    for (int i = 0; i < 25; ++i)
    {
        if (m_cells[i])
            return false;
    }

    return true;
}

/**
 * @brief Bucht das Lösungsergebnis genau einmal in der Statistik.
 */
void WorkshopTool_LightsOut::bookResultIfNeeded()
{
    if (m_resultBooked)
        return;
    m_resultBooked = true;

    const int slot = static_cast<int>(m_difficulty);
    if (m_stSolved[slot] < 0xFFFF)
        m_stSolved[slot]++;

    const uint32_t totalMs = (m_solvedMs >= m_startMs && m_startMs != 0)
                                 ? (m_solvedMs - m_startMs)
                                 : 0;

    uint16_t secs = (uint16_t)((totalMs + 500) / 1000);
    if (secs == 0)
        secs = 1; // Mindestens 1 Sekunde für "nichts"-Lösung

    if (secs > 5999)
        secs = 5999;

    if (m_stBest[slot] == 0 || secs < m_stBest[slot])
        m_stBest[slot] = secs;

    saveStats();
}

/**
 * @brief Liefert die Verwürfelungslänge für die angegebene Stufe.
 */
int WorkshopTool_LightsOut::scrambleLengthForDifficulty(Difficulty d) const
{
    switch (d)
    {
    case Difficulty::EASY:
        return SCRAMBLE_EASY;
    case Difficulty::MEDIUM:
        return SCRAMBLE_MEDIUM;
    case Difficulty::HARD:
        return SCRAMBLE_HARD;
    }

    return SCRAMBLE_EASY;
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Ereignisse auf dem Spielbrett (Touch-up-Semantik).
 */
bool WorkshopTool_LightsOut::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    // Solange die MessageBox sichtbar ist, schluckt sie alle Brett-Touches.
    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);

        m_touchActive = false;
        return false;
    }

    if (touched)
    {
        m_touchActive = true;
        m_touchLastX = tx;
        m_touchLastY = ty;
        return false;
    }

    if (!m_touchActive)
        return false;
    m_touchActive = false;

    if (m_state != GameState::PLAYING)
        return false;

    int row, col;
    if (!hitTestBoard(m_touchLastX, m_touchLastY, row, col))
        return false;

    // Erster Tap startet den Timer
    if (m_startMs == 0)
        m_startMs = millis();

    toggleCellAndNeighbors(row, col);
    m_moves++;

    if (isSolved())
    {
        m_state = GameState::SOLVED;
        m_solvedMs = millis();

        // Angezeigte Zeit sofort auf den gerundeten Endwert fixieren, damit die Sidebar exakt dieselbe Zahl zeigt, die bookResultIfNeeded als Bestzeit verbucht. Sonst entsteht ein Anzeige/Statistik-Mismatch.
        const uint32_t totalMs = (m_solvedMs >= m_startMs && m_startMs != 0)
                                     ? (m_solvedMs - m_startMs)
                                     : 0;

        uint16_t secs = static_cast<uint16_t>((totalMs + 500) / 1000);
        if (secs == 0 && m_startMs != 0)
            secs = 1;

        if (secs > 5999)
            secs = 5999;

        m_lastShownSeconds = secs;

        bookResultIfNeeded();
    }

    return true;
}

/**
 * @brief Wandelt absolute Touch-Koordinaten in (row,col) um.
 */
bool WorkshopTool_LightsOut::hitTestBoard(int32_t tx, int32_t ty, int &outRow, int &outCol) const
{
    // Display-Koordinaten in tool-relative Koordinaten umrechnen
    const int relAllX = static_cast<int>(tx) - WorkshopCfg::WORK_X - m_originX;
    const int relAllY = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_originY;

    // Brett beginnt bei tool-Y = TOP_GAP, tool-X = 0
    const int relX = relAllX;
    const int relY = relAllY - TOP_GAP;

    if (relX < 0 || relY < 0)
        return false;

    if (relX >= BOARD_SIZE || relY >= BOARD_SIZE)
        return false;

    const int stride = BOARD_CELL + BOARD_GAP;
    const int col = relX / stride;
    const int row = relY / stride;

    if (col < 0 || col >= BOARD_GRID || row < 0 || row >= BOARD_GRID)
        return false;

    // Tap im Gap zwischen Zellen ignorieren
    const int inCellX = relX - col * stride;
    const int inCellY = relY - row * stride;

    if (inCellX >= BOARD_CELL || inCellY >= BOARD_CELL)
        return false;

    outRow = row;
    outCol = col;
    return true;
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet Brett und Sidebar in den Work-Sprite.
 */
void WorkshopTool_LightsOut::draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h)
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

    // Aktuellen Sekundenwert in m_lastShownSeconds übernehmen. Das eigentliche Rendern der Zeitanzeige liest diesen Wert in drawSidebar.
    if (m_state == GameState::PLAYING && m_startMs != 0)
        m_lastShownSeconds = (uint16_t)((millis() - m_startMs) / 1000);

    drawBoard(sprite);
    drawSidebar(sprite);
}

/**
 * @brief Zeichnet das 5x5-Brett.
 */
void WorkshopTool_LightsOut::drawBoard(Gfx_Sprite *sprite)
{
    const int x0 = m_originX;
    const int y0 = m_originY + TOP_GAP;

    for (int row = 0; row < BOARD_GRID; ++row)
    {
        for (int col = 0; col < BOARD_GRID; ++col)
        {
            const int idx = row * BOARD_GRID + col;
            const int cx = x0 + col * (BOARD_CELL + BOARD_GAP);
            const int cy = y0 + row * (BOARD_CELL + BOARD_GAP);

            const bool on = m_cells[idx];
            const uint16_t outline = on ? C_CELL_ON_OUTLINE : C_CELL_OFF_OUTLINE;
            const uint16_t fill = on ? C_CELL_ON_FILL : C_CELL_OFF_FILL;

            // 2x fillSmoothRoundRect für abgerundetes Rechteck
            sprite->fillSmoothRoundRect(cx, cy, BOARD_CELL, BOARD_CELL, CELL_R, outline);
            sprite->fillSmoothRoundRect(cx + 2, cy + 2, BOARD_CELL - 4, BOARD_CELL - 4, CELL_R - 2, fill);
        }
    }
}

/**
 * @brief Zeichnet die rechte Sidebar.
 */
void WorkshopTool_LightsOut::drawSidebar(Gfx_Sprite *sprite)
{
    const int sideX = m_originX + BOARD_SIZE + COLUMN_GAP;
    const int sideY = m_originY + TOP_GAP;

    int y = sideY;
    const int yMoves = y;
    y += CARD_MOVES_H + CARD_GAP;
    const int yTime = y;
    y += CARD_TIME_H + CARD_GAP;
    const int yStatus = y;
    y += CARD_STATUS_H + CARD_GAP;
    const int yStats = y;

    // Karten-Hintergründe
    sprite->fillSmoothRoundRect(sideX, yMoves, SIDE_W, CARD_MOVES_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, yTime, SIDE_W, CARD_TIME_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, yStatus, SIDE_W, CARD_STATUS_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, yStats, SIDE_W, CARD_STATS_H, CARD_R, C_CARD_BG);

    char buf[32];

    // Vertikale Mittellinien der oberen zwei Karten
    const int cyMoves = yMoves + CARD_MOVES_H / 2;
    const int cyTime = yTime + CARD_TIME_H / 2;

    // Züge-/Zeit-Karte: Labels (links mittig)
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->setTextDatum(ML_DATUM);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LIGHTS_OUT_LABEL_MOVES), sideX + CARD_PAD_X, cyMoves);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LIGHTS_OUT_LABEL_TIME), sideX + CARD_PAD_X, cyTime);

    // Stats-Karte: Titel oben zentriert
    sprite->setTextDatum(TC_DATUM);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LIGHTS_OUT_LABEL_STATS), sideX + SIDE_W / 2, yStats + 6);
    sprite->unloadFont();

    // Züge-/Zeit-Karte: Werte (rechts mittig, tabularisch um Springen bei variablen Zifferbreiten zu vermeiden)
    sprite->loadFont(ComfortaaBold30);
    sprite->setTextColor(C_CARD_VALUE, C_CARD_BG);
    ensureTabMetrics(sprite, m_tab30);

    stringutils::safeFormat(buf, sizeof(buf), "%u", (unsigned)m_moves);
    drawTabularRight(sprite, buf, sideX + SIDE_W - CARD_PAD_X, cyMoves, m_tab30);

    formatTime(buf, sizeof(buf), m_lastShownSeconds);
    drawTabularRight(sprite, buf, sideX + SIDE_W - CARD_PAD_X, cyTime, m_tab30);
    sprite->unloadFont();

    // Status-Karte (zentriert)
    sprite->loadFont(ComfortaaBold16);
    sprite->setTextDatum(MC_DATUM);
   
    if (m_state == GameState::SOLVED)
    {
        sprite->setTextColor(C_STATUS_SOLVED, C_CARD_BG);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LIGHTS_OUT_STATUS_SOLVED), sideX + SIDE_W / 2, yStatus + CARD_STATUS_H / 2);
    }
    else
    {
        sprite->setTextColor(C_CARD_VALUE_DIM, C_CARD_BG);
        const __FlashStringHelper *txt = (m_startMs == 0)
                                             ? I18n::str(WorkshopKey::WORKSHOP_LIGHTS_OUT_STATUS_READY)
                                             : I18n::str(WorkshopKey::WORKSHOP_LIGHTS_OUT_STATUS_PLAYING);
        sprite->drawString(txt, sideX + SIDE_W / 2, yStatus + CARD_STATUS_H / 2);
    }
    
    sprite->unloadFont();

    // Stats-Karte: 3 Datenzeilen
    sprite->loadFont(ComfortaaBold14);

    const char *const labels[3] = {m_diffEasyLabel, m_diffMediumLabel, m_diffHardLabel};
    const int rowY0 = yStats + 26;
    const int rowH = (CARD_STATS_H - 30) / 3;

    for (int i = 0; i < 3; ++i)
    {
        const int ry = rowY0 + i * rowH;

        sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
        sprite->setTextDatum(TL_DATUM);
        sprite->drawString(labels[i], sideX + CARD_PAD_X, ry);

        char solvedBuf[12];
        char bestBuf[12];
        stringutils::safeFormat(solvedBuf, sizeof(solvedBuf), "%u", (unsigned)m_stSolved[i]);
        formatBest(bestBuf, sizeof(bestBuf), m_stBest[i]);

        char line[32];
        stringutils::safeFormat(line, sizeof(line), "%s   %s", solvedBuf, bestBuf);

        sprite->setTextColor(C_CARD_VALUE_DIM, C_CARD_BG);
        sprite->setTextDatum(TR_DATUM);
        sprite->drawString(line, sideX + SIDE_W - CARD_PAD_X, ry);
    }

    sprite->unloadFont();
}

/**
 * @brief Vermisst Ziffern- und Trennzeichenbreiten des aktuell geladenen Fonts.
 */
void WorkshopTool_LightsOut::ensureTabMetrics(Gfx_Sprite *sprite, TabMetrics &m)
{
    if (m.valid)
        return;

    int maxDigit = 0;
    char one[2] = {0, 0};

    for (char c = '0'; c <= '9'; ++c)
    {
        one[0] = c;
        const int w = sprite->textWidth(one);
        
        if (w > maxDigit)
            maxDigit = w;
    }

    m.digitW = maxDigit;
    m.colonW = sprite->textWidth(":");
    m.dashW = sprite->textWidth("-");
    m.valid = true;
}

/**
 * @brief Zeichnet Ziffernstring mit gleich breiten Slots, rechtsbündig endend bei rightX.
 */
void WorkshopTool_LightsOut::drawTabularRight(Gfx_Sprite *sprite, const char *text, int rightX, int cy, const TabMetrics &m)
{
    if (!text || !*text)
        return;

    // Slot-Breite pro Zeichen ermitteln und Gesamtbreite aufaddieren
    int totalW = 0;
    for (const char *p = text; *p; ++p)
    {
        if (*p == ':')
            totalW += m.colonW;
        else if (*p == '-')
            totalW += m.dashW;
        else
            totalW += m.digitW;
    }

    // Zeichen linksbündig an berechneten Slot-Zentren zeichnen
    int x = rightX - totalW;
    sprite->setTextDatum(MC_DATUM);

    char one[2] = {0, 0};
    for (const char *p = text; *p; ++p)
    {
        int slotW;
        if (*p == ':')
            slotW = m.colonW;
        else if (*p == '-')
            slotW = m.dashW;
        else
            slotW = m.digitW;

        one[0] = *p;
        sprite->drawString(one, x + slotW / 2, cy);
        x += slotW;
    }
}

// ===================================================================
// Footer
// ===================================================================

/**
 * @brief Aktualisiert die Toggle-Farben der drei Schwierigkeits-Buttons.
 */
void WorkshopTool_LightsOut::updateDifficultyButtons()
{
    auto apply = [](Button &b, bool active)
    {
        b.setOutlineColor(active ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);
        b.setFillColor(active ? TOGGLE_ACTIVE_FILL : TOGGLE_INACTIVE_FILL);
        b.setTextColor(active ? TOGGLE_ACTIVE_TEXT : TOGGLE_INACTIVE_TEXT);
    };

    apply(m_btnEasy, m_difficulty == Difficulty::EASY);
    apply(m_btnMedium, m_difficulty == Difficulty::MEDIUM);
    apply(m_btnHard, m_difficulty == Difficulty::HARD);
}

/**
 * @brief Zeichnet die Footer-Buttons.
 */
void WorkshopTool_LightsOut::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    m_btnEasy.drawButton();
    m_btnMedium.drawButton();
    m_btnHard.drawButton();
    m_btnNew.drawButton();
    m_btnReset.drawButton();
    m_btnHelp.drawButton();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
 */
bool WorkshopTool_LightsOut::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    if (m_msg && m_msg->isVisible())
        return false;

    const bool onEasy = touched && m_btnEasy.contains(tx, ty);
    const bool onMedium = touched && m_btnMedium.contains(tx, ty);
    const bool onHard = touched && m_btnHard.contains(tx, ty);
    const bool onNew = touched && m_btnNew.contains(tx, ty);
    const bool onReset = touched && m_btnReset.contains(tx, ty);
    const bool onHelp = touched && m_btnHelp.contains(tx, ty);

    const bool wasEasy = m_btnEasy.isPressed();
    const bool wasMedium = m_btnMedium.isPressed();
    const bool wasHard = m_btnHard.isPressed();
    const bool wasNew = m_btnNew.isPressed();
    const bool wasReset = m_btnReset.isPressed();
    const bool wasHelp = m_btnHelp.isPressed();

    m_btnEasy.press(onEasy);
    m_btnMedium.press(onMedium);
    m_btnHard.press(onHard);
    m_btnNew.press(onNew);
    m_btnReset.press(onReset);
    m_btnHelp.press(onHelp);

    // Aktion bei Loslassen
    if (m_btnEasy.justReleased())
    {
        if (m_difficulty != Difficulty::EASY)
        {
            m_difficulty = Difficulty::EASY;
            updateDifficultyButtons();
            resetGame();
            return true;
        }

        return false;
    }

    if (m_btnMedium.justReleased())
    {
        if (m_difficulty != Difficulty::MEDIUM)
        {
            m_difficulty = Difficulty::MEDIUM;
            updateDifficultyButtons();
            resetGame();
            return true;
        }

        return false;
    }

    if (m_btnHard.justReleased())
    {
        if (m_difficulty != Difficulty::HARD)
        {
            m_difficulty = Difficulty::HARD;
            updateDifficultyButtons();
            resetGame();
            return true;
        }
        return false;
    }

    if (m_btnNew.justReleased())
    {
        resetGame();
        return true;
    }

    if (m_btnReset.justReleased())
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
    if (onEasy != wasEasy)
        m_btnEasy.drawButton(onEasy);

    if (onMedium != wasMedium)
        m_btnMedium.drawButton(onMedium);

    if (onHard != wasHard)
        m_btnHard.drawButton(onHard);

    if (onNew != wasNew)
        m_btnNew.drawButton(onNew);

    if (onReset != wasReset)
        m_btnReset.drawButton(onReset);

    if (onHelp != wasHelp)
        m_btnHelp.drawButton(onHelp);

    return false;
}

/**
 * @brief Liefert einen statusabhängigen Untertitel.
 */
const char *WorkshopTool_LightsOut::getSubtitle() const
{
    if (m_state == GameState::SOLVED)
    {
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_LIGHTS_OUT_STATUS_SOLVED);
        return m_subtitleBuf;
    }

    const char *level = nullptr;
    switch (m_difficulty)
    {
    case Difficulty::EASY:
        level = m_diffEasyLabel;
        break;
    case Difficulty::MEDIUM:
        level = m_diffMediumLabel;
        break;
    case Difficulty::HARD:
        level = m_diffHardLabel;
        break;
    }

    if (level && level[0] != '\0')
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf),
                     WorkshopKey::WORKSHOP_LIGHTS_OUT_FMT_SUB_DIFFICULTY, level);
    else
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf),
                     WorkshopKey::WORKSHOP_LIGHTS_OUT_SUB_TITLE);

    return m_subtitleBuf;
}

// ===================================================================
// MessageBox-Dialog
// ===================================================================

/**
 * @brief Öffnet die Spielanleitung in einer modalen MessageBox.
 */
void WorkshopTool_LightsOut::showHelpDialog()
{
    if (!m_msg)
        return;

    m_msgPurpose = MsgPurpose::HELP;

    I18n::format(m_helpTitleBuf, sizeof(m_helpTitleBuf), WorkshopKey::WORKSHOP_LIGHTS_OUT_HELP_TITLE);
    I18n::format(m_helpTextBuf, sizeof(m_helpTextBuf), WorkshopKey::WORKSHOP_LIGHTS_OUT_HELP_TEXT);
    I18n::format(m_dialogCloseBuf, sizeof(m_dialogCloseBuf), WorkshopKey::WORKSHOP_DIALOG_CLOSE);

    m_msg->setTitle(m_helpTitleBuf);
    m_msg->setMessage(m_helpTextBuf);
    m_msg->setButtonLabel(m_dialogCloseBuf);
    m_msg->setTimeout(0);
    m_msg->setAutoBounds(DIALOG_HELP_X, 0, DIALOG_HELP_W, 480);
    m_msg->setCallback([this](bool confirmed)
                       { onMsgResult(confirmed); });

    if (owner())
        owner()->setWorkContentPaused(true);

    m_msg->start();
}

/**
 * @brief Öffnet den Bestätigungsdialog zum Zurücksetzen der Statistik.
 */
void WorkshopTool_LightsOut::showResetDialog()
{
    if (!m_msg)
        return;

    m_msgPurpose = MsgPurpose::RESET_STATS;

    I18n::format(m_resetTitleBuf, sizeof(m_resetTitleBuf), WorkshopKey::WORKSHOP_LIGHTS_OUT_RESET_TITLE);
    I18n::format(m_resetTextBuf, sizeof(m_resetTextBuf), WorkshopKey::WORKSHOP_LIGHTS_OUT_RESET_TEXT);
    I18n::format(m_dialogOkBuf, sizeof(m_dialogOkBuf), WorkshopKey::WORKSHOP_DIALOG_OK);
    I18n::format(m_dialogCancelBuf, sizeof(m_dialogCancelBuf), WorkshopKey::WORKSHOP_DIALOG_CANCEL);

    m_msg->setBounds(DIALOG_RESET_X, DIALOG_RESET_Y, DIALOG_RESET_W, DIALOG_RESET_H);
    m_msg->setTitle(m_resetTitleBuf);
    m_msg->setMessage(m_resetTextBuf);
    m_msg->setButtonLabels(m_dialogOkBuf, m_dialogCancelBuf);
    m_msg->setTimeout(15000);
    m_msg->setCallback([this](bool confirmed)
                       { onMsgResult(confirmed); });

    if (owner())
        owner()->setWorkContentPaused(true);

    m_msg->start();
}

/**
 * @brief Callback der MessageBox.
 */
void WorkshopTool_LightsOut::onMsgResult(bool confirmed)
{
    const MsgPurpose purpose = m_msgPurpose;
    m_msgPurpose = MsgPurpose::NONE;

    if (purpose == MsgPurpose::RESET_STATS && confirmed)
    {
        for (int i = 0; i < 3; ++i)
        {
            m_stSolved[i] = 0;
            m_stBest[i] = 0;
        }
        saveStats();
    }

    // Footer-Buttons in den Ruhezustand zurückversetzen und neu zeichnen.
    if (m_buttonsInitialized)
    {
        m_btnEasy.press(false);
        m_btnMedium.press(false);
        m_btnHard.press(false);
        m_btnNew.press(false);
        m_btnReset.press(false);
        m_btnHelp.press(false);

        m_btnEasy.drawButton();
        m_btnMedium.drawButton();
        m_btnHard.drawButton();
        m_btnNew.drawButton();
        m_btnReset.drawButton();
        m_btnHelp.drawButton();
    }

    if (owner())
        owner()->setWorkContentPaused(false);
}

// ===================================================================
// Persistenz
// ===================================================================

/**
 * @brief Liest die Statistik aus dem Preferences-Namespace "wslout".
 */
void WorkshopTool_LightsOut::loadStats()
{
    Preferences prefs;
    if (prefs.begin(PREFS_NS, /*readOnly=*/true))
    {
        m_stSolved[0] = (uint16_t)prefs.getUShort(PREFS_KEY_E_S, 0);
        m_stBest[0] = (uint16_t)prefs.getUShort(PREFS_KEY_E_B, 0);
        m_stSolved[1] = (uint16_t)prefs.getUShort(PREFS_KEY_M_S, 0);
        m_stBest[1] = (uint16_t)prefs.getUShort(PREFS_KEY_M_B, 0);
        m_stSolved[2] = (uint16_t)prefs.getUShort(PREFS_KEY_H_S, 0);
        m_stBest[2] = (uint16_t)prefs.getUShort(PREFS_KEY_H_B, 0);
        prefs.end();
    }
    else
    {
        for (int i = 0; i < 3; ++i)
        {
            m_stSolved[i] = 0;
            m_stBest[i] = 0;
        }
    }
}

/**
 * @brief Schreibt die Statistik in den Preferences-Namespace "wslout".
 */
void WorkshopTool_LightsOut::saveStats()
{
    Preferences prefs;
    if (prefs.begin(PREFS_NS, /*readOnly=*/false))
    {
        prefs.putUShort(PREFS_KEY_E_S, m_stSolved[0]);
        prefs.putUShort(PREFS_KEY_E_B, m_stBest[0]);
        prefs.putUShort(PREFS_KEY_M_S, m_stSolved[1]);
        prefs.putUShort(PREFS_KEY_M_B, m_stBest[1]);
        prefs.putUShort(PREFS_KEY_H_S, m_stSolved[2]);
        prefs.putUShort(PREFS_KEY_H_B, m_stBest[2]);
        prefs.end();
    }
}
