#include "WorkshopTool_Decibel.h"

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

namespace
{

    /**
     * @brief Ersetzt den Dezimalpunkt im Puffer durch ein Komma.
     * @param buf Puffer mit formatiertem Zahlenwert
     */
    void dotToComma(char *buf)
    {
        for (char *p = buf; *p != '\0'; ++p)
            if (*p == '.')
            {
                *p = ',';
                break;
            }
    }

} // namespace

// ===================================================================
// Statische Datenbasis
// ===================================================================

/** @brief Kurzsymbole Spannungssektion. */
const char *const WorkshopTool_Decibel::QTY_SYMBOLS_V[WorkshopTool_Decibel::NUM_QTY] =
    {
        "V1", "V2", "dB"};

/** @brief Kurzsymbole Leistungssektion. */
const char *const WorkshopTool_Decibel::QTY_SYMBOLS_P[WorkshopTool_Decibel::NUM_QTY] =
    {
        "P1", "P2", "dB"};

/** @brief Langnamen - identisch für beide Sektionen. */
const WorkshopKey WorkshopTool_Decibel::QTY_NAME_KEYS[WorkshopTool_Decibel::NUM_QTY] =
    {
        WorkshopKey::WORKSHOP_DECIBEL_NAME_INPUT,
        WorkshopKey::WORKSHOP_DECIBEL_NAME_OUTPUT,
        WorkshopKey::WORKSHOP_DECIBEL_NAME_LEVEL};

/** @brief Basiseinheiten Spannungssektion. */
const char *const WorkshopTool_Decibel::QTY_UNITS_V[WorkshopTool_Decibel::NUM_QTY] =
    {
        "V", "V", "dB"};

/** @brief Basiseinheiten Leistungssektion. */
const char *const WorkshopTool_Decibel::QTY_UNITS_P[WorkshopTool_Decibel::NUM_QTY] =
    {
        "W", "W", "dB"};

/** @brief Sektionskopf-Titel je Sektion (lokalisierbar). */
const WorkshopKey WorkshopTool_Decibel::SECTION_TITLE_KEYS[WorkshopTool_Decibel::NUM_SEC] =
    {
        WorkshopKey::WORKSHOP_DECIBEL_SECTION_VOLTAGE,
        WorkshopKey::WORKSHOP_DECIBEL_SECTION_POWER};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Ruft clearAll() auf, da PSRAM nicht vorinitialisiert ist.
 */
WorkshopTool_Decibel::WorkshopTool_Decibel()
{
    clearAll();
}

/**
 * @brief Setzt den Rechner beim Modul-Einstieg zurück.
 */
void WorkshopTool_Decibel::onEnter(LGFX * /*tft*/)
{
    clearAll();
}

/**
 * @brief Initialisiert alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_Decibel::clearAll()
{
    memset(m_sections, 0, sizeof(m_sections));
    memset(m_headerY, 0, sizeof(m_headerY));
    memset(m_fieldRects, 0, sizeof(m_fieldRects));

    m_sprite = nullptr;
    m_originX = 0;
    m_originY = 0;
    m_w = 0;
    m_h = 0;
    m_layoutValid = false;
    m_activeSection = 0;
    m_numpad.clearLayout();
    m_pressedSecIdx = -1;
    m_pressedField = -1;
    m_pressedKey = -1;
    m_touchWas = false;
    m_touchReady = false;

    // Sektion Spannung: V1=1 V (Slot 0, ältester), V2=2 V (Slot 1, neuester), fokussiert auf V2, dB wird berechnet
    Section &sv = m_sections[0];
    sv.fifoSlots[0] = 0; // V1 - ältester Eingabe-Slot
    sv.fifoSlots[1] = 1; // V2 - neuester Eingabe-Slot, fokussiert
    sv.computedQty = 2;  // dB berechnet
    sv.focusedQty = 1;   // V2 fokussiert
    sv.values[0] = 1.0;  // V1 = 1 V
    sv.values[1] = 2.0;  // V2 = 2 V
    formatEditValue(sv.values[0], sv.inputBuf[0], FIELD_CAP);
    formatEditValue(sv.values[1], sv.inputBuf[1], FIELD_CAP);
    compute(sv, true); // dB = 20 * log10(2/1) ≈ 6,02

    // Sektion Leistung: P1=1 W (Slot 0, ältester), P2=2 W (Slot 1, neuester), fokussiert auf P2, dB wird berechnet
    Section &sp = m_sections[1];
    sp.fifoSlots[0] = 0; // P1 - ältester Eingabe-Slot
    sp.fifoSlots[1] = 1; // P2 - neuester Eingabe-Slot, fokussiert
    sp.computedQty = 2;  // dB berechnet
    sp.focusedQty = 1;   // P2 fokussiert
    sp.values[0] = 1.0;  // P1 = 1 W
    sp.values[1] = 2.0;  // P2 = 2 W
    formatEditValue(sp.values[0], sp.inputBuf[0], FIELD_CAP);
    formatEditValue(sp.values[1], sp.inputBuf[1], FIELD_CAP);
    compute(sp, false); // dB = 10 * log10(2/1) ≈ 3,01
}

// ===================================================================
// Öffentliche Interface-Methoden
// ===================================================================

/**
 * @brief Zeichnet beide Sektionen und das Nummernpad vollständig.
 */
void WorkshopTool_Decibel::draw(Gfx_Sprite *sprite,
                                int originX, int originY,
                                int w, int h)
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

    // Sektionsköpfe und Felder teilen einen Font-Load
    m_sprite->loadFont(ComfortaaBold16);

    for (int s = 0; s < NUM_SEC; ++s)
        drawSectionHeader(s, m_headerY[s], s == m_activeSection);

    for (int s = 0; s < NUM_SEC; ++s)
    {
        for (int q = 0; q < NUM_QTY; ++q)
        {
            const bool pressed = (m_pressedSecIdx == s && m_pressedField == q);
            drawField(m_sections[s], s, q, m_fieldRects[s][q].y, m_w, pressed, cursorOn);
        }
    }

    m_sprite->unloadFont();

    m_numpad.draw(m_sprite, m_originX, m_originY, m_pressedKey);
}

/**
 * @brief Verarbeitet Touch-Events mit Touch-up-Semantik.
 */
bool WorkshopTool_Decibel::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        // Touch-Schutz: erst nach dem ersten sauberen touched=false-Frame
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        int hitSec, hitQty;
        if (hitTestField(cx, cy, hitSec, hitQty))
        {
            if (hitSec != m_pressedSecIdx || hitQty != m_pressedField)
            {
                m_pressedSecIdx = hitSec;
                m_pressedField = hitQty;
                m_pressedKey = -1;
                needsRedraw = true;
            }
        }
        else
        {
            const int hitKey = m_numpad.hitTest(cx, cy);
            if (hitKey >= 0 && hitKey != m_pressedKey)
            {
                m_pressedKey = hitKey;
                m_pressedSecIdx = -1;
                m_pressedField = -1;
                needsRedraw = true;
            }
        }

        m_touchWas = true;
    }
    else
    {
        // Erstes touched=false-Frame: Touch-ups sind ab jetzt wirksam
        m_touchReady = true;

        if (m_touchWas)
        {
            if (m_pressedSecIdx >= 0 && m_pressedField >= 0)
            {
                const int tappedSec = m_pressedSecIdx;
                const int tappedQty = m_pressedField;
                Section &sec = m_sections[tappedSec];

                // Aktive Sektion wechseln
                m_activeSection = tappedSec;

                if (isInput(sec, tappedQty))
                {
                    // Fokus auf das bereits als Eingabe aktive Feld verschieben
                    sec.focusedQty = static_cast<uint8_t>(tappedQty);
                }
                else
                {
                    // FIFO-2-Verdrängung: ältester Slot wird berechnet. sec.fifoSlots[0] wird zur neuen berechneten Größe, sec.fifoSlots[1] rückt auf [0], tapped kommt als [1].
                    sec.computedQty = sec.fifoSlots[0];
                    sec.fifoSlots[0] = sec.fifoSlots[1];
                    sec.fifoSlots[1] = static_cast<uint8_t>(tappedQty);
                    sec.focusedQty = static_cast<uint8_t>(tappedQty);

                    // Eingabepuffer mit dem zuletzt berechneten Wert vorbelegen
                    if (sec.computeOk)
                        formatEditValue(sec.values[tappedQty], sec.inputBuf[tappedQty], FIELD_CAP);
                    else
                        sec.inputBuf[tappedQty][0] = '\0';

                    compute(sec, tappedSec == 0);
                }

                needsRedraw = true;
            }
            else if (m_pressedKey >= 0 && m_pressedKey < WorkshopNumpad::NUM_KEYS)
            {
                executeKey(WorkshopNumpad::keyAction(m_pressedKey));
                needsRedraw = true;
            }
        }

        m_pressedSecIdx = -1;
        m_pressedField = -1;
        m_pressedKey = -1;
        m_touchWas = false;
    }

    return needsRedraw;
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet alle Feld-Rechtecke und das Numpad-Layout.
 */
void WorkshopTool_Decibel::computeLayout(int w, int h)
{
    // Y-Positionen der Sektionsköpfe (mit TOP_GAP-Versatz)
    m_headerY[0] = TOP_GAP;
    m_headerY[1] = TOP_GAP + SEC_ROWS_H + SECTION_GAP;

    // Feld-Rechtecke beider Sektionen (content-relativ)
    for (int s = 0; s < NUM_SEC; ++s)
    {
        const int firstRowY = m_headerY[s] + SECTION_HEADER_H;
        for (int q = 0; q < NUM_QTY; ++q)
            m_fieldRects[s][q] = {0, firstRowY + q * ROW_H, w, ROW_H};
    }

    // Nummernpad: bottom-anchored, belegt die unteren REQUIRED_H Pixel
    m_numpad.computeLayout(w, h - WorkshopNumpad::REQUIRED_H);
}

// ===================================================================
// Berechnung
// ===================================================================

/**
 * @brief Parst Eingabepuffer und berechnet die Ausgabegröße einer Sektion.
 */
void WorkshopTool_Decibel::compute(Section &sec, bool isVoltage)
{
    // computeOk vor allen frühen Returns zurücksetzen
    sec.computeOk = false;

    // Eingabewerte der zwei FIFO-Slots aus Puffern lesen
    for (int s = 0; s < NUM_FIFO; ++s)
    {
        const int q = static_cast<int>(sec.fifoSlots[s]);
        sec.values[q] = (sec.inputBuf[q][0] != '\0') ? atof(sec.inputBuf[q]) : 0.0;
    }

    // Finite-Check aller Eingaben
    for (int s = 0; s < NUM_FIFO; ++s)
    {
        if (!std::isfinite(sec.values[static_cast<int>(sec.fifoSlots[s])]))
            return;
    }

    // Skalierungsfaktor: 20 für Spannungspegel, 10 für Leistungspegel
    const double factor = isVoltage ? 20.0 : 10.0;

    const double x1 = sec.values[0];
    const double x2 = sec.values[1];
    const double db = sec.values[2];

    switch (sec.computedQty)
    {
    case 2: // dB = factor * log10(X2 / X1)
    {
        if (std::abs(x1) < 1.0e-30)
            return;

        const double ratio = x2 / x1;
        if (ratio <= 0.0)
            return; // log10 nur für positive Verhältnisse definiert

        const double result = factor * std::log10(ratio);
        if (!std::isfinite(result))
            return;

        sec.values[2] = result;
        sec.computeOk = true;
        break;
    }
    case 0: // X1 = X2 / 10^(dB / factor)
    {
        const double result = x2 / std::pow(10.0, db / factor);
        if (!std::isfinite(result))
            return;

        sec.values[0] = result;
        sec.computeOk = true;
        break;
    }
    case 1: // X2 = X1 * 10^(dB / factor)
    {
        const double result = x1 * std::pow(10.0, db / factor);
        if (!std::isfinite(result))
            return;

        sec.values[1] = result;
        sec.computeOk = true;
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
 * @brief Formatiert einen SI-Spannungswert als auto-skalierten Anzeigestring.
 */
void WorkshopTool_Decibel::formatVoltage(double val, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    const double absVal = std::abs(val);

    if (absVal == 0.0)
    {
        stringutils::safeCopy(buf, sz, "0 V");
        return;
    }

    double scaled;
    const char *prefix;

    if (absVal < 1.0e-3)
    {
        scaled = val * 1.0e6;
        prefix = "u";
    }
    else if (absVal < 1.0)
    {
        scaled = val * 1.0e3;
        prefix = "m";
    }
    else if (absVal < 1.0e3)
    {
        scaled = val;
        prefix = "";
    }
    else
    {
        scaled = val / 1.0e3;
        prefix = "k";
    }

    stringutils::safeFormat(buf, sz, "%.4g %sV", scaled, prefix);
    dotToComma(buf);
}

/**
 * @brief Formatiert einen SI-Leistungswert als auto-skalierten Anzeigestring.
 */
void WorkshopTool_Decibel::formatPower(double val, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    const double absVal = std::abs(val);

    if (absVal == 0.0)
    {
        stringutils::safeCopy(buf, sz, "0 W");
        return;
    }

    double scaled;
    const char *prefix;

    if (absVal < 1.0e-3)
    {
        scaled = val * 1.0e6;
        prefix = "u";
    }
    else if (absVal < 1.0)
    {
        scaled = val * 1.0e3;
        prefix = "m";
    }
    else if (absVal < 1.0e3)
    {
        scaled = val;
        prefix = "";
    }
    else
    {
        scaled = val / 1.0e3;
        prefix = "k";
    }

    stringutils::safeFormat(buf, sz, "%.4g %sW", scaled, prefix);
    dotToComma(buf);
}

/**
 * @brief Formatiert einen dB-Wert als Anzeigestring.
 */
void WorkshopTool_Decibel::formatDb(double val, char *buf, size_t sz)
{
    if (!buf || sz == 0)
        return;

    if (!std::isfinite(val))
    {
        stringutils::safeCopy(buf, sz, "---");
        return;
    }

    stringutils::safeFormat(buf, sz, "%.2f dB", val);
    dotToComma(buf);
}

/**
 * @brief Leitet die Formatierung an die fachspezifische Hilfsfunktion weiter.
 */
void WorkshopTool_Decibel::formatDisplayValue(const Section &sec, int qty, bool isVoltage, char *buf, size_t sz)
{
    if (qty == 2)
        formatDb(sec.values[qty], buf, sz);
    else if (isVoltage)
        formatVoltage(sec.values[qty], buf, sz);
    else
        formatPower(sec.values[qty], buf, sz);
}

/**
 * @brief Formatiert einen SI-Rohwert als editierbaren Dezimalstring.
 */
void WorkshopTool_Decibel::formatEditValue(double val, char *buf, size_t sz)
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

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Gibt zurück ob eine Größe als Eingabe-Slot aktiv ist.
 */
bool WorkshopTool_Decibel::isInput(const Section &sec, int qty)
{
    return (static_cast<int>(sec.fifoSlots[0]) == qty || static_cast<int>(sec.fifoSlots[1]) == qty);
}

/**
 * @brief Gibt die Akzentfarbe des linken Streifens einer Größen-Zeile zurück.
 */
uint16_t WorkshopTool_Decibel::fieldAccentColor(int qty)
{
    switch (qty)
    {
    case 0:
        return C_ACCENT_X1; // Eingang - Grün
    case 1:
        return C_ACCENT_X2; // Ausgang - Orange
    default:
        return C_ACCENT_DB; // dB - Cyan-Weiß
    }
}

/**
 * @brief Gibt die Akzentbalken-Farbe des Sektionskopfes zurück.
 */
uint16_t WorkshopTool_Decibel::sectionAccentColor(int secIdx)
{
    return (secIdx == 0) ? C_SEC_ACCENT_V : C_SEC_ACCENT_P;
}

// ===================================================================
// Tastatureingabe
// ===================================================================

/**
 * @brief Führt eine Tastenaktion auf dem fokussierten Feld der aktiven Sektion aus.
 */
void WorkshopTool_Decibel::executeKey(char action)
{
    Section &sec = m_sections[m_activeSection];
    char *const buf = sec.inputBuf[sec.focusedQty];
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
            break; // kein doppelter Dezimalpunkt

        if (len + 2 >= FIELD_CAP)
            break;

        if (len == 0 || (len == 1 && buf[0] == '-'))
        {
            // "0." bzw. "-0." automatisch voranstellen
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
    case 'B': // Backspace
    {
        if (len > 0)
            buf[len - 1] = '\0';

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
            buf[0] = '-';
            buf[1] = '\0';
        }
        else if (buf[0] == '-')
        {
            memmove(buf, buf + 1, len); // len Bytes inkl. '\0'
        }
        else
        {
            if (len + 1 >= FIELD_CAP)
                break;

            memmove(buf + 1, buf, len + 1); // Platz für Minuszeichen
            buf[0] = '-';
        }
        break;
    }
    default:
        break;
    }

    compute(sec, m_activeSection == 0);
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet den Sektionskopf einer Sektion.
 */
void WorkshopTool_Decibel::drawSectionHeader(int secIdx, int headerY, bool active)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ax = m_originX;
    const int ay = m_originY + headerY;

    // Hintergrund
    spr->fillRect(ax, ay, m_w, SECTION_HEADER_H, C_SEC_HDR_BG);

    // Trennlinie oberhalb (außer bei der ersten Sektion)
    if (secIdx > 0)
        spr->fillRect(ax, ay - 1, m_w, 1, C_SEP);

    // Akzentbalken am linken Rand für aktive Sektion
    if (active)
        spr->fillRect(ax, ay, HEADER_ACCENT_W, SECTION_HEADER_H, sectionAccentColor(secIdx));

    // Sektionsname
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(active ? C_SEC_HDR_TXT_ACT : C_SEC_HDR_TXT, C_SEC_HDR_BG);
    spr->drawString(I18n::str(SECTION_TITLE_KEYS[secIdx]), ax + HEADER_ACCENT_W + 8, ay + SECTION_HEADER_H / 2);
}

/**
 * @brief Zeichnet eine einzelne Größen-Zeile.
 */
void WorkshopTool_Decibel::drawField(const Section &sec, int secIdx, int qty, int fieldY, int w, bool pressed, bool cursorOn)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ax = m_originX;
    const int ay = m_originY + fieldY;

    const bool input = isInput(sec, qty);
    // Fokussiert nur wenn Eingabe, aktive Sektion und fokussiertes Feld
    const bool focused = input && (static_cast<int>(sec.focusedQty) == qty) && (secIdx == m_activeSection);

    uint16_t bgColor;
    if (pressed)
        bgColor = C_FIELD_PRESS;
    else if (focused)
        bgColor = C_FIELD_FOCUS;
    else
        bgColor = C_FIELD_INPUT;

    spr->fillRect(ax, ay, w, ROW_H, bgColor);

    // Trennlinie am unteren Rand
    spr->fillRect(ax, ay + ROW_H - 1, w, 1, C_SEP);

    // Farbakzent-Streifen am linken Rand
    spr->fillRect(ax, ay, ACCENT_W, ROW_H - 1, fieldAccentColor(qty));

    // Textfarben abhängig vom Feld-Zustand
    uint16_t lblColor, nameColor, unitColor;
    if (focused)
    {
        lblColor = C_LBL_FOCUS;
        nameColor = C_NAME_FOCUS;
        unitColor = C_UNIT_FOCUS;
    }
    else if (input)
    {
        lblColor = C_LBL_INPUT;
        nameColor = C_NAME_INPUT;
        unitColor = C_UNIT_INPUT;
    }
    else
    {
        lblColor = C_LBL_COMP;
        nameColor = C_NAME_COMP;
        unitColor = C_UNIT_COMP;
    }

    const int cy = ay + ROW_H / 2;

    // Kurzsymbol (V1/V2/dB bzw. P1/P2/dB) - ML_DATUM für einheitlichen Textstart
    const char *const *symbols = (secIdx == 0) ? QTY_SYMBOLS_V : QTY_SYMBOLS_P;
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblColor, bgColor);
    spr->drawString(symbols[qty], ax + ACCENT_W + 6, cy);

    // Langname
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(nameColor, bgColor);
    spr->drawString(I18n::str(QTY_NAME_KEYS[qty]), ax + LABEL_W + 4, cy);

    // Einheit - zentriert in der Einheiten-Spalte ganz rechts
    const char *const *units = (secIdx == 0) ? QTY_UNITS_V : QTY_UNITS_P;
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(unitColor, bgColor);
    spr->drawString(units[qty], ax + w - UNIT_W / 2, cy);

    // Wert - rechtsbündig im Mittelteil
    char displayBuf[32];
    const int valRightX = ax + w - UNIT_W - 6;

    if (focused)
    {
        // Eingabepuffer hält weiterhin '.' für atof - Komma nur in der Anzeige
        stringutils::safeCopy(displayBuf, sizeof(displayBuf), sec.inputBuf[qty]);
        dotToComma(displayBuf);
        stringutils::safeAppend(displayBuf, sizeof(displayBuf), "_");

        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_VAL_FOCUS, bgColor);
        spr->drawString(displayBuf, valRightX, cy);

        // Cursor ausgeblendet: per fillRect pixelgenau überdecken
        if (!cursorOn)
        {
            const int cursorW = spr->textWidth("_");
            const int cursorH = ROW_H - 2;
            spr->fillRect(valRightX - cursorW, ay + 1, cursorW, cursorH, bgColor);
        }
    }
    else if (input)
    {
        formatDisplayValue(sec, qty, secIdx == 0, displayBuf, sizeof(displayBuf));
        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_VAL_INPUT, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
    else
    {
        if (sec.computeOk)
            formatDisplayValue(sec, qty, secIdx == 0, displayBuf, sizeof(displayBuf));
        else
            stringutils::safeCopy(displayBuf, sizeof(displayBuf), "---");

        spr->setTextDatum(MR_DATUM);
        spr->setTextColor(C_VAL_COMP, bgColor);
        spr->drawString(displayBuf, valRightX, cy);
    }
}

// ===================================================================
// Treffertests
// ===================================================================

/**
 * @brief Treffertest auf alle Felder beider Sektionen.
 */
bool WorkshopTool_Decibel::hitTestField(int cx, int cy, int &outSecIdx, int &outQty) const
{
    for (int s = 0; s < NUM_SEC; ++s)
    {
        for (int q = 0; q < NUM_QTY; ++q)
        {
            const Rect &r = m_fieldRects[s][q];
            if (cx >= r.x && cx < r.x + r.w &&
                cy >= r.y && cy < r.y + r.h)
            {
                outSecIdx = s;
                outQty = q;
                return true;
            }
        }
    }
    return false;
}
