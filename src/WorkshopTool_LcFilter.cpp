#include "WorkshopTool_LcFilter.h"

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

namespace
{

    // ===================================================================
    // Spaltenaufteilung
    // ===================================================================

    constexpr int SCHEM_W = 250; ///< Breite der linken Spalte (Schaltplan)
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
            if (*p == '.')
                *p = ',';
    }

    // ===================================================================
    // Schaltplan-Geometrie
    // ===================================================================

    // Signalpfad (horizontal)
    constexpr int SIG_Y = 90;                 ///< Y-Position des Signalpfads im Schaltplan
    constexpr int VIN_X = 20;                 ///< Linkes Ende der Vin-Zuleitung
    constexpr int LONG_LX = 47;               ///< Längsbauteil linke Kante
    constexpr int LONG_W = 81;                ///< Längsbauteil Breite (horizontal)
    constexpr int LONG_H = 26;                ///< Längsbauteil Höhe (horizontal)
    constexpr int LONG_RX = LONG_LX + LONG_W; ///< 128
    constexpr int JUNCT_X = 158;              ///< Knotenpunkt X (Längs/Quer-Verzweigung)
    constexpr int VOUT_X = 215;               ///< Rechtes Ende der Vout-Leitung

    // Querbauteil (vertikal, gegen GND)
    constexpr int TRANS_TOP_Y = 115;                   ///< Querbauteil Oberkante
    constexpr int TRANS_W = 26;                        ///< Querbauteil Breite (vertikal)
    constexpr int TRANS_H = 81;                        ///< Querbauteil Höhe (vertikal)
    constexpr int TRANS_BOT_Y = TRANS_TOP_Y + TRANS_H; ///< 196
    constexpr int GND_Y = 215;                         ///< GND-Schiene Y

    // Touch-Hit-Padding
    constexpr int HIT_PAD = 12; ///< Zusätzlicher Abstand um Bauteile und Interaktionsbereiche für zuverlässige Touch-Erkennung

    // ===================================================================
    // Rechte Spalte - Farben
    // ===================================================================

    constexpr uint16_t C_BG = 0x1082;          ///< Hintergrundfarbe der rechten Spalte
    constexpr uint16_t C_FIELD_INPUT = 0x2945; ///< Farbe für aktive Eingabefelder
    constexpr uint16_t C_FIELD_FOCUS = 0x328A; ///< Farbe für fokussiertes Eingabefeld
    constexpr uint16_t C_FIELD_PRESS = 0x4B4D; ///< Farbe für gedrücktes Eingabefeld
    constexpr uint16_t C_SEP = 0x2104;         ///< Farbe für Trennlinien zwischen Feldern

    constexpr uint16_t C_LBL_FOCUS = 0xFFFF;  ///< Farbe für Label des fokussierten Felds
    constexpr uint16_t C_LBL_INPUT = 0xDEDB;  ///< Farbe für Label von Feldern mit Eingabe
    constexpr uint16_t C_LBL_COMP = 0x7BEF;   ///< Farbe für Label von berechneten Feldern
    constexpr uint16_t C_VAL_FOCUS = 0xFFFF;  ///< Farbe für Wert des fokussierten Felds
    constexpr uint16_t C_VAL_INPUT = 0xEF7D;  ///< Farbe für Wert von Feldern mit Eingabe
    constexpr uint16_t C_VAL_COMP = 0xB5F7;   ///< Farbe für Wert von berechneten Feldern
    constexpr uint16_t C_UNIT_FOCUS = 0xAD75; ///< Farbe für Einheit des fokussierten Felds
    constexpr uint16_t C_UNIT_INPUT = 0x8410; ///< Farbe für Einheit von Feldern mit Eingabe
    constexpr uint16_t C_UNIT_COMP = 0x4228;  ///< Farbe für Einheit von berechneten Feldern

    constexpr uint16_t C_INFO_TXT = 0xDEDB; ///< Farbe für Info-Text (Hinweise, Fehlermeldungen)

    // Akzentfarben je Größe
    constexpr uint16_t C_ACCENT_L = 0xFFE0;  ///< L  - Gelb
    constexpr uint16_t C_ACCENT_C = 0x04FF;  ///< C  - Cyan
    constexpr uint16_t C_ACCENT_FC = 0xFD20; ///< fc - Orange

    // ===================================================================
    // Schaltplan-Farben
    // ===================================================================

    constexpr uint16_t C_SCHEM_BG = 0x1082;     ///< Hintergrundfarbe des Schaltplans
    constexpr uint16_t C_WIRE = 0x8410;         ///< Farbe für Leitungen
    constexpr uint16_t C_COMP_OUTLINE = 0x6B6D; ///< Farbe für Bauteil-Umrisse
    constexpr uint16_t C_VAL_DIMMED = 0x7D7A;   ///< Farbe für Werte im Schaltplan, wenn nicht fokussiert (gedimmt)
    constexpr uint16_t C_RAIL = 0xAD75;         ///< Farbe für Vin/GND-Schienen (und Knotenpunkt, wenn nicht fokussiert)
    constexpr uint16_t C_JUNCTION = 0xAD75;     ///< Farbe für Knotenpunkt (wenn fokussiert, sonst C_RAIL)

    // ===================================================================
    // Footer-Toggle-Farben
    // ===================================================================

    constexpr uint16_t TOGGLE_ACTIVE_FILL = 0x328A;      ///< Farbe für aktiven Toggle-Button (Hintergrund)
    constexpr uint16_t TOGGLE_ACTIVE_TEXT = 0xFFFF;      ///< Farbe für aktiven Toggle-Button (Text)
    constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;   ///< Farbe für aktiven Toggle-Button (Rahmen)
    constexpr uint16_t TOGGLE_INACTIVE_FILL = 0x0000;    ///< Farbe für inaktiven Toggle-Button (Hintergrund)
    constexpr uint16_t TOGGLE_INACTIVE_TEXT = 0x7BEF;    ///< Farbe für inaktiven Toggle-Button (Text)
    constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8; ///< Farbe für inaktiven Toggle-Button (Rahmen)

    constexpr int FOOTER_BTN_W = 185;  ///< Breite der Toggle-Buttons im Footer
    constexpr int FOOTER_BTN_H = 44;   ///< Höhe der Toggle-Buttons im Footer
    constexpr int FOOTER_BTN_R = 6;    ///< Eckenradius der Toggle-Buttons im Footer
    constexpr int FOOTER_BTN_GAP = 10; ///< Abstand zwischen den Toggle-Buttons im Footer

} // namespace

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_LcFilter::WorkshopTool_LcFilter()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_LcFilter::onEnter(LGFX *tft)
{
    clearAll();
    initFooterButtons(tft);
}

/**
 * @brief Aufräumen beim Verlassen.
 */
void WorkshopTool_LcFilter::onExit()
{
    m_buttonsInitialized = false;
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_LcFilter::clearAll()
{
    memset(&m_tp, 0, sizeof(m_tp));
    memset(&m_hp, 0, sizeof(m_hp));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));
    memset(m_compHits, 0, sizeof(m_compHits));

    m_sprite = nullptr;
    m_originX = 0;
    m_originY = 0;
    m_w = 0;
    m_h = 0;
    m_layoutValid = false;
    m_computeOk = false;
    m_numpad.clearLayout();

    m_kind = LcFilterKind::LOW_PASS;

    m_pressedField = -1;
    m_pressedComp = -1;
    m_pressedKey = -1;
    m_touchWas = false;
    m_touchReady = false;

    // Beide Modi mit identischen Standardwerten initialisieren
    initModeState(m_tp);
    initModeState(m_hp);

    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));
    refreshSubtitle();

    compute();
}

/**
 * @brief Setzt einen ModeState auf Standardwerte (L=10 mH, C=100 nF).
 */
void WorkshopTool_LcFilter::initModeState(ModeState &ms) const
{
    memset(&ms, 0, sizeof(ms));

    // FIFO: L(0) als ältester, C(1) als neuester -> fc(2) wird berechnet
    ms.inputSlots[0] = 0; // L (ältester)
    ms.inputSlots[1] = 1; // C (neuester, fokussiert)
    ms.computedQty = 2;   // fc berechnet
    ms.focusedQty = 1;    // C fokussiert

    ms.values[0] = 10.0e-3;  // L  = 10 mH
    ms.values[1] = 100.0e-9; // C  = 100 nF

    // Eingabepuffer als Mantissen in der jeweiligen Anzeige-Skala
    formatMantissa(ms.values[0], scaleForQty(0), ms.bufs[0], FIELD_CAP);
    formatMantissa(ms.values[1], scaleForQty(1), ms.bufs[1], FIELD_CAP);
}

// ===================================================================
// Zugriff auf modusspezifische Daten
// ===================================================================

/**
 * @brief Gibt den aktiven ModeState zurück.
 */
WorkshopTool_LcFilter::ModeState &WorkshopTool_LcFilter::activeState()
{
    return (m_kind == LcFilterKind::LOW_PASS) ? m_tp : m_hp;
}

/**
 * @brief Gibt den aktiven ModeState zurück (const).
 */
const WorkshopTool_LcFilter::ModeState &WorkshopTool_LcFilter::activeState() const
{
    return (m_kind == LcFilterKind::LOW_PASS) ? m_tp : m_hp;
}

// ===================================================================
// Topologie-Helfer (Längs/Quer-Mapping)
// ===================================================================

/**
 * @brief Größenindex des Längs-Bauteils.
 *
 * Mapping:
 *   Tiefpass:  L längs (0), C quer (1)
 *   Hochpass:  C längs (1), L quer (0)
 */
int WorkshopTool_LcFilter::longitudinalQty() const
{
    return (m_kind == LcFilterKind::LOW_PASS) ? 0 : 1;
}

/**
 * @brief Größenindex des Quer-Bauteils.
 */
int WorkshopTool_LcFilter::transverseQty() const
{
    return (m_kind == LcFilterKind::LOW_PASS) ? 1 : 0;
}

/**
 * @brief Bauteilart für eine Größe - L ist immer Spule, C immer Kondensator.
 */
WorkshopTool_LcFilter::CompKind WorkshopTool_LcFilter::kindOfQty(int qty)
{
    return (qty == 0) ? CompKind::IND : CompKind::CAP;
}

// ===================================================================
// Symbol- und Einheiten-Helfer
// ===================================================================

/**
 * @brief Symboltext für Eingabefeld und Schaltplan-Label.
 */
const char *WorkshopTool_LcFilter::qtySymbol(int qty)
{
    if (qty == 0)
        return "L";

    if (qty == 1)
        return "C";

    return "fc";
}

/**
 * @brief Skalierte Anzeige-Einheit je Größe.
 */
const char *WorkshopTool_LcFilter::scaledUnit(int qty)
{
    if (qty == 0)
        return "mH";

    if (qty == 1)
        return "nF";

    return "kHz";
}

/**
 * @brief Skalierungsfaktor je Größe (Mantisse * scale = SI-Wert).
 */
double WorkshopTool_LcFilter::scaleForQty(int qty)
{
    if (qty == 0)
        return 1.0e-3; // mH -> H

    if (qty == 1)
        return 1.0e-9; // nF -> F

    return 1.0e3; // kHz -> Hz
}

/**
 * @brief Akzentfarbe je Größe.
 */
uint16_t WorkshopTool_LcFilter::accentColor(int qty)
{
    if (qty == 0)
        return C_ACCENT_L;

    if (qty == 1)
        return C_ACCENT_C;

    return C_ACCENT_FC;
}

/**
 * @brief Prüft ob eine Größe als Eingabe aktiv ist.
 */
bool WorkshopTool_LcFilter::isInput(int qty) const
{
    const ModeState &ms = activeState();

    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (static_cast<int>(ms.inputSlots[s]) == qty)
            return true;

    return false;
}

// ===================================================================
// Footer-Buttons (Tiefpass / Hochpass-Toggle)
// ===================================================================

/**
 * @brief Erstellt die zwei Footer-Toggle-Buttons.
 */
void WorkshopTool_LcFilter::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int totalW = 2 * FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int startX = WorkshopCfg::TOOL_FOOTER_X + (WorkshopCfg::TOOL_FOOTER_W - totalW) / 2;

    const int cxLow = startX + FOOTER_BTN_W / 2;
    const int cxHigh = cxLow + FOOTER_BTN_W + FOOTER_BTN_GAP;

    I18n::format(m_btnLowPassLabel,  sizeof(m_btnLowPassLabel),
                 WorkshopKey::WORKSHOP_LC_FILTER_BTN_LOW_PASS);
    I18n::format(m_btnHighPassLabel, sizeof(m_btnHighPassLabel),
                 WorkshopKey::WORKSHOP_LC_FILTER_BTN_HIGH_PASS);

    m_btnLowPass.initButton(
        tft,                   // Display-Objekt
        cxLow,                 // X-Mitte des Tiefpass-Buttons
        cy,                    // Y-Mitte der Buttons (beide auf gleicher Höhe)
        FOOTER_BTN_W,          // Breite der Buttons
        FOOTER_BTN_H,          // Höhe der Buttons
        FOOTER_BTN_R,          // Eckenradius der Buttons
        TOGGLE_ACTIVE_OUTLINE, // Rahmenfarbe für aktiven Zustand
        TOGGLE_ACTIVE_FILL,    // Füllfarbe für aktiven Zustand
        TOGGLE_ACTIVE_TEXT,    // Textfarbe für aktiven Zustand
        m_btnLowPassLabel,
        nullptr                // Kein Icon
    );

    m_btnLowPass.setFont(LeagueGothicCond34);

    m_btnHighPass.initButton(
        tft,                     // Display-Objekt
        cxHigh,                  // X-Mitte des Hochpass-Buttons
        cy,                      // Y-Mitte der Buttons (beide auf gleicher Höhe)
        FOOTER_BTN_W,            // Breite der Buttons
        FOOTER_BTN_H,            // Höhe der Buttons
        FOOTER_BTN_R,            // Eckenradius der Buttons
        TOGGLE_INACTIVE_OUTLINE, // Rahmenfarbe für inaktiven Zustand
        TOGGLE_INACTIVE_FILL,    // Füllfarbe für inaktiven Zustand
        TOGGLE_INACTIVE_TEXT,    // Textfarbe für inaktiven Zustand
        m_btnHighPassLabel,
        nullptr                  // Kein Icon
    );

    m_btnHighPass.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
}

/**
 * @brief Aktualisiert Toggle-Farben je nach aktivem Filtertyp.
 */
void WorkshopTool_LcFilter::updateButtonColors()
{
    const bool isLow = (m_kind == LcFilterKind::LOW_PASS);

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
void WorkshopTool_LcFilter::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    updateButtonColors();
    m_btnLowPass.drawButton();
    m_btnHighPass.drawButton();
}

/**
 * @brief Verarbeitet Touch auf den Footer-Buttons.
 */
bool WorkshopTool_LcFilter::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    const bool onLow = touched && m_btnLowPass.contains(tx, ty);
    const bool onHigh = touched && m_btnHighPass.contains(tx, ty);

    const bool wasLow = m_btnLowPass.isPressed();
    const bool wasHigh = m_btnHighPass.isPressed();

    m_btnLowPass.press(onLow);
    m_btnHighPass.press(onHigh);

    // Filtertyp-Wechsel bei Loslassen
    if (m_btnLowPass.justReleased() && m_kind != LcFilterKind::LOW_PASS)
    {
        m_kind = LcFilterKind::LOW_PASS;
        refreshSubtitle();
        m_layoutValid = false;
        compute();
        return true;
    }

    if (m_btnHighPass.justReleased() && m_kind != LcFilterKind::HIGH_PASS)
    {
        m_kind = LcFilterKind::HIGH_PASS;
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
 * @brief Dynamischer Untertitel je nach Filterart.
 */
const char *WorkshopTool_LcFilter::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Befüllt m_subtitleBuf passend zu m_kind.
 */
void WorkshopTool_LcFilter::refreshSubtitle()
{
    const WorkshopKey key = (m_kind == LcFilterKind::LOW_PASS)
                                ? WorkshopKey::WORKSHOP_LC_FILTER_SUB_LP
                                : WorkshopKey::WORKSHOP_LC_FILTER_SUB_HP;
    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), key);
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Schaltplan, Felder, Info-Zeile und Numpad.
 */
void WorkshopTool_LcFilter::draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
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
 * @brief Berechnet Spaltenaufteilung, Feld-Rects, Hit-Areas, Numpad.
 */
void WorkshopTool_LcFilter::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;
    if (m_rightW < 100)
        m_rightW = 100;

    // Eingabefelder (rechte Spalte, unter Headerbereich mit TOP_GAP)
    for (int i = 0; i < NUM_QTY; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * ROW_H, m_rightW, ROW_H};

    // Info-Zeile direkt nach den Feldern
    m_infoY = TOP_GAP + NUM_QTY * ROW_H + INFO_GAP_V;

    // Numpad am unteren Rand der rechten Spalte
    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);

    // Schaltplan-Touch-Areas (content-relativ, unter TOP_GAP)
    m_compHits[0] = {LONG_LX - HIT_PAD,
                     TOP_GAP + SIG_Y - LONG_H / 2 - HIT_PAD,
                     LONG_W + 2 * HIT_PAD,
                     LONG_H + 2 * HIT_PAD};

    m_compHits[1] = {JUNCT_X - TRANS_W / 2 - HIT_PAD,
                     TOP_GAP + TRANS_TOP_Y - HIT_PAD,
                     TRANS_W + 2 * HIT_PAD,
                     TRANS_H + 2 * HIT_PAD};
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Parst Eingabepuffer und berechnet die Ausgabegröße.
 */
void WorkshopTool_LcFilter::compute()
{
    ModeState &ms = activeState();

    // Eingabewerte aus Puffern parsen - Mantisse × Skala ergibt SI-Wert
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
    {
        const int q = static_cast<int>(ms.inputSlots[s]);
        const double scale = scaleForQty(q);
        ms.values[q] = (ms.bufs[q][0] != '\0') ? atof(ms.bufs[q]) * scale : 0.0;
    }

    m_computeOk = false;

    // Finite-Check
    for (int s = 0; s < NUM_INPUT_SLOTS; ++s)
        if (!std::isfinite(ms.values[static_cast<int>(ms.inputSlots[s])]))
            return;

    const double l = ms.values[0];  // Induktivität (H)
    const double c = ms.values[1];  // Kapazität (F)
    const double fc = ms.values[2]; // Resonanzfrequenz (Hz)

    switch (ms.computedQty)
    {
    case 2: // fc berechnen: fc = 1 / (2π · √(L · C))
    {
        if (l <= 0.0 || c <= 0.0)
            return;

        const double product = l * c;
        if (product <= 0.0)
            return;

        const double result = 1.0 / (LC_TWO_PI * std::sqrt(product));
        if (!std::isfinite(result) || result <= 0.0)
            return;

        ms.values[2] = result;
        m_computeOk = true;
        break;
    }
    case 0: // L berechnen: L = 1 / ((2π · fc)² · C)
    {
        if (c <= 0.0 || fc <= 0.0)
            return;

        const double w0 = LC_TWO_PI * fc;
        const double result = 1.0 / (w0 * w0 * c);
        if (!std::isfinite(result) || result <= 0.0)
            return;

        ms.values[0] = result;
        m_computeOk = true;
        break;
    }
    case 1: // C berechnen: C = 1 / ((2π · fc)² · L)
    {
        if (l <= 0.0 || fc <= 0.0)
            return;

        const double w0 = LC_TWO_PI * fc;
        const double result = 1.0 / (w0 * w0 * l);
        if (!std::isfinite(result) || result <= 0.0)
            return;

        ms.values[1] = result;
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
 * @brief Formatiert einen SI-Wert als Mantisse für eine gegebene Skala.
 */
void WorkshopTool_LcFilter::formatMantissa(double siValue, double scale, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

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
void WorkshopTool_LcFilter::formatSchematicValue(int qty, char *buf, size_t sz) const
{
    if (!buf || sz == 0)
        return;

    const ModeState &ms = activeState();
    const double val = ms.values[qty];

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "?");
        return;
    }

    const double absVal = std::abs(val);

    if (qty == 2) // Frequenz
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
    else if (qty == 1) // Kapazität
    {
        if (absVal >= 1.0e-6)
            stringutils::safeFormat(buf, sz, "%.3gu", val * 1.0e6);
        else if (absVal >= 1.0e-9)
            stringutils::safeFormat(buf, sz, "%.3gn", val * 1.0e9);
        else
            stringutils::safeFormat(buf, sz, "%.3gp", val * 1.0e12);
    }
    else // Induktivität (qty == 0)
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
// Zeichnen - Schaltplan
// ===================================================================

/**
 * @brief Zeichnet den kompletten Schaltplan-Bereich.
 *
 * Der Schaltplan beginnt direkt unter dem Headerbereich bei (ox, oy+TOP_GAP). Längs- und Quer-Bauteil werden anhand des aktiven Filtertyps bestimmt.
 */
void WorkshopTool_LcFilter::drawSchematic()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;
    const int sh = m_h - TOP_GAP;

    // Hintergrund
    spr->fillRect(ox, oy, SCHEM_W, sh, C_SCHEM_BG);

    // GND-Schiene
    spr->drawFastHLine(ox + 20, oy + GND_Y, SCHEM_W - 40, C_WIRE);

    // Längs- und Quer-Größenindex bestimmen
    const int longQ = longitudinalQty();
    const int transQ = transverseQty();

    const ModeState &ms = activeState();
    const bool longActive = (static_cast<int>(ms.focusedQty) == longQ);
    const bool transActive = (static_cast<int>(ms.focusedQty) == transQ);

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
    spr->fillCircle(ox + JUNCT_X, oy + SIG_Y, 3, C_JUNCTION);
    spr->fillCircle(ox + JUNCT_X, oy + GND_Y, 3, C_JUNCTION);

    // ===================================================================
    // Bauteile
    // ===================================================================

    drawComponentHoriz(kindOfQty(longQ), LONG_LX, SIG_Y, longActive);
    drawComponentVert(kindOfQty(transQ), JUNCT_X, TRANS_TOP_Y, transActive);

    // ===================================================================
    // Beschriftungen
    // ===================================================================

    spr->loadFont(ComfortaaBold14);

    // Vin / Vout / GND
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_RAIL, C_SCHEM_BG);
    spr->drawString("Vin", ox + 4, oy + SIG_Y - 14);
    spr->drawString("Vout", ox + VOUT_X + 4, oy + SIG_Y - 14);
    spr->drawString("GND", ox + 4, oy + GND_Y - 2);

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
        spr->drawString(qtySymbol(transQ), ox + JUNCT_X + TRANS_W / 2 + 8, oy + tcy - 10);

        char buf[16];
        formatSchematicValue(transQ, buf, sizeof(buf));
        spr->drawString(buf, ox + JUNCT_X + TRANS_W / 2 + 8, oy + tcy + 10);
    }

    spr->unloadFont();
}

// ===================================================================
// Zeichnen - Bauteil-Symbole (horizontal + vertikal)
// ===================================================================

/**
 * @brief Zeichnet ein horizontales Bauteil entlang des Signalpfads.
 */
void WorkshopTool_LcFilter::drawComponentHoriz(CompKind kind, int leftX, int centerY, bool active)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    switch (kind)
    {
    case CompKind::CAP:
    {
        // Kondensator
        const int plateGap = 8;
        const int plateH = 22;
        const int midX = leftX + LONG_W / 2;
        const int leftPlateX = midX - plateGap / 2;
        const int rightPlateX = midX + plateGap / 2;
        const uint16_t ol = active ? C_ACCENT_C : C_COMP_OUTLINE;

        // Verbindungsleitungen zur linken/rechten Bauteilkante
        spr->drawFastHLine(ox + leftX, oy + centerY, leftPlateX - leftX, ol);
        spr->drawFastHLine(ox + rightPlateX, oy + centerY, leftX + LONG_W - rightPlateX, ol);

        // Zwei vertikale Platten
        spr->fillRect(ox + leftPlateX - 1, oy + centerY - plateH / 2, 3, plateH, ol);
        spr->fillRect(ox + rightPlateX - 1, oy + centerY - plateH / 2, 3, plateH, ol);
        break;
    }
    case CompKind::IND:
    {
        // Spule
        const int numBumps = 4;
        const float bumpR = 10.0f;
        const float thickness = 2.5f;
        const int bumpD = static_cast<int>(2.0f * bumpR) - 1;
        const int totalW = (numBumps - 1) * bumpD + static_cast<int>(2.0f * bumpR);
        const int padX = (LONG_W - totalW) / 2;
        const int firstCx = leftX + padX + static_cast<int>(bumpR);
        const uint16_t ol = active ? C_ACCENT_L : C_COMP_OUTLINE;

        // Anschlussleitungen links und rechts
        if (padX > 0)
        {
            spr->drawFastHLine(ox + leftX, oy + centerY, padX, ol);
            spr->drawFastHLine(ox + leftX + LONG_W - padX, oy + centerY, padX, ol);
        }

        // Vier AA-Kreise nebeneinander (Mittelpunkt exakt auf Mittellinie)
        for (int i = 0; i < numBumps; ++i)
        {
            const int cx = firstCx + i * bumpD;
            spr->drawSmoothCircle(ox + cx, oy + centerY, bumpR, thickness, ol, C_SCHEM_BG);
        }

        // Untere Hälfte wegschneiden
        spr->fillRect(ox + leftX + padX, oy + centerY + 1, totalW, static_cast<int>(bumpR) + 2, C_SCHEM_BG);
        break;
    }
    }
}

/**
 * @brief Zeichnet ein vertikales Bauteil zwischen Knotenpunkt und GND.
 */
void WorkshopTool_LcFilter::drawComponentVert(CompKind kind, int centerX, int topY, bool active)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;

    switch (kind)
    {
    case CompKind::CAP:
    {
        // Kondensator
        const int plateGap = 8;
        const int plateW = 22;
        const int midY = topY + TRANS_H / 2;
        const int topPlateY = midY - plateGap / 2;
        const int botPlateY = midY + plateGap / 2;
        const uint16_t ol = active ? C_ACCENT_C : C_COMP_OUTLINE;

        // Verbindungsleitungen oben/unten
        spr->drawFastVLine(ox + centerX, oy + topY, topPlateY - topY, ol);
        spr->drawFastVLine(ox + centerX, oy + botPlateY, topY + TRANS_H - botPlateY, ol);

        // Zwei horizontale Platten
        spr->fillRect(ox + centerX - plateW / 2, oy + topPlateY - 1, plateW, 3, ol);
        spr->fillRect(ox + centerX - plateW / 2, oy + botPlateY - 1, plateW, 3, ol);
        break;
    }
    case CompKind::IND:
    {
        // Spule
        const int numBumps = 4;
        const float bumpR = 10.0f;
        const float thickness = 2.5f;
        const int bumpD = static_cast<int>(2.0f * bumpR) - 1;
        const int totalH = (numBumps - 1) * bumpD + static_cast<int>(2.0f * bumpR);
        const int padY = (TRANS_H - totalH) / 2;
        const int firstCy = topY + padY + static_cast<int>(bumpR);
        const uint16_t ol = active ? C_ACCENT_L : C_COMP_OUTLINE;

        // Anschlussleitungen oben und unten
        if (padY > 0)
        {
            spr->drawFastVLine(ox + centerX, oy + topY, padY, ol);
            spr->drawFastVLine(ox + centerX, oy + topY + TRANS_H - padY, padY, ol);
        }

        // Vier AA-Kreise untereinander (Mittelpunkt exakt auf Mittellinie)
        for (int i = 0; i < numBumps; ++i)
        {
            const int cy = firstCy + i * bumpD;
            spr->drawSmoothCircle(ox + centerX, oy + cy, bumpR, thickness, ol, C_SCHEM_BG);
        }

        // Rechte Hälfte wegschneiden
        spr->fillRect(ox + centerX + 1, oy + topY + padY, static_cast<int>(bumpR) + 2, totalH, C_SCHEM_BG);
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
void WorkshopTool_LcFilter::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);
    for (int i = 0; i < NUM_QTY; ++i)
    {
        const bool pressed = (m_pressedField == i) || (m_pressedComp == 0 && i == longitudinalQty()) || (m_pressedComp == 1 && i == transverseQty());
        drawField(i, pressed, cursorOn);
    }

    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Größen-Zeile.
 */
void WorkshopTool_LcFilter::drawField(int qty, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const Rect &fr = m_fieldRects[qty];
    const int ax = m_originX + fr.x;
    const int ay = m_originY + fr.y;

    const ModeState &ms = activeState();
    const bool input = isInput(qty);
    const bool focused = (static_cast<int>(ms.focusedQty) == qty);

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

    uint16_t lblClr, valClr, unitClr;
    if (focused)
    {
        lblClr = C_LBL_FOCUS;
        valClr = C_VAL_FOCUS;
        unitClr = C_UNIT_FOCUS;
    }
    else if (input)
    {
        lblClr = C_LBL_INPUT;
        valClr = C_VAL_INPUT;
        unitClr = C_UNIT_INPUT;
    }
    else
    {
        lblClr = C_LBL_COMP;
        valClr = C_VAL_COMP;
        unitClr = C_UNIT_COMP;
    }

    const int cy = ay + fr.h / 2;

    // Symbol (L / C / fc)
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblClr, bgColor);
    spr->drawString(qtySymbol(qty), ax + ACCENT_W + 6, cy);

    // Skalierte Einheit (mH / nF / kHz)
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(unitClr, bgColor);
    spr->drawString(scaledUnit(qty), ax + fr.w - UNIT_W / 2, cy);

    // Wert
    char displayBuf[32];
    const int valRightX = ax + fr.w - UNIT_W - 6;
    spr->setTextDatum(MR_DATUM);

    if (focused)
    {
        // Punkt -> Komma. Der interne Puffer bleibt unverändert mit '.' damit atof() weiterhin korrekt parst.
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), ms.bufs[qty]);
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
        formatMantissa(ms.values[qty], scaleForQty(qty), displayBuf, sizeof(displayBuf));
        dotToComma(displayBuf);
        spr->setTextColor(valClr, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
    else
    {
        if (m_computeOk)
        {
            formatMantissa(ms.values[qty], scaleForQty(qty), displayBuf, sizeof(displayBuf));
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
// Zeichnen - Info-Zeile (Steigung + Kennimpedanz)
// ===================================================================

/**
 * @brief Zeichnet den Info-Bereich mit Steigung und Kennimpedanz Z0.
 *
 * Z0 = √(L/C) wird aus den aktuellen L- und C-Werten berechnet und mit Auto-Skalierung (Ohm / kOhm) angezeigt. Bei ungültigen Werten (L oder C nicht positiv) erscheint "Z0 = ---".
 */
void WorkshopTool_LcFilter::drawInfoRow()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_infoY;
    const int rw = m_rightW;
    const int cy = oy + INFO_H / 2;

    spr->fillRect(ox, oy, rw, INFO_H, C_BG);
    spr->loadFont(ComfortaaBold14);

    // Steigung links (2. Ordnung -> 40 dB/Dekade)
    const bool isLow = (m_kind == LcFilterKind::LOW_PASS);
    const __FlashStringHelper *slopeStr = isLow
        ? I18n::str(WorkshopKey::WORKSHOP_LC_FILTER_SLOPE_LP)
        : I18n::str(WorkshopKey::WORKSHOP_LC_FILTER_SLOPE_HP);

    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_INFO_TXT, C_BG);
    spr->drawString(slopeStr, ox + 4, cy);

    // Kennimpedanz rechts: Z0 = √(L/C)
    char z0Buf[INFO_STR_CAP];
    const ModeState &ms = activeState();
    const double l = ms.values[0];
    const double c = ms.values[1];

    if (l > 0.0 && c > 0.0 && std::isfinite(l) && std::isfinite(c))
    {
        const double z0 = std::sqrt(l / c);
        if (std::isfinite(z0) && z0 > 0.0)
        {
            if (z0 >= 1000.0)
                stringutils::safeFormat(z0Buf, sizeof(z0Buf), "Z0 = %.3g kOhm", z0 / 1000.0);
            else
                stringutils::safeFormat(z0Buf, sizeof(z0Buf), "Z0 = %.3g Ohm", z0);
            
                dotToComma(z0Buf);
        }
        else
        {
            stringutils::safeCopy(z0Buf, sizeof(z0Buf), "Z0 = ---");
        }
    }
    else
    {
        stringutils::safeCopy(z0Buf, sizeof(z0Buf), "Z0 = ---");
    }

    spr->setTextDatum(MR_DATUM);
    spr->drawString(z0Buf, ox + rw - 4, cy);

    spr->unloadFont();
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf Eingabefelder.
 */
int WorkshopTool_LcFilter::hitTestField(int cx, int cy) const
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
int WorkshopTool_LcFilter::hitTestComponent(int cx, int cy) const
{
    for (int i = 0; i < 2; ++i)
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
// Tastatureingabe
// ===================================================================

/**
 * @brief Führt eine Tastenaktion auf dem aktiven Eingabepuffer aus.
 */
void WorkshopTool_LcFilter::executeKey(char action)
{
    ModeState &ms = activeState();
    char *const buf = ms.bufs[ms.focusedQty];
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
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch mit FIFO-Logik für Felder, Bauteile und Numpad.
 */
bool WorkshopTool_LcFilter::handleTouch(int32_t tx, int32_t ty, bool touched)
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
                m_pressedComp = hitComp;
                m_pressedField = -1;
                m_pressedKey = -1;
                needsRedraw = true;
            }
        }
        else if (cx >= m_rightX)
        {
            // Rechte Spalte: Eingabefelder oder NumpadK
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
            ModeState &ms = activeState();

            // Bauteil- oder Feld-Tap (gemeinsame FIFO-Logik)
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
                    ms.focusedQty = static_cast<uint8_t>(tapped);
                }
                else
                {
                    // FIFO: ältester Slot wird berechnet, tapped wird Eingabe
                    ms.computedQty = ms.inputSlots[0];
                    ms.inputSlots[0] = ms.inputSlots[1];
                    ms.inputSlots[1] = static_cast<uint8_t>(tapped);
                    ms.focusedQty = static_cast<uint8_t>(tapped);

                    if (m_computeOk)
                        formatMantissa(ms.values[tapped], scaleForQty(tapped),
                                       ms.bufs[tapped], FIELD_CAP);
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
        m_pressedComp = -1;
        m_pressedKey = -1;
        m_touchWas = false;
    }

    return needsRedraw;
}
