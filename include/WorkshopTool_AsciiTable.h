#ifndef WORKSHOP_TOOL_ASCII_TABLE_H
#define WORKSHOP_TOOL_ASCII_TABLE_H

// ============================================================================
// ASCII-Referenztabelle für die Bereiche 0-127 und 128-255
// ============================================================================

#include <stdint.h>

#include "Button.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"
#include "Theme.h"

// ============================================================================
// FARBKONFIGURATION
// ============================================================================

/**
 * @brief Bündelt die Farben der ASCII-Tabelle.
 *
 * Zum Anpassen einfach eine eigene Instanz erstellen und an den Konstruktor übergeben. Ohne Argument werden die Standardfarben aus Theme.h / RetroColors verwendet.
 *
 * Alle Werte im RGB565-Format.
 */
struct AsciiTableColors
{
    uint16_t tableBg;    ///< Tabellen-Hintergrund
    uint16_t headerBg;   ///< Kopfzeilen-Hintergrund (Spalten- und Zeilentitel)
    uint16_t headerTx;   ///< Kopfzeilen-Text
    uint16_t labelTx;    ///< Gedimmter Zeilenbeschriftungstext von "+0" bis "+F"
    uint16_t ctrlTx;     ///< Kürzel für Steuerzeichen wie NUL, LF und ESC
    uint16_t printTx;    ///< Druckbare Zeichen (0x21-0x7E und 0xA1-0xFF)
    uint16_t sep;        ///< Trennlinien zwischen Zellen
    uint16_t selBg;      ///< Hintergrund der ausgewählten Zelle
    uint16_t selTx;      ///< Text der ausgewählten Zelle
    uint16_t infoBg;     ///< Info-Streifen: Hintergrund
    uint16_t infoKey;    ///< Info-Streifen: Label-Text (DEC, HEX, OCT, BIN)
    uint16_t infoVal;    ///< Info-Streifen: Werte-Text
    uint16_t infoHint;   ///< Info-Streifen: Hinweistext bei fehlender Auswahl

    /**
     * @brief Liefert die Standardfarbpalette (Theme-basiert).
     * @return Vorausgefüllte AsciiTableColors-Instanz
     */
    static AsciiTableColors defaults()
    {
        AsciiTableColors c;
        c.tableBg  = 0x1082;              // Arbeitsbereich-Dunkelgrau (wie WorkshopCfg::C_WORK_BG)
        c.headerBg = 0x1082;              // Dunkler Kopfbereich mit Abgrenzung über die Textfarbe
        c.headerTx = 0xFFF5;              // Heller Kopftext in RetroColors::Cream
        c.labelTx  = 0x7D7A;              // Gedämpfte Beschriftungen in RetroColors::NordicLight
        c.ctrlTx   = Theme::ColorAmber;   // Steuerzeichen in Amber
        c.printTx  = 0xDEDB;              // Standardtext (grauweiss)
        c.sep      = 0x2945;              // Trennlinien (wie WorkshopCfg::C_SEP_LINE)
        c.selBg    = Theme::ColorSelectedRow; // Ausgewählt: Hintergrund
        c.selTx    = 0xFFFF;              // Ausgewählt: weißer Text
        c.infoBg   = 0x1082;              // Dunkler Hintergrund des Informationsbereichs
        c.infoKey  = 0x7D7A;              // Gedämpfte Beschriftungen in RetroColors::NordicLight
        c.infoVal  = 0xFFF5;              // Helle Werte in RetroColors::Cream
        c.infoHint = 0x6B6D;              // Hinweistext gedimmt
        return c;
    }
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_AsciiTable
 * @brief Interaktive ASCII-Referenztabelle im 8×16-Raster mit Umschaltung zwischen 0-127 und 128-255.
 *
 * Steuerzeichen erscheinen als dreistellige Kürzel wie NUL, LF oder ESC in Amber-Farbe, druckbare Zeichen als direktes Glyph. Beim Antippen einer Zelle werden DEC-, HEX-, OCT- und BIN-Darstellung sowie Name beziehungsweise Zeichen im Informationsstreifen am unteren Rand eingeblendet.
 *
 * Im Steuerbereich der Fußzeile befindet sich eine Umschaltung zwischen "0-127" und "128-255". Der erweiterte Bereich zeigt Windows-1252-Zeichen von 128 bis 159 als gerenderte Glyphen und ISO-8859-1-Zeichen von 160 bis 255 als UTF-8-kodierte Zeichen.
 *
 * Layout im Content-Bereich (547 × 327 px):
 *   - Lücke         2 px: HEADER_GAP
 *   - Kopfzeile    20 px: Spaltenbeschriftungen 0x00 ... 0x70 beziehungsweise 0x80 ... 0xF0
 *   - 16 Datenzeilen à 18 px (288 px): Zeilen +0 ... +F
 *   - Informationsstreifen 17 px: Details zum ausgewählten Zeichen
 *   - Summe: 2 + 20 + 288 + 17 = 327 px
 *
 * Farbgebung über AsciiTableColors konfigurierbar (Standardwerte in AsciiTableColors::defaults()).
 */
class WorkshopTool_AsciiTable : public WorkshopTool
{
public:
    /**
     * @brief Konstruktor
     * @param colors Farbpalette; ohne Argument werden Standardfarben verwendet
     */
    explicit WorkshopTool_AsciiTable(AsciiTableColors colors = AsciiTableColors::defaults());

    /** @brief Destruktor */
    ~WorkshopTool_AsciiTable() override = default;

    WorkshopTool_AsciiTable(const WorkshopTool_AsciiTable &) = delete;
    WorkshopTool_AsciiTable &operator=(const WorkshopTool_AsciiTable &) = delete;

    /**
     * @brief Initialisiert den Werkzeugzustand und die Footer-Buttons.
     * @param tft Zeiger auf das Display-Objekt
     */
    void onEnter(LGFX *tft) override;

    /** @brief Räumt den Werkzeugzustand beim Deaktivieren auf. */
    void onExit() override;

    /**
     * @brief Zeichnet die ASCII-Tabelle vollständig in den Work-Sprite.
     * @param spr Work-Sprite (Koordinaten sprite-relativ)
     * @param ox  X-Ursprung des Content-Bereichs (sprite-relativ)
     * @param oy  Y-Ursprung des Content-Bereichs (sprite-relativ)
     * @param w   Breite des Content-Bereichs in Pixeln
     * @param h   Höhe des Content-Bereichs in Pixeln
     *
     * Cacht ox/oy für den anschließenden hitTest()-Aufruf.
     */
    void draw(Gfx_Sprite *spr, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events und löst bei Bedarf einen Redraw aus.
     * @param tx      Absoluter Touch-X (Display-Koordinaten)
     * @param ty      Absoluter Touch-Y (Display-Koordinaten)
     * @param touched true wenn das Display berührt wird
     * @return true wenn ein Redraw des Arbeitsbereichs erforderlich ist
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    // ========================================================================
    // Footer-Steuerbereich
    // ========================================================================

    /**
     * @brief Zeichnet die Umschaltflächen "0-127" und "128-255" in die Fußzeile.
     * @param tft Display-Objekt
     * @param x   Linke Kante des Footer-Steuerbereichs (absolut)
     * @param y   Obere Kante (absolut)
     * @param w   Verfügbare Breite
     * @param h   Verfügbare Höhe
     */
    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events auf den Toggle-Buttons.
     * @return true wenn der Bereich umgeschaltet wurde (Workspace-Redraw nötig)
     */
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Gibt den zum aktiven Bereich passenden Untertitel zurück.
     * @return Statischer String oder nullptr für den Standardbereich 0-127
     */
    const char *getSubtitle() const override;

private:
    // ========================================================================
    // Zeichenfunktionen
    // ========================================================================

    /**
     * @brief Zeichnet das 8×16-Raster mit Kopfzeile und Zeilenbeschriftungen.
     */
    void drawTable(Gfx_Sprite *spr, int ox, int oy, int w, int h);

    /**
     * @brief Zeichnet den Info-Streifen am unteren Rand des Content-Bereichs.
     *
     * Zeigt bei fehlender Auswahl einen Hinweistext; sonst DEC / HEX / OCT / BIN sowie Glyph, Steuerzeichenkürzel oder Windows-1252-Zeichen.
     */
    void drawInfoStrip(Gfx_Sprite *spr, int ox, int oy, int w, int h);

    // ========================================================================
    // Hilfsfunktionen
    // ========================================================================

    /**
     * @brief Initialisiert die beiden Toggle-Buttons im Footer-Bereich.
     * @param tft Display-Objekt für die Button-Klasse
     */
    void initFooterButtons(LGFX *tft);

    /**
     * @brief Setzt Farben der Toggle-Buttons entsprechend m_showExtended.
     *
     * Der aktive Button erhält eine Highlight-Füllung, der inaktive eine dunkle Füllung mit gedimmtem Text.
     */
    void updateButtonColors();

    /**
     * @brief Ermittelt den ASCII-Code an einer absoluten Display-Position.
     * @return ASCII-Code 0-127 im Standardbereich beziehungsweise 128-255 im erweiterten Bereich,
     *         oder -1 wenn außerhalb des Datengitters
     */
    int hitTest(int32_t tx, int32_t ty) const;

    /**
     * @brief Gibt das Kürzel eines Steuerzeichens zurück (z. B. "LF").
     * @param ch ASCII-Code 0-31 oder 127
     * @return Unveränderliches String-Literal, niemals nullptr
     */
    static const char *ctrlName(int ch);

    /**
     * @brief Gibt die Langbezeichnung eines Steuerzeichens zurück.
     * @param ch ASCII-Code 0-31 oder 127
     * @return Unveränderliches String-Literal, niemals nullptr
     */
    static const char *ctrlDesc(int ch);

    /**
     * @brief Befüllt m_subtitleBuf je nach Bereichsmodus.
     *
     * Im Standardbereich (0-127) wird der Puffer geleert; Workshop fällt dann auf den statischen Untertitel aus PageDef zurück. Im erweiterten Bereich (128-255) wird der dynamische Subtitle eingesetzt.
     */
    void refreshSubtitle();

    // ========================================================================
    // Member-Variablen
    // ========================================================================

    AsciiTableColors m_colors;             ///< Aktive Farbpalette

    int m_selectedChar     = -1;           ///< Ausgewähltes Zeichen (-1 = keins)
    bool m_showExtended    = false;        ///< true für Bereich 128-255, false für Bereich 0-127
    bool m_touchWasActive  = false;        ///< true wenn im letzten Frame ein Touch aktiv war
    int32_t m_lastTouchX   = 0;           ///< Gecachte Touch-X-Position beim Drücken
    int32_t m_lastTouchY   = 0;           ///< Gecachte Touch-Y-Position beim Drücken

    char m_subtitleBuf[64] = {};           ///< Befüllt von refreshSubtitle(), gelesen von getSubtitle()

    // Gecachte Geometrie aus draw(), für hitTest() benötigt
    int m_ox = 0;                          ///< X-Ursprung Content-Bereich (sprite-relativ)
    int m_oy = 0;                          ///< Y-Ursprung Content-Bereich (sprite-relativ)

    // Footer-Toggle-Buttons
    Button m_btnStandard;                  ///< Schaltfläche "0-127"
    Button m_btnExtended;                  ///< Schaltfläche "128-255"
    bool m_buttonsInitialized = false;     ///< true nach erster initFooterButtons()-Ausführung
};

#endif // WORKSHOP_TOOL_ASCII_TABLE_H
