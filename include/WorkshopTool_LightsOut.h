#ifndef WORKSHOPTOOL_LIGHTSOUT_H
#define WORKSHOPTOOL_LIGHTSOUT_H

// ============================================================================
// WorkshopTool_LightsOut.h - Knobelspiel "Lights Out" für die Werkstattpause
// ============================================================================

#include <stdint.h>
#include <memory>

#include "Button.h"
#include "MessageBox.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_LightsOut
 * @brief Klassisches Knobelspiel Lights Out auf einem 5x5-Brett.
 *
 * Spielprinzip: Jede Zelle ist entweder an oder aus. Ein Tap auf eine Zelle invertiert den eigenen Zustand sowie die Zustände der vier orthogonal angrenzenden Nachbarzellen. Ziel ist es, alle 25 Zellen auszuschalten.
 *
 * Schwierigkeitsgrade: Eine neue Partie startet mit allen Zellen aus und führt anschließend eine zufällige Sequenz gültiger Tap-Operationen aus. Die Länge dieser Verwürfelungssequenz hängt von der gewählten Stufe ab: drei Tap-Operationen für leicht, sieben für mittel, zwölf für schwer. Da die Verwürfelung aus gültigen Operationen besteht, ist jede so erzeugte Stellung garantiert lösbar.
 *
 * Bewertung: Beim Lösen werden Anzahl der benötigten Züge sowie die verstrichene Spielzeit pro Schwierigkeitsstufe getrennt geführt. Pro Stufe werden die Anzahl gelöster Partien sowie die schnellste Lösungszeit (Bestzeit in Sekunden) im Preferences-Namespace "wslout" persistiert.
 *
 * Layout (547 x 327 px Netto-Inhaltsbereich):
 *   TOP_GAP                  10 px
 *   Spielbrett 306 x 306 px (links, vertikal mittig)
 *     5 x 5 Zellen, Zellgröße 58 x 58 px, Zell-Abstand 4 px
 *   Spalten-Lücke           14 px
 *   Sidebar 227 x 306 px (rechts)
 *     - Züge-Karte
 *     - Zeit-Karte (mm:ss)
 *     - Status-Karte (Spielen / Gelöst!)
 *     - Statistik-Karte (Solved/Best je Stufe)
 *
 * Footer-Steuerung: Sechs Buttons: drei Toggle-Buttons "leicht" / "mittel" / "schwer" für die Schwierigkeit, "neu" für eine neue Partie auf der aktuellen Stufe, "reset" zum Nullen der Statistik (Bestätigung über modale MessageBox) und "?" zum Öffnen der Spielanleitung. Im Gelöst-Zustand bleibt die Footer-Belegung unverändert, das Brett akzeptiert keine weiteren Taps.
 *
 * Persistenz: Sechs Zähler für gelöste Spiele und Bestzeiten je Stufe werden im Preferences-Namespace "wslout" abgelegt. Die gewählte Schwierigkeit wird nicht persistiert; jede neue Werkzeugsitzung startet auf "leicht".
 */
class WorkshopTool_LightsOut : public WorkshopTool
{
public:
    WorkshopTool_LightsOut();
    ~WorkshopTool_LightsOut() override = default;

    WorkshopTool_LightsOut(const WorkshopTool_LightsOut &) = delete;
    WorkshopTool_LightsOut &operator=(const WorkshopTool_LightsOut &) = delete;

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
     * Nutzt Touch-up-Semantik: die Aktion wird beim Loslassen (touched=false) anhand der zuletzt gesehenen Position aus touched=true-Frames ausgelöst.
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Zeichnet die Footer-Buttons.
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

private:
    // ========================================================================
    // PRIVATE INNERE TYPEN
    // ========================================================================

    /**
     * @brief Schwierigkeitsstufe, bestimmt Verwürfelungslänge und Statistik-Slot.
     */
    enum class Difficulty : uint8_t
    {
        EASY = 0,   ///< 3 Verwürfelungs-Tap-Operationen
        MEDIUM = 1, ///< 7 Verwürfelungs-Tap-Operationen
        HARD = 2    ///< 12 Verwürfelungs-Tap-Operationen
    };

    /**
     * @brief Globaler Spielzustand für die UI-Logik.
     */
    enum class GameState : uint8_t
    {
        PLAYING = 0, ///< Normales Spiel läuft (Timer ggf. aktiv)
        SOLVED = 1   ///< Alle Zellen aus, Partie ist gelöst
    };

    /**
     * @brief Zweck der modalen MessageBox.
     */
    enum class MsgPurpose : uint8_t
    {
        NONE = 0,
        RESET_STATS = 1,
        HELP = 2
    };

    /**
     * @brief Zifferbreiten-Cache für tabularisches Zeichnen.
     *
     * Bei proportionalen Fonts haben Ziffern unterschiedliche Breiten, sodass sich rechtsbündige Zahlenanzeigen bei jeder Zifferänderung seitlich verschieben. Der Cache hält die maximale Breite einer Ziffer (0-9) sowie Doppelpunkt- und Bindestrich-Breite, damit jedes Zeichen in einem gleich breiten Slot gezeichnet werden kann. Wird einmalig je Font beim ersten Sidebar-Redraw vermessen und bleibt für die restliche Tool-Laufzeit gültig.
     */
    struct TabMetrics
    {
        int digitW = 0;     ///< Maximale Breite einer Ziffer 0-9
        int colonW = 0;     ///< Breite des Doppelpunkts ':'
        int dashW = 0;      ///< Breite des Bindestrichs '-' (für "-:--")
        bool valid = false; ///< true, sobald initial vermessen
    };

    // ========================================================================
    // SPIELZUSTAND
    // ========================================================================

    bool m_cells[25] = {};                      ///< Zell-Belegung (true = an)
    GameState m_state = GameState::PLAYING;     ///< Aktueller Spielzustand
    Difficulty m_difficulty = Difficulty::EASY; ///< Aktuelle Schwierigkeitsstufe
    uint16_t m_moves = 0;                       ///< Züge der laufenden Partie
    bool m_resultBooked = false;                ///< Endergebnis bereits gewertet?

    // ========================================================================
    // TIMER (laufende Partie)
    // ========================================================================

    uint32_t m_startMs = 0;          ///< Startzeitpunkt (millis) der laufenden Partie, 0 bis zum ersten Tap, eingefroren beim Lösen
    uint32_t m_solvedMs = 0;         ///< Endzeit (millis) der gelösten Partie, sonst 0
    uint16_t m_lastShownSeconds = 0; ///< Letzter angezeigter Sekunden-Wert für 1-Hz-Redraw-Auslösung

    // ========================================================================
    // STATISTIK (persistiert in Preferences "wslout")
    // ========================================================================

    uint16_t m_stSolved[3] = {0, 0, 0}; ///< Anzahl gelöster Partien je Stufe (EASY, MEDIUM, HARD)
    uint16_t m_stBest[3] = {0, 0, 0};   ///< Bestzeit in Sekunden je Stufe (0 = noch keine Lösung)

    // ========================================================================
    // TOUCH-TRACKING
    // ========================================================================

    bool m_touchActive = false; ///< Touch-Sequenz läuft (touched=true gesehen)
    int32_t m_touchLastX = 0;   ///< Letzte gesehene X-Position während touched=true
    int32_t m_touchLastY = 0;   ///< Letzte gesehene Y-Position während touched=true

    // ========================================================================
    // GEOMETRIE-CACHE
    // ========================================================================

    int m_originX = 0; ///< X-Origin des Tools im Work-Sprite (gesetzt in draw())
    int m_originY = 0; ///< Y-Origin des Tools im Work-Sprite (gesetzt in draw())

    // ========================================================================
    // TABULAR-ZIFFERN
    // ========================================================================

    TabMetrics m_tab30; ///< Ziffer-Metriken für ComfortaaBold30 (Züge- und Zeitanzeige)

    // ========================================================================
    // FOOTER-BUTTONS
    // ========================================================================

    Button m_btnEasy;                  ///< Schwierigkeit "leicht" (Toggle)
    Button m_btnMedium;                ///< Schwierigkeit "mittel" (Toggle)
    Button m_btnHard;                  ///< Schwierigkeit "schwer" (Toggle)
    Button m_btnNew;                   ///< Neue Partie starten
    Button m_btnReset;                 ///< Statistik zurücksetzen (mit Bestätigung)
    Button m_btnHelp;                  ///< Spielanleitung anzeigen
    bool m_buttonsInitialized = false; ///< Buttons wurden in onEnter() initialisiert

    std::unique_ptr<MessageBox> m_msg;          ///< Modale Dialogbox für Reset-Bestätigung und Hilfetext
    MsgPurpose m_msgPurpose = MsgPurpose::NONE; ///< Aktueller Verwendungszweck der MessageBox

    // ========================================================================
    // I18N-PUFFER
    //
    // Button::initButton und die MessageBox-Setter speichern nur Zeiger. Die drei Schwierigkeitsbeschriftungen werden zusätzlich in der Statistiktabelle gelesen. getSubtitle() befüllt m_subtitleBuf bei jedem Aufruf neu.
    // ========================================================================

    char m_helpTitleBuf[40] = {};   ///< Puffer für den Hilfetitel
    char m_helpTextBuf[512] = {};   ///< Puffer für den Hilfetext
    char m_resetTitleBuf[40] = {};  ///< Puffer für den Titel des Rücksetzdialogs
    char m_resetTextBuf[256] = {};  ///< Puffer für den Text des Rücksetzdialogs
    char m_dialogCloseBuf[16] = {}; ///< Puffer für die Schaltfläche zum Schließen
    char m_dialogOkBuf[8] = {};     ///< Puffer für die Bestätigungsschaltfläche
    char m_dialogCancelBuf[16] = {};///< Puffer für die Abbruchschaltfläche
    char m_diffEasyLabel[16] = {};  ///< Puffer für "leicht" in Schaltfläche und Statistik
    char m_diffMediumLabel[16] = {};///< Puffer für "mittel" in Schaltfläche und Statistik
    char m_diffHardLabel[16] = {};  ///< Puffer für "schwer" in Schaltfläche und Statistik
    char m_btnNewLabel[16] = {};    ///< Puffer für "neu"
    char m_btnResetLabel[16] = {};  ///< Puffer für "reset"
    mutable char m_subtitleBuf[40] = {}; ///< Puffer für den dynamischen Untertitel

    // ========================================================================
    // SPIEL-LOGIK
    // ========================================================================

    /**
     * @brief Setzt das Brett zurück und verwürfelt für die aktuelle Stufe.
     */
    void resetGame();

    /**
     * @brief Invertiert die Zelle (r,c) und ihre orthogonalen Nachbarn.
     */
    void toggleCellAndNeighbors(int row, int col);

    /**
     * @brief Liefert true, wenn alle 25 Zellen aus sind.
     */
    bool isSolved() const;

    /**
     * @brief Bucht das Lösungsergebnis genau einmal in der Statistik.
     */
    void bookResultIfNeeded();

    /**
     * @brief Liefert die Verwürfelungslänge für die aktuelle Stufe.
     */
    int scrambleLengthForDifficulty(Difficulty d) const;

    // ========================================================================
    // ZEICHNEN
    // ========================================================================

    /**
     * @brief Zeichnet das 5x5-Brett in den Sprite (relative Koordinaten).
     */
    void drawBoard(Gfx_Sprite *sprite);

    /**
     * @brief Zeichnet die rechte Sidebar (Züge / Zeit / Status / Stats).
     */
    void drawSidebar(Gfx_Sprite *sprite);

    /**
     * @brief Füllt @p m einmalig mit den Ziffer-/Trennzeichen-Breiten des aktuell im @p sprite geladenen Fonts.
     *
     * Setzt @p m.valid nach erfolgreicher Messung auf true, weitere Aufrufe sind dann No-Ops. Muss aufgerufen werden, solange der betreffende Font geladen ist.
     */
    void ensureTabMetrics(Gfx_Sprite *sprite, TabMetrics &m);

    /**
     * @brief Zeichnet @p text rechtsbündig endend bei @p rightX mit gleich breiten Ziffern-Slots.
     *
     * Jede Ziffer erhält einen Slot der Breite @p m.digitW und wird darin horizontal zentriert, Doppelpunkt und Bindestrich bekommen ihre eigene Slot-Breite. Dadurch bleibt bei Folge-Aufrufen mit gleicher Zeichenanzahl jede Zifferposition stabil. Erwartet geladenen Font und zuvor gesetzte Textfarbe und überschreibt das Text-Datum auf MC_DATUM.
     *
     * @param sprite Ziel-Sprite
     * @param text   Zu zeichnender Text (unterstützt '0'-'9', ':', '-')
     * @param rightX X-Koordinate der rechten Außenkante
     * @param cy     Y-Koordinate der vertikalen Mitte
     * @param m      Ziffer-Metriken (muss gültig sein)
     */
    void drawTabularRight(Gfx_Sprite *sprite, const char *text, int rightX, int cy, const TabMetrics &m);

    // ========================================================================
    // FOOTER-HELFER
    // ========================================================================

    /**
     * @brief Aktualisiert die Toggle-Farben der drei Schwierigkeits-Buttons.
     */
    void updateDifficultyButtons();

    // ========================================================================
    // DIALOGE
    // ========================================================================

    /**
     * @brief Öffnet den Bestätigungsdialog zum Zurücksetzen der Statistik.
     */
    void showResetDialog();

    /**
     * @brief Öffnet die Spielanleitung in einer modalen MessageBox.
     */
    void showHelpDialog();

    /**
     * @brief Callback der MessageBox.
     */
    void onMsgResult(bool confirmed);

    // ========================================================================
    // PERSISTENZ
    // ========================================================================

    /**
     * @brief Liest die Statistik aus dem Preferences-Namespace "wslout".
     */
    void loadStats();

    /**
     * @brief Schreibt die Statistik in den Preferences-Namespace "wslout".
     */
    void saveStats();

    // ========================================================================
    // GEOMETRIE
    // ========================================================================

    /**
     * @brief Wandelt absolute Touch-Koordinaten in eine Zelle (row,col) um.
     * @return true, wenn der Tap auf einer Zelle des Bretts liegt
     */
    bool hitTestBoard(int32_t tx, int32_t ty, int &outRow, int &outCol) const;
};

#endif // WORKSHOPTOOL_LIGHTSOUT_H
