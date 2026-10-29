#include "WorkshopTool_StitchGauge.h"

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
// Anonymer Namespace - file-local Helfer und Layout-Konstanten
// ===================================================================

namespace
{

    // Spaltenaufteilung (Split-Layout)
    constexpr int SCHEM_W = 250; ///< Breite der linken Spalte (Visualisierung)
    constexpr int COL_GAP = 12;  ///< Abstand zwischen den Spalten

    // Visualisierung - Grenzen der Strickstück-Box (innerhalb der linken Spalte)
    constexpr int VIS_MARGIN_X = 35; ///< Horizontaler Polster für Beschriftung
    constexpr int VIS_MARGIN_Y = 35; ///< Vertikaler Polster für Beschriftung
    constexpr int VIS_LABEL_GAP = 8; ///< Abstand Box-Kante zur Beschriftung

    // V-Muster im Inneren
    constexpr int V_STEP_X = 9; ///< Horizontaler Rasterabstand der V-Andeutungen
    constexpr int V_STEP_Y = 7; ///< Vertikaler Rasterabstand
    constexpr int V_SIZE  = 2; ///< Halbe Breite/Höhe einer V-Andeutung

    /**
     * @brief Ersetzt den ersten Dezimalpunkt im Puffer durch ein Komma.
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

const WorkshopKey WorkshopTool_StitchGauge::QTY_SYMBOL_KEYS[NUM_QTY] = {
    WorkshopKey::WORKSHOP_STITCH_GAUGE_LABEL_STITCHES_PER_10CM,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_LABEL_WIDTH,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_LABEL_STITCHES,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_LABEL_ROWS_PER_10CM,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_LABEL_HEIGHT,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_LABEL_ROWS
};

const WorkshopKey WorkshopTool_StitchGauge::QTY_UNIT_KEYS[NUM_QTY] = {
    WorkshopKey::WORKSHOP_STITCH_GAUGE_UNIT_STITCHES,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_UNIT_CM,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_UNIT_STITCHES,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_UNIT_ROWS,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_UNIT_CM,
    WorkshopKey::WORKSHOP_STITCH_GAUGE_UNIT_ROWS
};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_StitchGauge::WorkshopTool_StitchGauge()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Moduleinstieg zurück.
 */
void WorkshopTool_StitchGauge::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_StitchGauge::clearAll()
{
    memset(m_inputBuf, 0, sizeof(m_inputBuf));
    memset(m_values, 0, sizeof(m_values));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));
    memset(m_computeOk, 0, sizeof(m_computeOk));

    m_sprite      = nullptr;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;
    m_layoutValid = false;
    m_numpad.clearLayout();

    m_pressedField = -1;
    m_pressedKey   = -1;
    m_touchWas     = false;
    m_touchReady   = false;

    // Maschen-Achse: M_10 (Slot 0), Breite (Slot 1), Maschen berechnet
    m_inputSlotsM[0] = 0; // M_10 ist der älteste Eingabe-Slot
    m_inputSlotsM[1] = 1; // Breite ist der neuere Eingabe-Slot
    m_computedM      = 2; // Maschen wird berechnet

    // Reihen-Achse: R_10 (Slot 0), Höhe (Slot 1), Reihen berechnet
    m_inputSlotsR[0] = 3;
    m_inputSlotsR[1] = 4;
    m_computedR      = 5;

    // Fokus liegt zu Beginn auf dem Probefeld der Maschen-Achse
    m_focusedQty = 0;

    m_values[0] = 22.0; // M_10
    m_values[1] = 50.0; // Breite cm
    m_values[3] = 30.0; // R_10
    m_values[4] = 60.0; // Höhe cm

    formatEditValue(m_values[0], m_inputBuf[0], FIELD_CAP);
    formatEditValue(m_values[1], m_inputBuf[1], FIELD_CAP);
    formatEditValue(m_values[3], m_inputBuf[3], FIELD_CAP);
    formatEditValue(m_values[4], m_inputBuf[4], FIELD_CAP);

    compute();
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Visualisierung, Eingabefelder und Numpad.
 */
void WorkshopTool_StitchGauge::draw(Gfx_Sprite *sprite, int originX, int originY,
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

    drawVisualization();
    drawFields(cursorOn);
    m_numpad.draw(m_sprite, m_originX + m_rightX, m_originY, m_pressedKey);
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Spaltenaufteilung, Feld-Rects und Numpad-Layout.
 */
void WorkshopTool_StitchGauge::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;

    if (m_rightW < 100)
        m_rightW = 100;

    // Eingabefelder: 3 Maschen-Achse, SECTION_GAP, 3 Reihen-Achse
    int rowY = TOP_GAP;
    for (int i = 0; i < NUM_QTY; ++i)
    {
        if (i == 3)
            rowY += SECTION_GAP;

        m_fieldRects[i] = {m_rightX, rowY, m_rightW, ROW_H};
        rowY += ROW_H;
    }

    // Numpad bottom-anchored
    m_numpad.computeLayout(m_rightW, h - WorkshopNumpad::REQUIRED_H);
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch mit FIFO-2-Logik je Achse.
 */
bool WorkshopTool_StitchGauge::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        if (cx >= m_rightX)
        {
            // Eingabefelder
            const int hitField = hitTestField(cx, cy);
            if (hitField >= 0)
            {
                if (hitField != m_pressedField)
                {
                    m_pressedField = hitField;
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
            if (m_pressedField >= 0 && m_pressedField < NUM_QTY)
            {
                onTapField(m_pressedField);
                needsRedraw = true;
            }
            else if (m_pressedKey >= 0 && m_pressedKey < WorkshopNumpad::NUM_KEYS)
            {
                executeKey(WorkshopNumpad::keyAction(m_pressedKey));
                needsRedraw = true;
            }
        }

        m_pressedField = -1;
        m_pressedKey   = -1;
        m_touchWas     = false;
    }

    return needsRedraw;
}

/**
 * @brief Löst die FIFO-2-Logik einer Sektion bei Tap auf eine Größe aus.
 */
void WorkshopTool_StitchGauge::onTapField(int qty)
{
    if (isInput(qty))
    {
        m_focusedQty = static_cast<uint8_t>(qty);
        return;
    }

    // qty ist die berechnete Größe seiner Sektion: FIFO-2 anwenden
    const int sec = sectionOf(qty);
    uint8_t *slots = (sec == 0) ? m_inputSlotsM : m_inputSlotsR;
    uint8_t &computed = (sec == 0) ? m_computedM : m_computedR;

    const uint8_t oldest = slots[0];
    slots[0]   = slots[1];
    slots[1]   = static_cast<uint8_t>(qty);
    computed   = oldest;

    m_focusedQty = static_cast<uint8_t>(qty);

    if (m_computeOk[sec])
        formatEditValue(m_values[qty], m_inputBuf[qty], FIELD_CAP);
    else
        m_inputBuf[qty][0] = '\0';

    compute();
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Parst Eingabepuffer und berechnet beide Achsen unabhängig.
 */
void WorkshopTool_StitchGauge::compute()
{
    // Eingabepuffer in numerische Werte übernehmen
    for (int s = 0; s < SLOTS_PER_SECTION; ++s)
    {
        const int qm = static_cast<int>(m_inputSlotsM[s]);
        const int qr = static_cast<int>(m_inputSlotsR[s]);
        m_values[qm] = (m_inputBuf[qm][0] != '\0') ? atof(m_inputBuf[qm]) : 0.0;
        m_values[qr] = (m_inputBuf[qr][0] != '\0') ? atof(m_inputBuf[qr]) : 0.0;
    }

    // Maschen-Achse
    m_computeOk[0] = false;
    {
        const double m10    = m_values[0];
        const double width  = m_values[1];
        const double stitch = m_values[2];

        bool ok = true;
        for (int s = 0; s < SLOTS_PER_SECTION && ok; ++s)
        {
            if (!std::isfinite(m_values[static_cast<int>(m_inputSlotsM[s])]))
                ok = false;
        }

        if (ok)
        {
            switch (m_computedM)
            {
            case 2: // Maschen = M_10 * Breite / 10
            {
                const double res = m10 * width / PROBE_CM;
                if (std::isfinite(res) && res >= 0.0)
                {
                    m_values[2] = res;
                    m_computeOk[0] = true;
                }
                break;
            }
            case 1: // Breite = Maschen * 10 / M_10
            {
                if (m10 > 1e-9)
                {
                    const double res = stitch * PROBE_CM / m10;
                    if (std::isfinite(res) && res >= 0.0)
                    {
                        m_values[1] = res;
                        m_computeOk[0] = true;
                    }
                }
                break;
            }
            case 0: // M_10 = Maschen * 10 / Breite
            {
                if (width > 1e-9)
                {
                    const double res = stitch * PROBE_CM / width;
                    if (std::isfinite(res) && res >= 0.0)
                    {
                        m_values[0] = res;
                        m_computeOk[0] = true;
                    }
                }
                break;
            }
            default:
                break;
            }
        }
    }

    // Reihen-Achse
    m_computeOk[1] = false;
    {
        const double r10    = m_values[3];
        const double height = m_values[4];
        const double rows   = m_values[5];

        bool ok = true;
        for (int s = 0; s < SLOTS_PER_SECTION && ok; ++s)
        {
            if (!std::isfinite(m_values[static_cast<int>(m_inputSlotsR[s])]))
                ok = false;
        }

        if (ok)
        {
            switch (m_computedR)
            {
            case 5: // Reihen = R_10 * Höhe / 10
            {
                const double res = r10 * height / PROBE_CM;
                if (std::isfinite(res) && res >= 0.0)
                {
                    m_values[5] = res;
                    m_computeOk[1] = true;
                }
                break;
            }
            case 4: // Höhe = Reihen * 10 / R_10
            {
                if (r10 > 1e-9)
                {
                    const double res = rows * PROBE_CM / r10;
                    if (std::isfinite(res) && res >= 0.0)
                    {
                        m_values[4] = res;
                        m_computeOk[1] = true;
                    }
                }
                break;
            }
            case 3: // R_10 = Reihen * 10 / Höhe
            {
                if (height > 1e-9)
                {
                    const double res = rows * PROBE_CM / height;
                    if (std::isfinite(res) && res >= 0.0)
                    {
                        m_values[3] = res;
                        m_computeOk[1] = true;
                    }
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Liefert den Sektions-Index einer Größe.
 */
int WorkshopTool_StitchGauge::sectionOf(int qty)
{
    return (qty < 3) ? 0 : 1;
}

/**
 * @brief Prüft, ob eine Größe derzeit Eingabe ist.
 */
bool WorkshopTool_StitchGauge::isInput(int qty) const
{
    const uint8_t computed = (sectionOf(qty) == 0) ? m_computedM : m_computedR;
    return (static_cast<int>(computed) != qty);
}

/**
 * @brief Liefert die Akzentfarbe der Achse, zu der die Größe gehört.
 */
uint16_t WorkshopTool_StitchGauge::accentColor(int qty)
{
    return (sectionOf(qty) == 0) ? C_ACCENT_M_AXIS : C_ACCENT_R_AXIS;
}

// ===================================================================
// Formatierung
// ===================================================================

/**
 * @brief Formatiert einen Wert mit Komma-Dezimalseparator für die Anzeige.
 */
void WorkshopTool_StitchGauge::formatDisplayValue(double val, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    if (val == 0.0)
    {
        stringutils::safeCopy(buf, sz, "0");
        return;
    }

    stringutils::safeFormat(buf, sz, "%.4g", val);
    dotToComma(buf);
}

/**
 * @brief Formatiert einen Wert mit Punkt-Dezimalseparator für den Edit-Puffer.
 */
void WorkshopTool_StitchGauge::formatEditValue(double val, char *buf, size_t sz)
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
 * @brief Formatiert einen Wert kompakt für die Visualisierungs-Beschriftung.
 */
void WorkshopTool_StitchGauge::formatVisLabel(int qty, char *buf, size_t sz) const
{
    if (!buf || sz == 0)
        return;

    const int sec = sectionOf(qty);
    if (!m_computeOk[sec] && !isInput(qty))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    const double val = m_values[qty];
    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    // Maße in cm bekommen ein "cm"-Suffix, Maschen/Reihen das passende Symbol
    if (qty == 1 || qty == 4)
    {
        // Breite/Höhe in cm
        I18n::format(buf, sz, WorkshopKey::WORKSHOP_STITCH_GAUGE_VIS_CM, val);
    }
    else if (qty == 2)
    {
        I18n::format(buf, sz, WorkshopKey::WORKSHOP_STITCH_GAUGE_VIS_STITCHES, val);
    }
    else if (qty == 5)
    {
        I18n::format(buf, sz, WorkshopKey::WORKSHOP_STITCH_GAUGE_VIS_ROWS, val);
    }
    else if (qty == 0)
    {
        I18n::format(buf, sz, WorkshopKey::WORKSHOP_STITCH_GAUGE_VIS_STITCHES_PER_10, val);
    }
    else // qty == 3
    {
        I18n::format(buf, sz, WorkshopKey::WORKSHOP_STITCH_GAUGE_VIS_ROWS_PER_10, val);
    }

    dotToComma(buf);
}

// ===================================================================
// Treffertest
// ===================================================================

/**
 * @brief Treffertest auf die Eingabefelder, content-relativ.
 */
int WorkshopTool_StitchGauge::hitTestField(int cx, int cy) const
{
    for (int i = 0; i < NUM_QTY; ++i)
    {
        const Rect &r = m_fieldRects[i];
        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h)
            return i;
    }

    return -1;
}

// ===================================================================
// Tastatureingabe
// ===================================================================

/**
 * @brief Verarbeitet ein Aktionszeichen vom Numpad auf dem fokussierten Feld.
 */
void WorkshopTool_StitchGauge::executeKey(char action)
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

        buf[len]     = action;
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
// Zeichnen - Visualisierung
// ===================================================================

/**
 * @brief Zeichnet die Strickstück-Visualisierung in der linken Spalte.
 */
void WorkshopTool_StitchGauge::drawVisualization()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY + TOP_GAP;
    const int colH = m_h - TOP_GAP;

    spr->fillRect(ox, oy, SCHEM_W, colH, C_VIS_BG);

    // Zentrum der Spalte
    const int cx = ox + SCHEM_W / 2;
    const int cy = oy + colH / 2;

    // Maximalgröße der Box (Polster für Beschriftung rundherum)
    const int maxW = SCHEM_W - 2 * VIS_MARGIN_X;
    const int maxH = colH    - 2 * VIS_MARGIN_Y;

    // Aspect-Verhältnis aus aktuellen Werten Breite:Höhe
    double width  = m_values[1];
    double height = m_values[4];
    if (!std::isfinite(width) || width <= 0.0)
        width = 1.0;
    if (!std::isfinite(height) || height <= 0.0)
        height = 1.0;

    const double aspect = width / height;

    int boxW = maxW;
    int boxH = static_cast<int>(static_cast<double>(boxW) / aspect);
    if (boxH > maxH)
    {
        boxH = maxH;
        boxW = static_cast<int>(static_cast<double>(boxH) * aspect);
    }

    if (boxW < 30)
        boxW = 30;
    if (boxH < 30)
        boxH = 30;

    const int boxLeft = cx - boxW / 2;
    const int boxTop  = cy - boxH / 2;

    // Füllung und Umrandung der Box
    spr->fillRect(boxLeft, boxTop, boxW, boxH, C_BOX_FILL);
    spr->drawRect(boxLeft, boxTop, boxW, boxH, C_BOX_OUTLINE);

    // V-Maschenmuster im Inneren
    drawVPattern(boxLeft, boxTop, boxW, boxH);

    // Akzentfarbe der aktiven Achse für Beschriftungs-Hervorhebung
    const int focusSec = sectionOf(static_cast<int>(m_focusedQty));
    const uint16_t mLabelClr = (focusSec == 0) ? C_ACCENT_M_AXIS : C_LABEL_DIM;
    const uint16_t rLabelClr = (focusSec == 1) ? C_ACCENT_R_AXIS : C_LABEL_DIM;

    spr->loadFont(ComfortaaBold14);

    // Maschen-Beschriftung oben (qty 2)
    char mBuf[24];
    formatVisLabel(2, mBuf, sizeof(mBuf));
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(mLabelClr, C_VIS_BG);
    spr->drawString(mBuf, cx, boxTop - VIS_LABEL_GAP - 8);

    // Breite-Beschriftung unten (qty 1)
    char wBuf[24];
    formatVisLabel(1, wBuf, sizeof(wBuf));
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(mLabelClr, C_VIS_BG);
    spr->drawString(wBuf, cx, boxTop + boxH + VIS_LABEL_GAP + 8);

    // Reihen-Beschriftung links (qty 5)
    char rBuf[24];
    formatVisLabel(5, rBuf, sizeof(rBuf));
    spr->setTextDatum(MR_DATUM);
    spr->setTextColor(rLabelClr, C_VIS_BG);
    spr->drawString(rBuf, boxLeft - VIS_LABEL_GAP, cy);

    // Höhe-Beschriftung rechts (qty 4)
    char hBuf[24];
    formatVisLabel(4, hBuf, sizeof(hBuf));
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(rLabelClr, C_VIS_BG);
    spr->drawString(hBuf, boxLeft + boxW + VIS_LABEL_GAP, cy);

    spr->unloadFont();

    // Probefeld-Beschriftungen (M_10 oben, R_10 unten - dezent)
    spr->loadFont(ComfortaaBold14);

    char m10Buf[24];
    formatVisLabel(0, m10Buf, sizeof(m10Buf));
    spr->setTextDatum(TL_DATUM);
    const bool m10Focused = (static_cast<int>(m_focusedQty) == 0);
    spr->setTextColor(m10Focused ? C_ACCENT_M_AXIS : C_VAL_DIM, C_VIS_BG);
    spr->drawString(m10Buf, ox + 6, oy + 4);

    char r10Buf[24];
    formatVisLabel(3, r10Buf, sizeof(r10Buf));
    spr->setTextDatum(TL_DATUM);
    const bool r10Focused = (static_cast<int>(m_focusedQty) == 3);
    spr->setTextColor(r10Focused ? C_ACCENT_R_AXIS : C_VAL_DIM, C_VIS_BG);
    spr->drawString(r10Buf, ox + 6, oy + colH - 22);

    spr->unloadFont();
}

/**
 * @brief Zeichnet das V-Maschenmuster im Inneren des Strickstück-Rechtecks.
 */
void WorkshopTool_StitchGauge::drawVPattern(int boxLeft, int boxTop, int boxW, int boxH)
{
    Gfx_Sprite *const spr = m_sprite;

    const int innerLeft = boxLeft + 2;
    const int innerTop  = boxTop  + 2;
    const int innerW    = boxW - 4;
    const int innerH    = boxH - 4;

    if (innerW < V_STEP_X || innerH < V_STEP_Y)
        return;

    for (int y = innerTop + V_STEP_Y / 2; y < innerTop + innerH - V_SIZE; y += V_STEP_Y)
    {
        // Zeilenversatz wechselt für das typische Strick-Muster
        const int rowIdx = (y - innerTop) / V_STEP_Y;
        const int xOffset = (rowIdx & 1) ? V_STEP_X / 2 : 0;

        for (int x = innerLeft + xOffset; x < innerLeft + innerW - V_SIZE * 2; x += V_STEP_X)
        {
            // V: zwei kurze Diagonalen
            spr->drawLine(x,             y + V_SIZE,
                          x + V_SIZE,    y,            C_VPATTERN);
            spr->drawLine(x + V_SIZE,    y,
                          x + V_SIZE * 2, y + V_SIZE,  C_VPATTERN);
        }
    }
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle sechs Eingabefelder.
 */
void WorkshopTool_StitchGauge::drawFields(bool cursorOn)
{
    // Hintergrund des Section-Gaps explizit füllen
    if (SECTION_GAP > 0 && m_rightW > 0)
    {
        const int gapY = m_originY + TOP_GAP + 3 * ROW_H;
        m_sprite->fillRect(m_originX + m_rightX, gapY, m_rightW, SECTION_GAP, C_BG);
    }

    m_sprite->loadFont(ComfortaaBold16);

    for (int i = 0; i < NUM_QTY; ++i)
        drawField(i, m_pressedField == i, cursorOn);

    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Feldzeile (Symbol, Wert, Einheit).
 */
void WorkshopTool_StitchGauge::drawField(int qty, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const Rect &fr = m_fieldRects[qty];
    const int ax = m_originX + fr.x;
    const int ay = m_originY + fr.y;

    const bool input   = isInput(qty);
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
        lblClr  = C_LBL_FOCUS;
        unitClr = C_UNIT_FOCUS;
    }
    else if (input)
    {
        lblClr  = C_LBL_INPUT;
        unitClr = C_UNIT_INPUT;
    }
    else
    {
        lblClr  = C_LBL_COMP;
        unitClr = C_UNIT_COMP;
    }

    const int cy = ay + fr.h / 2;

    // Symbol
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblClr, bgColor);
    spr->drawString(I18n::str(QTY_SYMBOL_KEYS[qty]), ax + ACCENT_W + 6, cy);

    // Einheit
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(unitClr, bgColor);
    spr->drawString(I18n::str(QTY_UNIT_KEYS[qty]), ax + fr.w - UNIT_W / 2, cy);

    // Wert
    char displayBuf[32];
    const int valRightX = ax + fr.w - UNIT_W - 6;
    spr->setTextDatum(MR_DATUM);

    if (focused)
    {
        // Eingabepuffer hält '.' für atof - Komma nur in der Anzeige
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
        formatDisplayValue(m_values[qty], displayBuf, sizeof(displayBuf));
        spr->setTextColor(C_VAL_INPUT, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
    else
    {
        const int sec = sectionOf(qty);
        if (m_computeOk[sec])
            formatDisplayValue(m_values[qty], displayBuf, sizeof(displayBuf));
        else
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");
        spr->setTextColor(C_VAL_COMP, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}
