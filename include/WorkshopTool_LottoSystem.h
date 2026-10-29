#ifndef WORKSHOPTOOL_LOTTOSYSTEM_H
#define WORKSHOPTOOL_LOTTOSYSTEM_H

#include <stdint.h>
#include <memory>

#include "Button.h"
#include "MessageBox.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_LottoSystem
 * @brief LOTTO 6 aus 49 als Vollsystem 007 bis 011.
 *
 * Das Werkzeug verwaltet einen lokalen Systemschein: Der Nutzer wählt ein Vollsystem, trägt die passende Anzahl Tippzahlen ein oder füllt sie per Zufall. Nach der Ziehung werden sechs Gewinnzahlen und die Superzahl-Trefferangabe eingegeben. Die Auswertung zählt alle Gewinne je Gewinnklasse, zeigt aber keine Gewinnbeträge.
 *
 * Persistenz: Der aktuelle Schein wird im AppState gespeichert und beim Öffnen wiederhergestellt.
 */
class WorkshopTool_LottoSystem : public WorkshopTool
{
public:
    WorkshopTool_LottoSystem() = default;
    ~WorkshopTool_LottoSystem() override = default;

    WorkshopTool_LottoSystem(const WorkshopTool_LottoSystem &) = delete;
    WorkshopTool_LottoSystem &operator=(const WorkshopTool_LottoSystem &) = delete;

    /**
     * @brief Initialisiert Zustand, Footer-Buttons und Hilfe-Dialog.
     * @param tft Display-Objekt für die Button-Klasse
     */
    void onEnter(LGFX *tft) override;

    /**
     * @brief Räumt Dialog und Touch-Zustand auf.
     */
    void onExit() override;

    /**
     * @brief Zeichnet Zahlenfeld, Tippübersicht und Auswertung.
     */
    void draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf Zahlenfeld und Superzahl-Eingabe.
     * @return true, wenn der Inhaltsbereich neu gezeichnet werden muss
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
     * @brief Liefert einen zustandsabhängigen Untertitel.
     */
    const char *getSubtitle() const override;

private:
    enum class InputMode : uint8_t
    {
        Ticket = 0, ///< Eingabe der Systemzahlen
        Draw = 1    ///< Eingabe der Gewinnzahlen
    };

    struct SystemDef
    {
        uint8_t numberCount; ///< Anzahl anzukreuzender Systemzahlen
        uint16_t fieldCount; ///< Anzahl der entstehenden Normalfelder
        uint32_t priceCents; ///< Spieleinsatz in Cent ohne Bearbeitungsgebühr
    };

    uint8_t m_systemIndex = 0;          ///< Index in der Systemtabelle
    InputMode m_mode = InputMode::Ticket; ///< Aktueller Eingabemodus
    uint64_t m_ticketMask = 0;          ///< Tippzahlen, Bit n steht für Zahl n
    uint64_t m_drawMask = 0;            ///< Gewinnzahlen, Bit n steht für Zahl n
    uint8_t m_ticketSuperNumber = 10;   ///< Tipp-Superzahl 0..9, 10=offen
    uint8_t m_drawSuperNumber = 10;     ///< Ziehungs-Superzahl 0..9, 10=offen
    bool m_superHit = false;            ///< true, wenn die Superzahl getroffen wurde
    uint16_t m_winCounts[9] = {};       ///< Gewinne je Gewinnklasse 1 bis 9

    bool m_touchActive = false; ///< Touch-Sequenz läuft
    int32_t m_touchLastX = 0;   ///< Letzte Touch-X während touched=true
    int32_t m_touchLastY = 0;   ///< Letzte Touch-Y während touched=true
    bool m_superDialogVisible = false; ///< true, wenn die Superzahl-Auswahl geöffnet ist

    int m_originX = 0; ///< X-Origin des Tools im Work-Sprite
    int m_originY = 0; ///< Y-Origin des Tools im Work-Sprite

    Button m_btnSystem[5]; ///< Systemwahl 007 bis 011
    Button m_btnRandom;    ///< Füllt den aktuellen Eingabemodus mit Zufallszahlen
    Button m_btnClear;     ///< Löscht den aktuellen Eingabemodus
    Button m_btnMode;      ///< Wechselt zwischen Tipp und Ziehung
    Button m_btnHelp;      ///< Öffnet die Kurzanleitung
    bool m_buttonsInitialized = false; ///< Footer-Buttons wurden initialisiert

    std::unique_ptr<MessageBox> m_msg; ///< Modale Dialogbox für die Kurzanleitung

    char m_helpTitleBuf[48] = {};       ///< Buffer für Hilfe-Titel
    char m_helpTextBuf[760] = {};       ///< Buffer für Hilfe-Text
    char m_dialogCloseBuf[16] = {};     ///< Buffer für Schließen-Knopf
    char m_btnModeTicketLabel[12] = {}; ///< Buffer für Tippmodus-Button
    char m_btnModeDrawLabel[12] = {};   ///< Buffer für Ziehungsmodus-Button
    mutable char m_subtitleBuf[80] = {}; ///< Buffer für dynamischen Untertitel

    static const SystemDef s_systems[5]; ///< Vollsysteme 007 bis 011

    void resetAll();                    ///< Löscht Tipp, Ziehung und Auswertung
    void loadState();                   ///< Lädt den gespeicherten Systemschein
    void saveState() const;             ///< Speichert den aktuellen Systemschein
    void setSystemIndex(uint8_t index); ///< Wechselt das Vollsystem
    void toggleNumber(uint8_t n);       ///< Wählt eine Zahl im aktuellen Modus an oder ab
    void fillRandom();                  ///< Füllt den aktuellen Modus per Zufall
    void clearCurrentMode();            ///< Löscht Tipp oder Ziehung
    void evaluate();                    ///< Berechnet Gewinnklassen aus Tipp und Ziehung
    void clearEvaluation();             ///< Setzt die Gewinnzähler zurück

    uint8_t activeSystemSize() const;       ///< Liefert 7 bis 11
    uint16_t activeFieldCount() const;      ///< Liefert Anzahl Normalfelder
    uint32_t activePriceCents() const;      ///< Liefert Spieleinsatz in Cent
    uint8_t ticketCount() const;            ///< Anzahl gesetzter Tippzahlen
    uint8_t drawCount() const;              ///< Anzahl gesetzter Gewinnzahlen
    bool isTicketComplete() const;          ///< true, wenn genug Tippzahlen gesetzt sind
    bool isDrawComplete() const;            ///< true, wenn sechs Gewinnzahlen gesetzt sind
    bool isSuperComplete() const;           ///< true, wenn beide Superzahlen gesetzt sind
    bool hasNumber(uint64_t mask, uint8_t n) const; ///< Prüft ein Zahlen-Bit
    uint8_t countMask(uint64_t mask) const; ///< Zählt gesetzte Bits
    uint8_t randomNumber(uint8_t maxInclusive) const; ///< Gleichverteilte Zufallszahl 1..maxInclusive
    uint64_t validNumberMask(uint64_t mask) const; ///< Entfernt ungültige Zahlenbits
    uint8_t validSuperNumber(uint8_t n) const; ///< Prüft eine Superzahl
    void trimMaskToLimit(uint64_t &mask, uint8_t maxCount) const; ///< Kürzt eine Zahlenmaske auf ein Maximum
    void updateSuperHit();              ///< Aktualisiert den Superzahl-Treffer

    void drawNumberGrid(Gfx_Sprite *sprite);      ///< Zeichnet die 49 Lottozahlen
    void drawSelectionSummary(Gfx_Sprite *sprite);///< Zeichnet die gewählten Zahlen
    void drawSidebar(Gfx_Sprite *sprite);         ///< Zeichnet Systemdaten und Gewinnklassen
    void drawSuperSummary(Gfx_Sprite *sprite, int sideX, int drawY) const; ///< Zeichnet die kompakte Superzahl-Anzeige
    void drawSuperDialog(Gfx_Sprite *sprite) const; ///< Zeichnet die Superzahl-Auswahl
    void drawSuperDialogRow(Gfx_Sprite *sprite, int rowY, WorkshopKey labelKey, uint8_t value, uint16_t activeFill) const; ///< Zeichnet eine Superzahl-Zeile
    void drawNumberCell(Gfx_Sprite *sprite, uint8_t n, int x, int y, bool ticket, bool drawn) const; ///< Zeichnet eine Zahl
    void drawNumberList(Gfx_Sprite *sprite, uint64_t mask, int x, int y, int w, WorkshopKey emptyKey, uint16_t color) const; ///< Zeichnet Zahlenliste

    void initFooterButtons(LGFX *tft);    ///< Initialisiert Footer-Buttons
    void updateFooterButtonColors();      ///< Aktualisiert Buttonfarben
    void showHelpDialog();                ///< Öffnet die Anleitung
    void onMsgResult(bool confirmed);     ///< Callback der MessageBox

    bool hitTestNumber(int32_t tx, int32_t ty, uint8_t &outNumber) const; ///< Ermittelt eine getippte Zahl
    bool hitTestSuperSummary(int32_t tx, int32_t ty) const; ///< Prüft die kompakte Superzahl-Anzeige
    bool hitTestSuperDialog(int32_t tx, int32_t ty, uint8_t &outNumber, bool &outDraw, bool &outClose) const; ///< Ermittelt eine Superzahl im Dialog
};

#endif // WORKSHOPTOOL_LOTTOSYSTEM_H
