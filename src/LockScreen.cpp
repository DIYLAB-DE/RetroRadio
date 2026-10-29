#include "LockScreen.h"

#include <cstring>

#include "AppState.h"
#include "Logger.h"
#include "Routes.h"

// Fonts
#include "fonts/ComfortaaBold30.h"

// Icons, Bilder
#include "images/logo_450.h"
#include "images/made_in_germany.h"

// ===================================================================
// Layout-Konstanten
// ===================================================================

namespace
{
    constexpr int DISPLAY_W = 800; ///< Display-Breite
    constexpr int DISPLAY_H = 480; ///< Display-Höhe

    // Logo
    constexpr int LOGO_W = 450;                      ///< Logo-Breite
    constexpr int LOGO_H = 263;                      ///< Logo-Höhe
    constexpr int LOGO_X = (DISPLAY_W - LOGO_W) / 2; ///< Logo-X-Position

    // Logo "Made in Germany"
    constexpr int MADEIN_W = 212;                        ///< Logo-Breite
    constexpr int MADEIN_H = 40;                         ///< Logo-Höhe
    constexpr int MADEIN_X = (DISPLAY_W - MADEIN_W) / 2; ///< Logo-X-Position

    // Logo-Block (Logo + "Made in Germany") vertikal zentriert
    constexpr int LOGO_GAP = 29;                                           ///< Abstand zwischen Logo und "Made in Germany"
    constexpr int LOGO_Y = (DISPLAY_H - LOGO_H - LOGO_GAP - MADEIN_H) / 2; ///< Logo-Y-Position
    constexpr int MADEIN_Y = LOGO_Y + LOGO_H + LOGO_GAP;                   ///< Y-Position des "Made in Germany"-Bilds

    // Keypad-Tasten
    constexpr int KEY_W = 65;     ///< Breite einer Taste
    constexpr int KEY_H = 70;     ///< Höhe einer Taste
    constexpr int KEY_GAP = 6;    ///< Abstand zwischen den Tasten
    constexpr int KEY_COLS = 2;   ///< Anzahl Tasten-Spalten
    constexpr int KEY_ROWS = 6;   ///< Anzahl Tasten-Zeilen
    constexpr int KEY_RADIUS = 4; ///< Eckenradius der Tasten

    constexpr int KEYPAD_W = KEY_COLS * KEY_W + (KEY_COLS - 1) * KEY_GAP; ///< Gesamtbreite des Keypads
    constexpr int KEYPAD_H = KEY_ROWS * KEY_H + (KEY_ROWS - 1) * KEY_GAP; ///< Gesamthöhe des Keypads
    constexpr int KEYPAD_MARGIN = 15;                                     ///< Abstand des Keypads zum rechten Rand
    constexpr int KEYPAD_X = DISPLAY_W - KEYPAD_MARGIN - KEYPAD_W;        ///< X-Position des Keypads
    constexpr int KEYPAD_Y = 15;                                          ///< Y-Position des Keypads

    // Button-Rahmen-Puls
    constexpr uint16_t C_CLEAR_PULSE_BORDER = 0xFD20; ///< Rahmenfarbe während der Löschbestätigung
    constexpr uint16_t C_ERROR_PULSE_BORDER = 0xF800; ///< Rahmenfarbe bei falscher Eingabe
    constexpr unsigned long BUTTON_PULSE_ON_MS = 85;  ///< Dauer einer sichtbaren Puls-Phase
    constexpr unsigned long BUTTON_PULSE_OFF_MS = 70; ///< Dauer einer normalen Puls-Phase
    constexpr uint8_t BUTTON_PULSE_PHASE_COUNT = 4;   ///< Zwei sichtbare Pulse mit normaler Phase dazwischen und am Ende
    constexpr int BUTTON_PULSE_FRAME_WIDTH = 3;       ///< Breite des Rahmen-Overlays

    // Farben
    constexpr uint16_t C_KEY_BORDER = 0x228E;       ///< Rahmenfarbe der Tasten
    constexpr uint16_t C_KEY_FILL = 0x0000;         ///< Füllfarbe der Tasten
    constexpr uint16_t C_KEY_FILL_PRESSED = 0x228E; ///< Füllfarbe einer gedrückten Taste
    constexpr uint16_t C_KEY_TEXT = 0xDE54;         ///< Textfarbe der Zifferntasten
    constexpr uint16_t C_KEY_TEXT_CLEAR = 0xFD20;   ///< Textfarbe der Löschtaste
    constexpr uint16_t C_KEY_TEXT_OK = 0x07E0;      ///< Textfarbe der OK-Taste

    // Tasten-Belegung (2 Spalten x 6 Zeilen)
    //  0=1   1=2
    //  2=3   3=4
    //  4=5   5=6
    //  6=7   7=8
    //  8=9   9=0
    // 10=c  11=ok
    constexpr int KEY_DIGIT_MAP[] = {
        1, 2,
        3, 4,
        5, 6,
        7, 8,
        9, 0,
        -1, -2};

    constexpr int KEY_CLEAR = -1;
    constexpr int KEY_OK = -2;

    constexpr const char *KEY_LABELS[] = {
        "1", "2",
        "3", "4",
        "5", "6",
        "7", "8",
        "9", "0",
        "c", "ok"};

    /**
     * @brief Liefert die Textfarbe für die angegebene Taste.
     */
    constexpr uint16_t keyTextColor(int keyIndex)
    {
        if (KEY_DIGIT_MAP[keyIndex] == KEY_CLEAR)
            return C_KEY_TEXT_CLEAR;

        if (KEY_DIGIT_MAP[keyIndex] == KEY_OK)
            return C_KEY_TEXT_OK;

        return C_KEY_TEXT;
    }
}

// ===================================================================
// Konstruktor / Destruktor
// ===================================================================

/**
 * @brief Konstruktor
 */
LockScreen::LockScreen()
{
    clearInput();
}

/**
 * @brief Destruktor
 */
LockScreen::~LockScreen()
{
    if (m_tft)
        m_tft->safeDeleteSprite(m_sprKeypad);
    else
        delete m_sprKeypad;

    m_sprKeypad = nullptr;
}

// ===================================================================
// Initialisierung
// ===================================================================

/**
 * @brief Initialisiert den Sperrbildschirm und erstellt die Buttons
 */
bool LockScreen::init(LGFX &tft)
{
    m_tft = &tft;
    clearInput();

    if (!initKeypadSprite())
        return false;

    initKeys();
    return true;
}

/**
 * @brief Setzt den Versatz des Keypads nach oben
 */
void LockScreen::setKeypadOffsetUp(uint16_t offsetPx)
{
    m_keypadOffsetUp = offsetPx;

    if (m_tft)
        initKeys();
}

/**
 * @brief Setzt die Inaktivitäts-Zeit, nach der das Keypad wieder ausgeblendet wird
 */
void LockScreen::setIdleTimeoutMs(unsigned long timeoutMs)
{
    m_idleTimeoutMs = timeoutMs;
}

/**
 * @brief Aktiviert den Sperrbildschirm
 */
void LockScreen::activate()
{
    clearInput();

    // Eine noch offene externe Entsperr-Anforderung verwerfen, damit ein frisch aktivierter Sperrbildschirm nicht sofort wieder aufspringt
    APP_STATE.consumeDisplayUnlockRequest();

    // m_touchActive beginnt mit true: Falls activate() während eines noch gehaltenen Fingers gerufen wird, muss erst das Loslassen abgewartet werden, bevor der nächste Touch als Aufweck-Tipp zählt. Bei kaltem Start ohne Touch räumt der erste update()-Aufruf das Flag im selben Frame wieder auf.
    m_touchActive = true;
    m_pressedKey = -1;
    m_keypadVisible = false;
    m_lastActivityMs = millis();
    cancelButtonPulse();
    drawScreen();

    // Webinterface mitsperren, sobald das Display gesperrt wird
    notifyDisplayLockChanged(true);
}

/**
 * @brief Erstellt die Zeichenfläche für das Keypad
 */
bool LockScreen::initKeypadSprite()
{
    if (!m_tft)
        return false;

    if (m_sprKeypad)
        return true;

    if (!m_tft->safeCreateSprite(m_sprKeypad, KEYPAD_W, KEYPAD_H))
    {
        LOG_ERROR("Keypad-Sprite konnte nicht erstellt werden!");
        return false;
    }

    return true;
}

/**
 * @brief Erstellt die Keypad-Buttons
 */
void LockScreen::initKeys()
{
    const int keypadY = KEYPAD_Y - static_cast<int>(m_keypadOffsetUp);

    for (int i = 0; i < KEY_COUNT; ++i)
    {
        int col = i % KEY_COLS;
        int row = i / KEY_COLS;

        int cx = KEYPAD_X + col * (KEY_W + KEY_GAP) + KEY_W / 2;
        int cy = keypadY + row * (KEY_H + KEY_GAP) + KEY_H / 2;

        std::unique_ptr<Button> btn(new Button());

        btn->initButton(
            m_tft,
            cx,
            cy,
            KEY_W,
            KEY_H,
            KEY_RADIUS,
            C_KEY_BORDER,
            C_KEY_FILL,
            keyTextColor(i),
            KEY_LABELS[i],
            nullptr);

        btn->setFont(ComfortaaBold30);

        m_keys[i] = std::move(btn);
    }
}

// ===================================================================
// Update
// ===================================================================

/**
 * @brief Führt pro Frame eine nicht blockierende Aktualisierung aus.
 */
bool LockScreen::update()
{
    if (!m_tft)
        return false;

    // Externe Entsperrung (z. B. korrekter Code im Webinterface) hat Vorrang
    if (APP_STATE.consumeDisplayUnlockRequest())
    {
        LOG_SUCCESS("Sperrbildschirm extern entsperrt");
        notifyDisplayLockChanged(false);
        return true;
    }

    updateButtonPulse();

    if (m_buttonPulseActive)
        return false;

    int32_t tx, ty;
    bool touched = m_tft->getTouch(&tx, &ty);

    // Jede Berührung gilt als Aktivität und setzt den Idle-Timer zurück
    if (touched)
        m_lastActivityMs = millis();

    // Idle-Timeout: Keypad nach längerer Inaktivität wieder ausblenden
    if (m_keypadVisible && m_idleTimeoutMs > 0 && !touched && !m_touchActive)
    {
        if ((millis() - m_lastActivityMs) >= m_idleTimeoutMs)
        {
            LOG_INFO_F("Keypad nach %lu ms Inaktivität ausgeblendet", m_idleTimeoutMs);
            hideKeypad();
            return false;
        }
    }

    // Solange das Keypad nicht sichtbar ist, dient der erste Touch nur zum Einblenden und wird nicht als Tastendruck verarbeitet. Erst nach Loslassen ist der nächste Touch ein echter Tastendruck.
    if (!m_keypadVisible)
    {
        if (touched && !m_touchActive)
        {
            m_touchActive = true;
            m_keypadVisible = true;
            m_pressedKey = -1;
            drawKeypad();
        }
        else if (!touched && m_touchActive)
        {
            m_touchActive = false;
        }
        return false;
    }

    if (touched)
    {
        if (!m_touchActive)
        {
            m_touchActive = true;
            m_pressedKey = -1;

            for (int i = 0; i < KEY_COUNT; ++i)
            {
                if (m_keys[i] && m_keys[i]->contains(tx, ty))
                {
                    m_pressedKey = i;
                    drawKeypadPressed(i);
                    break;
                }
            }
        }
    }
    else if (m_touchActive)
    {
        m_touchActive = false;

        if (m_pressedKey >= 0)
        {
            drawKeypad();

            if (handleKeyPress(m_pressedKey))
            {
                LOG_SUCCESS("Sperrbildschirm entsperrt");
                m_pressedKey = -1;
                notifyDisplayLockChanged(false);
                return true;
            }

            m_pressedKey = -1;
        }
    }

    return false;
}

/**
 * @brief Aktualisiert den Button-Rahmen-Puls
 */
void LockScreen::updateButtonPulse()
{
    if (!m_buttonPulseActive)
        return;

    const unsigned long now = millis();
    const unsigned long phaseDuration = (m_buttonPulsePhase % 2 == 0) ? BUTTON_PULSE_ON_MS : BUTTON_PULSE_OFF_MS;

    if (now - m_buttonPulsePhaseStart < phaseDuration)
        return;

    m_buttonPulsePhase++;
    m_buttonPulsePhaseStart = now;

    if (m_buttonPulsePhase >= BUTTON_PULSE_PHASE_COUNT)
    {
        m_buttonPulseActive = false;
        m_buttonPulsePhase = 0;
        drawKeypad();
        return;
    }

    if (m_buttonPulsePhase % 2 == 0)
        drawKeypadFrames(m_buttonPulseColor);
    else
        drawKeypad();
}

// ===================================================================
// Zeichenfunktionen
// ===================================================================

namespace
{
    bool s_bootImageOnScreen = false; ///< true, solange das beim Start gezeigte Bild unverändert auf dem Display steht
}

/**
 * @brief Zeichnet den gesamten Sperrbildschirm
 */
void LockScreen::drawScreen()
{
    // Beim Kaltstart steht das Startbild schon auf dem Schirm und ist Bild für Bild dasselbe, das drawLogo zeichnet. Ohne diese Abkürzung würde es gelöscht und sofort wieder aufgebaut, was als schwarzes Aufblitzen sichtbar ist. Zwischen Startbild und diesem Aufruf zeichnet nichts anderes auf das Display.
    if (s_bootImageOnScreen && !m_keypadVisible)
    {
        s_bootImageOnScreen = false;
        return;
    }

    s_bootImageOnScreen = false;
    m_tft->clear(TFT_BLACK);
    drawLogo();

    if (m_keypadVisible)
        drawKeypad();
}

/**
 * @brief Zeichnet das Logo
 */
void LockScreen::drawLogo()
{
    m_tft->pushImage(LOGO_X, LOGO_Y, LOGO_W, LOGO_H, logo_450);
    m_tft->pushImage(MADEIN_X, MADEIN_Y, MADEIN_W, MADEIN_H, made_in_germany);
}

/**
 * @brief Zeichnet das komplette Keypad
 */
void LockScreen::drawKeypad()
{
    renderKeypad(-1, false, C_KEY_BORDER);
}

/**
 * @brief Zeichnet das Keypad mit gedrückter Taste
 */
void LockScreen::drawKeypadPressed(int8_t pressedKey)
{
    renderKeypad(pressedKey, false, C_KEY_BORDER);
}

/**
 * @brief Zeichnet die Rahmen aller Keypad-Tasten
 */
void LockScreen::drawKeypadFrames(uint16_t color)
{
    renderKeypad(-1, true, color);
}

/**
 * @brief Zeichnet das Keypad in die Keypad-Zeichenfläche und überträgt es aufs Display
 */
void LockScreen::renderKeypad(int8_t pressedKey, bool pulseFrames, uint16_t pulseColor)
{
    if (!m_sprKeypad || !m_tft)
        return;

    m_sprKeypad->fillSprite(TFT_BLACK);
    m_sprKeypad->loadFont(ComfortaaBold30);
    m_sprKeypad->setTextDatum(MC_DATUM);

    for (int i = 0; i < KEY_COUNT; ++i)
    {
        const int col = i % KEY_COLS;
        const int row = i / KEY_COLS;
        const int x = col * (KEY_W + KEY_GAP);
        const int y = row * (KEY_H + KEY_GAP);
        const int cx = x + KEY_W / 2;
        const int cy = y + KEY_H / 2;
        const bool isPressed = (i == pressedKey);
        const uint16_t fillColor = isPressed ? C_KEY_FILL_PRESSED : C_KEY_FILL;
        const uint16_t textColor = keyTextColor(i);
        const uint16_t borderColor = pulseFrames ? pulseColor : C_KEY_BORDER;

        m_sprKeypad->fillRoundRect(x, y, KEY_W, KEY_H, KEY_RADIUS, fillColor);
        m_sprKeypad->drawRoundRect(x, y, KEY_W, KEY_H, KEY_RADIUS, borderColor);
        m_sprKeypad->setTextColor(textColor, fillColor);
        m_sprKeypad->drawString(KEY_LABELS[i], cx, cy);

        if (pulseFrames)
        {
            for (int frame = 1; frame < BUTTON_PULSE_FRAME_WIDTH; ++frame)
                m_sprKeypad->drawRoundRect(x + frame, y + frame, KEY_W - frame * 2, KEY_H - frame * 2, KEY_RADIUS, pulseColor);
        }
    }

    m_sprKeypad->unloadFont();

    const int keypadY = KEYPAD_Y - static_cast<int>(m_keypadOffsetUp);
    m_sprKeypad->pushSprite(KEYPAD_X, keypadY);
}

// ===================================================================
// Eingabe-Logik
// ===================================================================

/**
 * @brief Setzt die Eingabe zurück
 */
void LockScreen::clearInput()
{
    memset(m_input, 0, sizeof(m_input));
    m_inputLength = 0;
}

/**
 * @brief Blendet das Keypad aus und stellt den reinen Logo-Screen wieder her
 */
void LockScreen::hideKeypad()
{
    cancelButtonPulse();
    clearInput();
    m_keypadVisible = false;
    m_pressedKey = -1;
    drawScreen();
}

/**
 * @brief Beendet den Button-Rahmen-Puls
 */
void LockScreen::cancelButtonPulse()
{
    m_buttonPulseActive = false;
    m_buttonPulsePhase = 0;
    m_buttonPulsePhaseStart = 0;
    m_buttonPulseColor = 0;
}

/**
 * @brief Startet den Button-Rahmen-Puls
 */
void LockScreen::startButtonPulse(uint16_t color)
{
    m_buttonPulseColor = color;
    m_buttonPulseActive = true;
    m_buttonPulsePhase = 0;
    m_buttonPulsePhaseStart = millis();
    drawKeypadFrames(m_buttonPulseColor);
}

/**
 * @brief Verarbeitet einen Tastendruck
 */
bool LockScreen::handleKeyPress(int8_t keyIndex)
{
    if (keyIndex < 0 || keyIndex >= KEY_COUNT)
        return false;

    int mapping = KEY_DIGIT_MAP[keyIndex];

    if (mapping == KEY_CLEAR)
    {
        clearInput();
        startButtonPulse(C_CLEAR_PULSE_BORDER);
        return false;
    }

    if (mapping == KEY_OK)
    {
        if (validatePin())
            return true;

        clearInput();
        startButtonPulse(C_ERROR_PULSE_BORDER);
        return false;
    }

    if (m_inputLength < MAX_INPUT_LENGTH)
    {
        m_input[m_inputLength] = static_cast<char>('0' + mapping);
        m_inputLength++;
        m_input[m_inputLength] = '\0';
    }

    return false;
}

/**
 * @brief Vergleicht die Eingabe mit der gespeicherten PIN
 */
bool LockScreen::validatePin() const
{
    const char *storedPin = APP_STATE.getLockPin();

    if (!storedPin)
        return false;

    return strcmp(m_input, storedPin) == 0;
}

/**
 * @brief Zeichnet das Logo mittig auf das Display
 */
void LockScreen::drawBootLogo(LGFX &gfx)
{
    gfx.pushImage(LOGO_X, LOGO_Y, LOGO_W, LOGO_H, logo_450);
    gfx.pushImage(MADEIN_X, MADEIN_Y, MADEIN_W, MADEIN_H, made_in_germany);
    s_bootImageOnScreen = true;
}
