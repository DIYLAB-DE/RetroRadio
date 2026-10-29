#ifndef WORKSHOPTOOL_CIRCLEOFFIFTHS_H
#define WORKSHOPTOOL_CIRCLEOFFIFTHS_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_CircleOfFifths
 * @brief Interaktiver Quintenzirkel mit Tonarten, Vorzeichen und Stufenharmonik.
 *
 * Stellt die zwölf Dur-Tonarten und ihre parallelen Moll-Tonarten als Doppelring dar. Der äußere Ring trägt die Dur-Tonarten, der innere die Moll-Parallelen. Tap auf einen beliebigen Sektor wählt die zugehörige Dur-Tonart als aktive Tonika. Subdominante (Position links davon) und Dominante (Position rechts davon) sowie deren Moll-Parallelen werden farblich hervorgehoben und folgen der Auswahl automatisch.
 *
 * Layout (547 x 327 px Netto-Inhaltsbereich):
 *   Linke Spalte (327 px): Quadratischer Quintenzirkel mit zwei konzentrischen Ringen, Vorzeichen-Markern am Außenrand und einer Mittelfläche, die die aktuelle Tonart groß anzeigt.
 *   Rechte Spalte (208 px): Tonart-Header, Vorzeichen-Detail, Tonleitertöne der Dur-Tonart und tabellarische Stufenharmonik mit sechs Stufen (I, ii, iii, IV, V, vi).
 *
 * Tonart-Notation in deutscher Form: H für den Ton zwischen A und C, B für den Ton zwischen A und H. Die Tonart an Position 6 Uhr wird als Fis-Dur geführt (sechs Kreuze).
 *
 * Stufenharmonik in Dur (fester diatonischer Aufbau, ohne den verminderten vii-Akkord):
 *   I    Dur   Tonika
 *   ii   Moll
 *   iii  Moll
 *   IV   Dur   Subdominante
 *   V    Dur   Dominante
 *   vi   Moll  Tonika-Parallele
 *
 * Akkord-Schreibweise: Dur-Akkorde mit großem Anfangsbuchstaben (C, G, Fis), Moll-Akkorde mit kleinem Anfangsbuchstaben und angehängtem 'm' (am, em, fism, esm).
 *
 * Der Untertitel im Tool-Header wird dynamisch aus der gewählten Tonart und ihrer Vorzeichen-Anzahl gebildet (z. B. "G-Dur (1 Kreuz)").
 *
 * Render-Strategie: Workshop löscht den Inhaltsbereich vor jedem Aufruf von draw() mit Hintergrundfarbe. Damit ein Press-Wechsel nicht das komplette Bild neu rendern muss, hält das Tool einen vor-gerenderten Output-Cache in PSRAM (kompletter Tool-Inhaltsbereich, alle Elemente bis auf das Press-Overlay). Pro Frame werden zwei Schritte ausgeführt: Output-Cache in den Work-Sprite pushen, danach das Press-Overlay direkt in den Work-Sprite zeichnen. Der Cache wird neu aufgebaut, sobald sich die ausgewählte Tonart ändert oder das Layout neu vermessen wird. Press-Wechsel ohne Auswahl-Wechsel verwenden den Cache wieder und kosten nur einen Sprite-Push plus zwei Sektorfüllungen für das Overlay.
 */
class WorkshopTool_CircleOfFifths : public WorkshopTool
{
public:
    WorkshopTool_CircleOfFifths();
    ~WorkshopTool_CircleOfFifths() override;

    WorkshopTool_CircleOfFifths(const WorkshopTool_CircleOfFifths &)            = delete;
    WorkshopTool_CircleOfFifths &operator=(const WorkshopTool_CircleOfFifths &) = delete;

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

    /**
     * @brief Statische Beschreibung einer Dur-Tonart und ihrer Moll-Parallele.
     *
     * Die Reihenfolge im KEYS-Array entspricht der Position im Quintenzirkel im Uhrzeigersinn ab 12 Uhr (C-Dur). scale[] enthält die sieben diatonischen Töne der Dur-Tonart in deutscher Notation, beginnend mit dem Grundton.
     */
    struct KeyData
    {
        const char *major;     ///< Dur-Tonart-Name (z.B. "C", "Fis", "Es")
        const char *minor;     ///< Moll-Parallele-Name kleingeschrieben (z.B. "a", "dis")
        int8_t      acc;       ///< Vorzeichenanzahl, positiv = Kreuze, negativ = Bs
        const char *scale[7];  ///< Diatonische Tonleiter der Dur-Tonart
    };

    /** @brief Akkord-Geschlecht für die Stufenharmonik. */
    enum class ChordQuality : uint8_t
    {
        MAJOR = 0,
        MINOR = 1,
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_KEYS       = 12; ///< Sektoren im Quintenzirkel
    static constexpr int SCALE_LEN      = 7;  ///< Töne einer diatonischen Tonleiter
    static constexpr int NUM_DEGREES    = 6;  ///< Diatonische Stufen ohne vii
    static constexpr int SUBTITLE_CAP   = 48; ///< Pufferkapazität für getSubtitle
    static constexpr int CHORD_BUF_CAP  = 12; ///< Pufferkapazität für ein Akkord-Symbol
    static constexpr int DETAIL_BUF_CAP = 96; ///< Pufferkapazität für Vorzeichen-Detailtext
    static constexpr int HEAD_BUF_CAP   = 24; ///< Pufferkapazität für den Tonart-Header
    static constexpr int SCALE_BUF_CAP  = 64; ///< Pufferkapazität für die Tonleiterzeile

    // Geometrie der linken Spalte
    static constexpr int CIRCLE_W = 327; ///< Breite der Visualisierungsspalte
    static constexpr int COL_GAP  = 12;  ///< Abstand zwischen den Spalten

    // Radien der konzentrischen Ringe in Pixeln, Mittelpunkt-relativ
    static constexpr int RADIUS_DUR_OUTER  = 145;
    static constexpr int RADIUS_DUR_INNER  = 110;
    static constexpr int RADIUS_MOLL_OUTER = 110;
    static constexpr int RADIUS_MOLL_INNER = 75;
    static constexpr int RADIUS_INNER_DISC = 72;
    static constexpr int RADIUS_DUR_LABEL  = (RADIUS_DUR_OUTER + RADIUS_DUR_INNER) / 2;
    static constexpr int RADIUS_MOLL_LABEL = (RADIUS_MOLL_OUTER + RADIUS_MOLL_INNER) / 2;
    static constexpr int RADIUS_ACCIDENTAL = 159;

    // Y-Offsets innerhalb der Mittelfläche, gemessen vom Kreismittelpunkt
    static constexpr int INNER_DISC_TONIC_DY  = -8; ///< Vertikaler Versatz des Tonart-Großtextes
    static constexpr int INNER_DISC_SUFFIX_DY = 22; ///< Vertikaler Versatz des Suffix "Dur"

    // Sektorwinkel
    static constexpr float SECTOR_DEG      = 30.0f;  ///< 360 Grad / 12 Sektoren
    static constexpr float SECTOR_HALF_DEG = 15.0f;  ///< Halber Sektor
    static constexpr float FULL_TURN_DEG   = 360.0f; ///< Vollkreis in Grad

    // Y-Positionen in der rechten Spalte (content-relativ)
    static constexpr int RIGHT_HEAD_Y      = 8;   ///< Tonart-Header (groß)
    static constexpr int RIGHT_ACC_Y       = 50;  ///< Vorzeichen-Detail
    static constexpr int RIGHT_SCALE_HDR_Y = 78;  ///< Beschriftung "Tonleiter"
    static constexpr int RIGHT_SCALE_Y     = 102; ///< Tonleiter-Töne
    static constexpr int RIGHT_DEG_HDR_Y   = 132; ///< Beschriftung "Stufen"
    static constexpr int RIGHT_DEG_Y       = 156; ///< Erste Stufenzeile
    static constexpr int RIGHT_DEG_LINE_H  = 24;  ///< Vertikaler Abstand zwischen Stufenzeilen

    // Spaltenpositionen innerhalb der rechten Info-Spalte
    static constexpr int RIGHT_PAD_X     = 4;  ///< Linker Innenabstand der rechten Spalte
    static constexpr int DEG_COL_ROMAN_X = 8;  ///< X-Offset der Roman-Stufe
    static constexpr int DEG_COL_CHORD_X = 44; ///< X-Offset des Akkord-Symbols
    static constexpr int DEG_COL_FUNC_X  = 96; ///< X-Offset der Funktionsbeschreibung

    // Farben
    static constexpr uint16_t C_BG             = 0x1082; ///< Hintergrund (Workshop-Standard)
    static constexpr uint16_t C_RING_BG_DUR    = 0x2945; ///< Dur-Ring-Hintergrund (heller)
    static constexpr uint16_t C_RING_BG_MOLL   = 0x18A4; ///< Moll-Ring-Hintergrund (dunkler)
    static constexpr uint16_t C_RING_OUTLINE   = 0x4A69; ///< Konzentrische Ringgrenzen
    static constexpr uint16_t C_RING_SEPARATOR = 0x39E8; ///< Radiale Sektor-Trennlinien
    static constexpr uint16_t C_INNER_DISC_BG  = 0x18A4; ///< Mittelfläche (aktive Tonart)

    static constexpr uint16_t C_TEXT_PRIMARY = 0xDEDB; ///< Standardtext
    static constexpr uint16_t C_TEXT_DIM     = 0xAD75; ///< Sekundärer Text
    static constexpr uint16_t C_TEXT_LABEL   = 0xB5F7; ///< Beschriftungs-Header
    static constexpr uint16_t C_TEXT_INVERSE = 0x1082; ///< Schrift auf akzentuiertem Hintergrund

    // Tool-interne Akzentfarben passend zum Inhalt
    static constexpr uint16_t C_ACCENT_TONIC       = 0xFD20; ///< Tonika (warmes Orange)
    static constexpr uint16_t C_ACCENT_DOMINANT    = 0xFEA0; ///< Dominante (helles Gold)
    static constexpr uint16_t C_ACCENT_SUBDOMINANT = 0x07FF; ///< Subdominante (helles Cyan)
    static constexpr uint16_t C_ACCENT_PARALLEL    = 0xC1AC; ///< Tonika-Parallele (Pflaume)

    static constexpr uint16_t C_PRESS_OVERLAY = 0x4B4D; ///< Sektor unter Daumen

    static constexpr uint16_t C_ACCIDENTAL_SHARP = 0xFEA0; ///< Vorzeichen-Marker für Kreuze
    static constexpr uint16_t C_ACCIDENTAL_FLAT  = 0x07FF; ///< Vorzeichen-Marker für Bs

    // =========================================================================
    // Statische Datenbasis
    // =========================================================================

    static const KeyData      KEYS[NUM_KEYS];                   ///< Tonarten im Uhrzeigersinn ab 12 Uhr
    static const ChordQuality DEGREE_QUALITY[NUM_DEGREES];      ///< Akkord-Geschlecht je Stufe
    static const char *const  DEGREE_LABELS[NUM_DEGREES];       ///< Roman-Symbole I, ii, iii, IV, V, vi
    static const WorkshopKey  FUNCTION_LABEL_KEYS[NUM_DEGREES]; ///< Klartext-Funktion je Stufe (WORKSHOP_EMPTY wenn keine Hauptfunktion)
    static const int          DEGREE_OFFSET[NUM_DEGREES];       ///< Tonleiter-Index zur Stufe (1 -> 0, 2 -> 1, ...)
    static const int          POSITION_OFFSET[NUM_DEGREES];     ///< Position-Delta im Quintenzirkel (Tonika=0, IV=-1, V=+1, ii=-1, iii=+1, vi=0)

    // =========================================================================
    // Private Methoden - Lebenszyklus
    // =========================================================================

    /** @brief Setzt alle Member auf definierte Ausgangswerte ohne Sprite-Freigabe. */
    void clearAll();

    /** @brief Berechnet Spaltenaufteilung und Kreismittelpunkt. */
    void computeLayout(int w, int h);

    /** @brief Gibt den Output-Cache-Sprite frei und markiert ihn als ungültig. */
    void releaseOutputCache();

    // =========================================================================
    // Private Methoden - Cache-Aufbau
    // =========================================================================

    /**
     * @brief Erstellt den Output-Cache und rendert das komplette Tool-Bild ohne Press-Overlay hinein.
     *
     * Reserviert beim ersten Aufruf einen PSRAM-Sprite in Größe (m_w x m_h). Während des Aufbaus zeigen m_sprite und m_origin auf den Cache-Sprite mit Origin (0,0), so dass die Body-only-Methoden ohne Sonderpfad in den Cache zeichnen. Erfolgreicher Abschluss setzt m_outputCacheValid auf true und merkt sich m_selectedKey in m_outputCacheKey. Wenn der Sprite nicht erstellt werden kann, bleibt das Flag false und draw() fällt auf einen direkten Render in den Work-Sprite zurück.
     */
    void buildOutputCache();

    /** @brief Rendert das komplette Tool-Bild ohne Press-Overlay in das aktuelle m_sprite. Font-Sessions inklusive. */
    void renderFullSceneNoPress();

    /** @brief Zeichnet die zwei Ring-Vollflächen als zwei drawSmoothGauge-Aufrufe. Font-frei. */
    void drawRingsBackground();

    /** @brief Zeichnet die drei konzentrischen Ringgrenzen. Font-frei. */
    void drawRingsBorders();

    // =========================================================================
    // Private Methoden - Body-only-Renderer
    //
    // Diese Methoden setzen voraus, dass der Aufrufer den passenden Font bereits geladen hat. Sie laden und entladen keine Fonts. Das Datum wird in der Methode gesetzt. Die Vorbedingungen sind im jeweiligen @brief dokumentiert.
    // =========================================================================

    /**
     * @brief Zeichnet die Vorzeichen-Marker am Außenrand des Kreises.
     * @pre ComfortaaBold14 ist geladen.
     */
    void drawAccidentalMarkers();

    /**
     * @brief Zeichnet alle 12 Tonart-Beschriftungen im äußeren Ring mit Default-Hintergrund.
     * @pre ComfortaaBold16 ist geladen.
     *
     * Die Methode beachtet die aktuelle Auswahl nicht. Im Render-Loop werden die akzentuierten Positionen anschließend mit drawDurLabelsAccented überzeichnet.
     */
    void drawDurLabelsDefault();

    /**
     * @brief Zeichnet alle 12 Moll-Beschriftungen im inneren Ring mit Default-Hintergrund.
     * @pre ComfortaaBold14 ist geladen.
     */
    void drawMollLabelsDefault();

    /**
     * @brief Zeichnet die drei akzentuierten Tonart-Beschriftungen mit Akzent-Hintergrund.
     * @pre ComfortaaBold16 ist geladen.
     */
    void drawDurLabelsAccented();

    /**
     * @brief Zeichnet die drei akzentuierten Moll-Beschriftungen mit Akzent-Hintergrund.
     * @pre ComfortaaBold14 ist geladen.
     */
    void drawMollLabelsAccented();

    /**
     * @brief Zeichnet den Tonart-Großtext in die Mittelfläche.
     * @pre ComfortaaBold30 ist geladen.
     */
    void drawInnerDiscMain();

    /**
     * @brief Zeichnet das Suffix "Dur" unterhalb des Großtextes.
     * @pre ComfortaaBold14 ist geladen.
     */
    void drawInnerDiscSuffix();

    /** @brief Zeichnet die akzentuierten Sektoren beider Ringe. Font-frei. */
    void drawAccentSectors();

    /** @brief Zeichnet die zwölf radialen Trennlinien zwischen den Sektoren. Font-frei. */
    void drawRingSeparators();

    /**
     * @brief Zeichnet das Press-Overlay für den aktuell gedrückten Sektor in das aktuelle m_sprite.
     *
     * Diese Methode wird ausschließlich im Workshop-Sprite aufgerufen, nicht im Output-Cache. So bleibt der Cache wiederverwendbar, solange sich die Auswahl nicht ändert.
     */
    void drawPressOverlay();

    /**
     * @brief Zeichnet die Header-Region der Info-Spalte (Tonart-Name und Vorzeichen-Detailzeile).
     * @pre ComfortaaBold16 ist geladen.
     */
    void drawInfoPanelHeader();

    /**
     * @brief Zeichnet die Body-Region der Info-Spalte (Tonleiter und Stufen).
     * @pre ComfortaaBold14 ist geladen.
     */
    void drawInfoPanelBody();

    /**
     * @brief Zeichnet eine einzelne Stufenzeile in der rechten Spalte.
     * @pre ComfortaaBold14 ist geladen.
     * @param row  Zeilenindex 0..NUM_DEGREES-1
     * @param yPos Y-Position der Zeile (sprite-absolut)
     */
    void drawDegreeRow(int row, int yPos);

    // =========================================================================
    // Private Methoden - Geometrie und Treffertest
    // =========================================================================

    /**
     * @brief Füllt einen Sektor zwischen zwei Radien mit einer Farbe.
     * @param pos       Sektor-Position 0..11 (12 Uhr = 0, im Uhrzeigersinn)
     * @param innerR    Innenradius des Sektors in Pixeln
     * @param outerR    Außenradius des Sektors in Pixeln
     * @param fillColor RGB565-Farbe der Füllung
     */
    void drawRingSector(int pos, int innerR, int outerR, uint16_t fillColor);

    /**
     * @brief Treffertest auf einen Quintenzirkel-Sektor.
     * @param cx Content-relative Touch-X-Koordinate
     * @param cy Content-relative Touch-Y-Koordinate
     * @return Sektor-Index 0..11, oder -1 wenn außerhalb der Ringe
     *
     * Erfasst sowohl den Dur- als auch den Moll-Ring. Beide Ringe wählen dieselbe Position; ein Tap auf den inneren Ring wählt die zur Position gehörige Dur-Tonart.
     */
    int hitTestCircle(int cx, int cy) const;

    // =========================================================================
    // Private Helfer
    // =========================================================================

    /**
     * @brief Reduziert eine Sektor-Position modulo NUM_KEYS in den Bereich 0..NUM_KEYS-1.
     * @param p Beliebiger Positionsindex (auch negativ oder >= NUM_KEYS)
     * @return Wert im Bereich 0..NUM_KEYS-1
     */
    static int wrapPos(int p);

    /**
     * @brief Bildet das Akkord-Symbol für einen Stufen-Akkord.
     * @param tone Tonleiter-Ton kleingeschrieben (z. B. "fis")
     * @param q    Akkord-Geschlecht
     * @param buf  Ausgabepuffer
     * @param sz   Pufferkapazität
     */
    static void formatChord(const char *tone, ChordQuality q, char *buf, size_t sz);

    /**
     * @brief Bildet die Detailzeile zur Vorzeichen-Anzeige in der rechten Spalte.
     * @param keyIdx Index der Tonart in KEYS
     * @param buf    Ausgabepuffer
     * @param sz     Pufferkapazität
     *
     * Beispielausgaben: "ohne Vorzeichen", "1 Kreuz: fis", "3 Bs: b, es, as".
     */
    static void formatAccidentalDetail(int keyIdx, char *buf, size_t sz);

    /**
     * @brief Liefert die Akzentfarbe je Position-Delta zur Tonika.
     * @param posDelta Position-Delta zur Tonika (-1 = Subdominante, 0 = Tonika, +1 = Dominante)
     * @return RGB565-Farbe; 0 wenn keine Akzentuierung gewünscht ist
     */
    uint16_t functionAccent(int posDelta) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    LGFX       *m_tft         = nullptr; ///< Display-Instanz, wird für Sprite-Allokation gebraucht
    Gfx_Sprite *m_sprite      = nullptr; ///< Aktuelles Render-Target (Workshop-Sprite oder Output-Cache)
    Gfx_Sprite *m_outputCache = nullptr; ///< Vor-gerenderter Tool-Inhalt ohne Press-Overlay

    int  m_originX          = 0;
    int  m_originY          = 0;
    int  m_w                = 0;
    int  m_h                = 0;
    bool m_layoutValid      = false;
    bool m_outputCacheValid = false; ///< true wenn der Output-Cache erfolgreich aufgebaut wurde
    uint8_t m_outputCacheKey = 0;    ///< Auswahl-Index, für den der Output-Cache gerendert wurde

    int m_circleCx = 0; ///< Kreismittelpunkt X (content-relativ)
    int m_circleCy = 0; ///< Kreismittelpunkt Y (content-relativ)
    int m_panelX   = 0; ///< X-Start der rechten Info-Spalte (content-relativ)
    int m_panelW   = 0; ///< Breite der rechten Info-Spalte

    uint8_t m_selectedKey = 0; ///< Index in KEYS, 0..11

    mutable char m_subtitleBuf[SUBTITLE_CAP] = {}; ///< Puffer für getSubtitle()

    int  m_pressedKey = -1;   ///< Aktuell gedrückter Sektor-Index, -1 wenn keiner
    bool m_touchWas   = false;
    bool m_touchReady = false;
};

#endif // WORKSHOPTOOL_CIRCLEOFFIFTHS_H
