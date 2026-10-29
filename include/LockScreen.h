#ifndef LOCK_SCREEN_H
#define LOCK_SCREEN_H

#include <stdint.h>

#include <array>
#include <memory>

#include "Button.h"
#include "Gfx.h"

/**
 * @brief Sperrbildschirm mit PIN-Eingabe und Logo-Anzeige.
 *
 * Wird beim Systemstart als Gate vor dem MainScreen eingesetzt und kann jederzeit als Sperrbildschirm reaktiviert werden.
 */
class LockScreen
{
public:
    LockScreen();
    ~LockScreen();

    LockScreen(const LockScreen &) = delete;
    LockScreen &operator=(const LockScreen &) = delete;

    /**
     * @brief Initialisiert den Sperrbildschirm und erstellt die Buttons
     * @param tft Referenz auf das TFT-Display
     * @return true bei Erfolg
     */
    bool init(LGFX &tft);

    /**
     * @brief Zeichnet das Logo mittig auf das Display, ohne dass ein Sperrbildschirm bestehen muss.
     * @param gfx Referenz auf das TFT-Display
     *
     * Wird beim Start gerufen, damit der Bildschirm während der Einrichtung nicht leer bleibt. Die Bilddaten liegen als Feld mit innerer Bindung in LockScreen.cpp; bindet eine zweite Übersetzungseinheit denselben Kopf ein, bekommt sie eine eigene Kopie und der Flash-Bedarf steigt um rund 237 Kilobyte.
     */
    static void drawBootLogo(LGFX &gfx);

    /**
     * @brief Setzt den Versatz des Keypads nach oben
     * @param offsetPx Anzahl Pixel, um die das Keypad nach oben verschoben wird
     */
    void setKeypadOffsetUp(uint16_t offsetPx);

    /**
     * @brief Setzt die Inaktivitäts-Zeit, nach der das Keypad wieder ausgeblendet wird
     * @param timeoutMs Zeit in Millisekunden (0 schaltet das Auto-Ausblenden ab)
     *
     * Solange das Keypad sichtbar ist und kein Touch erfolgt, wird es nach Ablauf dieser Zeit ausgeblendet. Der nächste Touch dient dann wieder als Einblenden. Ohne Aufruf gilt der Default von 60 Sekunden.
     */
    void setIdleTimeoutMs(unsigned long timeoutMs);

    /**
     * @brief Aktiviert den Sperrbildschirm
     */
    void activate();

    /**
     * @brief Non-blocking Update, wird pro Frame aus MainScreen::update() aufgerufen
     * @return true wenn der Bildschirm entsperrt werden soll
     */
    bool update();

private:
    LGFX *m_tft = nullptr;             ///< Zeiger auf Display-Objekt
    Gfx_Sprite *m_sprKeypad = nullptr; ///< Zeichenfläche für das Keypad

    static constexpr int MAX_INPUT_LENGTH = 16; ///< Maximale Anzahl zwischengespeicherter Ziffern
    char m_input[MAX_INPUT_LENGTH + 1] = {};    ///< Aktuelle Eingabe
    uint8_t m_inputLength = 0;                  ///< Anzahl eingegebener Ziffern
    bool m_touchActive = false;                 ///< Touch gerade aktiv
    int8_t m_pressedKey = -1;                   ///< Index der aktuell gedrückten Taste (-1 = keine)
    bool m_keypadVisible = false;               ///< Keypad wird erst nach dem ersten Touch sichtbar
    uint16_t m_keypadOffsetUp = 0;              ///< Keypad-Versatz nach oben in Pixeln

    // Button-Rahmen-Puls
    bool m_buttonPulseActive = false;          ///< Rahmen-Puls läuft gerade
    uint8_t m_buttonPulsePhase = 0;            ///< Aktuelle Puls-Phase
    unsigned long m_buttonPulsePhaseStart = 0; ///< Zeitstempel der aktuellen Puls-Phase
    uint16_t m_buttonPulseColor = 0;           ///< Farbe der sichtbaren Puls-Phasen

    // Idle-Timeout
    static constexpr unsigned long DEFAULT_IDLE_TIMEOUT_MS = 30000;     ///< 30 Sekunden bis das Keypad wieder verschwindet
    unsigned long m_idleTimeoutMs = DEFAULT_IDLE_TIMEOUT_MS;            ///< Aktuelle Inaktivitäts-Zeit (0 = aus)
    unsigned long m_lastActivityMs = 0;                                 ///< Zeitstempel der letzten Touch-Aktivität

    // Keypad-Buttons
    static constexpr int KEY_COUNT = 12;                   ///< Gesamtzahl Tasten-Slots (3x4 Grid)
    std::array<std::unique_ptr<Button>, KEY_COUNT> m_keys; ///< Button-Array

    bool initKeypadSprite();
    void initKeys();
    void drawScreen();
    void drawLogo();
    void drawKeypad();
    void drawKeypadPressed(int8_t pressedKey);
    void drawKeypadFrames(uint16_t color);
    void renderKeypad(int8_t pressedKey, bool pulseFrames, uint16_t pulseColor);
    void clearInput();
    void cancelButtonPulse();
    void startButtonPulse(uint16_t color);
    void updateButtonPulse();
    void hideKeypad();

    /**
     * @brief Verarbeitet einen Tastendruck
     * @param keyIndex Index der gedrückten Taste (0-11)
     * @return true wenn die Eingabe korrekt war und entsperrt werden soll
     */
    bool handleKeyPress(int8_t keyIndex);

    /**
     * @brief Vergleicht die Eingabe mit der gespeicherten PIN
     * @return true wenn korrekt
     */
    bool validatePin() const;
};

#endif // LOCK_SCREEN_H
