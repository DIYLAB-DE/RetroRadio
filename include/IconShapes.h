#ifndef ICON_SHAPES_H
#define ICON_SHAPES_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Grundformen, aus denen die Symbole der Oberfläche zusammengesetzt sind
 */
enum class IconShapeKind : uint8_t
{
    Segment,         ///< Strecke mit runden Enden
    Polyline,        ///< Streckenzug aus mehreren Punkten, wahlweise geschlossen
    Ring,            ///< Kreisring, der Radius meint die Mittellinie
    Disc,            ///< Gefüllte Scheibe
    RoundRect,       ///< Rechteck mit runden Ecken, nur der Umriss
    FilledRoundRect, ///< Gefülltes Rechteck mit runden Ecken
    Arc,             ///< Kreisbogen zwischen zwei Winkeln in Grad
    FilledPolygon    ///< Geschlossene Kontur einer gefüllten Fläche
};

/**
 * @brief Eine einzelne Grundform innerhalb eines Symbols
 *
 * Die Bedeutung der Kennwerte in values hängt von kind ab: Segment nutzt vier Werte für Anfang und Ende, Ring und Disc drei für Mittelpunkt und Radius, die beiden Rechtecke fünf für die Ecken und den Eckenradius, Arc fünf für Mittelpunkt, Radius und die beiden Winkel. Der Streckenzug führt seine Punkte stattdessen in der gemeinsamen Punkttabelle.
 *
 * Die gefüllte Fläche führt ihre Punkte ebenfalls in der Punkttabelle und belegt values[0] mit der Aufdickung, um die die Fläche nach außen wächst. Alle Formen dieser Bauart innerhalb eines Symbols beschreiben zusammen eine einzige Fläche: Sie werden nicht einzeln ausgewertet, sondern gemeinsam nach der Even-Odd-Regel, damit innenliegende Konturen Löcher ergeben. Sie müssen deshalb dieselbe Aufdickung tragen.
 */
struct IconShape
{
    IconShapeKind kind; ///< Bauform der Grundform
    bool erase;         ///< true zieht die Form ab, statt sie hinzuzufügen
    float values[5];    ///< Kennwerte der Form, je nach Bauform unterschiedlich belegt
    uint16_t firstPoint; ///< Index des ersten Punktes in der Punkttabelle, nur Streckenzug
    uint8_t pointCount;  ///< Anzahl der Punkte, nur Streckenzug
    bool closed;         ///< true schließt den Streckenzug zum Umriss, nur Streckenzug
};

/**
 * @brief Beschreibung eines vollständigen Symbols
 */
struct IconDefinition
{
    const IconShape *shapes; ///< Grundformen des Symbols
    uint8_t shapeCount;      ///< Anzahl der Grundformen
    uint8_t width;           ///< Breite der Kachel in Bildpunkten
    uint8_t height;          ///< Höhe der Kachel in Bildpunkten, bei den beiden Ordnersymbolen kleiner als die Breite
    float stroke;            ///< Strichbreite in Bildpunkten, ohne Bedeutung für Symbole aus gefüllten Flächen
};

/**
 * @brief Die Symbole der Oberfläche
 *
 * Die Reihenfolge muss zur Tabelle in IconShapeData.h passen; ein static_assert dort wacht darüber.
 */
enum class IconId : uint8_t
{
    Breather, ///< Atempause, eine Medaille
    Calendar, ///< Kalenderblatt, die Tageszahl wird zur Laufzeit darübergelegt
    Config,   ///< Einstellungen, drei Schieberegler
    Contacts, ///< Kontakte, eine Person
    DayView,  ///< Tagesansicht, Kalenderblatt mit Haken
    Folder,   ///< Ordner im Dateibrowser
    FolderUp, ///< Eine Ebene höher im Dateibrowser
    Mute,     ///< Stummschaltung am Lautstärkeregler
    Off,      ///< Ausschalten
    Player,   ///< SD-Player, ein Notenblatt
    Pomodoro, ///< Kurzzeitmesser, eine Tomate
    QrCode,   ///< QR-Einblendung
    Radio,    ///< RetroRadio, ein Kofferradio
    ToDo,     ///< Aufgabenliste
    Weather,  ///< Wetter, Wolke mit Sonne
    Workshop, ///< Werkstatt, ein Taschenmesser
    Count     ///< Anzahl der Symbole
};

/**
 * @brief Zeichnet die Symbole der Oberfläche aus Grundformen
 *
 * Die früheren Symbole lagen als RGB565-Bitmaps vor und wurden zur Laufzeit umgefärbt. Sie werden jetzt aus Strecken, Bögen und Rechtecken gezeichnet, und zwar einmalig beim Start. Deshalb darf der Zeichner aufwendig arbeiten: Er rechnet mit echten Abständen und tastet jeden Bildpunkt mehrfach ab, was eine Kantenglättung ohne sichtbare Treppen ergibt.
 */
namespace IconShapes
{
    /**
     * @brief Zeichnet ein Symbol in einen neu angelegten Puffer.
     * @param id Gewünschtes Symbol
     * @param color Farbe der Zeichnung im RGB565-Format
     * @param background Farbe der freien Fläche im RGB565-Format
     * @param scale Ganzzahliger Vergrößerungsfaktor; 1 zeichnet die Kachelgröße des Symbols, größere Werte vervielfachen Breite, Höhe und Strichbreite
     * @return uint16_t* Neuer Puffer im PSRAM, der Aufrufer muss free() rufen, oder nullptr
     *
     * Der Puffer hat die Kantenlänge des Symbols mal dem Faktor und lässt sich unmittelbar an Button::initButton oder pushImage übergeben.
     */
    uint16_t *draw(IconId id, uint16_t color, uint16_t background, int scale = 1);

    /**
     * @brief Liefert die Breite eines Symbols in Bildpunkten.
     * @param id Gewünschtes Symbol
     * @return int Breite, oder 0 bei ungültiger Kennung
     */
    int width(IconId id);

    /**
     * @brief Liefert die Höhe eines Symbols in Bildpunkten.
     * @param id Gewünschtes Symbol
     * @return int Höhe, oder 0 bei ungültiger Kennung
     */
    int height(IconId id);

    /**
     * @brief Liefert ein einmalig gezeichnetes Symbol aus dem internen Zwischenspeicher.
     * @param id Gewünschtes Symbol
     * @param color Farbe der Zeichnung im RGB565-Format
     * @param background Farbe der freien Fläche im RGB565-Format
     * @return const uint16_t* Puffer des Symbols, oder nullptr
     *
     * Gedacht für Symbole mit fester Farbe, die an mehreren Stellen und wiederholt gezeichnet werden. Der Puffer bleibt für die Laufzeit des Geräts bestehen und darf nicht freigegeben werden; die betroffenen Symbole belegen zusammen nur wenige Kilobyte.
     */
    const uint16_t *cached(IconId id, uint16_t color, uint16_t background);
}

#endif // ICON_SHAPES_H
