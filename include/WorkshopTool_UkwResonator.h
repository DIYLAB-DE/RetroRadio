#ifndef WORKSHOPTOOL_UKWRESONATOR_H
#define WORKSHOPTOOL_UKWRESONATOR_H

// ============================================================================
// Resonanzkreis-Rechner für UKW von 80 bis 150 MHz
// ============================================================================

#include <stddef.h>
#include <stdint.h>

#include "Button.h"
#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_UkwResonator
 * @brief Interaktiver UKW-Resonanzkreis-Rechner mit Wheeler-Spulenberechnung.
 *
 * Berechnet einen Parallelschwingkreis aus Luftspule und Kondensator für den UKW-Bereich von 80 bis 150 MHz. Drei Hauptgrößen f, L und C bilden eine FIFO-3; zwei Größen werden eingegeben, die dritte ergibt sich aus der bekannten Resonanzbedingung f = 1 / (2π · √(L · C)). Aus dem Spulendorndurchmesser D, der Spulenlänge ℓ (beides eigenständige Parameterfelder) und dem über den Footer wählbaren Drahtdurchmesser d wird zusätzlich nach der Wheeler-Formel direkt die benötigte Windungszahl N ermittelt. Anschließend wird geprüft, ob N · d ≤ ℓ erfüllbar ist (sonst Wicklungs-Überlapp).
 *
 * Bedienung:
 *   - Footer: Vier Toggle-Buttons für d (0,5 / 0,8 / 1,0 / 1,5 mm Cu)
 *   - Fünf Eingabefelder für f, L und C als FIFO-3 sowie D und ℓ als reine Parameterfelder. Das Antippen einer berechneten Zeile verschiebt den ältesten Eingabeplatz an die Berechnungsposition.
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px):
 *     - Parallelschwingkreis-Schaltplan (oben)
 *     - Spulenseitenansicht mit N diagonalen AA-Wicklungslinien sowie Bemaßungspfeilen für D links und ℓ unten
 *     - Beschriftung mit N und Steighöhe (Pitch = ℓ / N)
 *   Rechte Spalte (285 px):
 *     - Fünf Eingabefelder (f, L, C, D, ℓ)
 *     - Info-Zeile mit Z₀, Q-Faktor und Bereichs-/Überlapp-Warnung
 *     - WorkshopNumpad
 *
 * Mathematik:
 *   f  = 1 / (2π · √(L · C))
 *   L  = 1 / ((2π · f)² · C)
 *   C  = 1 / ((2π · f)² · L)
 *   Z₀ = √(L / C)
 *
 *   Wheeler-Spulenformel (metrisch, einlagig, dichte Wicklung):
 *     L[µH] = D[m]² · N² / (0,4572 · D[m] + 1,016 · ℓ[m])
 *   Da L, D und ℓ alle bekannt sind, wird daraus eine Direktauflösung in N:
 *     N = √( L[µH] · (0,4572 · D + 1,016 · ℓ) / D² )
 *
 *   Q-Faktor (Skineffekt + Medhurst-Proximity-Näherung):
 *     δ = 0,0652 / √f      (Skintiefe Cu in Metern)
 *     R_ac = ρ_Cu · N · D / (d · δ)
 *     Q_ideal = ω · L / R_ac
 *     ψ = 1 + 2 / (ℓ/D + 0,4)        (Proximity-Näherung)
 *     Q ≈ Q_ideal / ψ
 *
 *   Wheeler-Genauigkeit ist etwa 1 % für ℓ > 0,4 · D; bei UKW-Spulen mit
 *   wenigen Windungen und kleinem Durchmesser typisch erfüllt.
 *
 * Frequenz-Bereichsprüfung:
 *   Liegt f außerhalb von 80...150 MHz, wird der Akzentstreifen des f-Feldes
 *   rot eingefärbt und in der Info-Zeile erscheint ein zusätzlicher Hinweis.
 *   Die Berechnung läuft trotzdem durch; der Bereich dient nur als Hinweis.
 */
class WorkshopTool_UkwResonator : public WorkshopTool
{
public:
    WorkshopTool_UkwResonator();
    ~WorkshopTool_UkwResonator() override = default;

    WorkshopTool_UkwResonator(const WorkshopTool_UkwResonator &)            = delete;
    WorkshopTool_UkwResonator &operator=(const WorkshopTool_UkwResonator &) = delete;

    /**
     * @brief Setzt den Rechner beim Modul-Einstieg in den Standardzustand zurück.
     * @param tft Display-Objekt (für Footer-Button-Initialisierung)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Aufräumen beim Verlassen. */
    void onExit() override;

    /**
     * @brief Zeichnet Schaltplan, Spulendarstellung, Eingabefelder, Info-Zeile, Numpad.
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
    // Footer-Steuerbereich (Drahtdurchmesser-Auswahl)
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
     * @brief Zustandsdaten des Rechners (FIFO über f/L/C plus D- und ℓ-Parameter).
     *
     * Indexkonvention:
     *   0 = f (Resonanzfrequenz, SI = Hz, Puffer-Mantisse in MHz)
     *   1 = L (Induktivität,    SI = H,  Puffer-Mantisse in nH)
     *   2 = C (Kapazität,       SI = F,  Puffer-Mantisse in pF)
     *   3 = D (Dorndurchmesser, SI = m,  Puffer-Mantisse in mm)
     *   4 = ℓ (Spulenlänge,     SI = m,  Puffer-Mantisse in mm)
     *
     * D und ℓ sind reine Parameterfelder: sie nehmen am FIFO-Verfahren nicht teil und gelten grundsätzlich als Eingabe (isInput(3)/isInput(4) liefern immer true). Aus L, D und ℓ wird per Wheeler-Direktformel die Windungs-zahl N berechnet, der Drahtdurchmesser d kommt vom Footer-Toggle.
     */
    struct State
    {
        double  values[5];     ///< SI-Werte: f, L, C, D, ℓ
        char    bufs[5][24];   ///< Eingabepuffer als Mantissen in Anzeige-Skala
        uint8_t inputSlots[2]; ///< FIFO über Indizes 0..2 (f/L/C)
        uint8_t computedQty;   ///< Index der berechneten Größe (0..2)
        uint8_t focusedQty;    ///< Fokussierte Größe (0..4, empfängt Numpad-Eingabe)
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_QTY         = 5;  ///< f(0), L(1), C(2), D(3), ℓ(4); D und ℓ sind Parameter außerhalb des FIFO
    static constexpr int NUM_INPUT_SLOTS = 2;  ///< Zwei FIFO-Eingaben aus {f,L,C}, eine wird berechnet
    static constexpr int FIELD_CAP       = 24; ///< Max. Eingabepufferlänge inkl. NUL
    static constexpr int INFO_STR_CAP    = 64; ///< Puffergröße für Info-Strings

    static constexpr int NUM_D_VALUES = 4; ///< Vier wählbare Drahtdurchmesser

    // Felder-Layout (rechte Spalte)
    static constexpr int TOP_GAP  = 10; ///< Abstand zwischen Kopf- und Inhaltsbereich
    static constexpr int ROW_H    = 32; ///< Höhe einer Eingabezeile (5 Felder müssen passen)
    static constexpr int UNIT_W   = 56; ///< Breite der Einheiten-Spalte (Platz für "MHz")
    static constexpr int ACCENT_W =  4; ///< Breite des linken Farbakzent-Streifens

    // Info-Bereich
    static constexpr int INFO_H     = 24; ///< Höhe des Info-Bereichs
    static constexpr int INFO_GAP_V =  4; ///< Vertikaler Abstand zu den Feldern

    // Cursor-Blink
    static constexpr unsigned long CURSOR_PERIOD_MS = 800;
    static constexpr unsigned long CURSOR_ON_MS     = 400;

    // Das Präfix verhindert eine Namenskollision mit dem TWO_PI-Makro aus Arduino.h.
    static constexpr double UKW_TWO_PI = 6.283185307179586;

    // Frequenz-Bereichswarnung (UKW)
    static constexpr double F_MIN_HZ = 80.0e6;  ///< Untere Bereichsgrenze
    static constexpr double F_MAX_HZ = 150.0e6; ///< Obere Bereichsgrenze

    // Wheeler-Konstanten (metrisch, m und µH)
    static constexpr double WHEELER_A = 0.4572; ///< Faktor vor D aus der Umrechnung von 18 Inch in Meter
    static constexpr double WHEELER_B = 1.016;  ///< Faktor vor ℓ aus der Umrechnung von 40 Inch in Meter

    // Material-Konstanten (Kupfer)
    static constexpr double RHO_COPPER = 1.68e-8; ///< Spezifischer Widerstand Cu in Ω·m
    static constexpr double SKIN_K     = 0.0652;  ///< δ[m] = SKIN_K / √(f[Hz])

    // =========================================================================
    // Lebenszyklus
    // =========================================================================

    /** @brief Setzt alle Member auf definierte Ausgangswerte. */
    void clearAll();

    /** @brief Initialisiert die vier Footer-Toggle-Buttons. */
    void initFooterButtons(LGFX *tft);

    /** @brief Setzt Farben der Toggle-Buttons entsprechend m_dIdx. */
    void updateButtonColors();

    /**
     * @brief Befüllt m_subtitleBuf passend zu m_dIdx.
     *
     * Wird in clearAll() und nach jeder Änderung an m_dIdx aufgerufen. getSubtitle() gibt anschließend den Puffer ohne weitere Formatierung zurück.
     */
    void refreshSubtitle();

    /** @brief Initialisiert den State mit Standardwerten (f=100 MHz, C=20 pF, D=6 mm, ℓ=10 mm). */
    void initState();

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
     * @brief Parst Eingabepuffer und berechnet die Ausgabegröße + Spulendaten.
     *
     * Schritt 1: FIFO-Berechnung der fehlenden Größe (f, L oder C) Schritt 2: Z₀ = √(L/C) Schritt 3: Wheeler-Direktformel für N mit ℓ als Eingabe Schritt 4: Plausibilitätsprüfung N · d > ℓ für Wicklungsüberlappung Schritt 5: Q-Faktor mit Skineffekt und Medhurst-Proximity-Korrektur
     *
     * Jeder Schritt setzt sein eigenes Gültig-Flag (m_computeOk, m_z0Ok, m_geomOk, m_geomOverlap, m_qOk). Bei ungültigen Eingaben werden die abhängigen Folgeschritte übersprungen.
     */
    void compute();

    /**
     * @brief Liefert den Skalierungsfaktor für eine Größe.
     * @param qty 0=f, 1=L, 2=C, 3=D, 4=ℓ
     * @return Faktor mit dem die Mantisse aus dem Eingabepuffer multipliziert
     *         werden muss, um den SI-Wert zu erhalten.
     *
     * Pro Größentyp wird eine UKW-taugliche Anzeige-Einheit gewählt:
     *   f -> MHz (Faktor 1e6)
     *   L -> nH  (Faktor 1e-9)
     *   C -> pF  (Faktor 1e-12)
     *   D -> mm  (Faktor 1e-3)
     *   ℓ -> mm  (Faktor 1e-3)
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
     * @param qty Größenindex (0-4)
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße
     */
    void formatSchematicValue(int qty, char *buf, size_t sz) const;

    /**
     * @brief Prüft ob eine Größe als Eingabe-Slot aktiv ist.
     *
     * D (Index 3) und ℓ (Index 4) liefern immer true, da sie als reine Parameterfelder nicht am FIFO teilnehmen und niemals berechnet werden.
     */
    bool isInput(int qty) const;

    /**
     * @brief Prüft ob die aktuelle Frequenz im UKW-Bereich liegt.
     */
    bool isFrequencyInRange() const;

    /**
     * @brief Liefert die Akzentfarbe einer Größe und bei einer Frequenz außerhalb des Bereichs Rot.
     */
    uint16_t accentColor(int qty) const;

    /** @brief Symboltext je Größe ("f", "L", "C", "D", "l"). */
    static const char *qtySymbol(int qty);

    /** @brief Skalierte Anzeige-Einheit je Größe ("MHz", "nH", "pF", "mm", "mm"). */
    static const char *scaledUnit(int qty);

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

    /** @brief Zeichnet den kompletten Parallelschwingkreis-Schaltplan. */
    void drawSchematic();

    /**
     * @brief Zeichnet einen vertikalen Kondensator (zwei horizontale Platten).
     * @param centerX Mittelachse (schaltplanrelativ)
     * @param topY    Obere Kante der Bauteil-Box (schaltplanrelativ)
     * @param active  true wenn das C-Feld fokussiert ist
     */
    void drawCapacitorVert(int centerX, int topY, bool active);

    /**
     * @brief Zeichnet eine vertikale Spule (vier AA-Halbkreise rechts der Mittelachse).
     * @param centerX Mittelachse (schaltplanrelativ)
     * @param topY    Obere Kante der Bauteil-Box (schaltplanrelativ)
     * @param active  true wenn das L-Feld fokussiert ist
     */
    void drawInductorVert(int centerX, int topY, bool active);

    /**
     * @brief Zeichnet die Spulen-Seitenansicht mit Bemaßungspfeilen für D
     *        und ℓ sowie die Beschriftung mit N und Pitch darunter.
     */
    void drawSpool();

    /**
     * @brief Zeichnet alle Eingabefelder.
     * @param cursorOn true wenn Cursor sichtbar
     */
    void drawFields(bool cursorOn);

    /**
     * @brief Zeichnet eine einzelne Größen-Zeile.
     * @param qty Größenindex (0-4)
     * @param pressed  true für Tap-Feedback
     * @param cursorOn true wenn Cursor sichtbar
     */
    void drawField(int qty, bool pressed, bool cursorOn);

    /** @brief Zeichnet den Info-Bereich (Z₀, Q, Bereichs- und Überlapp-Warnung). */
    void drawInfoRow();

    // =========================================================================
    // Touch-Verarbeitung
    // =========================================================================

    /** @brief Treffertest auf Eingabefelder (content-relativ). */
    int hitTestField(int cx, int cy) const;

    /**
     * @brief Treffertest auf Schaltplan-Bauteile (content-relativ).
     * @return 0 = L (Spule), 1 = C (Kondensator), -1 = kein Treffer
     */
    int hitTestComponent(int cx, int cy) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite  = nullptr; ///< Aktueller Work-Sprite
    int  m_originX        = 0;       ///< Inhaltsursprung X (sprite-relativ)
    int  m_originY        = 0;       ///< Inhaltsursprung Y (sprite-relativ)
    int  m_w              = 0;       ///< Netto-Breite
    int  m_h              = 0;       ///< Netto-Höhe
    bool m_layoutValid    = false;   ///< true wenn Layout aktuell

    State m_state;        ///< Eingabezustand (Werte, Puffer, FIFO)
    int   m_dIdx = 2;     ///< Index in D_VALUES (Default: 1,0 mm)

    bool   m_computeOk   = false; ///< true wenn FIFO-Berechnung gültig
    bool   m_z0Ok        = false; ///< true wenn Z₀ gültig
    bool   m_geomOk      = false; ///< true wenn Wheeler-Berechnung von N gültig
    bool   m_geomOverlap = false; ///< true wenn N · d > ℓ (Wicklungs-Überlapp)
    bool   m_qOk         = false; ///< true wenn Q-Faktor-Berechnung gültig

    double m_z0    = 0.0; ///< Kennimpedanz Z₀ in Ohm
    double m_N     = 0.0; ///< Berechnete Windungszahl (Float, nicht gerundet)
    double m_Q     = 0.0; ///< Q-Faktor (Schätzwert mit Proximity-Korrektur)

    // Layout (content-relativ)
    Rect m_fieldRects[NUM_QTY] = {}; ///< Eingabefeld-Bereiche
    Rect m_compHits[2]         = {}; ///< 0 = L, 1 = C (Schaltplan-Bauteile)
    int  m_rightX              = 0;  ///< X-Start der rechten Spalte
    int  m_rightW              = 0;  ///< Breite der rechten Spalte
    int  m_infoY               = 0;  ///< Y-Start der Info-Zeile (content-relativ)

    WorkshopNumpad m_numpad; ///< Nummernpad-Verwaltung

    // Footer-Buttons (vier Drahtdurchmesser-Auswahl)
    Button m_btnD[NUM_D_VALUES];        ///< Toggle-Buttons für d-Auswahl
    bool   m_buttonsInitialized = false; ///< true nach initFooterButtons()

    // Touch-Tracking
    int  m_pressedField = -1;    ///< Gedrücktes Eingabefeld (-1 = keines)
    int  m_pressedComp  = -1;    ///< Gedrücktes Bauteil (0=L, 1=C, -1=keines)
    int  m_pressedKey   = -1;    ///< Gedrückte Numpad-Taste (-1 = keine)
    bool m_touchWas     = false; ///< true wenn im letzten Frame Touch aktiv
    bool m_touchReady   = false; ///< Phantom-Touch-Schutz

    // Untertitelpuffer, den refreshSubtitle() befüllt und getSubtitle() liest
    char m_subtitleBuf[64] = {}; ///< Aktueller Untertitel-Text
};

#endif // WORKSHOPTOOL_UKWRESONATOR_H
