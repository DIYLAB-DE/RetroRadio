#ifndef WORKSHOPTOOL_RCFILTER_H
#define WORKSHOPTOOL_RCFILTER_H

// ============================================================================
// Universeller RC/RL-Filter-Rechner erster Ordnung
// ============================================================================

#include <stddef.h>
#include <stdint.h>

#include "Button.h"
#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

// ============================================================================
// FILTER-DIMENSIONEN
// ============================================================================

/**
 * @brief Beschreibt den Filtertyp Tiefpass oder Hochpass.
 *
 * Bestimmt zusammen mit FilterTopology die Schaltungstopologie und den Verlauf des Übertragungsgangs. Die Mathematik der Eckfrequenz fc ist für beide Typen gleich; nur der Frequenzgang dreht sich.
 */
enum class FilterKind : uint8_t
{
    LOW_PASS,  ///< Tiefpass (lässt tiefe Frequenzen durch)
    HIGH_PASS  ///< Hochpass (lässt hohe Frequenzen durch)
};

/**
 * @brief Beschreibt die Filtertopologie aus Widerstand und Kondensator oder Widerstand und Spule.
 *
 * Bestimmt das frequenzbestimmende Bauteil der Schaltung. Die formale Berechnung ist je Topologie unterschiedlich (RC: 1/(2πRC), RL: R/(2πL)), der grundsätzliche Aufbau aus Längs- und Querbauteil bleibt jedoch identisch.
 */
enum class FilterTopology : uint8_t
{
    RC,  ///< Widerstand + Kondensator
    RL   ///< Widerstand + Spule
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_RcFilter
 * @brief Interaktiver RC/RL-Filter-Rechner für vier Filtervarianten 1. Ordnung.
 *
 * Bedienung über zwei orthogonale Achsen:
 *   - Footer-Toggle: Tiefpass / Hochpass
 *   - Chip-Selektor im Inhaltsbereich: RC / RL
 *
 * Daraus ergeben sich vier Kombinationen, jede mit eigenem Wertesatz und eigenem FIFO-Zustand. Beim Umschalten bleiben die Werte je Kombination erhalten (sticky pro Filterart und Topologie).
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px):
 *     - Chip-Selektor RC/RL (oben, über dem Schaltplan)
 *     - Interaktiver Schaltplan mit Längs- und Querbauteil
 *   Rechte Spalte (285 px):
 *     - Drei Eingabefelder für R, C/L und fc mit zwei Eingaben per Nummernpad und einer berechneten Größe. Das Antippen der berechneten Zeile verschiebt die FIFO-Belegung.
 *     - Info-Zeile mit Steigung und Phase bei fc
 *     - WorkshopNumpad
 *
 * Schaltungstopologien:
 *   Tiefpass RC:  R längs, C quer (gegen GND)
 *   Tiefpass RL:  L längs, R quer (gegen GND)
 *   Hochpass RC:  C längs, R quer (gegen GND)
 *   Hochpass RL:  R längs, L quer (gegen GND)
 *
 * Mathematik:
 *   RC-Filter:  fc = 1 / (2π · R · C)
 *   RL-Filter:  fc = R / (2π · L)
 *   Tiefpass und Hochpass haben die gleiche Eckfrequenz; nur Schaltplan
 *   und Frequenzgang drehen sich.
 *
 * Standardwerte (alle vier Kombinationen):
 *   R = 1 kOhm, C = 100 nF / L = 100 mH -> fc ≈ 1,592 kHz
 *   Die identische fc zeigt didaktisch die Symmetrie der vier Varianten.
 */
class WorkshopTool_RcFilter : public WorkshopTool
{
public:
    WorkshopTool_RcFilter();
    ~WorkshopTool_RcFilter() override = default;

    WorkshopTool_RcFilter(const WorkshopTool_RcFilter &)            = delete;
    WorkshopTool_RcFilter &operator=(const WorkshopTool_RcFilter &) = delete;

    /**
     * @brief Setzt den Rechner beim Modul-Einstieg in den Standardzustand zurück.
     * @param tft Display-Objekt (für Footer-Button-Initialisierung)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Aufräumen beim Verlassen. */
    void onExit() override;

    /**
     * @brief Zeichnet Chip-Selektor, Schaltplan, Eingabefelder, Info und Numpad.
     * @param sprite   Work-Sprite des Workshop-Moduls
     * @param originX  Linke Kante des Inhaltsbereichs (sprite-relativ)
     * @param originY  Obere Kante des Inhaltsbereichs (sprite-relativ)
     * @param w        Verfügbare Breite in Pixeln (typisch 547)
     * @param h        Verfügbare Höhe in Pixeln (typisch 327)
     */
    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    /**
     * @brief Touch-Event: Chips, Schaltplan-Bauteile, Eingabefelder, Numpad.
     * @param tx      Absoluter Display-X
     * @param ty      Absoluter Display-Y
     * @param touched true bei aktivem Touch
     * @return true wenn Neuzeichnung erforderlich
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    // ========================================================================
    // Footer-Steuerbereich (Tiefpass/Hochpass-Toggle)
    // ========================================================================

    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;
    const char *getSubtitle() const override;

private:
    // =========================================================================
    // Innere Typen
    // =========================================================================

    /** @brief Achsenparalleles Rechteck (content-relativ). */
    struct Rect
    {
        int x; ///< Linke Kante
        int y; ///< Obere Kante
        int w; ///< Breite
        int h; ///< Höhe
    };

    /**
     * @brief Bauteil-Symbolart für die Schaltplan-Zeichenroutinen.
     */
    enum class CompKind : uint8_t
    {
        RES, ///< Widerstand (Rechteck)
        CAP, ///< Kondensator (zwei parallele Platten)
        IND  ///< Spule (vier Halbkreise)
    };

    /**
     * @brief Zustandsdaten einer Filterkombination (4 Stück insgesamt).
     *
     * Jede der vier Kombinationen Tief/Hoch × RC/RL hat eigene Werte, Puffer und FIFO-Zustand. Beim Umschalten zwischen Kombinationen bleiben die Eingaben des bisherigen Modus erhalten.
     *
     * Indexkonvention:
     *   0 = R   (Widerstand, Ohm)
     *   1 = C oder L (Kondensator/Spule, F oder H abhängig von der Topologie)
     *   2 = fc  (Eckfrequenz, Hz)
     */
    struct FilterState
    {
        double  values[3];      ///< SI-Werte: R, C/L, fc
        char    bufs[3][24];    ///< Eingabepuffer je Größe
        uint8_t inputSlots[2];  ///< FIFO-Indizes der Eingabegrößen von der ältesten zur neuesten
        uint8_t computedQty;    ///< Index der berechneten Größe (0-2)
        uint8_t focusedQty;     ///< Fokussierte Größe (empfängt Numpad-Eingabe)
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_QTY         = 3;  ///< R(0), C/L(1), fc(2)
    static constexpr int NUM_INPUT_SLOTS = 2;  ///< Zwei Eingaben, eine berechnet
    static constexpr int FIELD_CAP       = 24; ///< Max. Eingabepufferlänge inkl. NUL

    // Felder-Layout (rechte Spalte)
    static constexpr int TOP_GAP   = 10; ///< Abstand zwischen Kopf- und Inhaltsbereich
    static constexpr int ROW_H     = 38; ///< Höhe einer Eingabezeile
    static constexpr int UNIT_W    = 52; ///< Breite der Einheiten-Spalte
    static constexpr int ACCENT_W  =  4; ///< Breite des linken Farbakzent-Streifens

    // Info-Bereich
    static constexpr int INFO_H     = 24; ///< Höhe des Info-Bereichs
    static constexpr int INFO_GAP_V =  4; ///< Vertikaler Abstand zu den Feldern

    // Chip-Selektor (RC / RL) über dem Schaltplan
    static constexpr int CHIP_H        = 28; ///< Chip-Höhe
    static constexpr int CHIP_GAP      =  6; ///< Abstand zwischen den zwei Chips
    static constexpr int CHIP_R        =  6; ///< Eckenradius
    static constexpr int CHIP_TOP      = 10; ///< Abstand zwischen Kopfbereich und Chips
    static constexpr int CHIP_BOTTOM   = 10; ///< Abstand zwischen Chips und Schaltplan
    static constexpr int SEL_ROW_H     = CHIP_TOP + CHIP_H + CHIP_BOTTOM; ///< 48 px

    // Cursor-Blink
    static constexpr unsigned long CURSOR_PERIOD_MS = 800;
    static constexpr unsigned long CURSOR_ON_MS     = 400;

    // Mathematik
    static constexpr double RC_TWO_PI = 6.283185307179586;

    // =========================================================================
    // Lebenszyklus
    // =========================================================================

    /** @brief Setzt alle Member auf definierte Ausgangswerte. */
    void clearAll();

    /** @brief Initialisiert die Footer-Toggle-Buttons. */
    void initFooterButtons(LGFX *tft);

    /** @brief Setzt Farben der Toggle-Buttons entsprechend m_kind. */
    void updateButtonColors();

    /**
     * @brief Befüllt m_subtitleBuf passend zu m_kind und m_topology.
     *
     * Wird in clearAll() und nach jeder Zustandsänderung an m_kind oder m_topology aufgerufen. getSubtitle() gibt anschließend den Puffer ohne weitere Formatierung zurück.
     */
    void refreshSubtitle();

    /**
     * @brief Initialisiert einen FilterState mit Standardwerten.
     * @param fs        Ziel-Zustand
     * @param topology  Topologie (entscheidet ob C oder L als zweite Größe)
     */
    void initFilterState(FilterState &fs, FilterTopology topology) const;

    // =========================================================================
    // Layout
    // =========================================================================

    /**
     * @brief Berechnet Spaltenaufteilung, Feld-Rects, Hit-Areas, Numpad.
     * @param w Netto-Breite
     * @param h Netto-Höhe
     */
    void computeLayout(int w, int h);

    // =========================================================================
    // Berechnung
    // =========================================================================

    /**
     * @brief Parst die zwei Eingabepuffer und berechnet die eine Ausgabegröße.
     *
     * RC-Modus, FIFO über {R, C, fc}:
     *   fc = 1 / (2π · R · C)
     *   R  = 1 / (2π · C · fc)
     *   C  = 1 / (2π · R · fc)
     *
     * RL-Modus, FIFO über {R, L, fc}:
     *   fc = R / (2π · L)
     *   R  = 2π · L · fc
     *   L  = R / (2π · fc)
     *
     * Die Umschaltung zwischen Tiefpass und Hochpass beeinflusst die Eckfrequenz nicht;
     * nur Schaltplan und Frequenzgang drehen sich.
     */
    void compute();

    /**
     * @brief Liefert den Skalierungsfaktor für eine Größe.
     * @param qty 0=R, 1=C/L, 2=fc
     * @return Faktor mit dem die Mantisse aus dem Eingabepuffer multipliziert
     *         werden muss, um den SI-Wert zu erhalten.
     *
     * Pro Größentyp wird eine feste, praxistaugliche Anzeige-Einheit gewählt, damit die Eingabepuffer immer in natürlich lesbaren Mantissen arbeiten:
     *   R -> Ohm   (Faktor 1.0)
     *   fc -> Hz    (Faktor 1.0)
     *   C(RC) -> nF    (Faktor 1e-9)
     *   L(RL) -> mH    (Faktor 1e-3)
     */
    double scaleForQty(int qty) const;

    /**
     * @brief Formatiert einen SI-Wert als Mantisse für eine gegebene Skala.
     * @param siValue SI-Wert
     * @param scale   Skalierungsfaktor (siehe scaleForQty)
     * @param buf     Ausgabepuffer
     * @param sz      Puffergröße
     *
     * Das Ergebnis ist die Zahl `siValue / scale` formatiert mit %.6g. Es wird kein Dezimalkomma erzeugt; der Aufrufer ist dafür zuständig. (im Anzeigepfad via dotToComma, im Eingabepuffer bleibt der Punkt stehen damit atof korrekt parst).
     */
    static void formatMantissa(double siValue, double scale,
                                char *buf, size_t sz);

    /**
     * @brief Formatiert einen Bauteilwert für die Schaltplan-Beschriftung.
     * @param qty Größenindex (0-2)
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße
     */
    void formatSchematicValue(int qty, char *buf, size_t sz) const;

    /** @brief Prüft ob eine Größe als Eingabe-Slot aktiv ist. */
    bool isInput(int qty) const;

    /** @brief Akzentfarbe je Größe (topologieabhängig für Index 1). */
    uint16_t accentColor(int qty) const;

    /** @brief Symboltext für Eingabefeld und Schaltplan-Label. */
    const char *qtySymbol(int qty) const;

    /**
     * @brief Skalierte Anzeige-Einheit je Größe.
     * @return Statischer String, z.B. "Ohm", "nF", "mH", "Hz".
     *
     * Im Gegensatz zu einem reinen SI-Einheit-Helfer enthält dieser String bereits den passenden Präfix gemäß scaleForQty(), damit der Anwender die im Eingabefeld stehende Mantisse direkt mit der nebenstehenden Einheit korrekt interpretieren kann.
     */
    const char *scaledUnit(int qty) const;

    // =========================================================================
    // Topologie-Helfer (Längs/Quer-Mapping)
    // =========================================================================

    /** @brief true wenn der Widerstand R die Längsposition besetzt. */
    bool rIsLongitudinal() const;

    /** @brief Größenindex (0 oder 1) des Längs-Bauteils. */
    int longitudinalQty() const;

    /** @brief Größenindex (0 oder 1) des Quer-Bauteils. */
    int transverseQty() const;

    /** @brief Bauteilart (RES/CAP/IND) für eine Größe. */
    CompKind kindOfQty(int qty) const;

    // =========================================================================
    // Tastatureingabe
    // =========================================================================

    /**
     * @brief Führt eine Tastenaktion auf dem aktiven Eingabepuffer aus.
     * @param action '0'-'9', '.', 'S', 'B', 'C'
     */
    void executeKey(char action);

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /** @brief Zeichnet den Chip-Selektor RC/RL über dem Schaltplan. */
    void drawChipSelector();

    /** @brief Zeichnet den kompletten Schaltplan inkl. Bauteile und Labels. */
    void drawSchematic();

    /**
     * @brief Zeichnet ein horizontales Bauteil (Längs-Position auf Signalpfad).
     * @param kind     Bauteilart (RES/CAP/IND)
     * @param leftX    Linke Kante der Bauteil-Box (schaltplanrelativ)
     * @param centerY  Mittellinie auf SIG_Y (schaltplanrelativ)
     * @param active   true wenn das zugehörige Feld fokussiert ist
     */
    void drawComponentHoriz(CompKind kind, int leftX, int centerY, bool active);

    /**
     * @brief Zeichnet ein vertikales Bauteil (Quer-Position gegen GND).
     * @param kind     Bauteilart (RES/CAP/IND)
     * @param centerX  Mittelachse (schaltplanrelativ)
     * @param topY     Obere Kante der Bauteil-Box (schaltplanrelativ)
     * @param active   true wenn das zugehörige Feld fokussiert ist
     */
    void drawComponentVert(CompKind kind, int centerX, int topY, bool active);

    /**
     * @brief Zeichnet alle Eingabefelder.
     * @param cursorOn true wenn Cursor sichtbar
     */
    void drawFields(bool cursorOn);

    /**
     * @brief Zeichnet eine einzelne Größen-Zeile.
     * @param qty Größenindex (0-2)
     * @param pressed  true für Tap-Feedback
     * @param cursorOn true wenn Cursor sichtbar
     */
    void drawField(int qty, bool pressed, bool cursorOn);

    /** @brief Zeichnet den Info-Bereich (Steigung + Phase bei fc). */
    void drawInfoRow();

    // =========================================================================
    // Touch-Verarbeitung
    // =========================================================================

    /** @brief Treffertest auf Eingabefelder (content-relativ). */
    int hitTestField(int cx, int cy) const;

    /**
     * @brief Treffertest auf Schaltplan-Bauteile (content-relativ).
     * @return 0 = Längs-Bauteil, 1 = Quer-Bauteil, -1 = kein Treffer
     *
     * Der zurückgegebene Index entspricht der Position im m_compHits[]-Array, nicht dem Größenindex. Das Mapping auf den Größenindex erfolgt über longitudinalQty() / transverseQty().
     */
    int hitTestComponent(int cx, int cy) const;

    /** @brief Treffertest auf Chip-Selektor (content-relativ). */
    int hitTestChip(int cx, int cy) const;

    // =========================================================================
    // Zugriff auf modusspezifische Daten
    // =========================================================================

    /** @brief Referenz auf den aktiven FilterState. */
    FilterState &activeState();

    /** @brief Referenz auf den aktiven FilterState (const). */
    const FilterState &activeState() const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite  = nullptr; ///< Aktueller Work-Sprite
    int  m_originX        = 0;       ///< Inhaltsursprung X (sprite-relativ)
    int  m_originY        = 0;       ///< Inhaltsursprung Y (sprite-relativ)
    int  m_w              = 0;       ///< Netto-Breite
    int  m_h              = 0;       ///< Netto-Höhe
    bool m_layoutValid    = false;   ///< true wenn Layout aktuell

    // Aktive Filterart und Topologie
    FilterKind     m_kind     = FilterKind::LOW_PASS;
    FilterTopology m_topology = FilterTopology::RC;

    // Vier separate Zustände, indiziert per [kind][topology]
    FilterState m_states[2][2];

    bool m_computeOk = false; ///< true wenn letzte Berechnung gültig

    // Layout (content-relativ)
    Rect m_fieldRects[NUM_QTY] = {};  ///< Eingabefeld-Bereiche
    Rect m_compHits[2]         = {};  ///< 0=Längs, 1=Quer (Schaltplan-Bauteile)
    Rect m_chipRects[2]        = {};  ///< 0=RC, 1=RL (Chip-Selektor)
    int  m_rightX = 0;                ///< X-Start der rechten Spalte
    int  m_rightW = 0;                ///< Breite der rechten Spalte
    int  m_infoY  = 0;                ///< Y-Start der Info-Zeile (content-relativ)

    WorkshopNumpad m_numpad; ///< Nummernpad-Verwaltung

    // Footer-Buttons
    Button m_btnLowPass;   ///< Toggle-Button "tiefpass"
    Button m_btnHighPass;  ///< Toggle-Button "hochpass"
    bool m_buttonsInitialized = false; ///< true nach initFooterButtons()

    // Die Schaltflächen speichern nur die Zeiger auf diese Beschriftungspuffer. Deshalb müssen die Puffer während der gesamten Lebensdauer des Werkzeugs gültig bleiben.
    char m_btnLowPassLabel[24]  = {}; ///< Beschriftung Tiefpass-Button
    char m_btnHighPassLabel[24] = {}; ///< Beschriftung Hochpass-Button

    // Touch-Tracking
    int  m_pressedField = -1;    ///< Gedrücktes Eingabefeld (-1 = keines)
    int  m_pressedComp  = -1;    ///< Gedrücktes Bauteil (0=längs, 1=quer, -1=keines)
    int  m_pressedChip  = -1;    ///< Gedrückter Chip (0=RC, 1=RL, -1=keiner)
    int  m_pressedKey   = -1;    ///< Gedrückte Numpad-Taste (-1 = keine)
    bool m_touchWas     = false; ///< true wenn im letzten Frame Touch aktiv
    bool m_touchReady   = false; ///< Phantom-Touch-Schutz

    // Untertitelpuffer, den refreshSubtitle() befüllt und getSubtitle() liest
    char m_subtitleBuf[64] = {}; ///< Aktueller Untertitel-Text
};

#endif // WORKSHOPTOOL_RCFILTER_H
