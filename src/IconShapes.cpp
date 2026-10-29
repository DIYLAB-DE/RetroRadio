#include "IconShapes.h"

#include <Arduino.h>
#include <math.h>

#include "IconShapeData.h"

// ===================================================================
// Kennwerte des Zeichners
// ===================================================================

namespace
{
    constexpr int ICON_SUPERSAMPLE = 2;          ///< Abtastungen je Achse und Bildpunkt; drei bringen sichtbar nichts mehr, kosten aber mehr als das Doppelte
    constexpr float ICON_HALF_PIXEL = 0.5f;      ///< Halbe Breite der weichen Übergänge in Bildpunkten
    constexpr float ICON_FAR_AWAY = 1.0e9f;      ///< Startwert der Abstandssuche, größer als jede Kachel
    constexpr uint32_t ICON_MAX_5BIT = 31;       ///< Größter Wert eines fünfstelligen Farbanteils
    constexpr uint32_t ICON_MAX_6BIT = 63;       ///< Größter Wert eines sechsstelligen Farbanteils
    constexpr float ICON_DEGREE_FULL = 360.0f;   ///< Voller Winkel in Grad
    constexpr float ICON_RAD_TO_DEG = 57.29578f; ///< Umrechnung von Radiant in Grad
}

// ===================================================================
// Umschließende Rechtecke
// ===================================================================

/**
 * @brief Umschließendes Rechteck einer Form, bereits um Strichbreite und weiche Kante erweitert
 *
 * Ohne diese Vorabprüfung müsste jeder Bildpunkt gegen jede Form eine Wurzel ziehen. Beim Start summierte sich das auf über achtzehn Millionen Wurzeln und ließ das Gerät eine halbe Minute lang schwarz.
 */
struct IconBounds
{
    float minX; ///< Linke Grenze
    float minY; ///< Obere Grenze
    float maxX; ///< Rechte Grenze
    float maxY; ///< Untere Grenze
};

/**
 * @brief Bestimmt das umschließende Rechteck einer Form
 */
static IconBounds iconShapeBounds(const IconShape &shape, float stroke)
{
    // Eine gefüllte Fläche kennt keine Strichbreite; sie reicht um ihre Aufdickung über die Konturpunkte hinaus.
    const float margin = (shape.kind == IconShapeKind::FilledPolygon) ? shape.values[0] + 1.0f
                                                                      : stroke * 0.5f + 1.0f;
    IconBounds box = {0.0f, 0.0f, 0.0f, 0.0f};

    switch (shape.kind)
    {
    case IconShapeKind::Segment:
        box.minX = fminf(shape.values[0], shape.values[2]);
        box.maxX = fmaxf(shape.values[0], shape.values[2]);
        box.minY = fminf(shape.values[1], shape.values[3]);
        box.maxY = fmaxf(shape.values[1], shape.values[3]);
        break;

    case IconShapeKind::Polyline:
    case IconShapeKind::FilledPolygon:
    {
        const size_t first = static_cast<size_t>(shape.firstPoint) * 2;
        box.minX = box.maxX = ICON_POINTS[first];
        box.minY = box.maxY = ICON_POINTS[first + 1];
        for (uint8_t i = 1; i < shape.pointCount; ++i)
        {
            const size_t a = static_cast<size_t>(shape.firstPoint + i) * 2;
            box.minX = fminf(box.minX, ICON_POINTS[a]);
            box.maxX = fmaxf(box.maxX, ICON_POINTS[a]);
            box.minY = fminf(box.minY, ICON_POINTS[a + 1]);
            box.maxY = fmaxf(box.maxY, ICON_POINTS[a + 1]);
        }
        break;
    }

    case IconShapeKind::Ring:
    case IconShapeKind::Disc:
    case IconShapeKind::Arc:
        box.minX = shape.values[0] - shape.values[2];
        box.maxX = shape.values[0] + shape.values[2];
        box.minY = shape.values[1] - shape.values[2];
        box.maxY = shape.values[1] + shape.values[2];
        break;

    case IconShapeKind::RoundRect:
    case IconShapeKind::FilledRoundRect:
        box.minX = shape.values[0];
        box.maxX = shape.values[2];
        box.minY = shape.values[1];
        box.maxY = shape.values[3];
        break;
    }

    box.minX -= margin;
    box.minY -= margin;
    box.maxX += margin;
    box.maxY += margin;

    return box;
}

/**
 * @brief Meldet, ob ein Punkt außerhalb eines umschließenden Rechtecks liegt
 */
static inline bool iconOutside(const IconBounds &box, float px, float py)
{
    return px < box.minX || px > box.maxX || py < box.minY || py > box.maxY;
}

// ===================================================================
// Abstandsfunktionen der Grundformen
// ===================================================================

/**
 * @brief Abstand eines Punktes zu einer Strecke
 */
static float iconDistanceToSegment(float px, float py, float x0, float y0, float x1, float y1)
{
    const float vx = x1 - x0;
    const float vy = y1 - y0;
    const float wx = px - x0;
    const float wy = py - y0;

    const float lengthSq = vx * vx + vy * vy;
    float t = 0.0f;
    if (lengthSq > 0.0f)
        t = constrain((wx * vx + wy * vy) / lengthSq, 0.0f, 1.0f);

    return hypotf(wx - t * vx, wy - t * vy);
}

/**
 * @brief Grobe Vorprüfung, ob ein Punkt weit genug von einer Teilstrecke entfernt liegt
 *
 * Vergleicht nur das umschließende Rechteck der Strecke; das kostet ein paar Vergleiche statt einer Wurzel. Der Vergleich gegen beide Enden steht hier bewusst statt fminf und fmaxf: Die bleiben als Aufruf in libm stehen, und diese Prüfung läuft öfter als jede andere Zeile des Zeichners.
 */
static inline bool iconSegmentFarAway(float px, float py, float x0, float y0, float x1, float y1, float reach)
{
    if (px < x0 - reach && px < x1 - reach)
        return true;
    if (px > x0 + reach && px > x1 + reach)
        return true;
    if (py < y0 - reach && py < y1 - reach)
        return true;

    return py > y0 + reach && py > y1 + reach;
}

/**
 * @brief Vorzeichenbehafteter Abstand zum Rand eines Rechtecks mit runden Ecken, innerhalb negativ
 */
static float iconDistanceToRoundRect(float px, float py, float x0, float y0, float x1, float y1, float radius)
{
    const float centerX = (x0 + x1) * 0.5f;
    const float centerY = (y0 + y1) * 0.5f;
    const float halfWidth = (x1 - x0) * 0.5f - radius;
    const float halfHeight = (y1 - y0) * 0.5f - radius;

    const float qx = fabsf(px - centerX) - halfWidth;
    const float qy = fabsf(py - centerY) - halfHeight;

    const float outside = hypotf(fmaxf(qx, 0.0f), fmaxf(qy, 0.0f));
    const float inside = fminf(fmaxf(qx, qy), 0.0f);

    return outside + inside - radius;
}

/**
 * @brief Abstand zu einem Kreisbogen zwischen zwei Winkeln
 *
 * Außerhalb des Winkelbereichs zählt der Abstand zum näheren Bogenende; dadurch bekommt der Bogen runde Enden wie eine Strecke.
 */
static float iconDistanceToArc(float px, float py, float cx, float cy, float radius, float startDeg, float endDeg)
{
    float angle = atan2f(py - cy, px - cx) * ICON_RAD_TO_DEG;
    if (angle < 0.0f)
        angle += ICON_DEGREE_FULL;

    float low = fmodf(startDeg, ICON_DEGREE_FULL);
    if (low < 0.0f)
        low += ICON_DEGREE_FULL;

    float high = fmodf(endDeg, ICON_DEGREE_FULL);
    if (high < 0.0f)
        high += ICON_DEGREE_FULL;

    const bool onArc = (low <= high) ? (angle >= low && angle <= high)
                                     : (angle >= low || angle <= high);

    if (onArc)
        return fabsf(hypotf(px - cx, py - cy) - radius);

    const float startX = cx + radius * cosf(startDeg / ICON_RAD_TO_DEG);
    const float startY = cy + radius * sinf(startDeg / ICON_RAD_TO_DEG);
    const float endX = cx + radius * cosf(endDeg / ICON_RAD_TO_DEG);
    const float endY = cy + radius * sinf(endDeg / ICON_RAD_TO_DEG);

    return fminf(hypotf(px - startX, py - startY), hypotf(px - endX, py - endY));
}

/**
 * @brief Abstand zu den Kanten einer geschlossenen Kontur, dazu die Strahlenzählung für die Even-Odd-Regel
 * @param shape Kontur einer gefüllten Fläche
 * @param px Waagerechte Lage des Punktes
 * @param py Senkrechte Lage des Punktes
 * @param reach Reichweite, ab der eine Teilstrecke ohne Wurzel übersprungen werden darf
 * @param inside Wird umgeschaltet, wenn der Strahl nach rechts diese Kontur ungerade oft trifft
 * @return float Kleinster Abstand zu einer Kante, oder ICON_FAR_AWAY
 *
 * Die Strahlenzählung braucht jede Kante, die Wurzel dagegen nur die nahen; deshalb steht die Vorprüfung nur vor dem Abstand.
 */
static float iconFilledContourDistance(const IconShape &shape, float px, float py, float reach, bool &inside)
{
    const size_t first = static_cast<size_t>(shape.firstPoint) * 2;
    float nearest = ICON_FAR_AWAY;

    for (uint8_t i = 0; i < shape.pointCount; ++i)
    {
        const size_t a = first + static_cast<size_t>(i) * 2;
        const size_t b = first + static_cast<size_t>((i + 1) % shape.pointCount) * 2;
        const float ax = ICON_POINTS[a];
        const float ay = ICON_POINTS[a + 1];
        const float bx = ICON_POINTS[b];
        const float by = ICON_POINTS[b + 1];

        if (!iconSegmentFarAway(px, py, ax, ay, bx, by, reach))
        {
            const float distance = iconDistanceToSegment(px, py, ax, ay, bx, by);
            if (distance < nearest)
                nearest = distance;
        }

        if ((ay > py) != (by > py))
        {
            // Links von beiden Enden trifft der Strahl auf jeden Fall, rechts von beiden auf keinen; nur dazwischen ist die Teilung nötig.
            if (px < ax && px < bx)
                inside = !inside;
            else if ((px < ax || px < bx) && px < ax + (py - ay) * (bx - ax) / (by - ay))
                inside = !inside;
        }
    }

    return nearest;
}

/**
 * @brief Deckung einer einzelnen Grundform an einem Punkt
 *
 * Der Übergang von voll auf leer läuft über genau einen Bildpunkt, gemessen am echten Abstand zur Form.
 */
static float iconShapeCoverage(const IconShape &shape, float px, float py, float stroke, float scale)
{
    const float half = stroke * 0.5f;
    float distance = 0.0f;

    switch (shape.kind)
    {
    case IconShapeKind::Segment:
        distance = iconDistanceToSegment(px, py, shape.values[0], shape.values[1],
                                         shape.values[2], shape.values[3]) - half;
        break;

    case IconShapeKind::Polyline:
    {
        // Lange Streckenzüge stammen aus abgetasteten Kurven; ohne die grobe Vorprüfung je Teilstrecke wäre der Zeichner um ein Vielfaches langsamer.
        const float reach = half + 1.0f;
        float nearest = ICON_FAR_AWAY;

        for (uint8_t i = 0; i + 1 < shape.pointCount; ++i)
        {
            const size_t a = static_cast<size_t>(shape.firstPoint + i) * 2;
            if (iconSegmentFarAway(px, py, ICON_POINTS[a], ICON_POINTS[a + 1],
                                   ICON_POINTS[a + 2], ICON_POINTS[a + 3], reach))
                continue;

            nearest = fminf(nearest, iconDistanceToSegment(px, py,
                                                           ICON_POINTS[a], ICON_POINTS[a + 1],
                                                           ICON_POINTS[a + 2], ICON_POINTS[a + 3]));
        }

        if (shape.closed && shape.pointCount > 2)
        {
            const size_t first = static_cast<size_t>(shape.firstPoint) * 2;
            const size_t last = static_cast<size_t>(shape.firstPoint + shape.pointCount - 1) * 2;
            if (!iconSegmentFarAway(px, py, ICON_POINTS[last], ICON_POINTS[last + 1],
                                    ICON_POINTS[first], ICON_POINTS[first + 1], reach))
            {
                nearest = fminf(nearest, iconDistanceToSegment(px, py,
                                                               ICON_POINTS[last], ICON_POINTS[last + 1],
                                                               ICON_POINTS[first], ICON_POINTS[first + 1]));
            }
        }

        distance = nearest - half;
        break;
    }

    case IconShapeKind::Ring:
        distance = fabsf(hypotf(px - shape.values[0], py - shape.values[1]) - shape.values[2]) - half;
        break;

    case IconShapeKind::Disc:
        distance = hypotf(px - shape.values[0], py - shape.values[1]) - shape.values[2];
        break;

    case IconShapeKind::RoundRect:
        distance = fabsf(iconDistanceToRoundRect(px, py, shape.values[0], shape.values[1],
                                                 shape.values[2], shape.values[3], shape.values[4])) - half;
        break;

    case IconShapeKind::FilledRoundRect:
        distance = iconDistanceToRoundRect(px, py, shape.values[0], shape.values[1],
                                           shape.values[2], shape.values[3], shape.values[4]);
        break;

    case IconShapeKind::Arc:
        distance = iconDistanceToArc(px, py, shape.values[0], shape.values[1], shape.values[2],
                                     shape.values[3], shape.values[4]) - half;
        break;

    case IconShapeKind::FilledPolygon:
        return 0.0f;
    }

    return constrain(ICON_HALF_PIXEL - distance * scale, 0.0f, 1.0f);
}

// ===================================================================
// Public Funktionen
// ===================================================================

/**
 * @brief Liefert die Breite eines Symbols in Bildpunkten
 */
int IconShapes::width(IconId id)
{
    if (id >= IconId::Count)
        return 0;

    return ICON_DEFINITIONS[static_cast<size_t>(id)].width;
}

/**
 * @brief Liefert die Höhe eines Symbols in Bildpunkten
 */
int IconShapes::height(IconId id)
{
    if (id >= IconId::Count)
        return 0;

    return ICON_DEFINITIONS[static_cast<size_t>(id)].height;
}

/**
 * @brief Liefert ein einmalig gezeichnetes Symbol aus dem internen Zwischenspeicher
 */
const uint16_t *IconShapes::cached(IconId id, uint16_t color, uint16_t background)
{
    if (id >= IconId::Count)
        return nullptr;

    static const uint16_t *buffers[static_cast<size_t>(IconId::Count)] = {nullptr};

    const size_t index = static_cast<size_t>(id);
    if (!buffers[index])
        buffers[index] = IconShapes::draw(id, color, background);

    return buffers[index];
}

/**
 * @brief Zeichnet ein Symbol in einen neu angelegten Puffer
 */
uint16_t *IconShapes::draw(IconId id, uint16_t color, uint16_t background, int scale)
{
    if (id >= IconId::Count)
        return nullptr;
    if (scale < 1)
        scale = 1;

    // Die Formen bleiben in Kacheleinheiten; abgetastet wird in Bildschirmpixeln, deren Koordinaten durch den Faktor geteilt werden, und die Kantenweichheit bleibt dadurch einen halben Bildschirmpixel breit
    const float scaleF = static_cast<float>(scale);
    const IconDefinition &def = ICON_DEFINITIONS[static_cast<size_t>(id)];
    const int columns = def.width * scale;
    const int rows = def.height * scale;
    const size_t pixelCount = static_cast<size_t>(columns) * static_cast<size_t>(rows);

    uint16_t *out = static_cast<uint16_t *>(ps_malloc(pixelCount * sizeof(uint16_t)));
    if (!out)
        return nullptr;

    const float foreRed = static_cast<float>((color >> 11) & 0x1F);
    const float foreGreen = static_cast<float>((color >> 5) & 0x3F);
    const float foreBlue = static_cast<float>(color & 0x1F);
    const float backRed = static_cast<float>((background >> 11) & 0x1F);
    const float backGreen = static_cast<float>((background >> 5) & 0x3F);
    const float backBlue = static_cast<float>(background & 0x1F);

    const float step = 1.0f / static_cast<float>(ICON_SUPERSAMPLE);
    const float firstOffset = step * 0.5f;
    const float invSampleCount = 1.0f / static_cast<float>(ICON_SUPERSAMPLE * ICON_SUPERSAMPLE);

    // Umschließende Rechtecke einmal je Symbol bestimmen; sie ersparen im Bildpunkt die allermeisten Wurzeln.
    IconBounds bounds[ICON_MAX_SHAPE_COUNT];
    for (uint8_t i = 0; i < def.shapeCount; ++i)
        bounds[i] = iconShapeBounds(def.shapes[i], def.stroke);

    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < columns; ++x)
        {
            float sum = 0.0f;

            for (int sy = 0; sy < ICON_SUPERSAMPLE; ++sy)
            {
                const float py = (static_cast<float>(y) + firstOffset + step * static_cast<float>(sy)) / scaleF;

                for (int sx = 0; sx < ICON_SUPERSAMPLE; ++sx)
                {
                    const float px = (static_cast<float>(x) + firstOffset + step * static_cast<float>(sx)) / scaleF;

                    float added = 0.0f;
                    float removed = 0.0f;
                    float filledNearest = ICON_FAR_AWAY;
                    float filledGrow = 0.0f;
                    bool filledInside = false;

                    for (uint8_t i = 0; i < def.shapeCount; ++i)
                    {
                        if (iconOutside(bounds[i], px, py))
                            continue;

                        const IconShape &shape = def.shapes[i];

                        if (shape.kind == IconShapeKind::FilledPolygon)
                        {
                            filledGrow = shape.values[0];
                            filledNearest = fminf(filledNearest,
                                                  iconFilledContourDistance(shape, px, py,
                                                                            filledGrow + 1.0f,
                                                                            filledInside));
                            continue;
                        }

                        const float coverage = iconShapeCoverage(shape, px, py, def.stroke, scaleF);

                        if (shape.erase)
                            removed = fmaxf(removed, coverage);
                        else
                            added = fmaxf(added, coverage);
                    }

                    // Erst nach der Schleife steht fest, ob der Punkt nach der Even-Odd-Regel innerhalb der gefüllten Fläche liegt; ein Punkt tief im Inneren hat keine nahe Kante und behält deshalb den Startwert der Abstandssuche.
                    if (filledInside || filledNearest < ICON_FAR_AWAY)
                    {
                        const float signedDistance = filledInside ? -filledNearest : filledNearest;
                        added = fmaxf(added, constrain(ICON_HALF_PIXEL - (signedDistance - filledGrow) * scaleF, 0.0f, 1.0f));
                    }

                    sum += fmaxf(0.0f, added - removed);
                }
            }

            const float alpha = sum * invSampleCount;
            const uint32_t red = static_cast<uint32_t>(backRed + (foreRed - backRed) * alpha + 0.5f);
            const uint32_t green = static_cast<uint32_t>(backGreen + (foreGreen - backGreen) * alpha + 0.5f);
            const uint32_t blue = static_cast<uint32_t>(backBlue + (foreBlue - backBlue) * alpha + 0.5f);

            out[static_cast<size_t>(y) * static_cast<size_t>(columns) + static_cast<size_t>(x)] =
                static_cast<uint16_t>((constrain(red, 0u, ICON_MAX_5BIT) << 11) |
                                      (constrain(green, 0u, ICON_MAX_6BIT) << 5) |
                                      constrain(blue, 0u, ICON_MAX_5BIT));
        }
    }

    return out;
}
