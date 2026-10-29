#include "WorkshopTool_Resistor.h"

#include <cmath>
#include <cstring>

#include "StringUtils.h"
#include "I18n.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold16.h"

// ===================================================================
// Anonymer Namespace
// ===================================================================

namespace {

/**
 * @brief Ersetzt alle Dezimalpunkte durch Kommas im übergebenen Puffer.
 */
void dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.') *p = ',';
}

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

/** @brief RGB565-Darstellungsfarben der 12 normierten Widerstandsfarbcodefarben. */
const uint16_t WorkshopTool_Resistor::BAND_COLORS[NUM_COLORS] = {
    0x0000, // Schwarz (0)
    0x6180, // Braun   (1)
    0xA000, // Rot     (2)
    0xFC00, // Orange  (3)
    0xFFE0, // Gelb    (4)
    0x07E0, // Grün (5)
    0x001F, // Blau    (6)
    0x7817, // Violett (7)
    0x8410, // Grau    (8)
    0xFFFF, // Weiß    (9)
    0xFEA0, // Gold    (10)
    0xC618, // Silber  (11)
};

/** @brief Multiplikatoren gemäß IEC 60062: Index entspricht dem Farbindex. */
const double WorkshopTool_Resistor::MULT_VALUES[NUM_COLORS] = {
    1.0,     // Schwarz  ×1
    10.0,    // Braun    ×10
    100.0,   // Rot      ×100
    1.0e3,   // Orange   ×1 k
    1.0e4,   // Gelb     ×10 k
    1.0e5,   // Grün ×100 k
    1.0e6,   // Blau     ×1 M
    1.0e7,   // Violett  ×10 M
    1.0e8,   // Grau     ×100 M
    1.0e9,   // Weiß     ×1 G
    0.1,     // Gold     ×0,1
    0.01,    // Silber   ×0,01
};

/** @brief Toleranzstrings gemäß IEC 60062; leerer String wenn keine Norm. */
const char * const WorkshopTool_Resistor::TOL_STRINGS[NUM_COLORS] = {
    "",           // Schwarz  (kein Normwert)
    "+/-1%",      // Braun
    "+/-2%",      // Rot
    "+/-0.05%",   // Orange   (E96-Serie)
    "+/-0.02%",   // Gelb
    "+/-0.5%",    // Grün
    "+/-0.25%",   // Blau
    "+/-0.1%",    // Violett
    "+/-0.01%",   // Grau
    "",           // Weiß     (kein Normwert)
    "+/-5%",      // Gold
    "+/-10%",     // Silber
};

/** @brief Bandbeschriftungen im 4-Ring-Modus. */
const char * const WorkshopTool_Resistor::BAND_LABELS_4[4] = {
    "B1", "B2", "x", "Tol"
};

/** @brief Bandbeschriftungen im 5-Ring-Modus. */
const char * const WorkshopTool_Resistor::BAND_LABELS_5[5] = {
    "B1", "B2", "B3", "x", "Tol"
};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Konfiguriert Standardwerte (4,7 kOhm / 4-Ring).
 */
WorkshopTool_Resistor::WorkshopTool_Resistor()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_Resistor::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_Resistor::clearAll()
{
    memset(m_bands,      0, sizeof(m_bands));
    memset(m_valueStr,   0, sizeof(m_valueStr));
    memset(m_tolStr,     0, sizeof(m_tolStr));
    memset(m_colorRects, 0, sizeof(m_colorRects));
    memset(&m_tog4Rect,  0, sizeof(m_tog4Rect));
    memset(&m_tog5Rect,  0, sizeof(m_tog5Rect));

    m_sprite      = nullptr;
    m_is5Band     = false;
    m_bandCount   = 4;
    m_bandH       = 0;
    m_quadSize    = 0;
    m_valueDirty  = true;
    m_layoutValid = false;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;

    m_pressedBand  = -1;
    m_pressedColor = -1;
    m_pressedTog   =  0;
    m_touchWas     = false;
    m_touchReady   = false;

    // Standardwert: 4R - Gelb(4) / Violett(7) / Rot(2 = ×100) / Gold(10 = ±5%) = 4700 Ohm
    m_bands[0] = 4;   // Gelb     -> Stelle 10er
    m_bands[1] = 7;   // Violett  -> Stelle 1er
    m_bands[2] = 2;   // Rot      -> ×100
    m_bands[3] = 10;  // Gold     -> ±5%
    m_bands[4] = 10;  // Vorauswahl Toleranz für späteres Wechseln in 5R-Modus
}

// ===================================================================
// Öffentliche Interface-Methoden
// ===================================================================

/**
 * @brief Zeichnet den gesamten Tool-Bereich vollständig neu.
 */
void WorkshopTool_Resistor::draw(Gfx_Sprite *sprite,
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

    if (m_valueDirty)
    {
        computeValue();
        m_valueDirty = false;
    }

    drawResistorBody();
    drawValueRow();

    for (int i = 0; i < m_bandCount; ++i)
        drawBandRow(i);
}

/**
 * @brief Verarbeitet Touch-Events mit Touch-up-Semantik.
 */
bool WorkshopTool_Resistor::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        // Phantom-Touch beim Booten: erst nach einem sauberen touched=false-Frame erlaubt
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        // Toggle-Buttons prüfen
        bool out5 = false;
        if (hitTestToggle(cx, cy, out5))
        {
            const int newTog = out5 ? 5 : 4;
            if (newTog != m_pressedTog)
            {
                m_pressedTog   = newTog;
                m_pressedBand  = -1;
                m_pressedColor = -1;
                needsRedraw    = true;
            }
        }
        else
        {
            // Farbfelder je Band prüfen
            int hitBand  = -1;
            int hitColor = -1;
            for (int b = 0; b < m_bandCount && hitBand < 0; ++b)
            {
                const int c = hitTestColorField(cx, cy, b);
                if (c >= 0) { hitBand = b; hitColor = c; }
            }

            if (hitBand != m_pressedBand || hitColor != m_pressedColor)
            {
                m_pressedBand  = hitBand;
                m_pressedColor = hitColor;
                m_pressedTog   = 0;
                if (hitBand >= 0) needsRedraw = true;
            }
        }

        m_touchWas = true;
    }
    else
    {
        // Erstes touched=false-Frame: ab jetzt dürfen Touch-ups Aktionen auslösen
        m_touchReady = true;

        if (m_touchWas)
        {
            if (m_pressedTog == 4 && m_is5Band)
            {
                // Wechsel 5R -> 4R: Multiplikator und Toleranz aus Bändern 3/4 übernehmen
                m_bands[2]    = m_bands[3]; // 5R-Mult -> 4R-Mult
                m_bands[3]    = m_bands[4]; // 5R-Tol  -> 4R-Tol
                m_is5Band     = false;
                m_bandCount   = 4;
                m_layoutValid = false;
                m_valueDirty  = true;
                needsRedraw   = true;
            }
            else if (m_pressedTog == 5 && !m_is5Band)
            {
                // Wechsel 4R -> 5R: neues Digit-Band 3 einfügen (Schwarz = 0), bisheriges Mult-Band 2 rückt auf Position 3, Toleranz auf Position 4
                m_bands[4]    = m_bands[3]; // 4R-Tol  -> 5R-Tol
                m_bands[3]    = m_bands[2]; // 4R-Mult -> 5R-Mult
                m_bands[2]    = 0;          // neues Digit-Band = Schwarz
                m_is5Band     = true;
                m_bandCount   = 5;
                m_layoutValid = false;
                m_valueDirty  = true;
                needsRedraw   = true;
            }
            else if (m_pressedBand >= 0 && m_pressedColor >= 0)
            {
                m_bands[m_pressedBand] = static_cast<uint8_t>(m_pressedColor);
                m_valueDirty = true;
                needsRedraw  = true;
            }
        }

        m_pressedBand  = -1;
        m_pressedColor = -1;
        m_pressedTog   =  0;
        m_touchWas     = false;
    }

    return needsRedraw;
}

// ===================================================================
// Layout-Berechnung
// ===================================================================

/**
 * @brief Berechnet Farbfeld- und Toggle-Rechtecke (content-relativ).
 */
void WorkshopTool_Resistor::computeLayout(int w, int h)
{
    m_bandCount = m_is5Band ? 5 : 4;

    // Höhe der Selektor-Zone unterhalb von Körper- und Wertbereich
    const int selectorH = h - RESIST_H - VALUE_H;
    m_bandH = (m_bandCount > 0) ? (selectorH / m_bandCount) : 52;
    if (m_bandH < 12) m_bandH = 12;

    // Quadratgröße: gleichbreite Felder für 12 Farben mit QUAD_GAP Abstand. Der Auswahlrahmen (SEL_MARGIN) passt rechts ins Bild, weil LABEL_W kleiner ist als die tatsächliche Beschriftungsbreite.
    const int availW   = w - LABEL_W - (NUM_COLORS - 1) * QUAD_GAP;
    const int fieldW   = (NUM_COLORS > 0) ? (availW / NUM_COLORS) : 38;
    m_quadSize = fieldW;
    if (m_quadSize > m_bandH - 6) m_quadSize = m_bandH - 6;
    if (m_quadSize < 8)           m_quadSize = 8;

    // Farbfeld-Rects: content-relativ (Ursprung = originX/originY)
    for (int b = 0; b < MAX_BANDS; ++b)
    {
        const int rowRelY = RESIST_H + VALUE_H + b * m_bandH;
        const int qOfsY   = (m_bandH - m_quadSize) / 2;

        for (int c = 0; c < NUM_COLORS; ++c)
        {
            const int qx              = LABEL_W + c * (m_quadSize + QUAD_GAP);
            const int qy              = rowRelY + qOfsY;
            m_colorRects[b][c]        = {qx, qy, m_quadSize, m_quadSize};
        }
    }

    // Toggle-Rects: content-relativ, rechtsbündig in der Wertzeile
    const int togY    = RESIST_H + (VALUE_H - TOG_H) / 2;
    m_tog5Rect        = {w - TOG_W,               togY, TOG_W, TOG_H};
    m_tog4Rect        = {w - TOG_W * 2 - TOG_GAP, togY, TOG_W, TOG_H};
}

// ===================================================================
// Wertberechnung
// ===================================================================

/**
 * @brief Berechnet Widerstandswert und Toleranz aus den aktuellen Bandindizes.
 */
void WorkshopTool_Resistor::computeValue()
{
    double base    = 0.0;
    int    multIdx = 0;
    int    tolIdx  = 0;

    if (m_is5Band)
    {
        base    = static_cast<double>(m_bands[0]) * 100.0
                + static_cast<double>(m_bands[1]) *  10.0
                + static_cast<double>(m_bands[2]);
        multIdx = m_bands[3];
        tolIdx  = m_bands[4];
    }
    else
    {
        base    = static_cast<double>(m_bands[0]) * 10.0
                + static_cast<double>(m_bands[1]);
        multIdx = m_bands[2];
        tolIdx  = m_bands[3];
    }

    const int    safeMultIdx = (multIdx < NUM_COLORS) ? multIdx : 0;
    const int    safeTolIdx  = (tolIdx  < NUM_COLORS) ? tolIdx  : 0;
    const double ohm         = base * MULT_VALUES[safeMultIdx];

    formatResistance(ohm, m_valueStr, sizeof(m_valueStr));

    const char *tol = TOL_STRINGS[safeTolIdx];
    stringutils::safeCopy(m_tolStr, sizeof(m_tolStr), tol[0] ? tol : "?");
    dotToComma(m_tolStr);
}

// ===================================================================
// Zeichenmethoden
// ===================================================================

/**
 * @brief Zeichnet Widerstandskörper mit Leitungsdrähten und Farbringen.
 */
void WorkshopTool_Resistor::drawResistorBody()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;
    const int w  = m_w;

    // Bereichshintergrund löschen
    spr->fillRect(ox, oy, w, RESIST_H, C_BG);

    // Körper-Koordinaten (sprite-relativ)
    const int bodyX = ox + (w - BODY_W) / 2;
    const int bodyY = oy + (RESIST_H - BODY_H) / 2;
    const int cy    = bodyY + BODY_H / 2;

    // Leitungsdicke: Capsule-Radius = Hälfte der Dicke
    constexpr int LEAD_T = 10;

    // Anschlussleitungen (Schatten + Vorderseite)
    const int leftLeadX  = ox;
    const int leftLeadW  = bodyX - ox + 6;
    const int rightLeadX = bodyX + BODY_W - 6;
    const int rightLeadW = ox + w - rightLeadX;

    drawCapsule(spr, leftLeadX,  cy - LEAD_T / 2,     leftLeadW,  LEAD_T,     C_LEAD_SHADOW);
    drawCapsule(spr, rightLeadX, cy - LEAD_T / 2,     rightLeadW, LEAD_T,     C_LEAD_SHADOW);
    drawCapsule(spr, leftLeadX,  cy - LEAD_T / 2 - 1, leftLeadW,  LEAD_T - 2, C_LEAD);
    drawCapsule(spr, rightLeadX, cy - LEAD_T / 2 - 1, rightLeadW, LEAD_T - 2, C_LEAD);

    // Körper: drei geschachtelte Capsules für Plastik-Tiefenwirkung
    drawCapsule(spr, bodyX,     bodyY,     BODY_W,      BODY_H,      C_BODY_OUTER);
    drawCapsule(spr, bodyX + 3, bodyY + 2, BODY_W - 6,  BODY_H - 4,  C_BODY_MID);
    drawCapsule(spr, bodyX + 6, bodyY + 5, BODY_W - 12, BODY_H - 10, C_BODY_INNER);

    // Glanz- und Schattenstreifen VOR den Ringen So liegen die Ringe optisch korrekt darüber und der Schimmer wirkt als globale Beleuchtung des Körpers, nicht als Überlagerung der Ringe.
    {
        const int gx = bodyX + BODY_W / 12;
        const int gy = bodyY + BODY_H / 10;
        const int gw = BODY_W - (BODY_W / 6);
        const int gh = std::max(8, BODY_H / 5);
        drawCapsule(spr, gx, gy, gw, gh,
                    blendColor(0xFFFF, C_BODY_INNER, 0.31f));
    }
    {
        const int sx = bodyX + BODY_W / 10;
        const int sy = bodyY + BODY_H - std::max(10, BODY_H / 6) - 6;
        const int sw = BODY_W - (BODY_W / 5);
        const int sh = std::max(8, BODY_H / 8);
        drawCapsule(spr, sx, sy, sw, sh,
                    blendColor(C_SHAD_BLEND, C_BODY_OUTER, 0.31f));
    }

    // Farbringe (liegen über der Körperschattierung)
    constexpr int bandY = 2;
    constexpr int bandH = BODY_H - 4;

    if (m_bandCount == 4)
    {
        constexpr float pos[4]   = { 0.18f, 0.34f, 0.50f, 0.78f };
        constexpr float width[4] = { 0.075f, 0.075f, 0.075f, 0.055f };
        for (int i = 0; i < 4; ++i)
        {
            const int bw = static_cast<int>(BODY_W * width[i]);
            const int bx = bodyX + static_cast<int>(BODY_W * pos[i]) - bw / 2;
            const int ci = (m_bands[i] < NUM_COLORS) ? m_bands[i] : 0;
            drawSingleBand(spr, bx, bodyY + bandY, bw, bandH, BAND_COLORS[ci]);
        }
    }
    else
    {
        constexpr float pos[5]   = { 0.14f, 0.27f, 0.40f, 0.60f, 0.79f };
        constexpr float width[5] = { 0.065f, 0.065f, 0.065f, 0.065f, 0.050f };
        for (int i = 0; i < 5; ++i)
        {
            const int bw = static_cast<int>(BODY_W * width[i]);
            const int bx = bodyX + static_cast<int>(BODY_W * pos[i]) - bw / 2;
            const int ci = (m_bands[i] < NUM_COLORS) ? m_bands[i] : 0;
            drawSingleBand(spr, bx, bodyY + bandY, bw, bandH, BAND_COLORS[ci]);
        }
    }
}

/**
 * @brief Zeichnet die Wertzeile mit Ohm-Wert, Toleranz und Toggle-Buttons.
 */
void WorkshopTool_Resistor::drawValueRow()
{
    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + RESIST_H;

    spr->fillRect(ox, oy, m_w, VALUE_H, C_BG);

    spr->loadFont(ComfortaaBold16);

    // Widerstandswert (linksbündig)
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_TEXT_VAL, C_BG);
    spr->drawString(m_valueStr, ox + 6, oy + VALUE_H / 2);

    // Toleranz mit Abstand rechts vom Wert
    const int valW = spr->textWidth(m_valueStr);
    spr->setTextColor(C_TEXT_DIM, C_BG);
    spr->drawString(m_tolStr, ox + 6 + valW + 14, oy + VALUE_H / 2);

    // Toggle-Button "4R"
    {
        const Rect &r  = m_tog4Rect;
        const int   rx = m_originX + r.x;
        const int   ry = m_originY + r.y;
        const uint16_t fill = (m_pressedTog == 4)
                              ? C_TOG_PRESS
                              : (!m_is5Band ? C_TOG_ON : C_TOG_OFF);
        spr->fillSmoothRoundRect(rx, ry, r.w, r.h, 4, fill);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_TOG_TEXT, fill);
        spr->drawString("4R", rx + r.w / 2, ry + r.h / 2);
    }

    // Toggle-Button "5R"
    {
        const Rect &r  = m_tog5Rect;
        const int   rx = m_originX + r.x;
        const int   ry = m_originY + r.y;
        const uint16_t fill = (m_pressedTog == 5)
                              ? C_TOG_PRESS
                              : (m_is5Band ? C_TOG_ON : C_TOG_OFF);
        spr->fillSmoothRoundRect(rx, ry, r.w, r.h, 4, fill);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_TOG_TEXT, fill);
        spr->drawString("5R", rx + r.w / 2, ry + r.h / 2);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet eine Farbband-Auswahlzeile mit Beschriftung und 12 Farbfeldern.
 */
void WorkshopTool_Resistor::drawBandRow(int bandIdx)
{
    if (bandIdx < 0 || bandIdx >= m_bandCount)
        return;

    Gfx_Sprite * const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;

    const int rowAbsY = oy + RESIST_H + VALUE_H + bandIdx * m_bandH;

    // Zeilenhintergrund (alternierend)
    const uint16_t rowBg = (bandIdx % 2 == 0) ? C_ROW_EVEN : C_ROW_ODD;
    spr->fillRect(ox, rowAbsY, m_w, m_bandH, rowBg);

    // Bandbeschriftung (linksbündig, vertikal zentriert)
    const char *lbl = m_is5Band ? BAND_LABELS_5[bandIdx] : BAND_LABELS_4[bandIdx];
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_TEXT_DIM, rowBg);
    spr->drawString(lbl, ox + 4, rowAbsY + m_bandH / 2);
    spr->unloadFont();

    // Farbfelder zeichnen
    const bool digit    = isDigitBand(bandIdx, m_is5Band);
    const int  selC     = m_bands[bandIdx];
    const int  pressedC = (m_pressedBand == bandIdx) ? m_pressedColor : -1;

    for (int c = 0; c < NUM_COLORS; ++c)
    {
        const Rect    &r   = m_colorRects[bandIdx][c];
        const int      ax  = ox + r.x;
        const int      ay  = oy + r.y;
        const bool     dis = (digit && c >= 10); // Gold/Silber bei Digit-Band deaktiviert

        // Rahmenfarbe: ausgewähltes Feld = C_SEL_FRAME, sonst Zeilenhintergrund (unsichtbar). Der äußere RoundRect verschwindet optisch wenn er in rowBg gezeichnet wird - damit entfällt die Notwendigkeit eines drawRoundRect() vollständig.
        const uint16_t frameColor = (!dis && c == selC) ? C_SEL_FRAME : rowBg;
        spr->fillSmoothRoundRect(ax - SEL_MARGIN, ay - SEL_MARGIN,
                                  r.w + SEL_MARGIN * 2, r.h + SEL_MARGIN * 2,
                                  SEL_RADIUS, frameColor);

        // Inneres Feld: deaktiviert, gedrückt oder normale Bandfarbe
        const uint16_t fillColor = dis             ? C_DISABLED
                                 : (c == pressedC) ? C_PRESS_FILL
                                                   : BAND_COLORS[c];
        spr->fillSmoothRoundRect(ax, ay, r.w, r.h, QUAD_RADIUS, fillColor);
    }
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf die Toggle-Buttons (content-relativ).
 */
bool WorkshopTool_Resistor::hitTestToggle(int cx, int cy, bool &out5) const
{
    auto inRect = [](int x, int y, const Rect &r) -> bool
    {
        return x >= r.x && x < r.x + r.w
            && y >= r.y && y < r.y + r.h;
    };

    if (inRect(cx, cy, m_tog4Rect)) { out5 = false; return true; }
    if (inRect(cx, cy, m_tog5Rect)) { out5 = true;  return true; }
    return false;
}

/**
 * @brief Treffertest auf Farbfelder einer Bandzeile (content-relativ).
 */
int WorkshopTool_Resistor::hitTestColorField(int cx, int cy, int bandIdx) const
{
    if (bandIdx < 0 || bandIdx >= m_bandCount)
        return -1;

    const bool digit = isDigitBand(bandIdx, m_is5Band);

    for (int c = 0; c < NUM_COLORS; ++c)
    {
        // Gold/Silber sind bei Digit-Bändern nicht anklickbar
        if (digit && c >= 10)
            continue;

        const Rect &r = m_colorRects[bandIdx][c];
        if (cx >= r.x && cx < r.x + r.w &&
            cy >= r.y && cy < r.y + r.h)
        {
            return c;
        }
    }
    return -1;
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Gibt zurück ob ein Band ausschließlich Ziffern 0-9 trägt.
 */
bool WorkshopTool_Resistor::isDigitBand(int bandIdx, bool is5band)
{
    return is5band ? (bandIdx <= 2) : (bandIdx <= 1);
}

/**
 * @brief Formatiert einen Ohm-Wert als lesbaren String mit 3 signifikanten Stellen.
 */
void WorkshopTool_Resistor::formatResistance(double ohm, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (ohm < 0.0 || !std::isfinite(ohm))
    {
        stringutils::safeCopy(buf, sz, "? Ohm");
        return;
    }

    double      val;
    const char *unit;

    if (ohm < 1000.0)        { val = ohm;       unit = "Ohm";  }
    else if (ohm < 1.0e6)    { val = ohm/1.0e3; unit = "kOhm"; }
    else if (ohm < 1.0e9)    { val = ohm/1.0e6; unit = "MOhm"; }
    else                     { val = ohm/1.0e9; unit = "GOhm"; }

    // %.3g: 3 signifikante Stellen, trailing zeros werden von %g automatisch entfernt
    stringutils::safeFormat(buf, sz, "%.3g %s", val, unit);
    dotToComma(buf);
}

/**
 * @brief Zeichnet ein Capsule-Rechteck (fillSmoothRoundRect mit r = h/2).
 */
void WorkshopTool_Resistor::drawCapsule(Gfx_Sprite *spr,
                                         int x, int y, int w, int h,
                                         uint16_t color)
{
    if (w <= 0 || h <= 0)
        return;
    const int r = std::max(1, h / 2);
    spr->fillSmoothRoundRect(x, y, w, h, r, color);
}

/**
 * @brief Zeichnet einen einzelnen Farbring mit heller/dunkler 3D-Kante.
 */
void WorkshopTool_Resistor::drawSingleBand(Gfx_Sprite *spr,
                                            int x, int y, int w, int h,
                                            uint16_t color)
{
    if (w <= 0 || h <= 0)
        return;
    const int r = (w >= 10) ? 5 : 3;
    spr->fillSmoothRoundRect(x, y, w, h, r, color);

    // Heller Streifen links (simuliert Lichteinfall von links-oben)
    spr->fillRect(x + 1, y + 4, 2, h - 8, blendColor(0xFFFF, color, 0.27f));
    // Dunkler Streifen rechts (Schattenkante)
    spr->fillRect(x + w - 3, y + 4, 2, h - 8, scaleColor(color, 0.73f));
}
