#ifndef WORKSHOPTOOL_GAME2048_H
#define WORKSHOPTOOL_GAME2048_H

// ============================================================================
// Erholungsspiel 2048 für die Werkstattpause
// ============================================================================

#include <stdint.h>

#include <memory>

#include "Button.h"
#include "MessageBox.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_Game2048
 * @brief Klassisches 2048-Schiebepuzzle mit Wisch-Steuerung.
 *
 * 4×4-Brett, Standard-Spielregeln nach Gabriele Cirullis Original: Beim Wischen rutschen alle Kacheln in die gewählte Richtung. Zwei gleichwertige Kacheln verschmelzen zur doppelten Summe, aber jede Kachel verschmilzt höchstens einmal pro Zug. Nach jedem gültigen Zug erscheint eine neue Kachel auf einem zufälligen Leerfeld, mit 90 % Wahrscheinlichkeit eine Zwei und mit 10 % eine Vier.
 *
 * Spielende: Erreicht eine Kachel den Wert 2048, erscheint eine Siegeinblendung mit zwei Optionen: "weiter" lässt das Spiel fortlaufen, "neu" startet eine neue Runde. Sind alle Felder belegt und in keiner Richtung ein Zug mehr möglich, erscheint die Einblendung zum Spielende mit einer Schaltfläche "neu".
 *
 * Persistenz: Der Bestwert liegt als uint32_t mit dem Schlüssel "hi" im Preferences-Namespace "ws2048".
 *
 * Steuerung: Wischgesten innerhalb des Spielbretts. Der Berührungsbeginn wird beim ersten Frame mit touched=true gespeichert. Beim Frame mit touched=false bestimmt die größere Auslenkung die Richtung und gilt ab SWIPE_MIN_PX als Zug. Eine kurze Berührung ohne ausreichende Bewegung bleibt wirkungslos.
 *
 * Layout (547 × 327 px Netto-Inhaltsbereich):
 *   TOP_GAP                  7 px
 *   Spielfeld 320 × 320 px (links)
 *   Rechte Spalte ab ox+332, Breite 215 px
 *     - Score-Karte ("Punkte")
 *     - Highscore-Karte ("Bestwert")
 *     - Wisch-Hinweis (Pfeil-Diagramm)
 *
 * Footer-Steuerung: Standardmäßig sind drei Schaltflächen sichtbar: "neu" für eine neue Runde, "reset" zum bestätigten Zurücksetzen des Bestwerts und "?" für den Spielregeldialog. Während der Siegeinblendung erscheint zusätzlich "weiter", um nach 2048 fortzusetzen.
 */
class WorkshopTool_Game2048 : public WorkshopTool
{
public:
    WorkshopTool_Game2048()                                           = default;
    ~WorkshopTool_Game2048() override                                 = default;

    WorkshopTool_Game2048(const WorkshopTool_Game2048 &)              = delete;
    WorkshopTool_Game2048 &operator=(const WorkshopTool_Game2048 &)   = delete;

    /**
     * @brief Initialisiert Spielzustand, Highscore und Footer-Buttons.
     * @param tft Display-Objekt für die Button-Klasse
     */
    void onEnter(LGFX *tft) override;

    /**
     * @brief Räumt den Werkzeugzustand auf.
     */
    void onExit() override;

    /**
     * @brief Zeichnet Spielfeld und rechte Infospalte in den Work-Sprite.
     * @param sprite Ziel-Sprite des Arbeitsbereichs
     * @param ox     X-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param oy     Y-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param w      Verfügbare Breite in Pixeln
     * @param h      Verfügbare Höhe in Pixeln
     */
    void draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse zur Wisch-Erkennung.
     * @return true, wenn ein Zug ausgeführt wurde und der Inhaltsbereich
     *         neu gezeichnet werden muss
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Zeichnet die Fußzeilenschaltflächen "neu", "reset", "?" und gegebenenfalls "weiter".
     */
    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
     * @return true, wenn ein Button-Druck Spielzustand und/oder Anzeige
     *         verändert hat und ein Komplett-Redraw nötig ist
     */
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Liefert einen statusabhängigen Untertitel.
     */
    const char *getSubtitle() const override;

private:
    // ========================================================================
    // INNERE STRUKTUREN
    // ========================================================================

    /**
     * @brief Wisch-Richtung als Achse + Vorzeichen.
     */
    enum class Direction : uint8_t
    {
        NONE  = 0,
        UP    = 1,
        DOWN  = 2,
        LEFT  = 3,
        RIGHT = 4
    };

    /**
     * @brief Spielzustand für UI-Logik (Overlays).
     */
    enum class GameState : uint8_t
    {
        PLAYING  = 0, ///< Normales Spiel
        WON      = 1, ///< 2048 wurde erreicht, Sieg-Overlay aktiv
        GAMEOVER = 2  ///< Brett voll, keine Züge mehr möglich
    };

    // ========================================================================
    // SPIELZUSTAND
    // ========================================================================

    /// Brettmatrix; Wert 0 bedeutet leer, sonst die echte Kachelzahl (2..2048..).
    uint16_t m_board[4][4] = {};

    uint32_t  m_score        = 0;                ///< Aktueller Punktestand
    uint32_t  m_highscore    = 0;                ///< Bisheriger Bestwert (aus Preferences)
    GameState m_state        = GameState::PLAYING; ///< Spielphase
    bool      m_winAcknowledged = false;         ///< true, wenn 2048 erreicht und "weiter" gewählt wurde

    // ========================================================================
    // TOUCH-TRACKING
    // ========================================================================

    bool    m_touchActive = false; ///< Touch läuft, m_touchStartX/Y und m_touchLastX/Y sind gültig
    int32_t m_touchStartX = 0;     ///< X-Position bei touched=true (erstes Frame)
    int32_t m_touchStartY = 0;     ///< Y-Position bei touched=true (erstes Frame)
    int32_t m_touchLastX  = 0;     ///< Letzte gesehene X-Position während touched=true
    int32_t m_touchLastY  = 0;     ///< Letzte gesehene Y-Position während touched=true

    // ========================================================================
    // FOOTER-BUTTONS
    // ========================================================================

    Button m_btnContinue;             ///< "weiter", nur während der Siegeinblendung sichtbar
    Button m_btnNew;                  ///< "neu", immer sichtbar
    Button m_btnReset;                ///< "reset", setzt den Bestwert nach Bestätigung zurück
    Button m_btnHelp;                 ///< "?", öffnet den Spielregeldialog
    bool   m_buttonsInitialized = false;

    // ========================================================================
    // MESSAGEBOX (Spielregeln)
    // ========================================================================

    /**
     * @brief Zweck des aktuell geöffneten MessageBox-Dialogs.
     */
    enum class MsgPurpose : uint8_t
    {
        NONE            = 0, ///< Keine Box aktiv
        HELP            = 1, ///< Spielregeln-Dialog
        RESET_HIGHSCORE = 2  ///< Bestätigungsdialog für Bestwert-Reset
    };

    std::unique_ptr<MessageBox> m_msg;                  ///< MessageBox-Instanz (nur während Benutzung)
    MsgPurpose                  m_msgPurpose = MsgPurpose::NONE; ///< Aktueller Dialog-Zweck

    // ========================================================================
    // I18N-PUFFER
    //
    // Button::initButton und die MessageBox-Setter speichern nur Zeiger. Deshalb müssen die Beschriftungspuffer als Member die Lebensdauer des Werkzeugs überdauern. getSubtitle() befüllt m_subtitleBuf bei jedem Aufruf neu.
    // ========================================================================

    char m_helpTitleBuf[40] = {};   ///< Puffer für den Hilfetitel
    char m_helpTextBuf[640] = {};   ///< Puffer für den Hilfetext
    char m_resetTitleBuf[40] = {};  ///< Puffer für den Titel des Rücksetzdialogs
    char m_resetTextBuf[256] = {};  ///< Puffer für den Text des Rücksetzdialogs
    char m_dialogCloseBuf[16] = {}; ///< Puffer für die Schaltfläche zum Schließen
    char m_dialogOkBuf[8] = {};     ///< Puffer für die Bestätigungsschaltfläche
    char m_dialogCancelBuf[16] = {};///< Puffer für die Abbruchschaltfläche
    char m_btnNewLabel[16] = {};    ///< Puffer für "neu"
    char m_btnResetLabel[16] = {};  ///< Puffer für "reset"
    char m_btnContinueLabel[16] = {}; ///< Puffer für "weiter"
    mutable char m_subtitleBuf[48] = {}; ///< Puffer für den dynamischen Untertitel

    // ========================================================================
    // SPIELLOGIK
    // ========================================================================

    /**
     * @brief Setzt das Brett zurück und legt zwei Startkacheln aus.
     */
    void resetGame();

    /**
     * @brief Wählt zufällig ein leeres Feld und legt eine 2 (90 %) oder 4 (10 %).
     * @return true wenn eine Kachel platziert wurde, false wenn das Brett voll war
     */
    bool spawnRandomTile();

    /**
     * @brief Führt einen Zug in die gewählte Richtung aus.
     * @param dir Wisch-Richtung
     * @return true, wenn sich mindestens eine Kachel bewegt oder verschmolzen hat
     *
     * Aktualisiert Score, Highscore und Spielzustand. Spawnt bei einem gültigen Zug automatisch eine neue Kachel.
     */
    bool applyMove(Direction dir);

    /**
     * @brief Schiebt eine Vierer-Sequenz nach links und verschmilzt Paare.
     * @param line          Eingangs-/Ausgangs-Array mit vier Werten
     * @param outScoreGain  Liefert die Punkte aller Verschmelzungen dieser Linie
     * @return true, wenn sich die Linie tatsächlich verändert hat
     *
     * Hilfsfunktion für applyMove(): jede der acht Spalten- bzw. Zeilen-richtungen wird auf eine Links-Operation einer Hilfslinie reduziert.
     */
    bool slideAndMergeLine(uint16_t line[4], uint32_t &outScoreGain);

    /**
     * @brief Prüft, ob in mindestens einer Richtung ein Zug möglich wäre.
     */
    bool hasAnyMove() const;

    /**
     * @brief Prüft, ob das Brett eine 2048-Kachel enthält.
     */
    bool boardContains2048() const;

    // ========================================================================
    // PERSISTENZ
    // ========================================================================

    /**
     * @brief Liest den Bestwert aus dem Preferences-Namespace "ws2048".
     */
    void loadHighscore();

    /**
     * @brief Schreibt den Bestwert in den Preferences-Namespace "ws2048".
     */
    void saveHighscore();

    // ========================================================================
    // RENDERING
    // ========================================================================

    /**
     * @brief Zeichnet das 4×4-Brett mit allen Kacheln.
     */
    void drawBoard(Gfx_Sprite *sprite, int boardX, int boardY) const;

    /**
     * @brief Zeichnet eine einzelne Kachel an der angegebenen Position.
     * @param sprite Ziel-Sprite
     * @param px     Linke Kante der Kachel (sprite-relativ)
     * @param py     Obere Kante der Kachel (sprite-relativ)
     * @param value  Kachelwert (0 = leeres Feld)
     */
    void drawTile(Gfx_Sprite *sprite, int px, int py, uint16_t value) const;

    /**
     * @brief Zeichnet die Infospalte (Score, Highscore, Hinweis, Status).
     */
    void drawSidebar(Gfx_Sprite *sprite, int sideX, int sideY) const;

    /**
     * @brief Zeichnet ein zentriertes Overlay für Sieg- oder Game-Over-Status.
     */
    void drawOverlay(Gfx_Sprite *sprite, int boardX, int boardY) const;

    // ========================================================================
    // FARB- UND STILTABELLEN
    // ========================================================================

    /**
     * @brief Liefert Hintergrund- und Textfarbe einer Kachel anhand ihres Werts.
     * @param value     Kachelwert (Null = leer)
     * @param outBg     Hintergrundfarbe (RGB565)
     * @param outFg     Textfarbe (RGB565)
     */
    static void tilePalette(uint16_t value, uint16_t &outBg, uint16_t &outFg);

    // ========================================================================
    // FOOTER-HELFER
    // ========================================================================

    /**
     * @brief Initialisiert die beiden Footer-Buttons mit Position und Stil.
     */
    void initFooterButtons(LGFX *tft);

    /**
     * @brief Öffnet den Spielregeln-Dialog als modale MessageBox.
     */
    void showHelpDialog();

    /**
     * @brief Öffnet den Bestätigungsdialog zum Zurücksetzen des Bestwerts.
     */
    void showResetDialog();

    /**
     * @brief Callback der MessageBox nach Bestätigung oder Abbruch.
     */
    void onMsgResult(bool confirmed);
};

#endif // WORKSHOPTOOL_GAME2048_H
