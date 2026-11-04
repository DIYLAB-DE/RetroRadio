#ifndef AUDIO_SLIDER_TOUCH_H
#define AUDIO_SLIDER_TOUCH_H

#include <stdint.h>

/**
 * @brief Unterscheidet Tippen, Ziehen und Gedrückthalten an den vier Klangreglern.
 */
class AudioSliderTouch
{
public:
    static constexpr int COUNT = 4;                     ///< Balance, Tiefen, Mitten und Höhen
    static constexpr int LEFT[COUNT] = {5, 55, 105, 155}; ///< Linke Kante jeder Reglerspur im Sprite
    static constexpr int WIDTH = 31;                    ///< Breite der Reglerspur
    static constexpr int CENTER_X = 15;                 ///< Abstand des Kreismittelpunkts zur linken Kante
    static constexpr int RADIUS = 13;                   ///< Außenradius des Reglerkreises
    static constexpr int TOP = 24;                      ///< Oberste Reglerposition
    static constexpr int NEUTRAL = 64;                  ///< Mittellinie für Balance 0 und 0 dB
    static constexpr int BOTTOM = 104;                  ///< Unterste Reglerposition

    struct Target
    {
        int index = -1;      ///< Betroffener Regler oder -1 ohne Änderung
        int y = 0;           ///< Neue Reglerposition im Sprite
        bool center = false; ///< Langdruck hat die Mittelstellung ausgelöst
    };

    /**
     * @brief Liefert pro Berührung höchstens einen Sprung zur Mitte. Ziehen ist bis zum Loslassen von der Haltegeste ausgeschlossen.
     * @param touched Aktueller Berührungszustand
     * @param x X-Position im Reglersprite
     * @param y Y-Position im Reglersprite
     * @param positions Zuletzt gezeichnete Kreismittelpunkte
     * @param now Aktuelle Zeit in Millisekunden
     * @return Ziel für Anzeige und Klang oder ein leeres Ziel
     */
    Target update(bool touched, int x, int y, const int8_t (&positions)[COUNT], uint32_t now);

    /** @brief Verwirft eine Berührung bis zum Loslassen, etwa beim Öffnen eines Dialogs. */
    void cancel() { m_phase = Phase::Consumed; }

private:
    enum class Phase
    {
        Idle,     ///< Keine Berührung
        Holding,  ///< Finger ruht auf einem Reglerkreis
        Sliding,  ///< Berührung kann nur noch tippen oder ziehen
        Consumed   ///< Bis zum Loslassen keine weitere Änderung
    };

    Phase m_phase = Phase::Idle; ///< Zustand der aktuellen Berührung
    int m_slider = 0;            ///< Regler, auf dessen Kreis der Druck begann
    int m_startX = 0;            ///< Anfangsposition der Haltegeste
    int m_startY = 0;            ///< Anfangsposition für Haltegeste und kurzen Tipp
    uint32_t m_startTime = 0;    ///< Beginn der Haltegeste in Millisekunden
};

#endif // AUDIO_SLIDER_TOUCH_H
