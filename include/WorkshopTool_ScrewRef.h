#ifndef WORKSHOPTOOL_SCREWREF_H
#define WORKSHOPTOOL_SCREWREF_H

// ============================================================================
// Schraubenreferenz für M2-M12
// ============================================================================

#include <stdint.h>

#include "Button.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

// ============================================================================
// GEWINDETYP
// ============================================================================

/**
 * @brief Gewindeart für die Schrauben-Referenz.
 */
enum class ThreadMode : uint8_t
{
    COARSE,  ///< Regelgewinde (ISO-Metrisch)
    FINE     ///< Feingewinde
};

// ============================================================================
// SCHRAUBENDATEN
// ============================================================================

/**
 * @brief Datensatz für eine metrische Schraubengröße.
 *
 * Alle Maße in Millimetern, Drehmoment in Newtonmetern. Bezug: DIN 912 (Zylinderkopfschraube mit Innensechskant), DIN 13 (Gewinde), DIN EN 20273 (Durchgangsbohrung).
 */
struct ScrewData
{
    const char *name;     ///< Bezeichnung wie "M2" oder "M2,5"
    float nominal;        ///< Nenndurchmesser (mm)
    float pitch;          ///< Steigung Regelgewinde (mm)
    float pitchFine;      ///< Steigung Feingewinde (mm), 0 = kein Feingewinde
    float coreHole;       ///< Kernloch-Ø Regelgewinde (mm)
    float coreHoleFine;   ///< Kernloch-Ø Feingewinde (mm), 0 = nicht verfügbar
    float throughHole;    ///< Durchgangsbohrung Ø mittel (mm)
    float keySizeInbus;   ///< Schlüsselweite Innensechskant DIN 912 (mm)
    float keySizeHex;     ///< Schlüsselweite Außensechskant DIN 934 (mm)
    float headHeight;     ///< Kopfhöhe DIN 912 (mm)
    float headDia;        ///< Kopfdurchmesser DIN 912 (mm)
    float torque88;       ///< Anzugsdrehmoment Festigkeitsklasse 8.8 (Nm)
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_ScrewRef
 * @brief Interaktive Schrauben-Referenztabelle mit Seitenansicht und Bemaßung.
 *
 * Zeigt Kernloch-Ø, Durchgangsbohrung, Steigung, Schlüsselweite, Kopfmaße und Drehmoment für metrische Schrauben M2-M12 nach DIN 912.
 *
 * Layout (547 × 327 px):
 *   Oben (48 px): Größenauswahl als Touch-Chips von M2 bis M12
 *   Links (200 px): Seitenansicht mit Bemaßungspfeilen (fixe Größe)
 *   Rechts (335 px): Datentabelle mit hervorgehobenem Kernloch-Ø
 *
 * Footer-Toggle: Metrisch / Fein. Bei Größen ohne Feingewinde wird "kein Feingewinde" angezeigt.
 */
class WorkshopTool_ScrewRef : public WorkshopTool
{
public:
    WorkshopTool_ScrewRef();
    ~WorkshopTool_ScrewRef() override = default;

    WorkshopTool_ScrewRef(const WorkshopTool_ScrewRef &)            = delete;
    WorkshopTool_ScrewRef &operator=(const WorkshopTool_ScrewRef &) = delete;

    void onEnter(LGFX *tft) override;
    void onExit() override;

    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;
    const char *getSubtitle() const override;

private:
    // =========================================================================
    // Innere Typen
    // =========================================================================

    struct Rect
    {
        int x, y, w, h;
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_SIZES = 9;  ///< Anzahl Schraubengrößen (M2-M12)

    // Selector-Chips
    static constexpr int CHIP_H      = 28;  ///< Chip-Höhe
    static constexpr int CHIP_GAP    = 4;   ///< Abstand zwischen Chips
    static constexpr int CHIP_R      = 6;   ///< Eckenradius
    static constexpr int CHIP_TOP    = 10;  ///< Abstand zwischen Kopfbereich und Chips
    static constexpr int CHIP_BOTTOM = 10;  ///< Abstand zwischen Chips und folgendem Inhalt
    static constexpr int SEL_ROW_H   = CHIP_TOP + CHIP_H + CHIP_BOTTOM; ///< Gesamthöhe der Selector-Zone (48 px)

    // Datenzeilen
    static constexpr int DATA_ROW_H = 30; ///< Höhe einer Datenzeile

    // =========================================================================
    // Statische Daten
    // =========================================================================

    static const ScrewData SIZES[NUM_SIZES];

    // =========================================================================
    // Lebenszyklus
    // =========================================================================

    void clearAll();
    void initFooterButtons(LGFX *tft);
    void updateButtonColors();
    void computeLayout(int w, int h);
    void refreshSubtitle();

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /** @brief Zeichnet die Größenauswahl-Chips. */
    void drawSelector();

    /** @brief Zeichnet die Schrauben-Seitenansicht (fixe Größe, Werte variabel). */
    void drawSchematic();

    /**
     * @brief Zeichnet einen horizontalen Bemaßungspfeil.
     * @param y     Y-Position der Bemaßungslinie
     * @param x1    Linke Kante
     * @param x2    Rechte Kante
     * @param extTop Y-Start der Verlängerungslinien (Oberkante)
     * @param extBot Y-Ende der Verlängerungslinien (Unterkante)
     * @param label  Bemaßungstext
     * @param color  Linienfarbe
     * @param above  true = Text über der Linie, false = unter der Linie
     */
    void drawDimH(int y, int x1, int x2, int extTop, int extBot,
                  const char *label, uint16_t color, bool above);

    /**
     * @brief Zeichnet einen vertikalen Bemaßungspfeil.
     * @param x     X-Position der Bemaßungslinie
     * @param y1    Oberkante
     * @param y2    Unterkante
     * @param label Bemaßungstext
     * @param color Linienfarbe
     */
    void drawDimV(int x, int y1, int y2, const char *label, uint16_t color);

    /** @brief Zeichnet die Datentabelle. */
    void drawData();

    /**
     * @brief Zeichnet eine einzelne Datenzeile.
     * @param row Zeilenindex (0-7)
     * @param label     Feldbezeichnung
     * @param value     Wertstring
     * @param highlight true für Kernloch-Hervorhebung
     */
    void drawDataRow(int row, const char *label, const char *value, bool highlight);

    // =========================================================================
    // Touch-Verarbeitung
    // =========================================================================

    int hitTestChip(int cx, int cy) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite  = nullptr;
    int  m_originX        = 0;
    int  m_originY        = 0;
    int  m_w              = 0;
    int  m_h              = 0;
    bool m_layoutValid    = false;

    ThreadMode m_threadMode = ThreadMode::COARSE;
    int m_selectedIdx = 4;  ///< Standardauswahl: M5 (Index 4)

    // Layout
    Rect m_chipRects[NUM_SIZES] = {};  ///< Selector-Chip-Bereiche
    int  m_drawX  = 0;   ///< X-Start des Zeichenbereichs
    int  m_drawW  = 0;   ///< Breite des Zeichenbereichs
    int  m_drawY  = 0;   ///< Y-Start des Zeichenbereichs
    int  m_drawH  = 0;   ///< Höhe des Zeichenbereichs
    int  m_dataX  = 0;   ///< X-Start der Datentabelle
    int  m_dataW  = 0;   ///< Breite der Datentabelle
    int  m_dataY  = 0;   ///< Y-Start der Datentabelle

    // Footer-Buttons
    Button m_btnCoarse;
    Button m_btnFine;
    bool m_buttonsInitialized = false;

    char m_subtitleBuf[64]     = {}; ///< Befüllt von refreshSubtitle(), gelesen von getSubtitle()
    char m_btnCoarseLabel[24]  = {}; ///< Beschriftungspuffer für die Umschaltung auf Regelgewinde
    char m_btnFineLabel[24]    = {}; ///< Beschriftungspuffer für die Umschaltung auf Feingewinde

    // Touch-Tracking
    int  m_pressedChip = -1;
    bool m_touchWas    = false;
    bool m_touchReady  = false;
};

#endif // WORKSHOPTOOL_SCREWREF_H
