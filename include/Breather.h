#ifndef BREATHER_H
#define BREATHER_H

#include <stddef.h>
#include <stdint.h>

#include "BaseModule.h"
#include "Button.h"
#include "Gfx.h"

// ===========================================================================
// Sitzungs- und Übungs-Datenmodell
// ===========================================================================

/**
 * @brief Bauart der Übung. Bestimmt Animations- und Zeitlogik der Visualisierung.
 */
enum class ExerciseKind : uint8_t
{
    BREATHING_BOX,   ///< Atmung 4-4-4-4
    BREATHING_478,   ///< Atmung 4-7-8
    NECK_TILT,       ///< Kopf seitlich neigen, links und rechts
    SHOULDER_ROLL,   ///< Schulterkreisen
    SHOULDER_LIFT,   ///< Schultern hochziehen und fallen lassen
    EYE_FAR_NEAR,    ///< Augen-Wechsel zwischen Fern- und Nahpunkt
    EYE_PALMING,     ///< Augen mit den Handflächen abdunkeln
    EYE_CIRCLE,      ///< Augen kreisen lassen
    SPINE_TWIST,     ///< Wirbelsäulen-Drehung im Sitzen, Wechsel links und rechts
    WRIST_CIRCLE,    ///< Handgelenke kreisen
    HIP_OPENER,      ///< Hüftöffner im Stand oder Sitzen
    STAND_AND_REACH, ///< Aufstehen und nach oben strecken
    REST             ///< Ruhepunkt zwischen Übungen
};

/**
 * @brief Eine einzelne Übung als Bestandteil einer Sitzung.
 */
struct BreatherExercise
{
    ExerciseKind kind;       ///< Bauart der Übung
    const char *title;       ///< Kurzer Übungstitel
    const char *instruction; ///< Anweisungstext, ggf. mit Zeilenumbrüchen
    uint16_t durationSec;    ///< Dauer in Sekunden
};

/**
 * @brief Eine Sitzung als Folge von Übungen.
 */
struct BreatherSession
{
    const char *title;                ///< Titel der Sitzung
    const char *subtitle;             ///< Kurzbeschreibung, z. B. "Atem und Schultern lockern"
    uint16_t totalDurationSec;        ///< Gesamtdauer aller Übungen in Sekunden
    uint16_t accentColor;             ///< RGB565-Akzentfarbe für Kachel-Streifen, Fortschrittsbalken und Übungs-Titel
    const BreatherExercise *exercises; ///< Zeiger auf Übungs-Array
    uint8_t exerciseCount;            ///< Anzahl Übungen im Array
};

// ===========================================================================
// Modul-Bauarten und Phasen
// ===========================================================================

/**
 * @brief Aktueller Bildschirm des Breather-Moduls.
 */
enum class BreatherScreen : uint8_t
{
    SELECTION, ///< Sitzungs-Auswahl als Kachel-Raster
    RUNNING,   ///< Eine Sitzung läuft
    DONE       ///< Sitzung abgeschlossen, Bestätigungs-Bildschirm
};

/**
 * @brief Callbacks für das Breather-Modul.
 */
struct BreatherCallbacks
{
    void (*onExitRequested)() = nullptr;             ///< Aufruf beim Verlassen
    void (*onPlaySound)(const char *path) = nullptr; ///< Sound von SD-Karte abspielen
};

/**
 * @brief Breather-Modul für kurze Erholungs-, Atem- und Bewegungs-Sitzungen am Schreibtisch.
 *
 * Ablauf:
 * - Beim Eintritt zeigt das Modul ein Kachel-Raster mit sechs Sitzungen. Die obere Reihe ist nach Stimmung gegliedert (angespannt, müde, zappelig), die untere Reihe enthält gezielte Übungen (Augen, zwei Atemvarianten).
 * - Eine ausgewählte Sitzung läuft als Folge einzelner Übungen ab. Pro Übung erscheint eine Visualisierung zusammen mit einem Anweisungstext.
 * - Phasenwechsel zwischen Übungen werden durch einen kurzen Sound markiert. Fehlt die Sounddatei, verläuft der Phasenwechsel still.
 * - Während der Sitzung kann der Anwender pausieren, eine Übung überspringen oder die Sitzung verlassen.
 *
 * Architektur:
 * - Rein lokales Modul ohne Backend, millis()-basierte Zeitsteuerung.
 * - Sitzungs- und Übungsdefinitionen sind statisch im anonymen Namespace der .cpp abgelegt. Jede Sitzung trägt ihre eigene Akzentfarbe, die für Kachelstreifen, Fortschrittsbalken und Übungs-Titel verwendet wird.
 * - Ein persistentes PSRAM-Visualisierungs-Sprite hält den statischen Hintergrund einer animierten Übung. Bewegungs-Übungen aktualisieren pro Frame nur eine kleine Bounding-Box im Sprite und pushen sie via Display-Clip atomar aufs Display.
 * - Die animierte Kugel liegt als kleines, einmal pro Übung mit Antialiasing gezeichnetes Eigen-Sprite vor und wird per Sprite-zu-Sprite-Push in das Visualisierungs-Sprite kopiert.
 */
class Breather : public BaseModule
{
public:
    Breather();
    ~Breather();

    Breather(const Breather &) = delete;
    Breather &operator=(const Breather &) = delete;
    Breather(Breather &&) = delete;
    Breather &operator=(Breather &&) = delete;

    /**
     * @brief Initialisiert das Breather-Modul.
     * @param tft Referenz auf das Display-Objekt
     * @param callbacks Callback-Funktionen
     * @return true bei Erfolg
     */
    bool init(LGFX &tft, const BreatherCallbacks &callbacks);

    /**
     * @brief Hauptzeichenfunktion, wird zyklisch aufgerufen.
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück.
     * @return AppModule::BREATHER
     */
    AppModule type() const override;

private:
    // =======================================================================
    // Sprachwechsel
    // =======================================================================

    void redrawAll(); ///< Vollredraw nach Sprachwechsel, aktualisiert Button-Labels und erzwingt Bildschirm-Aufbau

    // =======================================================================
    // Ablaufsteuerung
    // =======================================================================

    void startSession(uint8_t sessionIndex); ///< Wechselt in den RUNNING-Bildschirm und startet die erste Übung
    void advanceExercise();                  ///< Geht zur nächsten Übung oder beendet die Sitzung
    void finishSession();                    ///< Wechselt in den DONE-Bildschirm
    void returnToSelection();                ///< Wechselt zurück zur Auswahl
    void togglePause();                      ///< Pausiert oder setzt eine laufende Sitzung fort
    void skipExercise();                     ///< Bricht die aktuelle Übung ab und springt zur nächsten

    // =======================================================================
    // Zeitberechnung
    // =======================================================================

    uint16_t getElapsedSecondsInExercise() const; ///< Verstrichene Sekunden in der aktuellen Übung
    uint32_t getElapsedMsInExercise() const;      ///< Verstrichene Millisekunden in der aktuellen Übung
    uint16_t getElapsedSecondsInSession() const;  ///< Verstrichene Sekunden insgesamt in der Sitzung

    // =======================================================================
    // Zeichnen: Auswahl-Bildschirm
    // =======================================================================

    void drawSelectionScreen();                             ///< Vollständiger Aufbau der Sitzungs-Auswahl
    void drawSelectionTile(uint8_t sessionIndex);           ///< Zeichnet eine einzelne Sitzungs-Kachel
    int8_t findSelectionTileAt(int32_t x, int32_t y) const; ///< Liefert Kachel-Index bei (x,y) oder -1

    // =======================================================================
    // Zeichnen: laufende Sitzung
    // =======================================================================

    void drawRunningStaticLayout(); ///< Zeichnet statische Elemente einmalig
    void drawRunningHeader();       ///< Sitzungs-Titel und Übungsfortschritt oben
    void drawProgressBar();         ///< Gesamt-Fortschrittsbalken oben
    void drawExerciseTitle();       ///< Übungs-Titel über der Visualisierung
    void drawInstruction();         ///< Anweisungstext rechts neben der Visualisierung
    void drawVisualization();       ///< Visualisierung im persistenten PSRAM-Sprite, Display-Updates per geclipptem Push
    void drawRemainingTime();       ///< Verbleibende Sekunden der aktuellen Übung
    void updateRunningControls();   ///< Pause-/Skip-Button-Beschriftung anpassen

    // =======================================================================
    // Visualisierungs-Zeichner
    // =======================================================================

    bool renderBreathingCircle(const uint16_t *phaseSecsArr, const char *const *phaseLabels, uint8_t phaseCount); ///< Generischer Atemkreis, liefert false wenn kein Redraw nötig war
    bool renderBreathingBox();                                                                                    ///< 4-4-4-4 Atemkreis
    bool renderBreathing478();                                                                                    ///< 4-7-8 Atemkreis
    bool renderPendulum(float periodSec, const char *axisLabel);                                                  ///< Pendelnde Bewegung links-rechts, liefert false wenn die Pixelposition unverändert ist
    bool renderRotation(float periodSec);                                                                         ///< Kreisende Bewegung, liefert false wenn die Pixelposition unverändert ist
    void renderEyeFarNear();                                                                                      ///< Wechsel zwischen kleinem und großem Kreis
    void renderEyePalming();                                                                                      ///< Statisches abgedunkeltes Symbol
    bool renderVerticalBob(float periodSec);                                                                      ///< Auf und ab pendelnde Bewegung, liefert false wenn die Pixelposition unverändert ist
    void renderRest();                                                                                            ///< Ruhiger statischer Punkt
    void prepareDotSprite();                                                                                      ///< Zeichnet die animierte Kugel einmalig in m_dotSprite

    // =======================================================================
    // Zeichnen: Abschluss-Bildschirm
    // =======================================================================

    void drawDoneScreen(); ///< Bestätigung "Geschafft" mit Sitzungs-Daten

    // =======================================================================
    // Zeichnen: gemeinsame Elemente
    // =======================================================================

    void drawDigitalClock(int32_t x, int32_t y); ///< Digitaluhr unten links

    // =======================================================================
    // Touch-Handling
    // =======================================================================

    void handleTouch(int32_t tx, int32_t ty, bool touched);          ///< Top-Level-Verteiler
    void handleSelectionTouch(int32_t tx, int32_t ty, bool touched); ///< Auswahl-Bildschirm
    void handleRunningTouch(int32_t tx, int32_t ty, bool touched);   ///< Laufende Sitzung
    void handleDoneTouch(int32_t tx, int32_t ty, bool touched);      ///< Abschluss-Bildschirm

    // =======================================================================
    // Hilfsfunktionen
    // =======================================================================

    enum class BreatherFontId : uint8_t
    {
        NONE,             ///< Kein Font aktiv
        LEAGUE_GOTHIC34,  ///< League Gothic Condensed 34
        COMFORTAA_BOLD14, ///< Comfortaa Bold 14
        COMFORTAA_BOLD16  ///< Comfortaa Bold 16
    };

    void useVizFont(BreatherFontId fontId);                                      ///< Lädt einen Visualisierungs-Font nur bei echtem Wechsel
    void unloadVizFont();                                                       ///< Entlädt den aktiven Visualisierungs-Font
    void playPhaseSound();                                                      ///< Spielt den Phasenwechsel-Sound
    static void formatDurationShort(uint16_t totalSec, char *out, size_t outSize); ///< "M:SS" oder "MM:SS"
    static void formatSeconds(uint16_t sec, char *out, size_t outSize);            ///< "SS" oder "M:SS"

    // =======================================================================
    // Member-Variablen
    // =======================================================================

    // Core
    LGFX *m_tft = nullptr;        ///< Display-Referenz
    BreatherCallbacks m_callbacks; ///< Callback-Funktionen
    bool m_initialized = false;   ///< Modul erfolgreich initialisiert

    // Buttons
    Button m_exitButton;  ///< Verlassen des Moduls, unten rechts
    Button m_backButton;  ///< Zurück zur Sitzungs-Auswahl, nur RUNNING
    Button m_pauseButton; ///< Pause/Weiter, nur RUNNING
    Button m_skipButton;  ///< Übung überspringen, nur RUNNING
    Button m_doneButton;  ///< Fertig, nur DONE

    // Bildschirm-Zustand
    BreatherScreen m_screen = BreatherScreen::SELECTION; ///< Aktiver Bildschirm
    bool m_screenChanged = true;                       ///< Layout muss neu aufgebaut werden
    bool m_touchHandover = true;                       ///< Erster Touch nach Modulstart wird ignoriert, bis einmal losgelassen wurde
    int8_t m_pressedSelectionTile = -1;                ///< Gedrückte Auswahl-Kachel oder -1
    int8_t m_lastDrawnPressedTile = -1;                ///< Zuletzt mit Druck-Optik gezeichnete Kachel, für Change-Detection auf dem Auswahl-Bildschirm

    // Sitzung und Übung
    int8_t m_currentSession = -1; ///< Index der laufenden Sitzung, -1 = keine
    uint8_t m_exerciseIndex = 0;  ///< Index der aktuellen Übung in der Sitzung
    bool m_paused = false;        ///< Sitzung pausiert

    // Zeitsteuerung
    unsigned long m_exerciseStartMs = 0; ///< millis() bei Übungsstart
    unsigned long m_pauseStartMs = 0;    ///< millis() bei Pausenbeginn
    unsigned long m_totalPausedMs = 0;   ///< Kumulierte Pausenzeit der aktuellen Übung
    unsigned long m_doneShownAtMs = 0;   ///< millis() beim Wechsel zu DONE

    // Sprites - Visualisierung als Einzel-Sprite mit Clip-Push
    Gfx_Sprite *m_vizSprite = nullptr; ///< PSRAM-Sprite für die Visualisierung
    Gfx_Sprite *m_dotSprite = nullptr; ///< Kleines Sprite mit der animierten Kugel, einmalig pro Übung gezeichnet

    // Sprites - übrige Bereiche
    Gfx_Sprite *m_headerSprite = nullptr;      ///< Sitzungs-Titel und Übungsfortschritt oben
    Gfx_Sprite *m_progressSprite = nullptr;    ///< Gesamt-Fortschrittsbalken oben
    Gfx_Sprite *m_titleSprite = nullptr;       ///< Übungs-Titel rechts
    Gfx_Sprite *m_instructionSprite = nullptr; ///< Anweisungstext rechts
    Gfx_Sprite *m_remainingSprite = nullptr;   ///< Verbleibende Zeit rechts
    Gfx_Sprite *m_clockSprite = nullptr;       ///< Digitaluhr unten links
    BreatherFontId m_vizFont = BreatherFontId::NONE; ///< Aktiver Font im Visualisierungs-Sprite
    int m_lastDrawnSecond = -1;                ///< Letzte gezeichnete Sekunde der Digitaluhr

    // Anzeige-Caches für Change-Detection
    int m_lastDisplayedRemaining = -1;      ///< Zuletzt gezeichnete Restsekunden der Übung
    uint8_t m_lastDisplayedExercise = 0xFF; ///< Zuletzt gezeichneter Übungs-Index
    int m_lastDisplayedProgress = -1;       ///< Zuletzt gezeichneter Fortschritt in Pixel
    bool m_pauseLabelIsResume = false;      ///< Pause-Button zeigt aktuell "WEITER"

    // Visualisierungs-Caches für Change-Detection
    bool m_vizStaticDrawn = false;        ///< Statische Visualisierung wurde bereits gezeichnet
    bool m_vizStaticInitialized = false;  ///< Statischer Hintergrund einer animierten Bewegungs-Übung wurde gezeichnet
    int m_lastDotX = 0;                   ///< Letzte X-Position des Kugel-Mittelpunkts in Sprite-Koordinaten
    int m_lastDotY = 0;                   ///< Letzte Y-Position des Kugel-Mittelpunkts in Sprite-Koordinaten
    int m_lastBreathRadius = -1;          ///< Zuletzt gezeichneter Atemkreis-Radius in Pixel
    uint8_t m_lastBreathPhase = 0xFF;     ///< Zuletzt gezeichnete Atemphase
    bool m_eyeFarNearDrawn = false;       ///< EyeFarNear wurde mindestens einmal gezeichnet
    bool m_lastEyeFarPhase = false;       ///< Zuletzt gezeichnete Phase bei EyeFarNear, true = fern

    // Push-Bounding-Box für geclippte Pushes auf das Display
    bool m_vizPushClipped = false;        ///< true wenn der nächste Push auf eine Bounding-Box geclippt werden soll
    int m_vizPushX = 0;                   ///< X der Push-Bounding-Box in Sprite-Koordinaten
    int m_vizPushY = 0;                   ///< Y der Push-Bounding-Box in Sprite-Koordinaten
    int m_vizPushW = 0;                   ///< Breite der Push-Bounding-Box
    int m_vizPushH = 0;                   ///< Höhe der Push-Bounding-Box
};

#endif // BREATHER_H
