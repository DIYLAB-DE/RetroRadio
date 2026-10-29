#ifndef DOTMATRIX_H
#define DOTMATRIX_H

#include <stdint.h>
#include <stddef.h>

class Gfx_Sprite;

/**
 * @class DotMatrix
 * @brief LED-Punktmatrix-Display mit HD44780-Font für ESP32
 */
class DotMatrix
{
public:
    /**
     * @brief Dot-Form Optionen
     */
    enum class DotShape
    {
        CIRCLE, ///< Runde Dots
        SQUARE  ///< Quadratische Dots
    };

    /**
     * @brief Konstruktor
     */
    DotMatrix();

    /**
     * @brief Destruktor
     */
    ~DotMatrix();

    // Kopieren verboten (roher Heap-Pointer)
    DotMatrix(const DotMatrix &) = delete;
    DotMatrix &operator=(const DotMatrix &) = delete;

    /**
     * @brief Initialisiert die DotMatrix mit den angegebenen Parametern
     * @param dotWidth Breite eines einzelnen Dots in Pixeln
     * @param dotHeight Höhe eines einzelnen Dots in Pixeln
     * @param dotSpace Abstand zwischen Dots in Pixeln
     * @param charSpace Abstand zwischen Zeichen in Pixeln
     */
    void init(int dotWidth, int dotHeight, int dotSpace = 1, int charSpace = -1);

    // Farbkonfiguration

    /**
     * @brief Setzt die Farbe für aktive Dots
     * @param color Hauptfarbe im RGB565-Format
     * @param shadow Schattenfarbe für 3D-Effekt im RGB565-Format
     */
    void setOnColor(uint16_t color, uint16_t shadow);

    /**
     * @brief Setzt die Farbe für inaktive Dots
     * @param color Hauptfarbe im RGB565-Format
     * @param shadow Schattenfarbe für 3D-Effekt im RGB565-Format
     */
    void setOffColor(uint16_t color, uint16_t shadow);

    /**
     * @brief Setzt die Hintergrundfarbe
     * @param color Hintergrundfarbe im RGB565-Format
     */
    void setBackgroundColor(uint16_t color);

    /**
     * @brief Aktiviert/Deaktiviert das Zeichnen von Off-Dots
     * @param show true = Off-Dots anzeigen, false = nur On-Dots
     */
    void setShowOffDots(bool show);

    /**
     * @brief Aktiviert/Deaktiviert den 3D-Gradient-Effekt
     * @param enable true = Gradient aktivieren, false = flache Dots
     */
    void setGradientEnabled(bool enable);

    /**
     * @brief Setzt die Form der Dots
     * @param shape DotShape::CIRCLE für runde, DotShape::SQUARE für quadratische Dots
     */
    void setDotShape(DotShape shape);

    // Textfunktionen

    /**
     * @brief Setzt den anzuzeigenden Text (UTF-8)
     * @param text Der anzuzeigende Text
     */
    void setText(const char *text);

    /**
     * @brief Reserviert Speicher für den Textpuffer
     *
     * Alloziert einmalig Speicher für die angegebene Kapazität, um spätere realloc()-Aufrufe in setText() zu vermeiden. Nützlich bei bekannter maximaler Textlänge (z.B. Uhrzeit/Datum).
     *
     * @param capacity Gewünschte Kapazität in Bytes (inkl. Null-Terminator)
     * @return true bei Erfolg, false bei Speicherfehler
     */
    bool reserveText(size_t capacity);

    /**
     * @brief Gibt den aktuell gesetzten Text zurück
     * @return Zeiger auf den internen Textpuffer
     */
    const char *getText() const;

    // Lauftext

    /**
     * @brief Aktiviert/Deaktiviert horizontales Scrolling
     * @param enable true = Scrolling aktivieren
     * @param speed Scroll-Geschwindigkeit in Pixeln pro Update
     */
    void setScrolling(bool enable, int speed = 1);

    /**
     * @brief Setzt die Scroll-Position zurück auf den Anfang
     */
    void resetScroll();

    /**
     * @brief Aktualisiert die Scroll-Position
     * @return true wenn ein Scroll-Zyklus abgeschlossen wurde
     */
    bool updateScroll();

    /**
     * @brief Aktiviert/Deaktiviert horizontales Scrolling (zeitbasiert)
     *
     * Im Gegensatz zu setScrolling()/updateScroll() basiert diese Variante auf realer Zeit und bleibt dadurch gleichmäßig, auch wenn Frames/Updates ausfallen.
     *
     * @param enable true = Scrolling aktivieren
     * @param pixelsPerSecond Scroll-Geschwindigkeit in Pixeln pro Sekunde
     */
    void setScrollingTimed(bool enable, int pixelsPerSecond = 30);

    /**
     * @brief Aktualisiert die Scroll-Position zeitbasiert
     * @param nowMs Aktuelle Zeit in Millisekunden
     * @return true wenn ein Scroll-Zyklus abgeschlossen wurde
     */
    bool updateScrollTimed(uint32_t nowMs);

    // Dimensionen

    /**
     * @brief Berechnet die Breite des Textes in Pixeln
     * @return Gesamtbreite des aktuellen Textes in Pixeln
     */
    int getTextWidth() const;

    /**
     * @brief Berechnet die Höhe der Matrix in Pixeln
     * @return Höhe der DotMatrix in Pixeln (5 Dots + Abstände)
     */
    int getHeight() const;

    /**
     * @brief Gibt die Breite eines einzelnen Zeichens in Pixeln zurück
     * @return Breite eines Zeichens inkl. Abstand
     */
    int getCharWidth() const;

    // Darstellung

    /**
     * @brief Zeichnet die DotMatrix in einen Sprite
     * @param sprite Zeiger auf das Gfx_Sprite-Objekt
     * @param x X-Position der linken oberen Ecke
     * @param y Y-Position der linken oberen Ecke
     */
    void draw(Gfx_Sprite *sprite, int x, int y);

    /**
     * @brief Zeichnet die DotMatrix mit Clipping in einen Sprite
     * @param sprite Zeiger auf das Gfx_Sprite-Objekt
     * @param x X-Position der linken oberen Ecke
     * @param y Y-Position der linken oberen Ecke
     * @param clipWidth Maximale Breite des sichtbaren Bereichs
     */
    void drawClipped(Gfx_Sprite *sprite, int x, int y, int clipWidth);

private:
    // Interne Hilfsfunktionen

    /**
     * @brief Holt das Bitmuster für ein Zeichen aus dem Font
     * @param codepoint Unicode-Codepoint des Zeichens
     * @param pattern Ausgabe-Array für 7 Zeilen (je 5 Bits) - HD44780 Format
     * @return true wenn Zeichen gefunden, false sonst
     */
    bool getCharPattern(uint16_t codepoint, uint8_t pattern[7]) const;

    /**
     * @brief Dekodiert ein UTF-8 Zeichen und gibt den Codepoint zurück
     * @param str Zeiger auf den String
     * @return Unicode-Codepoint
     */
    uint16_t decodeUtf8(const char *&str) const;

    /**
     * @brief Zählt die Anzahl der Unicode-Zeichen im UTF-8 String
     * @param str Der UTF-8 String
     * @return Anzahl der Zeichen
     */
    size_t countChars(const char *str) const;

    /**
     * @brief Zeichnet einen einzelnen geglätteten Dot mit Gradient
     * @param sprite Zeiger auf das Sprite-Objekt
     * @param cx X-Mittelpunkt des Dots
     * @param cy Y-Mittelpunkt des Dots
     * @param isOn true = leuchtender Dot, false = dunkler Dot
     */
    void drawDot(Gfx_Sprite *sprite, int cx, int cy, bool isOn);

    /**
     * @brief Internes Rendering mit optionalem Clipping
     * @param sprite Zeiger auf das Sprite-Objekt
     * @param x X-Position
     * @param y Y-Position
     * @param clipWidth Clipping-Breite (-1 = kein Clipping)
     */
    void drawInternal(Gfx_Sprite *sprite, int x, int y, int clipWidth);

    /**
     * @brief Rechnet Char-Anzahl und Textbreite neu (Cache)
     */
    void recalcMetrics() const;

    // Membervariablen

    char *_text;          ///< Interner Textpuffer (UTF-8)
    size_t _textLen;      ///< Byte-Länge des Textes
    size_t _textCapacity; ///< Kapazität des Textpuffers

    int _dotWidth;  ///< Breite eines Dots in Pixeln
    int _dotHeight; ///< Höhe eines Dots in Pixeln
    int _dotSpace;  ///< Abstand zwischen Dots
    int _charSpace; ///< Abstand zwischen Zeichen

    uint16_t _onColor;   ///< Farbe für aktive Dots
    uint16_t _onShadow;  ///< Schattenfarbe für aktive Dots
    uint16_t _offColor;  ///< Farbe für inaktive Dots
    uint16_t _offShadow; ///< Schattenfarbe für inaktive Dots
    uint16_t _bgColor;   ///< Hintergrundfarbe

    bool _showOffDots;     ///< Off-Dots anzeigen?
    bool _gradientEnabled; ///< 3D-Gradient-Effekt aktiviert?
    DotShape _dotShape;    ///< Form der Dots (rund/quadratisch)

    bool _scrollEnabled; ///< Scrolling aktiviert?
    int _scrollSpeed;    ///< Scroll-Geschwindigkeit in Pixeln
    int _scrollOffset;   ///< Aktuelle Scroll-Position

    // Caches für Performance
    mutable bool _metricsDirty;     ///< true = Breiten/Char-Cache ungültig
    mutable size_t _charCountCache; ///< Cache: Anzahl Unicode-Zeichen
    mutable int _textWidthCache;    ///< Cache: Textbreite in Pixeln

    // Zeitbasiertes Scrolling
    bool _scrollTimed;       ///< zeitbasiertes Scrolling aktiv?
    int _scrollSpeedPps;     ///< Pixel pro Sekunde
    uint32_t _lastScrollMs;  ///< Zeitstempel des letzten Updates
    int32_t _scrollOffsetFP; ///< Scroll-Offset in 16.16 Fixed-Point

    static const int CHAR_COLS = 5; ///< Spalten pro Zeichen
    static const int CHAR_ROWS = 7; ///< Zeilen pro Zeichen
};

#endif // DOTMATRIX_H
