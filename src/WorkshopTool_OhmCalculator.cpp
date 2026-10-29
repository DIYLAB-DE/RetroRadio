#include "WorkshopTool_OhmCalculator.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "StringUtils.h"
#include "I18n.h"
#include "Workshop.h"
#include "WorkshopNumpad.h"
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

/** @brief Kurzsymbole der vier Größen. */
const char * const WorkshopTool_OhmCalculator::QTY_SYMBOLS[NUM_QTY] =
{
    "U", "I", "R", "P"
};

/** @brief Langnamen der vier Größen. */
const WorkshopKey WorkshopTool_OhmCalculator::QTY_NAME_KEYS[NUM_QTY] =
{
    WorkshopKey::WORKSHOP_OHM_NAME_VOLTAGE,
    WorkshopKey::WORKSHOP_OHM_NAME_CURRENT,
    WorkshopKey::WORKSHOP_OHM_NAME_RESISTANCE,
    WorkshopKey::WORKSHOP_OHM_NAME_POWER
};

/**
 * @brief Basiseinheiten; "Ohm" statt Omega-Symbol (nicht in allen Fonts vorhanden).
 */
const char * const WorkshopTool_OhmCalculator::QTY_UNITS[NUM_QTY] =
{
    "V", "A", "Ohm", "W"
};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Konstruktor - initialisiert auf Standardwerte (U=12V, R=5000Ohm).
 */
WorkshopTool_OhmCalculator::WorkshopTool_OhmCalculator()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_OhmCalculator::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_OhmCalculator::clearAll()
{
    memset(m_inputBuf,   0, sizeof(m_inputBuf));
    memset(m_values,     0, sizeof(m_values));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));

    m_sprite      = nullptr;
    m_originX     = 0;
    m_originY     = 0;
    m_w           = 0;
    m_h           = 0;
    m_layoutValid = false;
    m_computeOk   = false;
    m_numpad.clearLayout();

    m_pressedField = -1;
    m_pressedKey   = -1;
    m_touchWas     = false;
    m_touchReady   = false;

    // Standardbelegung: U = 12 V (Slot 1, fokussiert), R = 5000 Ohm (Slot 0)
    m_inputSlot[0] = 2; // R (älterer Slot)
    m_inputSlot[1] = 0; // U (neuerer Slot, fokussiert)
    m_focusedQty   = 0;

    m_values[0] = 12.0;
    m_values[2] = 5000.0;

    formatEditValue(m_values[0], m_inputBuf[0], FIELD_CAP); // "12"
    formatEditValue(m_values[2], m_inputBuf[2], FIELD_CAP); // "5000"

    // I und P aus U, R berechnen
    compute();
}

// ===================================================================
// Öffentliche Interface-Methoden
// ===================================================================

/**
 * @brief Zeichnet Container, Felder, Formelzeile und Nummernpad vollständig.
 */
void WorkshopTool_OhmCalculator::draw(Gfx_Sprite *sprite,
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

    // Cursor-Blink: 400 ms an, 400 ms aus
    const bool cursorOn = (millis() % CURSOR_PERIOD_MS) < CURSOR_ON_MS;

    drawFields(cursorOn);
    drawFormulaRow();
    m_numpad.draw(m_sprite, m_originX, m_originY, m_pressedKey);
}

/**
 * @brief Verarbeitet Touch-Events mit Touch-up-Semantik.
 */
bool WorkshopTool_OhmCalculator::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        // Phantom-Touch-Schutz: erst nach dem ersten sauberen touched=false-Frame
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        // Treffertest Größen-Felder
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
            // Treffertest Nummernpad
            const int hitKey = m_numpad.hitTest(cx, cy);
            if (hitKey >= 0 && hitKey != m_pressedKey)
            {
                m_pressedKey   = hitKey;
                m_pressedField = -1;
                needsRedraw    = true;
            }
        }

        m_touchWas = true;
    }
    else
    {
        // Erstes touched=false-Frame: ab jetzt sind Touch-ups wirksam
        m_touchReady = true;

        if (m_touchWas)
        {
            if (m_pressedField >= 0)
            {
                const int tapped = m_pressedField;

                if (isInput(tapped))
                {
                    // Fokus auf das bereits als Eingabe aktive Feld verschieben
                    m_focusedQty = static_cast<uint8_t>(tapped);
                }
                else
                {
                    // Berechnetes Feld wird zur Eingabe: ältester Slot weicht (FIFO)
                    m_inputSlot[0] = m_inputSlot[1];
                    m_inputSlot[1] = static_cast<uint8_t>(tapped);
                    m_focusedQty   = static_cast<uint8_t>(tapped);

                    // Eingabepuffer mit dem aktuell berechneten Wert vorbelegen
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
        m_pressedKey   = -1;
        m_touchWas     = false;
    }

    return needsRedraw;
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet alle Feld- und Tasten-Rechtecke auf Basis der Netto-Abmessungen.
 */
void WorkshopTool_OhmCalculator::computeLayout(int w, int h)
{
    // Größen-Zeilen (content-relativ, TOP_GAP Abstand oben)
    for (int i = 0; i < NUM_QTY; ++i)
        m_fieldRects[i] = {0, TOP_GAP + i * ROW_H, w, ROW_H};

    // Nummernpad: TOP_GAP + Felder + Gap-oben + Formelzeile + Gap-unten
    m_numpad.computeLayout(w, h - WorkshopNumpad::REQUIRED_H);
}

// ===================================================================
// Wertberechnung
// ===================================================================

/**
 * @brief Parst Eingabepuffer und berechnet die zwei Ausgabegrößen.
 */
void WorkshopTool_OhmCalculator::compute()
{
    const int a = m_inputSlot[0];
    const int b = m_inputSlot[1];

    const double va = (m_inputBuf[a][0] != '\0') ? atof(m_inputBuf[a]) : 0.0;
    const double vb = (m_inputBuf[b][0] != '\0') ? atof(m_inputBuf[b]) : 0.0;
    m_values[a] = va;
    m_values[b] = vb;

    const double u = m_values[0];
    const double i = m_values[1];
    const double r = m_values[2];
    const double p = m_values[3];

    m_computeOk = false;

    if (!std::isfinite(va) || !std::isfinite(vb))
        return;

    const int lo = std::min(a, b);
    const int hi = std::max(a, b);

    if (lo == 0 && hi == 1)
    {
        if (std::abs(i) < 1e-18) return;
        const double cr = u / i;
        const double cp = u * i;
        if (!std::isfinite(cr) || !std::isfinite(cp) || cr < 0.0) return;
        m_values[2] = cr; m_values[3] = cp;
        m_computeOk = true;
    }
    else if (lo == 0 && hi == 2)
    {
        if (std::abs(r) < 1e-18) return;
        const double ci = u / r;
        const double cp = u * u / r;
        if (!std::isfinite(ci) || !std::isfinite(cp)) return;
        m_values[1] = ci; m_values[3] = cp;
        m_computeOk = true;
    }
    else if (lo == 0 && hi == 3)
    {
        if (std::abs(u) < 1e-18 || std::abs(p) < 1e-30) return;
        const double ci = p / u;
        const double cr = u * u / p;
        if (!std::isfinite(ci) || !std::isfinite(cr) || cr < 0.0) return;
        m_values[1] = ci; m_values[2] = cr;
        m_computeOk = true;
    }
    else if (lo == 1 && hi == 2)
    {
        const double cu = i * r;
        const double cp = i * i * r;
        if (!std::isfinite(cu) || !std::isfinite(cp) || cp < 0.0) return;
        m_values[0] = cu; m_values[3] = cp;
        m_computeOk = true;
    }
    else if (lo == 1 && hi == 3)
    {
        if (std::abs(i) < 1e-18) return;
        const double cu = p / i;
        const double cr = p / (i * i);
        if (!std::isfinite(cu) || !std::isfinite(cr) || cr < 0.0) return;
        m_values[0] = cu; m_values[2] = cr;
        m_computeOk = true;
    }
    else if (lo == 2 && hi == 3)
    {
        if (r < 0.0 || p < 0.0 || std::abs(r) < 1e-18) return;
        const double pr = p * r;
        if (pr < 0.0) return;
        const double cu = std::sqrt(pr);
        const double ci = std::sqrt(p / r);
        if (!std::isfinite(cu) || !std::isfinite(ci)) return;
        m_values[0] = cu; m_values[1] = ci;
        m_computeOk = true;
    }
}

/**
 * @brief Formatiert einen SI-Wert als auto-skalierten Anzeigestring.
 */
void WorkshopTool_OhmCalculator::formatDisplayValue(double val, int qty,
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

    if (qty == 2)
    {
        // Widerstand: kein µ/m-Präfix
        if      (absVal < 1.0e3) { scaled = val;         prefix = "";  }
        else if (absVal < 1.0e6) { scaled = val / 1.0e3; prefix = "k"; }
        else if (absVal < 1.0e9) { scaled = val / 1.0e6; prefix = "M"; }
        else                     { scaled = val / 1.0e9; prefix = "G"; }
    }
    else
    {
        // Spannung / Strom / Leistung
        if      (absVal < 1.0e-3) { scaled = val * 1.0e6; prefix = "u"; } // Micro als ASCII
        else if (absVal < 1.0)    { scaled = val * 1.0e3; prefix = "m"; }
        else if (absVal < 1.0e3)  { scaled = val;         prefix = "";  }
        else if (absVal < 1.0e6)  { scaled = val / 1.0e3; prefix = "k"; }
        else                      { scaled = val / 1.0e6; prefix = "M"; }
    }

    // %.4g: 4 signifikante Stellen, trailing zeros automatisch entfernt
    stringutils::safeFormat(buf, sz, "%.4g %s%s", scaled, prefix, unit);
}

/**
 * @brief Formatiert einen SI-Wert als editierbaren Dezimalstring.
 */
void WorkshopTool_OhmCalculator::formatEditValue(double val, char *buf, size_t sz)
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
 * @brief Befüllt buf mit dem ASCII-Formelstring für das aktuelle Slot-Paar.
 */
void WorkshopTool_OhmCalculator::buildFormulaStr(char *buf, size_t sz) const
{
    const int lo = std::min(static_cast<int>(m_inputSlot[0]),
                            static_cast<int>(m_inputSlot[1]));
    const int hi = std::max(static_cast<int>(m_inputSlot[0]),
                            static_cast<int>(m_inputSlot[1]));

    if      (lo == 0 && hi == 1) stringutils::safeCopy(buf, sz, "R = U / I   |   P = U * I");
    else if (lo == 0 && hi == 2) stringutils::safeCopy(buf, sz, "I = U / R   |   P = U*U / R");
    else if (lo == 0 && hi == 3) stringutils::safeCopy(buf, sz, "I = P / U   |   R = U*U / P");
    else if (lo == 1 && hi == 2) stringutils::safeCopy(buf, sz, "U = I * R   |   P = I*I * R");
    else if (lo == 1 && hi == 3) stringutils::safeCopy(buf, sz, "U = P / I   |   R = P / (I*I)");
    else if (lo == 2 && hi == 3) stringutils::safeCopy(buf, sz, "U = sqrt(P*R)   |   I = sqrt(P/R)");
    else                         stringutils::safeCopy(buf, sz, "");
}

/**
 * @brief Gibt zurück ob eine Größe als Eingabe-Slot aktiv ist.
 */
bool WorkshopTool_OhmCalculator::isInput(int qty) const
{
    return (qty == static_cast<int>(m_inputSlot[0]) ||
            qty == static_cast<int>(m_inputSlot[1]));
}

/**
 * @brief Gibt die Akzentfarbe des linken Streifens für eine Größe zurück.
 */
uint16_t WorkshopTool_OhmCalculator::accentColor(int qty)
{
    switch (qty)
    {
        case 0:  return C_ACCENT_U;
        case 1:  return C_ACCENT_I;
        case 2:  return C_ACCENT_R;
        case 3:  return C_ACCENT_P;
        default: return 0xFFFF;
    }
}

// ===================================================================
// Tastatureingabe
// ===================================================================

/**
 * @brief Führt eine Tastenaktion auf dem Eingabepuffer der fokussierten Größe aus.
 */
void WorkshopTool_OhmCalculator::executeKey(char action)
{
    char * const buf = m_inputBuf[m_focusedQty];
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
            if (strchr(buf, '.') != nullptr) break; // kein doppelter Punkt
            if (len + 2 >= FIELD_CAP) break;
            if (len == 0 || (len == 1 && buf[0] == '-'))
            {
                // "0." bzw. "-0." automatisch voranstellen
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
        case 'B': // Backspace
        {
            if (len > 0) buf[len - 1] = '\0';
            break;
        }
        case 'C': // Clear
        {
            buf[0] = '\0';
            break;
        }
        case 'S': // Vorzeichen togglen
        {
            if (len == 0)
            {
                buf[0] = '-'; buf[1] = '\0';
            }
            else if (buf[0] == '-')
            {
                memmove(buf, buf + 1, len); // Minus entfernen (len Bytes inkl. '\0')
            }
            else
            {
                if (len + 1 >= FIELD_CAP) break;
                memmove(buf + 1, buf, len + 1); // Platz für Minus schaffen
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
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet alle vier Größen-Zeilen (Font wird einmalig geladen).
 */
void WorkshopTool_OhmCalculator::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);
    for (int i = 0; i < NUM_QTY; ++i)
        drawField(i, m_pressedField == i, cursorOn);
    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Größen-Zeile.
 */
void WorkshopTool_OhmCalculator::drawField(int qty, bool pressed, bool cursorOn)
{
    Gfx_Sprite * const spr = m_sprite;
    const Rect        &fr  = m_fieldRects[qty];
    const int          ax  = m_originX + fr.x;
    const int          ay  = m_originY + fr.y;

    const bool input   = isInput(qty);
    const bool focused = input && (static_cast<int>(m_focusedQty) == qty);

    // Hintergrundfarbe: fokussiert oder einheitlich dunkel für alle anderen Zeilen
    uint16_t bgColor;
    if      (pressed) bgColor = C_FIELD_PRESS;
    else if (focused) bgColor = C_FIELD_FOCUS;
    else              bgColor = C_FIELD_INPUT;

    spr->fillRect(ax, ay, fr.w, fr.h, bgColor);

    // Trennlinie am unteren Rand (1 px)
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);

    // Farbakzent-Streifen am linken Rand
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, accentColor(qty));

    // Textfarben abhängig vom Feld-Zustand
    uint16_t lblColor, nameColor, unitColor;
    if (focused)
    {
        lblColor  = C_LBL_FOCUS;
        nameColor = C_NAME_FOCUS;
        unitColor = C_UNIT_FOCUS;
    }
    else if (input)
    {
        lblColor  = C_LBL_INPUT;
        nameColor = C_NAME_INPUT;
        unitColor = C_UNIT_INPUT;
    }
    else
    {
        lblColor  = C_LBL_COMP;
        nameColor = C_NAME_COMP;
        unitColor = C_UNIT_COMP;
    }

    const int cy = ay + fr.h / 2; // vertikale Mitte der Zeile

    // Kurzsymbol (U / I / R / P) - zentriert in der Symbol-Spalte
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(lblColor, bgColor);
    spr->drawString(QTY_SYMBOLS[qty],
                    ax + ACCENT_W + (LABEL_W - ACCENT_W) / 2, cy);

    // Langname in Klammern - linksbündig beginnend nach Symbol-Spalte
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(nameColor, bgColor);
    spr->drawString(I18n::str(QTY_NAME_KEYS[qty]), ax + LABEL_W + 4, cy);

    // Einheit - zentriert in der Einheiten-Spalte (ganz rechts)
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(unitColor, bgColor);
    spr->drawString(QTY_UNITS[qty], ax + fr.w - UNIT_W / 2, cy);

    // Wert - rechtsbündig im Mittelteil (6 px Abstand zur Einheitenspalte)
    char displayBuf[32];
    if (focused)
    {
        // Cursor-String aufbauen - immer mit "_", damit Position konstant bleibt
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), m_inputBuf[qty]);
        stringutils::safeAppend(displayBuf, sizeof(displayBuf), "_");
        dotToComma(displayBuf);

        const int valRightX = ax + fr.w - UNIT_W - 6;
        spr->setTextDatum(MR_DATUM);

        // Vollen String immer in Textfarbe zeichnen -> "12" springt nicht
        spr->setTextColor(C_VAL_FOCUS, bgColor);
        spr->drawString(displayBuf, valRightX, cy);

        // Cursor ausgeblendet: Cursor-Bereich pixelgenau mit fillRect überdecken, da drawString Anti-Aliasing-Artefakte hinterlässt die nicht bgColor sind
        if (!cursorOn)
        {
            const int cursorW = spr->textWidth("_");
            const int cursorH = fr.h - 2; // 1 px Abstand oben und unten
            spr->fillRect(valRightX - cursorW, ay + 1, cursorW, cursorH, bgColor);
        }
    }
    else if (input)
    {
        // Auto-skalierter Anzeigewert (ohne Cursor)
        formatDisplayValue(m_values[qty], qty, displayBuf, sizeof(displayBuf));
        dotToComma(displayBuf);

        const int valRightX = ax + fr.w - UNIT_W - 6;
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_VAL_INPUT, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
    else
    {
        // Berechneter Wert oder Fehlerplatzhalter
        if (m_computeOk)
        {
            formatDisplayValue(m_values[qty], qty, displayBuf, sizeof(displayBuf));
            dotToComma(displayBuf);
        }
        else
        {
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");
        }

        const int valRightX = ax + fr.w - UNIT_W - 6;
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_VAL_COMP, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}

/**
 * @brief Zeichnet die Formelzeile zwischen Feldern und Nummernpad.
 */
void WorkshopTool_OhmCalculator::drawFormulaRow()
{
    Gfx_Sprite * const spr = m_sprite;
    const int          ox  = m_originX;
    const int          oy  = m_originY + TOP_GAP + FIELDS_H + FORMULA_GAP_V;

    // Eigener Hintergrund hebt die Formelzeile vom Container ab
    spr->fillRect(ox, oy, m_w, FORMULA_H, C_BG);

    char formulaBuf[FORMULA_CAP];
    buildFormulaStr(formulaBuf, sizeof(formulaBuf));

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_FORMULA_TXT, C_BG);
    spr->drawString(formulaBuf, ox + m_w / 2, oy + FORMULA_H / 2);
    spr->unloadFont();
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf die vier Größen-Zeilen (content-relativ).
 */
int WorkshopTool_OhmCalculator::hitTestField(int cx, int cy) const
{
    for (int i = 0; i < NUM_QTY; ++i)
    {
        const Rect &r = m_fieldRects[i];
        if (cx >= r.x && cx < r.x + r.w &&
            cy >= r.y && cy < r.y + r.h)
        {
            return i;
        }
    }
    return -1;
}
