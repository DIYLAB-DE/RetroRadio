#ifndef WORKSHOPTOOL_LOGICGATE_H
#define WORKSHOPTOOL_LOGICGATE_H

// ============================================================================
// Interaktiver Logikgatter-Simulator
// ============================================================================

#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopTool.h"

// ============================================================================
// GATTERTYPEN
// ============================================================================

/**
 * @brief Logikgatter-Typ gemäß DIN 40700 / IEC 60617.
 */
enum class GateType : uint8_t
{
    AND = 0, ///< Konjunktion (&)
    OR,      ///< Disjunktion (>=1)
    XOR,     ///< Exklusiv-Oder (=1)
    NAND,    ///< Negierte Konjunktion (& + Negationskreis)
    NOR,     ///< Negierte Disjunktion (>=1 + Negationskreis)
    NOT,     ///< Inverter (1 + Negationskreis)
    COUNT    ///< Sentinel
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_LogicGate
 * @brief Interaktiver Logikgatter-Simulator mit DIN-Schaltplan und Wahrheitstabelle.
 *
 * Sechs Gattertypen (AND, OR, XOR, NAND, NOR, NOT) per Chip-Auswahl umschaltbar. Eingänge per Touch umschaltbar (0/1), Ausgang live berechnet.
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Oben (48 px): Chip-Selektor mit 6 Gattertypen
 *   Links (250 px): DIN-Schaltplan-Symbol mit interaktiven Eingängen
 *   Rechts (285 px): Vollständige Wahrheitstabelle mit Hervorhebung
 *   der aktuellen Eingangskombination
 *
 * DIN-Symbole: Rechteck mit Funktionstext ("&", ">=1", "=1", "1"), optionaler Negationskreis am Ausgang (NAND, NOR, NOT).
 *
 * Alle Gatter arbeiten mit zwei Eingängen; NOT verwendet nur einen Eingang. Eine Fußzeilensteuerung ist nicht erforderlich.
 */
class WorkshopTool_LogicGate : public WorkshopTool
{
public:
    WorkshopTool_LogicGate();
    ~WorkshopTool_LogicGate() override = default;

    WorkshopTool_LogicGate(const WorkshopTool_LogicGate &)            = delete;
    WorkshopTool_LogicGate &operator=(const WorkshopTool_LogicGate &) = delete;

    void onEnter(LGFX *tft) override;
    void onExit() override;

    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    const char *getSubtitle() const override;

private:
    // =========================================================================
    // Innere Typen
    // =========================================================================

    /** @brief Achsenausgerichtetes Rechteck für Hit-Tests. */
    struct Rect
    {
        int x, y, w, h;
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_GATE_TYPES = static_cast<int>(GateType::COUNT); ///< 6 Gattertypen

    // Selector-Chips
    static constexpr int CHIP_H      = 28;  ///< Chip-Höhe
    static constexpr int CHIP_GAP    = 4;   ///< Abstand zwischen Chips
    static constexpr int CHIP_R      = 6;   ///< Eckenradius
    static constexpr int CHIP_TOP    = 10;  ///< Abstand zwischen Kopfbereich und Chips
    static constexpr int CHIP_BOTTOM = 10;  ///< Abstand zwischen Chips und folgendem Inhalt
    static constexpr int SEL_ROW_H   = CHIP_TOP + CHIP_H + CHIP_BOTTOM; ///< Gesamthöhe Selector-Zone (48 px)

    // Spaltenaufteilung
    static constexpr int SCHEM_W  = 250; ///< Breite des Schaltplan-Bereichs
    static constexpr int COL_GAP  = 12;  ///< Abstand zwischen Schaltplan und Tabelle

    // DIN-Gatter-Symbol
    static constexpr int GATE_W        = 118; ///< Breite des Gatter-Rechtecks
    static constexpr int GATE_H        = 130; ///< Höhe des Gatter-Rechtecks
    static constexpr int GATE_R        = 4;   ///< Eckenradius
    static constexpr int NEG_CIRCLE_R  = 6;   ///< Radius des Negationskreises
    static constexpr int WIRE_LEN      = 22;  ///< Länge der Eingangs-/Ausgangsleitungen
    static constexpr int INPUT_OFFSET  = 30;  ///< Abstand Eingang A/B vom oberen/unteren Gatter-Rand

    // Wahrheitstabelle
    static constexpr int TABLE_ROW_H    = 34; ///< Zeilenhöhe
    static constexpr int TABLE_HEADER_H = 32; ///< Kopfzeilenhöhe

    // =========================================================================
    // Statische Daten
    // =========================================================================

    /** @brief Chip-Label für jeden Gattertyp. */
    static const char *const GATE_LABELS[NUM_GATE_TYPES];

    /** @brief DIN-Funktionstext im Gatter-Rechteck. */
    static const char *const GATE_SYMBOLS[NUM_GATE_TYPES];

    /** @brief Beschreibung für den Subtitle (lokalisierbar). */
    static const WorkshopKey GATE_DESCRIPTION_KEYS[NUM_GATE_TYPES];

    // =========================================================================
    // Lebenszyklus
    // =========================================================================

    /** @brief Setzt alle Member auf definierte Ausgangswerte. */
    void clearAll();

    /** @brief Berechnet Chip-Rects, Schaltplan- und Tabellenposition. */
    void computeLayout(int w, int h);

    // =========================================================================
    // Logik
    // =========================================================================

    /**
     * @brief Berechnet den Ausgang eines Gatters.
     * @param type Gattertyp
     * @param a    Eingang A
     * @param b    Eingang B
     * @return Logischer Ausgangswert
     */
    static bool computeGate(GateType type, bool a, bool b);

    /** @brief Gibt die Anzahl der Eingänge für den Gattertyp zurück (1 oder 2). */
    static int getNumInputs(GateType type);

    /** @brief Gibt true zurück wenn der Gattertyp einen Negationskreis hat. */
    static bool hasNegation(GateType type);

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /** @brief Zeichnet die Gattertyp-Auswahl-Chips. */
    void drawSelector();

    /** @brief Zeichnet den DIN-Schaltplan mit interaktiven Eingängen. */
    void drawSchematic();

    /** @brief Zeichnet das DIN-Gatter-Rechteck mit Funktionstext. */
    void drawGateBody(int gx, int gy);

    /** @brief Zeichnet die Wahrheitstabelle mit Hervorhebung. */
    void drawTruthTable();

    // =========================================================================
    // Touch-Verarbeitung
    // =========================================================================

    /** @brief Hit-Test auf Selector-Chips. Gibt Index zurück oder -1. */
    int hitTestChip(int cx, int cy) const;

    /**
     * @brief Hit-Test auf Eingangs-Touch-Bereiche.
     * @return 0 = Eingang A, 1 = Eingang B, -1 = kein Treffer
     */
    int hitTestInput(int cx, int cy) const;

    // =========================================================================
    // Untertitel
    // =========================================================================

    /**
     * @brief Befüllt m_subtitleBuf mit der lokalisierten Beschreibung von m_gateType.
     *
     * Wird in clearAll() und nach jeder m_gateType-Änderung aufgerufen, damit getSubtitle() einen stabilen Pointer auf den aktuellen Beschreibungstext zurückliefern kann (Workshop ruft getSubtitle() pro Redraw-Frame auf).
     */
    void refreshSubtitle();

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite  = nullptr; ///< Work-Sprite (gehört Workshop)
    int  m_originX        = 0;       ///< Content-Bereich X (sprite-relativ)
    int  m_originY        = 0;       ///< Content-Bereich Y (sprite-relativ)
    int  m_w              = 0;       ///< Content-Bereich Breite
    int  m_h              = 0;       ///< Content-Bereich Höhe
    bool m_layoutValid    = false;   ///< Layout wurde berechnet

    // Zustand
    GateType m_gateType   = GateType::AND; ///< Aktiver Gattertyp
    bool m_inputA         = false;         ///< Eingang A (0/1)
    bool m_inputB         = false;         ///< Eingang B (0/1)

    // Layout
    Rect m_chipRects[NUM_GATE_TYPES] = {};  ///< Selector-Chip-Bereiche (content-relativ)

    int m_schemX = 0; ///< X-Start Schaltplan-Bereich (content-relativ)
    int m_schemY = 0; ///< Y-Start Schaltplan-Bereich (content-relativ)
    int m_schemW = 0; ///< Breite Schaltplan-Bereich
    int m_schemH = 0; ///< Höhe Schaltplan-Bereich

    int m_tableX = 0; ///< X-Start Wahrheitstabelle (content-relativ)
    int m_tableY = 0; ///< Y-Start Wahrheitstabelle (content-relativ)
    int m_tableW = 0; ///< Breite Wahrheitstabelle
    int m_tableH = 0; ///< Höhe Wahrheitstabelle

    Rect m_inputTouchA = {}; ///< Touch-Bereich Eingang A (content-relativ)
    Rect m_inputTouchB = {}; ///< Touch-Bereich Eingang B (content-relativ)

    // Touch-Tracking
    int  m_pressedChip  = -1;    ///< Aktuell gedrückter Chip (-1 = keiner)
    int  m_pressedInput = -1;    ///< Aktuell gedrückter Eingang (-1 = keiner)
    bool m_touchWas     = false; ///< Im letzten Frame war Touch aktiv
    bool m_touchReady   = false; ///< Erstes Release nach onEnter empfangen

    // Subtitle-Puffer (dynamische Beschreibung des aktuellen Gattertyps)
    char m_subtitleBuf[64] = {}; ///< Befüllt von refreshSubtitle(), gelesen von getSubtitle()
};

#endif // WORKSHOPTOOL_LOGICGATE_H
