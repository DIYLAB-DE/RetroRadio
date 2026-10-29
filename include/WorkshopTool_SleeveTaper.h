#ifndef WORKSHOPTOOL_SLEEVETAPER_H
#define WORKSHOPTOOL_SLEEVETAPER_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_SleeveTaper
 * @brief Verteilt eine Maschenzunahme oder -abnahme gleichmäßig über die Reihen einer Ärmelschräge.
 *
 * Eingaben sind drei ganzzahlige Werte: Anfangsmaschen N (z.B. Bündchen), Endmaschen M (z.B. an der Achsel) und die Reihenanzahl R, über die der Übergang gestrickt wird. Die absolute Differenz D = |M - N| wird auf Y = D/2 beidseitige Anpassungsreihen verteilt, jede davon ergänzt oder entfernt eine Masche an jedem Reihenende. Der Reihen-Abstand zwischen den Anpassungsreihen ergibt sich nach BASE = R / Y mit Rest REM = R mod Y. Bei REM = 0 fallen alle Y Abstände gleich aus, sonst zerfällt die Folge in REM Abstände der Länge BASE+1 und (Y-REM) Abstände der Länge BASE.
 *
 * Layout (547 x 327 px Netto-Inhaltsbereich):
 *   Linke Spalte (250 px): Visualisierung als Trapez
 *     Titelzeile mit Richtung, Trapez-Umriss in Pyramidenform, Differenzzahl
 *     im Zentrum, Tick-Paare links und rechts an den Anpassungsreihen-Höhen,
 *     darunter mehrzeiliger Anweisungstext.
 *   Rechte Spalte (285 px):
 *     TOP_GAP                 10 px
 *     3 x ROW_H (Eingabe)     93 px (3 * 31)
 *     Restabstand            104 px
 *     Numpad (REQUIRED_H)    120 px (bottom-anchored)
 *     Summe                  327 px
 *
 * Maschenzahlen und Reihenanzahl sind ganzzahlig. Die Numpad-Tasten '.' und 'S' werden in diesem Werkzeug ignoriert; nur Ziffern, Backspace und Clear verändern den Eingabepuffer.
 *
 * Standardwerte: Von = 50, Auf = 86, Reihen = 80 (klassischer Ärmel von Bündchen zu Achsel, ergibt 18 Anpassungsreihen mit gemischten Abständen).
 *
 * Randfälle:
 *   N == 0 oder M == 0 oder R == 0  -> Eingabe unvollständig, Hinweistext
 *   N == M                          -> "Keine Schräge", gerade gestrickt
 *   D ungerade                      -> Warnung, Werte um 1 anpassen
 *   Y > R                           -> Zu viele Anpassungen für die Reihen
 */
class WorkshopTool_SleeveTaper : public WorkshopTool
{
public:
    WorkshopTool_SleeveTaper();
    ~WorkshopTool_SleeveTaper() override = default;

    WorkshopTool_SleeveTaper(const WorkshopTool_SleeveTaper &)            = delete;
    WorkshopTool_SleeveTaper &operator=(const WorkshopTool_SleeveTaper &) = delete;

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
        NONE     = 0, ///< Anfang und Ende sind gleich oder Eingabe leer
        INCREASE = 1, ///< Ende > Anfang
        DECREASE = 2  ///< Ende < Anfang
    };

    /** @brief Status der Verteilungsberechnung. */
    enum class Status : uint8_t
    {
        EMPTY      = 0, ///< Mindestens ein Eingabewert fehlt oder ist null
        NO_CHANGE  = 1, ///< Anfang == Ende, keine Schräge erforderlich
        OK         = 2, ///< Verteilung gültig berechnet
        ODD        = 3, ///< Differenz ist ungerade, beidseitige Anpassung passt nicht auf
        INFEASIBLE = 4  ///< Y > R, mehr als eine Anpassung pro Reihe wäre nötig
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_FIELDS = 3;  ///< 0 = Von, 1 = Auf, 2 = Über (Reihen)
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
    static constexpr int MAX_STITCH_COUNT = 9999; ///< Obergrenze für Maschenfelder
    static constexpr int MAX_ROW_COUNT    = 9999; ///< Obergrenze für das Reihen-Feld

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
    static constexpr uint16_t C_TRAP_OUTLINE = 0x4A69; ///< Umrisslinie des Trapezes
    static constexpr uint16_t C_TITLE_TEXT   = 0xDEDB; ///< Titelzeile über dem Trapez
    static constexpr uint16_t C_INSTR_TEXT   = 0xDEDB; ///< Anweisungstext unter dem Trapez
    static constexpr uint16_t C_HINT_TEXT    = 0xAD75; ///< Hinweistext bei Sonderzuständen

    // Layout der linken Spalte
    static constexpr int   VIS_TITLE_TOP_Y      = 12;   ///< Y-Oberkante der Titelzeile
    static constexpr int   VIS_TRAP_TOP_Y       = 50;   ///< Y-Position der Trapez-Oberkante
    static constexpr int   VIS_TRAP_BOTTOM_Y    = 185;  ///< Y-Position der Trapez-Unterkante
    static constexpr int   VIS_TRAP_HALF_NARROW = 30;   ///< Halbe Breite der schmalen Trapez-Seite
    static constexpr int   VIS_TRAP_HALF_WIDE   = 70;   ///< Halbe Breite der breiten Trapez-Seite
    static constexpr float VIS_TRAP_OUTLINE_W   = 1.5f; ///< Strichstärke der Trapez-Umrisslinie
    static constexpr int   VIS_TICK_GAP         = 3;    ///< Abstand vom Trapez-Rand zum Tick-Marker
    static constexpr int   VIS_TICK_LENGTH      = 8;    ///< Länge eines Tick-Markers
    static constexpr float VIS_TICK_WIDTH       = 2.0f; ///< Strichstärke eines Tick-Markers
    static constexpr int   VIS_INSTR_TOP_Y      = 205;  ///< Y-Start des Anweisungstextblocks
    static constexpr int   VIS_INSTR_LINE_H     = 20;   ///< Zeilenhöhe im Anweisungsblock

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

    /** @brief Liefert die halbe Trapez-Breite an der Unterkante je nach Richtung. */
    int trapHalfWidthBottom() const;

    /** @brief Liefert die halbe Trapez-Breite an der Oberkante je nach Richtung. */
    int trapHalfWidthTop() const;

    /** @brief Formatiert eine ganze Zahl ohne Vorzeichen. */
    static void formatInt(int v, char *buf, size_t sz);

    // =========================================================================
    // Private Methoden - Zeichnen
    // =========================================================================

    /** @brief Zeichnet Titel, Trapez, Mittelzahl, Tick-Paare und Anweisungstext. */
    void drawVisualization();

    /** @brief Zeichnet die vier Kanten des Trapezes als breite Linien. */
    void drawTrapezoid();

    /** @brief Zeichnet ein Tick-Paar links und rechts der Trapez-Kante auf einer Y-Höhe. */
    void drawTickPair(int yPosAbsolute, uint16_t color);

    /** @brief Zeichnet alle Eingabefelder. */
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
    int     m_values[NUM_FIELDS]              = {}; ///< Geparste Werte (N, M, R)
    uint8_t m_focusedField                    = 0;  ///< Aktuell fokussiertes Feld (0..2)

    // Berechnungsergebnis
    Direction m_direction     = Direction::NONE; ///< Zunahme, Abnahme oder keine Schräge
    Status    m_status        = Status::EMPTY;   ///< Gültigkeit des Ergebnisses
    int       m_diff          = 0;               ///< |M - N|
    int       m_increaseRows  = 0;               ///< Y = D / 2
    int       m_baseInterval  = 0;               ///< BASE = R / Y
    int       m_remainder     = 0;               ///< REM = R mod Y

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

#endif // WORKSHOPTOOL_SLEEVETAPER_H
