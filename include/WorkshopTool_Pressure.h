#ifndef WORKSHOPTOOL_PRESSURE_H
#define WORKSHOPTOOL_PRESSURE_H

// ============================================================================
// Druckumrechner für bar, psi, atm und Pa
// ============================================================================

#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_Pressure
 * @brief Bidirektionaler Druck-Umrechner mit Referenztabelle.
 *
 * Umrechnung über Pascal als Pivot:
 *   1 bar = 100.000 Pa
 *   1 psi = 6.894,757 Pa
 *   1 atm = 101.325 Pa
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px): Referenztabelle gängiger Druckwerte
 *   (Reifendruck, Pneumatik, Wasserleitungen etc.).
 *   Rechte Spalte (285 px): Vier Eingabefelder (bar, psi, atm, Pa),
 *   Info-Zeile und WorkshopNumpad.
 *
 * Alle vier Felder sind editierbar. Das fokussierte Feld ist die Eingabe, die anderen drei zeigen die umgerechneten Werte.
 */
class WorkshopTool_Pressure : public WorkshopTool
{
public:
    WorkshopTool_Pressure();
    ~WorkshopTool_Pressure() override = default;

    WorkshopTool_Pressure(const WorkshopTool_Pressure &)            = delete;
    WorkshopTool_Pressure &operator=(const WorkshopTool_Pressure &) = delete;

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

    /** @brief Zeile in der Referenztabelle. */
    struct RefEntry
    {
        float       bar;      ///< Druckwert in bar
        WorkshopKey labelKey; ///< Beschreibung (i18n-Key)
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_FIELDS = 4;   ///< bar (0), psi (1), atm (2), Pa (3)
    static constexpr int FIELD_CAP  = 16;  ///< Max. Eingabepufferlänge inkl. NUL
    static constexpr int NUM_REF    = 8;   ///< Einträge in der Referenztabelle

    // Umrechnungsfaktoren von der jeweiligen Einheit in Pascal
    static constexpr double BAR_TO_PA = 100000.0;
    static constexpr double PSI_TO_PA = 6894.757;
    static constexpr double ATM_TO_PA = 101325.0;

    // Felder-Layout (rechte Spalte)
    static constexpr int ROW_H    = 38; ///< Zeilenhöhe Eingabefeld
    static constexpr int UNIT_W   = 40; ///< Breite der Einheit
    static constexpr int ACCENT_W = 4;  ///< Akzentstreifen links

    // Info-Zeile
    static constexpr int INFO_H     = 24; ///< Höhe der Info-Zeile
    static constexpr int INFO_GAP_V = 4;  ///< Abstand zwischen Feldern und Information

    // Cursor
    static constexpr unsigned long CURSOR_PERIOD_MS = 800;
    static constexpr unsigned long CURSOR_ON_MS     = 400;

    // Spaltenaufteilung
    static constexpr int REF_W   = 250; ///< Breite der Referenztabelle
    static constexpr int COL_GAP = 12;  ///< Abstand zwischen Tabelle und Feldern

    // Referenztabelle
    static constexpr int REF_HDR_H = 22; ///< Kopfzeilenhöhe

    // =========================================================================
    // Statische Daten
    // =========================================================================

    static const RefEntry REF_TABLE[NUM_REF];
    static const WorkshopKey FIELD_LABEL_KEYS[NUM_FIELDS];
    static const WorkshopKey FIELD_UNIT_KEYS[NUM_FIELDS];
    static const uint16_t FIELD_ACCENTS[NUM_FIELDS];

    // =========================================================================
    // Private Methoden
    // =========================================================================

    void clearAll();
    void computeLayout(int w, int h);
    void compute();
    void executeKey(char action);

    void drawRefTable();
    void drawFields(bool cursorOn);
    void drawField(int idx, bool pressed, bool cursorOn);
    void drawInfoRow();

    int hitTestField(int cx, int cy) const;
    static void dotToComma(char *buf);

    /**
     * @brief Formatiert einen Druckwert kompakt (auto-skaliert).
     * @param val   Wert in der jeweiligen Einheit
     * @param isPa true für Pascalwerte, die bei großen Zahlen als kPa oder MPa ausgegeben werden
     * @param buf   Ausgabepuffer
     * @param sz    Puffergröße
     */
    static void formatPressure(double val, bool isPa, char *buf, size_t sz);

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite  = nullptr;
    int  m_originX        = 0;
    int  m_originY        = 0;
    int  m_w              = 0;
    int  m_h              = 0;
    bool m_layoutValid    = false;

    uint8_t m_focusedField         = 0;
    char    m_inputBuf[NUM_FIELDS][FIELD_CAP] = {};
    double  m_values[NUM_FIELDS]   = {};
    bool    m_computeOk            = false;

    Rect m_fieldRects[NUM_FIELDS]  = {};
    int  m_rightX  = 0;
    int  m_rightW  = 0;
    int  m_infoY   = 0;
    int  m_refRowH = 0; ///< Berechnete Zeilenhöhe der Referenztabelle

    WorkshopNumpad m_numpad;

    int  m_pressedField = -1;
    int  m_pressedKey   = -1;
    bool m_touchWas     = false;
    bool m_touchReady   = false;
};

#endif // WORKSHOPTOOL_PRESSURE_H
