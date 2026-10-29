#include "WorkshopTool_CircleOfFifths.h"

#include <cmath>
#include <cstring>
#include <pgmspace.h>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold30.h"

// ===================================================================
// Anonymer Namespace - file-local Helfer
// ===================================================================

namespace
{
    constexpr float DEG_TO_RAD_FACTOR = 0.017453292f;     ///< Pi / 180
    constexpr float RAD_FULL_TURN = 6.283185307f;         ///< Vollkreis in Bogenmaß
    constexpr float SEPARATOR_LINE_WIDTH = 1.2f;          ///< Breite der radialen Trennlinien
    constexpr float SEPARATOR_INSET_OUTER = 1.8f;         ///< Abstand der Trennlinien zur äußeren Ringkante
    constexpr float SEPARATOR_INSET_INNER = 0.8f;         ///< Abstand der Trennlinien zur inneren Ringkante

    constexpr float GEOM_CENTER_DX[12] = {
        0.000000000f, 0.500000000f, 0.866025404f, 1.000000000f,
        0.866025404f, 0.500000000f, 0.000000000f, -0.500000000f,
        -0.866025404f, -1.000000000f, -0.866025404f, -0.500000000f}; ///< X-Anteil der Sektor-Mittelachsen

    constexpr float GEOM_CENTER_DY[12] = {
        -1.000000000f, -0.866025404f, -0.500000000f, -0.000000000f,
        0.500000000f, 0.866025404f, 1.000000000f, 0.866025404f,
        0.500000000f, 0.000000000f, -0.500000000f, -0.866025404f}; ///< Y-Anteil der Sektor-Mittelachsen

    constexpr float GEOM_START_DX[12] = {
        -0.258819045f, 0.258819045f, 0.707106781f, 0.965925826f,
        0.965925826f, 0.707106781f, 0.258819045f, -0.258819045f,
        -0.707106781f, -0.965925826f, -0.965925826f, -0.707106781f}; ///< X-Anteil der linken Sektorkante

    constexpr float GEOM_START_DY[12] = {
        -0.965925826f, -0.965925826f, -0.707106781f, -0.258819045f,
        0.258819045f, 0.707106781f, 0.965925826f, 0.965925826f,
        0.707106781f, 0.258819045f, -0.258819045f, -0.707106781f}; ///< Y-Anteil der linken Sektorkante

    constexpr float GEOM_END_DX[12] = {
        0.258819045f, 0.707106781f, 0.965925826f, 0.965925826f,
        0.707106781f, 0.258819045f, -0.258819045f, -0.707106781f,
        -0.965925826f, -0.965925826f, -0.707106781f, -0.258819045f}; ///< X-Anteil der rechten Sektorkante

    constexpr float GEOM_END_DY[12] = {
        -0.965925826f, -0.707106781f, -0.258819045f, 0.258819045f,
        0.707106781f, 0.965925826f, 0.965925826f, 0.707106781f,
        0.258819045f, -0.258819045f, -0.707106781f, -0.965925826f}; ///< Y-Anteil der rechten Sektorkante

    /// Reihenfolge der Kreuz-Vorzeichen, beginnend bei einem Kreuz.
    const char *const SHARP_ORDER[7] = {"fis", "cis", "gis", "dis", "ais", "eis", "his"};

    /// Reihenfolge der B-Vorzeichen, beginnend bei einem b.
    const char *const FLAT_ORDER[7] = {"b", "es", "as", "des", "ges", "ces", "fes"};

    /**
     * @brief Begrenzt einen Wert auf ein geschlossenes Intervall.
     */
    inline int clampi(int v, int lo, int hi)
    {
        if (v < lo)
            return lo;

        if (v > hi)
            return hi;

        return v;
    }
} // namespace

// ===================================================================
// Statische Datenbasis - Tonarten
// ===================================================================

const WorkshopTool_CircleOfFifths::KeyData WorkshopTool_CircleOfFifths::KEYS[NUM_KEYS] = {
    // Position 0 (12 Uhr): C-Dur, keine Vorzeichen
    {"C", "a", 0, {"c", "d", "e", "f", "g", "a", "h"}},
    // Position 1: G-Dur, 1 Kreuz
    {"G", "e", 1, {"g", "a", "h", "c", "d", "e", "fis"}},
    // Position 2: D-Dur, 2 Kreuze
    {"D", "h", 2, {"d", "e", "fis", "g", "a", "h", "cis"}},
    // Position 3: A-Dur, 3 Kreuze
    {"A", "fis", 3, {"a", "h", "cis", "d", "e", "fis", "gis"}},
    // Position 4: E-Dur, 4 Kreuze
    {"E", "cis", 4, {"e", "fis", "gis", "a", "h", "cis", "dis"}},
    // Position 5: H-Dur, 5 Kreuze
    {"H", "gis", 5, {"h", "cis", "dis", "e", "fis", "gis", "ais"}},
    // Position 6 (6 Uhr): Fis-Dur, 6 Kreuze (enharmonisch Ges-Dur, 6 Bs)
    {"Fis", "dis", 6, {"fis", "gis", "ais", "h", "cis", "dis", "eis"}},
    // Position 7: Des-Dur, 5 Bs
    {"Des", "b", -5, {"des", "es", "f", "ges", "as", "b", "c"}},
    // Position 8: As-Dur, 4 Bs
    {"As", "f", -4, {"as", "b", "c", "des", "es", "f", "g"}},
    // Position 9: Es-Dur, 3 Bs
    {"Es", "c", -3, {"es", "f", "g", "as", "b", "c", "d"}},
    // Position 10: B-Dur, 2 Bs
    {"B", "g", -2, {"b", "c", "d", "es", "f", "g", "a"}},
    // Position 11: F-Dur, 1 b
    {"F", "d", -1, {"f", "g", "a", "b", "c", "d", "e"}},
};

// ===================================================================
// Statische Datenbasis - Stufenharmonik in Dur
// ===================================================================

const WorkshopTool_CircleOfFifths::ChordQuality
    WorkshopTool_CircleOfFifths::DEGREE_QUALITY[NUM_DEGREES] = {
        ChordQuality::MAJOR, // I
        ChordQuality::MINOR, // ii
        ChordQuality::MINOR, // iii
        ChordQuality::MAJOR, // IV
        ChordQuality::MAJOR, // V
        ChordQuality::MINOR, // vi
};

const char *const WorkshopTool_CircleOfFifths::DEGREE_LABELS[NUM_DEGREES] = {
    "I", "ii", "iii", "IV", "V", "vi"};

const WorkshopKey WorkshopTool_CircleOfFifths::FUNCTION_LABEL_KEYS[NUM_DEGREES] = {
    WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FUNC_TONIC,
    WorkshopKey::WORKSHOP_EMPTY,
    WorkshopKey::WORKSHOP_EMPTY,
    WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FUNC_SUBDOMINANT,
    WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FUNC_DOMINANT,
    WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FUNC_PARALLEL};

/// Tonleiter-Index der Stufe (Stufe N -> Tonleiterton scale[N-1]).
const int WorkshopTool_CircleOfFifths::DEGREE_OFFSET[NUM_DEGREES] = {0, 1, 2, 3, 4, 5};

/// Position-Delta zur Tonika im Quintenzirkel je Stufe.
const int WorkshopTool_CircleOfFifths::POSITION_OFFSET[NUM_DEGREES] = {
    0,  // I:   Tonika selbst
    -1, // ii:  Parallele der Subdominante
    +1, // iii: Parallele der Dominante
    -1, // IV:  Subdominante (Quart abwärts)
    +1, // V:   Dominante (Quint aufwärts)
    0,  // vi:  Tonika-Parallele
};

// ===================================================================
// Konstruktor / Destruktor
// ===================================================================

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte.
 */
WorkshopTool_CircleOfFifths::WorkshopTool_CircleOfFifths()
{
    clearAll();
}

/**
 * @brief Gibt den Output-Cache-Sprite frei, falls onExit nicht aufgerufen wurde.
 */
WorkshopTool_CircleOfFifths::~WorkshopTool_CircleOfFifths()
{
    releaseOutputCache();
}

// ===================================================================
// Lebenszyklus
// ===================================================================

/**
 * @brief Übernimmt die TFT-Referenz und setzt die Auswahl auf C-Dur zurück.
 */
void WorkshopTool_CircleOfFifths::onEnter(LGFX *tft)
{
    releaseOutputCache();
    clearAll();
    m_tft = tft;
}

/**
 * @brief Gibt den Output-Cache-Sprite beim Verlassen des Werkzeugs frei.
 */
void WorkshopTool_CircleOfFifths::onExit()
{
    releaseOutputCache();
    m_tft = nullptr;
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte ohne Sprite-Freigabe.
 */
void WorkshopTool_CircleOfFifths::clearAll()
{
    m_tft = nullptr;
    m_sprite = nullptr;
    m_outputCache = nullptr;

    m_originX = 0;
    m_originY = 0;
    m_w = 0;
    m_h = 0;
    m_layoutValid = false;
    m_outputCacheValid = false;
    m_outputCacheKey = 0;

    m_circleCx = 0;
    m_circleCy = 0;
    m_panelX = 0;
    m_panelW = 0;

    m_selectedKey = 0;

    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));

    m_pressedKey = -1;
    m_touchWas = false;
    m_touchReady = false;
}

/**
 * @brief Gibt den Output-Cache-Sprite frei und markiert ihn als ungültig.
 */
void WorkshopTool_CircleOfFifths::releaseOutputCache()
{
    if (m_tft)
        m_tft->safeDeleteSprite(m_outputCache);
    else
        m_outputCache = nullptr;

    m_outputCacheValid = false;
}

// ===================================================================
// Layout
// ===================================================================

/**
 * @brief Berechnet Kreismittelpunkt und Panel-Koordinaten.
 */
void WorkshopTool_CircleOfFifths::computeLayout(int w, int h)
{
    m_circleCx = CIRCLE_W / 2;
    m_circleCy = h / 2;

    m_panelX = CIRCLE_W + COL_GAP;
    m_panelW = w - m_panelX;
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet Quintenzirkel und Info-Spalte in den Work-Sprite.
 *
 * Push des Output-Caches in den Work-Sprite, danach Press-Overlay direkt im Work-Sprite. Wenn der Cache nicht zur aktuellen Auswahl passt oder beim Layout-Wechsel ungültig wurde, wird er vorher neu aufgebaut. Schlägt die Cache-Allokation fehl, rendert die Methode das Bild direkt in den Work-Sprite ohne Cache-Pfad.
 */
void WorkshopTool_CircleOfFifths::draw(Gfx_Sprite *sprite, int originX, int originY,
                                       int w, int h)
{
    m_sprite = sprite;
    m_originX = originX;
    m_originY = originY;

    if (!m_layoutValid || m_w != w || m_h != h)
    {
        m_w = w;
        m_h = h;
        computeLayout(w, h);
        m_layoutValid = true;
        releaseOutputCache();
    }

    if (!m_outputCacheValid || m_outputCacheKey != m_selectedKey)
        buildOutputCache();

    if (m_outputCacheValid && m_outputCache)
    {
        m_outputCache->pushSprite(m_sprite, m_originX, m_originY);
    }
    else
    {
        // Fallback: direkt in den Work-Sprite rendern
        m_sprite->fillRect(m_originX, m_originY, m_w, m_h, C_BG);
        renderFullSceneNoPress();
    }

    drawPressOverlay();
}

// ===================================================================
// Cache-Aufbau
// ===================================================================

/**
 * @brief Erstellt den Output-Cache und rendert das komplette Tool-Bild ohne Press-Overlay hinein.
 */
void WorkshopTool_CircleOfFifths::buildOutputCache()
{
    if (!m_tft)
        return;

    if (!m_outputCache)
    {
        if (!m_tft->safeCreateSprite(m_outputCache, m_w, m_h, true))
        {
            m_outputCacheValid = false;
            return;
        }
    }

    m_outputCache->clear(C_BG);

    Gfx_Sprite *const savedSprite = m_sprite;
    const int savedOriginX = m_originX;
    const int savedOriginY = m_originY;

    m_sprite = m_outputCache;
    m_originX = 0;
    m_originY = 0;

    renderFullSceneNoPress();

    m_sprite = savedSprite;
    m_originX = savedOriginX;
    m_originY = savedOriginY;

    m_outputCacheValid = true;
    m_outputCacheKey = m_selectedKey;
}

/**
 * @brief Rendert das komplette Tool-Bild ohne Press-Overlay in das aktuelle m_sprite.
 *
 * Phase 1 ohne Fonts: Ringgeometrie, Akzent-Sektoren, Trennlinien, Mittelfläche-Hintergrund, Info-Panel-Hintergrund. Phase 2 mit ComfortaaBold16: Default-Dur-Labels und akzentuierte Dur-Labels und Info-Header. Phase 3 mit ComfortaaBold14: Vorzeichen-Marker, Default-Moll-Labels, akzentuierte Moll-Labels, Disc-Suffix und Info-Body. Phase 4 mit ComfortaaBold30: Disc-Hauptlabel.
 */
void WorkshopTool_CircleOfFifths::renderFullSceneNoPress()
{
    drawRingsBackground();
    drawRingsBorders();
    drawAccentSectors();
    drawRingSeparators();

    m_sprite->loadFont(ComfortaaBold16);
    drawDurLabelsDefault();
    drawDurLabelsAccented();
    drawInfoPanelHeader();
    m_sprite->unloadFont();

    m_sprite->loadFont(ComfortaaBold14);
    drawAccidentalMarkers();
    drawMollLabelsDefault();
    drawMollLabelsAccented();
    drawInnerDiscSuffix();
    drawInfoPanelBody();
    m_sprite->unloadFont();

    m_sprite->loadFont(ComfortaaBold30);
    drawInnerDiscMain();
    m_sprite->unloadFont();
}

// ===================================================================
// Body-only-Renderer - Geometrie und Sektoren (font-frei)
// ===================================================================

/**
 * @brief Zeichnet die zwei Ring-Vollflächen als zwei drawSmoothGauge-Aufrufe.
 */
void WorkshopTool_CircleOfFifths::drawRingsBackground()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;

    const float thicknessDur = static_cast<float>(RADIUS_DUR_OUTER - RADIUS_DUR_INNER);
    const float thicknessMoll = static_cast<float>(RADIUS_MOLL_OUTER - RADIUS_MOLL_INNER);

    spr->drawSmoothGauge(
        xc,                                   ///< X-Koordinate des Mittelpunkts
        yc,                                   ///< Y-Koordinate des Mittelpunkts
        static_cast<float>(RADIUS_DUR_OUTER), ///< Äußerer Radius
        thicknessDur,                         ///< Dicke des Rings
        0.0f,                                 ///< Startwinkel in Grad, 0 Grad liegt oben und positive Werte laufen im Uhrzeigersinn
        FULL_TURN_DEG,                        ///< Endwinkel in Grad
        0.0f,                                 ///< Minimaler Wertebereich (hier 0.0f, da wir den Ring komplett füllen wollen)
        1.0f,                                 ///< Aktueller Wert innerhalb von valueMin bis valueMax (hier volle Füllung)
        1.0f,                                 ///< Maximaler Wertebereich (hier 1.0f, da wir den Ring komplett füllen wollen)
        C_RING_BG_DUR,                        ///< Farbe des gefüllten Kreissegments im RGB565-Format
        C_RING_BG_DUR,                        ///< Farbe der Spur außerhalb des aktuellen Werts im RGB565-Format (hier gleich, da wir den Ring komplett füllen)
        false,                                ///< trackVisible true, wenn die Spur angezeigt werden soll (hier false, da wir den Ring komplett füllen)
        C_BG                                  ///< Hintergrundfarbe im RGB565-Format
    );

    spr->drawSmoothGauge(
        xc,                                    ///< X-Koordinate des Mittelpunkts
        yc,                                    ///< Y-Koordinate des Mittelpunkts
        static_cast<float>(RADIUS_MOLL_OUTER), ///< Äußerer Radius
        thicknessMoll,                         ///< Dicke des Rings
        0.0f,                                  ///< Startwinkel in Grad, 0 Grad liegt oben und positive Werte laufen im Uhrzeigersinn
        FULL_TURN_DEG,                         ///< Endwinkel in Grad
        0.0f,                                  ///< Minimaler Wertebereich (hier 0.0f, da wir den Ring komplett füllen wollen)
        1.0f,                                  ///< Aktueller Wert innerhalb von valueMin bis valueMax (hier volle Füllung)
        1.0f,                                  ///< Maximaler Wertebereich (hier 1.0f, da wir den Ring komplett füllen wollen)
        C_RING_BG_MOLL,                        ///< Farbe des gefüllten Kreissegments im RGB565-Format
        C_RING_BG_MOLL,                        ///< Farbe der Spur außerhalb des aktuellen Werts im RGB565-Format (hier gleich, da wir den Ring komplett füllen)
        false,                                 ///< trackVisible true, wenn die Spur angezeigt werden soll (hier false, da wir den Ring komplett füllen)
        C_BG                                   ///< Hintergrundfarbe im RGB565-Format
    );
}

/**
 * @brief Zeichnet die drei konzentrischen Ringgrenzen.
 */
void WorkshopTool_CircleOfFifths::drawRingsBorders()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;

    spr->drawSmoothCircle(
        xc,                                   ///< X-Koordinate des Mittelpunkts
        yc,                                   ///< Y-Koordinate des Mittelpunkts
        static_cast<float>(RADIUS_DUR_OUTER), ///< Radius
        2.0f,                                 ///< Dicke der Kreislinie
        C_RING_OUTLINE,                       ///<
        C_BG                                  ///<
    );

    spr->drawSmoothCircle(
        xc,                                   ///< X-Koordinate des Mittelpunkts
        yc,                                   ///< Y-Koordinate des Mittelpunkts
        static_cast<float>(RADIUS_DUR_INNER), ///< Radius
        2.0f,                                 ///< Dicke der Kreislinie
        C_RING_OUTLINE,                       ///< Farbe der Kreislinie im RGB565-Format
        C_RING_BG_DUR                         ///< Hintergrundfarbe im RGB565-Format
    );

    spr->drawSmoothCircle(
        xc,                                    ///< X-Koordinate des Mittelpunkts
        yc,                                    ///< Y-Koordinate des Mittelpunkts
        static_cast<float>(RADIUS_MOLL_INNER), ///< Radius
        2.0f,                                  ///< Dicke der Kreislinie
        C_RING_OUTLINE,                        ///< Farbe der Kreislinie im RGB565-Format
        C_RING_BG_MOLL                         ///< Hintergrundfarbe im RGB565-Format
    );
}

/**
 * @brief Zeichnet die akzentuierten Sektoren beider Ringe.
 */
void WorkshopTool_CircleOfFifths::drawAccentSectors()
{
    const int selPos = static_cast<int>(m_selectedKey);

    // Außenring: drei Hauptfunktionen mit individuellen Farben
    for (int delta = -1; delta <= 1; ++delta)
    {
        const uint16_t accent = functionAccent(delta);
        if (accent == 0)
            continue;

        const int pos = wrapPos(selPos + delta);
        drawRingSector(pos, RADIUS_DUR_INNER, RADIUS_DUR_OUTER, accent);
    }

    // Innenring: alle drei Mollparallelen einheitlich in C_ACCENT_PARALLEL
    for (int delta = -1; delta <= 1; ++delta)
    {
        const int pos = wrapPos(selPos + delta);
        drawRingSector(pos, RADIUS_MOLL_INNER, RADIUS_MOLL_OUTER, C_ACCENT_PARALLEL);
    }
}

/**
 * @brief Zeichnet die zwölf radialen Trennlinien zwischen den Sektoren.
 */
void WorkshopTool_CircleOfFifths::drawRingSeparators()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;

    for (int p = 0; p < NUM_KEYS; ++p)
    {
        const float dx = GEOM_START_DX[p];
        const float dy = GEOM_START_DY[p];

        const float innerR = static_cast<float>(RADIUS_MOLL_INNER) + SEPARATOR_INSET_INNER;
        const float outerR = static_cast<float>(RADIUS_DUR_OUTER) - SEPARATOR_INSET_OUTER;

        const float x1 = xc + innerR * dx;
        const float y1 = yc + innerR * dy;
        const float x2 = xc + outerR * dx;
        const float y2 = yc + outerR * dy;

        spr->drawWideLine(x1, y1, x2, y2, SEPARATOR_LINE_WIDTH, C_RING_SEPARATOR);
    }
}

/**
 * @brief Zeichnet das Press-Overlay für den aktuell gedrückten Sektor.
 */
void WorkshopTool_CircleOfFifths::drawPressOverlay()
{
    if (m_pressedKey < 0 || m_pressedKey >= NUM_KEYS)
        return;

    drawRingSector(m_pressedKey, RADIUS_MOLL_INNER, RADIUS_MOLL_OUTER, C_PRESS_OVERLAY);
    drawRingSector(m_pressedKey, RADIUS_DUR_INNER, RADIUS_DUR_OUTER, C_PRESS_OVERLAY);
}

// ===================================================================
// Body-only-Renderer - Text (Font-Vorbedingung im jeweiligen @brief)
// ===================================================================

/**
 * @brief Zeichnet die Vorzeichen-Marker am Außenrand des Kreises.
 */
void WorkshopTool_CircleOfFifths::drawAccidentalMarkers()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;

    spr->setTextDatum(MC_DATUM);

    char buf[8];

    for (int p = 0; p < NUM_KEYS; ++p)
    {
        const int8_t acc = KEYS[p].acc;
        if (acc == 0)
            continue;

        const int x = xc + static_cast<int>(static_cast<float>(RADIUS_ACCIDENTAL) * GEOM_CENTER_DX[p]);
        const int y = yc + static_cast<int>(static_cast<float>(RADIUS_ACCIDENTAL) * GEOM_CENTER_DY[p]);

        if (acc > 0)
        {
            stringutils::safeFormat(buf, sizeof(buf), "%d#", static_cast<int>(acc));
            spr->setTextColor(C_ACCIDENTAL_SHARP);
        }
        else
        {
            stringutils::safeFormat(buf, sizeof(buf), "%db", -static_cast<int>(acc));
            spr->setTextColor(C_ACCIDENTAL_FLAT);
        }

        spr->drawString(buf, x, y);
    }
}

/**
 * @brief Zeichnet alle 12 Tonart-Beschriftungen mit Default-Hintergrund.
 */
void WorkshopTool_CircleOfFifths::drawDurLabelsDefault()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;

    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_TEXT_PRIMARY);

    const int leftPos = wrapPos(static_cast<int>(m_selectedKey) - 1);
    const int midPos = static_cast<int>(m_selectedKey);
    const int rightPos = wrapPos(static_cast<int>(m_selectedKey) + 1);

    for (int p = 0; p < NUM_KEYS; ++p)
    {
        if (p == leftPos || p == midPos || p == rightPos)
            continue;

        const int x = xc + static_cast<int>(static_cast<float>(RADIUS_DUR_LABEL) * GEOM_CENTER_DX[p]);
        const int y = yc + static_cast<int>(static_cast<float>(RADIUS_DUR_LABEL) * GEOM_CENTER_DY[p]);
        spr->drawString(KEYS[p].major, x, y);
    }
}

/**
 * @brief Zeichnet alle 12 Moll-Beschriftungen mit Default-Hintergrund.
 */
void WorkshopTool_CircleOfFifths::drawMollLabelsDefault()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;

    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_TEXT_DIM);

    char buf[CHORD_BUF_CAP];

    const int leftPos = wrapPos(static_cast<int>(m_selectedKey) - 1);
    const int midPos = static_cast<int>(m_selectedKey);
    const int rightPos = wrapPos(static_cast<int>(m_selectedKey) + 1);

    for (int p = 0; p < NUM_KEYS; ++p)
    {
        if (p == leftPos || p == midPos || p == rightPos)
            continue;

        const int x = xc + static_cast<int>(static_cast<float>(RADIUS_MOLL_LABEL) * GEOM_CENTER_DX[p]);
        const int y = yc + static_cast<int>(static_cast<float>(RADIUS_MOLL_LABEL) * GEOM_CENTER_DY[p]);
        stringutils::safeFormat(buf, sizeof(buf), "%sm", KEYS[p].minor);
        spr->drawString(buf, x, y);
    }
}

/**
 * @brief Zeichnet die drei akzentuierten Tonart-Beschriftungen.
 */
void WorkshopTool_CircleOfFifths::drawDurLabelsAccented()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;
    const int selPos = static_cast<int>(m_selectedKey);

    spr->setTextDatum(MC_DATUM);

    for (int delta = -1; delta <= 1; ++delta)
    {
        const uint16_t accent = functionAccent(delta);
        if (accent == 0)
            continue;

        const int p = wrapPos(selPos + delta);
        const int x = xc + static_cast<int>(static_cast<float>(RADIUS_DUR_LABEL) * GEOM_CENTER_DX[p]);
        const int y = yc + static_cast<int>(static_cast<float>(RADIUS_DUR_LABEL) * GEOM_CENTER_DY[p]);

        spr->setTextColor(C_TEXT_INVERSE);
        spr->drawString(KEYS[p].major, x, y);
    }
}

/**
 * @brief Zeichnet die drei akzentuierten Moll-Beschriftungen.
 */
void WorkshopTool_CircleOfFifths::drawMollLabelsAccented()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;
    const int selPos = static_cast<int>(m_selectedKey);

    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_TEXT_INVERSE);

    char buf[CHORD_BUF_CAP];

    for (int delta = -1; delta <= 1; ++delta)
    {
        const int p = wrapPos(selPos + delta);
        const int x = xc + static_cast<int>(static_cast<float>(RADIUS_MOLL_LABEL) * GEOM_CENTER_DX[p]);
        const int y = yc + static_cast<int>(static_cast<float>(RADIUS_MOLL_LABEL) * GEOM_CENTER_DY[p]);
        stringutils::safeFormat(buf, sizeof(buf), "%sm", KEYS[p].minor);
        spr->drawString(buf, x, y);
    }
}

/**
 * @brief Zeichnet den Tonart-Großtext in die Mittelfläche.
 */
void WorkshopTool_CircleOfFifths::drawInnerDiscMain()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;

    const KeyData &k = KEYS[m_selectedKey];

    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_TEXT_PRIMARY);
    spr->drawString(k.major, xc, yc + INNER_DISC_TONIC_DY);
}

/**
 * @brief Zeichnet das Suffix "Dur" unterhalb des Großtextes.
 */
void WorkshopTool_CircleOfFifths::drawInnerDiscSuffix()
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;

    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(C_TEXT_DIM);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_SUFFIX_MAJOR), xc, yc + INNER_DISC_SUFFIX_DY);
}

/**
 * @brief Zeichnet die Header-Region der Info-Spalte.
 */
void WorkshopTool_CircleOfFifths::drawInfoPanelHeader()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_panelX;
    const int oy = m_originY;

    const KeyData &k = KEYS[m_selectedKey];

    spr->setTextDatum(TL_DATUM);

    char headBuf[HEAD_BUF_CAP];
    I18n::format(headBuf, sizeof(headBuf), WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FMT_HEADER_KEY, k.major);
    spr->setTextColor(C_TEXT_PRIMARY);
    spr->drawString(headBuf, ox + RIGHT_PAD_X, oy + RIGHT_HEAD_Y);

    char accBuf[DETAIL_BUF_CAP];
    formatAccidentalDetail(static_cast<int>(m_selectedKey), accBuf, sizeof(accBuf));
    spr->setTextColor(C_TEXT_DIM);
    spr->drawString(accBuf, ox + RIGHT_PAD_X, oy + RIGHT_ACC_Y);
}

/**
 * @brief Zeichnet die Body-Region der Info-Spalte.
 */
void WorkshopTool_CircleOfFifths::drawInfoPanelBody()
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_panelX;
    const int oy = m_originY;

    const KeyData &k = KEYS[m_selectedKey];

    spr->setTextDatum(TL_DATUM);

    spr->setTextColor(C_TEXT_LABEL);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_HEADER_SCALE), ox + RIGHT_PAD_X, oy + RIGHT_SCALE_HDR_Y);

    char scaleBuf[SCALE_BUF_CAP];
    scaleBuf[0] = '\0';
    for (int i = 0; i < SCALE_LEN; ++i)
    {
        if (i > 0)
            stringutils::safeAppend(scaleBuf, sizeof(scaleBuf), " ");
        stringutils::safeAppend(scaleBuf, sizeof(scaleBuf), k.scale[i]);
    }

    spr->setTextColor(C_TEXT_PRIMARY);
    spr->drawString(scaleBuf, ox + RIGHT_PAD_X, oy + RIGHT_SCALE_Y);

    spr->setTextColor(C_TEXT_LABEL);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_HEADER_DEGREES), ox + RIGHT_PAD_X, oy + RIGHT_DEG_HDR_Y);

    for (int i = 0; i < NUM_DEGREES; ++i)
    {
        const int yPos = oy + RIGHT_DEG_Y + i * RIGHT_DEG_LINE_H;
        drawDegreeRow(i, yPos);
    }
}

/**
 * @brief Zeichnet eine einzelne Stufenzeile (Roman, Akkord, Funktion).
 */
void WorkshopTool_CircleOfFifths::drawDegreeRow(int row, int yPos)
{
    Gfx_Sprite *const spr = m_sprite;
    const int ox = m_originX + m_panelX;

    const KeyData &k = KEYS[m_selectedKey];
    const int toneIdx = DEGREE_OFFSET[row];
    const ChordQuality q = DEGREE_QUALITY[row];

    char chordBuf[CHORD_BUF_CAP];
    formatChord(k.scale[toneIdx], q, chordBuf, sizeof(chordBuf));

    const int posDelta = POSITION_OFFSET[row];
    uint16_t accent = functionAccent(posDelta);
    if (accent == 0)
        accent = (q == ChordQuality::MINOR) ? C_TEXT_DIM : C_TEXT_PRIMARY;

    spr->setTextDatum(TL_DATUM);

    spr->setTextColor(accent);
    spr->drawString(DEGREE_LABELS[row], ox + DEG_COL_ROMAN_X, yPos);

    spr->setTextColor(C_TEXT_PRIMARY);
    spr->drawString(chordBuf, ox + DEG_COL_CHORD_X, yPos);

    if (FUNCTION_LABEL_KEYS[row] != WorkshopKey::WORKSHOP_EMPTY)
    {
        spr->setTextColor(C_TEXT_DIM);
        spr->drawString(I18n::str(FUNCTION_LABEL_KEYS[row]), ox + DEG_COL_FUNC_X, yPos);
    }
}

// ===================================================================
// Geometrie und Treffertest
// ===================================================================

/**
 * @brief Füllt einen Sektor zwischen zwei Radien mit einer Farbe.
 */
void WorkshopTool_CircleOfFifths::drawRingSector(int pos, int innerR, int outerR,
                                                 uint16_t fillColor)
{
    Gfx_Sprite *const spr = m_sprite;
    const int xc = m_originX + m_circleCx;
    const int yc = m_originY + m_circleCy;

    float startDeg = static_cast<float>(wrapPos(pos)) * SECTOR_DEG - SECTOR_HALF_DEG;
    float endDeg = static_cast<float>(wrapPos(pos)) * SECTOR_DEG + SECTOR_HALF_DEG;

    if (startDeg < 0.0f)
        startDeg += FULL_TURN_DEG;

    const float r = static_cast<float>(outerR);
    const float thickness = static_cast<float>(outerR - innerR);

    spr->drawSmoothGauge(
        xc,        ///< X-Koordinate des Mittelpunkts
        yc,        ///< Y-Koordinate des Mittelpunkts
        r,         ///< Äußerer Radius
        thickness, ///< Dicke des Rings
        startDeg,  ///< Startwinkel in Grad, 0 Grad liegt oben und positive Werte laufen im Uhrzeigersinn
        endDeg,    ///< Endwinkel in Grad
        0.0f,      ///< Minimaler Wertebereich (hier 0.0f, da wir den Sektor komplett füllen wollen)
        1.0f,      ///< Aktueller Wert innerhalb von valueMin bis valueMax (hier volle Füllung)
        1.0f,      ///< Maximaler Wertebereich (hier 1.0f, da wir den Sektor komplett füllen wollen)
        fillColor, ///< Farbe des gefüllten Kreissegments im RGB565-Format
        fillColor, ///< Farbe der Spur außerhalb des aktuellen Werts im RGB565-Format (hier gleich, da wir den Sektor komplett füllen wollen)
        false,     ///< trackVisible true, wenn die Spur angezeigt werden soll (hier false, da wir den Sektor komplett füllen wollen)
        C_BG       ///< Hintergrundfarbe im RGB565-Format
    );
}

/**
 * @brief Treffertest auf Dur- und Moll-Ring.
 */
int WorkshopTool_CircleOfFifths::hitTestCircle(int cx, int cy) const
{
    const float dx = static_cast<float>(cx - m_circleCx);
    const float dy = static_cast<float>(cy - m_circleCy);
    const float r2 = dx * dx + dy * dy;

    const float rOuter2 = static_cast<float>(RADIUS_DUR_OUTER) * RADIUS_DUR_OUTER;
    const float rInner2 = static_cast<float>(RADIUS_MOLL_INNER) * RADIUS_MOLL_INNER;

    if (r2 < rInner2 || r2 > rOuter2)
        return -1;

    float angle = atan2f(dx, -dy);
    if (angle < 0.0f)
        angle += RAD_FULL_TURN;
    const float angleDeg = angle / DEG_TO_RAD_FACTOR;

    // Sektor 0 reicht von 345 Grad bis 15 Grad (Wraparound oben)
    const float shifted = angleDeg + SECTOR_HALF_DEG;
    int pos = static_cast<int>(shifted / SECTOR_DEG);
    pos = wrapPos(pos);

    return pos;
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Events auf den Quintenzirkel-Sektoren.
 */
bool WorkshopTool_CircleOfFifths::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    bool needsRedraw = false;

    if (touched)
    {
        if (!m_touchReady)
            return false;

        const int cx = tx - WorkshopCfg::WORK_X - m_originX;
        const int cy = ty - WorkshopCfg::WORK_Y - m_originY;

        const int hit = hitTestCircle(cx, cy);
        if (hit >= 0)
        {
            if (hit != m_pressedKey)
            {
                m_pressedKey = hit;
                needsRedraw = true;
            }
        }
        else if (m_pressedKey != -1)
        {
            m_pressedKey = -1;
            needsRedraw = true;
        }

        m_touchWas = true;
    }
    else
    {
        m_touchReady = true;

        if (m_touchWas && m_pressedKey >= 0 && m_pressedKey < NUM_KEYS)
        {
            if (static_cast<int>(m_selectedKey) != m_pressedKey)
            {
                m_selectedKey = static_cast<uint8_t>(m_pressedKey);
                needsRedraw = true;
            }
        }

        if (m_pressedKey != -1)
        {
            m_pressedKey = -1;
            needsRedraw = true;
        }

        m_touchWas = false;
    }

    return needsRedraw;
}

// ===================================================================
// Untertitel
// ===================================================================

/**
 * @brief Liefert den dynamischen Untertitel "Name-Dur (n Vorzeichen)".
 */
const char *WorkshopTool_CircleOfFifths::getSubtitle() const
{
    const KeyData &k = KEYS[m_selectedKey];

    if (k.acc == 0)
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf),
                     WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_NONE, k.major);
    else if (k.acc == 1)
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf),
                     WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_SHARP, k.major);
    else if (k.acc == -1)
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf),
                     WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_FLAT, k.major);
    else if (k.acc > 0)
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf),
                     WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_N_SHARPS, k.major, static_cast<int>(k.acc));
    else
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf),
                     WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_N_FLATS, k.major, -static_cast<int>(k.acc));

    return m_subtitleBuf;
}

// ===================================================================
// Helfer
// ===================================================================

/**
 * @brief Reduziert eine Sektor-Position modulo NUM_KEYS in den Bereich 0..NUM_KEYS-1.
 */
int WorkshopTool_CircleOfFifths::wrapPos(int p)
{
    int r = p % NUM_KEYS;
    if (r < 0)
        r += NUM_KEYS;
    return r;
}

/**
 * @brief Bildet das Akkord-Symbol aus Tonleiter-Ton und Geschlecht.
 */
void WorkshopTool_CircleOfFifths::formatChord(const char *tone, ChordQuality q,
                                              char *buf, size_t sz)
{
    if (sz == 0)
        return;

    if (q == ChordQuality::MAJOR)
    {
        stringutils::safeCopy(buf, sz, tone);
        if (buf[0] >= 'a' && buf[0] <= 'z')
            buf[0] = static_cast<char>(buf[0] - 'a' + 'A');
    }
    else
    {
        stringutils::safeFormat(buf, sz, "%sm", tone);
    }
}

/**
 * @brief Bildet die Vorzeichen-Detailzeile mit aufgezählten Tönen.
 */
void WorkshopTool_CircleOfFifths::formatAccidentalDetail(int keyIdx, char *buf, size_t sz)
{
    if (sz == 0)
        return;

    const KeyData &k = KEYS[clampi(keyIdx, 0, NUM_KEYS - 1)];

    if (k.acc == 0)
    {
        I18n::format(buf, sz, WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_NO_ACCIDENTALS);
        return;
    }

    const bool isSharp = (k.acc > 0);
    const int count = isSharp ? static_cast<int>(k.acc) : -static_cast<int>(k.acc);

    WorkshopKey unitKey;
    if (count == 1)
        unitKey = isSharp ? WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_UNIT_SHARP_SG
                          : WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_UNIT_FLAT_SG;
    else
        unitKey = isSharp ? WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_UNIT_SHARP_PL
                          : WorkshopKey::WORKSHOP_CIRCLE_OF_FIFTHS_UNIT_FLAT_PL;

    char unitBuf[12];
    I18n::format(unitBuf, sizeof(unitBuf), unitKey);

    char head[24];
    stringutils::safeFormat(head, sizeof(head), "%d %s: ", count, unitBuf);
    stringutils::safeCopy(buf, sz, head);

    const char *const *order = isSharp ? SHARP_ORDER : FLAT_ORDER;
    for (int i = 0; i < count && i < 7; ++i)
    {
        if (i > 0)
            stringutils::safeAppend(buf, sz, ", ");
        stringutils::safeAppend(buf, sz, order[i]);
    }
}

/**
 * @brief Liefert die Akzentfarbe je harmonischer Funktion.
 */
uint16_t WorkshopTool_CircleOfFifths::functionAccent(int posDelta) const
{
    switch (posDelta)
    {
    case 0:
        return C_ACCENT_TONIC;
    case -1:
        return C_ACCENT_SUBDOMINANT;
    case +1:
        return C_ACCENT_DOMINANT;
    default:
        return 0;
    }
}
