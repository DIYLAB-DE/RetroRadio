#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>

class Gfx_Sprite;

/**
 * @brief LovyanGFX-Geräteklasse für das 800x480-RGB-Display mit GT911-Touch.
 */
class LGFX : public lgfx::LGFX_Device
{
public:
    lgfx::Bus_RGB _bus_instance;       ///< RGB-Bus für die Display-Datenübertragung
    lgfx::Panel_RGB _panel_instance;   ///< RGB-Panel des Displays
    lgfx::Touch_GT911 _touch_instance; ///< GT911-Touch-Steuerung

    /**
     * @brief Konfiguriert Display, RGB-Bus und Touch.
     *
     * Initialisiert ein 800x480-RGB-Display mit parallelem 16-Bit-Datenbus, HSYNC/VSYNC-Timing, GT911-Touch-Controller über I2C und PSRAM-Unterstützung für Display-Puffer.
     */
    LGFX(void);

    /**
     * @brief Zeichnet einen Pixel nur innerhalb der gültigen Display-Grenzen.
     * @param x X-Koordinate des Pixels
     * @param y Y-Koordinate des Pixels
     * @param color Farbe des Pixels im RGB565-Format
     */
    void drawPixelClipped(int x, int y, uint16_t color);

    /**
     * @brief Löscht einen Sprite sicher und setzt den Zeiger zurück.
     * @param sprite Referenz auf den zu löschenden Sprite-Zeiger
     *
     * Ruft unloadFont(), deleteSprite() und delete auf, falls der Zeiger gültig ist.
     */
    void safeDeleteSprite(Gfx_Sprite *&sprite);

    /**
     * @brief Erstellt oder erneuert einen Sprite mit Fehlerbehandlung.
     * @param sprite Referenz auf den zu erstellenden Sprite-Zeiger
     * @param width Breite des Sprites in Pixeln
     * @param height Höhe des Sprites in Pixeln
     * @param usePsram true für PSRAM-Nutzung, false für internen Speicher
     * @return true bei erfolgreicher Erstellung oder Wiederverwendung, sonst false
     *
     * Ein vorhandener Sprite wird wiederverwendet, wenn seine Größe passt. Bei anderer Größe wird er vorher sicher gelöscht.
     */
    bool safeCreateSprite(Gfx_Sprite *&sprite, int width, int height, bool usePsram = true);

    /**
     * @brief Zeichnet einen geglätteten Kreisring.
     * @param xc X-Koordinate des Kreismittelpunkts in Pixeln
     * @param yc Y-Koordinate des Kreismittelpunkts in Pixeln
     * @param r Radius des Kreises in Pixeln
     * @param thickness Stärke der Kreislinie in Pixeln
     * @param color Farbe des Kreises im RGB565-Format
     * @param bg Hintergrundfarbe im RGB565-Format
     */
    void drawSmoothCircle(int xc, int yc, float r, float thickness, uint16_t color = 0xFFFF, uint16_t bg = 0x0000);

    /**
     * @brief Zeichnet eine geglättete Ellipse.
     * @param xc X-Koordinate des Ellipsenmittelpunkts in Pixeln
     * @param yc Y-Koordinate des Ellipsenmittelpunkts in Pixeln
     * @param rx Horizontaler Radius der Ellipse in Pixeln
     * @param ry Vertikaler Radius der Ellipse in Pixeln
     * @param thickness Stärke der Ellipsenlinie in Pixeln
     * @param color Farbe der Ellipse im RGB565-Format
     * @param bg Hintergrundfarbe im RGB565-Format
     */
    void drawSmoothEllipse(int xc, int yc, float rx, float ry, float thickness, uint16_t color = 0xFFFF, uint16_t bg = 0x0000);

    /**
     * @brief Zeichnet ein geglättetes Rundinstrument.
     * @param xc X-Koordinate des Mittelpunkts in Pixeln
     * @param yc Y-Koordinate des Mittelpunkts in Pixeln
     * @param r Radius des Rundinstruments in Pixeln
     * @param thickness Dicke des Rings in Pixeln
     * @param angleStart Startwinkel in Grad, 0 Grad liegt oben und positive Werte laufen im Uhrzeigersinn
     * @param angleEnd Endwinkel in Grad
     * @param valueMin Minimaler Wertebereich
     * @param valueMax Maximaler Wertebereich
     * @param value Aktueller Wert innerhalb von valueMin bis valueMax
     * @param color Farbe des gefüllten Kreissegments im RGB565-Format
     * @param colorTrack Farbe der Spur außerhalb des aktuellen Werts im RGB565-Format
     * @param trackVisible true, wenn die Spur angezeigt werden soll
     * @param bg Hintergrundfarbe im RGB565-Format
     */
    void drawSmoothGauge(int xc, int yc, float r, float thickness, float angleStart, float angleEnd, float valueMin, float valueMax, float value, uint16_t color = 0xFFFF, uint16_t colorTrack = 0x6b4d, bool trackVisible = false, uint16_t bg = 0x0000);

};

// ===================================================================
// Sprite-Erweiterung
// ===================================================================

/**
 * @brief Erweiterte Sprite-Klasse mit geglätteten Zeichenfunktionen.
 */
class Gfx_Sprite : public lgfx::LGFX_Sprite
{
public:
    using lgfx::LGFX_Sprite::LGFX_Sprite; ///< Erbt alle Konstruktoren der Basisklasse

    /**
     * @brief Zeichnet einen Pixel nur innerhalb der gültigen Sprite-Grenzen.
     * @param x X-Koordinate des Pixels
     * @param y Y-Koordinate des Pixels
     * @param color Farbe des Pixels im RGB565-Format
     */
    void drawPixelClipped(int x, int y, uint16_t color);

    /**
     * @brief Zeichnet einen geglätteten Kreisring in den Sprite.
     * @param xc X-Koordinate des Kreismittelpunkts in Pixeln
     * @param yc Y-Koordinate des Kreismittelpunkts in Pixeln
     * @param r Radius des Kreises in Pixeln
     * @param thickness Stärke der Kreislinie in Pixeln
     * @param color Farbe des Kreises im RGB565-Format
     * @param bg Hintergrundfarbe im RGB565-Format
     */
    void drawSmoothCircle(int xc, int yc, float r, float thickness, uint16_t color = 0xFFFF, uint16_t bg = 0x0000);

    /**
     * @brief Zeichnet eine geglättete Ellipse in den Sprite.
     * @param xc X-Koordinate des Ellipsenmittelpunkts in Pixeln
     * @param yc Y-Koordinate des Ellipsenmittelpunkts in Pixeln
     * @param rx Horizontaler Radius der Ellipse in Pixeln
     * @param ry Vertikaler Radius der Ellipse in Pixeln
     * @param thickness Stärke der Ellipsenlinie in Pixeln
     * @param color Farbe der Ellipse im RGB565-Format
     * @param bg Hintergrundfarbe im RGB565-Format
     */
    void drawSmoothEllipse(int xc, int yc, float rx, float ry, float thickness, uint16_t color = 0xFFFF, uint16_t bg = 0x0000);

    /**
     * @brief Zeichnet ein geglättetes Rundinstrument in den Sprite.
     * @param xc X-Koordinate des Mittelpunkts in Pixeln
     * @param yc Y-Koordinate des Mittelpunkts in Pixeln
     * @param r Radius des Rundinstruments in Pixeln
     * @param thickness Dicke des Rings in Pixeln
     * @param angleStart Startwinkel in Grad, 0 Grad liegt oben und positive Werte laufen im Uhrzeigersinn
     * @param angleEnd Endwinkel in Grad
     * @param valueMin Minimaler Wertebereich
     * @param valueMax Maximaler Wertebereich
     * @param value Aktueller Wert innerhalb von valueMin bis valueMax
     * @param color Farbe des gefüllten Kreissegments im RGB565-Format
     * @param colorTrack Farbe der Spur außerhalb des aktuellen Werts im RGB565-Format
     * @param trackVisible true, wenn die Spur angezeigt werden soll
     * @param bg Hintergrundfarbe im RGB565-Format
     */
    void drawSmoothGauge(int xc, int yc, float r, float thickness, float angleStart, float angleEnd, float valueMin, float valueMax, float value, uint16_t color = 0xFFFF, uint16_t colorTrack = 0x6b4d, bool trackVisible = false, uint16_t bg = 0x0000);

    /**
     * @brief Zeichnet die Nachbildung einer EM11-Abstimmröhre in den Sprite.
     * @param xc X-Koordinate des Schirmmittelpunkts in Pixeln
     * @param yc Y-Koordinate des Schirmmittelpunkts in Pixeln
     * @param r Radius des Leuchtschirms in Pixeln
     * @param level Abstimmpegel von 0.0 für schmale Leuchtsektoren bis 1.0 für weit geöffnete Sektoren
     * @param bg Farbe außerhalb des Leuchtschirms im RGB565-Format, gegen die der Schirmrand weich ausläuft
     *
     * Vier Leuchtsektoren wachsen aus der Mitte heraus: die beiden senkrechten öffnen früh und weit, die beiden waagerechten später und schmaler. Leuchtstoff, Randbogen, Kantenlicht und Metallkappe entsprechen der EM34-Darstellung.
     */
    void drawEm11MagicEye(int xc, int yc, float r, float level, uint16_t bg);

    /**
     * @brief Zeichnet die Nachbildung einer EM34-Abstimmröhre in den Sprite.
     * @param xc X-Koordinate des Schirmmittelpunkts in Pixeln
     * @param yc Y-Koordinate des Schirmmittelpunkts in Pixeln
     * @param r Radius des Leuchtschirms in Pixeln
     * @param level Abstimmpegel von 0.0 für weit geöffnete Schattenkeile bis 1.0 für geschlossene Keile
     * @param bg Farbe außerhalb des Leuchtschirms im RGB565-Format, gegen die der Schirmrand weich ausläuft
     *
     * Die beiden großen Schattenkeile folgen dem Pegel mit unterschiedlicher Empfindlichkeit, der linke schließt bereits vor Vollausschlag und der rechte bleibt bis zuletzt einen Spalt offen. Die beiden schmalen Nebenkeile stehen senkrecht dazu und sind pegelunabhängig. Die gesamte Achse ist gegenüber der Waagerechten geneigt.
     */
    void drawEm34MagicEye(int xc, int yc, float r, float level, uint16_t bg);

    /**
     * @brief Zeichnet die Nachbildung einer EM80-Abstimmröhre in den Sprite.
     * @param xc X-Koordinate der Mitte des umschließenden Rechtecks in Pixeln
     * @param yc Y-Koordinate der Mitte des umschließenden Rechtecks in Pixeln
     * @param halfWidth Halbe Breite des umschließenden Rechtecks in Pixeln, Rahmen eingeschlossen
     * @param halfHeight Halbe Höhe des umschließenden Rechtecks in Pixeln, Rahmen eingeschlossen
     * @param level Abstimmpegel von 0.0 für einen fast geschlossenen mittleren Fächer bis 1.0 für zwei schmale Schattenstrahlen
     * @param ring Farbe des Rahmens um den Leuchtschirm im RGB565-Format
     * @param bg Farbe außerhalb des Rahmens im RGB565-Format, gegen die der Rahmen weich ausläuft
     *
     * Der Leuchtschirm ist ein stehender Rundbogen mit senkrechten Seiten, halbkreisförmiger Oberkante und gerader Unterkante. Zwei Schattenstrahlen laufen an seiner Bodenmitte zusammen und teilen ihn in drei Leuchtfächer: einen mittleren, der mit dem Pegel aufklappt, und zwei äußere in den unteren Ecken, die dabei mitwachsen. In den Strahlen glimmt der Schirm weiter. Der Rahmen gehört hier zur Zeichnung, weil er dem Rundbogen folgen muss.
     */
    void drawEm80MagicEye(int xc, int yc, float halfWidth, float halfHeight, float level, uint16_t ring, uint16_t bg);

    /**
     * @brief Zeichnet die Nachbildung einer EM84-Abstimmröhre in den Sprite.
     * @param xc X-Koordinate der Mitte des umschließenden Rechtecks in Pixeln
     * @param yc Y-Koordinate der Mitte des umschließenden Rechtecks in Pixeln
     * @param halfWidth Halbe Breite des umschließenden Rechtecks in Pixeln, Rahmen eingeschlossen
     * @param halfHeight Halbe Höhe des umschließenden Rechtecks in Pixeln, Rahmen eingeschlossen
     * @param level Abstimmpegel von 0.0 für zwei kurze Balken bis 1.0 für ein fast durchgehendes Leuchtband
     * @param ring Farbe des Rahmens um den Leuchtschirm im RGB565-Format
     * @param bg Farbe außerhalb des Rahmens im RGB565-Format, gegen die der Rahmen weich ausläuft
     *
     * Der Leuchtschirm ist ein hochkant stehendes Band mit abgerundeten Ecken. Zwei Balken wachsen von oben und unten zur Mitte; dazwischen bleibt ein dunkler Steg, der bei Vollausschlag nur noch als schmale Naht stehenbleibt. Der Rahmen gehört hier zur Zeichnung, weil er dem Band folgen muss.
     */
    void drawEm84MagicEye(int xc, int yc, float halfWidth, float halfHeight, float level, uint16_t ring, uint16_t bg);
};

// ===================================================================
// Farb-Hilfsfunktionen
// ===================================================================

/**
 * @brief Wandelt einen Wert in eine 16-Bit-Regenbogenfarbe um.
 * @param value Eingangswert von 0 bis 255
 * @return Regenbogenfarbe im RGB565-Format
 *
 * Werte von 0 bis 191 werden auf einen Farbübergang von Rot über Blau bis Violett abgebildet. Der Zyklus wiederholt sich alle 192 Schritte.
 */
uint16_t rainbowColors(uint8_t value);

/**
 * @brief Skaliert die Helligkeit einer RGB565-Farbe.
 * @param color Originalfarbe im RGB565-Format
 * @param factor Helligkeitsfaktor, 1.0 entspricht der Originalfarbe
 * @return Skalierte Farbe im RGB565-Format
 *
 * Werte unter 0.0 werden auf 0.0 geklemmt. Jeder Farbkanal wird unabhängig skaliert und auf sein Kanalmaximum begrenzt.
 */
uint16_t scaleColor(uint16_t color, float factor);

/**
 * @brief Interpoliert linear zwischen zwei RGB565-Farben.
 * @param fg Vordergrundfarbe im RGB565-Format
 * @param bg Hintergrundfarbe im RGB565-Format
 * @param alpha Interpolationsfaktor, 0.0 entspricht bg und 1.0 entspricht fg
 * @return Interpolierte Farbe im RGB565-Format
 */
uint16_t blendColor(uint16_t fg, uint16_t bg, float alpha);

/**
 * @brief Berechnet die Helligkeit einer RGB565-Farbe.
 * @param color Farbe im RGB565-Format
 * @return Intensität von 0.0 für Schwarz bis 1.0 für Weiß
 */
float getIntensityFrom565(uint16_t color);

/**
 * @brief Berechnet eine pulsierende Farbe basierend auf der Laufzeit.
 * @param baseColor Basisfarbe im RGB565-Format
 * @param highlightColor Hervorhebungsfarbe im RGB565-Format
 * @param periodMs Pulsations-Periode in Millisekunden
 * @return Pulsierende Farbe im RGB565-Format
 */
uint16_t getPulsatingColor(uint16_t baseColor, uint16_t highlightColor, uint32_t periodMs);

#endif // GFX_H
