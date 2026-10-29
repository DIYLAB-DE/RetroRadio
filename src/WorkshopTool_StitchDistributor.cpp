#include "WorkshopTool_StitchDistributor.h"

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

const WorkshopKey WorkshopTool_StitchDistributor::FIELD_LABEL_KEYS[NUM_FIELDS] =
    {WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_LABEL_FROM,
     WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_LABEL_TO};

const WorkshopKey WorkshopTool_StitchDistributor::FIELD_UNIT_KEYS[NUM_FIELDS] =
    {WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_UNIT_STITCHES,
     WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_UNIT_STITCHES};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_StitchDistributor::WorkshopTool_StitchDistributor()
{
    clearAll();
}

/**
 * @brief Setzt den Verteiler beim Modul-Einstieg zurück.
 */
void WorkshopTool_StitchDistributor::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte und füllt die Felder.
 */
void WorkshopTool_StitchDistributor::clearAll()
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
    m_baseSpacing  = 0;
    m_remainder    = 0;

    m_values[0] = 84;
    m_values[1] = 96;
    formatInt(m_values[0], m_inputBuf[0], FIELD_CAP);
    formatInt(m_values[1], m_inputBuf[1], FIELD_CAP);

    compute();
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Visualisierung, Eingabefelder und Numpad.
 */
void WorkshopTool_StitchDistributor::draw(Gfx_Sprite *sprite, int originX, int originY,
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
void WorkshopTool_StitchDistributor::computeLayout(int w, int h)
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
bool WorkshopTool_StitchDistributor::handleTouch(int32_t tx, int32_t ty, bool touched)
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
 * @brief Liest beide Eingabepuffer und füllt Status/Diff/BASE/REM.
 */
void WorkshopTool_StitchDistributor::compute()
{
    for (int i = 0; i < NUM_FIELDS; ++i)
        m_values[i] = (m_inputBuf[i][0] != '\0') ? atoi(m_inputBuf[i]) : 0;

    const int n = m_values[0];
    const int m = m_values[1];

    if (n <= 0 || m <= 0)
    {
        m_direction   = Direction::NONE;
        m_status      = Status::EMPTY;
        m_diff        = 0;
        m_baseSpacing = 0;
        m_remainder   = 0;
        return;
    }

    if (n == m)
    {
        m_direction   = Direction::NONE;
        m_status      = Status::NO_CHANGE;
        m_diff        = 0;
        m_baseSpacing = 0;
        m_remainder   = 0;
        return;
    }

    const bool isIncrease = (m > n);
    m_direction           = isIncrease ? Direction::INCREASE : Direction::DECREASE;
    m_diff                = isIncrease ? (m - n) : (n - m);

    // Beide Richtungen verteilen die D Aktionen über die Ausgangsrunde von N Maschen.
    m_baseSpacing = n / m_diff;
    m_remainder   = n % m_diff;

    // Eine Abnahme braucht mindestens zwei Maschen pro Segment, damit ein "2 Maschen zusammen" überhaupt hineinpasst.
    if (!isIncrease && m_baseSpacing < 2)
    {
        m_status = Status::INFEASIBLE;
        return;
    }

    // Eine Zunahme braucht mindestens eine Masche pro Segment vor der neuen Masche.
    if (isIncrease && m_baseSpacing < 1)
    {
        m_status = Status::INFEASIBLE;
        return;
    }

    m_status = Status::OK;
}

/**
 * @brief Verarbeitet ein Aktionszeichen vom Numpad auf dem fokussierten Feld.
 */
void WorkshopTool_StitchDistributor::executeKey(char action)
{
    char *const  buf = m_inputBuf[m_focusedField];
    const size_t len = strlen(buf);

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
        if (v > MAX_STITCH_COUNT)
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
uint16_t WorkshopTool_StitchDistributor::accentColor() const
{
    switch (m_direction)
    {
    case Direction::INCREASE: return C_ACCENT_INC;
    case Direction::DECREASE: return C_ACCENT_DEC;
    default:                  return C_ACCENT_NONE;
    }
}

/**
 * @brief Formatiert eine ganze Zahl als reine Ziffernfolge.
 */
void WorkshopTool_StitchDistributor::formatInt(int v, char *buf, size_t sz)
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
int WorkshopTool_StitchDistributor::hitTestField(int cx, int cy) const
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
 * @brief Zeichnet Hintergrund, Titelzeile, Ring und Anweisungsblock.
 */
void WorkshopTool_StitchDistributor::drawVisualization()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;
    const int colH = m_h;

    spr->fillRect(ox, oy, SCHEM_W, colH, C_BG);

    // Titelzeile oben
    WorkshopKey titleKey = WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_TITLE_DISTRIBUTION;
    switch (m_status)
    {
    case Status::EMPTY:      titleKey = WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_TITLE_DISTRIBUTION; break;
    case Status::NO_CHANGE:  titleKey = WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_TITLE_NO_CHANGE;    break;
    case Status::OK:
    case Status::INFEASIBLE: titleKey = (m_direction == Direction::INCREASE)
                                            ? WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_TITLE_INCREASE
                                            : WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_TITLE_DECREASE;
                             break;
    }

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(TC_DATUM);
    spr->setTextColor(C_TITLE_TEXT, C_BG);
    spr->drawString(I18n::str(titleKey), ox + SCHEM_W / 2, oy + VIS_TITLE_TOP_Y);
    spr->unloadFont();

    drawRing();

    // Anweisungs- oder Hinweistextblock unter dem Ring
    drawInstructionBlock(ox + INSTR_PADDING_X,
                         oy + VIS_INSTR_TOP_Y,
                         SCHEM_W - 2 * INSTR_PADDING_X);
}

/**
 * @brief Zeichnet die Ringkontur, alle Tick-Marker und die Mittelzahl.
 */
void WorkshopTool_StitchDistributor::drawRing()
{
    Gfx_Sprite *const spr = m_sprite;
    const int rcx = m_originX + SCHEM_W / 2;
    const int rcy = m_originY + VIS_RING_CENTER_Y;

    // Ringkontur als geglätteter Kreis
    spr->drawSmoothCircle(rcx, rcy,
                          static_cast<float>(VIS_RING_RADIUS),
                          static_cast<float>(VIS_RING_THICKNESS),
                          C_RING_OUTLINE, C_BG);

    // Tick-Marker nur bei gültiger Verteilung
    if (m_status == Status::OK && m_diff > 0 && m_values[0] > 0)
    {
        const int n        = m_values[0];
        const int d        = m_diff;
        const int base     = m_baseSpacing;
        const int rem      = m_remainder;
        const uint16_t clr = accentColor();

        int cumulative = 0;
        for (int i = 0; i < d; ++i)
        {
            const int segLen = (i < rem) ? (base + 1) : base;
            cumulative += segLen;

            const float frac  = static_cast<float>(cumulative) / static_cast<float>(n);
            const float angle = (-90.0f + frac * 360.0f) * PI / 180.0f;
            drawTick(rcx, rcy, angle, clr);
        }
    }

    // Differenzzahl im Zentrum
    if (m_status == Status::OK || m_status == Status::INFEASIBLE)
    {
        char numBuf[12];
        const char prefix = (m_direction == Direction::INCREASE) ? '+' : '-';
        stringutils::safeFormat(numBuf, sizeof(numBuf), "%c%d", prefix, m_diff);

        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(accentColor(), C_BG);
        spr->drawString(numBuf, rcx, rcy);
        spr->unloadFont();
    }
    else if (m_status == Status::NO_CHANGE)
    {
        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_HINT_TEXT, C_BG);
        spr->drawString("0", rcx, rcy);
        spr->unloadFont();
    }
}

/**
 * @brief Zeichnet einen radialen Tick-Marker als geglättete breite Linie.
 */
void WorkshopTool_StitchDistributor::drawTick(int cx, int cy, double angleRad, uint16_t color)
{
    Gfx_Sprite *const spr = m_sprite;

    const float radialX = static_cast<float>(cos(angleRad));
    const float radialY = static_cast<float>(sin(angleRad));

    const float x1 = cx + VIS_TICK_INNER * radialX;
    const float y1 = cy + VIS_TICK_INNER * radialY;
    const float x2 = cx + VIS_TICK_OUTER * radialX;
    const float y2 = cy + VIS_TICK_OUTER * radialY;

    spr->drawWideLine(x1, y1, x2, y2, VIS_TICK_WIDTH, color);
}

/**
 * @brief Zeichnet den mehrzeiligen Anweisungs- oder Hinweistext.
 */
void WorkshopTool_StitchDistributor::drawInstructionBlock(int textX, int textTop, int textW)
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

    auto drawSegment = [&](int row, int count, int plainBefore, Direction dir)
    {
        char head[LINE_BUF_SIZE];
        if (plainBefore > 0)
            I18n::format(head, sizeof(head),
                         WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_FMT_SEG_WITH_PLAIN,
                         count, plainBefore);
        else
            I18n::format(head, sizeof(head),
                         WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_FMT_SEG_NO_PLAIN,
                         count);
        drawLine(head, row, C_INSTR_TEXT);

        const __FlashStringHelper *action;
        if (plainBefore > 0)
            action = (dir == Direction::INCREASE)
                         ? I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_ACTION_INC_WITH_PLAIN)
                         : I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_ACTION_DEC_WITH_PLAIN);
        else
            action = (dir == Direction::INCREASE)
                         ? I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_ACTION_INC_NO_PLAIN)
                         : I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_ACTION_DEC_NO_PLAIN);
        drawLine(action, row + 1, C_INSTR_TEXT);
    };

    switch (m_status)
    {
    case Status::EMPTY:
        drawLine(I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_HINT_EMPTY_1), 0, C_HINT_TEXT);
        drawLine(I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_HINT_EMPTY_2), 1, C_HINT_TEXT);
        break;

    case Status::NO_CHANGE:
        drawLine(I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_1), 0, C_HINT_TEXT);
        drawLine(I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_2), 1, C_HINT_TEXT);
        break;

    case Status::INFEASIBLE:
        drawLine(I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_1), 0, C_HINT_TEXT);
        drawLine(I18n::str(WorkshopKey::WORKSHOP_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_2), 1, C_HINT_TEXT);
        break;

    case Status::OK:
    {
        const int rem  = m_remainder;
        const int base = m_baseSpacing;
        const int d    = m_diff;

        if (rem == 0)
        {
            const int plainBefore = (m_direction == Direction::INCREASE) ? base : (base - 2);
            drawSegment(0, d, plainBefore, m_direction);
        }
        else
        {
            const int plainLong  = (m_direction == Direction::INCREASE) ? (base + 1) : (base - 1);
            const int plainShort = (m_direction == Direction::INCREASE) ? base       : (base - 2);

            drawSegment(0, rem,     plainLong,  m_direction);
            drawSegment(2, d - rem, plainShort, m_direction);
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
 * @brief Zeichnet beide Eingabefelder.
 */
void WorkshopTool_StitchDistributor::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);

    for (int i = 0; i < NUM_FIELDS; ++i)
        drawField(i, m_pressedField == i, cursorOn);

    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Feldzeile (Symbol, Wert, Einheit).
 */
void WorkshopTool_StitchDistributor::drawField(int idx, bool pressed, bool cursorOn)
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
