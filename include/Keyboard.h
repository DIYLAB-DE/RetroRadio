#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <memory>
#include <vector>

#include "Button.h"
#include "Gfx.h"

/**
 * @struct KeyDef
 * @brief Definiert die Eigenschaften einer einzelnen Taste auf der virtuellen Tastatur.
 */
struct KeyDef
{
    const char *label;    ///< Der Text, der auf der Taste angezeigt wird.
    const char *action;   ///< Die Aktion, die bei einem Tastendruck ausgeführt wird.
    float width_units;    ///< Die relative Breite der Taste in Einheiten (1.0 = Standardbreite).
    const uint16_t *icon; ///< Optional: Ein Icon, das anstelle des Textes angezeigt wird.
    uint16_t labelColor;  ///< Optional: Individuelle Textfarbe (0 = Standardfarbe verwenden).
    uint8_t labelDatum;   ///< Optional: Text-Ausrichtung (MC_DATUM=zentriert, ML_DATUM=links, MR_DATUM=rechts).
};

/**
 * @class Keyboard
 * @brief Virtuelle Touch-Tastatur für ESP32-Systeme mit LovyanGFX.
 *
 * @details Diese Klasse bietet eine vollständige virtuelle Tastatur mit deutschem QWERTZ-Layout, Modi für Klein- und Großbuchstaben sowie Sonderzeichen, Cursor-Positionierung per Touch und verdeckter Passworteingabe.
 *
 * @code
 * Keyboard keyboard(&gfx);
 * keyboard.setPosition(); // Tastatur zentrieren
 *
 * char buffer[64];
 * if (keyboard.show("SSID eingeben:", buffer, sizeof(buffer))) {
 *     LOG_SUCCESS_F("Eingabe: %s", buffer);
 * } else {
 *     LOG_INFO("Eingabe abgebrochen.");
 * }
 * @endcode
 */
class Keyboard
{
public:
    /**
     * @brief Konstruktor der Keyboard-Klasse.
     * @param gfx Ein Zeiger auf das LovyanGFX-Objekt für alle Grafikoperationen.
     */
    explicit Keyboard(LGFX *gfx);

    /**
     * @brief Destruktor. Gibt alle allozierten Ressourcen frei.
     */
    ~Keyboard();

    // Keine Kopien erlauben
    Keyboard(const Keyboard &) = delete;
    Keyboard &operator=(const Keyboard &) = delete;

    /**
     * @brief Zeigt die Tastatur an und startet die Eingabeschleife.
     * @param label Die Beschriftung über dem Eingabefeld.
     * @param buffer Der Puffer für den eingegebenen Text.
     * @param buffer_size Die Größe des Puffers.
     * @param initial_text Optionaler Text, der bei Anzeige bereits im Eingabefeld steht.
     * @return true wenn mit OK bestätigt, false wenn abgebrochen.
     */
    bool show(const char *label, char *buffer, size_t buffer_size, const char *initial_text = "");

    /**
     * @brief Setzt die Position der Tastatur auf dem Bildschirm.
     * @param x Die X-Koordinate der linken oberen Ecke.
     * @param y Die Y-Koordinate der linken oberen Ecke.
     */
    void setPosition(int x, int y);

    /**
     * @brief Zentriert die Tastatur auf einem 800x480 Pixel großen Bildschirm.
     */
    void setPosition();

    /**
     * @brief Legt die Schriftart für die Tastenbeschriftungen fest.
     * @param font Ein Zeiger auf die Schriftart-Daten.
     */
    void setKeyFont(const uint8_t *font) { m_keyFont = font; }

    /**
     * @brief Legt die Schriftart für das Eingabefeld fest.
     * @param font Ein Zeiger auf die Schriftart-Daten.
     */
    void setInputFont(const uint8_t *font) { m_inputFont = font; }

    /**
     * @brief Legt die Schriftart für das Label über dem Eingabefeld fest.
     * @param font Ein Zeiger auf die Schriftart-Daten.
     */
    void setLabelFont(const uint8_t *font) { m_labelFont = font; }

    /**
     * @brief Legt die Farbe für das Label fest.
     * @param color Ein 16-Bit RGB565 Farbwert.
     */
    void setLabelColor(uint16_t color) { m_labelColor = color; }

    /**
     * @brief Legt die Textfarbe für das Symbol-Layout (Kleinbuchstaben-Modus) fest.
     * @param color Ein 16-Bit RGB565 Farbwert.
     */
    void setSymbolsLowercaseLabelColor(uint16_t color) { m_symbolsLowercaseLabelColor = color; }

    /**
     * @brief Legt die Textfarbe für das Symbol-Layout (Großbuchstaben-Modus) fest.
     * @param color Ein 16-Bit RGB565 Farbwert.
     */
    void setSymbolsUppercaseLabelColor(uint16_t color) { m_symbolsUppercaseLabelColor = color; }

    /**
     * @brief Aktiviert oder deaktiviert den Passwort-Modus.
     * @param enable true für verdeckte Eingabe, false für normale Textanzeige.
     * @note Im Passwort-Modus erscheint ein Button, um die Eingabe temporär sichtbar zu machen.
     */
    void setPasswordMode(bool enable)
    {
        m_passwordMode = enable;
        m_passwordVisible = false;
    }

    /**
     * @brief Setzt die Beschriftung der Bestätigen-Taste (Default: "OK").
     * @param label Neuer Text für die Bestätigen-Taste.
     *
     * Der übergebene String wird intern in einen Member-Puffer kopiert, sodass die Lebensdauer des Aufrufer-Pointers nicht relevant ist. Maximal 15 Zeichen plus Null-Terminator. Wird vor jedem `show()` gerufen, um der UI-Sprache zu folgen.
     */
    void setOkLabel(const char *label);

private:
    // ============================================================
    // Enums
    // ============================================================

    /**
     * @enum KeyboardState
     * @brief Beschreibt den aktuellen Zustand bzw. das Layout der Tastatur.
     */
    enum class KeyboardState
    {
        LOWERCASE,         ///< Kleinbuchstaben (Standard)
        UPPERCASE,         ///< Großbuchstaben
        SYMBOLS_LOWERCASE, ///< Sonderzeichen (Kleinbuchstaben-Variante)
        SYMBOLS_UPPERCASE  ///< Sonderzeichen (Großbuchstaben-Variante)
    };

    // ============================================================
    // Konfigurationsstrukturen
    // ============================================================

    /**
     * @struct Colors
     * @brief Farbpalette im Ubuntu-Stil (RGB565).
     */
    struct Colors
    {
        static constexpr uint16_t KEY_NORMAL = 0x4A69;             ///< Standard-Taste
        static constexpr uint16_t KEY_PRESSED = 0x6B6D;            ///< Gedrückte Taste
        static constexpr uint16_t KEY_SPECIAL = 0x3986;            ///< Spezial-Tasten (Shift, etc.)
        static constexpr uint16_t KEY_SHIFT_ACTIVE = 0x4C16;       ///< Aktive Shift/Caps-Taste
        static constexpr uint16_t KEY_DISABLED = 0x2104;           ///< Deaktivierte Taste
        static constexpr uint16_t TEXT = 0xFFFF;                   ///< Textfarbe
        static constexpr uint16_t CURSOR = 0x07FF;                 ///< Cursor
        static constexpr uint16_t SYMBOLS_LOWERCASE_TEXT = 0xFFFF; ///< Symboltext Lowercase
        static constexpr uint16_t SYMBOLS_UPPERCASE_TEXT = 0xFFFF; ///< Symboltext Uppercase
    };

    /**
     * @struct Layout
     * @brief Geometrie-Konstanten für das Tastatur-Layout.
     *
     * Das Layout basiert auf einem Grid-System ähnlich Bootstrap:
     * - GRID_UNITS definiert die Gesamtanzahl der Units pro Reihe
     * - Jede Reihe hat exakt GRID_UNITS × UNIT_WIDTH Pixel verfügbar
     * - Tasten mit 1.0f width_units sind immer gleich breit
     * - Reihen mit weniger Units werden zentriert
     */
    struct Layout
    {
        static constexpr int KEYBOARD_WIDTH = 770; ///< Gesamtbreite
        static constexpr int KEYBOARD_ROWS = 5;    ///< Anzahl Reihen
        static constexpr int MAX_COLS = 16;        ///< Maximale Tasten pro Reihe
        static constexpr int KEY_HEIGHT = 45;      ///< Tastenhöhe
        static constexpr int KEY_GAP = 4;          ///< Abstand zwischen Tasten
        static constexpr int KEY_RADIUS = 4;       ///< Eckenradius der Tasten
        static constexpr int BORDER_RADIUS = 4;    ///< Rahmenradius
        static constexpr int PADDING = 10;         ///< Innenabstand
        static constexpr int LABEL_HEIGHT = 40;    ///< Höhe des Labels
        static constexpr int INPUT_HEIGHT = 36;    ///< Höhe des Eingabefelds
        static constexpr int INPUT_MARGIN = 8;     ///< Abstand um Eingabefeld

        /// Grid-Basis: Anzahl der Units, die eine volle Reihe ausmachen
        static constexpr float GRID_UNITS = 14.0f;

        /// Feste Breite einer 1.0 Unit in Pixeln (wird aus KEYBOARD_WIDTH berechnet)
        /// Formel: (KEYBOARD_WIDTH - 2*PADDING - (GRID_UNITS-1)*KEY_GAP) / GRID_UNITS
        static constexpr float UNIT_WIDTH = (KEYBOARD_WIDTH - 2 * PADDING - (static_cast<int>(GRID_UNITS) - 1) * KEY_GAP) / GRID_UNITS;

        // Berechnete Werte
        static constexpr int KEYBOARD_PART_HEIGHT = (KEYBOARD_ROWS * KEY_HEIGHT) + ((KEYBOARD_ROWS - 1) * KEY_GAP);
        static constexpr int LABEL_Y_REL = PADDING;
        static constexpr int INPUT_Y_REL = LABEL_Y_REL + LABEL_HEIGHT;
        static constexpr int KEYBOARD_Y_REL = INPUT_Y_REL + INPUT_HEIGHT + INPUT_MARGIN;
        static constexpr int TOTAL_HEIGHT = KEYBOARD_Y_REL + KEYBOARD_PART_HEIGHT + PADDING;

        // Standard-Position (zentriert auf 800x480)
        static constexpr int DEFAULT_X = (800 - KEYBOARD_WIDTH) / 2;
        static constexpr int DEFAULT_Y = (480 - TOTAL_HEIGHT) / 2;
    };

    /**
     * @struct Timing
     * @brief Timing-Konstanten für Touch-Handling und Animationen.
     */
    struct Timing
    {
        static constexpr uint32_t TOUCH_DEBOUNCE_MS = 30;  ///< Touch-Entprellung
        static constexpr uint32_t REPEAT_INITIAL_MS = 400; ///< Verzögerung bis Auto-Repeat
        static constexpr uint32_t REPEAT_INTERVAL_MS = 80; ///< Auto-Repeat Intervall
        static constexpr uint32_t CURSOR_BLINK_MS = 500;   ///< Cursor-Blink-Intervall
        static constexpr uint32_t STARTUP_GUARD_MS = 150;  ///< Schutz nach Öffnen
    };

    // ============================================================
    // Aktions-Konstanten
    // ============================================================

    static constexpr const char *ACTION_BACKSPACE = "BACKSPACE";
    static constexpr const char *ACTION_SHIFT = "SHIFT";
    static constexpr const char *ACTION_CAPSLOCK = "CAPSLOCK";
    static constexpr const char *ACTION_SYMBOLS = "SYMBOLS";
    static constexpr const char *ACTION_LETTERS = "LETTERS";
    static constexpr const char *ACTION_SPACE = "SPACE";
    static constexpr const char *ACTION_OK = "OK";
    static constexpr const char *ACTION_CANCEL = "CANCEL";
    static constexpr const char *ACTION_LEFT = "LEFT";
    static constexpr const char *ACTION_RIGHT = "RIGHT";

    // ============================================================
    // Layout-Definitionen
    // ============================================================

    static const KeyDef LAYOUT_LOWERCASE[Layout::KEYBOARD_ROWS][Layout::MAX_COLS];
    static const KeyDef LAYOUT_UPPERCASE[Layout::KEYBOARD_ROWS][Layout::MAX_COLS];
    static const KeyDef LAYOUT_SYMBOLS_LOWERCASE[Layout::KEYBOARD_ROWS][Layout::MAX_COLS];
    static const KeyDef LAYOUT_SYMBOLS_UPPERCASE[Layout::KEYBOARD_ROWS][Layout::MAX_COLS];

    static const KeyDef LAYOUT_EN_LOWERCASE[Layout::KEYBOARD_ROWS][Layout::MAX_COLS];
    static const KeyDef LAYOUT_EN_UPPERCASE[Layout::KEYBOARD_ROWS][Layout::MAX_COLS];
    static const KeyDef LAYOUT_EN_SYMBOLS_LOWERCASE[Layout::KEYBOARD_ROWS][Layout::MAX_COLS];
    static const KeyDef LAYOUT_EN_SYMBOLS_UPPERCASE[Layout::KEYBOARD_ROWS][Layout::MAX_COLS];

    // ============================================================
    // Member-Variablen
    // ============================================================

    // Grafik
    LGFX *m_tft;
    Gfx_Sprite *m_inputSprite;
    lgfx::rgb565_t *m_screenBuffer;
    bool m_backupValid;

    // Eingabe-Zustand
    char *m_inputBuffer;
    size_t m_bufferSize;
    size_t m_cursorPos;

    // UI-Zustand
    bool m_running;
    bool m_result;
    KeyboardState m_state;
    bool m_capsLock; ///< Caps Lock aktiv

    // Passwort-Modus
    bool m_passwordMode;
    bool m_passwordVisible;

    // Beschriftung der Bestätigen-Taste (überschreibt LAYOUT-Label für ACTION_OK)
    char m_okLabel[16] = "OK"; ///< Default sprachneutral, wird vom Aufrufer vor `show()` überschrieben

    // Fonts und Farben
    const uint8_t *m_keyFont;
    const uint8_t *m_inputFont;
    const uint8_t *m_labelFont;
    uint16_t m_labelColor;
    uint16_t m_symbolsLowercaseLabelColor; ///< Textfarbe für Symbol-Layout (Lowercase)
    uint16_t m_symbolsUppercaseLabelColor; ///< Textfarbe für Symbol-Layout (Uppercase)

    // Position
    int m_posX;
    int m_posY;

    // Touch-Handling
    Button *m_lastPressedButton;
    const char *m_lastAction;
    bool m_touchActive;
    uint32_t m_touchStartTime;
    uint32_t m_lastRepeatTime;
    uint32_t m_lastReleaseTime;
    uint32_t m_startTime;

    // Cursor-Blink
    bool m_lastBlinkState;
    bool m_forceRedrawInput;

    // Buttons
    std::unique_ptr<Button> m_closeButton;
    std::unique_ptr<Button> m_togglePasswordButton;
    std::vector<std::unique_ptr<Button>> m_buttons;
    std::vector<const char *> m_buttonActions;

    // ============================================================
    // Private Methoden
    // ============================================================

    // Screen-Backup
    void backupScreen();
    void restoreScreen();

    // Zeichnen
    void drawFrame();
    void drawLabel(const char *label);
    void setupAndDrawLayout();
    void drawInputField();

    // Touch-Handling
    void handleTouch();
    void processKey(const char *action);

    // Texteingabe
    void insertText(const char *text);
    void deleteCharacter();
    void moveCursor(int direction);
    size_t getCursorPositionFromTouch(int touch_x);

    // Zustandswechsel
    void switchState(KeyboardState newState);

    // Hilfsfunktionen
    uint16_t getKeyColor(const char *action) const;
    uint16_t getKeyTextColor(const char *action) const;
    bool isSpecialAction(const char *action) const;
    bool isRepeatableAction(const char *action) const;
    bool isSymbolState() const;
};

#endif // KEYBOARD_H
