#include "WorkshopTool_SleeveTaper.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "WorkshopNumpad.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"

// ===================================================================
// Anonymer Namespace - file-local Helfer und Layout-Konstanten
// ===================================================================

namespace
{
    // Spaltenaufteilung (Split-Layout)
    constexpr int SCHEM_W = 250; ///< Breite der linken Spalte (Visualisierung)
    constexpr int COL_GAP = 12;  ///< Abstand zwischen den Spalten

    // Innenabstand für den Anweisungstext
    constexpr int INSTR_PADDING_X = 10;

    // Reservierte Größe der Textpuffer für Segment- und Hinweiszeilen
    constexpr int LINE_BUF_SIZE = 48;
} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

const WorkshopKey WorkshopTool_SleeveTaper::FIELD_LABEL_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_SLEEVE_TAPER_LABEL_FROM,
    WorkshopKey::WORKSHOP_SLEEVE_TAPER_LABEL_TO,
    WorkshopKey::WORKSHOP_SLEEVE_TAPER_LABEL_OVER
};

const WorkshopKey WorkshopTool_SleeveTaper::FIELD_UNIT_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_SLEEVE_TAPER_UNIT_STITCHES,
    WorkshopKey::WORKSHOP_SLEEVE_TAPER_UNIT_STITCHES,
    WorkshopKey::WORKSHOP_SLEEVE_TAPER_UNIT_ROWS
};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_SleeveTaper::WorkshopTool_SleeveTaper()
{
    clearAll();
}

/**
 * @brief Setzt das Werkzeug beim Modul-Einstieg zurück.
 */
void WorkshopTool_SleeveTaper::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte und füllt die Felder.
 */
void WorkshopTool_SleeveTaper::clearAll()
{
    memset(m_inputBuf, 0, sizeof(m_inputBuf));
    memset(m_values, 0, sizeof(m_values));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));

    m_sprite      = nullptr;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;
    m_layoutValid = false;
    m_numpad.clearLayout();

    m_pressedField = -1;
    m_pressedKey   = -1;
    m_touchWas     = false;
    m_touchReady   = false;

    m_focusedField = 0;
    m_direction    = Direction::NONE;
    m_status       = Status::EMPTY;
    m_diff         = 0;
    m_increaseRows = 0;
    m_baseInterval = 0;
    m_remainder    = 0;

    m_values[0] = 50;
    m_values[1] = 86;
    m_values[2] = 80;
    formatInt(m_values[0], m_inputBuf[0], FIELD_CAP);
    formatInt(m_values[1], m_inputBuf[1], FIELD_CAP);
    formatInt(m_values[2], m_inputBuf[2], FIELD_CAP);

    compute();
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Visualisierung, Eingabefelder und Numpad.
 */
void WorkshopTool_SleeveTaper::draw(Gfx_Sprite *sprite, int originX, int originY,
                                    int w, int h)
{
    m_sprite  = sprite;
    m_originX = originX;
    m_originY = originY;

    if (!m_layoutValid || m_w != w || m_h != h)
    {
        m_w = w;
        m_h = h;
        computeLayout(w, h);
        m_layoutValid = true;
    }

    const bool cursorOn = (millis() % CURSOR_PERIOD_MS) < CURSOR_ON_MS;

    drawVisualization();
    drawFields(cursorOn);
    m_numpad.draw(m_sprite, m_originX + m_rightX, m_originY, m_pressedKey);
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Spaltenaufteilung, Feld-Rects und Numpad-Layout.
 */
void WorkshopTool_SleeveTaper::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;

    if (m_rightW < 100)
        m_rightW = 100;

    int rowY = TOP_GAP;
    for (int i = 0; i < NUM_FIELDS; ++i)
    {
        m_fieldRects[i] = {m_rightX, rowY, m_rightW, ROW_H};
        rowY += ROW_H;
    }

    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Events auf Eingabefeldern und Numpad.
 */
bool WorkshopTool_SleeveTaper::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        if (cx >= m_rightX)
        {
            const int hitField = hitTestField(cx, cy);
            if (hitField >= 0)
            {
                if (hitField != m_pressedField)
                {
                    m_pressedField = hitField;
                    m_pressedKey   = -1;
                    needsRedraw    = true;
                }
            }
            else
            {
                const int hitKey = m_numpad.hitTest(cx - m_rightX, cy);
                if (hitKey >= 0 && hitKey != m_pressedKey)
                {
                    m_pressedKey   = hitKey;
                    m_pressedField = -1;
                    needsRedraw    = true;
                }
            }
        }

        m_touchWas = true;
    }
    else
    {
        m_touchReady = true;

        if (m_touchWas)
        {
            if (m_pressedField >= 0 && m_pressedField < NUM_FIELDS)
            {
                m_focusedField = static_cast<uint8_t>(m_pressedField);
                needsRedraw    = true;
            }
            else if (m_pressedKey >= 0 && m_pressedKey < WorkshopNumpad::NUM_KEYS)
            {
                executeKey(WorkshopNumpad::keyAction(m_pressedKey));
                needsRedraw = true;
            }
        }

        m_pressedField = -1;
        m_pressedKey   = -1;
        m_touchWas     = false;
    }

    return needsRedraw;
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Liest alle Eingabepuffer und füllt Status, Diff, Y, BASE und REM.
 */
void WorkshopTool_SleeveTaper::compute()
{
    for (int i = 0; i < NUM_FIELDS; ++i)
        m_values[i] = (m_inputBuf[i][0] != '\0') ? atoi(m_inputBuf[i]) : 0;

    const int n = m_values[0];
    const int m = m_values[1];
    const int r = m_values[2];

    if (n <= 0 || m <= 0 || r <= 0)
    {
        m_direction    = Direction::NONE;
        m_status       = Status::EMPTY;
        m_diff         = 0;
        m_increaseRows = 0;
        m_baseInterval = 0;
        m_remainder    = 0;
        return;
    }

    if (n == m)
    {
        m_direction    = Direction::NONE;
        m_status       = Status::NO_CHANGE;
        m_diff         = 0;
        m_increaseRows = 0;
        m_baseInterval = 0;
        m_remainder    = 0;
        return;
    }

    const bool isIncrease = (m > n);
    m_direction           = isIncrease ? Direction::INCREASE : Direction::DECREASE;
    m_diff                = isIncrease ? (m - n) : (n - m);

    // Beidseitige Anpassung verlangt eine gerade Differenz, sonst bliebe eine Masche übrig.
    if ((m_diff & 1) != 0)
    {
        m_status       = Status::ODD;
        m_increaseRows = 0;
        m_baseInterval = 0;
        m_remainder    = 0;
        return;
    }

    m_increaseRows = m_diff / 2;

    // Y > R hieße: mehr als eine Anpassung pro Reihe, das geht nicht beidseitig in einem Durchgang.
    if (m_increaseRows > r)
    {
        m_status       = Status::INFEASIBLE;
        m_baseInterval = 0;
        m_remainder    = 0;
        return;
    }

    m_baseInterval = r / m_increaseRows;
    m_remainder    = r % m_increaseRows;
    m_status       = Status::OK;
}

/**
 * @brief Verarbeitet ein Aktionszeichen vom Numpad auf dem fokussierten Feld.
 */
void WorkshopTool_SleeveTaper::executeKey(char action)
{
    char *const  buf = m_inputBuf[m_focusedField];
    const size_t len = strlen(buf);

    // Reihen-Feld hat eine andere Obergrenze als die Maschen-Felder
    const int maxValue = (m_focusedField == 2) ? MAX_ROW_COUNT : MAX_STITCH_COUNT;

    switch (action)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
        if (len + 1 >= FIELD_CAP)
            break;

        // Führende Null wird durch die erste eingegebene Ziffer ersetzt
        if (len == 1 && buf[0] == '0')
        {
            buf[0] = action;
            break;
        }

        char candidate[FIELD_CAP];
        for (size_t i = 0; i < len; ++i)
            candidate[i] = buf[i];
        candidate[len]     = action;
        candidate[len + 1] = '\0';

        const int v = atoi(candidate);
        if (v > maxValue)
            break;

        buf[len]     = action;
        buf[len + 1] = '\0';
        break;
    }
    case 'B':
    {
        if (len > 0)
            buf[len - 1] = '\0';
        break;
    }
    case 'C':
    {
        buf[0] = '\0';
        break;
    }
    default:
        // Tasten '.' und 'S' sind in diesem Werkzeug ohne Wirkung
        break;
    }

    compute();
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Liefert die Akzentfarbe der aktuell ermittelten Richtung.
 */
uint16_t WorkshopTool_SleeveTaper::accentColor() const
{
    switch (m_direction)
    {
    case Direction::INCREASE: return C_ACCENT_INC;
    case Direction::DECREASE: return C_ACCENT_DEC;
    default:                  return C_ACCENT_NONE;
    }
}

/**
 * @brief Liefert die halbe Trapez-Breite unten je nach Richtung.
 */
int WorkshopTool_SleeveTaper::trapHalfWidthBottom() const
{
    if (m_direction == Direction::DECREASE)
        return VIS_TRAP_HALF_WIDE;
    if (m_direction == Direction::INCREASE)
        return VIS_TRAP_HALF_NARROW;
    return VIS_TRAP_HALF_NARROW;
}

/**
 * @brief Liefert die halbe Trapez-Breite oben je nach Richtung.
 */
int WorkshopTool_SleeveTaper::trapHalfWidthTop() const
{
    if (m_direction == Direction::DECREASE)
        return VIS_TRAP_HALF_NARROW;
    if (m_direction == Direction::INCREASE)
        return VIS_TRAP_HALF_WIDE;
    return VIS_TRAP_HALF_NARROW;
}

/**
 * @brief Formatiert eine ganze Zahl als reine Ziffernfolge.
 */
void WorkshopTool_SleeveTaper::formatInt(int v, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (v < 0)
        v = 0;

    stringutils::safeFormat(buf, sz, "%d", v);
}

// ===================================================================
// Treffertest
// ===================================================================

/**
 * @brief Treffertest auf die Eingabefelder, content-relativ.
 */
int WorkshopTool_SleeveTaper::hitTestField(int cx, int cy) const
{
    for (int i = 0; i < NUM_FIELDS; ++i)
    {
        const Rect &r = m_fieldRects[i];
        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h)
            return i;
    }

    return -1;
}

// ===================================================================
// Zeichnen - Visualisierung
// ===================================================================

/**
 * @brief Zeichnet Hintergrund, Titelzeile, Trapez, Tick-Paare und Anweisungsblock.
 */
void WorkshopTool_SleeveTaper::drawVisualization()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;

    spr->fillRect(ox, oy, SCHEM_W, m_h, C_BG);

    // Titelzeile oben
    WorkshopKey titleKey = WorkshopKey::WORKSHOP_SLEEVE_TAPER_TITLE_SLEEVE;
    switch (m_status)
    {
    case Status::EMPTY:      titleKey = WorkshopKey::WORKSHOP_SLEEVE_TAPER_TITLE_SLEEVE;   break;
    case Status::NO_CHANGE:  titleKey = WorkshopKey::WORKSHOP_SLEEVE_TAPER_TITLE_NO_TAPER; break;
    case Status::OK:
    case Status::ODD:
    case Status::INFEASIBLE: titleKey = (m_direction == Direction::INCREASE)
                                            ? WorkshopKey::WORKSHOP_SLEEVE_TAPER_TITLE_INCREASE
                                            : WorkshopKey::WORKSHOP_SLEEVE_TAPER_TITLE_DECREASE;
                             break;
    }

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(TC_DATUM);
    spr->setTextColor(C_TITLE_TEXT, C_BG);
    spr->drawString(I18n::str(titleKey), ox + SCHEM_W / 2, oy + VIS_TITLE_TOP_Y);
    spr->unloadFont();

    drawTrapezoid();

    // Differenzzahl im Trapez-Zentrum
    const int trapCenterY = oy + (VIS_TRAP_TOP_Y + VIS_TRAP_BOTTOM_Y) / 2;
    const int trapCenterX = ox + SCHEM_W / 2;

    if (m_status == Status::OK || m_status == Status::ODD || m_status == Status::INFEASIBLE)
    {
        char numBuf[12];
        const char prefix = (m_direction == Direction::INCREASE) ? '+' : '-';
        stringutils::safeFormat(numBuf, sizeof(numBuf), "%c%d", prefix, m_diff);

        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(accentColor(), C_BG);
        spr->drawString(numBuf, trapCenterX, trapCenterY);
        spr->unloadFont();
    }
    else if (m_status == Status::NO_CHANGE)
    {
        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_HINT_TEXT, C_BG);
        spr->drawString("0", trapCenterX, trapCenterY);
        spr->unloadFont();
    }

    // Tick-Paare an den Anpassungsreihen-Höhen
    if (m_status == Status::OK && m_increaseRows > 0 && m_values[2] > 0)
    {
        const int      rows = m_values[2];
        const int      base = m_baseInterval;
        const int      rem  = m_remainder;
        const uint16_t clr  = accentColor();
        const int      yBot = oy + VIS_TRAP_BOTTOM_Y;
        const int      yTop = oy + VIS_TRAP_TOP_Y;
        const int      span = yBot - yTop;

        int cumulative = 0;
        for (int i = 0; i < m_increaseRows; ++i)
        {
            const int interval = (i < rem) ? (base + 1) : base;
            cumulative += interval;

            const float frac    = static_cast<float>(cumulative) / static_cast<float>(rows);
            const int   yTickPx = yBot - static_cast<int>(lround(span * frac));
            drawTickPair(yTickPx, clr);
        }
    }

    // Anweisungs- oder Hinweistextblock unter dem Trapez
    drawInstructionBlock(ox + INSTR_PADDING_X,
                         oy + VIS_INSTR_TOP_Y,
                         SCHEM_W - 2 * INSTR_PADDING_X);
}

/**
 * @brief Zeichnet die vier Kanten des Trapezes als geglättete breite Linien.
 */
void WorkshopTool_SleeveTaper::drawTrapezoid()
{
    Gfx_Sprite *const spr = m_sprite;
    const int cx     = m_originX + SCHEM_W / 2;
    const int yBot   = m_originY + VIS_TRAP_BOTTOM_Y;
    const int yTop   = m_originY + VIS_TRAP_TOP_Y;
    const int hwBot  = trapHalfWidthBottom();
    const int hwTop  = trapHalfWidthTop();

    const float xBL = static_cast<float>(cx - hwBot);
    const float xBR = static_cast<float>(cx + hwBot);
    const float xTL = static_cast<float>(cx - hwTop);
    const float xTR = static_cast<float>(cx + hwTop);
    const float yB  = static_cast<float>(yBot);
    const float yT  = static_cast<float>(yTop);

    spr->drawWideLine(xBL, yB, xBR, yB, VIS_TRAP_OUTLINE_W, C_TRAP_OUTLINE);
    spr->drawWideLine(xTL, yT, xTR, yT, VIS_TRAP_OUTLINE_W, C_TRAP_OUTLINE);
    spr->drawWideLine(xBL, yB, xTL, yT, VIS_TRAP_OUTLINE_W, C_TRAP_OUTLINE);
    spr->drawWideLine(xBR, yB, xTR, yT, VIS_TRAP_OUTLINE_W, C_TRAP_OUTLINE);
}

/**
 * @brief Zeichnet ein Tick-Paar links und rechts der Trapez-Kante auf einer Y-Höhe.
 */
void WorkshopTool_SleeveTaper::drawTickPair(int yPosAbsolute, uint16_t color)
{
    Gfx_Sprite *const spr = m_sprite;
    const int yBot  = m_originY + VIS_TRAP_BOTTOM_Y;
    const int yTop  = m_originY + VIS_TRAP_TOP_Y;
    const int span  = yBot - yTop;
    if (span <= 0)
        return;

    const float t       = static_cast<float>(yBot - yPosAbsolute) / static_cast<float>(span);
    const float hwBot   = static_cast<float>(trapHalfWidthBottom());
    const float hwTop   = static_cast<float>(trapHalfWidthTop());
    const float halfWid = hwBot * (1.0f - t) + hwTop * t;

    const int cx        = m_originX + SCHEM_W / 2;
    const int leftEdge  = cx - static_cast<int>(lround(halfWid));
    const int rightEdge = cx + static_cast<int>(lround(halfWid));

    const int leftEnd    = leftEdge - VIS_TICK_GAP;
    const int leftStart  = leftEnd - VIS_TICK_LENGTH;
    const int rightStart = rightEdge + VIS_TICK_GAP;
    const int rightEnd   = rightStart + VIS_TICK_LENGTH;

    spr->drawWideLine(static_cast<float>(leftStart),  static_cast<float>(yPosAbsolute),
                      static_cast<float>(leftEnd),    static_cast<float>(yPosAbsolute),
                      VIS_TICK_WIDTH, color);
    spr->drawWideLine(static_cast<float>(rightStart), static_cast<float>(yPosAbsolute),
                      static_cast<float>(rightEnd),   static_cast<float>(yPosAbsolute),
                      VIS_TICK_WIDTH, color);
}

/**
 * @brief Zeichnet den mehrzeiligen Anweisungs- oder Hinweistext.
 */
void WorkshopTool_SleeveTaper::drawInstructionBlock(int textX, int textTop, int textW)
{
    Gfx_Sprite *const spr = m_sprite;
    const int textCenterX = textX + textW / 2;

    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(TC_DATUM);

    auto drawLine = [&](auto line, int row, uint16_t color)
    {
        spr->setTextColor(color, C_BG);
        spr->drawString(line, textCenterX, textTop + row * VIS_INSTR_LINE_H);
    };

    switch (m_status)
    {
    case Status::EMPTY:
        drawLine(I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_EMPTY_1), 0, C_HINT_TEXT);
        drawLine(I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_EMPTY_2), 1, C_HINT_TEXT);
        break;

    case Status::NO_CHANGE:
        drawLine(I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_NO_CHANGE_1), 0, C_HINT_TEXT);
        drawLine(I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_NO_CHANGE_2), 1, C_HINT_TEXT);
        break;

    case Status::ODD:
        drawLine(I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_ODD_1), 0, C_HINT_TEXT);
        drawLine(I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_ODD_2), 1, C_HINT_TEXT);
        drawLine(I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_ODD_3), 2, C_HINT_TEXT);
        break;

    case Status::INFEASIBLE:
    {
        const __FlashStringHelper *line0 = (m_direction == Direction::INCREASE)
                                               ? I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_TOO_MANY_INC)
                                               : I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_TOO_MANY_DEC);
        drawLine(line0,                                                              0, C_HINT_TEXT);
        drawLine(I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_HINT_TOO_MANY_2), 1, C_HINT_TEXT);
        break;
    }

    case Status::OK:
    {
        const int rem  = m_remainder;
        const int base = m_baseInterval;
        const int y    = m_increaseRows;

        const __FlashStringHelper *action = (m_direction == Direction::INCREASE)
                                                ? I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_ACTION_INC_BOTH)
                                                : I18n::str(WorkshopKey::WORKSHOP_SLEEVE_TAPER_ACTION_DEC_BOTH);

        char buf[LINE_BUF_SIZE];

        if (rem == 0)
        {
            I18n::format(buf, sizeof(buf), WorkshopKey::WORKSHOP_SLEEVE_TAPER_FMT_ROWS, y, base);
            drawLine(buf, 0, C_INSTR_TEXT);
            drawLine(action, 1, C_INSTR_TEXT);
        }
        else
        {
            I18n::format(buf, sizeof(buf), WorkshopKey::WORKSHOP_SLEEVE_TAPER_FMT_ROWS_AND, rem, base + 1);
            drawLine(buf, 0, C_INSTR_TEXT);

            I18n::format(buf, sizeof(buf), WorkshopKey::WORKSHOP_SLEEVE_TAPER_FMT_ROWS, y - rem, base);
            drawLine(buf, 1, C_INSTR_TEXT);

            drawLine(action, 2, C_INSTR_TEXT);
        }
        break;
    }
    }

    spr->unloadFont();
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle Eingabefelder.
 */
void WorkshopTool_SleeveTaper::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);

    for (int i = 0; i < NUM_FIELDS; ++i)
        drawField(i, m_pressedField == i, cursorOn);

    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Feldzeile (Symbol, Wert, Einheit).
 */
void WorkshopTool_SleeveTaper::drawField(int idx, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const Rect &fr = m_fieldRects[idx];
    const int ax = m_originX + fr.x;
    const int ay = m_originY + fr.y;

    const bool focused = (static_cast<int>(m_focusedField) == idx);

    uint16_t bgColor;
    if (pressed)
        bgColor = C_FIELD_PRESS;
    else if (focused)
        bgColor = C_FIELD_FOCUS;
    else
        bgColor = C_FIELD_INPUT;

    spr->fillRect(ax, ay, fr.w, fr.h, bgColor);
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, accentColor());

    uint16_t lblClr  = focused ? C_LBL_FOCUS  : C_LBL_INPUT;
    uint16_t unitClr = focused ? C_UNIT_FOCUS : C_UNIT_INPUT;

    const int cy = ay + fr.h / 2;

    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblClr, bgColor);
    spr->drawString(I18n::str(FIELD_LABEL_KEYS[idx]), ax + ACCENT_W + 6, cy);

    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(unitClr, bgColor);
    spr->drawString(I18n::str(FIELD_UNIT_KEYS[idx]), ax + fr.w - UNIT_W / 2, cy);

    char displayBuf[FIELD_CAP + 1];
    const int valRightX = ax + fr.w - UNIT_W - 6;
    spr->setTextDatum(MR_DATUM);

    if (focused)
    {
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), m_inputBuf[idx]);
        stringutils::safeAppend(displayBuf, sizeof(displayBuf), "_");
        spr->setTextColor(C_VAL_FOCUS, bgColor);
        spr->drawString(displayBuf, valRightX, cy);

        if (!cursorOn)
        {
            const int cursorW = spr->textWidth("_");
            spr->fillRect(valRightX - cursorW, ay + 1, cursorW, fr.h - 2, bgColor);
        }
    }
    else
    {
        if (m_inputBuf[idx][0] != '\0')
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), m_inputBuf[idx]);
        else
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");

        spr->setTextColor(C_VAL_INPUT, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}
