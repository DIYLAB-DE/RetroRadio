#ifndef WORKSHOP_TOOL_BIT_EDITOR_H
#define WORKSHOP_TOOL_BIT_EDITOR_H

// ============================================================================
// Interaktiver Binär-Editor für 8, 16 und 32 Bit
// ============================================================================

#include <stdint.h>

#include "Button.h"
#include "DotMatrix.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"
#include "Theme.h"

// ============================================================================
// FARBKONFIGURATION
// ============================================================================

/**
 * @brief Farbpalette des Bit-Editors.
 *
 * LCD-Panel im Taschenrechner-Stil: heller Hintergrund mit dunklen Segmenten und Ghost-Dots für inaktive Bereiche. Alle Werte im RGB565-Format.
 */
struct BitEditorColors
{
    uint16_t tableBg;   ///< Arbeitsbereich-Hintergrund (außerhalb LCD)
    uint16_t dispBg;    ///< LCD-Panel-Hintergrund (hell, wie Calculator)
    uint16_t lcdOn;     ///< DotMatrix ON-Dots (dunkel auf hell)
    uint16_t lcdOff;    ///< DotMatrix OFF-Dots (Ghost-Segmente)
    uint16_t dispLabel; ///< Bit-Nummern aktiver Zeilen
    uint16_t valKey;    ///< Wertanzeige: Label-Text (DEC, HEX, OCT, BIN)
    uint16_t valTx;     ///< Wertanzeige: Wertetext

    /**
     * @brief Liefert die Standardfarbpalette.
     * @return Vorausgefüllte BitEditorColors-Instanz
     */
    static BitEditorColors defaults()
    {
        BitEditorColors c;
        c.tableBg = 0x1082;   // WorkshopCfg::C_WORK_BG
        c.dispBg = 0xD6B4;    // Warmes Hellgrau (identisch Calculator)
        c.lcdOn = 0x2104;     // Dunkle LCD-Segmente (identisch Calculator)
        c.lcdOff = 0xC652;    // Ghost-Segmente (identisch Calculator)
        c.dispLabel = 0x2945; // Dunkle Labels (identisch Calculator)
        c.valKey = 0x7D7A;    // RetroColors::NordicLight
        c.valTx = 0xFFF5;     // RetroColors::Cream
        return c;
    }
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_BitEditor
 * @brief Interaktiver 8/16/32-Bit Binär-Editor mit LCD-Display.
 *
 * Ein simuliertes LCD-Display im Taschenrechner-Look zeigt 4 Zeilen à 8 Bit als DotMatrix-Kästchen. Die Darstellungsgröße bleibt in allen Modi identisch; nur die Anzahl aktiver Zeilen ändert sich:
 *
 *   8-Bit:  Zeile 4 aktiv  (Bits 7-0)
 *   16-Bit: Zeilen 3-4     (Bits 15-0)
 *   32-Bit: Zeilen 1-4     (Bits 31-0)
 *
 * Inaktive Zeilen erscheinen als Ghost-Pattern (ON = OFF-Farbe). Über jedem aktiven Bit steht die Bit-Nummer (ComfortaaBold14). Unterhalb des Panels: DEC, HEX, OCT, BIN (ComfortaaBold16).
 *
 * Alle Maße wie Punktgröße, Abstände und Panelhöhe berechnen sich aus wenigen Grundkonstanten.
 *
 * Farbgebung über BitEditorColors konfigurierbar.
 */
class WorkshopTool_BitEditor : public WorkshopTool
{
public:
    /**
     * @brief Konfiguriert die DotMatrix im LCD-Stil.
     * @param colors Farbpalette; ohne Argument werden Standardfarben verwendet
     */
    explicit WorkshopTool_BitEditor(BitEditorColors colors = BitEditorColors::defaults());

    /** @brief Destruktor */
    ~WorkshopTool_BitEditor() override = default;

    WorkshopTool_BitEditor(const WorkshopTool_BitEditor &) = delete;
    WorkshopTool_BitEditor &operator=(const WorkshopTool_BitEditor &) = delete;

    /**
     * @brief Initialisiert den Werkzeugzustand und die Footer-Buttons.
     * @param tft Zeiger auf das Display-Objekt
     */
    void onEnter(LGFX *tft) override;

    /** @brief Räumt den Werkzeugzustand beim Deaktivieren auf. */
    void onExit() override;

    /**
     * @brief Zeichnet den Bit-Editor vollständig in den Work-Sprite.
     * @param spr Work-Sprite (Koordinaten sprite-relativ)
     * @param ox  X-Ursprung des Content-Bereichs (sprite-relativ)
     * @param oy  Y-Ursprung des Content-Bereichs (sprite-relativ)
     * @param w   Verfügbare Breite in Pixeln
     * @param h Verfügbare Höhe in Pixeln; beim festen Layout nicht verwendet
     */
    void draw(Gfx_Sprite *spr, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events auf den Bit-Kästchen.
     * @param tx      Absoluter Touch-X (Display-Koordinaten)
     * @param ty      Absoluter Touch-Y (Display-Koordinaten)
     * @param touched true wenn das Display berührt wird
     * @return true wenn ein Redraw des Arbeitsbereichs erforderlich ist
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Zeichnet die Bitbreiten-Toggle und Aktions-Buttons in die Fußzeile.
     * @param tft Display-Objekt
     * @param x   Linke Kante des Footer-Steuerbereichs (absolut)
     * @param y   Obere Kante (absolut)
     * @param w   Verfügbare Breite
     * @param h   Verfügbare Höhe
     */
    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events auf den Footer-Buttons.
     * @param tx      Absoluter Touch-X
     * @param ty      Absoluter Touch-Y
     * @param touched true wenn das Display berührt wird
     * @return true wenn Workspace-Redraw nötig ist
     */
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Gibt den zur aktiven Bitbreite passenden Untertitel zurück.
     * @return Statischer String, niemals nullptr
     */
    const char *getSubtitle() const override;

private:
    // ========================================================================
    // Zeichenfunktionen
    // ========================================================================

    /**
     * @brief Zeichnet das LCD-Panel (fillSmoothRoundRect + 4×8 DotMatrix-Gitter).
     * @param spr    Work-Sprite
     * @param panelX Linke Kante des Panels (sprite-relativ)
     * @param panelY Obere Kante des Panels (sprite-relativ)
     * @param panelW Breite des Panels
     */
    void drawLcdPanel(Gfx_Sprite *spr, int panelX, int panelY, int panelW);

    /**
     * @brief Zeichnet die Wertanzeige als zwei Textzeilen unterhalb des Panels.
     * @param spr  Work-Sprite
     * @param ox   X-Ursprung des Content-Bereichs
     * @param valY Y-Position der ersten Wertzeile
     * @param w    Verfügbare Breite (aktuell nicht verwendet)
     */
    void drawValues(Gfx_Sprite *spr, int ox, int valY, int w);

    // ========================================================================
    // Hilfsfunktionen
    // ========================================================================

    /**
     * @brief Initialisiert die fünf Footer-Buttons (8/16/32 + NOT/CLR).
     * @param tft Display-Objekt für die Button-Klasse
     */
    void initFooterButtons(LGFX *tft);

    /** @brief Setzt Farben aller Toggle-Buttons entsprechend m_bitWidth. */
    void updateButtonColors();

    /**
     * @brief Ermittelt den Bit-Index an einer absoluten Display-Position.
     * @param tx Absoluter Touch-X
     * @param ty Absoluter Touch-Y
     * @return Bit-Index (0 = LSB), oder -1 wenn außerhalb oder inaktiv
     */
    int hitTestBit(int32_t tx, int32_t ty) const;

    /**
     * @brief Gibt den maskierten Wert für die aktuelle Bitbreite zurück.
     * @return Wert mit oberen Bits auf 0 maskiert
     */
    uint32_t maskedValue() const;

    /**
     * @brief Formatiert den aktuellen Wert als Binärstring mit Nibble-Leerzeichen.
     * @param buf     Ausgabepuffer (mindestens 44 Bytes für 32-Bit)
     * @param bufSize Puffergröße
     */
    void formatBinary(char *buf, size_t bufSize) const;

    /**
     * @brief Befüllt m_subtitleBuf passend zur aktuellen Bitbreite.
     */
    void refreshSubtitle();

    // ========================================================================
    // Member-Variablen
    // ========================================================================

    BitEditorColors m_colors; ///< Aktive Farbpalette

    uint32_t m_value = 0;   ///< Aktueller Wert
    uint8_t m_bitWidth = 8; ///< Aktive Bitbreite: 8, 16 oder 32

    char m_subtitleBuf[64] = {}; ///< Befüllt von refreshSubtitle(), gelesen von getSubtitle()

    DotMatrix m_dot; ///< Eine Instanz, pro Zelle wiederverwendet

    // Gecachte Geometrie (aus draw(), für hitTestBit benötigt)
    int m_panelX = 0;      ///< LCD-Panel linke Kante (sprite-relativ)
    int m_panelY = 0;      ///< LCD-Panel obere Kante (sprite-relativ)
    int m_gridOffsetX = 0; ///< Gitter-Offset innerhalb des Panels

    bool m_touchWasActive = false; ///< true wenn im letzten Frame Touch aktiv war

    // Footer-Buttons
    Button m_btn8;                     ///< Umschaltung "8"
    Button m_btn16;                    ///< Umschaltung "16"
    Button m_btn32;                    ///< Umschaltung "32"
    Button m_btnNot;                   ///< Aktion "NOT" zum Invertieren aller Bits
    Button m_btnClr;                   ///< Aktion "CLR" zum Zurücksetzen auf 0
    bool m_buttonsInitialized = false; ///< true nach initFooterButtons()
};

#endif // WORKSHOP_TOOL_BIT_EDITOR_H
