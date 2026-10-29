#ifndef WORKSHOPTOOL_TEMP_H
#define WORKSHOPTOOL_TEMP_H

// ============================================================================
// Temperaturumrechner für °C, °F und K
// ============================================================================

#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_Temp
 * @brief Bidirektionaler Temperatur-Umrechner mit Thermometer-Visualisierung.
 *
 * Formeln:
 *   °F = °C × 9/5 + 32
 *   K  = °C + 273,15
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px): Thermometer-Darstellung mit Farbverlauf
 *   (blau -> grün -> gelb -> rot), Quecksilbersäule und Referenzpunkten.
 *   Rechte Spalte (285 px): Drei Eingabefelder (°C, °F, K),
 *   Info-Zeile und WorkshopNumpad.
 *
 * Alle drei Felder sind editierbar. Das fokussierte Feld ist die Eingabe, die anderen beiden zeigen die umgerechneten Werte.
 */
class WorkshopTool_Temp : public WorkshopTool
{
public:
    WorkshopTool_Temp();
    ~WorkshopTool_Temp() override = default;

    WorkshopTool_Temp(const WorkshopTool_Temp &)            = delete;
    WorkshopTool_Temp &operator=(const WorkshopTool_Temp &) = delete;

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

    /** @brief Referenzpunkt auf der Thermometer-Skala. */
    struct RefPoint
    {
        float       celsius;  ///< Temperatur in °C
        WorkshopKey labelKey; ///< Schlüssel für die Beschriftung
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_FIELDS = 3;   ///< °C (0), °F (1), K (2)
    static constexpr int FIELD_CAP  = 16;  ///< Max. Eingabepufferlänge inkl. NUL

    static constexpr double KELVIN_OFFSET = 273.15; ///< K = °C + 273,15

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
    static constexpr int THERMO_W = 250; ///< Breite des Thermometer-Bereichs
    static constexpr int COL_GAP  = 12;  ///< Abstand zwischen Thermometer und Feldern

    // Thermometer-Geometrie
    static constexpr int THERMO_BAR_W    = 20;  ///< Breite der Quecksilbersäule
    static constexpr int THERMO_BAR_LEFT = 50;  ///< Linker Rand bis Säule
    static constexpr int THERMO_TOP      = 16;  ///< Oberer Rand
    static constexpr int THERMO_BOTTOM   = 16;  ///< Unterer Rand
    static constexpr int THERMO_BULB_R   = 16;  ///< Radius des Thermometer-Bulbs
    static constexpr int THERMO_BAR_R    = 4;   ///< Eckenradius der Säule

    // Skala: sichtbarer Bereich in °C
    static constexpr float SCALE_MIN_C = -40.0f;  ///< Untere Skalengrenze (°C)
    static constexpr float SCALE_MAX_C = 220.0f;  ///< Obere Skalengrenze (°C)

    // Referenzpunkte
    static constexpr int NUM_REF_POINTS = 7;

    // =========================================================================
    // Statische Daten
    // =========================================================================

    static const RefPoint REF_POINTS[NUM_REF_POINTS];
    static const WorkshopKey FIELD_LABEL_KEYS[NUM_FIELDS];
    static const WorkshopKey FIELD_UNIT_KEYS[NUM_FIELDS];
    static const uint16_t FIELD_ACCENTS[NUM_FIELDS];

    // =========================================================================
    // Lebenszyklus
    // =========================================================================

    void clearAll();
    void computeLayout(int w, int h);

    // =========================================================================
    // Berechnung
    // =========================================================================

    /** @brief Berechnet die nicht-fokussierten Werte aus dem fokussierten. */
    void compute();

    /** @brief Verarbeitet eine Numpad-Tasteneingabe. */
    void executeKey(char action);

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /** @brief Zeichnet die Thermometer-Visualisierung. */
    void drawThermometer();

    /** @brief Zeichnet alle Eingabefelder. */
    void drawFields(bool cursorOn);

    /** @brief Zeichnet ein einzelnes Eingabefeld. */
    void drawField(int idx, bool pressed, bool cursorOn);

    /** @brief Zeichnet die Info-Zeile mit Formelhinweis. */
    void drawInfoRow();

    // =========================================================================
    // Touch-Verarbeitung
    // =========================================================================

    /** @brief Hit-Test auf Eingabefelder. Gibt Index zurück oder -1. */
    int hitTestField(int cx, int cy) const;

    // =========================================================================
    // Hilfsfunktionen
    // =========================================================================

    /** @brief Ersetzt Dezimalpunkt durch Komma. */
    static void dotToComma(char *buf);

    /**
     * @brief Interpoliert eine Farbe auf der Thermometer-Skala.
     * @param celsius Temperatur in °C
     * @return RGB565-Farbe im Verlauf blau -> grün -> gelb -> rot
     */
    static uint16_t tempToColor(float celsius);

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite  = nullptr;
    int  m_originX        = 0;
    int  m_originY        = 0;
    int  m_w              = 0;
    int  m_h              = 0;
    bool m_layoutValid    = false;

    // Eingabezustand
    uint8_t m_focusedField         = 0;
    char    m_inputBuf[NUM_FIELDS][FIELD_CAP] = {};
    double  m_values[NUM_FIELDS]   = {};
    bool    m_computeOk            = false;

    // Layout (content-relativ)
    Rect m_fieldRects[NUM_FIELDS]  = {};
    int  m_rightX  = 0;
    int  m_rightW  = 0;
    int  m_infoY   = 0;

    // Thermometer (content-relativ)
    int m_thermoBarX = 0; ///< X-Position der Säule
    int m_thermoBarY = 0; ///< Y-Start der Säule (oben)
    int m_thermoBarH = 0; ///< Höhe der Säule

    WorkshopNumpad m_numpad;

    // Touch-Tracking
    int  m_pressedField = -1;
    int  m_pressedKey   = -1;
    bool m_touchWas     = false;
    bool m_touchReady   = false;
};

#endif // WORKSHOPTOOL_TEMP_H
