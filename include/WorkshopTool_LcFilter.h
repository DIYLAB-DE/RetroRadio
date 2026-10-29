#ifndef WORKSHOPTOOL_LCFILTER_H
#define WORKSHOPTOOL_LCFILTER_H

// ============================================================================
// LC-Filter-Rechner zweiter Ordnung
// ============================================================================

#include <stddef.h>
#include <stdint.h>

#include "Button.h"
#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

// ============================================================================
// FILTER-DIMENSION
// ============================================================================

/**
 * @brief Filtertyp des LC-Filters 2. Ordnung.
 *
 * Die Resonanzfrequenz f0 = 1/(2π·√(L·C)) ist für beide Typen gleich;
 * nur der Schaltplan (Reihenfolge von L und C im Signalpfad) und das Frequenzgang-Vorzeichen der Steigung drehen sich.
 */
enum class LcFilterKind : uint8_t
{
    LOW_PASS, ///< Tiefpass (L längs, C gegen GND)
    HIGH_PASS ///< Hochpass (C längs, L gegen GND)
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_LcFilter
 * @brief Interaktiver LC-Filter-Rechner 2. Ordnung mit Tiefpass/Hochpass-Toggle.
 *
 * Ein LC-Filter 2. Ordnung besteht aus einer Spule und einem Kondensator in einer festen Topologie mit einem Längs- und einem Quer-Bauteil. Durch den Wechsel der Bauteil-Reihenfolge entsteht entweder ein Tiefpass oder ein Hochpass; die Resonanzfrequenz bleibt gleich.
 *
 * Bedienung:
 *   - Footer-Toggle: Tiefpass / Hochpass
 *   - Drei Eingabefelder für L, C und fc mit zwei Eingaben und einer berechneten Größe. Das Antippen der berechneten Zeile verschiebt den ältesten Eingabeplatz an die Berechnungsposition.
 *
 * Jeder Modus (TP/HP) hat einen eigenen ModeState mit separaten Werten und FIFO-Zustand, damit der Anwender beim Umschalten nicht seine Eingaben verliert. Beide Modi starten mit denselben Standardwerten (L = 10 mH, C = 100 nF -> f0 ≈ 5,033 kHz), damit sichtbar wird, dass der Toggle nur die Schaltplan-Darstellung dreht, nicht die Eckfrequenz.
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px):
 *     - Interaktiver Schaltplan mit Längs- und Quer-Bauteil
 *   Rechte Spalte (285 px):
 *     - Drei Eingabefelder mit deutscher Dezimalkonvention
 *     - Info-Zeile mit Steigung und Kennimpedanz Z0 = √(L/C)
 *     - WorkshopNumpad
 *
 * Mathematik:
 *   f0 = 1 / (2π · √(L · C))
 *   L  = 1 / ((2π · f0)² · C)
 *   C  = 1 / ((2π · f0)² · L)
 *   Z0 = √(L / C)
 *
 * In dieser Schaltung gibt es keine Topologie-Achse; die Bauteilart je Position (Spule bzw. Kondensator) ist durch den Filtertyp eindeutig festgelegt. Entsprechend kommt der Inhaltsbereich ohne Chip-Selektor aus und der Schaltplan kann den linken Bereich großzügig ausfüllen.
 */
class WorkshopTool_LcFilter : public WorkshopTool
{
public:
    WorkshopTool_LcFilter();
    ~WorkshopTool_LcFilter() override = default;

    WorkshopTool_LcFilter(const WorkshopTool_LcFilter &)            = delete;
    WorkshopTool_LcFilter &operator=(const WorkshopTool_LcFilter &) = delete;

    /**
     * @brief Setzt den Rechner beim Modul-Einstieg in den Standardzustand zurück.
     * @param tft Display-Objekt (für Footer-Button-Initialisierung)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Aufräumen beim Verlassen. */
    void onExit() override;

    /**
     * @brief Zeichnet Schaltplan, Eingabefelder, Info-Zeile und Numpad.
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
     * @brief Touch-Event: Schaltplan-Bauteile, Eingabefelder, Numpad.
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
     *
     * Der LC-Filter verwendet ausschließlich Kondensator- und Spulensymbole.
     */
    enum class CompKind : uint8_t
    {
        CAP, ///< Kondensator (zwei parallele Platten)
        IND  ///< Spule (vier AA-Halbkreise)
    };

    /**
     * @brief Zustandsdaten eines Filtermodus (Tiefpass oder Hochpass).
     *
     * Jeder Modus hat eigene Eingabewerte, Puffer und FIFO-Zustand, die beim Moduswechsel erhalten bleiben.
     *
     * Indexkonvention:
     *   0 = L  (Induktivität, SI = Henry, Puffer-Mantisse in mH)
     *   1 = C  (Kapazität,    SI = Farad, Puffer-Mantisse in nF)
     *   2 = fc (Resonanzfreq, SI = Hz,    Puffer-Mantisse in kHz)
     */
    struct ModeState
    {
        double  values[3];     ///< SI-Werte: L, C, fc
        char    bufs[3][24];   ///< Eingabepuffer als Mantissen in Anzeige-Skala
        uint8_t inputSlots[2]; ///< FIFO-Indizes der Eingabegrößen von der ältesten zur neuesten
        uint8_t computedQty;   ///< Index der berechneten Größe (0-2)
        uint8_t focusedQty;    ///< Fokussierte Größe (empfängt Numpad-Eingabe)
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_QTY         = 3;  ///< L(0), C(1), fc(2)
    static constexpr int NUM_INPUT_SLOTS = 2;  ///< Zwei Eingaben, eine berechnet
    static constexpr int FIELD_CAP       = 24; ///< Max. Eingabepufferlänge inkl. NUL
    static constexpr int INFO_STR_CAP    = 64; ///< Puffergröße für Info-Strings

    // Felder-Layout (rechte Spalte)
    static constexpr int TOP_GAP  = 10; ///< Abstand zwischen Kopf- und Inhaltsbereich
    static constexpr int ROW_H    = 38; ///< Höhe einer Eingabezeile
    static constexpr int UNIT_W   = 52; ///< Breite der Einheiten-Spalte
    static constexpr int ACCENT_W =  4; ///< Breite des linken Farbakzent-Streifens

    // Info-Bereich
    static constexpr int INFO_H     = 24; ///< Höhe des Info-Bereichs
    static constexpr int INFO_GAP_V =  4; ///< Vertikaler Abstand zu den Feldern

    // Cursor-Blink
    static constexpr unsigned long CURSOR_PERIOD_MS = 800;
    static constexpr unsigned long CURSOR_ON_MS     = 400;

    // Das Präfix verhindert eine Namenskollision mit dem TWO_PI-Makro aus Arduino.h.
    static constexpr double LC_TWO_PI = 6.283185307179586;

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
     * @brief Befüllt m_subtitleBuf passend zu m_kind.
     *
     * Wird in clearAll() und nach jeder Änderung an m_kind aufgerufen. getSubtitle() gibt anschließend den Puffer ohne weitere Formatierung zurück.
     */
    void refreshSubtitle();

    /**
     * @brief Initialisiert einen ModeState mit Standardwerten.
     * @param ms Ziel-Zustand (wird komplett überschrieben)
     */
    void initModeState(ModeState &ms) const;

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
     * @brief Parst die zwei Eingabepuffer und berechnet die Ausgabegröße.
     *
     * Die FIFO legt fest welche Größe berechnet wird:
     *   fc berechnet: fc = 1 / (2π · √(L · C))
     *   L  berechnet: L  = 1 / ((2π · fc)² · C)
     *   C  berechnet: C  = 1 / ((2π · fc)² · L)
     *
     * Die Umschaltung zwischen Tiefpass und Hochpass beeinflusst die Resonanzfrequenz nicht;
     * sie ändert nur den Schaltplan und die Steigungs-Vorzeichen im Frequenzgang.
     */
    void compute();

    /**
     * @brief Liefert den Skalierungsfaktor für eine Größe.
     * @param qty 0=L, 1=C, 2=fc
     * @return Faktor mit dem die Mantisse aus dem Eingabepuffer multipliziert
     *         werden muss, um den SI-Wert zu erhalten.
     *
     * Pro Größentyp wird eine feste, praxistaugliche Anzeige-Einheit gewählt:
     *   L -> mH  (Faktor 1e-3)
     *   C -> nF  (Faktor 1e-9)
     *   fc -> kHz (Faktor 1e3)
     */
    static double scaleForQty(int qty);

    /**
     * @brief Formatiert einen SI-Wert als Mantisse für eine gegebene Skala.
     * @param siValue SI-Wert
     * @param scale   Skalierungsfaktor (siehe scaleForQty)
     * @param buf     Ausgabepuffer
     * @param sz      Puffergröße
     */
    static void formatMantissa(double siValue, double scale,
                                char *buf, size_t sz);

    /**
     * @brief Formatiert einen Bauteilwert für die Schaltplan-Beschriftung.
     * @param qty Größenindex (0-2)
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße
     *
     * Anders als im Eingabefeld (feste Skala) wird hier auto-skaliert mit den praktikablen SI-Präfixen, damit das Schaltplan-Label kompakt bleibt.
     */
    void formatSchematicValue(int qty, char *buf, size_t sz) const;

    /** @brief Prüft ob eine Größe als Eingabe-Slot aktiv ist. */
    bool isInput(int qty) const;

    /** @brief Akzentfarbe je Größe (L=Gelb, C=Cyan, fc=Orange). */
    static uint16_t accentColor(int qty);

    /** @brief Symboltext für Eingabefeld und Schaltplan-Label. */
    static const char *qtySymbol(int qty);

    /** @brief Skalierte Anzeige-Einheit je Größe ("mH", "nF", "kHz"). */
    static const char *scaledUnit(int qty);

    // =========================================================================
    // Topologie-Helfer (Längs/Quer-Mapping)
    // =========================================================================

    /**
     * @brief Größenindex (0=L oder 1=C) des Längs-Bauteils im Signalpfad.
     *
     * Tiefpass legt L in den Signalpfad, Hochpass legt C in den Signalpfad.
     */
    int longitudinalQty() const;

    /** @brief Größenindex (0=L oder 1=C) des Quer-Bauteils. */
    int transverseQty() const;

    /** @brief Bauteilart (CAP/IND) für eine Größe (nur qty=0 und qty=1). */
    static CompKind kindOfQty(int qty);

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

    /** @brief Zeichnet den kompletten Schaltplan inkl. Bauteile und Labels. */
    void drawSchematic();

    /**
     * @brief Zeichnet ein horizontales Bauteil (Längs-Position auf Signalpfad).
     * @param kind     Bauteilart (CAP oder IND)
     * @param leftX    Linke Kante der Bauteil-Box (schaltplanrelativ)
     * @param centerY  Mittellinie auf SIG_Y (schaltplanrelativ)
     * @param active   true wenn das zugehörige Feld fokussiert ist
     */
    void drawComponentHoriz(CompKind kind, int leftX, int centerY, bool active);

    /**
     * @brief Zeichnet ein vertikales Bauteil (Quer-Position gegen GND).
     * @param kind     Bauteilart (CAP oder IND)
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

    /** @brief Zeichnet den Info-Bereich (Steigung + Kennimpedanz Z0). */
    void drawInfoRow();

    // =========================================================================
    // Touch-Verarbeitung
    // =========================================================================

    /** @brief Treffertest auf Eingabefelder (content-relativ). */
    int hitTestField(int cx, int cy) const;

    /**
     * @brief Treffertest auf Schaltplan-Bauteile (content-relativ).
     * @return 0 = Längs-Bauteil, 1 = Quer-Bauteil, -1 = kein Treffer
     */
    int hitTestComponent(int cx, int cy) const;

    // =========================================================================
    // Zugriff auf modusspezifische Daten
    // =========================================================================

    /** @brief Referenz auf den aktiven ModeState. */
    ModeState &activeState();

    /** @brief Referenz auf den aktiven ModeState (const). */
    const ModeState &activeState() const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite  = nullptr; ///< Aktueller Work-Sprite
    int  m_originX        = 0;       ///< Inhaltsursprung X (sprite-relativ)
    int  m_originY        = 0;       ///< Inhaltsursprung Y (sprite-relativ)
    int  m_w              = 0;       ///< Netto-Breite
    int  m_h              = 0;       ///< Netto-Höhe
    bool m_layoutValid    = false;   ///< true wenn Layout aktuell

    // Aktiver Filtertyp und je Modus ein eigener Zustand
    LcFilterKind m_kind = LcFilterKind::LOW_PASS; ///< Aktiver Filtertyp (TP/HP)
    ModeState    m_tp;  ///< Tiefpass-Zustand
    ModeState    m_hp;  ///< Hochpass-Zustand

    bool m_computeOk = false; ///< true wenn letzte Berechnung gültig

    // Layout (content-relativ)
    Rect m_fieldRects[NUM_QTY] = {}; ///< Eingabefeld-Bereiche
    Rect m_compHits[2]         = {}; ///< 0=Längs, 1=Quer (Schaltplan-Bauteile)
    int  m_rightX              = 0;  ///< X-Start der rechten Spalte
    int  m_rightW              = 0;  ///< Breite der rechten Spalte
    int  m_infoY               = 0;  ///< Y-Start der Info-Zeile (content-relativ)

    WorkshopNumpad m_numpad; ///< Nummernpad-Verwaltung

    // Footer-Buttons
    Button m_btnLowPass;               ///< Toggle-Button "tiefpass"
    Button m_btnHighPass;              ///< Toggle-Button "hochpass"
    bool   m_buttonsInitialized = false; ///< true nach initFooterButtons()

    // Die Schaltflächen speichern nur die Zeiger auf diese Beschriftungspuffer. Deshalb müssen die Puffer während der gesamten Lebensdauer des Werkzeugs gültig bleiben.
    char m_btnLowPassLabel[24]  = {}; ///< Beschriftung Tiefpass-Button
    char m_btnHighPassLabel[24] = {}; ///< Beschriftung Hochpass-Button

    // Touch-Tracking
    int  m_pressedField = -1;    ///< Gedrücktes Eingabefeld (-1 = keines)
    int  m_pressedComp  = -1;    ///< Gedrücktes Bauteil (0=längs, 1=quer, -1=keines)
    int  m_pressedKey   = -1;    ///< Gedrückte Numpad-Taste (-1 = keine)
    bool m_touchWas     = false; ///< true wenn im letzten Frame Touch aktiv
    bool m_touchReady   = false; ///< Phantom-Touch-Schutz

    // Untertitelpuffer, den refreshSubtitle() befüllt und getSubtitle() liest
    char m_subtitleBuf[64] = {}; ///< Aktueller Untertitel-Text
};

#endif // WORKSHOPTOOL_LCFILTER_H
