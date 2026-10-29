#ifndef WORKSHOPTOOL_OPAMP_H
#define WORKSHOPTOOL_OPAMP_H

// ============================================================================
// Interaktiver Operationsverstärker-Rechner
// ============================================================================

#include <stddef.h>
#include <stdint.h>

#include "Button.h"
#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

// ============================================================================
// BETRIEBSART
// ============================================================================

/**
 * @brief Betriebsart des Op-Amp-Rechners.
 */
enum class OpAmpMode : uint8_t
{
    INVERTING,       ///< Invertierender Verstärker
    NON_INVERTING    ///< Nicht-invertierender Verstärker
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_OpAmp
 * @brief Interaktiver Op-Amp-Rechner mit Schaltplan und Footer-Toggle.
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px): Interaktiver Schaltplan mit Op-Amp-Dreieck,
 *   R1/Rf-Netzwerk und Verdrahtung (invertierend / nicht-invertierend).
 *   Rechte Spalte (285 px): Vier Eingabefelder (FIFO-3), Info-Bereich
 *   (Gain + dB) und WorkshopNumpad.
 *
 * Die FIFO-3-Logik verwaltet drei Eingaben und einen berechneten Wert. Tippen auf die berechnete Zeile oder das zugehörige Bauteil im Schaltplan tauscht sie mit dem ältesten Eingabeslot.
 *
 * Invertierend/Nicht-invertierend per Footer-Toggle umschaltbar. Jeder Modus hat einen eigenen Satz Eingabewerte und FIFO-Zustand, die beim Umschalten erhalten bleiben.
 *
 * Formeln:
 *   Invertierend:       Vout = -Vin × Rf / R1, Verstärkung = -Rf / R1
 *   Nicht-invertierend: Vout = Vin × (1 + Rf / R1), Gain = 1 + Rf / R1
 *
 * Standardwerte: Vin = 1 V, R1 = 10 kOhm, Rf = 47 kOhm.
 */
class WorkshopTool_OpAmp : public WorkshopTool
{
public:
    WorkshopTool_OpAmp();
    ~WorkshopTool_OpAmp() override = default;

    WorkshopTool_OpAmp(const WorkshopTool_OpAmp &)            = delete;
    WorkshopTool_OpAmp &operator=(const WorkshopTool_OpAmp &) = delete;

    /**
     * @brief Setzt den Rechner beim Modul-Einstieg in den Standardzustand zurück.
     * @param tft Display-Objekt (für Footer-Button-Initialisierung)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Aufräumen beim Verlassen. */
    void onExit() override;

    /**
     * @brief Zeichnet Schaltplan, Eingabefelder, Info-Bereich und Numpad.
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
    // Footer-Steuerbereich (Invertierend/Nicht-invertierend Toggle)
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
     * @brief Zustandsdaten für einen Betriebsmodus.
     *
     * Jeder Modus hat eigene Eingabewerte, Puffer und FIFO-Zustand, die beim Moduswechsel erhalten bleiben.
     */
    struct ModeState
    {
        double  values[4];       ///< SI-Werte: Vin(V), R1(Ohm), Rf(Ohm), Vout(V)
        char    bufs[4][24];     ///< Eingabepuffer je Größe
        uint8_t inputSlots[3];   ///< FIFO-Indizes der drei Eingabegrößen von der ältesten zur neuesten
        uint8_t computedQty;     ///< Index der berechneten Größe (0-3)
        uint8_t focusedQty;      ///< Fokussierte Größe (empfängt Numpad-Eingabe)
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_QTY         = 4;  ///< Vin(0), R1(1), Rf(2), Vout(3)
    static constexpr int NUM_INPUT_SLOTS = 3;  ///< Drei Eingaben, eine berechnet
    static constexpr int FIELD_CAP       = 24; ///< Max. Eingabepufferlänge inkl. NUL
    static constexpr int INFO_STR_CAP    = 48; ///< Puffergröße für Info-Strings

    // Felder-Layout (rechte Spalte)
    static constexpr int TOP_GAP = 10;    ///< Abstand zwischen Kopf- und Inhaltsbereich
    static constexpr int ROW_H    = 38;  ///< Höhe einer Eingabezeile
    static constexpr int UNIT_W   = 52;  ///< Breite der Einheiten-Spalte
    static constexpr int ACCENT_W =  4;  ///< Breite des linken Farbakzent-Streifens

    // Info-Bereich
    static constexpr int INFO_H     = 24; ///< Höhe des Info-Bereichs
    static constexpr int INFO_GAP_V =  4; ///< Vertikaler Abstand zu den Feldern

    // =========================================================================
    // Statische Daten
    // =========================================================================

    static const char * const QTY_SYMBOLS[NUM_QTY]; ///< "Vin", "R1", "Rf", "Vout"
    static const char * const QTY_UNITS[NUM_QTY];   ///< "V", "Ohm", "Ohm", "V"

    // =========================================================================
    // Lebenszyklus
    // =========================================================================

    /** @brief Setzt alle Member auf definierte Ausgangswerte. */
    void clearAll();

    /** @brief Initialisiert die Footer-Toggle-Buttons. */
    void initFooterButtons(LGFX *tft);

    /** @brief Setzt Farben der Toggle-Buttons entsprechend m_mode. */
    void updateButtonColors();

    /**
     * @brief Befüllt m_subtitleBuf passend zu m_mode.
     *
     * Wird in clearAll() und nach jeder Änderung an m_mode aufgerufen. getSubtitle() gibt anschließend den Puffer ohne weitere Formatierung zurück.
     */
    void refreshSubtitle();

    /** @brief Initialisiert einen ModeState mit Standardwerten. */
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
     * @brief Parst die drei Eingabepuffer und berechnet die eine Ausgabegröße.
     *
     * Invertierend:
     *   Vout = -Vin × Rf / R1
     *   Vin  = -Vout × R1 / Rf
     *   R1   = -Vin × Rf / Vout
     *   Rf   = -Vout × R1 / Vin
     *
     * Nicht-invertierend:
     *   Vout = Vin × (1 + Rf / R1)
     *   Vin  = Vout / (1 + Rf / R1)
     *   R1   = Rf / (Vout / Vin - 1)
     *   Rf   = R1 × (Vout / Vin - 1)
     */
    void compute();

    /**
     * @brief Formatiert einen SI-Wert als auto-skalierten Anzeigestring.
     * @param val   SI-Wert (V oder Ohm)
     * @param qty   0=Vin, 1=R1, 2=Rf, 3=Vout
     * @param buf   Ausgabepuffer
     * @param sz    Puffergröße
     */
    static void formatDisplayValue(double val, int qty, char *buf, size_t sz);

    /**
     * @brief Formatiert einen SI-Wert als editierbaren Dezimalstring (%.6g).
     * @param val SI-Wert
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße
     */
    static void formatEditValue(double val, char *buf, size_t sz);

    /**
     * @brief Prüft ob eine Größe als Eingabe-Slot aktiv ist.
     * @param qty Größenindex (0-3)
     */
    bool isInput(int qty) const;

    /**
     * @brief Akzentfarbe des linken Streifens.
     * @param qty 0 für Vin/Orange, 1 für R1/Grün, 2 für Rf/Gelb, 3 für Vout/Cyan
     */
    static uint16_t accentColor(int qty);

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

    /** @brief Zeichnet den kompletten Schaltplan (Op-Amp, R1, Rf, Verdrahtung). */
    void drawSchematic();

    /** @brief Zeichnet die invertierende Beschaltung. */
    void drawInvertingCircuit();

    /** @brief Zeichnet die nicht-invertierende Beschaltung. */
    void drawNonInvertingCircuit();

    /**
     * @brief Zeichnet das Op-Amp-Dreieck-Symbol.
     * @param triLX  X der linken Kante
     * @param triTY  Y der Oberkante
     * @param triRX  X der rechten Spitze
     * @param triBY  Y der Unterkante
     */
    void drawOpAmpTriangle(int triLX, int triTY, int triRX, int triBY);

    /**
     * @brief Zeichnet ein Widerstands-Symbol (Rechteck).
     * @param cx    Mittelpunkt X (schaltplanrelativ)
     * @param top   Oberkante Y
     * @param h     Höhe des Symbols
     * @param horiz true für horizontalen Widerstand (cx/top = links oben)
     * @param active true wenn das zugehörige Feld fokussiert ist
     */
    void drawResistor(int cx, int top, int h, bool horiz, bool active);

    /**
     * @brief Zeichnet alle Eingabefelder.
     * @param cursorOn true wenn Cursor sichtbar
     */
    void drawFields(bool cursorOn);

    /**
     * @brief Zeichnet eine einzelne Größen-Zeile.
     * @param qty Größenindex (0-3)
     * @param pressed  true für Tap-Feedback
     * @param cursorOn true wenn Cursor sichtbar
     */
    void drawField(int qty, bool pressed, bool cursorOn);

    /** @brief Zeichnet den Info-Bereich (Gain + dB). */
    void drawInfoRow();

    // =========================================================================
    // Touch-Verarbeitung
    // =========================================================================

    /** @brief Treffertest auf Eingabefelder (content-relativ). */
    int hitTestField(int cx, int cy) const;

    /** @brief Treffertest auf Schaltplan-Bauteile (content-relativ). */
    int hitTestComponent(int cx, int cy) const;

    /**
     * @brief Formatiert einen Bauteilwert für die Schaltplan-Beschriftung.
     * @param qty Größenindex (0-3)
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße
     */
    void formatSchematicValue(int qty, char *buf, size_t sz) const;

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

    OpAmpMode m_mode = OpAmpMode::INVERTING; ///< Aktuelle Betriebsart

    // Separate Zustandsdaten je Modus
    ModeState m_inv;  ///< Invertierender Modus (Werte, Puffer, FIFO-Zustand)
    ModeState m_ni;   ///< Nicht-invertierender Modus

    // Berechnungsergebnisse (abgeleitet, immer aktuell)
    double m_gain     = 0.0;  ///< Verstärkungsfaktor (mit Vorzeichen)
    double m_gainDb   = 0.0;  ///< Verstärkung in dB
    bool   m_computeOk = false; ///< true wenn letzte Berechnung gültig

    // Layout (content-relativ)
    Rect m_fieldRects[NUM_QTY] = {};  ///< Eingabefeld-Bereiche
    Rect m_compHits[NUM_QTY]   = {};  ///< Schaltplan-Bauteil-Hit-Areas
    int  m_rightX = 0;               ///< X-Start der rechten Spalte
    int  m_rightW = 0;               ///< Breite der rechten Spalte
    int  m_infoY  = 0;               ///< Y-Start der Info-Zeile (spaltenrelativ)

    WorkshopNumpad m_numpad; ///< Nummernpad-Verwaltung

    // Footer-Buttons
    Button m_btnInvert;  ///< Toggle-Button "invertierend"
    Button m_btnNonInv;  ///< Toggle-Button "nicht-invertierend"
    bool m_buttonsInitialized = false; ///< true nach initFooterButtons()

    // Die Schaltflächen speichern nur die Zeiger auf diese Beschriftungspuffer. Deshalb müssen die Puffer während der gesamten Lebensdauer des Werkzeugs gültig bleiben.
    char m_btnInvertLabel[24] = {}; ///< Beschriftung invertierend-Button
    char m_btnNonInvLabel[24] = {}; ///< Beschriftung nicht-invertierend-Button

    // Touch-Tracking
    int  m_pressedField = -1;    ///< Gedrücktes Eingabefeld (-1 = keines)
    int  m_pressedComp  = -1;    ///< Gedrücktes Bauteil im Schaltplan (-1 = keines)
    int  m_pressedKey   = -1;    ///< Gedrückte Numpad-Taste (-1 = keine)
    bool m_touchWas     = false; ///< true wenn im letzten Frame Touch aktiv
    bool m_touchReady   = false; ///< Phantom-Touch-Schutz

    // Untertitelpuffer, den refreshSubtitle() befüllt und getSubtitle() liest
    char m_subtitleBuf[64] = {}; ///< Aktueller Untertitel-Text
};

#endif // WORKSHOPTOOL_OPAMP_H
