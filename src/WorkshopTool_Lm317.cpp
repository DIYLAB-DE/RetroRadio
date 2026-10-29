#include "WorkshopTool_Lm317.h"

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
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================

namespace
{

    // Spaltenaufteilung
    constexpr int SCHEM_W = 250; ///< Breite der linken Spalte (Schaltplan)
    constexpr int COL_GAP = 12;  ///< Abstand zwischen Spalten

    // Schaltplan - IC-Gehäuse
    constexpr int PKG_X = 30;  ///< Gehäuse X
    constexpr int PKG_Y = 35;  ///< Gehäuse Y
    constexpr int PKG_W = 130; ///< Gehäuse Breite
    constexpr int PKG_H = 55;  ///< Gehäuse Höhe
    constexpr int PKG_R = 4;   ///< Eckenradius

    constexpr int IN_PIN_Y = PKG_Y + PKG_H / 2;  ///< Y-Mitte IN-Pin
    constexpr int OUT_PIN_X = PKG_X + PKG_W;     ///< X rechter Rand
    constexpr int ADJ_PIN_X = PKG_X + PKG_W / 2; ///< X ADJ-Pin (Mitte)
    constexpr int ADJ_PIN_Y = PKG_Y + PKG_H;     ///< Y ADJ-Pin (Unterkante)

    // Schaltplan - Verdrahtung und Bauteilkette
    constexpr int CHAIN_CX = 200;           ///< X-Mitte der R1/R2-Kette
    constexpr int COMP_W = 24;              ///< Bauteil-Breite
    constexpr int COMP_H = 42;              ///< Bauteil-Höhe (Widerstände)
    constexpr int R1_TOP = 100;             ///< R1 Oberkante
    constexpr int R1_BOT = R1_TOP + COMP_H; ///< R1 Unterkante
    constexpr int JUNCT_Y = 158;            ///< Knotenpunkt R1/R2/ADJ
    constexpr int R2_TOP = 172;             ///< R2 Oberkante
    constexpr int R2_BOT = R2_TOP + COMP_H; ///< R2 Unterkante
    constexpr int GND_RAIL_Y = 245;         ///< GND-Schiene Y

    // Vref-Annotation
    constexpr int VREF_ARROW_X = 235;

    // Touch-Hit-Areas
    constexpr int HIT_PAD = 8; ///< Vertikales Padding um Bauteile

    /**
     * @brief Ersetzt den (einzigen) Dezimalpunkt im Puffer durch ein Komma.
     * @param buf Puffer mit formatiertem Zahlenwert
     */
    void dotToComma(char *buf)
    {
        for (char *p = buf; *p != '\0'; ++p)
        {
            if (*p == '.')
            {
                *p = ',';
                break;
            }
        }
    }

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

const char *const WorkshopTool_Lm317::QTY_SYMBOLS[NUM_QTY] =
    {"Vref", "R1", "R2", "Vout"};

const char *const WorkshopTool_Lm317::QTY_UNITS[NUM_QTY] =
    {"V", "Ohm", "Ohm", "V"};

const double WorkshopTool_Lm317::E24_VALUES[E24_COUNT] =
    {
        1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0,
        2.2, 2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.3,
        4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_Lm317::WorkshopTool_Lm317()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_Lm317::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_Lm317::clearAll()
{
    memset(m_inputBuf, 0, sizeof(m_inputBuf));
    memset(m_values, 0, sizeof(m_values));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));
    memset(m_inputSlots, 0, sizeof(m_inputSlots));
    memset(m_compHits, 0, sizeof(m_compHits));

    m_sprite = nullptr;
    m_originX = 0;
    m_originY = 0;
    m_w = 0;
    m_h = 0;
    m_layoutValid = false;
    m_computeOk = false;
    m_numpad.clearLayout();

    m_pressedField = -1;
    m_pressedComp = -1;
    m_pressedKey = -1;
    m_touchWas = false;
    m_touchReady = false;

    // Vref=1,25 V (Slot 0), R1=240 Ohm (Slot 1), R2=720 Ohm (Slot 2, fokussiert)
    m_inputSlots[0] = 0; // Vref (ältester)
    m_inputSlots[1] = 1; // R1
    m_inputSlots[2] = 2; // R2 (neuester, fokussiert)
    m_computedQty = 3;   // Vout berechnet
    m_focusedQty = 2;    // R2 fokussiert

    m_values[0] = 1.25;  // Vref = 1,25 V
    m_values[1] = 240.0; // R1 = 240 Ohm
    m_values[2] = 720.0; // R2 = 720 Ohm

    formatEditValue(m_values[0], m_inputBuf[0], FIELD_CAP);
    formatEditValue(m_values[1], m_inputBuf[1], FIELD_CAP);
    formatEditValue(m_values[2], m_inputBuf[2], FIELD_CAP);

    compute();
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Schaltplan, Eingabefelder, Info-Zeile und Numpad.
 */
void WorkshopTool_Lm317::draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
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
 * @brief Berechnet Spaltenaufteilung, Feld-Rects, Hit-Areas und Numpad.
 */
void WorkshopTool_Lm317::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;

    if (m_rightW < 100)
        m_rightW = 100;

    // Eingabefelder
    for (int i = 0; i < NUM_QTY; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * ROW_H, m_rightW, ROW_H};

    // Info-Zeile unter den Feldern
    m_infoY = TOP_GAP + FIELDS_H + INFO_GAP_V;

    // Numpad am unteren Rand
    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);

    // Schaltplan-Touch-Areas
    // Vref -> Tap auf LM317-Gehäuse
    m_compHits[0] = {PKG_X - HIT_PAD, TOP_GAP + PKG_Y - HIT_PAD, PKG_W + 2 * HIT_PAD, PKG_H + 2 * HIT_PAD};
    // R1
    m_compHits[1] = {CHAIN_CX - COMP_W / 2 - HIT_PAD, TOP_GAP + R1_TOP - HIT_PAD, COMP_W + 2 * HIT_PAD, COMP_H + 2 * HIT_PAD};
    // R2
    m_compHits[2] = {CHAIN_CX - COMP_W / 2 - HIT_PAD, TOP_GAP + R2_TOP - HIT_PAD, COMP_W + 2 * HIT_PAD, COMP_H + 2 * HIT_PAD};
    // Vout -> Tap auf Vout-Beschriftungsbereich
    m_compHits[3] = {OUT_PIN_X, TOP_GAP + PKG_Y - HIT_PAD, SCHEM_W - OUT_PIN_X, PKG_H / 2 + 2 * HIT_PAD};
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch mit FIFO-3-Logik für Felder und Schaltplan-Bauteile.
 */
bool WorkshopTool_Lm317::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        // Schaltplan
        if (cx < SCHEM_W)
        {
            const int hitComp = hitTestComponent(cx, cy);
            if (hitComp >= 0 && hitComp != m_pressedComp)
            {
                m_pressedComp = hitComp;
                m_pressedField = -1;
                m_pressedKey = -1;
                needsRedraw = true;
            }
        }
        else if (cx >= m_rightX)
        {
            // Eingabefelder
            const int hitField = hitTestField(cx, cy);
            if (hitField >= 0)
            {
                if (hitField != m_pressedField)
                {
                    m_pressedField = hitField;
                    m_pressedComp = -1;
                    m_pressedKey = -1;
                    needsRedraw = true;
                }
            }
            else
            {
                // Numpad
                const int hitKey = m_numpad.hitTest(cx - m_rightX, cy);
                if (hitKey >= 0 && hitKey != m_pressedKey)
                {
                    m_pressedKey = hitKey;
                    m_pressedField = -1;
                    m_pressedComp = -1;
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
            // Tap auf Bauteil oder Feld -> gemeinsame FIFO-Logik
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
                    // FIFO: ältester Slot wird berechnet, tapped wird Eingabe
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

        m_pressedField = -1;
        m_pressedComp = -1;
        m_pressedKey = -1;
        m_touchWas = false;
    }

    return needsRedraw;
}

// ===================================================================
// Zeichnen - Schaltplan
// ===================================================================

/**
 * @brief Zeichnet den Schaltplan mit LM317-Gehäuse, R1/R2 und Verdrahtung.
 */
void WorkshopTool_Lm317::drawSchematic()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    // Hintergrund (ab TOP_GAP)
    spr->fillRect(ox, oy, SCHEM_W, m_h - TOP_GAP, C_SCHEM_BG);

    // GND-Schiene
    spr->drawFastHLine(ox + 15, oy + GND_RAIL_Y, SCHEM_W - 30, C_WIRE);

    // ===================================================================
    // Verdrahtung
    // ===================================================================

    // Vin-Draht -> IN-Pin
    spr->drawFastHLine(ox + 15, oy + IN_PIN_Y, PKG_X - 15, C_WIRE);

    // OUT-Pin -> Vout-Knotenpunkt -> Vout-Abgang weiter nach rechts
    spr->drawFastHLine(ox + OUT_PIN_X, oy + IN_PIN_Y, CHAIN_CX - OUT_PIN_X + 25, C_WIRE);

    // Vout-Linie nach unten zu R1
    spr->drawFastVLine(ox + CHAIN_CX, oy + IN_PIN_Y, R1_TOP - IN_PIN_Y, C_WIRE);

    // R1 nach unten zum Knotenpunkt
    spr->drawFastVLine(ox + CHAIN_CX, oy + R1_BOT, JUNCT_Y - R1_BOT, C_WIRE);

    // ADJ-Pin nach unten
    spr->drawFastVLine(ox + ADJ_PIN_X, oy + ADJ_PIN_Y, JUNCT_Y - ADJ_PIN_Y, C_WIRE);

    // ADJ horizontal zum Knotenpunkt
    spr->drawFastHLine(ox + ADJ_PIN_X, oy + JUNCT_Y, CHAIN_CX - ADJ_PIN_X, C_WIRE);

    // Knotenpunkt nach unten zu R2
    spr->drawFastVLine(ox + CHAIN_CX, oy + JUNCT_Y, R2_TOP - JUNCT_Y, C_WIRE);

    // R2 nach unten zu GND
    spr->drawFastVLine(ox + CHAIN_CX, oy + R2_BOT, GND_RAIL_Y - R2_BOT, C_WIRE);

    // ===================================================================
    // Knotenpunkte
    // ===================================================================

    spr->fillCircle(ox + CHAIN_CX, oy + IN_PIN_Y, 3, C_JUNCTION);   // Vout-Abzweig
    spr->fillCircle(ox + CHAIN_CX, oy + JUNCT_Y, 3, C_JUNCTION);    // R1/R2/ADJ
    spr->fillCircle(ox + CHAIN_CX, oy + GND_RAIL_Y, 3, C_JUNCTION); // GND-Anschluss

    // ===================================================================
    // Bauteile
    // ===================================================================

    const bool r1Active = (static_cast<int>(m_focusedQty) == 1);
    const bool r2Active = (static_cast<int>(m_focusedQty) == 2);

    drawResistor(CHAIN_CX, R1_TOP, COMP_H, r1Active);
    drawResistor(CHAIN_CX, R2_TOP, COMP_H, r2Active);

    // LM317-Gehäuse
    drawPackage(PKG_X, PKG_Y, PKG_W, PKG_H);

    // ===================================================================
    // Vref-Doppelpfeil (rechts neben R1, zwischen Vout und Knotenpunkt)
    // ===================================================================

    const int arrowX = ox + VREF_ARROW_X;
    const int arrowTop = oy + IN_PIN_Y;
    const int arrowBot = oy + JUNCT_Y;

    // Vertikale Linie
    spr->drawFastVLine(arrowX, arrowTop + 6, arrowBot - arrowTop - 12, C_VAL_DIMMED);
    // Pfeilspitze oben (nach oben)
    spr->fillTriangle(arrowX, arrowTop + 4, arrowX - 3, arrowTop + 10, arrowX + 3, arrowTop + 10, C_VAL_DIMMED);
    // Pfeilspitze unten (nach unten)
    spr->fillTriangle(arrowX, arrowBot - 4, arrowX - 3, arrowBot - 10, arrowX + 3, arrowBot - 10, C_VAL_DIMMED);

    // ===================================================================
    // Beschriftungen
    // ===================================================================

    spr->loadFont(ComfortaaBold14);

    // Vin / Vout
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_RAIL, C_SCHEM_BG);
    spr->drawString("Vin", ox + 5, oy + IN_PIN_Y - 14);

    const bool voutActive = (static_cast<int>(m_focusedQty) == 3);
    spr->setTextColor(voutActive ? C_VAL_ACTIVE : C_RAIL, C_SCHEM_BG);
    spr->drawString("Vout", ox + CHAIN_CX + 6, oy + IN_PIN_Y - 14);

    // GND
    spr->setTextColor(C_RAIL, C_SCHEM_BG);
    spr->drawString("GND", ox + 5, oy + GND_RAIL_Y);

    // Bauteil-Labels und Werte
    struct CompLabel
    {
        int qty;
        int cy;
        const char *label;
    };

    const CompLabel labels[] = {
        {1, (R1_TOP + R1_BOT) / 2, "R1"},
        {2, (R2_TOP + R2_BOT) / 2, "R2"},
    };

    for (const auto &cl : labels)
    {
        const bool active = (static_cast<int>(m_focusedQty) == cl.qty);
        const uint16_t clr = active ? C_VAL_ACTIVE : C_VAL_DIMMED;

        // Label links vom Bauteil
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(clr, C_SCHEM_BG);
        spr->drawString(cl.label, ox + CHAIN_CX - COMP_W / 2 - 6, oy + cl.cy);

        // Wert rechts vom Bauteil
        char valBuf[16];
        formatSchematicValue(cl.qty, valBuf, sizeof(valBuf));
        spr->setTextDatum(ML_DATUM);
        spr->drawString(valBuf, ox + CHAIN_CX + COMP_W / 2 + 4, oy + cl.cy);
    }

    // Vref-Beschriftung neben dem Doppelpfeil
    const bool vrefActive = (static_cast<int>(m_focusedQty) == 0);
    const uint16_t vrefClr = vrefActive ? C_VAL_ACTIVE : C_VAL_DIMMED;
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(vrefClr, C_SCHEM_BG);

    char vrefBuf[16];
    formatSchematicValue(0, vrefBuf, sizeof(vrefBuf));
    const int vrefCy = (arrowTop + arrowBot) / 2;
    spr->drawString("Vref", arrowX, vrefCy - 8);
    spr->drawString(vrefBuf, arrowX, vrefCy + 8);

    spr->unloadFont();
}

/**
 * @brief Zeichnet das LM317-Gehäuse.
 */
void WorkshopTool_Lm317::drawPackage(int pkgX, int pkgY, int pkgW, int pkgH)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ax = m_originX + pkgX;
    const int ay = m_originY + TOP_GAP + pkgY;

    // 2x fillSmoothRoundRect für abgerundetes Rechteck
    spr->fillSmoothRoundRect(ax, ay, pkgW, pkgH, PKG_R, C_IC_OUTLINE);
    spr->fillSmoothRoundRect(ax + 2, ay + 2, pkgW - 4, pkgH - 4, PKG_R, C_IC_BODY);

    // Kühlfahne oben (TO-220-Stil)
    spr->fillRect(ax + 10, ay, pkgW - 20, 10, C_IC_OUTLINE);

    // "LM317" zentriert
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_IC_TEXT, C_IC_BODY);
    spr->drawString("LM317", ax + pkgW / 2, ay + pkgH / 2);

    // Pin-Labels
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_PIN_LABEL, C_IC_BODY);
    spr->drawString("IN", ax + 6, ay + pkgH / 2);

    spr->setTextDatum(MR_DATUM);
    spr->drawString("OUT", ax + pkgW - 6, ay + pkgH / 2);

    spr->setTextDatum(MC_DATUM);
    spr->drawString("ADJ", ax + pkgW / 2, ay + pkgH - 10);

    spr->unloadFont();
}

/**
 * @brief Zeichnet ein Widerstands-Symbol.
 */
void WorkshopTool_Lm317::drawResistor(int cx, int top, int h, bool active)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ax = m_originX + cx - COMP_W / 2;
    const int ay = m_originY + TOP_GAP + top;
    const uint16_t fill = active ? 0x1A45 : C_COMP_BODY;
    const uint16_t outline = active ? C_VAL_ACTIVE : C_COMP_OUTLINE;

    spr->fillRect(ax, ay, COMP_W, h, fill);
    spr->drawRect(ax, ay, COMP_W, h, outline);
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle vier Eingabefelder.
 */
void WorkshopTool_Lm317::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);

    for (int i = 0; i < NUM_QTY; ++i)
        drawField(i, m_pressedField == i || m_pressedComp == i, cursorOn);

    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Größen-Zeile (Symbol, Wert, Einheit).
 */
void WorkshopTool_Lm317::drawField(int qty, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const Rect &fr = m_fieldRects[qty];
    const int ax = m_originX + fr.x;
    const int ay = m_originY + fr.y;

    const bool input = isInput(qty);
    const bool focused = (static_cast<int>(m_focusedQty) == qty);

    uint16_t bgColor;
    if (pressed)
        bgColor = C_FIELD_PRESS;
    else if (focused)
        bgColor = C_FIELD_FOCUS;
    else
        bgColor = C_FIELD_INPUT;

    spr->fillRect(ax, ay, fr.w, fr.h, bgColor);
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, accentColor(qty));

    // Textfarben für Symbol und Einheit
    uint16_t lblClr, unitClr;
    if (focused)
    {
        lblClr = C_LBL_FOCUS;
        unitClr = C_UNIT_FOCUS;
    }
    else if (input)
    {
        lblClr = C_LBL_INPUT;
        unitClr = C_UNIT_INPUT;
    }
    else
    {
        lblClr = C_LBL_COMP;
        unitClr = C_UNIT_COMP;
    }

    const int cy = ay + fr.h / 2;

    // Symbol (Vref / R1 / R2 / Vout)
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblClr, bgColor);
    spr->drawString(QTY_SYMBOLS[qty], ax + ACCENT_W + 6, cy);

    // Einheit
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(unitClr, bgColor);
    spr->drawString(QTY_UNITS[qty], ax + fr.w - UNIT_W / 2, cy);

    // Wert
    char displayBuf[32];
    const int valRightX = ax + fr.w - UNIT_W - 6;
    spr->setTextDatum(MR_DATUM);

    if (focused)
    {
        // Eingabepuffer hält weiterhin '.' für atof - Komma nur in der Anzeige
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), m_inputBuf[qty]);
        dotToComma(displayBuf);
        stringutils::safeAppend(displayBuf, sizeof(displayBuf), "_");
        spr->setTextColor(C_VAL_FOCUS, bgColor);
        spr->drawString(displayBuf, valRightX, cy);

        if (!cursorOn)
        {
            const int cursorW = spr->textWidth("_");
            spr->fillRect(valRightX - cursorW, ay + 1, cursorW, fr.h - 2, bgColor);
        }
    }
    else if (input)
    {
        formatDisplayValue(m_values[qty], qty, displayBuf, sizeof(displayBuf));
        spr->setTextColor(C_VAL_INPUT, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
    else
    {
        if (m_computeOk)
            formatDisplayValue(m_values[qty], qty, displayBuf, sizeof(displayBuf));
        else
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");
        spr->setTextColor(C_VAL_COMP, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}

/**
 * @brief Zeichnet den Info-Bereich (Formel + E24-Normwert).
 */
void WorkshopTool_Lm317::drawInfoRow()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_infoY;
    const int rw = m_rightW;
    const int cy = oy + INFO_H / 2;

    spr->fillRect(ox, oy, rw, INFO_H, C_BG);
    spr->loadFont(ComfortaaBold14);

    // Formel linksbündig
    char formulaBuf[INFO_STR_CAP];
    buildFormulaStr(formulaBuf, sizeof(formulaBuf));
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_FORMULA_TXT, C_BG);
    spr->drawString(formulaBuf, ox + 4, cy);

    // E24-Normwert rechts (nur wenn R1 oder R2 berechnet)
    if (m_computeOk && (m_computedQty == 1 || m_computedQty == 2))
    {
        char e24Buf[INFO_STR_CAP];
        formatE24(m_values[m_computedQty], e24Buf, sizeof(e24Buf));
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_INFO_TXT, C_BG);
        spr->drawString(e24Buf, ox + rw - 4, cy);
    }

    spr->unloadFont();
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf Eingabefelder.
 */
int WorkshopTool_Lm317::hitTestField(int cx, int cy) const
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
 * @brief Treffertest auf Schaltplan-Bauteile.
 */
int WorkshopTool_Lm317::hitTestComponent(int cx, int cy) const
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
 * @brief Parst Eingabepuffer und berechnet die Ausgabegröße.
 */
void WorkshopTool_Lm317::compute()
{
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
    {
        const int q = static_cast<int>(m_inputSlots[s]);
        m_values[q] = (m_inputBuf[q][0] != '\0') ? atof(m_inputBuf[q]) : 0.0;
    }

    m_computeOk = false;

    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
    {
        if (!std::isfinite(m_values[static_cast<int>(m_inputSlots[s])]))
            return;
    }

    const double vref = m_values[0];
    const double r1 = m_values[1];
    const double r2 = m_values[2];
    const double vout = m_values[3];

    switch (m_computedQty)
    {
    case 3: // Vout = Vref * (1 + R2/R1)
    {
        if (r1 < 1e-9)
            return;

        const double result = vref * (1.0 + r2 / r1);
        if (!std::isfinite(result) || result < 0.0)
            return;

        m_values[3] = result;
        m_computeOk = true;
        break;
    }
    case 0: // Vref = Vout * R1 / (R1 + R2)
    {
        const double denom = r1 + r2;

        if (denom < 1e-9)
            return;

        const double result = vout * r1 / denom;
        if (!std::isfinite(result) || result < 0.0)
            return;

        m_values[0] = result;
        m_computeOk = true;
        break;
    }
    case 1: // R1 = Vref * R2 / (Vout - Vref)
    {
        if (r2 < 0.0)
            return;

        const double denom = vout - vref;
        if (std::abs(denom) < 1e-15)
            return;

        const double result = vref * r2 / denom;
        if (!std::isfinite(result) || result < 0.0)
            return;

        m_values[1] = result;
        m_computeOk = true;
        break;
    }
    case 2: // R2 = R1 * (Vout / Vref - 1)
    {
        if (r1 < 0.0 || std::abs(vref) < 1e-15)
            return;

        const double result = r1 * (vout / vref - 1.0);
        if (!std::isfinite(result) || result < 0.0)
            return;

        m_values[2] = result;
        m_computeOk = true;
        break;
    }
    default:
        break;
    }
}

// ===================================================================
// Formatierung
// ===================================================================

/**
 * @brief Formatiert SI-Wert als auto-skalierten Anzeigestring (mit Komma).
 */
void WorkshopTool_Lm317::formatDisplayValue(double val, int qty,
                                            char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    const double absVal = std::abs(val);
    if (absVal == 0.0)
    {
        stringutils::safeFormat(buf, sz, "0 %s", QTY_UNITS[qty]);
        return;
    }

    double scaled;
    const char *prefix;
    const char *unit = QTY_UNITS[qty];
    const bool isResistance = (qty == 1 || qty == 2);

    if (isResistance)
    {
        if (absVal < 1.0e3)
        {
            scaled = val;
            prefix = "";
        }
        else if (absVal < 1.0e6)
        {
            scaled = val / 1.0e3;
            prefix = "k";
        }
        else if (absVal < 1.0e9)
        {
            scaled = val / 1.0e6;
            prefix = "M";
        }
        else
        {
            scaled = val / 1.0e9;
            prefix = "G";
        }
    }
    else
    {
        if (absVal < 1.0e-3)
        {
            scaled = val * 1.0e6;
            prefix = "u";
        }
        else if (absVal < 1.0)
        {
            scaled = val * 1.0e3;
            prefix = "m";
        }
        else if (absVal < 1.0e3)
        {
            scaled = val;
            prefix = "";
        }
        else if (absVal < 1.0e6)
        {
            scaled = val / 1.0e3;
            prefix = "k";
        }
        else
        {
            scaled = val / 1.0e6;
            prefix = "M";
        }
    }

    stringutils::safeFormat(buf, sz, "%.4g %s%s", scaled, prefix, unit);
    dotToComma(buf);
}

/**
 * @brief Formatiert SI-Wert als editierbaren Dezimalstring (Punkt für atof).
 */
void WorkshopTool_Lm317::formatEditValue(double val, char *buf, size_t sz)
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
 * @brief Formelstring für die berechnete Größe.
 */
void WorkshopTool_Lm317::buildFormulaStr(char *buf, size_t sz) const
{
    switch (m_computedQty)
    {
    case 3:
        stringutils::safeCopy(buf, sz, "Vout=Vref*(1+R2/R1)");
        break;
    case 0:
        stringutils::safeCopy(buf, sz, "Vref=Vout*R1/(R1+R2)");
        break;
    case 1:
        stringutils::safeCopy(buf, sz, "R1=Vref*R2/(Vout-Vref)");
        break;
    case 2:
        stringutils::safeCopy(buf, sz, "R2=R1*(Vout/Vref-1)");
        break;
    default:
        stringutils::safeCopy(buf, sz, "");
        break;
    }
}

/**
 * @brief Prüft ob eine Größe als Eingabe aktiv ist.
 */
bool WorkshopTool_Lm317::isInput(int qty) const
{
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
    {
        if (static_cast<int>(m_inputSlots[s]) == qty)
            return true;
    }

    return false;
}

/**
 * @brief Akzentfarbe je Größe.
 */
uint16_t WorkshopTool_Lm317::accentColor(int qty)
{
    switch (qty)
    {
    case 0:
        return C_ACCENT_VREF;
    case 1:
        return C_ACCENT_R1;
    case 2:
        return C_ACCENT_R2;
    case 3:
        return C_ACCENT_VOUT;
    default:
        return 0xFFFF;
    }
}

/**
 * @brief Nächster E24-Normwiderstand.
 */
double WorkshopTool_Lm317::nearestE24(double r)
{
    if (r <= 0.0 || !std::isfinite(r))
        return 0.0;

    const double expn = floor(log10(r));
    const double decade = pow(10.0, expn);

    double bestVal = E24_VALUES[0] * decade;
    double bestDiff = fabs(r - bestVal);

    for (int i = 1; i < E24_COUNT; ++i)
    {
        const double candidate = E24_VALUES[i] * decade;
        const double diff = fabs(r - candidate);
        if (diff < bestDiff)
        {
            bestDiff = diff;
            bestVal = candidate;
        }
    }

    const double nextFirst = decade * 10.0;
    if (fabs(r - nextFirst) < bestDiff)
        bestVal = nextFirst;

    return bestVal;
}

/**
 * @brief Formatiert E24-Normwert als String (mit Komma).
 */
void WorkshopTool_Lm317::formatE24(double r, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(r) || r <= 0.0)
    {
        stringutils::safeCopy(buf, sz, "E24: ---");
        return;
    }

    const double e24r = nearestE24(r);
    const double absE24 = fabs(e24r);

    double scaled;
    const char *prefix;

    if (absE24 < 1.0e3)
    {
        scaled = e24r;
        prefix = "";
    }
    else if (absE24 < 1.0e6)
    {
        scaled = e24r / 1.0e3;
        prefix = "k";
    }
    else if (absE24 < 1.0e9)
    {
        scaled = e24r / 1.0e6;
        prefix = "M";
    }
    else
    {
        scaled = e24r / 1.0e9;
        prefix = "G";
    }

    char valBuf[24];
    stringutils::safeFormat(valBuf, sizeof(valBuf), "%.4g", scaled);
    stringutils::safeFormat(buf, sz, "E24: %s %sOhm", valBuf, prefix);
    dotToComma(buf);
}

// ===================================================================
// Tastatureingabe
// ===================================================================

/**
 * @brief Führt eine Tastenaktion auf dem Eingabepuffer aus.
 */
void WorkshopTool_Lm317::executeKey(char action)
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
    {
        if (len + 1 >= FIELD_CAP)
            break;

        buf[len] = action;
        buf[len + 1] = '\0';
        break;
    }
    case '.':
    {
        if (strchr(buf, '.') != nullptr)
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
    case 'S':
    {
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
    }
    default:
        break;
    }

    compute();
}

// ===================================================================
// Schaltplan-Wertformatierung
// ===================================================================

/**
 * @brief Formatiert einen Bauteilwert kompakt für die Schaltplan-Beschriftung.
 */
void WorkshopTool_Lm317::formatSchematicValue(int qty, char *buf, size_t sz) const
{
    if (!buf || sz == 0)
        return;

    const double val = m_values[qty];
    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "?");
        return;
    }

    const bool isResistance = (qty == 1 || qty == 2);

    if (isResistance)
    {
        const double absVal = std::abs(val);
        if (absVal == 0.0)
            stringutils::safeCopy(buf, sz, "0");
        else if (absVal >= 1.0e6)
            stringutils::safeFormat(buf, sz, "%.3gM", val / 1.0e6);
        else if (absVal >= 1.0e3)
            stringutils::safeFormat(buf, sz, "%.3gk", val / 1.0e3);
        else
            stringutils::safeFormat(buf, sz, "%.3g", val);
    }
    else
    {
        // Spannung (Vref, Vout) - kompakt mit V-Suffix
        stringutils::safeFormat(buf, sz, "%.3gV", val);
    }

    dotToComma(buf);
}
