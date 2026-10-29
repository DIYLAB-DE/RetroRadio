#ifndef WORKSHOPTOOL_SHUTTHEBOX_H
#define WORKSHOPTOOL_SHUTTHEBOX_H

// ============================================================================
// WorkshopTool_ShutTheBox.h - Würfelspiel "Shut the Box" für die Werkstattpause
// ============================================================================

#include <stdint.h>
#include <memory>

#include "Button.h"
#include "MessageBox.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_ShutTheBox
 * @brief Klassisches Würfelspiel mit neun Klappen.
 *
 * Ziel ist es, die Klappen 1 bis 9 zu schließen. Nach jedem Wurf wählt der Spieler offene Zahlen, deren Summe dem Wurf entspricht, und bestätigt mit "legen". Solange 7, 8 oder 9 noch offen sind, wird mit zwei Würfeln gespielt. Sobald 7, 8 und 9 geschlossen sind, kann zwischen einem und zwei Würfeln gewählt werden.
 *
 * Spielende: Sind alle Klappen geschlossen, ist die Runde gewonnen. Gibt es nach einem Wurf keine mögliche offene Kombination für die gewürfelte Summe, endet die Runde mit dem Restwert der offenen Zahlen.
 *
 * Persistenz: Gespeichert werden der beste Restwert und die Anzahl gewonnener Runden im eigenen Preferences-Namespace "wsstb". Die laufende Partie und der Würfelmodus werden nicht gespeichert.
 */
class WorkshopTool_ShutTheBox : public WorkshopTool
{
public:
    WorkshopTool_ShutTheBox() = default;
    ~WorkshopTool_ShutTheBox() override = default;

    WorkshopTool_ShutTheBox(const WorkshopTool_ShutTheBox &) = delete;
    WorkshopTool_ShutTheBox &operator=(const WorkshopTool_ShutTheBox &) = delete;

    /**
     * @brief Initialisiert Spielzustand, Statistik, Dialog und Fußzeilen-Buttons.
     * @param tft Display-Objekt für die Button-Klasse
     */
    void onEnter(LGFX *tft) override;

    /**
     * @brief Räumt den Werkzeugzustand auf.
     */
    void onExit() override;

    /**
     * @brief Zeichnet Klappenfeld und Statuskarten in den Work-Sprite.
     */
    void draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf Klappen und Wurfkarte.
     * @return true, wenn der Inhaltsbereich neu gezeichnet werden muss
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Zeichnet die Fußzeilen-Buttons.
     */
    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf den Fußzeilen-Buttons.
     * @return true, wenn ein Komplett-Redraw nötig ist
     */
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Liefert einen statusabhängigen Untertitel.
     */
    const char *getSubtitle() const override;

private:
    /**
     * @brief Globaler Spielzustand für Darstellung und Eingabelogik.
     */
    enum class GameState : uint8_t
    {
        PLAYING = 0, ///< Normale Runde läuft
        WON = 1,     ///< Alle Klappen sind geschlossen
        GAMEOVER = 2 ///< Keine gültige Kombination für den aktuellen Wurf
    };

    /**
     * @brief Zweck der aktuell sichtbaren MessageBox.
     */
    enum class MsgPurpose : uint8_t
    {
        NONE = 0,       ///< Keine Box aktiv
        HELP = 1,       ///< Spielanleitung
        RESET_STATS = 2 ///< Statistik zurücksetzen
    };

    uint16_t m_closedMask = 0;       ///< Geschlossene Klappen, Bit 0 = Zahl 1
    uint16_t m_selectedMask = 0;     ///< Aktuell gewählte offene Klappen
    GameState m_state = GameState::PLAYING; ///< Aktueller Spielzustand
    bool m_hasRoll = false;          ///< true wenn ein Wurf gelegt werden muss
    bool m_oneDieMode = false;       ///< Gewählter Würfelmodus, wenn ein Würfel erlaubt ist
    bool m_resultBooked = false;     ///< Endergebnis bereits gespeichert
    uint8_t m_dice[2] = {1, 1};      ///< Aktuelle Würfelwerte
    uint8_t m_diceCount = 2;         ///< Würfelanzahl des aktuellen Wurfs
    uint16_t m_bestRest = 255;       ///< Bester Restwert, 255 = noch kein Ergebnis
    uint16_t m_wins = 0;             ///< Anzahl gewonnener Runden

    bool m_touchActive = false; ///< Touch-Sequenz läuft
    int32_t m_touchLastX = 0;   ///< Letzte Touch-X während touched=true
    int32_t m_touchLastY = 0;   ///< Letzte Touch-Y während touched=true

    int m_originX = 0; ///< X-Origin des Tools im Work-Sprite
    int m_originY = 0; ///< Y-Origin des Tools im Work-Sprite

    Button m_btnOne;   ///< Umschalter auf einen Würfel
    Button m_btnTwo;   ///< Umschalter auf zwei Würfel
    Button m_btnRoll;  ///< Würfeln
    Button m_btnLay;   ///< Auswahl legen
    Button m_btnNew;   ///< Neue Runde
    Button m_btnReset; ///< Statistik zurücksetzen
    Button m_btnHelp;  ///< Hilfe anzeigen
    bool m_buttonsInitialized = false; ///< Fußzeilen-Buttons wurden initialisiert

    std::unique_ptr<MessageBox> m_msg; ///< Modale Dialogbox für Hilfe und Reset
    MsgPurpose m_msgPurpose = MsgPurpose::NONE; ///< Aktueller Dialog-Zweck

    char m_helpTitleBuf[40] = {};   ///< Buffer für Hilfe-Titel
    char m_helpTextBuf[640] = {};   ///< Buffer für Hilfe-Text
    char m_resetTitleBuf[40] = {};  ///< Buffer für Reset-Titel
    char m_resetTextBuf[256] = {};  ///< Buffer für Reset-Text
    char m_dialogCloseBuf[16] = {}; ///< Buffer für Schließen-Knopf
    char m_dialogOkBuf[8] = {};     ///< Buffer für OK-Knopf
    char m_dialogCancelBuf[16] = {};///< Buffer für Abbrechen-Knopf
    char m_btnOneLabel[8] = {};     ///< Buffer für "1W"
    char m_btnTwoLabel[8] = {};     ///< Buffer für "2W"
    char m_btnRollLabel[12] = {};   ///< Buffer für "wurf"
    char m_btnLayLabel[12] = {};    ///< Buffer für "legen"
    char m_btnNewLabel[12] = {};    ///< Buffer für "neu"
    char m_btnResetLabel[12] = {};  ///< Buffer für "reset"
    mutable char m_subtitleBuf[64] = {}; ///< Buffer für den dynamischen Untertitel

    void resetGame();              ///< Startet eine neue Runde
    void rollDice();               ///< Führt einen Wurf aus
    bool laySelection();           ///< Schließt die aktuell gewählten Klappen
    void toggleNumber(uint8_t n);  ///< Wählt eine offene Klappe an oder ab
    bool canUseOneDie() const;     ///< Prüft, ob ein Würfel erlaubt ist
    bool canLaySelection() const;  ///< Prüft, ob die Auswahl zum Wurf passt
    bool hasCombination(uint8_t target) const; ///< Prüft, ob eine offene Kombination möglich ist
    bool allClosed() const;        ///< Prüft, ob alle Klappen geschlossen sind
    uint8_t selectedSum() const;   ///< Summe der gewählten Klappen
    uint8_t openSum() const;       ///< Summe der offenen Klappen
    uint8_t currentRollSum() const;///< Summe des aktuellen Wurfs
    uint8_t rollDie() const;       ///< Liefert eine Augenzahl 1 bis 6
    void bookResultIfNeeded();     ///< Speichert das Endergebnis genau einmal

    void drawBoard(Gfx_Sprite *sprite);              ///< Zeichnet die Klappen
    void drawSidebar(Gfx_Sprite *sprite);            ///< Zeichnet Statuskarten
    void drawDie(Gfx_Sprite *sprite, int cx, int cy, int size, uint8_t value, uint16_t fill, uint16_t pip) const; ///< Zeichnet einen Würfel
    void drawPips(Gfx_Sprite *sprite, int cx, int cy, int size, uint8_t value, uint16_t color) const; ///< Zeichnet Würfelaugen
    void drawOverlay(Gfx_Sprite *sprite);            ///< Zeichnet Sieg- oder Ende-Overlay

    void initFooterButtons(LGFX *tft);  ///< Initialisiert Fußzeilen-Buttons
    void updateFooterButtonColors();    ///< Aktualisiert Buttonfarben nach Zustand
    void showHelpDialog();              ///< Öffnet die Anleitung
    void showResetDialog();             ///< Öffnet den Reset-Dialog
    void onMsgResult(bool confirmed);   ///< Callback der MessageBox

    bool hitTestNumber(int32_t tx, int32_t ty, uint8_t &outNumber) const; ///< Ermittelt eine getippte Klappe
    bool hitTestRollCard(int32_t tx, int32_t ty) const; ///< Prüft Touch auf der Wurfkarte
    void loadStats();                   ///< Lädt die gespeicherte Statistik
    void saveStats();                   ///< Speichert die Statistik
};

#endif // WORKSHOPTOOL_SHUTTHEBOX_H
