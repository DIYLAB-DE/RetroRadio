#ifndef WORKSHOPTOOL_INCHMM_H
#define WORKSHOPTOOL_INCHMM_H

// ============================================================================
// Zoll-mm-Umrechner
// ============================================================================

#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_InchMm
 * @brief Bidirektionaler Zoll/mm-Umrechner mit Referenztabelle.
 *
 * Formel: mm = Zoll × 25,4
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px): Referenztabelle gängiger Zollbrüche
 *   (1/16" bis 1" in Sechzehntel-Schritten) mit mm-Äquivalent.
 *   Rechte Spalte (285 px): Zwei Eingabefelder (Zoll, mm),
 *   Info-Zeile (Umrechnungsfaktor) und WorkshopNumpad.
 *
 * Beide Felder sind editierbar. Das fokussierte Feld ist die Eingabe, das andere zeigt den umgerechneten Wert.
 */
class WorkshopTool_InchMm : public WorkshopTool
{
public:
    WorkshopTool_InchMm();
    ~WorkshopTool_InchMm() override = default;

    WorkshopTool_InchMm(const WorkshopTool_InchMm &)            = delete;
    WorkshopTool_InchMm &operator=(const WorkshopTool_InchMm &) = delete;

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

    /** @brief Achsenausgerichtetes Rechteck. */
    struct Rect
    {
        int x, y, w, h;
    };

    /** @brief Zeile in der Referenztabelle. */
    struct RefEntry
    {
        WorkshopKey fractionKey; ///< Schlüssel für die Bruchdarstellung
        float       mm;          ///< mm-Äquivalent
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_FIELDS  = 2;   ///< Zoll (0) und mm (1)
    static constexpr int FIELD_CAP   = 16;  ///< Max. Eingabepufferlänge inkl. NUL
    static constexpr int NUM_REF     = 16;  ///< Einträge in der Referenztabelle

    static constexpr double INCH_TO_MM = 25.4; ///< Umrechnungsfaktor

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
    // Lebenszyklus
    // =========================================================================

    void clearAll();
    void computeLayout(int w, int h);

    // =========================================================================
    // Berechnung
    // =========================================================================

    /** @brief Berechnet den nicht-fokussierten Wert aus dem fokussierten. */
    void compute();

    void executeKey(char action);

    // =========================================================================
    // Zeichnen
    // =========================================================================

    void drawRefTable();
    void drawFields(bool cursorOn);
    void drawField(int idx, bool pressed, bool cursorOn);
    void drawInfoRow();

    // =========================================================================
    // Touch-Verarbeitung
    // =========================================================================

    int hitTestField(int cx, int cy) const;

    // =========================================================================
    // Hilfsfunktionen
    // =========================================================================

    static void dotToComma(char *buf);

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
    int  m_refRowH = 0; ///< Berechnete Zeilenhöhe der Referenztabelle

    WorkshopNumpad m_numpad;

    // Touch-Tracking
    int  m_pressedField = -1;
    int  m_pressedKey   = -1;
    bool m_touchWas     = false;
    bool m_touchReady   = false;
};

#endif // WORKSHOPTOOL_INCHMM_H
