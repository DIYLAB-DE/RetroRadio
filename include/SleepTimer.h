#ifndef SLEEPTIMER_H
#define SLEEPTIMER_H

#include <functional>

#include "Button.h"
#include "Gfx.h"

#define MAX_TIMER_OPTIONS 6

/**
 * @brief Sleep-Timer Dialog mit Countdown-Funktionalität
 */
class SleepTimer
{
public:
    /**
     * @brief Konstruktor
     * @param gfx Referenz auf das Display-Objekt
     */
    explicit SleepTimer(LGFX *gfx);

    /**
     * @brief Destruktor
     */
    ~SleepTimer();

    /**
     * @brief Definiert Position und Größe des Dialogs
     * @param x X-Position (linke obere Ecke)
     * @param y Y-Position (linke obere Ecke)
     * @param w Breite in Pixeln
     * @param h Höhe in Pixeln
     */
    void setBounds(int x, int y, int w, int h);

    /**
     * @brief Setzt den Dialog-Titel
     * @param title Titeltext
     */
    void setTitle(const char *title);

    /**
     * @brief Setzt den Beschreibungstext
     * @param message Nachrichtentext
     */
    void setMessage(const char *message);

    /**
     * @brief Setzt das Label des Abbruch-Buttons
     * @param label Neuer Text. Bei nullptr bleibt der bisherige Wert erhalten.
     *
     * Das Label wird intern kopiert, der Aufrufer darf einen Stack-Puffer übergeben. Default ist "abbruch", solange der Setter nicht aufgerufen wurde.
     */
    void setCancelLabel(const char *label);

    /**
     * @brief Setzt das Label des Stopp-Buttons (sichtbar bei laufendem Sleep-Countdown)
     * @param label Neuer Text. Bei nullptr bleibt der bisherige Wert erhalten.
     *
     * Das Label wird intern kopiert, der Aufrufer darf einen Stack-Puffer übergeben. Default ist "stopp", solange der Setter nicht aufgerufen wurde.
     */
    void setStopLabel(const char *label);

    /**
     * @brief Konfiguriert die verfügbaren Timer-Optionen
     * @param labels Array mit Button-Beschriftungen (darf nicht nullptr sein)
     * @param values Array mit Minutenwerten (darf nicht nullptr sein)
     * @param count Anzahl der Optionen (max. MAX_TIMER_OPTIONS)
     * 
     * Bei ungültigen Eingaben (nullptr oder count <= 0) wird buttonCount auf 0 gesetzt. Einzelne nullptr-Labels werden durch Leerstrings ersetzt.
     */
    void setOptions(const char *labels[], const int values[], int count);

    /**
     * @brief Registriert Callback für Timer-Auswahl
     * @param cb Funktion mit Parameter int (Minuten, -1=Abbruch, -2=Timer gestoppt)
     * 
     * @note Für Heap-Stabilität: Lambdas sollten maximal 1-2 Pointer capturen
     *       (z.B. [this]). Vermeide std::bind und große Captures.
     */
    void setCallback(std::function<void(int minutes)> cb);

    /**
     * @brief Aktiviert automatisches Timeout für den Dialog
     * @param millis Timeout in Millisekunden (0 = deaktiviert)
     */
    void setTimeout(uint32_t millis);

    /**
     * @brief Startet einen Sleep-Countdown
     * @param minutes Dauer in Minuten (muss > 0 sein)
     * @param onTimeout Callback-Funktion bei Timer-Ablauf
     * 
     * Bei ungültigen Werten (minutes <= 0) wird ein eventuell laufender Timer abgebrochen und kein neuer gestartet.
     * 
     * @note Für Heap-Stabilität: Lambdas sollten maximal 1-2 Pointer capturen
     *       (z.B. [this]). Vermeide std::bind und große Captures.
     */
    void startSleepCountdown(int minutes, std::function<void()> onTimeout);

    /**
     * @brief Zeigt den Dialog an
     */
    void start();

    /**
     * @brief Aktualisiert Dialog und Timer
     * @note Muss in der Hauptschleife aufgerufen werden
     */
    void update();

    /**
     * @brief Stoppt den aktiven Sleep-Timer
     */
    void cancelSleepTimer();

    /**
     * @brief Prüft ob der Dialog aktiv ist
     * @return true wenn aktiv, false sonst
     */
    bool isActive() const;

    /**
     * @brief Prüft ob ein Sleep-Timer läuft
     * @return true wenn Timer aktiv, false sonst
     */
    bool isTimerActive() const;

    /**
     * @brief Gibt die verbleibende Zeit zurück
     * @param[out] minutes Verbleibende Minuten (-1 wenn inaktiv)
     * @param[out] seconds Verbleibende Sekunden (-1 wenn inaktiv)
     */
    void getRemainingTime(int &minutes, int &seconds) const;

private:
    /**
     * @brief UI-Konfiguration und Layout-Konstanten
     */
    struct UIConfig
    {
        // Farben
        static constexpr uint16_t COLOR_BORDER = 0x24f2;
        static constexpr uint16_t COLOR_BACKGROUND = 0x0000;
        static constexpr uint16_t COLOR_TITLE = 0xec84;
        static constexpr uint16_t COLOR_SEPARATOR = 0xd206;
        static constexpr uint16_t COLOR_TEXT = 0xC618; // Silver
        static constexpr uint16_t COLOR_BUTTON_BORDER = 0x39E8;
        static constexpr uint16_t COLOR_BUTTON_TEXT = 0x7BEF; // DarkGrey
        static constexpr uint16_t COLOR_CANCEL_TEXT = 0xD69A; // LightGrey
        static constexpr uint16_t COLOR_STOP_BORDER = 0xd206;
        static constexpr uint16_t COLOR_STOP_TEXT = 0xd206;

        // Layout
        static constexpr int PADDING = 20;
        static constexpr int BORDER_RADIUS = 4;
        static constexpr int TITLE_Y_OFFSET = 10;
        static constexpr int SEPARATOR_Y = 48;
        static constexpr int SEPARATOR_HEIGHT = 2;
        static constexpr int MESSAGE_Y = 65;

        // Buttons
        static constexpr int BUTTON_HEIGHT = 45;
        static constexpr int BUTTON_SPACING = 15;
        static constexpr int MATRIX_ROWS = 2;
        static constexpr int MATRIX_COLS = 3;
    };

    /**
     * @brief Interne Zustände
     */
    enum State
    {
        Inactive, ///< Dialog nicht aktiv
        Active    ///< Dialog aktiv und wartet auf Eingabe
    };

    // Display und UI-Komponenten
    LGFX *m_tft; // Pointer auf das Displayobjekt
    Button buttons[MAX_TIMER_OPTIONS];
    Button btnCancel;
    Button btnStopTimer;

    // Timer-Optionen
    const char *buttonLabels[MAX_TIMER_OPTIONS];
    int buttonValues[MAX_TIMER_OPTIONS];
    int buttonCount = 0;

    // Dialog-Konfiguration
    const char *titleText = "Sleep-Timer";
    const char *messageText = "";
    char cancelLabel[16] = "abbruch"; ///< Label des Abbruch-Buttons, durch setCancelLabel() überschreibbar
    char stopLabel[16] = "stopp";     ///< Label des Stopp-Buttons, durch setStopLabel() überschreibbar

    // Position und Größe
    int dialogX = 0;
    int dialogY = 0;
    int dialogW = 0;
    int dialogH = 0;

    // Dialog-Zustand
    State state = Inactive;
    std::function<void(int)> callback = nullptr;
    uint32_t timeoutMs = 0;
    uint32_t dialogStartTime = 0;

    // Sleep-Timer Zustand
    bool sleepTimerActive = false;
    uint32_t sleepTimerStart = 0;
    uint32_t sleepTimerDuration = 0;
    std::function<void()> timeoutCallback = nullptr;

    // Bildschirm-Backup
    lgfx::rgb565_t *m_screenBuffer = nullptr;
    bool m_backupValid = false;

    // Touch-Tracking
    struct TouchState
    {
        bool isTouching = false;
        int activeButton = -1; // -1=none, 0-5=option, 98=stop, 99=cancel
    } touchState;

    /**
     * @brief Erstellt Backup des Bildschirminhalts
     */
    void backup();

    /**
     * @brief Stellt Bildschirminhalt wieder her
     * 
     * Falls kein gültiges Backup vorhanden ist, wird der Dialog-Bereich mit der Hintergrundfarbe gefüllt um visuelle Artefakte zu vermeiden.
     */
    void restore();

    /**
     * @brief Zeichnet den kompletten Dialog
     */
    void draw();

    /**
     * @brief Zeichnet Rahmen und Hintergrund
     */
    void drawFrame();

    /**
     * @brief Zeichnet den Titel
     * 
     * Setzt TextDatum defensiv auf TL_DATUM, um unabhängig vom externen Zustand korrekt zu zeichnen.
     */
    void drawHeader();

    /**
     * @brief Zeichnet die Trennlinie
     */
    void drawSeparator();

    /**
     * @brief Zeichnet den Beschreibungstext
     * 
     * Setzt TextDatum defensiv auf TL_DATUM, um unabhängig vom externen Zustand korrekt zu zeichnen.
     */
    void drawMessage();

    /**
     * @brief Zeichnet die Timer-Option-Buttons
     */
    void drawTimerButtons();

    /**
     * @brief Zeichnet die unteren Steuer-Buttons
     */
    void drawControlButtons();

    /**
     * @brief Verarbeitet Touch-Eingaben
     */
    void processTouchInput();

    /**
     * @brief Prüft Dialog-Timeout
     */
    void checkDialogTimeout();

    /**
     * @brief Prüft Sleep-Timer und führt ggf. Callback aus
     */
    void checkSleepTimer();

    /**
     * @brief Findet Button an Touch-Position
     * @return Button-Index oder -1
     */
    int findButtonAtPosition(int x, int y);

    /**
     * @brief Aktualisiert Button-Highlight
     * @param newActiveButton Index des neu aktiven Buttons
     * 
     * Zeichnet nur die tatsächlich geänderten Buttons neu (alter und neuer), nicht alle. Reduziert SPI-Traffic und verhindert Flackern bei Touch-Rauschen.
     */
    void updateButtonHighlight(int newActiveButton);

    /**
     * @brief Beendet Dialog
     */
    void closeDialog(int result);
};

#endif // SLEEPTIMER_H
