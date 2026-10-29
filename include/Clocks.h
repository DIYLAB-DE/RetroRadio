#ifndef CLOCKS_H
#define CLOCKS_H

#include <stdint.h>

class DS1307_RTC;
class Gfx_Sprite;

/**
 * @brief Utility-Klasse für Uhr-Rendering
 *
 * Stellt statische Methoden für das Zeichnen analoger und digitaler Uhren mit optionaler Sleep-Timer-Integration bereit. RetroRadio und SDPlayer verwenden dieselben Zeichenmethoden.
 */
class Clocks
{
public:
    /**
     * @brief Zeichnet eine analoge Uhr mit optionalem Sleep-Timer
     * @param sprite Zeiger auf das Sprite (70x70), muss bereits erstellt sein
     * @param rtc Zeiger auf die Echtzeituhr
     * @param lastDrawnSecond Referenz auf Cache-Variable für letzte gezeichnete Sekunde
     * @param x X-Position für pushSprite
     * @param y Y-Position für pushSprite
     * @param remainingMinutes Verbleibende Timer-Minuten (-1 = kein Timer, 0-60)
     * @param remainingSeconds Verbleibende Timer-Sekunden (0-59)
     *
     * Die Funktion prüft, ob sich die Sekunde geändert hat und rendert nur dann neu. Bei aktivem Timer (remainingMinutes != -1) wird ein Ring-Gauge angezeigt.
     */
    static void drawAnalogClock(
        Gfx_Sprite *sprite,
        DS1307_RTC *rtc,
        int &lastDrawnSecond,
        int32_t x,
        int32_t y,
        int remainingMinutes = -1,
        int remainingSeconds = 0);

    /**
     * @brief Zeichnet eine digitale Uhr mit optionalem Sleep-Timer
     * @param sprite Zeiger auf das Sprite (150x45), muss bereits erstellt sein
     * @param rtc Zeiger auf die Echtzeituhr
     * @param lastDrawnSecond Referenz auf Cache-Variable für letzte gezeichnete Sekunde
     * @param x X-Position für pushSprite
     * @param y Y-Position für pushSprite
     * @param remainingMinutes Verbleibende Timer-Minuten (-1 = kein Timer, 0-60)
     * @param remainingSeconds Verbleibende Timer-Sekunden (0-59)
     *
     * Die Funktion zeigt die aktuelle Uhrzeit im Format HH:MM an. Bei aktivem Timer wird ein Ring-Gauge mit Minuten/Sekunden-Anzeige dargestellt.
     */
    static void drawDigitalClock(
        Gfx_Sprite *sprite,
        DS1307_RTC *rtc,
        int &lastDrawnSecond,
        int32_t x,
        int32_t y,
        int remainingMinutes = -1,
        int remainingSeconds = 0);

    /**
     * @brief Zeichnet eine einfache digitale Uhr ohne Timer-Features
     * @param sprite Zeiger auf das Sprite (150x45 ohne Sekunden, 220x45 mit Sekunden), muss bereits erstellt sein
     * @param rtc Zeiger auf die Echtzeituhr
     * @param lastDrawnSecond Referenz auf Cache-Variable für letzte gezeichnete Sekunde
     * @param x X-Position für pushSprite
     * @param y Y-Position für pushSprite
     * @param showSeconds true = Format HH:MM:SS, false = Format HH:MM
     * @param blinkingColons true = Doppelpunkte blinken im Sekundentakt, false = statisch
     *
     * Die Funktion zeigt die aktuelle Uhrzeit im gewählten Format an. Die Funktion prüft, ob sich die Sekunde geändert hat und rendert nur dann neu.
     */
    static void drawDigitalClockPlain(
        Gfx_Sprite *sprite,
        DS1307_RTC *rtc,
        int &lastDrawnSecond,
        int32_t x,
        int32_t y,
        bool showSeconds = false,
        bool blinkingColons = true);
};

#endif // CLOCKS_H
