#include "WorkshopTool_Temp.h"

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
// Farben - Thermometer
// ===================================================================

constexpr uint16_t C_THERMO_BG      = 0x1082; ///< Hintergrund
constexpr uint16_t C_THERMO_TUBE    = 0x2104; ///< Glasröhre (leer)
constexpr uint16_t C_THERMO_OUTLINE = 0x4A69; ///< Umriss
constexpr uint16_t C_THERMO_TICK    = 0x6B6D; ///< Skalenstriche
constexpr uint16_t C_THERMO_LABEL   = 0xAD75; ///< Skalenbeschriftung

// ===================================================================
// Akzentfarben
// ===================================================================

constexpr uint16_t C_ACCENT_C = 0x07E0; ///< Celsius (Grün)
constexpr uint16_t C_ACCENT_F = 0xFD20; ///< Fahrenheit (Orange)
constexpr uint16_t C_ACCENT_K = 0x5DDF; ///< Kelvin (Cyan)

// ===================================================================
// Layout
// ===================================================================

constexpr int TOP_GAP = 10; ///< Abstand Headerbereich -> Inhaltsbereich

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

const WorkshopTool_Temp::RefPoint WorkshopTool_Temp::REF_POINTS[NUM_REF_POINTS] = {
    { -40.0f, WorkshopKey::WORKSHOP_TEMP_REF_M40 },
    {   0.0f, WorkshopKey::WORKSHOP_TEMP_REF_0   },
    {  20.0f, WorkshopKey::WORKSHOP_TEMP_REF_20  },
    {  37.0f, WorkshopKey::WORKSHOP_TEMP_REF_37  },
    { 100.0f, WorkshopKey::WORKSHOP_TEMP_REF_100 },
    { 150.0f, WorkshopKey::WORKSHOP_TEMP_REF_150 },
    { 200.0f, WorkshopKey::WORKSHOP_TEMP_REF_200 },
};

const WorkshopKey WorkshopTool_Temp::FIELD_LABEL_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_TEMP_LABEL_CELSIUS,
    WorkshopKey::WORKSHOP_TEMP_LABEL_FAHRENHEIT,
    WorkshopKey::WORKSHOP_TEMP_LABEL_KELVIN
};

const WorkshopKey WorkshopTool_Temp::FIELD_UNIT_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_TEMP_UNIT_CELSIUS,
    WorkshopKey::WORKSHOP_TEMP_UNIT_FAHRENHEIT,
    WorkshopKey::WORKSHOP_TEMP_UNIT_KELVIN
};

const uint16_t WorkshopTool_Temp::FIELD_ACCENTS[NUM_FIELDS] = {
    C_ACCENT_C, C_ACCENT_F, C_ACCENT_K
};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_Temp::WorkshopTool_Temp()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_Temp::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_Temp::clearAll()
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

    // Standardwert: 20 °C (Raumtemperatur)
    m_values[0] = 20.0;

    stringutils::safeFormat(m_inputBuf[0], FIELD_CAP, "20");

    compute();
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Berechnet die nicht-fokussierten Werte aus dem fokussierten.
 */
void WorkshopTool_Temp::compute()
{
    const double input = atof(m_inputBuf[m_focusedField]);
    m_values[m_focusedField] = input;

    // Alle Werte über °C als Pivot berechnen
    double celsius = 0.0;

    switch (m_focusedField)
    {
    case 0: // °C eingegeben
        celsius = input;
        break;
    case 1: // °F eingegeben
        celsius = (input - 32.0) * 5.0 / 9.0;
        break;
    case 2: // K eingegeben
        celsius = input - KELVIN_OFFSET;
        break;
    }

    m_values[0] = celsius;
    m_values[1] = celsius * 9.0 / 5.0 + 32.0;
    m_values[2] = celsius + KELVIN_OFFSET;

    // Eingabefeld nicht überschreiben
    m_values[m_focusedField] = input;

    m_computeOk = std::isfinite(m_values[0])
               && std::isfinite(m_values[1])
               && std::isfinite(m_values[2]);
}

/**
 * @brief Ersetzt Dezimalpunkt durch Komma.
 */
void WorkshopTool_Temp::dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.') *p = ',';
}

/**
 * @brief Interpoliert eine Farbe auf der Thermometer-Skala.
 */
uint16_t WorkshopTool_Temp::tempToColor(float celsius)
{
    // Farbstützpunkte: -40°C blau, 0°C cyan, 20°C grün, 50°C gelb, 100°C+ rot
    uint8_t r, g, b;

    if (celsius <= -40.0f)
    {
        r = 0; g = 0; b = 255;
    }
    else if (celsius <= 0.0f)
    {
        const float t = (celsius + 40.0f) / 40.0f;
        r = 0;
        g = static_cast<uint8_t>(t * 200);
        b = static_cast<uint8_t>(255 - t * 55);
    }
    else if (celsius <= 20.0f)
    {
        const float t = celsius / 20.0f;
        r = static_cast<uint8_t>(t * 80);
        g = static_cast<uint8_t>(200 + t * 55);
        b = static_cast<uint8_t>(200 - t * 200);
    }
    else if (celsius <= 50.0f)
    {
        const float t = (celsius - 20.0f) / 30.0f;
        r = static_cast<uint8_t>(80 + t * 175);
        g = static_cast<uint8_t>(255 - t * 55);
        b = 0;
    }
    else
    {
        const float t = (celsius - 50.0f) / 50.0f;
        const float tc = (t > 1.0f) ? 1.0f : t;
        r = 255;
        g = static_cast<uint8_t>(200 * (1.0f - tc));
        b = 0;
    }

    // RGB888 -> RGB565
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Thermometer, Eingabefelder, Info-Zeile und Numpad.
 */
void WorkshopTool_Temp::draw(Gfx_Sprite *sprite,
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

    drawThermometer();
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
void WorkshopTool_Temp::computeLayout(int w, int h)
{
    m_rightX = THERMO_W + COL_GAP;
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

    // Thermometer
    m_thermoBarX = THERMO_BAR_LEFT;
    m_thermoBarY = THERMO_TOP;
    m_thermoBarH = h - THERMO_TOP - THERMO_BOTTOM - THERMO_BULB_R * 2;
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle Eingabefelder.
 */
void WorkshopTool_Temp::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);
    for (int i = 0; i < NUM_FIELDS; ++i)
        drawField(i, m_pressedField == i, cursorOn);
    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet ein einzelnes Eingabefeld.
 */
void WorkshopTool_Temp::drawField(int idx, bool pressed, bool cursorOn)
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

        // Cursor-Blinkphase
        if (!cursorOn)
        {
            const int cursorW = spr->textWidth("_");
            spr->fillRect(valRightX - cursorW, ay + 1, cursorW, fr.h - 2, bgColor);
        }
    }
    else
    {
        if (m_computeOk)
            stringutils::safeFormat(displayBuf, sizeof(displayBuf), "%.2f", m_values[idx]);
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
 * @brief Zeichnet die Info-Zeile mit Umrechnungshinweis.
 */
void WorkshopTool_Temp::drawInfoRow()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_infoY;
    const int cy = oy + INFO_H / 2;

    spr->fillRect(ox, oy, m_rightW, INFO_H, C_BG);

    // Formelhinweis je nach aktivem Feld
    const __FlashStringHelper *info = nullptr;
    switch (m_focusedField)
    {
    case 0: info = I18n::str(WorkshopKey::WORKSHOP_TEMP_FORMULA_C2F); break;
    case 1: info = I18n::str(WorkshopKey::WORKSHOP_TEMP_FORMULA_F2C); break;
    case 2: info = I18n::str(WorkshopKey::WORKSHOP_TEMP_FORMULA_K2C); break;
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
// Zeichnen - Thermometer
// ===================================================================

/**
 * @brief Zeichnet die Thermometer-Visualisierung mit Farbverlauf und Referenzpunkten.
 */
void WorkshopTool_Temp::drawThermometer()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;

    // Hintergrund
    spr->fillRect(ox, oy, THERMO_W, m_h, C_THERMO_BG);

    const int barX = ox + m_thermoBarX;
    const int barY = oy + m_thermoBarY;
    const int barH = m_thermoBarH;
    const float tempRange = SCALE_MAX_C - SCALE_MIN_C;

    // Geometrie
    const int bulbCx = barX + THERMO_BAR_W / 2;
    const int bulbCy = barY + barH + THERMO_BULB_R - 2;

    // Quecksilberfarbe basierend auf aktuellem Celsius-Wert
    const float celsiusVal = static_cast<float>(m_values[0]);
    const uint16_t mercuryColor = tempToColor(celsiusVal);

    // Schritt 1: Umrisse (Röhre + Brücke + Bulb)
    spr->fillSmoothRoundRect(barX - 2, barY - 2, THERMO_BAR_W + 4, barH + 4,
                             THERMO_BAR_R + 2, C_THERMO_OUTLINE);
    spr->fillRect(barX - 2, barY + barH - 4, THERMO_BAR_W + 4, THERMO_BULB_R + 4,
                  C_THERMO_OUTLINE);
    spr->fillSmoothCircle(bulbCx, bulbCy, THERMO_BULB_R + 2, C_THERMO_OUTLINE);

    // Schritt 2: Innenflächen (Röhre leer + Brücke + Bulb gefüllt)
    spr->fillSmoothRoundRect(barX, barY, THERMO_BAR_W, barH,
                             THERMO_BAR_R, C_THERMO_TUBE);
    spr->fillRect(barX, barY + barH - 2, THERMO_BAR_W, THERMO_BULB_R + 2,
                  C_THERMO_TUBE);
    spr->fillSmoothCircle(bulbCx, bulbCy, THERMO_BULB_R, mercuryColor);

    // Schritt 3: Quecksilbersäule + Brücke zum Bulb
    if (m_computeOk)
    {
        float clampedC = celsiusVal;
        if (clampedC < SCALE_MIN_C) clampedC = SCALE_MIN_C;
        if (clampedC > SCALE_MAX_C) clampedC = SCALE_MAX_C;

        // Hohe Temperatur = oben (kleines Y)
        const float fraction = (clampedC - SCALE_MIN_C) / tempRange;
        const int mercuryTop = barY + static_cast<int>(barH * (1.0f - fraction));

        // Quecksilber in der Röhre
        const int mercuryH = barY + barH - mercuryTop;
        if (mercuryH > 0)
            spr->fillRect(barX, mercuryTop, THERMO_BAR_W, mercuryH, mercuryColor);

        // Brücke Röhre -> Bulb (immer gefüllt)
        spr->fillRect(barX, barY + barH - 2, THERMO_BAR_W, THERMO_BULB_R + 2, mercuryColor);
    }

    // Skalenstriche und Referenzpunkte
    spr->loadFont(ComfortaaBold14);

    for (int i = 0; i < NUM_REF_POINTS; ++i)
    {
        const float c = REF_POINTS[i].celsius;
        if (c < SCALE_MIN_C || c > SCALE_MAX_C) continue;

        const int tickY = barY + static_cast<int>(barH * (SCALE_MAX_C - c) / tempRange);
        const int tickLeft = barX + THERMO_BAR_W + 4;

        // Skalenstrich
        spr->drawFastHLine(tickLeft, tickY, 8, C_THERMO_TICK);

        // Beschriftung rechts vom Strich
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_THERMO_LABEL, C_THERMO_BG);
        spr->drawString(I18n::str(REF_POINTS[i].labelKey), tickLeft + 12, tickY);
    }

    spr->unloadFont();

    // Marker (Dreieck) bei aktuellem Celsius-Wert
    if (m_computeOk)
    {
        float clampedC = celsiusVal;
        if (clampedC < SCALE_MIN_C) clampedC = SCALE_MIN_C;
        if (clampedC > SCALE_MAX_C) clampedC = SCALE_MAX_C;

        const int markerY = barY + static_cast<int>(barH * (SCALE_MAX_C - clampedC) / tempRange);
        const int mxLeft = barX - 4;
        constexpr int MW = 10;

        spr->fillTriangle(
            mxLeft,        markerY,
            mxLeft - MW,   markerY - MW / 2,
            mxLeft - MW,   markerY + MW / 2,
            mercuryColor);
    }
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Hit-Test auf Eingabefelder.
 */
int WorkshopTool_Temp::hitTestField(int cx, int cy) const
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
bool WorkshopTool_Temp::handleTouch(int32_t tx, int32_t ty, bool touched)
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
                // Fokuswechsel: berechneten Wert als Eingabepuffer übernehmen
                if (m_pressedField != m_focusedField)
                {
                    char buf[FIELD_CAP];
                    stringutils::safeFormat(buf, FIELD_CAP, "%.2f", m_values[m_pressedField]);
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
void WorkshopTool_Temp::executeKey(char action)
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
            if (len == 0 || (len == 1 && buf[0] == '-'))
            {
                const size_t ins = (len == 0) ? 0 : 1;
                memmove(buf + ins + 2, buf + len, 1);
                buf[ins]     = '0';
                buf[ins + 1] = '.';
                buf[ins + 2] = '\0';
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
            // Vorzeichen-Toggle (Temperaturen können negativ sein)
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
