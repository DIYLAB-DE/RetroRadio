#include "WorkshopTool_BitEditor.h"

#include "I18n.h"
#include "Workshop.h"
#include "StringUtils.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================

namespace
{
    // ===================================================================
    // DotMatrix-Konfiguration: init(DOT_W, DOT_H, DOT_S, 0)
    // ===================================================================

    //   charW und charH berechnen sich automatisch:
    //     charW = 5 × (DOT_W + DOT_S) - DOT_S
    //     charH = 7 × (DOT_H + DOT_S) - DOT_S
    //
    // Bit-Mapping:  bitIdx(row, col) = (3 - row) × 8 + (7 - col)
    //   Zeile 0: Bits 31-24   (aktiv nur im 32-Bit-Modus)
    //   Zeile 1: Bits 23-16   (aktiv ab 32 Bit)
    //   Zeile 2: Bits 15-8    (aktiv ab 16 Bit)
    //   Zeile 3: Bits 7-0     (immer aktiv)

    constexpr int DOT_W = 6; ///< Breite eines einzelnen Dots in Pixeln
    constexpr int DOT_H = 5; ///< Höhe eines einzelnen Dots in Pixeln
    constexpr int DOT_S = 1; ///< Abstand zwischen Dots in Pixeln

    // Automatisch berechnet
    constexpr int CHAR_W = 5 * (DOT_W + DOT_S) - DOT_S; ///< Breite eines DotMatrix-Zeichens
    constexpr int CHAR_H = 7 * (DOT_H + DOT_S) - DOT_S; ///< Höhe eines DotMatrix-Zeichens

    constexpr int GRID_COLS = 8; ///< Anzahl der Spalten im Bit-Editor (max. 8, da 8 Bits pro Zeile)
    constexpr int GRID_ROWS = 4; ///< Anzahl der Zeilen im Bit-Editor (4 Zeilen für 32 Bit, aber je nach Modus nur 1-3 Zeilen aktiv)

    // Horizontale Abstände
    constexpr int CELL_GAP = 25;                  ///< Abstand zwischen Zellen innerhalb eines Nibbles
    constexpr int NIBBLE_EXTRA = 60;              ///< Zusätzlicher Abstand an der Nibble-Grenze (Spalte 3 -> 4)
    constexpr int CELL_PITCH = CHAR_W + CELL_GAP; ///< Abstand von Zelle zu Zelle (außer Nibble-Grenze)

    // ===================================================================
    // Vertikales Layout
    // ===================================================================

    //   TOP_GAP          Abstand oben
    //   PANEL_H          LCD-Panel (berechnet)
    //   VAL_GAP          Abstand Panel -> Wertanzeige
    //   2 × VAL_LINE_H + VAL_LINE_GAP   Wertanzeige (2 Zeilen)
    //   -> Rest = unterer Rand (mind. 10 px)

    constexpr int TOP_GAP = 10;                             ///< Abstand Content-Ursprung -> Panel-Oberkante
    constexpr int PANEL_PAD_LEFT = 20;                      ///< Abstand Panel-Rand -> erstes Bit-Kästchen
    constexpr int PANEL_PAD_TOP = 5;                        ///< Oberer Innenabstand im Panel
    constexpr int PANEL_PAD_BOT = 20;                       ///< Unterer Innenabstand
    constexpr int BIT_NUM_H = 14;                           ///< Höhe für Bit-Nummern-Text (ComfortaaBold14)
    constexpr int NUM_DOT_GAP = 2;                          ///< Abstand zwischen Bit-Nummer und DotMatrix
    constexpr int ROW_H = BIT_NUM_H + NUM_DOT_GAP + CHAR_H; ///< Gesamthöhe pro Zeile
    constexpr int ROW_GAP = 2;                              ///< Abstand zwischen Zeilen
    constexpr int PANEL_CORNER_R = 8;                       ///< Eckenradius des Panels

    // panelH berechnet sich automatisch aus den Grundkonstanten
    constexpr int PANEL_H = PANEL_PAD_TOP + GRID_ROWS * ROW_H + (GRID_ROWS - 1) * ROW_GAP + PANEL_PAD_BOT; ///< Höhe des LCD-Panels (automatisch berechnet)

    constexpr int VAL_GAP = 10;     ///< Abstand Panel -> Wertanzeige
    constexpr int VAL_LINE_H = 20;  ///< Zeilenhöhe Wertanzeige
    constexpr int VAL_LINE_GAP = 9; ///< Abstand zwischen den zwei Wertzeilen

    // Wertanzeige X-Positionen
    constexpr int VAL_DEC_LBL = 0;   ///< X-Position des "DEC"-Labels (relativ zum Content-Ursprung)
    constexpr int VAL_DEC_X = 42;    ///< X-Position der Dezimalwertanzeige
    constexpr int VAL_HEX_LBL = 170; ///< X-Position des "HEX"-Labels
    constexpr int VAL_HEX_X = 212;   ///< X-Position der Hexadezimalwertanzeige
    constexpr int VAL_OCT_LBL = 340; ///< X-Position des "OCT"-Labels
    constexpr int VAL_OCT_X = 382;   ///< X-Position der Oktalwertanzeige
    constexpr int VAL_BIN_LBL = 0;   ///< X-Position des "BIN"-Labels
    constexpr int VAL_BIN_X = 42;    ///< X-Position der Binärwertanzeige

    // ===================================================================
    // Footer-Buttons
    // ===================================================================

    constexpr int FOOTER_BTN_W = 66;     ///< Breite der Toggle-Buttons im Footer (8, 16, 32) und Aktionsbuttons (NOT, CLR)
    constexpr int FOOTER_BTN_H = 44;     ///< Höhe der Toggle-Buttons im Footer (8, 16, 32) und Aktionsbuttons (NOT, CLR)
    constexpr int FOOTER_BTN_R = 6;      ///< Eckenradius der Toggle-Buttons im Footer (8, 16, 32) und Aktionsbuttons (NOT, CLR)
    constexpr int FOOTER_BTN_GAP = 8;    ///< Abstand zwischen den Toggle-Buttons im Footer (8, 16, 32) und Aktionsbuttons (NOT, CLR)
    constexpr int FOOTER_GROUP_GAP = 16; ///< Zusätzlicher Abstand zwischen Toggle-Buttons (8, 16, 32) und Aktionsbuttons (NOT, CLR)

    constexpr uint16_t TOGGLE_ACTIVE_FILL = 0x328A;      ///< Farbe für aktiven Toggle-Button (Hintergrund)
    constexpr uint16_t TOGGLE_ACTIVE_TEXT = 0xFFFF;      ///< Farbe für aktiven Toggle-Button (Text)
    constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;   ///< Farbe für aktiven Toggle-Button (Rahmen)
    constexpr uint16_t TOGGLE_INACTIVE_FILL = 0x0000;    ///< Farbe für inaktiven Toggle-Button (Hintergrund)
    constexpr uint16_t TOGGLE_INACTIVE_TEXT = 0x7BEF;    ///< Farbe für inaktiven Toggle-Button (Text)
    constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8; ///< Farbe für inaktiven Toggle-Button (Rahmen)
    constexpr uint16_t ACTION_FILL = 0x0000;             ///< Farbe für Aktions-Buttons (Hintergrund)
    constexpr uint16_t ACTION_TEXT = 0xAD75;             ///< Farbe für Aktions-Buttons (Text)
    constexpr uint16_t ACTION_OUTLINE = 0x39E8;          ///< Farbe für Aktions-Buttons (Rahmen)

} // namespace

// ===================================================================
// Konstruktor
// ===================================================================

/**
 * @brief Konfiguriert die DotMatrix im LCD-Stil (dunkel auf hell).
 */
WorkshopTool_BitEditor::WorkshopTool_BitEditor(BitEditorColors colors)
    : m_colors(colors)
{
    m_dot.init(DOT_W, DOT_H, DOT_S, 0);
    m_dot.setOnColor(m_colors.lcdOn, m_colors.lcdOn);
    m_dot.setOffColor(m_colors.lcdOff, m_colors.lcdOff);
    m_dot.setBackgroundColor(m_colors.dispBg);
    m_dot.setShowOffDots(true);
    m_dot.setDotShape(DotMatrix::DotShape::SQUARE);
    m_dot.setGradientEnabled(false);
}

// ===================================================================
// Lebenszyklus
// ===================================================================

/**
 * @brief Reserviert DotMatrix-Textpuffer und setzt den Editor zurück.
 */
void WorkshopTool_BitEditor::onEnter(LGFX *tft)
{
    m_value = 0;
    m_bitWidth = 8;
    m_touchWasActive = false;
    m_panelX = m_panelY = 0;
    m_gridOffsetX = 0;

    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));
    refreshSubtitle();

    m_dot.reserveText(2);
    initFooterButtons(tft);
}

/**
 * @brief Räumt den Werkzeugzustand auf.
 */
void WorkshopTool_BitEditor::onExit()
{
    m_value = 0;
    m_bitWidth = 8;
    m_buttonsInitialized = false;
}

// ===================================================================
// Footer-Buttons
// ===================================================================

/**
 * @brief Initialisiert die fünf Footer-Buttons.
 */
void WorkshopTool_BitEditor::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int totalW = 5 * FOOTER_BTN_W + 3 * FOOTER_BTN_GAP + FOOTER_GROUP_GAP;
    constexpr int startX = WorkshopCfg::TOOL_FOOTER_X + (WorkshopCfg::TOOL_FOOTER_W - totalW) / 2;

    const int cx8 = startX + FOOTER_BTN_W / 2;
    const int cx16 = cx8 + FOOTER_BTN_W + FOOTER_BTN_GAP;
    const int cx32 = cx16 + FOOTER_BTN_W + FOOTER_BTN_GAP;
    const int cxNot = cx32 + FOOTER_BTN_W / 2 + FOOTER_GROUP_GAP + FOOTER_BTN_W / 2;
    const int cxClr = cxNot + FOOTER_BTN_W + FOOTER_BTN_GAP;

    m_btn8.initButton(
        tft,                   // Display-Objekt
        cx8,                   // X-Zentrum
        cy,                    // Y-Zentrum
        FOOTER_BTN_W,          // Breite
        FOOTER_BTN_H,          // Höhe
        FOOTER_BTN_R,          // Eckenradius
        TOGGLE_ACTIVE_OUTLINE, // Randfarbe
        TOGGLE_ACTIVE_FILL,    // Füllfarbe
        TOGGLE_ACTIVE_TEXT,    // Textfarbe
        "8",                   // Text
        nullptr                // kein Icon
    );

    m_btn8.setFont(LeagueGothicCond34);

    m_btn16.initButton(
        tft,                     // Display-Objekt
        cx16,                    // X-Zentrum
        cy,                      // Y-Zentrum
        FOOTER_BTN_W,            // Breite
        FOOTER_BTN_H,            // Höhe
        FOOTER_BTN_R,            // Eckenradius
        TOGGLE_INACTIVE_OUTLINE, // Randfarbe
        TOGGLE_INACTIVE_FILL,    // Füllfarbe
        TOGGLE_INACTIVE_TEXT,    // Textfarbe
        "16",                    // Text
        nullptr                  // kein Icon
    );

    m_btn16.setFont(LeagueGothicCond34);

    m_btn32.initButton(
        tft,                     // Display-Objekt
        cx32,                    // X-Zentrum
        cy,                      // Y-Zentrum
        FOOTER_BTN_W,            // Breite
        FOOTER_BTN_H,            // Höhe
        FOOTER_BTN_R,            // Eckenradius
        TOGGLE_INACTIVE_OUTLINE, // Randfarbe
        TOGGLE_INACTIVE_FILL,    // Füllfarbe
        TOGGLE_INACTIVE_TEXT,    // Textfarbe
        "32",                    // Text
        nullptr                  // kein Icon
    );

    m_btn32.setFont(LeagueGothicCond34);

    m_btnNot.initButton(
        tft,            // Display-Objekt
        cxNot,          // X-Zentrum
        cy,             // Y-Zentrum
        FOOTER_BTN_W,   // Breite
        FOOTER_BTN_H,   // Höhe
        FOOTER_BTN_R,   // Eckenradius
        ACTION_OUTLINE, // Randfarbe
        ACTION_FILL,    // Füllfarbe
        ACTION_TEXT,    // Textfarbe
        "not",          // Text
        nullptr         // kein Icon
    );

    m_btnNot.setFont(LeagueGothicCond34);

    m_btnClr.initButton(
        tft,            // Display-Objekt
        cxClr,          // X-Zentrum
        cy,             // Y-Zentrum
        FOOTER_BTN_W,   // Breite
        FOOTER_BTN_H,   // Höhe
        FOOTER_BTN_R,   // Eckenradius
        ACTION_OUTLINE, // Randfarbe
        ACTION_FILL,    // Füllfarbe
        ACTION_TEXT,    // Textfarbe
        "clr",          // Text
        nullptr         // kein Icon
    );

    m_btnClr.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
}

/**
 * @brief Setzt Farben der Toggle-Buttons entsprechend m_bitWidth.
 */
void WorkshopTool_BitEditor::updateButtonColors()
{
    Button *toggles[] = {&m_btn8, &m_btn16, &m_btn32};
    const uint8_t widths[] = {8, 16, 32};

    for (int i = 0; i < 3; ++i)
    {
        if (widths[i] == m_bitWidth)
        {
            toggles[i]->setFillColor(TOGGLE_ACTIVE_FILL);
            toggles[i]->setTextColor(TOGGLE_ACTIVE_TEXT);
            toggles[i]->setOutlineColor(TOGGLE_ACTIVE_OUTLINE);
        }
        else
        {
            toggles[i]->setFillColor(TOGGLE_INACTIVE_FILL);
            toggles[i]->setTextColor(TOGGLE_INACTIVE_TEXT);
            toggles[i]->setOutlineColor(TOGGLE_INACTIVE_OUTLINE);
        }
    }
}

// ===================================================================
// Footer: Zeichnen und Touch
// ===================================================================

/**
 * @brief Zeichnet die Footer-Buttons.
 */
void WorkshopTool_BitEditor::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    updateButtonColors();
    m_btn8.drawButton();
    m_btn16.drawButton();
    m_btn32.drawButton();
    m_btnNot.drawButton();
    m_btnClr.drawButton();
}

/**
 * @brief Verarbeitet Touch-Events auf den Footer-Buttons.
 */
bool WorkshopTool_BitEditor::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    const bool on8 = touched && m_btn8.contains(tx, ty);
    const bool on16 = touched && m_btn16.contains(tx, ty);
    const bool on32 = touched && m_btn32.contains(tx, ty);
    const bool onNot = touched && m_btnNot.contains(tx, ty);
    const bool onClr = touched && m_btnClr.contains(tx, ty);

    const bool was8 = m_btn8.isPressed();
    const bool was16 = m_btn16.isPressed();
    const bool was32 = m_btn32.isPressed();
    const bool wasNot = m_btnNot.isPressed();
    const bool wasClr = m_btnClr.isPressed();

    m_btn8.press(on8);
    m_btn16.press(on16);
    m_btn32.press(on32);
    m_btnNot.press(onNot);
    m_btnClr.press(onClr);

    // Bitbreiten-Toggle bei Loslassen
    if (m_btn8.justReleased() && m_bitWidth != 8)
    {
        m_bitWidth = 8;
        m_value &= 0xFF;
        refreshSubtitle();
        return true;
    }
    if (m_btn16.justReleased() && m_bitWidth != 16)
    {
        m_bitWidth = 16;
        m_value &= 0xFFFF;
        refreshSubtitle();
        return true;
    }
    if (m_btn32.justReleased() && m_bitWidth != 32)
    {
        m_bitWidth = 32;
        refreshSubtitle();
        return true;
    }

    // Aktions-Buttons bei Loslassen
    if (m_btnNot.justReleased())
    {
        m_value = ~m_value;
        if (m_bitWidth < 32)
            m_value &= ((1UL << m_bitWidth) - 1);
        return true;
    }
    if (m_btnClr.justReleased())
    {
        m_value = 0;
        return true;
    }

    // Visuelle Rückmeldung
    if (on8 != was8)
        m_btn8.drawButton(on8);

    if (on16 != was16)
        m_btn16.drawButton(on16);

    if (on32 != was32)
        m_btn32.drawButton(on32);

    if (onNot != wasNot)
        m_btnNot.drawButton(onNot);

    if (onClr != wasClr)
        m_btnClr.drawButton(onClr);

    return false;
}

/**
 * @brief Gibt den zur aktiven Bitbreite passenden Untertitel zurück.
 */
const char *WorkshopTool_BitEditor::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Befüllt m_subtitleBuf passend zur aktuellen Bitbreite.
 */
void WorkshopTool_BitEditor::refreshSubtitle()
{
    WorkshopKey key;
    switch (m_bitWidth)
    {
    case 16:
        key = WorkshopKey::WORKSHOP_BIT_EDITOR_SUB_16;
        break;
    case 32:
        key = WorkshopKey::WORKSHOP_BIT_EDITOR_SUB_32;
        break;
    default:
        key = WorkshopKey::WORKSHOP_BIT_EDITOR_SUB_8;
        break;
    }
    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), key);
}

// ===================================================================
// Zeichenfunktionen
// ===================================================================

/**
 * @brief Zeichnet LCD-Panel und Wertanzeige in den Work-Sprite.
 */
void WorkshopTool_BitEditor::draw(Gfx_Sprite *spr, int ox, int oy, int w, int h)
{
    (void)h; // Festes Layout - Höhe wird nicht benötigt

    // Panel-Position cachen (für hitTestBit)
    m_panelX = ox;
    m_panelY = oy + TOP_GAP;
    m_gridOffsetX = PANEL_PAD_LEFT;

    drawLcdPanel(spr, m_panelX, m_panelY, w);
    drawValues(spr, ox, m_panelY + PANEL_H + VAL_GAP, w);
}

/**
 * @brief Zeichnet das LCD-Panel mit 4×8 DotMatrix-Gitter.
 */
void WorkshopTool_BitEditor::drawLcdPanel(Gfx_Sprite *spr, int panelX, int panelY, int panelW)
{
    // Heller LCD-Hintergrund
    spr->fillSmoothRoundRect(panelX, panelY, panelW, PANEL_H, PANEL_CORNER_R, m_colors.dispBg);

    const int gridX = panelX + m_gridOffsetX;

    // Pass 1: Bit-Nummern (ein Font-Lade-Zyklus)
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(m_colors.dispLabel, m_colors.dispBg);

    for (int row = 0; row < GRID_ROWS; ++row)
    {
        const int numY = panelY + PANEL_PAD_TOP + row * (ROW_H + ROW_GAP);

        for (int col = 0; col < GRID_COLS; ++col)
        {
            const int bitIdx = (3 - row) * 8 + (7 - col);
            if (bitIdx >= m_bitWidth)
                continue; // Inaktiv - keine Nummer

            const int cx = gridX + col * CELL_PITCH + (col >= 4 ? NIBBLE_EXTRA : 0);
            char numBuf[4];
            stringutils::safeFormat(numBuf, "%d", bitIdx);
            spr->drawString(numBuf, cx + CHAR_W / 2, numY + BIT_NUM_H / 2);
        }
    }
    spr->unloadFont();

    // Pass 2: DotMatrix-Kästchen
    m_dot.setOffColor(m_colors.lcdOff, m_colors.lcdOff);
    m_dot.setBackgroundColor(m_colors.dispBg);

    for (int row = 0; row < GRID_ROWS; ++row)
    {
        const int dotY = panelY + PANEL_PAD_TOP + row * (ROW_H + ROW_GAP) + BIT_NUM_H + NUM_DOT_GAP;

        for (int col = 0; col < GRID_COLS; ++col)
        {
            const int bitIdx = (3 - row) * 8 + (7 - col);
            const bool active = bitIdx < m_bitWidth;
            const int cx = gridX + col * CELL_PITCH + (col >= 4 ? NIBBLE_EXTRA : 0);

            if (active)
            {
                const bool bitSet = (m_value & (1UL << bitIdx)) != 0;
                m_dot.setOnColor(m_colors.lcdOn, m_colors.lcdOn);
                m_dot.setText(bitSet ? "1" : "0");
            }
            else
            {
                // Ghost-Pattern: ON = OFF - uniformes Raster
                m_dot.setOnColor(m_colors.lcdOff, m_colors.lcdOff);
                m_dot.setText("0");
            }

            m_dot.draw(spr, cx, dotY);
        }
    }
}

/**
 * @brief Zeichnet DEC / HEX / OCT + BIN als Plain-Text in ComfortaaBold16.
 */
void WorkshopTool_BitEditor::drawValues(Gfx_Sprite *spr, int ox, int valY, int w)
{
    (void)w;
    const uint32_t val = maskedValue();

    char decBuf[12];
    char hexBuf[12];
    char octBuf[14];
    char binBuf[44];

    stringutils::safeFormat(decBuf, "%lu", (unsigned long)val);

    if (m_bitWidth == 8)
        stringutils::safeFormat(hexBuf, "0x%02X", (unsigned int)val);
    else if (m_bitWidth == 16)
        stringutils::safeFormat(hexBuf, "0x%04X", (unsigned int)val);
    else
        stringutils::safeFormat(hexBuf, "0x%08lX", (unsigned long)val);

    stringutils::safeFormat(octBuf, "%lo", (unsigned long)val);
    formatBinary(binBuf, sizeof(binBuf));

    constexpr int FONT_OY = 2; ///< Vertikale Baseline-Korrektur für ComfortaaBold16
    const int cy1 = valY + VAL_LINE_H / 2 + FONT_OY;
    const int cy2 = valY + VAL_LINE_H + VAL_LINE_GAP + VAL_LINE_H / 2 + FONT_OY;

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);

    // Zeile 1: DEC    HEX    OCT
    spr->setTextColor(m_colors.valKey, m_colors.tableBg);
    spr->drawString("DEC", ox + VAL_DEC_LBL, cy1);
    spr->drawString("HEX", ox + VAL_HEX_LBL, cy1);
    spr->drawString("OCT", ox + VAL_OCT_LBL, cy1);

    spr->setTextColor(m_colors.valTx, m_colors.tableBg);
    spr->drawString(decBuf, ox + VAL_DEC_X, cy1);
    spr->drawString(hexBuf, ox + VAL_HEX_X, cy1);
    spr->drawString(octBuf, ox + VAL_OCT_X, cy1);

    // Zeile 2: BIN
    spr->setTextColor(m_colors.valKey, m_colors.tableBg);
    spr->drawString("BIN", ox + VAL_BIN_LBL, cy2);

    spr->setTextColor(m_colors.valTx, m_colors.tableBg);
    spr->drawString(binBuf, ox + VAL_BIN_X, cy2);

    spr->unloadFont();
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Events - toggelt aktive Bits.
 */
bool WorkshopTool_BitEditor::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    if (touched && !m_touchWasActive)
    {
        const int bitIdx = hitTestBit(tx, ty);
        if (bitIdx >= 0)
        {
            m_value ^= (1UL << bitIdx);
            m_touchWasActive = true;
            return true;
        }
    }

    if (!touched)
        m_touchWasActive = false;

    return false;
}

/**
 * @brief Ermittelt den Bit-Index an einer absoluten Display-Position.
 */
int WorkshopTool_BitEditor::hitTestBit(int32_t tx, int32_t ty) const
{
    const int gridX = m_panelX + m_gridOffsetX;
    const int relX = tx - (WorkshopCfg::WORK_X + gridX);
    const int relY = ty - (WorkshopCfg::WORK_Y + m_panelY);

    if (relX < 0 || relY < 0)
        return -1;

    for (int row = 0; row < GRID_ROWS; ++row)
    {
        const int rowY = PANEL_PAD_TOP + row * (ROW_H + ROW_GAP);
        const int dotTop = rowY + BIT_NUM_H + NUM_DOT_GAP;
        const int dotBot = dotTop + CHAR_H;

        if (relY >= dotTop && relY < dotBot)
        {
            for (int col = 0; col < GRID_COLS; ++col)
            {
                const int cx = col * CELL_PITCH + (col >= 4 ? NIBBLE_EXTRA : 0);
                if (relX >= cx && relX < cx + CHAR_W)
                {
                    const int bitIdx = (3 - row) * 8 + (7 - col);
                    return (bitIdx < m_bitWidth) ? bitIdx : -1;
                }
            }
            return -1;
        }
    }
    return -1;
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Gibt den maskierten Wert für die aktuelle Bitbreite zurück.
 */
uint32_t WorkshopTool_BitEditor::maskedValue() const
{
    if (m_bitWidth >= 32)
        return m_value;

    return m_value & ((1UL << m_bitWidth) - 1);
}

/**
 * @brief Formatiert den aktuellen Wert als Binärstring mit Nibble-Leerzeichen.
 */
void WorkshopTool_BitEditor::formatBinary(char *buf, size_t bufSize) const
{
    const uint32_t val = maskedValue();
    int pos = 0;

    for (int i = m_bitWidth - 1; i >= 0 && pos < static_cast<int>(bufSize) - 1; --i)
    {
        if (pos > 0 && ((i + 1) % 4 == 0))
        {
            if (pos >= static_cast<int>(bufSize) - 1)
                break;
            buf[pos++] = ' ';
        }
        buf[pos++] = (val & (1UL << i)) ? '1' : '0';
    }

    buf[pos] = '\0';
}
