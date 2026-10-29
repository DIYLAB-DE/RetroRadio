#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stdint.h>

class LGFX;
class Gfx_Sprite;

/**
 * @class Symbols
 * @brief Klasse zur Darstellung verschiedener grafischer Symbole
 * @note Alle Symbole folgen einem einheitlichen Layout-Prinzip mit einem
 *       Seitenverhältnis von 70:90 (Höhe:Breite)
 */
class Symbols {
   private:
    /** @brief Zeiger auf das LGFX-Display-Objekt */
    LGFX* _gfx;

    /**
     * @brief Zeichnet eine Wolke in ein Sprite
     * @param sprite Zeiger auf den Ziel-Sprite
     * @param cx X-Koordinate des Wolkenzentrums
     * @param cy Y-Koordinate des Wolkenzentrums
     * @param scale Skalierungsfaktor für die Wolkengröße
     * @param line_thickness Dicke der Wolkenkontur
     * @param fg_color Vordergrundfarbe für die Wolke
     * @param bg_color Hintergrundfarbe
     *
     * Private Hilfsfunktion zum zeichnen einer Wolke.
     */
    void drawCloud(Gfx_Sprite* sprite, float cx, float cy, float scale, int line_thickness, uint16_t fg_color, uint16_t bg_color);

    /**
     * @brief Zeichnet einen Mond in ein Sprite
     * @param sprite Zeiger auf den Ziel-Sprite
     * @param moon_cx X-Koordinate des Mondzentrums
     * @param moon_cy Y-Koordinate des Mondzentrums
     * @param scale Skalierungsfaktor für die Mondgröße
     * @param fg_color Vordergrundfarbe für den Mond
     * @param bg_color Hintergrundfarbe
     *
     * Private Hilfsfunktion zum zeichnen einer Mondsichel.
     */
    void drawMoon(Gfx_Sprite* sprite, float moon_cx, float moon_cy, float scale, uint16_t fg_color, uint16_t bg_color);

    /**
     * @brief Zeichnet Regen in ein Sprite
     * @param sprite Zeiger auf den Ziel-Sprite
     * @param cx X-Koordinate des Wolkenzentrums (als Referenz)
     * @param cy Y-Koordinate des Wolkenzentrums (als Referenz)
     * @param scale Skalierungsfaktor
     * @param line_thickness Dicke der Regentropfen
     * @param fg_color Vordergrundfarbe für den Regen
     * @param skip_index Index des Tropfens, der übersprungen werden soll (-1 = keinen überspringen)
     *
     * Private Hilfsfunktion zum zeichnen von Regen unter einer Wolke. Der skip_index Parameter ermöglicht es, einzelne Tropfen auszulassen.
     */
    void drawRain(Gfx_Sprite* sprite, float cx, float cy, float scale, int line_thickness, uint16_t fg_color, int skip_index = -1);

   public:
    /**
     * @brief Konstruktor der Symbols-Klasse
     * @param gfx Zeiger auf das LGFX-Display-Objekt
     */
    explicit Symbols(LGFX* gfx);

    /**
     * @brief Destruktor der Symbols-Klasse
     */
    ~Symbols() = default;

    /**
     * @brief Zeichnet eine Sonne hinter einer Wolke
     * @param x X-Position der oberen linken Ecke des Icons
     * @param y Y-Position der oberen linken Ecke des Icons
     * @param width Die Gesamtbreite der Icon-Leinwand (Minimum: 30 Pixel)
     * @param fg_color Die Farbe für ALLE Konturelemente (Wolke und Sonne)
     * @param bg_color Die Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showSunAndCloud(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);

    /**
     * @brief Zeichnet einen Mond hinter einer Wolke
     * @param x X-Position der oberen linken Ecke des Icons
     * @param y Y-Position der oberen linken Ecke des Icons
     * @param width Die Gesamtbreite der Icon-Leinwand (Minimum: 30 Pixel)
     * @param fg_color Die Farbe für ALLE Konturelemente (Wolke und Mond)
     * @param bg_color Die Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showMoonAndCloud(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);

    /**
     * @brief Zeichnet eine Wolke
     * @param x X-Position der oberen linken Ecke des Icons
     * @param y Y-Position der oberen linken Ecke des Icons
     * @param width Die Gesamtbreite der Icon-Leinwand (Minimum: 30 Pixel)
     * @param fg_color Die Farbe der Wolkenkontur
     * @param bg_color Die Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showCloud(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);

    /**
     * @brief Zeichnet eine Sonne
     * @param x X-Position der oberen linken Ecke des Icons
     * @param y Y-Position der oberen linken Ecke des Icons
     * @param width Die Gesamtbreite der Icon-Leinwand (Minimum: 30 Pixel)
     * @param fg_color Die Farbe der Sonnenkontur und -strahlen
     * @param bg_color Die Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showSun(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);

    /**
     * @brief Zeichnet eine Mondsichel
     * @param x X-Position der oberen linken Ecke des Icons
     * @param y Y-Position der oberen linken Ecke des Icons
     * @param width Die Gesamtbreite der Icon-Leinwand (Minimum: 30 Pixel)
     * @param fg_color Die Farbe der Mondkontur
     * @param bg_color Die Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showMoon(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);

    /**
     * @brief Zeichnet einen Mond mit Wolke und Regen
     * @param x X-Position der oberen linken Ecke des Icons
     * @param y Y-Position der oberen linken Ecke des Icons
     * @param width Die Gesamtbreite der Icon-Leinwand (Minimum: 30 Pixel)
     * @param fg_color Die Farbe für ALLE Konturelemente (Wolke, Mond, Regen)
     * @param bg_color Die Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showMoonAndCloudAndRain(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);

    /**
     * @brief Zeichnet eine regnende Wolke
     * @param x X-Position der oberen linken Ecke des Icons
     * @param y Y-Position der oberen linken Ecke des Icons
     * @param width Die Gesamtbreite der Icon-Leinwand (Minimum: 30 Pixel)
     * @param fg_color Die Farbe der Wolkenkontur und des Regens
     * @param bg_color Die Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showCloudAndRain(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);

    /**
     * @brief Zeichnet eine Wolke mit Schneeflocken
     * @param x X-Koordinate der oberen linken Ecke
     * @param y Y-Koordinate der oberen linken Ecke
     * @param width Breite des Icons in Pixeln (Minimum: 30)
     * @param fg_color Vordergrundfarbe für Wolke und Schneeflocken
     * @param bg_color Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showCloudAndSnow(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);

    /**
     * @brief Zeichnet eine Wolke mit Nebel
     * @param x X-Koordinate der oberen linken Ecke
     * @param y Y-Koordinate der oberen linken Ecke
     * @param width Breite des Icons in Pixeln (Minimum: 30)
     * @param fg_color Vordergrundfarbe für Wolke und Nebel
     * @param bg_color Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showCloudAndFog(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);

    /**
     * @brief Zeichnet eine Wolke mit Gewitter (Regen und Blitz)
     * @param x X-Koordinate der oberen linken Ecke
     * @param y Y-Koordinate der oberen linken Ecke
     * @param width Breite des Icons in Pixeln (Minimum: 30)
     * @param fg_color Vordergrundfarbe für Wolke, Regen und Blitz
     * @param bg_color Hintergrundfarbe
     * @param target Optionales Ziel-Sprite (nullptr = auf Display zeichnen)
     */
    void showCloudAndThunderstorm(int x, int y, int width, uint16_t fg_color, uint16_t bg_color, Gfx_Sprite* target = nullptr);
};

#endif  // SYMBOLS_H
