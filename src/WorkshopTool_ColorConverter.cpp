#include "WorkshopTool_ColorConverter.h"

#include <algorithm>
#include <cstring>

#include "Gfx.h"
#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold16.h"

// ===================================================================
// Statische Datenbasis
// ===================================================================

const char *const WorkshopTool_ColorConverter::QTY_SYMBOLS[WorkshopTool_ColorConverter::NUM_QTY] = {"R", "G", "B"};

// ===================================================================
// Konstruktor / Destruktor / Initialisierung
// ===================================================================

/**
 * @brief Konstruktor
 */
WorkshopTool_ColorConverter::WorkshopTool_ColorConverter()
{
    clearAll();
}

/**
 * @brief Destruktor
 */
WorkshopTool_ColorConverter::~WorkshopTool_ColorConverter()
{
    destroyFastSprites();
}

/**
 * @brief Wird beim Aktivieren des Tools im Workshop aufgerufen.
 */
void WorkshopTool_ColorConverter::onEnter(LGFX *tft)
{
    destroyFastSprites();
    clearAll();
    m_tft = tft;
}

/**
 * @brief Wird beim Verlassen des Tools aufgerufen, gibt Zone-Sprites frei.
 */
void WorkshopTool_ColorConverter::onExit()
{
    destroyFastSprites();
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte und initialisiert mit R=G=B=64.
 */
void WorkshopTool_ColorConverter::clearAll()
{
    destroyFastSprites();

    memset(m_values, 0, sizeof(m_values));
    memset(m_sliderY, 0, sizeof(m_sliderY));
    memset(m_hexStr, 0, sizeof(m_hexStr));

    m_tft = nullptr;
    m_sprite = nullptr;
    m_originX = 0;
    m_originY = 0;
    m_w = 0;
    m_h = 0;
    m_layoutValid = false;
    m_rgb565 = 0;
    m_rBack = 0;
    m_gBack = 0;
    m_bBack = 0;
    m_swatchY = 0;
    m_resultY = 0;
    m_trackW = 0;
    m_dragSlider = -1;
    m_touchWas = false;
    m_filteredTouchX = 0;
    m_touchFilterValid = false;

    m_forceFullRedraw = true;
    m_lastAuxPushMs = 0;

    m_fastSpritesReady = false;

    m_values[0] = 64;
    m_values[1] = 64;
    m_values[2] = 64;

    compute();
    markAllDirty();
}

/**
 * @brief Markiert alle Sektionen (Slider, Swatch, Result) als neu zu zeichnen.
 */
void WorkshopTool_ColorConverter::markAllDirty()
{
    m_dirtySliders = 0x07;
    m_dirtySwatch = true;
    m_dirtyResult = true;
}

/**
 * @brief Markiert einen einzelnen Slider als neu zu zeichnen.
 */
void WorkshopTool_ColorConverter::markSliderDirty(int qty)
{
    if (qty < 0 || qty >= NUM_QTY)
        return;

    m_dirtySliders |= static_cast<uint8_t>(1u << qty);
}

/**
 * @brief Liefert true, wenn mindestens eine Sektion neu gezeichnet werden muss.
 */
bool WorkshopTool_ColorConverter::hasDirty() const
{
    return (m_dirtySliders != 0u) || m_dirtySwatch || m_dirtyResult;
}

// ===================================================================
// Öffentliche Interface-Methoden
// ===================================================================

/**
 * @brief Zeichnet bei Bedarf alle dirty-markierten Sektionen in den Work-Sprite.
 */
void WorkshopTool_ColorConverter::draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h)
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
        m_forceFullRedraw = true;
        markAllDirty();

        destroyFastSprites();
        (void)ensureFastSprites();
    }

    if (!hasDirty() && !m_forceFullRedraw)
        return;

    m_sprite->loadFont(ComfortaaBold16);

    if (m_forceFullRedraw)
    {
        drawSliders();
        drawSwatch();
        drawResult();
        m_forceFullRedraw = false;
    }
    else
    {
        if (m_dirtySliders & 0x01)
            drawSlider(0);

        if (m_dirtySliders & 0x02)
            drawSlider(1);

        if (m_dirtySliders & 0x04)
            drawSlider(2);

        if (m_dirtySwatch)
            drawSwatch();

        if (m_dirtyResult)
            drawResult();
    }

    m_sprite->unloadFont();

    m_dirtySliders = 0;
    m_dirtySwatch = false;
    m_dirtyResult = false;
}

/**
 * @brief Verarbeitet Touch-Events für Slider-Drag mit Direkt-Push auf den TFT.
 */
bool WorkshopTool_ColorConverter::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    if (touched)
    {
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        if (!m_touchWas)
        {
            const int hitSlider = hitTestSlider(cy);
            if (hitSlider < 0)
                return false;

            m_touchWas = true;
            m_dragSlider = hitSlider;
            m_filteredTouchX = tx;
            m_touchFilterValid = true;
            markSliderDirty(m_dragSlider);

            const int newVal = sliderValueFromX(tx);
            if (newVal != m_values[m_dragSlider])
            {
                m_values[m_dragSlider] = newVal;
                compute();
                m_dirtySwatch = true;
                m_dirtyResult = true;
            }

            if (directFlushToDisplay(false))
                return false;

            return true;
        }

        if (m_dragSlider < 0)
            return false;

        if (!m_touchFilterValid)
        {
            m_filteredTouchX = tx;
            m_touchFilterValid = true;
        }
        else
        {
            const int weighted = (m_filteredTouchX * TOUCH_SMOOTH_OLD_WEIGHT) + (tx * TOUCH_SMOOTH_NEW_WEIGHT);
            m_filteredTouchX = weighted / (TOUCH_SMOOTH_OLD_WEIGHT + TOUCH_SMOOTH_NEW_WEIGHT);
        }

        const int newVal = sliderValueFromX(m_filteredTouchX);
        if (newVal == m_values[m_dragSlider])
            return false;

        m_values[m_dragSlider] = newVal;
        compute();
        markSliderDirty(m_dragSlider);
        m_dirtySwatch = true;
        m_dirtyResult = true;

        if (directFlushToDisplay(false))
            return false;

        return true;
    }

    const int releasedSlider = m_dragSlider;
    const bool wasActive = m_touchWas || (releasedSlider >= 0);

    m_touchWas = false;
    m_dragSlider = -1;
    m_touchFilterValid = false;

    if (releasedSlider >= 0)
        markSliderDirty(releasedSlider);

    if (!wasActive)
        return false;

    if (directFlushToDisplay(true))
        return false;

    return true;
}

// ===================================================================
// Layout / Berechnung
// ===================================================================

/**
 * @brief Berechnet Y-Positionen der Slider und horizontale Track-Breite.
 */
void WorkshopTool_ColorConverter::computeLayout(int w, int /*h*/)
{
    for (int i = 0; i < NUM_QTY; ++i)
        m_sliderY[i] = TOP_GAP + i * (SLIDER_H + SLIDER_GAP);

    const int slidersBottom = TOP_GAP + NUM_QTY * SLIDER_H + (NUM_QTY - 1) * SLIDER_GAP;
    m_swatchY = slidersBottom + SWATCH_GAP;
    m_resultY = m_swatchY + SWATCH_COLOR_H + RESULT_GAP;

    const int trackRightX = w - VAL_W - VAL_GAP;
    m_trackW = std::max(1, trackRightX - TRACK_LEFT_X);
}

/**
 * @brief Aktualisiert RGB565, HEX-String und die quantisierten Display-RGB-Werte.
 */
void WorkshopTool_ColorConverter::compute()
{
    const uint8_t r = static_cast<uint8_t>(m_values[0]);
    const uint8_t g = static_cast<uint8_t>(m_values[1]);
    const uint8_t b = static_cast<uint8_t>(m_values[2]);

    m_rgb565 = toRgb565(r, g, b);
    stringutils::safeFormat(m_hexStr, sizeof(m_hexStr), "#%02X%02X%02X", r, g, b);

    const uint8_t r5 = static_cast<uint8_t>((m_rgb565 >> 11) & 0x1F);
    const uint8_t g6 = static_cast<uint8_t>((m_rgb565 >> 5) & 0x3F);
    const uint8_t b5 = static_cast<uint8_t>(m_rgb565 & 0x1F);

    m_rBack = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
    m_gBack = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
    m_bBack = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
}

/**
 * @brief Konvertiert 8-Bit-RGB nach RGB565.
 */
uint16_t WorkshopTool_ColorConverter::toRgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(r >> 3) << 11) |
        (static_cast<uint16_t>(g >> 2) << 5) |
        (static_cast<uint16_t>(b >> 3)));
}

/**
 * @brief Liefert die Akzentfarbe (RGB565) für den jeweiligen Kanal.
 */
uint16_t WorkshopTool_ColorConverter::channelColor(int qty)
{
    switch (qty)
    {
    case 0:
        return C_CH_R;
    case 1:
        return C_CH_G;
    default:
        return C_CH_B;
    }
}

/**
 * @brief Rechnet eine absolute Display-X-Koordinate in einen Slider-Wert (0-255) um.
 */
int WorkshopTool_ColorConverter::sliderValueFromX(int displayX) const
{
    const int trackAbsLeft = WorkshopCfg::WORK_X + m_originX + TRACK_LEFT_X;
    int relX = displayX - trackAbsLeft;

    relX = std::max(0, std::min(m_trackW, relX));
    return (relX * 255 + (m_trackW / 2)) / std::max(1, m_trackW);
}

// ===================================================================
// Zeichnen in den persistierenden Work-Sprite
// ===================================================================

/**
 * @brief Zeichnet alle drei Slider in den Work-Sprite.
 */
void WorkshopTool_ColorConverter::drawSliders()
{
    for (int i = 0; i < NUM_QTY; ++i)
        drawSlider(i);
}

/**
 * @brief Zeichnet einen einzelnen Slider in den Work-Sprite.
 */
void WorkshopTool_ColorConverter::drawSlider(int qty)
{
    renderSliderTo(m_sprite, qty, m_originX, m_originY + m_sliderY[qty], m_w);
}

/**
 * @brief Zeichnet die Farbvorschau in den Work-Sprite.
 */
void WorkshopTool_ColorConverter::drawSwatch()
{
    renderSwatchTo(m_sprite, m_originX, m_originY + m_swatchY, m_w);
}

/**
 * @brief Zeichnet die Ergebnis-Zeile (quantisierte Display-RGB-Werte) in den Work-Sprite.
 */
void WorkshopTool_ColorConverter::drawResult()
{
    renderResultTo(m_sprite, m_originX, m_originY + m_resultY, m_w);
}

/**
 * @brief Rendert einen Slider in das angegebene Sprite (Work- oder Zone-Sprite).
 */
void WorkshopTool_ColorConverter::renderSliderTo(Gfx_Sprite *spr, int qty, int baseX, int baseY, int width)
{
    if (!spr)
        return;

    const bool active = (m_dragSlider == qty);
    const uint16_t chCol = channelColor(qty);

    spr->fillRect(baseX, baseY, width, SLIDER_H, C_SLIDER_BG);

    const int midY = baseY + SLIDER_MID;

    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(active ? C_LBL_ACTIVE : C_LBL_NORMAL, C_SLIDER_BG);
    spr->drawString(QTY_SYMBOLS[qty], baseX + SYM_X, midY);

    char valBuf[5];
    stringutils::safeFormat(valBuf, sizeof(valBuf), "%d", m_values[qty]);
    spr->setTextDatum(MR_DATUM);
    spr->setTextColor(active ? C_VAL_ACTIVE : C_VAL_NORMAL, C_SLIDER_BG);
    spr->drawString(valBuf, baseX + width - VAL_GAP, midY);

    const int trackAbsX = baseX + TRACK_LEFT_X;
    const int trackAbsY = baseY + TRACK_TOP;

    // Track-Hintergrund
    spr->fillRect(trackAbsX, trackAbsY, m_trackW, TRACK_H, C_TRACK_EMPTY);

    // Track-Füllung entsprechend aktuellem Wert
    const int fillW = (m_values[qty] * m_trackW) / 255;
    if (fillW > 0)
    {
        const uint16_t spurCol = active ? chCol : scaleColor(chCol, 0.70f);
        spr->fillRect(trackAbsX, trackAbsY, fillW, TRACK_H, spurCol);
    }

    const int thumbMinX = trackAbsX;
    const int thumbMaxX = trackAbsX + std::max(0, m_trackW - THUMB_W);
    int thumbX = trackAbsX + fillW - THUMB_W / 2;
    thumbX = std::max(thumbMinX, std::min(thumbMaxX, thumbX));

    // Thumb zeichnen
    spr->fillRect(thumbX, trackAbsY, THUMB_W, TRACK_H, active ? C_THUMB_ACTIVE : C_THUMB_NORMAL);
}

/**
 * @brief Rendert die Farbvorschau (Swatch) inkl. HEX- und RGB565-Beschriftung in das Sprite.
 */
void WorkshopTool_ColorConverter::renderSwatchTo(Gfx_Sprite *spr, int baseX, int baseY, int width)
{
    if (!spr)
        return;

    // Bounding-Box mit Hintergrundfarbe füllen, damit fillSmoothRoundRect die Ecken sauber gegen einen definierten Hintergrund anti-aliasen kann.
    spr->fillRect(baseX, baseY, width, SWATCH_COLOR_H, C_BG);
    spr->fillSmoothRoundRect(baseX, baseY, width, SWATCH_COLOR_H, SWATCH_CORNER_R, m_rgb565);

    // Textfarbe automatisch aus Helligkeit der Farbfläche bestimmen
    const float intensity = getIntensityFrom565(m_rgb565);
    const bool isDark = (intensity < 0.45f);
    const uint16_t txtColor = isDark ? C_SWATCH_TXT_LIGHT : C_SWATCH_TXT_DARK;
    const uint16_t lblColor = isDark ? C_SWATCH_LBL_LIGHT : C_SWATCH_LBL_DARK;

    // Vertikale Mitte der Farbfläche
    const int cy = baseY + SWATCH_COLOR_H / 2;
    const int midX = baseX + width / 2;

    // Vertikaler Separator in der Mitte
    spr->drawFastVLine(midX, baseY + 8, SWATCH_COLOR_H - 16, lblColor);

    // Vertikaler Versatz Label/Wert zur Swatch-Mitte (verhindert Überschneiden)
    constexpr int LBL_DY = -13;
    constexpr int VAL_DY = 13;

    // Linke Hälfte: "HEX" Label + "#RRGGBB" Wert
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblColor, m_rgb565);
    spr->drawString("HEX", baseX + 12, cy + LBL_DY);

    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(txtColor, m_rgb565);
    spr->drawString(m_hexStr, baseX + 12, cy + VAL_DY);

    // Rechte Hälfte: "RGB565" Label + "0xXXXX" Wert
    char rgb565Str[8];
    stringutils::safeFormat(rgb565Str, sizeof(rgb565Str), "0x%04X", m_rgb565);

    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(lblColor, m_rgb565);
    spr->drawString("RGB565", midX + 12, cy + LBL_DY);

    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(txtColor, m_rgb565);
    spr->drawString(rgb565Str, midX + 12, cy + VAL_DY);
}

/**
 * @brief Rendert die Ergebnis-Zeile mit den drei quantisierten Display-RGB-Werten.
 */
void WorkshopTool_ColorConverter::renderResultTo(Gfx_Sprite *spr, int baseX, int baseY, int width)
{
    if (!spr)
        return;

    // Ergebnisbereich löschen
    spr->fillRect(baseX, baseY, width, RESULT_H, C_BG);

    // Kopfzeile
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(C_RESULT_KEY, C_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_COLOR_CONVERTER_RGB_HINT), baseX, baseY + RESULT_HEADER_H / 2 + 1);

    // Wertezeile
    constexpr int SQ = 13;         ///< Farbquadrat-Seitenlänge
    constexpr int SQ_GAP = 10;     ///< Abstand Zahlenwert zum Quadrat
    constexpr int GROUP_STEP = 70; ///< Abstand zwischen den drei Gruppen

    const int sqY = baseY + RESULT_H - SQ;

    const struct
    {
        int val;
        uint16_t accent;
    } cols[3] = {
        {static_cast<int>(m_rBack), C_CH_R},
        {static_cast<int>(m_gBack), C_CH_G},
        {static_cast<int>(m_bBack), C_CH_B},
    };

    for (int i = 0; i < 3; ++i)
    {
        const int sqX = baseX + i * GROUP_STEP;

        // Farbquadrat
        spr->fillRect(sqX, sqY, SQ, SQ, cols[i].accent);

        // Zahlenwert
        char numBuf[5];
        stringutils::safeFormat(numBuf, sizeof(numBuf), "%d", cols[i].val);

        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_RESULT_NUM, C_BG);
        spr->drawString(numBuf, sqX + SQ + SQ_GAP, sqY + SQ / 2);
    }
}

// ===================================================================
// Teilupdates auf Display mittels temporärer Zone-Sprites
// ===================================================================

/**
 * @brief Erstellt die Zone-Sprites (lazy) für direkte Teil-Updates auf den TFT.
 */
bool WorkshopTool_ColorConverter::ensureFastSprites()
{
    if (m_fastSpritesReady)
        return true;

    if (!m_tft || m_w <= 0)
        return false;

    auto createZoneSprite = [this](Gfx_Sprite *&spr, int w, int h) -> bool
    {
        if (spr)
            return true;

        spr = new Gfx_Sprite(m_tft);
        if (!spr)
            return false;

        spr->setColorDepth(16);
        if (!spr->createSprite(w, h))
        {
            delete spr;
            spr = nullptr;
            return false;
        }

        return true;
    };

    for (int i = 0; i < NUM_QTY; ++i)
    {
        if (!createZoneSprite(m_sliderSprites[i], m_w, SLIDER_H))
        {
            destroyFastSprites();
            return false;
        }
    }

    if (!createZoneSprite(m_swatchSprite, m_w, SWATCH_COLOR_H) || !createZoneSprite(m_resultSprite, m_w, RESULT_H))
    {
        destroyFastSprites();
        return false;
    }

    m_fastSpritesReady = true;

    // Font einmalig in alle Zone-Sprites laden (bleibt bis destroyFastSprites() aktiv)
    for (int i = 0; i < NUM_QTY; ++i)
        m_sliderSprites[i]->loadFont(ComfortaaBold16);

    m_swatchSprite->loadFont(ComfortaaBold16);
    m_resultSprite->loadFont(ComfortaaBold16);

    return true;
}

/**
 * @brief Gibt alle Zone-Sprites frei und entlädt die zugehörigen Fonts.
 */
void WorkshopTool_ColorConverter::destroyFastSprites()
{
    auto destroySprite = [](Gfx_Sprite *&spr)
    {
        if (!spr)
            return;
        spr->unloadFont();
        spr->deleteSprite();
        delete spr;
        spr = nullptr;
    };

    for (int i = 0; i < NUM_QTY; ++i)
        destroySprite(m_sliderSprites[i]);

    destroySprite(m_swatchSprite);
    destroySprite(m_resultSprite);
    m_fastSpritesReady = false;
}

/**
 * @brief Pusht alle dirty-markierten Sektionen direkt via Zone-Sprites auf den TFT.
 */
bool WorkshopTool_ColorConverter::directFlushToDisplay(bool forceAux)
{
    if (!hasDirty())
        return true;

    if (!m_tft || !m_sprite || !m_layoutValid)
        return false;

    if (!ensureFastSprites())
        return false;

    const uint32_t now = millis();
    const bool isDragging = (m_dragSlider >= 0);
    const bool auxAllowed = forceAux || !isDragging || ((now - m_lastAuxPushMs) >= AUX_PUSH_MS);
    const int absX = WorkshopCfg::WORK_X + m_originX;
    const int absY = WorkshopCfg::WORK_Y + m_originY;

    for (int i = 0; i < NUM_QTY; ++i)
    {
        if ((m_dirtySliders & (1u << i)) == 0u)
            continue;

        Gfx_Sprite *zone = m_sliderSprites[i];
        renderSliderTo(zone, i, 0, 0, m_w);
        zone->pushSprite(absX, absY + m_sliderY[i]);
    }

    m_dirtySliders = 0u;

    if (auxAllowed)
    {
        if (m_dirtySwatch)
        {
            renderSwatchTo(m_swatchSprite, 0, 0, m_w);
            m_swatchSprite->pushSprite(absX, absY + m_swatchY);
            m_dirtySwatch = false;
            m_lastAuxPushMs = now;
        }

        if (m_dirtyResult)
        {
            renderResultTo(m_resultSprite, 0, 0, m_w);
            m_resultSprite->pushSprite(absX, absY + m_resultY);
            m_dirtyResult = false;
            m_lastAuxPushMs = now;
        }
    }

    // Work-Sprite ist jetzt veraltet - beim nächsten draw() komplett neu aufbauen.
    m_forceFullRedraw = true;

    return true;
}

// ===================================================================
// Treffertest
// ===================================================================

/**
 * @brief Liefert den Index des Sliders unter der content-relativen Y-Koordinate, oder -1.
 */
int WorkshopTool_ColorConverter::hitTestSlider(int cy) const
{
    for (int i = 0; i < NUM_QTY; ++i)
    {
        if (cy >= m_sliderY[i] && cy < m_sliderY[i] + SLIDER_H)
            return i;
    }
    return -1;
}
