#ifndef WORKSHOPTOOL_OHMCALCULATOR_H
#define WORKSHOPTOOL_OHMCALCULATOR_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_OhmCalculator
 * @brief Interaktiver Rechner für das Ohmsche Gesetz (U = R × I, P = U × I).
 *
 * Zeigt vier Größen (U, I, R, P) in je einer Zeile. Zwei davon sind Eingaben, die anderen beiden werden in Echtzeit berechnet. Der Nutzer kann durch Tippen auf eine berechnete Zeile die Eingabe-Kombination wechseln (FIFO-Verdrängung: der ältere Eingabe-Slot weicht dem neu gewählten Feld).
 *
 * Layout (typisch 537 × 327 px):
 *   - Vier Größen-Zeilen (ROW_H px je): U / I / R / P
 *   - Formelzeile (FORMULA_H px): verwendete Berechnungsformeln
 *   - Nummernpad (Rest): 4 × 4 Tasten-Grid
 *
 * Unterstützte Formel-Kombinationen:
 *   U, I -> R = U/I,          P = U×I
 *   U, R -> I = U/R,          P = U²/R
 *   U, P -> I = P/U,          R = U²/P
 *   I, R -> U = I×R,          P = I²×R
 *   I, P -> U = P/I,          R = P/I²
 *   R, P -> U = sqrt(P×R),    I = sqrt(P/R)
 *
 * Standardwerte beim Öffnen: U = 12 V, R = 5000 Ohm (-> I = 2,4 mA, P = 28,8 mW).
 *
 * Einheitenskalierung: Anzeigewerte werden automatisch skaliert (u/m/k/M/G), Eingabe immer in SI-Basiseinheiten (V, A, Ohm, W). Anzeige in deutscher Komma-Notation (dotToComma im Anzeigepfad).
 *
 * Cursor: blinkt in der fokussierten Zeile (400 ms an / 400 ms aus via millis()). Workshop löst einen Redraw exakt bei jedem Cursor-Zustandswechsel aus.
 *
 * Zeichnet ausschließlich in den übergebenen Work-Sprite (kein eigener Sprite).
 */
class WorkshopTool_OhmCalculator : public WorkshopTool
{
public:
    /**
     * @brief Konstruktor. Setzt alle Member und Standardwerte (U=12V, R=5000Ohm).
     */
    WorkshopTool_OhmCalculator();

    /** @brief Destruktor. Keine eigenen Ressourcen vorhanden. */
    ~WorkshopTool_OhmCalculator() override = default;

    WorkshopTool_OhmCalculator(const WorkshopTool_OhmCalculator &)            = delete;
    WorkshopTool_OhmCalculator &operator=(const WorkshopTool_OhmCalculator &) = delete;

    /**
     * @brief Setzt den Rechner beim Modul-Einstieg in den Standardzustand zurück.
     * @param tft Display-Objekt (nicht dauerhaft gespeichert)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Kein Aufräumaufwand beim Verlassen. */
    void onExit() override {}

    /**
     * @brief Zeichnet alle vier Größen-Zeilen, Formelzeile und Nummernpad.
     * @param sprite   Work-Sprite des Workshop-Moduls
     * @param originX  Linke Kante des Inhaltsbereichs (sprite-relativ)
     * @param originY  Obere Kante des Inhaltsbereichs (sprite-relativ)
     * @param w        Verfügbare Breite in Pixeln (typisch 537)
     * @param h        Verfügbare Höhe in Pixeln (typisch 327)
     *
     * Zeichnet alle vier Größen-Zeilen, die Formelzeile und das Nummernpad direkt in den übergebenen Work-Sprite, ohne eigenen Container. m_originX/m_originY werden auf originX/originY gesetzt; computeLayout() und alle Treffertests arbeiten mit diesen Koordinaten.
     */
    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events für Felder und Nummernpad-Tasten.
     * @param tx      Absoluter Display-X (bei touched=false ignoriert)
     * @param ty      Absoluter Display-Y (bei touched=false ignoriert)
     * @param touched true bei aktivem Touch
     * @return true wenn Neuzeichnung des Arbeitsbereichs erforderlich
     *
     * Verwendet Touch-up-Semantik: visuelles Feedback beim Drücken, Aktion erst beim Loslassen. Phantom-Touch-Schutz über m_touchReady.
     *
     * Feld-Tap-Logik:
     *   - Eingabe-Feld: Fokus verschiebt sich auf dieses Feld.
     *   - Berechnetes Feld: wird zur Eingabe; der ältere FIFO-Slot weicht.
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    // =========================================================================
    // Innere Typen
    // =========================================================================

    /** @brief Achsenparalleles Rechteck (content-relativ zum Inhaltsursprung). */
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

    static constexpr int NUM_QTY     = 4;  ///< Anzahl physikalischer Größen (U, I, R, P)
    static constexpr int FIELD_CAP   = 24; ///< Max. Länge des Eingabepuffers inkl. NUL
    static constexpr int FORMULA_CAP = 56; ///< Puffergröße für den Formelstring

    // Felder-Layout
    static constexpr int TOP_GAP  = 10;              ///< Abstand zwischen Titelbox-Unterkante und erster Berechnungszeile
    static constexpr int ROW_H    = 40;              ///< Höhe einer Größen-Zeile in Pixeln
    static constexpr int LABEL_W  = 36;              ///< Breite der Symbol-Spalte (U/I/R/P)
    static constexpr int UNIT_W   = 60;              ///< Breite der Einheiten-Spalte
    static constexpr int ACCENT_W =  4;              ///< Breite des linken Farbakzent-Streifens
    static constexpr int FIELDS_H = ROW_H * NUM_QTY; ///< Gesamthöhe aller Felder (160 px)

    // Formelzeile
    static constexpr int FORMULA_H      = 24; ///< Höhe der Formelzeile in Pixeln
    static constexpr int FORMULA_GAP_V  =  6; ///< Vertikaler Abstand oberhalb und unterhalb der Formelzeile in Pixeln

    // Cursor-Blinken
    static constexpr unsigned long CURSOR_PERIOD_MS = 800; ///< Gesamtdauer eines Blink-Zyklus in ms
    static constexpr unsigned long CURSOR_ON_MS     = 400; ///< Einschaltdauer innerhalb des Zyklus in ms

    // UI-Farbpalette (RGB565)
    static constexpr uint16_t C_BG           = 0x1082; ///< Arbeitsbereich-Hintergrund (= WorkshopCfg::C_WORK_BG)
    static constexpr uint16_t C_FIELD_INPUT  = 0x2945; ///< Alle nicht-fokussierten Zeilen (einheitlich)
    static constexpr uint16_t C_FIELD_FOCUS  = 0x328A; ///< Fokussierte Zeile (Petrol = WorkshopCfg::C_SELECTED)
    static constexpr uint16_t C_FIELD_PRESS  = 0x4B4D; ///< Feld beim Drücken (Tap-Feedback)
    static constexpr uint16_t C_SEP          = 0x2104; ///< Trennlinie zwischen Zeilen

    // Textfarben je Feld-Zustand (drei Stufen: fokussiert / Eingabe / berechnet)
    static constexpr uint16_t C_LBL_FOCUS   = 0xFFFF; ///< Symbol fokussiert (reines Weiß)
    static constexpr uint16_t C_LBL_INPUT   = 0xDEDB; ///< Symbol Eingabe-Feld
    static constexpr uint16_t C_LBL_COMP    = 0x7BEF; ///< Symbol berechnetes Feld
    static constexpr uint16_t C_NAME_FOCUS  = 0xB5F7; ///< Langname fokussiert
    static constexpr uint16_t C_NAME_INPUT  = 0x8410; ///< Langname Eingabe-Feld
    static constexpr uint16_t C_NAME_COMP   = 0x4228; ///< Langname berechnetes Feld
    static constexpr uint16_t C_VAL_FOCUS   = 0xFFFF; ///< Wert fokussiert (reines Weiß)
    static constexpr uint16_t C_VAL_INPUT   = 0xEF7D; ///< Wert Eingabe-Feld (helles Warmweiß)
    static constexpr uint16_t C_VAL_COMP    = 0xB5F7; ///< Wert berechnetes Feld
    static constexpr uint16_t C_UNIT_FOCUS  = 0xAD75; ///< Einheit fokussiert
    static constexpr uint16_t C_UNIT_INPUT  = 0x8410; ///< Einheit Eingabe-Feld
    static constexpr uint16_t C_UNIT_COMP   = 0x4228; ///< Einheit berechnetes Feld
    static constexpr uint16_t C_FORMULA_TXT = 0xDEDB; ///< Formeltext (gut sichtbar)

    // Farbakzente (linker Streifen, je Größe)
    static constexpr uint16_t C_ACCENT_U = 0xFD20; ///< Spannung U (Orange)
    static constexpr uint16_t C_ACCENT_I = 0x001F; ///< Strom I (Blau)
    static constexpr uint16_t C_ACCENT_R = 0x07E0; ///< Widerstand R (Grün)
    static constexpr uint16_t C_ACCENT_P = 0xF800; ///< Leistung P (Rot)

    // =========================================================================
    // Statische Datenbasis
    // =========================================================================

    /** @brief Kurzsymbole der vier Größen: "U", "I", "R", "P". */
    static const char * const QTY_SYMBOLS[NUM_QTY];

    /** @brief Langnamen der vier Größen in Klammern, lokalisierbar. */
    static const WorkshopKey QTY_NAME_KEYS[NUM_QTY];

    /**
     * @brief Basiseinheiten der vier Größen.
     *
     * Verwendet "Ohm" statt Omega-Symbol, da dieses in Embedded-Fonts nicht sicher verfügbar ist. Alle anderen Einheiten (V, A, W) sind ASCII.
     */
    static const char * const QTY_UNITS[NUM_QTY];

    // =========================================================================
    // Initialisierung und Layout
    // =========================================================================

    /**
     * @brief Setzt alle Zustands-, Layout- und Touch-Variablen auf definierte Ausgangswerte.
     *
     * Wird vom Konstruktor und von onEnter() aufgerufen. Alle char[]-Puffer werden explizit per memset genullt. Vorbelegt: U = 12 V, R = 5000 Ohm als Eingaben; I und P werden berechnet.
     */
    void clearAll();

    /**
     * @brief Berechnet Feld- und Tasten-Rechtecke für die gegebene Inhaltsgröße.
     * @param w Verfügbare Breite in Pixeln (= w aus draw())
     * @param h Verfügbare Höhe  in Pixeln (= h aus draw())
     *
     * Alle berechneten Rects sind content-relativ (Ursprung = m_originX/m_originY). Wird automatisch aufgerufen wenn sich w oder h ändert.
     */
    void computeLayout(int w, int h);

    // =========================================================================
    // Wertberechnung
    // =========================================================================

    /**
     * @brief Parst Eingabepuffer und berechnet die zwei Ausgabegrößen.
     *
     * Formel-Kombinationen:
     *   {U,I}->R,P  {U,R}->I,P  {U,P}->I,R  {I,R}->U,P  {I,P}->U,R  {R,P}->U,I
     * Setzt m_computeOk bei gültigem Ergebnis (kein NaN/Inf, kein Div-durch-0, keine negativen R- oder P-Werte).
     */
    void compute();

    /**
     * @brief Formatiert einen SI-Wert als auto-skalierten Anzeigestring.
     * @param val   SI-Wert in Basiseinheit (V, A, Ohm, W)
     * @param qty   Größenindex: 0=U, 1=I, 2=R, 3=P
     * @param buf   Ausgabepuffer
     * @param sz    Puffergröße in Bytes
     *
     * Skalierungsbeispiele: "2.4 mA", "5 kOhm", "28.8 mW". Ungültige Werte wie NaN oder Inf ergeben "---". Mikro wird als "u" ausgegeben. Die Komma-Konvertierung erfolgt erst im Anzeigepfad in drawField().
     */
    static void formatDisplayValue(double val, int qty, char *buf, size_t sz);

    /**
     * @brief Formatiert einen SI-Wert als editierbaren Dezimalstring (%.6g).
     * @param val   SI-Wert
     * @param buf   Ausgabepuffer
     * @param sz    Puffergröße in Bytes
     *
     * Wird beim Wechsel einer berechneten Zeile zur Eingabe verwendet. Behält den Dezimalpunkt bei, damit atof() den Wert weiterverarbeiten kann.
     */
    static void formatEditValue(double val, char *buf, size_t sz);

    /**
     * @brief Befüllt buf mit dem ASCII-Formelstring für das aktuelle Slot-Paar.
     * @param buf Ausgabepuffer (mindestens FORMULA_CAP Bytes)
     * @param sz  Puffergröße
     */
    void buildFormulaStr(char *buf, size_t sz) const;

    /** @brief Gibt zurück ob eine Größe als Eingabe-Slot aktiv ist. */
    bool isInput(int qty) const;

    /**
     * @brief Gibt die Akzentfarbe des linken Streifens für eine Größen-Zeile zurück.
     * @param qty 0 für U/Orange, 1 für I/Blau, 2 für R/Grün, 3 für P/Rot
     */
    static uint16_t accentColor(int qty);

    // =========================================================================
    // Tastatureingabe
    // =========================================================================

    /**
     * @brief Führt eine Tastenaktion auf dem Eingabepuffer der fokussierten Größe aus.
     * @param action '0'-'9' = Ziffer, '.' = Dezimalpunkt, 'S' = Vorzeichen togglen,
     *               'B' = Backspace, 'C' = Clear
     */
    void executeKey(char action);

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /**
     * @brief Zeichnet alle vier Größen-Zeilen (Font wird einmalig geladen).
     * @param cursorOn true wenn der Cursor gerade sichtbar sein soll (Blink-Phase)
     */
    void drawFields(bool cursorOn);

    /**
     * @brief Zeichnet eine einzelne Größen-Zeile.
     * @param qty      Größenindex (0=U, 1=I, 2=R, 3=P)
     * @param pressed  true für Tap-Feedback
     * @param cursorOn true wenn der Cursor gerade sichtbar sein soll
     *
     * Setzt voraus, dass ComfortaaBold16 bereits geladen ist. Fokussierte Zeile: hellerer Hintergrund (C_FIELD_FOCUS), reinweißer Text, permanent gezeichneter Cursor (Farbe wechselt zwischen C_VAL_FOCUS und bgColor). Alle drei Anzeigepfade (fokussiert/Eingabe/berechnet) konvertieren den Anzeigestring per dotToComma() in deutsche Komma-Notation.
     */
    void drawField(int qty, bool pressed, bool cursorOn);

    /**
     * @brief Zeichnet die Formelzeile zwischen Feldern und Nummernpad.
     *
     * Hintergrund C_BG (= Arbeitsbereich-Hintergrund), Text in C_FORMULA_TXT.
     */
    void drawFormulaRow();

    // =========================================================================
    // Treffertests
    // =========================================================================

    /**
     * @brief Treffertest auf die vier Größen-Zeilen (content-relativ).
     * @return Größenindex 0-3 oder -1 bei keinem Treffer
     */
    int hitTestField(int cx, int cy) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite   = nullptr; ///< Aktueller Work-Sprite (in draw() gesetzt)
    int  m_originX         = 0;       ///< Inhaltsursprung X (sprite-relativ, = originX aus draw())
    int  m_originY         = 0;       ///< Inhaltsursprung Y (sprite-relativ, = originY aus draw())
    int  m_w               = 0;       ///< Zuletzt verwendete Netto-Breite
    int  m_h               = 0;       ///< Zuletzt verwendete Netto-Höhe
    bool m_layoutValid     = false;   ///< true wenn Layout-Rects aktuell berechnet sind

    // Eingabe-Zustand
    uint8_t m_inputSlot[2]                 = {};    ///< Indizes der beiden Eingabe-Größen (0=U..3=P)
    uint8_t m_focusedQty                   = 0;     ///< Fokussierte Größe (empfängt Tastatureingabe)
    char    m_inputBuf[NUM_QTY][FIELD_CAP] = {};    ///< Eingabepuffer je Größe (Dezimalstring in SI-Einheit)
    double  m_values[NUM_QTY]              = {};    ///< SI-Werte aller vier Größen (V, A, Ohm, W)
    bool    m_computeOk                    = false; ///< true wenn letzte Berechnung gültig war

    // Layout-Rects (content-relativ, berechnet in computeLayout)
    Rect m_fieldRects[NUM_QTY] = {}; ///< Klick-Bereich der Größen-Zeilen

    WorkshopNumpad m_numpad; ///< Nummernpad-Verwaltung (Layout, Zeichnung, Treffertest)

    // Touch-Tracking
    int  m_pressedField = -1;    ///< Index der gedrückten Größen-Zeile (-1 = keine)
    int  m_pressedKey   = -1;    ///< Index der gedrückten Nummernpad-Taste (-1 = keine)
    bool m_touchWas     = false; ///< true wenn im letzten Frame ein Touch aktiv war
    bool m_touchReady   = false; ///< Phantom-Touch-Schutz: aktiv nach erstem touched=false-Frame
};

#endif // WORKSHOPTOOL_OHMCALCULATOR_H
