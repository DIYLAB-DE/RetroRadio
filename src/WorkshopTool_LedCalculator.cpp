#include "WorkshopTool_LedCalculator.h"

#include <algorithm>
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
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================

namespace
{

    // Header-Abstand
    constexpr int TOP_GAP = 10;

    /**
     * @brief Ersetzt alle Dezimalpunkte durch Kommas im übergebenen Puffer.
     */
    void dotToComma(char *buf)
    {
        for (char *p = buf; *p != '\0'; ++p)
            if (*p == '.')
                *p = ',';
    }

    constexpr int SCHEM_W = 250; ///< Breite des Schaltplanbereichs (links), rechts davon die Felder + Numpad
    constexpr int COL_GAP = 12;  ///< Abstand zwischen Schaltplan und rechtem Bereich (Felder + Numpad)

    // Schaltplan - Vertikale Bauteilkette
    constexpr int CHAIN_CX = 125;           ///< X-Mitte der Kette (zentriert im 250px-Bereich)
    constexpr int UB_RAIL_Y = 25;           ///< Ub-Schiene
    constexpr int COMP_W = 24;              ///< Widerstands Breite
    constexpr int COMP_R_H = 45;            ///< Widerstands Höhe
    constexpr int R_TOP = 52;               ///< Widerstand Oberkante
    constexpr int R_BOT = R_TOP + COMP_R_H; ///< Widerstand Unterkante

    constexpr int LED_TOP = 118;                    ///< LED Anode
    constexpr int LED_TRI_H = 32;                   ///< Dreieck-Höhe
    constexpr int LED_TRI_W = 28;                   ///< Dreieck-Basisbreite (halbe Breite)
    constexpr int LED_CATH_Y = LED_TOP + LED_TRI_H; ///< LED Kathode
    constexpr int LED_CATH_W = 32;                  ///< Kathoden-Breite

    constexpr int GND_RAIL_Y = 210; ///< GND-Schiene

    // Strompfeil (If) - links der Bauteilkette, Doppelpfeil zwischen Ub und GND
    constexpr int IF_ARROW_X = 30; ///< X-Position des Pfeils

    // Vertikaler Abstand der Pfeilspitzen zu den Schienen-Labels
    constexpr int ARROW_TIP_OFFSET = 14;  ///< Spitze: Abstand zur Schiene
    constexpr int ARROW_BASE_OFFSET = 20; ///< Pfeilbasis: Abstand zur Schiene
    constexpr int ARROW_LINE_OFFSET = 16; ///< Vertikale Linie: Abstand zur Schiene

    constexpr int HIT_PAD = 10; /// Zusätzlicher Abstand für Touch-Hitboxen (außerhalb der sichtbaren Komponenten)

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

const char *const WorkshopTool_LedCalculator::QTY_SYMBOLS[NUM_QTY] =
    {"Ub", "Uf", "If", "R"};

const char *const WorkshopTool_LedCalculator::QTY_UNITS[NUM_QTY] =
    {"V", "V", "A", "Ohm"};

const double WorkshopTool_LedCalculator::E12_VALUES[E12_COUNT] =
    {
        1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Initialisiert den LED-Rechner mit Standardwerten.
 */
WorkshopTool_LedCalculator::WorkshopTool_LedCalculator() { clearAll(); }

/**
 * @brief Setzt den LED-Rechner beim Aktivieren zurück.
 */
void WorkshopTool_LedCalculator::onEnter(LGFX * /*tft*/) { clearAll(); }

/**
 * @brief Setzt alle Eingaben, Slots und UI-Zustände auf die Standardwerte zurück.
 */
void WorkshopTool_LedCalculator::clearAll()
{
    memset(m_inputBuf, 0, sizeof(m_inputBuf));
    memset(m_values, 0, sizeof(m_values));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));
    memset(m_inputSlots, 0, sizeof(m_inputSlots));
    memset(m_compHits, 0, sizeof(m_compHits));

    m_sprite = nullptr;
    m_originX = m_originY = m_w = m_h = 0;
    m_layoutValid = false;
    m_computeOk = false;
    m_pValue = 0.0;
    m_numpad.clearLayout();

    m_pressedField = m_pressedComp = m_pressedKey = -1;
    m_touchWas = m_touchReady = false;

    m_inputSlots[0] = 0; // Ub
    m_inputSlots[1] = 1; // Uf
    m_inputSlots[2] = 2; // If fokussiert
    m_computedQty = 3;   // R berechnet
    m_focusedQty = 2;

    m_values[0] = 5.0;  // Ub = 5 V
    m_values[1] = 2.0;  // Uf = 2.0 V
    m_values[2] = 0.02; // If = 20 mA

    formatEditValue(m_values[0], m_inputBuf[0], FIELD_CAP);
    formatEditValue(m_values[1], m_inputBuf[1], FIELD_CAP);
    formatEditValue(m_values[2], m_inputBuf[2], FIELD_CAP);
    compute();
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet den kompletten Tool-Inhalt (Schaltplan, Felder, Info, Numpad).
 */
void WorkshopTool_LedCalculator::draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
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

    const bool cursorOn = (millis() % CURSOR_PERIOD_MS) < CURSOR_ON_MS;
    drawSchematic();
    drawFields(cursorOn);
    drawInfoRow();
    m_numpad.draw(m_sprite, m_originX + m_rightX, m_originY, m_pressedKey);
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Spaltenaufteilung, Feldrechtecke und Touch-Hitboxen.
 */
void WorkshopTool_LedCalculator::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;

    if (m_rightW < 100)
        m_rightW = 100;

    // Felder beginnen TOP_GAP unter dem Header
    for (int i = 0; i < NUM_QTY; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * ROW_H, m_rightW, ROW_H};

    m_infoY = TOP_GAP + FIELDS_H + INFO_GAP_V;
    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);

    // Touch-Areas: 0=Ub(Schiene), 1=Uf(LED), 2=If(Pfeil), 3=R(Widerstand) Alle Y-Werte um TOP_GAP verschoben. Reihenfolge wichtig: hitTestComponent liefert den ersten Treffer, d.h. Ub (Index 0) gewinnt im überlappenden Schienenbereich gegenüber dem If-Pfeil.
    m_compHits[0] = {20, TOP_GAP + UB_RAIL_Y - HIT_PAD, 200, 24};
    m_compHits[3] = {CHAIN_CX - COMP_W / 2 - HIT_PAD, TOP_GAP + R_TOP - HIT_PAD, COMP_W + 2 * HIT_PAD, COMP_R_H + 2 * HIT_PAD};
    m_compHits[1] = {CHAIN_CX - LED_TRI_W / 2 - HIT_PAD, TOP_GAP + LED_TOP - HIT_PAD, LED_TRI_W + 2 * HIT_PAD, LED_TRI_H + 2 * HIT_PAD + 10};
    m_compHits[2] = {IF_ARROW_X - HIT_PAD, TOP_GAP + UB_RAIL_Y - HIT_PAD, 30 + 2 * HIT_PAD, GND_RAIL_Y - UB_RAIL_Y + 2 * HIT_PAD};
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Events für Schaltplan, Felder und Numpad.
 */
bool WorkshopTool_LedCalculator::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;
        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        if (cx < SCHEM_W)
        {
            const int hit = hitTestComponent(cx, cy);
            if (hit >= 0 && hit != m_pressedComp)
            {
                m_pressedComp = hit;
                m_pressedField = m_pressedKey = -1;
                needsRedraw = true;
            }
        }
        else if (cx >= m_rightX)
        {
            const int hitF = hitTestField(cx, cy);
            if (hitF >= 0 && hitF != m_pressedField)
            {
                m_pressedField = hitF;
                m_pressedComp = m_pressedKey = -1;
                needsRedraw = true;
            }
            else
            {
                const int hitK = m_numpad.hitTest(cx - m_rightX, cy);
                if (hitK >= 0 && hitK != m_pressedKey)
                {
                    m_pressedKey = hitK;
                    m_pressedField = m_pressedComp = -1;
                    needsRedraw = true;
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
            int tapped = -1;
            if (m_pressedComp >= 0 && m_pressedComp < NUM_QTY)
                tapped = m_pressedComp;
            else if (m_pressedField >= 0 && m_pressedField < NUM_QTY)
                tapped = m_pressedField;

            if (tapped >= 0)
            {
                if (isInput(tapped))
                {
                    m_focusedQty = static_cast<uint8_t>(tapped);
                }
                else
                {
                    m_computedQty = m_inputSlots[0];
                    m_inputSlots[0] = m_inputSlots[1];
                    m_inputSlots[1] = m_inputSlots[2];
                    m_inputSlots[2] = static_cast<uint8_t>(tapped);
                    m_focusedQty = static_cast<uint8_t>(tapped);
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
 * @brief Zeichnet den Schaltplan inkl. Bauteile, Beschriftungen und Strompfeil.
 */
void WorkshopTool_LedCalculator::drawSchematic()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    spr->fillRect(ox, oy, SCHEM_W, m_h - TOP_GAP, C_SCHEM_BG);

    // Ub/GND-Schienen
    spr->drawFastHLine(ox + 15, oy + UB_RAIL_Y, SCHEM_W - 30, C_WIRE);
    spr->drawFastHLine(ox + 15, oy + GND_RAIL_Y, SCHEM_W - 30, C_WIRE);

    const int cx = ox + CHAIN_CX;

    // Vertikale Verdrahtung
    spr->drawFastVLine(cx, oy + UB_RAIL_Y, R_TOP - UB_RAIL_Y, C_WIRE);        // Ub -> R
    spr->drawFastVLine(cx, oy + R_BOT, LED_TOP - R_BOT, C_WIRE);              // R -> LED
    spr->drawFastVLine(cx, oy + LED_CATH_Y, GND_RAIL_Y - LED_CATH_Y, C_WIRE); // LED -> GND

    // Knotenpunkte
    spr->fillCircle(cx, oy + UB_RAIL_Y, 3, C_JUNCTION);
    spr->fillCircle(cx, oy + GND_RAIL_Y, 3, C_JUNCTION);

    // Bauteile
    const bool rActive = (static_cast<int>(m_focusedQty) == 3);
    drawResistor(CHAIN_CX, R_TOP, COMP_R_H, rActive);

    const bool ufActive = (static_cast<int>(m_focusedQty) == 1);
    const bool ifActive = (static_cast<int>(m_focusedQty) == 2);
    drawLedSymbol(CHAIN_CX, LED_TOP, ufActive, ifActive);

    // ===================================================================
    // Strompfeil If (links der Kette, Doppelpfeil zwischen Ub- und GND-Schiene)
    // ===================================================================

    const int iax = ox + IF_ARROW_X;
    spr->drawFastVLine(iax, oy + UB_RAIL_Y + ARROW_LINE_OFFSET, GND_RAIL_Y - UB_RAIL_Y - 2 * ARROW_LINE_OFFSET, C_VAL_DIMMED);
    // Pfeilspitze oben
    spr->fillTriangle(iax, oy + UB_RAIL_Y + ARROW_TIP_OFFSET, iax - 3, oy + UB_RAIL_Y + ARROW_BASE_OFFSET, iax + 3, oy + UB_RAIL_Y + ARROW_BASE_OFFSET, C_VAL_DIMMED);
    // Pfeilspitze unten
    spr->fillTriangle(iax, oy + GND_RAIL_Y - ARROW_TIP_OFFSET, iax - 3, oy + GND_RAIL_Y - ARROW_BASE_OFFSET, iax + 3, oy + GND_RAIL_Y - ARROW_BASE_OFFSET, C_VAL_DIMMED);

    // ===================================================================
    // Beschriftungen
    // ===================================================================

    spr->loadFont(ComfortaaBold14);

    // Ub / GND
    const bool ubActive = (static_cast<int>(m_focusedQty) == 0);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(ubActive ? C_VAL_ACTIVE : C_RAIL, C_SCHEM_BG);
    spr->drawString("Ub", ox + 5, oy + UB_RAIL_Y);
    spr->setTextColor(C_RAIL, C_SCHEM_BG);
    spr->drawString("GND", ox + 5, oy + GND_RAIL_Y);

    // Ub-Wert rechts unter der Schiene
    char valBuf[16];
    formatSchematicValue(0, valBuf, sizeof(valBuf));
    spr->setTextDatum(MR_DATUM);
    spr->setTextColor(ubActive ? C_VAL_ACTIVE : C_VAL_DIMMED, C_SCHEM_BG);
    spr->drawString(valBuf, ox + SCHEM_W - 18, oy + UB_RAIL_Y + 14);

    // R-Label und Wert
    {
        const uint16_t clr = rActive ? C_VAL_ACTIVE : C_VAL_DIMMED;
        const int rCy = (R_TOP + R_BOT) / 2;

        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(clr, C_SCHEM_BG);
        spr->drawString("R", ox + CHAIN_CX - COMP_W / 2 - 6, oy + rCy);

        formatSchematicValue(3, valBuf, sizeof(valBuf));
        spr->setTextDatum(ML_DATUM);
        spr->drawString(valBuf, ox + CHAIN_CX + COMP_W / 2 + 6, oy + rCy);
    }

    // Uf-Wert rechts neben LED (mit Abstand zu Emissionspfeilen)
    {
        const uint16_t clr = ufActive ? C_VAL_ACTIVE : C_VAL_DIMMED;
        const int ledCy = LED_TOP + LED_TRI_H / 2;
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(clr, C_SCHEM_BG);
        spr->drawString("Uf", ox + CHAIN_CX + LED_TRI_W / 2 + 30, oy + ledCy - 8);

        formatSchematicValue(1, valBuf, sizeof(valBuf));
        spr->drawString(valBuf, ox + CHAIN_CX + LED_TRI_W / 2 + 30, oy + ledCy + 8);
    }

    // If-Wert mittig auf dem Strompfeil (Hintergrundfarbe blendet die Linie aus)
    {
        const uint16_t clr = ifActive ? C_VAL_ACTIVE : C_VAL_DIMMED;
        const int ifCy = (UB_RAIL_Y + GND_RAIL_Y) / 2;
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(clr, C_SCHEM_BG);
        spr->drawString("If", ox + IF_ARROW_X, oy + ifCy - 8);

        formatSchematicValue(2, valBuf, sizeof(valBuf));
        spr->drawString(valBuf, ox + IF_ARROW_X, oy + ifCy + 8);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet das LED-Symbol: Dreieck (Spitze unten) + Kathodenstrich + Emissionspfeile.
 */
void WorkshopTool_LedCalculator::drawLedSymbol(int cx, int topY, bool ufActive, bool /*ifActive*/)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ax = m_originX + cx;
    const int ay = m_originY + TOP_GAP + topY;

    const uint16_t bodyClr = ufActive ? C_VAL_ACTIVE : C_LED_CATHODE;
    const uint16_t cathClr = ufActive ? C_VAL_ACTIVE : C_LED_CATHODE;

    // Dreieck (oben = Anode, Spitze unten = Kathode)
    spr->fillTriangle(ax - LED_TRI_W / 2, ay, ax + LED_TRI_W / 2, ay, ax, ay + LED_TRI_H, bodyClr);
    // Kathodenstrich
    spr->fillRect(ax - LED_CATH_W / 2, ay + LED_TRI_H, LED_CATH_W, 3, cathClr);

    // Emissionspfeile
    const uint16_t emClr = C_LED_EMISSION;
    const float lw = 1.2f;

    // Erster Emissionspfeil
    const int esx1 = ax + LED_TRI_W / 2 + 3;
    const int esy1 = ay + LED_TRI_H / 2 + 2;
    const int eex1 = esx1 + 12;
    const int eey1 = esy1 - 10;
    spr->drawWedgeLine(esx1, esy1, eex1, eey1, lw, lw, emClr);
    spr->drawWedgeLine(eex1, eey1, eex1 - 5, eey1 + 2, lw, lw, emClr);
    spr->drawWedgeLine(eex1, eey1, eex1 - 2, eey1 + 5, lw, lw, emClr);

    // Zweiter Emissionspfeil
    const int esx2 = esx1 + 2;
    const int esy2 = esy1 + 10;
    const int eex2 = esx2 + 12;
    const int eey2 = esy2 - 10;
    spr->drawWedgeLine(esx2, esy2, eex2, eey2, lw, lw, emClr);
    spr->drawWedgeLine(eex2, eey2, eex2 - 5, eey2 + 2, lw, lw, emClr);
    spr->drawWedgeLine(eex2, eey2, eex2 - 2, eey2 + 5, lw, lw, emClr);
}

/**
 * @brief Zeichnet einen einzelnen Widerstand.
 */
void WorkshopTool_LedCalculator::drawResistor(int cx, int top, int h, bool active)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ax = m_originX + cx - COMP_W / 2;
    const int ay = m_originY + TOP_GAP + top;
    spr->fillRect(ax, ay, COMP_W, h, active ? (uint16_t)0x1A45 : C_COMP_BODY);
    spr->drawRect(ax, ay, COMP_W, h, active ? C_VAL_ACTIVE : C_COMP_OUTLINE);
}

// ===================================================================
// Zeichnen - Felder, Info-Zeile
// ===================================================================

/**
 * @brief Zeichnet alle vier Eingabefelder.
 */
void WorkshopTool_LedCalculator::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);

    for (int i = 0; i < NUM_QTY; ++i)
        drawField(i, m_pressedField == i || m_pressedComp == i, cursorOn);

    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet ein einzelnes Feld inkl. Symbol, Wert, Einheit und Cursor.
 */
void WorkshopTool_LedCalculator::drawField(int qty, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const Rect &fr = m_fieldRects[qty];
    const int ax = m_originX + fr.x, ay = m_originY + fr.y;
    const bool input = isInput(qty);
    const bool focused = (static_cast<int>(m_focusedQty) == qty);

    uint16_t bg;
    if (pressed)
        bg = C_FIELD_PRESS;
    else if (focused)
        bg = C_FIELD_FOCUS;
    else
        bg = C_FIELD_INPUT;

    spr->fillRect(ax, ay, fr.w, fr.h, bg);
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, accentColor(qty));

    uint16_t lC, vC, uC;
    if (focused)
    {
        lC = C_LBL_FOCUS;
        vC = C_VAL_FOCUS;
        uC = C_UNIT_FOCUS;
    }
    else if (input)
    {
        lC = C_LBL_INPUT;
        vC = C_VAL_INPUT;
        uC = C_UNIT_INPUT;
    }
    else
    {
        lC = C_LBL_COMP;
        vC = C_VAL_COMP;
        uC = C_UNIT_COMP;
    }

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
 * @brief Zeichnet die Info-Zeile mit Formel links und Leistung/E12-Wert rechts.
 */
void WorkshopTool_LedCalculator::drawInfoRow()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_infoY;
    const int cy = oy + INFO_H / 2;

    spr->fillRect(ox, oy, m_rightW, INFO_H, C_BG);
    spr->loadFont(ComfortaaBold14);

    // Formel links
    char formulaBuf[INFO_STR_CAP];
    buildFormulaStr(formulaBuf, sizeof(formulaBuf));
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_FORMULA_TXT, C_BG);
    spr->drawString(formulaBuf, ox + 4, cy);

    // P + E12 rechts
    char infoBuf[INFO_STR_CAP];
    if (m_computeOk)
    {
        char pBuf[24];
        formatPower(m_pValue, pBuf, sizeof(pBuf));
        if (m_computedQty == 3)
        {
            char e12Buf[32];
            formatE12(m_values[3], e12Buf, sizeof(e12Buf));
            stringutils::safeFormat(infoBuf, sizeof(infoBuf), "P:%s %s", pBuf, e12Buf);
        }
        else
            stringutils::safeFormat(infoBuf, sizeof(infoBuf), "P: %s", pBuf);
    }
    else
        stringutils::safeCopy(infoBuf, sizeof(infoBuf), "P: ---");

    spr->setTextDatum(MR_DATUM);
    spr->setTextColor(C_P_INFO_TXT, C_BG);
    spr->drawString(infoBuf, ox + m_rightW - 4, cy);

    spr->unloadFont();
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Liefert den Index des Feldes unter (cx, cy) oder -1.
 */
int WorkshopTool_LedCalculator::hitTestField(int cx, int cy) const
{
    for (int i = 0; i < NUM_QTY; ++i)
    {
        const Rect &r = m_fieldRects[i];

        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h)
            return i;
    }
    return -1;
}

/**
 * @brief Liefert den Index der Schaltplan-Komponente unter (cx, cy) oder -1.
 */
int WorkshopTool_LedCalculator::hitTestComponent(int cx, int cy) const
{
    for (int i = 0; i < NUM_QTY; ++i)
    {
        const Rect &r = m_compHits[i];

        if (r.w <= 0)
            continue;

        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h)
            return i;
    }
    return -1;
}

// ===================================================================
// Wertberechnung
// ===================================================================

/**
 * @brief Berechnet den computed-Wert anhand der drei Eingabewerte und die Verlustleistung.
 */
void WorkshopTool_LedCalculator::compute()
{
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
    {
        const int q = static_cast<int>(m_inputSlots[s]);
        m_values[q] = (m_inputBuf[q][0] != '\0') ? atof(m_inputBuf[q]) : 0.0;
    }

    m_computeOk = false;

    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (!std::isfinite(m_values[static_cast<int>(m_inputSlots[s])]))
            return;

    const double ub = m_values[0], uf = m_values[1], iF = m_values[2], r = m_values[3];
    switch (m_computedQty)
    {
    case 3:
        if (std::abs(iF) < 1e-18)
            return;

        m_values[3] = (ub - uf) / iF;
        m_computeOk = std::isfinite(m_values[3]);
        break;
    case 0:
        m_values[0] = uf + iF * r;
        m_computeOk = std::isfinite(m_values[0]);
        break;
    case 1:
        m_values[1] = ub - iF * r;
        m_computeOk = std::isfinite(m_values[1]);
        break;
    case 2:
        if (std::abs(r) < 1e-18)
            return;

        m_values[2] = (ub - uf) / r;
        m_computeOk = std::isfinite(m_values[2]);
        break;
    default:
        break;
    }

    const double p = (m_values[0] - m_values[1]) * m_values[2];
    m_pValue = std::isfinite(p) ? p : 0.0;
}

// ===================================================================
// Formatierung
// ===================================================================

/**
 * @brief Formatiert einen Wert mit SI-Präfix für die Feld-Anzeige.
 */
void WorkshopTool_LedCalculator::formatDisplayValue(double val, int qty, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    const double a = std::abs(val);
    if (a == 0.0)
    {
        stringutils::safeFormat(buf, sz, "0 %s", QTY_UNITS[qty]);
        return;
    }

    double s;
    const char *p;
    if (qty == 3) // Widerstand
    {
        if (a < 1e3)
        {
            s = val;
            p = "";
        }
        else if (a < 1e6)
        {
            s = val / 1e3;
            p = "k";
        }
        else if (a < 1e9)
        {
            s = val / 1e6;
            p = "M";
        }
        else
        {
            s = val / 1e9;
            p = "G";
        }
    }
    else if (qty == 2) // Strom
    {
        if (a < 1e-6)
        {
            s = val * 1e9;
            p = "n";
        }
        else if (a < 1e-3)
        {
            s = val * 1e6;
            p = "u";
        }
        else if (a < 1.0)
        {
            s = val * 1e3;
            p = "m";
        }
        else if (a < 1e3)
        {
            s = val;
            p = "";
        }
        else
        {
            s = val / 1e3;
            p = "k";
        }
    }
    else // Spannung
    {
        if (a < 1e-3)
        {
            s = val * 1e6;
            p = "u";
        }
        else if (a < 1.0)
        {
            s = val * 1e3;
            p = "m";
        }
        else if (a < 1e3)
        {
            s = val;
            p = "";
        }
        else if (a < 1e6)
        {
            s = val / 1e3;
            p = "k";
        }
        else
        {
            s = val / 1e6;
            p = "M";
        }
    }

    stringutils::safeFormat(buf, sz, "%.4g %s%s", s, p, QTY_UNITS[qty]);
}

/**
 * @brief Formatiert einen Wert für den Eingabepuffer (ohne Einheit, mit Punkt).
 */
void WorkshopTool_LedCalculator::formatEditValue(double val, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(val) || val == 0.0)
    {
        stringutils::safeCopy(buf, sz, "0");
        return;
    }

    stringutils::safeFormat(buf, sz, "%.6g", val);
}

/**
 * @brief Formatiert eine Leistung mit passender Einheit (uW/mW/W).
 */
void WorkshopTool_LedCalculator::formatPower(double p, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(p))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    const double a = std::abs(p);
    if (a == 0.0)
    {
        stringutils::safeCopy(buf, sz, "0 W");
        return;
    }

    double s;
    const char *u;
    if (a < 1e-3)
    {
        s = p * 1e6;
        u = "uW";
    }
    else if (a < 1.0)
    {
        s = p * 1e3;
        u = "mW";
    }
    else
    {
        s = p;
        u = "W";
    }

    stringutils::safeFormat(buf, sz, "%.4g %s", s, u);
    dotToComma(buf);
}

/**
 * @brief Liefert den Formelstring für die aktuell berechnete Größe.
 */
void WorkshopTool_LedCalculator::buildFormulaStr(char *buf, size_t sz) const
{
    switch (m_computedQty)
    {
    case 3:
        stringutils::safeCopy(buf, sz, "R=(Ub-Uf)/If");
        break;
    case 0:
        stringutils::safeCopy(buf, sz, "Ub=Uf+If*R");
        break;
    case 1:
        stringutils::safeCopy(buf, sz, "Uf=Ub-If*R");
        break;
    case 2:
        stringutils::safeCopy(buf, sz, "If=(Ub-Uf)/R");
        break;
    default:
        stringutils::safeCopy(buf, sz, "");
        break;
    }
}

/**
 * @brief Prüft ob die Größe @p qty aktuell ein Eingabe-Slot ist.
 */
bool WorkshopTool_LedCalculator::isInput(int qty) const
{
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (static_cast<int>(m_inputSlots[s]) == qty)
            return true;

    return false;
}

/**
 * @brief Liefert die Akzentfarbe der jeweiligen Größe.
 */
uint16_t WorkshopTool_LedCalculator::accentColor(int qty)
{
    switch (qty)
    {
    case 0:
        return C_ACCENT_UB;
    case 1:
        return C_ACCENT_UF;
    case 2:
        return C_ACCENT_IF;
    case 3:
        return C_ACCENT_R;
    default:
        return 0xFFFF;
    }
}

/**
 * @brief Liefert den nächstgelegenen E12-Normwert zum übergebenen Widerstand.
 */
double WorkshopTool_LedCalculator::nearestE12(double r)
{
    if (r <= 0.0 || !std::isfinite(r))
        return 0.0;

    const double expn = floor(log10(r));
    const double decade = pow(10.0, expn);
    double bestVal = E12_VALUES[0] * decade;
    double bestDiff = fabs(r - bestVal);

    for (int i = 1; i < E12_COUNT; ++i)
    {
        const double c = E12_VALUES[i] * decade;
        const double d = fabs(r - c);

        if (d < bestDiff)
        {
            bestDiff = d;
            bestVal = c;
        }
    }

    if (fabs(r - decade * 10.0) < bestDiff)
        bestVal = decade * 10.0;

    return bestVal;
}

/**
 * @brief Formatiert den nächsten E12-Wert mit SI-Präfix als "E12:wert prefix".
 */
void WorkshopTool_LedCalculator::formatE12(double r, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(r) || r <= 0.0)
    {
        stringutils::safeCopy(buf, sz, "E12:---");
        return;
    }

    const double e = nearestE12(r);
    const double a = fabs(e);
    double s;
    const char *p;

    if (a < 1e3)
    {
        s = e;
        p = "";
    }
    else if (a < 1e6)
    {
        s = e / 1e3;
        p = "k";
    }
    else if (a < 1e9)
    {
        s = e / 1e6;
        p = "M";
    }
    else
    {
        s = e / 1e9;
        p = "G";
    }

    char vb[24];
    stringutils::safeFormat(vb, sizeof(vb), "%.4g", s);
    stringutils::safeFormat(buf, sz, "E12:%s %s", vb, p);
    dotToComma(buf);
}

/**
 * @brief Verarbeitet einen Tastendruck des Numpads.
 */
void WorkshopTool_LedCalculator::executeKey(char action)
{
    char *const buf = m_inputBuf[m_focusedQty];
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
        if (len + 1 >= FIELD_CAP)
            break;

        buf[len] = action;
        buf[len + 1] = '\0';
        break;
    case '.':
        if (strchr(buf, '.'))
            break;

        if (len + 2 >= FIELD_CAP)
            break;

        if (len == 0 || (len == 1 && buf[0] == '-'))
        {
            memmove(buf + (len == 0 ? 1 : 2), buf + len, 1);
            buf[len] = '0';
            buf[len + 1] = '.';
            buf[len + 2] = '\0';
        }
        else
        {
            buf[len] = '.';
            buf[len + 1] = '\0';
        }
        break;
    case 'B':
        if (len > 0)
            buf[len - 1] = '\0';

        break;
    case 'C':
        buf[0] = '\0';
        break;
    case 'S':
        if (len == 0)
        {
            buf[0] = '-';
            buf[1] = '\0';
        }
        else if (buf[0] == '-')
        {
            memmove(buf, buf + 1, len);
        }
        else
        {
            if (len + 1 >= FIELD_CAP)
                break;

            memmove(buf + 1, buf, len + 1);
            buf[0] = '-';
        }
        break;
    default:
        break;
    }

    compute();
}

/**
 * @brief Formatiert einen Wert kompakt für die Schaltplan-Anzeige.
 */
void WorkshopTool_LedCalculator::formatSchematicValue(int qty, char *buf, size_t sz) const
{
    if (!buf || sz == 0)
        return;

    const double val = m_values[qty];

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "?");
        return;
    }

    if (qty == 3) // Widerstand
    {
        const double a = std::abs(val);
        if (a >= 1e6)
            stringutils::safeFormat(buf, sz, "%.3gM", val / 1e6);
        else if (a >= 1e3)
            stringutils::safeFormat(buf, sz, "%.3gk", val / 1e3);
        else
            stringutils::safeFormat(buf, sz, "%.3g", val);
    }
    else if (qty == 2) // Strom
    {
        const double a = std::abs(val);
        if (a >= 1.0)
            stringutils::safeFormat(buf, sz, "%.3gA", val);
        else if (a >= 1e-3)
            stringutils::safeFormat(buf, sz, "%.3gmA", val * 1e3);
        else
            stringutils::safeFormat(buf, sz, "%.3guA", val * 1e6);
    }
    else // Spannung
    {
        stringutils::safeFormat(buf, sz, "%.3gV", val);
    }

    dotToComma(buf);
}
