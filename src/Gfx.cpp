#include "Gfx.h"

#include <driver/i2c.h>
#include <cmath>
#include "Pins.h"

/**
 *  @brief Namensraum für interne Implementierungsdetails
 */
namespace
{
    constexpr int DISPLAY_WIDTH = 800;              ///< Breite des Displays in Pixeln
    constexpr int DISPLAY_HEIGHT = 480;             ///< Höhe des Displays in Pixeln
    constexpr int DISPLAY_OFFSET = 0;               ///< Versatz des Displays in Pixeln
    constexpr int TOUCH_X_MAX = DISPLAY_WIDTH - 1;  ///< Maximale X-Koordinate des Touchscreens
    constexpr int TOUCH_Y_MAX = DISPLAY_HEIGHT - 1; ///< Maximale Y-Koordinate des Touchscreens
    constexpr int BUS_WRITE_FREQUENCY = 13000000;   ///< Schreibfrequenz des Busses in Hz
    constexpr int I2C_TOUCH_FREQUENCY = 100000;     ///< I2C-Frequenz des Touchscreens
    constexpr int HSYNC_FRONT_PORCH = 8;            ///< HSYNC Front Porch in Pixeln
    constexpr int HSYNC_PULSE_WIDTH = 4;            ///< HSYNC Pulse Width in Pixeln
    constexpr int HSYNC_BACK_PORCH = 8;             ///< HSYNC Back Porch in Pixeln
    constexpr int VSYNC_FRONT_PORCH = 8;            ///< VSYNC Front Porch in Pixeln
    constexpr int VSYNC_PULSE_WIDTH = 4;            ///< VSYNC Pulse Width in Pixeln
    constexpr int VSYNC_BACK_PORCH = 8;             ///< VSYNC Back Porch in Pixeln
    constexpr int SIGNAL_POLARITY_LOW = 0;          ///< Signal-Polarität (niedrig)
    constexpr int PCLK_IDLE_HIGH = 1;               ///< PCLK im ruhigen Zustand (hoch)
    constexpr int USE_PSRAM = 2;                    ///< PSRAM für Framebuffer verwenden

    constexpr uint8_t RAINBOW_COLOR_COUNT = 192; ///< Anzahl der Farben im zyklischen RGB565-Verlauf
    constexpr uint16_t RAINBOW_COLOR_LUT[RAINBOW_COLOR_COUNT] = {
        0xF800, 0xF820, 0xF840, 0xF860, 0xF880, 0xF8A0, 0xF8C0, 0xF8E0,
        0xF900, 0xF920, 0xF940, 0xF960, 0xF980, 0xF9A0, 0xF9C0, 0xF9E0,
        0xFA00, 0xFA20, 0xFA40, 0xFA60, 0xFA80, 0xFAA0, 0xFAC0, 0xFAE0,
        0xFB00, 0xFB20, 0xFB40, 0xFB60, 0xFB80, 0xFBA0, 0xFBC0, 0xFBE0,
        0xFBE0, 0xF3E0, 0xEBE0, 0xE3E0, 0xDBE0, 0xD3E0, 0xCBE0, 0xC3E0,
        0xBBE0, 0xB3E0, 0xABE0, 0xA3E0, 0x9BE0, 0x93E0, 0x8BE0, 0x83E0,
        0x7BE0, 0x73E0, 0x6BE0, 0x63E0, 0x5BE0, 0x53E0, 0x4BE0, 0x43E0,
        0x3BE0, 0x33E0, 0x2BE0, 0x23E0, 0x1BE0, 0x13E0, 0x0BE0, 0x03E0,
        0x03E0, 0x03E1, 0x03E2, 0x03E3, 0x03E4, 0x03E5, 0x03E6, 0x03E7,
        0x03E8, 0x03E9, 0x03EA, 0x03EB, 0x03EC, 0x03ED, 0x03EE, 0x03EF,
        0x03F0, 0x03F1, 0x03F2, 0x03F3, 0x03F4, 0x03F5, 0x03F6, 0x03F7,
        0x03F8, 0x03F9, 0x03FA, 0x03FB, 0x03FC, 0x03FD, 0x03FE, 0x03FF,
        0x03FF, 0x03DF, 0x03BF, 0x039F, 0x037F, 0x035F, 0x033F, 0x031F,
        0x02FF, 0x02DF, 0x02BF, 0x029F, 0x027F, 0x025F, 0x023F, 0x021F,
        0x01FF, 0x01DF, 0x01BF, 0x019F, 0x017F, 0x015F, 0x013F, 0x011F,
        0x00FF, 0x00DF, 0x00BF, 0x009F, 0x007F, 0x005F, 0x003F, 0x001F,
        0x001F, 0x081F, 0x101F, 0x181F, 0x201F, 0x281F, 0x301F, 0x381F,
        0x401F, 0x481F, 0x501F, 0x581F, 0x601F, 0x681F, 0x701F, 0x781F,
        0x801F, 0x881F, 0x901F, 0x981F, 0xA01F, 0xA81F, 0xB01F, 0xB81F,
        0xC01F, 0xC81F, 0xD01F, 0xD81F, 0xE01F, 0xE81F, 0xF01F, 0xF81F,
        0xF81F, 0xF81E, 0xF81D, 0xF81C, 0xF81B, 0xF81A, 0xF819, 0xF818,
        0xF817, 0xF816, 0xF815, 0xF814, 0xF813, 0xF812, 0xF811, 0xF810,
        0xF80F, 0xF80E, 0xF80D, 0xF80C, 0xF80B, 0xF80A, 0xF809, 0xF808,
        0xF807, 0xF806, 0xF805, 0xF804, 0xF803, 0xF802, 0xF801, 0xF800};
}

/**
 * @brief Konfiguriert Display, RGB-Bus und Touch
 */
LGFX::LGFX(void)
{
    // Panel-Konfiguration: Speicher- und Displaygröße
    {
        auto cfg = _panel_instance.config();
        cfg.memory_width = DISPLAY_WIDTH;
        cfg.memory_height = DISPLAY_HEIGHT;
        cfg.panel_width = DISPLAY_WIDTH;
        cfg.panel_height = DISPLAY_HEIGHT;
        cfg.offset_x = DISPLAY_OFFSET;
        cfg.offset_y = DISPLAY_OFFSET;
        _panel_instance.config(cfg);
    }

    // Panel
    {
        auto cfg = _panel_instance.config_detail();
        cfg.use_psram = USE_PSRAM;
        _panel_instance.config_detail(cfg);
    }

    // Bus
    {
        auto cfg = _bus_instance.config();
        cfg.panel = &_panel_instance;

        // Datenleitungen D0-D15
        cfg.pin_d0 = rgbPins::LCD_RGB_D0;
        cfg.pin_d1 = rgbPins::LCD_RGB_D1;
        cfg.pin_d2 = rgbPins::LCD_RGB_D2;
        cfg.pin_d3 = rgbPins::LCD_RGB_D3;
        cfg.pin_d4 = rgbPins::LCD_RGB_D4;
        cfg.pin_d5 = rgbPins::LCD_RGB_D5;
        cfg.pin_d6 = rgbPins::LCD_RGB_D6;
        cfg.pin_d7 = rgbPins::LCD_RGB_D7;
        cfg.pin_d8 = rgbPins::LCD_RGB_D8;
        cfg.pin_d9 = rgbPins::LCD_RGB_D9;
        cfg.pin_d10 = rgbPins::LCD_RGB_D10;
        cfg.pin_d11 = rgbPins::LCD_RGB_D11;
        cfg.pin_d12 = rgbPins::LCD_RGB_D12;
        cfg.pin_d13 = rgbPins::LCD_RGB_D13;
        cfg.pin_d14 = rgbPins::LCD_RGB_D14;
        cfg.pin_d15 = rgbPins::LCD_RGB_D15;

        // Steuersignale
        cfg.pin_henable = rgbPins::LCD_RGB_DE;
        cfg.pin_vsync = rgbPins::LCD_RGB_VSYNC;
        cfg.pin_hsync = rgbPins::LCD_RGB_HSYNC;
        cfg.pin_pclk = rgbPins::LCD_RGB_PCLK;
        cfg.freq_write = BUS_WRITE_FREQUENCY;

        // HSYNC/VSYNC Timing
        cfg.hsync_polarity = SIGNAL_POLARITY_LOW;
        cfg.hsync_front_porch = HSYNC_FRONT_PORCH;
        cfg.hsync_pulse_width = HSYNC_PULSE_WIDTH;
        cfg.hsync_back_porch = HSYNC_BACK_PORCH;
        cfg.vsync_polarity = SIGNAL_POLARITY_LOW;
        cfg.vsync_front_porch = VSYNC_FRONT_PORCH;
        cfg.vsync_pulse_width = VSYNC_PULSE_WIDTH;
        cfg.vsync_back_porch = VSYNC_BACK_PORCH;
        cfg.pclk_idle_high = PCLK_IDLE_HIGH;
        _bus_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);

    // Touch
    {
        auto cfg = _touch_instance.config();
        cfg.x_min = DISPLAY_OFFSET;
        cfg.x_max = TOUCH_X_MAX;
        cfg.y_min = DISPLAY_OFFSET;
        cfg.y_max = TOUCH_Y_MAX;
        cfg.bus_shared = false;
        cfg.offset_rotation = DISPLAY_OFFSET;
        cfg.i2c_port = i2cPins::I2C_MASTER_NUM;
        cfg.pin_sda = i2cPins::I2C_MASTER_SDA_IO;
        cfg.pin_scl = i2cPins::I2C_MASTER_SCL_IO;
        cfg.pin_int = GPIO_NUM_NC;
        cfg.pin_rst = GPIO_NUM_NC;
        cfg.freq = I2C_TOUCH_FREQUENCY;
        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
}

/**
 * @brief Löscht ein Sprite sicher und gibt den Speicher frei
 */
void LGFX::safeDeleteSprite(Gfx_Sprite *&sprite)
{
    if (sprite)
    {
        // SmoothFont-Speicher freigeben
        sprite->unloadFont();

        sprite->deleteSprite();
        delete sprite;
        sprite = nullptr;
    }
}

/**
 * @brief Erstellt ein Sprite sicher mit Fehlerbehandlung
 */
bool LGFX::safeCreateSprite(Gfx_Sprite *&sprite, int width, int height, bool usePsram)
{
    if (sprite)
    {
        // Wenn Größe passt, Sprite wiederverwenden
        if (sprite->width() == width && sprite->height() == height)
        {
            sprite->setPsram(usePsram);
            sprite->setSwapBytes(true);
            sprite->setColorDepth(16);
            return true;
        }

        // Größe passt nicht, altes Sprite löschen
        safeDeleteSprite(sprite);
    }

    sprite = new Gfx_Sprite(this);
    if (!sprite)
        return false;

    sprite->setPsram(usePsram);
    sprite->setSwapBytes(true);
    sprite->setColorDepth(16);

    if (!sprite->createSprite(width, height))
    {
        safeDeleteSprite(sprite);
        return false;
    }

    return true;
}

// ===================================================================
// Globale Hilfsfunktionen
// ===================================================================

/**
 * @brief Zeichnet einen einzelnen Pixel nur innerhalb der gültigen Grenzen
 */
void LGFX::drawPixelClipped(int x, int y, uint16_t color)
{
    if (x >= 0 && y >= 0 && x < this->width() && y < this->height())
        this->drawPixel(x, y, color);
}

/**
 * @brief Zeichnet einen einzelnen Pixel nur innerhalb der gültigen Grenzen
 */
void Gfx_Sprite::drawPixelClipped(int x, int y, uint16_t color)
{
    if (x >= 0 && y >= 0 && x < this->width() && y < this->height())
        this->drawPixel(x, y, color);
}

/**
 * @brief Skaliert die Helligkeit einer RGB565-Farbe in beide Richtungen
 */
uint16_t scaleColor(uint16_t color, float factor)
{
    if (factor < 0.0f)
        factor = 0.0f;

    // Kanalwerte extrahieren
    const uint32_t r = (color >> 11) & 0x1F;
    const uint32_t g = (color >> 5) & 0x3F;
    const uint32_t b = color & 0x1F;

    // Skalieren und auf Kanalmaximum klemmen
    const uint32_t rs = (r * 256u * static_cast<uint32_t>(factor * 256.0f)) >> 16u;
    const uint32_t gs = (g * 256u * static_cast<uint32_t>(factor * 256.0f)) >> 16u;
    const uint32_t bs = (b * 256u * static_cast<uint32_t>(factor * 256.0f)) >> 16u;

    return static_cast<uint16_t>(
        ((rs > 31u ? 31u : rs) << 11) |
        ((gs > 63u ? 63u : gs) << 5) |
        (bs > 31u ? 31u : bs));
}

/**
 * @brief Lineare Interpolation zwischen zwei RGB565-Farben
 */
uint16_t blendColor(uint16_t fg, uint16_t bg, float alpha)
{
    // Alpha auf gültigen Bereich begrenzen
    if (alpha >= 1.0f)
        return fg;

    if (alpha <= 0.0f)
        return bg;

    // Integer-Faktor für die Interpolation berechnen
    uint32_t alpha256 = (uint32_t)(alpha * 256.0f);
    uint32_t inv_alpha = 256 - alpha256;

    // Extraktion der einzelnen Farbkanäle
    uint32_t r1 = (fg >> 11) & 0x1F;
    uint32_t g1 = (fg >> 5) & 0x3F;
    uint32_t b1 = fg & 0x1F;

    uint32_t r2 = (bg >> 11) & 0x1F;
    uint32_t g2 = (bg >> 5) & 0x3F;
    uint32_t b2 = bg & 0x1F;

    // Integer-basierte Interpolation (schneller als Float)
    uint32_t r = (r1 * alpha256 + r2 * inv_alpha) >> 8;
    uint32_t g = (g1 * alpha256 + g2 * inv_alpha) >> 8;
    uint32_t b = (b1 * alpha256 + b2 * inv_alpha) >> 8;

    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

/**
 * @brief Berechnet die Helligkeit einer RGB565-Farbe
 */
float getIntensityFrom565(uint16_t color)
{
    // Extrahiere 5/6/5-Bit-Farbkanäle
    uint8_t r5 = (color >> 11) & 0x1F;
    uint8_t g6 = (color >> 5) & 0x3F;
    uint8_t b5 = color & 0x1F;

    // Skaliere auf 8-Bit (0-255) für eine genaue Helligkeitsberechnung.
    uint8_t r8 = (r5 * 527 + 23) >> 6;
    uint8_t g8 = (g6 * 259 + 33) >> 6;
    uint8_t b8 = (b5 * 527 + 23) >> 6;

    // Leuchtdichte und Normierung auf 0.0 - 1.0
    return (0.2126f * r8 + 0.7152f * g8 + 0.0722f * b8) / 255.0f;
}

/**
 * @brief Berechnet eine pulsierende Farbe basierend auf der Zeit
 */
uint16_t getPulsatingColor(uint16_t baseColor, uint16_t highlightColor, uint32_t periodMs)
{
    uint32_t now = millis();
    float phase = (float)(now % periodMs) / (float)periodMs; // 0.0 bis 1.0

    // Sinus-Welle für sanfte Pulsation: 0.5 + 0.5 * sin(2 * PI * phase)
    float sine = 0.5f + 0.5f * sin(2.0f * PI * phase);

    return blendColor(highlightColor, baseColor, sine);
}

/**
 * @brief Wandelt einen Wert in eine 16-Bit-Regenbogenfarbe um
 */
uint16_t rainbowColors(uint8_t value)
{
    if (value >= RAINBOW_COLOR_COUNT)
        value -= RAINBOW_COLOR_COUNT;

    return RAINBOW_COLOR_LUT[value];
}

// ===================================================================
// Core-Implementierungen für geglättete Zeichenfunktionen
// ===================================================================

/**
 * @brief Core-Funktion zum Zeichnen eines geglätteten Kreisrings
 */
template <typename DrawPixelFunc>
void drawSmoothCircleCore(int xc, int yc, float r, float thickness, uint16_t color, uint16_t bg, DrawPixelFunc drawPixel)
{
    // Validierung von Radius und Dicke
    if (r <= 0.0f || thickness <= 0.0f)
        return;

    if (thickness > r)
        thickness = r;

    const float rOuter = r + 0.5f;
    const float rInner = r - thickness;
    const float rOuter2 = rOuter * rOuter;
    const float rInner2 = rInner * rInner;

    // Pixel-Plot mit Alpha-Schnellpfad: Vollalpha umgeht blendColor
    auto plotPixel = [&](int x, int y, float dy2)
    {
        const float dx = static_cast<float>(x) - xc;
        const float dist2 = dx * dx + dy2;

        if (dist2 > rOuter2 || dist2 < rInner2)
            return;

        const float dist = sqrtf(dist2);
        const float alphaOuter = fminf(1.0f, rOuter - dist);
        const float alphaInner = fminf(1.0f, dist - rInner);
        const float alpha = fmaxf(0.0f, fminf(alphaOuter, alphaInner));

        if (alpha >= 0.999f)
            drawPixel(x, y, color);
        else if (alpha > 0.0f)
            drawPixel(x, y, blendColor(color, bg, alpha));
    };

    const int yTop = static_cast<int>(yc - r - 1);
    const int yBottom = static_cast<int>(yc + r + 1);

    for (int y = yTop; y <= yBottom; ++y)
    {
        const float dy = static_cast<float>(y) - yc;
        const float dy2 = dy * dy;

        if (dy2 > rOuter2)
            continue;

        const float xOuter = sqrtf(rOuter2 - dy2);

        if (dy2 < rInner2)
        {
            // Zeile schneidet das innere Loch: linker und rechter Bogen separat
            const float xInner = sqrtf(rInner2 - dy2);

            const int xL0 = static_cast<int>(xc - xOuter - 1);
            const int xL1 = static_cast<int>(xc - xInner + 1);
            for (int x = xL0; x <= xL1; ++x)
                plotPixel(x, y, dy2);

            const int xR0 = static_cast<int>(xc + xInner - 1);
            const int xR1 = static_cast<int>(xc + xOuter + 1);
            for (int x = xR0; x <= xR1; ++x)
                plotPixel(x, y, dy2);
        }
        else
        {
            // Polkappen-Zeile: ein zusammenhängender Bogen über die volle Breite
            const int xL = static_cast<int>(xc - xOuter - 1);
            const int xR = static_cast<int>(xc + xOuter + 1);
            for (int x = xL; x <= xR; ++x)
                plotPixel(x, y, dy2);
        }
    }
}

/**
 * @brief Approximierte geometrische Distanz zwischen Punkt (dx, dy) und Ellipse
 */
static inline float ellipseDistance(float dx, float dy, float rx, float ry)
{
    // Normalisierte Koordinaten und ihre quadrierte Norm
    const float nx = dx / rx;
    const float ny = dy / ry;
    const float n2 = nx * nx + ny * ny;

    // Punkt im Mittelpunkt -> Distanz ist die kürzere Halbachse
    if (n2 < 0.000001f)
        return fminf(rx, ry);

    const float n = sqrtf(n2);
    return fabsf(n - 1.0f) / n * sqrtf(dx * dx + dy * dy);
}

/**
 * @brief Core-Funktion zum Zeichnen einer geglätteten Ellipsen-Outline
 */
template <typename DrawPixelFunc>
void drawSmoothEllipseCore(int xc, int yc, float rx, float ry, float thickness, uint16_t color, uint16_t bg, DrawPixelFunc drawPixel)
{
    // Validierung von Radien und Dicke
    if (rx <= 0.0f || ry <= 0.0f || thickness <= 0.0f)
        return;

    if (thickness > rx)
        thickness = rx;

    if (thickness > ry)
        thickness = ry;

    const float minR = fminf(rx, ry);
    const bool hasInnerEllipse = (thickness < minR);
    const float rxInner = rx - thickness;
    const float ryInner = ry - thickness;

    // Konservative Span-Grenzen: AA-Band ist immer +/- 1 Pixel breit, mit Padding 2 sicher abgedeckt
    const float rxAaOuter = rx + 2.0f;
    const float ryAaOuter = ry + 2.0f;
    const float ryAaOuter2 = ryAaOuter * ryAaOuter;

    const float rxAaHole = rx - 2.0f;
    const float ryAaHole = ry - 2.0f;
    const float ryAaHole2 = ryAaHole * ryAaHole;
    const bool hasAaHole = (rxAaHole > 0.0f && ryAaHole > 0.0f);

    // Pixel-Plot mit echter Distanzprüfung und Vollalpha-Schnellpfad
    auto plotPixel = [&](int x, int y, float dx, float dy)
    {
        const float outerDist = ellipseDistance(dx, dy, rx, ry);
        if (outerDist > 1.0f)
            return;

        const float alphaOuter = fminf(1.0f, fmaxf(0.0f, 1.0f - outerDist));
        float alphaInner = 1.0f;
        if (hasInnerEllipse)
        {
            const float innerDist = ellipseDistance(dx, dy, rxInner, ryInner);
            if (innerDist < 1.0f)
                alphaInner = fminf(1.0f, fmaxf(0.0f, innerDist));
        }

        const float alpha = alphaOuter * alphaInner;
        if (alpha >= 0.999f)
            drawPixel(x, y, color);
        else if (alpha > 0.0f)
            drawPixel(x, y, blendColor(color, bg, alpha));
    };

    const int yTop = static_cast<int>(yc - ry - 2);
    const int yBottom = static_cast<int>(yc + ry + 2);

    for (int y = yTop; y <= yBottom; ++y)
    {
        const float dy = static_cast<float>(y) - yc;
        const float dy2 = dy * dy;

        if (dy2 > ryAaOuter2)
            continue;

        const float xOuter = rxAaOuter * sqrtf(fmaxf(0.0f, 1.0f - dy2 / ryAaOuter2));

        if (hasAaHole && dy2 < ryAaHole2)
        {
            // Zeile schneidet das innere Loch: nur die zwei AA-Bögen iterieren
            const float xHole = rxAaHole * sqrtf(1.0f - dy2 / ryAaHole2);

            const int xL0 = static_cast<int>(xc - xOuter - 1);
            const int xL1 = static_cast<int>(xc - xHole + 1);
            for (int x = xL0; x <= xL1; ++x)
                plotPixel(x, y, static_cast<float>(x) - xc, dy);

            const int xR0 = static_cast<int>(xc + xHole - 1);
            const int xR1 = static_cast<int>(xc + xOuter + 1);
            for (int x = xR0; x <= xR1; ++x)
                plotPixel(x, y, static_cast<float>(x) - xc, dy);
        }
        else
        {
            // Polkappen-Zeile oder zu kleine Ellipse: durchgehender Span
            const int xL = static_cast<int>(xc - xOuter - 1);
            const int xR = static_cast<int>(xc + xOuter + 1);
            for (int x = xL; x <= xR; ++x)
                plotPixel(x, y, static_cast<float>(x) - xc, dy);
        }
    }
}

struct GaugeArcData
{
    float startX;
    float startY;
    float endX;
    float endY;
    float span;
    bool visible;
    bool fullCircle;
};

/**
 * @brief Normiert einen Winkel auf den Bereich 0 bis 2 PI
 */
static inline float normalizeGaugeAngle(float angle)
{
    constexpr float GAUGE_FULL_TURN = 6.283185307f;

    while (angle < 0.0f)
        angle += GAUGE_FULL_TURN;

    while (angle >= GAUGE_FULL_TURN)
        angle -= GAUGE_FULL_TURN;

    return angle;
}

/**
 * @brief Berechnet die im Uhrzeigersinn laufende Winkelspanne
 */
static inline float gaugeClockwiseSpan(float startRad, float endRad)
{
    constexpr float GAUGE_FULL_TURN = 6.283185307f;

    if (fabsf(endRad - startRad) >= GAUGE_FULL_TURN)
        return GAUGE_FULL_TURN;

    if (startRad <= endRad)
        return endRad - startRad;

    return GAUGE_FULL_TURN - normalizeGaugeAngle(startRad) + normalizeGaugeAngle(endRad);
}

/**
 * @brief Bereitet die Begrenzungsvektoren eines Gauge-Bogens vor
 */
static inline GaugeArcData makeGaugeArc(float startRad, float spanRad)
{
    constexpr float GAUGE_FULL_TURN = 6.283185307f;
    constexpr float FULL_CIRCLE_EPSILON = 0.0001f;

    GaugeArcData arc;
    arc.visible = (spanRad > 0.000001f);
    arc.fullCircle = (spanRad >= GAUGE_FULL_TURN - FULL_CIRCLE_EPSILON);

    if (spanRad > GAUGE_FULL_TURN)
        spanRad = GAUGE_FULL_TURN;

    arc.span = spanRad;

    const float normalizedStart = normalizeGaugeAngle(startRad);
    const float normalizedEnd = normalizeGaugeAngle(startRad + spanRad);

    arc.startX = sinf(normalizedStart);
    arc.startY = -cosf(normalizedStart);
    arc.endX = sinf(normalizedEnd);
    arc.endY = -cosf(normalizedEnd);

    return arc;
}

/**
 * @brief Prüft einen Punkt per Kreuzprodukt gegen einen vorberechneten Gauge-Bogen
 */
static inline bool isGaugeVectorInsideArc(float dx, float dy, const GaugeArcData &arc)
{
    if (!arc.visible)
        return false;

    if (arc.fullCircle)
        return true;

    constexpr float PI_VALUE = 3.141592654f;

    const float crossStartPoint = arc.startX * dy - arc.startY * dx;
    const float crossPointEnd = dx * arc.endY - dy * arc.endX;

    if (arc.span <= PI_VALUE)
        return crossStartPoint >= 0.0f && crossPointEnd >= 0.0f;

    const float crossEndPoint = arc.endX * dy - arc.endY * dx;
    const float crossPointStart = dx * arc.startY - dy * arc.startX;
    return !(crossEndPoint >= 0.0f && crossPointStart >= 0.0f);
}

/**
 * @brief Core-Funktion zum Zeichnen eines geglätteten Rundinstruments (Gauge)
 */
template <typename DrawPixelFunc>
void drawSmoothGaugeCore(int xc, int yc, float r, float thickness, float angleStartDeg, float angleEndDeg, float valueMin, float valueMax, float value, uint16_t color, uint16_t colorTrack, bool trackVisible, uint16_t bg, DrawPixelFunc drawPixel)
{
    if (r <= 0.0f || thickness <= 0.0f || thickness > r || valueMin >= valueMax)
        return;

    const float t = fmaxf(0.0f, fminf(1.0f, (value - valueMin) / (valueMax - valueMin)));

    constexpr float DEG_TO_RAD_FACTOR = 0.017453292f;

    const float angleStartRad = angleStartDeg * DEG_TO_RAD_FACTOR;
    const float angleEndRad = angleEndDeg * DEG_TO_RAD_FACTOR;
    const float angleActualRad = angleStartRad + (angleEndRad - angleStartRad) * t;

    const GaugeArcData valueArc = makeGaugeArc(angleStartRad, gaugeClockwiseSpan(angleStartRad, angleActualRad));
    const GaugeArcData trackArc = makeGaugeArc(angleStartRad, gaugeClockwiseSpan(angleStartRad, angleEndRad));

    const float rOuter = r + 0.5f;
    const float rInner = r - thickness;
    const float rOuter2 = rOuter * rOuter;
    const float rInner2 = rInner * rInner;

    auto plotPixel = [&](int x, int y, float dx, float dy, float dy2)
    {
        const float dist2 = dx * dx + dy2;
        if (dist2 > rOuter2 || dist2 < rInner2)
            return;

        const bool inValueRange = isGaugeVectorInsideArc(dx, dy, valueArc);
        bool inTrack = false;

        if (!inValueRange && trackVisible)
            inTrack = isGaugeVectorInsideArc(dx, dy, trackArc);

        if (!inValueRange && !inTrack)
            return;

        const float dist = sqrtf(dist2);
        const float alphaOuter = fminf(1.0f, rOuter - dist);
        const float alphaInner = fminf(1.0f, dist - rInner);
        const float alpha = fmaxf(0.0f, fminf(alphaOuter, alphaInner));

        const uint16_t pixelColor = inValueRange ? color : colorTrack;
        if (alpha >= 0.999f)
            drawPixel(x, y, pixelColor);
        else if (alpha > 0.0f)
            drawPixel(x, y, blendColor(pixelColor, bg, alpha));
    };

    const int yTop = static_cast<int>(yc - r - 1);
    const int yBottom = static_cast<int>(yc + r + 1);

    for (int y = yTop; y <= yBottom; ++y)
    {
        const float dy = static_cast<float>(y) - yc;
        const float dy2 = dy * dy;

        if (dy2 > rOuter2)
            continue;

        const float xOuter = sqrtf(rOuter2 - dy2);

        if (dy2 < rInner2)
        {
            const float xInner = sqrtf(rInner2 - dy2);

            const int xL0 = static_cast<int>(xc - xOuter - 1);
            const int xL1 = static_cast<int>(xc - xInner + 1);
            for (int x = xL0; x <= xL1; ++x)
                plotPixel(x, y, static_cast<float>(x) - xc, dy, dy2);

            const int xR0 = static_cast<int>(xc + xInner - 1);
            const int xR1 = static_cast<int>(xc + xOuter + 1);
            for (int x = xR0; x <= xR1; ++x)
                plotPixel(x, y, static_cast<float>(x) - xc, dy, dy2);
        }
        else
        {
            const int xL = static_cast<int>(xc - xOuter - 1);
            const int xR = static_cast<int>(xc + xOuter + 1);
            for (int x = xL; x <= xR; ++x)
                plotPixel(x, y, static_cast<float>(x) - xc, dy, dy2);
        }
    }
}


// ===================================================================
// Magische Augen EM11, EM34 und EM80
// ===================================================================

// Vorbild der EM34-Darstellung: EM34-Emulator von N. Waldt, veröffentlicht unter der MIT-Lizenz.

/**
 * @brief Namensraum für die Kennwerte der Abstimmröhren
 *
 * Die beiden runden Bauformen EM11 und EM34 teilen sich Leuchtstoff, Randbogen, Kantenlicht und Metallkappe und unterscheiden sich nur in der Geometrie ihrer Leuchtflächen; ihre Kennwerte tragen den Namensteil EYE oder den Namen der Röhre. Die EM80 ist ein stehender Rundbogen mit drei Leuchtfächern und bringt mit den Kennwerten EM80 ihre eigene Geometrie und ihren Rahmen mit; Leuchtstofffarben, Restleuchten und Randbogen teilt sie sich mit den runden Bauformen.
 *
 * Der Namensteil RATIO bezeichnet einen Anteil: bei den runden Bauformen am Schirmradius, bei der EM80 gibt es keinen, ihre Winkel stehen in Grad und ihre Längen in Pixeln. Alle übrigen Längen stehen in Pixeln. Sämtliche weichen Kanten werden über Pixelabstände gebildet und behalten dadurch bei jeder Sprite-Größe dieselbe Breite.
 */
namespace
{
    constexpr int EYE_SUPERSAMPLE = 1;    ///< Abtastungen je Achse und Bildpunkt
    constexpr float EYE_EDGE_AA = 0.8f;   ///< Halbe Breite jeder weichen Kante in Pixeln
    constexpr float EYE_DEG_TO_RAD = 0.01745329f; ///< Umrechnung von Grad in Radiant

    constexpr float EYE_EDGE_BAND_RATIO = 0.07f;  ///< Weicher Auslauf des Leuchtschirms nach außen
    constexpr float EYE_EDGE_BAND_MIN = 1.2f;     ///< Kleinster Auslauf des Leuchtschirms in Pixeln
    constexpr float EYE_RADIAL_BASE = 0.42f;      ///< Grundhelligkeit des Leuchtschirms in der Mitte
    constexpr float EYE_RADIAL_SPAN = 0.58f;      ///< Helligkeitszuwachs bis zum Schirmrand
    constexpr float EYE_RIM_INNER_RATIO = 0.051f; ///< Tiefe der Innenkante des hellen Randbogens unter dem Schirmrand
    constexpr float EYE_RIM_INNER_MIN = 1.6f;     ///< Kleinste Tiefe der Innenkante in Pixeln
    constexpr float EYE_RIM_OUTER_RATIO = 0.008f; ///< Tiefe der Außenkante des hellen Randbogens unter dem Schirmrand
    constexpr float EYE_RIM_OUTER_MIN = 0.4f;     ///< Kleinste Tiefe der Außenkante in Pixeln

    constexpr float EYE_BRIGHTNESS_BASE = 185.0f;        ///< Helligkeit des leuchtenden Phosphors
    constexpr float EYE_BRIGHTNESS_SHADOW = 46.0f;       ///< Restleuchten außerhalb der Leuchtflächen
    constexpr float EYE_BRIGHTNESS_RIM = 165.0f;         ///< Helligkeit des gelbgrünen Randbogens
    constexpr float EYE_BRIGHTNESS_EDGE = 72.0f;         ///< Helligkeit des Kantenlichts an den Hauptkanten
    constexpr float EYE_BRIGHTNESS_SECOND_EDGE = 52.0f;  ///< Helligkeit des Kantenlichts an den Nebenkeilen der EM34
    constexpr float EYE_BRIGHTNESS_CORNER = 92.0f;       ///< Helligkeit der Eckpunkte am Schirmrand
    constexpr float EYE_BRIGHTNESS_MAX = 255.0f;         ///< Obergrenze der Helligkeitsrechnung

    constexpr float EYE_EDGE_PEAK = 0.7f;        ///< Pixelabstand zur Kante mit dem hellsten Kantenlicht
    constexpr float EYE_EDGE_END = 2.0f;         ///< Pixelabstand, an dem das Kantenlicht erlischt
    constexpr float EYE_SECOND_EDGE_PEAK = 0.7f; ///< Pixelabstand mit dem hellsten Kantenlicht an den Nebenkeilen
    constexpr float EYE_SECOND_EDGE_END = 1.8f;  ///< Pixelabstand, an dem das Kantenlicht der Nebenkeile erlischt

    constexpr float EYE_CORNER_RADIUS_RATIO = 0.95f; ///< Abstand der Eckpunkte vom Mittelpunkt als Anteil des Radius
    constexpr float EYE_CORNER_CORE_RATIO = 0.05f;   ///< Radius des voll leuchtenden Eckkerns als Anteil des Radius
    constexpr float EYE_CORNER_CORE_MIN = 0.6f;      ///< Kleinster Radius des Eckkerns in Pixeln
    constexpr float EYE_CORNER_ROUND_RATIO = 0.14f;  ///< Radius, an dem das Eckleuchten erlischt, als Anteil des Radius
    constexpr float EYE_CORNER_ROUND_MIN = 2.0f;     ///< Kleinster Erlöschradius des Eckleuchtens in Pixeln

    constexpr float EYE_PHOSPHOR_RED = 0.11f;     ///< Rotanteil des Phosphorgrüns
    constexpr float EYE_PHOSPHOR_GREEN = 1.00f;   ///< Grünanteil des Phosphorgrüns
    constexpr float EYE_PHOSPHOR_BLUE = 0.26f;    ///< Blauanteil des Phosphorgrüns
    constexpr float EYE_RIM_RED = 0.60f;          ///< Rotanteil des gelbgrünen Randlichts
    constexpr float EYE_RIM_GREEN = 1.00f;        ///< Grünanteil des gelbgrünen Randlichts
    constexpr float EYE_RIM_BLUE = 0.07f;         ///< Blauanteil des gelbgrünen Randlichts
    constexpr float EYE_TINT_EDGE = 0.88f;        ///< Gelbanteil des Kantenlichts an den Hauptkanten
    constexpr float EYE_TINT_CORNER = 0.95f;      ///< Gelbanteil der Eckpunkte
    constexpr float EYE_TINT_SECOND_EDGE = 0.72f; ///< Gelbanteil des Kantenlichts an den Nebenkeilen

    constexpr float EYE_CAP_RADIUS_RATIO = 0.45f; ///< Außenradius der Metallkappe als Anteil des Schirmradius
    constexpr float EYE_CAP_GRAY_LIGHT = 75.0f;   ///< Helligkeit der Metallkappe in ihrer Mitte
    constexpr float EYE_CAP_GRAY_DARK = 30.0f;    ///< Helligkeit der Metallkappe an ihrem Rand
    constexpr float EYE_CAP_EDGE_START = 0.78f;   ///< Relative Position, ab der die Kappe zum Rand hin abdunkelt
    constexpr float EYE_CAP_EDGE_DARKEN = 22.0f;  ///< Stärke der Randabdunklung der Kappe
    constexpr float EYE_CAP_TINT_GREEN = 0.98f;   ///< Grünanteil des warm getönten Metallgraus
    constexpr float EYE_CAP_TINT_BLUE = 0.93f;    ///< Blauanteil des warm getönten Metallgraus

    constexpr float EM34_AXIS_ANGLE = -0.7853982f;   ///< Drehung der Keilachse in Radiant, ein negativer Wert neigt die Anzeige nach rechts oben
    constexpr float EM34_HALF_ANGLE_OPEN = 1.18f;    ///< Halber Schattenwinkel bei Pegel 0 in Radiant
    constexpr float EM34_HALF_ANGLE_CLOSED = 0.015f; ///< Halber Schattenwinkel bei vollem Pegel in Radiant
    constexpr float EM34_SENSITIVITY_LEFT = 1.18f;   ///< Empfindlichkeit des linken Hauptkeils, er schließt schon vor Vollausschlag
    constexpr float EM34_SENSITIVITY_RIGHT = 0.78f;  ///< Empfindlichkeit des rechten Hauptkeils, er bleibt bis zuletzt einen Spalt offen
    constexpr float EM34_SMALL_LENGTH_RATIO = 0.90f;       ///< Länge der Nebenkeile als Anteil des Radius
    constexpr float EM34_SMALL_HALF_WIDTH_RATIO = 0.1294f; ///< Halbe Breite der Nebenkeile am Schirmmittelpunkt
    constexpr float EM34_SMALL_TIP_RATIO = 0.02f;          ///< Halbe Breite der Nebenkeile an ihrer Spitze
    constexpr float EM34_SMALL_FADE_START = 0.90f;         ///< Relative Keillänge, ab der die Spitze ausläuft

    constexpr float EM11_AXIS_ANGLE = 0.0f;            ///< Drehung der Sektorachsen in Radiant, 0 stellt die Sektoren aufrecht
    constexpr float EM11_UPRIGHT_MIN_DEG = 12.0f;      ///< Öffnungswinkel der senkrechten Leuchtsektoren bei Pegel 0
    constexpr float EM11_UPRIGHT_MAX_DEG = 88.0f;      ///< Öffnungswinkel der senkrechten Leuchtsektoren bei Vollausschlag
    constexpr float EM11_CROSSWISE_MIN_DEG = 6.0f;     ///< Öffnungswinkel der waagerechten Leuchtsektoren bei Pegel 0
    constexpr float EM11_CROSSWISE_MAX_DEG = 70.0f;    ///< Öffnungswinkel der waagerechten Leuchtsektoren bei Vollausschlag
    constexpr float EM11_CROSSWISE_DELAY = 0.15f;      ///< Pegel, ab dem die waagerechten Sektoren merklich öffnen

    // Die EM80 ist keine Scheibe, sondern ein stehender Rundbogen mit senkrechten Seiten, halbkreisförmiger Oberkante und gerader Unterkante. Zwei schmale Schattenstrahlen laufen an der Bodenmitte zusammen und teilen den Schirm in DREI Leuchtfächer: einen mittleren, der mit dem Pegel aufklappt, und zwei äußere in den unteren Ecken, die dabei mitwachsen. Die Winkel sind an der Schattenbild-Tafel gemessen und lassen sich frei nachstellen.
    constexpr float EM80_RING_THICKNESS = 4.0f;       ///< Breite des dunklen Rahmens um den Leuchtschirm in Pixeln
    constexpr float EM80_INNER_MIN_DEG = 5.0f;        ///< Innenkante der Schattenstrahlen bei Pegel 0, zugleich der halbe Winkel des mittleren Fächers
    constexpr float EM80_INNER_MAX_DEG = 38.0f;       ///< Innenkante der Schattenstrahlen bei Vollausschlag
    constexpr float EM80_OUTER_OPEN_DEG = 80.0f;      ///< Außenkante der Schattenstrahlen bei Pegel 0, dahinter beginnen die äußeren Fächer
    constexpr float EM80_OUTER_CLOSED_DEG = 46.0f;    ///< Außenkante der Schattenstrahlen bei Vollausschlag
    constexpr float EM80_WEDGE_AA = 1.2f;             ///< Halbe Breite der Strahlenkanten in Pixeln, in der Vorlage weicher als der Schirmrand
    constexpr float EM80_BRIGHTNESS_BASE = 172.0f;    ///< Helligkeit in den Leuchtfächern
    constexpr float EM80_RADIAL_BASE = 0.70f;         ///< Helligkeit der Leuchtfächer am Zusammenlauf
    constexpr float EM80_RADIAL_SPAN = 0.30f;         ///< Helligkeitszuwachs der Leuchtfächer bis zur Schirmoberkante
    constexpr float EM80_EDGE_BAND = 1.4f;            ///< Weicher Auslauf des Leuchtschirms nach außen in Pixeln

    // Die EM84 ist ein Leuchtband. Zwei Balken wachsen von beiden Enden zur Mitte; bei Vollausschlag treffen sie sich und lassen nur einen dünnen dunklen Steg stehen. Das Verhältnis des Leuchtschirms stammt aus der Schattenbild-Tafel, die das Band liegend zeigt; hier steht es hochkant, weil das besser in die RetroSkala passt.
    constexpr float EM84_RING_THICKNESS = 4.0f;    ///< Breite des dunklen Rahmens um den Leuchtschirm in Pixeln
    constexpr float EM84_CORNER = 3.0f;            ///< Eckenradius des Leuchtschirms in Pixeln
    constexpr float EM84_BAR_MIN = 0.08f;          ///< Leuchtlänge je Balken bei Pegel 0 als Anteil der halben Schirmbreite
    constexpr float EM84_BAR_MAX = 0.98f;          ///< Leuchtlänge je Balken bei Vollausschlag, der Rest bleibt als schmale dunkle Naht
    constexpr float EM84_BAR_AA = 0.8f;            ///< Halbe Breite der Balkenkante in Pixeln, schmal genug damit die Naht bei Vollausschlag stehen bleibt
    constexpr float EM84_RADIAL_BASE = 0.70f;      ///< Helligkeit der Balken an ihrem inneren Ende
    constexpr float EM84_RADIAL_SPAN = 0.30f;      ///< Helligkeitszuwachs der Balken bis zum Schirmrand
    constexpr float EM84_EDGE_BAND = 1.4f;         ///< Weicher Auslauf des Leuchtschirms nach außen in Pixeln

    constexpr float EYE_RGB565_5BIT = 255.0f / 31.0f; ///< Streckung eines fünfstelligen Farbanteils auf den vollen Bereich
    constexpr float EYE_RGB565_6BIT = 255.0f / 63.0f; ///< Streckung eines sechsstelligen Farbanteils auf den vollen Bereich

    /**
     * @brief Bauform der nachgebildeten Abstimmröhre
     */
    enum class EyeGeometry
    {
        Em11, ///< Vier Leuchtsektoren, senkrecht und waagerecht, mit unterschiedlicher Öffnungscharakteristik
        Em34, ///< Zwei große Schattenkeile mit unterschiedlicher Empfindlichkeit und zwei feste Nebenkeile
        Em80, ///< Drei Leuchtfächer auf einem stehenden Rundbogen, geteilt von zwei Schattenstrahlen
        Em84  ///< Hochkant stehendes Leuchtband mit zwei Balken, die von den Enden zur Mitte wachsen
    };

    /**
     * @brief Umriss eines stehenden Rundbogens
     *
     * Senkrechte Seiten, halbkreisförmige Oberkante, gerade Unterkante. Alle Maße sind auf den Mittelpunkt des umschließenden Rechtecks bezogen.
     */
    struct EyeArch
    {
        float halfWidth;    ///< Halbe Breite des geraden Teils
        float arcCenterY;   ///< Höhe des Bogenmittelpunkts, darüber ist der Umriss kreisförmig
        float radiusSq;     ///< Quadrierter Bogenradius, er entspricht der halben Breite
        float invTwoRadius; ///< Kehrwert des doppelten Bogenradius, linearisiert den Randabstand
        float bottom;       ///< Höhe der Unterkante
    };

    /**
     * @brief Umriss eines Rechtecks mit abgerundeten Ecken
     *
     * Alle Maße sind auf den Mittelpunkt bezogen. Die Eckradien liegen als vorgezogene Halbmaße vor, damit der Pixelpfad ohne Subtraktion des Radius auskommt.
     */
    struct EyeRoundRect
    {
        float halfWidth;       ///< Halbe Breite
        float halfHeight;      ///< Halbe Höhe
        float innerHalfWidth;  ///< Halbe Breite abzüglich des Eckenradius
        float innerHalfHeight; ///< Halbe Höhe abzüglich des Eckenradius
        float radiusSq;        ///< Quadrierter Eckenradius
        float invTwoRadius;    ///< Kehrwert des doppelten Eckenradius, linearisiert den Randabstand
    };

    /**
     * @brief Alle je Aufruf einmalig berechneten Größen einer Abstimmröhre
     *
     * Zu jeder weichen Kante gehört der Kehrwert ihrer Breite, damit im Pixelinneren keine Division nötig ist.
     */
    struct EyeSetup
    {
        EyeGeometry geometry;    ///< Bauform, die den Zuschnitt der Leuchtflächen bestimmt
        int spanX;               ///< Halbe Breite des Durchlaufbereichs in Bildpunkten
        int spanTop;             ///< Oberste Zeile des Durchlaufbereichs relativ zum Anker
        int spanBottom;          ///< Unterste Zeile des Durchlaufbereichs relativ zum Anker
        float rowLimitSq;        ///< Quadrierter Außenradius für die zeilenweise Begrenzung der runden Bauformen
        float cosAxis;           ///< Kosinus der Achsdrehung
        float sinAxis;           ///< Sinus der Achsdrehung
        float radiusSq;          ///< Quadrierter Radius des Leuchtschirms
        float invTwoRadius;      ///< Kehrwert des doppelten Radius, wandelt Quadratabstände in Randtiefen
        float invRadiusSq;       ///< Kehrwert des quadrierten Radius
        float invAaSpan;         ///< Kehrwert der vollen Kantenbreite
        float invEdgeBand;       ///< Kehrwert der Schirmrandbreite
        float rimInnerDepth;     ///< Tiefe der Innenkante des Randbogens unter dem Schirmrand
        float rimOuterDepth;     ///< Tiefe der Außenkante des Randbogens unter dem Schirmrand
        float invEdgeRise;       ///< Kehrwert der Anstiegsbreite des Kantenlichts
        float invEdgeFall;       ///< Kehrwert der Abfallbreite des Kantenlichts
        float invSecondRise;     ///< Kehrwert der Anstiegsbreite des Kantenlichts an den Nebenkeilen
        float invSecondFall;     ///< Kehrwert der Abfallbreite des Kantenlichts an den Nebenkeilen
        float capRadius;         ///< Außenradius der Metallkappe
        float capRadiusSq;       ///< Quadrierter Außenradius der Metallkappe
        float invTwoCapRadius;   ///< Kehrwert des doppelten Kappenradius
        float invCapRadius;      ///< Kehrwert des Kappenradius
        float cornerCoreSq;      ///< Quadrierter Radius des Eckkerns
        float invCornerSpan;     ///< Kehrwert der quadrierten Spanne des Eckleuchtens
        float cornerAreaSq;      ///< Quadrierter Abstand, unterhalb dessen kein Eckleuchten geprüft wird
        float cosHalfLeft;       ///< Kosinus des halben Öffnungswinkels links, nur EM34
        float sinHalfLeft;       ///< Sinus des halben Öffnungswinkels links, nur EM34
        float cosHalfRight;      ///< Kosinus des halben Öffnungswinkels rechts, nur EM34
        float sinHalfRight;      ///< Sinus des halben Öffnungswinkels rechts, nur EM34
        float invSmallLength;    ///< Kehrwert der Nebenkeillänge, nur EM34
        float smallSpan;         ///< Differenz zwischen mittlerer und Spitzenbreite der Nebenkeile, nur EM34
        float smallTip;          ///< Halbe Breite der Nebenkeile an der Spitze, nur EM34
        float invFadeSpan;       ///< Kehrwert des Auslaufbereichs an der Keilspitze, nur EM34
        float cornerAlongLeft;   ///< Achsabstand der linken Eckpunkte, nur EM34
        float cornerAcrossLeft;  ///< Querabstand der linken Eckpunkte, nur EM34
        float cornerAlongRight;  ///< Achsabstand der rechten Eckpunkte, nur EM34
        float cornerAcrossRight; ///< Querabstand der rechten Eckpunkte, nur EM34
        float cosUpright;        ///< Kosinus des halben Sektorwinkels senkrecht, nur EM11
        float sinUpright;        ///< Sinus des halben Sektorwinkels senkrecht, nur EM11
        float cosCrosswise;      ///< Kosinus des halben Sektorwinkels waagerecht, nur EM11
        float sinCrosswise;      ///< Sinus des halben Sektorwinkels waagerecht, nur EM11
        float cornerUprightX;    ///< Querabstand der Eckpunkte an den senkrechten Sektoren, nur EM11
        float cornerUprightY;    ///< Achsabstand der Eckpunkte an den senkrechten Sektoren, nur EM11
        float cornerCrosswiseX;  ///< Achsabstand der Eckpunkte an den waagerechten Sektoren, nur EM11
        float cornerCrosswiseY;  ///< Querabstand der Eckpunkte an den waagerechten Sektoren, nur EM11
        EyeArch ringArch;        ///< Außenkante des dunklen Rahmens, nur EM80
        EyeArch screenArch;      ///< Kante des Leuchtschirms innerhalb des Rahmens, nur EM80
        float apexY;             ///< Höhe des Zusammenlaufs der Fächer an der Bodenmitte, nur EM80
        float cosInner;          ///< Kosinus der Innenkante der Schattenstrahlen, nur EM80
        float sinInner;          ///< Sinus der Innenkante der Schattenstrahlen, nur EM80
        float cosOuter;          ///< Kosinus der Außenkante der Schattenstrahlen, nur EM80
        float sinOuter;          ///< Sinus der Außenkante der Schattenstrahlen, nur EM80
        float invBeamSpanSq;     ///< Kehrwert des quadrierten Abstands vom Zusammenlauf zur Schirmoberkante, nur EM80
        float invWedgeAaSpan;    ///< Kehrwert der vollen Breite der Strahlenkanten, nur EM80
        EyeRoundRect ringRect;   ///< Außenkante des Rahmens, nur EM84
        EyeRoundRect screenRect; ///< Kante des Leuchtschirms innerhalb des Rahmens, nur EM84
        float gapHalfLength;     ///< Halbe Länge des dunklen Stegs zwischen den beiden Balken, nur EM84
        float invBarAaSpan;      ///< Kehrwert der vollen Breite der Balkenkante, nur EM84
        float invBandHalfLength; ///< Kehrwert der halben Bandlänge, nur EM84
        float ringRed;           ///< Rotanteil der Rahmenfarbe, nur EM80 und EM84
        float ringGreen;         ///< Grünanteil der Rahmenfarbe, nur EM80 und EM84
        float ringBlue;          ///< Blauanteil der Rahmenfarbe, nur EM80 und EM84
    };

    /**
     * @brief Farbe und Deckung einer einzelnen Abtastung
     */
    struct EyeSample
    {
        float red;      ///< Rotanteil von 0 bis 255
        float green;    ///< Grünanteil von 0 bis 255
        float blue;     ///< Blauanteil von 0 bis 255
        float coverage; ///< Anteil, zu dem die Abtastung auf dem Leuchtschirm liegt
    };
}

/**
 * @brief Weicher Hermite-Übergang mit vorberechnetem Kehrwert der Kantenbreite
 */
static inline float eyeRamp(float x, float edge0, float invSpan)
{
    float t = (x - edge0) * invSpan;
    t = constrain(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/**
 * @brief Füllt die Größen, die beide Bauformen gemeinsam haben
 */
static void eyePrepareShared(EyeSetup &s, float r, float axisAngle)
{
    s.cosAxis = cosf(axisAngle);
    s.sinAxis = sinf(axisAngle);

    // Runde Bauformen laufen zeilenweise nur über den Kreisausschnitt.
    const float outerRadius = r + 1.0f;
    s.rowLimitSq = outerRadius * outerRadius;
    s.spanX = static_cast<int>(ceilf(r)) + 1;
    s.spanTop = -s.spanX;
    s.spanBottom = s.spanX;

    s.radiusSq = r * r;
    s.invTwoRadius = 1.0f / (2.0f * r);
    s.invRadiusSq = 1.0f / s.radiusSq;
    s.invAaSpan = 1.0f / (2.0f * EYE_EDGE_AA);
    s.invEdgeBand = 1.0f / fmaxf(EYE_EDGE_BAND_MIN, r * EYE_EDGE_BAND_RATIO);
    s.rimInnerDepth = fmaxf(EYE_RIM_INNER_MIN, r * EYE_RIM_INNER_RATIO);
    s.rimOuterDepth = fmaxf(EYE_RIM_OUTER_MIN, r * EYE_RIM_OUTER_RATIO);

    s.invEdgeRise = 1.0f / EYE_EDGE_PEAK;
    s.invEdgeFall = 1.0f / (EYE_EDGE_END - EYE_EDGE_PEAK);
    s.invSecondRise = 1.0f / EYE_SECOND_EDGE_PEAK;
    s.invSecondFall = 1.0f / (EYE_SECOND_EDGE_END - EYE_SECOND_EDGE_PEAK);

    s.capRadius = r * EYE_CAP_RADIUS_RATIO;
    s.capRadiusSq = s.capRadius * s.capRadius;
    s.invTwoCapRadius = 1.0f / (2.0f * s.capRadius);
    s.invCapRadius = 1.0f / s.capRadius;

    const float core = fmaxf(EYE_CORNER_CORE_MIN, r * EYE_CORNER_CORE_RATIO);
    const float round = fmaxf(EYE_CORNER_ROUND_MIN, r * EYE_CORNER_ROUND_RATIO);
    s.cornerCoreSq = core * core;
    s.invCornerSpan = 1.0f / (round * round - s.cornerCoreSq);

    // Das Eckleuchten sitzt am Schirmrand; weiter innen wird es gar nicht erst geprüft.
    const float cornerArea = r * EYE_CORNER_RADIUS_RATIO - round;
    s.cornerAreaSq = cornerArea * cornerArea;
}

/**
 * @brief Bereitet eine EM34-Darstellung vor
 */
static EyeSetup eyePrepareEm34(float r, float level)
{
    EyeSetup s;
    s.geometry = EyeGeometry::Em34;
    eyePrepareShared(s, r, EM34_AXIS_ANGLE);

    const float levelLeft = fminf(level * EM34_SENSITIVITY_LEFT, 1.0f);
    const float levelRight = fminf(level * EM34_SENSITIVITY_RIGHT, 1.0f);
    const float halfLeft = EM34_HALF_ANGLE_OPEN + (EM34_HALF_ANGLE_CLOSED - EM34_HALF_ANGLE_OPEN) * levelLeft;
    const float halfRight = EM34_HALF_ANGLE_OPEN + (EM34_HALF_ANGLE_CLOSED - EM34_HALF_ANGLE_OPEN) * levelRight;

    s.cosHalfLeft = cosf(halfLeft);
    s.sinHalfLeft = sinf(halfLeft);
    s.cosHalfRight = cosf(halfRight);
    s.sinHalfRight = sinf(halfRight);

    s.invSmallLength = 1.0f / (r * EM34_SMALL_LENGTH_RATIO);
    s.smallTip = r * EM34_SMALL_TIP_RATIO;
    s.smallSpan = r * EM34_SMALL_HALF_WIDTH_RATIO - s.smallTip;
    s.invFadeSpan = 1.0f / (1.0f - EM34_SMALL_FADE_START);

    const float cornerRadius = r * EYE_CORNER_RADIUS_RATIO;
    s.cornerAlongLeft = cornerRadius * s.cosHalfLeft;
    s.cornerAcrossLeft = cornerRadius * s.sinHalfLeft;
    s.cornerAlongRight = cornerRadius * s.cosHalfRight;
    s.cornerAcrossRight = cornerRadius * s.sinHalfRight;

    return s;
}

/**
 * @brief Bereitet eine EM11-Darstellung vor
 *
 * Die senkrechten Sektoren öffnen früh und weit, die waagerechten später und schmaler.
 */
static EyeSetup eyePrepareEm11(float r, float level)
{
    EyeSetup s;
    s.geometry = EyeGeometry::Em11;
    eyePrepareShared(s, r, EM11_AXIS_ANGLE);

    const float uprightDeg = EM11_UPRIGHT_MIN_DEG + (EM11_UPRIGHT_MAX_DEG - EM11_UPRIGHT_MIN_DEG) * level * level;

    float delayed = (level - EM11_CROSSWISE_DELAY) / (1.0f - EM11_CROSSWISE_DELAY);
    delayed = constrain(delayed, 0.0f, 1.0f);
    const float eased = delayed * delayed * (3.0f - 2.0f * delayed);
    const float crosswiseDeg = EM11_CROSSWISE_MIN_DEG + (EM11_CROSSWISE_MAX_DEG - EM11_CROSSWISE_MIN_DEG) * eased;

    const float halfUpright = uprightDeg * 0.5f * EYE_DEG_TO_RAD;
    const float halfCrosswise = crosswiseDeg * 0.5f * EYE_DEG_TO_RAD;

    s.cosUpright = cosf(halfUpright);
    s.sinUpright = sinf(halfUpright);
    s.cosCrosswise = cosf(halfCrosswise);
    s.sinCrosswise = sinf(halfCrosswise);

    const float cornerRadius = r * EYE_CORNER_RADIUS_RATIO;
    s.cornerUprightX = cornerRadius * s.sinUpright;
    s.cornerUprightY = cornerRadius * s.cosUpright;
    s.cornerCrosswiseX = cornerRadius * s.cosCrosswise;
    s.cornerCrosswiseY = cornerRadius * s.sinCrosswise;

    return s;
}

/**
 * @brief Füllt einen Rundbogen aus halber Breite, Oberkante und Unterkante
 */
static void eyeFillArch(EyeArch &a, float halfWidth, float top, float bottom)
{
    a.halfWidth = halfWidth;
    a.arcCenterY = top + halfWidth;
    a.radiusSq = halfWidth * halfWidth;
    a.invTwoRadius = 1.0f / (2.0f * halfWidth);
    a.bottom = bottom;
}

/**
 * @brief Bereitet eine EM80-Darstellung vor
 *
 * Übergeben werden die halben Maße des umschließenden Rechtecks; darin liegt außen der dunkle Rahmen und innen der Leuchtschirm.
 */
static EyeSetup eyePrepareEm80(float halfWidth, float halfHeight, float level, uint16_t ring)
{
    EyeSetup s;
    s.geometry = EyeGeometry::Em80;

    // Der Rundbogen wird zeilenweise in voller Breite durchlaufen; die Deckung verwirft die Ecken.
    s.spanX = static_cast<int>(ceilf(halfWidth)) + 1;
    s.spanBottom = static_cast<int>(ceilf(halfHeight)) + 1;
    s.spanTop = -s.spanBottom;
    s.rowLimitSq = 0.0f;

    eyeFillArch(s.ringArch, halfWidth, -halfHeight, halfHeight);

    const float screenHalfWidth = halfWidth - EM80_RING_THICKNESS;
    const float screenTop = -halfHeight + EM80_RING_THICKNESS;
    const float screenBottom = halfHeight - EM80_RING_THICKNESS;
    const float screenHeight = screenBottom - screenTop;
    eyeFillArch(s.screenArch, screenHalfWidth, screenTop, screenBottom);

    // Die Innenkante der Schattenstrahlen wandert gleichmäßig nach außen, die Außenkante folgt erst spät und lässt dabei die äußeren Fächer wachsen.
    const float innerAngle = (EM80_INNER_MIN_DEG + (EM80_INNER_MAX_DEG - EM80_INNER_MIN_DEG) * level) * EYE_DEG_TO_RAD;
    const float outerAngle = (EM80_OUTER_OPEN_DEG - (EM80_OUTER_OPEN_DEG - EM80_OUTER_CLOSED_DEG) * level * level) * EYE_DEG_TO_RAD;
    s.cosInner = cosf(innerAngle);
    s.sinInner = sinf(innerAngle);
    s.cosOuter = cosf(outerAngle);
    s.sinOuter = sinf(outerAngle);

    // Alle Fächer laufen an der Bodenmitte des Leuchtschirms zusammen.
    s.apexY = screenBottom;
    const float beamSpan = fmaxf(1.0f, screenHeight);
    s.invBeamSpanSq = 1.0f / (beamSpan * beamSpan);
    s.invWedgeAaSpan = 1.0f / (2.0f * EM80_WEDGE_AA);

    // Weiche Kanten und Randbogen des Leuchtschirms
    s.invAaSpan = 1.0f / (2.0f * EYE_EDGE_AA);
    s.invEdgeBand = 1.0f / EM80_EDGE_BAND;
    s.rimInnerDepth = EYE_RIM_INNER_MIN;
    s.rimOuterDepth = EYE_RIM_OUTER_MIN;

    s.ringRed = static_cast<float>((ring >> 11) & 0x1F) * EYE_RGB565_5BIT;
    s.ringGreen = static_cast<float>((ring >> 5) & 0x3F) * EYE_RGB565_6BIT;
    s.ringBlue = static_cast<float>(ring & 0x1F) * EYE_RGB565_5BIT;

    return s;
}

/**
 * @brief Füllt ein Rechteck mit abgerundeten Ecken aus halben Maßen und Eckenradius
 */
static void eyeFillRoundRect(EyeRoundRect &r, float halfWidth, float halfHeight, float corner)
{
    r.halfWidth = halfWidth;
    r.halfHeight = halfHeight;
    r.innerHalfWidth = halfWidth - corner;
    r.innerHalfHeight = halfHeight - corner;
    r.radiusSq = corner * corner;
    r.invTwoRadius = 1.0f / (2.0f * corner);
}

/**
 * @brief Bereitet eine EM84-Darstellung vor
 *
 * Übergeben werden die halben Maße des umschließenden Rechtecks; darin liegt außen der dunkle Rahmen und innen das Leuchtband.
 */
static EyeSetup eyePrepareEm84(float halfWidth, float halfHeight, float level, uint16_t ring)
{
    EyeSetup s;
    s.geometry = EyeGeometry::Em84;

    // Das Leuchtband ist klein genug, um es zeilenweise in voller Breite zu durchlaufen.
    s.spanX = static_cast<int>(ceilf(halfWidth)) + 1;
    s.spanBottom = static_cast<int>(ceilf(halfHeight)) + 1;
    s.spanTop = -s.spanBottom;
    s.rowLimitSq = 0.0f;

    eyeFillRoundRect(s.ringRect, halfWidth, halfHeight, EM84_CORNER + EM84_RING_THICKNESS);

    const float screenHalfWidth = halfWidth - EM84_RING_THICKNESS;
    const float screenHalfHeight = halfHeight - EM84_RING_THICKNESS;
    eyeFillRoundRect(s.screenRect, screenHalfWidth, screenHalfHeight, EM84_CORNER);

    // Beide Balken wachsen gleich weit von den Enden nach innen; dazwischen bleibt der dunkle Steg. Das Band steht hochkant, seine Längsachse ist also die Höhe.
    const float litLength = EM84_BAR_MIN + (EM84_BAR_MAX - EM84_BAR_MIN) * level;
    s.gapHalfLength = screenHalfHeight * (1.0f - litLength);
    s.invBarAaSpan = 1.0f / (2.0f * EM84_BAR_AA);
    s.invBandHalfLength = 1.0f / fmaxf(1.0f, screenHalfHeight);

    // Weiche Kanten und Randbogen wie bei den runden Bauformen
    s.invAaSpan = 1.0f / (2.0f * EYE_EDGE_AA);
    s.invEdgeBand = 1.0f / EM84_EDGE_BAND;
    s.rimInnerDepth = EYE_RIM_INNER_MIN;
    s.rimOuterDepth = EYE_RIM_OUTER_MIN;

    s.ringRed = static_cast<float>((ring >> 11) & 0x1F) * EYE_RGB565_5BIT;
    s.ringGreen = static_cast<float>((ring >> 5) & 0x3F) * EYE_RGB565_6BIT;
    s.ringBlue = static_cast<float>(ring & 0x1F) * EYE_RGB565_5BIT;

    return s;
}

/**
 * @brief Linearisierter Abstand zum Rand eines Rechtecks mit abgerundeten Ecken, innerhalb der Fläche positiv
 *
 * Nur im Eckbereich wird der Kreisabstand aus dem Quadratabstand linearisiert, damit der Pixelpfad ohne Wurzel auskommt.
 */
static inline float eyeRoundRectDepth(const EyeRoundRect &r, float dx, float dy)
{
    const float across = fabsf(dx);
    const float along = fabsf(dy);
    const float overX = across - r.innerHalfWidth;
    const float overY = along - r.innerHalfHeight;

    if (overX <= 0.0f || overY <= 0.0f)
        return fminf(r.halfWidth - across, r.halfHeight - along);

    return (r.radiusSq - (overX * overX + overY * overY)) * r.invTwoRadius;
}

/**
 * @brief Linearisierter Abstand zum Rand eines Rundbogens, innerhalb der Fläche positiv
 *
 * Oberhalb des Bogenmittelpunkts wird der Kreisabstand aus dem Quadratabstand linearisiert, damit der Pixelpfad ohne Wurzel auskommt.
 */
static inline float eyeArchDepth(const EyeArch &a, float dx, float dy)
{
    const float floorDepth = a.bottom - dy;

    if (dy >= a.arcCenterY)
        return fminf(a.halfWidth - fabsf(dx), floorDepth);

    const float arcY = dy - a.arcCenterY;
    const float arcDepth = (a.radiusSq - (dx * dx + arcY * arcY)) * a.invTwoRadius;

    return fminf(arcDepth, floorDepth);
}

/**
 * @brief Helligkeit einer metallischen Kappe an einer relativen Position
 */
static inline float eyeCapGray(float position, float light, float dark)
{
    position = constrain(position, 0.0f, 1.0f);

    float gray = light - (light - dark) * position;
    gray -= eyeRamp(position, EYE_CAP_EDGE_START, 1.0f / (1.0f - EYE_CAP_EDGE_START)) * EYE_CAP_EDGE_DARKEN;

    return constrain(gray, 0.0f, EYE_BRIGHTNESS_MAX);
}

/**
 * @brief Berechnet Farbe und Deckung der runden Bauformen an einem Punkt relativ zum Schirmmittelpunkt
 *
 * Abstände zum Schirmrand werden aus dem Quadratabstand linearisiert, weil sie nur in Randnähe ausgewertet werden. Dadurch bleibt der Pixelpfad frei von Wurzeln; nur innerhalb der Metallkappe wird eine gezogen.
 */
static EyeSample eyeSampleRound(const EyeSetup &s, float dx, float dy)
{
    EyeSample out;
    out.red = 0.0f;
    out.green = 0.0f;
    out.blue = 0.0f;

    const float distSq = dx * dx + dy * dy;
    const float rimDepth = (s.radiusSq - distSq) * s.invTwoRadius;
    out.coverage = constrain(rimDepth + 0.5f, 0.0f, 1.0f);
    if (out.coverage <= 0.0f)
        return out;

    const float signedAlong = dx * s.cosAxis + dy * s.sinAxis;
    const float signedAcross = -dx * s.sinAxis + dy * s.cosAxis;
    const float along = fabsf(signedAlong);
    const float across = fabsf(signedAcross);

    const float edgeFade = eyeRamp(rimDepth, 0.0f, s.invEdgeBand);

    float visible = 0.0f;
    float edgeGlow = 0.0f;
    float secondEdgeGlow = 0.0f;
    float cornerGlow = 0.0f;

    if (s.geometry == EyeGeometry::Em34)
    {
        const bool leftHalf = (signedAlong < 0.0f);
        const float cosHalf = leftHalf ? s.cosHalfLeft : s.cosHalfRight;
        const float sinHalf = leftHalf ? s.sinHalfLeft : s.sinHalfRight;

        // Vorzeichenbehafteter Pixelabstand zur Keilkante; positive Werte liegen auf der leuchtenden Seite.
        const float wedgeDistance = across * cosHalf - along * sinHalf;
        const float mainVisible = eyeRamp(wedgeDistance, -EYE_EDGE_AA, s.invAaSpan);

        // Nebenkeile stehen senkrecht zu den Hauptkeilen und sind pegelunabhängig.
        const float wedgePos = fminf(across * s.invSmallLength, 1.0f);
        const float narrowing = 1.0f - wedgePos;
        const float halfWidth = s.smallTip + s.smallSpan * narrowing * narrowing;
        const float smallDistance = along - halfWidth;
        const float smallVisible = eyeRamp(smallDistance, -EYE_EDGE_AA, s.invAaSpan);
        const float wedgeFade = 1.0f - eyeRamp(wedgePos, EM34_SMALL_FADE_START, s.invFadeSpan);
        const float smallFactor = 1.0f - (1.0f - smallVisible) * wedgeFade;

        visible = mainVisible * smallFactor;

        if (wedgeDistance > 0.0f && wedgeDistance < EYE_EDGE_END)
        {
            edgeGlow = eyeRamp(wedgeDistance, 0.0f, s.invEdgeRise) *
                       (1.0f - eyeRamp(wedgeDistance, EYE_EDGE_PEAK, s.invEdgeFall)) * smallFactor * edgeFade;
        }

        if (smallDistance > 0.0f && smallDistance < EYE_SECOND_EDGE_END)
        {
            secondEdgeGlow = eyeRamp(smallDistance, 0.0f, s.invSecondRise) *
                             (1.0f - eyeRamp(smallDistance, EYE_SECOND_EDGE_PEAK, s.invSecondFall)) *
                             mainVisible * wedgeFade * edgeFade;
        }

        if (distSq > s.cornerAreaSq)
        {
            const float cornerAlong = leftHalf ? s.cornerAlongLeft : s.cornerAlongRight;
            const float cornerAcross = leftHalf ? s.cornerAcrossLeft : s.cornerAcrossRight;
            const float deltaAlong = along - cornerAlong;
            const float deltaAcross = across - cornerAcross;
            const float nearestSq = deltaAlong * deltaAlong + deltaAcross * deltaAcross;
            cornerGlow = (1.0f - eyeRamp(nearestSq, s.cornerCoreSq, s.invCornerSpan)) * visible * edgeFade;
        }
    }
    else
    {
        // Vier Leuchtsektoren: der Punkt leuchtet, wenn er in einem der beiden Sektorpaare liegt.
        const float uprightDistance = across * s.sinUpright - along * s.cosUpright;
        const float crosswiseDistance = along * s.sinCrosswise - across * s.cosCrosswise;
        const float litDistance = fmaxf(uprightDistance, crosswiseDistance);

        visible = eyeRamp(litDistance, -EYE_EDGE_AA, s.invAaSpan);

        if (litDistance > 0.0f && litDistance < EYE_EDGE_END)
        {
            edgeGlow = eyeRamp(litDistance, 0.0f, s.invEdgeRise) *
                       (1.0f - eyeRamp(litDistance, EYE_EDGE_PEAK, s.invEdgeFall)) * edgeFade;
        }

        if (distSq > s.cornerAreaSq)
        {
            const float deltaUprightX = along - s.cornerUprightX;
            const float deltaUprightY = across - s.cornerUprightY;
            const float deltaCrossX = along - s.cornerCrosswiseX;
            const float deltaCrossY = across - s.cornerCrosswiseY;
            const float nearestSq = fminf(deltaUprightX * deltaUprightX + deltaUprightY * deltaUprightY,
                                          deltaCrossX * deltaCrossX + deltaCrossY * deltaCrossY);
            cornerGlow = (1.0f - eyeRamp(nearestSq, s.cornerCoreSq, s.invCornerSpan)) * visible * edgeFade;
        }
    }

    const float radial = EYE_RADIAL_BASE + EYE_RADIAL_SPAN * distSq * s.invRadiusSq;

    float brightness = edgeFade * (EYE_BRIGHTNESS_BASE * visible * radial + EYE_BRIGHTNESS_SHADOW * (1.0f - visible));
    float yellowTint = EYE_TINT_EDGE * edgeGlow + EYE_TINT_SECOND_EDGE * secondEdgeGlow + EYE_TINT_CORNER * cornerGlow;

    brightness += EYE_BRIGHTNESS_EDGE * edgeGlow;
    brightness += EYE_BRIGHTNESS_SECOND_EDGE * secondEdgeGlow;
    brightness += EYE_BRIGHTNESS_CORNER * cornerGlow;

    // Heller Randbogen, nur in Randnähe
    if (rimDepth < s.rimInnerDepth + EYE_EDGE_AA)
    {
        const float rimBand = eyeRamp(rimDepth, s.rimOuterDepth - EYE_EDGE_AA, s.invAaSpan) *
                              (1.0f - eyeRamp(rimDepth, s.rimInnerDepth - EYE_EDGE_AA, s.invAaSpan));
        const float rimGlow = rimBand * visible * edgeFade;
        brightness += EYE_BRIGHTNESS_RIM * rimGlow;
        yellowTint += rimGlow;
    }

    brightness = fminf(brightness, EYE_BRIGHTNESS_MAX);
    yellowTint = constrain(yellowTint, 0.0f, 1.0f);

    out.red = brightness * (EYE_PHOSPHOR_RED + (EYE_RIM_RED - EYE_PHOSPHOR_RED) * yellowTint);
    out.green = brightness * (EYE_PHOSPHOR_GREEN + (EYE_RIM_GREEN - EYE_PHOSPHOR_GREEN) * yellowTint);
    out.blue = brightness * (EYE_PHOSPHOR_BLUE + (EYE_RIM_BLUE - EYE_PHOSPHOR_BLUE) * yellowTint);

    if (distSq < s.capRadiusSq + s.capRadius)
    {
        const float capDepth = (s.capRadiusSq - distSq) * s.invTwoCapRadius;
        const float capCoverage = constrain(capDepth + 0.5f, 0.0f, 1.0f);
        if (capCoverage > 0.0f)
        {
            const float gray = eyeCapGray(sqrtf(distSq) * s.invCapRadius, EYE_CAP_GRAY_LIGHT, EYE_CAP_GRAY_DARK);
            out.red = gray * capCoverage + out.red * (1.0f - capCoverage);
            out.green = gray * EYE_CAP_TINT_GREEN * capCoverage + out.green * (1.0f - capCoverage);
            out.blue = gray * EYE_CAP_TINT_BLUE * capCoverage + out.blue * (1.0f - capCoverage);
        }
    }

    return out;
}

/**
 * @brief Verrechnet Leuchtstoff, Randbogen und Rahmen für die Bauformen mit eigenem Rahmen
 * @param out Abtastung, deren Farbanteile gefüllt werden
 * @param s Vorbereitete Größen der Abstimmröhre
 * @param screenDepth Abstand zum Schirmrand, innerhalb der Fläche positiv
 * @param screenCoverage Anteil, zu dem die Abtastung auf dem Leuchtschirm liegt
 * @param visible Anteil, zu dem die Abtastung in einer Leuchtfläche liegt
 * @param radial Helligkeitsfaktor der Leuchtfläche an dieser Stelle
 * @param baseBrightness Helligkeit der voll ausgeleuchteten Fläche
 *
 * EM80 und EM84 unterscheiden sich nur im Zuschnitt ihrer Leuchtflächen; Restleuchten, Schirmauslauf, Randbogen und der Übergang zum Rahmen sind bei beiden gleich.
 */
static inline void eyeShadeFramedScreen(EyeSample &out, const EyeSetup &s, float screenDepth, float screenCoverage,
                                        float visible, float radial, float baseBrightness)
{
    const float edgeFade = eyeRamp(screenDepth, 0.0f, s.invEdgeBand);

    float brightness = edgeFade * (baseBrightness * visible * radial + EYE_BRIGHTNESS_SHADOW * (1.0f - visible));
    float yellowTint = 0.0f;

    // Heller Randbogen, nur in Randnähe; Helligkeit, Breite und Färbung entsprechen den runden Bauformen.
    if (screenDepth < s.rimInnerDepth + EYE_EDGE_AA)
    {
        const float rimBand = eyeRamp(screenDepth, s.rimOuterDepth - EYE_EDGE_AA, s.invAaSpan) *
                              (1.0f - eyeRamp(screenDepth, s.rimInnerDepth - EYE_EDGE_AA, s.invAaSpan));
        const float rimGlow = rimBand * visible * edgeFade;
        brightness += EYE_BRIGHTNESS_RIM * rimGlow;
        yellowTint = constrain(rimGlow, 0.0f, 1.0f);
    }

    brightness = fminf(brightness, EYE_BRIGHTNESS_MAX);

    const float red = brightness * (EYE_PHOSPHOR_RED + (EYE_RIM_RED - EYE_PHOSPHOR_RED) * yellowTint);
    const float green = brightness * (EYE_PHOSPHOR_GREEN + (EYE_RIM_GREEN - EYE_PHOSPHOR_GREEN) * yellowTint);
    const float blue = brightness * (EYE_PHOSPHOR_BLUE + (EYE_RIM_BLUE - EYE_PHOSPHOR_BLUE) * yellowTint);

    // Am Übergang zum Rahmen wird der Leuchtschirm gegen die Rahmenfarbe verrechnet.
    const float outside = 1.0f - screenCoverage;
    out.red = red * screenCoverage + s.ringRed * outside;
    out.green = green * screenCoverage + s.ringGreen * outside;
    out.blue = blue * screenCoverage + s.ringBlue * outside;
}

/**
 * @brief Berechnet Farbe und Deckung der EM80 an einem Punkt relativ zur Mitte des umschließenden Rechtecks
 *
 * Zwei Schattenstrahlen laufen an der Bodenmitte zusammen und teilen den Schirm in drei Leuchtfächer: der mittlere liegt innerhalb ihrer Innenkante, die beiden äußeren jenseits ihrer Außenkante. In den Strahlen glimmt der Schirm weiter, wie es die Vorlage zeigt. Alle Randabstände laufen über eyeArchDepth und bleiben damit wurzelfrei.
 */
static EyeSample eyeSampleEm80(const EyeSetup &s, float dx, float dy)
{
    EyeSample out;

    const float ringDepth = eyeArchDepth(s.ringArch, dx, dy);
    out.coverage = constrain(ringDepth + 0.5f, 0.0f, 1.0f);
    out.red = s.ringRed;
    out.green = s.ringGreen;
    out.blue = s.ringBlue;
    if (out.coverage <= 0.0f)
        return out;

    const float screenDepth = eyeArchDepth(s.screenArch, dx, dy);
    const float screenCoverage = constrain(screenDepth + 0.5f, 0.0f, 1.0f);
    if (screenCoverage <= 0.0f)
        return out;

    // Vorzeichenbehaftete Pixelabstände zu den beiden Kanten eines Schattenstrahls, gemessen vom Zusammenlauf an der Bodenmitte.
    const float alongBeam = s.apexY - dy;
    const float acrossBeam = fabsf(dx);
    const float innerDistance = alongBeam * s.sinInner - acrossBeam * s.cosInner;
    const float outerDistance = alongBeam * s.sinOuter - acrossBeam * s.cosOuter;

    // Es leuchtet, wer innerhalb der Innenkante liegt (mittlerer Fächer) oder jenseits der Außenkante (die beiden äußeren Fächer).
    const float visible = fmaxf(eyeRamp(innerDistance, -EM80_WEDGE_AA, s.invWedgeAaSpan),
                                eyeRamp(-outerDistance, -EM80_WEDGE_AA, s.invWedgeAaSpan));

    // Die Fächer werden zur Schirmoberkante hin heller; der Quadratabstand zum Zusammenlauf genügt als Maß.
    const float beamSq = fminf((dx * dx + alongBeam * alongBeam) * s.invBeamSpanSq, 1.0f);
    const float radial = EM80_RADIAL_BASE + EM80_RADIAL_SPAN * beamSq;

    eyeShadeFramedScreen(out, s, screenDepth, screenCoverage, visible, radial, EM80_BRIGHTNESS_BASE);

    return out;
}

/**
 * @brief Berechnet Farbe und Deckung der EM84 an einem Punkt relativ zur Mitte des umschließenden Rechtecks
 *
 * Das Band steht hochkant; zwei Balken wachsen von oben und unten zur Mitte, dazwischen bleibt ein dunkler Steg, der bei Vollausschlag nur noch als schmale Naht stehenbleibt. Alle Randabstände laufen über eyeRoundRectDepth und bleiben damit wurzelfrei.
 */
static EyeSample eyeSampleEm84(const EyeSetup &s, float dx, float dy)
{
    EyeSample out;

    const float ringDepth = eyeRoundRectDepth(s.ringRect, dx, dy);
    out.coverage = constrain(ringDepth + 0.5f, 0.0f, 1.0f);
    out.red = s.ringRed;
    out.green = s.ringGreen;
    out.blue = s.ringBlue;
    if (out.coverage <= 0.0f)
        return out;

    const float screenDepth = eyeRoundRectDepth(s.screenRect, dx, dy);
    const float screenCoverage = constrain(screenDepth + 0.5f, 0.0f, 1.0f);
    if (screenCoverage <= 0.0f)
        return out;

    // Vorzeichenbehafteter Pixelabstand zur Balkenkante; positive Werte liegen im Leuchtenden.
    const float along = fabsf(dy);
    const float visible = eyeRamp(along - s.gapHalfLength, -EM84_BAR_AA, s.invBarAaSpan);

    // Die Balken werden zu den Enden hin heller, wie die runden Bauformen zum Schirmrand.
    const float reach = fminf(along * s.invBandHalfLength, 1.0f);
    const float radial = EM84_RADIAL_BASE + EM84_RADIAL_SPAN * reach * reach;

    eyeShadeFramedScreen(out, s, screenDepth, screenCoverage, visible, radial, EYE_BRIGHTNESS_BASE);

    return out;
}

/**
 * @brief Core-Funktion zum Zeichnen einer Abstimmröhre
 *
 * Der Durchlaufbereich steht in der Vorbereitung; die runden Bauformen beschränken jede Bildzeile zusätzlich auf den Kreisausschnitt.
 */
template <typename TDisplay>
static void drawMagicEyeCore(TDisplay *display, int xc, int yc, const EyeSetup &setup, uint16_t bg)
{
    const float step = 1.0f / static_cast<float>(EYE_SUPERSAMPLE);
    const float firstOffset = step * 0.5f - 0.5f;
    const float invSampleCount = 1.0f / static_cast<float>(EYE_SUPERSAMPLE * EYE_SUPERSAMPLE);
    const bool isArch = (setup.geometry == EyeGeometry::Em80);
    const bool isBar = (setup.geometry == EyeGeometry::Em84);

    for (int offsetY = setup.spanTop; offsetY <= setup.spanBottom; ++offsetY)
    {
        const float rowY = static_cast<float>(offsetY);

        int halfWidth = setup.spanX;
        if (!isArch && !isBar)
        {
            const float rest = setup.rowLimitSq - rowY * rowY;
            if (rest <= 0.0f)
                continue;

            halfWidth = static_cast<int>(sqrtf(rest));
        }
        else if (isArch && rowY < setup.ringArch.arcCenterY)
        {
            // Oberhalb des Bogenmittelpunkts spart der Kreisausschnitt die leeren Ecken aus. Nur die EM80 füllt ringArch; das Leuchtband läuft immer über die volle Breite.
            const float arcY = rowY - setup.ringArch.arcCenterY;
            const float rest = setup.ringArch.radiusSq - arcY * arcY;
            if (rest <= 0.0f)
                continue;

            halfWidth = static_cast<int>(sqrtf(rest)) + 1;
        }

        for (int offsetX = -halfWidth; offsetX <= halfWidth; ++offsetX)
        {
            float sumRed = 0.0f;
            float sumGreen = 0.0f;
            float sumBlue = 0.0f;
            float sumCoverage = 0.0f;

            for (int sy = 0; sy < EYE_SUPERSAMPLE; ++sy)
            {
                const float dy = rowY + firstOffset + step * static_cast<float>(sy);

                for (int sx = 0; sx < EYE_SUPERSAMPLE; ++sx)
                {
                    const float dx = static_cast<float>(offsetX) + firstOffset + step * static_cast<float>(sx);
                    const EyeSample sample = isArch  ? eyeSampleEm80(setup, dx, dy)
                                             : isBar ? eyeSampleEm84(setup, dx, dy)
                                                     : eyeSampleRound(setup, dx, dy);
                    if (sample.coverage <= 0.0f)
                        continue;

                    sumRed += sample.red * sample.coverage;
                    sumGreen += sample.green * sample.coverage;
                    sumBlue += sample.blue * sample.coverage;
                    sumCoverage += sample.coverage;
                }
            }

            if (sumCoverage <= 0.0f)
                continue;

            const float invSum = 1.0f / sumCoverage;
            const uint32_t red = static_cast<uint32_t>(sumRed * invSum);
            const uint32_t green = static_cast<uint32_t>(sumGreen * invSum);
            const uint32_t blue = static_cast<uint32_t>(sumBlue * invSum);
            const uint16_t color = static_cast<uint16_t>(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));

            display->drawPixelClipped(xc + offsetX, yc + offsetY, blendColor(color, bg, sumCoverage * invSampleCount));
        }
    }
}

// ===================================================================
// Public Member-Funktionen
// ===================================================================




/**
 * @brief Zeichnet einen geglätteten Kreisring mit variablem Radius, Dicke und Farbe
 */
void LGFX::drawSmoothCircle(int xc, int yc, float r, float thickness, uint16_t color, uint16_t bg)
{
    drawSmoothCircleCore(xc, yc, r, thickness, color, bg, [this](int x, int y, uint16_t col)
                         { this->drawPixelClipped(x, y, col); });
}

/**
 * @brief Zeichnet einen geglätteten Kreisring innerhalb eines Sprites
 */
void Gfx_Sprite::drawSmoothCircle(int xc, int yc, float r, float thickness, uint16_t color, uint16_t bg)
{
    drawSmoothCircleCore(xc, yc, r, thickness, color, bg, [this](int x, int y, uint16_t col)
                         { this->drawPixelClipped(x, y, col); });
}

/**
 * @brief Zeichnet eine geglättete Ellipsen-Outline mit variablem Radius, Dicke und Farbe
 */
void LGFX::drawSmoothEllipse(int xc, int yc, float rx, float ry, float thickness, uint16_t color, uint16_t bg)
{
    drawSmoothEllipseCore(xc, yc, rx, ry, thickness, color, bg, [this](int x, int y, uint16_t col)
                          { this->drawPixelClipped(x, y, col); });
}

/**
 * @brief Zeichnet eine geglättete Ellipsen-Outline innerhalb eines Sprites
 */
void Gfx_Sprite::drawSmoothEllipse(int xc, int yc, float rx, float ry, float thickness, uint16_t color, uint16_t bg)
{
    drawSmoothEllipseCore(xc, yc, rx, ry, thickness, color, bg, [this](int x, int y, uint16_t col)
                          { this->drawPixelClipped(x, y, col); });
}

/**
 * @brief Zeichnet ein geglättetes Rundinstrument (Gauge) mit variablen Parametern
 */
void LGFX::drawSmoothGauge(int xc, int yc, float r, float thickness, float angleStart, float angleEnd, float valueMin, float valueMax, float value, uint16_t color, uint16_t colorTrack, bool trackVisible, uint16_t bg)
{
    this->startWrite();
    drawSmoothGaugeCore(xc, yc, r, thickness, angleStart, angleEnd, valueMin, valueMax, value, color, colorTrack, trackVisible, bg, [this](int x, int y, uint16_t col)
                        { this->drawPixelClipped(x, y, col); });
    this->endWrite();
}

/**
 * @brief Zeichnet ein geglättetes Rundinstrument (Gauge) innerhalb eines Sprites
 */
void Gfx_Sprite::drawSmoothGauge(int xc, int yc, float r, float thickness, float angleStart, float angleEnd, float valueMin, float valueMax, float value, uint16_t color, uint16_t colorTrack, bool trackVisible, uint16_t bg)
{
    drawSmoothGaugeCore(xc, yc, r, thickness, angleStart, angleEnd, valueMin, valueMax, value, color, colorTrack, trackVisible, bg, [this](int x, int y, uint16_t col)
                        { this->drawPixelClipped(x, y, col); });
}


/**
 * @brief Zeichnet die Nachbildung einer EM11-Abstimmröhre innerhalb eines Sprites
 */
void Gfx_Sprite::drawEm11MagicEye(int xc, int yc, float r, float level, uint16_t bg)
{
    if (r < 1.0f)
        return;

    level = constrain(level, 0.0f, 1.0f);
    drawMagicEyeCore(this, xc, yc, eyePrepareEm11(r, level), bg);
}

/**
 * @brief Zeichnet die Nachbildung einer EM34-Abstimmröhre innerhalb eines Sprites
 */
void Gfx_Sprite::drawEm34MagicEye(int xc, int yc, float r, float level, uint16_t bg)
{
    if (r < 1.0f)
        return;

    level = constrain(level, 0.0f, 1.0f);
    drawMagicEyeCore(this, xc, yc, eyePrepareEm34(r, level), bg);
}

/**
 * @brief Zeichnet die Nachbildung einer EM80-Abstimmröhre innerhalb eines Sprites
 *
 * Anders als bei den runden Bauformen gehört der dunkle Rahmen zur Zeichnung, weil er dem Rundbogen folgen muss.
 */
void Gfx_Sprite::drawEm80MagicEye(int xc, int yc, float halfWidth, float halfHeight, float level, uint16_t ring, uint16_t bg)
{
    // Innerhalb des Rahmens muss noch ein Leuchtschirm übrig bleiben.
    if (halfWidth < EM80_RING_THICKNESS + 2.0f || halfHeight < EM80_RING_THICKNESS + 2.0f)
        return;

    level = constrain(level, 0.0f, 1.0f);
    drawMagicEyeCore(this, xc, yc, eyePrepareEm80(halfWidth, halfHeight, level, ring), bg);
}

/**
 * @brief Zeichnet die Nachbildung einer EM84-Abstimmröhre innerhalb eines Sprites
 *
 * Wie bei der EM80 gehört der dunkle Rahmen zur Zeichnung, weil er dem liegenden Leuchtband folgen muss.
 */
void Gfx_Sprite::drawEm84MagicEye(int xc, int yc, float halfWidth, float halfHeight, float level, uint16_t ring, uint16_t bg)
{
    // Der Eckenradius des Rahmens darf nicht größer sein als seine halben Maße.
    const float minHalf = EM84_RING_THICKNESS + EM84_CORNER + 1.0f;
    if (halfWidth < minHalf || halfHeight < minHalf)
        return;

    level = constrain(level, 0.0f, 1.0f);
    drawMagicEyeCore(this, xc, yc, eyePrepareEm84(halfWidth, halfHeight, level, ring), bg);
}
