#ifndef WORKSHOPTOOL_COLORCONVERTER_H
#define WORKSHOPTOOL_COLORCONVERTER_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_ColorConverter
 * @brief Konvertiert per Schieberegler gewählte RGB-Werte in HEX, RGB565 und eine Farbvorschau.
 *
 * Drei horizontale Schieberegler steuern R, G und B jeweils im Bereich 0-255.
 */
class WorkshopTool_ColorConverter : public WorkshopTool
{
public:
    /**
     * @brief Konstruktor. Ruft clearAll() auf (Startwert R=64, G=64, B=64).
     */
    WorkshopTool_ColorConverter();

    ~WorkshopTool_ColorConverter() override;

    WorkshopTool_ColorConverter(const WorkshopTool_ColorConverter &) = delete;
    WorkshopTool_ColorConverter &operator=(const WorkshopTool_ColorConverter &) = delete;

    void onEnter(LGFX *tft) override;
    void onExit() override;

    /**
     * @brief Zeichnet Slider, Farbvorschau und Ergebnis-Zeile in den Work-Sprite.
     * @param sprite   Work-Sprite des Workshop-Moduls
     * @param originX  Linke Kante des Inhaltsbereichs (sprite-relativ)
     * @param originY  Obere Kante des Inhaltsbereichs (sprite-relativ)
     * @param w        Verfügbare Breite
     * @param h        Verfügbare Höhe
     */
    void draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events für Slider-Drag.
     *
     * Bei erfolgreicher Direktaktualisierung auf dem TFT wird false zurückgegeben, damit Workshop keinen kompletten Content-Redraw anstößt. Bei fehlenden Bereichs-Sprites löst true ein normales Neuzeichnen des Workshops aus.
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    // =========================================================================
    // Vertikales Layout
    // =========================================================================

    static constexpr int NUM_QTY = 3; ///< Anzahl Kanäle (R, G, B)

    static constexpr int TOP_GAP = 10;                              ///< Oberer Abstand in Pixeln (Workshop-Konvention)
    static constexpr int SLIDER_H = 54;                             ///< Höhe eines Slider-Elements in Pixeln
    static constexpr int SLIDER_GAP = 4;                            ///< Abstand zwischen zwei Slidern in Pixeln
    static constexpr int SWATCH_GAP = 10;                           ///< Abstand zwischen letztem Schieberegler und Farbvorschau
    static constexpr int SWATCH_COLOR_H = 84;                       ///< Höhe der Farbfläche in Pixeln
    static constexpr int SWATCH_CORNER_R = 8;                       ///< Eckenradius der Farbvorschau in Pixeln
    static constexpr int RESULT_GAP = 10;                           ///< Abstand zwischen Vorschau und Ergebnis
    static constexpr int RESULT_HEADER_H = 18;                      ///< Höhe der Ergebnis-Kopfzeile in Pixeln
    static constexpr int RESULT_VAL_H = 28;                         ///< Höhe der Ergebnis-Wertezeile in Pixeln
    static constexpr int RESULT_H = RESULT_HEADER_H + RESULT_VAL_H; ///< Gesamthöhe des Ergebnisbereichs in Pixeln

    // =========================================================================
    // Internes Layout der Schieberegler
    // =========================================================================

    static constexpr int SLIDER_MID = SLIDER_H / 2;
    static constexpr int TRACK_H = 20;
    static constexpr int TRACK_TOP = SLIDER_MID - TRACK_H / 2;
    static constexpr int THUMB_W = 14;

    // =========================================================================
    // Horizontales Layout relativ zum Inhaltsbereich
    // =========================================================================

    static constexpr int SYM_X = 8;
    static constexpr int SYM_W = 20;
    static constexpr int TRACK_LEFT_X = SYM_X + SYM_W + 8;
    static constexpr int VAL_W = 36;
    static constexpr int VAL_GAP = 6;

    // =========================================================================
    // Touch-Verarbeitung und Aktualisierungstakt
    // =========================================================================

    static constexpr int TOUCH_SMOOTH_OLD_WEIGHT = 3; ///< 3/5 alt
    static constexpr int TOUCH_SMOOTH_NEW_WEIGHT = 2; ///< 2/5 neu

    static constexpr uint32_t AUX_PUSH_MS = 120; ///< Swatch/Result max. ~8 Hz während Drag

    // =========================================================================
    // Farben im RGB565-Format
    // =========================================================================

    static constexpr uint16_t C_BG = 0x1082;          ///< Arbeitsbereich-Hintergrund
    static constexpr uint16_t C_SLIDER_BG = 0x2945;   ///< Slider-Hintergrund
    static constexpr uint16_t C_TRACK_EMPTY = 0x1082; ///< Track-Hintergrund

    static constexpr uint16_t C_LBL_ACTIVE = 0xFFFF; ///< Slider-Label (R/G/B) bei Aktivität
    static constexpr uint16_t C_LBL_NORMAL = 0xDEDB; ///< Slider-Label (R/G/B) im Normalzustand
    static constexpr uint16_t C_VAL_ACTIVE = 0xFFFF; ///< Slider-Wert bei Aktivität
    static constexpr uint16_t C_VAL_NORMAL = 0xEF7D; ///< Slider-Wert im Normalzustand

    static constexpr uint16_t C_CH_R = 0xF800; ///< Farb-Akzent für Rot-Kanal (Slider-Füllung bei Aktivität)
    static constexpr uint16_t C_CH_G = 0x07E0; ///< Farb-Akzent für Grün-Kanal (Slider-Füllung bei Aktivität)
    static constexpr uint16_t C_CH_B = 0x001F; ///< Farb-Akzent für Blau-Kanal (Slider-Füllung bei Aktivität)

    static constexpr uint16_t C_THUMB_ACTIVE = 0xFFFF; ///< Slider-Thumb bei Aktivität
    static constexpr uint16_t C_THUMB_NORMAL = 0xBDD7; ///< Slider-Thumb im Normalzustand

    // Swatch-Textfarben: hell/dunkel per getIntensityFrom565() zur Laufzeit gewählt
    static constexpr uint16_t C_SWATCH_TXT_LIGHT = 0xFFFF; ///< Wert-Text auf dunkler Farbfläche
    static constexpr uint16_t C_SWATCH_TXT_DARK = 0x0000;  ///< Wert-Text auf heller Farbfläche
    static constexpr uint16_t C_SWATCH_LBL_LIGHT = 0xBDF7; ///< Label-Text (gedimmt) auf dunkler Fläche
    static constexpr uint16_t C_SWATCH_LBL_DARK = 0x39E8;  ///< Label-Text (gedimmt) auf heller Fläche

    static constexpr uint16_t C_RESULT_KEY = 0x8410; ///< Ergebnis-Kopfzeile
    static constexpr uint16_t C_RESULT_NUM = 0xEF7D; ///< Ergebnis-Zahlenwerte

    // =========================================================================
    // Statische Datenbasis
    // =========================================================================

    static const char *const QTY_SYMBOLS[NUM_QTY];

    // =========================================================================
    // Private Methoden
    // =========================================================================

    void clearAll();
    void markAllDirty();
    void markSliderDirty(int qty);
    bool hasDirty() const;

    void computeLayout(int w, int h);
    void compute();

    static uint16_t toRgb565(uint8_t r, uint8_t g, uint8_t b);
    static uint16_t channelColor(int qty);
    int sliderValueFromX(int displayX) const;

    void drawSliders();
    void drawSlider(int qty);
    void drawSwatch();
    void drawResult();

    void renderSliderTo(Gfx_Sprite *spr, int qty, int baseX, int baseY, int width);
    void renderSwatchTo(Gfx_Sprite *spr, int baseX, int baseY, int width);
    void renderResultTo(Gfx_Sprite *spr, int baseX, int baseY, int width);

    bool ensureFastSprites();
    void destroyFastSprites();
    bool directFlushToDisplay(bool forceAux);

    int hitTestSlider(int cy) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    LGFX *m_tft = nullptr;          ///< Direktes TFT-Ziel für schnelle Teilupdates
    Gfx_Sprite *m_sprite = nullptr; ///< Persistenter Work-Sprite von Workshop
    int m_originX = 0;
    int m_originY = 0;
    int m_w = 0;
    int m_h = 0;
    bool m_layoutValid = false;

    int m_values[NUM_QTY] = {};

    uint16_t m_rgb565 = 0;
    char m_hexStr[8] = {};
    uint8_t m_rBack = 0;
    uint8_t m_gBack = 0;
    uint8_t m_bBack = 0;

    int m_sliderY[NUM_QTY] = {};
    int m_swatchY = 0;
    int m_resultY = 0;
    int m_trackW = 0;

    int m_dragSlider = -1;
    bool m_touchWas = false;
    int m_filteredTouchX = 0;
    bool m_touchFilterValid = false;

    bool m_forceFullRedraw = true;
    uint8_t m_dirtySliders = 0x07;
    bool m_dirtySwatch = true;
    bool m_dirtyResult = true;
    uint32_t m_lastAuxPushMs = 0;

    // Kleine Zone-Sprites für direkte TFT-Updates während Drag.
    Gfx_Sprite *m_sliderSprites[NUM_QTY] = {nullptr, nullptr, nullptr};
    Gfx_Sprite *m_swatchSprite = nullptr;
    Gfx_Sprite *m_resultSprite = nullptr;
    bool m_fastSpritesReady = false;
};

#endif // WORKSHOPTOOL_COLORCONVERTER_H
