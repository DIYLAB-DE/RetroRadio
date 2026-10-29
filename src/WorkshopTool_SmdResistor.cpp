#include "WorkshopTool_SmdResistor.h"

#include <cmath>
#include <cstring>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold30.h"
#include "fonts/LeagueGothicCond34.h"

namespace
{
    constexpr uint16_t TOGGLE_ACTIVE_FILL = 0x328A;      ///< Aktiver Button-Hintergrund
    constexpr uint16_t TOGGLE_ACTIVE_TEXT = 0xFFFF;      ///< Aktiver Button-Text
    constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;   ///< Aktiver Button-Rand
    constexpr uint16_t TOGGLE_INACTIVE_FILL = 0x0000;    ///< Inaktiver Button-Hintergrund
    constexpr uint16_t TOGGLE_INACTIVE_TEXT = 0xC618;    ///< Inaktiver Button-Text
    constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x4A49; ///< Inaktiver Button-Rand
    constexpr int FOOTER_BTN_W = 104;                    ///< Breite eines Modus-Buttons
    constexpr int FOOTER_BTN_H = 44;                     ///< Höhe der Modus-Buttons
    constexpr int FOOTER_BTN_R = 6;                      ///< Eckenradius der Modus-Buttons
    constexpr int FOOTER_BTN_GAP = 8;                    ///< Abstand zwischen Modus-Buttons

    struct SmdPackageRow
    {
        const char *code; ///< Bauformcode
        const char *mm;   ///< Länge x Breite in Millimetern
    };

    constexpr SmdPackageRow SMD_PACKAGE_ROWS[] = {
        {"2512", "6,3 x 3,2"},
        {"1210", "3,2 x 2,5"},
        {"1206", "3,2 x 1,6"},
        {"0805", "2,0 x 1,25"},
        {"0603", "1,6 x 0,8"},
        {"0402", "1,0 x 0,5"},
        {"0201", "0,6 x 0,3"}};
}

// ===================================================================
// Statische Datenbasis
// ===================================================================

/** @brief I18n-Keys der drei Modus-Buttons. */
const WorkshopKey WorkshopTool_SmdResistor::MODE_LABEL_KEYS[WorkshopTool_SmdResistor::NUM_MODES] = {
    WorkshopKey::WORKSHOP_SMD_RESISTOR_MODE_3,
    WorkshopKey::WORKSHOP_SMD_RESISTOR_MODE_4,
    WorkshopKey::WORKSHOP_SMD_RESISTOR_MODE_R};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Setzt den Startzustand.
 */
WorkshopTool_SmdResistor::WorkshopTool_SmdResistor()
{
    clearAll();
}

/**
 * @brief Setzt das Werkzeug beim Betreten zurück.
 */
void WorkshopTool_SmdResistor::onEnter(LGFX *tft)
{
    clearAll();
    initFooterButtons(tft);
}

/**
 * @brief Initialisiert alle Member auf definierte Werte.
 */
void WorkshopTool_SmdResistor::clearAll()
{
    memset(m_cellRects, 0, sizeof(m_cellRects));
    memset(m_selectedRows, 0, sizeof(m_selectedRows));
    memset(m_code, 0, sizeof(m_code));
    memset(m_valueStr, 0, sizeof(m_valueStr));
    memset(m_modeLabels, 0, sizeof(m_modeLabels));

    m_sprite = nullptr;
    m_originX = 0;
    m_originY = 0;
    m_w = 0;
    m_h = 0;
    m_layoutValid = false;
    m_dirty = true;
    m_selectorRect = {};
    m_previewRect = {};
    m_resultRect = {};
    m_mode = Mode::Digit3;
    m_resultKind = ResultKind::Invalid;
    m_errorKey = WorkshopKey::WORKSHOP_SMD_RESISTOR_ERR_INVALID;

    m_pressedCol = -1;
    m_pressedRow = -1;
    m_touchWas = false;
    m_touchReady = false;
    m_footerButtonsInitialized = false;

    setDefaultForMode();
}

// ===================================================================
// Öffentliche Interface-Methoden
// ===================================================================

/**
 * @brief Zeichnet das Werkzeug vollständig neu.
 */
void WorkshopTool_SmdResistor::draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
{
    m_sprite = sprite;
    m_originX = originX;
    m_originY = originY;

    if (!m_layoutValid || m_w != w || m_h != h)
    {
        m_w = w;
        m_h = h;
        computeLayout(w, h);
        m_layoutValid = true;
    }

    if (m_dirty)
    {
        updateCodeFromSelection();
        decode();
        m_dirty = false;
    }

    m_sprite->fillRect(m_originX, m_originY, m_w, m_h, C_BG);
    drawSelector();
    drawPreview();
    drawResult();
}

/**
 * @brief Verarbeitet Touch-Ereignisse mit Touch-up-Semantik.
 */
bool WorkshopTool_SmdResistor::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        int col = -1;
        int row = -1;
        const bool cellHit = hitTestCell(cx, cy, col, row);

        if (cellHit)
        {
            if (m_pressedCol != col || m_pressedRow != row)
            {
                m_pressedCol = col;
                m_pressedRow = row;
                needsRedraw = true;
            }
        }
        else if (m_pressedCol >= 0 || m_pressedRow >= 0)
        {
            m_pressedCol = -1;
            m_pressedRow = -1;
            needsRedraw = true;
        }

        m_touchWas = true;
    }
    else
    {
        m_touchReady = true;

        if (m_touchWas)
        {
            if (m_pressedCol >= 0 && m_pressedCol < activeCols() && isRowValid(m_pressedCol, m_pressedRow))
            {
                m_selectedRows[m_pressedCol] = static_cast<uint8_t>(m_pressedRow);
                m_dirty = true;
                needsRedraw = true;
            }
        }

        if (m_pressedCol >= 0 || m_pressedRow >= 0)
            needsRedraw = true;

        m_pressedCol = -1;
        m_pressedRow = -1;
        m_touchWas = false;
    }

    return needsRedraw;
}

/**
 * @brief Zeichnet die Modus-Buttons im Footer.
 */
void WorkshopTool_SmdResistor::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    if (!m_footerButtonsInitialized)
        initFooterButtons(tft);

    updateFooterButtonColors();
    for (int i = 0; i < NUM_MODES; ++i)
        m_modeButtons[i].drawButton();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf den Footer-Modus-Buttons.
 */
bool WorkshopTool_SmdResistor::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_footerButtonsInitialized)
        return false;

    bool changed = false;

    for (int i = 0; i < NUM_MODES; ++i)
    {
        const bool on = touched && m_modeButtons[i].contains(tx, ty);
        const bool was = m_modeButtons[i].isPressed();
        m_modeButtons[i].press(on);

        if (m_modeButtons[i].justReleased() && static_cast<int>(m_mode) != i)
        {
            setMode(static_cast<Mode>(i));
            changed = true;
        }

        if (!changed && on != was)
            m_modeButtons[i].drawButton(on);
    }

    return changed;
}

// ===================================================================
// Layout und Zustand
// ===================================================================

/**
 * @brief Berechnet Auswahl-, Vorschau- und Ergebnisrechtecke.
 */
void WorkshopTool_SmdResistor::computeLayout(int w, int h)
{
    const int mainH = h - BODY_TOP - RESULT_H - RESULT_GAP;
    m_selectorRect = {0, BODY_TOP, SELECT_W, mainH};
    m_resultRect = {0, h - RESULT_H, w, RESULT_H};

    const int previewX = SELECT_W + COL_GAP;
    m_previewRect = {previewX, BODY_TOP, w - previewX, mainH};

    const int cols = activeCols();
    const int rows = activeRows();
    const int cellW = (SELECT_W - (cols - 1) * COL_GAP) / cols;
    const int cellH = (mainH - (rows - 1) * CELL_GAP) / rows;

    for (int c = 0; c < MAX_COLS; ++c)
    {
        for (int r = 0; r < MAX_ROWS; ++r)
        {
            if (c < cols && r < rows)
            {
                const int x = c * (cellW + COL_GAP);
                const int y = BODY_TOP + r * (cellH + CELL_GAP);
                m_cellRects[c][r] = {x, y, cellW, cellH};
            }
            else
            {
                m_cellRects[c][r] = {};
            }
        }
    }
}

/**
 * @brief Stellt sinnvolle Beispielcodes für den aktiven Modus ein.
 */
void WorkshopTool_SmdResistor::setDefaultForMode()
{
    memset(m_selectedRows, 0, sizeof(m_selectedRows));

    switch (m_mode)
    {
    case Mode::Digit3:
        m_selectedRows[0] = 4;
        m_selectedRows[1] = 7;
        m_selectedRows[2] = 2;
        break;
    case Mode::Digit4:
        m_selectedRows[0] = 4;
        m_selectedRows[1] = 7;
        m_selectedRows[2] = 0;
        m_selectedRows[3] = 1;
        break;
    case Mode::RCode:
        m_selectedRows[0] = 4;
        m_selectedRows[1] = R_MARK_ROW;
        m_selectedRows[2] = 7;
        m_selectedRows[3] = BLANK_ROW;
        break;
    }

    m_dirty = true;
}

/**
 * @brief Wechselt den Modus und setzt dessen Standardcode.
 */
void WorkshopTool_SmdResistor::setMode(Mode mode)
{
    if (m_mode == mode)
        return;

    m_mode = mode;
    m_layoutValid = false;
    setDefaultForMode();
    updateFooterButtonColors();
}

/**
 * @brief Initialisiert die drei Modus-Buttons im Footer.
 */
void WorkshopTool_SmdResistor::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int startX = WorkshopCfg::TOOL_FOOTER_X;

    for (int i = 0; i < NUM_MODES; ++i)
    {
        I18n::format(m_modeLabels[i], sizeof(m_modeLabels[i]), MODE_LABEL_KEYS[i]);
        const int cx = startX + FOOTER_BTN_W / 2 + i * (FOOTER_BTN_W + FOOTER_BTN_GAP);

        m_modeButtons[i].initButton(
            tft,                     // Grafikobjekt
            cx,                      // Button-Zentrum X
            cy,                      // Button-Zentrum Y
            FOOTER_BTN_W,            // Button-Breite
            FOOTER_BTN_H,            // Button-Höhe
            FOOTER_BTN_R,            // Button-Radius
            TOGGLE_INACTIVE_OUTLINE, // Rahmenfarbe
            TOGGLE_INACTIVE_FILL,    // Füllfarbe
            TOGGLE_INACTIVE_TEXT,    // Textfarbe
            m_modeLabels[i],         // Beschriftung
            nullptr                  // kein Icon
        );

        m_modeButtons[i].setFont(LeagueGothicCond34);
    }

    m_footerButtonsInitialized = true;
    updateFooterButtonColors();
}

/**
 * @brief Aktualisiert die Farben der Modus-Buttons.
 */
void WorkshopTool_SmdResistor::updateFooterButtonColors()
{
    if (!m_footerButtonsInitialized)
        return;

    for (int i = 0; i < NUM_MODES; ++i)
    {
        const bool active = (static_cast<int>(m_mode) == i);
        m_modeButtons[i].setFillColor(active ? TOGGLE_ACTIVE_FILL : TOGGLE_INACTIVE_FILL);
        m_modeButtons[i].setTextColor(active ? TOGGLE_ACTIVE_TEXT : TOGGLE_INACTIVE_TEXT);
        m_modeButtons[i].setOutlineColor(active ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);
    }
}

/**
 * @brief Baut m_code aus der Spaltenauswahl.
 */
void WorkshopTool_SmdResistor::updateCodeFromSelection()
{
    memset(m_code, 0, sizeof(m_code));

    const int cols = activeCols();
    size_t out = 0;
    for (int i = 0; i < cols && out < CODE_MAX; ++i)
    {
        const uint8_t row = m_selectedRows[i];
        if (row == BLANK_ROW)
            continue;

        m_code[out] = (row == R_MARK_ROW) ? 'R' : static_cast<char>('0' + row);
        ++out;
    }
}

/**
 * @brief Liefert die Anzahl Spalten für den aktiven Modus.
 */
int WorkshopTool_SmdResistor::activeCols() const
{
    return (m_mode == Mode::Digit3) ? 3 : 4;
}

/**
 * @brief Liefert die Anzahl Auswahlzeilen für den aktiven Modus.
 */
int WorkshopTool_SmdResistor::activeRows() const
{
    return (m_mode == Mode::RCode) ? R_ROWS : DIGIT_ROWS;
}

// ===================================================================
// Auswertung
// ===================================================================

/**
 * @brief Dekodiert den aktuellen Code.
 */
void WorkshopTool_SmdResistor::decode()
{
    memset(m_valueStr, 0, sizeof(m_valueStr));

    const size_t len = strlen(m_code);
    if (len == 0)
    {
        m_resultKind = ResultKind::Invalid;
        m_errorKey = WorkshopKey::WORKSHOP_SMD_RESISTOR_ERR_EMPTY;
        return;
    }

    double ohm = 0.0;
    if (isZeroCode())
    {
        formatResistance(0.0, m_valueStr, sizeof(m_valueStr));
        m_resultKind = ResultKind::Zero;
        return;
    }

    if (isNumericCode())
    {
        if (decodeNumeric(len, ohm))
        {
            formatResistance(ohm, m_valueStr, sizeof(m_valueStr));
            m_resultKind = (len == 3) ? ResultKind::Numeric3 : ResultKind::Numeric4;
            return;
        }

        m_resultKind = ResultKind::Invalid;
        m_errorKey = WorkshopKey::WORKSHOP_SMD_RESISTOR_ERR_INVALID;
        return;
    }

    if (decodeRCode(ohm))
    {
        formatResistance(ohm, m_valueStr, sizeof(m_valueStr));
        m_resultKind = (ohm == 0.0) ? ResultKind::Zero : ResultKind::RCode;
        return;
    }

    m_resultKind = ResultKind::Invalid;
    m_errorKey = WorkshopKey::WORKSHOP_SMD_RESISTOR_ERR_R_CODE;
}

/**
 * @brief Prüft auf reinen Zifferncode.
 */
bool WorkshopTool_SmdResistor::isNumericCode() const
{
    for (const char *p = m_code; *p != '\0'; ++p)
        if (*p < '0' || *p > '9')
            return false;

    return true;
}

/**
 * @brief Prüft auf Null-Ohm-Code.
 */
bool WorkshopTool_SmdResistor::isZeroCode() const
{
    bool hasDigit = false;
    for (const char *p = m_code; *p != '\0'; ++p)
    {
        if (*p == 'R')
            continue;

        if (*p < '0' || *p > '9')
            return false;

        hasDigit = true;
        if (*p != '0')
            return false;
    }

    return hasDigit;
}

/**
 * @brief Dekodiert 3- und 4-stellige Zifferncodes.
 */
bool WorkshopTool_SmdResistor::decodeNumeric(size_t len, double &outOhm) const
{
    if (len != 3 && len != 4)
        return false;

    int mantissa = 0;
    const size_t mantissaLen = len - 1;
    for (size_t i = 0; i < mantissaLen; ++i)
        mantissa = mantissa * 10 + (m_code[i] - '0');

    const int exponent = m_code[len - 1] - '0';
    double factor = 1.0;
    for (int i = 0; i < exponent; ++i)
        factor *= 10.0;

    outOhm = static_cast<double>(mantissa) * factor;
    return mantissa > 0;
}

/**
 * @brief Dekodiert R-Notation.
 */
bool WorkshopTool_SmdResistor::decodeRCode(double &outOhm) const
{
    const size_t len = strlen(m_code);
    if (len < 2 || len > CODE_MAX)
        return false;

    int rPos = -1;
    int digits = 0;
    double value = 0.0;
    double frac = 0.1;
    bool afterR = false;

    for (size_t i = 0; i < len; ++i)
    {
        const char ch = m_code[i];
        if (ch == 'R')
        {
            if (rPos >= 0)
                return false;
            rPos = static_cast<int>(i);
            afterR = true;
            continue;
        }

        if (ch < '0' || ch > '9')
            return false;

        ++digits;
        if (afterR)
        {
            value += static_cast<double>(ch - '0') * frac;
            frac *= 0.1;
        }
        else
        {
            value = value * 10.0 + static_cast<double>(ch - '0');
        }
    }

    if (rPos < 0 || digits == 0)
        return false;

    outOhm = value;
    return true;
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet die spaltenbasierte Code-Auswahl.
 */
void WorkshopTool_SmdResistor::drawSelector()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;
    const int cols = activeCols();
    const int rows = activeRows();

    spr->fillRect(ox + m_selectorRect.x, oy + m_selectorRect.y, m_selectorRect.w, m_selectorRect.h, C_BG);

    spr->loadFont(ComfortaaBold16);
    for (int c = 0; c < cols; ++c)
        for (int r = 0; r < rows; ++r)
            drawCell(c, r, m_pressedCol == c && m_pressedRow == r);

    spr->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Auswahlzelle.
 */
void WorkshopTool_SmdResistor::drawCell(int col, int row, bool pressed)
{
    constexpr int BORDER_T = 2;

    if (!isRowValid(col, row))
        return;

    Gfx_Sprite *const spr = m_sprite;
    const Rect &r = m_cellRects[col][row];
    const int ax = m_originX + r.x;
    const int ay = m_originY + r.y;
    const bool selected = (m_selectedRows[col] == row);
    const char label[2] = {static_cast<char>((row == R_MARK_ROW) ? 'R' : ((row == BLANK_ROW) ? '-' : ('0' + row))), '\0'};

    const uint16_t border = selected ? C_ACCENT : C_CELL_BORDER;
    const uint16_t fill = pressed ? C_CELL_PRESS : (selected ? C_CELL_ACTIVE : C_CELL);
    const uint16_t text = selected ? C_TEXT_BRIGHT : C_TEXT;

    spr->fillSmoothRoundRect(ax, ay, r.w, r.h, CELL_R, border);
    spr->fillSmoothRoundRect(ax + BORDER_T, ay + BORDER_T, r.w - 2 * BORDER_T, r.h - 2 * BORDER_T, CELL_R - 1, fill);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(text);
    spr->drawString(label, ax + r.w / 2, ay + r.h / 2 + TEXT_OY - 1);
}

/**
 * @brief Zeichnet die rechte Bauteilvorschau.
 */
void WorkshopTool_SmdResistor::drawPreview()
{
    constexpr int CHIP_TOP = 12;
    constexpr int TABLE_GAP = 12;
    constexpr int TABLE_PAD_X = 12;

    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_previewRect.x;
    const int oy = m_originY + m_previewRect.y;

    spr->fillRect(ox, oy, m_previewRect.w, m_previewRect.h, C_BG);

    const int chipX = ox + (m_previewRect.w - CHIP_W) / 2;
    const int chipY = oy + CHIP_TOP;
    drawChip(chipX, chipY);

    const int tableX = ox + TABLE_PAD_X;
    const int tableY = chipY + CHIP_H + TABLE_GAP;
    const int tableW = m_previewRect.w - 2 * TABLE_PAD_X;

    drawPackageTable(tableX, tableY, tableW);
}

/**
 * @brief Zeichnet ein flaches SMD-Bauteil im Workshop-Stil.
 */
void WorkshopTool_SmdResistor::drawChip(int x, int y)
{
    constexpr int OUTER_Y = 8;
    constexpr int OUTER_H_TRIM = 16;
    constexpr int EDGE_INSET = 2;
    constexpr int PAD_SHADOW_Y = 10;
    constexpr int PAD_SHADOW_H_TRIM = 20;
    constexpr int PAD_FILL_Y = 13;
    constexpr int PAD_FILL_H_TRIM = 26;
    constexpr int PAD_OVERLAP_SHADOW = 10;
    constexpr int PAD_OVERLAP_FILL = 8;
    constexpr int BODY_EDGE_OX = 2;
    constexpr int BODY_EDGE_Y = 7;
    constexpr int BODY_EDGE_H_TRIM = 14;
    constexpr int BODY_Y = 10;
    constexpr int BODY_H_TRIM = 20;
    constexpr int BODY_LINE_X = 6;
    constexpr int BODY_SIDE_LINE_Y = 14;
    constexpr int BODY_SIDE_LINE_H_TRIM = 28;
    constexpr int BODY_TOP_LINE_Y = 15;
    constexpr int BODY_BOTTOM_LINE_DY = 17;
    constexpr int PAD_LIGHT_Y = 15;
    constexpr int PAD_DARK_DY = 16;
    constexpr int PAD_LINE_INSET_X = 6;
    constexpr int PAD_LINE_W_TRIM = 12;
    constexpr int PAD_JOIN_LINE_X = 1;
    constexpr uint16_t BODY_TOP_LINE_COLOR = 0x39E7;
    constexpr uint16_t BODY_BOTTOM_LINE_COLOR = 0x2104;
    constexpr uint16_t BODY_SIDE_LINE_COLOR = 0x18E3;

    Gfx_Sprite *const spr = m_sprite;
    const int bodyX = x + CHIP_PAD_W;
    const int bodyW = CHIP_W - 2 * CHIP_PAD_W;
    const int padInnerW = CHIP_PAD_W + 2 * EDGE_INSET;

    spr->fillSmoothRoundRect(x, y + OUTER_Y, CHIP_W, CHIP_H - OUTER_H_TRIM, CHIP_R, C_CHIP_EDGE);
    spr->fillSmoothRoundRect(x + EDGE_INSET, y + PAD_SHADOW_Y, CHIP_PAD_W + PAD_OVERLAP_FILL, CHIP_H - PAD_SHADOW_H_TRIM, CHIP_R - 1, C_CHIP_PAD_SHADOW);
    spr->fillSmoothRoundRect(x + CHIP_W - CHIP_PAD_W - PAD_OVERLAP_SHADOW, y + PAD_SHADOW_Y, CHIP_PAD_W + PAD_OVERLAP_FILL, CHIP_H - PAD_SHADOW_H_TRIM, CHIP_R - 1, C_CHIP_PAD_SHADOW);

    const int leftPadX = x + 2 * EDGE_INSET;
    const int rightPadX = x + CHIP_W - CHIP_PAD_W - PAD_OVERLAP_FILL;
    const int padY = y + PAD_FILL_Y;
    const int padH = CHIP_H - PAD_FILL_H_TRIM;
    spr->fillSmoothRoundRect(leftPadX, padY, padInnerW, padH, CHIP_R - 1, C_CHIP_PAD);
    spr->fillSmoothRoundRect(rightPadX, padY, padInnerW, padH, CHIP_R - 1, C_CHIP_PAD);
    spr->drawFastHLine(leftPadX + PAD_LINE_INSET_X, y + PAD_LIGHT_Y, padInnerW - PAD_LINE_W_TRIM, C_CHIP_PAD_LIGHT);
    spr->drawFastHLine(rightPadX + PAD_LINE_INSET_X, y + PAD_LIGHT_Y, padInnerW - PAD_LINE_W_TRIM, C_CHIP_PAD_LIGHT);
    spr->drawFastHLine(leftPadX + PAD_LINE_INSET_X, y + CHIP_H - PAD_DARK_DY, padInnerW - PAD_LINE_W_TRIM, C_CHIP_PAD_DARK);
    spr->drawFastHLine(rightPadX + PAD_LINE_INSET_X, y + CHIP_H - PAD_DARK_DY, padInnerW - PAD_LINE_W_TRIM, C_CHIP_PAD_DARK);
    spr->fillSmoothRoundRect(bodyX - BODY_EDGE_OX, y + BODY_EDGE_Y, bodyW + 2 * BODY_EDGE_OX, CHIP_H - BODY_EDGE_H_TRIM, CHIP_R, C_CHIP_EDGE);
    spr->fillSmoothRoundRect(bodyX, y + BODY_Y, bodyW, CHIP_H - BODY_H_TRIM, CHIP_R - 1, C_CHIP_BODY);

    spr->drawFastVLine(bodyX - PAD_JOIN_LINE_X, y + BODY_SIDE_LINE_Y, CHIP_H - BODY_SIDE_LINE_H_TRIM, C_CHIP_PAD_DARK);
    spr->drawFastVLine(bodyX + bodyW, y + BODY_SIDE_LINE_Y, CHIP_H - BODY_SIDE_LINE_H_TRIM, C_CHIP_PAD_DARK);
    spr->drawFastVLine(bodyX + 2, y + BODY_SIDE_LINE_Y, CHIP_H - BODY_SIDE_LINE_H_TRIM, BODY_SIDE_LINE_COLOR);
    spr->drawFastVLine(bodyX + bodyW - 3, y + BODY_SIDE_LINE_Y, CHIP_H - BODY_SIDE_LINE_H_TRIM, BODY_SIDE_LINE_COLOR);
    spr->drawFastHLine(bodyX + BODY_LINE_X, y + BODY_TOP_LINE_Y, bodyW - 2 * BODY_LINE_X, BODY_TOP_LINE_COLOR);
    spr->drawFastHLine(bodyX + BODY_LINE_X, y + CHIP_H - BODY_BOTTOM_LINE_DY, bodyW - 2 * BODY_LINE_X, BODY_BOTTOM_LINE_COLOR);

    spr->loadFont(ComfortaaBold30);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_TEXT_BRIGHT, C_CHIP_BODY);
    spr->drawString(m_code, bodyX + bodyW / 2, y + CHIP_H / 2 + 1);
    spr->unloadFont();
}

/**
 * @brief Zeichnet eine kompakte Referenz der SMD-Baugrößen.
 */
void WorkshopTool_SmdResistor::drawPackageTable(int x, int y, int w)
{
    constexpr int HEADER_H = 21;
    constexpr int ROW_H = 23;
    constexpr int PAD_X = 6;
    constexpr int LINE_Y_OFFSET = 1;
    constexpr uint16_t C_TABLE_LINE = 0x4208;

    Gfx_Sprite *const spr = m_sprite;
    const int rows = static_cast<int>(sizeof(SMD_PACKAGE_ROWS) / sizeof(SMD_PACKAGE_ROWS[0]));
    const int tableH = HEADER_H + rows * ROW_H;

    spr->fillRect(x, y, w, tableH, C_BG);
    spr->drawFastHLine(x, y + HEADER_H - LINE_Y_OFFSET, w, C_ACCENT);

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_TEXT_DIM);
    spr->drawString("SMD", x + PAD_X, y + HEADER_H / 2);

    spr->setTextDatum(MR_DATUM);
    spr->drawString("mm", x + w - PAD_X, y + HEADER_H / 2);

    for (int i = 0; i < rows; ++i)
    {
        const int rowY = y + HEADER_H + i * ROW_H;
        const int textY = rowY + ROW_H / 2;
        spr->drawFastHLine(x, rowY + ROW_H - LINE_Y_OFFSET, w, C_TABLE_LINE);

        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_TEXT_BRIGHT);
        spr->drawString(SMD_PACKAGE_ROWS[i].code, x + PAD_X, textY);

        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_TEXT);
        spr->drawString(SMD_PACKAGE_ROWS[i].mm, x + w - PAD_X, textY);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet die Ergebniszeile.
 */
void WorkshopTool_SmdResistor::drawResult()
{
    constexpr int CODE_LABEL_X = 14;
    constexpr int CODE_VALUE_X = 74;
    constexpr int VALUE_LABEL_X = 150;
    constexpr int VALUE_TEXT_X = 210;
    constexpr int RIGHT_PAD_X = 12;

    Gfx_Sprite *const spr = m_sprite;
    const int x = m_originX + m_resultRect.x;
    const int y = m_originY + m_resultRect.y;
    const bool ok = (m_resultKind != ResultKind::Invalid);

    spr->fillRect(x, y, m_resultRect.w, m_resultRect.h, C_PANEL_ALT);

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_TEXT_DIM, C_PANEL_ALT);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_SMD_RESISTOR_LABEL_CODE), x + CODE_LABEL_X, y + m_resultRect.h / 2);
    spr->setTextColor(C_TEXT_BRIGHT, C_PANEL_ALT);
    spr->drawString(m_code, x + CODE_VALUE_X, y + m_resultRect.h / 2);

    spr->setTextColor(C_TEXT_DIM, C_PANEL_ALT);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_SMD_RESISTOR_LABEL_VALUE), x + VALUE_LABEL_X, y + m_resultRect.h / 2);

    if (ok)
    {
        spr->setTextColor(C_TEXT_BRIGHT, C_PANEL_ALT);
        spr->drawString(m_valueStr, x + VALUE_TEXT_X, y + m_resultRect.h / 2);
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_ACCENT, C_PANEL_ALT);
        spr->drawString(I18n::str(resultTextKey()), x + m_resultRect.w - RIGHT_PAD_X, y + m_resultRect.h / 2);
    }
    else
    {
        spr->setTextColor(C_ERR, C_PANEL_ALT);
        spr->drawString(I18n::str(m_errorKey), x + VALUE_TEXT_X, y + m_resultRect.h / 2);
    }

    spr->unloadFont();
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf die Auswahlzellen.
 */
bool WorkshopTool_SmdResistor::hitTestCell(int cx, int cy, int &outCol, int &outRow) const
{
    const int cols = activeCols();
    const int rows = activeRows();

    for (int c = 0; c < cols; ++c)
    {
        for (int r = 0; r < rows; ++r)
        {
            if (!isRowValid(c, r))
                continue;

            const Rect &rect = m_cellRects[c][r];
            if (cx >= rect.x && cx < rect.x + rect.w && cy >= rect.y && cy < rect.y + rect.h)
            {
                outCol = c;
                outRow = r;
                return true;
            }
        }
    }

    outCol = -1;
    outRow = -1;
    return false;
}

/**
 * @brief Prüft, ob eine Zelle im aktuellen Modus auswählbar ist.
 */
bool WorkshopTool_SmdResistor::isRowValid(int col, int row) const
{
    if (col < 0 || col >= activeCols() || row < 0 || row >= activeRows())
        return false;

    if (m_mode != Mode::RCode && row >= DIGIT_ROWS)
        return false;

    return true;
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Formatiert einen Widerstandswert mit drei signifikanten Stellen.
 */
void WorkshopTool_SmdResistor::formatResistance(double ohm, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (ohm < 0.0 || !std::isfinite(ohm))
    {
        stringutils::safeCopy(buf, sz, "? Ohm");
        return;
    }

    double val = ohm;
    const char *unit = "Ohm";
    if (ohm >= 1.0e9)
    {
        val = ohm / 1.0e9;
        unit = "GOhm";
    }
    else if (ohm >= 1.0e6)
    {
        val = ohm / 1.0e6;
        unit = "MOhm";
    }
    else if (ohm >= 1000.0)
    {
        val = ohm / 1000.0;
        unit = "kOhm";
    }

    stringutils::safeFormat(buf, sz, "%.3g %s", val, unit);
    dotToComma(buf);
}

/**
 * @brief Ersetzt Dezimalpunkte durch Kommas.
 */
void WorkshopTool_SmdResistor::dotToComma(char *buf)
{
    if (!buf)
        return;

    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.')
            *p = ',';
}

/**
 * @brief Liefert den Hinweistext zum Ergebnis.
 */
WorkshopKey WorkshopTool_SmdResistor::resultTextKey() const
{
    switch (m_resultKind)
    {
    case ResultKind::Numeric3:
        return WorkshopKey::WORKSHOP_SMD_RESISTOR_NOTE_3_DIGIT;
    case ResultKind::Numeric4:
        return WorkshopKey::WORKSHOP_SMD_RESISTOR_NOTE_4_DIGIT;
    case ResultKind::RCode:
        return WorkshopKey::WORKSHOP_SMD_RESISTOR_NOTE_R_CODE;
    case ResultKind::Zero:
        return WorkshopKey::WORKSHOP_SMD_RESISTOR_NOTE_ZERO;
    case ResultKind::Invalid:
    default:
        return m_errorKey;
    }
}
