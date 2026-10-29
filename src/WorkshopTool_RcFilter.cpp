#include "WorkshopTool_RcFilter.h"

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

constexpr int SCHEM_W = 250;  ///< Breite der linken Spalte (Schaltplan + Chips)
constexpr int COL_GAP = 12;   ///< Abstand zwischen Spalten

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

    // Der Schaltplan-Bereich beginnt bei (0, SEL_ROW_H) im Content-Sprite, also direkt unter dem Chip-Selektor. Alle Konstanten hier sind relativ zum Schaltplan-Origin (0, 0) und werden zur Laufzeit auf m_originX + m_originY + SEL_ROW_H aufaddiert.

// Signalpfad (horizontal)
constexpr int SIG_Y       = 90;   ///< Y-Position des Signalpfads im Schaltplan
constexpr int VIN_X       = 20;   ///< Linkes Ende der Vin-Zuleitung
constexpr int LONG_LX     = 47;   ///< Längsbauteil linke Kante
constexpr int LONG_W      = 81;   ///< Längsbauteil Breite (horizontal)
constexpr int LONG_H      = 26;   ///< Längsbauteil Höhe (horizontal)
constexpr int LONG_RX     = LONG_LX + LONG_W; ///< 128
constexpr int JUNCT_X     = 158;  ///< Knotenpunkt X (Längs/Quer-Verzweigung)
constexpr int VOUT_X      = 215;  ///< Rechtes Ende der Vout-Leitung

// Querbauteil (vertikal, gegen GND)
constexpr int TRANS_TOP_Y = 115;  ///< Querbauteil Oberkante
constexpr int TRANS_W     = 26;   ///< Querbauteil Breite (vertikal)
constexpr int TRANS_H     = 81;   ///< Querbauteil Höhe (vertikal)
constexpr int TRANS_BOT_Y = TRANS_TOP_Y + TRANS_H; ///< 196
constexpr int GND_Y       = 215;  ///< GND-Schiene Y

// Touch-Hit-Padding
constexpr int HIT_PAD = 12;

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

constexpr uint16_t C_INFO_TXT   = 0xDEDB;

// Akzentfarben je Größe
constexpr uint16_t C_ACCENT_R  = 0x07E0; ///< R  - Grün
constexpr uint16_t C_ACCENT_C  = 0x04FF; ///< C  - Cyan
constexpr uint16_t C_ACCENT_L  = 0xFFE0; ///< L  - Gelb
constexpr uint16_t C_ACCENT_FC = 0xFD20; ///< fc - Orange

// ===================================================================
// Chip-Selektor (RC / RL) - Farben
// ===================================================================

constexpr uint16_t C_CHIP_UNSEL    = 0x2104; ///< Inaktiver Chip-Hintergrund
constexpr uint16_t C_CHIP_PRESS    = 0x4B4D; ///< Gedrückter Chip
constexpr uint16_t C_CHIP_TXT_SEL  = 0xFFFF; ///< Aktiver Chip-Text
constexpr uint16_t C_CHIP_TXT_UN   = 0x8410; ///< Inaktiver Chip-Text

// ===================================================================
// Schaltplan-Farben
// ===================================================================

constexpr uint16_t C_SCHEM_BG     = 0x1082;
constexpr uint16_t C_WIRE         = 0x8410;
constexpr uint16_t C_COMP_BODY    = 0x2945;
constexpr uint16_t C_COMP_OUTLINE = 0x6B6D;
constexpr uint16_t C_VAL_DIMMED   = 0x7D7A;
constexpr uint16_t C_RAIL         = 0xAD75;
constexpr uint16_t C_JUNCTION     = 0xAD75;
constexpr uint16_t C_COMP_HILITE  = 0x1A45; ///< Hervorgehobene Bauteil-Füllung

// ===================================================================
// Farben der Footer-Umschalter
// ===================================================================

constexpr uint16_t TOGGLE_ACTIVE_FILL    = 0x328A;
constexpr uint16_t TOGGLE_ACTIVE_TEXT    = 0xFFFF;
constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;
constexpr uint16_t TOGGLE_INACTIVE_FILL    = 0x0000;
constexpr uint16_t TOGGLE_INACTIVE_TEXT    = 0x7BEF;
constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8;

constexpr int FOOTER_BTN_W   = 185;
constexpr int FOOTER_BTN_H   = 44;
constexpr int FOOTER_BTN_R   = 6;
constexpr int FOOTER_BTN_GAP = 10;

} // namespace

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_RcFilter::WorkshopTool_RcFilter()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_RcFilter::onEnter(LGFX *tft)
{
    clearAll();
    initFooterButtons(tft);
}

/**
 * @brief Aufräumen beim Verlassen.
 */
void WorkshopTool_RcFilter::onExit()
{
    m_buttonsInitialized = false;
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_RcFilter::clearAll()
{
    memset(m_states,     0, sizeof(m_states));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));
    memset(m_compHits,   0, sizeof(m_compHits));
    memset(m_chipRects,  0, sizeof(m_chipRects));

    m_sprite      = nullptr;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;
    m_layoutValid = false;
    m_computeOk   = false;
    m_numpad.clearLayout();

    m_kind     = FilterKind::LOW_PASS;
    m_topology = FilterTopology::RC;

    m_pressedField = -1;
    m_pressedComp  = -1;
    m_pressedChip  = -1;
    m_pressedKey   = -1;
    m_touchWas     = false;
    m_touchReady   = false;

    // Alle vier Kombinationen mit Standardwerten initialisieren
    initFilterState(m_states[0][0], FilterTopology::RC); // TP RC
    initFilterState(m_states[0][1], FilterTopology::RL); // TP RL
    initFilterState(m_states[1][0], FilterTopology::RC); // HP RC
    initFilterState(m_states[1][1], FilterTopology::RL); // HP RL

    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));
    refreshSubtitle();

    compute();
}

/**
 * @brief Setzt einen FilterState auf Standardwerte (R=1k, C=100n / L=100mH).
 */
void WorkshopTool_RcFilter::initFilterState(FilterState &fs, FilterTopology topology) const
{
    memset(&fs, 0, sizeof(fs));

    // FIFO: R(0) als ältester, C/L(1) als neuester -> fc(2) wird berechnet
    fs.inputSlots[0] = 0; // R (ältester)
    fs.inputSlots[1] = 1; // C oder L (neuester, fokussiert)
    fs.computedQty   = 2; // fc berechnet
    fs.focusedQty    = 1; // C/L fokussiert

    fs.values[0] = 1000.0;  // R = 1 kOhm

    if (topology == FilterTopology::RC)
        fs.values[1] = 100.0e-9;  // C = 100 nF
    else
        fs.values[1] = 100.0e-3;  // L = 100 mH

    // Eingabepuffer als Mantissen in der jeweiligen Anzeige-Skala initialisieren. R steht direkt in Ohm (Skala 1.0), C in nF (1e-9), L in mH (1e-3).
    const double scale1 = (topology == FilterTopology::RC) ? 1.0e-9 : 1.0e-3;
    formatMantissa(fs.values[0], 1.0,    fs.bufs[0], FIELD_CAP);
    formatMantissa(fs.values[1], scale1, fs.bufs[1], FIELD_CAP);
}

// ===================================================================
// Zugriff auf modusspezifische Daten
// ===================================================================

/**
 * @brief Gibt den aktiven FilterState zurück.
 */
WorkshopTool_RcFilter::FilterState &WorkshopTool_RcFilter::activeState()
{
    return m_states[static_cast<int>(m_kind)][static_cast<int>(m_topology)];
}

/**
 * @brief Gibt den aktiven FilterState zurück (const).
 */
const WorkshopTool_RcFilter::FilterState &WorkshopTool_RcFilter::activeState() const
{
    return m_states[static_cast<int>(m_kind)][static_cast<int>(m_topology)];
}

// ===================================================================
// Topologie-Helfer (Längs/Quer-Mapping)
// ===================================================================

/**
 * @brief R ist Längs-Bauteil bei (TP+RC) und (HP+RL).
 *
 * Mapping-Tabelle:
 *   TP RC: R längs, C quer
 *   TP RL: L längs, R quer
 *   HP RC: C längs, R quer
 *   HP RL: R längs, L quer
 */
bool WorkshopTool_RcFilter::rIsLongitudinal() const
{
    return (m_kind == FilterKind::LOW_PASS  && m_topology == FilterTopology::RC) ||
           (m_kind == FilterKind::HIGH_PASS && m_topology == FilterTopology::RL);
}

/**
 * @brief Größenindex des Längs-Bauteils.
 */
int WorkshopTool_RcFilter::longitudinalQty() const
{
    return rIsLongitudinal() ? 0 : 1;
}

/**
 * @brief Größenindex des Quer-Bauteils.
 */
int WorkshopTool_RcFilter::transverseQty() const
{
    return rIsLongitudinal() ? 1 : 0;
}

/**
 * @brief Bauteilart für eine Größe (R immer Widerstand, Index 1 topologieabhängig).
 */
WorkshopTool_RcFilter::CompKind WorkshopTool_RcFilter::kindOfQty(int qty) const
{
    if (qty == 0) return CompKind::RES;
    return (m_topology == FilterTopology::RC) ? CompKind::CAP : CompKind::IND;
}

// ===================================================================
// Symbol- und Einheiten-Helfer (topologieabhängig)
// ===================================================================

/**
 * @brief Symboltext für Eingabefeld und Schaltplan-Label.
 */
const char *WorkshopTool_RcFilter::qtySymbol(int qty) const
{
    if (qty == 0) return "R";
    if (qty == 2) return "fc";
    return (m_topology == FilterTopology::RC) ? "C" : "L";
}

/**
 * @brief Skalierte Anzeige-Einheit je Größe.
 */
const char *WorkshopTool_RcFilter::scaledUnit(int qty) const
{
    if (qty == 0) return "Ohm";
    if (qty == 2) return "Hz";
    return (m_topology == FilterTopology::RC) ? "nF" : "mH";
}

/**
 * @brief Skalierungsfaktor je Größe (Mantisse * scale = SI-Wert).
 */
double WorkshopTool_RcFilter::scaleForQty(int qty) const
{
    if (qty == 0 || qty == 2) return 1.0;
    return (m_topology == FilterTopology::RC) ? 1.0e-9 : 1.0e-3;
}

/**
 * @brief Akzentfarbe je Größe.
 */
uint16_t WorkshopTool_RcFilter::accentColor(int qty) const
{
    if (qty == 0) return C_ACCENT_R;
    if (qty == 2) return C_ACCENT_FC;
    return (m_topology == FilterTopology::RC) ? C_ACCENT_C : C_ACCENT_L;
}

/**
 * @brief Prüft ob eine Größe als Eingabe aktiv ist.
 */
bool WorkshopTool_RcFilter::isInput(int qty) const
{
    const FilterState &fs = activeState();
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (static_cast<int>(fs.inputSlots[s]) == qty)
            return true;
    return false;
}

// ===================================================================
// Footer-Buttons (Tiefpass / Hochpass-Toggle)
// ===================================================================

/**
 * @brief Erstellt die zwei Footer-Toggle-Buttons.
 */
void WorkshopTool_RcFilter::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int totalW = 2 * FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int startX = WorkshopCfg::TOOL_FOOTER_X +
                           (WorkshopCfg::TOOL_FOOTER_W - totalW) / 2;

    const int cxLow  = startX + FOOTER_BTN_W / 2;
    const int cxHigh = cxLow + FOOTER_BTN_W + FOOTER_BTN_GAP;

    I18n::format(m_btnLowPassLabel,  sizeof(m_btnLowPassLabel),
                 WorkshopKey::WORKSHOP_RC_FILTER_BTN_LOW_PASS);
    I18n::format(m_btnHighPassLabel, sizeof(m_btnHighPassLabel),
                 WorkshopKey::WORKSHOP_RC_FILTER_BTN_HIGH_PASS);

    m_btnLowPass.initButton(tft, cxLow, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
                            TOGGLE_ACTIVE_OUTLINE, TOGGLE_ACTIVE_FILL, TOGGLE_ACTIVE_TEXT,
                            m_btnLowPassLabel, nullptr);
    m_btnLowPass.setFont(LeagueGothicCond34);

    m_btnHighPass.initButton(tft, cxHigh, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
                             TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
                             m_btnHighPassLabel, nullptr);
    m_btnHighPass.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
}

/**
 * @brief Aktualisiert Toggle-Farben je nach aktivem Filtertyp.
 */
void WorkshopTool_RcFilter::updateButtonColors()
{
    const bool isLow = (m_kind == FilterKind::LOW_PASS);

    m_btnLowPass.setFillColor(isLow ? TOGGLE_ACTIVE_FILL : TOGGLE_INACTIVE_FILL);
    m_btnLowPass.setTextColor(isLow ? TOGGLE_ACTIVE_TEXT : TOGGLE_INACTIVE_TEXT);
    m_btnLowPass.setOutlineColor(isLow ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);

    m_btnHighPass.setFillColor(isLow ? TOGGLE_INACTIVE_FILL : TOGGLE_ACTIVE_FILL);
    m_btnHighPass.setTextColor(isLow ? TOGGLE_INACTIVE_TEXT : TOGGLE_ACTIVE_TEXT);
    m_btnHighPass.setOutlineColor(isLow ? TOGGLE_INACTIVE_OUTLINE : TOGGLE_ACTIVE_OUTLINE);
}

/**
 * @brief Zeichnet die Footer-Toggle-Buttons.
 */
void WorkshopTool_RcFilter::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft; (void)x; (void)y; (void)w; (void)h;
    if (!m_buttonsInitialized)
        return;

    updateButtonColors();
    m_btnLowPass.drawButton();
    m_btnHighPass.drawButton();
}

/**
 * @brief Verarbeitet Touch auf den Footer-Buttons.
 */
bool WorkshopTool_RcFilter::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    const bool onLow  = touched && m_btnLowPass.contains(tx, ty);
    const bool onHigh = touched && m_btnHighPass.contains(tx, ty);

    const bool wasLow  = m_btnLowPass.isPressed();
    const bool wasHigh = m_btnHighPass.isPressed();

    m_btnLowPass.press(onLow);
    m_btnHighPass.press(onHigh);

    // Filtertyp-Wechsel bei Loslassen
    if (m_btnLowPass.justReleased() && m_kind != FilterKind::LOW_PASS)
    {
        m_kind = FilterKind::LOW_PASS;
        refreshSubtitle();
        m_layoutValid = false;
        compute();
        return true;
    }
    if (m_btnHighPass.justReleased() && m_kind != FilterKind::HIGH_PASS)
    {
        m_kind = FilterKind::HIGH_PASS;
        refreshSubtitle();
        m_layoutValid = false;
        compute();
        return true;
    }

    // Visuelle Rückmeldung
    if (onLow != wasLow)
        m_btnLowPass.drawButton(onLow);
    if (onHigh != wasHigh)
        m_btnHighPass.drawButton(onHigh);

    return false;
}

/**
 * @brief Dynamischer Untertitel je nach Filterart und Topologie.
 */
const char *WorkshopTool_RcFilter::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Befüllt m_subtitleBuf passend zu m_kind und m_topology.
 */
void WorkshopTool_RcFilter::refreshSubtitle()
{
    static const WorkshopKey SUBTITLE_KEYS[2][2] = {
        // LOW_PASS
        { WorkshopKey::WORKSHOP_RC_FILTER_SUB_LP_RC,
          WorkshopKey::WORKSHOP_RC_FILTER_SUB_LP_RL },
        // HIGH_PASS
        { WorkshopKey::WORKSHOP_RC_FILTER_SUB_HP_RC,
          WorkshopKey::WORKSHOP_RC_FILTER_SUB_HP_RL }
    };
    const int ki = static_cast<int>(m_kind);
    const int ti = static_cast<int>(m_topology);
    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), SUBTITLE_KEYS[ki][ti]);
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Chip-Selektor, Schaltplan, Felder, Info und Numpad.
 */
void WorkshopTool_RcFilter::draw(Gfx_Sprite *sprite,
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

    drawChipSelector();
    drawSchematic();
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
void WorkshopTool_RcFilter::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;
    if (m_rightW < 100) m_rightW = 100;

    // Chip-Selektor (über dem Schaltplan, nur über die linke Spalte)
    const int chipW = (SCHEM_W - CHIP_GAP) / 2;
    m_chipRects[0] = {0,                       CHIP_TOP, chipW, CHIP_H};
    m_chipRects[1] = {chipW + CHIP_GAP,        CHIP_TOP, chipW, CHIP_H};

    // Eingabefelder (rechte Spalte, unter Headerbereich mit TOP_GAP)
    for (int i = 0; i < NUM_QTY; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * ROW_H, m_rightW, ROW_H};

    // Info-Zeile direkt nach den Feldern
    m_infoY = TOP_GAP + NUM_QTY * ROW_H + INFO_GAP_V;

    // Numpad am unteren Rand der rechten Spalte
    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);

    // Schaltplan-Touch-Areas (content-relativ)
    // Längs-Bauteil (horizontal auf SIG_Y)
    m_compHits[0] = {LONG_LX - HIT_PAD,
                     SEL_ROW_H + SIG_Y - LONG_H / 2 - HIT_PAD,
                     LONG_W + 2 * HIT_PAD,
                     LONG_H + 2 * HIT_PAD};

    // Quer-Bauteil (vertikal, zentriert auf JUNCT_X)
    m_compHits[1] = {JUNCT_X - TRANS_W / 2 - HIT_PAD,
                     SEL_ROW_H + TRANS_TOP_Y - HIT_PAD,
                     TRANS_W + 2 * HIT_PAD,
                     TRANS_H + 2 * HIT_PAD};
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Parst Eingabepuffer und berechnet die Ausgabegröße.
 */
void WorkshopTool_RcFilter::compute()
{
    FilterState &fs = activeState();

    // Eingabewerte aus Puffern parsen - Mantisse × Skala ergibt SI-Wert
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
    {
        const int q = static_cast<int>(fs.inputSlots[s]);
        const double scale = scaleForQty(q);
        fs.values[q] = (fs.bufs[q][0] != '\0') ? atof(fs.bufs[q]) * scale : 0.0;
    }

    m_computeOk = false;

    // Finite-Check
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (!std::isfinite(fs.values[static_cast<int>(fs.inputSlots[s])]))
            return;

    const double r  = fs.values[0]; // Widerstand (Ohm)
    const double cl = fs.values[1]; // Kondensator (F) oder Spule (H)
    const double fc = fs.values[2]; // Eckfrequenz (Hz)

    if (m_topology == FilterTopology::RC)
    {
        // RC-Filter: fc = 1 / (2π · R · C)
        switch (fs.computedQty)
        {
            case 2: // fc berechnen
            {
                if (r <= 0.0 || cl <= 0.0) return;
                const double result = 1.0 / (RC_TWO_PI * r * cl);
                if (!std::isfinite(result)) return;
                fs.values[2] = result;
                m_computeOk = true;
                break;
            }
            case 0: // R berechnen
            {
                if (cl <= 0.0 || fc <= 0.0) return;
                const double result = 1.0 / (RC_TWO_PI * cl * fc);
                if (!std::isfinite(result) || result <= 0.0) return;
                fs.values[0] = result;
                m_computeOk = true;
                break;
            }
            case 1: // C berechnen
            {
                if (r <= 0.0 || fc <= 0.0) return;
                const double result = 1.0 / (RC_TWO_PI * r * fc);
                if (!std::isfinite(result) || result <= 0.0) return;
                fs.values[1] = result;
                m_computeOk = true;
                break;
            }
            default: break;
        }
    }
    else // RL-Filter
    {
        // RL-Filter: fc = R / (2π · L)
        switch (fs.computedQty)
        {
            case 2: // fc berechnen
            {
                if (r <= 0.0 || cl <= 0.0) return;
                const double result = r / (RC_TWO_PI * cl);
                if (!std::isfinite(result)) return;
                fs.values[2] = result;
                m_computeOk = true;
                break;
            }
            case 0: // R berechnen: R = 2π · L · fc
            {
                if (cl <= 0.0 || fc <= 0.0) return;
                const double result = RC_TWO_PI * cl * fc;
                if (!std::isfinite(result) || result <= 0.0) return;
                fs.values[0] = result;
                m_computeOk = true;
                break;
            }
            case 1: // L berechnen: L = R / (2π · fc)
            {
                if (r <= 0.0 || fc <= 0.0) return;
                const double result = r / (RC_TWO_PI * fc);
                if (!std::isfinite(result) || result <= 0.0) return;
                fs.values[1] = result;
                m_computeOk = true;
                break;
            }
            default: break;
        }
    }
}

// ===================================================================
// Formatierung
// ===================================================================

/**
 * @brief Formatiert einen SI-Wert als Mantisse für eine gegebene Skala.
 */
void WorkshopTool_RcFilter::formatMantissa(double siValue, double scale,
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
void WorkshopTool_RcFilter::formatSchematicValue(int qty, char *buf, size_t sz) const
{
    if (!buf || sz == 0) return;

    const FilterState &fs = activeState();
    const double val = fs.values[qty];

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "?");
        return;
    }

    const double absVal = std::abs(val);

    if (qty == 0) // Widerstand
    {
        if (absVal == 0.0)
            stringutils::safeCopy(buf, sz, "0");
        else if (absVal >= 1.0e6)
            stringutils::safeFormat(buf, sz, "%.3gM", val / 1.0e6);
        else if (absVal >= 1.0e3)
            stringutils::safeFormat(buf, sz, "%.3gk", val / 1.0e3);
        else
            stringutils::safeFormat(buf, sz, "%.3g", val);
    }
    else if (qty == 2) // Frequenz
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
    else if (m_topology == FilterTopology::RC) // Kapazität
    {
        if (absVal >= 1.0e-6)
            stringutils::safeFormat(buf, sz, "%.3gu", val * 1.0e6);
        else if (absVal >= 1.0e-9)
            stringutils::safeFormat(buf, sz, "%.3gn", val * 1.0e9);
        else
            stringutils::safeFormat(buf, sz, "%.3gp", val * 1.0e12);
    }
    else // Induktivität
    {
        if (absVal >= 1.0)
            stringutils::safeFormat(buf, sz, "%.3gH", val);
        else if (absVal >= 1.0e-3)
            stringutils::safeFormat(buf, sz, "%.3gm", val * 1.0e3);
        else
            stringutils::safeFormat(buf, sz, "%.3gu", val * 1.0e6);
    }

    dotToComma(buf);
}

// ===================================================================
// Zeichnen - Chip-Selektor (RC / RL)
// ===================================================================

/**
 * @brief Zeichnet die zwei Topologie-Auswahl-Chips über dem Schaltplan.
 */
void WorkshopTool_RcFilter::drawChipSelector()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;

    // Hintergrund der Selector-Zone (nur über die Schaltplan-Spalte)
    spr->fillRect(ox, oy, SCHEM_W, SEL_ROW_H, C_BG);

    static const char *const CHIP_LABELS[2] = {"RC", "RL"};
    const int selIdx = static_cast<int>(m_topology);

    spr->loadFont(ComfortaaBold16);

    for (int i = 0; i < 2; ++i)
    {
        const Rect &r = m_chipRects[i];
        const bool sel     = (i == selIdx);
        const bool pressed = (i == m_pressedChip);

        // Hintergrund- und Textfarbe je nach Zustand bestimmen. Aktiver Chip bekommt den Petrol-Fokus-Ton; die Topologie wird zusätzlich über einen schmalen Akzentstreifen (siehe unten) farblich kodiert.
        uint16_t bg, fg;
        if (pressed)
        {
            bg = C_CHIP_PRESS;
            fg = C_CHIP_TXT_SEL;
        }
        else if (sel)
        {
            // Der gewählte Chip erhält die Akzentfarbe der Topologie als Kontur und einen petrolfarbenen Hintergrund.
            bg = C_FIELD_FOCUS;
            fg = C_CHIP_TXT_SEL;
        }
        else
        {
            bg = C_CHIP_UNSEL;
            fg = C_CHIP_TXT_UN;
        }

        spr->fillSmoothRoundRect(ox + r.x, oy + r.y, r.w, r.h, CHIP_R, bg);

        // Bei aktivem Chip einen farbigen Akzentstreifen oben einziehen
        if (sel && !pressed)
        {
            const uint16_t accent = (i == 0) ? C_ACCENT_C : C_ACCENT_L;
            spr->fillRect(ox + r.x + 6, oy + r.y + 2, r.w - 12, 2, accent);
        }

        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(fg, bg);
        spr->drawString(CHIP_LABELS[i], ox + r.x + r.w / 2, oy + r.y + r.h / 2);
    }

    spr->unloadFont();
}

// ===================================================================
// Zeichnen - Schaltplan
// ===================================================================

/**
 * @brief Zeichnet den kompletten Schaltplan-Bereich.
 *
 * Der Schaltplan beginnt unter dem Chip-Selektor bei (ox, oy + SEL_ROW_H). Die Geometrie aus dem anonymen Namespace ist relativ zu dieser Origin.
 *
 * Längs- und Querbauteil werden anhand der aktuellen Filterart und Topologie über kindOfQty() bestimmt - dieselbe Routine zeichnet alle vier Schaltungsvarianten.
 */
void WorkshopTool_RcFilter::drawSchematic()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + SEL_ROW_H;
    const int sh = m_h - SEL_ROW_H;

    // Hintergrund (nur Schaltplan-Spalte unter dem Selektor)
    spr->fillRect(ox, oy, SCHEM_W, sh, C_SCHEM_BG);

    // GND-Schiene
    spr->drawFastHLine(ox + 20, oy + GND_Y, SCHEM_W - 40, C_WIRE);

    // Längs- und Quer-Größenindex bestimmen
    const int longQ  = longitudinalQty();
    const int transQ = transverseQty();

    const FilterState &fs = activeState();
    const bool longActive  = (static_cast<int>(fs.focusedQty) == longQ);
    const bool transActive = (static_cast<int>(fs.focusedQty) == transQ);

    // ===================================================================
    // Verdrahtung
    // ===================================================================

    // Vin -> linke Bauteilkante
    spr->drawFastHLine(ox + VIN_X, oy + SIG_Y, LONG_LX - VIN_X, C_WIRE);

    // Längsbauteil rechte Kante -> Knotenpunkt
    spr->drawFastHLine(ox + LONG_RX, oy + SIG_Y, JUNCT_X - LONG_RX, C_WIRE);

    // Knotenpunkt -> Vout
    spr->drawFastHLine(ox + JUNCT_X, oy + SIG_Y, VOUT_X - JUNCT_X, C_WIRE);

    // Knotenpunkt -> Quer-Bauteil oben
    spr->drawFastVLine(ox + JUNCT_X, oy + SIG_Y, TRANS_TOP_Y - SIG_Y, C_WIRE);

    // Quer-Bauteil unten -> GND-Schiene
    spr->drawFastVLine(ox + JUNCT_X, oy + TRANS_BOT_Y, GND_Y - TRANS_BOT_Y, C_WIRE);

    // Knotenpunkte
    spr->fillCircle(ox + JUNCT_X, oy + SIG_Y,  3, C_JUNCTION);
    spr->fillCircle(ox + JUNCT_X, oy + GND_Y, 3, C_JUNCTION);

    // ===================================================================
    // Bauteile
    // ===================================================================

    drawComponentHoriz(kindOfQty(longQ),  LONG_LX,  SIG_Y,       longActive);
    drawComponentVert (kindOfQty(transQ), JUNCT_X,  TRANS_TOP_Y, transActive);

    // ===================================================================
    // Beschriftungen
    // ===================================================================

    spr->loadFont(ComfortaaBold14);

    // Vin / Vout / GND
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_RAIL, C_SCHEM_BG);
    spr->drawString("Vin",  ox + 4, oy + SIG_Y - 14);
    spr->drawString("Vout", ox + VOUT_X + 4, oy + SIG_Y - 14);
    spr->drawString("GND",  ox + 4, oy + GND_Y - 2);

    // Längsbauteil-Label und Wert (über/unter dem Bauteil)
    {
        const uint16_t clr = longActive ? accentColor(longQ) : C_VAL_DIMMED;
        const int lcx = LONG_LX + LONG_W / 2;

        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(clr, C_SCHEM_BG);
        spr->drawString(qtySymbol(longQ), ox + lcx, oy + SIG_Y - LONG_H / 2 - 12);

        char buf[16];
        formatSchematicValue(longQ, buf, sizeof(buf));
        spr->drawString(buf, ox + lcx, oy + SIG_Y + LONG_H / 2 + 12);
    }

    // Querbauteil-Label und Wert (rechts vom Bauteil)
    {
        const uint16_t clr = transActive ? accentColor(transQ) : C_VAL_DIMMED;
        const int tcy = TRANS_TOP_Y + TRANS_H / 2;

        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(clr, C_SCHEM_BG);
        spr->drawString(qtySymbol(transQ),
                        ox + JUNCT_X + TRANS_W / 2 + 8,
                        oy + tcy - 10);

        char buf[16];
        formatSchematicValue(transQ, buf, sizeof(buf));
        spr->drawString(buf,
                        ox + JUNCT_X + TRANS_W / 2 + 8,
                        oy + tcy + 10);
    }

    spr->unloadFont();
}

// ===================================================================
// Zeichnen - Bauteil-Symbole (horizontal + vertikal)
// ===================================================================

/**
 * @brief Zeichnet ein horizontales Bauteil entlang des Signalpfads.
 */
void WorkshopTool_RcFilter::drawComponentHoriz(CompKind kind, int leftX,
                                                int centerY, bool active)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + SEL_ROW_H;

    const uint16_t fillCol = active ? C_COMP_HILITE : C_COMP_BODY;

    switch (kind)
    {
        case CompKind::RES:
        {
            // Widerstand: Rechteck zentriert auf centerY
            const int boxY = centerY - LONG_H / 2;
            const uint16_t ol = active ? C_ACCENT_R : C_COMP_OUTLINE;
            spr->fillRect(ox + leftX, oy + boxY, LONG_W, LONG_H, fillCol);
            spr->drawRect(ox + leftX, oy + boxY, LONG_W, LONG_H, ol);
            break;
        }
        case CompKind::CAP:
        {
            // Kondensator: zwei vertikale Platten in der Mitte der Bauteil-Box Die Leitung läuft seitlich bis an die Platten heran.
            const int plateGap = 8;
            const int plateH   = 22;
            const int midX     = leftX + LONG_W / 2;
            const int leftPlateX  = midX - plateGap / 2;
            const int rightPlateX = midX + plateGap / 2;
            const uint16_t ol = active ? C_ACCENT_C : C_COMP_OUTLINE;

            // Verbindungsleitungen zur linken/rechten Bauteilkante
            spr->drawFastHLine(ox + leftX, oy + centerY,
                               leftPlateX - leftX, ol);
            spr->drawFastHLine(ox + rightPlateX, oy + centerY,
                               leftX + LONG_W - rightPlateX, ol);

            // Zwei vertikale Platten
            spr->fillRect(ox + leftPlateX - 1, oy + centerY - plateH / 2,
                          3, plateH, ol);
            spr->fillRect(ox + rightPlateX - 1, oy + centerY - plateH / 2,
                          3, plateH, ol);
            break;
        }
        case CompKind::IND:
        {
            // Spule: vier AA-Halbkreise, tangential nebeneinander angeordnet. Die Kreise werden voll gezeichnet, die untere Hälfte wird mit fillRect weggeschnitten - damit entsteht eine durchgehende Wellenkontur. Die Mittellinie selbst wird NICHT gezeichnet, weil die Halbkreise an ihren Tangentenpunkten ineinander übergehen.
            const int   numBumps  = 4;
            const float bumpR     = 10.0f;
            const float thickness = 2.5f;
            const int   bumpD     = static_cast<int>(2.0f * bumpR) - 1;  ///< 1 px Überlappung
            const int   totalW    = (numBumps - 1) * bumpD + static_cast<int>(2.0f * bumpR);
            const int   padX      = (LONG_W - totalW) / 2;
            const int   firstCx   = leftX + padX + static_cast<int>(bumpR);
            const uint16_t ol     = active ? C_ACCENT_L : C_COMP_OUTLINE;

            // Kurze Anschlussleitungen links und rechts (außerhalb der Bumps)
            if (padX > 0)
            {
                spr->drawFastHLine(ox + leftX, oy + centerY, padX, ol);
                spr->drawFastHLine(ox + leftX + LONG_W - padX, oy + centerY,
                                   padX, ol);
            }

            // Vier AA-Kreise nebeneinander (Mittelpunkt exakt auf Mittellinie)
            for (int i = 0; i < numBumps; ++i)
            {
                const int cx = firstCx + i * bumpD;
                spr->drawSmoothCircle(ox + cx, oy + centerY, bumpR,
                                      thickness, ol, C_SCHEM_BG);
            }

            // Untere Hälfte aller Bumps mit BG wegschneiden - alles ab der Pixelreihe direkt unter der Mittellinie. Die obere Hälfte bleibt als saubere Wellenkontur stehen.
            spr->fillRect(ox + leftX + padX, oy + centerY + 1,
                          totalW, static_cast<int>(bumpR) + 2, C_SCHEM_BG);
            break;
        }
    }
}

/**
 * @brief Zeichnet ein vertikales Bauteil zwischen Knotenpunkt und GND.
 */
void WorkshopTool_RcFilter::drawComponentVert(CompKind kind, int centerX,
                                               int topY, bool active)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + SEL_ROW_H;

    switch (kind)
    {
        case CompKind::RES:
        {
            // Widerstand: vertikales Rechteck zentriert auf centerX
            const int boxX = centerX - TRANS_W / 2;
            const uint16_t fillCol = active ? C_COMP_HILITE : C_COMP_BODY;
            const uint16_t ol      = active ? C_ACCENT_R   : C_COMP_OUTLINE;
            spr->fillRect(ox + boxX, oy + topY, TRANS_W, TRANS_H, fillCol);
            spr->drawRect(ox + boxX, oy + topY, TRANS_W, TRANS_H, ol);
            break;
        }
        case CompKind::CAP:
        {
            // Kondensator: zwei horizontale Platten in der Mitte
            const int plateGap = 8;
            const int plateW   = 22;
            const int midY     = topY + TRANS_H / 2;
            const int topPlateY = midY - plateGap / 2;
            const int botPlateY = midY + plateGap / 2;
            const uint16_t ol  = active ? C_ACCENT_C : C_COMP_OUTLINE;

            // Verbindungsleitungen oben/unten
            spr->drawFastVLine(ox + centerX, oy + topY,
                               topPlateY - topY, ol);
            spr->drawFastVLine(ox + centerX, oy + botPlateY,
                               topY + TRANS_H - botPlateY, ol);

            // Zwei horizontale Platten
            spr->fillRect(ox + centerX - plateW / 2, oy + topPlateY - 1,
                          plateW, 3, ol);
            spr->fillRect(ox + centerX - plateW / 2, oy + botPlateY - 1,
                          plateW, 3, ol);
            break;
        }
        case CompKind::IND:
        {
            // Spule vertikal: vier AA-Halbkreise, tangential untereinander. Volle AA-Kreise zeichnen, rechte Hälfte wegschneiden - die verbleibenden linken Halbkreise bilden eine durchgehende Wellenkontur ohne separate Mittellinie.
            const int   numBumps  = 4;
            const float bumpR     = 10.0f;
            const float thickness = 2.5f;
            const int   bumpD     = static_cast<int>(2.0f * bumpR) - 1;  ///< 1 px Überlappung
            const int   totalH    = (numBumps - 1) * bumpD + static_cast<int>(2.0f * bumpR);
            const int   padY      = (TRANS_H - totalH) / 2;
            const int   firstCy   = topY + padY + static_cast<int>(bumpR);
            const uint16_t ol     = active ? C_ACCENT_L : C_COMP_OUTLINE;

            // Kurze Anschlussleitungen oben und unten (außerhalb der Bumps)
            if (padY > 0)
            {
                spr->drawFastVLine(ox + centerX, oy + topY, padY, ol);
                spr->drawFastVLine(ox + centerX, oy + topY + TRANS_H - padY,
                                   padY, ol);
            }

            // Vier AA-Kreise untereinander (Mittelpunkt exakt auf Mittellinie)
            for (int i = 0; i < numBumps; ++i)
            {
                const int cy = firstCy + i * bumpD;
                spr->drawSmoothCircle(ox + centerX, oy + cy, bumpR,
                                      thickness, ol, C_SCHEM_BG);
            }

            // Rechte Hälfte aller Bumps mit BG wegschneiden - alles ab der Pixelspalte direkt rechts der Mittellinie. Die linke Hälfte bleibt als saubere Wellenkontur stehen.
            spr->fillRect(ox + centerX + 1, oy + topY + padY,
                          static_cast<int>(bumpR) + 2, totalH, C_SCHEM_BG);
            break;
        }
    }
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle drei Eingabefelder.
 */
void WorkshopTool_RcFilter::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);
    for (int i = 0; i < NUM_QTY; ++i)
    {
        const bool pressed = (m_pressedField == i) ||
                             (m_pressedComp == 0 && i == longitudinalQty()) ||
                             (m_pressedComp == 1 && i == transverseQty());
        drawField(i, pressed, cursorOn);
    }
    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Größen-Zeile.
 */
void WorkshopTool_RcFilter::drawField(int qty, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const Rect &fr = m_fieldRects[qty];
    const int   ax = m_originX + fr.x;
    const int   ay = m_originY + fr.y;

    const FilterState &fs = activeState();
    const bool input   = isInput(qty);
    const bool focused = (static_cast<int>(fs.focusedQty) == qty);

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

    // Symbol (R / C/L / fc)
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblClr, bgColor);
    spr->drawString(qtySymbol(qty), ax + ACCENT_W + 6, cy);

    // Skalierte Einheit (Ohm / nF / mH / Hz)
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
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), fs.bufs[qty]);
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
        formatMantissa(fs.values[qty], scaleForQty(qty),
                       displayBuf, sizeof(displayBuf));
        dotToComma(displayBuf);
        spr->setTextColor(valClr, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
    else
    {
        if (m_computeOk)
        {
            formatMantissa(fs.values[qty], scaleForQty(qty),
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
// Zeichnen - Info-Zeile (Steigung + Phase bei fc)
// ===================================================================

/**
 * @brief Zeichnet den Info-Bereich mit Steigung und Phase bei fc.
 */
void WorkshopTool_RcFilter::drawInfoRow()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_infoY;
    const int rw = m_rightW;
    const int cy = oy + INFO_H / 2;

    spr->fillRect(ox, oy, rw, INFO_H, C_BG);
    spr->loadFont(ComfortaaBold14);

    // Steigung links, Phase bei fc rechts
    const bool isLow = (m_kind == FilterKind::LOW_PASS);
    const __FlashStringHelper *slopeStr = isLow
        ? I18n::str(WorkshopKey::WORKSHOP_RC_FILTER_SLOPE_LP)
        : I18n::str(WorkshopKey::WORKSHOP_RC_FILTER_SLOPE_HP);
    const char *phaseStr = isLow ? "phi @ fc = -45°" : "phi @ fc = +45°";

    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_INFO_TXT, C_BG);
    spr->drawString(slopeStr, ox + 4, cy);

    spr->setTextDatum(MR_DATUM);
    spr->drawString(phaseStr, ox + rw - 4, cy);

    spr->unloadFont();
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf Eingabefelder.
 */
int WorkshopTool_RcFilter::hitTestField(int cx, int cy) const
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
 *        Rückgabe: 0 = Längs, 1 = Quer, -1 = kein Treffer.
 */
int WorkshopTool_RcFilter::hitTestComponent(int cx, int cy) const
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

/**
 * @brief Treffertest auf Chip-Selektor.
 */
int WorkshopTool_RcFilter::hitTestChip(int cx, int cy) const
{
    for (int i = 0; i < 2; ++i)
    {
        const Rect &r = m_chipRects[i];
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
void WorkshopTool_RcFilter::executeKey(char action)
{
    FilterState &fs = activeState();
    char * const buf = fs.bufs[fs.focusedQty];
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
 * @brief Verarbeitet Touch mit FIFO-2-Logik für Felder, Bauteile und Chips.
 */
bool WorkshopTool_RcFilter::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        // Linke Spalte: Chip-Selektor oder Schaltplan-Bauteile
        if (cx < SCHEM_W)
        {
            // Chip-Selektor (oberhalb SEL_ROW_H)
            if (cy < SEL_ROW_H)
            {
                const int hitChip = hitTestChip(cx, cy);
                if (hitChip >= 0 && hitChip != m_pressedChip)
                {
                    m_pressedChip  = hitChip;
                    m_pressedField = -1;
                    m_pressedComp  = -1;
                    m_pressedKey   = -1;
                    needsRedraw    = true;
                }
            }
            else
            {
                // Schaltplan-Bauteile
                const int hitComp = hitTestComponent(cx, cy);
                if (hitComp >= 0 && hitComp != m_pressedComp)
                {
                    m_pressedComp  = hitComp;
                    m_pressedField = -1;
                    m_pressedChip  = -1;
                    m_pressedKey   = -1;
                    needsRedraw    = true;
                }
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
                    m_pressedChip  = -1;
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
                    m_pressedChip  = -1;
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
            FilterState &fs = activeState();

            // Chip-Tap -> Topologie wechseln
            if (m_pressedChip >= 0)
            {
                const FilterTopology newTop = (m_pressedChip == 0)
                                                  ? FilterTopology::RC
                                                  : FilterTopology::RL;
                if (newTop != m_topology)
                {
                    m_topology = newTop;
                    refreshSubtitle();
                    compute(); // Aktiver State hat sich geändert
                }
                needsRedraw = true;
            }
            // Bauteil- oder Feld-Tap (gemeinsame FIFO-Logik)
            else
            {
                int tapped = -1;
                if (m_pressedComp == 0)
                    tapped = longitudinalQty();
                else if (m_pressedComp == 1)
                    tapped = transverseQty();
                else if (m_pressedField >= 0 && m_pressedField < NUM_QTY)
                    tapped = m_pressedField;

                if (tapped >= 0)
                {
                    if (isInput(tapped))
                    {
                        // Bereits Eingabe -> nur Fokus verschieben
                        fs.focusedQty = static_cast<uint8_t>(tapped);
                    }
                    else
                    {
                        // FIFO: ältester Slot wird berechnet, tapped wird Eingabe
                        fs.computedQty   = fs.inputSlots[0];
                        fs.inputSlots[0] = fs.inputSlots[1];
                        fs.inputSlots[1] = static_cast<uint8_t>(tapped);
                        fs.focusedQty    = static_cast<uint8_t>(tapped);

                        if (m_computeOk)
                            formatMantissa(fs.values[tapped], scaleForQty(tapped),
                                           fs.bufs[tapped], FIELD_CAP);
                        else
                            fs.bufs[tapped][0] = '\0';

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
        }

        m_pressedField = -1;
        m_pressedComp  = -1;
        m_pressedChip  = -1;
        m_pressedKey   = -1;
        m_touchWas     = false;
    }

    return needsRedraw;
}
