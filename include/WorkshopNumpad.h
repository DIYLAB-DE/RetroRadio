#ifndef WORKSHOPNUMPAD_H
#define WORKSHOPNUMPAD_H

#include <stdint.h>

#include "Gfx.h"

/**
 * @class WorkshopNumpad
 * @brief Gemeinsames 4×4-Nummernpad für alle Workshop-Rechner-Tools.
 *
 * Kapselt Layout, Zeichnung und Treffertest des standardisierten Zifferneingabefeldes für die Workshop-Rechner.
 *
 * Layout des 4×4-Grids (content-relativ ab startY):
 *   Linke 3 Spalten × 4 Zeilen (normale Höhe):  7-9 / 4-6 / 1-3 / . 0 +/-
 *   Rechte Spalte (doppelte Höhe):               BS (Zeilen 0+1), C (Zeilen 2+3)
 *
 * Festgelegte Tastenhöhe (TARGET_BTN_H):
 *   Alle Tools reservieren REQUIRED_H Pixel am unteren Ende ihres Inhaltsbereichs
 *   für das Numpad. Dadurch ist die Tastenhöhe unabhängig von der darüber
 *   liegenden Feldsektion stets 27 px und bleibt dadurch gut bedienbar.
 *
 * Koordinatensystem:
 *   computeLayout() erhält (w, startY), wobei startY = h - REQUIRED_H (content-relativ).
 *   draw() und hitTest() arbeiten mit denselben content-relativen Koordinaten.
 *
 * Aktionszeichen (für executeKey im jeweiligen Tool):
 *   '0'-'9' = Ziffer, '.' = Dezimalpunkt, 'S' = Vorzeichen umschalten,
 *   'B' = Backspace, 'C' = Clear
 */
class WorkshopNumpad
{
public:
    // =========================================================================
    // Öffentliche Layout-Konstanten
    // =========================================================================

    /** @brief Anzahl der Nummernpad-Tasten (12 normale + BS + C). */
    static constexpr int NUM_KEYS = 14;

    /** @brief Anzahl Tastenzeilen im Grid. */
    static constexpr int PAD_ROWS = 4;

    /** @brief Anzahl Tastenspalten im Grid. */
    static constexpr int PAD_COLS = 4;

    /** @brief Abstand zwischen zwei Tasten in Pixeln. */
    static constexpr int BTN_GAP = 4;

    /** @brief Verbindliche Ziel-Tastenhöhe in Pixeln für alle Werkzeuge. */
    static constexpr int TARGET_BTN_H = 27;

    /**
     * @brief Reservierter Höhenbedarf des Nummernpads in Pixeln.
     *
     * Berechnet sich aus vier Tastenzeilen zu je TARGET_BTN_H und drei Lücken zu je BTN_GAP. Jedes Werkzeug setzt padY auf h - REQUIRED_H, sodass das Nummernpad exakt diesen Bereich am unteren Rand des Inhaltsbereichs belegt.
     */
    static constexpr int REQUIRED_H = PAD_ROWS * TARGET_BTN_H + (PAD_ROWS - 1) * BTN_GAP;

    // =========================================================================
    // Lebenszyklus
    // =========================================================================

    /** @brief Setzt alle Layout-Rects und internen Größen auf null. */
    void clearLayout();

    // =========================================================================
    // Layout
    // =========================================================================

    /**
     * @brief Berechnet Tasten-Rechtecke für die gegebene Breite und Y-Position.
     * @param w      Verfügbare Breite in Pixeln (= Inhaltsbereich-Breite des Tools)
     * @param startY Y-Start des Nummernpads relativ zum Inhalt, entsprechend h - REQUIRED_H
     *
     * Alle berechneten Rects sind content-relativ (Ursprung = originX/originY aus dem Tool). computeLayout() muss bei jeder Größenänderung aufgerufen werden; ein layoutValid-Flag liegt im jeweiligen Tool.
     */
    void computeLayout(int w, int startY);

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /**
     * @brief Zeichnet Hintergrund und alle Tasten des Nummernpads.
     * @param spr        Ziel-Sprite (Work-Sprite des Workshop-Moduls)
     * @param originX    Linke Kante des Inhaltsbereichs im Sprite
     * @param originY    Obere Kante des Inhaltsbereichs im Sprite
     * @param pressedKey Index der aktuell gedrückten Taste; -1 bedeutet keine Taste
     *
     * Lädt intern ComfortaaBold16 für die Tastenbeschriftung. Der Hintergrund wird mit dem Arbeitsbereich-Hintergrund (0x1082) gefüllt, bevor die Tasten gezeichnet werden.
     */
    void draw(Gfx_Sprite *spr, int originX, int originY, int pressedKey) const;

    /**
     * @brief Zeichnet eine einzelne Taste (für Tap-Feedback).
     * @param spr        Ziel-Sprite
     * @param originX    Linke Kante des Inhaltsbereichs im Sprite
     * @param originY    Obere Kante des Inhaltsbereichs im Sprite
     * @param keyIdx Tastenindex 0 bis NUM_KEYS-1
     * @param pressed    true für Tap-Feedback (Farbe BTN_PRESSED_FILL)
     *
     * Setzt voraus, dass ComfortaaBold16 bereits geladen ist.
     */
    void drawKey(Gfx_Sprite *spr, int originX, int originY,
                 int keyIdx, bool pressed) const;

    // =========================================================================
    // Treffertest
    // =========================================================================

    /**
     * @brief Treffertest auf das Nummernpad (content-relativ).
     * @param cx X-Koordinate relativ zum Inhaltsursprung (originX/originY)
     * @param cy Y-Koordinate relativ zum Inhaltsursprung
     * @return Tastenindex 0 bis NUM_KEYS-1 oder -1 bei keinem Treffer
     */
    int hitTest(int cx, int cy) const;

    // =========================================================================
    // Datenzugriff
    // =========================================================================

    /**
     * @brief Gibt das Aktionszeichen einer Taste zurück.
     * @param keyIdx Tastenindex 0 bis NUM_KEYS-1
     * @return Aktionszeichen ('0'-'9', '.', 'S', 'B', 'C') oder '\0' bei ungültigem Index
     *
     * Wird von executeKey() im jeweiligen Tool verwendet, um die Eingabe zu verarbeiten, ohne direkten Zugriff auf das interne KEYS-Array zu benötigen.
     */
    static char keyAction(int keyIdx);

private:
    // =========================================================================
    // Innere Typen
    // =========================================================================

    /** @brief Achsenparalleles Rechteck (content-relativ). */
    struct Rect
    {
        int x; ///< Linke Kante
        int y; ///< Obere Kante
        int w; ///< Breite
        int h; ///< Höhe
    };

    /** @brief Tastentyp für Farbzuordnung. */
    enum class KeyType : uint8_t
    {
        DIGIT,     ///< Ziffern 0-9 und Dezimalpunkt
        ACTION,    ///< Vorzeichen (+/-)
        BACKSPACE, ///< Backspace-Taste (Vektorsymbol)
        CLEAR      ///< Clear-Taste
    };

    /** @brief Statische Beschreibung einer Nummernpad-Taste. */
    struct KeyEntry
    {
        char    label[5]; ///< Anzeigetext (ASCII, max. 4 Zeichen + NUL)
        char    action;   ///< Aktionszeichen
        KeyType type;     ///< Tastentyp für Farbzuordnung
    };

    // =========================================================================
    // Interne Konstanten
    // =========================================================================

    static constexpr int BTN_RADIUS  = 6; ///< Eckenradius der Tasten in Pixeln
    static constexpr int BTN_TEXT_OY = 2; ///< Vertikaler Korrektur-Offset für Textzentrierung (Font-Metrik-Ausgleich)

    /** @brief Hintergrundfarbe des Nummernpad-Bereichs (= WorkshopCfg::C_WORK_BG, RGB565). */
    static constexpr uint16_t C_PAD_BG = 0x1082;

    static constexpr uint16_t BTN_DIGIT_FILL   = 0x2945; ///< Zifferntasten-Hintergrund
    static constexpr uint16_t BTN_DIGIT_TEXT   = 0xDEDB; ///< Zifferntasten-Beschriftung
    static constexpr uint16_t BTN_ACTION_FILL  = 0x328A; ///< Aktionstasten-Hintergrund (+/-)
    static constexpr uint16_t BTN_ACTION_TEXT  = 0xFFFF; ///< Aktionstasten-Beschriftung
    static constexpr uint16_t BTN_BS_FILL      = 0x3ACF; ///< Backspace-Taste (RetroColors::Nordic)
    static constexpr uint16_t BTN_BS_TEXT      = 0xFFFF; ///< Backspace-Vektorsymbol-Farbe
    static constexpr uint16_t BTN_CLR_FILL     = 0x7800; ///< Clear-Taste (Dunkelrot)
    static constexpr uint16_t BTN_CLR_TEXT     = 0xFFFF; ///< Clear-Taste-Beschriftung
    static constexpr uint16_t BTN_PRESSED_FILL = 0x6B6D; ///< Gedrückte Taste (Tap-Feedback)

    // =========================================================================
    // Statische Datenbasis
    // =========================================================================

    /**
     * @brief Tastendefinitionen des 4×4-Grids.
     *
     * Indizes 0-11: linke 3 Spalten × 4 Zeilen mit normaler Höhe. Index 12: BS - rechte Spalte, Zeilen 0+1 (doppelt hoch). Index 13: C - rechte Spalte, Zeilen 2+3 (doppelt hoch).
     */
    static const KeyEntry KEYS[NUM_KEYS];

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Rect m_keyRects[NUM_KEYS] = {}; ///< Tasten-Rechtecke (content-relativ, von computeLayout() befüllt)
    int  m_btnW               = 0;  ///< Berechnete Tastenbreite in Pixeln
    int  m_btnH               = 0;  ///< Berechnete Tastenhöhe in Pixeln (= TARGET_BTN_H nach computeLayout)
    int  m_startY             = 0;  ///< Y-Start des Nummernpads relativ zum Inhalt, entsprechend h - REQUIRED_H
    int  m_w                  = 0;  ///< Inhaltsbereich-Breite in Pixeln (für Hintergrund-fillRect)

    // =========================================================================
    // Interne Zeichenmethoden
    // =========================================================================

    /**
     * @brief Zeichnet das Backspace-Vektorsymbol (Pfeil-Raute mit Kreuz).
     * @param spr   Ziel-Sprite
     * @param cx    Mittelpunkt X (sprite-relativ)
     * @param cy    Mittelpunkt Y (sprite-relativ)
     * @param size  Gesamtgröße des Symbols in Pixeln
     * @param color Linienfarbe (RGB565)
     *
     * Geometrie: Rahmen oben, unten und rechts, schräge Pfeilkanten links sowie ein Kreuz in der Mitte.
     */
    void drawBackspaceSymbol(Gfx_Sprite *spr, int cx, int cy,
                             int size, uint16_t color) const;
};

#endif // WORKSHOPNUMPAD_H
