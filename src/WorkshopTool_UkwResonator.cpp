#include "WorkshopTool_UkwResonator.h"

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

// ===================================================================
// Spaltenaufteilung
// ===================================================================

constexpr int SCHEM_W = 250; ///< Breite der linken Spalte (Schaltplan + Spule)
constexpr int COL_GAP = 12;  ///< Abstand zwischen Spalten

// ===================================================================
// Hilfsfunktion - Dezimalkomma (deutsches Format)
// ===================================================================

/**
 * @brief Ersetzt alle Dezimalpunkte durch Kommas im übergebenen Puffer.
 */
void dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.') *p = ',';
}

// ===================================================================
// Schaltplan-Geometrie (relativ zum Schaltplan-Origin)
// ===================================================================

// Das Schaltbild ist ein klassischer Parallelschwingkreis: zwei waagerechte Sammelschienen, dazwischen C (links) und L (rechts), zwei Anschluss-Dots links auf den Schienen. Keine GND-Schiene, keine Vin/Vout-Beschriftung. Die Beschriftungen stehen seitlich neben den Bauteilen (C links, L rechts).

constexpr int TERM_X      = 35;  ///< X-Position der beiden Anschluss-Dots
constexpr int LEFT_BUS_X  = 110; ///< Mittelachse C (vertikal)
constexpr int RIGHT_BUS_X = 205; ///< Mittelachse L (vertikal)

constexpr int TOP_RAIL_Y = 20;   ///< Y der oberen Sammelschiene
constexpr int BOT_RAIL_Y = 135;  ///< Y der unteren Sammelschiene

constexpr int TRANS_W     = 30;  ///< Bauteil-Boxbreite (Referenz für Kondensator-Platten)
constexpr int TRANS_H     = 95;  ///< Bauteil-Boxhöhe
constexpr int COMP_TOP_Y  = (TOP_RAIL_Y + BOT_RAIL_Y - TRANS_H) / 2; // 30
constexpr int COMP_BOT_Y  = COMP_TOP_Y + TRANS_H;                    // 125

constexpr int TERM_DOT_R  = 4;   ///< Radius der linken Anschlusspunkte
constexpr int JUNCT_DOT_R = 3;   ///< Radius der T-Knotenpunkte

// Plattenbreite des Kondensators - auch für die Platzierung des C-Labels gebraucht
constexpr int CAP_PLATE_W     = 28; ///< Breite der Kondensator-Platten
constexpr int INDUCTOR_BUMP_R = 11; ///< Radius der Induktor-Halbkreise

// Label-Layout neben den Bauteilen
constexpr int LABEL_GAP = 4;  ///< Horizontaler Abstand Label zum Bauteilrand
constexpr int LABEL_DY  = 12; ///< Vertikaler Halb-Abstand zwischen Symbol und Wert (ergibt ca. 10 px visuellen Gap)

constexpr int HIT_PAD = 12; ///< Touch-Hit-Padding um Schaltplan-Bauteile

// Coil-Visualisierung (diagonale AA-Wicklungslinien)
constexpr int COIL_AREA_Y      = 175;  ///< Y-Start der Coil-Zone (relativ zum Schaltplan-Origin)
constexpr int COIL_AREA_H      = 50;   ///< Höhe der Coil-Zone
constexpr int COIL_X_OFFSET    = 90;   ///< Linke Kante der Coil-Zone (Platz für D-Bemaßung links)
constexpr int COIL_AREA_W      = 155;  ///< Breite der Coil-Zone

constexpr float COIL_SCALE_MAX = 12.0f; ///< Obere Schranke der Skala (px/mm)
constexpr int   COIL_SLANT     = 5;     ///< Visueller horizontaler Versatz pro Wicklungslinie
constexpr float COIL_LINE_MIN  = 1.5f;  ///< Minimale Strichstärke der Wicklungslinien
constexpr int   COIL_N_MAX     = 30;    ///< Visuelle Cap der Windungsanzahl (Lesbarkeit)

// Bemaßungspfeile
constexpr int DIM_X_LEFT     = 70; ///< X-Position des vertikalen D-Pfeils
constexpr int LARROW_Y       = 240;///< Y-Position des horizontalen ℓ-Pfeils
constexpr int DIM_ARROW_LEN  = 6;  ///< Länge der Pfeilspitzen entlang der Hauptachse
constexpr int DIM_ARROW_HW   = 3;  ///< Halbe Breite der Pfeilspitzen quer zur Hauptachse
constexpr int DIM_TEXT_GAP   = 4;  ///< Abstand Label zur Pfeillinie

constexpr uint16_t C_DIM = 0x9492; ///< Bemaßungs-Farbe (warm-grau, identisch mit Parameterfeld-Akzent)

// Text-Labels (N und Pitch unter dem Coil)
constexpr int COIL_TEXT_Y  = 272;  ///< Y-Position des N-Labels
constexpr int COIL_TEXT_DY = 21;   ///< Vertikaler Abstand N -> Pitch

// ===================================================================
// Rechte Spalte - Farben
// ===================================================================

constexpr uint16_t C_BG          = 0x1082;
constexpr uint16_t C_FIELD_INPUT = 0x2945;
constexpr uint16_t C_FIELD_FOCUS = 0x328A;
constexpr uint16_t C_FIELD_PRESS = 0x4B4D;
constexpr uint16_t C_SEP         = 0x2104;

constexpr uint16_t C_LBL_FOCUS  = 0xFFFF;
constexpr uint16_t C_LBL_INPUT  = 0xDEDB;
constexpr uint16_t C_LBL_COMP   = 0x7BEF;
constexpr uint16_t C_VAL_FOCUS  = 0xFFFF;
constexpr uint16_t C_VAL_INPUT  = 0xEF7D;
constexpr uint16_t C_VAL_COMP   = 0xB5F7;
constexpr uint16_t C_UNIT_FOCUS = 0xAD75;
constexpr uint16_t C_UNIT_INPUT = 0x8410;
constexpr uint16_t C_UNIT_COMP  = 0x4228;

constexpr uint16_t C_INFO_TXT  = 0xDEDB;
constexpr uint16_t C_WARN_TXT  = 0xFD20; ///< Orange für Bereichswarnung in der Info-Zeile

// Akzentfarben je Größe
constexpr uint16_t C_ACCENT_F = 0xFD20; ///< f - Orange
constexpr uint16_t C_ACCENT_L = 0xFFE0; ///< L - Gelb
constexpr uint16_t C_ACCENT_C = 0x04FF; ///< C - Cyan
constexpr uint16_t C_ACCENT_PARAM = 0x9492; ///< D und ℓ - gedämpftes Warmgrau (Parameterfeld)
constexpr uint16_t C_ACCENT_F_OUT = 0xF800; ///< f außerhalb 80...150 MHz - Rot
constexpr uint16_t C_OVERLAP_RED  = 0xF800; ///< Wicklungs-Überlapp - Rot

// ===================================================================
// Schaltplan-Farben
// ===================================================================

constexpr uint16_t C_SCHEM_BG     = 0x1082;
constexpr uint16_t C_WIRE         = 0x8410;
constexpr uint16_t C_COMP_OUTLINE = 0x6B6D;
constexpr uint16_t C_VAL_DIMMED   = 0x7D7A;
constexpr uint16_t C_RAIL         = 0xAD75;
constexpr uint16_t C_JUNCTION     = 0xAD75;

// ===================================================================
// Footer-Toggle-Farben
// ===================================================================

constexpr uint16_t TOGGLE_ACTIVE_FILL    = 0x328A;
constexpr uint16_t TOGGLE_ACTIVE_TEXT    = 0xFFFF;
constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;
constexpr uint16_t TOGGLE_INACTIVE_FILL    = 0x0000;
constexpr uint16_t TOGGLE_INACTIVE_TEXT    = 0x7BEF;
constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8;

constexpr int FOOTER_BTN_W   = 90;
constexpr int FOOTER_BTN_H   = 44;
constexpr int FOOTER_BTN_R   = 6;
constexpr int FOOTER_BTN_GAP = 8;

// d-Auswahl: vier Drahtdurchmesser für die Luftspule (Cu-Draht)
constexpr double D_VALUES[4] = {0.5e-3, 0.8e-3, 1.0e-3, 1.5e-3};
const char *const D_LABELS[4] = {"0,5 mm", "0,8 mm", "1,0 mm", "1,5 mm"};

// ===================================================================
// Bemaßungspfeil-Helfer
// ===================================================================

// Beide Funktionen erwarten absolute Sprite-Koordinaten und einen vom Aufrufer bereits geladenen Font.

/**
 * @brief Zeichnet einen horizontalen Bemaßungspfeil mit Label darunter.
 */
void drawDimArrowH(Gfx_Sprite *spr, int x0, int x1, int y,
                   const char *label, uint16_t color, uint16_t bg)
{
    if (x1 <= x0 + 2 * DIM_ARROW_LEN) return;

    spr->drawFastHLine(x0 + DIM_ARROW_LEN, y,
                       x1 - x0 - 2 * DIM_ARROW_LEN + 1, color);

    spr->fillTriangle(x0,                 y,
                      x0 + DIM_ARROW_LEN, y - DIM_ARROW_HW,
                      x0 + DIM_ARROW_LEN, y + DIM_ARROW_HW, color);

    spr->fillTriangle(x1,                 y,
                      x1 - DIM_ARROW_LEN, y - DIM_ARROW_HW,
                      x1 - DIM_ARROW_LEN, y + DIM_ARROW_HW, color);

    if (label && *label)
    {
        spr->setTextDatum(TC_DATUM);
        spr->setTextColor(color, bg);
        spr->drawString(label, (x0 + x1) / 2, y + DIM_TEXT_GAP);
    }
}

/**
 * @brief Zeichnet einen vertikalen Bemaßungspfeil mit Label links daneben.
 */
void drawDimArrowV(Gfx_Sprite *spr, int x, int y0, int y1,
                   const char *label, uint16_t color, uint16_t bg)
{
    if (y1 <= y0 + 2 * DIM_ARROW_LEN) return;

    spr->drawFastVLine(x, y0 + DIM_ARROW_LEN,
                       y1 - y0 - 2 * DIM_ARROW_LEN + 1, color);

    spr->fillTriangle(x,                y0,
                      x - DIM_ARROW_HW, y0 + DIM_ARROW_LEN,
                      x + DIM_ARROW_HW, y0 + DIM_ARROW_LEN, color);

    spr->fillTriangle(x,                y1,
                      x - DIM_ARROW_HW, y1 - DIM_ARROW_LEN,
                      x + DIM_ARROW_HW, y1 - DIM_ARROW_LEN, color);

    if (label && *label)
    {
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(color, bg);
        spr->drawString(label, x - DIM_TEXT_GAP, (y0 + y1) / 2);
    }
}

} // namespace

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_UkwResonator::WorkshopTool_UkwResonator()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_UkwResonator::onEnter(LGFX *tft)
{
    clearAll();
    initFooterButtons(tft);
}

/**
 * @brief Aufräumen beim Verlassen.
 */
void WorkshopTool_UkwResonator::onExit()
{
    m_buttonsInitialized = false;
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_UkwResonator::clearAll()
{
    memset(&m_state,     0, sizeof(m_state));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));
    memset(m_compHits,   0, sizeof(m_compHits));

    m_sprite      = nullptr;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;
    m_layoutValid = false;
    m_numpad.clearLayout();

    m_dIdx = 2; // 1,0 mm Cu

    m_computeOk   = false;
    m_z0Ok        = false;
    m_geomOk      = false;
    m_geomOverlap = false;
    m_qOk         = false;
    m_z0          = 0.0;
    m_N           = 0.0;
    m_Q           = 0.0;

    m_pressedField = -1;
    m_pressedComp  = -1;
    m_pressedKey   = -1;
    m_touchWas     = false;
    m_touchReady   = false;

    initState();

    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));
    refreshSubtitle();

    compute();
}

/**
 * @brief Setzt den State auf UKW-Standardwerte (f=100 MHz, C=20 pF, D=6 mm, ℓ=10 mm).
 */
void WorkshopTool_UkwResonator::initState()
{
    memset(&m_state, 0, sizeof(m_state));

    // FIFO: C(2) ältester, f(0) neuester -> L(1) wird berechnet
    m_state.inputSlots[0] = 2; // C (ältester)
    m_state.inputSlots[1] = 0; // f (neuester, fokussiert)
    m_state.computedQty   = 1; // L wird berechnet
    m_state.focusedQty    = 0; // f fokussiert

    // SI-Werte: f=100 MHz, C=20 pF, D=6 mm, ℓ=10 mm. L wird durch compute() ergänzt.
    m_state.values[0] = 100.0e6;  // f  = 100 MHz
    m_state.values[1] = 0.0;       // L  -> Berechnung
    m_state.values[2] = 20.0e-12;  // C  = 20 pF
    m_state.values[3] = 6.0e-3;    // D  = 6 mm
    m_state.values[4] = 10.0e-3;   // ℓ  = 10 mm

    // Eingabepuffer als Mantissen in der jeweiligen Anzeige-Skala
    formatMantissa(m_state.values[0], scaleForQty(0), m_state.bufs[0], FIELD_CAP);
    formatMantissa(m_state.values[2], scaleForQty(2), m_state.bufs[2], FIELD_CAP);
    formatMantissa(m_state.values[3], scaleForQty(3), m_state.bufs[3], FIELD_CAP);
    formatMantissa(m_state.values[4], scaleForQty(4), m_state.bufs[4], FIELD_CAP);
}

// ===================================================================
// Symbol- und Einheiten-Helfer
// ===================================================================

/**
 * @brief Symboltext je Größe.
 */
const char *WorkshopTool_UkwResonator::qtySymbol(int qty)
{
    if (qty == 0) return "f";
    if (qty == 1) return "L";
    if (qty == 2) return "C";
    if (qty == 3) return "D";
    return "l";
}

/**
 * @brief Skalierte Anzeige-Einheit je Größe.
 */
const char *WorkshopTool_UkwResonator::scaledUnit(int qty)
{
    if (qty == 0) return "MHz";
    if (qty == 1) return "nH";
    if (qty == 2) return "pF";
    return "mm"; // D und ℓ sind beide in mm
}

/**
 * @brief Skalierungsfaktor je Größe (Mantisse * scale = SI-Wert).
 */
double WorkshopTool_UkwResonator::scaleForQty(int qty)
{
    if (qty == 0) return 1.0e6;  // MHz -> Hz
    if (qty == 1) return 1.0e-9; // nH  -> H
    if (qty == 2) return 1.0e-12;// pF  -> F
    return 1.0e-3;               // mm  -> m  (D und ℓ)
}

/**
 * @brief Akzentfarbe je Größe - f wird rot wenn außerhalb 80...150 MHz.
 *        D und ℓ teilen sich die gedämpfte Parameterfarbe.
 */
uint16_t WorkshopTool_UkwResonator::accentColor(int qty) const
{
    if (qty == 0)
    {
        if (m_computeOk && !isFrequencyInRange())
            return C_ACCENT_F_OUT;
        return C_ACCENT_F;
    }
    if (qty == 1) return C_ACCENT_L;
    if (qty == 2) return C_ACCENT_C;
    return C_ACCENT_PARAM; // D (3) und ℓ (4)
}

/**
 * @brief Prüft ob eine Größe als Eingabe aktiv ist.
 *
 * D (Index 3) und ℓ (Index 4) sind immer Eingabe - sie nehmen nicht am FIFO teil.
 */
bool WorkshopTool_UkwResonator::isInput(int qty) const
{
    if (qty == 3 || qty == 4) return true;
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (static_cast<int>(m_state.inputSlots[s]) == qty)
            return true;
    return false;
}

/**
 * @brief Prüft ob die aktuelle Frequenz im UKW-Bereich liegt.
 */
bool WorkshopTool_UkwResonator::isFrequencyInRange() const
{
    const double f = m_state.values[0];
    return std::isfinite(f) && f >= F_MIN_HZ && f <= F_MAX_HZ;
}

// ===================================================================
// Footer-Buttons (vier Drahtdurchmesser-Auswahl)
// ===================================================================

/**
 * @brief Erstellt die vier Footer-Toggle-Buttons für die d-Auswahl.
 */
void WorkshopTool_UkwResonator::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int totalW = NUM_D_VALUES * FOOTER_BTN_W +
                           (NUM_D_VALUES - 1) * FOOTER_BTN_GAP;
    constexpr int startX = WorkshopCfg::TOOL_FOOTER_X +
                           (WorkshopCfg::TOOL_FOOTER_W - totalW) / 2;

    for (int i = 0; i < NUM_D_VALUES; ++i)
    {
        const int cx = startX + FOOTER_BTN_W / 2 +
                       i * (FOOTER_BTN_W + FOOTER_BTN_GAP);
        m_btnD[i].initButton(tft, cx, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
                              TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
                              D_LABELS[i], nullptr);
        m_btnD[i].setFont(LeagueGothicCond34);
    }

    m_buttonsInitialized = true;
}

/**
 * @brief Aktualisiert Toggle-Farben - der aktive Index ist ausgefüllt.
 */
void WorkshopTool_UkwResonator::updateButtonColors()
{
    for (int i = 0; i < NUM_D_VALUES; ++i)
    {
        const bool active = (i == m_dIdx);
        m_btnD[i].setFillColor(active ? TOGGLE_ACTIVE_FILL : TOGGLE_INACTIVE_FILL);
        m_btnD[i].setTextColor(active ? TOGGLE_ACTIVE_TEXT : TOGGLE_INACTIVE_TEXT);
        m_btnD[i].setOutlineColor(active ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);
    }
}

/**
 * @brief Zeichnet die vier Footer-Toggle-Buttons.
 */
void WorkshopTool_UkwResonator::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft; (void)x; (void)y; (void)w; (void)h;
    if (!m_buttonsInitialized)
        return;

    updateButtonColors();
    for (int i = 0; i < NUM_D_VALUES; ++i)
        m_btnD[i].drawButton();
}

/**
 * @brief Verarbeitet Touch auf den Footer-Buttons.
 */
bool WorkshopTool_UkwResonator::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    bool changed = false;

    for (int i = 0; i < NUM_D_VALUES; ++i)
    {
        const bool on  = touched && m_btnD[i].contains(tx, ty);
        const bool was = m_btnD[i].isPressed();
        m_btnD[i].press(on);

        // Drahtdurchmesser-Wechsel bei Loslassen
        if (m_btnD[i].justReleased() && m_dIdx != i)
        {
            m_dIdx        = i;
            refreshSubtitle();
            m_layoutValid = false;
            compute();
            changed = true;
        }

        // Visuelle Rückmeldung
        if (!changed && on != was)
            m_btnD[i].drawButton(on);
    }

    return changed;
}

/**
 * @brief Dynamischer Untertitel - vier statische Strings je nach d-Auswahl.
 */
const char *WorkshopTool_UkwResonator::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Befüllt m_subtitleBuf passend zu m_dIdx.
 */
void WorkshopTool_UkwResonator::refreshSubtitle()
{
    static const WorkshopKey SUBTITLE_KEYS[NUM_D_VALUES] = {
        WorkshopKey::WORKSHOP_UKW_RESONATOR_SUB_D05,
        WorkshopKey::WORKSHOP_UKW_RESONATOR_SUB_D08,
        WorkshopKey::WORKSHOP_UKW_RESONATOR_SUB_D10,
        WorkshopKey::WORKSHOP_UKW_RESONATOR_SUB_D15
    };
    int idx = m_dIdx;
    if (idx < 0 || idx >= NUM_D_VALUES)
        idx = 2;
    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), SUBTITLE_KEYS[idx]);
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Schaltplan, Spulendarstellung, Felder, Info-Zeile und Numpad.
 */
void WorkshopTool_UkwResonator::draw(Gfx_Sprite *sprite,
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
    drawSpool();
    drawFields(cursorOn);
    drawInfoRow();
    m_numpad.draw(m_sprite, m_originX + m_rightX, m_originY, m_pressedKey);
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Spaltenaufteilung, Feld-Rects, Hit-Areas, Numpad.
 */
void WorkshopTool_UkwResonator::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;
    if (m_rightW < 100) m_rightW = 100;

    // Eingabefelder (rechte Spalte, unter Headerbereich mit TOP_GAP)
    for (int i = 0; i < NUM_QTY; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * ROW_H, m_rightW, ROW_H};

    // Info-Zeile direkt nach den Feldern
    m_infoY = TOP_GAP + NUM_QTY * ROW_H + INFO_GAP_V;

    // Numpad am unteren Rand der rechten Spalte
    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);

    // Schaltplan-Touch-Areas (content-relativ, unter TOP_GAP) Index 0 = L (rechte Vertikale), Index 1 = C (linke Vertikale)
    m_compHits[0] = {RIGHT_BUS_X - TRANS_W / 2 - HIT_PAD,
                     TOP_GAP + COMP_TOP_Y - HIT_PAD,
                     TRANS_W + 2 * HIT_PAD,
                     TRANS_H + 2 * HIT_PAD};

    m_compHits[1] = {LEFT_BUS_X - TRANS_W / 2 - HIT_PAD,
                     TOP_GAP + COMP_TOP_Y - HIT_PAD,
                     TRANS_W + 2 * HIT_PAD,
                     TRANS_H + 2 * HIT_PAD};
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Parst Eingabepuffer und berechnet die Ausgabegröße + Spulendaten.
 */
void WorkshopTool_UkwResonator::compute()
{
    State &s = m_state;

    // Eingabewerte aus Puffern parsen - Mantisse × Skala ergibt SI-Wert
    for (int slot = 0; slot < NUM_INPUT_SLOTS; ++slot)
    {
        const int q = static_cast<int>(s.inputSlots[slot]);
        const double scale = scaleForQty(q);
        s.values[q] = (s.bufs[q][0] != '\0') ? atof(s.bufs[q]) * scale : 0.0;
    }
    // D und ℓ werden immer geparst (eigenständige Parameterfelder)
    s.values[3] = (s.bufs[3][0] != '\0') ? atof(s.bufs[3]) * scaleForQty(3) : 0.0;
    s.values[4] = (s.bufs[4][0] != '\0') ? atof(s.bufs[4]) * scaleForQty(4) : 0.0;

    // Alle Folge-Flags zurücksetzen
    m_computeOk   = false;
    m_z0Ok        = false;
    m_geomOk      = false;
    m_geomOverlap = false;
    m_qOk         = false;
    m_z0          = 0.0;
    m_N           = 0.0;
    m_Q           = 0.0;

    // Finite-Check für die FIFO-Eingaben
    for (int slot = 0; slot < NUM_INPUT_SLOTS; ++slot)
        if (!std::isfinite(s.values[static_cast<int>(s.inputSlots[slot])]))
            return;

    const double f = s.values[0]; // Frequenz (Hz)
    const double l = s.values[1]; // Induktivität (H)
    const double c = s.values[2]; // Kapazität (F)

    // ===================================================================
    // Schritt 1 - fehlende FIFO-Größe berechnen
    // ===================================================================
    switch (s.computedQty)
    {
        case 0: // f berechnen: f = 1 / (2π · √(L · C))
        {
            if (l <= 0.0 || c <= 0.0) return;
            const double product = l * c;
            if (product <= 0.0) return;
            const double result = 1.0 / (UKW_TWO_PI * std::sqrt(product));
            if (!std::isfinite(result) || result <= 0.0) return;
            s.values[0] = result;
            m_computeOk = true;
            break;
        }
        case 1: // L berechnen: L = 1 / ((2π · f)² · C)
        {
            if (f <= 0.0 || c <= 0.0) return;
            const double w0 = UKW_TWO_PI * f;
            const double result = 1.0 / (w0 * w0 * c);
            if (!std::isfinite(result) || result <= 0.0) return;
            s.values[1] = result;
            m_computeOk = true;
            break;
        }
        case 2: // C berechnen: C = 1 / ((2π · f)² · L)
        {
            if (f <= 0.0 || l <= 0.0) return;
            const double w0 = UKW_TWO_PI * f;
            const double result = 1.0 / (w0 * w0 * l);
            if (!std::isfinite(result) || result <= 0.0) return;
            s.values[2] = result;
            m_computeOk = true;
            break;
        }
        default:
            return;
    }

    // Aktuelle (jetzt vollständige) Werte
    const double fNow  = s.values[0];
    const double lNow  = s.values[1];
    const double cNow  = s.values[2];
    const double dDorn = s.values[3]; // m
    const double lCoil = s.values[4]; // m
    const double dWire = D_VALUES[(m_dIdx >= 0 && m_dIdx < NUM_D_VALUES) ? m_dIdx : 2];

    // ===================================================================
    // Schritt 2 - Kennimpedanz Z₀ = √(L/C)
    // ===================================================================
    if (lNow > 0.0 && cNow > 0.0)
    {
        const double z = std::sqrt(lNow / cNow);
        if (std::isfinite(z) && z > 0.0)
        {
            m_z0   = z;
            m_z0Ok = true;
        }
    }

    // ===================================================================
    // Schritt 3 - Wheeler-Direktformel für die Windungszahl N
    // ===================================================================

    // Wheeler (metrisch, einlagig):
    //   L[µH] = D²·N² / (0,4572·D + 1,016·ℓ)
    //
    // Mit ℓ als Eingabe wird das eine simple Direktformel für N:
    //   N² = L[µH] · (0,4572·D + 1,016·ℓ) / D²
    //   N  = √(...)
    if (dDorn <= 0.0 || dWire <= 0.0 || lNow <= 0.0 || lCoil <= 0.0)
        return;

    const double L_uH  = lNow * 1.0e6;
    const double denom = dDorn * dDorn;
    if (denom <= 0.0) return;

    const double n2 = L_uH * (WHEELER_A * dDorn + WHEELER_B * lCoil) / denom;
    if (n2 <= 0.0 || !std::isfinite(n2)) return;

    const double nCalc = std::sqrt(n2);
    if (!std::isfinite(nCalc) || nCalc <= 0.0) return;

    m_N      = nCalc;
    m_geomOk = true;

    // ===================================================================
    // Schritt 4 - Plausibilitätsprüfung
    // ===================================================================

    // Prüft, ob N Windungen mit dem Drahtdurchmesser d in die vorgegebene Spulenlänge ℓ passen. Wenn N · d > ℓ, würden sich die Wicklungen überlappen. Das ist bei lackiertem Cu-Draht wegen des Lacks nicht möglich und führt bei versilbertem Draht zu Kurzschlüssen. Der Zustand wird visuell und in der Info-Zeile signalisiert.
    m_geomOverlap = (m_N * dWire > lCoil);

    // ===================================================================
    // Schritt 5 - Q-Faktor mit Skineffekt und Medhurst-Proximity-Korrektur
    // ===================================================================
    if (fNow <= 0.0) return;

    const double delta = SKIN_K / std::sqrt(fNow); // Skintiefe in m
    if (delta <= 0.0) return;

    const double rAc = RHO_COPPER * m_N * dDorn / (dWire * delta);
    if (rAc <= 0.0 || !std::isfinite(rAc)) return;

    const double omegaL = UKW_TWO_PI * fNow * lNow;
    const double qIdeal = omegaL / rAc;
    const double lOverD = lCoil / dDorn;
    const double psi    = 1.0 + 2.0 / (lOverD + 0.4);
    const double qReal  = qIdeal / psi;

    if (std::isfinite(qReal) && qReal > 0.0)
    {
        m_Q   = qReal;
        m_qOk = true;
    }
}

// ===================================================================
// Formatierung
// ===================================================================

/**
 * @brief Formatiert einen SI-Wert als Mantisse für eine gegebene Skala.
 */
void WorkshopTool_UkwResonator::formatMantissa(double siValue, double scale,
                                                char *buf, size_t sz)
{
    if (!buf || sz == 0) return;

    if (!std::isfinite(siValue) || scale == 0.0)
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }
    if (siValue == 0.0)
    {
        stringutils::safeCopy(buf, sz, "0");
        return;
    }

    const double mantissa = siValue / scale;
    stringutils::safeFormat(buf, sz, "%.6g", mantissa);
}

/**
 * @brief Formatiert einen Bauteilwert für die Schaltplan-Beschriftung.
 */
void WorkshopTool_UkwResonator::formatSchematicValue(int qty, char *buf, size_t sz) const
{
    if (!buf || sz == 0) return;

    const double val = m_state.values[qty];

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "?");
        return;
    }

    const double absVal = std::abs(val);

    if (qty == 0) // Frequenz
    {
        if (absVal == 0.0)
            stringutils::safeCopy(buf, sz, "0");
        else if (absVal >= 1.0e6)
            stringutils::safeFormat(buf, sz, "%.3gMHz", val / 1.0e6);
        else if (absVal >= 1.0e3)
            stringutils::safeFormat(buf, sz, "%.3gkHz", val / 1.0e3);
        else
            stringutils::safeFormat(buf, sz, "%.3gHz", val);
    }
    else if (qty == 1) // Induktivität
    {
        if (absVal >= 1.0e-6)
            stringutils::safeFormat(buf, sz, "%.3gu", val * 1.0e6);
        else
            stringutils::safeFormat(buf, sz, "%.3gn", val * 1.0e9);
    }
    else if (qty == 2) // Kapazität
    {
        if (absVal >= 1.0e-9)
            stringutils::safeFormat(buf, sz, "%.3gn", val * 1.0e9);
        else
            stringutils::safeFormat(buf, sz, "%.3gp", val * 1.0e12);
    }
    else if (qty == 3) // D - Dorndurchmesser
    {
        stringutils::safeFormat(buf, sz, "%.3gmm", val * 1.0e3);
    }
    else // ℓ - Spulenlänge
    {
        stringutils::safeFormat(buf, sz, "%.3gmm", val * 1.0e3);
    }

    dotToComma(buf);
}

// ===================================================================
// Zeichnen - Schaltplan (Parallelschwingkreis)
// ===================================================================

/**
 * @brief Zeichnet den kompletten Parallelschwingkreis-Schaltplan.
 *
 * Topologie: Zwei Anschluss-Dots links auf den Sammelschienen, dazwischen C (linke Vertikale) und L (rechte Vertikale) parallel zwischen oberer und unterer Schiene. Keine GND-Schiene, keine Vin/Vout-Beschriftung.
 */
void WorkshopTool_UkwResonator::drawSchematic()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    // Hintergrund (linke Spalte komplett - Schaltplan + Spule + Labels)
    spr->fillRect(ox, oy, SCHEM_W, m_h - TOP_GAP, C_SCHEM_BG);

    const bool lActive = (static_cast<int>(m_state.focusedQty) == 1);
    const bool cActive = (static_cast<int>(m_state.focusedQty) == 2);

    // ===================================================================
    // Verdrahtung
    // ===================================================================

    // Obere Sammelschiene: vom Anschluss-Dot bis zur rechten Vertikale
    spr->drawFastHLine(ox + TERM_X, oy + TOP_RAIL_Y,
                       RIGHT_BUS_X - TERM_X + 1, C_WIRE);

    // Untere Sammelschiene
    spr->drawFastHLine(ox + TERM_X, oy + BOT_RAIL_Y,
                       RIGHT_BUS_X - TERM_X + 1, C_WIRE);

    // Vertikale Stub-Wires von den Sammelschienen zu den Bauteilen (die Bauteile selbst zeichnen ihre Anschlussleitungen innerhalb der Box)
    spr->drawFastVLine(ox + LEFT_BUS_X, oy + TOP_RAIL_Y,
                       COMP_TOP_Y - TOP_RAIL_Y, C_WIRE);
    spr->drawFastVLine(ox + LEFT_BUS_X, oy + COMP_BOT_Y,
                       BOT_RAIL_Y - COMP_BOT_Y, C_WIRE);
    spr->drawFastVLine(ox + RIGHT_BUS_X, oy + TOP_RAIL_Y,
                       COMP_TOP_Y - TOP_RAIL_Y, C_WIRE);
    spr->drawFastVLine(ox + RIGHT_BUS_X, oy + COMP_BOT_Y,
                       BOT_RAIL_Y - COMP_BOT_Y, C_WIRE);

    // T-Knotenpunkte nur am Kondensator - dort verzweigt die obere Rail nach unten zum C-Bauteil, während sie horizontal weiter nach rechts zum L läuft (echtes T-Stück). An der Spule endet die Rail hingegen mit einem reinen 90°-Knick - in Schaltplänen üblicherweise OHNE Punkt.
    spr->fillCircle(ox + LEFT_BUS_X, oy + TOP_RAIL_Y, JUNCT_DOT_R, C_JUNCTION);
    spr->fillCircle(ox + LEFT_BUS_X, oy + BOT_RAIL_Y, JUNCT_DOT_R, C_JUNCTION);

    // Anschluss-Dots links (zwei Terminale für den Resonator)
    spr->fillCircle(ox + TERM_X, oy + TOP_RAIL_Y, TERM_DOT_R, C_RAIL);
    spr->fillCircle(ox + TERM_X, oy + BOT_RAIL_Y, TERM_DOT_R, C_RAIL);

    // ===================================================================
    // Bauteile
    // ===================================================================

    drawCapacitorVert(LEFT_BUS_X,  COMP_TOP_Y, cActive);
    drawInductorVert (RIGHT_BUS_X, COMP_TOP_Y, lActive);

    // ===================================================================
    // Beschriftungen links neben den Bauteilen
    // ===================================================================

    // C: Symbol "C" und Wert stehen links vom Kondensator, rechtsbündig an einer Vertikalen 4 px links des linken Plattenrands. L: Symbol "L" und Wert stehen ebenfalls links von der Spule, rechtsbündig 4 px links der Mittelachse. Da die Bögen nach rechts ausbuchten, ist die Mittelachse zugleich der linke Bauteilrand. Beide Label-Blöcke sind vertikal um die Bauteilmitte zentriert, mit dem Symbol darüber und dem Wert darunter, jeweils LABEL_DY vom Mittelpunkt versetzt.

    const int midY = COMP_TOP_Y + TRANS_H / 2;

    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MR_DATUM);

    // C-Beschriftung (rechts-bündig links vom Kondensator)
    {
        const uint16_t clr = cActive ? C_ACCENT_C : C_VAL_DIMMED;
        spr->setTextColor(clr, C_SCHEM_BG);

        const int labelX = LEFT_BUS_X - CAP_PLATE_W / 2 - LABEL_GAP;
        spr->drawString("C", ox + labelX, oy + midY - LABEL_DY);

        char buf[16];
        formatSchematicValue(2, buf, sizeof(buf));
        spr->drawString(buf, ox + labelX, oy + midY + LABEL_DY);
    }

    // L-Beschriftung (rechts-bündig links von der Spule)
    {
        const uint16_t clr = lActive ? C_ACCENT_L : C_VAL_DIMMED;
        spr->setTextColor(clr, C_SCHEM_BG);

        const int labelX = RIGHT_BUS_X - LABEL_GAP;
        spr->drawString("L", ox + labelX, oy + midY - LABEL_DY);

        char buf[16];
        formatSchematicValue(1, buf, sizeof(buf));
        spr->drawString(buf, ox + labelX, oy + midY + LABEL_DY);
    }

    spr->unloadFont();
}

// ===================================================================
// Zeichnen - Bauteil-Symbole (vertikal)
// ===================================================================

/**
 * @brief Zeichnet einen vertikalen Kondensator (zwei horizontale Platten).
 */
void WorkshopTool_UkwResonator::drawCapacitorVert(int centerX, int topY, bool active)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    const int plateGap  = 10;
    const int plateW    = CAP_PLATE_W;
    const int midY      = topY + TRANS_H / 2;
    const int topPlateY = midY - plateGap / 2;
    const int botPlateY = midY + plateGap / 2;
    const uint16_t ol   = active ? C_ACCENT_C : C_COMP_OUTLINE;

    // Verbindungsleitungen oben/unten zur Platte
    spr->drawFastVLine(ox + centerX, oy + topY,
                       topPlateY - topY, ol);
    spr->drawFastVLine(ox + centerX, oy + botPlateY,
                       topY + TRANS_H - botPlateY, ol);

    // Zwei horizontale Platten (3 px dick)
    spr->fillRect(ox + centerX - plateW / 2, oy + topPlateY - 1,
                  plateW, 3, ol);
    spr->fillRect(ox + centerX - plateW / 2, oy + botPlateY - 1,
                  plateW, 3, ol);
}

/**
 * @brief Zeichnet eine vertikale Spule (vier AA-Halbkreise rechts der Mittelachse).
 *
 * Die Halbkreise buchten nach außen (rechts) aus, die vertikale Anschluss- Mittelachse verläuft auf der linken Seite bei centerX. Dadurch zeigt die Wellenkontur vom Parallelschwingkreis weg und blockiert nicht den Blick auf den Kondensator links daneben.
 */
void WorkshopTool_UkwResonator::drawInductorVert(int centerX, int topY, bool active)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    const int   numBumps  = 4;
    const float bumpR     = static_cast<float>(INDUCTOR_BUMP_R);
    const float thickness = 2.5f;
    const int   bumpD     = static_cast<int>(2.0f * bumpR) - 1;
    const int   totalH    = (numBumps - 1) * bumpD + static_cast<int>(2.0f * bumpR);
    const int   padY      = (TRANS_H - totalH) / 2;
    const int   firstCy   = topY + padY + static_cast<int>(bumpR);
    const uint16_t ol     = active ? C_ACCENT_L : C_COMP_OUTLINE;

    // Kurze Anschlussleitungen oben und unten (außerhalb der Bumps)
    if (padY > 0)
    {
        spr->drawFastVLine(ox + centerX, oy + topY, padY, ol);
        spr->drawFastVLine(ox + centerX, oy + topY + TRANS_H - padY, padY, ol);
    }

    // Vier AA-Kreise untereinander (Mittelpunkt exakt auf der Anschluss-Mittelachse)
    for (int i = 0; i < numBumps; ++i)
    {
        const int cy = firstCy + i * bumpD;
        spr->drawSmoothCircle(ox + centerX, oy + cy, bumpR,
                              thickness, ol, C_SCHEM_BG);
    }

    // Linke Hälfte aller Bumps mit BG wegschneiden - übrig bleibt die rechte Wellenkontur. Das Rechteck endet exakt bei (centerX - 1), sodass die vertikale Mittelachse bei centerX erhalten bleibt.
    const int wipeW = static_cast<int>(bumpR) + 2;
    spr->fillRect(ox + centerX - wipeW, oy + topY + padY,
                  wipeW, totalH, C_SCHEM_BG);
}

// ===================================================================
// Zeichnen - Spulen-Seitenansicht
// ===================================================================

/**
 * @brief Zeichnet die Spulen-Seitenansicht mit Bemaßungspfeilen.
 */
void WorkshopTool_UkwResonator::drawSpool()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    // Hintergrund des kompletten Coil-Bereichs säubern
    spr->fillRect(ox, oy + COIL_AREA_Y, SCHEM_W,
                  m_h - TOP_GAP - COIL_AREA_Y, C_BG);

    // Bei ungültiger Berechnung nur Platzhalter anzeigen
    if (!m_geomOk || m_N <= 0.0)
    {
        spr->loadFont(ComfortaaBold14);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_VAL_DIMMED, C_BG);
        spr->drawString("---", ox + SCHEM_W / 2,
                        oy + COIL_AREA_Y + COIL_AREA_H / 2);
        spr->unloadFont();
        return;
    }

    const double dDorn = m_state.values[3];
    const double lCoil = m_state.values[4];
    const int dIdx = (m_dIdx >= 0 && m_dIdx < NUM_D_VALUES) ? m_dIdx : 2;
    const double dWire = D_VALUES[dIdx];
    if (dDorn <= 0.0 || lCoil <= 0.0) return;

    const double lengthMm   = lCoil * 1000.0;
    const double diameterMm = dDorn * 1000.0;
    const double dWireMm    = dWire * 1000.0;

    // Adaptive Skala (px/mm), gemeinsam für beide Achsen. Nur nach oben deckeln - eine untere Schranke würde bei langen, schlanken Spulen die Geometrie aus der Coil-Zone herausdrücken. Die Sichtbarkeit dünner Wicklungslinien wird stattdessen über COIL_LINE_MIN erzwungen.
    double scale;
    {
        const double sL = COIL_AREA_W / lengthMm;
        const double sD = COIL_AREA_H / diameterMm;
        scale = (sL < sD) ? sL : sD;
        if (scale > COIL_SCALE_MAX) scale = COIL_SCALE_MAX;
    }

    const int lengthPx   = static_cast<int>(std::round(lengthMm * scale));
    const int diameterPx = static_cast<int>(std::round(diameterMm * scale));
    // drawWideLine() erwartet einen Radius, nicht die Strichstärke - daher halbieren
    float wirePx = static_cast<float>(dWireMm * scale * 0.5);
    if (wirePx < COIL_LINE_MIN) wirePx = COIL_LINE_MIN;

    // Coil zentriert in der Coil-Zone
    const int coilLeft  = COIL_X_OFFSET + (COIL_AREA_W - lengthPx) / 2;
    const int coilRight = coilLeft + lengthPx;
    const int coilTop   = COIL_AREA_Y + (COIL_AREA_H - diameterPx) / 2;
    const int coilBot   = coilTop + diameterPx;

    int nInt = static_cast<int>(std::round(m_N));
    if (nInt < 1)          nInt = 1;
    if (nInt > COIL_N_MAX) nInt = COIL_N_MAX;

    // pitchPx so wählen, dass Linie 0 bei coilLeft startet und Linie N-1 bei coilRight endet (Spannweite = lengthPx)
    const float pitchPx = (nInt > 1)
        ? static_cast<float>(lengthPx - COIL_SLANT) / (nInt - 1)
        : 0.0f;

    const uint16_t wireColor = m_geomOverlap ? C_OVERLAP_RED : C_ACCENT_L;

    for (int i = 0; i < nInt; ++i)
    {
        const int xt = coilLeft + static_cast<int>(std::round(i * pitchPx));
        const int xb = xt + COIL_SLANT;
        spr->drawWideLine(ox + xt, oy + coilTop,
                          ox + xb, oy + coilBot,
                          wirePx, wireColor);
    }

    spr->loadFont(ComfortaaBold14);

    // D-Pfeil (vertikal, links) und ℓ-Pfeil (horizontal, unten) - beide beziehen sich auf die tatsächlich gezeichnete Coil-Geometrie
    char dBuf[16];
    stringutils::safeFormat(dBuf, sizeof(dBuf), "%.3g mm", diameterMm);
    dotToComma(dBuf);
    drawDimArrowV(spr, ox + DIM_X_LEFT, oy + coilTop, oy + coilBot,
                  dBuf, C_DIM, C_BG);

    char lBuf[16];
    stringutils::safeFormat(lBuf, sizeof(lBuf), "%.3g mm", lengthMm);
    dotToComma(lBuf);
    drawDimArrowH(spr, ox + coilLeft, ox + coilRight, oy + LARROW_Y,
                  lBuf, C_DIM, C_BG);

    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_INFO_TXT, C_BG);

    // N: Zahl separat formatieren und konvertieren, damit der Punkt in "Wdg." beim Komma-Tausch nicht ebenfalls ersetzt wird
    char numBuf[16];
    char buf[40];
    stringutils::safeFormat(numBuf, sizeof(numBuf), "%.1f", m_N);
    dotToComma(numBuf);
    I18n::format(buf, sizeof(buf), WorkshopKey::WORKSHOP_UKW_RESONATOR_FMT_TURNS, numBuf);
    spr->drawString(buf, ox + SCHEM_W / 2, oy + COIL_TEXT_Y);

    const double pitchMm = lengthMm / m_N;
    stringutils::safeFormat(buf, sizeof(buf), "Pitch = %.2f mm", pitchMm);
    dotToComma(buf);
    spr->drawString(buf, ox + SCHEM_W / 2, oy + COIL_TEXT_Y + COIL_TEXT_DY);

    spr->unloadFont();
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle vier Eingabefelder.
 */
void WorkshopTool_UkwResonator::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);
    for (int i = 0; i < NUM_QTY; ++i)
    {
        // Bauteil-Tap im Schaltplan färbt das zugehörige Feld als gedrückt: m_pressedComp 0 = L (Index 1), 1 = C (Index 2)
        const bool pressed = (m_pressedField == i) ||
                             (m_pressedComp == 0 && i == 1) ||
                             (m_pressedComp == 1 && i == 2);
        drawField(i, pressed, cursorOn);
    }
    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Größen-Zeile.
 */
void WorkshopTool_UkwResonator::drawField(int qty, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const Rect &fr = m_fieldRects[qty];
    const int   ax = m_originX + fr.x;
    const int   ay = m_originY + fr.y;

    const bool input   = isInput(qty);
    const bool focused = (static_cast<int>(m_state.focusedQty) == qty);

    uint16_t bgColor;
    if      (pressed) bgColor = C_FIELD_PRESS;
    else if (focused) bgColor = C_FIELD_FOCUS;
    else              bgColor = C_FIELD_INPUT;

    spr->fillRect(ax, ay, fr.w, fr.h, bgColor);
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, accentColor(qty));

    uint16_t lblClr, valClr, unitClr;
    if      (focused) { lblClr = C_LBL_FOCUS; valClr = C_VAL_FOCUS; unitClr = C_UNIT_FOCUS; }
    else if (input)   { lblClr = C_LBL_INPUT; valClr = C_VAL_INPUT; unitClr = C_UNIT_INPUT; }
    else              { lblClr = C_LBL_COMP;  valClr = C_VAL_COMP;  unitClr = C_UNIT_COMP;  }

    const int cy = ay + fr.h / 2;

    // Symbol (f / L / C / D)
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblClr, bgColor);
    spr->drawString(qtySymbol(qty), ax + ACCENT_W + 6, cy);

    // Skalierte Einheit (MHz / nH / pF / mm)
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(unitClr, bgColor);
    spr->drawString(scaledUnit(qty), ax + fr.w - UNIT_W / 2, cy);

    // Wert
    char displayBuf[32];
    const int valRightX = ax + fr.w - UNIT_W - 6;
    spr->setTextDatum(MR_DATUM);

    if (focused)
    {
        // Anzeige in deutscher Konvention: Punkt -> Komma. Der interne Puffer bleibt unverändert mit '.' damit atof() weiterhin korrekt parst.
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), m_state.bufs[qty]);
        dotToComma(displayBuf);
        stringutils::safeAppend(displayBuf, sizeof(displayBuf), "_");
        spr->setTextColor(valClr, bgColor);
        spr->drawString(displayBuf, valRightX, cy);

        if (!cursorOn)
        {
            const int cursorW = spr->textWidth("_");
            spr->fillRect(valRightX - cursorW, ay + 1, cursorW, fr.h - 2, bgColor);
        }
    }
    else if (input)
    {
        formatMantissa(m_state.values[qty], scaleForQty(qty),
                       displayBuf, sizeof(displayBuf));
        dotToComma(displayBuf);
        spr->setTextColor(valClr, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
    else
    {
        if (m_computeOk)
        {
            formatMantissa(m_state.values[qty], scaleForQty(qty),
                           displayBuf, sizeof(displayBuf));
            dotToComma(displayBuf);
        }
        else
        {
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");
        }
        spr->setTextColor(valClr, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}

// ===================================================================
// Zeichnen - Info-Zeile (Z₀, Q, Bereichswarnung)
// ===================================================================

/**
 * @brief Zeichnet den Info-Bereich mit Z₀, Q-Faktor und ggf. Bereichswarnung.
 */
void WorkshopTool_UkwResonator::drawInfoRow()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_infoY;
    const int rw = m_rightW;
    const int cy = oy + INFO_H / 2;

    spr->fillRect(ox, oy, rw, INFO_H, C_BG);
    spr->loadFont(ComfortaaBold14);

    // Z₀ links
    char z0Buf[INFO_STR_CAP];
    if (m_z0Ok)
    {
        if (m_z0 >= 1000.0)
            stringutils::safeFormat(z0Buf, sizeof(z0Buf),
                                     "Z0 = %.3g kOhm", m_z0 / 1000.0);
        else
            stringutils::safeFormat(z0Buf, sizeof(z0Buf),
                                     "Z0 = %.0f Ohm", m_z0);
        dotToComma(z0Buf);
    }
    else
    {
        stringutils::safeCopy(z0Buf, sizeof(z0Buf), "Z0 = ---");
    }
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_INFO_TXT, C_BG);
    spr->drawString(z0Buf, ox + 4, cy);

    // Q-Faktor mittig
    char qBuf[INFO_STR_CAP];
    if (m_qOk)
    {
        const int qInt = static_cast<int>(std::round(m_Q));
        I18n::format(qBuf, sizeof(qBuf), WorkshopKey::WORKSHOP_UKW_RESONATOR_FMT_Q_APPROX, qInt);
    }
    else
    {
        stringutils::safeCopy(qBuf, sizeof(qBuf), "Q = ---");
    }
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_INFO_TXT, C_BG);
    spr->drawString(qBuf, ox + rw / 2, cy);

    // Warnung rechts: Überlapp hat Vorrang vor Frequenz-Bereich
    if (m_geomOk && m_geomOverlap)
    {
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_OVERLAP_RED, C_BG);
        spr->drawString(I18n::str(WorkshopKey::WORKSHOP_UKW_RESONATOR_WARN_OVERLAP),
                        ox + rw - 4, cy);
    }
    else if (m_computeOk && !isFrequencyInRange())
    {
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_WARN_TXT, C_BG);
        spr->drawString(I18n::str(WorkshopKey::WORKSHOP_UKW_RESONATOR_WARN_OUT_OF_RANGE),
                        ox + rw - 4, cy);
    }

    spr->unloadFont();
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf Eingabefelder.
 */
int WorkshopTool_UkwResonator::hitTestField(int cx, int cy) const
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
 *        Rückgabe: 0 = L (rechte Vertikale), 1 = C (linke Vertikale), -1 = kein Treffer.
 */
int WorkshopTool_UkwResonator::hitTestComponent(int cx, int cy) const
{
    for (int i = 0; i < 2; ++i)
    {
        const Rect &r = m_compHits[i];
        if (r.w <= 0) continue;
        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h)
            return i;
    }
    return -1;
}

// ===================================================================
// Tastatureingabe
// ===================================================================

/**
 * @brief Führt eine Tastenaktion auf dem aktiven Eingabepuffer aus.
 */
void WorkshopTool_UkwResonator::executeKey(char action)
{
    char * const buf = m_state.bufs[m_state.focusedQty];
    const size_t len = strlen(buf);

    switch (action)
    {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            if (len + 1 >= FIELD_CAP) break;
            buf[len] = action; buf[len + 1] = '\0';
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
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch mit FIFO-Logik für Felder, Bauteile und Numpad.
 *
 * Spezialfall D (Index 3): tap rotiert NIE die FIFO, sondern verschiebt nur den Eingabefokus auf D. D ist immer Eingabe und niemals berechnet.
 */
bool WorkshopTool_UkwResonator::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        // Linke Spalte: Schaltplan-Bauteile
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
            // Rechte Spalte: Eingabefelder oder Numpad
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
            // Bauteil- oder Feld-Tap (gemeinsame FIFO-Logik)
            // Schaltplan-Bauteile: 0=L (Index 1), 1=C (Index 2)
            int tapped = -1;
            if (m_pressedComp == 0)
                tapped = 1; // L
            else if (m_pressedComp == 1)
                tapped = 2; // C
            else if (m_pressedField >= 0 && m_pressedField < NUM_QTY)
                tapped = m_pressedField;

            if (tapped >= 0)
            {
                if (tapped == 3 || tapped == 4)
                {
                    // D und ℓ sind reine Parameterfelder - nur Fokus verschieben, niemals an der FIFO drehen
                    m_state.focusedQty = static_cast<uint8_t>(tapped);
                }
                else if (isInput(tapped))
                {
                    // Bereits FIFO-Eingabe -> nur Fokus verschieben
                    m_state.focusedQty = static_cast<uint8_t>(tapped);
                }
                else
                {
                    // FIFO: ältester Slot wird berechnet, tapped wird Eingabe
                    m_state.computedQty   = m_state.inputSlots[0];
                    m_state.inputSlots[0] = m_state.inputSlots[1];
                    m_state.inputSlots[1] = static_cast<uint8_t>(tapped);
                    m_state.focusedQty    = static_cast<uint8_t>(tapped);

                    if (m_computeOk)
                        formatMantissa(m_state.values[tapped], scaleForQty(tapped),
                                       m_state.bufs[tapped], FIELD_CAP);
                    else
                        m_state.bufs[tapped][0] = '\0';

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
        m_pressedComp  = -1;
        m_pressedKey   = -1;
        m_touchWas     = false;
    }

    return needsRedraw;
}
