#ifndef WORKSHOPTOOL_STITCHDISTRIBUTOR_H
#define WORKSHOPTOOL_STITCHDISTRIBUTOR_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_StitchDistributor
 * @brief Verteilt eine Maschenzunahme oder -abnahme gleichmäßig über eine Runde.
 *
 * Eingaben sind zwei ganzzahlige Maschenzahlen "Von" (N) und "Auf" (M). Aus der Differenz D = |M - N| ergibt sich die Verteilung über die Ausgangs-runde nach dem Schema BASE = N / D mit Rest REM = N mod D. Bei REM = 0 fallen alle D Segmente gleich lang aus, sonst zerfällt die Runde in REM Segmente der Länge BASE+1 und (D-REM) Segmente der Länge BASE. Jedes Segment endet mit der Aktion "1 Masche zunehmen" (Zunahme) oder "2 Maschen zusammen" (Abnahme).
 *
 * Layout (547 x 327 px Netto-Inhaltsbereich):
 *   Linke Spalte (250 px): Visualisierung
 *     Titelzeile mit Richtung, Kreisring der die Runde darstellt mit Tick-
 *     Markern an den Aktionspositionen, Differenzzahl im Ringzentrum,
 *     darunter mehrzeiliger Anweisungstext.
 *   Rechte Spalte (285 px):
 *     TOP_GAP                 10 px
 *     2 x ROW_H (Eingabe)     62 px (2 * 31)
 *     Restabstand            135 px
 *     Numpad (REQUIRED_H)    120 px (bottom-anchored)
 *     Summe                  327 px
 *
 * Maschenzahlen sind ganzzahlig. Die Numpad-Tasten '.' und 'S' (Vorzeichen) werden in diesem Werkzeug ignoriert; nur Ziffern, Backspace und Clear verändern den Eingabepuffer.
 *
 * Standardwerte: Von = 84, Auf = 96 (entspricht dem Beispiel aus der Aufgabenstellung, ergibt 12 Zunahmen alle 7 Maschen).
 *
 * Randfälle:
 *   N == 0 oder M == 0 -> Eingabe unvollständig, Hinweistext
 *   N == M             -> "Keine Änderung", Runde wird gerade gestrickt
 *   D > N (Zunahme)    -> mehr Zunahmen als Maschen, in einer Runde nicht
 *                         sinnvoll machbar
 *   D > N/2 (Abnahme)  -> BASE < 2, eine Zusammenstrickaktion passt nicht
 *                         mehr in das Segment
 */
class WorkshopTool_StitchDistributor : public WorkshopTool
{
public:
    WorkshopTool_StitchDistributor();
    ~WorkshopTool_StitchDistributor() override = default;

    WorkshopTool_StitchDistributor(const WorkshopTool_StitchDistributor &)            = delete;
    WorkshopTool_StitchDistributor &operator=(const WorkshopTool_StitchDistributor &) = delete;

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

    /** @brief Richtung der Maschenanpassung. */
    enum class Direction : uint8_t
    {
        NONE     = 0, ///< Von und Auf sind gleich
        INCREASE = 1, ///< Auf > Von
        DECREASE = 2  ///< Auf < Von
    };

    /** @brief Status der Verteilungsberechnung. */
    enum class Status : uint8_t
    {
        EMPTY      = 0, ///< Mindestens ein Eingabewert fehlt oder ist null
        NO_CHANGE  = 1, ///< Von == Auf, keine Verteilung erforderlich
        OK         = 2, ///< Verteilung gültig berechnet
        INFEASIBLE = 3  ///< Differenz zu groß für eine einzelne Runde
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_FIELDS = 2;  ///< 0 = Von, 1 = Auf
    static constexpr int FIELD_CAP  = 8;  ///< Eingabepuffer-Kapazität inkl. NUL

    // Globaler Top-Abstand zwischen Header und Tool-Inhalt
    static constexpr int TOP_GAP = 10;

    // Felder-Layout (rechte Spalte)
    static constexpr int ROW_H    = 31; ///< Höhe einer Eingabezeile
    static constexpr int UNIT_W   = 56; ///< Spaltenbreite für die Einheit
    static constexpr int ACCENT_W = 4;  ///< Breite des Akzentbalkens am linken Feldrand

    // Cursor-Blinken
    static constexpr unsigned long CURSOR_PERIOD_MS = 800;
    static constexpr unsigned long CURSOR_ON_MS     = 400;

    // Eingabe-Begrenzung
    static constexpr int MAX_STITCH_COUNT = 9999; ///< Obergrenze pro Eingabefeld

    // Farben - Hintergrund und Eingabefelder
    static constexpr uint16_t C_BG          = 0x1082;
    static constexpr uint16_t C_FIELD_INPUT = 0x2945;
    static constexpr uint16_t C_FIELD_FOCUS = 0x328A;
    static constexpr uint16_t C_FIELD_PRESS = 0x4B4D;
    static constexpr uint16_t C_SEP         = 0x2104;

    // Textfarben für die Eingabefelder
    static constexpr uint16_t C_LBL_FOCUS  = 0xFFFF;
    static constexpr uint16_t C_LBL_INPUT  = 0xDEDB;
    static constexpr uint16_t C_VAL_FOCUS  = 0xFFFF;
    static constexpr uint16_t C_VAL_INPUT  = 0xEF7D;
    static constexpr uint16_t C_UNIT_FOCUS = 0xAD75;
    static constexpr uint16_t C_UNIT_INPUT = 0x8410;

    // Akzentfarben pro Richtung
    static constexpr uint16_t C_ACCENT_INC  = 0xD986; ///< Zunahme (warmes Garn-Rot)
    static constexpr uint16_t C_ACCENT_DEC  = 0x9BBC; ///< Abnahme (mittleres Lila)
    static constexpr uint16_t C_ACCENT_NONE = 0xAD75; ///< Neutralton ohne aktive Richtung

    // Visualisierungs-Farben
    static constexpr uint16_t C_RING_OUTLINE = 0x4A69; ///< Umrisslinie des Rings
    static constexpr uint16_t C_TITLE_TEXT   = 0xDEDB; ///< Titelzeile über dem Ring
    static constexpr uint16_t C_INSTR_TEXT   = 0xDEDB; ///< Anweisungstext unter dem Ring
    static constexpr uint16_t C_HINT_TEXT    = 0xAD75; ///< Hinweistext bei Sonderzuständen

    // Layout der linken Spalte
    static constexpr int VIS_TITLE_TOP_Y    = 12;  ///< Y-Mitte der Titelzeile
    static constexpr int VIS_RING_CENTER_Y  = 130; ///< Y-Position des Ringmittelpunkts
    static constexpr int VIS_RING_RADIUS    = 60;  ///< Ringradius in Pixeln
    static constexpr int VIS_RING_THICKNESS = 2;   ///< Strichstärke der Ringlinie
    static constexpr int VIS_TICK_INNER     = 56;   ///< Innenradius des Tick-Markers
    static constexpr int VIS_TICK_OUTER     = 70;   ///< Außenradius des Tick-Markers
    static constexpr float VIS_TICK_WIDTH   = 2.5f; ///< Strichstärke eines Tick-Markers in Pixeln
    static constexpr int VIS_INSTR_TOP_Y    = 215;  ///< Y-Start des Anweisungstextblocks
    static constexpr int VIS_INSTR_LINE_H   = 20;   ///< Zeilenhöhe im Anweisungsblock

    // =========================================================================
    // Statische Datenbasis
    // =========================================================================

    static const WorkshopKey FIELD_LABEL_KEYS[NUM_FIELDS]; ///< Anzeigetexte der Felder
    static const WorkshopKey FIELD_UNIT_KEYS[NUM_FIELDS];  ///< Einheiten der Felder

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

    /** @brief Liest Eingabepuffer in m_values und berechnet die Verteilung. */
    void compute();

    /** @brief Verarbeitet ein Aktionszeichen vom Numpad auf dem fokussierten Feld. */
    void executeKey(char action);

    /** @brief Liefert die Akzentfarbe der aktuellen Richtung. */
    uint16_t accentColor() const;

    /** @brief Formatiert eine ganze Zahl ohne Vorzeichen. */
    static void formatInt(int v, char *buf, size_t sz);

    // =========================================================================
    // Private Methoden - Zeichnen
    // =========================================================================

    /** @brief Zeichnet Titel, Ring, Mittelzahl und Anweisungstext. */
    void drawVisualization();

    /** @brief Zeichnet Ringkontur, Tick-Marker und Mittelzahl. */
    void drawRing();

    /** @brief Zeichnet einen radialen Tick-Marker als geglättete breite Linie. */
    void drawTick(int cx, int cy, double angleRad, uint16_t color);

    /** @brief Zeichnet beide Eingabefelder. */
    void drawFields(bool cursorOn);

    /** @brief Zeichnet eine einzelne Feldzeile (Symbol, Wert, Einheit). */
    void drawField(int idx, bool pressed, bool cursorOn);

    /**
     * @brief Zeichnet den mehrzeiligen Anweisungs- oder Hinweistext.
     * @param textX Linke Kante des Textbereichs (sprite-absolut)
     * @param textTop Obere Kante des Textbereichs (sprite-absolut)
     * @param textW Verfügbare Breite in Pixeln
     */
    void drawInstructionBlock(int textX, int textTop, int textW);

    // =========================================================================
    // Private Methoden - Treffertests
    // =========================================================================

    /** @brief Treffertest auf die Eingabefelder, content-relativ. */
    int hitTestField(int cx, int cy) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite      = nullptr;
    int         m_originX     = 0;
    int         m_originY     = 0;
    int         m_w           = 0;
    int         m_h           = 0;
    bool        m_layoutValid = false;

    char    m_inputBuf[NUM_FIELDS][FIELD_CAP] = {}; ///< Editierbare Eingabepuffer
    int     m_values[NUM_FIELDS]              = {}; ///< Geparste Maschenzahlen
    uint8_t m_focusedField                    = 0;  ///< Aktuell fokussiertes Feld (0..1)

    // Berechnungsergebnis
    Direction m_direction   = Direction::NONE; ///< Zunahme, Abnahme oder keine Änderung
    Status    m_status      = Status::EMPTY;   ///< Gültigkeit des Ergebnisses
    int       m_diff        = 0;               ///< |M - N|
    int       m_baseSpacing = 0;               ///< BASE = N / D (Ganzzahldivision)
    int       m_remainder   = 0;               ///< REM = N mod D

    // Layout (content-relativ)
    Rect m_fieldRects[NUM_FIELDS] = {}; ///< Feld-Rechtecke
    int  m_rightX                 = 0;  ///< X-Start der rechten Spalte
    int  m_rightW                 = 0;  ///< Breite der rechten Spalte

    WorkshopNumpad m_numpad;

    // Touch-Tracking
    int  m_pressedField = -1;
    int  m_pressedKey   = -1;
    bool m_touchWas     = false;
    bool m_touchReady   = false;
};

#endif // WORKSHOPTOOL_STITCHDISTRIBUTOR_H
