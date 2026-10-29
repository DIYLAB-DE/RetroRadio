#ifndef PROGRESSBOX_H
#define PROGRESSBOX_H

#include <functional>

#include "Gfx.h"

/**
 * @brief Nicht-blockierende Fortschrittsanzeige mit Sprite-Rendering
 *
 * Zeigt einen Fortschrittsbalken mit Text in einem modalen Dialog an. Der Hintergrund wird gesichert und nach Beendigung wiederhergestellt. Rendering erfolgt flackerfrei über Sprites.
 */
class ProgressBox
{
public:
    /**
     * @brief Konstruktor
     * @param gfx Referenz auf das Display-Objekt
     */
    explicit ProgressBox(LGFX &gfx);

    /**
     * @brief Destruktor - gibt alle Ressourcen frei
     */
    ~ProgressBox();

    /**
     * @brief Position und Größe der ProgressBox festlegen
     *
     * Ohne Aufruf dieser Methode wird die Box automatisch zentriert.
     *
     * @param x X-Position oben links
     * @param y Y-Position oben links
     * @param w Breite in Pixeln
     * @param h Höhe in Pixeln
     */
    void setBounds(int x, int y, int w, int h);

    /**
     * @brief Automatisches Zentrieren aktivieren
     *
     * Die Box wird mittig auf dem Display positioniert. Standard-Größe: 400x200 Pixel
     */
    void setAutoCenter(bool center = true);

    /**
     * @brief Titel der ProgressBox setzen
     * @param title Titel-Text (wird kopiert)
     */
    void setTitle(const char *title);

    /**
     * @brief Nachrichtentext setzen
     * @param message Text über dem Fortschrittsbalken (wird kopiert)
     */
    void setMessage(const char *message);

    /**
     * @brief Segment-Layout (Strich-/Lückenbreite) konfigurieren
     */
    void setBarStyle(int segmentWidth, int gapWidth);

    /**
     * @brief Farben der Fortschrittsbalken-Segmente setzen
     */
    void setBarColors(uint16_t active, uint16_t cap, uint16_t bg);

    /**
     * @brief Callback für Abschluss setzen
     *
     * Wird aufgerufen wenn finish() aufgerufen wird.
     *
     * @param callback Callback-Funktion
     */
    void setCallback(std::function<void()> callback);

    /**
     * @brief ProgressBox anzeigen
     *
     * Sichert den Hintergrund und zeichnet die Box initial.
     *
     * @param message Optionaler initialer Text
     * @param progress Optionaler initialer Fortschritt (0-100)
     */
    void start(const char *message = nullptr, int progress = 0);

    /**
     * @brief Fortschritt aktualisieren
     *
     * Aktualisiert nur den Fortschrittsbalken ohne Flackern.
     *
     * @param progress Fortschritt in Prozent (0-100), -1 für unbestimmt
     */
    void updateProgress(int progress);

    /**
     * @brief Nachrichtentext aktualisieren
     *
     * Aktualisiert nur den Text ohne Flackern.
     *
     * @param message Neuer Text
     */
    void updateMessage(const char *message);

    /**
     * @brief Fortschritt und Text gleichzeitig aktualisieren
     *
     * Effizienter als separate Aufrufe.
     *
     * @param message Neuer Text
     * @param progress Neuer Fortschritt (0-100), -1 für unbestimmt
     */
    void update(const char *message, int progress);

    /**
     * @brief ProgressBox schließen
     *
     * Stellt den Hintergrund wieder her und ruft ggf. Callback auf.
     */
    void finish();

    /**
     * @brief Prüft ob die Box aktiv ist
     * @return true wenn aktiv, false wenn geschlossen
     */
    bool isActive() const;

    /**
     * @brief Prüft ob der Caller die darunterliegende UI neu zeichnen muss
     * @return true wenn Backup/Restore fehlgeschlagen ist
     * 
     * Nach dem Schließen der ProgressBox sollte der Caller diese Methode prüfen. Bei true muss die UI unter der ProgressBox neu gerendert werden, da der ursprüngliche Bildschirminhalt nicht wiederhergestellt werden konnte.
     */
    bool needsRedraw() const;

    /**
     * @brief Animationsupdate für unbestimmten Fortschritt
     *
     * Sollte zyklisch aufgerufen werden wenn progress = -1. Animiert einen "Indeterminate" Progress-Balken.
     */
    void updateAnimation();

private:
    /**
     * @brief UI-Konfiguration
     */
    struct UIConfig
    {
        // Farben
        static constexpr uint16_t COLOR_BORDER = 0x39E8;
        static constexpr uint16_t COLOR_TITLE = 0xFFFF;
        static constexpr uint16_t COLOR_TEXT = 0xBDF7;
        static constexpr uint16_t COLOR_PROGRESS_ACTIVE = TFT_DARKGREY;
        static constexpr uint16_t COLOR_PROGRESS_CAP = 0x3186;
        static constexpr uint16_t COLOR_PROGRESS_BG = 0x2104;
        static constexpr uint16_t COLOR_PROGRESS_DEFAULT = 0x07E0;
        static constexpr uint16_t COLOR_SEPARATOR = 0xD206;

        // Geometrie
        static constexpr int DEFAULT_WIDTH = 400;
        static constexpr int DEFAULT_HEIGHT = 200;
        static constexpr int PADDING = 20;
        static constexpr int BORDER_RADIUS = 4;
        static constexpr int PROGRESS_HEIGHT = 40;
        static constexpr int LINE_HEIGHT_FACTOR = 2; ///< Zeilenhöhe = Fontgröße + x Pixel Zeilenabstand
        static constexpr int TITLE_Y_OFFSET = 10;
        static constexpr int SEPARATOR_Y = 48;
        static constexpr int TITLE_HEIGHT = 30;
        static constexpr int TEXT_Y_OFFSET = 40;
        static constexpr int TEXT_Y_START = 65;

        // Segment-Layout
        static constexpr int DEFAULT_SEG_W = 1;
        static constexpr int DEFAULT_GAP_W = 2;

        // Text-Limits
        static constexpr size_t MAX_TITLE_LEN = 64;
        static constexpr size_t MAX_MESSAGE_LEN = 128;
    };

    /**
     * @brief Zustand der ProgressBox
     */
    enum class State
    {
        Hidden,  ///< Nicht sichtbar
        Active,  ///< Aktiv und sichtbar
        Finished ///< Beendet
    };

    // Core
    LGFX &m_tft;
    Gfx_Sprite *m_sprite = nullptr;
    State m_state = State::Hidden;
    std::function<void()> m_callback = nullptr;

    // Text-Puffer
    char m_titleText[UIConfig::MAX_TITLE_LEN] = "";
    char m_messageText[UIConfig::MAX_MESSAGE_LEN] = "";

    // Geometrie
    int m_boxX = 0;
    int m_boxY = 0;
    int m_boxW = UIConfig::DEFAULT_WIDTH;
    int m_boxH = UIConfig::DEFAULT_HEIGHT;
    bool m_autoCenter = true;

    // Fortschritt
    int m_currentProgress = 0;

    // Segmentiertes Rendering
    int m_segW = UIConfig::DEFAULT_SEG_W;
    int m_gapW = UIConfig::DEFAULT_GAP_W;
    uint16_t m_barColorActive = UIConfig::COLOR_PROGRESS_ACTIVE;
    uint16_t m_barColorCap = UIConfig::COLOR_PROGRESS_CAP;
    uint16_t m_barColorBG = UIConfig::COLOR_PROGRESS_BG;

    // Animation für unbestimmten Fortschritt
    uint32_t m_animationTime = 0;
    int m_animationPos = 0;
    bool m_indeterminate = false;

    // Bildschirm-Backup
    lgfx::rgb565_t *m_backupBuffer = nullptr;
    size_t m_backupBufferSize = 0;
    bool m_backupValid = false;
    bool m_needsRedraw = false;  ///< true wenn Restore fehlgeschlagen, Caller muss UI neu zeichnen

    // Private Methoden
    void calculateBounds();
    void backup();
    void restore();
    void drawBox();
    void drawFrame();
    void drawTitle();
    void drawMessage();
    void drawMultilineText(int x, int y, int maxWidth, const char *text);
    void drawProgressBar();
    void drawIndeterminateBar();
};

#endif // PROGRESSBOX_H
