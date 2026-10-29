#include "SleepTimer.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "Logger.h"
#include "StringUtils.h"
#include "Theme.h"

#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

/**
 * @brief Konstruktor
 */
SleepTimer::SleepTimer(LGFX *gfx) : m_tft(gfx) {}

/**
 * @brief Destruktor
 */
SleepTimer::~SleepTimer()
{
    m_backupValid = false;

    if (m_screenBuffer != nullptr)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
    }
}

/**
 * @brief Definiert Position und Größe des Dialogs
 */
void SleepTimer::setBounds(int x, int y, int w, int h)
{
    dialogX = x;
    dialogY = y;
    dialogW = w;
    dialogH = h;
}

/**
 * @brief Setzt den Dialog-Titel
 */
void SleepTimer::setTitle(const char *title)
{
    titleText = title ? title : "AUSSCHALTEN";
}

/**
 * @brief Setzt den Beschreibungstext
 */
void SleepTimer::setMessage(const char *message)
{
    messageText = message ? message : "";
}

/**
 * @brief Setzt das Label des Abbruch-Buttons
 */
void SleepTimer::setCancelLabel(const char *label)
{
    if (!label)
        return;
    stringutils::safeCopy(cancelLabel, label);
}

/**
 * @brief Setzt das Label des Stopp-Buttons (sichtbar bei laufendem Sleep-Countdown)
 */
void SleepTimer::setStopLabel(const char *label)
{
    if (!label)
        return;
    stringutils::safeCopy(stopLabel, label);
}

/**
 * @brief Konfiguriert die verfügbaren Timer-Optionen
 */
void SleepTimer::setOptions(const char *labels[], const int values[], int count)
{
    // Defensive Prüfung: Ungültige Eingaben abfangen
    if (!labels || !values || count <= 0)
    {
        buttonCount = 0;
        return;
    }

    // Auf Maximum begrenzen
    buttonCount = (count > MAX_TIMER_OPTIONS) ? MAX_TIMER_OPTIONS : count;

    for (int i = 0; i < buttonCount; ++i)
    {
        buttonLabels[i] = labels[i] ? labels[i] : "";  // Null-Schutz für einzelne Labels
        buttonValues[i] = values[i];
    }
}

/**
 * @brief Registriert Callback für Timer-Auswahl
 */
void SleepTimer::setCallback(std::function<void(int)> cb)
{
    callback = cb;
}

/**
 * @brief Aktiviert automatisches Timeout für den Dialog
 */
void SleepTimer::setTimeout(uint32_t millis)
{
    timeoutMs = millis;
}

/**
 * @brief Startet einen Sleep-Countdown
 */
void SleepTimer::startSleepCountdown(int minutes, std::function<void()> onTimeout)
{
    // Ungültige Werte abfangen
    if (minutes <= 0)
    {
        cancelSleepTimer();
        return;
    }

    sleepTimerStart = millis();
    sleepTimerDuration = static_cast<uint32_t>(minutes) * 60000UL;
    sleepTimerActive = true;
    timeoutCallback = onTimeout;
}

/**
 * @brief Zeigt den Dialog an
 */
void SleepTimer::start()
{
    // Zustand initialisieren
    state = Active;
    touchState = TouchState();
    dialogStartTime = millis();

    // Dialog anzeigen
    backup();
    draw();
}

/**
 * @brief Prüft ob der Dialog aktiv ist
 */
bool SleepTimer::isActive() const
{
    return state == Active;
}

/**
 * @brief Prüft ob ein Sleep-Timer läuft
 */
bool SleepTimer::isTimerActive() const
{
    return sleepTimerActive;
}

/**
 * @brief Gibt die verbleibende Zeit zurück
 */
void SleepTimer::getRemainingTime(int &minutes, int &seconds) const
{
    if (!sleepTimerActive)
    {
        minutes = -1;
        seconds = -1;
        return;
    }

    uint32_t elapsed = millis() - sleepTimerStart;
    if (elapsed >= sleepTimerDuration)
    {
        minutes = 0;
        seconds = 0;
        return;
    }

    uint32_t remaining = (sleepTimerDuration - elapsed) / 1000;
    minutes = remaining / 60;
    seconds = remaining % 60;
}

/**
 * @brief Aktualisiert Dialog und Timer
 */
void SleepTimer::update()
{
    // Sleep-Timer überwachen
    checkSleepTimer();

    // Dialog-Updates nur, wenn der Dialog aktiv ist
    if (state == Active)
    {
        processTouchInput();
        checkDialogTimeout();
    }
}

/**
 * @brief Stoppt den aktiven Sleep-Timer
 */
void SleepTimer::cancelSleepTimer()
{
    if (sleepTimerActive)
    {
        sleepTimerActive = false;
        timeoutCallback = nullptr;
    }
}

/**
 * @brief Sichert den Bildschirminhalt
 */
void SleepTimer::backup()
{
    size_t size = dialogW * dialogH * sizeof(lgfx::rgb565_t);

    // Buffer immer neu anlegen, denn die Größe könnte sich geändert haben
    if (m_screenBuffer != nullptr)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
    }

    m_screenBuffer = static_cast<lgfx::rgb565_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (m_screenBuffer != nullptr)
    {
        m_tft->readRect(dialogX, dialogY, dialogW, dialogH, m_screenBuffer);
        m_backupValid = true;
    }
    else
    {
        m_backupValid = false;
        LOG_ERROR("SleepTimer: Konnte Backup-Buffer nicht anlegen");
    }
}

/**
 * @brief Stellt Bildschirminhalt wieder her
 */
void SleepTimer::restore()
{
    if (m_backupValid && m_screenBuffer != nullptr)
    {
        m_tft->startWrite();
        m_tft->pushImage(dialogX, dialogY, dialogW, dialogH, m_screenBuffer);
        m_tft->endWrite();
        m_backupValid = false;
        return;
    }

    LOG_WARNING("Restore abgebrochen: Backup ungültig oder Buffer null.");

    // Fallback: Dialogfläche löschen (besser als stehenlassen)
    m_tft->startWrite();
    m_tft->fillRect(dialogX, dialogY, dialogW, dialogH, UIConfig::COLOR_BACKGROUND);
    m_tft->endWrite();
}

/**
 * @brief Zeichnet den kompletten Dialog
 */
void SleepTimer::draw()
{
    m_tft->startWrite();

    drawFrame();
    drawHeader();
    drawSeparator();
    drawMessage();
    drawTimerButtons();
    drawControlButtons();

    m_tft->endWrite();
}

/**
 * @brief Zeichnet Rahmen und Hintergrund
 */
void SleepTimer::drawFrame()
{
    // äußerer Rahmen
    m_tft->fillSmoothRoundRect(dialogX, dialogY, dialogW, dialogH, UIConfig::BORDER_RADIUS, UIConfig::COLOR_BORDER);
    // innerer Hintergrund
    m_tft->fillSmoothRoundRect(dialogX + 1, dialogY + 1, dialogW - 2, dialogH - 2, UIConfig::BORDER_RADIUS, UIConfig::COLOR_BACKGROUND);
}

/**
 * @brief Zeichnet den Titel
 */
void SleepTimer::drawHeader()
{
    int headerX = dialogX + UIConfig::PADDING;
    int headerY = dialogY + UIConfig::TITLE_Y_OFFSET;

    // Titel
    m_tft->loadFont(ComfortaaBold18);
    m_tft->setTextDatum(TL_DATUM);  // Defensiv: Textausrichtung explizit setzen
    m_tft->setTextColor(Theme::ColorTitle, Theme::ColorMainBackGround);
    m_tft->drawString(titleText, headerX, headerY + 8);
    m_tft->unloadFont();
}

/**
 * @brief Zeichnet die Trennlinie
 */
void SleepTimer::drawSeparator()
{
    m_tft->fillRect(dialogX + UIConfig::PADDING, dialogY + UIConfig::SEPARATOR_Y, dialogW - 2 * UIConfig::PADDING, UIConfig::SEPARATOR_HEIGHT, Theme::ColorDialogSeparator);
}

/**
 * @brief Zeichnet den Beschreibungstext
 */
void SleepTimer::drawMessage()
{
    if (messageText && *messageText)
    {
        m_tft->loadFont(ComfortaaBold14);
        m_tft->setTextDatum(TL_DATUM);  // Defensiv: Textausrichtung explizit setzen
        m_tft->setTextColor(UIConfig::COLOR_TEXT, UIConfig::COLOR_BACKGROUND);
        m_tft->drawString(messageText, dialogX + UIConfig::PADDING, dialogY + UIConfig::MESSAGE_Y);
        m_tft->unloadFont();
    }
}

/**
 * @brief Zeichnet die Timer-Option-Buttons
 */
void SleepTimer::drawTimerButtons()
{
    // Button-Layout berechnen
    int contentWidth = dialogW - 2 * UIConfig::PADDING;
    int btnW = (contentWidth - (UIConfig::MATRIX_COLS - 1) * UIConfig::BUTTON_SPACING) / UIConfig::MATRIX_COLS;

    // Position der Button-Matrix berechnen (mit Padding unten berücksichtigen)
    int controlButtonsHeight = UIConfig::BUTTON_HEIGHT + UIConfig::BUTTON_SPACING;
    int matrixHeight = UIConfig::MATRIX_ROWS * UIConfig::BUTTON_HEIGHT + (UIConfig::MATRIX_ROWS - 1) * UIConfig::BUTTON_SPACING;
    int startY = dialogY + dialogH - UIConfig::PADDING - controlButtonsHeight - matrixHeight - UIConfig::BUTTON_SPACING;

    // Timer-Buttons zeichnen
    for (int i = 0; i < buttonCount && i < 6; ++i)
    {
        int col = i % UIConfig::MATRIX_COLS;
        int row = i / UIConfig::MATRIX_COLS;

        int x = dialogX + UIConfig::PADDING + col * (btnW + UIConfig::BUTTON_SPACING);
        int y = startY + row * (UIConfig::BUTTON_HEIGHT + UIConfig::BUTTON_SPACING);

        buttons[i].initButton(
            m_tft,
            x + btnW / 2,
            y + UIConfig::BUTTON_HEIGHT / 2,
            btnW,
            UIConfig::BUTTON_HEIGHT,
            UIConfig::BORDER_RADIUS,
            UIConfig::COLOR_BUTTON_BORDER,
            UIConfig::COLOR_BACKGROUND,
            UIConfig::COLOR_BUTTON_TEXT,
            buttonLabels[i],
            nullptr);

        buttons[i].setFont(LeagueGothicCond34);
        buttons[i].drawButton();
    }
}

/**
 * @brief Zeichnet die unteren Steuer-Buttons
 */
void SleepTimer::drawControlButtons()
{
    // Layout für untere Buttons
    int contentWidth = dialogW - 2 * UIConfig::PADDING;
    int btnY = dialogY + dialogH - UIConfig::PADDING - UIConfig::BUTTON_HEIGHT;

    if (sleepTimerActive)
    {
        // Zwei Buttons nebeneinander
        int btnW = (contentWidth - UIConfig::BUTTON_SPACING) / 2;

        // Stop-Button (links)
        btnStopTimer.initButton(
            m_tft,
            dialogX + UIConfig::PADDING + btnW / 2,
            btnY + UIConfig::BUTTON_HEIGHT / 2,
            btnW,
            UIConfig::BUTTON_HEIGHT,
            UIConfig::BORDER_RADIUS,
            UIConfig::COLOR_STOP_BORDER,
            UIConfig::COLOR_BACKGROUND,
            UIConfig::COLOR_STOP_TEXT,
            stopLabel,
            nullptr);
        btnStopTimer.setFont(LeagueGothicCond34);
        btnStopTimer.drawButton();

        // Cancel-Button (rechts)
        btnCancel.initButton(
            m_tft,
            dialogX + UIConfig::PADDING + btnW + UIConfig::BUTTON_SPACING + btnW / 2,
            btnY + UIConfig::BUTTON_HEIGHT / 2,
            btnW,
            UIConfig::BUTTON_HEIGHT,
            UIConfig::BORDER_RADIUS,
            UIConfig::COLOR_BUTTON_BORDER,
            UIConfig::COLOR_BACKGROUND,
            UIConfig::COLOR_CANCEL_TEXT,
            cancelLabel,
            nullptr);
    }
    else
    {
        // Nur Cancel-Button (volle Breite)
        btnCancel.initButton(
            m_tft,
            dialogX + dialogW / 2,
            btnY + UIConfig::BUTTON_HEIGHT / 2,
            contentWidth,
            UIConfig::BUTTON_HEIGHT,
            UIConfig::BORDER_RADIUS,
            UIConfig::COLOR_BUTTON_BORDER,
            UIConfig::COLOR_BACKGROUND,
            UIConfig::COLOR_CANCEL_TEXT,
            cancelLabel,
            nullptr);
    }

    btnCancel.setFont(LeagueGothicCond34);
    btnCancel.drawButton();
}

/**
 * @brief Verarbeitet Touch-Eingaben
 */
void SleepTimer::processTouchInput()
{
    lgfx::touch_point_t touch;
    bool currentlyTouching = m_tft->getTouch(&touch);

    if (currentlyTouching)
    {
        int buttonIndex = findButtonAtPosition(touch.x, touch.y);

        if (buttonIndex != touchState.activeButton)
        {
            updateButtonHighlight(buttonIndex);
            touchState.activeButton = buttonIndex;
        }

        touchState.isTouching = true;
    }
    else if (touchState.isTouching)
    {
        // Touch losgelassen - Aktion ausführen
        if (touchState.activeButton >= 0 && touchState.activeButton < buttonCount)
        {
            // Timer-Option ausgewählt
            closeDialog(buttonValues[touchState.activeButton]);
        }
        else if (touchState.activeButton == 98)
        {
            // Timer stoppen
            cancelSleepTimer();
            closeDialog(-2);
        }
        else if (touchState.activeButton == 99)
        {
            // Abbruch
            closeDialog(-1);
        }
        else
        {
            // Keine Auswahl - Buttons zurücksetzen
            updateButtonHighlight(-1);
        }

        touchState = TouchState();
    }
}

/**
 * @brief Prüft Dialog-Timeout
 */
void SleepTimer::checkDialogTimeout()
{
    // Timeout -> Abbruch
    if (timeoutMs > 0 && (millis() - dialogStartTime) > timeoutMs)
    {
        closeDialog(-1);
    }
}

/**
 * @brief Prüft Sleep-Timer und führt ggf. Callback aus
 */
void SleepTimer::checkSleepTimer()
{
    if (sleepTimerActive && (millis() - sleepTimerStart >= sleepTimerDuration))
    {
        sleepTimerActive = false;
        if (timeoutCallback)
        {
            auto cb = timeoutCallback;
            timeoutCallback = nullptr;
            cb();
        }
    }
}

/**
 * @brief Findet Button an Touch-Position
 */
int SleepTimer::findButtonAtPosition(int x, int y)
{
    // Timer-Buttons prüfen
    for (int i = 0; i < buttonCount; ++i)
    {
        if (buttons[i].contains(x, y))
            return i;
    }

    // Control-Buttons prüfen
    if (sleepTimerActive && btnStopTimer.contains(x, y))
        return 98;

    if (btnCancel.contains(x, y))
        return 99;

    return -1;
}

/**
 * @brief Aktualisiert Button-Highlight
 */
void SleepTimer::updateButtonHighlight(int newActiveButton)
{
    int oldActiveButton = touchState.activeButton;

    // Nichts zu tun wenn sich nichts geändert hat
    if (oldActiveButton == newActiveButton)
        return;

    // Vorherigen Button zurücksetzen (falls vorhanden)
    if (oldActiveButton >= 0 && oldActiveButton < buttonCount)
        buttons[oldActiveButton].drawButton(false);
    else if (oldActiveButton == 98 && sleepTimerActive)
        btnStopTimer.drawButton(false);
    else if (oldActiveButton == 99)
        btnCancel.drawButton(false);

    // Neuen Button hervorheben (falls vorhanden)
    if (newActiveButton >= 0 && newActiveButton < buttonCount)
        buttons[newActiveButton].drawButton(true);
    else if (newActiveButton == 98 && sleepTimerActive)
        btnStopTimer.drawButton(true);
    else if (newActiveButton == 99)
        btnCancel.drawButton(true);
}

/**
 * @brief Beendet Dialog
 */
void SleepTimer::closeDialog(int result)
{
    restore();
    state = Inactive;

    // Callback ausführen
    if (callback)
    {
        auto cb = callback;
        callback = nullptr;
        cb(result);
    }
}
