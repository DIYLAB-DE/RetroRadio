#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <stddef.h>
#include <functional>
#include "Button.h"
#include "Gfx.h"

/**
 * @brief Modale, nicht-blockierende Dialogbox für Bestätigungs-/Hinweisdialoge.
 */
class MessageBox
{
public:
    /**
     * @brief Konstruktor
     * @param gfx Referenz auf das Displayobjekt
     */
    explicit MessageBox(LGFX *gfx);

    /**
     * @brief Destruktor - gibt ggf. den Backup-Puffer frei
     */
    ~MessageBox();

    /**
     * @brief Definiert Position und Größe der MessageBox
     * @param x X-Position oben links
     * @param y Y-Position oben links
     * @param w Breite in Pixeln
     * @param h Höhe in Pixeln
     */
    void setBounds(int x, int y, int w, int h);

    /**
     * @brief Definiert Position und Breite, Höhe wird automatisch berechnet
     * @param x X-Position oben links
     * @param minY Minimale Y-Position (Box wird nicht höher platziert)
     * @param w Breite in Pixeln
     * @param screenHeight Bildschirmhöhe für vertikale Zentrierung
     *
     * Die Höhe wird basierend auf dem Nachrichtentext berechnet. Die Box wird vertikal zentriert, aber nie oberhalb von minY.
     *
     * @warning MUSS nach setMessage() aufgerufen werden! Spätere Änderungen
     *          am Text invalidieren die berechnete Höhe.
     */
    void setAutoBounds(int x, int minY, int w, int screenHeight = 480);

    /**
     * @brief Setzt Labels für OK/Abbrechen; wenn cancelLabel leer/null, nur ein Button.
     * @param okLabel     Beschriftung für den primären Button
     * @param cancelLabel Beschriftung für den sekundären Button oder nullptr/"" für keinen Cancel-Button
     */
    void setButtonLabels(const char *okLabel, const char *cancelLabel);

    /**
     * @brief Ein-Button-Variante (nur OK).
     * @param okLabel Beschriftung für den Button
     */
    void setButtonLabel(const char *okLabel);

    /**
     * @brief Automatisches Timeout (0 = deaktiviert)
     * @param millis Millisekunden
     */
    void setTimeout(uint32_t millis);

    /**
     * @brief Timeout für den Pending-Zustand (Warten auf externe Bestätigung)
     * @param millis Millisekunden (0 = deaktiviert, default: 30000)
     *
     * Falls die externe Verarbeitung nicht innerhalb dieser Zeit completeConfirmation() aufruft, wird der Dialog mit false beendet.
     */
    void setPendingTimeout(uint32_t millis);

    /**
     * @brief Setzt den Titel (Kopfzeile). Intern kopiert (trunkiert).
     * @param title Titeltext
     */
    void setTitle(const char *title);

    /**
     * @brief Setzt den Nachrichtentext. Intern kopiert (trunkiert). '\n' möglich.
     * @param message Nachrichtentext
     *
     * @warning Bei Verwendung von setAutoBounds() MUSS setMessage() VORHER
     *          aufgerufen werden, da die Höhe aus dem Text berechnet wird.
     */
    void setMessage(const char *message);

    /**
     * @brief Setzt zweispaltige Detailzeilen. Intern kopiert (trunkiert).
     * @param labels Linke Spalte mit Beschriftungen
     * @param values Rechte Spalte mit Werten
     * @param count Anzahl der Zeilen
     */
    void setDetailRows(const char *const labels[], const char *const values[], size_t count);

    /**
     * @brief Callback nach Abschluss (true = OK, false = Abbrechen/Timeout)
     * @param callback Funktionsobjekt
     */
    void setCallback(std::function<void(bool)> callback);

    /**
     * @brief Startet die Anzeige (Backup + Zeichnen). Nicht blockierend.
     */
    void start();

    /**
     * @brief Aktualisiert Touch/Timeout. Nicht blockierend, zyklisch aufrufen.
     */
    void update();

    /**
     * @brief Ist die Box aktiv (wartet auf Eingabe)?
     * @return true nur wenn State::Active (User kann OK/Cancel klicken)
     * @note Für "ist die Box sichtbar?" stattdessen isVisible() verwenden
     */
    bool isActive() const;

    /**
     * @brief Ist die Box sichtbar (auf dem Bildschirm)?
     * @return true wenn Active oder ConfirmationPending
     *
     * Verwenden wenn geprüft werden soll, ob die Box noch angezeigt wird, unabhängig davon ob sie gerade auf Input wartet oder busy ist.
     */
    bool isVisible() const;

    /**
     * @brief Ist die Box busy (wartet auf externe Verarbeitung)?
     * @return true wenn im ConfirmationPending-Zustand
     *
     * In diesem Zustand wurde OK geklickt und die Box wartet auf completeConfirmation(). Cancel ist weiterhin möglich.
     */
    bool isBusy() const;

    /**
     * @brief Setzt den Text der im Lade-Zustand angezeigt wird
     * @param text Lade-Text (z.B. "Wird aktualisiert...")
     */
    void setLoadingText(const char *text);

    /**
     * @brief Schließt die Box nach externer Verarbeitung ab
     * @param success true wenn erfolgreich, false bei Fehler
     *
     * Nur aufrufen wenn isBusy() true ist. Ruft den Callback mit dem Ergebnis auf und schließt die Box.
     */
    void completeConfirmation(bool success);

    /**
     * @brief Prüft ob der Caller die darunterliegende UI neu zeichnen muss
     * @return true wenn Backup/Restore fehlgeschlagen ist
     *
     * Nach dem Schließen der MessageBox sollte der Caller diese Methode prüfen. Bei true muss die UI unter der MessageBox neu gerendert werden, da der ursprüngliche Bildschirminhalt nicht wiederhergestellt werden konnte.
     */
    bool needsRedraw() const;

private:
    // UI-Konstanten
    struct UIConfig
    {
        static constexpr uint16_t COLOR_SEPARATOR = 0xd206;
        static constexpr uint16_t COLOR_TEXT = 0xbdf7;
        static constexpr uint16_t COLOR_DETAIL_LABEL = 0x7BEF;
        static constexpr uint16_t COLOR_DETAIL_VALUE = 0xD69A;
        static constexpr uint16_t COLOR_DETAIL_DIVIDER = 0x39E8;
        static constexpr uint16_t COLOR_BUTTON_BORDER = 0x39E8;
        static constexpr uint16_t COLOR_BUTTON_TEXT_OK = 0xBDF7;
        static constexpr uint16_t COLOR_BUTTON_TEXT_CANCEL = 0x7BEF;

        static constexpr int PADDING = 20;
        static constexpr int BORDER_RADIUS = 4;
        static constexpr int BUTTON_WIDTH = 80;
        static constexpr int BUTTON_HEIGHT = 45;
        static constexpr int BUTTON_SPACING = 10;
        static constexpr int BUTTON_PADDING_X = 18;
        static constexpr int LINE_HEIGHT_FACTOR = 2; ///< Zeilenhöhe = Fontgröße + x Pixel Zeilenabstand
        static constexpr int TITLE_Y_OFFSET = 10;
        static constexpr int SEPARATOR_Y = 48;
        static constexpr int TEXT_Y_START = 65;

        static constexpr size_t MAX_LABEL_LEN = 32;
        static constexpr size_t MAX_TITLE_LEN = 64;
        static constexpr size_t MAX_MESSAGE_LEN = 512;
        static constexpr size_t MAX_DETAIL_ROWS = 10;
        static constexpr size_t MAX_DETAIL_LABEL_LEN = 24;
        static constexpr size_t MAX_DETAIL_VALUE_LEN = 48;
    };

    struct DetailRow
    {
        char label[UIConfig::MAX_DETAIL_LABEL_LEN] = ""; ///< Beschriftung der Detailzeile
        char value[UIConfig::MAX_DETAIL_VALUE_LEN] = ""; ///< Wert der Detailzeile
    };

    // Touch-Zustand
    struct TouchState
    {
        bool isTouching = false;
        bool wasOnOk = false;
        bool wasOnCancel = false;
    };

    enum class State
    {
        Hidden,
        Active,
        ConfirmationPending, ///< OK wurde geklickt, wartet auf externe Bestätigung
        Finished
    };

    // Display und UI-Komponenten
    LGFX *m_tft; // Pointer auf das Displayobjekt
    Button m_btnOk;
    Button m_btnCancel;
    State m_state = State::Hidden;
    std::function<void(bool)> m_callback = nullptr;

    // Texte (feste Puffer, String-frei)
    char m_labelOk[UIConfig::MAX_LABEL_LEN] = "OK";
    char m_labelCancel[UIConfig::MAX_LABEL_LEN] = "Abbrechen";
    char m_titleText[UIConfig::MAX_TITLE_LEN] = "";
    char m_messageText[UIConfig::MAX_MESSAGE_LEN] = "";
    DetailRow m_detailRows[UIConfig::MAX_DETAIL_ROWS] = {};
    size_t m_detailRowCount = 0;
    bool m_hasCancel = true;

    // Geometrie
    int m_msgX = 0, m_msgY = 0, m_msgW = 0, m_msgH = 0;
    bool m_autoBoundsCalculated = false; ///< true wenn setAutoBounds() aufgerufen wurde

    // Timeout
    uint32_t m_timeoutMs = 0;
    uint32_t m_startTime = 0;
    uint32_t m_pendingTimeoutMs = 30000; ///< Timeout im Pending-Zustand (default 30s)
    uint32_t m_pendingStartTime = 0;     ///< Startzeit des Pending-Zustands

    // Bildschirm-Backup
    lgfx::rgb565_t *m_screenBuffer = nullptr;
    size_t m_bufferSizeBytes = 0;
    bool m_backupValid = false;
    bool m_needsRedraw = false; ///< true wenn Restore fehlgeschlagen, Caller muss UI neu zeichnen

    // Touch/Ergebnis
    TouchState m_touchState;
    bool m_resultConfirmed = false;

    // Lade-Zustand
    char m_loadingText[64] = "Bitte warten..."; ///< Text im Lade-Zustand

    // private Methoden
    void backup();
    void restore();
    void draw();
    void drawFrame();
    void drawHeader();
    void drawSeparator();
    void drawMessageText();
    void drawDetailRows(); ///< Zeichnet zweispaltige Detailzeilen
    void drawButtons();
    int buttonWidthForLabel(const char *label) const;                                      ///< Berechnet Buttonbreite aus Textbreite und Innenabstand
    void calculateButtonLayout(int &okX, int &okW, int &cancelX, int &cancelW) const;      ///< Berechnet Position und Breite der Dialogbuttons
    void drawMultilineText(int x, int y, int maxWidth, const char *text); ///< Text mit Wortumbruch zeichnen (wortweise optimiert)
    int calculateTextLineCount(int maxWidth, const char *text);           ///< Berechnet Anzahl Zeilen (wortweise optimiert)
    void processTouchInput();
    void processPendingInput(); ///< Touch-Input im Pending-Zustand (nur Cancel)
    void checkTimeout();
    void checkPendingTimeout(); ///< Pending-Timeout überwachen
    void finish(bool result);
    void drawLoadingState(); ///< Zeichnet den Lade-Zustand (Buttons werden durch zentrierten Text ersetzt)
};

#endif // MESSAGEBOX_H
