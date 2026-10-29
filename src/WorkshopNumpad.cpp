#include "WorkshopNumpad.h"

#include <cstring>

#include "fonts/ComfortaaBold16.h"

// ===================================================================
// Statische Datenbasis
// ===================================================================

/**
 * @brief Tastendefinitionen: 7-9 / 4-6 / 1-3 / . 0 +/- | BS / C (rechts, doppelt hoch).
 */
const WorkshopNumpad::KeyEntry WorkshopNumpad::KEYS[WorkshopNumpad::NUM_KEYS] =
    {
        // Zeile 0, Spalten 0-2
        {"7", '7', KeyType::DIGIT},      // Index 0
        {"8", '8', KeyType::DIGIT},      // Index 1
        {"9", '9', KeyType::DIGIT},      // Index 2
                                         // Zeile 1, Spalten 0-2
        {"4", '4', KeyType::DIGIT},      // Index 3
        {"5", '5', KeyType::DIGIT},      // Index 4
        {"6", '6', KeyType::DIGIT},      // Index 5
                                         // Zeile 2, Spalten 0-2
        {"1", '1', KeyType::DIGIT},      // Index 6
        {"2", '2', KeyType::DIGIT},      // Index 7
        {"3", '3', KeyType::DIGIT},      // Index 8
                                         // Zeile 3, Spalten 0-2
        {",", '.', KeyType::DIGIT},      // Index 9
        {"0", '0', KeyType::DIGIT},      // Index 10
        {"+/-", 'S', KeyType::ACTION},   // Index 11
                                         // Rechte Spalte, doppelt hoch
        {"BS", 'B', KeyType::BACKSPACE}, // Index 12 - Zeilen 0+1
        {"C", 'C', KeyType::CLEAR},      // Index 13 - Zeilen 2+3
};

// ===================================================================
// Lebenszyklus
// ===================================================================

/**
 * @brief Setzt alle Layout-Rects und internen Größen auf null.
 */
void WorkshopNumpad::clearLayout()
{
    memset(m_keyRects, 0, sizeof(m_keyRects));
    m_btnW = 0;
    m_btnH = 0;
    m_startY = 0;
    m_w = 0;
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet alle Tasten-Rechtecke (content-relativ).
 */
void WorkshopNumpad::computeLayout(int w, int startY)
{
    m_w = w;
    m_startY = startY;

    // Tastenhöhe: durch REQUIRED_H und BTN_GAP exakt TARGET_BTN_H
    m_btnH = TARGET_BTN_H;
    m_btnW = (w - (PAD_COLS - 1) * BTN_GAP) / PAD_COLS;
    if (m_btnW < 1)
        m_btnW = 1;

    const int tallH = 2 * m_btnH + BTN_GAP;

    const int colX[4] = {
        0,
        m_btnW + BTN_GAP,
        2 * (m_btnW + BTN_GAP),
        3 * (m_btnW + BTN_GAP)};

    const int rowY[4] = {
        startY,
        startY + m_btnH + BTN_GAP,
        startY + 2 * (m_btnH + BTN_GAP),
        startY + 3 * (m_btnH + BTN_GAP)};

    // Indizes 0-11: linke 3 Spalten, Zeilen 0-3
    for (int k = 0; k < 12; ++k)
    {
        const int row = k / 3;
        const int col = k % 3;
        m_keyRects[k] = {colX[col], rowY[row], m_btnW, m_btnH};
    }

    // Index 12: BS - rechte Spalte, Zeilen 0+1 (doppelt hoch)
    m_keyRects[12] = {colX[3], rowY[0], m_btnW, tallH};

    // Index 13: C  - rechte Spalte, Zeilen 2+3 (doppelt hoch)
    m_keyRects[13] = {colX[3], rowY[2], m_btnW, tallH};
}

// ===================================================================
// Datenzugriff
// ===================================================================

/**
 * @brief Gibt das Aktionszeichen einer Taste zurück.
 */
char WorkshopNumpad::keyAction(int keyIdx)
{
    if (keyIdx < 0 || keyIdx >= NUM_KEYS)
        return '\0';
    return KEYS[keyIdx].action;
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet Hintergrund und alle Tasten des Nummernpads.
 */
void WorkshopNumpad::draw(Gfx_Sprite *spr, int originX, int originY, int pressedKey) const
{
    // Hintergrund des Nummernpad-Bereichs füllen
    spr->fillRect(originX, originY + m_startY, m_w, REQUIRED_H, C_PAD_BG);

    spr->loadFont(ComfortaaBold16);
    for (int k = 0; k < NUM_KEYS; ++k)
        drawKey(spr, originX, originY, k, pressedKey == k);

    spr->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Nummernpad-Taste.
 */
void WorkshopNumpad::drawKey(Gfx_Sprite *spr, int originX, int originY, int keyIdx, bool pressed) const
{
    if (keyIdx < 0 || keyIdx >= NUM_KEYS)
        return;

    const Rect &r = m_keyRects[keyIdx];
    const KeyEntry &e = KEYS[keyIdx];
    const int ax = originX + r.x;
    const int ay = originY + r.y;

    if (r.w <= 0 || r.h <= 0)
        return;

    // Farben nach Zustand und Tastentyp
    uint16_t fill, textColor;
    if (pressed)
    {
        fill = BTN_PRESSED_FILL;
        textColor = 0xFFFF;
    }
    else
    {
        switch (e.type)
        {
        case KeyType::DIGIT:
            fill = BTN_DIGIT_FILL;
            textColor = BTN_DIGIT_TEXT;
            break;
        case KeyType::ACTION:
            fill = BTN_ACTION_FILL;
            textColor = BTN_ACTION_TEXT;
            break;
        case KeyType::BACKSPACE:
            fill = BTN_BS_FILL;
            textColor = BTN_BS_TEXT;
            break;
        case KeyType::CLEAR:
        default:
            fill = BTN_CLR_FILL;
            textColor = BTN_CLR_TEXT;
            break;
        }
    }

    spr->fillSmoothRoundRect(ax, ay, r.w, r.h, BTN_RADIUS, fill);

    const int cx = ax + r.w / 2;
    const int cy = ay + r.h / 2 + BTN_TEXT_OY;

    if (e.action == 'B')
    {
        // Vektorsymbol statt Textlabel
        drawBackspaceSymbol(spr, cx, cy, m_btnH, textColor);
    }
    else
    {
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(textColor, fill);
        spr->drawString(e.label, cx, cy);
    }
}

/**
 * @brief Zeichnet das Backspace-Vektorsymbol
 */
void WorkshopNumpad::drawBackspaceSymbol(Gfx_Sprite *spr, int cx, int cy,
                                         int size, uint16_t color) const
{
    const float sc = size / 24.0f;
    const int16_t half = static_cast<int16_t>(size / 2);
    const int16_t lx = static_cast<int16_t>(cx - half);
    const int16_t ty = static_cast<int16_t>(cy - half);

    // Rahmen: oben, unten, rechts
    spr->drawSmoothLine(lx + static_cast<int16_t>(7 * sc), ty + static_cast<int16_t>(3 * sc), lx + static_cast<int16_t>(22 * sc), ty + static_cast<int16_t>(3 * sc), color);
    spr->drawSmoothLine(lx + static_cast<int16_t>(7 * sc), ty + static_cast<int16_t>(21 * sc), lx + static_cast<int16_t>(22 * sc), ty + static_cast<int16_t>(21 * sc), color);
    spr->drawSmoothLine(lx + static_cast<int16_t>(22 * sc), ty + static_cast<int16_t>(3 * sc), lx + static_cast<int16_t>(22 * sc), ty + static_cast<int16_t>(21 * sc), color);
    // Schräge Kanten links
    spr->drawSmoothLine(lx + static_cast<int16_t>(0 * sc), ty + static_cast<int16_t>(12 * sc), lx + static_cast<int16_t>(7 * sc), ty + static_cast<int16_t>(3 * sc), color);
    spr->drawSmoothLine(lx + static_cast<int16_t>(0 * sc), ty + static_cast<int16_t>(12 * sc), lx + static_cast<int16_t>(7 * sc), ty + static_cast<int16_t>(21 * sc), color);
    // Kreuz in der Mitte
    spr->drawSmoothLine(lx + static_cast<int16_t>(9 * sc), ty + static_cast<int16_t>(8 * sc), lx + static_cast<int16_t>(18 * sc), ty + static_cast<int16_t>(17 * sc), color);
    spr->drawSmoothLine(lx + static_cast<int16_t>(9 * sc), ty + static_cast<int16_t>(17 * sc), lx + static_cast<int16_t>(18 * sc), ty + static_cast<int16_t>(8 * sc), color);
}

// ===================================================================
// Treffertest
// ===================================================================

/**
 * @brief Treffertest auf das Nummernpad
 */
int WorkshopNumpad::hitTest(int cx, int cy) const
{
    for (int k = 0; k < NUM_KEYS; ++k)
    {
        const Rect &r = m_keyRects[k];
        if (r.w <= 0 || r.h <= 0)
            continue;

        if (cx >= r.x && cx < r.x + r.w && cy >= r.y && cy < r.y + r.h)
            return k;
    }
    return -1;
}
