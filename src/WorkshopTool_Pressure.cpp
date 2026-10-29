#include "WorkshopTool_Pressure.h"

#include <cmath>
#include <cstring>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"

// ===================================================================
// Anonymer Namespace - Farb- und Layoutkonstanten
// ===================================================================

namespace {

// ===================================================================
// Farben - Eingabefelder
// ===================================================================

constexpr uint16_t C_BG          = 0x1082; ///< Hintergrund
constexpr uint16_t C_FIELD_FOCUS = 0x328A; ///< Fokussiertes Feld (Eingabe)
constexpr uint16_t C_FIELD_COMP  = 0x2104; ///< Berechnetes Feld
constexpr uint16_t C_FIELD_PRESS = 0x4B4D; ///< Feld gedrückt
constexpr uint16_t C_SEP         = 0x2104; ///< Trennlinie

constexpr uint16_t C_LBL_FOCUS  = 0xFFFF; ///< Label fokussiert
constexpr uint16_t C_LBL_COMP   = 0x7BEF; ///< Label berechnet
constexpr uint16_t C_VAL_FOCUS  = 0xFFFF; ///< Wert fokussiert
constexpr uint16_t C_VAL_COMP   = 0xB5F7; ///< Wert berechnet
constexpr uint16_t C_UNIT_FOCUS = 0xAD75; ///< Einheit fokussiert
constexpr uint16_t C_UNIT_COMP  = 0x4228; ///< Einheit berechnet

// ===================================================================
// Farben - Info-Zeile
// ===================================================================

constexpr uint16_t C_INFO_TXT = 0xB5F7; ///< Info-Text

// ===================================================================
// Farben - Referenztabelle
// ===================================================================

constexpr uint16_t C_REF_BG      = 0x1082; ///< Tabellenhintergrund
constexpr uint16_t C_REF_HDR_BG  = 0x2104; ///< Kopfzeile Hintergrund
constexpr uint16_t C_REF_HDR_TXT = 0xDEDB; ///< Kopfzeile Text
constexpr uint16_t C_REF_ROW0    = 0x18C3; ///< Zeile gerade
constexpr uint16_t C_REF_ROW1    = 0x1082; ///< Zeile ungerade
constexpr uint16_t C_REF_BAR     = 0xDEDB; ///< bar-Wert
constexpr uint16_t C_REF_DESC    = 0xB5F7; ///< Beschreibung

// ===================================================================
// Akzentfarben
// ===================================================================

constexpr uint16_t C_ACCENT_BAR = 0x07E0; ///< bar (Grün)
constexpr uint16_t C_ACCENT_PSI = 0xFD20; ///< psi (Orange)
constexpr uint16_t C_ACCENT_ATM = 0x5DDF; ///< atm (Cyan)
constexpr uint16_t C_ACCENT_PA  = 0xFFE0; ///< Pa (Gelb)

// ===================================================================
// Layout
// ===================================================================

constexpr int TOP_GAP = 10; ///< Abstand Headerbereich -> Inhaltsbereich

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

const WorkshopTool_Pressure::RefEntry WorkshopTool_Pressure::REF_TABLE[NUM_REF] = {
    { 0.3f,    WorkshopKey::WORKSHOP_PRESSURE_REF_BLOOD      },
    { 1.013f,  WorkshopKey::WORKSHOP_PRESSURE_REF_ATMOSPHERE },
    { 2.5f,    WorkshopKey::WORKSHOP_PRESSURE_REF_TYRE_CAR   },
    { 4.0f,    WorkshopKey::WORKSHOP_PRESSURE_REF_TYRE_TRUCK },
    { 6.0f,    WorkshopKey::WORKSHOP_PRESSURE_REF_PNEUMATIC  },
    { 10.0f,   WorkshopKey::WORKSHOP_PRESSURE_REF_WATER_PIPE },
    { 200.0f,  WorkshopKey::WORKSHOP_PRESSURE_REF_DIVE_TANK  },
    { 300.0f,  WorkshopKey::WORKSHOP_PRESSURE_REF_HYDRAULIC  },
};

const WorkshopKey WorkshopTool_Pressure::FIELD_LABEL_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_PRESSURE_UNIT_BAR,
    WorkshopKey::WORKSHOP_PRESSURE_UNIT_PSI,
    WorkshopKey::WORKSHOP_PRESSURE_UNIT_ATM,
    WorkshopKey::WORKSHOP_PRESSURE_UNIT_PA
};

const WorkshopKey WorkshopTool_Pressure::FIELD_UNIT_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_PRESSURE_UNIT_BAR,
    WorkshopKey::WORKSHOP_PRESSURE_UNIT_PSI,
    WorkshopKey::WORKSHOP_PRESSURE_UNIT_ATM,
    WorkshopKey::WORKSHOP_PRESSURE_UNIT_PA
};

const uint16_t WorkshopTool_Pressure::FIELD_ACCENTS[NUM_FIELDS] = {
    C_ACCENT_BAR, C_ACCENT_PSI, C_ACCENT_ATM, C_ACCENT_PA
};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_Pressure::WorkshopTool_Pressure()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_Pressure::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_Pressure::clearAll()
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

    m_focusedField = 0;
    m_pressedField = -1;
    m_pressedKey   = -1;
    m_touchWas     = false;
    m_touchReady   = false;

    // Standardwert: 1 bar
    m_values[0] = 1.0;

    stringutils::safeFormat(m_inputBuf[0], FIELD_CAP, "1");

    compute();
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Berechnet die nicht-fokussierten Werte über Pascal als Pivot.
 */
void WorkshopTool_Pressure::compute()
{
    const double input = atof(m_inputBuf[m_focusedField]);
    m_values[m_focusedField] = input;

    // Eingabe -> Pascal
    double pa = 0.0;
    switch (m_focusedField)
    {
    case 0: pa = input * BAR_TO_PA; break;
    case 1: pa = input * PSI_TO_PA; break;
    case 2: pa = input * ATM_TO_PA; break;
    case 3: pa = input;             break;
    }

    // Pascal -> alle Einheiten
    m_values[0] = pa / BAR_TO_PA;
    m_values[1] = pa / PSI_TO_PA;
    m_values[2] = pa / ATM_TO_PA;
    m_values[3] = pa;

    // Eingabefeld nicht überschreiben
    m_values[m_focusedField] = input;

    m_computeOk = std::isfinite(m_values[0])
               && std::isfinite(m_values[1])
               && std::isfinite(m_values[2])
               && std::isfinite(m_values[3]);
}

/**
 * @brief Ersetzt Dezimalpunkt durch Komma.
 */
void WorkshopTool_Pressure::dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.') *p = ',';
}

/**
 * @brief Formatiert einen Druckwert kompakt.
 */
void WorkshopTool_Pressure::formatPressure(double val, bool isPa, char *buf, size_t sz)
{
    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    if (isPa)
    {
        const double absVal = std::abs(val);
        if (absVal >= 1.0e6)
            stringutils::safeFormat(buf, sz, "%.4g M", val / 1.0e6);
        else if (absVal >= 1.0e3)
            stringutils::safeFormat(buf, sz, "%.4g k", val / 1.0e3);
        else
            stringutils::safeFormat(buf, sz, "%.4g", val);
    }
    else
    {
        stringutils::safeFormat(buf, sz, "%.4g", val);
    }

    dotToComma(buf);
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Referenztabelle, Eingabefelder, Info-Zeile und Numpad.
 */
void WorkshopTool_Pressure::draw(Gfx_Sprite *sprite,
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

    drawRefTable();
    drawFields(cursorOn);
    drawInfoRow();
    m_numpad.draw(m_sprite, m_originX + m_rightX, m_originY, m_pressedKey);
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Spaltenaufteilung, Feld-Rects und Numpad-Position.
 */
void WorkshopTool_Pressure::computeLayout(int w, int h)
{
    m_rightX = REF_W + COL_GAP;
    m_rightW = w - m_rightX;
    if (m_rightW < 100) m_rightW = 100;

    // Eingabefelder (rechte Spalte, TOP_GAP unter Headerbereich)
    for (int i = 0; i < NUM_FIELDS; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * ROW_H, m_rightW, ROW_H};

    // Info-Zeile nach den Feldern
    m_infoY = TOP_GAP + NUM_FIELDS * ROW_H + INFO_GAP_V;

    // Numpad am unteren Rand der rechten Spalte
    const int padY = h - WorkshopNumpad::REQUIRED_H;
    m_numpad.computeLayout(m_rightW, padY);

    // Referenztabelle - Zeilenhöhe dynamisch
    const int refAvailH = h - TOP_GAP - REF_HDR_H;
    m_refRowH = refAvailH / NUM_REF;
    if (m_refRowH < 14) m_refRowH = 14;
    if (m_refRowH > 34) m_refRowH = 34;
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle Eingabefelder.
 */
void WorkshopTool_Pressure::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);
    for (int i = 0; i < NUM_FIELDS; ++i)
        drawField(i, m_pressedField == i, cursorOn);
    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet ein einzelnes Eingabefeld.
 */
void WorkshopTool_Pressure::drawField(int idx, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const Rect &fr  = m_fieldRects[idx];
    const int ax = m_originX + fr.x;
    const int ay = m_originY + fr.y;

    const bool focused = (m_focusedField == idx);

    uint16_t bgColor;
    if      (pressed) bgColor = C_FIELD_PRESS;
    else if (focused) bgColor = C_FIELD_FOCUS;
    else              bgColor = C_FIELD_COMP;

    spr->fillRect(ax, ay, fr.w, fr.h, bgColor);
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, FIELD_ACCENTS[idx]);

    // Textfarben
    uint16_t lblClr, valClr, unitClr;
    if (focused) { lblClr = C_LBL_FOCUS; valClr = C_VAL_FOCUS; unitClr = C_UNIT_FOCUS; }
    else         { lblClr = C_LBL_COMP;  valClr = C_VAL_COMP;  unitClr = C_UNIT_COMP;  }

    const int cy = ay + fr.h / 2;

    // Label
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblClr, bgColor);
    spr->drawString(I18n::str(FIELD_LABEL_KEYS[idx]), ax + ACCENT_W + 6, cy);

    // Einheit
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(unitClr, bgColor);
    spr->drawString(I18n::str(FIELD_UNIT_KEYS[idx]), ax + fr.w - UNIT_W / 2, cy);

    // Wert
    char displayBuf[32];
    const int valRightX = ax + fr.w - UNIT_W - 6;
    spr->setTextDatum(MR_DATUM);

    if (focused)
    {
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), m_inputBuf[idx]);
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
    else
    {
        if (m_computeOk)
            formatPressure(m_values[idx], idx == 3, displayBuf, sizeof(displayBuf));
        else
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");
        spr->setTextColor(C_VAL_COMP, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}

// ===================================================================
// Zeichnen - Info-Zeile
// ===================================================================

/**
 * @brief Zeichnet den Info-Text mit Umrechnungshinweis.
 */
void WorkshopTool_Pressure::drawInfoRow()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_infoY;
    const int cy = oy + INFO_H / 2;

    spr->fillRect(ox, oy, m_rightW, INFO_H, C_BG);

    const __FlashStringHelper *info = nullptr;
    switch (m_focusedField)
    {
    case 0: info = I18n::str(WorkshopKey::WORKSHOP_PRESSURE_INFO_BAR); break;
    case 1: info = I18n::str(WorkshopKey::WORKSHOP_PRESSURE_INFO_PSI); break;
    case 2: info = I18n::str(WorkshopKey::WORKSHOP_PRESSURE_INFO_ATM); break;
    case 3: info = I18n::str(WorkshopKey::WORKSHOP_PRESSURE_INFO_PA);  break;
    }

    if (info)
    {
        spr->loadFont(ComfortaaBold14);
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_INFO_TXT, C_BG);
        spr->drawString(info, ox + 4, cy);
        spr->unloadFont();
    }
}

// ===================================================================
// Zeichnen - Referenztabelle
// ===================================================================

/**
 * @brief Zeichnet die Referenztabelle gängiger Druckwerte.
 */
void WorkshopTool_Pressure::drawRefTable()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;

    // Hintergrund
    spr->fillRect(ox, oy, REF_W, m_h, C_REF_BG);

    const int tableY = oy + TOP_GAP;
    const int colSplit = 60; // bar-Spalte 60 px, Rest Beschreibung

    // Kopfzeile
    spr->fillSmoothRoundRect(ox + 4, tableY, REF_W - 8, REF_HDR_H, 3, C_REF_HDR_BG);

    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_REF_HDR_TXT, C_REF_HDR_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_PRESSURE_UNIT_BAR), ox + colSplit / 2, tableY + REF_HDR_H / 2);

    spr->setTextDatum(ML_DATUM);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_PRESSURE_REF_HDR_APPLICATION), ox + colSplit + 8, tableY + REF_HDR_H / 2);

    // Datenzeilen
    const int dataY = tableY + REF_HDR_H + 2;

    for (int i = 0; i < NUM_REF; ++i)
    {
        const int rowY = dataY + i * m_refRowH;
        const uint16_t rowBg = (i & 1) ? C_REF_ROW1 : C_REF_ROW0;

        spr->fillRect(ox + 4, rowY, REF_W - 8, m_refRowH - 1, rowBg);

        const int rowCy = rowY + m_refRowH / 2;

        // bar-Wert
        char barBuf[12];
        if (REF_TABLE[i].bar >= 1.0f && REF_TABLE[i].bar == static_cast<int>(REF_TABLE[i].bar))
            stringutils::safeFormat(barBuf, sizeof(barBuf), "%d", static_cast<int>(REF_TABLE[i].bar));
        else
            stringutils::safeFormat(barBuf, sizeof(barBuf), "%.3g", REF_TABLE[i].bar);
        dotToComma(barBuf);

        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_REF_BAR, rowBg);
        spr->drawString(barBuf, ox + colSplit / 2, rowCy);

        // Beschreibung
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_REF_DESC, rowBg);
        spr->drawString(I18n::str(REF_TABLE[i].labelKey), ox + colSplit + 8, rowCy);
    }

    spr->unloadFont();
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Hit-Test auf Eingabefelder.
 */
int WorkshopTool_Pressure::hitTestField(int cx, int cy) const
{
    for (int i = 0; i < NUM_FIELDS; ++i)
    {
        const Rect &r = m_fieldRects[i];
        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h)
            return i;
    }
    return -1;
}

/**
 * @brief Verarbeitet Touch-Events (Felder + Numpad).
 */
bool WorkshopTool_Pressure::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    const int cx = tx - WorkshopCfg::WORK_X - m_originX;
    const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

    if (touched)
    {
        if (!m_touchReady) return false;

        if (cx >= m_rightX)
        {
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
            if (m_pressedField >= 0 && m_pressedField < NUM_FIELDS)
            {
                if (m_pressedField != m_focusedField)
                {
                    char buf[FIELD_CAP];
                    formatPressure(m_values[m_pressedField],
                                   m_pressedField == 3, buf, FIELD_CAP);
                    // Komma zurück zu Punkt für atof-Kompatibilität
                    for (char *p = buf; *p; ++p)
                        if (*p == ',') *p = '.';
                    // Suffix entfernen (k, M)
                    size_t len = strlen(buf);
                    while (len > 0 && (buf[len - 1] == ' ' || buf[len - 1] == 'k' || buf[len - 1] == 'M'))
                        buf[--len] = '\0';
                    stringutils::safeCopy(m_inputBuf[m_pressedField], FIELD_CAP, buf);
                    m_focusedField = static_cast<uint8_t>(m_pressedField);
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
// Tasteneingabe
// ===================================================================

/**
 * @brief Verarbeitet eine Numpad-Tasteneingabe.
 */
void WorkshopTool_Pressure::executeKey(char action)
{
    char *const buf = m_inputBuf[m_focusedField];
    const size_t len = strlen(buf);

    switch (action)
    {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        {
            if (len + 1 >= FIELD_CAP) break;
            buf[len] = action;
            buf[len + 1] = '\0';
            break;
        }
        case '.':
        {
            if (strchr(buf, '.') != nullptr) break;
            if (len + 2 >= FIELD_CAP) break;
            if (len == 0)
            {
                buf[0] = '0';
                buf[1] = '.';
                buf[2] = '\0';
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
            break;
        default:
            break;
    }

    compute();
}
