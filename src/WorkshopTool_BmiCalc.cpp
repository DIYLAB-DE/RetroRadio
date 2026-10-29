#include "WorkshopTool_BmiCalc.h"

#include <cmath>
#include <cstring>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold18.h"

// ===================================================================
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================

namespace
{

    // ===================================================================
    // Farben - Eingabefelder
    // ===================================================================

    constexpr uint16_t C_BG = 0x1082;          ///< Hintergrund
    constexpr uint16_t C_FIELD_INPUT = 0x2945; ///< Feld ohne Fokus
    constexpr uint16_t C_FIELD_FOCUS = 0x328A; ///< Feld mit Fokus
    constexpr uint16_t C_FIELD_PRESS = 0x4B4D; ///< Feld gedrückt
    constexpr uint16_t C_SEP = 0x2104;         ///< Trennlinie
    constexpr uint16_t C_LBL_FOCUS = 0xFFFF;   ///< Label fokussiert
    constexpr uint16_t C_LBL_INPUT = 0xDEDB;   ///< Label unfokussiert
    constexpr uint16_t C_VAL_FOCUS = 0xFFFF;   ///< Wert fokussiert
    constexpr uint16_t C_VAL_INPUT = 0xEF7D;   ///< Wert unfokussiert
    constexpr uint16_t C_UNIT_FOCUS = 0xAD75;  ///< Einheit fokussiert
    constexpr uint16_t C_UNIT_INPUT = 0x8410;  ///< Einheit unfokussiert

    // ===================================================================
    // Farben - Ergebnisbereich
    // ===================================================================

    constexpr uint16_t C_RESULT_BG = 0x2104;   ///< Ergebnis-Hintergrund
    constexpr uint16_t C_RESULT_NONE = 0x4228; ///< Keine gültige Berechnung
    constexpr uint16_t C_HINT_TXT = 0x8410;    ///< Hinweistext (gedimmt)

    // ===================================================================
    // Farben - Skala
    // ===================================================================

    constexpr uint16_t C_SCALE_BG = 0x1082;     ///< Skalenhintergrund
    constexpr uint16_t C_SCALE_BORDER = 0x4A69; ///< Skalenrahmen
    constexpr uint16_t C_SCALE_LINE = 0x6B6D;   ///< Grenzlinien
    constexpr uint16_t C_SCALE_LABEL = 0xAD75;  ///< Grenzwert-Text
    constexpr uint16_t C_MARKER_FILL = 0xFFFF;  ///< Pfeilmarker
    constexpr uint16_t C_ZONE_LABEL = 0xDEDB;   ///< Zonenbezeichnung

    // ===================================================================
    // Akzentfarben - Eingabefelder
    // ===================================================================

    constexpr uint16_t C_ACCENT_HEIGHT = 0x07E0; ///< Größe (Grün)
    constexpr uint16_t C_ACCENT_WEIGHT = 0xFD20; ///< Gewicht (Orange)

    // ===================================================================
    // Layout
    // ===================================================================

    constexpr int TOP_GAP = 10; ///< Abstand Headerbereich -> Inhaltsbereich

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

const WorkshopTool_BmiCalc::BmiZone WorkshopTool_BmiCalc::ZONES[NUM_ZONES] = {
    { 0.0f,  16.0f, 0x439F, WorkshopKey::WORKSHOP_BMI_ZONE_SEVERE_UNDER },
    {16.0f,  18.5f, 0x65DF, WorkshopKey::WORKSHOP_BMI_ZONE_UNDER        },
    {18.5f,  25.0f, 0x07E0, WorkshopKey::WORKSHOP_BMI_ZONE_NORMAL       },
    {25.0f,  30.0f, 0xFFE0, WorkshopKey::WORKSHOP_BMI_ZONE_OVER         },
    {30.0f,  35.0f, 0xFD20, WorkshopKey::WORKSHOP_BMI_ZONE_OBESE_1      },
    {35.0f,  40.0f, 0xF800, WorkshopKey::WORKSHOP_BMI_ZONE_OBESE_2      },
    {40.0f,  99.0f, 0xA000, WorkshopKey::WORKSHOP_BMI_ZONE_OBESE_3      },
};

const WorkshopKey WorkshopTool_BmiCalc::FIELD_LABEL_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_BMI_LABEL_HEIGHT,
    WorkshopKey::WORKSHOP_BMI_LABEL_WEIGHT
};

const WorkshopKey WorkshopTool_BmiCalc::FIELD_UNIT_KEYS[NUM_FIELDS] = {
    WorkshopKey::WORKSHOP_BMI_UNIT_CM,
    WorkshopKey::WORKSHOP_BMI_UNIT_KG
};

const uint16_t WorkshopTool_BmiCalc::FIELD_ACCENTS[NUM_FIELDS] = {
    C_ACCENT_HEIGHT, C_ACCENT_WEIGHT};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf.
 */
WorkshopTool_BmiCalc::WorkshopTool_BmiCalc()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_BmiCalc::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_BmiCalc::clearAll()
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
    m_bmi = 0.0;
    m_numpad.clearLayout();

    m_focusedField = 0;
    m_pressedField = -1;
    m_pressedKey = -1;
    m_touchWas = false;
    m_touchReady = false;

    // Standardwerte: 175 cm, 75 kg
    m_values[0] = 175.0;
    m_values[1] = 75.0;

    stringutils::safeFormat(m_inputBuf[0], FIELD_CAP, "175");
    stringutils::safeFormat(m_inputBuf[1], FIELD_CAP, "75");

    compute();
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Berechnet BMI aus den aktuellen Eingabewerten.
 */
void WorkshopTool_BmiCalc::compute()
{
    m_values[0] = atof(m_inputBuf[0]);
    m_values[1] = atof(m_inputBuf[1]);

    const double heightM = m_values[0] / 100.0;

    if (heightM > 0.0 && m_values[1] > 0.0)
    {
        m_bmi = m_values[1] / (heightM * heightM);
        m_computeOk = std::isfinite(m_bmi) && m_bmi > 0.0;
    }
    else
    {
        m_bmi = 0.0;
        m_computeOk = false;
    }
}

/**
 * @brief Gibt die BMI-Zone für einen Wert zurück.
 */
const WorkshopTool_BmiCalc::BmiZone *WorkshopTool_BmiCalc::getZone(float bmi)
{
    for (int i = 0; i < NUM_ZONES; ++i)
    {
        if (bmi >= ZONES[i].bmiMin && bmi < ZONES[i].bmiMax)
            return &ZONES[i];
    }

    return nullptr;
}

/**
 * @brief Ersetzt Dezimalpunkt durch Komma.
 */
void WorkshopTool_BmiCalc::dotToComma(char *buf)
{
    for (char *p = buf; *p != '\0'; ++p)
        if (*p == '.')
            *p = ',';
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Skala, Eingabefelder, Ergebnis und Numpad.
 */
void WorkshopTool_BmiCalc::draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
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

    drawScale();
    drawFields(cursorOn);
    drawResult();
    m_numpad.draw(m_sprite, m_originX + m_rightX, m_originY, m_pressedKey);
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Spaltenaufteilung, Feld-Rects und Numpad-Position.
 */
void WorkshopTool_BmiCalc::computeLayout(int w, int h)
{
    m_rightX = SCHEM_W + COL_GAP;
    m_rightW = w - m_rightX;
    if (m_rightW < 100)
        m_rightW = 100;

    // Eingabefelder (rechte Spalte, TOP_GAP unter Headerbereich)
    for (int i = 0; i < NUM_FIELDS; ++i)
        m_fieldRects[i] = {m_rightX, TOP_GAP + i * ROW_H, m_rightW, ROW_H};

    // Ergebnis nach den Feldern
    m_resultY = TOP_GAP + NUM_FIELDS * ROW_H + RESULT_GAP_V;

    // Numpad am unteren Rand der rechten Spalte
    const int padY = h - WorkshopNumpad::REQUIRED_H;
    m_numpad.computeLayout(m_rightW, padY);

    // Skala (linke Spalte)
    m_scaleBarX = SCALE_BAR_LEFT;
    m_scaleBarY = SCALE_TOP;
    m_scaleBarH = h - SCALE_TOP - SCALE_BOTTOM;
}

// ===================================================================
// Zeichnen - Eingabefelder
// ===================================================================

/**
 * @brief Zeichnet alle Eingabefelder.
 */
void WorkshopTool_BmiCalc::drawFields(bool cursorOn)
{
    m_sprite->loadFont(ComfortaaBold16);

    for (int i = 0; i < NUM_FIELDS; ++i)
        drawField(i, m_pressedField == i, cursorOn);

    m_sprite->unloadFont();
}

/**
 * @brief Zeichnet ein einzelnes Eingabefeld.
 */
void WorkshopTool_BmiCalc::drawField(int idx, bool pressed, bool cursorOn)
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
        bgColor = C_FIELD_INPUT;

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
        lblClr = C_LBL_INPUT;
        valClr = C_VAL_INPUT;
        unitClr = C_UNIT_INPUT;
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
        // Formatierten Wert anzeigen (Dezimalkomma)
        stringutils::safeFormat(displayBuf, sizeof(displayBuf), "%.1f", m_values[idx]);
        dotToComma(displayBuf);
        spr->setTextColor(C_VAL_INPUT, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}

// ===================================================================
// Zeichnen - Ergebnisbereich
// ===================================================================

/**
 * @brief Zeichnet den BMI-Wert, die WHO-Kategorie und den Anwendungs-Hinweis.
 */
void WorkshopTool_BmiCalc::drawResult()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_rightX;
    const int oy = m_originY + m_resultY;
    const int rw = m_rightW;

    // Hintergrund
    spr->fillSmoothRoundRect(ox + 2, oy, rw - 4, RESULT_H, 6, C_RESULT_BG);

    if (!m_computeOk)
    {
        // Kein gültiger BMI
        spr->loadFont(ComfortaaBold16);
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_RESULT_NONE, C_RESULT_BG);
        spr->drawString("---", ox + rw / 2, oy + RESULT_H / 2);
        spr->unloadFont();
    }
    else
    {
        const BmiZone *zone = getZone(static_cast<float>(m_bmi));
        const uint16_t valColor = zone ? zone->color : C_RESULT_NONE;

        // BMI-Wert (links)
        char bmiBuf[16];
        stringutils::safeFormat(bmiBuf, sizeof(bmiBuf), "%.1f", m_bmi);
        dotToComma(bmiBuf);

        char resultBuf[32];
        I18n::format(resultBuf, sizeof(resultBuf), WorkshopKey::WORKSHOP_BMI_FMT_RESULT, bmiBuf);

        spr->loadFont(ComfortaaBold18);
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(valColor, C_RESULT_BG);
        spr->drawString(resultBuf, ox + 12, oy + RESULT_H / 2 - 10);
        spr->unloadFont();

        // Kategorie-Text (darunter)
        if (zone)
        {
            spr->loadFont(ComfortaaBold14);
            spr->setTextDatum(ML_DATUM);
            spr->setTextColor(valColor, C_RESULT_BG);
            spr->drawString(I18n::str(zone->labelKey), ox + 12, oy + RESULT_H / 2 + 14);
            spr->unloadFont();
        }
    }

    // Hinweis unter der Ergebnisbox (immer sichtbar)
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_HINT_TXT, C_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_BMI_NOTE_ADULTS), ox + rw / 2, oy + RESULT_H + 14);
    spr->unloadFont();
}

// ===================================================================
// Zeichnen - BMI-Skala
// ===================================================================

/**
 * @brief Zeichnet die vertikale BMI-Farbskala mit Grenzwerten und Marker.
 */
void WorkshopTool_BmiCalc::drawScale()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX;
    const int oy = m_originY;

    // Hintergrund
    spr->fillRect(ox, oy, SCHEM_W, m_h, C_SCALE_BG);

    const int barX = ox + m_scaleBarX;
    const int barY = oy + m_scaleBarY;
    const int barH = m_scaleBarH;
    const float bmiRange = SCALE_BMI_MAX - SCALE_BMI_MIN;

    // Rahmen
    spr->fillSmoothRoundRect(barX - 1, barY - 1, SCALE_BAR_W + 2, barH + 2, 2, C_SCALE_BORDER);

    // Farbige Zonensegmente auf Rahmen
    for (int i = 0; i < NUM_ZONES; ++i)
    {
        const BmiZone &z = ZONES[i];

        // Auf sichtbaren Bereich clippen
        float zMin = z.bmiMin;
        float zMax = z.bmiMax;

        if (zMin < SCALE_BMI_MIN)
            zMin = SCALE_BMI_MIN;

        if (zMax > SCALE_BMI_MAX)
            zMax = SCALE_BMI_MAX;

        if (zMin >= zMax)
            continue;

        // Hoher BMI = oben (kleines Y), niedriger BMI = unten (großes Y)
        const int y1 = barY + static_cast<int>(barH * (SCALE_BMI_MAX - zMax) / bmiRange);
        const int y2 = barY + static_cast<int>(barH * (SCALE_BMI_MAX - zMin) / bmiRange);

        spr->fillRect(barX, y1, SCALE_BAR_W, y2 - y1, z.color);
    }

    // Grenzlinien, Grenzwert-Labels und Zonennamen
    constexpr float boundaries[] = {16.0f, 18.5f, 25.0f, 30.0f, 35.0f, 40.0f};
    constexpr int numBounds = sizeof(boundaries) / sizeof(boundaries[0]);

    spr->loadFont(ComfortaaBold14);

    // Grenzlinien und Zahlenwerte
    spr->setTextDatum(ML_DATUM);
    for (int i = 0; i < numBounds; ++i)
    {
        const float bmi = boundaries[i];
        const int lineY = barY + static_cast<int>(barH * (SCALE_BMI_MAX - bmi) / bmiRange);

        spr->drawFastHLine(barX, lineY, SCALE_BAR_W, C_SCALE_LINE);

        char label[8];
        if (bmi == static_cast<int>(bmi))
            stringutils::safeFormat(label, sizeof(label), "%d", static_cast<int>(bmi));
        else
            stringutils::safeFormat(label, sizeof(label), "%.1f", bmi);
        dotToComma(label);

        spr->setTextColor(C_SCALE_LABEL, C_SCALE_BG);
        spr->drawString(label, barX + SCALE_BAR_W + 6, lineY);
    }

    // Zonennamen (vertikal zentriert)
    for (int i = 0; i < NUM_ZONES; ++i)
    {
        const BmiZone &z = ZONES[i];
        float zMin = z.bmiMin;
        float zMax = z.bmiMax;

        if (zMin < SCALE_BMI_MIN)
            zMin = SCALE_BMI_MIN;

        if (zMax > SCALE_BMI_MAX)
            zMax = SCALE_BMI_MAX;

        if (zMin >= zMax)
            continue;

        const int y1 = barY + static_cast<int>(barH * (SCALE_BMI_MAX - zMax) / bmiRange);
        const int y2 = barY + static_cast<int>(barH * (SCALE_BMI_MAX - zMin) / bmiRange);
        const int zoneH = y2 - y1;

        if (zoneH >= 14)
        {
            spr->setTextDatum(ML_DATUM);
            spr->setTextColor(C_ZONE_LABEL, C_SCALE_BG);
            spr->drawString(I18n::str(z.labelKey), barX + SCALE_BAR_W + 38, y1 + zoneH / 2);
        }
    }

    spr->unloadFont();

    // Pfeilmarker bei aktuellem BMI
    if (m_computeOk)
    {
        float clampedBmi = static_cast<float>(m_bmi);

        if (clampedBmi < SCALE_BMI_MIN)
            clampedBmi = SCALE_BMI_MIN;

        if (clampedBmi > SCALE_BMI_MAX)
            clampedBmi = SCALE_BMI_MAX;

        const int markerY = barY + static_cast<int>(barH * (SCALE_BMI_MAX - clampedBmi) / bmiRange);
        const int mxLeft = barX - 4;

        // Dreieck
        spr->fillTriangle(mxLeft, markerY, mxLeft - MARKER_W, markerY - MARKER_W / 2, mxLeft - MARKER_W, markerY + MARKER_W / 2, C_MARKER_FILL);
    }
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Hit-Test auf Eingabefelder.
 */
int WorkshopTool_BmiCalc::hitTestField(int cx, int cy) const
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
bool WorkshopTool_BmiCalc::handleTouch(int32_t tx, int32_t ty, bool touched)
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
            // Eingabefelder
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
                // Numpad
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
                m_focusedField = static_cast<uint8_t>(m_pressedField);
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
void WorkshopTool_BmiCalc::executeKey(char action)
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
        // Vorzeichen-Toggle ignorieren (BMI-Werte sind immer positiv)
        break;
    default:
        break;
    }

    compute();
}
