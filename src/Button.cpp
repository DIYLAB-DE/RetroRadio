/*
 * Diese Klasse geht auf lgfx::LGFX_Button aus LovyanGFX zurück und ist stark erweitert worden.
 * Die Herkunft zeigt sich unter anderem an _init_button, drawButton, contains, press, isPressed,
 * justPressed, justReleased, dem Zeichenrückruf und den Feldern für Textversatz und Ausrichtung.
 * LovyanGFX selbst folgt darin TFT_eSPI von Bodmer und dieses wiederum Adafruit_GFX.
 *
 * Copyright (c) 2020 lovyan03 (https://github.com/lovyan03)
 * Copyright (c) 2020 Bodmer (https://github.com/Bodmer)
 * Copyright (c) 2012 Adafruit Industries
 *
 * Weitergabe im Quelltext und in binärer Form, mit oder ohne Veränderung, ist unter der
 * FreeBSD-Lizenz mit zwei Bedingungen erlaubt, sofern dieser Urheberrechtsvermerk, die
 * Bedingungen und der Haftungsausschluss erhalten bleiben. Der vollständige Wortlaut steht in
 * docs/LICENSE-LovyanGFX.txt.
 */

#include "Button.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "StringUtils.h"

// ===================================================================
// Senkrechte Mitte der Beschriftung
// ===================================================================

namespace
{
    constexpr size_t VLW_HEADER_BYTES = 24;      ///< Länge des Kopfes einer VLW-Schrift
    constexpr size_t VLW_GLYPH_BYTES = 28;       ///< Länge eines Eintrags der Zeichentafel
    constexpr size_t VLW_ASCENT_OFFSET = 16;     ///< Lage der Oberlänge im Kopf
    constexpr size_t VLW_DESCENT_OFFSET = 20;    ///< Lage der Unterlänge im Kopf
    constexpr uint16_t VLW_CAPITAL_H = 'H';      ///< Zeichen, an dem die Versalhöhe gemessen wird
    constexpr uint8_t DATUM_VERTICAL_MIDDLE = 4; ///< Bit der Ausrichtung, das die senkrechte Mitte meint
    constexpr size_t FONT_CACHE_SIZE = 4;        ///< Anzahl der Schriften, deren Maß gemerkt wird

    /**
     * @brief Gemerkte Verschiebung einer Schrift
     */
    struct FontShift
    {
        const uint8_t *font; ///< Schrift, zu der der Wert gehört
        int16_t shift;       ///< Verschiebung gegenüber der Ausrichtung auf den Schriftkasten
    };

    /**
     * @brief Liest eine vierstellige Zahl aus einer VLW-Schrift, höchstwertiges Byte zuerst
     */
    int32_t vlwValue(const uint8_t *data, size_t offset)
    {
        return (static_cast<int32_t>(data[offset]) << 24) | (static_cast<int32_t>(data[offset + 1]) << 16) |
               (static_cast<int32_t>(data[offset + 2]) << 8) | static_cast<int32_t>(data[offset + 3]);
    }

    /**
     * @brief Bestimmt, um wie viel die Beschriftung tiefer gehört als die Ausrichtung auf die Mitte sie setzt
     *
     * LovyanGFX legt bei senkrechter Mitte den Schriftkasten mittig, also Oberlänge und Unterlänge über alle Zeichen der Schrift. Das Auge misst dagegen am Versalband. Bei League Gothic stammt die Unterlänge von 9 aus den Zeichen senkrechter Strich und geschweifte Klammer, die in keiner Beschriftung vorkommen; ohne Ausgleich säße jede Beschriftung zwei Bildpunkte zu hoch.
     */
    int16_t vlwMiddleShift(const uint8_t *font)
    {
        const int32_t glyphCount = vlwValue(font, 0);
        if (glyphCount <= 0)
            return 0;

        // LovyanGFX beginnt bei den Werten des Kopfes und hebt sie an, wo ein Zeichen weiter ausschlägt.
        int32_t capHeight = 0;
        int32_t ascent = vlwValue(font, VLW_ASCENT_OFFSET);
        int32_t descent = vlwValue(font, VLW_DESCENT_OFFSET);

        for (int32_t i = 0; i < glyphCount; ++i)
        {
            const size_t entry = VLW_HEADER_BYTES + static_cast<size_t>(i) * VLW_GLYPH_BYTES;
            const int32_t unicode = vlwValue(font, entry);
            const int32_t height = vlwValue(font, entry + 4);
            const int32_t top = vlwValue(font, entry + 16);

            if (top > ascent)
                ascent = top;
            if (height - top > descent)
                descent = height - top;
            if (unicode == VLW_CAPITAL_H)
                capHeight = height;
        }

        if (capHeight <= 0)
            return 0;

        return static_cast<int16_t>((capHeight - ascent + descent) / 2);
    }

    /**
     * @brief Liefert die Verschiebung einer Schrift und merkt sie sich
     */
    int16_t fontMiddleShift(const uint8_t *font)
    {
        if (!font)
            return 0;

        static FontShift cache[FONT_CACHE_SIZE] = {};
        static size_t used = 0;

        for (size_t i = 0; i < used; ++i)
        {
            if (cache[i].font == font)
                return cache[i].shift;
        }

        const int16_t shift = vlwMiddleShift(font);
        if (used < FONT_CACHE_SIZE)
        {
            cache[used].font = font;
            cache[used].shift = shift;
            ++used;
        }

        return shift;
    }
}

/**
 * @brief Interne Initialisierung des Buttons
 */
void Button::_init_button(LGFX *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, const char *label, const uint16_t *icon, float textsize_x, float textsize_y)
{
    m_tft = gfx;
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_r = r;
    m_icon = icon;
    m_textsize_x = textsize_x;
    _textsize_y = textsize_y <= std::numeric_limits<float>::epsilon()
                      ? textsize_x
                      : textsize_y;

    setLabelText(label);
}

/**
 * @brief Setzt den Button-Text
 */
void Button::setLabelText(const char *label)
{
    stringutils::safeCopy(m_label, label);
}

/**
 * @brief Gibt den Button-Text zurück
 */
char *Button::getLabelText()
{
    return m_label;
}

/**
 * @brief Setzt das Icon
 */
void Button::setIcon(const uint16_t *icon)
{
    m_icon = icon;
}

/**
 * @brief Setzt die Icon-Position
 */
void Button::setIconPosition(IconPosition pos)
{
    m_iconPos = pos;
}

/**
 * @brief Setzt die Icon-Größe
 */
void Button::setIconSize(int16_t width, int16_t height)
{
    m_iconWidth = width;
    m_iconHeight = height;
}

/**
 * @brief Setzt die Schriftart (Legacy-Format)
 */
void Button::setFont(const uint8_t *font)
{
    m_font_legacy = font;
    m_font_gfx = nullptr; // Sicherstellen, dass nur ein Font-Typ aktiv ist
}

/**
 * @brief Setzt die Schriftart (LGFX GFXfont Format)
 */
void Button::setFont(const lgfx::v1::GFXfont *font)
{
    m_font_gfx = font;
    m_font_legacy = nullptr; // Sicherstellen, dass nur ein Font-Typ aktiv ist
}

/**
 * @brief Aktiviert Zahlenkreis
 */
void Button::enableNumberCircle(uint8_t number)
{
    m_useNumberCircle = true;
    m_circleNumber = number;
}

/**
 * @brief Deaktiviert Zahlenkreis
 */
void Button::disableNumberCircle()
{
    m_useNumberCircle = false;
}

/**
 * @brief Setzt Text-Position und -Ausrichtung
 */
void Button::setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum)
{
    m_xd = x_delta;
    m_yd = y_delta;
    m_textdatum = datum;
}

/**
 * @brief Setzt einen zusätzlichen X/Y-Offset für das Label.
 */
void Button::setLabelOffset(int16_t x_offset, int16_t y_offset)
{
    m_labelOffsetX = x_offset;
    m_labelOffsetY = y_offset;
}

/**
 * @brief Prüft ob Label ein Symbol ist
 */
bool Button::isSymbolLabel(const char *label) const
{
    if (!label || strlen(label) < 3)
        return false;

    return (label[0] == '^' && label[strlen(label) - 1] == '^');
}

/**
 * @brief Extrahiert Größe und Farbe aus Symbol-Label
 * @param label Symbol-Label (z.B. "^POWER:24:F800^" oder "^STAR::07E0^")
 * @param defaultSize Standard-Größe
 * @param color [in/out] Farbe - wird nur geändert wenn im Label angegeben
 * @return Größe des Symbols
 */
int16_t Button::parseSymbolAttributes(const char *label, int16_t defaultSize, uint16_t &color) const
{
    const char *firstColon = strchr(label, ':');

    if (!firstColon)
        return defaultSize;

    // Größe parsen (wenn vorhanden)
    int16_t size = defaultSize;
    if (firstColon[1] != ':' && firstColon[1] != '^')
    {
        size = atoi(firstColon + 1);
        if (size <= 0 || size > 64)
            size = defaultSize;
    }

    // Farbe parsen (wenn vorhanden)
    const char *secondColon = strchr(firstColon + 1, ':');
    if (secondColon && secondColon[1] != '^')
    {
        // RGB565 Hex-Farbe parsen (z.B. F800 für rot, 07E0 für grün, 001F für blau)
        char *endPtr;
        unsigned long hexColor = strtoul(secondColon + 1, &endPtr, 16);

        // Nur übernehmen wenn gültiger Hex-Wert geparst wurde
        if (endPtr != secondColon + 1)
            color = (uint16_t)(hexColor & 0xFFFF);
    }

    return size;
}

/**
 * @brief Zeichnet Symbol basierend auf Label
 */
void Button::drawSymbolFromLabel(const char *label, int16_t x, int16_t y, uint16_t color)
{
    // Farbe kann durch Label überschrieben werden
    uint16_t symbolColor = color;
    int16_t size = parseSymbolAttributes(label, 24, symbolColor);

    if (strncmp(label, "^BACKSPACE", 10) == 0)
        drawBackSpaceSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^CAPSLOCK", 9) == 0)
        drawCapsLockSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^SHIFT", 6) == 0)
        drawShiftSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^EYEON", 6) == 0)
        drawEyeOnSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^EYEOFF", 7) == 0)
        drawEyeOffSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^LOCK", 5) == 0)
        drawLockSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^REFRESH", 8) == 0)
        drawRefreshSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^DEL", 4) == 0)
        drawDeleteSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^HOME", 5) == 0)
        drawHomeSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^SET", 4) == 0)
        drawSettingsSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^BACK", 5) == 0)
        drawBackSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^PLAY", 5) == 0)
        drawPlaySymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^PAUSE", 6) == 0)
        drawPauseSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^STOP", 5) == 0)
        drawStopSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^PLUS", 5) == 0)
        drawPlusSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^MINUS", 6) == 0)
        drawMinusSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^FORW", 5) == 0)
        drawForwardSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^BCKW", 5) == 0)
        drawBackwardSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^SKIPF", 6) == 0)
        drawSkipForwardSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^SKIPB", 6) == 0)
        drawSkipBackwardSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^STAR", 5) == 0)
        drawStarSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^POWER", 6) == 0)
        drawPowerSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^FOLDER", 7) == 0)
        drawFolderSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^CLEFT", 6) == 0)
        drawCLeftSymbol(x, y, size, symbolColor);
    else if (strncmp(label, "^CRIGHT", 7) == 0)
        drawCRightSymbol(x, y, size, symbolColor);
    // Fallback: normaler Text
    else
        target()->drawString(label, x, y);
}

/**
 * @brief Zeichnet Backspace-Symbol mit glatten Linien
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawBackSpaceSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    float scale = size / 24.0f;
    int16_t half = size / 2;

    // Koordinaten relativ zur Mitte berechnen
    int16_t left = x - half;
    int16_t top = y - half;

    // Rahmen des Backspace-Symbols zeichnen
    target()->drawSmoothLine(left + (7 * scale), top + (3 * scale), left + (22 * scale), top + (3 * scale), color);
    target()->drawSmoothLine(left + (7 * scale), top + (21 * scale), left + (22 * scale), top + (21 * scale), color);
    target()->drawSmoothLine(left + (22 * scale), top + (3 * scale), left + (22 * scale), top + (21 * scale), color);

    // Schräge Kanten links
    target()->drawSmoothLine(left + (0 * scale), top + (12 * scale), left + (7 * scale), top + (3 * scale), color);
    target()->drawSmoothLine(left + (0 * scale), top + (12 * scale), left + (7 * scale), top + (21 * scale), color);

    // Kreuz in der Mitte
    target()->drawSmoothLine(left + (9 * scale), top + (8 * scale), left + (18 * scale), top + (17 * scale), color);
    target()->drawSmoothLine(left + (9 * scale), top + (17 * scale), left + (18 * scale), top + (8 * scale), color);
}

/**
 * @brief Zeichnet Shift-Symbol
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln (nur Höhe wird skaliert!)
 * @param color Farbe des Symbols
 */
void Button::drawShiftSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Breite des Symbols
    const float BASE_WIDTH = 12.0f;

    // Nur Höhen-Skalierung
    float heightScale = size / 24.0f;
    float widthScale = BASE_WIDTH / 24.0f;

    // Koordinaten relativ zur Mitte berechnen
    int16_t halfWidth = BASE_WIDTH / 2;
    int16_t halfHeight = size / 2;
    int16_t left = x - halfWidth;
    int16_t top = y - halfHeight;

    // Koordinaten für die Pfeilform berechnen
    int16_t centerX = left + (12 * widthScale);
    int16_t tipY = top + (2 * heightScale);
    int16_t notchY = top + (10 * heightScale);
    int16_t bottomY = top + (22.5 * heightScale);
    int16_t leftX = left + (2.5 * widthScale);
    int16_t stemLeftX = left + (7 * widthScale);
    int16_t stemRightX = left + (17 * widthScale);
    int16_t rightX = left + (21.5 * widthScale);

    // Pfeilspitze
    target()->drawSmoothLine(centerX, tipY, leftX, notchY, color);
    target()->drawSmoothLine(centerX, tipY, rightX, notchY, color);

    // gerade Linien
    target()->drawLine(leftX, notchY, stemLeftX, notchY, color);        // Linke Einbuchtung
    target()->drawLine(stemLeftX, notchY, stemLeftX, bottomY, color);   // Linker Stiel
    target()->drawLine(stemLeftX, bottomY, stemRightX, bottomY, color); // Untere Verbindung
    target()->drawLine(stemRightX, bottomY, stemRightX, notchY, color); // Rechter Stiel
    target()->drawLine(stemRightX, notchY, rightX, notchY, color);      // Rechte Einbuchtung
}

/**
 * @brief Zeichnet Caps-Lock-Symbol (Pfeil mit Rechteck unten)
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln (nur Höhe wird skaliert!)
 * @param color Farbe des Symbols
 */
void Button::drawCapsLockSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Breite des Symbols
    const float BASE_WIDTH = 12.0f;

    // Nur Höhen-Skalierung
    float heightScale = size / 24.0f;
    float widthScale = BASE_WIDTH / 24.0f;

    // Koordinaten relativ zur Mitte berechnen
    int16_t halfWidth = BASE_WIDTH / 2;
    int16_t halfHeight = size / 2;
    int16_t left = x - halfWidth;
    int16_t top = y - halfHeight;

    // Koordinaten für die Pfeilform berechnen
    int16_t centerX = left + (12 * widthScale);
    int16_t tipY = top + (2 * heightScale);
    int16_t notchY = top + (10 * heightScale);
    int16_t stemBottomY = top + (17 * heightScale);
    int16_t leftX = left + (2.5 * widthScale);
    int16_t stemLeftX = left + (7 * widthScale);
    int16_t stemRightX = left + (17 * widthScale);
    int16_t rightX = left + (21.5 * widthScale);

    // Pfeilspitze
    target()->drawSmoothLine(centerX, tipY, leftX, notchY, color);
    target()->drawSmoothLine(centerX, tipY, rightX, notchY, color);

    // gerade Linien
    target()->drawLine(leftX, notchY, stemLeftX, notchY, color);                // Linke Einbuchtung
    target()->drawLine(stemLeftX, notchY, stemLeftX, stemBottomY, color);       // Linker Stiel
    target()->drawLine(stemLeftX, stemBottomY, stemRightX, stemBottomY, color); // Untere Verbindung
    target()->drawLine(stemRightX, stemBottomY, stemRightX, notchY, color);     // Rechter Stiel
    target()->drawLine(stemRightX, notchY, rightX, notchY, color);              // Rechte Einbuchtung

    // Rechteck unter dem Pfeil
    int16_t rectTop = top + (21 * heightScale);
    int16_t rectWidth = stemRightX - stemLeftX + 1;
    int16_t rectHeight = 5 * heightScale;
    target()->drawRect(stemLeftX, rectTop, rectWidth, rectHeight, color);
}

/**
 * @brief Zeichnet Eye-Symbol mit Kantenglättung
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawEyeOnSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    float scale = size / 24.0f;

    // Koordinaten relativ zur Mitte berechnen
    int16_t half = size / 2;
    int16_t left = x - half;
    int16_t top = y - half;

    // Auge als Ellipse zeichnen (Breite: 20px, Höhe: 12px bei 24px Basis)
    drawSmoothEllipseOnTarget(x, y, 12.0f * scale, 8.0f * scale, 1.7f, color, m_fillcolor);

    // Pupille als Kreis in der Mitte zeichnen (Radius: 3px bei 24px Basis)
    drawSmoothCircleOnTarget(x, y, 4 * scale, 1.7f, color, m_fillcolor);
}

/**
 * @brief Zeichnet Eye-Symbol mit Kantenglättung und Durchstreichung
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawEyeOffSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    float scale = size / 24.0f;

    // Koordinaten relativ zur Mitte berechnen
    int16_t half = size / 2;
    int16_t left = x - half;
    int16_t top = y - half;

    // Auge als Ellipse zeichnen (Breite: 20px, Höhe: 12px bei 24px Basis)
    drawSmoothEllipseOnTarget(x, y, 12.0f * scale, 8.0f * scale, 1.7f, color, m_fillcolor);

    // Pupille als Kreis in der Mitte zeichnen (Radius: 3px bei 24px Basis)
    drawSmoothCircleOnTarget(x, y, 4 * scale, 1.7f, color, m_fillcolor);

    // Durchstreichende Linie von links oben nach rechts unten
    target()->drawSmoothLine(left + (2 * scale), top + (6 * scale), left + (22 * scale), top + (18 * scale), TFT_WHITE);
}

/**
 * @brief Zeichnet geschlossenes Schloss-Symbol mit glatten Linien
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawLockSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    float scale = size / 24.0f;

    // Koordinaten relativ zur Mitte berechnen
    int16_t half = size / 2;
    int16_t left = x - half;
    int16_t top = y - half;

    // Schloss-Körper (Rechteck mit abgerundeten Ecken)
    int16_t body_x = left + (4 * scale);
    int16_t body_y = top + (10 * scale);
    int16_t body_w = 16 * scale + 1;
    int16_t body_h = 12 * scale;

    // Schloss-Bügel
    drawSmoothCircleOnTarget(x, y - (3 * scale), 5, 2.0f, color, m_fillcolor);

    // Rahmen
    target()->fillSmoothRoundRect(body_x, body_y, body_w, body_h, 2 * scale, color);
    target()->fillSmoothRoundRect(body_x + 1, body_y + 1, body_w - 2, body_h - 2, 2 * scale, m_fillcolor);

    // Schlüsselloch
    target()->fillSmoothCircle(x, y + (3 * scale), 2 * scale, color);
}

/**
 * @brief Zeichnet Refresh-Symbol mit glatten Linien
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawRefreshSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    float scale = size / 24.0f;

    // Geglätteten Kreis zeichnen
    drawSmoothCircleOnTarget(x, y, 10 * scale, 2 * scale, color, m_fillcolor);

    // Unteres Dreieck mit Hintergrundfarbe (löscht den Bereich rechts oben)
    target()->fillTriangle(
        x + (6 * scale), y - (6 * scale),   // Links
        x + (12 * scale), y - (12 * scale), // Rechts oben
        x + (12 * scale), y,                // Rechts mitte
        m_fillcolor);

    // Pfeilspitze rechts oben (zeigt nach rechts)
    target()->fillTriangle(
        x + (8 * scale), y - (8 * scale),  // Spitze (zeigt nach rechts)
        x + (4 * scale), y - (12 * scale), // Oben links
        x + (4 * scale), y - (4 * scale),  // Unten links
        color);
}

/**
 * @brief Zeichnet das Löschsymbol als Kreuz.
 */
void Button::drawDeleteSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t quarter = size / 4;

    // Kreuz zeichnen
    target()->drawWideLine(x - quarter, y - quarter, x + quarter, y + quarter, 1.5f, color);
    target()->drawWideLine(x + quarter, y - quarter, x - quarter, y + quarter, 1.5f, color);
}

/**
 * @brief Zeichnet Home-Symbol
 */
void Button::drawHomeSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t half = size / 2;
    int16_t quarter = size / 4;

    // Dach
    target()->drawLine(x, y - half, x - quarter, y - quarter, color);
    target()->drawLine(x, y - half, x + quarter, y - quarter, color);

    // Haus
    target()->drawRect(x - quarter, y - quarter, half, half, color);

    // Tür
    int16_t door_width = quarter / 2;
    target()->drawRect(x - door_width / 2, y, door_width, quarter, color);
}

/**
 * @brief Zeichnet Settings-Symbol (Zahnrad)
 */
void Button::drawSettingsSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t radius = size / 3;
    int16_t inner_radius = radius / 2;

    // Äußerer und innerer Kreis
    target()->drawCircle(x, y, radius, color);
    target()->drawCircle(x, y, inner_radius, color);

    // Zahnrad-Zähne
    for (int i = 0; i < 8; i++)
    {
        float angle = i * 45.0 * 3.14159 / 180.0;
        int16_t x1 = x + cos(angle) * radius;
        int16_t y1 = y + sin(angle) * radius;
        int16_t x2 = x + cos(angle) * (radius + 3);
        int16_t y2 = y + sin(angle) * (radius + 3);
        target()->drawLine(x1, y1, x2, y2, color);
    }
}

/**
 * @brief Zeichnet Back-Symbol (Pfeil nach links)
 */
void Button::drawBackSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t quarter = size / 4;

    // Pfeil nach links
    target()->drawLine(x - quarter, y, x + quarter, y - quarter, color);
    target()->drawLine(x - quarter, y, x + quarter, y + quarter, color);
    target()->drawLine(x - quarter, y, x + quarter / 2, y, color);
}

/**
 * @brief Zeichnet Play-Symbol
 */
void Button::drawPlaySymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t quarter = size / 4;

    // Dreieck
    target()->fillTriangle(x - quarter, y - quarter,
                        x - quarter, y + quarter,
                        x + quarter, y, color);
}

/**
 * @brief Zeichnet Pause-Symbol
 */
void Button::drawPauseSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t quarter = size / 4;
    int16_t bar_width = quarter / 2;

    // Zwei vertikale Balken
    target()->fillRect(x - quarter, y - quarter, bar_width, size / 2, color);
    target()->fillRect(x + quarter / 2, y - quarter, bar_width, size / 2, color);
}

/**
 * @brief Zeichnet Stop-Symbol
 */
void Button::drawStopSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t quarter = size / 4;

    // Quadrat
    target()->fillRect(x - quarter, y - quarter, size / 2, size / 2, color);
}

/**
 * @brief Zeichnet Plus-Symbol
 */
void Button::drawPlusSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t quarter = size / 4;
    int16_t thickness = 2;

    // Horizontaler und vertikaler Balken
    target()->fillRect(x - quarter, y - thickness / 2, size / 2, thickness, color);
    target()->fillRect(x - thickness / 2, y - quarter, thickness, size / 2, color);
}

/**
 * @brief Zeichnet Minus-Symbol
 */
void Button::drawMinusSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t quarter = size / 4;
    int16_t thickness = 2;

    // Horizontaler Balken
    target()->fillRect(x - quarter, y - thickness / 2, size / 2, thickness, color);
}

/**
 * @brief Zeichnet Forward-Symbol (doppelte spitze Klammer nach rechts)
 */
void Button::drawForwardSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    float scale = size / 24.0f;
    float lineWidth = 1.3f * scale;

    // Gesamtbreite des Symbols berechnen
    int16_t symbolWidth = 15 * scale; // Von -7.5 bis +7.5
    int16_t halfWidth = symbolWidth / 2;

    // Klammer-Breite und Abstand
    int16_t bracketWidth = 6 * scale;
    int16_t spacing = 2 * scale;

    // Positionen berechnen (perfekt zentriert)
    int16_t x1_left = x - halfWidth;
    int16_t x2_left = x1_left + bracketWidth;
    int16_t x1_right = x2_left + spacing;
    int16_t x2_right = x1_right + bracketWidth;

    int16_t y_top = y - (5 * scale);
    int16_t y_bottom = y + (5 * scale);

    // Klammern zeichnen
    target()->drawWideLine(x1_left, y_top, x2_left, y, lineWidth, color);
    target()->drawWideLine(x1_left, y_bottom, x2_left, y, lineWidth, color);
    target()->drawWideLine(x1_right, y_top, x2_right, y, lineWidth, color);
    target()->drawWideLine(x1_right, y_bottom, x2_right, y, lineWidth, color);
}

/**
 * @brief Zeichnet Backward-Symbol (doppelte spitze Klammer nach links)
 */
void Button::drawBackwardSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    float scale = size / 24.0f;
    float lineWidth = 1.3f * scale;

    int16_t symbolWidth = 15 * scale;
    int16_t halfWidth = symbolWidth / 2;
    int16_t bracketWidth = 6 * scale;
    int16_t spacing = 2 * scale;

    // Positionen berechnen
    int16_t x2_right = x + halfWidth;
    int16_t x1_right = x2_right - bracketWidth;
    int16_t x2_left = x1_right - spacing;
    int16_t x1_left = x2_left - bracketWidth;

    int16_t y_top = y - (5 * scale);
    int16_t y_bottom = y + (5 * scale);

    // Klammern zeichnen
    target()->drawWideLine(x2_right, y_top, x1_right, y, lineWidth, color);
    target()->drawWideLine(x2_right, y_bottom, x1_right, y, lineWidth, color);
    target()->drawWideLine(x2_left, y_top, x1_left, y, lineWidth, color);
    target()->drawWideLine(x2_left, y_bottom, x1_left, y, lineWidth, color);
}

/**
 * @brief Zeichnet Titelsprung-Symbol vorwärts (Doppeldreieck mit Haltebalken rechts)
 */
void Button::drawSkipForwardSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t half = size / 4;         // halbe Symbolhöhe
    int16_t triW = (size * 5) / 16;  // Breite eines Dreiecks
    int16_t barW = size / 10;        // Breite des Haltebalkens
    int16_t left = x - (2 * triW + barW) / 2;

    target()->fillTriangle(left, y - half, left, y + half, left + triW, y, color);
    target()->fillTriangle(left + triW, y - half, left + triW, y + half, left + 2 * triW, y, color);
    target()->fillRect(left + 2 * triW, y - half, barW, 2 * half, color);
}

/**
 * @brief Zeichnet Titelsprung-Symbol rückwärts (Haltebalken links, Doppeldreieck nach links)
 */
void Button::drawSkipBackwardSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    int16_t half = size / 4;
    int16_t triW = (size * 5) / 16;
    int16_t barW = size / 10;
    int16_t left = x - (2 * triW + barW) / 2;

    target()->fillRect(left, y - half, barW, 2 * half, color);
    target()->fillTriangle(left + barW + triW, y - half, left + barW + triW, y + half, left + barW, y, color);
    target()->fillTriangle(left + barW + 2 * triW, y - half, left + barW + 2 * triW, y + half, left + barW + triW, y, color);
}

/**
 * @brief Zeichnet Star-Symbol (5-zackiger Stern)
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawStarSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    float scale = size / 24.0f;

    // Radien für äußere und innere Punkte
    float outerRadius = 10 * scale;
    float innerRadius = 4 * scale;

    // Breite der Linien
    float lineWidth = 0.8f * scale;

    // 5 Zacken = 10 Punkte (5 außen, 5 innen)
    // Startwinkel bei -90° (oben)
    float angleStep = 72.0f; // 360° / 5 Zacken

    // Arrays für die Koordinaten
    int16_t outerX[5], outerY[5];
    int16_t innerX[5], innerY[5];

    // Punkte berechnen
    for (int i = 0; i < 5; i++)
    {
        // Äußere Punkte (Spitzen)
        float outerAngle = (-90.0f + i * angleStep) * PI / 180.0f;
        outerX[i] = x + cos(outerAngle) * outerRadius;
        outerY[i] = y + sin(outerAngle) * outerRadius;

        // Innere Punkte (zwischen den Spitzen)
        float innerAngle = (-90.0f + i * angleStep + angleStep / 2) * PI / 180.0f;
        innerX[i] = x + cos(innerAngle) * innerRadius;
        innerY[i] = y + sin(innerAngle) * innerRadius;
    }

    // Stern zeichnen - von jeder Spitze zu den zwei gegenüberliegenden inneren Punkten
    for (int i = 0; i < 5; i++)
    {
        // Von Spitze zum nächsten inneren Punkt
        target()->drawWideLine(outerX[i], outerY[i], innerX[i], innerY[i], lineWidth, color);

        // Von Spitze zum vorherigen inneren Punkt
        int prevInner = (i + 4) % 5; // (i - 1 + 5) % 5
        target()->drawWideLine(outerX[i], outerY[i], innerX[prevInner], innerY[prevInner], lineWidth, color);
    }
}

/**
 * @brief Zeichnet Power-Symbol (Ein/Aus Symbol)
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawPowerSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    float scale = size / 24.0f;

    // Kreisparameter
    float radius = 9 * scale;

    // Kreisbogen zeichnen (mit Öffnung oben)
    drawSmoothGaugeOnTarget(x, y, radius, 1.8f * scale, 30.0f, 330.0f, 0.0f, 1.0f, 1.0f, color, m_fillcolor, false, m_fillcolor);

    // Vertikale Linie in der Mitte zeichnen
    target()->drawWideLine(x, y - radius, x, y - (2 * scale), 1.3f * scale, color);
}

/**
 * @brief Zeichnet Ordner-Symbol (Folder-Icon)
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawFolderSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    const float scale = size / 24.0f;
    const int16_t lw = std::max(static_cast<int16_t>(2 * scale), static_cast<int16_t>(2)); ///< Linienbreite (min. 2px)

    // Ordner-Körper
    const int16_t bw = static_cast<int16_t>(20 * scale);
    const int16_t bh = static_cast<int16_t>(14 * scale);
    const int16_t bx = x - bw / 2;
    const int16_t by = y - bh / 2 + static_cast<int16_t>(2 * scale);
    const int16_t br = static_cast<int16_t>(2 * scale);

    // Körper: Farb-Rect -> Hintergrund-Rect (Lock-Pattern)
    target()->fillSmoothRoundRect(bx, by, bw, bh, br, color);
    const int16_t brInner = (br > lw) ? (br - lw) : 0;
    target()->fillSmoothRoundRect(bx + lw, by + lw, bw - 2 * lw, bh - 2 * lw, brInner, m_fillcolor);

    // Tab-Lasche oben links
    const int16_t tabW = static_cast<int16_t>(8 * scale);
    const int16_t tabH = static_cast<int16_t>(4 * scale);
    const int16_t tabX = bx;
    const int16_t tabY = by - tabH + lw;
    const int16_t tabR = static_cast<int16_t>(1.5f * scale);

    // Tab: Farb-Rect -> Hintergrund-Rect
    const int16_t tabRInner = (tabR > lw) ? (tabR - lw) : 0;
    target()->fillSmoothRoundRect(tabX, tabY, tabW, tabH + br, tabR, color);
    target()->fillSmoothRoundRect(tabX + lw, tabY + lw, tabW - 2 * lw, tabH + br - lw, tabRInner, m_fillcolor);
}

/**
 * @brief Zeichnet Cursor-Links-Symbol (Pfeil nach links)
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawCLeftSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    float scale = size / 24.0f;
    
    // Linienbreite für gut sichtbare Cursor
    float lineWidth = 1.5f * scale;
    
    // Koordinaten für den Pfeil
    int16_t arrowTipX = x - (10 * scale);      // Spitze links
    int16_t arrowBaseX = x + (10 * scale);     // Basis rechts
    int16_t arrowTopY = y - (8 * scale);       // Obere Linie
    int16_t arrowBottomY = y + (8 * scale);    // Untere Linie
    
    // Horizontale Linie (Pfeil-Stiel)
    target()->drawWideLine(arrowTipX + (3 * scale), y, arrowBaseX, y, lineWidth, color);
    
    // Obere schräge Linie (Pfeilspitze)
    target()->drawWideLine(arrowTipX, y, arrowTipX + (8 * scale), arrowTopY, lineWidth, color);
    
    // Untere schräge Linie (Pfeilspitze)
    target()->drawWideLine(arrowTipX, y, arrowTipX + (8 * scale), arrowBottomY, lineWidth, color);
}

/**
 * @brief Zeichnet Cursor-Rechts-Symbol (Pfeil nach rechts)
 * @param x X-Koordinate des Symbols (Zentrum)
 * @param y Y-Koordinate des Symbols (Zentrum)
 * @param size Größe des Symbols in Pixeln
 * @param color Farbe des Symbols
 */
void Button::drawCRightSymbol(int16_t x, int16_t y, int16_t size, uint16_t color)
{
    // Skalierung basierend auf 24px Referenzgröße
    float scale = size / 24.0f;
    
    // Linienbreite für gut sichtbare Cursor
    float lineWidth = 1.5f * scale;
    
    // Koordinaten für den Pfeil
    int16_t arrowTipX = x + (10 * scale);      // Spitze rechts
    int16_t arrowBaseX = x - (10 * scale);     // Basis links
    int16_t arrowTopY = y - (8 * scale);       // Obere Linie
    int16_t arrowBottomY = y + (8 * scale);    // Untere Linie
    
    // Horizontale Linie (Pfeil-Stiel)
    target()->drawWideLine(arrowBaseX, y, arrowTipX - (3 * scale), y, lineWidth, color);
    
    // Obere schräge Linie (Pfeilspitze)
    target()->drawWideLine(arrowTipX, y, arrowTipX - (8 * scale), arrowTopY, lineWidth, color);
    
    // Untere schräge Linie (Pfeilspitze)
    target()->drawWideLine(arrowTipX, y, arrowTipX - (8 * scale), arrowBottomY, lineWidth, color);
}

/**
 * @brief Zeichnet den Button
 */
void Button::drawButton(bool inverted, const char *long_name)
{
    // Benutzerdefinierte Zeichenfunktion verwenden
    if (m_drawCb)
    {
        m_drawCb(m_tft, m_x, m_y, m_w, m_h, m_r, inverted, long_name ? long_name : m_label);
        return;
    }

    // Text-Style sichern
    auto style = target()->getTextStyle();

    // Font setzen, je nachdem welcher Typ vorhanden ist
    if (m_font_gfx)
    {
        target()->setFont(m_font_gfx);
    }
    else if (m_font_legacy)
    {
        target()->loadFont(m_font_legacy);
    }

    // Farben für normalen/invertierten Zustand Bei gedrückt: nur Rahmen aufhellen, Füllung bleibt dunkel (kein heller Rand um Icons)
    auto fill = m_fillcolor;
    auto text = m_textcolor;
    auto outline = inverted ? m_textcolor : m_outlinecolor;

    // Rahmenbreite: 2px bei gedrückt (visuelles Feedback), 1px normal
    int16_t borderWidth = inverted ? 2 : 1;

    // Überschreibe Farben für den deaktivierten Zustand
    if (!m_enabled)
    {
        fill = 0x2104;    // Ein sehr dunkles Grau für den Hintergrund
        text = 0x5AEB;    // Ein gedimmtes, aber sichtbares Grau für den Text
        outline = 0x2104; // Rahmen wie Hintergrund (unsichtbar)
    }

    target()->setTextPadding(0);
    target()->setTextColor(text, fill);

    // Button-Position (oben links). Bei Sprite-Target werden die Display-Koordinaten in Sprite-Koordinaten umgerechnet (Sprite-Ursprung subtrahiert).
    int x0 = m_x - m_targetOriginX - (m_w / 2);
    int y0 = m_y - m_targetOriginY - (m_h / 2);

    // Button zeichnen
    target()->startWrite();
    target()->fillSmoothRoundRect(x0, y0, m_w, m_h, m_r, outline);
    target()->fillSmoothRoundRect(x0 + borderWidth, y0 + borderWidth,
                                  m_w - 2 * borderWidth, m_h - 2 * borderWidth,
                                  m_r, fill);

    int16_t icon_offset_x = 0;

    // Zahlenkreis zeichnen
    if (m_useNumberCircle)
    {
        int16_t circle_x = x0;

        switch (m_iconPos)
        {
        case IconPosition::Left:
            circle_x += 5 + (m_iconWidth >> 1);
            icon_offset_x = m_iconWidth + 10;
            break;
        case IconPosition::Center:
            circle_x += (m_w >> 1);
            break;
        case IconPosition::Right:
            circle_x += m_w - 5 - (m_iconWidth >> 1);
            break;
        }

        int16_t circle_y = y0 + (m_h >> 1);
        int16_t radius = m_iconWidth >> 1;

        target()->fillSmoothCircle(circle_x, circle_y, radius - 1, 0x39E8);
        target()->setTextDatum(MC_DATUM);
        target()->setTextColor(TFT_WHITE, 0x39E8);
        target()->drawNumber(m_circleNumber, circle_x, circle_y + 1);
    }
    // Icon zeichnen
    else if (m_icon)
    {
        int16_t icon_x = x0;

        switch (m_iconPos)
        {
        case IconPosition::Left:
            icon_x += 5;
            icon_offset_x = m_iconWidth + 10;
            break;
        case IconPosition::Center:
            icon_x += (m_w - m_iconWidth) / 2;
            break;
        case IconPosition::Right:
            icon_x += m_w - m_iconWidth - 5;
            break;
        }

        int16_t icon_y = y0 + (m_h - m_iconHeight) / 2;

        if (m_enabled)
        {
            // Button ist AKTIVIERT
            target()->pushImage(icon_x, icon_y, m_iconWidth, m_iconHeight, m_icon);
        }
        else
        {
            // Button ist DEAKTIVIERT
            const uint16_t disabled_fg = TFT_SILVER;
            const uint16_t disabled_bg = fill;

            for (int16_t j = 0; j < m_iconHeight; j++)
            {
                for (int16_t i = 0; i < m_iconWidth; i++)
                {
                    uint16_t original_pixel = m_icon[j * m_iconWidth + i];
                    float intensity = getIntensityFrom565(original_pixel);
                    if (intensity < 0.05f)
                        continue;

                    uint16_t blended_pixel = blendColor(disabled_fg, disabled_bg, intensity);
                    target()->drawPixel(icon_x + i, icon_y + j, blended_pixel);
                }
            }
        }
    }

    const char *display_text = long_name ? long_name : m_label;

    // Symbol oder Text zeichnen
    if (isSymbolLabel(display_text))
    {
        // Symbol zeichnen - respektiert jetzt textdatum wie normaler Text
        int16_t symbol_x = x0 + (m_w >> 1) + m_xd;

        // Position basierend auf Datum anpassen (wie bei normalem Text)
        switch (m_textdatum)
        {
        case ML_DATUM:
        case TL_DATUM:
        case BL_DATUM:
            symbol_x = x0 + 5 + m_xd + icon_offset_x;
            break;
        case MC_DATUM:
        case TC_DATUM:
        case BC_DATUM:
            symbol_x = x0 + (m_w >> 1) + m_xd + (icon_offset_x >> 1);
            break;
        case MR_DATUM:
        case TR_DATUM:
        case BR_DATUM:
            symbol_x = x0 + m_w - 5 + m_xd; // Symbole haben feste Breite
            break;
        }

        int16_t symbol_y = y0 + (m_h >> 1) + m_yd;

        drawSymbolFromLabel(display_text, symbol_x + m_labelOffsetX, symbol_y + m_labelOffsetY, text);
    }
    else
    {
        // Normaler Text
        int32_t text_width = target()->textWidth(display_text);
        int16_t text_x = x0 + (m_w >> 1) + m_xd;

        // Text-Position basierend auf Datum anpassen
        switch (m_textdatum)
        {
        case ML_DATUM:
        case TL_DATUM:
        case BL_DATUM:
            text_x = x0 + 5 + m_xd + icon_offset_x;
            break;
        case MC_DATUM:
        case TC_DATUM:
        case BC_DATUM:
            text_x = x0 + (m_w >> 1) + m_xd + (icon_offset_x >> 1);
            break;
        case MR_DATUM:
        case TR_DATUM:
        case BR_DATUM:
            text_x = x0 + m_w - text_width - 5 + m_xd;
            break;
        }

        // Bei senkrechter Mitte auf das Versalband ausrichten statt auf den Schriftkasten.
        const int16_t middleShift = (m_textdatum & DATUM_VERTICAL_MIDDLE) ? fontMiddleShift(m_font_legacy) : 0;

        target()->setTextColor(text, fill);
        target()->setTextDatum(m_textdatum);
        target()->drawString(display_text, text_x + m_labelOffsetX,
                             y0 + (m_h >> 1) + m_yd + m_labelOffsetY + middleShift);
    }

    target()->endWrite();

    // Font entladen, falls es ein Legacy-Font war
    if (m_font_legacy)
    {
        target()->unloadFont();
    }

    // Text-Style wiederherstellen
    target()->setTextStyle(style);
}
