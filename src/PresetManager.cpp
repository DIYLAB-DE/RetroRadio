#include "PresetManager.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "Logger.h"
#include "Theme.h"
#include "StationManager.h"  // Für Station-Definition (Member-Zugriff in start())

#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

/**
 * @brief Konstruktor
 */
PresetManager::PresetManager(LGFX *gfx) : m_tft(gfx) {}

/**
 * @brief Destruktor - gibt Speicher frei
 */
PresetManager::~PresetManager()
{
    if (m_screenBuffer != nullptr)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
    }
}

/**
 * @brief Definiert Position und Größe des Dialogs
 */
void PresetManager::setBounds(int x, int y, int w, int h)
{
    dialogX = x;
    dialogY = y;
    dialogW = w;
    dialogH = h;
}

/**
 * @brief Setzt den Dialog-Titel
 */
void PresetManager::setTitle(const char *title)
{
    titleText = title ? title : "Favoriten";
}

/**
 * @brief Registriert Callback für Auswahlereignisse
 */
void PresetManager::setCallback(std::function<void(int)> cb)
{
    callback = cb;
}

/**
 * @brief Aktiviert automatisches Timeout
 */
void PresetManager::setTimeout(uint32_t millis)
{
    timeoutMs = millis;
}

/**
 * @brief Startet den Dialog mit Bounds- und Preset-Validierung
 */
void PresetManager::start(Station *stations, int count)
{
    // Bounds-Check: Dialog muss gültige Größe haben
    if (dialogW <= 0 || dialogH <= 0)
    {
        LOG_ERROR_F("PresetManager: Ungültige Dialog-Größe (%d x %d)", dialogW, dialogH);
        return;
    }

    // Zustand initialisieren
    state = Active;
    selectedIndex = -1;
    touchState = TouchState();
    presetCount = 0;
    startTime = millis(); // Startzeit für Timeout merken

    // Favoriten sammeln
    for (int i = 0; i < count && presetCount < MAX_PRESETS; ++i)
    {
        if (stations[i].preset)
        {
            presetEntries[presetCount].name = stations[i].name;
            presetEntries[presetCount].stationIndex = i;
            presetCount++;
        }
    }

    // Dialog anzeigen
    backup();
    draw();
}

/**
 * @brief Prüft ob der Dialog aktiv ist
 */
bool PresetManager::isActive() const
{
    return state == Active;
}

/**
 * @brief Gibt den zuletzt ausgewählten Station-Index zurück
 */
int PresetManager::getSelectedIndex() const
{
    return selectedIndex;
}

/**
 * @brief Aktualisiert Dialog-Logik und Touch-Verarbeitung
 */
void PresetManager::update()
{
    if (state != Active)
        return;

    processTouchInput();
    checkTimeout();
}

/**
 * @brief Sichert den Bildschirminhalt
 */
void PresetManager::backup()
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
        LOG_ERROR("PresetManager: Konnte Backup-Buffer nicht anlegen");
    }
}

/**
 * @brief Stellt Bildschirminhalt wieder her
 */
void PresetManager::restore()
{
    if (m_backupValid && m_screenBuffer != nullptr)
    {
        m_tft->pushImage(dialogX, dialogY, dialogW, dialogH, m_screenBuffer);
        m_backupValid = false;
    }
    else
    {
        LOG_WARNING("Restore abgebrochen: Backup ungültig oder Buffer null.");
    }
}

/**
 * @brief Zeichnet den kompletten Dialog
 */
void PresetManager::draw()
{
    m_tft->startWrite();

    drawFrame();
    drawHeader();
    drawSeparator();
    drawPresetButtons();
    drawCancelButton();

    m_tft->endWrite();
}

/**
 * @brief Zeichnet Rahmen und Hintergrund
 */
void PresetManager::drawFrame()
{
    // äußerer Rahmen
    m_tft->fillSmoothRoundRect(dialogX, dialogY, dialogW, dialogH, UIConfig::BORDER_RADIUS, UIConfig::COLOR_BORDER);
    // innerer Hintergrund
    m_tft->fillSmoothRoundRect(dialogX + 1, dialogY + 1, dialogW - 2, dialogH - 2, UIConfig::BORDER_RADIUS, UIConfig::COLOR_BACKGROUND);
}

/**
 * @brief Zeichnet den Titel
 */
void PresetManager::drawHeader()
{
    int headerX = dialogX + UIConfig::PADDING;
    int headerY = dialogY + UIConfig::TITLE_Y_OFFSET;

    // Titel
    m_tft->loadFont(ComfortaaBold18);
    m_tft->setTextColor(Theme::ColorTitle, Theme::ColorMainBackGround);
    m_tft->drawString(titleText, headerX, headerY + 8);
    m_tft->unloadFont();
}

/**
 * @brief Zeichnet die Trennlinie
 */
void PresetManager::drawSeparator()
{
    m_tft->fillRect(dialogX + UIConfig::PADDING, dialogY + UIConfig::SEPARATOR_Y, dialogW - 2 * UIConfig::PADDING, UIConfig::SEPARATOR_HEIGHT, Theme::ColorDialogSeparator);
}

/**
 * @brief Zeichnet Preset-Buttons oder Hinweis bei leerer Liste
 */
void PresetManager::drawPresetButtons()
{
    // Hinweis anzeigen wenn keine Favoriten vorhanden
    if (presetCount == 0)
    {
        int centerX = dialogX + dialogW / 2;
        int centerY = dialogY + UIConfig::CONTENT_START_Y + 80;

        m_tft->loadFont(ComfortaaBold18);
        m_tft->setTextColor(UIConfig::COLOR_BUTTON_TEXT, UIConfig::COLOR_BACKGROUND);
        m_tft->setTextDatum(middle_center);
        m_tft->drawString("Keine Favoriten vorhanden", centerX, centerY);
        m_tft->setTextDatum(top_left);
        m_tft->unloadFont();
        return;
    }

    // Button-Layout berechnen
    int contentWidth = dialogW - 2 * UIConfig::PADDING;
    int columnWidth = (contentWidth - UIConfig::BUTTON_SPACING) / 2;
    int startY = dialogY + UIConfig::CONTENT_START_Y;

    // Buttons erstellen und zeichnen
    for (int i = 0; i < presetCount; ++i)
    {
        int column = i / UIConfig::BUTTONS_PER_COLUMN;
        int row = i % UIConfig::BUTTONS_PER_COLUMN;

        int x = dialogX + UIConfig::PADDING + column * (columnWidth + UIConfig::BUTTON_SPACING);
        int y = startY + row * (UIConfig::BUTTON_HEIGHT + UIConfig::BUTTON_SPACING);

        // Button initialisieren
        presetButtons[i].initButton(
            m_tft,
            x + columnWidth / 2,             // Center X
            y + UIConfig::BUTTON_HEIGHT / 2, // Center Y
            columnWidth,
            UIConfig::BUTTON_HEIGHT,
            UIConfig::BORDER_RADIUS,
            UIConfig::COLOR_BUTTON_BORDER,
            UIConfig::COLOR_BACKGROUND,
            UIConfig::COLOR_BUTTON_TEXT,
            presetEntries[i].name,
            nullptr);

        // Button konfigurieren
        presetButtons[i].setFont(ComfortaaBold18);
        presetButtons[i].setLabelDatum(0, 0, textdatum_t::middle_left);
        presetButtons[i].setIconSize(UIConfig::NUMBER_CIRCLE_SIZE, UIConfig::NUMBER_CIRCLE_SIZE);
        presetButtons[i].setIconPosition(Button::IconPosition::Left);
        presetButtons[i].enableNumberCircle(i + 1);
        presetButtons[i].drawButton();
    }
}

/**
 * @brief Zeichnet den Cancel-Button
 */
void PresetManager::drawCancelButton()
{
    // Cancel-Button mit gleichem Padding wie andere Elemente
    int btnWidth = dialogW - 2 * UIConfig::PADDING;
    int btnX = dialogX + UIConfig::PADDING;
    int btnY = dialogY + dialogH - UIConfig::PADDING - UIConfig::CANCEL_BUTTON_HEIGHT;

    btnCancel.initButton(
        m_tft,
        btnX + btnWidth / 2,                       // Center X
        btnY + UIConfig::CANCEL_BUTTON_HEIGHT / 2, // Center Y
        btnWidth,
        UIConfig::CANCEL_BUTTON_HEIGHT,
        UIConfig::BORDER_RADIUS,
        UIConfig::COLOR_CANCEL_BORDER,
        UIConfig::COLOR_BACKGROUND,
        UIConfig::COLOR_CANCEL_TEXT,
        "abbruch",
        nullptr);

    btnCancel.setFont(LeagueGothicCond34);
    btnCancel.drawButton();
}

/**
 * @brief Prüft Timeout und führt ggf. Abbruch aus
 */
void PresetManager::checkTimeout()
{
    // Timeout -> Abbruch
    if (timeoutMs > 0 && (millis() - startTime) > timeoutMs)
        finish(-1);
}

/**
 * @brief Verarbeitet Touch-Eingaben mit Keep-alive für Timeout
 */
void PresetManager::processTouchInput()
{
    lgfx::touch_point_t touch;
    bool currentlyTouching = m_tft->getTouch(&touch);

    if (currentlyTouching)
    {
        // Keep-alive: Timeout bei Interaktion zurücksetzen
        startTime = millis();

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
        if (touchState.activeButton >= 0 && touchState.activeButton < presetCount)
            finish(presetEntries[touchState.activeButton].stationIndex); // Preset ausgewählt
        else if (touchState.activeButton == 99)
            finish(-1); // Abbruch
        else
            updateButtonHighlight(-1); // Keine Auswahl - Buttons zurücksetzen

        touchState = TouchState();
    }
}

/**
 * @brief Findet Button an Touch-Position
 */
int PresetManager::findButtonAtPosition(int x, int y)
{
    // Preset-Buttons prüfen
    for (int i = 0; i < presetCount; ++i)
    {
        if (presetButtons[i].contains(x, y))
            return i;
    }

    // Cancel-Button prüfen
    if (btnCancel.contains(x, y))
        return 99;

    return -1;
}

/**
 * @brief Aktualisiert Button-Highlight effizient
 */
void PresetManager::updateButtonHighlight(int newActiveButton)
{
    // Alten aktiven Button merken (noch nicht überschrieben in touchState)
    int oldActiveButton = touchState.activeButton;

    // Nur den alten Button auf normal zurücksetzen (wenn gültig und unterschiedlich)
    if (oldActiveButton != newActiveButton)
    {
        if (oldActiveButton >= 0 && oldActiveButton < presetCount)
        {
            presetButtons[oldActiveButton].drawButton(false);
        }
        else if (oldActiveButton == 99)
        {
            btnCancel.drawButton(false);
        }
    }

    // Nur den neuen Button auf highlighted setzen (wenn gültig)
    if (newActiveButton >= 0 && newActiveButton < presetCount)
    {
        presetButtons[newActiveButton].drawButton(true);
    }
    else if (newActiveButton == 99)
    {
        btnCancel.drawButton(true);
    }
}

/**
 * @brief Beendet Dialog mit Ergebnis
 */
void PresetManager::finish(int result)
{
    selectedIndex = result;
    restore();
    state = Inactive;

    // Callback ausführen
    if (callback)
    {
        auto cb = callback;
        callback = nullptr;
        cb(selectedIndex);
    }
}
