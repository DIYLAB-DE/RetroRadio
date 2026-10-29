#ifndef WORKSHOPTOOL_LEDCALCULATOR_H
#define WORKSHOPTOOL_LEDCALCULATOR_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_LedCalculator
 * @brief Interaktiver LED-Vorwiderstand-Rechner mit Schaltplan.
 *
 * Schaltung:
 *   Ub ──[R]──[LED (Uf, If)]── GND
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px): Interaktiver Schaltplan mit vertikaler
 *   Bauteilkette (Ub -> R -> LED -> GND), LED-Symbol mit Emissionspfeilen,
 *   Bauteilwerte und Spannungs-/Stromannotationen.
 *   Rechte Spalte (285 px): Vier Eingabefelder (FIFO-3), Info-Bereich
 *   (Formel + P + E12-Normwert) und WorkshopNumpad.
 *
 * FIFO-3: Drei Eingaben (Ub, Uf, If, R), eine berechnet. Tippen auf Bauteile im Schaltplan wählt das zugehörige Feld.
 *
 * Standardwerte: Ub=5 V, Uf=2,0 V, If=20 mA -> R=150 Ohm, P=60 mW.
 */
class WorkshopTool_LedCalculator : public WorkshopTool
{
public:
    WorkshopTool_LedCalculator();
    ~WorkshopTool_LedCalculator() override = default;

    WorkshopTool_LedCalculator(const WorkshopTool_LedCalculator &)            = delete;
    WorkshopTool_LedCalculator &operator=(const WorkshopTool_LedCalculator &) = delete;

    void onEnter(LGFX *tft) override;
    void onExit() override {}

    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    struct Rect { int x, y, w, h; };

    static constexpr int NUM_QTY         = 4;
    static constexpr int NUM_INPUT_SLOTS = 3;
    static constexpr int FIELD_CAP       = 24;
    static constexpr int INFO_STR_CAP    = 64;
    static constexpr int E12_COUNT       = 12;

    static constexpr int ROW_H    = 38;
    static constexpr int UNIT_W   = 52;
    static constexpr int ACCENT_W =  4;
    static constexpr int FIELDS_H = ROW_H * NUM_QTY;

    static constexpr int INFO_H     = 24;
    static constexpr int INFO_GAP_V =  4;

    static constexpr unsigned long CURSOR_PERIOD_MS = 800;
    static constexpr unsigned long CURSOR_ON_MS     = 400;

    // Farben der Felder
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
    static constexpr uint16_t C_P_INFO_TXT  = 0xB5F7;

    static constexpr uint16_t C_ACCENT_UB = 0xFD20; ///< Orange
    static constexpr uint16_t C_ACCENT_UF = 0x801F; ///< Lila
    static constexpr uint16_t C_ACCENT_IF = 0x001F; ///< Blau
    static constexpr uint16_t C_ACCENT_R  = 0x07E0; ///< Grün

    // Farben des Schaltplans
    static constexpr uint16_t C_SCHEM_BG     = 0x1082;
    static constexpr uint16_t C_WIRE         = 0x8410;
    static constexpr uint16_t C_COMP_BODY    = 0x2945;
    static constexpr uint16_t C_COMP_OUTLINE = 0x6B6D;
    static constexpr uint16_t C_VAL_ACTIVE   = 0x07E0;
    static constexpr uint16_t C_VAL_DIMMED   = 0x7D7A;
    static constexpr uint16_t C_RAIL         = 0xAD75;
    static constexpr uint16_t C_JUNCTION     = 0xAD75;
    static constexpr uint16_t C_LED_CATHODE  = 0xAD75; ///< LED-Dreieck und Kathodenstrich
    static constexpr uint16_t C_LED_EMISSION = 0xFFE0; ///< Emissionspfeile (Gelb)

    // Statische Daten
    static const char * const QTY_SYMBOLS[NUM_QTY];
    static const char * const QTY_UNITS[NUM_QTY];
    static const double E12_VALUES[E12_COUNT];

    // Methoden
    void clearAll();
    void computeLayout(int w, int h);
    void compute();
    static void formatDisplayValue(double val, int qty, char *buf, size_t sz);
    static void formatEditValue(double val, char *buf, size_t sz);
    static void formatPower(double p, char *buf, size_t sz);
    void buildFormulaStr(char *buf, size_t sz) const;
    bool isInput(int qty) const;
    static uint16_t accentColor(int qty);
    static double nearestE12(double r);
    static void formatE12(double r, char *buf, size_t sz);
    void executeKey(char action);

    void drawSchematic();
    void drawLedSymbol(int cx, int topY, bool ufActive, bool ifActive);
    void drawResistor(int cx, int top, int h, bool active);
    void drawFields(bool cursorOn);
    void drawField(int qty, bool pressed, bool cursorOn);
    void drawInfoRow();
    void formatSchematicValue(int qty, char *buf, size_t sz) const;

    int hitTestField(int cx, int cy) const;
    int hitTestComponent(int cx, int cy) const;

    // Member
    Gfx_Sprite *m_sprite = nullptr;
    int m_originX = 0, m_originY = 0, m_w = 0, m_h = 0;
    bool m_layoutValid = false;

    uint8_t m_inputSlots[NUM_INPUT_SLOTS] = {};
    uint8_t m_computedQty = 0;
    uint8_t m_focusedQty  = 0;
    char    m_inputBuf[NUM_QTY][FIELD_CAP] = {};
    double  m_values[NUM_QTY] = {};
    double  m_pValue = 0.0;
    bool    m_computeOk = false;

    Rect m_fieldRects[NUM_QTY] = {};
    Rect m_compHits[NUM_QTY]   = {};
    int  m_rightX = 0;
    int  m_rightW = 0;
    int  m_infoY  = 0;

    WorkshopNumpad m_numpad;

    int  m_pressedField = -1;
    int  m_pressedComp  = -1;
    int  m_pressedKey   = -1;
    bool m_touchWas     = false;
    bool m_touchReady   = false;
};

#endif // WORKSHOPTOOL_LEDCALCULATOR_H
