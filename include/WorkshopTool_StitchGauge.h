#ifndef WORKSHOPTOOL_STITCHGAUGE_H
#define WORKSHOPTOOL_STITCHGAUGE_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_StitchGauge
 * @brief Maschenprobe-Rechner mit zwei unabhängigen FIFO-2-Achsen.
 *
 * Wandelt eine Strickprobe (Maschen und Reihen pro 10 cm Probestück) in die benötigten Maschen- und Reihenzahlen für ein Strickstück der gewünschten Breite und Höhe um. Die Berechnung zerfällt in zwei voneinander getrennte Achsen, die jeweils einer eigenen FIFO-2-Logik folgen:
 *
 * Maschen-Achse (Felder 0, 1, 2):
 *   M / 10 cm  : Probe-Maschenzahl pro 10 cm Probestückbreite
 *   Breite     : Zielbreite des Strickstücks in cm
 *   Maschen    : Anzuschlagende Maschenzahl
 *
 * Reihen-Achse (Felder 3, 4, 5):
 *   R / 10 cm  : Probe-Reihenzahl pro 10 cm Probestückhöhe
 *   Höhe       : Zielhöhe des Strickstücks in cm
 *   Reihen     : Zu strickende Reihenzahl
 *
 * Formeln:
 *   Maschen = M_pro_10 * Breite / 10
 *   Reihen  = R_pro_10 * Höhe   / 10
 *   Auf jede der drei Größen einer Achse auflösbar, FIFO-2 vergibt nach
 *   einem Tap auf die berechnete Größe den ältesten Eingabe-Slot als
 *   neue berechnete Größe.
 *
 * Layout (547 x 327 px Netto-Inhaltsbereich):
 *   Linke Spalte (250 px): Stilisierte Strickstück-Visualisierung
 *     Rechteck der Größe Breite x Höhe, proportional skaliert, mit
 *     V-Maschenmuster und umlaufender Beschriftung. Aktive Achse wird in
 *     den Beschriftungen farblich hervorgehoben.
 *   Rechte Spalte (285 px):
 *     TOP_GAP                 10 px
 *     3 x ROW_H (M-Achse)     93 px (3 * 31)
 *     SECTION_GAP              4 px
 *     3 x ROW_H (R-Achse)     93 px (3 * 31)
 *     Restabstand              7 px
 *     Numpad (REQUIRED_H)    120 px (bottom-anchored)
 *     Summe                  327 px
 *
 * Standardwerte: M=22 / 10 cm, B=50 cm, R=30 / 10 cm, H=60 cm
 *   -> Maschen = 110, Reihen = 180.
 *
 * Eingabepuffer halten den Dezimalpunkt für atof(); die Anzeige formatiert Komma. Eingaben sind auf realistische Stricklängen ausgelegt, die berechneten Werte werden ohne Aufrundung dargestellt - die Strickerin entscheidet selbst, wie sie eine Bruchzahl auf eine ganze Maschenzahl rundet.
 */
class WorkshopTool_StitchGauge : public WorkshopTool
{
public:
    WorkshopTool_StitchGauge();
    ~WorkshopTool_StitchGauge() override = default;

    WorkshopTool_StitchGauge(const WorkshopTool_StitchGauge &)            = delete;
    WorkshopTool_StitchGauge &operator=(const WorkshopTool_StitchGauge &) = delete;

    void onEnter(LGFX *tft) override;
    void onExit() override {}

    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    // =========================================================================
    // Innere Typen
    // =========================================================================

    /** @brief Achsenparalleles Rechteck (content-relativ). */
    struct Rect
    {
        int x, y, w, h;
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_QTY            = 6;  ///< 0..2 = M-Achse, 3..5 = R-Achse
    static constexpr int NUM_SECTIONS       = 2;  ///< Zwei FIFO-2-Sektionen
    static constexpr int SLOTS_PER_SECTION  = 2;  ///< Zwei Eingabe-Slots pro Sektion
    static constexpr int FIELD_CAP          = 16; ///< Eingabepuffer-Kapazität inkl. NUL

    // Globaler Top-Abstand zwischen Header und Tool-Inhalt
    static constexpr int TOP_GAP = 10;

    // Felder-Layout (rechte Spalte)
    static constexpr int ROW_H        = 31; ///< Höhe einer Eingabezeile
    static constexpr int SECTION_GAP  = 4;  ///< Vertikaler Abstand zwischen den zwei Achsen
    static constexpr int UNIT_W       = 56; ///< Spaltenbreite für die Einheit
    static constexpr int ACCENT_W     = 4;  ///< Breite des Akzentbalkens am linken Feldrand

    // Cursor-Blinken
    static constexpr unsigned long CURSOR_PERIOD_MS = 800;
    static constexpr unsigned long CURSOR_ON_MS     = 400;

    // Probefläche
    static constexpr double PROBE_CM = 10.0; ///< Bezugslänge der Strickprobe

    // Farben - Hintergrund und Eingabefelder
    static constexpr uint16_t C_BG          = 0x1082;
    static constexpr uint16_t C_FIELD_INPUT = 0x2945;
    static constexpr uint16_t C_FIELD_FOCUS = 0x328A;
    static constexpr uint16_t C_FIELD_PRESS = 0x4B4D;
    static constexpr uint16_t C_SEP         = 0x2104;

    // Textfarben (drei Zustände: fokussiert / Eingabe / berechnet)
    static constexpr uint16_t C_LBL_FOCUS  = 0xFFFF;
    static constexpr uint16_t C_LBL_INPUT  = 0xDEDB;
    static constexpr uint16_t C_LBL_COMP   = 0x7BEF;
    static constexpr uint16_t C_VAL_FOCUS  = 0xFFFF;
    static constexpr uint16_t C_VAL_INPUT  = 0xEF7D;
    static constexpr uint16_t C_VAL_COMP   = 0xB5F7;
    static constexpr uint16_t C_UNIT_FOCUS = 0xAD75;
    static constexpr uint16_t C_UNIT_INPUT = 0x8410;
    static constexpr uint16_t C_UNIT_COMP  = 0x4228;

    // Achsen-Akzentfarben
    static constexpr uint16_t C_ACCENT_M_AXIS = 0xD986; ///< Maschen-Achse (warmes Garn-Rot)
    static constexpr uint16_t C_ACCENT_R_AXIS = 0x9BBC; ///< Reihen-Achse (mittleres Lila)

    // Visualisierungs-Farben
    static constexpr uint16_t C_VIS_BG       = 0x1082;
    static constexpr uint16_t C_BOX_FILL     = 0x18A4; ///< Hintergrund des Strickstück-Rechtecks
    static constexpr uint16_t C_BOX_OUTLINE  = 0x4A69; ///< Umrandung des Strickstücks
    static constexpr uint16_t C_VPATTERN     = 0x3187; ///< V-Andeutung im Inneren
    static constexpr uint16_t C_LABEL_DIM    = 0xAD75; ///< Inaktive Beschriftung
    static constexpr uint16_t C_VAL_DIM      = 0x7D7A; ///< Inaktiver Wert

    // =========================================================================
    // Statische Datenbasis
    // =========================================================================

    static const WorkshopKey QTY_SYMBOL_KEYS[NUM_QTY]; ///< Anzeigetexte der Felder
    static const WorkshopKey QTY_UNIT_KEYS[NUM_QTY];   ///< Einheiten der Felder

    // =========================================================================
    // Private Methoden - Lebenszyklus
    // =========================================================================

    /** @brief Setzt alle Member auf definierte Ausgangswerte. */
    void clearAll();

    /** @brief Berechnet Spaltenaufteilung, Feld-Rects und Numpad-Layout. */
    void computeLayout(int w, int h);

    // =========================================================================
    // Private Methoden - Berechnung
    // =========================================================================

    /** @brief Parst Eingabepuffer und berechnet beide Achsen. */
    void compute();

    /**
     * @brief Löst die FIFO-2-Logik einer Sektion bei Tap auf eine Größe aus.
     * @param qty Index der getippten Größe (0..NUM_QTY-1)
     */
    void onTapField(int qty);

    /**
     * @brief Liefert den Sektions-Index einer Größe.
     * @param qty Größen-Index (0..NUM_QTY-1)
     * @return 0 für Maschen-Achse, 1 für Reihen-Achse
     */
    static int sectionOf(int qty);

    /** @brief Prüft, ob eine Größe derzeit Eingabe (true) oder berechnet (false) ist. */
    bool isInput(int qty) const;

    /** @brief Liefert die Akzentfarbe der Achse, zu der die Größe gehört. */
    static uint16_t accentColor(int qty);

    /** @brief Formatiert einen Wert mit Komma-Dezimalseparator für die Anzeige. */
    static void formatDisplayValue(double val, char *buf, size_t sz);

    /** @brief Formatiert einen Wert mit Punkt-Dezimalseparator für den Edit-Puffer. */
    static void formatEditValue(double val, char *buf, size_t sz);

    /**
     * @brief Formatiert einen Wert kompakt für die Visualisierungs-Beschriftung.
     * @param qty Größen-Index
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße
     */
    void formatVisLabel(int qty, char *buf, size_t sz) const;

    /** @brief Verarbeitet ein Aktionszeichen vom Numpad auf dem fokussierten Feld. */
    void executeKey(char action);

    // =========================================================================
    // Private Methoden - Zeichnen
    // =========================================================================

    /** @brief Zeichnet die Strickstück-Visualisierung (linke Spalte). */
    void drawVisualization();

    /** @brief Zeichnet alle sechs Eingabefelder (rechte Spalte oberhalb des Numpads). */
    void drawFields(bool cursorOn);

    /** @brief Zeichnet eine einzelne Feldzeile. */
    void drawField(int qty, bool pressed, bool cursorOn);

    /**
     * @brief Zeichnet das V-Maschenmuster im Inneren des Strickstück-Rechtecks.
     * @param boxLeft Linke Kante des Rechtecks (sprite-absolut)
     * @param boxTop  Obere Kante (sprite-absolut)
     * @param boxW    Breite
     * @param boxH    Höhe
     */
    void drawVPattern(int boxLeft, int boxTop, int boxW, int boxH);

    // =========================================================================
    // Private Methoden - Treffertests
    // =========================================================================

    /** @brief Treffertest auf die Eingabefelder, content-relativ. */
    int hitTestField(int cx, int cy) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite     = nullptr;
    int         m_originX    = 0;
    int         m_originY    = 0;
    int         m_w          = 0;
    int         m_h          = 0;
    bool        m_layoutValid = false;

    // FIFO-2-Zustand pro Sektion
    uint8_t m_inputSlotsM[SLOTS_PER_SECTION] = {}; ///< Eingabe-Slots der Maschen-Achse (alt, neu)
    uint8_t m_inputSlotsR[SLOTS_PER_SECTION] = {}; ///< Eingabe-Slots der Reihen-Achse (alt, neu)
    uint8_t m_computedM                      = 0;  ///< Berechnete Größe der Maschen-Achse
    uint8_t m_computedR                      = 0;  ///< Berechnete Größe der Reihen-Achse
    uint8_t m_focusedQty                     = 0;  ///< Aktuell fokussierte Größe (0..NUM_QTY-1)

    char    m_inputBuf[NUM_QTY][FIELD_CAP] = {}; ///< Editierbare Eingabepuffer (mit '.')
    double  m_values[NUM_QTY]              = {}; ///< Aktuelle numerische Werte
    bool    m_computeOk[NUM_SECTIONS]      = {}; ///< Berechnungsstatus pro Achse

    // Layout (content-relativ)
    Rect m_fieldRects[NUM_QTY] = {}; ///< Feld-Rechtecke
    int  m_rightX              = 0;  ///< X-Start der rechten Spalte
    int  m_rightW              = 0;  ///< Breite der rechten Spalte

    WorkshopNumpad m_numpad;

    // Touch-Tracking
    int  m_pressedField = -1;
    int  m_pressedKey   = -1;
    bool m_touchWas     = false;
    bool m_touchReady   = false;
};

#endif // WORKSHOPTOOL_STITCHGAUGE_H
