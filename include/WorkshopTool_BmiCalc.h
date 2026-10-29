#ifndef WORKSHOPTOOL_BMICALC_H
#define WORKSHOPTOOL_BMICALC_H

#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_BmiCalc
 * @brief Interaktiver BMI-Rechner mit farbiger Skala und WHO-Kategorien.
 *
 * Formel: BMI = Gewicht (kg) / (Größe (m))²
 */
class WorkshopTool_BmiCalc : public WorkshopTool
{
public:
    WorkshopTool_BmiCalc();
    ~WorkshopTool_BmiCalc() override = default;

    WorkshopTool_BmiCalc(const WorkshopTool_BmiCalc &) = delete;
    WorkshopTool_BmiCalc &operator=(const WorkshopTool_BmiCalc &) = delete;

    void onEnter(LGFX *tft) override;
    void onExit() override {}

    void draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h) override;

    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    // =========================================================================
    // Innere Typen
    // =========================================================================

    /** @brief Achsenausgerichtetes Rechteck für Hit-Tests und Layout. */
    struct Rect
    {
        int x, y, w, h;
    };

    /** @brief BMI-Bereich mit Grenzwerten und Farbe. */
    struct BmiZone
    {
        float       bmiMin;   ///< Untere BMI-Grenze (inklusiv)
        float       bmiMax;   ///< Obere BMI-Grenze (exklusiv)
        uint16_t    color;    ///< Skalenfarbe (RGB565)
        WorkshopKey labelKey; ///< Kategorie-Bezeichnung (i18n-Key)
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_FIELDS = 2; ///< Größe (0) und Gewicht (1)
    static constexpr int FIELD_CAP = 16; ///< Max. Eingabepufferlänge inkl. NUL

    // Felder-Layout (rechte Spalte)
    static constexpr int ROW_H = 38;   ///< Zeilenhöhe Eingabefeld
    static constexpr int UNIT_W = 40;  ///< Breite der Einheit
    static constexpr int ACCENT_W = 4; ///< Akzentstreifen links

    // Ergebnisbereich
    static constexpr int RESULT_H = 50;    ///< Höhe des Ergebnisbereichs
    static constexpr int RESULT_GAP_V = 6; ///< Abstand zwischen Feldern und Ergebnis

    // Cursor
    static constexpr unsigned long CURSOR_PERIOD_MS = 800;
    static constexpr unsigned long CURSOR_ON_MS = 400;

    // Spaltenaufteilung
    static constexpr int SCHEM_W = 250; ///< Breite der Skalenansicht
    static constexpr int COL_GAP = 12;  ///< Abstand zwischen Skala und Feldern

    // BMI-Skala
    static constexpr float SCALE_BMI_MIN = 15.0f; ///< Untere Skalengrenze
    static constexpr float SCALE_BMI_MAX = 42.0f; ///< Obere Skalengrenze
    static constexpr int SCALE_BAR_W = 30;        ///< Breite des Farbalkens
    static constexpr int SCALE_BAR_LEFT = 30;     ///< Linker Rand bis Farbalken (Platz für Marker)
    static constexpr int SCALE_TOP = 16;          ///< Oberer Rand (px)
    static constexpr int SCALE_BOTTOM = 16;       ///< Unterer Rand (px)
    static constexpr int MARKER_W = 12;           ///< Breite des Pfeilmarkers

    // BMI-Zonen (WHO-Klassifikation)
    static constexpr int NUM_ZONES = 7;

    // =========================================================================
    // Statische Daten
    // =========================================================================

    static const BmiZone ZONES[NUM_ZONES];
    static const WorkshopKey FIELD_LABEL_KEYS[NUM_FIELDS];
    static const WorkshopKey FIELD_UNIT_KEYS[NUM_FIELDS];
    static const uint16_t FIELD_ACCENTS[NUM_FIELDS];

    // =========================================================================
    // Lebenszyklus
    // =========================================================================

    /** @brief Setzt alle Member auf definierte Ausgangswerte. */
    void clearAll();

    /** @brief Berechnet Feld-Rects, Skalen- und Numpad-Position. */
    void computeLayout(int w, int h);

    // =========================================================================
    // Berechnung
    // =========================================================================

    /** @brief Berechnet BMI aus den aktuellen Eingabewerten. */
    void compute();

    /**
     * @brief Gibt die BMI-Zone für einen Wert zurück.
     * @param bmi BMI-Wert
     * @return Zeiger auf passende BmiZone, oder nullptr
     */
    static const BmiZone *getZone(float bmi);

    /**
     * @brief Verarbeitet eine Numpad-Tasteneingabe.
     * @param action Aktionszeichen ('0'-'9', '.', 'B', 'C')
     */
    void executeKey(char action);

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /** @brief Zeichnet die vertikale BMI-Farbskala mit Marker. */
    void drawScale();

    /** @brief Zeichnet alle Eingabefelder. */
    void drawFields(bool cursorOn);

    /**
     * @brief Zeichnet ein einzelnes Eingabefeld.
     * @param idx     Feldindex (0 = Größe, 1 = Gewicht)
     * @param pressed Feld wird gerade berührt
     * @param cursorOn Cursor-Phase (an/aus)
     */
    void drawField(int idx, bool pressed, bool cursorOn);

    /** @brief Zeichnet den Ergebnisbereich (BMI-Wert + Kategorie). */
    void drawResult();

    // =========================================================================
    // Touch-Verarbeitung
    // =========================================================================

    /** @brief Hit-Test auf Eingabefelder. Gibt Index zurück oder -1. */
    int hitTestField(int cx, int cy) const;

    // =========================================================================
    // Hilfsfunktionen
    // =========================================================================

    /**
     * @brief Ersetzt Dezimalpunkt durch Komma im Puffer.
     * @param buf Puffer mit formatiertem Zahlenwert
     */
    static void dotToComma(char *buf);

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite = nullptr; ///< Work-Sprite (gehört Workshop)
    int m_originX = 0;              ///< Content-Bereich X (sprite-relativ)
    int m_originY = 0;              ///< Content-Bereich Y (sprite-relativ)
    int m_w = 0;                    ///< Content-Bereich Breite
    int m_h = 0;                    ///< Content-Bereich Höhe
    bool m_layoutValid = false;     ///< Layout wurde berechnet

    // Eingabezustand
    uint8_t m_focusedField = 0;                  ///< Aktives Eingabefeld (0 oder 1)
    char m_inputBuf[NUM_FIELDS][FIELD_CAP] = {}; ///< Eingabepuffer
    double m_values[NUM_FIELDS] = {};            ///< Geparste Werte (cm, kg)
    double m_bmi = 0.0;                          ///< Berechneter BMI
    bool m_computeOk = false;                    ///< BMI gültig berechenbar

    // Layout (content-relativ)
    Rect m_fieldRects[NUM_FIELDS] = {}; ///< Eingabefeld-Bereiche
    int m_rightX = 0;                   ///< X-Start der rechten Spalte
    int m_rightW = 0;                   ///< Breite der rechten Spalte
    int m_resultY = 0;                  ///< Y-Start des Ergebnisbereichs

    // Skala (content-relativ)
    int m_scaleBarX = 0; ///< X-Position des Farbalkens
    int m_scaleBarY = 0; ///< Y-Start des Farbalkens
    int m_scaleBarH = 0; ///< Höhe des Farbalkens

    WorkshopNumpad m_numpad;

    // Touch-Tracking
    int m_pressedField = -1;   ///< Gedrücktes Feld (-1 = keines)
    int m_pressedKey = -1;     ///< Gedrückte Numpad-Taste (-1 = keine)
    bool m_touchWas = false;   ///< Im letzten Frame war Touch aktiv
    bool m_touchReady = false; ///< Erstes Release nach onEnter empfangen
};

#endif // WORKSHOPTOOL_BMICALC_H
