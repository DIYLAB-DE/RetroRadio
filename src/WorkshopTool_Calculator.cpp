#include "WorkshopTool_Calculator.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold16.h"

// ===================================================================
// Statische Datenbasis
// ===================================================================

/**
 * @brief Tastenbelegung - 5×5 Grid.
 *
 * Reihe 0: C    ⌫    (    )    / Reihe 1: 7    8    9    √    * Reihe 2: 4    5    6    x²   - Reihe 3: 1    2    3    π    + Reihe 4: +/-  0    .    %    =
 */
const WorkshopTool_Calculator::KeyEntry WorkshopTool_Calculator::KEYS[NUM_BUTTONS] = {
    // Reihe 0
    {"C", 'C', BtnType::CLEAR},     // 0
    {"BS", 'B', BtnType::MODIFIER}, // 1
    {"(", '(', BtnType::MODIFIER},  // 2
    {")", ')', BtnType::MODIFIER},  // 3
    {"/", '/', BtnType::OPERATOR},  // 4
    // Reihe 1
    {"7", '7', BtnType::DIGIT},    // 5
    {"8", '8', BtnType::DIGIT},    // 6
    {"9", '9', BtnType::DIGIT},    // 7
    {"√", 'R', BtnType::FUNCTION}, // 8
    {"*", '*', BtnType::OPERATOR}, // 9
    // Reihe 2
    {"4", '4', BtnType::DIGIT},     // 10
    {"5", '5', BtnType::DIGIT},     // 11
    {"6", '6', BtnType::DIGIT},     // 12
    {"x²", 'Q', BtnType::FUNCTION}, // 13
    {"-", '-', BtnType::OPERATOR},  // 14
    // Reihe 3
    {"1", '1', BtnType::DIGIT},    // 15
    {"2", '2', BtnType::DIGIT},    // 16
    {"3", '3', BtnType::DIGIT},    // 17
    {"π", 'P', BtnType::FUNCTION}, // 18
    {"+", '+', BtnType::OPERATOR}, // 19
    // Reihe 4
    {"+/-", '~', BtnType::MODIFIER}, // 20
    {"0", '0', BtnType::DIGIT},      // 21
    {".", '.', BtnType::DOT},        // 22
    {"%", '%', BtnType::MODIFIER},   // 23
    {"=", '=', BtnType::EQUALS},     // 24
};

// ===================================================================
// Konstruktor / Initialisierung
// ===================================================================

/**
 * @brief Konfiguriert alle drei DotMatrix-Instanzen mit LCD-Farbschema.
 */
WorkshopTool_Calculator::WorkshopTool_Calculator()
{
    m_dotDez.init(4, 4, 1, 3);
    m_dotDez.setOnColor(LCD_ON_COLOR, LCD_ON_COLOR);
    m_dotDez.setOffColor(LCD_OFF_COLOR, LCD_OFF_COLOR);
    m_dotDez.setBackgroundColor(DISP_BG);
    m_dotDez.setShowOffDots(true);
    m_dotDez.setDotShape(DotMatrix::DotShape::SQUARE);
    m_dotDez.setGradientEnabled(false);

    m_dotHex.init(4, 4, 1, 3);
    m_dotHex.setOnColor(LCD_ON_COLOR, LCD_ON_COLOR);
    m_dotHex.setOffColor(LCD_OFF_COLOR, LCD_OFF_COLOR);
    m_dotHex.setBackgroundColor(DISP_BG);
    m_dotHex.setShowOffDots(true);
    m_dotHex.setDotShape(DotMatrix::DotShape::SQUARE);
    m_dotHex.setGradientEnabled(false);

    m_dotBin.init(4, 4, 1, 3);
    m_dotBin.setOnColor(LCD_ON_COLOR, LCD_ON_COLOR);
    m_dotBin.setOffColor(LCD_OFF_COLOR, LCD_OFF_COLOR);
    m_dotBin.setBackgroundColor(DISP_BG);
    m_dotBin.setShowOffDots(true);
    m_dotBin.setDotShape(DotMatrix::DotShape::SQUARE);
    m_dotBin.setGradientEnabled(false);

    // PSRAM explizit in definierten Startzustand setzen
    clearAll();
}

/**
 * @brief Reserviert Textpuffer und setzt den Rechner in den Startzustand.
 */
void WorkshopTool_Calculator::onEnter(LGFX * /*tft*/)
{
    m_dotDez.reserveText(DOTS_FIELD_CHARS + 1);
    m_dotHex.reserveText(DOTS_FIELD_CHARS + 1);
    m_dotBin.reserveText(DOTS_FIELD_CHARS + 1);
    clearAll();
}

// ===================================================================
// Öffentliche Interface-Methoden
// ===================================================================

/**
 * @brief Zeichnet Anzeige und Tastatur.
 */
void WorkshopTool_Calculator::draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
{
    if (!m_layoutValid || m_w != w || m_h != h)
    {
        m_w = w;
        m_h = h;
        computeLayout(w, h);
        m_layoutValid = true;
    }

    m_originX = originX;
    m_originY = originY;

    if (m_valueDirty)
    {
        buildTrace();
        updateDisplay();
        updateDisplayStrings();
        m_valueDirty = false;
    }

    drawDisplay(sprite, originX, originY, w);
    drawButtons(sprite, originX, originY);
}

/**
 * @brief Verarbeitet Touch-Events mit Touch-down-Feedback und Touch-up-Auslösung.
 */
bool WorkshopTool_Calculator::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        // Erst nach einem sauberen touched=false Frame dürfen Touch-Ups Aktionen auslösen
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;
        const int hit = hitTest(cx, cy);

        if (hit != m_pressedBtn)
        {
            m_pressedBtn = hit;
            needsRedraw = true;
        }

        m_touchWas = true;
    }
    else
    {
        // Erstes touched=false Frame nach dem Booten - ab jetzt ist Touch bereit
        m_touchReady = true;

        if (m_pressedBtn >= 0 && m_touchWas)
        {
            executeAction(m_pressedBtn);
            needsRedraw = true;
        }

        m_pressedBtn = -1;
        m_touchWas = false;
    }

    return needsRedraw;
}

// ===================================================================
// Layout-Berechnung
// ===================================================================

/**
 * @brief Berechnet alle Tastenrechtecke als reines 5×5-Grid.
 */
void WorkshopTool_Calculator::computeLayout(int w, int h)
{
    // rowH aus dem verfügbaren Platz berechnen (Displaybereich + Mindestabstand oben)
    const int btnAreaH = h - (EXPR_H + DISPLAY_H + BTN_DISPLAY_GAP);
    const int rowH = (btnAreaH - (BTN_ROWS - 1) * BTN_GAP) / BTN_ROWS;
    const int colW = (w - (BTN_COLS - 1) * BTN_GAP) / BTN_COLS;

    // Die Tasten werden unten verankert, sodass der Restplatz zwischen Anzeige und Tastatur liegt. Der äußere TITLE_MARGIN von 10 px bildet den gesamten unteren Abstand.
    const int btnsTotalH = BTN_ROWS * rowH + (BTN_ROWS - 1) * BTN_GAP;
    const int btnAreaY = h - btnsTotalH;

    for (int r = 0; r < BTN_ROWS; ++r)
    {
        const int y = btnAreaY + r * (rowH + BTN_GAP);
        for (int c = 0; c < BTN_COLS; ++c)
        {
            const int idx = r * BTN_COLS + c;
            m_btnRects[idx].x = c * (colW + BTN_GAP);
            m_btnRects[idx].y = y;
            m_btnRects[idx].w = colW;
            m_btnRects[idx].h = rowH;
        }
    }
}

// ===================================================================
// Treffertest
// ===================================================================

/**
 * @brief Treffertest für inhaltsbereich-relative Koordinaten.
 */
int WorkshopTool_Calculator::hitTest(int cx, int cy) const
{
    for (int i = 0; i < NUM_BUTTONS; ++i)
    {
        const BtnRect &r = m_btnRects[i];
        if (cx >= r.x && cx < r.x + r.w &&
            cy >= r.y && cy < r.y + r.h)
        {
            return i;
        }
    }
    return -1;
}

// ===================================================================
// Eingabeverarbeitung
// ===================================================================

/**
 * @brief Führt die Aktion einer Taste aus.
 */
void WorkshopTool_Calculator::executeAction(int btnIdx)
{
    if (btnIdx < 0 || btnIdx >= NUM_BUTTONS)
        return;

    const KeyEntry &k = KEYS[btnIdx];

    switch (k.type)
    {
    case BtnType::DIGIT:
    case BtnType::DOT:
        appendInput(k.action);
        break;

    case BtnType::OPERATOR:
        appendInput(k.action);
        break;

    case BtnType::MODIFIER:
        switch (k.action)
        {
        case '(':
            openParen();
            break;
        case ')':
            closeParen();
            break;
        case 'B':
            backspace();
            break;
        case '~':
            toggleSign();
            break;
        case '%':
            applyPercent();
            break;
        default:
            break;
        }
        break;

    case BtnType::FUNCTION:
        switch (k.action)
        {
        case 'R':
            applySquareRoot();
            break;
        case 'Q':
            applySquare();
            break;
        case 'P':
            appendPi();
            break;
        default:
            break;
        }
        break;

    case BtnType::CLEAR:
        clearAll();
        break;

    case BtnType::EQUALS:
        executeEquals();
        break;
    }
}

/**
 * @brief Kernfunktion der Zeicheneingabe (Ziffern, '.', Operatoren).
 */
void WorkshopTool_Calculator::appendInput(char value)
{
    if (value == '\0')
        return;

    // Ziffer
    if (value >= '0' && value <= '9')
    {
        if (m_justEvaluated)
        {
            m_expression[0] = '\0';
            m_justEvaluated = false;
        }

        const char lastNow = exprLastChar(m_expression);

        // Implizite Multiplikation nach schließender Klammer
        if (lastNow == ')')
            stringutils::safeAppend(m_expression, "*");

        // Führende Null ersetzen: "0" -> neue Ziffer (nicht bei "0.")
        const size_t lenNow = strlen(m_expression);
        if (lenNow > 0 && value != '0')
        {
            const char prevChar = (lenNow >= 2) ? m_expression[lenNow - 2] : '\0';
            const bool prevIsStart = (lenNow == 1) || isOperatorChar(prevChar) || prevChar == '(';

            if (m_expression[lenNow - 1] == '0' && prevIsStart)
            {
                m_expression[lenNow - 1] = value;
                m_valueDirty = true;
                return;
            }
        }

        const char tmp[2] = {value, '\0'};
        stringutils::safeAppend(m_expression, tmp);
        m_valueDirty = true;
        return;
    }

    // Dezimalpunkt
    if (value == '.')
    {
        if (m_justEvaluated)
        {
            m_expression[0] = '\0';
            m_justEvaluated = false;
        }

        // Prüfen ob die aktuelle Zahl bereits einen Punkt hat
        bool hasDot = false;
        for (int i = static_cast<int>(strlen(m_expression)) - 1; i >= 0; --i)
        {
            if (m_expression[i] == '.')
            {
                hasDot = true;
                break;
            }

            if (isOperatorChar(m_expression[i]) || m_expression[i] == '(' || m_expression[i] == ')')
                break;
        }
        if (hasDot)
            return;

        const char lastNow = exprLastChar(m_expression);
        if (lastNow == ')')
            stringutils::safeAppend(m_expression, "*0.");
        else if (lastNow == '\0' || isOperatorChar(lastNow) || lastNow == '(')
            stringutils::safeAppend(m_expression, "0.");
        else
            stringutils::safeAppend(m_expression, ".");

        m_valueDirty = true;
        return;
    }

    // Operator
    if (isOperatorChar(value))
    {
        if (m_justEvaluated)
            m_justEvaluated = false; // Ergebnis behalten, Operator anhängen

        const char lastNow = exprLastChar(m_expression);

        if (m_expression[0] == '\0')
        {
            // Leerer Ausdruck: nur unäres '-' erlaubt
            if (value == '-')
                stringutils::safeAppend(m_expression, "-");
            return;
        }

        if (isOperatorChar(lastNow))
        {
            // '-' darf als unäres Vorzeichen angehängt werden, alle anderen Operatoren ersetzen den letzten
            if (value == '-' && lastNow != '-')
            {
                stringutils::safeAppend(m_expression, "-");
            }
            else
            {
                const size_t lenNow = strlen(m_expression);
                m_expression[lenNow - 1] = value;
            }
        }
        else if (lastNow == '(')
        {
            if (value == '-')
                stringutils::safeAppend(m_expression, "-");
        }
        else
        {
            const char tmp[2] = {value, '\0'};
            stringutils::safeAppend(m_expression, tmp);
        }

        m_valueDirty = true;
    }
}

/**
 * @brief Hängt eine öffnende Klammer an.
 */
void WorkshopTool_Calculator::openParen()
{
    if (m_justEvaluated)
    {
        m_expression[0] = '\0';
        m_justEvaluated = false;
    }

    const char last = exprLastChar(m_expression);

    // Implizite Multiplikation nach Ziffer, Punkt oder ')'
    if ((last >= '0' && last <= '9') || last == '.' || last == ')')
        stringutils::safeAppend(m_expression, "*");

    stringutils::safeAppend(m_expression, "(");
    m_valueDirty = true;
}

/**
 * @brief Hängt eine schließende Klammer an.
 */
void WorkshopTool_Calculator::closeParen()
{
    const size_t len = strlen(m_expression);

    // Offene Klammern zählen
    const int opens = static_cast<int>(std::count(m_expression, m_expression + len, '('));
    const int closes = static_cast<int>(std::count(m_expression, m_expression + len, ')'));

    if (opens <= closes)
        return; // keine offene Klammer vorhanden

    const char last = exprLastChar(m_expression);
    if (!isNumericChar(last) && last != ')')
        return; // nur nach Zahl oder schließender Klammer

    stringutils::safeAppend(m_expression, ")");
    m_valueDirty = true;
}

/**
 * @brief Löscht das letzte Zeichen.
 */
void WorkshopTool_Calculator::backspace()
{
    const size_t len = strlen(m_expression);

    if (len == 0)
        return;

    // Nach = : kompletten Ausdruck löschen
    if (m_justEvaluated)
    {
        clearAll();
        return;
    }

    m_expression[len - 1] = '\0';
    m_valueDirty = true;
}

/**
 * @brief Negiert den letzten Zahlenwert im Ausdruck.
 */
void WorkshopTool_Calculator::toggleSign()
{
    const size_t len = strlen(m_expression);

    if (len == 0)
    {
        stringutils::safeAppend(m_expression, "-");
        m_valueDirty = true;
        return;
    }

    // Nach = : Gesamtergebnis negieren
    if (m_justEvaluated)
    {
        if (m_expression[0] == '-')
            exprEraseAt(m_expression, 0);
        else
            exprInsertAt(m_expression, EXPR_CAP, 0, '-');
        m_justEvaluated = false;
        m_valueDirty = true;
        return;
    }

    const char last = exprLastChar(m_expression);

    // Kein Zahlenwert am Ende: Vorzeichen oder Klammer anhängen
    if (!isNumericChar(last))
    {
        if (isOperatorChar(last) || last == '(')
            stringutils::safeAppend(m_expression, "-");

        m_valueDirty = true;
        return;
    }

    // Rückwärts über die letzte Zahl scannen
    int end = static_cast<int>(len) - 1;
    int start = end;
    while (start > 0 && isNumericChar(m_expression[start - 1]))
        --start;

    // Prüfen ob unäres '-' unmittelbar vor der Zahl steht
    const int minusPos = start - 1;
    const bool hasUnaryMinus = (minusPos >= 0) && (m_expression[minusPos] == '-') && (minusPos == 0 || isOperatorChar(m_expression[minusPos - 1]) || m_expression[minusPos - 1] == '(');

    if (hasUnaryMinus)
    {
        exprEraseAt(m_expression, static_cast<size_t>(minusPos));
    }
    else
    {
        const char before = (start > 0) ? m_expression[start - 1] : '\0';
        if (start == 0 || isOperatorChar(before) || before == '(')
        {
            exprInsertAt(m_expression, EXPR_CAP, static_cast<size_t>(start), '-');
        }
        else
        {
            // Zahl in (-n) einschließen
            char numBuf[32] = "";
            const int numLen = end - start + 1;
            if (numLen > 0 && numLen < static_cast<int>(sizeof(numBuf)) - 4)
            {
                memcpy(numBuf, m_expression + start, static_cast<size_t>(numLen));
                numBuf[numLen] = '\0';
                m_expression[start] = '\0';
                stringutils::safeAppend(m_expression, "(-");
                stringutils::safeAppend(m_expression, numBuf);
                stringutils::safeAppend(m_expression, ")");
            }
        }
    }

    m_valueDirty = true;
}

/**
 * @brief Dividiert die letzte Zahl durch 100.
 */
void WorkshopTool_Calculator::applyPercent()
{
    double val;
    int absStart;
    if (!findLastNumber(m_expression, val, absStart))
        return;

    val /= 100.0;
    m_expression[absStart] = '\0';
    char buf[32];
    formatDecimal(val, buf, sizeof(buf));
    stringutils::safeAppend(m_expression, buf);
    m_valueDirty = true;
}

/**
 * @brief Ersetzt die letzte Zahl durch ihre Quadratwurzel.
 */
void WorkshopTool_Calculator::applySquareRoot()
{
    double val;
    int absStart;
    if (!findLastNumber(m_expression, val, absStart))
        return;

    if (val < 0.0)
        return; // Wurzel aus negativer Zahl nicht definiert

    m_expression[absStart] = '\0';
    char buf[32];
    formatDecimal(std::sqrt(val), buf, sizeof(buf));
    stringutils::safeAppend(m_expression, buf);
    m_valueDirty = true;
}

/**
 * @brief Ersetzt die letzte Zahl durch ihr Quadrat.
 */
void WorkshopTool_Calculator::applySquare()
{
    double val;
    int absStart;
    if (!findLastNumber(m_expression, val, absStart))
        return;

    m_expression[absStart] = '\0';
    char buf[32];
    formatDecimal(val * val, buf, sizeof(buf));
    stringutils::safeAppend(m_expression, buf);
    m_valueDirty = true;
}

/**
 * @brief Hängt den π-Wert an den Ausdruck an.
 */
void WorkshopTool_Calculator::appendPi()
{
    if (m_justEvaluated)
    {
        m_expression[0] = '\0';
        m_justEvaluated = false;
    }

    const char last = exprLastChar(m_expression);

    // Implizite Multiplikation nach Ziffer, Punkt oder ')'
    if ((last >= '0' && last <= '9') || last == '.' || last == ')')
        stringutils::safeAppend(m_expression, "*");

    stringutils::safeAppend(m_expression, "3.14159265358979");
    m_valueDirty = true;
}

/**
 * @brief Wertet den Ausdruck aus und speichert das Ergebnis.
 */
void WorkshopTool_Calculator::executeEquals()
{
    if (m_expression[0] == '\0')
        return;

    double val;
    if (tryEvaluate(m_expression, val))
    {
        // Rechenweg vor dem Überschreiben von m_expression sichern
        buildTrace();

        formatDecimal(val, m_expression, EXPR_CAP);
        m_justEvaluated = true;
    }

    m_valueDirty = true;
}

/**
 * @brief Setzt den Rechner vollständig zurück.
 */
void WorkshopTool_Calculator::clearAll()
{
    // PSRAM Initialisierung
    memset(m_expression, 0, sizeof(m_expression));
    memset(m_traceStr, 0, sizeof(m_traceStr));

    m_resultText[0] = '0';
    m_resultText[1] = '\0';
    m_displayValue = 0.0;
    m_justEvaluated = false;
    m_valueDirty = true;

    // DotMatrix auf Startzustand setzen
    padLeft(m_dezStr, sizeof(m_dezStr), "0", DOTS_FIELD_CHARS);
    padLeft(m_hexStr, sizeof(m_hexStr), nullptr, DOTS_FIELD_CHARS);
    padLeft(m_binStr, sizeof(m_binStr), nullptr, DOTS_FIELD_CHARS);

    m_dotDez.setText(m_dezStr);
    m_dotHex.setText(m_hexStr);
    m_dotBin.setText(m_binStr);
}

// ===================================================================
// Anzeige-Rekonstruktion
// ===================================================================

/**
 * @brief Baut m_traceStr aus m_expression auf.
 */
void WorkshopTool_Calculator::buildTrace()
{
    // Wenn m_justEvaluated gesetzt ist, wurde der Trace bereits in executeEquals() gesichert - nicht überschreiben, er soll bis C sichtbar bleiben
    if (m_justEvaluated)
        return;

    memset(m_traceStr, 0, sizeof(m_traceStr));

    if (m_expression[0] == '\0')
        return;

    size_t dstPos = 0;
    const size_t dstMax = sizeof(m_traceStr) - 1;
    char prev = '\0';

    for (const char *p = m_expression; *p && dstPos < dstMax; ++p)
    {
        const char c = *p;

        if (isOperatorChar(c))
        {
            // Binärer Operator: Leerzeichen links und rechts
            const bool isBinary = (prev != '\0') && (prev != '(') && !isOperatorChar(prev);

            if (isBinary && dstPos + 3 <= dstMax)
            {
                m_traceStr[dstPos++] = ' ';
                m_traceStr[dstPos++] = c;
                m_traceStr[dstPos++] = ' ';
            }
            else if (dstPos < dstMax)
            {
                m_traceStr[dstPos++] = c;
            }
        }
        else
        {
            m_traceStr[dstPos++] = c;
        }

        prev = c;
    }

    m_traceStr[dstPos] = '\0';
}

/**
 * @brief Berechnet m_displayValue und m_resultText aus dem aktuellen Ausdruck.
 */
void WorkshopTool_Calculator::updateDisplay()
{
    if (m_expression[0] == '\0')
    {
        m_displayValue = 0.0;
        stringutils::safeCopy(m_resultText, sizeof(m_resultText), "0");
        return;
    }

    double val;
    if (tryEvaluate(m_expression, val))
    {
        m_displayValue = val;
        formatDecimal(val, m_resultText, sizeof(m_resultText));
        return;
    }

    // Fallback: letzte Zahl im Ausdruck anzeigen
    int absStart;
    if (findLastNumber(m_expression, val, absStart))
    {
        m_displayValue = val;
        formatDecimal(val, m_resultText, sizeof(m_resultText));
        return;
    }

    m_displayValue = 0.0;
    stringutils::safeCopy(m_resultText, sizeof(m_resultText), "0");
}

/**
 * @brief Füllt die DotMatrix-Texte aus m_resultText und m_displayValue.
 */
void WorkshopTool_Calculator::updateDisplayStrings()
{
    padLeft(m_dezStr, sizeof(m_dezStr), m_resultText, DOTS_FIELD_CHARS);

    const double val = m_displayValue;
    const bool isNonNegInt = std::isfinite(val) && (val >= 0.0) && (val <= 4294967295.0) && (val == std::floor(val));

    if (isNonNegInt)
    {
        const unsigned long ival = static_cast<unsigned long>(val);
        char tmp[24];

        stringutils::safeFormat(tmp, sizeof(tmp), "0x%lx", ival);
        padLeft(m_hexStr, sizeof(m_hexStr), tmp, DOTS_FIELD_CHARS);

        if (ival <= 65535U)
        {
            formatBinary(static_cast<unsigned int>(ival), tmp);
            padLeft(m_binStr, sizeof(m_binStr), tmp, DOTS_FIELD_CHARS);
        }
        else
        {
            padLeft(m_binStr, sizeof(m_binStr), nullptr, DOTS_FIELD_CHARS);
        }
    }
    else
    {
        padLeft(m_hexStr, sizeof(m_hexStr), nullptr, DOTS_FIELD_CHARS);
        padLeft(m_binStr, sizeof(m_binStr), nullptr, DOTS_FIELD_CHARS);
    }

    m_dotDez.setText(m_dezStr);
    m_dotHex.setText(m_hexStr);
    m_dotBin.setText(m_binStr);
}

// ===================================================================
// Auswertungsalgorithmus
// ===================================================================

/**
 * @brief Wertet einen mathematischen Ausdrucks-String aus.
 */
bool WorkshopTool_Calculator::tryEvaluate(const char *expr, double &outVal)
{
    if (!expr || expr[0] == '\0')
        return false;

    // Statische Puffer
    static CalcToken tokens[TOKEN_CAP];
    static CalcToken rpn[TOKEN_CAP];
    int tokenCount = 0;
    int rpnCount = 0;

    if (!calcTokenize(expr, tokens, TOKEN_CAP, tokenCount))
        return false;

    if (!calcToRpn(tokens, tokenCount, rpn, TOKEN_CAP, rpnCount))
        return false;

    if (!calcEvalRpn(rpn, rpnCount, outVal))
        return false;

    return std::isfinite(outVal);
}

/**
 * @brief Zerlegt den Ausdruck in Token.
 */
bool WorkshopTool_Calculator::calcTokenize(const char *expr, CalcToken *tokens, int cap, int &outCount)
{
    outCount = 0;
    if (!expr || !tokens || cap <= 0)
        return false;

    for (const char *p = expr; *p != '\0';)
    {
        const char ch = *p;

        // Zahl
        if ((ch >= '0' && ch <= '9') || ch == '.')
        {
            if (outCount >= cap)
                return false;

            char numBuf[48];
            int n = 0, dotCnt = 0;

            while (*p && ((*p >= '0' && *p <= '9') || *p == '.'))
            {
                if (*p == '.' && ++dotCnt > 1)
                    return false;

                if (n + 1 >= static_cast<int>(sizeof(numBuf)))
                    return false;

                numBuf[n++] = *p++;
            }
            numBuf[n] = '\0';

            char *endPtr = nullptr;
            const double number = strtod(numBuf, &endPtr);
            if (!endPtr || *endPtr != '\0')
                return false;

            tokens[outCount].type = CalcToken::NUMBER;
            tokens[outCount].op = 0;
            tokens[outCount].number = number;
            ++outCount;
            continue;
        }

        // Klammer
        if (ch == '(' || ch == ')')
        {
            if (outCount >= cap)
                return false;

            tokens[outCount].type = CalcToken::PAREN;
            tokens[outCount].op = ch;
            tokens[outCount].number = 0.0;
            ++outCount;
            ++p;
            continue;
        }

        // Operator
        if (isOperatorChar(ch))
        {
            if (outCount >= cap)
                return false;

            const bool isUnary =
                (outCount == 0) ||
                (tokens[outCount - 1].type == CalcToken::OPERATOR) ||
                (tokens[outCount - 1].type == CalcToken::PAREN &&
                 tokens[outCount - 1].op == '(');

            tokens[outCount].type = CalcToken::OPERATOR;
            tokens[outCount].number = 0.0;

            if (isUnary && ch == '+')
                tokens[outCount].op = 'P'; // unäres +
            else if (isUnary && ch == '-')
                tokens[outCount].op = 'M'; // unäres -
            else
                tokens[outCount].op = ch;

            ++outCount;
            ++p;
            continue;
        }

        if (ch == ' ')
        {
            ++p;
            continue;
        } // defensiv: Leerzeichen ignorieren

        return false; // unbekanntes Zeichen
    }

    return (outCount > 0);
}

/**
 * @brief Konvertiert Infix-Tokens in Umgekehrte Polnische Notation.
 */
bool WorkshopTool_Calculator::calcToRpn(const CalcToken *tokens, int count, CalcToken *outRpn, int cap, int &outCount)
{
    outCount = 0;
    if (!tokens || !outRpn || cap <= 0)
        return false;

    auto prec = [](char op) -> int
    {
        switch (op)
        {
        case 'P':
        case 'M':
            return 3;
        case '*':
        case '/':
            return 2;
        case '+':
        case '-':
            return 1;
        default:
            return 0;
        }
    };
    auto isRightAssoc = [](char op) -> bool
    { return op == 'P' || op == 'M'; };

    CalcToken opStack[TOKEN_CAP];
    int opCount = 0;

    for (int i = 0; i < count; ++i)
    {
        const CalcToken &t = tokens[i];

        if (t.type == CalcToken::NUMBER)
        {
            if (outCount >= cap)
                return false;

            outRpn[outCount++] = t;
            continue;
        }

        if (t.type == CalcToken::OPERATOR)
        {
            while (opCount > 0)
            {
                const CalcToken &top = opStack[opCount - 1];
                if (top.type != CalcToken::OPERATOR)
                    break;

                const bool shouldPop = isRightAssoc(t.op)
                                           ? (prec(t.op) < prec(top.op))
                                           : (prec(t.op) <= prec(top.op));

                if (!shouldPop)
                    break;

                if (outCount >= cap)
                    return false;

                outRpn[outCount++] = opStack[--opCount];
            }
            if (opCount >= TOKEN_CAP)
                return false;
            opStack[opCount++] = t;
            continue;
        }

        if (t.type == CalcToken::PAREN && t.op == '(')
        {
            if (opCount >= TOKEN_CAP)
                return false;

            opStack[opCount++] = t;
            continue;
        }

        if (t.type == CalcToken::PAREN && t.op == ')')
        {
            bool foundOpen = false;
            while (opCount > 0)
            {
                const CalcToken top = opStack[--opCount];
                if (top.type == CalcToken::PAREN && top.op == '(')
                {
                    foundOpen = true;
                    break;
                }

                if (outCount >= cap)
                    return false;

                outRpn[outCount++] = top;
            }
            if (!foundOpen)
                return false;
        }
    }

    while (opCount > 0)
    {
        const CalcToken top = opStack[--opCount];
        if (top.type == CalcToken::PAREN)
            return false;

        if (outCount >= cap)
            return false;

        outRpn[outCount++] = top;
    }

    return true;
}

/**
 * @brief Wertet eine RPN-Tokenfolge aus.
 */
bool WorkshopTool_Calculator::calcEvalRpn(const CalcToken *rpn, int count, double &outVal)
{
    double evalStack[TOKEN_CAP];
    int evalCount = 0;

    for (int i = 0; i < count; ++i)
    {
        const CalcToken &t = rpn[i];

        if (t.type == CalcToken::NUMBER)
        {
            if (evalCount >= TOKEN_CAP)
                return false;

            evalStack[evalCount++] = t.number;
            continue;
        }

        if (t.type != CalcToken::OPERATOR)
            return false;

        // Unäre Operatoren
        if (t.op == 'P' || t.op == 'M')
        {
            if (evalCount < 1)
                return false;

            const double a = evalStack[--evalCount];
            evalStack[evalCount++] = (t.op == 'M') ? -a : a;
            continue;
        }

        // Binäre Operatoren
        if (evalCount < 2)
            return false;

        const double b = evalStack[--evalCount];
        const double a = evalStack[--evalCount];
        double result;

        switch (t.op)
        {
        case '+':
            result = a + b;
            break;
        case '-':
            result = a - b;
            break;
        case '*':
            result = a * b;
            break;
        case '/':
            if (std::abs(b) < 1e-15)
                return false;
            result = a / b;
            break;
        default:
            return false;
        }

        if (!std::isfinite(result))
            return false;

        evalStack[evalCount++] = result;
    }

    if (evalCount != 1)
        return false;

    outVal = evalStack[0];
    return true;
}

// ===================================================================
// String-Hilfsfunktionen
// ===================================================================

/**
 * @brief Letztes Zeichen von s.
 */
char WorkshopTool_Calculator::exprLastChar(const char *s)
{
    if (!s || s[0] == '\0')
        return '\0';

    return s[strlen(s) - 1];
}

/**
 * @brief true wenn c ein Rechenoperator ist.
 */
bool WorkshopTool_Calculator::isOperatorChar(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/';
}

/**
 * @brief true wenn c eine Ziffer oder '.' ist.
 */
bool WorkshopTool_Calculator::isNumericChar(char c)
{
    return (c >= '0' && c <= '9') || c == '.';
}

/**
 * @brief Löscht das Zeichen an Position pos.
 */
void WorkshopTool_Calculator::exprEraseAt(char *buf, size_t pos)
{
    const size_t len = strlen(buf);
    if (pos >= len)
        return;

    memmove(buf + pos, buf + pos + 1, len - pos); // inkl. '\0'
}

/**
 * @brief Fügt c an Position pos ein.
 */
bool WorkshopTool_Calculator::exprInsertAt(char *buf, size_t cap, size_t pos, char c)
{
    const size_t len = strlen(buf);
    if (len + 1 >= cap || pos > len)
        return false;

    memmove(buf + pos + 1, buf + pos, len - pos + 1);
    buf[pos] = c;
    return true;
}

/**
 * @brief Ermittelt die letzte Zahl am Ende von expr inkl. vorangehendem unären Minus.
 */
bool WorkshopTool_Calculator::findLastNumber(const char *expr, double &outVal, int &outAbsStart)
{
    if (!expr || expr[0] == '\0')
        return false;

    const int len = static_cast<int>(strlen(expr));
    if (!isNumericChar(expr[len - 1]))
        return false;

    int end = len - 1;
    int start = end;
    while (start > 0 && isNumericChar(expr[start - 1]))
        --start;

    // Vorangehendes unäres Minuszeichen einbeziehen
    const int minusPos = start - 1;
    const bool hasUnaryMinus = (minusPos >= 0) && (expr[minusPos] == '-') && (minusPos == 0 || isOperatorChar(expr[minusPos - 1]) || expr[minusPos - 1] == '(');
    const int absStart = hasUnaryMinus ? minusPos : start;

    // Zahlenwert mit Vorzeichen parsen
    char numBuf[48] = "";
    const int numLen = end - absStart + 1;

    if (numLen <= 0 || numLen >= static_cast<int>(sizeof(numBuf)))
        return false;

    memcpy(numBuf, expr + absStart, static_cast<size_t>(numLen));
    numBuf[numLen] = '\0';

    char *endPtr = nullptr;
    outVal = strtod(numBuf, &endPtr);
    if (!endPtr || endPtr == numBuf)
        return false;

    outAbsStart = absStart;
    return true;
}

/**
 * @brief Formatiert val als kompakte Dezimalzahl.
 */
void WorkshopTool_Calculator::formatDecimal(double val, char *buf, size_t sz)
{
    if (!std::isfinite(val))
    {
        I18n::format(buf, sz, WorkshopKey::WORKSHOP_CALCULATOR_ERROR);
        return;
    }
    if (val == std::floor(val) && val >= -2147483648.0 && val <= 4294967295.0)
    {
        if (val < 0.0)
            stringutils::safeFormat(buf, sz, "%lld", static_cast<long long>(val));
        else
            stringutils::safeFormat(buf, sz, "%lu", static_cast<unsigned long>(val));
    }
    else
    {
        stringutils::safeFormat(buf, sz, "%.10g", val);
    }
}

/**
 * @brief Formatiert val als 16-Bit-Binärstring "0bNNNN...NNNN".
 */
void WorkshopTool_Calculator::formatBinary(unsigned int val, char *buf)
{
    buf[0] = '0';
    buf[1] = 'b';
    for (int bit = 15; bit >= 0; --bit)
        buf[2 + (15 - bit)] = ((val >> bit) & 1U) ? '1' : '0';

    buf[18] = '\0';
}

/**
 * @brief Füllt src links mit Leerzeichen auf totalChars Zeichen auf.
 */
void WorkshopTool_Calculator::padLeft(char *dst, size_t dstSz, const char *src, int totalChars)
{
    const int srcLen = (src && *src) ? static_cast<int>(strlen(src)) : 0;
    const int usedSrc = (srcLen > totalChars) ? totalChars : srcLen;
    const int spaces = totalChars - usedSrc;

    if (static_cast<int>(dstSz) < totalChars + 1)
        return;

    for (int i = 0; i < spaces; ++i)
        dst[i] = ' ';

    if (usedSrc > 0)
        memcpy(dst + spaces, src + (srcLen - usedSrc), static_cast<size_t>(usedSrc));

    dst[totalChars] = '\0';
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet Rechenweg-Zeile und LCD-Anzeigebereich.
 */
void WorkshopTool_Calculator::drawDisplay(Gfx_Sprite *spr, int ox, int oy, int w)
{
    spr->fillRect(ox, oy, w, EXPR_H, WorkshopCfg::C_WORK_BG);

    if (m_traceStr[0] != '\0')
    {
        spr->loadFont(ComfortaaBold16);
        spr->setTextColor(EXPR_COLOR, WorkshopCfg::C_WORK_BG);

        constexpr int SIDE_PAD = 6;
        const int maxW = w - SIDE_PAD * 2;
        const char *ptr = m_traceStr;

        while (*ptr && spr->textWidth(ptr) > maxW)
            ++ptr;

        spr->setTextDatum(MR_DATUM);
        spr->drawString(ptr, ox + w - SIDE_PAD, oy + EXPR_H / 2);
        spr->unloadFont();
    }

    const int lcdY = oy + EXPR_H;
    spr->fillSmoothRoundRect(ox, lcdY, w, DISPLAY_H, DISPLAY_CORNER_R, DISP_BG);

    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(DISP_LABEL_CLR, DISP_BG);
    spr->drawString("DEC", ox + DISPLAY_LBL_X, lcdY + DISP_DEZ_REL_Y + DISP_CHAR_H / 2 + 2);
    spr->drawString("HEX", ox + DISPLAY_LBL_X, lcdY + DISP_HEX_REL_Y + DISP_CHAR_H / 2 + 2);
    spr->drawString("BIN", ox + DISPLAY_LBL_X, lcdY + DISP_BIN_REL_Y + DISP_CHAR_H / 2 + 2);
    spr->unloadFont();

    const int dotsX = ox + DOTS_START_X;
    m_dotDez.draw(spr, dotsX, lcdY + DISP_DEZ_REL_Y);
    m_dotHex.draw(spr, dotsX, lcdY + DISP_HEX_REL_Y);
    m_dotBin.draw(spr, dotsX, lcdY + DISP_BIN_REL_Y);
}

/**
 * @brief Zeichnet alle 25 Tasten der Rechnertastatur (5×5 Grid).
 */
void WorkshopTool_Calculator::drawButtons(Gfx_Sprite *spr, int ox, int oy)
{
    spr->loadFont(ComfortaaBold16);
    spr->setTextDatum(MC_DATUM);

    for (int i = 0; i < NUM_BUTTONS; ++i)
    {
        const BtnRect &r = m_btnRects[i];
        const KeyEntry &k = KEYS[i];
        const bool pressed = (i == m_pressedBtn);

        uint16_t fillColor;
        uint16_t textColor;

        if (pressed)
        {
            fillColor = BTN_PRESSED_FILL;
            textColor = BTN_PRESSED_TEXT;
        }
        else
        {
            switch (k.type)
            {
            case BtnType::DIGIT:
            case BtnType::DOT:
                fillColor = BTN_NUM_FILL;
                textColor = BTN_NUM_TEXT;
                break;
            case BtnType::MODIFIER:
                // BS visuell von den übrigen Modifikatoren absetzen
                if (k.action == 'B')
                {
                    fillColor = BTN_BS_FILL;
                    textColor = BTN_BS_TEXT;
                }
                else
                {
                    fillColor = BTN_MOD_FILL;
                    textColor = BTN_MOD_TEXT;
                }
                break;
            case BtnType::FUNCTION:
                fillColor = BTN_FN_FILL;
                textColor = BTN_FN_TEXT;
                break;
            case BtnType::OPERATOR:
                fillColor = BTN_OP_FILL;
                textColor = BTN_OP_TEXT;
                break;
            case BtnType::CLEAR:
                fillColor = BTN_CLR_FILL;
                textColor = BTN_CLR_TEXT;
                break;
            case BtnType::EQUALS:
                fillColor = BTN_EQ_FILL;
                textColor = BTN_EQ_TEXT;
                break;
            default:
                fillColor = BTN_NUM_FILL;
                textColor = BTN_NUM_TEXT;
                break;
            }
        }

        const int bx = ox + r.x;
        const int by = oy + r.y;

        spr->fillSmoothRoundRect(bx, by, r.w, r.h, BTN_RADIUS, fillColor);

        if (k.action == 'B')
        {
            // Backspace-Symbol statt Textlabel zeichnen
            const int16_t cx = static_cast<int16_t>(bx + r.w / 2);
            const int16_t cy = static_cast<int16_t>(by + r.h / 2 + BTN_TEXT_OY);
            const int16_t size = static_cast<int16_t>(r.h - 10); // Symbol etwas kleiner als Tastenhöhe

            const float sc = size / 24.0f;
            const int16_t half = size / 2;
            const int16_t lx = cx - half;
            const int16_t ty = cy - half;

            // Rahmen: oben, unten, rechts
            spr->drawSmoothLine(lx + static_cast<int16_t>(7 * sc), ty + static_cast<int16_t>(3 * sc), lx + static_cast<int16_t>(22 * sc), ty + static_cast<int16_t>(3 * sc), textColor);
            spr->drawSmoothLine(lx + static_cast<int16_t>(7 * sc), ty + static_cast<int16_t>(21 * sc), lx + static_cast<int16_t>(22 * sc), ty + static_cast<int16_t>(21 * sc), textColor);
            spr->drawSmoothLine(lx + static_cast<int16_t>(22 * sc), ty + static_cast<int16_t>(3 * sc), lx + static_cast<int16_t>(22 * sc), ty + static_cast<int16_t>(21 * sc), textColor);
            // Schräge Kanten links (Pfeilspitze)
            spr->drawSmoothLine(lx + static_cast<int16_t>(0 * sc), ty + static_cast<int16_t>(12 * sc), lx + static_cast<int16_t>(7 * sc), ty + static_cast<int16_t>(3 * sc), textColor);
            spr->drawSmoothLine(lx + static_cast<int16_t>(0 * sc), ty + static_cast<int16_t>(12 * sc), lx + static_cast<int16_t>(7 * sc), ty + static_cast<int16_t>(21 * sc), textColor);
            // Kreuz in der Mitte
            spr->drawSmoothLine(lx + static_cast<int16_t>(9 * sc), ty + static_cast<int16_t>(8 * sc), lx + static_cast<int16_t>(18 * sc), ty + static_cast<int16_t>(17 * sc), textColor);
            spr->drawSmoothLine(lx + static_cast<int16_t>(9 * sc), ty + static_cast<int16_t>(17 * sc), lx + static_cast<int16_t>(18 * sc), ty + static_cast<int16_t>(8 * sc), textColor);
        }
        else
        {
            spr->setTextColor(textColor, fillColor);
            spr->drawString(k.label, bx + r.w / 2, by + r.h / 2 + BTN_TEXT_OY);
        }
    }

    spr->unloadFont();
}
