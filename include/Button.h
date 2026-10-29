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

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

#include "Gfx.h"

/**
 * @brief Button-Klasse
 */
class Button
{
public:
    /**
     * @brief Position des Icons innerhalb des Buttons
     */
    enum class IconPosition
    {
        Left,   ///< Icon links vom Text
        Center, ///< Icon zentriert (ohne Text)
        Right   ///< Icon rechts vom Text
    };

    /**
     * @brief Initialisiert den Button mit allen Parametern
     *
     * @param gfx Pointer auf das LGFX Display-Objekt
     * @param x X-Position des Button-Zentrums
     * @param y Y-Position des Button-Zentrums
     * @param w Breite des Buttons
     * @param h Höhe des Buttons
     * @param r Radius für abgerundete Ecken
     * @param outline Farbe des Rahmens
     * @param fill Füllfarbe des Buttons
     * @param textcolor Textfarbe
     * @param label Beschriftung des Buttons
     * @param icon Pointer auf Icon-Daten (RGB565)
     * @param textsize_x Horizontale Textgröße
     * @param textsize_y Vertikale Textgröße (0 = wie textsize_x)
     */
    template <typename T>
    void initButton(LGFX *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
                    uint16_t r, const T &outline, const T &fill, const T &textcolor,
                    const char *label, const uint16_t *icon,
                    float textsize_x = 1.0f, float textsize_y = 0.0f)
    {
        m_outlinecolor = outline;
        m_fillcolor = fill;
        m_textcolor = textcolor;
        _init_button(gfx, x, y, w, h, r, label, icon, textsize_x, textsize_y);
    }

    /**
     * @brief Setzt den Button-Text
     * @param label Neuer Text für den Button
     */
    void setLabelText(const char *label);

    /**
     * @brief Gibt den aktuellen Button-Text zurück
     * @return Pointer auf den Text
     */
    char *getLabelText();

    /**
     * @brief Setzt das Icon für den Button
     * @param icon Pointer auf Icon-Daten (RGB565)
     */
    void setIcon(const uint16_t *icon);

    /**
     * @brief Setzt die Position des Icons
     * @param pos Icon-Position (Left, Center, Right)
     */
    void setIconPosition(IconPosition pos);

    /**
     * @brief Setzt die Größe des Icons
     * @param width Icon-Breite in Pixeln
     * @param height Icon-Höhe in Pixeln
     */
    void setIconSize(int16_t width, int16_t height);

    /**
     * @brief Setzt die Schriftart (Legacy-Format)
     * @param font Pointer auf Font-Daten (uint8_t Array)
     */
    void setFont(const uint8_t *font);

    /**
     * @brief Setzt die Schriftart (LGFX GFXfont Format)
     * @param font Pointer auf GFXfont-Struktur
     */
    void setFont(const lgfx::v1::GFXfont *font);

    /**
     * @brief Aktiviert einen Zahlenkreis anstelle des Icons
     * @param number Anzuzeigende Zahl (0-99)
     */
    void enableNumberCircle(uint8_t number);

    /**
     * @brief Deaktiviert den Zahlenkreis
     */
    void disableNumberCircle();

    /**
     * @brief Setzt die Text-Position relativ zum Button-Zentrum
     * @param x_delta X-Verschiebung vom Zentrum
     * @param y_delta Y-Verschiebung vom Zentrum
     * @param datum Text-Ausrichtung (Standard: middle_center)
     */
    void setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum = MC_DATUM);

    /**
     * @brief Setzt einen zusätzlichen X/Y-Offset für das Label.
     * @param x_offset Horizontaler Offset in Pixeln.
     * @param y_offset Vertikaler Offset in Pixeln.
     */
    void setLabelOffset(int16_t x_offset, int16_t y_offset);

    /**
     * @brief Setzt die Rahmenfarbe
     * @param clr Neue Rahmenfarbe
     */
    template <typename T>
    void setOutlineColor(const T &clr) { m_outlinecolor = clr; }

    /**
     * @brief Setzt die Füllfarbe
     * @param clr Neue Füllfarbe
     */
    template <typename T>
    void setFillColor(const T &clr) { m_fillcolor = clr; }

    /**
     * @brief Setzt die Textfarbe
     * @param clr Neue Textfarbe
     */
    template <typename T>
    void setTextColor(const T &clr) { m_textcolor = clr; }

    /**
     * @brief Lenkt alle Zeichenoperationen in ein Sprite um
     * @param target Sprite-Zeichenziel (PSRAM-Sprite). nullptr = wie clearDrawTarget()
     * @param spriteOriginX X-Position, an der das Sprite später auf dem Display gepusht wird
     * @param spriteOriginY Y-Position, an der das Sprite später auf dem Display gepusht wird
     *
     * Ermöglicht flimmerfreies Zeichnen über einen Sprite-Puffer. Der Button wird vollständig im PSRAM-Sprite aufgebaut und anschließend in einem Schritt auf das Display übertragen.
     *
     * Die Touch-Hit-Detection (contains, isPressed) bleibt unverändert, sie arbeitet weiterhin mit den absoluten Display-Koordinaten aus initButton.
     *
     * Die Origin-Koordinaten geben an, wo das Sprite später auf dem Display landet. Der Button rechnet intern um: Sprite-Koordinate = Display-Koordinate - Origin.
     *
     * Beispiel: Button bei Display-Center (630, 230), Größe 260x56, Sprite 260x56 mit Push-Ziel (500, 202): setDrawTarget(sprite, 500, 202). Der Button zeichnet dann intern bei Sprite-Center (130, 28).
     *
     * Bei einem Sprite in Displaygröße oder einem Button an der Sprite-Position (0, 0) können beide Ursprungsparameter entfallen.
     *
     * Hinweis: Der Target-Typ ist Gfx_Sprite und nicht LovyanGFX, weil die Custom-Smooth-Methoden (drawSmoothCircle/Ellipse/Gauge) nur in Gfx_Sprite und LGFX existieren, nicht in der gemeinsamen Basisklasse.
     */
    void setDrawTarget(Gfx_Sprite *target, int16_t spriteOriginX = 0, int16_t spriteOriginY = 0)
    {
        m_drawTarget = target;
        m_targetOriginX = spriteOriginX;
        m_targetOriginY = spriteOriginY;
    }

    /**
     * @brief Schaltet das Sprite-Target ab. Folgende Zeichenoperationen gehen wieder
     *        direkt aufs Display, das in initButton gesetzt wurde.
     */
    void clearDrawTarget()
    {
        m_drawTarget = nullptr;
        m_targetOriginX = 0;
        m_targetOriginY = 0;
    }

    /**
     * @brief Zeichnet den Button
     * @param inverted True für invertierten Zustand (gedrückt)
     * @param long_name Alternativer Text (nullptr = normaler Text)
     */
    void drawButton(bool inverted = false, const char *long_name = nullptr);

    /**
     * @brief Prüft ob Koordinaten innerhalb des Buttons liegen
     * @param x X-Koordinate
     * @param y Y-Koordinate
     * @return True wenn Punkt im Button liegt
     */
    bool contains(int16_t x, int16_t y) const
    {
        return abs(x - m_x) <= (m_w / 2) && abs(y - m_y) <= (m_h / 2);
    }

    /**
     * @brief Setzt den Gedrückt-Status
     * @param p True = gedrückt, False = nicht gedrückt
     */
    void press(bool p)
    {
        m_last_press = m_pressed;
        m_pressed = p;
    }

    /**
     * @brief Gibt zurück ob Button gedrückt ist
     * @return True wenn gedrückt
     */
    bool isPressed(void) const { return m_pressed; }

    /**
     * @brief Gibt zurück ob Button gerade gedrückt wurde
     * @return True bei Zustandswechsel zu gedrückt
     */
    bool justPressed(void) const { return (m_pressed && !m_last_press); }

    /**
     * @brief Gibt zurück ob Button gerade losgelassen wurde
     * @return True bei Zustandswechsel zu nicht gedrückt
     */
    bool justReleased(void) const { return (!m_pressed && m_last_press); }

    /**
     * @brief Callback-Funktion für benutzerdefiniertes Zeichnen
     */
    typedef void (*drawCb)(LGFX *m_tft, int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, bool invert, const char *long_name);

    /**
     * @brief Setzt eine benutzerdefinierte Zeichenfunktion
     * @param cb Callback-Funktion
     */
    void setDrawCb(drawCb cb) { m_drawCb = cb; };

    /**
     * @brief Setzt den Zustand des Buttons
     * @param enabled true aktiviert den Button
     * @param redraw false setzt nur den Zustand, ohne zu zeichnen
     *
     * Ein unveränderter Zustand löst nie ein Neuzeichnen aus. Ohne Neuzeichnen ist für Aufrufer gedacht, die unmittelbar danach ohnehin die ganze Fläche neu aufbauen; sonst zeichnet der Button sich dort zweimal.
     */
    void setEnabled(bool enabled, bool redraw = true)
    {
        if (m_enabled == enabled)
            return;

        m_enabled = enabled;

        if (redraw)
            drawButton();
    }

    /**
     * @brief Aktiviert den Button
     */
    void enable() { setEnabled(true); }

    /**
     * @brief Deaktiviert den Button
     */
    void disable() { setEnabled(false); }

    /**
     * @brief Prüft, ob der Button aktiviert ist
     * @return true, wenn aktiviert
     */
    bool isEnabled() const { return m_enabled; }

private:
    /**
     * @brief Liefert das aktuelle Zeichenziel (Sprite-Target oder Display)
     * @return Zeiger auf das Ziel-LGFXBase-Objekt
     */
    LovyanGFX *target() { return m_drawTarget ? static_cast<LovyanGFX *>(m_drawTarget) : static_cast<LovyanGFX *>(m_tft); }

    /**
     * @brief Zeichnet einen geglätteten Kreis auf Sprite-Target oder Display
     *
     * Branches manuell zwischen m_drawTarget (Gfx_Sprite) und m_tft (LGFX), weil drawSmoothCircle nicht in der gemeinsamen LovyanGFX-Basisklasse existiert, sondern nur in den Custom-Erweiterungen Gfx_Sprite und LGFX.
     */
    void drawSmoothCircleOnTarget(int xc, int yc, float r, float thickness, uint16_t color, uint16_t bg)
    {
        if (m_drawTarget)
            m_drawTarget->drawSmoothCircle(xc, yc, r, thickness, color, bg);
        else
            m_tft->drawSmoothCircle(xc, yc, r, thickness, color, bg);
    }

    /**
     * @brief Zeichnet eine geglättete Ellipse auf Sprite-Target oder Display
     */
    void drawSmoothEllipseOnTarget(int xc, int yc, float rx, float ry, float thickness, uint16_t color, uint16_t bg)
    {
        if (m_drawTarget)
            m_drawTarget->drawSmoothEllipse(xc, yc, rx, ry, thickness, color, bg);
        else
            m_tft->drawSmoothEllipse(xc, yc, rx, ry, thickness, color, bg);
    }

    /**
     * @brief Zeichnet ein geglättetes Rundinstrument auf Sprite-Target oder Display
     */
    void drawSmoothGaugeOnTarget(int xc, int yc, float r, float thickness, float angleStart, float angleEnd,
                                  float valueMin, float valueMax, float value,
                                  uint16_t color, uint16_t colorTrack, bool trackVisible, uint16_t bg)
    {
        if (m_drawTarget)
            m_drawTarget->drawSmoothGauge(xc, yc, r, thickness, angleStart, angleEnd, valueMin, valueMax, value, color, colorTrack, trackVisible, bg);
        else
            m_tft->drawSmoothGauge(xc, yc, r, thickness, angleStart, angleEnd, valueMin, valueMax, value, color, colorTrack, trackVisible, bg);
    }

    /**
     * @brief Interne Initialisierung
     */
    void _init_button(LGFX *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t r, const char *label, const uint16_t *icon, float textsize_x, float textsize_y);

    /**
     * @brief Prüft ob Label ein Symbol-Code ist (^SYMBOL^)
     */
    bool isSymbolLabel(const char *label) const;

    /**
     * @brief Zeichnet Symbol basierend auf Label-Code
     */
    void drawSymbolFromLabel(const char *label, int16_t x, int16_t y, uint16_t color);

    /**
     * @brief Extrahiert Größe aus Symbol-Label (^SYMBOL:size^)
     */
    int16_t parseSymbolAttributes(const char *label, int16_t defaultSize, uint16_t &color) const;

    // Symbol-Zeichenmethoden
    void drawShiftSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawCapsLockSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawBackSpaceSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawEyeOnSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawEyeOffSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawLockSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawRefreshSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawDeleteSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawHomeSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawSettingsSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawBackSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawPlaySymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawPauseSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawStopSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawPlusSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawMinusSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawForwardSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawBackwardSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawSkipForwardSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawSkipBackwardSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawStarSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawPowerSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawFolderSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawCLeftSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);
    void drawCRightSymbol(int16_t x, int16_t y, int16_t size, uint16_t color);

    // Member-Variablen
    LGFX *m_tft = nullptr;                   ///< Display-Objekt
    drawCb m_drawCb = nullptr;               ///< Benutzerdefinierte Zeichenfunktion
    int16_t m_x = 0;                         ///< X-Position (Zentrum)
    int16_t m_y = 0;                         ///< Y-Position (Zentrum)
    int16_t m_xd = 0;                        ///< X-Verschiebung für Text
    int16_t m_yd = 0;                        ///< Y-Verschiebung für Text
    uint16_t m_w = 0;                        ///< Breite
    uint16_t m_h = 0;                        ///< Höhe
    uint16_t m_r = 0;                        ///< Eckenradius
    uint16_t m_outlinecolor = TFT_LIGHTGREY; ///< Rahmenfarbe
    uint16_t m_fillcolor = TFT_BLUE;         ///< Füllfarbe
    uint16_t m_textcolor = TFT_WHITE;        ///< Textfarbe
    uint8_t m_textdatum = MC_DATUM;          ///< Text-Ausrichtung
    static constexpr size_t label_length = 30;
    char m_label[label_length + 1];                ///< Button-Text
    const uint16_t *m_icon = nullptr;              ///< Icon-Daten
    float m_textsize_x, _textsize_y;               ///< Textgröße
    bool m_pressed = false;                        ///< Aktueller Status
    bool m_last_press = false;                     ///< Vorheriger Status
    IconPosition m_iconPos = IconPosition::Center; ///< Icon-Position
    int16_t m_iconWidth = 32;                      ///< Icon-Breite
    int16_t m_iconHeight = 32;                     ///< Icon-Höhe
    const uint8_t *m_font_legacy = nullptr;        ///< Zeiger für Legacy-Schriftarten
    const lgfx::v1::GFXfont *m_font_gfx = nullptr; ///< Zeiger für LGFX v1 Schriftarten
    bool m_useNumberCircle = false;                ///< Zahlenkreis aktiviert
    uint8_t m_circleNumber = 0;                    ///< Anzuzeigende Zahl
    bool m_enabled = true;                         ///< Button ist aktiviert/deaktiviert
    int16_t m_labelOffsetX = 0;                    ///< Zusätzlicher X-Offset für das Label
    int16_t m_labelOffsetY = 0;                    ///< Zusätzlicher Y-Offset für das Label
    Gfx_Sprite *m_drawTarget = nullptr;            ///< Alternatives Zeichenziel (PSRAM-Sprite). nullptr = aufs Display zeichnen
    int16_t m_targetOriginX = 0;                   ///< X-Position, an der das Sprite-Ziel später auf dem Display liegt
    int16_t m_targetOriginY = 0;                   ///< Y-Position, an der das Sprite-Ziel später auf dem Display liegt
};

#endif // BUTTON_H
