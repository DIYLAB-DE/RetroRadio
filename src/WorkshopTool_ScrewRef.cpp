#include "WorkshopTool_ScrewRef.h"

#include <cstring>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
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

constexpr int DRAW_W   = 200;  ///< Breite des Zeichenbereichs
constexpr int COL_GAP  = 12;   ///< Abstand Zeichnung -> Daten

// ===================================================================
// Hilfsfunktion - Dezimalkomma (deutsches Format)
// ===================================================================

/**
 * @brief Ersetzt den Dezimalpunkt durch ein Komma im übergebenen Puffer.
 * @param buf Puffer mit formatiertem Zahlenwert
 */
void dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.') *p = ',';
}

// ===================================================================
// Farben - Selector-Chips
// ===================================================================

constexpr uint16_t C_BG           = 0x1082;  ///< Hintergrund
constexpr uint16_t C_CHIP_SEL     = 0x328A;  ///< Gewählter Chip
constexpr uint16_t C_CHIP_UNSEL   = 0x2104;  ///< Nicht gewählter Chip
constexpr uint16_t C_CHIP_PRESS   = 0x4B4D;  ///< Gedrückter Chip
constexpr uint16_t C_CHIP_TXT_SEL = 0xFFFF;  ///< Text gewählt
constexpr uint16_t C_CHIP_TXT_UN  = 0x8410;  ///< Text nicht gewählt

// ===================================================================
// Farben - Zeichnung
// ===================================================================

constexpr uint16_t C_SCREW_BODY   = 0x6B6D;  ///< Schraubenkörper
constexpr uint16_t C_SCREW_HEAD   = 0x7BEF;  ///< Kopf (etwas heller)
constexpr uint16_t C_SCREW_THREAD = 0x528A;  ///< Gewindemarkierung
constexpr uint16_t C_HEX_INDENT   = 0x2945;  ///< Innensechskant-Vertiefung
constexpr uint16_t C_DIM_LINE     = 0x8410;  ///< Bemaßungslinien
constexpr uint16_t C_DIM_CORE     = 0xF800;  ///< Kernloch-Bemaßung (Rot)
constexpr uint16_t C_DIM_TEXT     = 0xAD75;  ///< Bemaßungstext
constexpr uint16_t C_DRAW_BG      = 0x1082;  ///< Zeichnungshintergrund

// ===================================================================
// Farben - Datentabelle
// ===================================================================

constexpr uint16_t C_ROW_BG       = 0x18C3;  ///< Zeilen-Hintergrund
constexpr uint16_t C_ROW_BG_ALT   = 0x1082;  ///< Alternierend
constexpr uint16_t C_ROW_LABEL    = 0x7D7A;  ///< Label-Text
constexpr uint16_t C_ROW_VALUE    = 0xDEDB;  ///< Wert-Text
constexpr uint16_t C_ROW_HL_BG    = 0x3000;  ///< Kernloch-Zeile Hintergrund (dunkelrot)
constexpr uint16_t C_ROW_HL_LABEL = 0xFB20;  ///< Kernloch-Zeile Label
constexpr uint16_t C_ROW_HL_VALUE = 0xFFFF;  ///< Kernloch-Zeile Wert

// ===================================================================
// Footer-Toggle
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
// Statische Datenbasis - Schraubengrößen M2 bis M12
// ===================================================================

const ScrewData WorkshopTool_ScrewRef::SIZES[NUM_SIZES] =
{
    //  name    nom   pitch pFine  core  coreFine through inbus  hex   headH  headD torque88
    { "M2",    2.0f, 0.40f, 0.0f,  1.60f, 0.0f,   2.4f, 1.5f,  4.0f, 2.0f,  3.8f,  0.4f },
    { "M2,5",  2.5f, 0.45f, 0.0f,  2.05f, 0.0f,   2.9f, 2.0f,  5.0f, 2.5f,  4.5f,  0.8f },
    { "M3",    3.0f, 0.50f, 0.35f, 2.50f, 2.65f,  3.4f, 2.5f,  5.5f, 3.0f,  5.5f,  1.3f },
    { "M4",    4.0f, 0.70f, 0.50f, 3.30f, 3.50f,  4.5f, 3.0f,  7.0f, 4.0f,  7.0f,  3.0f },
    { "M5",    5.0f, 0.80f, 0.50f, 4.20f, 4.50f,  5.5f, 4.0f,  8.0f, 5.0f,  8.5f,  5.7f },
    { "M6",    6.0f, 1.00f, 0.75f, 5.00f, 5.25f,  6.6f, 5.0f, 10.0f, 6.0f, 10.0f, 10.0f },
    { "M8",    8.0f, 1.25f, 1.00f, 6.80f, 7.00f,  9.0f, 6.0f, 13.0f, 8.0f, 13.0f, 24.0f },
    { "M10",  10.0f, 1.50f, 1.25f, 8.50f, 8.75f, 11.0f, 8.0f, 17.0f,10.0f, 16.0f, 47.0f },
    { "M12",  12.0f, 1.75f, 1.50f,10.20f,10.50f, 13.5f,10.0f, 19.0f,12.0f, 18.0f, 82.0f },
};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_ScrewRef::WorkshopTool_ScrewRef()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_ScrewRef::onEnter(LGFX *tft)
{
    clearAll();
    initFooterButtons(tft);
}

/**
 * @brief Aufräumen beim Verlassen.
 */
void WorkshopTool_ScrewRef::onExit()
{
    m_buttonsInitialized = false;
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_ScrewRef::clearAll()
{
    memset(m_chipRects, 0, sizeof(m_chipRects));
    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));

    m_sprite      = nullptr;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;
    m_layoutValid = false;

    m_threadMode  = ThreadMode::COARSE;
    m_selectedIdx = 4; // M5

    m_pressedChip = -1;
    m_touchWas    = false;
    m_touchReady  = false;

    refreshSubtitle();
}

// ===================================================================
// Footer-Buttons
// ===================================================================

/**
 * @brief Erstellt die Footer-Toggle-Buttons.
 */
void WorkshopTool_ScrewRef::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int totalW = 2 * FOOTER_BTN_W + FOOTER_BTN_GAP;
    constexpr int startX = WorkshopCfg::TOOL_FOOTER_X +
                           (WorkshopCfg::TOOL_FOOTER_W - totalW) / 2;

    const int cxCoarse = startX + FOOTER_BTN_W / 2;
    const int cxFine   = cxCoarse + FOOTER_BTN_W + FOOTER_BTN_GAP;

    I18n::format(m_btnCoarseLabel, sizeof(m_btnCoarseLabel),
                 WorkshopKey::WORKSHOP_SCREW_REF_BTN_COARSE);
    I18n::format(m_btnFineLabel, sizeof(m_btnFineLabel),
                 WorkshopKey::WORKSHOP_SCREW_REF_BTN_FINE);

    m_btnCoarse.initButton(tft, cxCoarse, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
                           TOGGLE_ACTIVE_OUTLINE, TOGGLE_ACTIVE_FILL, TOGGLE_ACTIVE_TEXT,
                           m_btnCoarseLabel, nullptr);
    m_btnCoarse.setFont(LeagueGothicCond34);

    m_btnFine.initButton(tft, cxFine, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
                         TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
                         m_btnFineLabel, nullptr);
    m_btnFine.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
}

/**
 * @brief Aktualisiert Toggle-Farben je nach aktivem Modus.
 */
void WorkshopTool_ScrewRef::updateButtonColors()
{
    const bool isCoarse = (m_threadMode == ThreadMode::COARSE);

    m_btnCoarse.setFillColor(isCoarse ? TOGGLE_ACTIVE_FILL : TOGGLE_INACTIVE_FILL);
    m_btnCoarse.setTextColor(isCoarse ? TOGGLE_ACTIVE_TEXT : TOGGLE_INACTIVE_TEXT);
    m_btnCoarse.setOutlineColor(isCoarse ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);

    m_btnFine.setFillColor(isCoarse ? TOGGLE_INACTIVE_FILL : TOGGLE_ACTIVE_FILL);
    m_btnFine.setTextColor(isCoarse ? TOGGLE_INACTIVE_TEXT : TOGGLE_ACTIVE_TEXT);
    m_btnFine.setOutlineColor(isCoarse ? TOGGLE_INACTIVE_OUTLINE : TOGGLE_ACTIVE_OUTLINE);
}

/**
 * @brief Zeichnet die Footer-Toggle-Buttons.
 */
void WorkshopTool_ScrewRef::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft; (void)x; (void)y; (void)w; (void)h;
    if (!m_buttonsInitialized) return;

    updateButtonColors();
    m_btnCoarse.drawButton();
    m_btnFine.drawButton();
}

/**
 * @brief Verarbeitet Touch auf Footer-Buttons.
 */
bool WorkshopTool_ScrewRef::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized) return false;

    const bool onCoarse = touched && m_btnCoarse.contains(tx, ty);
    const bool onFine   = touched && m_btnFine.contains(tx, ty);

    const bool wasCoarse = m_btnCoarse.isPressed();
    const bool wasFine   = m_btnFine.isPressed();

    m_btnCoarse.press(onCoarse);
    m_btnFine.press(onFine);

    if (m_btnCoarse.justReleased() && m_threadMode != ThreadMode::COARSE)
    {
        m_threadMode = ThreadMode::COARSE;
        refreshSubtitle();
        return true;
    }
    if (m_btnFine.justReleased() && m_threadMode != ThreadMode::FINE)
    {
        m_threadMode = ThreadMode::FINE;
        refreshSubtitle();
        return true;
    }

    if (onCoarse != wasCoarse)
        m_btnCoarse.drawButton(onCoarse);
    if (onFine != wasFine)
        m_btnFine.drawButton(onFine);

    return false;
}

/**
 * @brief Dynamischer Untertitel je nach Gewindeart.
 */
const char *WorkshopTool_ScrewRef::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Befüllt m_subtitleBuf passend zur aktuellen Gewindeart.
 */
void WorkshopTool_ScrewRef::refreshSubtitle()
{
    const WorkshopKey key = (m_threadMode == ThreadMode::COARSE)
                                ? WorkshopKey::WORKSHOP_SCREW_REF_SUB_COARSE
                                : WorkshopKey::WORKSHOP_SCREW_REF_SUB_FINE;
    I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), key);
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Selector, Zeichnung und Datentabelle.
 */
void WorkshopTool_ScrewRef::draw(Gfx_Sprite *sprite,
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
    drawData();
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Selector-Chips, Zeichenbereich und Datentabelle.
 */
void WorkshopTool_ScrewRef::computeLayout(int w, int h)
{
    // Selector-Chips (volle Breite, bündig mit Headerbereich)
    const int totalGap = (NUM_SIZES - 1) * CHIP_GAP;
    const int chipW = (w - totalGap) / NUM_SIZES;
    int cx = 0;

    for (int i = 0; i < NUM_SIZES; ++i)
    {
        // Letzter Chip bekommt Restbreite
        const int cw = (i == NUM_SIZES - 1) ? (w - cx) : chipW;
        m_chipRects[i] = {cx, CHIP_TOP, cw, CHIP_H};
        cx += cw + CHIP_GAP;
    }

    // Zeichenbereich (links, unter Selector)
    m_drawX = 0;
    m_drawW = DRAW_W;
    m_drawY = SEL_ROW_H;
    m_drawH = h - SEL_ROW_H;

    // Datentabelle (rechts, unter Selector)
    m_dataX = DRAW_W + COL_GAP;
    m_dataW = w - m_dataX;
    m_dataY = SEL_ROW_H;
}

// ===================================================================
// Zeichnen - Selector-Chips
// ===================================================================

/**
 * @brief Zeichnet die Größenauswahl-Chips.
 */
void WorkshopTool_ScrewRef::drawSelector()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;

    // Hintergrund der Selector-Zeile
    spr->fillRect(ox, oy, m_w, SEL_ROW_H, C_BG);

    spr->loadFont(ComfortaaBold14);

    for (int i = 0; i < NUM_SIZES; ++i)
    {
        const Rect &r = m_chipRects[i];
        const bool sel = (i == m_selectedIdx);
        const bool pressed = (i == m_pressedChip);

        uint16_t bg, fg;
        if (pressed)     { bg = C_CHIP_PRESS;  fg = C_CHIP_TXT_SEL; }
        else if (sel)    { bg = C_CHIP_SEL;     fg = C_CHIP_TXT_SEL; }
        else             { bg = C_CHIP_UNSEL;   fg = C_CHIP_TXT_UN;  }

        spr->fillSmoothRoundRect(ox + r.x, oy + r.y, r.w, r.h, CHIP_R, bg);

        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(fg, bg);
        spr->drawString(SIZES[i].name, ox + r.x + r.w / 2, oy + r.y + r.h / 2);
    }

    spr->unloadFont();
}

// ===================================================================
// Zeichnen - Schrauben-Seitenansicht
// ===================================================================

/**
 * @brief Zeichnet die Seitenansicht (fixe Größe) mit Bemaßung.
 */
void WorkshopTool_ScrewRef::drawSchematic()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX + m_drawX;
    const int oy = m_originY + m_drawY;

    // Hintergrund
    spr->fillRect(ox, oy, m_drawW, m_drawH, C_DRAW_BG);

    const ScrewData &s = SIZES[m_selectedIdx];

    // Fixe Zeichnung auf Basis M12 (größte Schraube)
    const ScrewData &ref = SIZES[NUM_SIZES - 1]; // M12
    const float refShaftLen = ref.nominal * 1.8f;
    const float refTotalH   = ref.headHeight + refShaftLen;
    const float refTotalW   = ref.headDia;

    constexpr int TOP_MARGIN = 38;
    constexpr int BOT_MARGIN = 62;
    const float scaleX = (m_drawW * 0.45f) / refTotalW;
    const float scaleY = static_cast<float>(m_drawH - TOP_MARGIN - BOT_MARGIN) / refTotalH;
    const float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    // Pixelmaße - immer auf Basis M12-Proportionen
    const int headW  = static_cast<int>(ref.headDia * scale + 0.5f);
    const int headH  = static_cast<int>(ref.headHeight * scale + 0.5f);
    const int shaftW = static_cast<int>(ref.nominal * scale + 0.5f);
    const int shaftH = static_cast<int>(refShaftLen * scale + 0.5f);
    const int hexW   = static_cast<int>(ref.keySizeInbus * scale + 0.5f);

    // Zentrierung (leicht nach links für Bemaßung rechts)
    const int centerX = ox + m_drawW / 2 - 12;
    const int topY = oy + TOP_MARGIN;

    // Kopf zeichnen
    const int hx = centerX - headW / 2;
    const int hy = topY;

    spr->fillRect(hx, hy, headW, headH, C_SCREW_HEAD);
    spr->drawRect(hx, hy, headW, headH, C_DIM_LINE);

    // Innensechskant-Vertiefung
    const int hexIndentH = headH / 3;
    const int hexIndentX = centerX - hexW / 2;
    if (hexW > 2 && hexIndentH > 1)
    {
        spr->fillRect(hexIndentX, hy, hexW, hexIndentH + 1, C_HEX_INDENT);
        spr->drawRect(hexIndentX, hy, hexW, hexIndentH + 1, C_DIM_LINE);
    }

    // Schaft mit Gewindeprofil
    const int sx = centerX - shaftW / 2;
    const int sy = hy + headH;

    // Gewindeparameter
    constexpr int TOOTH_DEPTH = 4;  ///< Zahntiefe links/rechts (Pixel)
    constexpr int PITCH_PX    = 10; ///< Visueller Gewindeabstand (Pixel)
    constexpr int CAP_H       = 3;  ///< Glatter Abschluss oben/unten (Pixel)

    const int coreLeft  = sx + TOOTH_DEPTH;
    const int coreRight = sx + shaftW - TOOTH_DEPTH;
    const int corePx    = shaftW - 2 * TOOTH_DEPTH;
    const float fsx     = static_cast<float>(sx);
    const float fsr     = static_cast<float>(sx + shaftW - 1);
    const float fl      = static_cast<float>(coreLeft);
    const float fr      = static_cast<float>(coreRight);

    // Kernbereich füllen (zwischen Gewindewurzeln)
    spr->fillRect(coreLeft, sy, corePx, shaftH, C_SCREW_BODY);

    // Glatter Abschluss oben/unten (volle Breite)
    spr->fillRect(sx, sy, shaftW, CAP_H, C_SCREW_BODY);
    spr->fillRect(sx, sy + shaftH - CAP_H, shaftW, CAP_H, C_SCREW_BODY);

    const int tStart = sy + CAP_H;
    const int tEnd   = sy + shaftH - CAP_H;

    // Erster Wurzelpunkt (nach dem Einlauf-Halbzahn)
    const int firstRoot = tStart + PITCH_PX / 2;

    // ===================================================================
    // Einlauf-Halbzahn oben: Schräge von Vollbreite -> Kernbreite
    // ===================================================================
    spr->fillTriangle(sx, tStart, coreLeft, firstRoot, coreLeft, tStart, C_SCREW_BODY);
    spr->fillTriangle(sx + shaftW - 1, tStart, coreRight, firstRoot, coreRight, tStart, C_SCREW_BODY);

    // ===================================================================
    // Volle Zähne (Wurzel -> Spitze -> Wurzel)
    // ===================================================================
    for (int y = firstRoot; y + PITCH_PX <= tEnd; y += PITCH_PX)
    {
        const int peakY = y + PITCH_PX / 2;

        spr->fillTriangle(coreLeft, y,
                          sx, peakY,
                          coreLeft, y + PITCH_PX, C_SCREW_BODY);

        spr->fillTriangle(coreRight, y,
                          sx + shaftW - 1, peakY,
                          coreRight, y + PITCH_PX, C_SCREW_BODY);

        // Schräge Gewindeflanke (AA-Linie, leichter Steigungswinkel)
        spr->drawWideLine(
            fsx + 2.0f,                   static_cast<float>(peakY + 1),
            static_cast<float>(sx + shaftW - 3), static_cast<float>(peakY - 1),
            0.7f, C_SCREW_THREAD);
    }

    // Letzten Wurzelpunkt finden
    int lastRoot = firstRoot;
    for (int y = firstRoot; y + PITCH_PX <= tEnd; y += PITCH_PX)
        lastRoot = y + PITCH_PX;

    // ===================================================================
    // Auslauf-Halbzahn unten: Wurzel -> Spitze (Kernbreite -> Vollbreite)
    // ===================================================================
    const int lastPeak = lastRoot + PITCH_PX / 2;

    // Halber Zahn: von letzter Wurzel auswärts zur Spitze
    spr->fillTriangle(coreLeft, lastRoot, sx, lastPeak, coreLeft, lastPeak, C_SCREW_BODY);
    spr->fillTriangle(coreRight, lastRoot, sx + shaftW - 1, lastPeak, coreRight, lastPeak, C_SCREW_BODY);

    // Restfläche zwischen letzter Spitze und Cap (falls vorhanden)
    if (lastPeak < tEnd)
        spr->fillRect(sx, lastPeak, shaftW, tEnd - lastPeak, C_SCREW_BODY);

    // ===================================================================
    // Kontur: Einlauf-Halbzahn (AA-Linien, diagonal)
    // ===================================================================
    spr->drawWideLine(fsx, static_cast<float>(tStart),
                      fl,  static_cast<float>(firstRoot),
                      0.5f, C_DIM_LINE);
    spr->drawWideLine(fsr, static_cast<float>(tStart),
                      fr,  static_cast<float>(firstRoot),
                      0.5f, C_DIM_LINE);

    // ===================================================================
    // Kontur: Volle Zähne (AA-Zickzack)
    // ===================================================================
    for (int y = firstRoot; y + PITCH_PX <= tEnd; y += PITCH_PX)
    {
        const int peakY = y + PITCH_PX / 2;

        // Linke Kontur: Wurzel -> Spitze -> Wurzel
        spr->drawWideLine(fl,  static_cast<float>(y),
                          fsx, static_cast<float>(peakY),
                          0.5f, C_DIM_LINE);
        spr->drawWideLine(fsx, static_cast<float>(peakY),
                          fl,  static_cast<float>(y + PITCH_PX),
                          0.5f, C_DIM_LINE);

        // Rechte Kontur
        spr->drawWideLine(fr,  static_cast<float>(y),
                          fsr, static_cast<float>(peakY),
                          0.5f, C_DIM_LINE);
        spr->drawWideLine(fsr, static_cast<float>(peakY),
                          fr,  static_cast<float>(y + PITCH_PX),
                          0.5f, C_DIM_LINE);
    }

    // ===================================================================
    // Kontur: Auslauf-Halbzahn (Wurzel -> Spitze, gleicher Winkel wie Zähne)
    // ===================================================================
    spr->drawWideLine(fl,  static_cast<float>(lastRoot),
                      fsx, static_cast<float>(lastPeak),
                      0.5f, C_DIM_LINE);
    spr->drawWideLine(fr,  static_cast<float>(lastRoot),
                      fsr, static_cast<float>(lastPeak),
                      0.5f, C_DIM_LINE);

    // ===================================================================
    // Abschluss-Konturen
    // ===================================================================

    // Oben: nur vertikale Cap-Kanten (keine HLine - Kopf-drawRect reicht)
    spr->drawFastVLine(sx, sy, CAP_H, C_DIM_LINE);
    spr->drawFastVLine(sx + shaftW - 1, sy, CAP_H, C_DIM_LINE);

    // Unten: durchgehende Vertikale vom Auslauf-Peak bis Schaftende
    const int bottomY = sy + shaftH - 1;
    if (lastPeak < bottomY)
    {
        spr->drawFastVLine(sx, lastPeak, bottomY - lastPeak, C_DIM_LINE);
        spr->drawFastVLine(sx + shaftW - 1, lastPeak, bottomY - lastPeak, C_DIM_LINE);
    }
    spr->drawFastHLine(sx, bottomY, shaftW, C_DIM_LINE);

    // Fase am Schaftende
    constexpr int CHAMFER = 3;
    spr->fillTriangle(sx, sy + shaftH,
                      sx + CHAMFER, sy + shaftH,
                      sx, sy + shaftH + CHAMFER, C_DRAW_BG);
    spr->fillTriangle(sx + shaftW - 1, sy + shaftH,
                      sx + shaftW - 1 - CHAMFER, sy + shaftH,
                      sx + shaftW - 1, sy + shaftH + CHAMFER, C_DRAW_BG);

    // Bemaßung (Werte der gewählten Größe, Zeichnung fix)
    spr->loadFont(ComfortaaBold14);

    // Kopfdurchmesser (dk) - über dem Kopf
    char dkBuf[16];
    stringutils::safeFormat(dkBuf, sizeof(dkBuf), "dk %.4g", s.headDia);
    dotToComma(dkBuf);
    drawDimH(hy - 14, hx, hx + headW - 1, hy - 18, hy, dkBuf, C_DIM_LINE, true);

    // Kopfhöhe (k) - vertikale Bemaßung rechts
    char kBuf[16];
    stringutils::safeFormat(kBuf, sizeof(kBuf), "k %.4g", s.headHeight);
    dotToComma(kBuf);
    const int dimKx = hx + headW + 18;
    drawDimV(dimKx, hy, hy + headH, kBuf, C_DIM_LINE);

    // SW Inbus - zentriert im Schraubenkopf
    char swBuf[16];
    I18n::format(swBuf, sizeof(swBuf),
                 WorkshopKey::WORKSHOP_SCREW_REF_FMT_DIM_SW, s.keySizeInbus);
    dotToComma(swBuf);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_DIM_TEXT, C_SCREW_HEAD);
    spr->drawString(swBuf, centerX, hy + headH / 2 + 2);

    // Nenndurchmesser (d) - unter dem Schaft
    char dBuf[16];
    stringutils::safeFormat(dBuf, sizeof(dBuf), "d %.4g", s.nominal);
    dotToComma(dBuf);
    const int dimNomY = sy + shaftH + 14;
    drawDimH(dimNomY, sx, sx + shaftW - 1, sy + shaftH, dimNomY + 4, dBuf, C_DIM_LINE, false);

    // Kernloch-Ø - darunter, ROT hervorgehoben
    const bool isFine = (m_threadMode == ThreadMode::FINE);
    const float coreVal = isFine ? s.coreHoleFine : s.coreHole;

    if (coreVal > 0.0f)
    {
        // Zeichnung fix M12: Kernloch-Breite immer auf Basis M12-Regelgewinde
        const float refCoreRatio = ref.coreHole / ref.nominal;
        const int coreW  = static_cast<int>(shaftW * refCoreRatio + 0.5f);
        const int coreX1 = centerX - coreW / 2;
        const int coreX2 = coreX1 + coreW;

        // Text zeigt den Wert der gewählten Größe
        char coreBuf[24];
        I18n::format(coreBuf, sizeof(coreBuf),
                     WorkshopKey::WORKSHOP_SCREW_REF_FMT_DIM_DRILL, coreVal);
        dotToComma(coreBuf);
        const int dimCoreY = dimNomY + 26;
        drawDimH(dimCoreY, coreX1, coreX2, dimNomY + 10, dimCoreY + 4, coreBuf, C_DIM_CORE, false);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet einen horizontalen Bemaßungspfeil.
 */
void WorkshopTool_ScrewRef::drawDimH(int y, int x1, int x2, int extTop, int extBot,
                                      const char *label, uint16_t color, bool above)
{
    Gfx_Sprite * const spr = m_sprite;

    // Verlängerungslinien
    spr->drawFastVLine(x1, extTop, extBot - extTop, color);
    spr->drawFastVLine(x2, extTop, extBot - extTop, color);

    // Horizontale Bemaßungslinie
    spr->drawFastHLine(x1, y, x2 - x1, color);

    // Pfeilspitzen (kleine Dreiecke nach innen)
    spr->fillTriangle(x1, y, x1 + 4, y - 2, x1 + 4, y + 2, color);
    spr->fillTriangle(x2, y, x2 - 4, y - 2, x2 - 4, y + 2, color);

    // Label
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(color, C_DRAW_BG);
    const int textY = above ? y - 10 : y + 15;
    spr->drawString(label, (x1 + x2) / 2, textY);
}

/**
 * @brief Zeichnet einen vertikalen Bemaßungspfeil.
 */
void WorkshopTool_ScrewRef::drawDimV(int x, int y1, int y2, const char *label, uint16_t color)
{
    Gfx_Sprite * const spr = m_sprite;

    // Vertikale Bemaßungslinie
    spr->drawFastVLine(x, y1, y2 - y1, color);

    // Pfeilspitzen
    spr->fillTriangle(x, y1, x - 2, y1 + 4, x + 2, y1 + 4, color);
    spr->fillTriangle(x, y2, x - 2, y2 - 4, x + 2, y2 - 4, color);

    // Label rechts neben der Linie, vertikal zentriert
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(color, C_DRAW_BG);
    spr->drawString(label, x + 4, (y1 + y2) / 2);
}

// ===================================================================
// Zeichnen - Datentabelle
// ===================================================================

/**
 * @brief Zeichnet die Datentabelle mit allen Schraubenkennwerten.
 */
void WorkshopTool_ScrewRef::drawData()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX + m_dataX;
    const int oy = m_originY + m_dataY;

    // Hintergrund
    spr->fillRect(ox, oy, m_dataW, m_drawH, C_BG);

    const ScrewData &s = SIZES[m_selectedIdx];
    const bool isFine = (m_threadMode == ThreadMode::FINE);
    const bool hasFine = (s.pitchFine > 0.0f);

    char buf[32];
    char lbl[40];

    // Titelzeile: Schraubenbezeichnung
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(0xFFFF, C_BG);

    char titleBuf[48];
    if (isFine && hasFine)
        I18n::format(titleBuf, sizeof(titleBuf),
                     WorkshopKey::WORKSHOP_SCREW_REF_FMT_TITLE_FINE,
                     s.name, s.pitchFine);
    else
        I18n::format(titleBuf, sizeof(titleBuf),
                     WorkshopKey::WORKSHOP_SCREW_REF_FMT_TITLE_COARSE,
                     s.name, s.pitch);
    dotToComma(titleBuf);
    spr->drawString(titleBuf, ox + 6, oy + 14);
    spr->unloadFont();

    // Zeile 0: Steigung
    if (isFine && !hasFine)
        I18n::format(buf, sizeof(buf), WorkshopKey::WORKSHOP_SCREW_REF_NO_FINE_THREAD);
    else if (isFine)
        stringutils::safeFormat(buf, sizeof(buf), "%.4g mm", s.pitchFine);
    else
        stringutils::safeFormat(buf, sizeof(buf), "%.4g mm", s.pitch);
    dotToComma(buf);
    I18n::format(lbl, sizeof(lbl), WorkshopKey::WORKSHOP_SCREW_REF_LABEL_PITCH);
    drawDataRow(0, lbl, buf, false);

    // Zeile 1: Kernloch-Ø (HERVORGEHOBEN)
    if (isFine && !hasFine)
        stringutils::safeCopy(buf, sizeof(buf), "---");
    else if (isFine)
        stringutils::safeFormat(buf, sizeof(buf), "%.4g mm", s.coreHoleFine);
    else
        stringutils::safeFormat(buf, sizeof(buf), "%.4g mm", s.coreHole);
    dotToComma(buf);
    I18n::format(lbl, sizeof(lbl), WorkshopKey::WORKSHOP_SCREW_REF_LABEL_TAPPING);
    drawDataRow(1, lbl, buf, !(isFine && !hasFine));

    // Zeile 2: Durchgangsbohrung
    stringutils::safeFormat(buf, sizeof(buf), "%.4g mm", s.throughHole);
    dotToComma(buf);
    I18n::format(lbl, sizeof(lbl), WorkshopKey::WORKSHOP_SCREW_REF_LABEL_CLEARANCE);
    drawDataRow(2, lbl, buf, false);

    // Zeile 3: SW Inbus (DIN 912)
    stringutils::safeFormat(buf, sizeof(buf), "%.4g mm", s.keySizeInbus);
    dotToComma(buf);
    I18n::format(lbl, sizeof(lbl), WorkshopKey::WORKSHOP_SCREW_REF_LABEL_AF_SOCKET);
    drawDataRow(3, lbl, buf, false);

    // Zeile 4: SW Sechskant (DIN 934)
    stringutils::safeFormat(buf, sizeof(buf), "%.4g mm", s.keySizeHex);
    dotToComma(buf);
    I18n::format(lbl, sizeof(lbl), WorkshopKey::WORKSHOP_SCREW_REF_LABEL_AF_HEX);
    drawDataRow(4, lbl, buf, false);

    // Zeile 5: Kopfhöhe
    stringutils::safeFormat(buf, sizeof(buf), "%.4g mm", s.headHeight);
    dotToComma(buf);
    I18n::format(lbl, sizeof(lbl), WorkshopKey::WORKSHOP_SCREW_REF_LABEL_HEAD_HEIGHT);
    drawDataRow(5, lbl, buf, false);

    // Zeile 6: Kopf-Ø
    stringutils::safeFormat(buf, sizeof(buf), "%.4g mm", s.headDia);
    dotToComma(buf);
    I18n::format(lbl, sizeof(lbl), WorkshopKey::WORKSHOP_SCREW_REF_LABEL_HEAD_DIA);
    drawDataRow(6, lbl, buf, false);

    // Zeile 7: Drehmoment
    if (s.torque88 < 1.0f)
        stringutils::safeFormat(buf, sizeof(buf), "%.1f Nm (8.8)", s.torque88);
    else
        stringutils::safeFormat(buf, sizeof(buf), "%.4g Nm (8.8)", s.torque88);
    dotToComma(buf);
    I18n::format(lbl, sizeof(lbl), WorkshopKey::WORKSHOP_SCREW_REF_LABEL_TORQUE);
    drawDataRow(7, lbl, buf, false);
}

/**
 * @brief Zeichnet eine einzelne Datenzeile.
 */
void WorkshopTool_ScrewRef::drawDataRow(int row, const char *label,
                                         const char *value, bool highlight)
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX + m_dataX;
    const int oy = m_originY + m_dataY + 34 + row * DATA_ROW_H;

    const uint16_t bg    = highlight ? C_ROW_HL_BG    : ((row & 1) ? C_ROW_BG_ALT : C_ROW_BG);
    const uint16_t lblC  = highlight ? C_ROW_HL_LABEL : C_ROW_LABEL;
    const uint16_t valC  = highlight ? C_ROW_HL_VALUE : C_ROW_VALUE;

    spr->fillRect(ox, oy, m_dataW, DATA_ROW_H, bg);

    spr->loadFont(ComfortaaBold14);

    // Label links
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblC, bg);
    spr->drawString(label, ox + 6, oy + DATA_ROW_H / 2);

    // Wert rechts
    spr->setTextDatum(MR_DATUM);
    spr->setTextColor(valC, bg);
    spr->drawString(value, ox + m_dataW - 6, oy + DATA_ROW_H / 2);

    spr->unloadFont();
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch für Selector-Chips.
 */
bool WorkshopTool_ScrewRef::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady) return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        const int hitChip = hitTestChip(cx, cy);
        if (hitChip >= 0 && hitChip != m_pressedChip)
        {
            m_pressedChip = hitChip;
            needsRedraw   = true;
        }

        m_touchWas = true;
    }
    else
    {
        m_touchReady = true;

        if (m_touchWas && m_pressedChip >= 0)
        {
            if (m_pressedChip != m_selectedIdx)
            {
                m_selectedIdx = m_pressedChip;
                needsRedraw = true;
            }
        }

        m_pressedChip = -1;
        m_touchWas    = false;
    }

    return needsRedraw;
}

/**
 * @brief Treffertest auf Selector-Chips.
 */
int WorkshopTool_ScrewRef::hitTestChip(int cx, int cy) const
{
    for (int i = 0; i < NUM_SIZES; ++i)
    {
        const Rect &r = m_chipRects[i];
        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h)
            return i;
    }
    return -1;
}
