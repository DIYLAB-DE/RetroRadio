#include "WorkshopTool_InchMm.h"

#include <cmath>
#include <cstring>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"

// ===================================================================
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================

namespace
{

    // ===================================================================
    // Farben - Eingabefelder
    // ===================================================================

    constexpr uint16_t C_BG = 0x1082;          ///< Hintergrund
    constexpr uint16_t C_FIELD_FOCUS = 0x328A; ///< Fokussiertes Feld (Eingabe)
    constexpr uint16_t C_FIELD_COMP = 0x2104;  ///< Berechnetes Feld
    constexpr uint16_t C_FIELD_PRESS = 0x4B4D; ///< Feld gedrückt
    constexpr uint16_t C_SEP = 0x2104;         ///< Trennlinie

    constexpr uint16_t C_LBL_FOCUS = 0xFFFF;  ///< Label fokussiert
    constexpr uint16_t C_LBL_COMP = 0x7BEF;   ///< Label berechnet
    constexpr uint16_t C_VAL_FOCUS = 0xFFFF;  ///< Wert fokussiert
    constexpr uint16_t C_VAL_COMP = 0xB5F7;   ///< Wert berechnet
    constexpr uint16_t C_UNIT_FOCUS = 0xAD75; ///< Einheit fokussiert
    constexpr uint16_t C_UNIT_COMP = 0x4228;  ///< Einheit berechnet

    // ===================================================================
    // Farben - Info-Zeile
    // ===================================================================

    constexpr uint16_t C_INFO_TXT = 0xB5F7; ///< Info-Text

    // ===================================================================
    // Farben - Referenztabelle
    // ===================================================================

    constexpr uint16_t C_REF_BG = 0x1082;      ///< Tabellenhintergrund
    constexpr uint16_t C_REF_HDR_BG = 0x2104;  ///< Kopfzeile Hintergrund
    constexpr uint16_t C_REF_HDR_TXT = 0xDEDB; ///< Kopfzeile Text
    constexpr uint16_t C_REF_ROW0 = 0x18C3;    ///< Zeile gerade
    constexpr uint16_t C_REF_ROW1 = 0x1082;    ///< Zeile ungerade
    constexpr uint16_t C_REF_FRAC = 0xDEDB;    ///< Bruch-Text
    constexpr uint16_t C_REF_MM = 0xB5F7;      ///< mm-Wert

    // ===================================================================
    // Akzentfarben
    // ===================================================================

    constexpr uint16_t C_ACCENT_INCH = 0xFD20; ///< Zoll (Orange)
    constexpr uint16_t C_ACCENT_MM = 0x07E0;   ///< mm (Grün)

    // ===================================================================
    // Layout
    // ===================================================================

    constexpr int TOP_GAP = 10; ///< Abstand Headerbereich -> Inhaltsbereich

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

const WorkshopTool_InchMm::RefEntry WorkshopTool_InchMm::REF_TABLE[NUM_REF] = {
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_1_16,   1.59f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_1_8,    3.18f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_3_16,   4.76f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_1_4,    6.35f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_5_16,   7.94f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_3_8,    9.53f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_7_16,  11.11f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_1_2,   12.70f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_9_16,  14.29f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_5_8,   15.88f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_11_16, 17.46f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_3_4,   19.05f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_13_16, 20.64f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_7_8,   22.23f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_15_16, 23.81f},
    {WorkshopKey::WORKSHOP_INCHMM_FRAC_1,     25.40f},
};

const WorkshopKey WorkshopTool_InchMm::FIELD_LABEL_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_INCHMM_LABEL_INCH,
    WorkshopKey::WORKSHOP_INCHMM_LABEL_MM
};

const WorkshopKey WorkshopTool_InchMm::FIELD_UNIT_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_INCHMM_UNIT_INCH,
    WorkshopKey::WORKSHOP_INCHMM_UNIT_MM
};

const uint16_t WorkshopTool_InchMm::FIELD_ACCENTS[NUM_FIELDS] = {
    C_ACCENT_INCH, C_ACCENT_MM};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_InchMm::WorkshopTool_InchMm()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_InchMm::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_InchMm::clearAll()
{
    memset(m_inputBuf, 0, sizeof(m_inputBuf));
    memset(m_values, 0, sizeof(m_values));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));

    m_sprite = nullptr;
    m_originX = 0;
    m_originY = 0;
    m_w = 0;
    m_h = 0;
    m_layoutValid = false;
    m_computeOk = false;
    m_numpad.clearLayout();

    m_focusedField = 0;
    m_pressedField = -1;
    m_pressedKey = -1;
    m_touchWas = false;
    m_touchReady = false;

    // Standardwert: 1 Zoll
    m_values[0] = 1.0;
    m_values[1] = 25.4;

    stringutils::safeFormat(m_inputBuf[0], FIELD_CAP, "1");

    compute();
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Berechnet den nicht-fokussierten Wert aus dem fokussierten Wert.
 */
void WorkshopTool_InchMm::compute()
{
    const double input = atof(m_inputBuf[m_focusedField]);
    m_values[m_focusedField] = input;

    if (m_focusedField == 0)
    {
        // Zoll -> mm
        m_values[1] = input * INCH_TO_MM;
    }
    else
    {
        // mm -> Zoll
        m_values[0] = (INCH_TO_MM > 0.0) ? input / INCH_TO_MM : 0.0;
    }

    m_computeOk = std::isfinite(m_values[0]) && std::isfinite(m_values[1]);
}

/**
 * @brief Ersetzt Dezimalpunkt durch Komma.
 */
void WorkshopTool_InchMm::dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.')
            *p = ',';
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Referenztabelle, Eingabefelder, Info-Zeile und Numpad.
 */
void WorkshopTool_InchMm::draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
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
void WorkshopTool_InchMm::computeLayout(int w, int h)
{
    m_rightX = REF_W + COL_GAP;
    m_rightW = w - m_rightX;
    if (m_rightW < 100)
        m_rightW = 100;

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
    if (m_refRowH < 14)
        m_refRowH = 14;

    if (m_refRowH > 22)
        m_refRowH = 22;
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle Eingabefelder.
 */
void WorkshopTool_InchMm::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);

    for (int i = 0; i < NUM_FIELDS; ++i)
        drawField(i, m_pressedField == i, cursorOn);

    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet ein einzelnes Eingabefeld.
 */
void WorkshopTool_InchMm::drawField(int idx, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const Rect &fr = m_fieldRects[idx];
    const int ax = m_originX + fr.x;
    const int ay = m_originY + fr.y;

    const bool focused = (m_focusedField == idx);

    uint16_t bgColor;
    if (pressed)
        bgColor = C_FIELD_PRESS;
    else if (focused)
        bgColor = C_FIELD_FOCUS;
    else
        bgColor = C_FIELD_COMP;

    spr->fillRect(ax, ay, fr.w, fr.h, bgColor);
    spr->fillRect(ax, ay + fr.h - 1, fr.w, 1, C_SEP);
    spr->fillRect(ax, ay, ACCENT_W, fr.h - 1, FIELD_ACCENTS[idx]);

    // Textfarben
    uint16_t lblClr, valClr, unitClr;
    if (focused)
    {
        lblClr = C_LBL_FOCUS;
        valClr = C_VAL_FOCUS;
        unitClr = C_UNIT_FOCUS;
    }
    else
    {
        lblClr = C_LBL_COMP;
        valClr = C_VAL_COMP;
        unitClr = C_UNIT_COMP;
    }

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

        // Cursor-Blinkphase
        if (!cursorOn)
        {
            const int cursorW = spr->textWidth("_");
            spr->fillRect(valRightX - cursorW, ay + 1, cursorW, fr.h - 2, bgColor);
        }
    }
    else
    {
        // Berechneten Wert anzeigen
        if (m_computeOk)
            stringutils::safeFormat(displayBuf, sizeof(displayBuf), "%.4g", m_values[idx]);
        else
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");

        dotToComma(displayBuf);
        spr->setTextColor(C_VAL_COMP, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}

// ===================================================================
// Zeichnen - Info-Zeile
// ===================================================================

/**
 * @brief Zeichnet den Umrechnungsfaktor als Info-Text.
 */
void WorkshopTool_InchMm::drawInfoRow()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_infoY;
    const int cy = oy + INFO_H / 2;

    spr->fillRect(ox, oy, m_rightW, INFO_H, C_BG);
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_INFO_TXT, C_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_INCHMM_INFO_FACTOR), ox + 4, cy);
    spr->unloadFont();
}

// ===================================================================
// Zeichnen - Referenztabelle
// ===================================================================

/**
 * @brief Zeichnet die Bruch-Referenztabelle (1/16" bis 1").
 */
void WorkshopTool_InchMm::drawRefTable()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;

    // Hintergrund
    spr->fillRect(ox, oy, REF_W, m_h, C_REF_BG);

    const int tableY = oy + TOP_GAP;
    const int colSplit = REF_W / 2;

    // Kopfzeile
    spr->fillSmoothRoundRect(ox + 4, tableY, REF_W - 8, REF_HDR_H, 3, C_REF_HDR_BG);
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_REF_HDR_TXT, C_REF_HDR_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_INCHMM_LABEL_INCH), ox + colSplit / 2, tableY + REF_HDR_H / 2);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_INCHMM_LABEL_MM), ox + colSplit + colSplit / 2, tableY + REF_HDR_H / 2);

    // Datenzeilen
    const int dataY = tableY + REF_HDR_H + 2;

    for (int i = 0; i < NUM_REF; ++i)
    {
        const int rowY = dataY + i * m_refRowH;
        const uint16_t rowBg = (i & 1) ? C_REF_ROW1 : C_REF_ROW0;

        spr->fillRect(ox + 4, rowY, REF_W - 8, m_refRowH - 1, rowBg);

        const int rowCy = rowY + m_refRowH / 2;

        // Bruch
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_REF_FRAC, rowBg);
        spr->drawString(I18n::str(REF_TABLE[i].fractionKey), ox + colSplit / 2, rowCy);

        // mm-Wert
        char mmBuf[12];
        stringutils::safeFormat(mmBuf, sizeof(mmBuf), "%.2f", REF_TABLE[i].mm);
        dotToComma(mmBuf);
        spr->setTextColor(C_REF_MM, rowBg);
        spr->drawString(mmBuf, ox + colSplit + colSplit / 2, rowCy);
    }

    spr->unloadFont();
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Hit-Test auf Eingabefelder.
 */
int WorkshopTool_InchMm::hitTestField(int cx, int cy) const
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
bool WorkshopTool_InchMm::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    const int cx = tx - WorkshopCfg::WORK_X - m_originX;
    const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        if (cx >= m_rightX)
        {
            const int hitField = hitTestField(cx, cy);
            if (hitField >= 0)
            {
                if (hitField != m_pressedField)
                {
                    m_pressedField = hitField;
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
            if (m_pressedField >= 0 && m_pressedField < NUM_FIELDS)
            {
                // Fokuswechsel: berechneten Wert als Eingabepuffer übernehmen
                if (m_pressedField != m_focusedField)
                {
                    char buf[FIELD_CAP];
                    stringutils::safeFormat(buf, FIELD_CAP, "%.4g", m_values[m_pressedField]);
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
        m_pressedKey = -1;
        m_touchWas = false;
    }

    return needsRedraw;
}

// ===================================================================
// Tasteneingabe
// ===================================================================

/**
 * @brief Verarbeitet eine Numpad-Tasteneingabe.
 */
void WorkshopTool_InchMm::executeKey(char action)
{
    char *const buf = m_inputBuf[m_focusedField];
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

        if (len == 0)
        {
            buf[0] = '0';
            buf[1] = '.';
            buf[2] = '\0';
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
        break;
    default:
        break;
    }

    compute();
}
