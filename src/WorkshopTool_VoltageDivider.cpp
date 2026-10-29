#include "WorkshopTool_VoltageDivider.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "StringUtils.h"
#include "I18n.h"
#include "Workshop.h"
#include "WorkshopNumpad.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"

// ===================================================================
// Anonymer Namespace
// ===================================================================

namespace {

// Header-Abstand (Workshop-Tool-Konvention)
constexpr int TOP_GAP = 10;

/**
 * @brief Ersetzt alle Dezimalpunkte durch Kommas im übergebenen Puffer.
 */
void dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.') *p = ',';
}

// Spaltenaufteilung
constexpr int SCHEM_W = 250;
constexpr int COL_GAP = 12;

// Schaltplan-Geometrie (relativ zu oy, oy enthält bereits TOP_GAP)
constexpr int CHAIN_CX    = 85;    ///< X-Mitte der R1/R2-Kette
constexpr int UIN_RAIL_Y  = 25;    ///< Uin-Schiene
constexpr int COMP_W      = 24;    ///< Bauteil-Breite
constexpr int COMP_H      = 48;    ///< Bauteil-Höhe
constexpr int R1_TOP      = 50;
constexpr int R1_BOT      = R1_TOP + COMP_H;  // 98
constexpr int JUNCT_Y     = 118;   ///< Knotenpunkt (Uout-Abgriff)
constexpr int R2_TOP      = 138;
constexpr int R2_BOT      = R2_TOP + COMP_H;  // 186
constexpr int GND_RAIL_Y  = 210;

constexpr int UOUT_WIRE_END = 200; ///< Rechtes Ende der Uout-Leitung
constexpr int HIT_PAD = 8;

// Spannungspfeile
constexpr int UIN_ARROW_X  = 28;   ///< X-Position Uin-Pfeil (links)
constexpr int UOUT_ARROW_X = 215;  ///< X-Position Uout-Pfeil (rechts)

// Vertikaler Abstand der Pfeilspitzen zu den Schienen-Labels
constexpr int ARROW_TIP_OFFSET  = 14;  ///< Spitze: Abstand zur Schiene
constexpr int ARROW_BASE_OFFSET = 20;  ///< Pfeilbasis: Abstand zur Schiene
constexpr int ARROW_LINE_OFFSET = 16;  ///< Vertikale Linie: Abstand zur Schiene

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

const char * const WorkshopTool_VoltageDivider::QTY_SYMBOLS[NUM_QTY] =
    {"Uin", "R1", "R2", "Uout"};

const char * const WorkshopTool_VoltageDivider::QTY_UNITS[NUM_QTY] =
    {"V", "Ohm", "Ohm", "V"};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Initialisiert den Spannungsteiler mit Standardwerten.
 */
WorkshopTool_VoltageDivider::WorkshopTool_VoltageDivider() { clearAll(); }

/**
 * @brief Setzt den Spannungsteiler beim Aktivieren zurück.
 */
void WorkshopTool_VoltageDivider::onEnter(LGFX * /*tft*/) { clearAll(); }

/**
 * @brief Setzt alle Eingaben, Slots und UI-Zustände auf die Standardwerte zurück.
 */
void WorkshopTool_VoltageDivider::clearAll()
{
    memset(m_inputBuf,   0, sizeof(m_inputBuf));
    memset(m_values,     0, sizeof(m_values));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));
    memset(m_inputSlots, 0, sizeof(m_inputSlots));
    memset(m_compHits,   0, sizeof(m_compHits));

    m_sprite = nullptr;
    m_originX = m_originY = m_w = m_h = 0;
    m_layoutValid = false;
    m_computeOk   = false;
    m_numpad.clearLayout();

    m_pressedField = m_pressedComp = m_pressedKey = -1;
    m_touchWas = m_touchReady = false;

    m_inputSlots[0] = 0; // Uin (ältester)
    m_inputSlots[1] = 1; // R1
    m_inputSlots[2] = 2; // R2 (fokussiert)
    m_computedQty   = 3; // Uout berechnet
    m_focusedQty    = 2;

    m_values[0] = 12.0;    m_values[1] = 10000.0;  m_values[2] = 10000.0;
    formatEditValue(m_values[0], m_inputBuf[0], FIELD_CAP);
    formatEditValue(m_values[1], m_inputBuf[1], FIELD_CAP);
    formatEditValue(m_values[2], m_inputBuf[2], FIELD_CAP);
    compute();
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet den kompletten Tool-Inhalt (Schaltplan, Felder, Formel, Numpad).
 */
void WorkshopTool_VoltageDivider::draw(Gfx_Sprite *sprite,
                                        int originX, int originY,
                                        int w, int h)
{
    m_sprite = sprite;  m_originX = originX;  m_originY = originY;

    if (!m_layoutValid || m_w != w || m_h != h)
    {
        m_w = w;  m_h = h;
        computeLayout(w, h);
        m_layoutValid = true;
    }

    const bool cursorOn = (millis() % CURSOR_PERIOD_MS) < CURSOR_ON_MS;
    drawSchematic();
    drawFields(cursorOn);
    drawFormulaRow();
    m_numpad.draw(m_sprite, m_originX + m_rightX, m_originY, m_pressedKey);
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Spaltenaufteilung, Feldrechtecke und Touch-Hitboxen.
 */
void WorkshopTool_VoltageDivider::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;
    if (m_rightW < 100) m_rightW = 100;

    // Felder beginnen TOP_GAP unter dem Header
    for (int i = 0; i < NUM_QTY; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * ROW_H, m_rightW, ROW_H};

    m_formulaY = TOP_GAP + FIELDS_H + FORMULA_GAP_V;
    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);

    // Touch-Areas im Schaltplan (alle Y-Werte um TOP_GAP verschoben)
    m_compHits[0] = {UIN_ARROW_X - 15, TOP_GAP + UIN_RAIL_Y - HIT_PAD,
                     CHAIN_CX + COMP_W / 2 + 15 - (UIN_ARROW_X - 15), 30};
    m_compHits[1] = {CHAIN_CX - COMP_W / 2 - HIT_PAD, TOP_GAP + R1_TOP - HIT_PAD,
                     COMP_W + 2 * HIT_PAD, COMP_H + 2 * HIT_PAD};
    m_compHits[2] = {CHAIN_CX - COMP_W / 2 - HIT_PAD, TOP_GAP + R2_TOP - HIT_PAD,
                     COMP_W + 2 * HIT_PAD, COMP_H + 2 * HIT_PAD};
    m_compHits[3] = {CHAIN_CX, TOP_GAP + JUNCT_Y - HIT_PAD,
                     UOUT_WIRE_END - CHAIN_CX + HIT_PAD, 2 * HIT_PAD + 4};
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Events für Schaltplan, Felder und Numpad.
 */
bool WorkshopTool_VoltageDivider::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady) return false;
        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        if (cx < SCHEM_W)
        {
            const int hit = hitTestComponent(cx, cy);
            if (hit >= 0 && hit != m_pressedComp)
            { m_pressedComp = hit; m_pressedField = m_pressedKey = -1; needsRedraw = true; }
        }
        else if (cx >= m_rightX)
        {
            const int hitF = hitTestField(cx, cy);
            if (hitF >= 0 && hitF != m_pressedField)
            { m_pressedField = hitF; m_pressedComp = m_pressedKey = -1; needsRedraw = true; }
            else
            {
                const int hitK = m_numpad.hitTest(cx - m_rightX, cy);
                if (hitK >= 0 && hitK != m_pressedKey)
                { m_pressedKey = hitK; m_pressedField = m_pressedComp = -1; needsRedraw = true; }
            }
        }
        m_touchWas = true;
    }
    else
    {
        m_touchReady = true;
        if (m_touchWas)
        {
            int tapped = -1;
            if (m_pressedComp >= 0 && m_pressedComp < NUM_QTY) tapped = m_pressedComp;
            else if (m_pressedField >= 0 && m_pressedField < NUM_QTY) tapped = m_pressedField;

            if (tapped >= 0)
            {
                if (isInput(tapped))
                {
                    m_focusedQty = static_cast<uint8_t>(tapped);
                }
                else
                {
                    m_computedQty   = m_inputSlots[0];
                    m_inputSlots[0] = m_inputSlots[1];
                    m_inputSlots[1] = m_inputSlots[2];
                    m_inputSlots[2] = static_cast<uint8_t>(tapped);
                    m_focusedQty    = static_cast<uint8_t>(tapped);
                    if (m_computeOk)
                        formatEditValue(m_values[tapped], m_inputBuf[tapped], FIELD_CAP);
                    else
                        m_inputBuf[tapped][0] = '\0';
                    compute();
                }
                needsRedraw = true;
            }
            else if (m_pressedKey >= 0 && m_pressedKey < WorkshopNumpad::NUM_KEYS)
            {
                executeKey(WorkshopNumpad::keyAction(m_pressedKey));
                needsRedraw = true;
            }
        }
        m_pressedField = m_pressedComp = m_pressedKey = -1;
        m_touchWas = false;
    }
    return needsRedraw;
}

// ===================================================================
// Zeichnen - Schaltplan
// ===================================================================

/**
 * @brief Zeichnet den Schaltplan inkl. Bauteile, Beschriftungen und Pfeile.
 */
void WorkshopTool_VoltageDivider::drawSchematic()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    spr->fillRect(ox, oy, SCHEM_W, m_h - TOP_GAP, C_SCHEM_BG);

    // Uin/GND-Schienen
    spr->drawFastHLine(ox + 15, oy + UIN_RAIL_Y, SCHEM_W - 30, C_WIRE);
    spr->drawFastHLine(ox + 15, oy + GND_RAIL_Y, SCHEM_W - 30, C_WIRE);

    const int cx = ox + CHAIN_CX;

    // Vertikale Verdrahtung
    spr->drawFastVLine(cx, oy + UIN_RAIL_Y, R1_TOP - UIN_RAIL_Y, C_WIRE);
    spr->drawFastVLine(cx, oy + R1_BOT, JUNCT_Y - R1_BOT, C_WIRE);
    spr->drawFastVLine(cx, oy + JUNCT_Y, R2_TOP - JUNCT_Y, C_WIRE);
    spr->drawFastVLine(cx, oy + R2_BOT, GND_RAIL_Y - R2_BOT, C_WIRE);

    // Uout-Leitung horizontal vom Knotenpunkt nach rechts
    spr->drawFastHLine(cx, oy + JUNCT_Y, UOUT_WIRE_END - CHAIN_CX + 20, C_WIRE);

    // Knotenpunkte
    spr->fillCircle(cx, oy + UIN_RAIL_Y,  3, C_JUNCTION);
    spr->fillCircle(cx, oy + JUNCT_Y,     3, C_JUNCTION);
    spr->fillCircle(cx, oy + GND_RAIL_Y,  3, C_JUNCTION);

    // Bauteile
    const bool r1Active = (static_cast<int>(m_focusedQty) == 1);
    const bool r2Active = (static_cast<int>(m_focusedQty) == 2);
    drawResistor(CHAIN_CX, R1_TOP, COMP_H, r1Active);
    drawResistor(CHAIN_CX, R2_TOP, COMP_H, r2Active);

    // ===================================================================
    // Spannungspfeile (Uin links, Uout rechts)
    // ===================================================================

    // Pfeilspitzen halten ARROW_TIP_OFFSET Abstand zu den Schienen, damit sie die Labels "Uin"/"GND"/"Uout" nicht überzeichnen.

    // Uin-Pfeil (GND -> Uin)
    const int uinAx = ox + UIN_ARROW_X;
    spr->drawFastVLine(uinAx,
                       oy + UIN_RAIL_Y + ARROW_LINE_OFFSET,
                       GND_RAIL_Y - UIN_RAIL_Y - 2 * ARROW_LINE_OFFSET,
                       C_VAL_DIMMED);
    spr->fillTriangle(uinAx,     oy + UIN_RAIL_Y + ARROW_TIP_OFFSET,
                      uinAx - 3, oy + UIN_RAIL_Y + ARROW_BASE_OFFSET,
                      uinAx + 3, oy + UIN_RAIL_Y + ARROW_BASE_OFFSET, C_VAL_DIMMED);
    spr->fillTriangle(uinAx,     oy + GND_RAIL_Y - ARROW_TIP_OFFSET,
                      uinAx - 3, oy + GND_RAIL_Y - ARROW_BASE_OFFSET,
                      uinAx + 3, oy + GND_RAIL_Y - ARROW_BASE_OFFSET, C_VAL_DIMMED);

    // Uout-Pfeil (GND -> Uout-Knotenpunkt)
    const int uoutAx = ox + UOUT_ARROW_X;
    spr->drawFastVLine(uoutAx,
                       oy + JUNCT_Y + ARROW_LINE_OFFSET,
                       GND_RAIL_Y - JUNCT_Y - 2 * ARROW_LINE_OFFSET,
                       C_VAL_DIMMED);
    spr->fillTriangle(uoutAx,     oy + JUNCT_Y + ARROW_TIP_OFFSET,
                      uoutAx - 3, oy + JUNCT_Y + ARROW_BASE_OFFSET,
                      uoutAx + 3, oy + JUNCT_Y + ARROW_BASE_OFFSET, C_VAL_DIMMED);
    spr->fillTriangle(uoutAx,     oy + GND_RAIL_Y - ARROW_TIP_OFFSET,
                      uoutAx - 3, oy + GND_RAIL_Y - ARROW_BASE_OFFSET,
                      uoutAx + 3, oy + GND_RAIL_Y - ARROW_BASE_OFFSET, C_VAL_DIMMED);

    // ===================================================================
    // Beschriftungen
    // ===================================================================

    spr->loadFont(ComfortaaBold14);

    // Uin / GND
    spr->setTextDatum(ML_DATUM);
    const bool uinActive = (static_cast<int>(m_focusedQty) == 0);
    spr->setTextColor(uinActive ? C_VAL_ACTIVE : C_RAIL, C_SCHEM_BG);
    spr->drawString("Uin", ox + 5, oy + UIN_RAIL_Y);
    spr->setTextColor(C_RAIL, C_SCHEM_BG);
    spr->drawString("GND", ox + 5, oy + GND_RAIL_Y);

    // Uout
    const bool uoutActive = (static_cast<int>(m_focusedQty) == 3);
    spr->setTextColor(uoutActive ? C_VAL_ACTIVE : C_RAIL, C_SCHEM_BG);
    spr->drawString("Uout", ox + UOUT_WIRE_END + 6, oy + JUNCT_Y);

    // Uin-Wert neben Pfeil
    char valBuf[16];
    formatSchematicValue(0, valBuf, sizeof(valBuf));
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(uinActive ? C_VAL_ACTIVE : C_VAL_DIMMED, C_SCHEM_BG);
    spr->drawString("Uin", ox + UIN_ARROW_X, oy + (UIN_RAIL_Y + GND_RAIL_Y) / 2 - 8);
    spr->drawString(valBuf, ox + UIN_ARROW_X, oy + (UIN_RAIL_Y + GND_RAIL_Y) / 2 + 8);

    // Uout-Wert neben Pfeil
    formatSchematicValue(3, valBuf, sizeof(valBuf));
    spr->setTextColor(uoutActive ? C_VAL_ACTIVE : C_VAL_DIMMED, C_SCHEM_BG);
    spr->drawString("Uout", ox + UOUT_ARROW_X, oy + (JUNCT_Y + GND_RAIL_Y) / 2 - 8);
    spr->drawString(valBuf, ox + UOUT_ARROW_X, oy + (JUNCT_Y + GND_RAIL_Y) / 2 + 8);

    // R1/R2-Labels und Werte
    for (int i = 1; i <= 2; ++i)
    {
        const bool active = (static_cast<int>(m_focusedQty) == i);
        const uint16_t clr = active ? C_VAL_ACTIVE : C_VAL_DIMMED;
        const int compCy = (i == 1) ? (R1_TOP + R1_BOT) / 2 : (R2_TOP + R2_BOT) / 2;

        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(clr, C_SCHEM_BG);
        spr->drawString(QTY_SYMBOLS[i], ox + CHAIN_CX - COMP_W / 2 - 6, oy + compCy);

        formatSchematicValue(i, valBuf, sizeof(valBuf));
        spr->setTextDatum(ML_DATUM);
        spr->drawString(valBuf, ox + CHAIN_CX + COMP_W / 2 + 6, oy + compCy);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet einen einzelnen Widerstand (Block-Symbol).
 */
void WorkshopTool_VoltageDivider::drawResistor(int cx, int top, int h, bool active)
{
    Gfx_Sprite * const spr = m_sprite;
    const int ax = m_originX + cx - COMP_W / 2;
    const int ay = m_originY + TOP_GAP + top;
    spr->fillRect(ax, ay, COMP_W, h, active ? (uint16_t)0x1A45 : C_COMP_BODY);
    spr->drawRect(ax, ay, COMP_W, h, active ? C_VAL_ACTIVE : C_COMP_OUTLINE);
}

// ===================================================================
// Zeichnen - Felder, Formelzeile
// ===================================================================

/**
 * @brief Zeichnet alle vier Eingabefelder.
 */
void WorkshopTool_VoltageDivider::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);
    for (int i = 0; i < NUM_QTY; ++i)
        drawField(i, m_pressedField == i || m_pressedComp == i, cursorOn);
    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet ein einzelnes Feld inkl. Symbol, Wert, Einheit und Cursor.
 */
void WorkshopTool_VoltageDivider::drawField(int qty, bool pressed, bool cursorOn)
{
    Gfx_Sprite * const spr = m_sprite;
    const Rect &fr = m_fieldRects[qty];
    const int ax = m_originX + fr.x, ay = m_originY + fr.y;
    const bool input = isInput(qty);
    const bool focused = (static_cast<int>(m_focusedQty) == qty);

    uint16_t bg;
    if (pressed) bg = C_FIELD_PRESS;
    else if (focused) bg = C_FIELD_FOCUS;
    else bg = C_FIELD_INPUT;

    spr->fillRect(ax, ay, fr.w, fr.h, bg);
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, accentColor(qty));

    uint16_t lC, vC, uC;
    if      (focused) { lC = C_LBL_FOCUS; vC = C_VAL_FOCUS; uC = C_UNIT_FOCUS; }
    else if (input)   { lC = C_LBL_INPUT; vC = C_VAL_INPUT; uC = C_UNIT_INPUT; }
    else              { lC = C_LBL_COMP;  vC = C_VAL_COMP;  uC = C_UNIT_COMP;  }

    const int cy = ay + fr.h / 2;

    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lC, bg);
    spr->drawString(QTY_SYMBOLS[qty], ax + ACCENT_W + 6, cy);

    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(uC, bg);
    spr->drawString(QTY_UNITS[qty], ax + fr.w - UNIT_W / 2, cy);

    char displayBuf[32];
    const int valRX = ax + fr.w - UNIT_W - 6;
    spr->setTextDatum(MR_DATUM);

    // Anzeigepfad: dotToComma in allen drei Zuständen anwenden. Der interne Eingabepuffer bleibt mit '.' bestehen, damit atof() korrekt parst.
    if (focused)
    {
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), m_inputBuf[qty]);
        stringutils::safeAppend(displayBuf, sizeof(displayBuf), "_");
        dotToComma(displayBuf);
        spr->setTextColor(vC, bg);
        spr->drawString(displayBuf, valRX, cy);
        if (!cursorOn)
        {
            const int cw = spr->textWidth("_");
            spr->fillRect(valRX - cw, ay + 1, cw, fr.h - 2, bg);
        }
    }
    else if (input)
    {
        formatDisplayValue(m_values[qty], qty, displayBuf, sizeof(displayBuf));
        dotToComma(displayBuf);
        spr->setTextColor(vC, bg);
        spr->drawString(displayBuf, valRX, cy);
    }
    else
    {
        if (m_computeOk)
        {
            formatDisplayValue(m_values[qty], qty, displayBuf, sizeof(displayBuf));
            dotToComma(displayBuf);
        }
        else
        {
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");
        }
        spr->setTextColor(vC, bg);
        spr->drawString(displayBuf, valRX, cy);
    }
}

/**
 * @brief Zeichnet die Formelzeile unterhalb der Eingabefelder.
 */
void WorkshopTool_VoltageDivider::drawFormulaRow()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_formulaY;
    spr->fillRect(ox, oy, m_rightW, FORMULA_H, C_BG);

    char buf[FORMULA_CAP];
    buildFormulaStr(buf, sizeof(buf));
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_FORMULA_TXT, C_BG);
    spr->drawString(buf, ox + 4, oy + FORMULA_H / 2);
    spr->unloadFont();
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Liefert den Index des Feldes unter (cx, cy) oder -1.
 */
int WorkshopTool_VoltageDivider::hitTestField(int cx, int cy) const
{
    for (int i = 0; i < NUM_QTY; ++i)
    {
        const Rect &r = m_fieldRects[i];
        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h) return i;
    }
    return -1;
}

/**
 * @brief Liefert den Index der Schaltplan-Komponente unter (cx, cy) oder -1.
 */
int WorkshopTool_VoltageDivider::hitTestComponent(int cx, int cy) const
{
    for (int i = 0; i < NUM_QTY; ++i)
    {
        const Rect &r = m_compHits[i];
        if (r.w <= 0) continue;
        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h) return i;
    }
    return -1;
}

// ===================================================================
// Wertberechnung
// ===================================================================

/**
 * @brief Berechnet den computed-Wert anhand der drei Eingabewerte.
 */
void WorkshopTool_VoltageDivider::compute()
{
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
    {
        const int q = static_cast<int>(m_inputSlots[s]);
        m_values[q] = (m_inputBuf[q][0] != '\0') ? atof(m_inputBuf[q]) : 0.0;
    }
    m_computeOk = false;

    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (!std::isfinite(m_values[static_cast<int>(m_inputSlots[s])])) return;

    const double uin = m_values[0], r1 = m_values[1], r2 = m_values[2], uout = m_values[3];
    switch (m_computedQty)
    {
        case 3: { const double d = r1 + r2; if (d < 1e-9) return;
                  m_values[3] = uin * r2 / d; m_computeOk = std::isfinite(m_values[3]); break; }
        case 0: { if (r2 < 1e-9) return;
                  m_values[0] = uout * (r1 + r2) / r2; m_computeOk = std::isfinite(m_values[0]); break; }
        case 1: { const double dV = uin - uout; if (std::abs(dV) < 1e-15 || r2 < 0.0) return;
                  m_values[1] = r2 * (uin - uout) / uout;
                  m_computeOk = std::isfinite(m_values[1]) && m_values[1] >= 0.0; break; }
        case 2: { const double dV = uin - uout; if (std::abs(dV) < 1e-15 || r1 < 0.0) return;
                  m_values[2] = r1 * uout / (uin - uout);
                  m_computeOk = std::isfinite(m_values[2]) && m_values[2] >= 0.0; break; }
        default: break;
    }
}

/**
 * @brief Formatiert einen Wert mit SI-Präfix für die Feld-Anzeige.
 */
void WorkshopTool_VoltageDivider::formatDisplayValue(double val, int qty, char *buf, size_t sz)
{
    if (!buf || sz == 0) return;
    if (!std::isfinite(val)) { stringutils::safeCopy(buf, sz, "---"); return; }
    const double a = std::abs(val);
    if (a == 0.0) { stringutils::safeFormat(buf, sz, "0 %s", QTY_UNITS[qty]); return; }

    double s; const char *p;
    const bool isR = (qty == 1 || qty == 2);
    if (isR)
    {
        if      (a < 1e3) { s = val;       p = "";  }
        else if (a < 1e6) { s = val / 1e3; p = "k"; }
        else if (a < 1e9) { s = val / 1e6; p = "M"; }
        else              { s = val / 1e9; p = "G"; }
    }
    else
    {
        if      (a < 1e-3) { s = val * 1e6; p = "u"; }
        else if (a < 1.0)  { s = val * 1e3; p = "m"; }
        else if (a < 1e3)  { s = val;       p = "";  }
        else if (a < 1e6)  { s = val / 1e3; p = "k"; }
        else               { s = val / 1e6; p = "M"; }
    }
    stringutils::safeFormat(buf, sz, "%.4g %s%s", s, p, QTY_UNITS[qty]);
}

/**
 * @brief Formatiert einen Wert für den Eingabepuffer (ohne Einheit, mit Punkt).
 */
void WorkshopTool_VoltageDivider::formatEditValue(double val, char *buf, size_t sz)
{
    if (!buf || sz == 0) return;
    if (!std::isfinite(val) || val == 0.0) { stringutils::safeCopy(buf, sz, "0"); return; }
    stringutils::safeFormat(buf, sz, "%.6g", val);
}

/**
 * @brief Liefert den Formelstring für die aktuell berechnete Größe.
 */
void WorkshopTool_VoltageDivider::buildFormulaStr(char *buf, size_t sz) const
{
    switch (m_computedQty)
    {
        case 0: stringutils::safeCopy(buf, sz, "Uin=Uout*(R1+R2)/R2");   break;
        case 1: stringutils::safeCopy(buf, sz, "R1=R2*(Uin-Uout)/Uout"); break;
        case 2: stringutils::safeCopy(buf, sz, "R2=R1*Uout/(Uin-Uout)"); break;
        case 3: stringutils::safeCopy(buf, sz, "Uout=Uin*R2/(R1+R2)");   break;
        default: stringutils::safeCopy(buf, sz, ""); break;
    }
}

/**
 * @brief Prüft ob die Größe @p qty aktuell ein Eingabe-Slot ist.
 */
bool WorkshopTool_VoltageDivider::isInput(int qty) const
{
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (static_cast<int>(m_inputSlots[s]) == qty) return true;
    return false;
}

/**
 * @brief Liefert die Akzentfarbe der jeweiligen Größe.
 */
uint16_t WorkshopTool_VoltageDivider::accentColor(int qty)
{
    switch (qty) {
        case 0: return C_ACCENT_UIN;  case 1: return C_ACCENT_R1;
        case 2: return C_ACCENT_R2;   case 3: return C_ACCENT_UOUT;
        default: return 0xFFFF;
    }
}

/**
 * @brief Verarbeitet einen Tastendruck des Numpads.
 */
void WorkshopTool_VoltageDivider::executeKey(char action)
{
    char * const buf = m_inputBuf[m_focusedQty];
    const size_t len = strlen(buf);
    switch (action)
    {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            if (len + 1 >= FIELD_CAP) break;
            buf[len] = action; buf[len + 1] = '\0'; break;
        case '.':
            if (strchr(buf, '.')) break;
            if (len + 2 >= FIELD_CAP) break;
            if (len == 0 || (len == 1 && buf[0] == '-'))
            { memmove(buf + (len == 0 ? 1 : 2), buf + len, 1);
              buf[len] = '0'; buf[len + 1] = '.'; buf[len + 2] = '\0'; }
            else { buf[len] = '.'; buf[len + 1] = '\0'; }
            break;
        case 'B': if (len > 0) buf[len - 1] = '\0'; break;
        case 'C': buf[0] = '\0'; break;
        case 'S':
            if (len == 0) { buf[0] = '-'; buf[1] = '\0'; }
            else if (buf[0] == '-') memmove(buf, buf + 1, len);
            else { if (len + 1 >= FIELD_CAP) break; memmove(buf + 1, buf, len + 1); buf[0] = '-'; }
            break;
        default: break;
    }
    compute();
}

/**
 * @brief Formatiert einen Wert kompakt für die Schaltplan-Anzeige.
 */
void WorkshopTool_VoltageDivider::formatSchematicValue(int qty, char *buf, size_t sz) const
{
    if (!buf || sz == 0) return;
    const double val = m_values[qty];
    if (!std::isfinite(val)) { stringutils::safeCopy(buf, sz, "?"); return; }
    const bool isR = (qty == 1 || qty == 2);
    if (isR)
    {
        const double a = std::abs(val);
        if (a >= 1e6) stringutils::safeFormat(buf, sz, "%.3gM", val / 1e6);
        else if (a >= 1e3) stringutils::safeFormat(buf, sz, "%.3gk", val / 1e3);
        else stringutils::safeFormat(buf, sz, "%.3g", val);
    }
    else
    {
        stringutils::safeFormat(buf, sz, "%.4gV", val);
    }
    dotToComma(buf);
}
