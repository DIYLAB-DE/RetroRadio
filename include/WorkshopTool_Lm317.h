#ifndef WORKSHOPTOOL_LM317_H
#define WORKSHOPTOOL_LM317_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_Lm317
 * @brief Interaktiver Rechner für den LM317-Spannungsregler mit Schaltplan.
 *
 * Schaltung:
 *   Vin ──┤IN   OUT├──┬── Vout
 *         │ LM317  │  │
 *         └──ADJ───┘  │
 *              │       │
 *              ├─[R1]──┘
 *              │
 *             [R2]
 *              │
 *             GND
 *
 * Formeln (I_adj vernachlässigt, typisch < 100 uA):
 *   Vout = Vref * (1 + R2 / R1)
 *   Vref = Vout * R1 / (R1 + R2)   [R1 + R2 > 0]
 *   R1   = Vref * R2 / (Vout - Vref)  [Vout > Vref, R2 > 0]
 *   R2   = R1 * (Vout / Vref - 1)  [Vref > 0, R1 > 0]
 *
 * Layout (547 x 327 px Content-Bereich):
 *   Linke Spalte (250 px): Interaktiver Schaltplan mit LM317-Gehäuse, R1/R2-Netzwerk, Verdrahtung und Bauteilwerten.
 *   Rechte Spalte (285 px): Vier Eingabefelder (FIFO-3), Info-Bereich (Formel + E24-Normwert) und WorkshopNumpad.
 *
 * FIFO-3-Logik: drei Eingaben, eine berechnet. Tippen auf die berechnete Zeile oder das zugehörige Bauteil im Schaltplan tauscht sie mit dem ältesten Eingabe-Slot.
 *
 * Standardwerte: Vref=1,25 V, R1=240 Ohm, R2=720 Ohm -> Vout = 5,0 V.
 */
class WorkshopTool_Lm317 : public WorkshopTool
{
public:
    WorkshopTool_Lm317();
    ~WorkshopTool_Lm317() override = default;

    WorkshopTool_Lm317(const WorkshopTool_Lm317 &)            = delete;
    WorkshopTool_Lm317 &operator=(const WorkshopTool_Lm317 &) = delete;

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

    struct Rect
    {
        int x, y, w, h;
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_QTY         = 4;  ///< Vref(0), R1(1), R2(2), Vout(3)
    static constexpr int NUM_INPUT_SLOTS = 3;  ///< Drei Eingaben, eine berechnet
    static constexpr int FIELD_CAP       = 24; ///< Max. Eingabepufferlänge inkl. NUL
    static constexpr int INFO_STR_CAP    = 64; ///< Puffergröße für Info-/Formelstrings
    static constexpr int E24_COUNT       = 24; ///< Werte in der E24-Normreihe

    // Globaler Top-Abstand zwischen Header und Tool-Inhalt
    static constexpr int TOP_GAP = 10;

    // Felder-Layout (rechte Spalte)
    static constexpr int ROW_H    = 38;
    static constexpr int UNIT_W   = 52;
    static constexpr int ACCENT_W =  4;
    static constexpr int FIELDS_H = ROW_H * NUM_QTY;

    // Info-Bereich
    static constexpr int INFO_H     = 24;
    static constexpr int INFO_GAP_V =  4;

    // Cursor
    static constexpr unsigned long CURSOR_PERIOD_MS = 800;
    static constexpr unsigned long CURSOR_ON_MS     = 400;

    // Farben - Eingabefelder
    static constexpr uint16_t C_BG          = 0x1082;
    static constexpr uint16_t C_FIELD_INPUT = 0x2945;
    static constexpr uint16_t C_FIELD_FOCUS = 0x328A;
    static constexpr uint16_t C_FIELD_PRESS = 0x4B4D;
    static constexpr uint16_t C_SEP         = 0x2104;

    static constexpr uint16_t C_LBL_FOCUS  = 0xFFFF;
    static constexpr uint16_t C_LBL_INPUT  = 0xDEDB;
    static constexpr uint16_t C_LBL_COMP   = 0x7BEF;
    static constexpr uint16_t C_VAL_FOCUS  = 0xFFFF;
    static constexpr uint16_t C_VAL_INPUT  = 0xEF7D;
    static constexpr uint16_t C_VAL_COMP   = 0xB5F7;
    static constexpr uint16_t C_UNIT_FOCUS = 0xAD75;
    static constexpr uint16_t C_UNIT_INPUT = 0x8410;
    static constexpr uint16_t C_UNIT_COMP  = 0x4228;

    static constexpr uint16_t C_FORMULA_TXT = 0xDEDB;
    static constexpr uint16_t C_INFO_TXT    = 0xB5F7;

    // Akzentfarben
    static constexpr uint16_t C_ACCENT_VREF = 0x801F; ///< Lila
    static constexpr uint16_t C_ACCENT_R1   = 0x07E0; ///< Grün
    static constexpr uint16_t C_ACCENT_R2   = 0xFFE0; ///< Gelb
    static constexpr uint16_t C_ACCENT_VOUT = 0xFD20; ///< Orange

    // Schaltplan-Farben
    static constexpr uint16_t C_SCHEM_BG     = 0x1082;
    static constexpr uint16_t C_IC_BODY      = 0x2945;
    static constexpr uint16_t C_IC_OUTLINE   = 0x4A69;
    static constexpr uint16_t C_IC_TEXT      = 0xDEDB;
    static constexpr uint16_t C_WIRE         = 0x8410;
    static constexpr uint16_t C_COMP_BODY    = 0x2945;
    static constexpr uint16_t C_COMP_OUTLINE = 0x6B6D;
    static constexpr uint16_t C_VAL_ACTIVE   = 0x07E0;
    static constexpr uint16_t C_VAL_DIMMED   = 0x7D7A;
    static constexpr uint16_t C_RAIL         = 0xAD75;
    static constexpr uint16_t C_JUNCTION     = 0xAD75;
    static constexpr uint16_t C_PIN_LABEL    = 0x6B6D;

    // =========================================================================
    // Statische Daten
    // =========================================================================

    static const char * const QTY_SYMBOLS[NUM_QTY];
    static const char * const QTY_UNITS[NUM_QTY];
    static const double E24_VALUES[E24_COUNT];

    // =========================================================================
    // Private Methoden - Lebenszyklus
    // =========================================================================

    void clearAll();
    void computeLayout(int w, int h);

    // =========================================================================
    // Private Methoden - Berechnung
    // =========================================================================

    void compute();
    static void formatDisplayValue(double val, int qty, char *buf, size_t sz);
    static void formatEditValue(double val, char *buf, size_t sz);
    void buildFormulaStr(char *buf, size_t sz) const;
    bool isInput(int qty) const;
    static uint16_t accentColor(int qty);
    static double nearestE24(double r);
    static void formatE24(double r, char *buf, size_t sz);
    void executeKey(char action);

    // =========================================================================
    // Private Methoden - Zeichnen
    // =========================================================================

    /** @brief Zeichnet den Schaltplan (LM317-Gehäuse, R1/R2, Verdrahtung). */
    void drawSchematic();

    /**
     * @brief Zeichnet das LM317-Gehäuse als Rechteck mit Pin-Labels.
     * @param pkgX  Linke Kante des Gehäuses (schaltplanrelativ)
     * @param pkgY  Obere Kante
     * @param pkgW  Breite
     * @param pkgH  Höhe
     */
    void drawPackage(int pkgX, int pkgY, int pkgW, int pkgH);

    /**
     * @brief Zeichnet ein Widerstands-Symbol.
     * @param cx     Mittelpunkt X (schaltplanrelativ)
     * @param top    Oberkante Y
     * @param h      Höhe
     * @param active true wenn das zugehörige Feld fokussiert ist
     */
    void drawResistor(int cx, int top, int h, bool active);

    void drawFields(bool cursorOn);
    void drawField(int qty, bool pressed, bool cursorOn);
    void drawInfoRow();

    // =========================================================================
    // Private Methoden - Touch
    // =========================================================================

    int hitTestField(int cx, int cy) const;
    int hitTestComponent(int cx, int cy) const;

    /**
     * @brief Formatiert einen Bauteilwert kompakt für die Schaltplan-Beschriftung.
     * @param qty Größenindex (0=Vref, 1=R1, 2=R2, 3=Vout)
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße
     */
    void formatSchematicValue(int qty, char *buf, size_t sz) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite  = nullptr;
    int  m_originX        = 0;
    int  m_originY        = 0;
    int  m_w              = 0;
    int  m_h              = 0;
    bool m_layoutValid    = false;

    // FIFO-3 Eingabe-Zustand
    uint8_t m_inputSlots[NUM_INPUT_SLOTS]  = {};
    uint8_t m_computedQty                  = 0;
    uint8_t m_focusedQty                   = 0;
    char    m_inputBuf[NUM_QTY][FIELD_CAP] = {};
    double  m_values[NUM_QTY]              = {};
    bool    m_computeOk                    = false;

    // Layout (content-relativ)
    Rect m_fieldRects[NUM_QTY] = {};  ///< Eingabefeld-Bereiche
    Rect m_compHits[NUM_QTY]   = {};  ///< Schaltplan-Bauteil-Hit-Areas (Vref, R1, R2, Vout)
    int  m_rightX = 0;               ///< X-Start der rechten Spalte
    int  m_rightW = 0;               ///< Breite der rechten Spalte
    int  m_infoY  = 0;               ///< Y-Start der Info-Zeile (spaltenrelativ)

    WorkshopNumpad m_numpad;

    // Touch-Tracking
    int  m_pressedField = -1;
    int  m_pressedComp  = -1;    ///< Gedrücktes Bauteil im Schaltplan (-1 = keines)
    int  m_pressedKey   = -1;
    bool m_touchWas     = false;
    bool m_touchReady   = false;
};

#endif // WORKSHOPTOOL_LM317_H
