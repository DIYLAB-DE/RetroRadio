#ifndef WORKSHOPTOOL_RESISTOR_H
#define WORKSHOPTOOL_RESISTOR_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_Resistor
 * @brief Interaktiver Widerstandsfarbcode-Rechner (4- und 5-Band).
 *
 * Ermöglicht die visuelle Auswahl von Farbbändern per Touch. Der berechnete Widerstandswert und die Toleranz werden in Echtzeit aus den gewählten Farben ermittelt und angezeigt. Der Widerstandskörper aktualisiert sich live.
 *
 * Layout (von oben nach unten):
 *   - Körpervorschau (RESIST_H px): abgerundeter Körper + Leitungsdrähte + Ringe
 *   - Wertzeile    (VALUE_H px):  Ohm-Wert, Toleranz, Toggle-Buttons [4R] [5R]
 *   - Bandselektor (Rest):        je eine Farbzeile pro aktivem Band
 *
 * Farbbänder:
 *   4-Ring: B1 (10er), B2 (1er), Mult (Multiplikator), Tol (Toleranz)
 *   5-Ring: B1 (100er), B2 (10er), B3 (1er), Mult, Tol
 *
 * Ziffernbänder erlauben nur Farben 0-9 (Schwarz-Weiß); Gold und Silber sind dort ausgegraut und nicht auswählbar.
 *
 * Standardwert: 4R mit Gelb / Violett / Rot / Gold = 4,7 kOhm ±5 %.
 *
 * Zeichnet ausschließlich in den übergebenen Work-Sprite (kein eigener Sprite).
 */
class WorkshopTool_Resistor : public WorkshopTool
{
public:
    /**
     * @brief Konstruktor. Setzt alle Bänder auf den 4,7-kOhm-Standardwert.
     */
    WorkshopTool_Resistor();

    /** @brief Destruktor. Keine eigenen Ressourcen vorhanden. */
    ~WorkshopTool_Resistor() override = default;

    WorkshopTool_Resistor(const WorkshopTool_Resistor &)            = delete;
    WorkshopTool_Resistor &operator=(const WorkshopTool_Resistor &) = delete;

    /**
     * @brief Setzt den Rechner in den Ausgangszustand zurück.
     * @param tft Display-Objekt (wird nicht dauerhaft gespeichert)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Kein Aufräumaufwand beim Verlassen. */
    void onExit() override {}

    /**
     * @brief Zeichnet Körpervorschau, Wertzeile und Farbband-Selektoren.
     * @param sprite   Work-Sprite des Workshop-Moduls
     * @param originX  Linke Kante des Inhaltsbereichs (sprite-relativ)
     * @param originY  Obere Kante des Inhaltsbereichs (sprite-relativ)
     * @param w        Verfügbare Breite in Pixeln
     * @param h        Verfügbare Höhe in Pixeln
     */
    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events für Farbfelder und Toggle-Button.
     * @param tx      Absoluter Display-X (bei touched=false ignoriert)
     * @param ty      Absoluter Display-Y (bei touched=false ignoriert)
     * @param touched true bei aktivem Touch
     * @return true wenn Neuzeichnung des Arbeitsbereichs erforderlich
     *
     * Verwendet Touch-up-Semantik: die Aktion (Farbwahl, Moduswechsel) wird erst beim Loslassen ausgeführt. Beim Berühren wird visuelles Feedback gegeben (Hervorhebung des getroffenen Felds).
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

    static constexpr int NUM_COLORS  = 12; ///< Anzahl Farbcode-Farben von Schwarz bis Weiß sowie Gold und Silber
    static constexpr int MAX_BANDS   = 5;  ///< Maximale Bandanzahl (5-Ring-Modus)

    // Bereichshöhen
    static constexpr int RESIST_H    = 82; ///< Höhe des Widerstandskörper-Bereichs in Pixeln
    static constexpr int VALUE_H     = 34; ///< Höhe der Wertanzeige-Zeile in Pixeln

    // Farbfeld-Raster
    static constexpr int LABEL_W     = 42; ///< Breite der Bandbeschriftungsspalte in Pixeln (kompakt, längste Aufschrift "Tol" passt mit Abstand)
    static constexpr int QUAD_RADIUS = 10; ///< Eckenradius der Farbquadrate
    static constexpr int SEL_MARGIN  = 2;  ///< Abstand zwischen Auswahlrahmen- und Farbfeld-Außenkante
    static constexpr int SEL_RADIUS  = QUAD_RADIUS + SEL_MARGIN; ///< Eckenradius des Auswahlrahmens
    static constexpr int QUAD_GAP    = SEL_MARGIN * 2 + 1; ///< Abstand zwischen zwei Farbquadraten; mindestens SEL_MARGIN*2+1 verhindert ein Übermalen des Auswahlrahmens

    // Widerstandskörper-Geometrie
    static constexpr int BODY_W      = 320; ///< Körper-Breite in Pixeln
    static constexpr int BODY_H      = 60;  ///< Höhe des Widerstandskörpers in Pixeln

    // Toggle-Button-Abmessungen
    static constexpr int TOG_W   = 38; ///< Breite eines Toggle-Buttons in Pixeln
    static constexpr int TOG_H   = 22; ///< Höhe eines Toggle-Buttons in Pixeln
    static constexpr int TOG_GAP =  4; ///< Abstand zwischen den beiden Toggle-Buttons in Pixeln

    // UI-Farbpalette (RGB565)
    static constexpr uint16_t C_BG          = 0x1082; ///< Hintergrund (= WorkshopCfg::C_WORK_BG)
    static constexpr uint16_t C_LEAD        = 0xBDFA; ///< Anschlussleitung in hellem Silbergrau (RGB 185,191,209)
    static constexpr uint16_t C_LEAD_SHADOW = 0x8CB4; ///< Leitungsschatten in dunklem Silbergrau (RGB 143,151,167)
    static constexpr uint16_t C_BODY_OUTER  = 0x9368; ///< Gedimmte äußere Körperschicht (RGB 147,109,70)
    static constexpr uint16_t C_BODY_MID    = 0xB46C; ///< Körper mittlere Schicht (rgb 177,142,100)
    static constexpr uint16_t C_BODY_INNER  = 0xC592; ///< Körper innere Schicht (rgb 194,177,150)
    static constexpr uint16_t C_SHAD_BLEND  = 0x4162; ///< Schattenfarbe für Körper-Schattenstreifen (rgb 70,45,20)
    static constexpr uint16_t C_TEXT_DIM    = 0xAD75; ///< Gedimmter Text (Bandbeschriftungen)
    static constexpr uint16_t C_TEXT_VAL    = 0xFFFF; ///< Widerstandswert (weiß, hervorgehoben)
    static constexpr uint16_t C_SEL_FRAME   = 0xFFFF; ///< Farbe des Auswahlrahmens (konfigurierbar)
    static constexpr uint16_t C_PRESS_FILL  = 0x6B6D; ///< Farbfeld-Hintergrund beim Drücken
    static constexpr uint16_t C_ROW_EVEN    = 0x2124; ///< Zeilen-Hintergrund (gerade Indizes)
    static constexpr uint16_t C_ROW_ODD     = 0x2945; ///< Zeilen-Hintergrund (ungerade Indizes)
    static constexpr uint16_t C_TOG_ON      = 0x328A; ///< Aktiver Toggle-Button
    static constexpr uint16_t C_TOG_OFF     = 0x2945; ///< Inaktiver Toggle-Button
    static constexpr uint16_t C_TOG_PRESS   = 0x6B6D; ///< Toggle-Button beim Drücken
    static constexpr uint16_t C_TOG_TEXT    = 0xDEDB; ///< Toggle-Beschriftung
    static constexpr uint16_t C_DISABLED    = 0x2104; ///< Ausgegraut (Gold/Silber bei Digit-Bändern)

    // =========================================================================
    // Statische Datenbasis
    // =========================================================================

    static const uint16_t     BAND_COLORS[NUM_COLORS]; ///< RGB565-Darstellungsfarben der 12 Farbbänder
    static const double       MULT_VALUES[NUM_COLORS]; ///< Multiplikatoren (Index 0-9: ×1...×1G, 10: ×0,1, 11: ×0,01)
    static const char * const TOL_STRINGS[NUM_COLORS]; ///< Toleranzstrings ("+/-5%" usw.), "" wenn nicht normiert

    static const char * const BAND_LABELS_4[4]; ///< Bandbeschriftungen im 4-Ring-Modus: "B1","B2","x","Tol"
    static const char * const BAND_LABELS_5[5]; ///< Bandbeschriftungen im 5-Ring-Modus: "B1","B2","B3","x","Tol"

    // =========================================================================
    // Initialisierung und Layout
    // =========================================================================

    /**
     * @brief Setzt alle Zustands-, Layout- und Touch-Variablen auf definierte Ausgangswerte.
     *
     * Wird vom Konstruktor und von onEnter() aufgerufen. Alle char[]-Puffer werden explizit mit memset genullt, alle Bandindizes auf den 4,7-kOhm-Standardwert (Gelb/Violett/Rot/Gold) gesetzt.
     */
    void clearAll();

    /**
     * @brief Berechnet Farbfeld- und Toggle-Rechtecke für die gegebene Inhaltsgröße.
     * @param w Verfügbare Breite
     * @param h Verfügbare Höhe
     *
     * Alle berechneten Rects sind content-relativ (Ursprung = originX/originY bei draw()). computeLayout() wird automatisch aufgerufen wenn sich w/h ändert oder m_is5Band toggled.
     */
    void computeLayout(int w, int h);

    // =========================================================================
    // Wertberechnung
    // =========================================================================

    /**
     * @brief Berechnet m_valueStr und m_tolStr aus den aktuellen Bandindizes.
     *
     * 4-Ring: Wert = (B0×10 + B1) × MULT_VALUES[B2], Toleranz = TOL_STRINGS[B3] 5-Ring: Wert = (B0×100 + B1×10 + B2) × MULT_VALUES[B3], Toleranz = TOL_STRINGS[B4]
     *
     * Beide Strings werden für die deutsche Anzeige mit dotToComma() in Komma-Notation umgewandelt.
     */
    void computeValue();

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /** @brief Zeichnet Körper, Drähte und Farbringe in den Work-Sprite. */
    void drawResistorBody();

    /** @brief Zeichnet Wertzeile (Ohm-Wert, Toleranz, 4R/5R-Toggle). */
    void drawValueRow();

    /**
     * @brief Zeichnet eine Farbband-Auswahlzeile.
     * @param bandIdx 0-basierter Bandindex (0 = erstes / oberstes Band)
     */
    void drawBandRow(int bandIdx);

    // =========================================================================
    // Treffertests
    // =========================================================================

    /**
     * @brief Prüft ob content-relative Koordinaten auf einen Toggle-Button treffen.
     * @param cx    Content-relative X-Koordinate
     * @param cy    Content-relative Y-Koordinate
     * @param out5 Ausgabe: true bei getroffenem 5R-Button, false bei getroffenem 4R-Button
     * @return true bei Treffer auf einen der beiden Toggle-Buttons
     */
    bool hitTestToggle(int cx, int cy, bool &out5) const;

    /**
     * @brief Prüft ob content-relative Koordinaten auf ein Farbfeld einer Bandzeile treffen.
     * @param cx      Content-relative X-Koordinate
     * @param cy      Content-relative Y-Koordinate
     * @param bandIdx Zu testender Bandindex
     * @return Farbindex 0-11 bei Treffer; -1 wenn kein Treffer oder Feld deaktiviert
     *
     * Gold (Index 10) und Silber (Index 11) liefern -1 bei Digit-Bändern.
     */
    int hitTestColorField(int cx, int cy, int bandIdx) const;

    // =========================================================================
    // Hilfsfunktionen
    // =========================================================================

    /**
     * @brief Gibt zurück ob ein Band im aktuellen Modus ein reines Zifferband ist.
     * @param bandIdx 0-basierter Bandindex
     * @param is5band true für 5-Ring-Modus
     * @return true, wenn das Band nur Ziffern 0-9 trägt und Gold oder Silber nicht erlaubt sind
     */
    static bool isDigitBand(int bandIdx, bool is5band);

    /**
     * @brief Formatiert einen Ohm-Wert als lesbaren String mit 3 signifikanten Stellen.
     * @param ohm Widerstandswert in Ohm
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße in Bytes
     *
     * Ausgabe-Beispiele: "470 Ohm", "4,7 kOhm", "2,2 MOhm", "1,5 GOhm". Nicht-normale Werte (NaN, Inf, negativ) werden als "? Ohm" ausgegeben. Der Punkt aus printf wird vor der Rückgabe per dotToComma() in Komma gewandelt.
     */
    static void formatResistance(double ohm, char *buf, size_t sz);

    /**
     * @brief Zeichnet ein Capsule-Rechteck (abgerundetes Rect mit r = h/2).
     * @param spr    Ziel-Sprite
     * @param x y    Obere linke Ecke (sprite-relativ)
     * @param w h    Breite und Höhe
     * @param color  Füllfarbe (RGB565)
     */
    static void drawCapsule(Gfx_Sprite *spr, int x, int y, int w, int h, uint16_t color);

    /**
     * @brief Zeichnet einen einzelnen Farbring mit hellem/dunklem 3D-Rand.
     * @param spr   Ziel-Sprite
     * @param x y   Obere linke Ecke (sprite-relativ)
     * @param w h   Breite und Höhe des Rings
     * @param color Ringfarbe (RGB565)
     */
    static void drawSingleBand(Gfx_Sprite *spr, int x, int y, int w, int h, uint16_t color);

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite   = nullptr; ///< Aktueller Work-Sprite (wird in draw() gesetzt)
    int  m_originX          = 0;       ///< Inhaltsursprung X (sprite-relativ)
    int  m_originY          = 0;       ///< Inhaltsursprung Y (sprite-relativ)
    int  m_w                = 0;       ///< Zuletzt übergebene Inhaltsbreite
    int  m_h                = 0;       ///< Zuletzt übergebene Inhaltshöhe
    bool m_layoutValid      = false;   ///< true wenn Farbfeld-Rects aktuell berechnet sind

    int  m_bandCount        = 4;  ///< Anzahl aktiver Bänder (4 oder 5)
    int  m_bandH            = 0;  ///< Berechnete Zeilenhöhe pro Band in Pixeln
    int  m_quadSize         = 0;  ///< Seitenlänge der Farbquadrate in Pixeln

    Rect m_tog4Rect         = {};                         ///< Toggle-Button "4R" (content-relativ)
    Rect m_tog5Rect         = {};                         ///< Toggle-Button "5R" (content-relativ)
    Rect m_colorRects[MAX_BANDS][NUM_COLORS]  = {};       ///< Farbfeld-Rechtecke je Band und Farbe (content-relativ)

    uint8_t m_bands[MAX_BANDS] = {}; ///< Aktiver Farbindex je Band (0-11)
    bool    m_is5Band          = false; ///< true = 5-Ring-Modus aktiv
    char    m_valueStr[32]     = {};    ///< Formatierter Widerstandswert (z.B. "4,7 kOhm")
    char    m_tolStr[16]       = {};    ///< Formatierte Toleranz (z.B. "+/-5%")
    bool    m_valueDirty       = true;  ///< true wenn Neuberechnung von Wert/Toleranz erforderlich

    int  m_pressedBand  = -1;  ///< Bandindex des aktuell gedrückten Felds (-1 = keins)
    int  m_pressedColor = -1;  ///< Farbindex des aktuell gedrückten Felds (-1 = keins)
    int  m_pressedTog   =  0;  ///< Aktuell gedrückter Toggle: 0 = keiner, 4 = 4R-Button, 5 = 5R-Button
    bool m_touchWas     = false; ///< true wenn im letzten Frame ein Touch aktiv war
    bool m_touchReady   = false; ///< true nach erstem touched=false-Frame (Phantom-Touch-Schutz beim Boot)
};

#endif // WORKSHOPTOOL_RESISTOR_H
