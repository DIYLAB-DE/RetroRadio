#include "WorkshopTool_ShutTheBox.h"

#include <Preferences.h>
#include <esp_random.h>

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
    constexpr int CONTENT_W = 547; ///< Netto-Breite des Werkzeugbereichs
    constexpr int CONTENT_H = 327; ///< Netto-Höhe des Werkzeugbereichs
    constexpr int TOP_GAP = 7;     ///< Abstand von Tool-Top zum Inhalt

    constexpr int BOARD_X = 0;       ///< X-Versatz des Klappenfelds
    constexpr int BOARD_Y = TOP_GAP; ///< Y-Versatz des Klappenfelds
    constexpr int FLAP_W = 100;      ///< Breite einer Klappe
    constexpr int FLAP_H = 100;      ///< Höhe einer Klappe
    constexpr int FLAP_GAP = 10;     ///< Abstand zwischen Klappen
    constexpr int FLAP_R = 8;        ///< Eckenradius der Klappen
    constexpr int BOARD_GRID = 3;    ///< Klappen pro Zeile und Spalte
    constexpr int BOARD_W = BOARD_GRID * FLAP_W + (BOARD_GRID - 1) * FLAP_GAP;
    constexpr int BOARD_H = BOARD_GRID * FLAP_H + (BOARD_GRID - 1) * FLAP_GAP;

    constexpr int SIDE_W = 210;                ///< Breite der Statusspalte
    constexpr int SIDE_X = CONTENT_W - SIDE_W; ///< X-Versatz der Statusspalte
    constexpr int CARD_R = 8;                  ///< Eckenradius der Statuskarten
    constexpr int CARD_GAP = 6;                ///< Vertikaler Abstand zwischen Karten
    constexpr int CARD_ROLL_H = 104;           ///< Höhe der Wurfkarte
    constexpr int CARD_SMALL_H = 52;           ///< Höhe kleiner Karten
    constexpr int BIG_VALUE_Y_OFFSET = 3;      ///< Vertikaler Ausgleich für große Ziffern
    constexpr int DICE_ONE_SIZE = 54;          ///< Kantenlänge bei einem Würfel
    constexpr int DICE_TWO_SIZE = 48;          ///< Kantenlänge bei zwei Würfeln
    constexpr int DICE_ONE_X_OFF = -27;        ///< X-Versatz bei einem Würfel
    constexpr int DICE_LEFT_X_OFF = -51;       ///< X-Versatz des linken Würfels
    constexpr int DICE_RIGHT_X_OFF = 11;       ///< X-Versatz des rechten Würfels

    constexpr int FOOTER_BTN_H = 44;     ///< Höhe der Fußzeilen-Buttons
    constexpr int FOOTER_BTN_R = 6;      ///< Eckenradius der Fußzeilen-Buttons
    constexpr int FOOTER_BTN_GAP = 4;    ///< Abstand zwischen Fußzeilen-Buttons
    constexpr int FOOTER_BTN_MARGIN = 2; ///< Linker Rand im Footer
    constexpr int BTN_W_DICE = 42;       ///< Breite für 1W/2W
    constexpr int BTN_W_ROLL = 58;       ///< Breite für Wurf
    constexpr int BTN_W_LAY = 62;        ///< Breite für Legen
    constexpr int BTN_W_NEW = 50;        ///< Breite für Neu
    constexpr int BTN_W_RESET = 58;      ///< Breite für Reset
    constexpr int BTN_W_HELP = 44;       ///< Breite für Hilfe

    constexpr uint16_t C_FLAP_OPEN = 0xAB66;           ///< Holzton offener Klappen
    constexpr uint16_t C_FLAP_OPEN_EDGE = 0xE671;      ///< Heller Rand offener Klappen
    constexpr uint16_t C_FLAP_OPEN_GRAIN = 0x7B84;     ///< Dunkle Holzmaserung
    constexpr uint16_t C_FLAP_OPEN_LIGHT = 0xDD2B;     ///< Heller Holzreflex
    constexpr uint16_t C_FLAP_OPEN_SHADE = 0x5A63;     ///< Dunkler Holzschatten
    constexpr uint16_t C_FLAP_SELECTED = 0xF5A7;       ///< Gewählte Klappe
    constexpr uint16_t C_FLAP_SELECTED_GRAIN = 0xBD24; ///< Maserung gewählter Klappen
    constexpr uint16_t C_FLAP_CLOSED = 0x3144;         ///< Geschlossene Klappe
    constexpr uint16_t C_FLAP_CLOSED_EDGE = 0x6B07;    ///< Rand geschlossener Klappen
    constexpr uint16_t C_FLAP_CLOSED_GRAIN = 0x4A05;   ///< Maserung geschlossener Klappen
    constexpr uint16_t C_FLAP_TEXT = 0xFFFF;           ///< Heller Klappentext
    constexpr uint16_t C_FLAP_TEXT_DARK = 0x8C71;      ///< Gedimmter Klappentext
    constexpr uint16_t C_FLAP_TEXT_SHADOW = 0x2104;    ///< Schatten hinter Klappenzahlen

    constexpr uint16_t C_CARD_BG = 0x2945;    ///< Hintergrund der Statuskarten
    constexpr uint16_t C_CARD_LABEL = 0xAD75; ///< Label-Farbe
    constexpr uint16_t C_CARD_VALUE = 0xFFFF; ///< Wert-Farbe
    constexpr uint16_t C_CARD_DIM = 0xB5B6;   ///< Gedimmte Wert-Farbe
    constexpr uint16_t C_ACCENT = 0xFEA0;     ///< Goldener Akzent
    constexpr uint16_t C_WARN = 0xF9C7;       ///< Warnfarbe
    constexpr uint16_t C_OK = 0x07E0;         ///< Erfolgsfarbe

    constexpr uint16_t C_DIE_FILL = 0xE6D7;   ///< Würfel-Fläche
    constexpr uint16_t C_DIE_EDGE = 0x8A65;   ///< Würfel-Rand
    constexpr uint16_t C_DIE_LIGHT = 0xFFF8;  ///< Heller Würfelreflex
    constexpr uint16_t C_DIE_SHADOW = 0x18C3; ///< Würfelschatten
    constexpr uint16_t C_DIE_PIP = 0x3921;    ///< Würfelaugen

    constexpr uint16_t TOGGLE_ACTIVE_FILL = 0x328A;      ///< Aktiver Button-Hintergrund
    constexpr uint16_t TOGGLE_ACTIVE_TEXT = 0xFFFF;      ///< Aktiver Button-Text
    constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;   ///< Aktiver Button-Rand
    constexpr uint16_t TOGGLE_INACTIVE_FILL = 0x0000;    ///< Inaktiver Button-Hintergrund
    constexpr uint16_t TOGGLE_INACTIVE_TEXT = 0x7BEF;    ///< Inaktiver Button-Text
    constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8; ///< Inaktiver Button-Rand
    constexpr uint16_t TOGGLE_DISABLED_TEXT = 0x528A;    ///< Nicht nutzbarer Button-Text

    constexpr int DIALOG_HELP_W = 700;                       ///< Breite des Hilfe-Dialogs
    constexpr int DIALOG_HELP_X = (800 - DIALOG_HELP_W) / 2; ///< X-Position des Hilfe-Dialogs
    constexpr int DIALOG_RESET_W = 480;                      ///< Breite des Reset-Dialogs
    constexpr int DIALOG_RESET_H = 220;                      ///< Höhe des Reset-Dialogs
    constexpr int DIALOG_RESET_X = (800 - DIALOG_RESET_W) / 2;
    constexpr int DIALOG_RESET_Y = (480 - DIALOG_RESET_H) / 2;

    const char *const PREFS_NS = "wsstb";        ///< Preferences-Namespace
    const char *const PREFS_KEY_BEST = "br";     ///< Bester Restwert
    const char *const PREFS_KEY_WINS = "wi";     ///< Gewonnene Runden
    constexpr uint16_t BEST_UNSET = 255;         ///< Marker für fehlenden Bestwert
    constexpr uint16_t ALL_CLOSED_MASK = 0x01FF; ///< Bits 0..8 gesetzt
}

// ===================================================================
// Lifecycle
// ===================================================================

/**
 * @brief Initialisiert Spielzustand, Statistik, Dialog und Fußzeilen-Buttons.
 */
void WorkshopTool_ShutTheBox::onEnter(LGFX *tft)
{
    loadStats();
    resetGame();
    initFooterButtons(tft);

    if (!m_msg)
        m_msg.reset(new MessageBox(tft));

    m_msgPurpose = MsgPurpose::NONE;
}

/**
 * @brief Räumt den Werkzeugzustand auf.
 */
void WorkshopTool_ShutTheBox::onExit()
{
    if (owner())
        owner()->setWorkContentPaused(false);

    m_msg.reset();
    m_msgPurpose = MsgPurpose::NONE;
    m_buttonsInitialized = false;
    m_touchActive = false;
}

// ===================================================================
// Spiel-Logik
// ===================================================================

/**
 * @brief Startet eine neue Runde.
 */
void WorkshopTool_ShutTheBox::resetGame()
{
    m_closedMask = 0;
    m_selectedMask = 0;
    m_state = GameState::PLAYING;
    m_hasRoll = false;
    m_oneDieMode = false;
    m_resultBooked = false;
    m_dice[0] = 1;
    m_dice[1] = 1;
    m_diceCount = 2;
    m_touchActive = false;
}

/**
 * @brief Liefert eine gleichverteilte Augenzahl 1 bis 6.
 */
uint8_t WorkshopTool_ShutTheBox::rollDie() const
{
    constexpr uint32_t RANGE = 0xFFFFFFFFu - (0xFFFFFFFFu % 6u);

    uint32_t v;
    do
    {
        v = esp_random();
    } while (v >= RANGE);

    return static_cast<uint8_t>((v % 6u) + 1u);
}

/**
 * @brief Führt einen Wurf aus und prüft sofort auf fehlende Kombinationen.
 */
void WorkshopTool_ShutTheBox::rollDice()
{
    if (m_state != GameState::PLAYING || m_hasRoll)
        return;

    if (!canUseOneDie())
        m_oneDieMode = false;

    m_diceCount = m_oneDieMode ? 1 : 2;
    m_dice[0] = rollDie();
    m_dice[1] = (m_diceCount == 2) ? rollDie() : 0;
    m_selectedMask = 0;
    m_hasRoll = true;

    if (!hasCombination(currentRollSum()))
    {
        m_state = GameState::GAMEOVER;
        bookResultIfNeeded();
    }

    updateFooterButtonColors();
}

/**
 * @brief Wählt eine offene Klappe an oder ab.
 */
void WorkshopTool_ShutTheBox::toggleNumber(uint8_t n)
{
    if (n < 1 || n > 9)
        return;

    if (m_state != GameState::PLAYING || !m_hasRoll)
        return;

    const uint16_t bit = static_cast<uint16_t>(1u << (n - 1));
    if ((m_closedMask & bit) != 0)
        return;

    if ((m_selectedMask & bit) != 0)
        m_selectedMask &= static_cast<uint16_t>(~bit);
    else
        m_selectedMask |= bit;

    updateFooterButtonColors();
}

/**
 * @brief Schließt die aktuell gewählten Klappen, wenn ihre Summe passt.
 */
bool WorkshopTool_ShutTheBox::laySelection()
{
    if (!canLaySelection())
        return false;

    m_closedMask |= m_selectedMask;
    m_selectedMask = 0;
    m_hasRoll = false;

    if (allClosed())
    {
        m_state = GameState::WON;
        bookResultIfNeeded();
    }

    updateFooterButtonColors();
    return true;
}

/**
 * @brief Prüft, ob ein Würfel erlaubt ist.
 */
bool WorkshopTool_ShutTheBox::canUseOneDie() const
{
    constexpr uint16_t HIGH_MASK = static_cast<uint16_t>((1u << 6) | (1u << 7) | (1u << 8));
    return (m_closedMask & HIGH_MASK) == HIGH_MASK;
}

/**
 * @brief Prüft, ob die Auswahl zum aktuellen Wurf passt.
 */
bool WorkshopTool_ShutTheBox::canLaySelection() const
{
    return m_state == GameState::PLAYING && m_hasRoll && m_selectedMask != 0 && selectedSum() == currentRollSum();
}

/**
 * @brief Prüft, ob eine offene Kombination die Zielsumme erreicht.
 */
bool WorkshopTool_ShutTheBox::hasCombination(uint8_t target) const
{
    for (uint16_t mask = 1; mask <= ALL_CLOSED_MASK; ++mask)
    {
        if ((mask & m_closedMask) != 0)
            continue;

        uint8_t sum = 0;
        for (uint8_t n = 1; n <= 9; ++n)
        {
            if ((mask & (1u << (n - 1))) != 0)
                sum = static_cast<uint8_t>(sum + n);
        }

        if (sum == target)
            return true;
    }

    return false;
}

/**
 * @brief Prüft, ob alle Klappen geschlossen sind.
 */
bool WorkshopTool_ShutTheBox::allClosed() const
{
    return (m_closedMask & ALL_CLOSED_MASK) == ALL_CLOSED_MASK;
}

/**
 * @brief Liefert die Summe der aktuell gewählten Klappen.
 */
uint8_t WorkshopTool_ShutTheBox::selectedSum() const
{
    uint8_t sum = 0;
    for (uint8_t n = 1; n <= 9; ++n)
    {
        if ((m_selectedMask & (1u << (n - 1))) != 0)
            sum = static_cast<uint8_t>(sum + n);
    }
    return sum;
}

/**
 * @brief Liefert die Summe der offenen Klappen.
 */
uint8_t WorkshopTool_ShutTheBox::openSum() const
{
    uint8_t sum = 0;
    for (uint8_t n = 1; n <= 9; ++n)
    {
        if ((m_closedMask & (1u << (n - 1))) == 0)
            sum = static_cast<uint8_t>(sum + n);
    }
    return sum;
}

/**
 * @brief Liefert die Summe des aktuellen Wurfs.
 */
uint8_t WorkshopTool_ShutTheBox::currentRollSum() const
{
    return static_cast<uint8_t>(m_dice[0] + ((m_diceCount == 2) ? m_dice[1] : 0));
}

/**
 * @brief Speichert das Endergebnis genau einmal.
 */
void WorkshopTool_ShutTheBox::bookResultIfNeeded()
{
    if (m_resultBooked)
        return;

    m_resultBooked = true;
    const uint8_t rest = openSum();

    if (m_bestRest == BEST_UNSET || rest < m_bestRest)
        m_bestRest = rest;

    if (m_state == GameState::WON && m_wins < 0xFFFF)
        m_wins++;

    saveStats();
}

// ===================================================================
// Touch
// ===================================================================

/**
 * @brief Verarbeitet Touch-Ereignisse auf Klappen und Wurfkarte.
 */
bool WorkshopTool_ShutTheBox::handleTouch(int32_t tx, int32_t ty, bool touched)
{
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

    uint8_t n = 0;
    if (hitTestNumber(m_touchLastX, m_touchLastY, n))
    {
        const uint16_t before = m_selectedMask;
        toggleNumber(n);
        return before != m_selectedMask;
    }

    if (hitTestRollCard(m_touchLastX, m_touchLastY))
    {
        if (m_state == GameState::PLAYING && !m_hasRoll)
        {
            rollDice();
            return true;
        }
    }

    return false;
}

/**
 * @brief Ermittelt die getippte Klappe aus absoluten Touch-Koordinaten.
 */
bool WorkshopTool_ShutTheBox::hitTestNumber(int32_t tx, int32_t ty, uint8_t &outNumber) const
{
    const int relX = static_cast<int>(tx) - WorkshopCfg::WORK_X - m_originX - BOARD_X;
    const int relY = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_originY - BOARD_Y;

    if (relX < 0 || relY < 0 || relX >= BOARD_W || relY >= BOARD_H)
        return false;

    const int strideX = FLAP_W + FLAP_GAP;
    const int strideY = FLAP_H + FLAP_GAP;
    const int col = relX / strideX;
    const int row = relY / strideY;

    if (col < 0 || col >= BOARD_GRID || row < 0 || row >= BOARD_GRID)
        return false;

    const int inX = relX - col * strideX;
    const int inY = relY - row * strideY;
    if (inX >= FLAP_W || inY >= FLAP_H)
        return false;

    outNumber = static_cast<uint8_t>(row * BOARD_GRID + col + 1);
    return outNumber >= 1 && outNumber <= 9;
}

/**
 * @brief Prüft Touch auf der Wurfkarte.
 */
bool WorkshopTool_ShutTheBox::hitTestRollCard(int32_t tx, int32_t ty) const
{
    const int relX = static_cast<int>(tx) - WorkshopCfg::WORK_X - m_originX;
    const int relY = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_originY;
    return relX >= SIDE_X && relX < SIDE_X + SIDE_W && relY >= TOP_GAP && relY < TOP_GAP + CARD_ROLL_H;
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet Klappenfeld und Statuskarten in den Work-Sprite.
 */
void WorkshopTool_ShutTheBox::draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h)
{
    (void)w;
    (void)h;

    if (!sprite)
        return;

    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);
        return;
    }

    m_originX = ox;
    m_originY = oy;

    drawBoard(sprite);
    drawSidebar(sprite);

    if (m_state == GameState::WON || m_state == GameState::GAMEOVER)
        drawOverlay(sprite);
}

/**
 * @brief Zeichnet die neun Klappen.
 */
void WorkshopTool_ShutTheBox::drawBoard(Gfx_Sprite *sprite)
{
    const int x0 = m_originX + BOARD_X;
    const int y0 = m_originY + BOARD_Y;

    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(MC_DATUM);

    for (uint8_t n = 1; n <= 9; ++n)
    {
        const int idx = n - 1;
        const int row = idx / BOARD_GRID;
        const int col = idx % BOARD_GRID;
        const int x = x0 + col * (FLAP_W + FLAP_GAP);
        const int y = y0 + row * (FLAP_H + FLAP_GAP);
        const uint16_t bit = static_cast<uint16_t>(1u << idx);
        const bool closed = (m_closedMask & bit) != 0;
        const bool selected = (m_selectedMask & bit) != 0;

        uint16_t edge = C_FLAP_OPEN_EDGE;
        uint16_t fill = C_FLAP_OPEN;
        uint16_t text = C_FLAP_TEXT;
        uint16_t grain = C_FLAP_OPEN_GRAIN;
        uint16_t light = C_FLAP_OPEN_LIGHT;
        uint16_t shade = C_FLAP_OPEN_SHADE;

        if (closed)
        {
            edge = C_FLAP_CLOSED_EDGE;
            fill = C_FLAP_CLOSED;
            text = C_FLAP_TEXT_DARK;
            grain = C_FLAP_CLOSED_GRAIN;
            light = C_FLAP_CLOSED_EDGE;
            shade = 0x18C3;
        }
        else if (selected)
        {
            edge = C_ACCENT;
            fill = C_FLAP_SELECTED;
            text = 0x2104;
            grain = C_FLAP_SELECTED_GRAIN;
            light = 0xFF36;
            shade = 0x9B82;
        }

        sprite->fillSmoothRoundRect(x, y, FLAP_W, FLAP_H, FLAP_R, edge);
        sprite->fillSmoothRoundRect(x + 3, y + 3, FLAP_W - 6, FLAP_H - 6, FLAP_R - 2, fill);
        sprite->drawFastHLine(x + 12, y + 9, FLAP_W - 24, light);
        sprite->drawFastHLine(x + 10, y + FLAP_H - 9, FLAP_W - 20, shade);
        sprite->drawFastVLine(x + FLAP_W - 9, y + 14, FLAP_H - 28, shade);

        for (uint8_t line = 0; line < 4; ++line)
        {
            const int gy = y + 22 + line * 17 + ((idx + line) % 3);
            const int gx = x + 12 + ((idx * 5 + line * 7) % 15);
            const int gw = FLAP_W - 25 - ((idx + line * 3) % 17);
            sprite->drawFastHLine(gx, gy, gw, grain);
        }

        char numBuf[4];
        stringutils::safeFormat(numBuf, sizeof(numBuf), "%u", static_cast<unsigned>(n));
        sprite->setTextColor(C_FLAP_TEXT_SHADOW);
        sprite->drawString(numBuf, x + FLAP_W / 2 + 2, y + FLAP_H / 2 + 3);
        sprite->setTextColor(text);
        sprite->drawString(numBuf, x + FLAP_W / 2, y + FLAP_H / 2 + 1);

        if (closed)
        {
            sprite->drawLine(x + 18, y + FLAP_H / 2, x + FLAP_W - 18, y + FLAP_H / 2, shade);
            sprite->drawLine(x + 18, y + FLAP_H / 2 + 1, x + FLAP_W - 18, y + FLAP_H / 2 + 1, C_FLAP_TEXT_DARK);
        }
    }

    sprite->unloadFont();
}

/**
 * @brief Zeichnet die Statusspalte.
 */
void WorkshopTool_ShutTheBox::drawSidebar(Gfx_Sprite *sprite)
{
    const int sideX = m_originX + SIDE_X;
    int y = m_originY + TOP_GAP;

    const int rollY = y;
    y += CARD_ROLL_H + CARD_GAP;
    const int pickY = y;
    y += CARD_SMALL_H + CARD_GAP;
    const int openY = y;
    y += CARD_SMALL_H + CARD_GAP;
    const int statsY = y;
    const int statsH = BOARD_Y + BOARD_H - (statsY - m_originY);

    sprite->fillSmoothRoundRect(sideX, rollY, SIDE_W, CARD_ROLL_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, pickY, SIDE_W, CARD_SMALL_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, openY, SIDE_W, CARD_SMALL_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, statsY, SIDE_W, statsH, CARD_R, C_CARD_BG);

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->setTextDatum(TC_DATUM);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_SHUT_THE_BOX_LABEL_ROLL), sideX + SIDE_W / 2, rollY + 8);
    sprite->unloadFont();

    if (m_hasRoll)
    {
        const int dieY = rollY + CARD_ROLL_H / 2 + BIG_VALUE_Y_OFFSET;
        if (m_diceCount == 1)
        {
            drawDie(sprite, sideX + SIDE_W / 2 + DICE_ONE_X_OFF, dieY, DICE_ONE_SIZE, m_dice[0], C_DIE_FILL, C_DIE_PIP);
        }
        else
        {
            drawDie(sprite, sideX + SIDE_W / 2 + DICE_LEFT_X_OFF, dieY, DICE_TWO_SIZE, m_dice[0], C_DIE_FILL, C_DIE_PIP);
            drawDie(sprite, sideX + SIDE_W / 2 + DICE_RIGHT_X_OFF, dieY, DICE_TWO_SIZE, m_dice[1], C_DIE_FILL, C_DIE_PIP);
        }

        char sumBuf[8];
        stringutils::safeFormat(sumBuf, sizeof(sumBuf), "%u", static_cast<unsigned>(currentRollSum()));
        sprite->loadFont(ComfortaaBold30);
        sprite->setTextDatum(MR_DATUM);
        sprite->setTextColor(C_ACCENT, C_CARD_BG);
        sprite->drawString(sumBuf, sideX + SIDE_W - 14, dieY);
        sprite->unloadFont();
    }
    else
    {
        sprite->loadFont(ComfortaaBold16);
        sprite->setTextDatum(MC_DATUM);
        sprite->setTextColor(C_CARD_DIM, C_CARD_BG);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_SHUT_THE_BOX_HINT_ROLL), sideX + SIDE_W / 2, rollY + CARD_ROLL_H / 2 + 8);
        sprite->unloadFont();
    }

    char buf[24];
    const uint8_t pick = selectedSum();
    const uint16_t pickColor = canLaySelection() ? C_OK : ((pick > 0) ? C_WARN : C_CARD_VALUE);
    stringutils::safeFormat(buf, sizeof(buf), "%u", static_cast<unsigned>(pick));
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(ML_DATUM);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_SHUT_THE_BOX_LABEL_PICK), sideX + 12, pickY + CARD_SMALL_H / 2);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(MR_DATUM);
    sprite->setTextColor(pickColor, C_CARD_BG);
    sprite->drawString(buf, sideX + SIDE_W - 16, pickY + CARD_SMALL_H / 2 + BIG_VALUE_Y_OFFSET);
    sprite->unloadFont();

    stringutils::safeFormat(buf, sizeof(buf), "%u", static_cast<unsigned>(openSum()));
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(ML_DATUM);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_SHUT_THE_BOX_LABEL_OPEN), sideX + 12, openY + CARD_SMALL_H / 2);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(MR_DATUM);
    sprite->setTextColor(C_CARD_VALUE, C_CARD_BG);
    sprite->drawString(buf, sideX + SIDE_W - 16, openY + CARD_SMALL_H / 2 + BIG_VALUE_Y_OFFSET);
    sprite->unloadFont();

    if (m_bestRest == BEST_UNSET)
        stringutils::safeCopy(buf, sizeof(buf), "-");
    else
        stringutils::safeFormat(buf, sizeof(buf), "%u", static_cast<unsigned>(m_bestRest));

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(ML_DATUM);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_SHUT_THE_BOX_LABEL_BEST), sideX + 12, statsY + 20);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_SHUT_THE_BOX_LABEL_WINS), sideX + 12, statsY + 50);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold16);
    sprite->setTextDatum(MR_DATUM);
    sprite->setTextColor(C_CARD_VALUE, C_CARD_BG);
    sprite->drawString(buf, sideX + SIDE_W - 16, statsY + 20);
    stringutils::safeFormat(buf, sizeof(buf), "%u", static_cast<unsigned>(m_wins));
    sprite->drawString(buf, sideX + SIDE_W - 16, statsY + 50);
    sprite->unloadFont();
}

/**
 * @brief Zeichnet einen Würfel.
 */
void WorkshopTool_ShutTheBox::drawDie(Gfx_Sprite *sprite, int cx, int cy, int size, uint8_t value, uint16_t fill, uint16_t pip) const
{
    const int x = cx - size / 2;
    const int y = cy - size / 2;
    const int r = size / 6;

    sprite->fillSmoothRoundRect(x + 3, y + 4, size, size, r, C_DIE_SHADOW);
    sprite->fillSmoothRoundRect(x, y, size, size, r, C_DIE_EDGE);
    sprite->fillSmoothRoundRect(x + 3, y + 3, size - 7, size - 7, r - 2, fill);
    sprite->drawFastHLine(x + 10, y + 7, size - 20, C_DIE_LIGHT);
    sprite->drawFastVLine(x + 7, y + 10, size - 20, C_DIE_LIGHT);
    drawPips(sprite, cx, cy, size, value, pip);
}

/**
 * @brief Zeichnet Würfelaugen.
 */
void WorkshopTool_ShutTheBox::drawPips(Gfx_Sprite *sprite, int cx, int cy, int size, uint8_t value, uint16_t color) const
{
    const int off = static_cast<int>(size * 0.27f);
    const int pipR = size / 11;
    const int xl = cx - off;
    const int xc = cx;
    const int xr = cx + off;
    const int yt = cy - off;
    const int yc = cy;
    const int yb = cy + off;

    auto pip = [&](int x, int y)
    {
        sprite->fillSmoothCircle(x, y, pipR, color);
    };

    switch (value)
    {
    case 1:
        pip(xc, yc);
        break;
    case 2:
        pip(xl, yt);
        pip(xr, yb);
        break;
    case 3:
        pip(xl, yt);
        pip(xc, yc);
        pip(xr, yb);
        break;
    case 4:
        pip(xl, yt);
        pip(xr, yt);
        pip(xl, yb);
        pip(xr, yb);
        break;
    case 5:
        pip(xl, yt);
        pip(xr, yt);
        pip(xc, yc);
        pip(xl, yb);
        pip(xr, yb);
        break;
    case 6:
        pip(xl, yt);
        pip(xr, yt);
        pip(xl, yc);
        pip(xr, yc);
        pip(xl, yb);
        pip(xr, yb);
        break;
    default:
        pip(xc, yc);
        break;
    }
}

/**
 * @brief Zeichnet ein zentriertes Ende-Overlay.
 */
void WorkshopTool_ShutTheBox::drawOverlay(Gfx_Sprite *sprite)
{
    constexpr int OV_W = 280;
    constexpr int OV_H = 118;
    const int ovX = m_originX + BOARD_X + (BOARD_W - OV_W) / 2;
    const int ovY = m_originY + BOARD_Y + (BOARD_H - OV_H) / 2;
    const uint16_t frame = (m_state == GameState::WON) ? C_OK : C_WARN;

    sprite->fillSmoothRoundRect(ovX, ovY, OV_W, OV_H, 12, frame);
    sprite->fillSmoothRoundRect(ovX + 3, ovY + 3, OV_W - 6, OV_H - 6, 10, 0x0841);

    const WorkshopKey titleKey = (m_state == GameState::WON)
                                     ? WorkshopKey::WORKSHOP_SHUT_THE_BOX_OVERLAY_WON_TITLE
                                     : WorkshopKey::WORKSHOP_SHUT_THE_BOX_OVERLAY_LOST_TITLE;

    const WorkshopKey lineKey = (m_state == GameState::WON)
                                    ? WorkshopKey::WORKSHOP_SHUT_THE_BOX_OVERLAY_WON_LINE2
                                    : WorkshopKey::WORKSHOP_SHUT_THE_BOX_OVERLAY_LOST_LINE2;

    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(C_CARD_VALUE, 0x0841);
    sprite->drawString(I18n::str(titleKey), ovX + OV_W / 2, ovY + 38);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold16);
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(C_CARD_VALUE, 0x0841);
    sprite->drawString(I18n::str(lineKey), ovX + OV_W / 2, ovY + 78);
    sprite->unloadFont();
}

// ===================================================================
// Footer
// ===================================================================

/**
 * @brief Initialisiert die Fußzeilen-Buttons.
 */
void WorkshopTool_ShutTheBox::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int x0 = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN;
    constexpr int cx1 = x0 + BTN_W_DICE / 2;
    constexpr int cx2 = cx1 + BTN_W_DICE / 2 + FOOTER_BTN_GAP + BTN_W_DICE / 2;
    constexpr int cx3 = cx2 + BTN_W_DICE / 2 + FOOTER_BTN_GAP + BTN_W_ROLL / 2;
    constexpr int cx4 = cx3 + BTN_W_ROLL / 2 + FOOTER_BTN_GAP + BTN_W_LAY / 2;
    constexpr int cx5 = cx4 + BTN_W_LAY / 2 + FOOTER_BTN_GAP + BTN_W_NEW / 2;
    constexpr int cx6 = cx5 + BTN_W_NEW / 2 + FOOTER_BTN_GAP + BTN_W_RESET / 2;
    constexpr int cx7 = cx6 + BTN_W_RESET / 2 + FOOTER_BTN_GAP + BTN_W_HELP / 2;

    I18n::format(m_btnOneLabel, sizeof(m_btnOneLabel), WorkshopKey::WORKSHOP_SHUT_THE_BOX_BTN_ONE);
    I18n::format(m_btnTwoLabel, sizeof(m_btnTwoLabel), WorkshopKey::WORKSHOP_SHUT_THE_BOX_BTN_TWO);
    I18n::format(m_btnRollLabel, sizeof(m_btnRollLabel), WorkshopKey::WORKSHOP_SHUT_THE_BOX_BTN_ROLL);
    I18n::format(m_btnLayLabel, sizeof(m_btnLayLabel), WorkshopKey::WORKSHOP_SHUT_THE_BOX_BTN_LAY);
    I18n::format(m_btnNewLabel, sizeof(m_btnNewLabel), WorkshopKey::WORKSHOP_SHUT_THE_BOX_BTN_NEW);
    I18n::format(m_btnResetLabel, sizeof(m_btnResetLabel), WorkshopKey::WORKSHOP_SHUT_THE_BOX_BTN_RESET);

    m_btnOne.initButton(tft, cx1, cy, BTN_W_DICE, FOOTER_BTN_H, FOOTER_BTN_R, TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT, m_btnOneLabel, nullptr);
    m_btnTwo.initButton(tft, cx2, cy, BTN_W_DICE, FOOTER_BTN_H, FOOTER_BTN_R, TOGGLE_ACTIVE_OUTLINE, TOGGLE_ACTIVE_FILL, TOGGLE_ACTIVE_TEXT, m_btnTwoLabel, nullptr);
    m_btnRoll.initButton(tft, cx3, cy, BTN_W_ROLL, FOOTER_BTN_H, FOOTER_BTN_R, TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT, m_btnRollLabel, nullptr);
    m_btnLay.initButton(tft, cx4, cy, BTN_W_LAY, FOOTER_BTN_H, FOOTER_BTN_R, TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT, m_btnLayLabel, nullptr);
    m_btnNew.initButton(tft, cx5, cy, BTN_W_NEW, FOOTER_BTN_H, FOOTER_BTN_R, TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT, m_btnNewLabel, nullptr);
    m_btnReset.initButton(tft, cx6, cy, BTN_W_RESET, FOOTER_BTN_H, FOOTER_BTN_R, TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT, m_btnResetLabel, nullptr);
    m_btnHelp.initButton(tft, cx7, cy, BTN_W_HELP, FOOTER_BTN_H, FOOTER_BTN_R, TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT, "?", nullptr);

    m_btnOne.setFont(LeagueGothicCond34);
    m_btnTwo.setFont(LeagueGothicCond34);
    m_btnRoll.setFont(LeagueGothicCond34);
    m_btnLay.setFont(LeagueGothicCond34);
    m_btnNew.setFont(LeagueGothicCond34);
    m_btnReset.setFont(LeagueGothicCond34);
    m_btnHelp.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
    updateFooterButtonColors();
}

/**
 * @brief Aktualisiert Buttonfarben nach aktuellem Spielzustand.
 */
void WorkshopTool_ShutTheBox::updateFooterButtonColors()
{
    if (!m_buttonsInitialized)
        return;

    if (!canUseOneDie())
        m_oneDieMode = false;

    auto apply = [](Button &b, bool active, bool enabled)
    {
        b.setOutlineColor(active ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);
        b.setFillColor(active ? TOGGLE_ACTIVE_FILL : TOGGLE_INACTIVE_FILL);
        b.setTextColor(enabled ? (active ? TOGGLE_ACTIVE_TEXT : TOGGLE_INACTIVE_TEXT) : TOGGLE_DISABLED_TEXT);
    };

    const bool oneAllowed = canUseOneDie() && m_state == GameState::PLAYING && !m_hasRoll;
    apply(m_btnOne, m_oneDieMode && oneAllowed, oneAllowed);
    apply(m_btnTwo, !m_oneDieMode, m_state == GameState::PLAYING && !m_hasRoll);
    apply(m_btnRoll, m_state == GameState::PLAYING && !m_hasRoll, m_state == GameState::PLAYING && !m_hasRoll);
    apply(m_btnLay, canLaySelection(), m_state == GameState::PLAYING && m_hasRoll);
    apply(m_btnNew, false, true);
    apply(m_btnReset, false, true);
    apply(m_btnHelp, false, true);
}

/**
 * @brief Zeichnet die Fußzeilen-Buttons.
 */
void WorkshopTool_ShutTheBox::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    updateFooterButtonColors();
    m_btnOne.drawButton();
    m_btnTwo.drawButton();
    m_btnRoll.drawButton();
    m_btnLay.drawButton();
    m_btnNew.drawButton();
    m_btnReset.drawButton();
    m_btnHelp.drawButton();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf den Fußzeilen-Buttons.
 */
bool WorkshopTool_ShutTheBox::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    if (m_msg && m_msg->isVisible())
        return false;

    const bool oneEnabled = canUseOneDie() && m_state == GameState::PLAYING && !m_hasRoll;
    const bool twoEnabled = m_state == GameState::PLAYING && !m_hasRoll;
    const bool rollEnabled = m_state == GameState::PLAYING && !m_hasRoll;
    const bool layEnabled = m_state == GameState::PLAYING && m_hasRoll;

    const bool onOne = oneEnabled && touched && m_btnOne.contains(tx, ty);
    const bool onTwo = twoEnabled && touched && m_btnTwo.contains(tx, ty);
    const bool onRoll = rollEnabled && touched && m_btnRoll.contains(tx, ty);
    const bool onLay = layEnabled && touched && m_btnLay.contains(tx, ty);
    const bool onNew = touched && m_btnNew.contains(tx, ty);
    const bool onReset = touched && m_btnReset.contains(tx, ty);
    const bool onHelp = touched && m_btnHelp.contains(tx, ty);

    const bool wasOne = m_btnOne.isPressed();
    const bool wasTwo = m_btnTwo.isPressed();
    const bool wasRoll = m_btnRoll.isPressed();
    const bool wasLay = m_btnLay.isPressed();
    const bool wasNew = m_btnNew.isPressed();
    const bool wasReset = m_btnReset.isPressed();
    const bool wasHelp = m_btnHelp.isPressed();

    m_btnOne.press(onOne);
    m_btnTwo.press(onTwo);
    m_btnRoll.press(onRoll);
    m_btnLay.press(onLay);
    m_btnNew.press(onNew);
    m_btnReset.press(onReset);
    m_btnHelp.press(onHelp);

    if (m_btnOne.justReleased())
    {
        m_oneDieMode = true;
        updateFooterButtonColors();
        return true;
    }

    if (m_btnTwo.justReleased())
    {
        m_oneDieMode = false;
        updateFooterButtonColors();
        return true;
    }

    if (m_btnRoll.justReleased())
    {
        rollDice();
        return true;
    }

    if (m_btnLay.justReleased())
        return laySelection();

    if (m_btnNew.justReleased())
    {
        resetGame();
        updateFooterButtonColors();
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

    if (onOne != wasOne)
        m_btnOne.drawButton(onOne);
    if (onTwo != wasTwo)
        m_btnTwo.drawButton(onTwo);
    if (onRoll != wasRoll)
        m_btnRoll.drawButton(onRoll);
    if (onLay != wasLay)
        m_btnLay.drawButton(onLay);
    if (onNew != wasNew)
        m_btnNew.drawButton(onNew);
    if (onReset != wasReset)
        m_btnReset.drawButton(onReset);
    if (onHelp != wasHelp)
        m_btnHelp.drawButton(onHelp);

    return false;
}

// ===================================================================
// Untertitel
// ===================================================================

/**
 * @brief Liefert einen statusabhängigen Untertitel.
 */
const char *WorkshopTool_ShutTheBox::getSubtitle() const
{
    switch (m_state)
    {
    case GameState::WON:
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_SHUT_THE_BOX_SUB_WON);
        break;
    case GameState::GAMEOVER:
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_SHUT_THE_BOX_SUB_GAMEOVER);
        break;
    default:
        if (m_hasRoll)
            I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_SHUT_THE_BOX_FMT_SUB_PICK, static_cast<unsigned>(currentRollSum()));
        else
            I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_SHUT_THE_BOX_SUB_READY);
        break;
    }

    return m_subtitleBuf;
}

// ===================================================================
// Dialoge
// ===================================================================

/**
 * @brief Öffnet die Spielanleitung.
 */
void WorkshopTool_ShutTheBox::showHelpDialog()
{
    if (!m_msg)
        return;

    m_msgPurpose = MsgPurpose::HELP;
    I18n::format(m_helpTitleBuf, sizeof(m_helpTitleBuf), WorkshopKey::WORKSHOP_SHUT_THE_BOX_HELP_TITLE);
    I18n::format(m_helpTextBuf, sizeof(m_helpTextBuf), WorkshopKey::WORKSHOP_SHUT_THE_BOX_HELP_TEXT);
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
 * @brief Öffnet den Reset-Dialog.
 */
void WorkshopTool_ShutTheBox::showResetDialog()
{
    if (!m_msg)
        return;

    m_msgPurpose = MsgPurpose::RESET_STATS;
    I18n::format(m_resetTitleBuf, sizeof(m_resetTitleBuf), WorkshopKey::WORKSHOP_SHUT_THE_BOX_RESET_TITLE);
    I18n::format(m_resetTextBuf, sizeof(m_resetTextBuf), WorkshopKey::WORKSHOP_SHUT_THE_BOX_RESET_TEXT);
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
void WorkshopTool_ShutTheBox::onMsgResult(bool confirmed)
{
    const MsgPurpose purpose = m_msgPurpose;
    m_msgPurpose = MsgPurpose::NONE;

    if (purpose == MsgPurpose::RESET_STATS && confirmed)
    {
        m_bestRest = BEST_UNSET;
        m_wins = 0;
        saveStats();
    }

    if (m_buttonsInitialized)
    {
        m_btnOne.press(false);
        m_btnTwo.press(false);
        m_btnRoll.press(false);
        m_btnLay.press(false);
        m_btnNew.press(false);
        m_btnReset.press(false);
        m_btnHelp.press(false);

        updateFooterButtonColors();
        m_btnOne.drawButton();
        m_btnTwo.drawButton();
        m_btnRoll.drawButton();
        m_btnLay.drawButton();
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
 * @brief Lädt die gespeicherte Statistik.
 */
void WorkshopTool_ShutTheBox::loadStats()
{
    Preferences prefs;
    if (prefs.begin(PREFS_NS, true))
    {
        m_bestRest = prefs.getUShort(PREFS_KEY_BEST, BEST_UNSET);
        m_wins = prefs.getUShort(PREFS_KEY_WINS, 0);
        prefs.end();
    }
    else
    {
        m_bestRest = BEST_UNSET;
        m_wins = 0;
    }
}

/**
 * @brief Speichert die Statistik.
 */
void WorkshopTool_ShutTheBox::saveStats()
{
    Preferences prefs;
    if (prefs.begin(PREFS_NS, false))
    {
        prefs.putUShort(PREFS_KEY_BEST, m_bestRest);
        prefs.putUShort(PREFS_KEY_WINS, m_wins);
        prefs.end();
    }
}
