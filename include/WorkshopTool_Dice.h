#ifndef WORKSHOPTOOL_DICE_H
#define WORKSHOPTOOL_DICE_H

// ============================================================================
// WorkshopTool_Dice.h - Würfel-Werkzeug für externe Spiele
// ============================================================================

#include <stdint.h>
#include <memory>

#include "Button.h"
#include "MessageBox.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_Dice
 * @brief Digitaler Würfel als Zufallszahlen-Quelle für Brett- und Gesellschaftsspiele.
 *
 * Das Werkzeug liefert gleichverteilte Zufallswerte von 1 bis 6 pro Würfel. Als Quelle dient der Hardware-Zufallsgenerator des ESP32 (esp_random); Rejection-Sampling entfernt den Modulo-Bias. Das Werkzeug enthält keine Spielregeln, Bewertung oder persistente Statistik.
 *
 * Modi: Zwei Buttons in der Fußzeile schalten zwischen einem und zwei Würfeln um. Der Modus wird nicht persistiert; jede neue Werkzeugsitzung startet mit einem Würfel.
 *
 * Bedienung: Ein Tap irgendwo im Würfelbereich löst einen Wurf aus. Die Animation dauert rund 1,2 Sekunden und zeigt in drei Zwischenschritten wechselnde Fake-Augenzahlen, bevor sich der endgültige Wert einstellt.
 *
 * Redraw-Takt: Das Tool ist nicht in der Cursor-Blink-Ausnahmeliste von Workshop::draw() eingetragen. Dadurch erhält es alle 400 ms automatisch einen Content-Redraw, den die Animation als festen Taktgeber für ihre drei Zwischenschritte nutzt. Ein zusätzlicher Sofort-Redraw wird beim Touch-Release über den Rückgabewert von handleTouch() angestoßen.
 *
 * Layout (547 x 327 px Netto-Inhaltsbereich):
 *   Linker Würfelbereich (333 x 307 px)
 *     1-Würfel-Modus: ein Würfel 200 x 200 px zentriert
 *     2-Würfel-Modus: zwei Würfel 148 x 148 px, 30 px Abstand
 *   Spalten-Lücke              14 px
 *   Sidebar (200 x 307 px)
 *     - Ergebnis-Karte (große Zahl bzw. Summe)
 *     - Historie-Karte (letzte zehn Würfe)
 *
 * Footer-Steuerung: Drei Buttons: "1 Würfel" und "2 Würfel" als Toggle-Paar für den Modus, "?" zum Öffnen der Kurzanleitung.
 *
 * Zustand: Alle Werte leben ausschließlich in RAM. Es gibt keinen Preferences-Zugriff, die Historie geht beim Tool-Wechsel verloren.
 */
class WorkshopTool_Dice : public WorkshopTool
{
public:
    WorkshopTool_Dice()                                     = default;
    ~WorkshopTool_Dice() override                           = default;

    WorkshopTool_Dice(const WorkshopTool_Dice &)            = delete;
    WorkshopTool_Dice &operator=(const WorkshopTool_Dice &) = delete;

    /**
     * @brief Initialisiert Zustand, Footer-Buttons und MessageBox.
     * @param tft Display-Objekt für die Button-Klasse
     */
    void onEnter(LGFX *tft) override;

    /**
     * @brief Gibt die MessageBox frei und verwirft den Touch-Zustand.
     */
    void onExit() override;

    /**
     * @brief Zeichnet Würfel und Sidebar in den Work-Sprite.
     * @param sprite Ziel-Sprite (m_workSprite des Workshop-Moduls)
     * @param ox     X-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param oy     Y-Ursprung des Werkzeugbereichs (sprite-relativ)
     * @param w      Verfügbare Breite in Pixeln
     * @param h      Verfügbare Höhe in Pixeln
     *
     * Während eines aktiven Wurfs wechseln die angezeigten Würfelaugen in jedem Frame. Bei Animations-Ende wird das fertige Wurfergebnis in die Historie geschoben.
     */
    void draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf dem Würfelbereich.
     * @return true, wenn der Inhaltsbereich neu gezeichnet werden muss
     *
     * Nutzt Touch-up-Semantik: die Wurfaktion wird beim Loslassen ausgelöst, sofern der letzte gesehene Touch auf dem Würfelbereich lag und gerade keine Animation läuft.
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Zeichnet die Footer-Buttons (Modus-Toggle und Hilfe).
     */
    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
     * @return true, wenn ein Komplett-Redraw nötig ist (Modus-Wechsel)
     */
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Liefert einen modusabhängigen Untertitel.
     */
    const char *getSubtitle() const override;

private:
    // ========================================================================
    // HISTORIE
    // ========================================================================

    static constexpr int HISTORY_CAPACITY = 10; ///< Maximale Anzahl Einträge in der Historie

    /**
     * @brief Ein einzelner Eintrag in der Wurf-Historie.
     *
     * Wird im Modus des Wurfs gespeichert, spätere Modus-Wechsel verändern die Historie nicht. Bei count=1 ist nur values[0] gültig, bei count=2 beide Werte.
     */
    struct RollEntry
    {
        uint8_t count;     ///< Anzahl geworfener Würfel (1 oder 2)
        uint8_t values[2]; ///< Augenzahlen (values[1] nur bei count=2 gültig)
    };

    // ========================================================================
    // ZUSTAND
    // ========================================================================

    bool m_twoDice = false;              ///< Aktiver Modus: false = ein Würfel, true = zwei Würfel
    uint8_t m_valFinal[2] = {1, 1};      ///< Wurfergebnis (values[1] nur bei m_twoDice relevant)
    bool m_hasResult = false;            ///< Mindestens ein Wurf wurde bereits ausgeführt

    // Animations-Zustand
    bool m_animating = false;            ///< Läuft gerade eine Wurf-Animation?
    uint32_t m_animStartMs = 0;          ///< Startzeitpunkt (millis) der Animation
    uint8_t m_animFakeVal[2] = {1, 1};   ///< Gerade angezeigte Zwischen-Augenzahlen

    // Historie (Slot 0 = neuester Wurf)
    RollEntry m_history[HISTORY_CAPACITY] = {}; ///< Ring-ähnlicher Puffer (push-front)
    int m_historyCount = 0;                     ///< Aktuell belegte Slots, 0..HISTORY_CAPACITY

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
    // FOOTER-BUTTONS
    // ========================================================================

    Button m_btnOne;                   ///< Modus-Toggle "1 Würfel"
    Button m_btnTwo;                   ///< Modus-Toggle "2 Würfel"
    Button m_btnHelp;                  ///< Öffnet die Kurzanleitung
    bool m_buttonsInitialized = false; ///< Buttons wurden in onEnter() initialisiert

    // ========================================================================
    // MESSAGEBOX
    // ========================================================================

    std::unique_ptr<MessageBox> m_msg; ///< Modale Dialogbox für die Kurzanleitung

    // ========================================================================
    // I18N-PUFFER
    //
    // Button::initButton und MessageBox::setTitle / setMessage / setButtonLabel speichern nur den übergebenen Zeiger, daher muss der Puffer die Lebensdauer des Werkzeugs überdauern. getSubtitle() befüllt m_subtitleBuf bei jedem Aufruf neu.
    // ========================================================================

    char m_helpTitleBuf[40] = {};   ///< Puffer für den Hilfetitel
    char m_helpTextBuf[512] = {};   ///< Puffer für den Hilfetext
    char m_dialogCloseBuf[16] = {}; ///< Puffer für die Schaltfläche zum Schließen
    char m_btnOneLabel[16] = {};    ///< Puffer für "1 Würfel"
    char m_btnTwoLabel[16] = {};    ///< Puffer für "2 Würfel"
    mutable char m_subtitleBuf[40] = {}; ///< Puffer für den dynamischen Untertitel

    // ========================================================================
    // WURF-LOGIK
    // ========================================================================

    /**
     * @brief Liefert eine gleichverteilte Augenzahl 1 bis 6 aus dem Hardware-RNG.
     *
     * Verwendet esp_random() als Quelle und Rejection-Sampling zur Eliminierung des Modulo-Bias, den eine direkte Restwertbildung (esp_random() % 6) über den 32-Bit-Wertebereich hinterlassen würde.
     */
    uint8_t rollDie() const;

    /**
     * @brief Startet die Wurf-Animation und berechnet die finalen Augenzahlen.
     *
     * Die endgültigen Werte werden sofort festgelegt, die Animation dient rein der Visualisierung.
     */
    void startRoll();

    /**
     * @brief Schiebt einen neuen Wurf als neuesten Eintrag in die Historie.
     */
    void pushHistory(uint8_t count, uint8_t v0, uint8_t v1);

    // ========================================================================
    // ZEICHNEN
    // ========================================================================

    /**
     * @brief Zeichnet den Würfelbereich inklusive Animation.
     */
    void drawDiceArea(Gfx_Sprite *sprite);

    /**
     * @brief Zeichnet einen einzelnen Würfel mit Schatten, Hülle und Augen.
     * @param sprite    Ziel-Sprite
     * @param cx        X-Koordinate des Würfel-Zentrums (sprite-relativ)
     * @param cy        Y-Koordinate des Würfel-Zentrums (sprite-relativ)
     * @param size      Kantenlänge in Pixeln
     * @param value     Augenzahl 1 bis 6
     * @param redAccent true für den zweiten Würfel im Doppel-Modus (roter Korpus)
     */
    void drawSingleDie(Gfx_Sprite *sprite, int cx, int cy, int size, uint8_t value, bool redAccent);

    /**
     * @brief Zeichnet die Ergebnis- und die Historie-Karte in die rechte Sidebar.
     */
    void drawSidebar(Gfx_Sprite *sprite);

    /**
     * @brief Zeichnet die Augen eines Würfels der Größe size an die fünf klassischen Positionen.
     * @param sprite Ziel-Sprite
     * @param cx     X-Mitte des Würfels
     * @param cy     Y-Mitte des Würfels
     * @param size   Kantenlänge des Würfels
     * @param value  Augenzahl 1 bis 6
     * @param pipCol Farbe der Augen
     */
    void drawPips(Gfx_Sprite *sprite, int cx, int cy, int size, uint8_t value, uint16_t pipCol);

    // ========================================================================
    // FOOTER-HELFER
    // ========================================================================

    /**
     * @brief Aktualisiert die Toggle-Farben der beiden Modus-Buttons.
     */
    void updateModeButtons();

    // ========================================================================
    // DIALOGE
    // ========================================================================

    /**
     * @brief Öffnet die Kurzanleitung in einer modalen MessageBox.
     */
    void showHelpDialog();

    /**
     * @brief Callback der MessageBox.
     */
    void onMsgResult(bool confirmed);

    // ========================================================================
    // GEOMETRIE
    // ========================================================================

    /**
     * @brief Prüft, ob die absoluten Touch-Koordinaten im Würfelbereich liegen.
     */
    bool hitTestDiceArea(int32_t tx, int32_t ty) const;
};

#endif // WORKSHOPTOOL_DICE_H
