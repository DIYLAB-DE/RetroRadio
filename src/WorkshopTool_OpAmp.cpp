#include "WorkshopTool_OpAmp.h"

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

constexpr int SCHEM_W = 250;  ///< Breite der linken Spalte (Schaltplan)
constexpr int COL_GAP = 12;   ///< Abstand zwischen Spalten

// ===================================================================
// Hilfsfunktion - Dezimalkomma (deutsches Format)
// ===================================================================

/**
 * @brief Ersetzt alle Dezimalpunkte durch Kommas im übergebenen Puffer.
 * @param buf Puffer mit formatiertem Zahlenwert
 */
void dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.') *p = ',';
}

// ===================================================================
// Schaltplan - Op-Amp-Dreieck
// ===================================================================

constexpr int TRI_LX  = 112;  ///< Linke Kante des Dreiecks
constexpr int TRI_RX  = 198;  ///< Rechte Spitze des Dreiecks
constexpr int TRI_TY  = 95;   ///< Oberkante des Dreiecks
constexpr int TRI_BY  = 205;  ///< Unterkante des Dreiecks
constexpr int TRI_CY  = (TRI_TY + TRI_BY) / 2; ///< Vertikale Mitte (150)

constexpr int PIN_MINUS_Y = TRI_TY + (TRI_BY - TRI_TY) / 4;     ///< - Eingang (122)
constexpr int PIN_PLUS_Y  = TRI_BY - (TRI_BY - TRI_TY) / 4;     ///< + Eingang (177)
constexpr int PIN_OUT_Y   = TRI_CY;                                ///< Ausgang (150)

// ===================================================================
// Schaltplan - Bauteile und Verdrahtung
// ===================================================================

constexpr int COMP_W   = 24;  ///< Breite Widerstands-Symbol (vertikal)
constexpr int COMP_H_V = 36;  ///< Höhe Widerstands-Symbol (vertikal)
constexpr int COMP_H_H = 24;  ///< Höhe Widerstands-Symbol (horizontal)
constexpr int COMP_W_H = 42;  ///< Breite Widerstands-Symbol (horizontal)

// Invertierend: R1 horizontal, Rf horizontal (oben als Feedback)
constexpr int INV_R1_LX  = 30;   ///< R1 linke Kante
constexpr int INV_R1_RX  = INV_R1_LX + COMP_W_H; ///< R1 rechte Kante (72)
constexpr int INV_R1_Y   = PIN_MINUS_Y;           ///< R1 Y-Mitte (= - Eingang)

constexpr int INV_JUNCT_X = 85;  ///< Knotenpunkt R1/Rf/-Eingang

constexpr int INV_RF_LX  = 98;   ///< Rf linke Kante
constexpr int INV_RF_RX  = INV_RF_LX + COMP_W_H; ///< Rf rechte Kante (140)
constexpr int INV_RF_Y   = 55;   ///< Rf Y-Mitte (Feedback-Linie oben)

constexpr int INV_OUT_X  = 210;  ///< Ausgangs-Knotenpunkt X
constexpr int GND_Y      = 240;  ///< GND-Linie Y

// Nicht-invertierend: Vin am +, R1 vertikal von - nach GND, Rf horizontal (Feedback)
constexpr int NI_JUNCT_X  = 85;   ///< Knotenpunkt -/R1/Rf
constexpr int NI_R1_TOP   = 195;  ///< R1 Oberkante (vertikal, unter dem Knotenpunkt)
constexpr int NI_R1_BOT   = NI_R1_TOP + COMP_H_V; ///< R1 Unterkante (231)

constexpr int NI_RF_LX   = 98;    ///< Rf linke Kante (horizontal, Feedback)
constexpr int NI_RF_RX   = NI_RF_LX + COMP_W_H; ///< Rf rechte Kante (140)
constexpr int NI_RF_Y    = 55;    ///< Rf Y-Mitte

constexpr int NI_OUT_X   = 210;   ///< Ausgangs-Knotenpunkt X

// Touch-Hit-Areas
constexpr int HIT_PAD = 10;  ///< Padding um Bauteile

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

constexpr uint16_t C_FORMULA_TXT = 0xDEDB;
constexpr uint16_t C_INFO_TXT    = 0xB5F7;

// Akzentfarben
constexpr uint16_t C_ACCENT_VIN  = 0xFD20; ///< Vin - Orange
constexpr uint16_t C_ACCENT_R1   = 0x07E0; ///< R1 - Grün
constexpr uint16_t C_ACCENT_RF   = 0xFFE0; ///< Rf - Gelb
constexpr uint16_t C_ACCENT_VOUT = 0x04FF; ///< Vout - Cyan

// ===================================================================
// Schaltplan-Farben
// ===================================================================

constexpr uint16_t C_SCHEM_BG     = 0x1082;
constexpr uint16_t C_OPAMP_BODY   = 0x2945;
constexpr uint16_t C_OPAMP_OL     = 0x4A69;
constexpr uint16_t C_WIRE         = 0x8410;
constexpr uint16_t C_COMP_BODY    = 0x2945;
constexpr uint16_t C_COMP_OUTLINE = 0x6B6D;
constexpr uint16_t C_VAL_ACTIVE   = 0x07E0;
constexpr uint16_t C_VAL_DIMMED   = 0x7D7A;
constexpr uint16_t C_RAIL         = 0xAD75;
constexpr uint16_t C_JUNCTION     = 0xAD75;
constexpr uint16_t C_PIN_LABEL    = 0xDEDB;

// ===================================================================
// Cursor-Blink
// ===================================================================

constexpr unsigned long CURSOR_PERIOD_MS = 800;
constexpr unsigned long CURSOR_ON_MS     = 400;

// ===================================================================
// Footer-Toggle-Farben
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
// Statische Datenbasis
// ===================================================================

const char * const WorkshopTool_OpAmp::QTY_SYMBOLS[NUM_QTY] =
    {"Vin", "R1", "Rf", "Vout"};

const char * const WorkshopTool_OpAmp::QTY_UNITS[NUM_QTY] =
    {"V", "Ohm", "Ohm", "V"};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_OpAmp::WorkshopTool_OpAmp()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_OpAmp::onEnter(LGFX *tft)
{
    clearAll();
    initFooterButtons(tft);
}

/**
 * @brief Aufräumen beim Verlassen.
 */
void WorkshopTool_OpAmp::onExit()
{
    m_buttonsInitialized = false;
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_OpAmp::clearAll()
{
    memset(&m_inv, 0, sizeof(m_inv));
    memset(&m_ni,  0, sizeof(m_ni));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));
    memset(m_compHits,   0, sizeof(m_compHits));

    m_sprite      = nullptr;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;
    m_layoutValid = false;
    m_computeOk   = false;
    m_gain        = 0.0;
    m_gainDb      = 0.0;
    m_numpad.clearLayout();

    m_mode = OpAmpMode::INVERTING;

    m_pressedField = -1;
    m_pressedComp  = -1;
    m_pressedKey   = -1;
    m_touchWas     = false;
    m_touchReady   = false;

    // Beide Modi mit Standardwerten initialisieren
    initModeState(m_inv);
    initModeState(m_ni);

    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));
    refreshSubtitle();

    compute();
}

/**
 * @brief Setzt einen ModeState auf Standardwerte.
 */
void WorkshopTool_OpAmp::initModeState(ModeState &ms) const
{
    memset(&ms, 0, sizeof(ms));

    // Vin=1 V (Slot 0), R1=10 kOhm (Slot 1), Rf=47 kOhm (Slot 2) -> Vout berechnet
    ms.inputSlots[0] = 0; // Vin (ältester)
    ms.inputSlots[1] = 1; // R1
    ms.inputSlots[2] = 2; // Rf (neuester, fokussiert)
    ms.computedQty   = 3; // Vout berechnet
    ms.focusedQty    = 2; // Rf fokussiert

    ms.values[0] = 1.0;      // Vin = 1 V
    ms.values[1] = 10000.0;  // R1 = 10 kOhm
    ms.values[2] = 47000.0;  // Rf = 47 kOhm

    formatEditValue(ms.values[0], ms.bufs[0], FIELD_CAP);
    formatEditValue(ms.values[1], ms.bufs[1], FIELD_CAP);
    formatEditValue(ms.values[2], ms.bufs[2], FIELD_CAP);
}

// ===================================================================
// Zugriff auf modusspezifische Daten
// ===================================================================

/**
 * @brief Gibt den aktiven ModeState zurück.
 */
WorkshopTool_OpAmp::ModeState &WorkshopTool_OpAmp::activeState()
{
    return (m_mode == OpAmpMode::INVERTING) ? m_inv : m_ni;
}

/**
 * @brief Gibt den aktiven ModeState zurück (const).
 */
const WorkshopTool_OpAmp::ModeState &WorkshopTool_OpAmp::activeState() const
{
    return (m_mode == OpAmpMode::INVERTING) ? m_inv : m_ni;
}

// ===================================================================
// Footer-Buttons
// ===================================================================

/**
 * @brief Erstellt die zwei Footer-Toggle-Buttons.
 */
void WorkshopTool_OpAmp::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int totalW = 2 * FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int startX = WorkshopCfg::TOOL_FOOTER_X +
                           (WorkshopCfg::TOOL_FOOTER_W - totalW) / 2;

    const int cxInv  = startX + FOOTER_BTN_W / 2;
    const int cxNonI = cxInv + FOOTER_BTN_W + FOOTER_BTN_GAP;

    I18n::format(m_btnInvertLabel, sizeof(m_btnInvertLabel),
                 WorkshopKey::WORKSHOP_OPAMP_BTN_INVERTING);
    I18n::format(m_btnNonInvLabel, sizeof(m_btnNonInvLabel),
                 WorkshopKey::WORKSHOP_OPAMP_BTN_NON_INVERTING);

    m_btnInvert.initButton(tft, cxInv, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
                           TOGGLE_ACTIVE_OUTLINE, TOGGLE_ACTIVE_FILL, TOGGLE_ACTIVE_TEXT,
                           m_btnInvertLabel, nullptr);
    m_btnInvert.setFont(LeagueGothicCond34);

    m_btnNonInv.initButton(tft, cxNonI, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
                           TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
                           m_btnNonInvLabel, nullptr);
    m_btnNonInv.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
}

/**
 * @brief Aktualisiert Toggle-Farben je nach aktivem Modus.
 */
void WorkshopTool_OpAmp::updateButtonColors()
{
    const bool isInv = (m_mode == OpAmpMode::INVERTING);

    m_btnInvert.setFillColor(isInv ? TOGGLE_ACTIVE_FILL : TOGGLE_INACTIVE_FILL);
    m_btnInvert.setTextColor(isInv ? TOGGLE_ACTIVE_TEXT : TOGGLE_INACTIVE_TEXT);
    m_btnInvert.setOutlineColor(isInv ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);

    m_btnNonInv.setFillColor(isInv ? TOGGLE_INACTIVE_FILL : TOGGLE_ACTIVE_FILL);
    m_btnNonInv.setTextColor(isInv ? TOGGLE_INACTIVE_TEXT : TOGGLE_ACTIVE_TEXT);
    m_btnNonInv.setOutlineColor(isInv ? TOGGLE_INACTIVE_OUTLINE : TOGGLE_ACTIVE_OUTLINE);
}

/**
 * @brief Zeichnet die Footer-Toggle-Buttons.
 */
void WorkshopTool_OpAmp::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft; (void)x; (void)y; (void)w; (void)h;
    if (!m_buttonsInitialized)
        return;

    updateButtonColors();
    m_btnInvert.drawButton();
    m_btnNonInv.drawButton();
}

/**
 * @brief Verarbeitet Touch auf Footer-Buttons.
 */
bool WorkshopTool_OpAmp::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    const bool onInv  = touched && m_btnInvert.contains(tx, ty);
    const bool onNonI = touched && m_btnNonInv.contains(tx, ty);

    const bool wasInv  = m_btnInvert.isPressed();
    const bool wasNonI = m_btnNonInv.isPressed();

    m_btnInvert.press(onInv);
    m_btnNonInv.press(onNonI);

    // Moduswechsel bei Loslassen
    if (m_btnInvert.justReleased() && m_mode != OpAmpMode::INVERTING)
    {
        m_mode = OpAmpMode::INVERTING;
        refreshSubtitle();
        m_layoutValid = false;
        compute();
        return true;
    }
    if (m_btnNonInv.justReleased() && m_mode != OpAmpMode::NON_INVERTING)
    {
        m_mode = OpAmpMode::NON_INVERTING;
        refreshSubtitle();
        m_layoutValid = false;
        compute();
        return true;
    }

    // Visuelle Rückmeldung
    if (onInv != wasInv)
        m_btnInvert.drawButton(onInv);
    if (onNonI != wasNonI)
        m_btnNonInv.drawButton(onNonI);

    return false;
}

/**
 * @brief Dynamischer Untertitel je nach Betriebsart.
 */
const char *WorkshopTool_OpAmp::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Befüllt m_subtitleBuf passend zu m_mode.
 */
void WorkshopTool_OpAmp::refreshSubtitle()
{
    const WorkshopKey key = (m_mode == OpAmpMode::INVERTING)
                                ? WorkshopKey::WORKSHOP_OPAMP_SUB_INVERTING
                                : WorkshopKey::WORKSHOP_OPAMP_SUB_NON_INVERTING;
    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), key);
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Schaltplan, Eingabefelder, Info-Bereich und Numpad.
 */
void WorkshopTool_OpAmp::draw(Gfx_Sprite *sprite,
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
void WorkshopTool_OpAmp::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;
    if (m_rightW < 100) m_rightW = 100;

    // Eingabefelder (rechte Spalte, 10px unter Headerbereich)
    for (int i = 0; i < NUM_QTY; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * ROW_H, m_rightW, ROW_H};

    // Info-Zeile nach den Feldern
    m_infoY = TOP_GAP + NUM_QTY * ROW_H + INFO_GAP_V;

    // Numpad am unteren Rand der rechten Spalte
    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);

    // Schaltplan-Touch-Areas (content-relativ)
    if (m_mode == OpAmpMode::INVERTING)
    {
        // Vin: linker Bereich
        m_compHits[0] = {0, INV_R1_Y - 25, INV_R1_LX, 50};
        // R1: Widerstand-Bereich
        m_compHits[1] = {INV_R1_LX - HIT_PAD, INV_R1_Y - COMP_H_H / 2 - HIT_PAD,
                         COMP_W_H + 2 * HIT_PAD, COMP_H_H + 2 * HIT_PAD};
        // Rf: Feedback-Widerstand
        m_compHits[2] = {INV_RF_LX - HIT_PAD, INV_RF_Y - COMP_H_H / 2 - HIT_PAD,
                         COMP_W_H + 2 * HIT_PAD, COMP_H_H + 2 * HIT_PAD};
        // Vout: Ausgangsbereich
        m_compHits[3] = {TRI_RX, PIN_OUT_Y - 25, SCHEM_W - TRI_RX, 50};
    }
    else
    {
        // Vin: linker Bereich am + Eingang
        m_compHits[0] = {0, PIN_PLUS_Y - 25, TRI_LX - 10, 50};
        // R1: vertikaler Widerstand
        m_compHits[1] = {NI_JUNCT_X - COMP_W / 2 - HIT_PAD, NI_R1_TOP - HIT_PAD,
                         COMP_W + 2 * HIT_PAD, COMP_H_V + 2 * HIT_PAD};
        // Rf: Feedback-Widerstand
        m_compHits[2] = {NI_RF_LX - HIT_PAD, NI_RF_Y - COMP_H_H / 2 - HIT_PAD,
                         COMP_W_H + 2 * HIT_PAD, COMP_H_H + 2 * HIT_PAD};
        // Vout: Ausgangsbereich
        m_compHits[3] = {TRI_RX, PIN_OUT_Y - 25, SCHEM_W - TRI_RX, 50};
    }
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Parst Eingabepuffer und berechnet die Ausgabegröße.
 */
void WorkshopTool_OpAmp::compute()
{
    ModeState &ms = activeState();

    // Eingabewerte aus Puffern parsen
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
    {
        const int q = static_cast<int>(ms.inputSlots[s]);
        ms.values[q] = (ms.bufs[q][0] != '\0') ? atof(ms.bufs[q]) : 0.0;
    }

    m_computeOk = false;
    m_gain   = 0.0;
    m_gainDb = 0.0;

    // Finite-Check
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (!std::isfinite(ms.values[static_cast<int>(ms.inputSlots[s])]))
            return;

    const double vin  = ms.values[0];
    const double r1   = ms.values[1];
    const double rf   = ms.values[2];
    const double vout = ms.values[3];

    if (m_mode == OpAmpMode::INVERTING)
    {
        switch (ms.computedQty)
        {
            case 3: // Vout = -Vin × Rf / R1
            {
                if (std::abs(r1) < 1e-15) return;
                const double result = -vin * rf / r1;
                if (!std::isfinite(result)) return;
                ms.values[3] = result;
                m_computeOk = true;
                break;
            }
            case 0: // Vin = -Vout × R1 / Rf
            {
                if (std::abs(rf) < 1e-15) return;
                const double result = -vout * r1 / rf;
                if (!std::isfinite(result)) return;
                ms.values[0] = result;
                m_computeOk = true;
                break;
            }
            case 1: // R1 = -Vin × Rf / Vout
            {
                if (std::abs(vout) < 1e-15) return;
                const double result = -vin * rf / vout;
                if (!std::isfinite(result) || result < 0.0) return;
                ms.values[1] = result;
                m_computeOk = true;
                break;
            }
            case 2: // Rf = -Vout × R1 / Vin
            {
                if (std::abs(vin) < 1e-15) return;
                const double result = -vout * r1 / vin;
                if (!std::isfinite(result) || result < 0.0) return;
                ms.values[2] = result;
                m_computeOk = true;
                break;
            }
            default: break;
        }

        // Gain berechnen
        if (m_computeOk && std::abs(ms.values[1]) > 1e-15)
        {
            m_gain = -ms.values[2] / ms.values[1];
            const double absGain = std::abs(m_gain);
            m_gainDb = (absGain > 1e-15) ? 20.0 * log10(absGain) : -999.0;
        }
    }
    else // NON_INVERTING
    {
        switch (ms.computedQty)
        {
            case 3: // Vout = Vin × (1 + Rf / R1)
            {
                if (std::abs(r1) < 1e-15) return;
                const double result = vin * (1.0 + rf / r1);
                if (!std::isfinite(result)) return;
                ms.values[3] = result;
                m_computeOk = true;
                break;
            }
            case 0: // Vin = Vout / (1 + Rf / R1)
            {
                if (std::abs(r1) < 1e-15) return;
                const double gain = 1.0 + rf / r1;
                if (std::abs(gain) < 1e-15) return;
                const double result = vout / gain;
                if (!std::isfinite(result)) return;
                ms.values[0] = result;
                m_computeOk = true;
                break;
            }
            case 1: // R1 = Rf / (Vout / Vin - 1)
            {
                if (std::abs(vin) < 1e-15) return;
                const double ratio = vout / vin;
                if (ratio <= 1.0) return; // Gain muss > 1 sein
                const double result = rf / (ratio - 1.0);
                if (!std::isfinite(result) || result < 0.0) return;
                ms.values[1] = result;
                m_computeOk = true;
                break;
            }
            case 2: // Rf = R1 × (Vout / Vin - 1)
            {
                if (std::abs(vin) < 1e-15) return;
                const double result = r1 * (vout / vin - 1.0);
                if (!std::isfinite(result) || result < 0.0) return;
                ms.values[2] = result;
                m_computeOk = true;
                break;
            }
            default: break;
        }

        // Gain berechnen
        if (m_computeOk && std::abs(ms.values[1]) > 1e-15)
        {
            m_gain = 1.0 + ms.values[2] / ms.values[1];
            const double absGain = std::abs(m_gain);
            m_gainDb = (absGain > 1e-15) ? 20.0 * log10(absGain) : -999.0;
        }
    }
}

// ===================================================================
// Zeichnen - Schaltplan
// ===================================================================

/**
 * @brief Zeichnet den kompletten Schaltplan-Bereich.
 */
void WorkshopTool_OpAmp::drawSchematic()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;

    // Hintergrund
    spr->fillRect(ox, oy, SCHEM_W, m_h, C_SCHEM_BG);

    // Op-Amp-Dreieck (beide Modi gleich positioniert)
    drawOpAmpTriangle(TRI_LX, TRI_TY, TRI_RX, TRI_BY);

    if (m_mode == OpAmpMode::INVERTING)
        drawInvertingCircuit();
    else
        drawNonInvertingCircuit();
}

/**
 * @brief Zeichnet das Op-Amp-Dreieck-Symbol.
 */
void WorkshopTool_OpAmp::drawOpAmpTriangle(int triLX, int triTY, int triRX, int triBY)
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;
    const int triCY = (triTY + triBY) / 2;

    // Outline-Trick: erst größeres Dreieck in Outline-Farbe, dann inneres in Body-Farbe
    constexpr int OL = 2; // Outline-Breite
    spr->fillTriangle(ox + triLX - OL, oy + triTY - OL,
                      ox + triLX - OL, oy + triBY + OL,
                      ox + triRX + OL + 2, oy + triCY,
                      C_OPAMP_OL);
    spr->fillTriangle(ox + triLX + OL, oy + triTY + OL + 1,
                      ox + triLX + OL, oy + triBY - OL - 1,
                      ox + triRX - OL, oy + triCY,
                      C_OPAMP_BODY);

    // +/- Beschriftungen in der Nähe der Eingänge
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(ML_DATUM);

    // - oben (invertierender Eingang)
    spr->setTextColor(C_PIN_LABEL, C_OPAMP_BODY);
    spr->drawString("-", ox + triLX + 10, oy + PIN_MINUS_Y);

    // + unten (nicht-invertierender Eingang)
    spr->drawString("+", ox + triLX + 10, oy + PIN_PLUS_Y);

    spr->unloadFont();
}

/**
 * @brief Zeichnet die invertierende Beschaltung.
 *
 * Schema:
 *              Rf
 *         ┌───[Rf]────┐
 *         │            │
 * Vin──[R1]──┬── (-)  ╲
 *             │         ├── Vout
 *        GND──── (+)  ╱
 */
void WorkshopTool_OpAmp::drawInvertingCircuit()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;
    const ModeState &ms = activeState();

    // ===================================================================
    // Verdrahtung
    // ===================================================================

    // Vin -> R1 (horizontal bei y=PIN_MINUS_Y)
    spr->drawFastHLine(ox + 8, oy + INV_R1_Y, INV_R1_LX - 8, C_WIRE);

    // R1 -> Knotenpunkt
    spr->drawFastHLine(ox + INV_R1_RX, oy + INV_R1_Y, INV_JUNCT_X - INV_R1_RX, C_WIRE);

    // Knotenpunkt -> - Eingang
    spr->drawFastHLine(ox + INV_JUNCT_X, oy + PIN_MINUS_Y, TRI_LX - INV_JUNCT_X, C_WIRE);

    // Knotenpunkt nach oben zum Feedback
    spr->drawFastVLine(ox + INV_JUNCT_X, oy + INV_RF_Y, PIN_MINUS_Y - INV_RF_Y, C_WIRE);

    // Feedback-Linie: Knotenpunkt -> Rf links
    spr->drawFastHLine(ox + INV_JUNCT_X, oy + INV_RF_Y, INV_RF_LX - INV_JUNCT_X, C_WIRE);

    // Rf rechts -> Ausgangs-Knotenpunkt
    spr->drawFastHLine(ox + INV_RF_RX, oy + INV_RF_Y, INV_OUT_X - INV_RF_RX, C_WIRE);

    // Ausgangs-Knotenpunkt nach unten zum Ausgang
    spr->drawFastVLine(ox + INV_OUT_X, oy + INV_RF_Y, PIN_OUT_Y - INV_RF_Y, C_WIRE);

    // Ausgangsleitung vom Dreieck -> rechts
    spr->drawFastHLine(ox + TRI_RX, oy + PIN_OUT_Y, INV_OUT_X - TRI_RX, C_WIRE);

    // Vout weiter nach rechts
    spr->drawFastHLine(ox + INV_OUT_X, oy + PIN_OUT_Y, 35, C_WIRE);

    // + Eingang -> GND (kurze Leitung nach links und dann nach unten)
    spr->drawFastHLine(ox + INV_JUNCT_X, oy + PIN_PLUS_Y, TRI_LX - INV_JUNCT_X, C_WIRE);
    spr->drawFastVLine(ox + INV_JUNCT_X, oy + PIN_PLUS_Y, GND_Y - PIN_PLUS_Y, C_WIRE);

    // GND-Symbol (drei abnehmende Linien)
    constexpr int gndX = INV_JUNCT_X;
    spr->drawFastHLine(ox + gndX - 10, oy + GND_Y,      21, C_WIRE);
    spr->drawFastHLine(ox + gndX - 6,  oy + GND_Y + 4,  13, C_WIRE);
    spr->drawFastHLine(ox + gndX - 3,  oy + GND_Y + 8,   7, C_WIRE);

    // ===================================================================
    // Knotenpunkte
    // ===================================================================

    spr->fillCircle(ox + INV_JUNCT_X, oy + PIN_MINUS_Y, 3, C_JUNCTION);
    spr->fillCircle(ox + INV_OUT_X, oy + PIN_OUT_Y, 3, C_JUNCTION);

    // ===================================================================
    // Bauteile
    // ===================================================================

    const bool r1Active = (static_cast<int>(ms.focusedQty) == 1);
    const bool rfActive = (static_cast<int>(ms.focusedQty) == 2);

    // R1 horizontal
    drawResistor(INV_R1_LX, INV_R1_Y - COMP_H_H / 2, COMP_H_H, true, r1Active);
    // Rf horizontal (Feedback)
    drawResistor(INV_RF_LX, INV_RF_Y - COMP_H_H / 2, COMP_H_H, true, rfActive);

    // ===================================================================
    // Beschriftungen
    // ===================================================================

    spr->loadFont(ComfortaaBold14);

    // Vin-Label links
    const bool vinActive = (static_cast<int>(ms.focusedQty) == 0);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(vinActive ? C_VAL_ACTIVE : C_RAIL, C_SCHEM_BG);
    spr->drawString("Vin", ox + 5, oy + INV_R1_Y - 16);
    char vinBuf[16];
    formatSchematicValue(0, vinBuf, sizeof(vinBuf));
    spr->drawString(vinBuf, ox + 5, oy + INV_R1_Y + 16);

    // Vout-Label rechts
    const bool voutActive = (static_cast<int>(ms.focusedQty) == 3);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(voutActive ? C_VAL_ACTIVE : C_RAIL, C_SCHEM_BG);
    spr->drawString("Vout", ox + INV_OUT_X + 6, oy + PIN_OUT_Y - 16);
    char voutBuf[16];
    formatSchematicValue(3, voutBuf, sizeof(voutBuf));
    spr->drawString(voutBuf, ox + INV_OUT_X + 6, oy + PIN_OUT_Y + 16);

    // R1-Label (über dem Widerstand)
    spr->setTextDatum(MC_DATUM);
    const int r1CX = INV_R1_LX + COMP_W_H / 2;
    spr->setTextColor(r1Active ? C_VAL_ACTIVE : C_VAL_DIMMED, C_SCHEM_BG);
    spr->drawString("R1", ox + r1CX, oy + INV_R1_Y - COMP_H_H / 2 - 10);
    char r1Buf[16];
    formatSchematicValue(1, r1Buf, sizeof(r1Buf));
    spr->drawString(r1Buf, ox + r1CX, oy + INV_R1_Y + COMP_H_H / 2 + 10);

    // Rf-Label (über dem Widerstand)
    const int rfCX = INV_RF_LX + COMP_W_H / 2;
    spr->setTextColor(rfActive ? C_VAL_ACTIVE : C_VAL_DIMMED, C_SCHEM_BG);
    spr->drawString("Rf", ox + rfCX, oy + INV_RF_Y - COMP_H_H / 2 - 10);
    char rfBuf[16];
    formatSchematicValue(2, rfBuf, sizeof(rfBuf));
    spr->drawString(rfBuf, ox + rfCX, oy + INV_RF_Y + COMP_H_H / 2 + 10);

    // GND-Label
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_RAIL, C_SCHEM_BG);
    spr->drawString("GND", ox + gndX + 14, oy + GND_Y + 2);

    spr->unloadFont();
}

/**
 * @brief Zeichnet die nicht-invertierende Beschaltung.
 *
 * Schema:
 *              Rf
 *         ┌───[Rf]────┐
 *         │            │
 *         ├──── (-)    ╲
 *         │             ├── Vout
 * Vin──────── (+)      ╱
 *         │
 *        [R1]
 *         │
 *        GND
 */
void WorkshopTool_OpAmp::drawNonInvertingCircuit()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;
    const ModeState &ms = activeState();

    // ===================================================================
    // Verdrahtung
    // ===================================================================

    // Vin -> + Eingang (horizontal bei y=PIN_PLUS_Y)
    spr->drawFastHLine(ox + 8, oy + PIN_PLUS_Y, TRI_LX - 8, C_WIRE);

    // - Eingang -> Knotenpunkt (horizontal bei y=PIN_MINUS_Y)
    spr->drawFastHLine(ox + NI_JUNCT_X, oy + PIN_MINUS_Y, TRI_LX - NI_JUNCT_X, C_WIRE);

    // Knotenpunkt nach oben zum Feedback
    spr->drawFastVLine(ox + NI_JUNCT_X, oy + NI_RF_Y, PIN_MINUS_Y - NI_RF_Y, C_WIRE);

    // Feedback: Knotenpunkt -> Rf links
    spr->drawFastHLine(ox + NI_JUNCT_X, oy + NI_RF_Y, NI_RF_LX - NI_JUNCT_X, C_WIRE);

    // Rf rechts -> Ausgangs-Knotenpunkt
    spr->drawFastHLine(ox + NI_RF_RX, oy + NI_RF_Y, NI_OUT_X - NI_RF_RX, C_WIRE);

    // Ausgangs-Knotenpunkt nach unten zum Ausgang
    spr->drawFastVLine(ox + NI_OUT_X, oy + NI_RF_Y, PIN_OUT_Y - NI_RF_Y, C_WIRE);

    // Ausgangsleitung vom Dreieck -> rechts
    spr->drawFastHLine(ox + TRI_RX, oy + PIN_OUT_Y, NI_OUT_X - TRI_RX, C_WIRE);

    // Vout weiter nach rechts
    spr->drawFastHLine(ox + NI_OUT_X, oy + PIN_OUT_Y, 35, C_WIRE);

    // Knotenpunkt nach unten zu R1
    spr->drawFastVLine(ox + NI_JUNCT_X, oy + PIN_MINUS_Y, NI_R1_TOP - PIN_MINUS_Y, C_WIRE);

    // R1 nach unten zu GND
    spr->drawFastVLine(ox + NI_JUNCT_X, oy + NI_R1_BOT, GND_Y - NI_R1_BOT, C_WIRE);

    // GND-Symbol
    constexpr int gndX = NI_JUNCT_X;
    spr->drawFastHLine(ox + gndX - 10, oy + GND_Y,      21, C_WIRE);
    spr->drawFastHLine(ox + gndX - 6,  oy + GND_Y + 4,  13, C_WIRE);
    spr->drawFastHLine(ox + gndX - 3,  oy + GND_Y + 8,   7, C_WIRE);

    // ===================================================================
    // Knotenpunkte
    // ===================================================================

    spr->fillCircle(ox + NI_JUNCT_X, oy + PIN_MINUS_Y, 3, C_JUNCTION);
    spr->fillCircle(ox + NI_OUT_X, oy + PIN_OUT_Y, 3, C_JUNCTION);

    // ===================================================================
    // Bauteile
    // ===================================================================

    const bool r1Active = (static_cast<int>(ms.focusedQty) == 1);
    const bool rfActive = (static_cast<int>(ms.focusedQty) == 2);

    // R1 vertikal (unter dem Knotenpunkt)
    drawResistor(NI_JUNCT_X, NI_R1_TOP, COMP_H_V, false, r1Active);

    // Rf horizontal (Feedback)
    drawResistor(NI_RF_LX, NI_RF_Y - COMP_H_H / 2, COMP_H_H, true, rfActive);

    // ===================================================================
    // Beschriftungen
    // ===================================================================

    spr->loadFont(ComfortaaBold14);

    // Vin-Label links
    const bool vinActive = (static_cast<int>(ms.focusedQty) == 0);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(vinActive ? C_VAL_ACTIVE : C_RAIL, C_SCHEM_BG);
    spr->drawString("Vin", ox + 5, oy + PIN_PLUS_Y - 16);
    char vinBuf[16];
    formatSchematicValue(0, vinBuf, sizeof(vinBuf));
    spr->drawString(vinBuf, ox + 5, oy + PIN_PLUS_Y + 16);

    // Vout-Label rechts
    const bool voutActive = (static_cast<int>(ms.focusedQty) == 3);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(voutActive ? C_VAL_ACTIVE : C_RAIL, C_SCHEM_BG);
    spr->drawString("Vout", ox + NI_OUT_X + 6, oy + PIN_OUT_Y - 16);
    char voutBuf[16];
    formatSchematicValue(3, voutBuf, sizeof(voutBuf));
    spr->drawString(voutBuf, ox + NI_OUT_X + 6, oy + PIN_OUT_Y + 16);

    // R1-Label (links vom vertikalen Widerstand)
    spr->setTextDatum(MR_DATUM);
    const int r1CY = (NI_R1_TOP + NI_R1_BOT) / 2;
    spr->setTextColor(r1Active ? C_VAL_ACTIVE : C_VAL_DIMMED, C_SCHEM_BG);
    spr->drawString("R1", ox + NI_JUNCT_X - COMP_W / 2 - 4, oy + r1CY - 8);
    char r1Buf[16];
    formatSchematicValue(1, r1Buf, sizeof(r1Buf));
    spr->drawString(r1Buf, ox + NI_JUNCT_X - COMP_W / 2 - 4, oy + r1CY + 8);

    // Rf-Label (über dem Widerstand)
    const int rfCX = NI_RF_LX + COMP_W_H / 2;
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(rfActive ? C_VAL_ACTIVE : C_VAL_DIMMED, C_SCHEM_BG);
    spr->drawString("Rf", ox + rfCX, oy + NI_RF_Y - COMP_H_H / 2 - 10);
    char rfBuf[16];
    formatSchematicValue(2, rfBuf, sizeof(rfBuf));
    spr->drawString(rfBuf, ox + rfCX, oy + NI_RF_Y + COMP_H_H / 2 + 10);

    // GND-Label
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_RAIL, C_SCHEM_BG);
    spr->drawString("GND", ox + gndX + 14, oy + GND_Y + 2);

    spr->unloadFont();
}

/**
 * @brief Zeichnet ein Widerstands-Symbol.
 */
void WorkshopTool_OpAmp::drawResistor(int cx, int top, int h, bool horiz, bool active)
{
    Gfx_Sprite * const spr = m_sprite;
    const uint16_t fill = active ? 0x1A45 : C_COMP_BODY;
    const uint16_t outline = active ? C_VAL_ACTIVE : C_COMP_OUTLINE;

    if (horiz)
    {
        // Horizontaler Widerstand: cx=links, top=oben, Breite=COMP_W_H, Höhe=h
        const int ax = m_originX + cx;
        const int ay = m_originY + top;
        spr->fillRect(ax, ay, COMP_W_H, h, fill);
        spr->drawRect(ax, ay, COMP_W_H, h, outline);
    }
    else
    {
        // Vertikaler Widerstand: cx=Mitte-X, top=oben, Höhe=h
        const int ax = m_originX + cx - COMP_W / 2;
        const int ay = m_originY + top;
        spr->fillRect(ax, ay, COMP_W, h, fill);
        spr->drawRect(ax, ay, COMP_W, h, outline);
    }
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle vier Eingabefelder.
 */
void WorkshopTool_OpAmp::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);
    for (int i = 0; i < NUM_QTY; ++i)
        drawField(i, m_pressedField == i || m_pressedComp == i, cursorOn);
    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Größen-Zeile.
 */
void WorkshopTool_OpAmp::drawField(int qty, bool pressed, bool cursorOn)
{
    Gfx_Sprite * const spr = m_sprite;
    const Rect        &fr  = m_fieldRects[qty];
    const int          ax  = m_originX + fr.x;
    const int          ay  = m_originY + fr.y;

    const ModeState &ms = activeState();
    const bool input   = isInput(qty);
    const bool focused = (static_cast<int>(ms.focusedQty) == qty);

    uint16_t bgColor;
    if      (pressed) bgColor = C_FIELD_PRESS;
    else if (focused) bgColor = C_FIELD_FOCUS;
    else              bgColor = C_FIELD_INPUT;

    spr->fillRect(ax, ay, fr.w, fr.h, bgColor);
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, accentColor(qty));

    // Textfarben
    uint16_t lblClr, valClr, unitClr;
    if      (focused) { lblClr = C_LBL_FOCUS; valClr = C_VAL_FOCUS; unitClr = C_UNIT_FOCUS; }
    else if (input)   { lblClr = C_LBL_INPUT; valClr = C_VAL_INPUT; unitClr = C_UNIT_INPUT; }
    else              { lblClr = C_LBL_COMP;  valClr = C_VAL_COMP;  unitClr = C_UNIT_COMP;  }

    const int cy = ay + fr.h / 2;

    // Symbol (Vin / R1 / Rf / Vout)
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
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), ms.bufs[qty]);
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
        formatDisplayValue(ms.values[qty], qty, displayBuf, sizeof(displayBuf));
        spr->setTextColor(C_VAL_INPUT, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
    else
    {
        if (m_computeOk)
            formatDisplayValue(ms.values[qty], qty, displayBuf, sizeof(displayBuf));
        else
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");
        spr->setTextColor(C_VAL_COMP, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}

/**
 * @brief Zeichnet den Info-Bereich (Gain + dB).
 */
void WorkshopTool_OpAmp::drawInfoRow()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_infoY;
    const int rw = m_rightW;
    const int cy = oy + INFO_H / 2;

    spr->fillRect(ox, oy, rw, INFO_H, C_BG);
    spr->loadFont(ComfortaaBold14);

    if (m_computeOk && std::isfinite(m_gain) && std::isfinite(m_gainDb))
    {
        char infoBuf[INFO_STR_CAP];
        stringutils::safeFormat(infoBuf, sizeof(infoBuf),
                                "A = %.4g (%.1f dB)", m_gain, m_gainDb);
        dotToComma(infoBuf);
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_FORMULA_TXT, C_BG);
        spr->drawString(infoBuf, ox + 4, cy);
    }
    else
    {
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_INFO_TXT, C_BG);
        spr->drawString("A = ---", ox + 4, cy);
    }

    spr->unloadFont();
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch mit FIFO-3-Logik für Felder und Schaltplan-Bauteile.
 */
bool WorkshopTool_OpAmp::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        // Schaltplan (linke Spalte)
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
            // Eingabefelder
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
                // Numpad
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
            ModeState &ms = activeState();

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
                    ms.focusedQty = static_cast<uint8_t>(tapped);
                }
                else
                {
                    // FIFO: ältester Slot wird berechnet, tapped wird Eingabe
                    ms.computedQty   = ms.inputSlots[0];
                    ms.inputSlots[0] = ms.inputSlots[1];
                    ms.inputSlots[1] = ms.inputSlots[2];
                    ms.inputSlots[2] = static_cast<uint8_t>(tapped);
                    ms.focusedQty    = static_cast<uint8_t>(tapped);

                    if (m_computeOk)
                        formatEditValue(ms.values[tapped], ms.bufs[tapped], FIELD_CAP);
                    else
                        ms.bufs[tapped][0] = '\0';

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

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf Eingabefelder.
 */
int WorkshopTool_OpAmp::hitTestField(int cx, int cy) const
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
int WorkshopTool_OpAmp::hitTestComponent(int cx, int cy) const
{
    for (int i = 0; i < NUM_QTY; ++i)
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
void WorkshopTool_OpAmp::executeKey(char action)
{
    ModeState &ms = activeState();
    char * const buf = ms.bufs[ms.focusedQty];
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
// Formatierung
// ===================================================================

/**
 * @brief Formatiert SI-Wert als auto-skalierten Anzeigestring.
 */
void WorkshopTool_OpAmp::formatDisplayValue(double val, int qty,
                                             char *buf, size_t sz)
{
    if (!buf || sz == 0) return;

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

    double      scaled;
    const char *prefix;
    const char *unit = QTY_UNITS[qty];
    const bool  isResistance = (qty == 1 || qty == 2);

    if (isResistance)
    {
        if      (absVal < 1.0e3) { scaled = val;         prefix = "";  }
        else if (absVal < 1.0e6) { scaled = val / 1.0e3; prefix = "k"; }
        else if (absVal < 1.0e9) { scaled = val / 1.0e6; prefix = "M"; }
        else                     { scaled = val / 1.0e9; prefix = "G"; }
    }
    else
    {
        if      (absVal < 1.0e-3) { scaled = val * 1.0e6; prefix = "u"; }
        else if (absVal < 1.0)    { scaled = val * 1.0e3; prefix = "m"; }
        else if (absVal < 1.0e3)  { scaled = val;         prefix = "";  }
        else if (absVal < 1.0e6)  { scaled = val / 1.0e3; prefix = "k"; }
        else                      { scaled = val / 1.0e6; prefix = "M"; }
    }

    stringutils::safeFormat(buf, sz, "%.4g %s%s", scaled, prefix, unit);
    dotToComma(buf);
}

/**
 * @brief Formatiert SI-Wert als editierbaren Dezimalstring.
 */
void WorkshopTool_OpAmp::formatEditValue(double val, char *buf, size_t sz)
{
    if (!buf || sz == 0) return;
    if (!std::isfinite(val) || val == 0.0)
    {
        stringutils::safeCopy(buf, sz, "0");
        return;
    }
    stringutils::safeFormat(buf, sz, "%.6g", val);
}

/**
 * @brief Formatiert einen Bauteilwert für die Schaltplan-Beschriftung.
 */
void WorkshopTool_OpAmp::formatSchematicValue(int qty, char *buf, size_t sz) const
{
    if (!buf || sz == 0) return;

    const ModeState &ms = activeState();
    const double val = ms.values[qty];

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
        stringutils::safeFormat(buf, sz, "%.3gV", val);
    }

    dotToComma(buf);
}

/**
 * @brief Prüft ob eine Größe als Eingabe aktiv ist.
 */
bool WorkshopTool_OpAmp::isInput(int qty) const
{
    const ModeState &ms = activeState();
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (static_cast<int>(ms.inputSlots[s]) == qty)
            return true;
    return false;
}

/**
 * @brief Akzentfarbe je Größe.
 */
uint16_t WorkshopTool_OpAmp::accentColor(int qty)
{
    switch (qty)
    {
        case 0:  return C_ACCENT_VIN;
        case 1:  return C_ACCENT_R1;
        case 2:  return C_ACCENT_RF;
        case 3:  return C_ACCENT_VOUT;
        default: return 0xFFFF;
    }
}
