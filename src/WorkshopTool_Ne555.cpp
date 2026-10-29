#include "WorkshopTool_Ne555.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "WorkshopNumpad.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================

namespace {

/**
 * @brief Ersetzt den (einzigen) Dezimalpunkt im Puffer durch ein Komma.
 * @param buf Puffer mit formatiertem Zahlenwert
 */
void dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.') { *p = ','; break; }
}

// ===================================================================
// Spaltenaufteilung
// ===================================================================

constexpr int SCHEM_W  = 250; ///< Breite der linken Spalte (Schaltplan)
constexpr int COL_GAP  = 12;  ///< Abstand zwischen Spalten
constexpr int RIGHT_X_OFFSET = SCHEM_W + COL_GAP; ///< X-Start rechte Spalte

// ===================================================================
// Schaltplan - Gemeinsame Geometrie (schaltplanrelativ, ohne TOP_GAP)
// ===================================================================

constexpr int VCC_RAIL_Y = 10;  ///< Y-Position VCC-Schiene
constexpr int GND_RAIL_Y = 185; ///< Y-Position GND-Schiene
constexpr int CHAIN_CX   = 48;  ///< Mittelpunkt X der Bauteilkette

constexpr int COMP_W    = 24;   ///< Breite Widerstands-Symbol
constexpr int COMP_R_H  = 36;   ///< Höhe Widerstands-Symbol
constexpr int CAP_PLATE_W = 22; ///< Breite Kondensator-Platten
constexpr int CAP_GAP     = 8;  ///< Abstand zwischen Kondensator-Platten

constexpr int HIT_W = 90;      ///< Breite der Bauteil-Touch-Area
constexpr int HIT_PAD = 6;     ///< Vertikales Padding der Touch-Area

// IC-Gehäuse
constexpr int IC_X   = 132;    ///< X-Position des IC-Blocks
constexpr int IC_W   = 90;     ///< Breite des IC-Blocks
constexpr int IC_R   = 4;      ///< Eckenradius des IC-Blocks

// ===================================================================
// Schaltplan - Astabile Positionen
// ===================================================================

constexpr int AST_R1_TOP = 26;                        ///< R1 Oberkante
constexpr int AST_R1_BOT = AST_R1_TOP + COMP_R_H;    ///< R1 Unterkante (62)
constexpr int AST_NODE_A = 70;                        ///< Knotenpunkt A (R1/R2)
constexpr int AST_R2_TOP = 80;                        ///< R2 Oberkante
constexpr int AST_R2_BOT = AST_R2_TOP + COMP_R_H;    ///< R2 Unterkante (116)
constexpr int AST_NODE_B = 124;                       ///< Knotenpunkt B (R2/C)
constexpr int AST_CAP_Y  = 140;                       ///< Kondensator Y (obere Platte)

constexpr int AST_IC_Y = 48;                          ///< IC Y-Start
constexpr int AST_IC_H = 90;                          ///< IC Höhe
constexpr int AST_PIN7_Y = AST_NODE_A;                ///< Pin 7 DIS (= Node A)
constexpr int AST_PIN6_Y = 97;                        ///< Pin 6 THR
constexpr int AST_PIN2_Y = AST_NODE_B;                ///< Pin 2 TRI (= Node B)
constexpr int AST_PIN3_Y = 90;                        ///< Pin 3 OUT

// ===================================================================
// Schaltplan - Monostabile Positionen
// ===================================================================

constexpr int MONO_R_TOP  = 26;                       ///< R Oberkante
constexpr int MONO_R_BOT  = MONO_R_TOP + COMP_R_H;   ///< R Unterkante
constexpr int MONO_NODE_Y = 70;                       ///< Knotenpunkt (R/C -> THR+DIS)
constexpr int MONO_CAP_Y  = 88;                       ///< Kondensator Y

constexpr int MONO_IC_Y = 48;
constexpr int MONO_IC_H = 90;
constexpr int MONO_PIN7_Y = MONO_NODE_Y;              ///< Pin 7 DIS
constexpr int MONO_PIN6_Y = 97;                       ///< Pin 6 THR
constexpr int MONO_PIN2_Y = AST_NODE_B;               ///< Pin 2 TRI (Trigger-Eingang)
constexpr int MONO_PIN3_Y = 90;                       ///< Pin 3 OUT

// ===================================================================
// Oszillogramm
// ===================================================================

constexpr int OSCILL_GAP      = 5;   ///< Abstand Schaltplan -> Oszillogramm
constexpr int OSCILL_MARGIN_L = 28;  ///< Linker Rand (Achsenbeschriftung)
constexpr int OSCILL_MARGIN_R = 5;
constexpr int OSCILL_MARGIN_T = 6;
constexpr int OSCILL_MARGIN_B = 12;  ///< Unterer Rand (Zeitachsenbeschriftung)

// ===================================================================
// Rechte Spalte - Eingabefelder
// ===================================================================

constexpr int INPUT_ROW_H  = 26;  ///< Höhe einer Eingabezeile
constexpr int RESULT_GAP   = 6;   ///< Abstand Felder -> Ergebnisse
constexpr int RESULT_ROW_H = 22;  ///< Höhe einer Ergebniszeile
constexpr int ACCENT_W     = 4;   ///< Breite des linken Farbakzent-Streifens
constexpr int UNIT_W       = 52;  ///< Breite der Einheiten-Spalte

// ===================================================================
// Cursor-Blink
// ===================================================================

constexpr unsigned long CURSOR_PERIOD_MS = 800;
constexpr unsigned long CURSOR_ON_MS     = 400;

// ===================================================================
// Farben - Eingabefelder
// ===================================================================

constexpr uint16_t C_FIELD_INPUT = 0x2945;
constexpr uint16_t C_FIELD_FOCUS = 0x328A;
constexpr uint16_t C_FIELD_PRESS = 0x4B4D;
constexpr uint16_t C_SEP         = 0x2104;

constexpr uint16_t C_LBL_FOCUS  = 0xFFFF;
constexpr uint16_t C_LBL_INPUT  = 0xDEDB;
constexpr uint16_t C_VAL_FOCUS  = 0xFFFF;
constexpr uint16_t C_VAL_INPUT  = 0xEF7D;
constexpr uint16_t C_UNIT_FOCUS = 0xAD75;
constexpr uint16_t C_UNIT_INPUT = 0x8410;

// ===================================================================
// Farben - Akzentstreifen (links am Eingabefeld)
// ===================================================================

constexpr uint16_t C_ACCENT_R1 = 0x07E0; ///< R1 - Grün
constexpr uint16_t C_ACCENT_R2 = 0xFFE0; ///< R2 - Gelb
constexpr uint16_t C_ACCENT_C  = 0x04FF; ///< C  - Cyan

// ===================================================================
// Farben - Footer-Toggle
// ===================================================================

constexpr uint16_t TOGGLE_ACTIVE_FILL      = 0x328A;
constexpr uint16_t TOGGLE_ACTIVE_TEXT      = 0xFFFF;
constexpr uint16_t TOGGLE_ACTIVE_OUTLINE   = 0x328A;
constexpr uint16_t TOGGLE_INACTIVE_FILL    = 0x0000;
constexpr uint16_t TOGGLE_INACTIVE_TEXT    = 0x7BEF;
constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8;

constexpr int FOOTER_BTN_W   = 130;
constexpr int FOOTER_BTN_H   = 44;
constexpr int FOOTER_BTN_R   = 6;
constexpr int FOOTER_BTN_GAP = 10;

// ===================================================================
// Statische Labels
// ===================================================================

const char * const AST_LABELS[]  = {"R1", "R2", "C"};
const char * const MONO_LABELS[] = {"R", "C"};
const char * const AST_UNITS[]   = {"kOhm", "kOhm", "uF"};
const char * const MONO_UNITS[]  = {"kOhm", "uF"};

const char * const AST_RESULT_LABELS[]  = {"f", "T", "Duty", "t_hi", "t_lo"};
const char * const MONO_RESULT_LABELS[] = {"t_pulse"};

} // namespace

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_Ne555::WorkshopTool_Ne555(Ne555Colors colors)
    : m_colors(colors)
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_Ne555::onEnter(LGFX *tft)
{
    clearAll();
    initFooterButtons(tft);
}

/**
 * @brief Aufräumen beim Verlassen.
 */
void WorkshopTool_Ne555::onExit()
{
    m_buttonsInitialized = false;
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_Ne555::clearAll()
{
    memset(m_astBuf,  0, sizeof(m_astBuf));
    memset(m_monoBuf, 0, sizeof(m_monoBuf));
    memset(m_astVal,  0, sizeof(m_astVal));
    memset(m_monoVal, 0, sizeof(m_monoVal));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));
    memset(m_compHits,   0, sizeof(m_compHits));

    m_sprite      = nullptr;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;
    m_layoutValid = false;
    m_computeOk   = false;
    m_numpad.clearLayout();

    m_mode        = Ne555Mode::ASTABLE;
    m_activeField = 0;

    m_pressedField = -1;
    m_pressedComp  = -1;
    m_pressedKey   = -1;
    m_touchWas     = false;
    m_touchReady   = false;

    // Astabil-Standardwerte: R1=10k, R2=47k, C=0,1uF -> f ≈ 138 Hz
    m_astVal[0] = 10.0;
    m_astVal[1] = 47.0;
    m_astVal[2] = 0.1;
    stringutils::safeCopy(m_astBuf[0], FIELD_CAP, "10");
    stringutils::safeCopy(m_astBuf[1], FIELD_CAP, "47");
    stringutils::safeCopy(m_astBuf[2], FIELD_CAP, "0.1");

    // Monostabil-Standardwerte: R=10k, C=1uF -> t ≈ 11 ms
    m_monoVal[0] = 10.0;
    m_monoVal[1] = 1.0;
    stringutils::safeCopy(m_monoBuf[0], FIELD_CAP, "10");
    stringutils::safeCopy(m_monoBuf[1], FIELD_CAP, "1");

    m_frequency = 0.0;
    m_period    = 0.0;
    m_dutyCycle = 0.0;
    m_tHigh     = 0.0;
    m_tLow      = 0.0;
    m_tPulse    = 0.0;

    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));
    refreshSubtitle();

    compute();
}

// ===================================================================
// Footer-Buttons
// ===================================================================

/**
 * @brief Erstellt die zwei Footer-Toggle-Buttons.
 */
void WorkshopTool_Ne555::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int totalW = 2 * FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int startX = WorkshopCfg::TOOL_FOOTER_X +
                           (WorkshopCfg::TOOL_FOOTER_W - totalW) / 2;

    const int cxAst  = startX + FOOTER_BTN_W / 2;
    const int cxMono = cxAst + FOOTER_BTN_W + FOOTER_BTN_GAP;

    I18n::format(m_btnAstableLabel,    sizeof(m_btnAstableLabel),
                 WorkshopKey::WORKSHOP_NE555_BTN_ASTABLE);
    I18n::format(m_btnMonostableLabel, sizeof(m_btnMonostableLabel),
                 WorkshopKey::WORKSHOP_NE555_BTN_MONOSTABLE);

    m_btnAstable.initButton(tft, cxAst, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
                            TOGGLE_ACTIVE_OUTLINE, TOGGLE_ACTIVE_FILL, TOGGLE_ACTIVE_TEXT,
                            m_btnAstableLabel, nullptr);
    m_btnAstable.setFont(LeagueGothicCond34);

    m_btnMono.initButton(tft, cxMono, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
                         TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
                         m_btnMonostableLabel, nullptr);
    m_btnMono.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
}

/**
 * @brief Aktualisiert Toggle-Farben je nach aktivem Modus.
 */
void WorkshopTool_Ne555::updateButtonColors()
{
    const bool isAst = (m_mode == Ne555Mode::ASTABLE);

    m_btnAstable.setFillColor(isAst ? TOGGLE_ACTIVE_FILL : TOGGLE_INACTIVE_FILL);
    m_btnAstable.setTextColor(isAst ? TOGGLE_ACTIVE_TEXT : TOGGLE_INACTIVE_TEXT);
    m_btnAstable.setOutlineColor(isAst ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);

    m_btnMono.setFillColor(isAst ? TOGGLE_INACTIVE_FILL : TOGGLE_ACTIVE_FILL);
    m_btnMono.setTextColor(isAst ? TOGGLE_INACTIVE_TEXT : TOGGLE_ACTIVE_TEXT);
    m_btnMono.setOutlineColor(isAst ? TOGGLE_INACTIVE_OUTLINE : TOGGLE_ACTIVE_OUTLINE);
}

/**
 * @brief Zeichnet die Footer-Toggle-Buttons.
 */
void WorkshopTool_Ne555::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft; (void)x; (void)y; (void)w; (void)h;
    if (!m_buttonsInitialized)
        return;

    updateButtonColors();
    m_btnAstable.drawButton();
    m_btnMono.drawButton();
}

/**
 * @brief Verarbeitet Touch auf Footer-Buttons.
 */
bool WorkshopTool_Ne555::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    const bool onAst  = touched && m_btnAstable.contains(tx, ty);
    const bool onMono = touched && m_btnMono.contains(tx, ty);

    const bool wasAst  = m_btnAstable.isPressed();
    const bool wasMono = m_btnMono.isPressed();

    m_btnAstable.press(onAst);
    m_btnMono.press(onMono);

    // Moduswechsel bei Loslassen
    if (m_btnAstable.justReleased() && m_mode != Ne555Mode::ASTABLE)
    {
        m_mode = Ne555Mode::ASTABLE;
        refreshSubtitle();
        m_activeField = 0;
        m_layoutValid = false;
        compute();
        return true;
    }
    if (m_btnMono.justReleased() && m_mode != Ne555Mode::MONOSTABLE)
    {
        m_mode = Ne555Mode::MONOSTABLE;
        refreshSubtitle();
        m_activeField = 0;
        m_layoutValid = false;
        compute();
        return true;
    }

    // Visuelle Rückmeldung
    if (onAst != wasAst)
        m_btnAstable.drawButton(onAst);
    if (onMono != wasMono)
        m_btnMono.drawButton(onMono);

    return false;
}

/**
 * @brief Dynamischer Untertitel je nach Betriebsart.
 */
const char *WorkshopTool_Ne555::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Befüllt m_subtitleBuf passend zu m_mode.
 */
void WorkshopTool_Ne555::refreshSubtitle()
{
    const WorkshopKey key = (m_mode == Ne555Mode::ASTABLE)
                                ? WorkshopKey::WORKSHOP_NE555_SUB_ASTABLE
                                : WorkshopKey::WORKSHOP_NE555_SUB_MONOSTABLE;
    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), key);
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet alle Bereiche: Schaltplan, Oszillogramm, Felder, Ergebnisse, Numpad.
 */
void WorkshopTool_Ne555::draw(Gfx_Sprite *sprite,
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

    const bool cursorOn = (millis() % CURSOR_PERIOD_MS) < CURSOR_ON_MS;

    drawSchematic();
    drawOscillogram();
    drawInputFields(cursorOn);
    drawResults();
    m_numpad.draw(m_sprite, m_originX + m_rightX, m_originY, m_pressedKey);
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet alle Positionen und Rects.
 */
void WorkshopTool_Ne555::computeLayout(int w, int h)
{
    m_rightX = RIGHT_X_OFFSET;
    m_rightW = w - m_rightX;
    if (m_rightW < 100) m_rightW = 100;

    // Schaltplan- und Oszillogramm-Aufteilung in linker Spalte
    m_oscillH = 65;
    m_oscillY = h - m_oscillH;
    m_schemH  = m_oscillY - OSCILL_GAP;

    // Eingabefelder (rechte Spalte, mit TOP_GAP-Versatz)
    const int nf = fieldCount();
    for (int i = 0; i < nf; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * INPUT_ROW_H, m_rightW, INPUT_ROW_H};

    // Ergebnis-Start direkt unter den Feldern
    m_resultsY = TOP_GAP + nf * INPUT_ROW_H + RESULT_GAP;

    // Numpad am unteren Rand der rechten Spalte (bottom-anchored)
    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);

    // Schaltplan-Touch-Areas (content-relativ, mit TOP_GAP-Versatz)
    if (m_mode == Ne555Mode::ASTABLE)
    {
        m_compHits[0] = {0, TOP_GAP + AST_R1_TOP - HIT_PAD, HIT_W, COMP_R_H + 2 * HIT_PAD};
        m_compHits[1] = {0, TOP_GAP + AST_R2_TOP - HIT_PAD, HIT_W, COMP_R_H + 2 * HIT_PAD};
        m_compHits[2] = {0, TOP_GAP + AST_CAP_Y - HIT_PAD,  HIT_W, CAP_GAP + 2 * HIT_PAD};
    }
    else
    {
        m_compHits[0] = {0, TOP_GAP + MONO_R_TOP - HIT_PAD,  HIT_W, COMP_R_H + 2 * HIT_PAD};
        m_compHits[1] = {0, TOP_GAP + MONO_CAP_Y - HIT_PAD,  HIT_W, CAP_GAP + 2 * HIT_PAD};
    }
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Berechnet Ergebniswerte aus den aktuellen Eingaben.
 */
void WorkshopTool_Ne555::compute()
{
    m_computeOk = false;

    // Eingabewerte aus Puffern parsen
    const int nf = fieldCount();
    for (int i = 0; i < nf; ++i)
    {
        const char *buf = inputBuf(i);
        fieldValue(i) = (buf[0] != '\0') ? atof(buf) : 0.0;
    }

    if (m_mode == Ne555Mode::ASTABLE)
    {
        const double r1_ohm = m_astVal[0] * 1000.0; // kOhm -> Ohm
        const double r2_ohm = m_astVal[1] * 1000.0;
        const double c_f    = m_astVal[2] * 1.0e-6;  // uF -> F

        if (r1_ohm <= 0.0 || r2_ohm <= 0.0 || c_f <= 0.0)
            return;

        m_tHigh    = 0.693 * (r1_ohm + r2_ohm) * c_f;
        m_tLow     = 0.693 * r2_ohm * c_f;
        m_period   = m_tHigh + m_tLow;
        m_frequency = (m_period > 0.0) ? (1.0 / m_period) : 0.0;
        m_dutyCycle = (r1_ohm + r2_ohm) / (r1_ohm + 2.0 * r2_ohm) * 100.0;
        m_computeOk = std::isfinite(m_frequency) && m_frequency > 0.0;
    }
    else
    {
        const double r_ohm = m_monoVal[0] * 1000.0;
        const double c_f   = m_monoVal[1] * 1.0e-6;

        if (r_ohm <= 0.0 || c_f <= 0.0)
            return;

        m_tPulse    = 1.1 * r_ohm * c_f;
        m_computeOk = std::isfinite(m_tPulse) && m_tPulse > 0.0;
    }
}

// ===================================================================
// Zeichnen - Schaltplan
// ===================================================================

/**
 * @brief Zeichnet den kompletten Schaltplan-Bereich.
 */
void WorkshopTool_Ne555::drawSchematic()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    // Hintergrund (ab TOP_GAP, Höhe entsprechend reduziert)
    spr->fillRect(ox, oy, SCHEM_W, m_schemH - TOP_GAP, m_colors.schemBg);

    if (m_mode == Ne555Mode::ASTABLE)
        drawAstableCircuit();
    else
        drawMonostableCircuit();
}

/**
 * @brief Zeichnet die astabile Schaltung.
 */
void WorkshopTool_Ne555::drawAstableCircuit()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;
    const uint16_t cw = m_colors.wire;

    // VCC/GND-Schienen
    spr->drawFastHLine(ox + 20, oy + VCC_RAIL_Y, SCHEM_W - 40, cw);
    spr->drawFastHLine(ox + 20, oy + GND_RAIL_Y, SCHEM_W - 40, cw);

    // Vertikale Verdrahtung der Bauteilkette
    const int cx = ox + CHAIN_CX;
    spr->drawFastVLine(cx, oy + VCC_RAIL_Y, AST_R1_TOP - VCC_RAIL_Y, cw);       // VCC -> R1
    spr->drawFastVLine(cx, oy + AST_R1_BOT, AST_NODE_A - AST_R1_BOT, cw);       // R1 -> Node A
    spr->drawFastVLine(cx, oy + AST_NODE_A, AST_R2_TOP - AST_NODE_A, cw);        // Node A -> R2
    spr->drawFastVLine(cx, oy + AST_R2_BOT, AST_NODE_B - AST_R2_BOT, cw);        // R2 -> Node B
    spr->drawFastVLine(cx, oy + AST_NODE_B, AST_CAP_Y - AST_NODE_B, cw);         // Node B -> Cap
    spr->drawFastVLine(cx, oy + AST_CAP_Y + CAP_GAP, GND_RAIL_Y - AST_CAP_Y - CAP_GAP, cw); // Cap -> GND

    // Horizontale Verdrahtung zu IC-Pins
    spr->drawFastHLine(cx, oy + AST_NODE_A, IC_X - CHAIN_CX, cw);               // Node A -> Pin 7

    // Node B -> Pin 6 + Pin 2 (T-Routing)
    const int routeX = ox + 108;
    spr->drawFastHLine(cx, oy + AST_NODE_B, IC_X - CHAIN_CX, cw);               // Node B horizontal -> Pin 2
    spr->drawFastVLine(routeX, oy + AST_PIN6_Y, AST_NODE_B - AST_PIN6_Y, cw);   // Vertikal hoch zu Pin 6
    spr->drawFastHLine(routeX, oy + AST_PIN6_Y, IC_X - 108, cw);                // Horizontal zu Pin 6

    // VCC-Verbindung zum IC (oberer Rand)
    const int icCx = ox + IC_X + IC_W / 2;
    spr->drawFastVLine(icCx, oy + VCC_RAIL_Y, AST_IC_Y - VCC_RAIL_Y, cw);
    // GND-Verbindung zum IC (unterer Rand)
    spr->drawFastVLine(icCx, oy + AST_IC_Y + AST_IC_H, GND_RAIL_Y - AST_IC_Y - AST_IC_H, cw);

    // Knotenpunkte (ausgefüllte Kreise)
    spr->fillCircle(cx, oy + AST_NODE_A, 3, m_colors.junction);
    spr->fillCircle(cx, oy + AST_NODE_B, 3, m_colors.junction);
    spr->fillCircle(routeX, oy + AST_NODE_B, 2, m_colors.junction);

    // Knotenpunkte an VCC/GND-Schienen
    spr->fillCircle(cx,   oy + VCC_RAIL_Y, 3, m_colors.junction);
    spr->fillCircle(cx,   oy + GND_RAIL_Y, 3, m_colors.junction);
    spr->fillCircle(icCx, oy + VCC_RAIL_Y, 3, m_colors.junction);
    spr->fillCircle(icCx, oy + GND_RAIL_Y, 3, m_colors.junction);

    // Bauteil-Symbole
    drawResistor(CHAIN_CX, AST_R1_TOP, COMP_R_H, m_activeField == 0);
    drawResistor(CHAIN_CX, AST_R2_TOP, COMP_R_H, m_activeField == 1);
    drawCapacitor(CHAIN_CX, AST_CAP_Y, m_activeField == 2);

    // IC-Gehäuse
    drawIcBlock(AST_IC_Y, AST_IC_H, AST_PIN7_Y, AST_PIN6_Y, AST_PIN2_Y, AST_PIN3_Y);

    // Beschriftungen (ComfortaaBold14)
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(ML_DATUM);

    // VCC / GND
    spr->setTextColor(m_colors.rail, m_colors.schemBg);
    spr->drawString("VCC", ox + 3, oy + VCC_RAIL_Y);
    spr->drawString("GND", ox + 3, oy + GND_RAIL_Y);

    // Bauteil-Labels links, Werte rechts
    for (int i = 0; i < AST_FIELDS; ++i)
    {
        const bool active = (m_activeField == i);
        const uint16_t clr = active ? m_colors.valActive : m_colors.valInactive;
        int compCy;
        if (i == 0)      compCy = (AST_R1_TOP + AST_R1_BOT) / 2;
        else if (i == 1) compCy = (AST_R2_TOP + AST_R2_BOT) / 2;
        else             compCy = AST_CAP_Y + CAP_GAP / 2;

        // Label links
        spr->setTextColor(clr, m_colors.schemBg);
        spr->drawString(AST_LABELS[i], ox + 4, oy + compCy);

        // Wert rechts vom Bauteil
        char valBuf[16];
        formatSchematicValue(i, valBuf, sizeof(valBuf));
        spr->drawString(valBuf, ox + CHAIN_CX + COMP_W / 2 + 6, oy + compCy);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet die monostabile Schaltung.
 */
void WorkshopTool_Ne555::drawMonostableCircuit()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;
    const uint16_t cw = m_colors.wire;

    // VCC/GND-Schienen
    spr->drawFastHLine(ox + 20, oy + VCC_RAIL_Y, SCHEM_W - 40, cw);
    spr->drawFastHLine(ox + 20, oy + GND_RAIL_Y, SCHEM_W - 40, cw);

    // Vertikale Bauteilkette
    const int cx = ox + CHAIN_CX;
    spr->drawFastVLine(cx, oy + VCC_RAIL_Y, MONO_R_TOP - VCC_RAIL_Y, cw);
    spr->drawFastVLine(cx, oy + MONO_R_BOT, MONO_NODE_Y - MONO_R_BOT, cw);
    spr->drawFastVLine(cx, oy + MONO_NODE_Y, MONO_CAP_Y - MONO_NODE_Y, cw);
    spr->drawFastVLine(cx, oy + MONO_CAP_Y + CAP_GAP, GND_RAIL_Y - MONO_CAP_Y - CAP_GAP, cw);

    // Node -> Pin 7 + Pin 6
    const int routeX = ox + 108;
    spr->drawFastHLine(cx, oy + MONO_NODE_Y, IC_X - CHAIN_CX, cw);              // -> Pin 7
    spr->drawFastVLine(routeX, oy + MONO_NODE_Y, MONO_PIN6_Y - MONO_NODE_Y, cw); // Vertikal runter zu Pin 6
    spr->drawFastHLine(routeX, oy + MONO_PIN6_Y, IC_X - 108, cw);

    // Trigger-Pfeil -> Pin 2
    const int trigX = ox + 100;
    spr->drawFastHLine(trigX, oy + MONO_PIN2_Y, IC_X - 100, cw);
    // Pfeilspitze links
    spr->fillTriangle(trigX, oy + MONO_PIN2_Y,
                      trigX + 6, oy + MONO_PIN2_Y - 4,
                      trigX + 6, oy + MONO_PIN2_Y + 4, cw);

    // IC VCC/GND
    const int icCx = ox + IC_X + IC_W / 2;
    spr->drawFastVLine(icCx, oy + VCC_RAIL_Y, MONO_IC_Y - VCC_RAIL_Y, cw);
    spr->drawFastVLine(icCx, oy + MONO_IC_Y + MONO_IC_H, GND_RAIL_Y - MONO_IC_Y - MONO_IC_H, cw);

    // Knotenpunkte
    spr->fillCircle(cx, oy + MONO_NODE_Y, 3, m_colors.junction);
    spr->fillCircle(routeX, oy + MONO_NODE_Y, 2, m_colors.junction);

    // Knotenpunkte an VCC/GND-Schienen
    spr->fillCircle(cx,   oy + VCC_RAIL_Y, 3, m_colors.junction);
    spr->fillCircle(cx,   oy + GND_RAIL_Y, 3, m_colors.junction);
    spr->fillCircle(icCx, oy + VCC_RAIL_Y, 3, m_colors.junction);
    spr->fillCircle(icCx, oy + GND_RAIL_Y, 3, m_colors.junction);

    // Bauteile
    drawResistor(CHAIN_CX, MONO_R_TOP, COMP_R_H, m_activeField == 0);
    drawCapacitor(CHAIN_CX, MONO_CAP_Y, m_activeField == 1);

    // IC
    drawIcBlock(MONO_IC_Y, MONO_IC_H, MONO_PIN7_Y, MONO_PIN6_Y, MONO_PIN2_Y, MONO_PIN3_Y);

    // Beschriftungen
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(ML_DATUM);

    spr->setTextColor(m_colors.rail, m_colors.schemBg);
    spr->drawString("VCC", ox + 3, oy + VCC_RAIL_Y);
    spr->drawString("GND", ox + 3, oy + GND_RAIL_Y);

    // Trigger-Label
    spr->setTextColor(m_colors.pinLabel, m_colors.schemBg);
    spr->drawString("TRI", ox + 82, oy + MONO_PIN2_Y - 12);

    // Bauteil-Labels und Werte
    for (int i = 0; i < MONO_FIELDS; ++i)
    {
        const bool active = (m_activeField == i);
        const uint16_t clr = active ? m_colors.valActive : m_colors.valInactive;
        int compCy;
        if (i == 0) compCy = (MONO_R_TOP + MONO_R_BOT) / 2;
        else        compCy = MONO_CAP_Y + CAP_GAP / 2;

        spr->setTextColor(clr, m_colors.schemBg);
        spr->drawString(MONO_LABELS[i], ox + 4, oy + compCy);

        char valBuf[16];
        formatSchematicValue(i, valBuf, sizeof(valBuf));
        spr->drawString(valBuf, ox + CHAIN_CX + COMP_W / 2 + 6, oy + compCy);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet das IC-Gehäuse mit Pin-Beschriftung.
 */
void WorkshopTool_Ne555::drawIcBlock(int icY, int icH,
                                      int pin7Y, int pin6Y, int pin2Y, int pin3Y)
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;
    const int ax = ox + IC_X;
    const int ay = oy + icY;

    // IC-Körper (Outline-Trick: 2× fillSmoothRoundRect)
    spr->fillSmoothRoundRect(ax, ay, IC_W, icH, IC_R, m_colors.icOutline);
    spr->fillSmoothRoundRect(ax + 2, ay + 2, IC_W - 4, icH - 4, IC_R, m_colors.icBody);

    // Kerbe oben Mitte (DIP-Markierung)
    const int notchCx = ax + IC_W / 2;
    spr->fillCircle(notchCx, ay, 5, m_colors.schemBg);
    spr->fillCircle(notchCx, ay + 1, 4, m_colors.icOutline);

    // "NE555" zentriert
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(m_colors.icText, m_colors.icBody);
    spr->drawString("NE555", ax + IC_W / 2, ay + icH / 2);

    // Pin-Labels auf der linken Seite
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(m_colors.pinLabel, m_colors.icBody);
    spr->drawString("7", ax + 5, oy + pin7Y);
    spr->drawString("6", ax + 5, oy + pin6Y);
    spr->drawString("2", ax + 5, oy + pin2Y);

    // Pin 3 (OUT) auf der rechten Seite + Pfeil
    spr->setTextDatum(MR_DATUM);
    spr->drawString("3", ax + IC_W - 5, oy + pin3Y);
    spr->unloadFont();

    // Ausgangspfeil (Pin 3)
    const int arrowX = ax + IC_W;
    spr->drawFastHLine(arrowX, oy + pin3Y, 14, m_colors.wire);
    spr->fillTriangle(arrowX + 14, oy + pin3Y,
                      arrowX + 10, oy + pin3Y - 4,
                      arrowX + 10, oy + pin3Y + 4, m_colors.wire);
}

/**
 * @brief Zeichnet ein Widerstands-Symbol.
 */
void WorkshopTool_Ne555::drawResistor(int cx, int top, int h, bool active)
{
    Gfx_Sprite * const spr = m_sprite;
    const int ax = m_originX + cx - COMP_W / 2;
    const int ay = m_originY + TOP_GAP + top;
    const uint16_t fill = active ? 0x1A45 : m_colors.compBody;
    const uint16_t outline = active ? m_colors.valActive : m_colors.compOutline;

    spr->fillRect(ax, ay, COMP_W, h, fill);
    spr->drawRect(ax, ay, COMP_W, h, outline);
}

/**
 * @brief Zeichnet ein Kondensator-Symbol (zwei Platten).
 */
void WorkshopTool_Ne555::drawCapacitor(int cx, int topY, bool active)
{
    Gfx_Sprite * const spr = m_sprite;
    const int ax = m_originX + cx - CAP_PLATE_W / 2;
    const int ay1 = m_originY + TOP_GAP + topY;
    const int ay2 = ay1 + CAP_GAP;
    const uint16_t clr = active ? m_colors.valActive : m_colors.compOutline;

    spr->fillRect(ax, ay1, CAP_PLATE_W, 3, clr);
    spr->fillRect(ax, ay2, CAP_PLATE_W, 3, clr);
}

// ===================================================================
// Zeichnen - Oszillogramm
// ===================================================================

/**
 * @brief Zeichnet das Oszillogramm des Ausgangssignals.
 */
void WorkshopTool_Ne555::drawOscillogram()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + m_oscillY;
    const int ow = SCHEM_W;
    const int oh = m_oscillH;

    // Hintergrund - entspricht der Content-Container-Farbe des Schaltplans
    spr->fillRect(ox, oy, ow, oh, m_colors.schemBg);

    // Rahmen oben
    spr->drawFastHLine(ox, oy, ow, m_colors.oscAxis);

    // Signalbereich
    const int sigX = ox + OSCILL_MARGIN_L;
    const int sigW = ow - OSCILL_MARGIN_L - OSCILL_MARGIN_R;
    const int sigTop = oy + OSCILL_MARGIN_T;
    const int sigBot = oy + oh - OSCILL_MARGIN_B;
    const int sigH = sigBot - sigTop;

    if (sigW < 20 || sigH < 10)
        return;

    // Achsenbeschriftungen
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(m_colors.oscText, m_colors.schemBg);
    spr->drawString("t", ox + ow / 2, oy + oh - 4);

    spr->setTextDatum(MC_DATUM);
    spr->drawString("V", ox + 10, oy + oh / 2 - 2);
    spr->unloadFont();

    if (!m_computeOk)
        return;

    const uint16_t sigClr = m_colors.oscSignal;

    if (m_mode == Ne555Mode::ASTABLE)
    {
        // Rechteckwelle: 2,5 Perioden
        const double totalTime = 2.5 * m_period;
        if (totalTime <= 0.0) return;

        const double scaleX = static_cast<double>(sigW) / totalTime;
        double t = 0.0;
        int prevX = sigX;
        bool high = true;

        for (int p = 0; p < 3; ++p)
        {
            // High-Phase
            const double dt_h = (p < 2 || high) ? m_tHigh : 0.0;
            int nextX = sigX + static_cast<int>(std::min((t + dt_h), totalTime) * scaleX);
            if (nextX > sigX + sigW) nextX = sigX + sigW;

            if (nextX > prevX)
            {
                spr->drawFastHLine(prevX, sigTop + 2, nextX - prevX, sigClr);
                // Vertikale Flanke (fallend) - nicht beim letzten High-Phase-Ende
                if (t + dt_h < totalTime)
                    spr->drawFastVLine(nextX, sigTop + 2, sigH - 1, sigClr);
            }
            t += dt_h;
            prevX = nextX;

            if (t >= totalTime) break;

            // Low-Phase
            nextX = sigX + static_cast<int>(std::min((t + m_tLow), totalTime) * scaleX);
            if (nextX > sigX + sigW) nextX = sigX + sigW;

            if (nextX > prevX)
            {
                spr->drawFastHLine(prevX, sigBot, nextX - prevX, sigClr);
                // Vertikale Flanke (steigend)
                if (t + m_tLow < totalTime)
                    spr->drawFastVLine(nextX, sigTop + 2, sigH - 1, sigClr);
            }
            t += m_tLow;
            prevX = nextX;

            if (t >= totalTime) break;
        }
    }
    else
    {
        // Monostabil: Trigger -> Puls -> Low
        const double totalTime = m_tPulse * 2.5;
        if (totalTime <= 0.0) return;

        const double scaleX = static_cast<double>(sigW) / totalTime;
        const int trigX2 = sigX + static_cast<int>(0.1 * totalTime * scaleX); // 10% Vorlauf
        const int pulseEnd = sigX + static_cast<int>((0.1 * totalTime + m_tPulse) * scaleX);

        // Anfangspegel: Low
        spr->drawFastHLine(sigX, sigBot, trigX2 - sigX, sigClr);

        // Steigende Flanke
        spr->drawFastVLine(trigX2, sigTop + 2, sigH - 1, sigClr);

        // High-Pegel
        const int pulseW = std::min(pulseEnd, sigX + sigW) - trigX2;
        if (pulseW > 0)
            spr->drawFastHLine(trigX2, sigTop + 2, pulseW, sigClr);

        // Fallende Flanke
        if (pulseEnd < sigX + sigW)
        {
            spr->drawFastVLine(pulseEnd, sigTop + 2, sigH - 1, sigClr);
            spr->drawFastHLine(pulseEnd, sigBot, sigX + sigW - pulseEnd, sigClr);
        }
    }
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle Eingabefelder.
 */
void WorkshopTool_Ne555::drawInputFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);
    const int nf = fieldCount();
    for (int i = 0; i < nf; ++i)
        drawInputField(i, m_pressedField == i, cursorOn);
    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet ein einzelnes Eingabefeld.
 */
void WorkshopTool_Ne555::drawInputField(int idx, bool pressed, bool cursorOn)
{
    Gfx_Sprite * const spr = m_sprite;
    const Rect        &fr  = m_fieldRects[idx];
    const int          ax  = m_originX + fr.x;
    const int          ay  = m_originY + fr.y;
    const bool focused = (m_activeField == idx);

    uint16_t bgColor;
    if      (pressed) bgColor = C_FIELD_PRESS;
    else if (focused) bgColor = C_FIELD_FOCUS;
    else              bgColor = C_FIELD_INPUT;

    spr->fillRect(ax, ay, fr.w, fr.h, bgColor);
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);

    // Akzentstreifen
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, accentColor(idx));

    // Textfarben
    const uint16_t lblClr  = focused ? C_LBL_FOCUS  : C_LBL_INPUT;
    const uint16_t unitClr = focused ? C_UNIT_FOCUS  : C_UNIT_INPUT;
    const uint16_t valClr  = focused ? C_VAL_FOCUS   : C_VAL_INPUT;

    const int cy = ay + fr.h / 2;

    // Label
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblClr, bgColor);
    spr->drawString(fieldLabel(idx), ax + ACCENT_W + 6, cy);

    // Einheit
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(unitClr, bgColor);
    spr->drawString(fieldUnit(idx), ax + fr.w - UNIT_W / 2, cy);

    // Wert + Cursor
    const int valRightX = ax + fr.w - UNIT_W - 6;
    spr->setTextDatum(MR_DATUM);
    spr->setTextColor(valClr, bgColor);

    char displayBuf[32];

    if (focused)
    {
        // Eingabepuffer hält weiterhin '.' für atof - Komma nur in der Anzeige
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), inputBuf(idx));
        dotToComma(displayBuf);
        stringutils::safeAppend(displayBuf, sizeof(displayBuf), "_");
        spr->drawString(displayBuf, valRightX, cy);

        if (!cursorOn)
        {
            const int cursorW = spr->textWidth("_");
            const int cursorH = fr.h - 2;
            spr->fillRect(valRightX - cursorW, ay + 1, cursorW, cursorH, bgColor);
        }
    }
    else
    {
        // Unfokussierter Eingabewert: aus Puffer kopieren, Komma ersetzen, anzeigen
        const char *buf = inputBuf(idx);
        if (buf[0] != '\0')
        {
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), buf);
            dotToComma(displayBuf);
            spr->drawString(displayBuf, valRightX, cy);
        }
        else
        {
            spr->drawString("0", valRightX, cy);
        }
    }
}

// ===================================================================
// Zeichnen - Ergebnisse
// ===================================================================

/**
 * @brief Zeichnet die Ergebniszeilen unter den Eingabefeldern.
 */
void WorkshopTool_Ne555::drawResults()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_resultsY;
    const int rw = m_rightW;

    const int numResults = (m_mode == Ne555Mode::ASTABLE) ? AST_RESULTS : MONO_RESULTS;
    const char * const *labels = (m_mode == Ne555Mode::ASTABLE)
                                     ? AST_RESULT_LABELS
                                     : MONO_RESULT_LABELS;

    spr->loadFont(ComfortaaBold14);

    for (int i = 0; i < numResults; ++i)
    {
        const int ry = oy + i * RESULT_ROW_H;
        spr->fillRect(ox, ry, rw, RESULT_ROW_H, m_colors.resultBg);
        spr->fillRect(ox, ry + RESULT_ROW_H - 1, rw, 1, C_SEP);

        const int cy = ry + RESULT_ROW_H / 2;

        // Label
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(m_colors.resultLabel, m_colors.resultBg);
        spr->drawString(labels[i], ox + 6, cy);

        // Wert
        char valBuf[32];
        if (!m_computeOk)
        {
            stringutils::safeCopy(valBuf, sizeof(valBuf), "---");
        }
        else if (m_mode == Ne555Mode::ASTABLE)
        {
            switch (i)
            {
                case 0: formatFreq(m_frequency, valBuf, sizeof(valBuf)); break;
                case 1: formatTime(m_period,    valBuf, sizeof(valBuf)); break;
                case 2:
                    stringutils::safeFormat(valBuf, sizeof(valBuf), "%.1f %%", m_dutyCycle);
                    dotToComma(valBuf);
                    break;
                case 3: formatTime(m_tHigh,     valBuf, sizeof(valBuf)); break;
                case 4: formatTime(m_tLow,      valBuf, sizeof(valBuf)); break;
                default: valBuf[0] = '\0'; break;
            }
        }
        else
        {
            formatTime(m_tPulse, valBuf, sizeof(valBuf));
        }

        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(m_colors.resultValue, m_colors.resultBg);
        spr->drawString(valBuf, ox + rw - 6, cy);
    }

    spr->unloadFont();
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Events mit Touch-up-Semantik.
 */
bool WorkshopTool_Ne555::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        // Treffertest: Schaltplan-Bauteile (linke Spalte)
        if (cx < SCHEM_W)
        {
            const int hitComp = hitTestComponent(cx, cy);
            if (hitComp >= 0 && hitComp != m_pressedComp)
            {
                m_pressedComp  = hitComp;
                m_pressedField = -1;
                m_pressedKey   = -1;
                needsRedraw    = true;
            }
        }
        else if (cx >= m_rightX)
        {
            // Treffertest: Eingabefelder
            const int hitField = hitTestField(cx, cy);
            if (hitField >= 0)
            {
                if (hitField != m_pressedField)
                {
                    m_pressedField = hitField;
                    m_pressedComp  = -1;
                    m_pressedKey   = -1;
                    needsRedraw    = true;
                }
            }
            else
            {
                // Treffertest: Numpad (lokale Koordinaten)
                const int hitKey = m_numpad.hitTest(cx - m_rightX, cy);
                if (hitKey >= 0 && hitKey != m_pressedKey)
                {
                    m_pressedKey   = hitKey;
                    m_pressedField = -1;
                    m_pressedComp  = -1;
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
            // Bauteil-Tap -> Feld aktivieren
            if (m_pressedComp >= 0 && m_pressedComp < fieldCount())
            {
                m_activeField = m_pressedComp;
                needsRedraw = true;
            }
            // Feld-Tap -> Feld aktivieren
            else if (m_pressedField >= 0 && m_pressedField < fieldCount())
            {
                m_activeField = m_pressedField;
                needsRedraw = true;
            }
            // Numpad-Tap -> Taste ausführen
            else if (m_pressedKey >= 0 && m_pressedKey < WorkshopNumpad::NUM_KEYS)
            {
                executeKey(WorkshopNumpad::keyAction(m_pressedKey));
                needsRedraw = true;
            }
        }

        m_pressedField = -1;
        m_pressedComp  = -1;
        m_pressedKey   = -1;
        m_touchWas     = false;
    }

    return needsRedraw;
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf Eingabefelder.
 */
int WorkshopTool_Ne555::hitTestField(int cx, int cy) const
{
    const int nf = fieldCount();
    for (int i = 0; i < nf; ++i)
    {
        const Rect &r = m_fieldRects[i];
        if (cx >= r.x && cx < r.x + r.w &&
            cy >= r.y && cy < r.y + r.h)
            return i;
    }
    return -1;
}

/**
 * @brief Treffertest auf Schaltplan-Bauteile.
 */
int WorkshopTool_Ne555::hitTestComponent(int cx, int cy) const
{
    const int nf = fieldCount();
    for (int i = 0; i < nf; ++i)
    {
        const Rect &r = m_compHits[i];
        if (cx >= r.x && cx < r.x + r.w &&
            cy >= r.y && cy < r.y + r.h)
            return i;
    }
    return -1;
}

// ===================================================================
// Eingabeverarbeitung
// ===================================================================

/**
 * @brief Verarbeitet eine Numpad-Tasten-Aktion auf dem aktiven Feld.
 */
void WorkshopTool_Ne555::executeKey(char action)
{
    char * const buf = inputBuf(m_activeField);
    const size_t len = strlen(buf);

    switch (action)
    {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            if (len + 1 >= FIELD_CAP) break;
            buf[len]     = action;
            buf[len + 1] = '\0';
            break;
        }
        case '.':
        {
            if (strchr(buf, '.') != nullptr) break;
            if (len + 2 >= FIELD_CAP) break;
            if (len == 0 || (len == 1 && buf[0] == '-'))
            {
                memmove(buf + (len == 0 ? 1 : 2), buf + len, 1);
                buf[len]     = '0';
                buf[len + 1] = '.';
                buf[len + 2] = '\0';
            }
            else
            {
                buf[len]     = '.';
                buf[len + 1] = '\0';
            }
            break;
        }
        case 'B':
        {
            if (len > 0) buf[len - 1] = '\0';
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
                buf[0] = '-'; buf[1] = '\0';
            }
            else if (buf[0] == '-')
            {
                memmove(buf, buf + 1, len);
            }
            else
            {
                if (len + 1 >= FIELD_CAP) break;
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
// Formatierung
// ===================================================================

/**
 * @brief Formatiert einen Zeitwert mit automatischer Skalierung.
 */
void WorkshopTool_Ne555::formatTime(double seconds, char *buf, size_t sz)
{
    if (!buf || sz == 0) return;

    if (!std::isfinite(seconds) || seconds <= 0.0)
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    if (seconds >= 1.0)
        stringutils::safeFormat(buf, sz, "%.4g s", seconds);
    else if (seconds >= 1.0e-3)
        stringutils::safeFormat(buf, sz, "%.4g ms", seconds * 1.0e3);
    else if (seconds >= 1.0e-6)
        stringutils::safeFormat(buf, sz, "%.4g us", seconds * 1.0e6);
    else
        stringutils::safeFormat(buf, sz, "%.4g ns", seconds * 1.0e9);

    dotToComma(buf);
}

/**
 * @brief Formatiert eine Frequenz mit automatischer Skalierung.
 */
void WorkshopTool_Ne555::formatFreq(double hz, char *buf, size_t sz)
{
    if (!buf || sz == 0) return;

    if (!std::isfinite(hz) || hz <= 0.0)
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    if (hz >= 1.0e6)
        stringutils::safeFormat(buf, sz, "%.4g MHz", hz / 1.0e6);
    else if (hz >= 1.0e3)
        stringutils::safeFormat(buf, sz, "%.4g kHz", hz / 1.0e3);
    else
        stringutils::safeFormat(buf, sz, "%.4g Hz", hz);

    dotToComma(buf);
}

/**
 * @brief Formatiert einen Bauteilwert kompakt für die Schaltplan-Beschriftung.
 */
void WorkshopTool_Ne555::formatSchematicValue(int fieldIdx, char *buf, size_t sz) const
{
    if (!buf || sz == 0) return;

    const double val = fieldValue(fieldIdx);
    if (val <= 0.0 || !std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "?");
        return;
    }

    if (isResistor(fieldIdx))
    {
        // Eingabe in kOhm
        if (val >= 1000.0)
            stringutils::safeFormat(buf, sz, "%.3gM", val / 1000.0);
        else
            stringutils::safeFormat(buf, sz, "%.3gk", val);
    }
    else
    {
        // Eingabe in uF
        if (val >= 1.0)
            stringutils::safeFormat(buf, sz, "%.3gu", val);
        else if (val >= 0.001)
            stringutils::safeFormat(buf, sz, "%.3gn", val * 1000.0);
        else
            stringutils::safeFormat(buf, sz, "%.3gp", val * 1.0e6);
    }

    dotToComma(buf);
}

// ===================================================================
// Modusspezifische Zugriffsmethoden
// ===================================================================

/**
 * @brief Anzahl Eingabefelder im aktuellen Modus.
 */
int WorkshopTool_Ne555::fieldCount() const
{
    return (m_mode == Ne555Mode::ASTABLE) ? AST_FIELDS : MONO_FIELDS;
}

/**
 * @brief Kurzlabel des Feldes.
 */
const char *WorkshopTool_Ne555::fieldLabel(int idx) const
{
    if (m_mode == Ne555Mode::ASTABLE)
        return (idx >= 0 && idx < AST_FIELDS) ? AST_LABELS[idx] : "";
    return (idx >= 0 && idx < MONO_FIELDS) ? MONO_LABELS[idx] : "";
}

/**
 * @brief Einheitentext des Feldes.
 */
const char *WorkshopTool_Ne555::fieldUnit(int idx) const
{
    if (m_mode == Ne555Mode::ASTABLE)
        return (idx >= 0 && idx < AST_FIELDS) ? AST_UNITS[idx] : "";
    return (idx >= 0 && idx < MONO_FIELDS) ? MONO_UNITS[idx] : "";
}

/**
 * @brief Zeiger auf den Eingabepuffer des Feldes.
 */
char *WorkshopTool_Ne555::inputBuf(int idx)
{
    if (m_mode == Ne555Mode::ASTABLE)
        return (idx >= 0 && idx < AST_FIELDS) ? m_astBuf[idx] : m_astBuf[0];
    return (idx >= 0 && idx < MONO_FIELDS) ? m_monoBuf[idx] : m_monoBuf[0];
}

/**
 * @brief Zeiger auf den Eingabepuffer (const).
 */
const char *WorkshopTool_Ne555::inputBuf(int idx) const
{
    if (m_mode == Ne555Mode::ASTABLE)
        return (idx >= 0 && idx < AST_FIELDS) ? m_astBuf[idx] : m_astBuf[0];
    return (idx >= 0 && idx < MONO_FIELDS) ? m_monoBuf[idx] : m_monoBuf[0];
}

/**
 * @brief Referenz auf den SI-Wert des Feldes.
 */
double &WorkshopTool_Ne555::fieldValue(int idx)
{
    if (m_mode == Ne555Mode::ASTABLE)
        return (idx >= 0 && idx < AST_FIELDS) ? m_astVal[idx] : m_astVal[0];
    return (idx >= 0 && idx < MONO_FIELDS) ? m_monoVal[idx] : m_monoVal[0];
}

/**
 * @brief SI-Wert des Feldes (const).
 */
double WorkshopTool_Ne555::fieldValue(int idx) const
{
    if (m_mode == Ne555Mode::ASTABLE)
        return (idx >= 0 && idx < AST_FIELDS) ? m_astVal[idx] : 0.0;
    return (idx >= 0 && idx < MONO_FIELDS) ? m_monoVal[idx] : 0.0;
}

/**
 * @brief true wenn das Feld einen Widerstand repräsentiert.
 */
bool WorkshopTool_Ne555::isResistor(int idx) const
{
    if (m_mode == Ne555Mode::ASTABLE)
        return (idx == 0 || idx == 1); // R1, R2
    return (idx == 0);                  // R
}

/**
 * @brief Akzentfarbe des Feldes.
 */
uint16_t WorkshopTool_Ne555::accentColor(int idx) const
{
    if (m_mode == Ne555Mode::ASTABLE)
    {
        switch (idx)
        {
            case 0:  return C_ACCENT_R1;
            case 1:  return C_ACCENT_R2;
            case 2:  return C_ACCENT_C;
            default: return 0xFFFF;
        }
    }
    // Monostabil: R = Grün, C = Cyan
    return (idx == 0) ? C_ACCENT_R1 : C_ACCENT_C;
}
