#ifndef WORKSHOPTOOL_VOLTAGEDIVIDER_H
#define WORKSHOPTOOL_VOLTAGEDIVIDER_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_VoltageDivider
 * @brief Interaktiver Spannungsteiler-Rechner mit Schaltplan.
 *
 * Schaltung:
 *       Uin
 *        │
 *       [R1]
 *        │
 *   ─────┤──── Uout
 *        │
 *       [R2]
 *        │
 *       GND
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px): Interaktiver Schaltplan mit R1/R2-Kette,
 *   Uin/Uout/GND-Beschriftung, Bauteilwerte und Spannungspfeile.
 *   Rechte Spalte (285 px): Vier Eingabefelder (FIFO-3), Formelzeile
 *   und WorkshopNumpad.
 *
 * FIFO-3: Drei Eingaben, eine berechnet. Tippen auf die berechnete Zeile oder das zugehörige Element im Schaltplan tauscht sie mit dem ältesten Eingabe-Slot.
 *
 * Standardwerte: Uin=12 V, R1=10 kOhm, R2=10 kOhm -> Uout=6 V.
 */
class WorkshopTool_VoltageDivider : public WorkshopTool
{
public:
    WorkshopTool_VoltageDivider();
    ~WorkshopTool_VoltageDivider() override = default;

    WorkshopTool_VoltageDivider(const WorkshopTool_VoltageDivider &)            = delete;
    WorkshopTool_VoltageDivider &operator=(const WorkshopTool_VoltageDivider &) = delete;

    void onEnter(LGFX *tft) override;
    void onExit() override {}

    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    struct Rect { int x, y, w, h; };

    // Konstanten
    static constexpr int NUM_QTY         = 4;
    static constexpr int NUM_INPUT_SLOTS = 3;
    static constexpr int FIELD_CAP       = 24;
    static constexpr int FORMULA_CAP     = 48;

    static constexpr int ROW_H    = 38;
    static constexpr int UNIT_W   = 52;
    static constexpr int ACCENT_W =  4;
    static constexpr int FIELDS_H = ROW_H * NUM_QTY;

    static constexpr int FORMULA_H     = 24;
    static constexpr int FORMULA_GAP_V =  4;

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

    static constexpr uint16_t C_ACCENT_UIN  = 0xFD20;
    static constexpr uint16_t C_ACCENT_R1   = 0x07E0;
    static constexpr uint16_t C_ACCENT_R2   = 0xFFE0;
    static constexpr uint16_t C_ACCENT_UOUT = 0x04FF;

    // Farben des Schaltplans
    static constexpr uint16_t C_SCHEM_BG     = 0x1082;
    static constexpr uint16_t C_WIRE         = 0x8410;
    static constexpr uint16_t C_COMP_BODY    = 0x2945;
    static constexpr uint16_t C_COMP_OUTLINE = 0x6B6D;
    static constexpr uint16_t C_VAL_ACTIVE   = 0x07E0;
    static constexpr uint16_t C_VAL_DIMMED   = 0x7D7A;
    static constexpr uint16_t C_RAIL         = 0xAD75;
    static constexpr uint16_t C_JUNCTION     = 0xAD75;

    // Statische Daten
    static const char * const QTY_SYMBOLS[NUM_QTY];
    static const char * const QTY_UNITS[NUM_QTY];

    // Methoden
    void clearAll();
    void computeLayout(int w, int h);
    void compute();
    static void formatDisplayValue(double val, int qty, char *buf, size_t sz);
    static void formatEditValue(double val, char *buf, size_t sz);
    void buildFormulaStr(char *buf, size_t sz) const;
    bool isInput(int qty) const;
    static uint16_t accentColor(int qty);
    void executeKey(char action);

    void drawSchematic();
    void drawResistor(int cx, int top, int h, bool active);
    void drawFields(bool cursorOn);
    void drawField(int qty, bool pressed, bool cursorOn);
    void drawFormulaRow();
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
    bool    m_computeOk = false;

    Rect m_fieldRects[NUM_QTY] = {};
    Rect m_compHits[NUM_QTY]   = {};
    int  m_rightX = 0;
    int  m_rightW = 0;
    int  m_formulaY = 0;

    WorkshopNumpad m_numpad;

    int  m_pressedField = -1;
    int  m_pressedComp  = -1;
    int  m_pressedKey   = -1;
    bool m_touchWas     = false;
    bool m_touchReady   = false;
};

#endif // WORKSHOPTOOL_VOLTAGEDIVIDER_H
