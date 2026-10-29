#ifndef WORKSHOPTOOL_FLIGHTRADAR_H
#define WORKSHOPTOOL_FLIGHTRADAR_H

#include <stdint.h>
#include <memory>

#include "Button.h"
#include "MessageBox.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_FlightRadar
 * @brief OpenSky-Radaranzeige mit laufender Scanline und begrenzter Flugzeugliste.
 *
 * Das Werkzeug startet beim Betreten einen Hintergrund-Worker. Dieser ruft in einem budgetierten Intervall eine kleine OpenSky-Bounding-Box ab und schreibt einen kompakten Schnappschuss in einen festen Puffer. Der Zeichenpfad liest nur diesen Schnappschuss, projiziert bekannte Flugzeuge weiter und führt keine Netzwerkoperationen aus.
 *
 * Der Mittelpunkt ist der gespeicherte Wetterstandort. Der Suchradius und optionale OpenSky-Zugangsdaten werden persistent über AppState gespeichert.
 */
class WorkshopTool_FlightRadar : public WorkshopTool
{
public:
    WorkshopTool_FlightRadar() = default;
    ~WorkshopTool_FlightRadar() override;

    WorkshopTool_FlightRadar(const WorkshopTool_FlightRadar &) = delete;
    WorkshopTool_FlightRadar &operator=(const WorkshopTool_FlightRadar &) = delete;
    WorkshopTool_FlightRadar(WorkshopTool_FlightRadar &&) = delete;
    WorkshopTool_FlightRadar &operator=(WorkshopTool_FlightRadar &&) = delete;

    /**
     * @brief Startet den Radar-Worker, sofern er nicht bereits läuft.
     * @param tft Display-Objekt, aktuell nicht direkt benötigt
     */
    void onEnter(LGFX *tft) override;

    /**
     * @brief Markiert den Radar-Worker zum Stoppen.
     */
    void onExit() override;

    /**
     * @brief Zeichnet Radar und Flugzeugliste in den Work-Sprite.
     * @param sprite Ziel-Sprite des Workshop-Moduls
     * @param originX Linke Kante des Inhaltsbereichs
     * @param originY Obere Kante des Inhaltsbereichs
     * @param w Verfügbare Breite
     * @param h Verfügbare Höhe
     */
    void draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h) override;

    /**
     * @brief Zeichnet nur den animierten Radarbereich.
     * @param sprite Ziel-Sprite des Workshop-Moduls
     * @param originX Linke Kante des Inhaltsbereichs
     * @param originY Obere Kante des Inhaltsbereichs
     * @param w Verfügbare Breite
     * @param h Verfügbare Höhe
     */
    void drawAnimationFrame(Gfx_Sprite *sprite, int originX, int originY, int w, int h) override;

    /**
     * @brief Verarbeitet einen Tap, ohne das OpenSky-Abrufbudget zu umgehen.
     * @return true beim Loslassen nach einem Tap
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Fordert Anzeigeaktualisierungen im begrenzten Frame-Takt an.
     */
    bool needsAnimationFrame() const override;

    /**
     * @brief Liefert den Bereich, der bei Animationen zum Display übertragen werden muss.
     * @param x Linke Kante im Work-Sprite
     * @param y Obere Kante im Work-Sprite
     * @param w Breite in Pixeln
     * @param h Höhe in Pixeln
     * @return true wenn ein gültiger Radarbereich bekannt ist
     */
    bool getPushBoundingBox(int &x, int &y, int &w, int &h) const override;

    /**
     * @brief Überträgt nur die geänderten Bereiche eines Animationsframes zum Display.
     * @param tft Display-Objekt
     * @param sprite Work-Sprite des Workshop-Moduls
     * @param spriteX Displayposition der linken Sprite-Kante
     * @param spriteY Displayposition der oberen Sprite-Kante
     * @return true wenn die Bereiche übertragen wurden und Workshop keinen eigenen Push ausführen muss
     */
    bool pushAnimationFrame(LGFX *tft, Gfx_Sprite *sprite, int spriteX, int spriteY) override;

    /**
     * @brief Zeichnet die Footer-Buttons.
     */
    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
     * @return true wenn ein vollständiger Redraw nötig ist
     */
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Liefert den Untertitel mit aktuellem Radius.
     */
    const char *getSubtitle() const override;

private:
    enum class LabelMode : uint8_t
    {
        Full,   ///< Symbol, Bezeichnung, Höhe und Geschwindigkeit
        Name,   ///< Symbol und Bezeichnung
        Symbol  ///< Nur Symbol
    };

    bool ensureSweepSprite(int sweepRadius); ///< Erstellt den Keil-Sprite des Sweeps bei Bedarf
    void deleteSweepSprite();             ///< Gibt den Sweep-Sprite frei
    bool ensureRadarBackgroundSprite(int w, int h, int cx, int cy, int radius, bool roundBottomLeft); ///< Erstellt den statischen Radarhintergrund samt Overlay
    void deleteRadarBackgroundSprite();    ///< Gibt die Radarhintergrund-Sprites frei
    bool buildOverlayPixelList();          ///< Extrahiert die sichtbaren Overlay-Pixel in eine Liste
    void deleteOverlayPixelList();         ///< Gibt die Overlay-Pixelliste frei
    void drawRadarOverlayLayer(Gfx_Sprite *sprite, int limitX, int limitY, int limitW, int limitH); ///< Zeichnet die Overlay-Pixel eines Bereichs über den Sweep
    float drawSweep(Gfx_Sprite *sprite, int cx, int cy, int radius); ///< Zeichnet den laufenden Sweep und liefert den Winkel, -1 wenn nichts gezeichnet wurde
    void renderRadarLayers(Gfx_Sprite *sprite, int count, bool hasBackground); ///< Zeichnet Sweep, Overlay-Ausschnitt, Marker und Beschriftungen
    void deleteProgressSprite();                  ///< Gibt den Fortschrittsbalken-Sprite frei
    void updateProgressBar(bool force);           ///< Zeichnet den Fortschrittsbalken bis zum nächsten Abruf
    void initFooterButtons(LGFX *tft);             ///< Initialisiert die Footer-Buttons
    void updateFooterButtonColors();               ///< Aktualisiert die Footer-Buttonfarben
    void drawLabelModeButtonSymbol(bool pressed);  ///< Zeichnet den Label-Modus-Button
    void toggleLabelMode();                        ///< Schaltet die Radar-Beschriftung weiter
    void setRadiusKm(uint8_t radiusKm);            ///< Setzt den Suchradius
    int aircraftListRowAt(int32_t x, int32_t y) const; ///< Liefert die berührte Listenzeile
    void showAircraftDetailDialog(uint8_t rowIndex); ///< Öffnet Details zu einer Listenzeile
    void showHelpDialog();                         ///< Öffnet die Kurzanleitung
    void onMsgResult(bool confirmed);              ///< Callback der MessageBox

    bool m_touchActive = false;        ///< Touch-Sequenz im Inhaltsbereich aktiv
    int32_t m_touchStartX = 0;         ///< Start-X der Touch-Sequenz
    int32_t m_touchStartY = 0;         ///< Start-Y der Touch-Sequenz
    LGFX *m_tft = nullptr;             ///< Display-Objekt für Sprite-Verwaltung
    Button m_radiusButtons[4];          ///< Radius-Buttons im Footer
    Button m_labelModeButton;           ///< Umschalter für die Radar-Beschriftung
    Button m_helpButton;                ///< Hilfe-Button im Footer
    bool m_buttonsInitialized = false;  ///< Footer-Buttons wurden initialisiert
    LabelMode m_labelMode = LabelMode::Full; ///< Aktueller Umfang der Radar-Beschriftung
    std::unique_ptr<MessageBox> m_msg;  ///< Modale Dialogbox für Hilfe und Flugzeugdetails
    Gfx_Sprite *m_sweepSprite = nullptr; ///< Keil-Sprite mit dem Verlauf der Scanline
    Gfx_Sprite *m_progressSprite = nullptr; ///< 1px Fortschrittsbalken am rechten Displayrand
    Gfx_Sprite *m_radarBackgroundSprite = nullptr; ///< Statischer Radarhintergrund
    Gfx_Sprite *m_radarOverlaySprite = nullptr; ///< Statisches Radaroverlay
    uint16_t *m_overlayPixels = nullptr; ///< Overlay-Pixel als Tripel aus X, Y und Farbe, relativ zur Radarbox
    int m_overlayPixelCount = 0;       ///< Anzahl der Einträge in der Overlay-Pixelliste
    int m_sweepSpriteRadius = 0;       ///< Sweep-Länge, für die der Keil-Sprite gerendert wurde
    int m_radarBackgroundW = 0;        ///< Breite des Radarhintergrunds
    int m_radarBackgroundH = 0;        ///< Höhe des Radarhintergrunds
    int m_radarBackgroundCx = 0;       ///< Mittelpunkt X im Radarhintergrund
    int m_radarBackgroundCy = 0;       ///< Mittelpunkt Y im Radarhintergrund
    int m_radarBackgroundRadius = 0;   ///< Radius im Radarhintergrund
    bool m_radarBackgroundCorner = false; ///< Radarhintergrund enthält die Rundung der linken unteren Ecke
    bool m_pushWholeBox = false;       ///< true wenn Workshop die gesamte Radarbox pushen muss statt der Einzelbereiche
    bool m_radarBoxValid = false;      ///< true wenn die Radar-Bounding-Box gesetzt ist
    int m_radarBoxX = 0;               ///< Linke Kante des animierten Bereichs
    int m_radarBoxY = 0;               ///< Obere Kante des animierten Bereichs
    int m_radarBoxW = 0;               ///< Breite des animierten Bereichs
    int m_radarBoxH = 0;               ///< Höhe des animierten Bereichs
    int m_radarCx = 0;                 ///< Mittelpunkt X des Radars
    int m_radarCy = 0;                 ///< Mittelpunkt Y des Radars
    int m_radarRadius = 0;             ///< Radius des Radars
    int m_aircraftListX = 0;           ///< Linke Kante der Flugzeugliste
    int m_aircraftListY = 0;           ///< Obere Kante der Flugzeugliste
    int m_aircraftListW = 0;           ///< Breite der Flugzeugliste
    int m_aircraftListH = 0;           ///< Höhe der Flugzeugliste
    int m_aircraftListVisible = 0;     ///< Anzahl sichtbarer Listenzeilen
    uint32_t m_listRedrawDueMs = 0;    ///< Zeitpunkt für das verzögerte Neuzeichnen der Liste, 0 = nichts offen
    uint8_t m_listPushBand = 0;        ///< Nächstes zu übertragendes Band der Liste
    uint8_t m_listPushBands = 0;       ///< Anzahl der Bänder einer offenen Listenübertragung, 0 = keine offen
    uint8_t m_searchRadiusKm = 10;      ///< Suchradius in Kilometern
    int m_lastAircraftCount = -1;      ///< zuletzt vollständig gezeichnete Flugzeuganzahl
    uint32_t m_lastDataVersion = 0;    ///< zuletzt vollständig gezeichnete Datenversion
    uint8_t m_lastRadarStatus = 0;     ///< zuletzt vollständig gezeichneter Radarstatus
    uint32_t m_lastProgressUpdateMs = 0; ///< letzte Aktualisierung des Fortschrittsbalkens
    mutable uint32_t m_nextFrameMs = 0; ///< frühester Zeitpunkt für den nächsten Animationsframe
    mutable bool m_framePending = false; ///< true wenn ein Animationsframe zum Zeichnen ansteht
    mutable char m_subtitleBuf[112] = {}; ///< Buffer für dynamischen Untertitel
    char m_aircraftDetailTitleBuf[64] = {}; ///< Buffer für Detaildialog-Titel
    char m_helpTitleBuf[48] = {};       ///< Buffer für Hilfe-Titel
    char m_helpTextBuf[768] = {};       ///< Buffer für Hilfe-Text
    char m_dialogCloseBuf[16] = {};     ///< Buffer für Schließen-Knopf
};

#endif // WORKSHOPTOOL_FLIGHTRADAR_H
