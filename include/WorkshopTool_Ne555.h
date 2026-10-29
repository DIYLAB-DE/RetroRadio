#ifndef WORKSHOPTOOL_NE555_H
#define WORKSHOPTOOL_NE555_H

// ============================================================================
// Interaktiver NE555-Timer-Rechner
// ============================================================================

#include <stdint.h>

#include "Button.h"
#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

// ============================================================================
// FARBKONFIGURATION
// ============================================================================

/**
 * @brief Farbpalette des NE555-Rechners.
 *
 * Schaltplan-Bereich mit IC-Gehäuse, Bauteil-Symbolen und Oszillogramm. Eingabefelder und Ergebnisse rechts daneben. Alle Werte im RGB565-Format.
 */
struct Ne555Colors
{
    // Schaltplan
    uint16_t schemBg;       ///< Schaltplan-Hintergrund
    uint16_t icBody;        ///< IC-Gehäuse-Füllung
    uint16_t icOutline;     ///< IC-Gehäuse-Kontur
    uint16_t icText;        ///< IC-Beschriftung ("NE555")
    uint16_t wire;          ///< Leitungsbahnen
    uint16_t compBody;      ///< Bauteil-Symbole (Füllung)
    uint16_t compOutline;   ///< Bauteil-Symbole (Kontur)
    uint16_t valActive;     ///< Aktiver Bauteil-Wert (hervorgehoben)
    uint16_t valInactive;   ///< Inaktive Bauteil-Werte (gedimmt)
    uint16_t rail;          ///< VCC/GND-Schienenbeschriftung
    uint16_t pinLabel;      ///< IC-Pin-Beschriftung
    uint16_t junction;      ///< Knotenpunkte (Lötaugen)

    // Oszillogramm
    uint16_t oscBg;         ///< Oszillogramm-Hintergrund
    uint16_t oscAxis;       ///< Achsen und Raster
    uint16_t oscSignal;     ///< Signalkurve
    uint16_t oscText;       ///< Achsenbeschriftung

    // Ergebnisse
    uint16_t resultBg;      ///< Ergebnis-Zeilenhintergrund
    uint16_t resultLabel;   ///< Ergebnisbeschriftung für f, T oder Tastverhältnis
    uint16_t resultValue;   ///< Ergebnis-Wert

    /**
     * @brief Liefert die Standardfarbpalette.
     * @return Vorausgefüllte Ne555Colors-Instanz
     */
    static Ne555Colors defaults()
    {
        Ne555Colors c;
        c.schemBg     = 0x1082;
        c.icBody      = 0x2945;
        c.icOutline   = 0x4A69;
        c.icText      = 0xDEDB;
        c.wire        = 0x8410;
        c.compBody    = 0x2945;
        c.compOutline = 0x6B6D;
        c.valActive   = 0x07E0;
        c.valInactive = 0x7D7A;
        c.rail        = 0xAD75;
        c.pinLabel    = 0x6B6D;
        c.junction    = 0xAD75;
        c.oscBg       = 0x0841;
        c.oscAxis     = 0x4A69;
        c.oscSignal   = 0x07E0;
        c.oscText     = 0x6B6D;
        c.resultBg    = 0x18C3;
        c.resultLabel = 0x7D7A;
        c.resultValue = 0xFFF5;
        return c;
    }
};

// ============================================================================
// BETRIEBSART
// ============================================================================

/**
 * @brief Betriebsart des NE555-Timer-Rechners.
 */
enum class Ne555Mode : uint8_t
{
    ASTABLE,     ///< Astabile Kippstufe (Rechteckgenerator)
    MONOSTABLE   ///< Monostabile Kippstufe (Einschuss-Timer)
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_Ne555
 * @brief Interaktiver NE555-Timer-Rechner mit Schaltplan und Oszillogramm.
 *
 * Layout (547 × 327 px Content-Bereich):
 *   Linke Spalte (250 px): Schaltplan mit IC-Gehäuse, R1/R2/C-Symbolen
 *   und Oszillogramm des Ausgangssignals darunter.
 *   Rechte Spalte (285 px): Eingabefelder (FIFO-3 Astabil / FIFO-2
 *   Monostabil), Ergebniszeilen und WorkshopNumpad.
 *
 * Interaktion: Bauteilwerte können sowohl über die Eingabefelder als auch per Tap auf das Bauteil-Symbol im Schaltplan ausgewählt werden. Das Numpad editiert das aktive Feld; die Ergebnisse berechnen sich sofort nach jeder Eingabeänderung.
 *
 * Astabil/Monostabil per Footer-Toggle umschaltbar. Jeder Modus hat einen eigenen Satz Eingabewerte, die beim Umschalten erhalten bleiben.
 *
 * Farbgebung über Ne555Colors konfigurierbar.
 */
class WorkshopTool_Ne555 : public WorkshopTool
{
public:
    /**
     * @brief Konstruktor. Setzt alle Member und Standardwerte.
     * @param colors Farbpalette (Default: Ne555Colors::defaults())
     */
    explicit WorkshopTool_Ne555(Ne555Colors colors = Ne555Colors::defaults());

    /** @brief Destruktor. Keine eigenen Ressourcen vorhanden. */
    ~WorkshopTool_Ne555() override = default;

    WorkshopTool_Ne555(const WorkshopTool_Ne555 &)            = delete;
    WorkshopTool_Ne555 &operator=(const WorkshopTool_Ne555 &) = delete;

    /**
     * @brief Setzt den Rechner beim Modul-Einstieg in den Standardzustand zurück.
     * @param tft Display-Objekt (für Footer-Button-Initialisierung)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Aufräumen beim Verlassen. */
    void onExit() override;

    /**
     * @brief Zeichnet Schaltplan, Oszillogramm, Eingabefelder, Ergebnisse und Numpad.
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
     * @brief Touch-Event: Schaltplan-Komponenten, Eingabefelder, Numpad.
     * @param tx      Absoluter Display-X
     * @param ty      Absoluter Display-Y
     * @param touched true bei aktivem Touch
     * @return true wenn Neuzeichnung erforderlich
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    // ========================================================================
    // Footer-Steuerbereich (Astabil/Monostabil-Toggle)
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

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int FIELD_CAP   = 24;  ///< Max. Länge Eingabepuffer inkl. NUL
    static constexpr int AST_FIELDS  = 3;   ///< Astabil: R1, R2, C
    static constexpr int MONO_FIELDS = 2;   ///< Monostabil: R, C
    static constexpr int MAX_FIELDS  = 3;   ///< Maximum beider Modi
    static constexpr int AST_RESULTS = 5;   ///< f, T, Duty, t_hi, t_lo
    static constexpr int MONO_RESULTS = 1;  ///< t_pulse

    /// Globaler Top-Abstand zwischen Header und Tool-Inhalt
    static constexpr int TOP_GAP = 10;

    // =========================================================================
    // Lebenszyklus
    // =========================================================================

    /** @brief Setzt alle Member auf definierte Ausgangswerte. */
    void clearAll();

    /** @brief Initialisiert die Footer-Toggle-Buttons (Astabil/Monostabil). */
    void initFooterButtons(LGFX *tft);

    /** @brief Setzt Farben der Toggle-Buttons entsprechend m_mode. */
    void updateButtonColors();

    /**
     * @brief Befüllt m_subtitleBuf passend zu m_mode.
     *
     * Wird in clearAll() und nach jeder Änderung an m_mode aufgerufen. getSubtitle() gibt anschließend den Puffer ohne weitere Formatierung zurück.
     */
    void refreshSubtitle();

    // =========================================================================
    // Layout und Berechnung
    // =========================================================================

    /**
     * @brief Berechnet alle Layout-Rects für Felder, Hit-Areas, Numpad.
     * @param w Netto-Breite des Inhaltsbereichs
     * @param h Netto-Höhe des Inhaltsbereichs
     */
    void computeLayout(int w, int h);

    /** @brief Berechnet Ergebniswerte aus den aktuellen Eingaben. */
    void compute();

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /**
     * @brief Zeichnet den Schaltplan (Bauteile, IC, Verdrahtung).
     *
     * Lädt intern ComfortaaBold14 für Beschriftungen.
     */
    void drawSchematic();

    /** @brief Zeichnet die astabile Schaltung (R1/R2/C + IC). */
    void drawAstableCircuit();

    /** @brief Zeichnet die monostabile Schaltung (R/C + IC). */
    void drawMonostableCircuit();

    /**
     * @brief Zeichnet das IC-Gehäuse mit Pin-Labels.
     * @param icY  Oberkante des IC-Rechtecks (schaltplanrelativ)
     * @param icH  Höhe des IC-Rechtecks
     * @param pin7Y Y-Position Pin 7 DIS
     * @param pin6Y Y-Position Pin 6 THR
     * @param pin2Y Y-Position Pin 2 TRI
     * @param pin3Y Y-Position Pin 3 OUT
     */
    void drawIcBlock(int icY, int icH, int pin7Y, int pin6Y, int pin2Y, int pin3Y);

    /**
     * @brief Zeichnet ein Widerstands-Symbol (Rechteck).
     * @param cx    Mittelpunkt X (schaltplanrelativ)
     * @param top   Oberkante Y
     * @param h     Höhe des Symbols
     * @param active true wenn das zugehörige Feld aktiv ist
     */
    void drawResistor(int cx, int top, int h, bool active);

    /**
     * @brief Zeichnet ein Kondensator-Symbol (zwei Platten).
     * @param cx    Mittelpunkt X (schaltplanrelativ)
     * @param topY  Y der oberen Platte
     * @param active true wenn das zugehörige Feld aktiv ist
     */
    void drawCapacitor(int cx, int topY, bool active);

    /**
     * @brief Zeichnet das Oszillogramm des Ausgangssignals.
     *
     * Astabil: 2-3 Perioden Rechteckwelle mit korrektem Tastverhältnis. Monostabil: einzelner Ausgangspuls nach Trigger-Impuls.
     */
    void drawOscillogram();

    /**
     * @brief Zeichnet alle Eingabefelder der rechten Spalte.
     * @param cursorOn true wenn Cursor gerade sichtbar (Blink-Phase)
     *
     * Lädt intern ComfortaaBold16 für Beschriftungen.
     */
    void drawInputFields(bool cursorOn);

    /**
     * @brief Zeichnet ein einzelnes Eingabefeld.
     * @param idx Feldindex (0-2 astabil, 0-1 monostabil)
     * @param pressed  true für Tap-Feedback
     * @param cursorOn true wenn Cursor sichtbar
     *
     * Setzt voraus, dass ComfortaaBold16 bereits geladen ist.
     */
    void drawInputField(int idx, bool pressed, bool cursorOn);

    /**
     * @brief Zeichnet die Ergebniszeilen unter den Eingabefeldern.
     *
     * Lädt intern ComfortaaBold14 für kompakte Darstellung.
     */
    void drawResults();

    // =========================================================================
    // Touch- und Eingabeverarbeitung
    // =========================================================================

    /**
     * @brief Treffertest auf Eingabefelder (content-relativ).
     * @return Feldindex 0-2 beziehungsweise 0-1 oder -1
     */
    int hitTestField(int cx, int cy) const;

    /**
     * @brief Treffertest auf Schaltplan-Bauteile (content-relativ).
     * @return Feldindex 0-2 beziehungsweise 0-1 oder -1
     */
    int hitTestComponent(int cx, int cy) const;

    /**
     * @brief Verarbeitet eine Numpad-Tasten-Aktion.
     * @param action Aktionszeichen ('0'-'9', '.', 'S', 'B', 'C')
     */
    void executeKey(char action);

    // =========================================================================
    // Formatierung
    // =========================================================================

    /**
     * @brief Formatiert einen Zeitwert mit automatischer Skalierung (us/ms/s).
     * @param seconds Zeitwert in Sekunden
     * @param buf     Ausgabepuffer
     * @param sz      Puffergröße
     */
    static void formatTime(double seconds, char *buf, size_t sz);

    /**
     * @brief Formatiert eine Frequenz mit automatischer Skalierung (Hz/kHz/MHz).
     * @param hz  Frequenz in Hertz
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße
     */
    static void formatFreq(double hz, char *buf, size_t sz);

    /**
     * @brief Formatiert einen Bauteilwert für die Schaltplan-Beschriftung.
     * @param fieldIdx   Feldindex
     * @param buf        Ausgabepuffer
     * @param sz         Puffergröße
     *
     * Widerstände: "10k", "1M" etc. Kondensatoren: "0.1u", "100n" etc.
     */
    void formatSchematicValue(int fieldIdx, char *buf, size_t sz) const;

    // =========================================================================
    // Zugriff auf modusspezifische Daten
    // =========================================================================

    /** @brief Anzahl Eingabefelder im aktuellen Modus. */
    int fieldCount() const;

    /** @brief Kurzlabel des Feldes (R1/R2/C bzw. R/C). */
    const char *fieldLabel(int idx) const;

    /** @brief Einheitentext des Feldes (kOhm/uF). */
    const char *fieldUnit(int idx) const;

    /** @brief Zeiger auf den Eingabepuffer des Feldes im aktuellen Modus. */
    char *inputBuf(int idx);

    /** @brief Zeiger auf den Eingabepuffer des Feldes (const). */
    const char *inputBuf(int idx) const;

    /** @brief Zeiger auf den SI-Wert des Feldes im aktuellen Modus. */
    double &fieldValue(int idx);

    /** @brief SI-Wert des Feldes (const). */
    double fieldValue(int idx) const;

    /** @brief true wenn das Feld einen Widerstand repräsentiert. */
    bool isResistor(int idx) const;

    /** @brief Akzentfarbe des linken Farbstreifens für ein Feld. */
    uint16_t accentColor(int idx) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Ne555Colors m_colors; ///< Aktive Farbpalette

    Gfx_Sprite *m_sprite  = nullptr; ///< Aktueller Work-Sprite (in draw() gesetzt)
    int  m_originX        = 0;       ///< Inhaltsursprung X (sprite-relativ)
    int  m_originY        = 0;       ///< Inhaltsursprung Y (sprite-relativ)
    int  m_w              = 0;       ///< Netto-Breite
    int  m_h              = 0;       ///< Netto-Höhe
    bool m_layoutValid    = false;   ///< true wenn Layout aktuell

    Ne555Mode m_mode = Ne555Mode::ASTABLE; ///< Aktuelle Betriebsart
    int m_activeField = 0;                 ///< Aktives Eingabefeld (empfängt Numpad-Eingabe)

    // Astabile Werte mit Erhaltung beim Moduswechsel
    char   m_astBuf[AST_FIELDS][FIELD_CAP] = {}; ///< Eingabepuffer R1/R2/C
    double m_astVal[AST_FIELDS]            = {}; ///< Werte in kOhm/kOhm/uF

    // Monostabile Werte
    char   m_monoBuf[MONO_FIELDS][FIELD_CAP] = {}; ///< Eingabepuffer R/C
    double m_monoVal[MONO_FIELDS]            = {}; ///< Werte in kOhm/uF

    // Berechnungsergebnisse
    double m_frequency = 0.0; ///< Frequenz in Hz (Astabil)
    double m_period    = 0.0; ///< Periodendauer in Sekunden (Astabil)
    double m_dutyCycle = 0.0; ///< Tastverhältnis in Prozent (Astabil)
    double m_tHigh     = 0.0; ///< High-Zeit in Sekunden (Astabil)
    double m_tLow      = 0.0; ///< Low-Zeit in Sekunden (Astabil)
    double m_tPulse    = 0.0; ///< Pulsbreite in Sekunden (Monostabil)
    bool   m_computeOk = false; ///< true wenn letzte Berechnung gültig

    // Layout-Rechtecke relativ zum Inhaltsbereich
    Rect m_fieldRects[MAX_FIELDS] = {}; ///< Eingabefeld-Bereiche
    Rect m_compHits[MAX_FIELDS]   = {}; ///< Schaltplan-Bauteil-Hit-Areas
    int  m_rightX = 0;                 ///< X-Start der rechten Spalte (content-relativ)
    int  m_rightW = 0;                 ///< Breite der rechten Spalte
    int  m_schemH = 0;                 ///< Schaltplan-Höhe (ohne Oszillogramm)
    int  m_oscillY = 0;                ///< Y-Start des Oszillogramms (content-relativ)
    int  m_oscillH = 0;                ///< Höhe des Oszillogramms
    int  m_resultsY = 0;               ///< Y-Start der Ergebniszeilen (spaltenrelativ)

    // Nummernpad
    WorkshopNumpad m_numpad; ///< Nummernpad-Verwaltung

    // Fußzeilen-Buttons
    Button m_btnAstable;  ///< Toggle-Button "Astabil"
    Button m_btnMono;     ///< Toggle-Button "Monostabil"
    bool m_buttonsInitialized = false; ///< true nach initFooterButtons()

    // Die Schaltflächen speichern nur die Zeiger auf diese Beschriftungspuffer. Deshalb müssen die Puffer während der gesamten Lebensdauer des Werkzeugs gültig bleiben.
    char m_btnAstableLabel[24]    = {}; ///< Beschriftung Astabil-Button
    char m_btnMonostableLabel[24] = {}; ///< Beschriftung Monostabil-Button

    // Touch-Verfolgung
    int  m_pressedField = -1;    ///< Gedrücktes Eingabefeld (-1 = keines)
    int  m_pressedComp  = -1;    ///< Gedrücktes Bauteil im Schaltplan (-1 = keines)
    int  m_pressedKey   = -1;    ///< Gedrückte Numpad-Taste (-1 = keine)
    bool m_touchWas     = false; ///< true wenn im letzten Frame ein Touch aktiv war
    bool m_touchReady   = false; ///< Phantom-Touch-Schutz

    // Untertitelpuffer; refreshSubtitle befüllt ihn und getSubtitle liest ihn
    char m_subtitleBuf[64] = {}; ///< Aktueller Untertitel-Text
};

#endif // WORKSHOPTOOL_NE555_H
