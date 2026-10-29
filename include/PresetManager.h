#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <functional>

#include "Button.h"
#include "Gfx.h"

#define MAX_PRESETS 10

struct Station; // Forward Declaration

/**
 * @brief Preset-Eintrag mit Stationsreferenz
 * 
 * @note Speichert nur Pointer auf Stationsnamen, keine Kopie.
 *       Quelldaten müssen für Dialogdauer gültig bleiben.
 */
struct PresetEntry
{
    const char *name; ///< Name des Presets (Zeiger auf Station-Name, nicht kopiert)
    int stationIndex; ///< Index der zugehörigen Station
};

/**
 * @brief Dialog zur Auswahl von Favoriten/Preset-Stationen
 *
 * Diese Klasse zeigt eine übersichtliche Liste von bis zu 10 Preset-Stationen in einem modalen Dialog an. Die Auswahl erfolgt per Touch mit visueller Rückmeldung.
 */
class PresetManager
{
public:
    /**
     * @brief Konstruktor
     * @param gfx Referenz auf das Display-Objekt
     */
    explicit PresetManager(LGFX *gfx);

    /**
     * @brief Destruktor - gibt Speicher frei
     */
    ~PresetManager();

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
     * @param title Titeltext (Pointer muss für Dialogdauer gültig bleiben)
     */
    void setTitle(const char *title);

    /**
     * @brief Registriert Callback für Auswahlereignisse
     * @param cb Funktion mit Parameter int (stationIndex oder -1 für Abbruch)
     */
    void setCallback(std::function<void(int selectedIndex)> cb);

    /**
     * @brief Aktiviert automatisches Timeout
     * @param millis Timeout in Millisekunden (0 = deaktiviert)
     * 
     * Der Timeout-Timer wird bei jeder Touch-Interaktion zurückgesetzt (Keep-alive), sodass der Dialog nur bei Inaktivität schließt.
     */
    void setTimeout(uint32_t millis);

    /**
     * @brief Startet den Dialog mit der Senderliste
     * @param stations Array aller verfügbaren Stationen
     * @param count Anzahl der Stationen im Array
     * 
     * @note Die Station-Namen werden nur als Pointer gespeichert,
     *       nicht kopiert. Die Stationsdaten müssen für die gesamte
     *       Dialogdauer gültig bleiben.
     * @note Erfordert gültige Dialog-Bounds (setBounds mit w,h > 0).
     *       Bei ungültigen Bounds wird nicht gestartet.
     */
    void start(Station *stations, int count);

    /**
     * @brief Aktualisiert Dialog-Logik und Touch-Verarbeitung
     * @note Muss in der Hauptschleife aufgerufen werden
     */
    void update();

    /**
     * @brief Prüft ob der Dialog aktiv ist
     * @return true wenn aktiv, false sonst
     */
    bool isActive() const;

    /**
     * @brief Gibt den zuletzt ausgewählten Station-Index zurück
     * @return Station-Index oder -1 wenn abgebrochen/keine Auswahl
     */
    int getSelectedIndex() const;

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
        static constexpr uint16_t COLOR_BUTTON_BORDER = 0x0000;
        static constexpr uint16_t COLOR_BUTTON_TEXT = 0x7BEF; // DarkGrey
        static constexpr uint16_t COLOR_BUTTON_NUMBER_BG = 0x39E8;
        static constexpr uint16_t COLOR_BUTTON_NUMBER_TEXT = 0xFFFF;
        static constexpr uint16_t COLOR_CANCEL_BORDER = 0x39E8;
        static constexpr uint16_t COLOR_CANCEL_TEXT = 0xD69A; // LightGrey

        // Layout
        static constexpr int PADDING = 20;
        static constexpr int BORDER_RADIUS = 4;
        static constexpr int TITLE_Y_OFFSET = 10;
        static constexpr int SEPARATOR_Y = 48;
        static constexpr int SEPARATOR_HEIGHT = 2;
        static constexpr int CONTENT_START_Y = 70;

        // Buttons
        static constexpr int BUTTON_HEIGHT = 45;
        static constexpr int BUTTON_SPACING = 15;
        static constexpr int CANCEL_BUTTON_HEIGHT = 45;
        static constexpr int NUMBER_CIRCLE_SIZE = 40;
        static constexpr int BUTTONS_PER_COLUMN = 5;
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
    Button presetButtons[MAX_PRESETS];
    Button btnCancel;

    // Preset-Daten
    PresetEntry presetEntries[MAX_PRESETS];
    int presetCount = 0;

    // Dialog-Konfiguration
    const char *titleText = "Favoriten"; ///< Titel (Pointer, nicht kopiert)

    // Position und Größe
    int dialogX = 0;
    int dialogY = 0;
    int dialogW = 0;
    int dialogH = 0;

    // Zustand
    State state = Inactive;
    int selectedIndex = -1;
    std::function<void(int)> callback = nullptr;

    // Timeout-Verwaltung
    uint32_t timeoutMs = 0;
    uint32_t startTime = 0;

    // Bildschirm-Backup
    lgfx::rgb565_t *m_screenBuffer = nullptr;
    bool m_backupValid = false;

    // Touch-Tracking
    struct TouchState
    {
        bool isTouching = false;
        int activeButton = -1; // -1=none, 0-9=preset, 99=cancel
    } touchState;

    /**
     * @brief Erstellt Backup des Bildschirminhalts
     */
    void backup();

    /**
     * @brief Stellt Bildschirminhalt wieder her
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
     */
    void drawHeader();

    /**
     * @brief Zeichnet die Trennlinie
     */
    void drawSeparator();

    /**
     * @brief Zeichnet alle Preset-Buttons oder Hinweis bei leerer Liste
     */
    void drawPresetButtons();

    /**
     * @brief Zeichnet den Cancel-Button
     */
    void drawCancelButton();

    /**
     * @brief Verarbeitet Touch-Eingaben
     * 
     * Setzt bei Touch-Aktivität den Timeout-Timer zurück (Keep-alive).
     */
    void processTouchInput();

    /**
     * @brief Prüft Timeout und führt ggf. Abbruch aus
     * 
     * Timeout greift nur bei Inaktivität - Touch-Interaktion setzt den Timer in processTouchInput() zurück.
     */
    void checkTimeout();

    /**
     * @brief Findet Button an Touch-Position
     * @return Button-Index oder -1
     */
    int findButtonAtPosition(int x, int y);

    /**
     * @brief Aktualisiert Button-Highlight effizient
     * @param newActiveButton Neuer aktiver Button-Index
     * 
     * Zeichnet nur die tatsächlich geänderten Buttons neu (max. 2), nicht alle. Nutzt touchState.activeButton als vorherigen Zustand.
     */
    void updateButtonHighlight(int newActiveButton);

    /**
     * @brief Beendet Dialog mit Ergebnis
     */
    void finish(int result);
};

#endif // PRESETMANAGER_H
