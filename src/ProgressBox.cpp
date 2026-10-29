#include "ProgressBox.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "StringUtils.h"
#include "Theme.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold18.h"

/**
 * @brief Konstruktor
 */
ProgressBox::ProgressBox(LGFX &gfx) : m_tft(gfx) {}

/**
 * @brief Destruktor
 */
ProgressBox::~ProgressBox()
{
    // Wenn noch aktiv, Screen wiederherstellen (ohne Callback)
    if (m_state == State::Active)
    {
        restore();
    }

    // Font entladen und Sprite freigeben
    if (m_sprite)
    {
        m_sprite->unloadFont();
    }
    m_tft.safeDeleteSprite(m_sprite);

    if (m_backupBuffer)
    {
        heap_caps_free(m_backupBuffer);
        m_backupBuffer = nullptr;
        m_backupBufferSize = 0;
    }
}

/**
 * @brief Position und Größe festlegen
 */
void ProgressBox::setBounds(int x, int y, int w, int h)
{
    m_boxX = x;
    m_boxY = y;
    // Mindestgröße 1x1 sicherstellen um Sprite-Erstellung zu ermöglichen
    m_boxW = (w < 1) ? 1 : w;
    m_boxH = (h < 1) ? 1 : h;
    m_autoCenter = false;
}

/**
 * @brief Automatisches Zentrieren aktivieren
 */
void ProgressBox::setAutoCenter(bool center)
{
    m_autoCenter = center;
    if (center)
    {
        // Standard-Größe setzen
        m_boxW = UIConfig::DEFAULT_WIDTH;
        m_boxH = UIConfig::DEFAULT_HEIGHT;
    }
}

/**
 * @brief Titel setzen
 */
void ProgressBox::setTitle(const char *title)
{
    stringutils::safeCopy(m_titleText, title);
}

/**
 * @brief Nachrichtentext setzen
 */
void ProgressBox::setMessage(const char *message)
{
    stringutils::safeCopy(m_messageText, message);
}

/**
 * @brief Segment-Layout (Strich-/Lückenbreite) konfigurieren
 */
void ProgressBox::setBarStyle(int segmentWidth, int gapWidth)
{
    m_segW = (segmentWidth <= 0) ? 1 : segmentWidth;
    m_gapW = (gapWidth < 0) ? 0 : gapWidth;
}

/**
 * @brief Farben der Fortschrittsbalken-Segmente setzen
 */
void ProgressBox::setBarColors(uint16_t active, uint16_t cap, uint16_t bg)
{
    m_barColorActive = active;
    m_barColorCap = cap;
    m_barColorBG = bg;
}

/**
 * @brief Callback setzen
 */
void ProgressBox::setCallback(std::function<void()> cb)
{
    m_callback = cb;
}

/**
 * @brief ProgressBox anzeigen
 */
void ProgressBox::start(const char *message, int progress)
{
    if (message)
    {
        stringutils::safeCopy(m_messageText, message);
    }

    m_currentProgress = constrain(progress, -1, 100);
    m_indeterminate = (m_currentProgress < 0);
    m_animationTime = millis();
    m_animationPos = 0;
    m_needsRedraw = false;

    // Position berechnen
    calculateBounds();

    // Hintergrund sichern
    backup();

    // Sprite erstellen
    m_tft.safeCreateSprite(m_sprite, m_boxW, m_boxH);

    // Abbruch wenn Sprite-Erstellung fehlgeschlagen (Speichermangel)
    if (!m_sprite)
    {
        log_e("ProgressBox: Sprite-Erstellung fehlgeschlagen - nicht genug Speicher");
        return;
    }

    // Initial zeichnen
    drawBox();

    // Sprite auf Display
    m_sprite->pushSprite(m_boxX, m_boxY);

    m_state = State::Active;
}

/**
 * @brief Fortschritt aktualisieren
 */
void ProgressBox::updateProgress(int progress)
{
    if (m_state != State::Active)
        return;

    int newProgress = constrain(progress, -1, 100);
    bool wasIndeterminate = m_indeterminate;
    m_indeterminate = (newProgress < 0);

    // Nur neu zeichnen wenn sich was geändert hat
    if (newProgress != m_currentProgress || wasIndeterminate != m_indeterminate)
    {
        m_currentProgress = newProgress;

        if (m_sprite)
        {
            // Fortschrittsbereich neu zeichnen
            drawProgressBar();

            // Nur Balkenbereich übertragen (Performance-Optimierung)
            const int barX = m_boxX + UIConfig::PADDING;
            const int barY = m_boxY + m_boxH - UIConfig::PADDING - UIConfig::PROGRESS_HEIGHT - 5;
            const int barW = m_boxW - 2 * UIConfig::PADDING;
            const int barH = UIConfig::PROGRESS_HEIGHT;

            m_tft.setClipRect(barX, barY, barW, barH);
            m_sprite->pushSprite(m_boxX, m_boxY);
            m_tft.clearClipRect();
        }
    }
}

/**
 * @brief Nachrichtentext aktualisieren
 */
void ProgressBox::updateMessage(const char *message)
{
    if (m_state != State::Active)
        return;
    if (!message)
        return;

    // Nur aktualisieren wenn Text anders ist
    if (strcmp(m_messageText, message) != 0)
    {
        stringutils::safeCopy(m_messageText, message);

        if (m_sprite)
        {
            // Nur Textbereich neu zeichnen
            drawMessage();
            m_sprite->pushSprite(m_boxX, m_boxY);
        }
    }
}

/**
 * @brief Fortschritt und Text gleichzeitig aktualisieren
 */
void ProgressBox::update(const char *message, int progress)
{
    if (m_state != State::Active)
        return;

    bool needRedraw = false;

    // Message prüfen
    if (message && strcmp(m_messageText, message) != 0)
    {
        stringutils::safeCopy(m_messageText, message);
        needRedraw = true;
    }

    // Progress prüfen
    int newProgress = constrain(progress, -1, 100);
    bool wasIndeterminate = m_indeterminate;
    m_indeterminate = (newProgress < 0);

    if (newProgress != m_currentProgress || wasIndeterminate != m_indeterminate)
    {
        m_currentProgress = newProgress;
        needRedraw = true;
    }

    // Einmal neu zeichnen wenn nötig
    if (needRedraw && m_sprite)
    {
        drawBox();
        m_sprite->pushSprite(m_boxX, m_boxY);
    }
}

/**
 * @brief Animation für unbestimmten Fortschritt
 */
void ProgressBox::updateAnimation()
{
    if (m_state != State::Active || !m_indeterminate)
        return;

    uint32_t now = millis();
    if (now - m_animationTime > 50)
    { // 20 FPS
        m_animationTime = now;
        m_animationPos = (m_animationPos + 5) % 100;

        if (m_sprite)
        {
            drawIndeterminateBar();

            // Nur Balkenbereich übertragen (Performance-Optimierung)
            const int barX = m_boxX + UIConfig::PADDING;
            const int barY = m_boxY + m_boxH - UIConfig::PADDING - UIConfig::PROGRESS_HEIGHT - 5;
            const int barW = m_boxW - 2 * UIConfig::PADDING;
            const int barH = UIConfig::PROGRESS_HEIGHT;

            m_tft.setClipRect(barX, barY, barW, barH);
            m_sprite->pushSprite(m_boxX, m_boxY);
            m_tft.clearClipRect();
        }
    }
}

/**
 * @brief ProgressBox schließen
 */
void ProgressBox::finish()
{
    if (m_state != State::Active)
        return;

    // Hintergrund wiederherstellen
    restore();

    // Font entladen und Sprite freigeben
    if (m_sprite)
    {
        m_sprite->unloadFont();
    }
    m_tft.safeDeleteSprite(m_sprite);

    m_state = State::Finished;

    // Callback aufrufen
    if (m_callback)
    {
        auto cb = m_callback;
        m_callback = nullptr;
        cb();
    }

    // Nur auf Hidden setzen wenn Callback nicht erneut start() aufgerufen hat
    if (m_state == State::Finished)
    {
        m_state = State::Hidden;
    }
}

/**
 * @brief Prüft ob aktiv
 */
bool ProgressBox::isActive() const
{
    return m_state == State::Active;
}

/**
 * @brief Prüft, ob der Aufrufer die Benutzeroberfläche neu zeichnen muss.
 */
bool ProgressBox::needsRedraw() const
{
    return m_needsRedraw;
}

/**
 * @brief Position berechnen
 */
void ProgressBox::calculateBounds()
{
    // Dimensionen auf Display-Größe begrenzen
    m_boxW = constrain(m_boxW, 1, m_tft.width());
    m_boxH = constrain(m_boxH, 1, m_tft.height());

    if (m_autoCenter)
    {
        m_boxX = (m_tft.width() - m_boxW) / 2;
        m_boxY = (m_tft.height() - m_boxH) / 2;
    }

    // Sicherstellen dass Box im Display ist
    m_boxX = constrain(m_boxX, 0, m_tft.width() - m_boxW);
    m_boxY = constrain(m_boxY, 0, m_tft.height() - m_boxH);
}

/**
 * @brief Hintergrund sichern
 */
void ProgressBox::backup()
{
    m_backupValid = false;

    if (m_boxW <= 0 || m_boxH <= 0)
        return;

    const size_t size = static_cast<size_t>(m_boxW) * static_cast<size_t>(m_boxH) * sizeof(lgfx::rgb565_t);

    // Buffer allokieren wenn nötig
    if (!m_backupBuffer || size > m_backupBufferSize)
    {
        if (m_backupBuffer)
        {
            heap_caps_free(m_backupBuffer);
            m_backupBuffer = nullptr;
            m_backupBufferSize = 0;
        }

        m_backupBuffer = static_cast<lgfx::rgb565_t *>(
            heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

        if (m_backupBuffer)
        {
            m_backupBufferSize = size;
        }
    }

    // Bildschirmbereich sichern
    if (m_backupBuffer)
    {
        m_tft.readRect(m_boxX, m_boxY, m_boxW, m_boxH, m_backupBuffer);
        m_backupValid = true;
    }
}

/**
 * @brief Hintergrund wiederherstellen
 */
void ProgressBox::restore()
{
    if (m_backupValid && m_backupBuffer)
    {
        m_tft.pushImage(m_boxX, m_boxY, m_boxW, m_boxH, m_backupBuffer);
        m_backupValid = false;
        m_needsRedraw = false;
    }
    else
    {
        // Kein gültiges Backup vorhanden - Bereich clearen statt Artefakte hinterlassen
        m_tft.fillRect(m_boxX, m_boxY, m_boxW, m_boxH, Theme::ColorMainBackGround);
        m_needsRedraw = true;
        log_w("ProgressBox::restore(): Kein Backup, Bereich gecleant. Caller muss UI neu zeichnen.");
    }
}

/**
 * @brief Komplette Box zeichnen
 */
void ProgressBox::drawBox()
{
    if (!m_sprite)
        return;

    drawFrame();
    drawTitle();
    drawMessage();
    drawProgressBar();
}

/**
 * @brief Rahmen zeichnen
 */
void ProgressBox::drawFrame()
{
    if (!m_sprite)
        return;

    m_sprite->fillSmoothRoundRect(0, 0, m_boxW, m_boxH, UIConfig::BORDER_RADIUS, Theme::ColorDialogBorder);
    m_sprite->fillSmoothRoundRect(1, 1, m_boxW - 2, m_boxH - 2, UIConfig::BORDER_RADIUS, Theme::ColorMainBackGround);
}

/**
 * @brief Titel zeichnen
 */
void ProgressBox::drawTitle()
{
    if (!m_sprite)
        return;

    const int headerX = UIConfig::PADDING;
    const int headerY = UIConfig::TITLE_Y_OFFSET;

    m_sprite->loadFont(ComfortaaBold18);
    m_sprite->setTextColor(Theme::ColorTitle, Theme::ColorMainBackGround);
    m_sprite->drawString(m_titleText, headerX, headerY + 8);
    // Font bleibt geladen - wird erst am Ende entladen (Performance)

    // Separator-Linie
    m_sprite->fillRect(UIConfig::PADDING, UIConfig::SEPARATOR_Y, m_boxW - 2 * UIConfig::PADDING, 2, UIConfig::COLOR_SEPARATOR);
}

/**
 * @brief Nachricht zeichnen
 */
void ProgressBox::drawMessage()
{
    if (!m_sprite)
        return;

    m_sprite->loadFont(ComfortaaBold14);
    m_sprite->setTextColor(UIConfig::COLOR_TEXT, Theme::ColorMainBackGround);
    drawMultilineText(UIConfig::PADDING, UIConfig::TEXT_Y_START, m_boxW - 2 * UIConfig::PADDING, m_messageText);
    // Font bleibt geladen - wird erst am Ende entladen (Performance)
}

/**
 * @brief Mehrzeiligen Text mit Umbruch zeichnen
 */
void ProgressBox::drawMultilineText(int x, int y, int maxWidth, const char *text)
{
    if (!text || !*text)
        return;

    char lineBuffer[256];
    const char *currentPos = text;
    int yOffset = 0;
    const int lineHeight = m_sprite->fontHeight() + UIConfig::LINE_HEIGHT_FACTOR;

    while (*currentPos)
    {
        int lineLen = 0;
        int lastSpace = -1;
        const char *lineStart = currentPos;

        while (*currentPos && *currentPos != '\n' && lineLen < static_cast<int>(sizeof(lineBuffer)) - 1)
        {
            if (*currentPos == ' ')
                lastSpace = lineLen;
            lineBuffer[lineLen++] = *currentPos++;
            lineBuffer[lineLen] = '\0';

            if (m_sprite->textWidth(lineBuffer) > maxWidth)
            {
                if (lastSpace > 0)
                {
                    lineLen = lastSpace;
                    currentPos = lineStart + lastSpace;
                }
                else
                {
                    // Mindestens 1 Zeichen behalten um Endlosschleife zu vermeiden
                    if (lineLen > 1)
                    {
                        lineLen--;
                        currentPos = lineStart + lineLen;
                    }
                    // else: lineLen == 1 -> Zeichen ist breiter als maxWidth, trotzdem zeichnen und weitermachen
                }
                break;
            }
        }

        if (*currentPos == '\n')
            currentPos++;
        while (*currentPos == ' ')
            currentPos++;

        if (lineLen > 0)
        {
            lineBuffer[lineLen] = '\0';
            while (lineLen > 0 && lineBuffer[lineLen - 1] == ' ')
            {
                lineBuffer[--lineLen] = '\0';
            }
            if (lineLen > 0)
            {
                m_sprite->drawString(lineBuffer, x, y + yOffset);
                yOffset += lineHeight;
            }
        }
    }
}

/**
 * @brief Fortschrittsbalken zeichnen
 */
void ProgressBox::drawProgressBar()
{
    if (!m_sprite)
        return;

    // Geometrie des Balkens
    const int barY = m_boxH - UIConfig::PADDING - UIConfig::PROGRESS_HEIGHT - 5;
    const int barX = UIConfig::PADDING;
    const int barW = m_boxW - 2 * UIConfig::PADDING;
    const int barH = UIConfig::PROGRESS_HEIGHT;

    // Hintergrund/Fläche leeren
    m_sprite->fillRect(barX, barY, barW, barH, m_barColorBG);

    // Indeterminate -> separater Pfad
    if (m_indeterminate)
    {
        drawIndeterminateBar();
        return;
    }

    // Füllbreite nach Prozent
    const int filled = (m_currentProgress <= 0)     ? 0
                       : (m_currentProgress >= 100) ? barW
                                                    : int((uint32_t)m_currentProgress * (uint32_t)barW / 100U);

    // Segment-Muster
    int period = m_segW + m_gapW;
    if (period <= 0)
        period = 1;

    for (int ix = 0; ix < barW; ++ix)
    {
        const bool isGap = (m_gapW > 0) && ((ix % period) >= m_segW);
        uint16_t col = m_barColorBG;
        if (!isGap)
        {
            col = (ix < filled) ? m_barColorActive : m_barColorCap;
        }
        m_sprite->drawFastVLine(barX + ix, barY, barH, col);
    }
}

/**
 * @brief Animierten Balken für unbestimmten Fortschritt zeichnen
 */
void ProgressBox::drawIndeterminateBar()
{
    if (!m_sprite)
        return;

    const int barY = m_boxH - UIConfig::PADDING - UIConfig::PROGRESS_HEIGHT - 5;
    const int barX = UIConfig::PADDING;
    const int barW = m_boxW - 2 * UIConfig::PADDING;
    const int barH = UIConfig::PROGRESS_HEIGHT;

    // Fläche neu löschen
    m_sprite->fillRect(barX, barY, barW, barH, m_barColorBG);

    // Wanderndes Fenster mit etwa 30 % der Gesamtbreite und segmentierter Darstellung
    const int winW = barW * 30 / 100;
    const int winStart = (barW - winW) * m_animationPos / 100;
    const int winEnd = winStart + winW;

    int period = m_segW + m_gapW;
    if (period <= 0)
        period = 1;

    for (int ix = 0; ix < barW; ++ix)
    {
        const bool isGap = (m_gapW > 0) && ((ix % period) >= m_segW);
        if (isGap)
        {
            // Lücken bleiben Hintergrund
            continue;
        }
        const bool inWindow = (ix >= winStart) && (ix < winEnd);
        uint16_t col = inWindow ? m_barColorActive : m_barColorCap;
        m_sprite->drawFastVLine(barX + ix, barY, barH, col);
    }
}
