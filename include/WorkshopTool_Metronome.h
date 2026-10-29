#ifndef WORKSHOPTOOL_METRONOME_H
#define WORKSHOPTOOL_METRONOME_H

#include <stdint.h>
#include <memory>

#include "Button.h"
#include "MessageBox.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_Metronome
 * @brief Metronom mit großen Beat-Lichtern und optionalem Klick-Sound.
 *
 * Darstellung: Eine waagrechte Reihe großer Lichter zeigt die Schläge eines Takts. Der aktuelle Schlag leuchtet hell, alle anderen sind dunkel. Der erste Schlag (bei 6/8 zusätzlich der vierte) ist als Akzent-Schlag farblich abgesetzt. In der Mitte stehen das Tempo in Schlägen pro Minute und die italienische Tempo-Bezeichnung, beide in konstanter Farbe ohne Aufleuchten beim Schlag. Die Taktart-Beschriftung steht dezent unter den Lichtern.
 *
 * Alle Elemente behalten feste Positionen. Schlagwechsel ändern ausschließlich Farben und vermeiden dadurch bewegte Geometrie mit sichtbarem Tearing auf dem RGB-Display.
 *
 * Bedienung: Start/Stop, BPM +/-, Tap-Tempo (letzte vier Taps mitteln), Taktart-Wechsel (2/4, 3/4, 4/4, 6/8). Tippen auf den Inhaltsbereich startet und stoppt das Metronom.
 *
 * Sound: Bei jedem Schlag wird über den Workshop-Owner ein Klick-Sound ausgelöst. Der Akzent-Schlag erhält einen höheren Klick als die übrigen Schläge. Erwartet werden die Dateien /sounds/metro_hi.wav und /sounds/metro_lo.wav auf der SD-Karte.
 *
 * Layout (547 x 327 px Netto-Inhaltsbereich):
 *   Beat-Lichter              Waagrechte Reihe, zentriert bei y ≈ 80, Radius 36 px für aktiv, 28 px für inaktiv
 *   BPM-Wert                  Zentriert bei y ≈ 180, ComfortaaBold30
 *   Tempo-Name                Zentriert bei y ≈ 215, ComfortaaBold14
 *   Fußzeile                  Taktart bzw. Tipphinweis bei y ≈ 290, ComfortaaBold14
 */
class WorkshopTool_Metronome : public WorkshopTool
{
public:
    WorkshopTool_Metronome();
    ~WorkshopTool_Metronome() override = default;

    WorkshopTool_Metronome(const WorkshopTool_Metronome &) = delete;
    WorkshopTool_Metronome &operator=(const WorkshopTool_Metronome &) = delete;

    /**
     * @brief Initialisiert Zustand und Footer-Buttons.
     * @param tft Display-Objekt für die Button-Klasse
     */
    void onEnter(LGFX *tft) override;

    /**
     * @brief Räumt den Werkzeugzustand auf (Footer-Buttons abmelden).
     */
    void onExit() override;

    /**
     * @brief Zeichnet Beat-Lichter, BPM-Anzeige und Fußzeile in den Work-Sprite.
     * @param sprite Ziel-Sprite (m_workSprite des Workshop-Moduls)
     * @param ox     X-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param oy     Y-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param w      Verfügbare Breite in Pixeln
     * @param h      Verfügbare Höhe in Pixeln
     */
    void draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse im Inhaltsbereich (Tap startet/stoppt).
     * @return true wenn der Inhaltsbereich neu gezeichnet werden muss
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Zeichnet die Footer-Buttons.
     */
    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
     * @return true wenn ein Komplett-Redraw nötig ist
     */
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Liefert den dynamischen Untertitel mit BPM und Tempo-Name.
     */
    const char *getSubtitle() const override;

    /**
     * @brief Liefert true solange das Metronom läuft und Animation braucht.
     */
    bool needsAnimationFrame() const override;

private:
    // ========================================================================
    // Konstanten
    // ========================================================================

    static constexpr int BPM_MIN = 30;        ///< Minimales Tempo
    static constexpr int BPM_MAX = 300;       ///< Maximales Tempo
    static constexpr int BPM_DEFAULT = 120;   ///< Standard-Tempo
    static constexpr int TAP_HISTORY = 4;     ///< Anzahl gespeicherter Tap-Zeitpunkte
    static constexpr int SUBTITLE_CAP = 48;   ///< Puffergröße für den Untertitel

    // ========================================================================
    // Zustand
    // ========================================================================

    int m_bpm = BPM_DEFAULT;     ///< Aktuelles Tempo in BPM
    int m_timeSigIdx = 2;        ///< Index in die Taktart-Tabelle (Default: 4/4)
    bool m_running = false;      ///< Metronom läuft
    uint32_t m_startMs = 0;      ///< Startzeitpunkt (millis())
    int m_lastBeatIndex = -1;    ///< Zuletzt erkannter Beat-Index für Schlagdetektion

    // ========================================================================
    // Tap-Tempo
    // ========================================================================

    uint32_t m_tapTimes[TAP_HISTORY] = {}; ///< Zeitstempel der letzten Taps
    int m_tapCount = 0;                    ///< Gesamtzahl registrierter Taps

    // ========================================================================
    // Touch-Tracking
    // ========================================================================

    bool m_touchActive = false; ///< Touch-Sequenz läuft (touched=true gesehen)

    // ========================================================================
    // Untertitel
    // ========================================================================

    char m_subtitleBuf[SUBTITLE_CAP] = {}; ///< Puffer für den dynamischen Untertitel

    // ========================================================================
    // Footer-Buttons
    // ========================================================================

    Button m_btnStartStop;             ///< Start/Stop-Toggle
    Button m_btnMinus;                 ///< BPM verringern
    Button m_btnPlus;                  ///< BPM erhöhen
    Button m_btnTap;                   ///< Tap-Tempo
    Button m_btnTimeSig;               ///< Taktart wechseln
    Button m_btnHelp;                  ///< Hilfe anzeigen
    bool m_buttonsInitialized = false; ///< Buttons wurden in onEnter() initialisiert

    std::unique_ptr<MessageBox> m_msg; ///< Modale Dialogbox für Hilfetext

    // ========================================================================
    // Dialog-Beschriftungs-Puffer (MessageBox speichert nur Pointer)
    // ========================================================================

    char m_helpTitleBuf[40] = {};   ///< Puffer für Hilfe-Titel
    char m_helpTextBuf[512] = {};   ///< Puffer für Hilfe-Text
    char m_dialogCloseBuf[16] = {}; ///< Puffer für Schließen-Knopf

    // ========================================================================
    // Methoden - Lebenszyklus
    // ========================================================================

    /**
     * @brief Setzt alle Member auf definierte Ausgangswerte.
     */
    void clearAll();

    // ========================================================================
    // Methoden - Zeichnen
    // ========================================================================

    /**
     * @brief Zeichnet die Beat-Lichter waagrecht zentriert.
     * @param sprite Ziel-Sprite
     * @param cx Mittelpunkt X
     * @param cy Y-Mitte der Lichter-Reihe
     * @param currentBeat Aktuell aktiver Beat-Index oder -1 wenn nichts aktiv
     */
    void drawBeatLights(Gfx_Sprite *sprite, int cx, int cy, int currentBeat);

    /**
     * @brief Zeichnet BPM-Wert und Tempo-Bezeichnung.
     * @param sprite Ziel-Sprite
     * @param cx Mittelpunkt X
     * @param bpmCenterY Y-Mitte der BPM-Zahl
     * @param tempoCenterY Y-Mitte des Tempo-Namens
     */
    void drawCenter(Gfx_Sprite *sprite, int cx, int bpmCenterY, int tempoCenterY);

    /**
     * @brief Zeichnet Taktart oder Tipphinweis in der Fußzeile.
     * @param sprite Ziel-Sprite
     * @param cx Mittelpunkt X
     * @param cy Y-Mitte des Texts
     */
    void drawFooterText(Gfx_Sprite *sprite, int cx, int cy);

    // ========================================================================
    // Methoden - Helfer
    // ========================================================================

    /**
     * @brief Liefert die italienische Tempo-Bezeichnung zum aktuellen BPM-Wert.
     * @return Zeiger auf statischen String
     */
    const char *getTempoName() const;

    /**
     * @brief Berechnet die Anzahl Schläge pro Takt aus der aktuellen Taktart.
     */
    int beatsPerMeasure() const;

    /**
     * @brief Prüft ob ein Beat-Index ein Akzent-Schlag ist.
     */
    bool isAccentBeat(int beatIdx) const;

    /**
     * @brief Aktualisiert den Untertitel-Puffer.
     */
    void updateSubtitle();

    // ========================================================================
    // Methoden - Tap-Tempo
    // ========================================================================

    /**
     * @brief Registriert einen Tap und berechnet ggf. ein neues BPM aus den letzten Taps.
     */
    void processTapTempo();

    // ========================================================================
    // Methoden - Footer
    // ========================================================================

    /**
     * @brief Aktualisiert Farben und Label des Start/Stop-Buttons.
     */
    void updateStartStopButton();

    /**
     * @brief Aktualisiert das Label des Taktart-Buttons.
     */
    void updateTimeSigButton();

    /**
     * @brief Setzt alle Footer-Buttons in den Ruhezustand und zeichnet sie neu.
     */
    void resetFooterButtons();

    // ========================================================================
    // Methoden - Dialog
    // ========================================================================

    /**
     * @brief Öffnet die Bedienungsanleitung in einer modalen MessageBox.
     */
    void showHelpDialog();
};

#endif // WORKSHOPTOOL_METRONOME_H
