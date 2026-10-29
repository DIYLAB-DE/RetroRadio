#include "StationPicker.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "Logger.h"
#include "StationManager.h"
#include "StringUtils.h"
#include "Theme.h"

#include "fonts/ComfortaaBold10.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Konstruktor, Destruktor
// ===================================================================

/**
 * @brief Konstruktor
 */
StationPicker::StationPicker(LGFX *gfx) : m_tft(gfx) {}

/**
 * @brief Destruktor
 */
StationPicker::~StationPicker()
{
    if (m_screenBuffer)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
    }
}

// ===================================================================
// API
// ===================================================================

/**
 * @brief Registriert Callback
 */
void StationPicker::setCallback(std::function<void(int)> cb)
{
    m_callback = cb;
}

/**
 * @brief Setzt den Dialog-Titel
 */
void StationPicker::setTitle(const char *title)
{
    m_titleText = title ? title : "SENDER";
}

/**
 * @brief Setzt das Label des Abbrechen-Buttons
 */
void StationPicker::setCancelLabel(const char *label)
{
    m_cancelLabel = label ? label : "Abbruch";
}

/**
 * @brief Setzt Timeout
 */
void StationPicker::setTimeout(uint32_t ms)
{
    m_timeoutMs = ms;
}

/**
 * @brief Startet den Dialog
 */
void StationPicker::start(const Station *stations, int count, int activeStation)
{
    if (!stations || count <= 0)
        return;

    m_stations = stations;
    m_stationCount = count;
    m_activeStation = activeStation;
    m_active = true;
    m_touchedCell = -1;
    m_isTouching = false;
    m_startTime = millis();

    // Grid-Layout berechnen
    m_rows = (m_stationCount + SPLayout::COLS - 1) / SPLayout::COLS;
    if (m_rows > SPLayout::ROWS)
        m_rows = SPLayout::ROWS; // Max 7 Zeilen

    // Verfügbarer Platz für Grid
    const int gridW = SPLayout::DLG_W - 2 * SPLayout::PADDING;
    const int cancelArea = SPLayout::CANCEL_H + SPLayout::CANCEL_PAD;
    const int gridH = SPLayout::DLG_H - SPLayout::GRID_TOP - cancelArea - SPLayout::PADDING;

    m_cellW = gridW / SPLayout::COLS;
    m_cellH = gridH / m_rows;
    m_gridX = SPLayout::DLG_X + SPLayout::PADDING;
    m_gridY = SPLayout::DLG_Y + SPLayout::GRID_TOP;

    backup();
    draw();
}

/**
 * @brief Dialog aktiv?
 */
bool StationPicker::isActive() const
{
    return m_active;
}

/**
 * @brief Führt die zyklische Aktualisierung aus.
 */
void StationPicker::update()
{
    if (!m_active)
        return;

    processTouchInput();
    checkTimeout();
}

// ===================================================================
// Backup / Restore
// ===================================================================

/**
 * @brief Sichert Bildschirminhalt in PSRAM
 */
void StationPicker::backup()
{
    const size_t size = SPLayout::DLG_W * SPLayout::DLG_H * sizeof(lgfx::rgb565_t);

    if (m_screenBuffer)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
    }

    m_screenBuffer = static_cast<lgfx::rgb565_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (m_screenBuffer)
    {
        m_tft->readRect(SPLayout::DLG_X, SPLayout::DLG_Y, SPLayout::DLG_W, SPLayout::DLG_H, m_screenBuffer);
        m_backupValid = true;
    }
    else
    {
        m_backupValid = false;
        LOG_ERROR("StationPicker: Backup-Buffer fehlgeschlagen");
    }
}

/**
 * @brief Stellt Bildschirminhalt wieder her
 */
void StationPicker::restore()
{
    if (m_backupValid && m_screenBuffer)
    {
        m_tft->pushImage(SPLayout::DLG_X, SPLayout::DLG_Y, SPLayout::DLG_W, SPLayout::DLG_H, m_screenBuffer);
        m_backupValid = false;
    }
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet den kompletten Dialog
 */
void StationPicker::draw()
{
    m_tft->startWrite();

    drawFrame();
    drawHeader();
    drawSeparator();
    drawGrid();
    drawCancelButton();

    m_tft->endWrite();
}

/**
 * @brief abgerundeter Rahmen
 */
void StationPicker::drawFrame()
{
    m_tft->fillSmoothRoundRect(SPLayout::DLG_X, SPLayout::DLG_Y, SPLayout::DLG_W, SPLayout::DLG_H, SPLayout::BORDER_RADIUS, SPColors::BORDER);
    m_tft->fillSmoothRoundRect(SPLayout::DLG_X + 1, SPLayout::DLG_Y + 1, SPLayout::DLG_W - 2, SPLayout::DLG_H - 2, SPLayout::BORDER_RADIUS, SPColors::BG);
}

/**
 * @brief Titel
 */
void StationPicker::drawHeader()
{
    m_tft->loadFont(ComfortaaBold18);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->setTextColor(Theme::ColorTitle, SPColors::BG);
    m_tft->drawString(m_titleText, SPLayout::DLG_X + SPLayout::PADDING, SPLayout::DLG_Y + SPLayout::TITLE_Y);
    m_tft->unloadFont();
}

/**
 * @brief Trennlinie
 */
void StationPicker::drawSeparator()
{
    m_tft->fillRect(SPLayout::DLG_X + SPLayout::PADDING, SPLayout::DLG_Y + SPLayout::SEP_Y, SPLayout::DLG_W - 2 * SPLayout::PADDING, SPLayout::SEP_H, Theme::ColorDialogSeparator);
}

/**
 * @brief Zeichnet alle Zellen
 */
void StationPicker::drawGrid()
{
    m_tft->loadFont(ComfortaaBold10);

    for (int i = 0; i < m_stationCount && i < SPLayout::COLS * m_rows; ++i)
        drawCell(i, false);

    m_tft->unloadFont();
}

/**
 * @brief Zeichnet eine einzelne Zelle
 * @note Font (ComfortaaBold10) muss vom Aufrufer geladen sein!
 */
void StationPicker::drawCell(int index, bool highlighted)
{
    if (index < 0 || index >= m_stationCount)
        return;

    const int col = index % SPLayout::COLS;
    const int row = index / SPLayout::COLS;

    // Zellenposition (äußere Kante inkl. Gap)
    const int cellX = m_gridX + col * m_cellW;
    const int cellY = m_gridY + row * m_cellH;

    // Innere Zelle (Gap abziehen)
    constexpr int G = SPLayout::CELL_GAP;
    const int innerX = cellX;
    const int innerY = cellY;
    const int innerW = m_cellW - G;
    const int innerH = m_cellH - G;

    // Zustand bestimmen
    const bool isActive = (index == m_activeStation);
    const bool isPreset = m_stations[index].preset;

    // Farben wählen
    uint16_t bgColor, borderColor, numColor, nameColor;

    if (highlighted)
    {
        bgColor = SPColors::TOUCH_BG;
        borderColor = SPColors::TOUCH_BORDER;
        numColor = SPColors::CELL_NUM;
        nameColor = 0xFFFF;
    }
    else if (isActive)
    {
        bgColor = SPColors::ACTIVE_BG;
        borderColor = SPColors::ACTIVE_BORDER;
        numColor = SPColors::ACTIVE_NUM;
        nameColor = SPColors::ACTIVE_NAME;
    }
    else
    {
        bgColor = SPColors::CELL_BG;
        borderColor = SPColors::CELL_BORDER;
        numColor = SPColors::CELL_NUM;
        nameColor = isPreset ? SPColors::PRESET_NAME : SPColors::CELL_NAME;
    }

    // Zelle zeichnen: Rahmen -> Füllung
    m_tft->fillSmoothRoundRect(innerX, innerY, innerW, innerH, 3, borderColor);
    m_tft->fillSmoothRoundRect(innerX + 1, innerY + 1, innerW - 2, innerH - 2, 3, bgColor);

    // Preset-Marker: kleiner orangefarbener Punkt oben rechts
    if (isPreset)
    {
        const int dotX = innerX + innerW - 7;
        const int dotY = innerY + 5;
        m_tft->fillCircle(dotX, dotY, 2, SPColors::PRESET_DOT);
    }

    // Sendernummer (oben)
    constexpr int TEXT_PAD_X = 5;
    constexpr int NUM_Y_OFFSET = 5;
    char numBuf[4];
    stringutils::safeFormat(numBuf, "%d", index + 1);

    m_tft->setTextDatum(TL_DATUM);
    m_tft->setTextColor(numColor, bgColor);
    m_tft->drawString(numBuf, innerX + TEXT_PAD_X, innerY + NUM_Y_OFFSET);

    // Sendername (unten)
    const int nameY = innerY + innerH / 2 + 2;
    const int maxNameW = innerW - 2 * TEXT_PAD_X;

    char nameBuf[STATION_SHORTNAME_SIZE];
    stringutils::safeCopy(nameBuf, m_stations[index].shortName);

    // UTF-8-sichere Truncation
    while (m_tft->textWidth(nameBuf) > maxNameW && strlen(nameBuf) > 1)
    {
        size_t len = strlen(nameBuf) - 1;
        while (len > 0 && (nameBuf[len] & 0xC0) == 0x80)
            --len;
        nameBuf[len] = '\0';
    }

    m_tft->setTextColor(nameColor, bgColor);
    m_tft->drawString(nameBuf, innerX + TEXT_PAD_X, nameY);
}

/**
 * @brief Abbrechen-Button
 */
void StationPicker::drawCancelButton()
{
    const int btnW = SPLayout::DLG_W - 2 * SPLayout::PADDING;
    const int btnX = SPLayout::DLG_X + SPLayout::PADDING + btnW / 2;
    const int btnY = SPLayout::DLG_Y + SPLayout::DLG_H - SPLayout::PADDING - SPLayout::CANCEL_H / 2;

    m_cancelBtn.initButton(
        m_tft,                   // Display-Objekt
        btnX,                    // X-Mitte des Buttons
        btnY,                    // Y-Mitte des Buttons
        btnW,                    // Button-Breite
        SPLayout::CANCEL_H,      // Button-Höhe
        SPLayout::BORDER_RADIUS, // Eckenradius
        SPColors::CANCEL_BORDER, // Rahmenfarbe
        SPColors::BG,            // Füllfarbe
        SPColors::CANCEL_TEXT,   // Textfarbe
        m_cancelLabel,           // Beschriftung (von setCancelLabel oder Default)
        nullptr                  // Kein Icon
    );

    m_cancelBtn.setFont(LeagueGothicCond34);
    m_cancelBtn.drawButton();
}

// ===================================================================
// Touch-Verarbeitung
// ===================================================================

/**
 * @brief Touch-Handling mit Keep-alive für Timeout
 */
void StationPicker::processTouchInput()
{
    lgfx::touch_point_t touch;
    const bool currentlyTouching = m_tft->getTouch(&touch);

    if (currentlyTouching)
    {
        // Keep-alive: Timeout bei Interaktion zurücksetzen
        m_startTime = millis();

        // Zelle oder Cancel-Button?
        const int cell = cellAtPosition(touch.x, touch.y);
        const bool onCancel = m_cancelBtn.contains(touch.x, touch.y);

        const int newTouched = onCancel ? -2 : cell; // -2 = Cancel, -1 = nichts

        // Highlight aktualisieren (nur bei Änderung)
        if (newTouched != m_touchedCell)
        {
            m_tft->loadFont(ComfortaaBold10);

            // Altes Highlight entfernen
            if (m_touchedCell >= 0)
                drawCell(m_touchedCell, false);
            else if (m_touchedCell == -2)
                m_cancelBtn.drawButton(false);

            // Neues Highlight setzen
            if (newTouched >= 0)
                drawCell(newTouched, true);
            else if (newTouched == -2)
                m_cancelBtn.drawButton(true);

            m_tft->unloadFont();
            m_touchedCell = newTouched;
        }

        m_isTouching = true;
    }
    else if (m_isTouching)
    {
        // Touch losgelassen -> Aktion ausführen
        if (m_touchedCell >= 0)
            finish(m_touchedCell); // Sender ausgewählt
        else if (m_touchedCell == -2)
            finish(-1); // Abbruch
        else
            m_touchedCell = -1; // Kein gültiges Ziel -> nur Highlight zurücksetzen

        m_isTouching = false;
    }
}

/**
 * @brief Berechnet Zellenindex aus Touch-Koordinaten
 */
int StationPicker::cellAtPosition(int tx, int ty) const
{
    if (tx < m_gridX || ty < m_gridY)
        return -1;

    const int col = (tx - m_gridX) / m_cellW;
    const int row = (ty - m_gridY) / m_cellH;

    if (col >= SPLayout::COLS || row >= m_rows)
        return -1;

    const int idx = row * SPLayout::COLS + col;
    return (idx < m_stationCount) ? idx : -1;
}

/**
 * @brief Timeout prüfen
 */
void StationPicker::checkTimeout()
{
    if (m_timeoutMs > 0 && (millis() - m_startTime) > m_timeoutMs)
        finish(-1);
}

/**
 * @brief Dialog beenden und Callback aufrufen
 */
void StationPicker::finish(int result)
{
    restore();
    m_active = false;

    if (m_callback)
    {
        auto cb = m_callback;
        m_callback = nullptr;
        cb(result);
    }
}
