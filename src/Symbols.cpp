#include "Symbols.h"

#include <Arduino.h>
#include <algorithm>
#include <cmath>
#include "Gfx.h"

// Konstruktor
/**
 * @brief Konstruktor der Symbols-Klasse
 */
Symbols::Symbols(LGFX* gfx) : _gfx(gfx) {
}

// ===================================================================
// Private Hilfsfunktionen
// ===================================================================

/**
 * @brief Zeichnet eine Wolke in ein Sprite
 */
void Symbols::drawCloud(Gfx_Sprite* sprite, float cx, float cy, float scale, int line_thickness, uint16_t fg_color, uint16_t bg_color) {
    const int margin_thickness = line_thickness;

    // Radien für die drei Wolkenbereiche
    const float p1_side_r = 18.0f * scale, p1_main_r = 24.0f * scale;
    const float p2_side_r = p1_side_r - margin_thickness, p2_main_r = p1_main_r - margin_thickness;
    const float p3_side_r = p2_side_r - line_thickness, p3_main_r = p2_main_r - line_thickness;

    // Positionen der Wolkenteile
    const float side_x_offset = 25.0f * scale, main_puff_x_offset = 4.0f * scale;
    const float main_puff_y = cy - (4.0f * scale), side_puff_y = cy + (6.0f * scale);

    // Äußerer Rand (Hintergrundfarbe)
    sprite->fillSmoothCircle(round(cx - side_x_offset), round(side_puff_y), round(p1_side_r), bg_color);
    sprite->fillSmoothCircle(round(cx + side_x_offset), round(side_puff_y), round(p1_side_r), bg_color);
    sprite->fillSmoothCircle(round(cx + main_puff_x_offset), round(main_puff_y), round(p1_main_r), bg_color);
    sprite->fillRect(round(cx - side_x_offset), round(side_puff_y), round(side_x_offset * 2), round(p1_side_r) + 1, bg_color);

    // Sichtbare Kontur (Vordergrundfarbe)
    sprite->fillSmoothCircle(round(cx - side_x_offset), round(side_puff_y), round(p2_side_r), fg_color);
    sprite->fillSmoothCircle(round(cx + side_x_offset), round(side_puff_y), round(p2_side_r), fg_color);
    sprite->fillSmoothCircle(round(cx + main_puff_x_offset), round(main_puff_y), round(p2_main_r), fg_color);
    sprite->fillRect(round(cx - side_x_offset), round(side_puff_y), round(side_x_offset * 2), round(p2_side_r) + 1, fg_color);

    // Innerer Ausstanzer (Hintergrundfarbe)
    if (p3_main_r > 0) {
        sprite->fillSmoothCircle(round(cx - side_x_offset), round(side_puff_y), round(p3_side_r), bg_color);
        sprite->fillSmoothCircle(round(cx + side_x_offset), round(side_puff_y), round(p3_side_r), bg_color);
        sprite->fillSmoothCircle(round(cx + main_puff_x_offset), round(main_puff_y), round(p3_main_r), bg_color);
        sprite->fillRect(round(cx - side_x_offset + line_thickness), round(side_puff_y), round(side_x_offset * 2 - line_thickness * 2), round(p2_side_r) + 1 - line_thickness, bg_color);
    }
}

/**
 * @brief Zeichnet einen Mond in ein Sprite
 */
void Symbols::drawMoon(Gfx_Sprite* sprite, float moon_cx, float moon_cy, float scale, uint16_t fg_color, uint16_t bg_color) {
    const float moon_thickness = std::max(1.5f, static_cast<float>(round(scale * 90.0f / 45.0f + 0.55f)));
    const float moon_r = 30.0f * scale;
    const float inner_offset = moon_r * 0.65f;

    // Mond als Vollkreis zeichnen
    sprite->drawSmoothCircle(round(moon_cx), round(moon_cy), round(moon_r), moon_thickness, fg_color, bg_color);

    // Stück rausbeißen
    sprite->fillSmoothCircle(round(moon_cx + inner_offset), round(moon_cy - inner_offset), moon_r - moon_thickness, TFT_BLACK);

    // Der innere Bogen des Mondes
    sprite->drawSmoothGauge(round(moon_cx + inner_offset), round(moon_cy - inner_offset), moon_r, moon_thickness, 163.0f, 285.0f, 0.0f, 1.0f, 1.0f, fg_color, fg_color, false, bg_color);
}

/**
 * @brief Zeichnet Regen in ein Sprite
 */
void Symbols::drawRain(Gfx_Sprite* sprite, float cx, float cy, float scale, int line_thickness, uint16_t fg_color, int skip_index) {
    const float p1_side_r = 18.0f * scale;
    const float side_x_offset = 25.0f * scale;
    const float side_puff_y = cy + (6.0f * scale);

    // Regen-Parameter
    const int num_drops = 4;
    const float rain_length = 12.0f * scale;
    const float rain_angle = -30.0f * M_PI / 180.0f;
    const float dx = sin(rain_angle), dy = cos(rain_angle);

    const float drop_spacing = (side_x_offset * 1.2f) / (num_drops - 1);
    const float start_offset_x = cx - (side_x_offset * 0.6f);

    // Regentropfen zeichnen
    for (int i = 0; i < num_drops; ++i) {
        if (i == skip_index) continue;  // Tropfen überspringen (für Blitz)

        float start_x = start_offset_x + (i * drop_spacing);
        float start_y = side_puff_y + p1_side_r + (5 * scale);
        float end_x = start_x + rain_length * dx;
        float end_y = start_y + rain_length * dy;

        sprite->drawWedgeLine(start_x, start_y, end_x, end_y, line_thickness * 0.5f, line_thickness * 0.5f, fg_color);
    }
}

// ===================================================================
// Öffentliche show...-Funktionen
// ===================================================================

/**
 * @brief Zeichnet eine Sonne hinter einer Wolke
 */
void Symbols::showSunAndCloud(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Sonne zeichnen
    const int cloud_width_ref = static_cast<int>(width * (2.0f / 3.0f));
    const float sun_scale = cloud_width_ref / 90.0f;
    const float sun_thickness = std::max(1.0f, static_cast<float>(round(cloud_width_ref / 45.0f + 0.55f)));
    const float sun_r = 16.0f * sun_scale;
    const float sun_cx = width * 0.7f, sun_cy = height * 0.35f;

    // Sonnenkörper zeichnen
    sprite->drawSmoothCircle(round(sun_cx), round(sun_cy), round(sun_r), sun_thickness, fg_color, bg_color);

    // Sonnenstrahlen
    for (int i = 0; i < 8; ++i) {
        float angle = i * (360.0f / 8) * M_PI / 180.0f;
        float x1 = sun_cx + cos(angle) * (sun_r * 1.5f), y1 = sun_cy + sin(angle) * (sun_r * 1.5f);
        float x2 = sun_cx + cos(angle) * (sun_r * 2.3f), y2 = sun_cy + sin(angle) * (sun_r * 2.3f);
        sprite->drawWedgeLine(x1, y1, x2, y2, sun_thickness / 2.0f, sun_thickness / 2.0f, fg_color);
    }

    // Wolke zeichnen
    const float scale = cloud_width_ref / 90.0f;
    const float cx = width * 0.45f, cy = height * 0.5f;
    const int line_thickness = std::max(1, static_cast<int>(round(cloud_width_ref / 45.0f + 0.55f)));
    drawCloud(sprite, cx, cy, scale, line_thickness, fg_color, bg_color);

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}

/**
 * @brief Zeichnet einen Mond hinter einer Wolke
 */
void Symbols::showMoonAndCloud(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Mond zeichnen
    const int cloud_width_ref = static_cast<int>(width * (2.0f / 3.0f));
    const float moon_scale = cloud_width_ref / 90.0f;
    const float moon_cx = width * 0.7f, moon_cy = height * 0.35f;
    drawMoon(sprite, moon_cx, moon_cy, moon_scale, fg_color, bg_color);

    // Wolke zeichnen
    const float scale = cloud_width_ref / 90.0f;
    const float cx = width * 0.45f, cy = height * 0.5f;
    const int line_thickness = std::max(1, static_cast<int>(round(cloud_width_ref / 45.0f + 0.55f)));
    drawCloud(sprite, cx, cy, scale, line_thickness, fg_color, bg_color);

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}

/**
 * @brief Zeichnet eine Wolke
 */
void Symbols::showCloud(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Wolke zeichnen
    const float scale = (width * (2.0f / 3.0f)) / 90.0f;
    const float cx = width * 0.5f, cy = height * 0.5f;
    const int line_thickness = std::max(1, static_cast<int>(round((width * (2.0f / 3.0f)) / 45.0f + 0.55f)));
    drawCloud(sprite, cx, cy, scale, line_thickness, fg_color, bg_color);

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}

/**
 * @brief Zeichnet eine Sonne
 */
void Symbols::showSun(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Sonne zeichnen
    const int sun_diameter = static_cast<int>(width * (2.0f / 3.0f));
    const float scale = sun_diameter / 90.0f;
    const float sun_thickness = std::max(1.0f, sun_diameter / 25.0f);
    const float sun_r = 16.0f * scale;
    const float cx = width * 0.5f, cy = height * 0.5f;

    // Sonnenkörper zeichnen
    sprite->drawSmoothCircle(round(cx), round(cy), round(sun_r), sun_thickness, fg_color, bg_color);

    // Sonnenstrahlen
    for (int i = 0; i < 8; ++i) {
        float angle = i * (360.0f / 8) * M_PI / 180.0f;
        float x1 = cx + cos(angle) * (sun_r * 1.5f), y1 = cy + sin(angle) * (sun_r * 1.5f);
        float x2 = cx + cos(angle) * (sun_r * 2.3f), y2 = cy + sin(angle) * (sun_r * 2.3f);
        sprite->drawWedgeLine(x1, y1, x2, y2, sun_thickness / 2.0f, sun_thickness / 2.0f, fg_color);
    }

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}

/**
 * @brief Zeichnet eine Mondsichel
 */
void Symbols::showMoon(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Mond zeichnen
    const float moon_scale = (width * (2.0f / 3.0f)) / 90.0f;
    const float moon_cx = width * 0.5f, moon_cy = height * 0.5f;
    drawMoon(sprite, moon_cx, moon_cy, moon_scale, fg_color, bg_color);

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}

/**
 * @brief Zeichnet einen Mond mit Wolke und Regen
 */
void Symbols::showMoonAndCloudAndRain(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Mond zeichnen
    const int ref_width = static_cast<int>(width * (2.0f / 3.0f));
    const float moon_scale = ref_width / 90.0f;
    const float moon_cx = width * 0.7f, moon_cy = height * 0.35f;
    drawMoon(sprite, moon_cx, moon_cy, moon_scale, fg_color, bg_color);

    // Wolke zeichnen
    const float scale = ref_width / 90.0f;
    const float cx = width * 0.45f, cy = height * 0.5f;
    const int line_thickness = std::max(1, static_cast<int>(round(ref_width / 45.0f + 0.55f)));
    drawCloud(sprite, cx, cy, scale, line_thickness, fg_color, bg_color);

    // Regen zeichnen
    drawRain(sprite, cx, cy, scale, line_thickness, fg_color);

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}

/**
 * @brief Zeichnet eine regnende Wolke
 */
void Symbols::showCloudAndRain(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Wolke und Regen zeichnen
    const float scale = (width * (2.0f / 3.0f)) / 90.0f;
    const float cx = width * 0.5f, cy = height * 0.5f;
    const int line_thickness = std::max(1, static_cast<int>(round((width * (2.0f / 3.0f)) / 45.0f + 0.55f)));
    drawCloud(sprite, cx, cy, scale, line_thickness, fg_color, bg_color);
    drawRain(sprite, cx, cy, scale, line_thickness, fg_color);

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}

/**
 * @brief Zeichnet eine Wolke mit Schneeflocken
 */
void Symbols::showCloudAndSnow(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Wolke zeichnen
    const float scale = (width * (2.0f / 3.0f)) / 90.0f;
    const float cx = width * 0.5f, cy = height * 0.5f;
    const int line_thickness = std::max(1, static_cast<int>(round((width * (2.0f / 3.0f)) / 45.0f + 0.55f)));
    drawCloud(sprite, cx, cy, scale, line_thickness, fg_color, bg_color);

    // Schneeflocken zeichnen
    const float p1_side_r = 18.0f * scale;
    const float side_puff_y = cy + (6.0f * scale);
    const float snowflake_size = 8.0f * scale;
    const float snow_y = side_puff_y + p1_side_r + (15 * scale);

    // Linke Schneeflocke (4 Linien = 8 Strahlen)
    float left_snow_x = cx - (25.0f * scale * 0.5f);
    for (int i = 0; i < 4; ++i) {
        float angle = (i * M_PI / 4);
        float dx = cos(angle) * snowflake_size, dy = sin(angle) * snowflake_size;
        sprite->drawWedgeLine(left_snow_x - dx, snow_y - dy, left_snow_x + dx, snow_y + dy, line_thickness * 0.6f, line_thickness * 0.6f, fg_color);
    }

    // Rechte Schneeflocke (3 Linien = 6 Strahlen)
    float right_snow_x = cx + (25.0f * scale * 0.5f);
    float right_snow_y = snow_y - (2 * scale);
    const float right_snowflake_size = snowflake_size * 0.7f;
    for (int i = 0; i < 3; ++i) {
        float angle = (i * M_PI / 3);
        float dx = cos(angle) * right_snowflake_size, dy = sin(angle) * right_snowflake_size;
        sprite->drawWedgeLine(right_snow_x - dx, right_snow_y - dy, right_snow_x + dx, right_snow_y + dy, line_thickness * 0.6f, line_thickness * 0.6f, fg_color);
    }

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}

/**
 * @brief Zeichnet eine Wolke mit Nebel
 */
void Symbols::showCloudAndFog(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Wolke zeichnen
    const float scale = (width * (2.0f / 3.0f)) / 90.0f;
    const float cx = width * 0.5f, cy = height * 0.5f;
    const int line_thickness = std::max(1, static_cast<int>(round((width * (2.0f / 3.0f)) / 45.0f + 0.55f)));
    drawCloud(sprite, cx, cy, scale, line_thickness, fg_color, bg_color);

    // Nebel zeichnen
    const float p1_side_r = 18.0f * scale;
    const float side_puff_y = cy + (6.0f * scale);
    const float fog_y = side_puff_y + p1_side_r + (5 * scale);
    const float fog_length = 25.0f * scale * 1.8f;
    const float fog_thickness = line_thickness * 1.5f;
    const float gap_size = fog_length * 0.25f;

    // Obere Nebellinie (Lücke in der Mitte)
    float upper_y = fog_y, line_start_x = cx - fog_length / 2, line_end_x = cx + fog_length / 2;
    sprite->drawWedgeLine(line_start_x, upper_y, cx - gap_size / 2, upper_y, fog_thickness / 2, fog_thickness / 2, fg_color);
    sprite->drawWedgeLine(cx + gap_size / 2, upper_y, line_end_x, upper_y, fog_thickness / 2, fog_thickness / 2, fg_color);

    // Untere Nebellinie (Lücke bei 3/4)
    float lower_y = fog_y + (8 * scale), lower_length = fog_length * 0.7f;
    float lower_start_x = cx - lower_length / 2, lower_end_x = cx + lower_length / 2;
    float lower_gap_position = lower_length * 0.75f;
    sprite->drawWedgeLine(lower_start_x, lower_y, lower_start_x + lower_gap_position - gap_size / 2, lower_y, fog_thickness / 2, fog_thickness / 2, fg_color);
    sprite->drawWedgeLine(lower_start_x + lower_gap_position + gap_size / 2, lower_y, lower_end_x, lower_y, fog_thickness / 2, fog_thickness / 2, fg_color);

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}

/**
 * @brief Zeichnet eine Wolke mit Gewitter (Regen und Blitz)
 */
void Symbols::showCloudAndThunderstorm(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target) {
    // Setup & Validierung
    width = std::max(width, 30);
    const int height = static_cast<int>(width * (70.0f / 90.0f));

    Gfx_Sprite* sprite = nullptr;
    if (!_gfx->safeCreateSprite(sprite, width, height)) return;
    sprite->clear(bg_color);

    // Wolke zeichnen
    const float scale = (width * (2.0f / 3.0f)) / 90.0f;
    const float cx = width * 0.5f, cy = height * 0.5f;
    const int line_thickness = std::max(1, static_cast<int>(round((width * (2.0f / 3.0f)) / 45.0f + 0.55f)));
    drawCloud(sprite, cx, cy, scale, line_thickness, fg_color, bg_color);

    // Blitz zeichnen
    const float p1_main_r = 24.0f * scale;
    const float side_puff_y = cy + (6.0f * scale), main_puff_y = cy - (4.0f * scale);
    const float lightning_thickness = line_thickness * 1.8f;
    const float bolt_x = cx + (25.0f * scale * 0.4f);
    const float bolt_start_y = main_puff_y - p1_main_r - (5 * scale);
    const float bolt_width = 15.0f * scale;

    // Zickzack-Blitz: Oberer Teil
    sprite->drawWedgeLine(bolt_x, bolt_start_y, cx - (bolt_width * 0.3f), side_puff_y, lightning_thickness, lightning_thickness, fg_color);

    // Mittelteil
    sprite->drawWedgeLine(cx - (bolt_width * 0.3f), side_puff_y, cx + (bolt_width * 0.7f), side_puff_y - (5 * scale), lightning_thickness, lightning_thickness, fg_color);

    // Unterer Teil
    const float final_x = cx + (bolt_width * 0.7f), final_y = side_puff_y - (5 * scale);
    const float rain_end_y = side_puff_y + 18.0f * scale + (5 * scale) + 12.0f * scale * cos(-30.0f * M_PI / 180.0f);
    const float bottom_length = (rain_end_y - final_y) / cos(-30.0f * M_PI / 180.0f);
    sprite->drawWedgeLine(final_x, final_y, final_x + bottom_length * sin(-30.0f * M_PI / 180.0f), rain_end_y, lightning_thickness, lightning_thickness * 0.3f, fg_color);

    // Regen zeichnen (einen Tropfen für Blitz auslassen)
    drawRain(sprite, cx, cy, scale, line_thickness, fg_color, 1);

    // Ausgabe: auf Ziel-Sprite oder Display
    if (target)
        sprite->pushSprite(target, x, y);
    else
        sprite->pushSprite(x, y);
    _gfx->safeDeleteSprite(sprite);
}
