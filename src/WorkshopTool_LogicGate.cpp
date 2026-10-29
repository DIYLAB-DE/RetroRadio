#include "WorkshopTool_LogicGate.h"

#include <cstring>

#include "I18n.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold18.h"

// ===================================================================
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================

namespace {

// ===================================================================
// Farben - Selector-Chips
// ===================================================================

constexpr uint16_t C_BG           = 0x1082; ///< Hintergrund
constexpr uint16_t C_CHIP_SEL     = 0x328A; ///< Gewählter Chip
constexpr uint16_t C_CHIP_UNSEL   = 0x2104; ///< Nicht gewählter Chip
constexpr uint16_t C_CHIP_PRESS   = 0x4B4D; ///< Gedrückter Chip
constexpr uint16_t C_CHIP_TXT_SEL = 0xFFFF; ///< Text gewählt
constexpr uint16_t C_CHIP_TXT_UN  = 0x8410; ///< Text nicht gewählt

// ===================================================================
// Farben - DIN-Gatter-Symbol
// ===================================================================

constexpr uint16_t C_GATE_BODY    = 0x2945; ///< Gatter-Gehäuse Füllung
constexpr uint16_t C_GATE_OUTLINE = 0x4A69; ///< Gatter-Gehäuse Umriss
constexpr uint16_t C_GATE_TEXT    = 0xDEDB; ///< Funktionstext im Gatter
constexpr uint16_t C_WIRE        = 0x8410; ///< Leitungsfarbe
constexpr uint16_t C_NEG_CIRCLE  = 0x4A69; ///< Negationskreis Umriss

// ===================================================================
// Farben - Eingänge / Ausgang
// ===================================================================

constexpr uint16_t C_HIGH        = 0x07E0; ///< HIGH (1) - Grün
constexpr uint16_t C_LOW         = 0x4228; ///< LOW (0) - gedimmt
constexpr uint16_t C_LABEL_DIM   = 0x7D7A; ///< Eingangsbezeichnung gedimmt

// ===================================================================
// Farben - Wahrheitstabelle
// ===================================================================

constexpr uint16_t C_TBL_HDR_BG  = 0x2104; ///< Kopfzeile Hintergrund
constexpr uint16_t C_TBL_HDR_TXT = 0xDEDB; ///< Kopfzeile Text
constexpr uint16_t C_TBL_ROW0    = 0x18C3; ///< Datenzeile gerade
constexpr uint16_t C_TBL_ROW1    = 0x1082; ///< Datenzeile ungerade
constexpr uint16_t C_TBL_ACTIVE  = 0x328A; ///< Aktive Zeile (Petrol)
constexpr uint16_t C_TBL_VAL_1   = 0x07E0; ///< Ausgangswert "1" (Grün)
constexpr uint16_t C_TBL_VAL_0   = 0x7D7A; ///< Ausgangswert "0" (gedimmt)
constexpr uint16_t C_TBL_IN_TXT  = 0xDEDB; ///< Eingangswert Text

// ===================================================================
// Eingangstoggle-Darstellung
// ===================================================================

constexpr int TOGGLE_W = 36; ///< Breite des Toggle-Rechtecks
constexpr int TOGGLE_H = 22; ///< Höhe des Toggle-Rechtecks
constexpr int TOGGLE_R = 4;  ///< Eckenradius

// ===================================================================
// Schaltplan - Geometrie
// ===================================================================

constexpr int TOP_GAP = 10; ///< Abstand Selector -> Schaltplan-Inhalt

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

const char *const WorkshopTool_LogicGate::GATE_LABELS[NUM_GATE_TYPES] = {
    "AND", "OR", "XOR", "NAND", "NOR", "NOT"
};

const char *const WorkshopTool_LogicGate::GATE_SYMBOLS[NUM_GATE_TYPES] = {
    "&", ">=1", "=1", "&", ">=1", "1"
};

const WorkshopKey WorkshopTool_LogicGate::GATE_DESCRIPTION_KEYS[NUM_GATE_TYPES] = {
    WorkshopKey::WORKSHOP_LOGIC_GATE_DESC_AND,
    WorkshopKey::WORKSHOP_LOGIC_GATE_DESC_OR,
    WorkshopKey::WORKSHOP_LOGIC_GATE_DESC_XOR,
    WorkshopKey::WORKSHOP_LOGIC_GATE_DESC_NAND,
    WorkshopKey::WORKSHOP_LOGIC_GATE_DESC_NOR,
    WorkshopKey::WORKSHOP_LOGIC_GATE_DESC_NOT
};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_LogicGate::WorkshopTool_LogicGate()
{
    clearAll();
}

/**
 * @brief Setzt den Simulator beim Modul-Einstieg zurück.
 */
void WorkshopTool_LogicGate::onEnter(LGFX *tft)
{
    (void)tft;
    clearAll();
}

/**
 * @brief Aufräumen beim Verlassen.
 */
void WorkshopTool_LogicGate::onExit()
{
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_LogicGate::clearAll()
{
    memset(m_chipRects, 0, sizeof(m_chipRects));
    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));

    m_sprite      = nullptr;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;
    m_layoutValid = false;

    m_gateType    = GateType::AND;
    m_inputA      = false;
    m_inputB      = false;

    m_schemX = 0;
    m_schemY = 0;
    m_schemW = 0;
    m_schemH = 0;
    m_tableX = 0;
    m_tableY = 0;
    m_tableW = 0;
    m_tableH = 0;

    m_inputTouchA = {};
    m_inputTouchB = {};

    m_pressedChip  = -1;
    m_pressedInput = -1;
    m_touchWas     = false;
    m_touchReady   = false;

    refreshSubtitle();
}

// ===================================================================
// Logik
// ===================================================================

/**
 * @brief Berechnet den Ausgang eines Gatters.
 */
bool WorkshopTool_LogicGate::computeGate(GateType type, bool a, bool b)
{
    switch (type)
    {
    case GateType::AND:  return a && b;
    case GateType::OR:   return a || b;
    case GateType::XOR:  return a ^ b;
    case GateType::NAND: return !(a && b);
    case GateType::NOR:  return !(a || b);
    case GateType::NOT:  return !a;
    default:             return false;
    }
}

/**
 * @brief Gibt die Eingangszahl für den Gattertyp zurück.
 */
int WorkshopTool_LogicGate::getNumInputs(GateType type)
{
    return (type == GateType::NOT) ? 1 : 2;
}

/**
 * @brief Prüft ob der Gattertyp einen Negationskreis hat.
 */
bool WorkshopTool_LogicGate::hasNegation(GateType type)
{
    return (type == GateType::NAND || type == GateType::NOR || type == GateType::NOT);
}

/**
 * @brief Dynamischer Untertitel je nach Gattertyp.
 *
 * Liefert einen stabilen Pointer auf m_subtitleBuf, der bei jeder m_gateType-Änderung in handleTouch() (und einmal in clearAll()) per refreshSubtitle() befüllt wird. Workshop ruft getSubtitle() pro Redraw, daher hier keine Formatierung.
 */
const char *WorkshopTool_LogicGate::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Befüllt m_subtitleBuf mit der lokalisierten Beschreibung von m_gateType.
 */
void WorkshopTool_LogicGate::refreshSubtitle()
{
    const int idx = static_cast<int>(m_gateType);
    if (idx >= 0 && idx < NUM_GATE_TYPES)
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), GATE_DESCRIPTION_KEYS[idx]);
    else
        m_subtitleBuf[0] = '\0';
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Selector-Chips, Schaltplan- und Tabellenbereich.
 */
void WorkshopTool_LogicGate::computeLayout(int w, int h)
{
    // Selector-Chips (volle Breite)
    const int totalGap = (NUM_GATE_TYPES - 1) * CHIP_GAP;
    const int chipW = (w - totalGap) / NUM_GATE_TYPES;
    int cx = 0;

    for (int i = 0; i < NUM_GATE_TYPES; ++i)
    {
        const int cw = (i == NUM_GATE_TYPES - 1) ? (w - cx) : chipW;
        m_chipRects[i] = {cx, CHIP_TOP, cw, CHIP_H};
        cx += cw + CHIP_GAP;
    }

    // Schaltplan (links, unter Selector)
    m_schemX = 0;
    m_schemY = SEL_ROW_H;
    m_schemW = SCHEM_W;
    m_schemH = h - SEL_ROW_H;

    // Wahrheitstabelle (rechts, unter Selector)
    m_tableX = SCHEM_W + COL_GAP;
    m_tableY = SEL_ROW_H;
    m_tableW = w - m_tableX;
    m_tableH = h - SEL_ROW_H;
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Selector, Schaltplan und Wahrheitstabelle.
 */
void WorkshopTool_LogicGate::draw(Gfx_Sprite *sprite,
                                   int originX, int originY,
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

    drawSelector();
    drawSchematic();
    drawTruthTable();
}

// ===================================================================
// Zeichnen - Selector-Chips
// ===================================================================

/**
 * @brief Zeichnet die Gattertyp-Auswahl-Chips.
 */
void WorkshopTool_LogicGate::drawSelector()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;
    const int selIdx = static_cast<int>(m_gateType);

    // Hintergrund der Selector-Zeile
    spr->fillRect(ox, oy, m_w, SEL_ROW_H, C_BG);

    spr->loadFont(ComfortaaBold14);

    for (int i = 0; i < NUM_GATE_TYPES; ++i)
    {
        const Rect &r = m_chipRects[i];
        const bool sel = (i == selIdx);
        const bool pressed = (i == m_pressedChip);

        uint16_t bg, fg;
        if (pressed)     { bg = C_CHIP_PRESS;  fg = C_CHIP_TXT_SEL; }
        else if (sel)    { bg = C_CHIP_SEL;     fg = C_CHIP_TXT_SEL; }
        else             { bg = C_CHIP_UNSEL;   fg = C_CHIP_TXT_UN;  }

        spr->fillSmoothRoundRect(ox + r.x, oy + r.y, r.w, r.h, CHIP_R, bg);

        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(fg, bg);
        spr->drawString(GATE_LABELS[i], ox + r.x + r.w / 2, oy + r.y + r.h / 2);
    }

    spr->unloadFont();
}

// ===================================================================
// Zeichnen - DIN-Schaltplan
// ===================================================================

/**
 * @brief Zeichnet das DIN-Gatter-Rechteck mit Funktionstext (18 px).
 */
void WorkshopTool_LogicGate::drawGateBody(int gx, int gy)
{
    Gfx_Sprite *const spr = m_sprite;

    // Outline-Trick: 2× fillSmoothRoundRect
    spr->fillSmoothRoundRect(gx - 1, gy - 1, GATE_W + 2, GATE_H + 2,
                             GATE_R + 1, C_GATE_OUTLINE);
    spr->fillSmoothRoundRect(gx, gy, GATE_W, GATE_H, GATE_R, C_GATE_BODY);

    // Funktionstext zentriert (18 px)
    const int idx = static_cast<int>(m_gateType);
    spr->loadFont(ComfortaaBold18);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_GATE_TEXT, C_GATE_BODY);
    spr->drawString(GATE_SYMBOLS[idx], gx + GATE_W / 2, gy + GATE_H / 2);
    spr->unloadFont();
}

/**
 * @brief Zeichnet den DIN-Schaltplan mit Eingängen und Ausgang.
 */
void WorkshopTool_LogicGate::drawSchematic()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_schemX;
    const int oy = m_originY + m_schemY;

    // Hintergrund
    spr->fillRect(ox, oy, m_schemW, m_schemH, C_BG);

    const int numIn = getNumInputs(m_gateType);
    const bool negated = hasNegation(m_gateType);

    // Gatter positionieren: Toggle beginnt bei ox (= 10 px vom Sprite-Rand)
    const int contentH = m_schemH - TOP_GAP;
    const int gateX = ox + TOGGLE_W + 6 + WIRE_LEN;
    const int gateY = oy + TOP_GAP + (contentH - GATE_H) / 2;

    // Gatter-Rechteck + Funktionstext
    drawGateBody(gateX, gateY);

    // Eingangsleitungen + Toggles
    const int wireStartX = gateX - WIRE_LEN;

    if (numIn == 1)
    {
        // NOT: Ein Eingang in der Mitte
        const int inY = gateY + GATE_H / 2;
        spr->drawFastHLine(wireStartX, inY, WIRE_LEN, C_WIRE);

        // Toggle-Rechteck
        const uint16_t colA = m_inputA ? C_HIGH : C_LOW;
        const int togX = wireStartX - TOGGLE_W - 6;
        const int togY = inY - TOGGLE_H / 2;
        spr->fillSmoothRoundRect(togX, togY, TOGGLE_W, TOGGLE_H, TOGGLE_R, colA);

        // "0"/"1" im Toggle + Label "A" im Gatter
        spr->loadFont(ComfortaaBold14);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(0x0000, colA);
        spr->drawString(m_inputA ? "1" : "0", togX + TOGGLE_W / 2, togY + TOGGLE_H / 2);

        // Label "A" im Gatter (links innen, Höhe Eingang)
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_LABEL_DIM, C_GATE_BODY);
        spr->drawString("A", gateX + 8, inY);
        spr->unloadFont();

        // Touch-Bereich A (content-relativ)
        m_inputTouchA = {
            togX - m_originX - m_schemX - 10,
            togY - m_originY - m_schemY - 10,
            TOGGLE_W + 20,
            TOGGLE_H + 20
        };
        m_inputTouchB = {};
    }
    else
    {
        // 2 Eingänge: fester Abstand vom oberen/unteren Gatter-Rand
        const int inYA = gateY + INPUT_OFFSET;
        const int inYB = gateY + GATE_H - INPUT_OFFSET;

        // Leitung A
        spr->drawFastHLine(wireStartX, inYA, WIRE_LEN, C_WIRE);
        // Leitung B
        spr->drawFastHLine(wireStartX, inYB, WIRE_LEN, C_WIRE);

        // Toggle A
        const uint16_t colA = m_inputA ? C_HIGH : C_LOW;
        const int togAX = wireStartX - TOGGLE_W - 6;
        const int togAY = inYA - TOGGLE_H / 2;
        spr->fillSmoothRoundRect(togAX, togAY, TOGGLE_W, TOGGLE_H, TOGGLE_R, colA);

        spr->loadFont(ComfortaaBold14);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(0x0000, colA);
        spr->drawString(m_inputA ? "1" : "0", togAX + TOGGLE_W / 2, togAY + TOGGLE_H / 2);

        // Toggle B
        const uint16_t colB = m_inputB ? C_HIGH : C_LOW;
        const int togBX = wireStartX - TOGGLE_W - 6;
        const int togBY = inYB - TOGGLE_H / 2;
        spr->fillSmoothRoundRect(togBX, togBY, TOGGLE_W, TOGGLE_H, TOGGLE_R, colB);

        spr->setTextColor(0x0000, colB);
        spr->drawString(m_inputB ? "1" : "0", togBX + TOGGLE_W / 2, togBY + TOGGLE_H / 2);

        // Labels "A" und "B" im Gatter (links innen)
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_LABEL_DIM, C_GATE_BODY);
        spr->drawString("A", gateX + 8, inYA);
        spr->drawString("B", gateX + 8, inYB);
        spr->unloadFont();

        // Touch-Bereiche (content-relativ)
        m_inputTouchA = {
            togAX - m_originX - m_schemX - 10,
            togAY - m_originY - m_schemY - 10,
            TOGGLE_W + 20,
            TOGGLE_H + 20
        };
        m_inputTouchB = {
            togBX - m_originX - m_schemX - 10,
            togBY - m_originY - m_schemY - 10,
            TOGGLE_W + 20,
            TOGGLE_H + 20
        };
    }

    // Ausgangsleitung
    const int outStartX = gateX + GATE_W;
    const int outY = gateY + GATE_H / 2;
    const int outEndX = gateX + GATE_W + WIRE_LEN;

    if (negated)
    {
        // Negationskreis am Ausgang
        const int circleX = outStartX + NEG_CIRCLE_R + 2;
        spr->drawCircle(circleX, outY, NEG_CIRCLE_R, C_NEG_CIRCLE);
        // Leitung nach dem Kreis
        spr->drawFastHLine(circleX + NEG_CIRCLE_R + 1, outY,
                           outEndX - circleX - NEG_CIRCLE_R - 1, C_WIRE);
    }
    else
    {
        spr->drawFastHLine(outStartX, outY, WIRE_LEN, C_WIRE);
    }

    // Ausgangswert
    const bool output = computeGate(m_gateType, m_inputA, m_inputB);
    const uint16_t colOut = output ? C_HIGH : C_LOW;

    // Toggle-Rechteck am Ausgang (read-only)
    const int outTogX = outEndX + 6;
    const int outTogY = outY - TOGGLE_H / 2;
    spr->fillSmoothRoundRect(outTogX, outTogY, TOGGLE_W, TOGGLE_H, TOGGLE_R, colOut);

    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(0x0000, colOut);
    spr->drawString(output ? "1" : "0", outTogX + TOGGLE_W / 2, outTogY + TOGGLE_H / 2);

    // Label "Y" im Gatter (rechts innen, Höhe Ausgang)
    spr->setTextDatum(MR_DATUM);
    spr->setTextColor(C_LABEL_DIM, C_GATE_BODY);
    spr->drawString("Y", gateX + GATE_W - 8, outY);
    spr->unloadFont();
}

// ===================================================================
// Zeichnen - Wahrheitstabelle
// ===================================================================

/**
 * @brief Zeichnet die vollständige Wahrheitstabelle mit Hervorhebung.
 */
void WorkshopTool_LogicGate::drawTruthTable()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_tableX;
    const int oy = m_originY + m_tableY;

    // Hintergrund
    spr->fillRect(ox, oy, m_tableW, m_tableH, C_BG);

    const int numIn = getNumInputs(m_gateType);
    const int numRows = (numIn == 1) ? 2 : 4;
    const int numCols = numIn + 1; // Eingänge + Ausgang

    // Spaltenpositionen berechnen
    const int colW = m_tableW / numCols;

    // Tabelle vertikal zentrieren im verfügbaren Bereich
    const int totalTableH = TABLE_HEADER_H + numRows * TABLE_ROW_H;
    const int tableStartY = oy + TOP_GAP + (m_tableH - TOP_GAP - totalTableH) / 2;

    // Aktive Zeile bestimmen
    int activeRow = 0;
    if (numIn == 1)
    {
        activeRow = m_inputA ? 1 : 0;
    }
    else
    {
        activeRow = (m_inputA ? 2 : 0) + (m_inputB ? 1 : 0);
    }

    // Kopfzeile
    spr->fillSmoothRoundRect(ox + 4, tableStartY, m_tableW - 8, TABLE_HEADER_H,
                             4, C_TBL_HDR_BG);

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_TBL_HDR_TXT, C_TBL_HDR_BG);

    if (numIn == 1)
    {
        // NOT: nur A und Y
        spr->drawString("A", ox + colW / 2, tableStartY + TABLE_HEADER_H / 2);
        spr->drawString("Y", ox + colW + colW / 2, tableStartY + TABLE_HEADER_H / 2);
    }
    else
    {
        spr->drawString("A", ox + colW / 2, tableStartY + TABLE_HEADER_H / 2);
        spr->drawString("B", ox + colW + colW / 2, tableStartY + TABLE_HEADER_H / 2);
        spr->drawString("Y", ox + 2 * colW + colW / 2, tableStartY + TABLE_HEADER_H / 2);
    }
    spr->unloadFont();

    // Datenzeilen
    const int dataStartY = tableStartY + TABLE_HEADER_H + 2;

    spr->loadFont(ComfortaaBold16);

    for (int row = 0; row < numRows; ++row)
    {
        const int rowY = dataStartY + row * TABLE_ROW_H;

        // Zeilen-Eingangswerte
        bool rA, rB;
        if (numIn == 1)
        {
            rA = (row == 1);
            rB = false;
        }
        else
        {
            rA = (row & 0x02) != 0;
            rB = (row & 0x01) != 0;
        }

        const bool rOut = computeGate(m_gateType, rA, rB);
        const bool isActive = (row == activeRow);

        // Zeilenhintergrund
        uint16_t rowBg;
        if (isActive) rowBg = C_TBL_ACTIVE;
        else          rowBg = (row & 1) ? C_TBL_ROW1 : C_TBL_ROW0;

        spr->fillSmoothRoundRect(ox + 4, rowY, m_tableW - 8, TABLE_ROW_H - 2,
                                 3, rowBg);

        // Eingangswerte zeichnen
        spr->setTextDatum(MC_DATUM);

        // Spalte A
        spr->setTextColor(C_TBL_IN_TXT, rowBg);
        spr->drawString(rA ? "1" : "0",
                        ox + colW / 2,
                        rowY + TABLE_ROW_H / 2 - 1);

        if (numIn == 2)
        {
            // Spalte B
            spr->drawString(rB ? "1" : "0",
                            ox + colW + colW / 2,
                            rowY + TABLE_ROW_H / 2 - 1);
        }

        // Spalte Y (Ausgang) - farbkodiert
        const int yColX = ox + (numCols - 1) * colW + colW / 2;
        spr->setTextColor(rOut ? C_TBL_VAL_1 : C_TBL_VAL_0, rowBg);
        spr->drawString(rOut ? "1" : "0",
                        yColX,
                        rowY + TABLE_ROW_H / 2 - 1);
    }

    spr->unloadFont();
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Hit-Test auf Selector-Chips.
 */
int WorkshopTool_LogicGate::hitTestChip(int cx, int cy) const
{
    for (int i = 0; i < NUM_GATE_TYPES; ++i)
    {
        const Rect &r = m_chipRects[i];
        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h)
            return i;
    }
    return -1;
}

/**
 * @brief Hit-Test auf Eingangs-Touch-Bereiche.
 */
int WorkshopTool_LogicGate::hitTestInput(int cx, int cy) const
{
    // Koordinaten sind relativ zum Schaltplanbereich
    const int sx = cx - m_schemX;
    const int sy = cy - m_schemY;

    const Rect &a = m_inputTouchA;
    if (a.w > 0 && sx >= a.x && sx < a.x + a.w && sy >= a.y && sy < a.y + a.h)
        return 0;

    if (getNumInputs(m_gateType) > 1)
    {
        const Rect &b = m_inputTouchB;
        if (b.w > 0 && sx >= b.x && sx < b.x + b.w && sy >= b.y && sy < b.y + b.h)
            return 1;
    }

    return -1;
}

/**
 * @brief Verarbeitet Touch-Events (Chips + Eingänge).
 */
bool WorkshopTool_LogicGate::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    // Content-relative Koordinaten
    const int cx = tx - WorkshopCfg::WORK_X - m_originX;
    const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

    if (touched)
    {
        if (!m_touchReady) return false;

        // Chip-Selektor testen
        const int hitChip = hitTestChip(cx, cy);
        if (hitChip >= 0 && hitChip != m_pressedChip)
        {
            m_pressedChip = hitChip;
            needsRedraw = true;
        }

        // Eingangs-Toggle testen
        if (m_pressedInput < 0 && hitChip < 0)
        {
            const int hitIn = hitTestInput(cx, cy);
            if (hitIn >= 0)
                m_pressedInput = hitIn;
        }

        m_touchWas = true;
    }
    else
    {
        m_touchReady = true;

        if (m_touchWas)
        {
            // Chip-Release
            if (m_pressedChip >= 0)
            {
                const GateType newType = static_cast<GateType>(m_pressedChip);
                if (newType != m_gateType)
                {
                    m_gateType = newType;
                    refreshSubtitle();
                    // Eingänge beibehalten, aber B bei NOT ignorieren
                    needsRedraw = true;
                }
            }

            // Eingangs-Release -> Toggle
            if (m_pressedInput >= 0)
            {
                if (m_pressedInput == 0)
                    m_inputA = !m_inputA;
                else
                    m_inputB = !m_inputB;
                needsRedraw = true;
            }
        }

        m_pressedChip  = -1;
        m_pressedInput = -1;
        m_touchWas     = false;
    }

    return needsRedraw;
}
