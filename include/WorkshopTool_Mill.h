#ifndef WORKSHOPTOOL_MILL_H
#define WORKSHOPTOOL_MILL_H

// ============================================================================
// Brettspiel Mühle für die Werkstattpause
// ============================================================================

#include <stdint.h>
#include <memory>

#include "Button.h"
#include "MessageBox.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_Mill
 * @brief Klassisches Brettspiel Mühle gegen einen Bot-Gegner.
 *
 * Spielregeln nach den verbreiteten Konventionen:
 *
 * Setzphase: Jeder Spieler hat neun Steine, die abwechselnd auf einen freien Punkt gesetzt werden. Der Spieler ist Weiß und beginnt, der Bot ist Schwarz.
 *
 * Zugphase: Sobald alle 18 Steine gesetzt sind, verschiebt jeder Zug einen eigenen Stein entlang einer Linie auf einen benachbarten freien Punkt.
 *
 * Sprungphase: Sobald ein Spieler nur noch drei Steine besitzt, darf er mit jedem eigenen Stein auf einen beliebigen freien Punkt springen.
 *
 * Mühle und Schlag: Wer drei eigene Steine entlang einer Linie zur Mühle schließt, schlägt einen gegnerischen Stein. Steine in einer gegnerischen Mühle dürfen nicht geschlagen werden, außer wenn alle gegnerischen Steine in Mühlen stehen.
 *
 * Spielende: Sieg, sobald der Gegner weniger als drei Steine hat oder keinen gültigen Zug mehr machen kann. Remis bei 50 aufeinanderfolgenden Halbzügen ohne geschlossene Mühle oder bei dreifacher Stellungswiederholung.
 *
 * Bot-Gegner: Minimax mit Alpha-Beta-Pruning, Suchtiefe abhängig von der Stufe (leicht/mittel). Eine künstliche Mindest-Denkzeit verhindert das "Teleportieren" des Bot-Steins. Beim Wechsel der Stufe per Footer-Toggle läuft die aktuelle Partie weiter; ab dem nächsten Bot-Zug gilt die neue Tiefe.
 *
 * Persistenz: Drei Statistiken für gewonnene, verlorene und unentschiedene Partien werden im Preferences-Namespace "wsmill" abgelegt. Die gewählte Schwierigkeit wird nicht persistiert; jede neue Werkzeugsitzung startet auf "leicht".
 *
 * Steuerung: In der Setzphase setzt ein Tap auf einen freien Punkt einen Stein. In der Zug-/Sprungphase wählt der erste Tap einen eigenen Stein aus (Auswahl-Ring), der zweite Tap führt zum freien Zielpunkt. Ein Tap auf einen anderen eigenen Stein wechselt die Auswahl, ein Tap ins Leere oder auf einen gegnerischen Stein hebt sie auf. Schließt der Spieler eine Mühle, wechselt das Spiel in den Schlag-Modus: alle schlagbaren gegnerischen Steine werden mit rotem Ring markiert.
 *
 * Layout (547 × 327 px Netto-Inhaltsbereich):
 *   TOP_GAP                  7 px (vertikal mittig im Inhaltsbereich)
 *   Spielbrett 320 × 320 px (links)
 *   Spalten-Lücke           12 px
 *   Sidebar 215 × 320 px (rechts)
 *     - Phase-Karte
 *     - Spieler-am-Zug-Karte
 *     - Steine-Karte (in Hand / am Brett)
 *     - Hinweis-Karte (Statuszeile)
 *     - Statistik-Karte (Siege / Niederlagen / Remis)
 *
 * Footer-Steuerung: Fünf Schaltflächen: "leicht" und "mittel" für die Bot-Stärke, "neu" für eine neue Partie, "reset" zum bestätigten Zurücksetzen der Statistik und "?" zum Öffnen der Spielanleitung. Während der Sieg-, Niederlage- oder Remiseinblendung verschwinden die Umschalt- und Rücksetzschaltflächen, während "neu" und "?" verfügbar bleiben.
 */
class WorkshopTool_Mill : public WorkshopTool
{
public:
    WorkshopTool_Mill();
    ~WorkshopTool_Mill() override                       = default;

    WorkshopTool_Mill(const WorkshopTool_Mill &)            = delete;
    WorkshopTool_Mill &operator=(const WorkshopTool_Mill &) = delete;

    /**
     * @brief Initialisiert Spielzustand, Statistik und Footer-Buttons.
     * @param tft Display-Objekt für die Button-Klasse
     */
    void onEnter(LGFX *tft) override;

    /**
     * @brief Räumt den Werkzeugzustand auf (Footer-Buttons abmelden).
     */
    void onExit() override;

    /**
     * @brief Zeichnet Brett und Sidebar in den Work-Sprite.
     * @param sprite Ziel-Sprite (m_workSprite des Workshop-Moduls)
     * @param ox     X-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param oy     Y-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param w      Verfügbare Breite in Pixeln
     * @param h      Verfügbare Höhe in Pixeln
     */
    void draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf dem Spielbrett.
     * @return true, wenn der Inhaltsbereich neu gezeichnet werden muss
     *
     * Nutzt Touch-up-Semantik (Aktion bei touched=false) mit Nearest-Point-Hittest gegen die 24 Brettpunkte.
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Zeichnet die Footer-Buttons abhängig vom Spielzustand.
     */
    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
     * @return true, wenn ein Komplett-Redraw nötig ist
     */
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Liefert einen statusabhängigen Untertitel.
     */
    const char *getSubtitle() const override;

    // Öffentliche innere Typen Cell und Position sind öffentlich, da lokale Hilfsfunktionen in der Implementierung darauf zugreifen. Alle anderen inneren Typen bleiben ausschließlich für Memberfunktionen zugänglich.

    /**
     * @brief Belegung eines Brettpunkts.
     */
    enum class Cell : uint8_t
    {
        EMPTY = 0, ///< Leerer Punkt
        WHITE = 1, ///< Spieler-Stein
        BLACK = 2  ///< Bot-Stein
    };

    /**
     * @brief Statische Snapshot-Struktur des kompletten Spielfelds für
     *        die Bot-Suche und die Stellungswiederholungs-Erkennung.
     */
    struct Position
    {
        Cell    cells[24];        ///< Belegung der 24 Brettpunkte
        uint8_t whiteInHand;      ///< Noch zu setzende weiße Steine
        uint8_t blackInHand;      ///< Noch zu setzende schwarze Steine
        uint8_t whiteOnBoard;     ///< Anzahl weißer Steine am Brett
        uint8_t blackOnBoard;     ///< Anzahl schwarzer Steine am Brett
        Cell    sideToMove;       ///< Wer ist am Zug
    };

private:
    // ========================================================================
    // PRIVATE INNERE TYPEN
    // ========================================================================

    /**
     * @brief Spielphase eines Spielers (gilt asymmetrisch pro Seite).
     */
    enum class Phase : uint8_t
    {
        PLACING = 0, ///< Spieler hat noch Steine in Hand
        MOVING  = 1, ///< Spieler hat alle Steine gesetzt, mehr als drei am Brett
        FLYING  = 2  ///< Spieler hat genau drei Steine, darf springen
    };

    /**
     * @brief Globaler Spielzustand für die UI-Logik.
     */
    enum class GameState : uint8_t
    {
        PLAYING  = 0, ///< Normales Spiel läuft
        WON      = 1, ///< Spieler hat gewonnen
        LOST     = 2, ///< Bot hat gewonnen
        DRAW     = 3  ///< Remis
    };

    /**
     * @brief Bot-Schwierigkeitsstufe (steuert die Suchtiefe).
     */
    enum class Difficulty : uint8_t
    {
        EASY   = 0,
        MEDIUM = 1
    };

    /**
     * @brief Sub-Status für die Schlag-Auswahl nach einer geschlossenen Mühle.
     */
    enum class CaptureMode : uint8_t
    {
        NONE         = 0, ///< Kein Schlag fällig
        HUMAN_CHOOSE = 1  ///< Spieler muss einen schlagbaren Bot-Stein wählen
    };

    // ========================================================================
    // SPIELZUSTAND
    // ========================================================================

    Position    m_pos{};                                  ///< Aktueller Spielzustand
    GameState   m_state            = GameState::PLAYING;  ///< UI-Spielzustand
    CaptureMode m_capture          = CaptureMode::NONE;   ///< Steht ein Schlag aus?
    int         m_selected         = -1;                  ///< Ausgewählter eigener Stein in der Zugphase, -1 wenn keiner
    uint8_t     m_movesSinceMill   = 0;                   ///< Halbzüge seit letzter Mühle (50-Züge-Regel)

    /// Statistik im Preferences-Namespace "wsmill"
    uint32_t m_stWon  = 0;
    uint32_t m_stLost = 0;
    uint32_t m_stDraw = 0;

    /// Schwierigkeitsstufe (nicht persistiert, immer EASY beim Eintritt)
    Difficulty m_difficulty = Difficulty::EASY;

    /// Kompakte Snapshot-Hashes der letzten Stellungen für die
    /// dreifache-Wiederholungs-Erkennung. Wird beim Beginn einer neuen
    /// Mühle-Periode (Schlag) zurückgesetzt.
    static constexpr int HISTORY_CAP = 64;
    uint64_t m_history[HISTORY_CAP] = {};
    int      m_historyCount = 0;

    /// Hatte der Spieler zuletzt resetGame() ausgelöst, ohne dass das
    /// Ergebnis schon gewertet wurde? Verhindert doppelte Statistik-Updates.
    bool m_resultBooked = false;

    // ========================================================================
    // BOT-DENKZEIT
    // ========================================================================

    /// Wenn != 0: Zeitpunkt (millis), ab dem der Bot ziehen darf. Sorgt
    /// für eine künstliche Mindest-Denkzeit, damit der Zug nicht "blitzt".
    uint32_t m_botMoveAt = 0;

    /// Wenn true: der Bot ist am Zug und muss bei der nächsten draw()-
    /// oder Touch-Tick-Auswertung beschäftigt werden.
    bool m_botPending = false;

    // ========================================================================
    // TOUCH-TRACKING
    // ========================================================================

    bool    m_touchActive = false; ///< Touch-Sequenz läuft (touched=true gesehen)
    int32_t m_touchLastX  = 0;     ///< Letzte gesehene X-Position während touched=true
    int32_t m_touchLastY  = 0;     ///< Letzte gesehene Y-Position während touched=true

    // ========================================================================
    // GEOMETRIE-CACHE
    // ========================================================================

    /// Origin des Tools innerhalb des Work-Sprites, gesetzt in draw().
    /// Wird in handleTouch() benötigt, um absolute Display-Koordinaten in
    /// brett-relative Koordinaten umzurechnen.
    int m_originX = 0;
    int m_originY = 0;

    // ========================================================================
    // FOOTER-BUTTONS
    // ========================================================================

    Button m_btnEasy;            ///< Umschaltung auf Schwierigkeit "leicht"
    Button m_btnMedium;          ///< Umschaltung auf Schwierigkeit "mittel"
    Button m_btnNew;             ///< Neue Partie starten
    Button m_btnReset;           ///< Statistik zurücksetzen (mit Bestätigungsdialog)
    Button m_btnHelp;            ///< Spielanleitung anzeigen
    bool   m_buttonsInitialized = false;

    /// Modale Dialogbox für Bestätigung und Spielanleitung. Wird in
    /// onEnter() angelegt und in onExit() per reset() vollständig
    /// freigegeben, damit nach dem Verlassen des Tools nichts zurückbleibt.
    std::unique_ptr<MessageBox> m_msg;

    /// Aktueller Zweck der MessageBox. Wird im Callback ausgewertet, um
    /// zwischen Reset-Bestätigung und reiner Hilfe-Anzeige zu unterscheiden.
    enum class MsgPurpose : uint8_t
    {
        NONE       = 0,
        RESET_STATS = 1,
        HELP        = 2
    };
    MsgPurpose m_msgPurpose = MsgPurpose::NONE;

    // ========================================================================
    // I18N-PUFFER
    //
    // Button::initButton und die MessageBox-Setter speichern nur Zeiger. Deshalb müssen die Beschriftungspuffer als Member die Lebensdauer des Werkzeugs überdauern. getSubtitle() befüllt m_subtitleBuf bei jedem Aufruf neu.
    // ========================================================================

    char m_helpTitleBuf[40] = {};   ///< Puffer für den Hilfetitel
    char m_helpTextBuf[512] = {};   ///< Puffer für den Hilfetext
    char m_resetTitleBuf[40] = {};  ///< Puffer für den Titel des Rücksetzdialogs
    char m_resetTextBuf[256] = {};  ///< Puffer für den Text des Rücksetzdialogs
    char m_dialogCloseBuf[16] = {}; ///< Puffer für die Schaltfläche zum Schließen
    char m_dialogOkBuf[8] = {};     ///< Puffer für die Bestätigungsschaltfläche
    char m_dialogCancelBuf[16] = {};///< Puffer für die Abbruchschaltfläche
    char m_btnEasyLabel[16] = {};   ///< Puffer für "leicht"
    char m_btnMediumLabel[16] = {}; ///< Puffer für "mittel"
    char m_btnNewLabel[16] = {};    ///< Puffer für "neu"
    char m_btnResetLabel[16] = {};  ///< Puffer für "reset"
    mutable char m_subtitleBuf[64] = {}; ///< Puffer für den dynamischen Untertitel

    // ========================================================================
    // SPIEL-LOGIK
    // ========================================================================

    /**
     * @brief Setzt das Brett zurück auf Startzustand für eine neue Partie.
     */
    void resetGame();

    /**
     * @brief Wendet einen menschlichen Setz-/Zug-/Sprung-Tap auf den
     *        Punkt @p idx an.
     * @return true, wenn die Aktion einen Redraw erfordert
     */
    bool handleHumanTapOnPoint(int idx);

    /**
     * @brief Wendet einen Schlag-Tap im CaptureMode::HUMAN_CHOOSE auf
     *        den Punkt @p idx an.
     * @return true, wenn ein Stein geschlagen wurde
     */
    bool handleHumanCaptureTap(int idx);

    /**
     * @brief Setzt einen Stein der angegebenen Farbe auf @p idx und
     *        prüft auf eine neu entstandene Mühle.
     * @return true, wenn dadurch eine Mühle geschlossen wurde
     */
    bool placeStone(Position &p, int idx, Cell color) const;

    /**
     * @brief Bewegt einen Stein von @p from nach @p to (unverändert
     *        durch Spring-/Zug-Phase).
     * @return true, wenn dadurch eine Mühle geschlossen wurde
     */
    bool moveStone(Position &p, int from, int to) const;

    /**
     * @brief Entfernt einen Stein der gegnerischen Farbe vom Brett.
     */
    void removeStone(Position &p, int idx) const;

    /**
     * @brief Wechselt die am Zug befindliche Farbe in @p p.
     */
    void switchSide(Position &p) const;

    /**
     * @brief Ermittelt die Phase eines Spielers in der gegebenen Stellung.
     */
    Phase phaseOf(const Position &p, Cell color) const;

    /**
     * @brief Prüft, ob der Stein an @p idx Teil einer geschlossenen
     *        Mühle des Eigentümers ist.
     */
    bool isInMill(const Position &p, int idx) const;

    /**
     * @brief Prüft, ob der Punkt @p idx aus Sicht von @p attacker
     *        legal geschlagen werden darf (Mühle-Ausnahme beachten).
     */
    bool canCapture(const Position &p, int idx, Cell attacker) const;

    /**
     * @brief Liefert true, wenn @p attacker mindestens einen Stein
     *        ausserhalb einer Mühle hat (klassische "alle in Mühlen"-
     *        Ausnahmeregel).
     */
    bool hasNonMillStones(const Position &p, Cell defender) const;

    /**
     * @brief Prüft, ob der angegebene Spieler in der gegebenen Stellung
     *        überhaupt einen legalen Zug ausführen kann.
     */
    bool hasAnyMove(const Position &p, Cell color) const;

    /**
     * @brief Aktualisiert den globalen Spielzustand nach einem Halbzug
     *        (Sieg/Niederlage/Remis-Erkennung).
     */
    void updateGameState();

    /**
     * @brief Bucht das aktuelle Endergebnis genau einmal in der Statistik.
     */
    void bookResultIfNeeded();

    // ========================================================================
    // BOT
    // ========================================================================

    /**
     * @brief Plant einen Bot-Zug ein (setzt m_botPending und Mindestzeit).
     */
    void scheduleBotMove();

    /**
     * @brief Führt den eingeplanten Bot-Zug aus, sobald die Mindest-
     *        Denkzeit abgelaufen ist.
     * @return true, wenn ein Zug ausgeführt wurde (Redraw erforderlich)
     */
    bool tickBot();

    /**
     * @brief Berechnet den besten Bot-Zug per Minimax mit Alpha-Beta.
     * @param outFrom Quellpunkt (-1 in der Setzphase)
     * @param outTo   Zielpunkt (Setz-/Zug-Ziel)
     * @param outCap  Geschlagener Stein (-1 wenn keiner)
     */
    void computeBotMove(int &outFrom, int &outTo, int &outCap);

    /**
     * @brief Rekursive Minimax-Suche mit Alpha-Beta-Pruning.
     * @return Bewertung der Stellung aus Sicht des Bots (BLACK)
     */
    int  minimax(Position &p, int depth, int alpha, int beta, Cell maximizer) const;

    /**
     * @brief Statische Stellungsbewertung aus Sicht des Bots.
     */
    int  evaluate(const Position &p) const;

    /**
     * @brief Liefert die Suchtiefe abhängig von Schwierigkeit und Phase.
     */
    int  searchDepth(const Position &p) const;

    // ========================================================================
    // ZOBRIST-HASH UND WIEDERHOLUNGS-ERKENNUNG
    // ========================================================================

    /**
     * @brief Berechnet einen 64-Bit-Hash der aktuellen Stellung.
     */
    uint64_t hashPosition(const Position &p) const;

    /**
     * @brief Trägt die aktuelle Stellung in den Wiederholungs-Verlauf ein.
     */
    void recordHistory();

    /**
     * @brief Setzt den Wiederholungs-Verlauf zurück (nach Mühle/Schlag).
     */
    void clearHistory();

    /**
     * @brief Prüft, ob der zuletzt eingetragene Hash bereits dreimal
     *        im Verlauf vorhanden ist.
     */
    bool isThreefoldRepetition() const;

    // ========================================================================
    // PERSISTENZ
    // ========================================================================

    /**
     * @brief Liest die Statistik aus dem Preferences-Namespace "wsmill".
     */
    void loadStats();

    /**
     * @brief Schreibt die Statistik in den Preferences-Namespace "wsmill".
     */
    void saveStats();

    // ========================================================================
    // RENDERING
    // ========================================================================

    /**
     * @brief Zeichnet das komplette Spielbrett inklusive aller Steine
     *        und Selektions-/Schlag-Markierungen.
     */
    void drawBoard(Gfx_Sprite *sprite, int boardX, int boardY) const;

    /**
     * @brief Zeichnet die Sidebar-Karten an der angegebenen Position.
     */
    void drawSidebar(Gfx_Sprite *sprite, int sideX, int sideY) const;

    /**
     * @brief Zeichnet ein zentriertes Overlay für Sieg/Niederlage/Remis.
     */
    void drawOverlay(Gfx_Sprite *sprite, int boardX, int boardY) const;

    /**
     * @brief Liefert die Pixel-Koordinaten eines Brettpunkts (sprite-rel.).
     */
    void pointPixel(int idx, int boardX, int boardY, int &outX, int &outY) const;

    /**
     * @brief Sucht den nächstgelegenen Brettpunkt zu einem Touch-Punkt.
     * @return Punkt-Index oder -1, wenn ausserhalb der Toleranz
     */
    int  hitTestPoint(int cx, int cy, int boardX, int boardY) const;

    // ========================================================================
    // FOOTER-HELFER
    // ========================================================================

    /**
     * @brief Initialisiert die fünf Footer-Buttons mit Position und Stil.
     */
    void initFooterButtons(LGFX *tft);

    /**
     * @brief Aktualisiert die Footer-Button-Farben nach einem Stufen-Wechsel.
     */
    void updateDifficultyButtons();

    /**
     * @brief Öffnet die Spielanleitung in einer modalen MessageBox.
     */
    void showHelpDialog();

    /**
     * @brief Öffnet den Bestätigungsdialog zum Zurücksetzen der Statistik.
     */
    void showResetDialog();

    /**
     * @brief Wertet das Ergebnis der MessageBox entsprechend ihrem Zweck aus.
     * @param confirmed true, wenn der Spieler den primären Button gewählt hat
     */
    void onMsgResult(bool confirmed);
};

#endif // WORKSHOPTOOL_MILL_H
