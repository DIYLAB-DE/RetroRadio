#include "WorkshopTool_Game2048.h"

#include <Arduino.h>
#include <Preferences.h>
#include <esp_random.h>

#include "I18n.h"
#include "MessageBox.h"
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
    // ===================================================================
    // Inhaltsbereich-Layout
    // ===================================================================
    constexpr int TOP_GAP = 7; ///< Vertikaler Versatz vom Header-Ende zum Spielfeld

    // Spielfeld: 4 Kacheln à 72 px + 5 Lücken à 6 px + 2 px Innenrand = 320 px
    constexpr int BOARD_SIZE = 320;    ///< Äußere Kantenlänge des Spielfelds
    constexpr int BOARD_PAD = 6;       ///< Lücke zwischen Kacheln und Rand
    constexpr int TILE_SIZE = 72;      ///< Kachel-Kantenlänge
    constexpr int TILE_R = 6;          ///< Eckenradius der Kacheln
    constexpr int BOARD_INNER_OFF = 1; ///< Restpuffer (1+5×6+4×72+1 = 320)

    // Rechte Spalte
    constexpr int SIDE_GAP = 12;                      ///< Abstand zwischen Brett und Spalte
    constexpr int SIDE_X_OFF = BOARD_SIZE + SIDE_GAP; ///< X-Versatz der Sidebar relativ zu ox
    constexpr int SIDE_W = 215;                       ///< Breite der Sidebar (= 547 - 320 - 12)

    // ===================================================================
    // Wisch-Erkennung
    // ===================================================================
    constexpr int SWIPE_MIN_PX = 36; ///< Minimale Auslenkung für gültige Wisch-Geste

    // ===================================================================
    // Sidebar-Karten (Score / Highscore)
    // ===================================================================
    constexpr int CARD_H = 64;     ///< Höhe einer Kennzahl-Karte
    constexpr int CARD_R = 8;      ///< Eckenradius der Karten
    constexpr int CARD_GAP = 8;    ///< Vertikaler Abstand zwischen Karten
    constexpr int HINT_BOX_H = 84; ///< Höhe der Wisch-Hinweis-Box

    // ===================================================================
    // Footer-Buttons
    // ===================================================================
    constexpr int FOOTER_BTN_W = 80;     ///< Breite der Buttons im Footer
    constexpr int FOOTER_BTN_H = 44;     ///< Höhe der Buttons im Footer
    constexpr int FOOTER_BTN_R = 6;      ///< Eckenradius der Buttons im Footer
    constexpr int FOOTER_BTN_GAP = 8;    ///< Abstand zwischen den Buttons im Footer
    constexpr int FOOTER_BTN_MARGIN = 2; ///< Abstand der Buttons zum Footer-Rand

    constexpr uint16_t TOGGLE_ACTIVE_FILL = 0x328A;      ///< Farbe für aktiven Toggle-Button (Hintergrund)
    constexpr uint16_t TOGGLE_ACTIVE_TEXT = 0xFFFF;      ///< Farbe für aktiven Toggle-Button (Text)
    constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;   ///< Farbe für aktiven Toggle-Button (Rahmen)
    constexpr uint16_t TOGGLE_INACTIVE_FILL = 0x0000;    ///< Farbe für inaktiven Toggle-Button (Hintergrund)
    constexpr uint16_t TOGGLE_INACTIVE_TEXT = 0x7BEF;    ///< Farbe für inaktiven Toggle-Button (Text)
    constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8; ///< Farbe für inaktiven Toggle-Button (Rahmen)

    // ===================================================================
    // Allgemeine Farbpalette (RetroRadio-Schema)
    // ===================================================================
    constexpr uint16_t C_BOARD_FRAME = 0xC52E;        ///< Spielfeld-Rahmen (helles Beige)
    constexpr uint16_t C_TILE_EMPTY = 0xD5B4;         ///< Leeres Feld (etwas heller als der Rahmen)
    constexpr uint16_t C_CARD_BG = 0x2945;            ///< Karten-Hintergrund
    constexpr uint16_t C_CARD_LABEL = 0xAD75;         ///< Karten-Label (gedimmt)
    constexpr uint16_t C_CARD_VALUE = 0xFFFF;         ///< Karten-Wert (hell)
    constexpr uint16_t C_HINT_BG = 0x2104;            ///< Hinweis-Box-Hintergrund
    constexpr uint16_t C_HINT_FG = 0xDEDB;            ///< Hinweis-Text
    constexpr uint16_t C_HINT_ARROW = 0xFD20;         ///< Pfeile in der Hinweis-Box (Orange)
    constexpr uint16_t C_OVERLAY_BG = 0x0841;         ///< Overlay-Schatten
    constexpr uint16_t C_OVERLAY_FRAME = 0xFD20;      ///< Overlay-Rahmen (Orange = Sieg)
    constexpr uint16_t C_OVERLAY_FRAME_LOSE = 0xC9C7; ///< Overlay-Rahmen (Rot-Braun = Game Over)
    constexpr uint16_t C_OVERLAY_TEXT = 0xFFFF;       ///< Overlay-Text

    // ===================================================================
    // Preferences-Namespace und -Schlüssel
    // ===================================================================
    const char *const PREFS_NS = "ws2048";
    const char *const PREFS_KEY_HIGH = "hi";

    // ===================================================================
    // MessageBox-Dialog
    // ===================================================================
    constexpr int DIALOG_HELP_W = 700;                       ///< Dialogbreite in Pixeln
    constexpr int DIALOG_HELP_X = (800 - DIALOG_HELP_W) / 2; ///< Horizontal zentriert auf 800 px Display

    constexpr int DIALOG_RESET_W = 480; ///< Breite des Reset-Bestätigungsdialogs
    constexpr int DIALOG_RESET_H = 220; ///< Feste Höhe des Reset-Dialogs
    constexpr int DIALOG_RESET_X = (800 - DIALOG_RESET_W) / 2;
    constexpr int DIALOG_RESET_Y = (480 - DIALOG_RESET_H) / 2;

    /**
     * @brief Liefert eine Zufallszahl im Bereich [0, n) über esp_random().
     */
    uint32_t randRange(uint32_t n)
    {
        return (n > 0) ? (esp_random() % n) : 0;
    }

} // namespace

// ===================================================================
// Lebenszyklus
// ===================================================================

/**
 * @brief Initialisiert Spielzustand, Highscore und Footer-Buttons.
 */
void WorkshopTool_Game2048::onEnter(LGFX *tft)
{
    loadHighscore();
    resetGame();
    initFooterButtons(tft);

    // MessageBox für Spielregeln-Dialog bereitstellen
    m_msg = std::make_unique<MessageBox>(tft);
    m_msgPurpose = MsgPurpose::NONE;
}

/**
 * @brief Räumt den Werkzeugzustand auf.
 */
void WorkshopTool_Game2048::onExit()
{
    // Sicherheitsnetz: Pause-Flag auf jeden Fall zurücknehmen, falls der Spieler das Tool mit offener Box verlässt.
    if (owner())
        owner()->setWorkContentPaused(false);

    m_msg.reset();
    m_msgPurpose = MsgPurpose::NONE;

    // Highscore wird bei jeder Verbesserung sofort persistiert, daher hier keine zusätzliche Speicheraktion nötig.
    m_buttonsInitialized = false;
    m_touchActive = false;
}

// ===================================================================
// Untertitel
// ===================================================================

/**
 * @brief Liefert einen statusabhängigen Untertitel als statischen Pointer.
 */
const char *WorkshopTool_Game2048::getSubtitle() const
{
    WorkshopKey key;
    switch (m_state)
    {
    case GameState::WON:
        key = WorkshopKey::WORKSHOP_GAME_2048_SUB_WON;
        break;
    case GameState::GAMEOVER:
        key = WorkshopKey::WORKSHOP_GAME_2048_SUB_GAMEOVER;
        break;
    default:
        key = WorkshopKey::WORKSHOP_GAME_2048_SUB_PLAYING;
        break;
    }

    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), key);
    return m_subtitleBuf;
}

// ===================================================================
// Persistenz
// ===================================================================

/**
 * @brief Liest den Highscore aus dem eigenen Preferences-Namespace.
 */
void WorkshopTool_Game2048::loadHighscore()
{
    Preferences prefs;
    if (prefs.begin(PREFS_NS, /*readOnly=*/true))
    {
        m_highscore = prefs.getUInt(PREFS_KEY_HIGH, 0);
        prefs.end();
    }
    else
    {
        m_highscore = 0;
    }
}

/**
 * @brief Schreibt den Highscore in den eigenen Preferences-Namespace.
 */
void WorkshopTool_Game2048::saveHighscore()
{
    Preferences prefs;
    if (prefs.begin(PREFS_NS, /*readOnly=*/false))
    {
        prefs.putUInt(PREFS_KEY_HIGH, m_highscore);
        prefs.end();
    }
}

// ===================================================================
// Spiellogik
// ===================================================================

/**
 * @brief Setzt das Brett zurück und legt zwei Startkacheln aus.
 */
void WorkshopTool_Game2048::resetGame()
{
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            m_board[r][c] = 0;

    m_score = 0;
    m_state = GameState::PLAYING;
    m_winAcknowledged = false;

    spawnRandomTile();
    spawnRandomTile();
}

/**
 * @brief Wählt zufällig ein leeres Feld und setzt eine 2 (90 %) oder 4 (10 %).
 */
bool WorkshopTool_Game2048::spawnRandomTile()
{
    // Leere Felder sammeln
    uint8_t empties[16];
    uint8_t count = 0;
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
            if (m_board[r][c] == 0)
                empties[count++] = static_cast<uint8_t>(r * 4 + c);
    }

    if (count == 0)
        return false;

    const uint8_t pick = empties[randRange(count)];
    const int r = pick / 4;
    const int c = pick % 4;

    // 10 % Wahrscheinlichkeit für eine 4
    m_board[r][c] = (randRange(10) == 0) ? 4 : 2;
    return true;
}

/**
 * @brief Schiebt eine Vierer-Linie nach links und verschmilzt benachbarte Paare.
 */
bool WorkshopTool_Game2048::slideAndMergeLine(uint16_t line[4], uint32_t &outScoreGain)
{
    uint16_t original[4] = {line[0], line[1], line[2], line[3]};
    uint16_t compact[4] = {0, 0, 0, 0};
    int writeIdx = 0;

    // 1. Nicht-Null-Werte nach links zusammenschieben
    for (int i = 0; i < 4; ++i)
        if (line[i] != 0)
            compact[writeIdx++] = line[i];

    // 2. Benachbarte Paare verschmelzen (jede Kachel höchstens einmal pro Zug)
    uint16_t merged[4] = {0, 0, 0, 0};
    int mWrite = 0;
    int i = 0;
    while (i < 4)
    {
        if (compact[i] == 0)
        {
            ++i;
            continue;
        }
        if (i + 1 < 4 && compact[i] == compact[i + 1])
        {
            const uint32_t newVal = static_cast<uint32_t>(compact[i]) * 2;
            merged[mWrite++] = static_cast<uint16_t>(newVal);
            outScoreGain += newVal;
            i += 2;
        }
        else
        {
            merged[mWrite++] = compact[i];
            ++i;
        }
    }

    // 3. Zurückschreiben
    for (int k = 0; k < 4; ++k)
        line[k] = merged[k];

    // 4. Veränderung detektieren
    for (int k = 0; k < 4; ++k)
        if (line[k] != original[k])
            return true;
    return false;
}

/**
 * @brief Führt einen Zug in die gewählte Richtung aus.
 */
bool WorkshopTool_Game2048::applyMove(Direction dir)
{
    if (dir == Direction::NONE)
        return false;

    bool anyChanged = false;
    uint32_t scoreGain = 0;

    // Alle vier Richtungen werden auf eine "schiebe Linie nach links"- Operation reduziert. Wir lesen jeweils vier Werte aus dem Brett in eine Hilfslinie, in der Index 0 der Zielkante in der gewählten Richtung entspricht, schieben links und schreiben sie in der gleichen Reihenfolge zurück.

    for (int idx = 0; idx < 4; ++idx)
    {
        uint16_t line[4];

        // Linie aus dem Brett extrahieren
        switch (dir)
        {
        case Direction::LEFT:
            for (int k = 0; k < 4; ++k)
                line[k] = m_board[idx][k];
            break;
        case Direction::RIGHT:
            for (int k = 0; k < 4; ++k)
                line[k] = m_board[idx][3 - k];
            break;
        case Direction::UP:
            for (int k = 0; k < 4; ++k)
                line[k] = m_board[k][idx];
            break;
        case Direction::DOWN:
            for (int k = 0; k < 4; ++k)
                line[k] = m_board[3 - k][idx];
            break;
        case Direction::NONE:
            return false;
        }

        if (slideAndMergeLine(line, scoreGain))
            anyChanged = true;

        // Linie zurückschreiben
        switch (dir)
        {
        case Direction::LEFT:
            for (int k = 0; k < 4; ++k)
                m_board[idx][k] = line[k];
            break;
        case Direction::RIGHT:
            for (int k = 0; k < 4; ++k)
                m_board[idx][3 - k] = line[k];
            break;
        case Direction::UP:
            for (int k = 0; k < 4; ++k)
                m_board[k][idx] = line[k];
            break;
        case Direction::DOWN:
            for (int k = 0; k < 4; ++k)
                m_board[3 - k][idx] = line[k];
            break;
        case Direction::NONE:
            break;
        }
    }

    if (!anyChanged)
        return false;

    // Score-Verarbeitung und Highscore-Update
    m_score += scoreGain;
    if (m_score > m_highscore)
    {
        m_highscore = m_score;
        saveHighscore();
    }

    spawnRandomTile();

    // Sieg-Erkennung (nur einmalig auslösen)
    if (!m_winAcknowledged && boardContains2048())
        m_state = GameState::WON;
    else if (!hasAnyMove())
        m_state = GameState::GAMEOVER;

    return true;
}

/**
 * @brief Prüft, ob das Brett eine 2048-Kachel enthält.
 */
bool WorkshopTool_Game2048::boardContains2048() const
{
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            if (m_board[r][c] >= 2048)
                return true;

    return false;
}

/**
 * @brief Prüft, ob in mindestens einer Richtung ein Zug möglich wäre.
 */
bool WorkshopTool_Game2048::hasAnyMove() const
{
    // Leere Felder erlauben immer einen Zug
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            if (m_board[r][c] == 0)
                return true;

    // Horizontale Nachbarn
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 3; ++c)
            if (m_board[r][c] == m_board[r][c + 1])
                return true;

    // Vertikale Nachbarn
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 3; ++r)
            if (m_board[r][c] == m_board[r + 1][c])
                return true;

    return false;
}

// ===================================================================
// Touch im Spielfeld
// ===================================================================

/**
 * @brief Verarbeitet Touch-Ereignisse zur Wisch-Erkennung.
 */
bool WorkshopTool_Game2048::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    // Wenn eine MessageBox aktiv ist, bekommt sie alle Touches im Inhaltsbereich und blockiert gleichzeitig die Wisch-Erkennung im Spiel.
    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);

        m_touchActive = false;
        return false;
    }

    // Während eines Overlays (Sieg/Game Over) sind Wisch-Gesten gesperrt. Der Spieler muss erst über den Footer entscheiden.
    if (m_state == GameState::WON || m_state == GameState::GAMEOVER)
    {
        m_touchActive = false;
        return false;
    }

    if (touched)
    {
        if (!m_touchActive)
        {
            m_touchActive = true;
            m_touchStartX = tx;
            m_touchStartY = ty;
        }

        // Letzten gültigen Punkt mitschreiben - beim Release sind tx/ty Null.
        m_touchLastX = tx;
        m_touchLastY = ty;
        return false;
    }

    // touched=false: Wisch-Auswertung gegen den letzten gültigen Punkt
    if (!m_touchActive)
        return false;

    m_touchActive = false;

    const int32_t dx = m_touchLastX - m_touchStartX;
    const int32_t dy = m_touchLastY - m_touchStartY;
    const int32_t adx = (dx >= 0) ? dx : -dx;
    const int32_t ady = (dy >= 0) ? dy : -dy;

    Direction dir = Direction::NONE;
    if (adx > ady)
    {
        if (adx >= SWIPE_MIN_PX)
            dir = (dx > 0) ? Direction::RIGHT : Direction::LEFT;
    }
    else
    {
        if (ady >= SWIPE_MIN_PX)
            dir = (dy > 0) ? Direction::DOWN : Direction::UP;
    }

    if (dir == Direction::NONE)
        return false;

    return applyMove(dir);
}

// ===================================================================
// Footer-Buttons
// ===================================================================

/**
 * @brief Initialisiert die vier Footer-Buttons mit Position und Stil.
 */
void WorkshopTool_Game2048::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int cx1 = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN + FOOTER_BTN_W / 2;
    constexpr int cx2 = cx1 + FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int cx3 = cx2 + FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int cx4 = cx3 + FOOTER_BTN_W + FOOTER_BTN_GAP;

    I18n::format(m_btnNewLabel, sizeof(m_btnNewLabel), WorkshopKey::WORKSHOP_GAME_2048_BTN_NEW);
    I18n::format(m_btnResetLabel, sizeof(m_btnResetLabel), WorkshopKey::WORKSHOP_GAME_2048_BTN_RESET);
    I18n::format(m_btnContinueLabel, sizeof(m_btnContinueLabel), WorkshopKey::WORKSHOP_GAME_2048_BTN_CONTINUE);

    m_btnNew.initButton(
        tft,                     // Display-Objekt
        cx1,                     // X-Mitte des Buttons
        cy,                      // Y-Mitte des Buttons
        FOOTER_BTN_W,            // Breite
        FOOTER_BTN_H,            // Höhe
        FOOTER_BTN_R,            // Eckenradius
        TOGGLE_INACTIVE_OUTLINE, // Rahmenfarbe
        TOGGLE_INACTIVE_FILL,    // Füllfarbe
        TOGGLE_INACTIVE_TEXT,    // Textfarbe
        m_btnNewLabel,           // Beschriftung
        nullptr                  // kein Icon
    );

    m_btnNew.setFont(LeagueGothicCond34);

    m_btnReset.initButton(
        tft,                     // Display-Objekt
        cx2,                     // X-Mitte des Buttons
        cy,                      // Y-Mitte des Buttons
        FOOTER_BTN_W,            // Breite
        FOOTER_BTN_H,            // Höhe
        FOOTER_BTN_R,            // Eckenradius
        TOGGLE_INACTIVE_OUTLINE, // Rahmenfarbe
        TOGGLE_INACTIVE_FILL,    // Füllfarbe
        TOGGLE_INACTIVE_TEXT,    // Textfarbe
        m_btnResetLabel,         // Beschriftung
        nullptr                  // kein Icon
    );

    m_btnReset.setFont(LeagueGothicCond34);

    m_btnHelp.initButton(
        tft,                     // Display-Objekt
        cx3,                     // X-Mitte des Buttons
        cy,                      // Y-Mitte des Buttons
        FOOTER_BTN_W,            // Breite
        FOOTER_BTN_H,            // Höhe
        FOOTER_BTN_R,            // Eckenradius
        TOGGLE_INACTIVE_OUTLINE, // Rahmenfarbe
        TOGGLE_INACTIVE_FILL,    // Füllfarbe
        TOGGLE_INACTIVE_TEXT,    // Textfarbe
        "?",                     // Beschriftung
        nullptr                  // kein Icon
    );

    m_btnHelp.setFont(LeagueGothicCond34);

    m_btnContinue.initButton(
        tft,                   // Display-Objekt
        cx4,                   // X-Mitte des Buttons
        cy,                    // Y-Mitte des Buttons
        FOOTER_BTN_W,          // Breite
        FOOTER_BTN_H,          // Höhe
        FOOTER_BTN_R,          // Eckenradius
        TOGGLE_ACTIVE_OUTLINE, // Rahmenfarbe
        TOGGLE_ACTIVE_FILL,    // Füllfarbe
        TOGGLE_ACTIVE_TEXT,    // Textfarbe
        m_btnContinueLabel,    // Beschriftung
        nullptr                // kein Icon
    );

    m_btnContinue.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
}

/**
 * @brief Zeichnet die Footer-Buttons abhängig vom Spielzustand.
 */
void WorkshopTool_Game2048::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    if (m_state == GameState::WON)
        m_btnContinue.drawButton();

    m_btnNew.drawButton();

    if (m_state == GameState::PLAYING)
        m_btnReset.drawButton();

    m_btnHelp.drawButton();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
 */
bool WorkshopTool_Game2048::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    // Während eine MessageBox sichtbar ist, Footer-Touches ignorieren.
    if (m_msg && m_msg->isVisible())
        return false;

    // "weiter" ist nur im Sieg-Overlay aktiv, "reset" nur während PLAYING
    const bool continueActive = (m_state == GameState::WON);
    const bool resetActive = (m_state == GameState::PLAYING);

    const bool onHelp = touched && m_btnHelp.contains(tx, ty);
    const bool onContinue = continueActive && touched && m_btnContinue.contains(tx, ty);
    const bool onNew = touched && m_btnNew.contains(tx, ty);
    const bool onReset = resetActive && touched && m_btnReset.contains(tx, ty);

    const bool helpWasPressed = m_btnHelp.isPressed();
    const bool continueWasPressed = m_btnContinue.isPressed();
    const bool newWasPressed = m_btnNew.isPressed();
    const bool resetWasPressed = m_btnReset.isPressed();

    m_btnHelp.press(onHelp);
    if (continueActive)
        m_btnContinue.press(onContinue);

    m_btnNew.press(onNew);
    if (resetActive)
        m_btnReset.press(onReset);

    // Aktion bei Loslassen
    if (m_btnHelp.justReleased())
    {
        showHelpDialog();
        // return false: Kein Workspace-Redraw auslösen - die Box übernimmt ab jetzt die Darstellung, ein Footer-Redraw würde sie überschreiben.
        return false;
    }

    if (resetActive && m_btnReset.justReleased())
    {
        showResetDialog();
        return false;
    }

    if (continueActive && m_btnContinue.justReleased())
    {
        m_winAcknowledged = true;
        // Brett kann zum Zeitpunkt des Sieges bereits voll und zuglos sein. Ohne diese Prüfung würde das Spiel in PLAYING hängen, weil applyMove() bei !anyChanged sofort zurückkehrt.
        m_state = hasAnyMove() ? GameState::PLAYING : GameState::GAMEOVER;
        return true;
    }

    if (m_btnNew.justReleased())
    {
        resetGame();
        return true;
    }

    // Visuelle Press-Rückmeldung ohne Aktion
    if (onHelp != helpWasPressed)
        m_btnHelp.drawButton(onHelp);

    if (continueActive && onContinue != continueWasPressed)
        m_btnContinue.drawButton(onContinue);

    if (onNew != newWasPressed)
        m_btnNew.drawButton(onNew);

    if (resetActive && onReset != resetWasPressed)
        m_btnReset.drawButton(onReset);

    return false;
}

// ===================================================================
// MessageBox - Spielregeln-Dialog
// ===================================================================

/**
 * @brief Öffnet den Spielregeln-Dialog als modale MessageBox.
 */
void WorkshopTool_Game2048::showHelpDialog()
{
    if (!m_msg)
        return;

    m_msgPurpose = MsgPurpose::HELP;

    I18n::format(m_helpTitleBuf, sizeof(m_helpTitleBuf), WorkshopKey::WORKSHOP_GAME_2048_HELP_TITLE);
    I18n::format(m_helpTextBuf, sizeof(m_helpTextBuf), WorkshopKey::WORKSHOP_GAME_2048_HELP_TEXT);
    I18n::format(m_dialogCloseBuf, sizeof(m_dialogCloseBuf), WorkshopKey::WORKSHOP_DIALOG_CLOSE);

    m_msg->setTitle(m_helpTitleBuf);
    m_msg->setMessage(m_helpTextBuf);
    m_msg->setButtonLabel(m_dialogCloseBuf);
    m_msg->setTimeout(0);

    // Dialog-Bounds einheitlich mit WorkshopTool_Mill: 700 px breit, horizontal auf 800 px Display zentriert, Höhe automatisch, vertikal im 480 px hohen Display zentriert.
    m_msg->setAutoBounds(DIALOG_HELP_X, 0, DIALOG_HELP_W, 480);

    m_msg->setCallback([this](bool ok)
                       { onMsgResult(ok); });

    if (owner())
        owner()->setWorkContentPaused(true);

    m_msg->start();
}

/**
 * @brief Öffnet den Bestätigungsdialog zum Zurücksetzen des Bestwerts.
 */
void WorkshopTool_Game2048::showResetDialog()
{
    if (!m_msg)
        return;

    m_msgPurpose = MsgPurpose::RESET_HIGHSCORE;

    I18n::format(m_resetTitleBuf, sizeof(m_resetTitleBuf), WorkshopKey::WORKSHOP_GAME_2048_RESET_TITLE);
    I18n::format(m_resetTextBuf, sizeof(m_resetTextBuf), WorkshopKey::WORKSHOP_GAME_2048_RESET_TEXT);
    I18n::format(m_dialogOkBuf, sizeof(m_dialogOkBuf), WorkshopKey::WORKSHOP_DIALOG_OK);
    I18n::format(m_dialogCancelBuf, sizeof(m_dialogCancelBuf), WorkshopKey::WORKSHOP_DIALOG_CANCEL);

    m_msg->setBounds(DIALOG_RESET_X, DIALOG_RESET_Y, DIALOG_RESET_W, DIALOG_RESET_H);
    m_msg->setTitle(m_resetTitleBuf);
    m_msg->setMessage(m_resetTextBuf);
    m_msg->setButtonLabels(m_dialogOkBuf, m_dialogCancelBuf);
    m_msg->setTimeout(0);
    m_msg->setCallback([this](bool confirmed)
                       { onMsgResult(confirmed); });

    if (owner())
        owner()->setWorkContentPaused(true);

    m_msg->start();
}

/**
 * @brief Callback der MessageBox - wertet das Ergebnis je nach Zweck aus.
 */
void WorkshopTool_Game2048::onMsgResult(bool confirmed)
{
    const MsgPurpose purpose = m_msgPurpose;
    m_msgPurpose = MsgPurpose::NONE;

    if (purpose == MsgPurpose::RESET_HIGHSCORE && confirmed)
    {
        // Bestwert auf den aktuellen Punktestand senken.
        m_highscore = m_score;
        saveHighscore();
    }

    if (owner())
        owner()->setWorkContentPaused(false);

    // Footer-Buttons nach dem Unpause explizit in den Ruhezustand zeichnen, da setWorkContentPaused(false) nur den Arbeitsbereich-Redraw anfordert.
    if (m_buttonsInitialized)
    {
        if (m_state == GameState::WON)
            m_btnContinue.drawButton();

        m_btnNew.drawButton();
        if (m_state == GameState::PLAYING)
            m_btnReset.drawButton();

        m_btnHelp.drawButton();
    }
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Spielfeld und rechte Infospalte in den Work-Sprite.
 */
void WorkshopTool_Game2048::draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h)
{
    (void)w;
    (void)h;

    if (!sprite)
        return;

    // Solange die MessageBox aktiv ist, den Spielinhalt nicht in den Sprite schreiben
    if (m_msg && m_msg->isVisible())
        return;

    const int boardX = ox;
    const int boardY = oy + TOP_GAP;
    const int sideX = ox + SIDE_X_OFF;
    const int sideY = oy + TOP_GAP;

    drawBoard(sprite, boardX, boardY);
    drawSidebar(sprite, sideX, sideY);

    if (m_state == GameState::WON || m_state == GameState::GAMEOVER)
        drawOverlay(sprite, boardX, boardY);
}

// ===================================================================
// Zeichnen - Spielfeld
// ===================================================================

/**
 * @brief Zeichnet das 4×4-Brett mit allen Kacheln.
 */
void WorkshopTool_Game2048::drawBoard(Gfx_Sprite *sprite, int boardX, int boardY) const
{
    // Brettrahmen
    sprite->fillSmoothRoundRect(boardX, boardY, BOARD_SIZE, BOARD_SIZE, 10, C_BOARD_FRAME);

    // Pass 1: Hintergründe und Kacheln zeichnen
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            const int px = boardX + BOARD_INNER_OFF + BOARD_PAD + c * (TILE_SIZE + BOARD_PAD);
            const int py = boardY + BOARD_INNER_OFF + BOARD_PAD + r * (TILE_SIZE + BOARD_PAD);
            drawTile(sprite, px, py, m_board[r][c]);
        }
    }

    // Pass 2: Zahlen über alle Kacheln in einem einzigen Font-Load
    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(MC_DATUM);
    for (int r = 0; r < 4; ++r)
    {
        for (int c = 0; c < 4; ++c)
        {
            const uint16_t value = m_board[r][c];
            if (value == 0)
                continue;

            uint16_t bg, fg;
            tilePalette(value, bg, fg);

            const int px = boardX + BOARD_INNER_OFF + BOARD_PAD + c * (TILE_SIZE + BOARD_PAD);
            const int py = boardY + BOARD_INNER_OFF + BOARD_PAD + r * (TILE_SIZE + BOARD_PAD);

            char numBuf[8];
            stringutils::safeFormat(numBuf, sizeof(numBuf), "%u", value);

            sprite->setTextColor(fg, bg);
            sprite->drawString(numBuf, px + TILE_SIZE / 2, py + TILE_SIZE / 2);
        }
    }
    sprite->unloadFont();
}

/**
 * @brief Zeichnet den Kachel-Hintergrund an der angegebenen Position.
 *
 * Die Beschriftung wird in einem eigenen Pass von drawBoard() gezeichnet, damit der Bold30-Font nur einmal pro Frame geladen werden muss.
 */
void WorkshopTool_Game2048::drawTile(Gfx_Sprite *sprite, int px, int py, uint16_t value) const
{
    uint16_t bg = 0;
    uint16_t fgUnused = 0;
    tilePalette(value, bg, fgUnused);

    sprite->fillSmoothRoundRect(px, py, TILE_SIZE, TILE_SIZE, TILE_R, bg);
}

// ===================================================================
// Zeichnen - Sidebar
// ===================================================================

/**
 * @brief Zeichnet Score, Highscore und Wisch-Hinweis-Box.
 */
void WorkshopTool_Game2048::drawSidebar(Gfx_Sprite *sprite, int sideX, int sideY) const
{
    // ===================================================================
    // Score-Karte
    // ===================================================================
    int cardY = sideY;
    sprite->fillSmoothRoundRect(sideX, cardY, SIDE_W, CARD_H, CARD_R, C_CARD_BG);

    // ===================================================================
    // Highscore-Karte
    // ===================================================================
    const int hiCardY = cardY + CARD_H + CARD_GAP;
    sprite->fillSmoothRoundRect(sideX, hiCardY, SIDE_W, CARD_H, CARD_R, C_CARD_BG);

    // ===================================================================
    // Wisch-Hinweis-Box
    // ===================================================================
    const int hintY = hiCardY + CARD_H + CARD_GAP;
    sprite->fillSmoothRoundRect(sideX, hintY, SIDE_W, HINT_BOX_H, CARD_R, C_HINT_BG);

    // Vier Pfeile vom Zentrum nach aussen
    {
        constexpr int VALUE_RIGHT_PAD = 20; ///< Rechter Innenabstand der Werte/Pfeile
        const int armLen = 28;
        const int cx = sideX + SIDE_W - VALUE_RIGHT_PAD - armLen;
        const int cy = hintY + HINT_BOX_H / 2;
        const int shaftR = 6; ///< Schaftdicke
        const int tipR = 0;   ///< Spitze

        // Pfeil nach oben
        sprite->drawWedgeLine(cx, cy, cx, cy - armLen, shaftR, tipR, C_HINT_ARROW);
        // Pfeil nach unten
        sprite->drawWedgeLine(cx, cy, cx, cy + armLen, shaftR, tipR, C_HINT_ARROW);
        // Pfeil nach links
        sprite->drawWedgeLine(cx, cy, cx - armLen, cy, shaftR, tipR, C_HINT_ARROW);
        // Pfeil nach rechts
        sprite->drawWedgeLine(cx, cy, cx + armLen, cy, shaftR, tipR, C_HINT_ARROW);
    }

    // ===================================================================
    // Texte: Bold14 für Labels und Hinweis - vertikal mittig in den Containern
    // ===================================================================
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(ML_DATUM);

    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_GAME_2048_LABEL_SCORE), sideX + 12, cardY + CARD_H / 2);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_GAME_2048_LABEL_BEST), sideX + 12, hiCardY + CARD_H / 2);

    sprite->setTextColor(C_HINT_FG, C_HINT_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_GAME_2048_LABEL_HINT), sideX + 12, hintY + HINT_BOX_H / 2);

    sprite->unloadFont();

    // ===================================================================
    // Werte: Bold30 für Score und Highscore, vertikal mittig in der Karte
    // ===================================================================
    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(MR_DATUM);
    sprite->setTextColor(C_CARD_VALUE, C_CARD_BG);

    char numBuf[16];
    stringutils::safeFormat(numBuf, sizeof(numBuf), "%lu", static_cast<unsigned long>(m_score));
    sprite->drawString(numBuf, sideX + SIDE_W - 20, cardY + CARD_H / 2);

    stringutils::safeFormat(numBuf, sizeof(numBuf), "%lu", static_cast<unsigned long>(m_highscore));
    sprite->drawString(numBuf, sideX + SIDE_W - 20, hiCardY + CARD_H / 2);

    sprite->unloadFont();
}

// ===================================================================
// Zeichnen - Overlay (Sieg / Game Over)
// ===================================================================

/**
 * @brief Zeichnet ein zentriertes Overlay über dem Spielfeld.
 */
void WorkshopTool_Game2048::drawOverlay(Gfx_Sprite *sprite, int boardX, int boardY) const
{
    constexpr int OV_W = 260;
    constexpr int OV_H = 120;

    const int ovX = boardX + (BOARD_SIZE - OV_W) / 2;
    const int ovY = boardY + (BOARD_SIZE - OV_H) / 2;

    const uint16_t frameColor = (m_state == GameState::WON)
                                    ? C_OVERLAY_FRAME
                                    : C_OVERLAY_FRAME_LOSE;

    // zwei fillSmoothRoundRect, aussen Rahmen, innen Füllung
    sprite->fillSmoothRoundRect(ovX, ovY, OV_W, OV_H, 12, frameColor);
    sprite->fillSmoothRoundRect(ovX + 3, ovY + 3, OV_W - 6, OV_H - 6, 10, C_OVERLAY_BG);

    const __FlashStringHelper *title = (m_state == GameState::WON)
                                           ? I18n::str(WorkshopKey::WORKSHOP_GAME_2048_OVERLAY_WON_TITLE)
                                           : I18n::str(WorkshopKey::WORKSHOP_GAME_2048_OVERLAY_LOST_TITLE);
    const __FlashStringHelper *line2 = (m_state == GameState::WON)
                                           ? I18n::str(WorkshopKey::WORKSHOP_GAME_2048_OVERLAY_WON_LINE2)
                                           : I18n::str(WorkshopKey::WORKSHOP_GAME_2048_OVERLAY_LOST_LINE2);

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

// ===================================================================
// Farbtabelle der Kacheln
// ===================================================================

/**
 * @brief Liefert Hintergrund- und Textfarbe einer Kachel anhand des Werts.
 *
 * Die Palette beginnt bei dunklen, dezenten Tönen für kleine Werte und wandert über warme Mittelfarben zu leuchtenden Akzentfarben für hohe Werte. 2048 wird als kräftiges Gold dargestellt, alles darüber bleibt in dieser Farbe.
 */
void WorkshopTool_Game2048::tilePalette(uint16_t value, uint16_t &outBg, uint16_t &outFg)
{
    switch (value)
    {
    case 0:
        outBg = C_TILE_EMPTY;
        outFg = C_TILE_EMPTY; // unsichtbar
        break;
    case 2:
        outBg = 0xEF7D;
        outFg = 0x632C;
        break; // Cremeweiß, dunkelbrauner Text
    case 4:
        outBg = 0xEEFA;
        outFg = 0x632C;
        break; // Helles Creme, dunkelbrauner Text
    case 8:
        outBg = 0x9B45;
        outFg = 0xFFFF;
        break; // warmes Braun
    case 16:
        outBg = 0xC305;
        outFg = 0xFFFF;
        break; // helles Braun
    case 32:
        outBg = 0xE2A4;
        outFg = 0xFFFF;
        break; // Orange-Rot
    case 64:
        outBg = 0xFA42;
        outFg = 0xFFFF;
        break; // kräftiges Rot-Orange
    case 128:
        outBg = 0xEEA8;
        outFg = 0xFFFF;
        break; // warmes Gelb
    case 256:
        outBg = 0xEE45;
        outFg = 0xFFFF;
        break; // sattes Gelb
    case 512:
        outBg = 0xEDC2;
        outFg = 0xFFFF;
        break; // Honig
    case 1024:
        outBg = 0xED40;
        outFg = 0xFFFF;
        break; // dunkles Gold
    case 2048:
        outBg = 0xFEA0;
        outFg = 0xFFFF;
        break; // strahlendes Gold
    default:
        outBg = 0xF800;
        outFg = 0xFFFF;
        break; // Rot für 4096+
    }
}
