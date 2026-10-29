#include "MessageBox.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include "Logger.h"
#include "StringUtils.h"
#include "Theme.h"

// Fonts
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

/**
 * @brief Konstruktor
 */
MessageBox::MessageBox(LGFX *gfx) : m_tft(gfx) {}

/**
 * @brief Destruktor
 */
MessageBox::~MessageBox()
{
    if (m_screenBuffer != nullptr)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
        m_bufferSizeBytes = 0;
    }
}

/**
 * @brief Position und Größe der MessageBox festlegen
 */
void MessageBox::setBounds(int x, int y, int w, int h)
{
    m_msgX = x;
    m_msgY = y;
    m_msgW = w;
    m_msgH = h;
    m_autoBoundsCalculated = false;  // Manuelles Override, kein AutoBounds
}

/**
 * @brief Berechnet die Anzahl der Zeilen für einen Text bei gegebener Breite
 */
int MessageBox::calculateTextLineCount(int maxWidth, const char *text)
{
    if (!text || !*text)
        return 0;

    m_tft->loadFont(ComfortaaBold14);

    char lineBuffer[256] = {0};  // Initialisiert für sicheren snprintf-Zugriff
    char wordBuffer[64];
    int lineLen = 0;
    int wordLen = 0;
    int lineCount = 0;
    const char *pos = text;

    while (*pos)
    {
        // Expliziter Zeilenumbruch - auch Leerzeilen müssen gezählt werden, damit aufeinanderfolgende '\n' als Absatz-Trennung wirken.
        if (*pos == '\n')
        {
            if (lineLen > 0)
            {
                // Aktuelle Zeile enthält Text: diese zählen, Buffer leeren.
                lineCount++;
                lineLen = 0;
            }
            else
            {
                // Leere Zeile (z. B. zweites '\n' in Folge): als Leerzeile zählen.
                lineCount++;
            }
            pos++;
            continue;
        }

        // Wort sammeln (bis Space oder Ende)
        wordLen = 0;
        while (*pos && *pos != ' ' && *pos != '\n' && wordLen < static_cast<int>(sizeof(wordBuffer)) - 1)
        {
            wordBuffer[wordLen++] = *pos++;
        }
        wordBuffer[wordLen] = '\0';

        // Spaces überspringen
        while (*pos == ' ')
            pos++;

        if (wordLen == 0)
            continue;

        // Prüfen ob Wort noch in aktuelle Zeile passt
        char testBuffer[256];
        if (lineLen > 0)
        {
            snprintf(testBuffer, sizeof(testBuffer), "%s %s", lineBuffer, wordBuffer);
        }
        else
        {
            strncpy(testBuffer, wordBuffer, sizeof(testBuffer) - 1);
            testBuffer[sizeof(testBuffer) - 1] = '\0';
        }

        if (m_tft->textWidth(testBuffer) <= maxWidth)
        {
            // Wort passt - zur Zeile hinzufügen
            if (lineLen > 0)
            {
                lineBuffer[lineLen++] = ' ';
            }
            memcpy(lineBuffer + lineLen, wordBuffer, wordLen);
            lineLen += wordLen;
            lineBuffer[lineLen] = '\0';
        }
        else
        {
            // Wort passt nicht - aktuelle Zeile abschließen
            if (lineLen > 0)
            {
                lineCount++;
            }
            // Wort als neue Zeile beginnen
            memcpy(lineBuffer, wordBuffer, wordLen);
            lineLen = wordLen;
            lineBuffer[lineLen] = '\0';
        }
    }

    // Letzte Zeile zählen
    if (lineLen > 0)
        lineCount++;

    m_tft->unloadFont();
    return lineCount;
}

/**
 * @brief Setzt Bounds mit automatischer Höhenberechnung
 */
void MessageBox::setAutoBounds(int x, int minY, int w, int screenHeight)
{
    // Verfügbare Breite für Text
    int textMaxWidth = w - 2 * UIConfig::PADDING;

    // Font-Höhe ermitteln
    m_tft->loadFont(ComfortaaBold14);
    int lineHeight = m_tft->fontHeight() + UIConfig::LINE_HEIGHT_FACTOR;
    m_tft->unloadFont();

    // Anzahl Textzeilen berechnen
    int lineCount = 0;
    int textHeight = 0;
    if (m_detailRowCount > 0)
    {
        lineCount = static_cast<int>(m_detailRowCount);
        textHeight = lineCount * (lineHeight + 2) - 2;
    }
    else
    {
        lineCount = calculateTextLineCount(textMaxWidth, m_messageText);
        if (lineCount < 1)
            lineCount = 1;
        textHeight = lineCount * lineHeight;
    }

    // Gesamthöhe berechnen:
    // - Header (Titel + Separator): TEXT_Y_START
    // - Text: lineCount * lineHeight
    // - Abstand zum Button: 15px
    // - Button: BUTTON_HEIGHT
    // - Padding unten: PADDING
    int h = UIConfig::TEXT_Y_START + textHeight + 15 + UIConfig::BUTTON_HEIGHT + UIConfig::PADDING;

    // Mindesthöhe
    if (h < 150)
        h = 150;

    // Y-Position: zentriert, aber nicht oberhalb von minY
    int centeredY = (screenHeight - h) / 2;
    int finalY = (centeredY < minY) ? minY : centeredY;

    m_msgX = x;
    m_msgY = finalY;
    m_msgW = w;
    m_msgH = h;
    m_autoBoundsCalculated = true;
}

/**
 * @brief Zwei-Button-Variante (Abbrechen optional)
 */
void MessageBox::setButtonLabels(const char *okLabel, const char *cancelLabel)
{
    stringutils::safeCopy(m_labelOk, okLabel ? okLabel : "OK");
    if (cancelLabel && *cancelLabel)
    {
        stringutils::safeCopy(m_labelCancel, cancelLabel);
        m_hasCancel = true;
    }
    else
    {
        m_labelCancel[0] = '\0';
        m_hasCancel = false;
    }
}

/**
 * @brief Ein-Button-Variante
 */
void MessageBox::setButtonLabel(const char *okLabel)
{
    setButtonLabels(okLabel, nullptr);
}

/**
 * @brief Timeout setzen (0 = deaktiviert)
 */
void MessageBox::setTimeout(uint32_t millis)
{
    m_timeoutMs = millis;
}

/**
 * @brief Pending-Timeout setzen
 */
void MessageBox::setPendingTimeout(uint32_t millis)
{
    m_pendingTimeoutMs = millis;
}

/**
 * @brief Titeltext setzen
 */
void MessageBox::setTitle(const char *title)
{
    stringutils::safeCopy(m_titleText, title);
}

/**
 * @brief Nachrichtentext setzen
 */
void MessageBox::setMessage(const char *message)
{
    // Warnung wenn AutoBounds bereits berechnet wurde
    if (m_autoBoundsCalculated)
    {
        LOG_WARNING("MessageBox::setMessage() nach setAutoBounds() - Höhe passt evtl. nicht mehr!");
    }
    stringutils::safeCopy(m_messageText, message);
    m_detailRowCount = 0;
}

/**
 * @brief Zweispaltige Detailzeilen setzen.
 */
void MessageBox::setDetailRows(const char *const labels[], const char *const values[], size_t count)
{
    if (!labels || !values)
    {
        m_detailRowCount = 0;
        return;
    }

    if (m_autoBoundsCalculated)
        LOG_WARNING("MessageBox::setDetailRows() nach setAutoBounds() - Höhe passt evtl. nicht mehr!");

    if (count > UIConfig::MAX_DETAIL_ROWS)
        count = UIConfig::MAX_DETAIL_ROWS;

    for (size_t i = 0; i < count; ++i)
    {
        stringutils::safeCopy(m_detailRows[i].label, sizeof(m_detailRows[i].label), labels[i]);
        stringutils::safeCopy(m_detailRows[i].value, sizeof(m_detailRows[i].value), values[i]);
    }

    m_detailRowCount = count;
    m_messageText[0] = '\0';
}

/**
 * @brief Callback setzen
 */
void MessageBox::setCallback(std::function<void(bool)> cb)
{
    m_callback = cb;
}

/**
 * @brief Anzeige starten (Backup + Zeichnen)
 */
void MessageBox::start()
{
    m_resultConfirmed = false;
    m_needsRedraw = false;
    m_autoBoundsCalculated = false;  // Flag zurücksetzen für nächste Verwendung
    m_startTime = millis();
    m_state = State::Active;
    m_touchState = TouchState(); // Touch-State zurücksetzen

    backup();
    draw();
}

/**
 * @brief Prüfen, ob die Box aktiv ist (wartet auf Input)
 */
bool MessageBox::isActive() const
{
    return m_state == State::Active;
}

/**
 * @brief Prüfen, ob die Box sichtbar ist
 */
bool MessageBox::isVisible() const
{
    return m_state == State::Active || m_state == State::ConfirmationPending;
}

/**
 * @brief Prüfen, ob die Box busy ist (wartet auf externe Verarbeitung)
 */
bool MessageBox::isBusy() const
{
    return m_state == State::ConfirmationPending;
}

/**
 * @brief Führt die zyklische Touch- und Zeitüberschreitungslogik aus.
 */
void MessageBox::update()
{
    // Im Pending-Zustand: Cancel erlauben und Pending-Timeout prüfen
    if (m_state == State::ConfirmationPending)
    {
        processPendingInput();
        checkPendingTimeout();
        return;
    }

    if (m_state != State::Active)
        return;

    processTouchInput();
    checkTimeout();
}

/**
 * @brief Sichert den Bildschirminhalt
 */
void MessageBox::backup()
{
    size_t size = m_msgW * m_msgH * sizeof(lgfx::rgb565_t);

    // Buffer immer neu anlegen, denn die Größe könnte sich geändert haben
    if (m_screenBuffer != nullptr)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
    }

    m_screenBuffer = static_cast<lgfx::rgb565_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (m_screenBuffer != nullptr)
    {
        m_tft->startWrite();
        m_tft->readRect(m_msgX, m_msgY, m_msgW, m_msgH, m_screenBuffer);
        m_tft->endWrite();
        m_backupValid = true;
    }
    else
    {
        m_backupValid = false;
        LOG_ERROR("MessageBox: Konnte Backup-Buffer nicht anlegen");
    }
}

/**
 * @brief Stellt das gesicherte Rechteck wieder her
 */
void MessageBox::restore()
{
    m_tft->startWrite();

    if (m_backupValid && m_screenBuffer != nullptr)
    {
        m_tft->pushImage(m_msgX, m_msgY, m_msgW, m_msgH, m_screenBuffer);
        m_backupValid = false;
        m_needsRedraw = false;
    }
    else
    {
        // Kein gültiges Backup vorhanden - Bereich clearen statt Artefakte hinterlassen
        m_tft->fillRect(m_msgX, m_msgY, m_msgW, m_msgH, Theme::ColorMainBackGround);
        m_needsRedraw = true;
        LOG_WARNING("MessageBox::restore(): Kein Backup, Bereich gecleant. Caller muss UI neu zeichnen.");
    }

    m_tft->endWrite();
}

/**
 * @brief Zeichnet die gesamte MessageBox
 */
void MessageBox::draw()
{
    m_tft->startWrite();
    m_tft->setTextDatum(TL_DATUM);

    drawFrame();
    drawHeader();
    drawSeparator();
    drawMessageText();
    drawButtons();

    m_tft->endWrite();
}

/**
 * @brief Rahmen zeichnen
 */
void MessageBox::drawFrame()
{
    m_tft->fillSmoothRoundRect(m_msgX, m_msgY, m_msgW, m_msgH, UIConfig::BORDER_RADIUS, Theme::ColorDialogBorder);
    m_tft->fillSmoothRoundRect(m_msgX + 1, m_msgY + 1, m_msgW - 2, m_msgH - 2, UIConfig::BORDER_RADIUS, Theme::ColorMainBackGround);
}

/**
 * @brief Header zeichnen
 */
void MessageBox::drawHeader()
{
    const int headerX = m_msgX + UIConfig::PADDING;
    const int headerY = m_msgY + UIConfig::TITLE_Y_OFFSET;

    m_tft->loadFont(ComfortaaBold18);
    m_tft->setTextColor(Theme::ColorTitle, Theme::ColorMainBackGround);
    m_tft->drawString(m_titleText, headerX, headerY + 8);
    m_tft->unloadFont();
}

/**
 * @brief Separator-Linie zeichnen
 */
void MessageBox::drawSeparator()
{
    m_tft->fillRect(m_msgX + UIConfig::PADDING, m_msgY + UIConfig::SEPARATOR_Y, m_msgW - 2 * UIConfig::PADDING, 2, Theme::ColorDialogSeparator);
}

/**
 * @brief Nachrichtentext zeichnen
 */
void MessageBox::drawMessageText()
{
    if (m_detailRowCount > 0)
    {
        drawDetailRows();
        return;
    }

    m_tft->loadFont(ComfortaaBold14);
    m_tft->setTextColor(UIConfig::COLOR_TEXT, Theme::ColorMainBackGround);
    drawMultilineText(m_msgX + UIConfig::PADDING, m_msgY + UIConfig::TEXT_Y_START, m_msgW - 2 * UIConfig::PADDING, m_messageText);
    m_tft->unloadFont();
}

/**
 * @brief Zweispaltige Detailzeilen zeichnen.
 */
void MessageBox::drawDetailRows()
{
    if (m_detailRowCount == 0)
        return;

    constexpr int ROW_GAP_Y = 2;
    constexpr int COLUMN_GAP_X = 16;
    constexpr int DIVIDER_W = 1;

    const int contentX = m_msgX + UIConfig::PADDING;
    const int contentY = m_msgY + UIConfig::TEXT_Y_START;
    const int contentW = m_msgW - 2 * UIConfig::PADDING;
    const int dividerX = contentX + contentW / 2;
    const int labelW = contentW / 2 - COLUMN_GAP_X;
    const int valueX = dividerX + COLUMN_GAP_X;

    m_tft->loadFont(ComfortaaBold14);
    const int rowH = m_tft->fontHeight() + UIConfig::LINE_HEIGHT_FACTOR + ROW_GAP_Y;
    const int dividerH = static_cast<int>(m_detailRowCount) * rowH - ROW_GAP_Y;

    m_tft->fillRect(dividerX, contentY - 1, DIVIDER_W, dividerH + 2, UIConfig::COLOR_DETAIL_DIVIDER);

    for (size_t i = 0; i < m_detailRowCount; ++i)
    {
        const int y = contentY + static_cast<int>(i) * rowH;
        m_tft->setTextDatum(TR_DATUM);
        m_tft->setTextColor(UIConfig::COLOR_DETAIL_LABEL, Theme::ColorMainBackGround);
        m_tft->drawString(m_detailRows[i].label, contentX + labelW, y);

        m_tft->setTextDatum(TL_DATUM);
        m_tft->setTextColor(UIConfig::COLOR_DETAIL_VALUE, Theme::ColorMainBackGround);
        m_tft->drawString(m_detailRows[i].value, valueX, y);
    }

    m_tft->setTextDatum(TL_DATUM);
    m_tft->unloadFont();
}

/**
 * @brief Buttons zeichnen
 */
void MessageBox::drawButtons()
{
    const int btnY = m_msgY + m_msgH - UIConfig::PADDING - UIConfig::BUTTON_HEIGHT / 2;
    int btnOkX = 0;
    int btnOkW = UIConfig::BUTTON_WIDTH;
    int btnCancelX = 0;
    int btnCancelW = UIConfig::BUTTON_WIDTH;
    calculateButtonLayout(btnOkX, btnOkW, btnCancelX, btnCancelW);

    if (m_hasCancel)
    {
        m_btnOk.initButton(
            m_tft,
            btnOkX,
            btnY,
            btnOkW,
            UIConfig::BUTTON_HEIGHT,
            UIConfig::BORDER_RADIUS,
            UIConfig::COLOR_BUTTON_BORDER,
            Theme::ColorMainBackGround,
            UIConfig::COLOR_BUTTON_TEXT_OK, m_labelOk,
            nullptr);

        m_btnOk.setFont(LeagueGothicCond34);
        m_btnOk.drawButton();

        m_btnCancel.initButton(
            m_tft,
            btnCancelX,
            btnY,
            btnCancelW,
            UIConfig::BUTTON_HEIGHT,
            UIConfig::BORDER_RADIUS,
            UIConfig::COLOR_BUTTON_BORDER,
            Theme::ColorMainBackGround,
            UIConfig::COLOR_BUTTON_TEXT_CANCEL,
            m_labelCancel,
            nullptr);

        m_btnCancel.setFont(LeagueGothicCond34);
        m_btnCancel.drawButton();
    }
    else
    {
        m_btnOk.initButton(
            m_tft,
            btnOkX,
            btnY,
            btnOkW,
            UIConfig::BUTTON_HEIGHT,
            UIConfig::BORDER_RADIUS,
            UIConfig::COLOR_BUTTON_BORDER,
            Theme::ColorMainBackGround,
            UIConfig::COLOR_BUTTON_TEXT_OK,
            m_labelOk,
            nullptr);

        m_btnOk.setFont(LeagueGothicCond34);
        m_btnOk.drawButton();
    }
}

/**
 * @brief Berechnet die Buttonbreite für ein Label aus Textbreite und Innenabstand.
 */
int MessageBox::buttonWidthForLabel(const char *label) const
{
    if (!m_tft)
        return UIConfig::BUTTON_WIDTH;

    m_tft->loadFont(LeagueGothicCond34);
    const int textW = m_tft->textWidth(label ? label : "");
    m_tft->unloadFont();

    const int wantedW = textW + 2 * UIConfig::BUTTON_PADDING_X;
    if (wantedW < UIConfig::BUTTON_WIDTH)
        return UIConfig::BUTTON_WIDTH;

    return wantedW;
}

/**
 * @brief Berechnet Position und Breite der Dialogbuttons.
 */
void MessageBox::calculateButtonLayout(int &okX, int &okW, int &cancelX, int &cancelW) const
{
    okW = buttonWidthForLabel(m_labelOk);
    cancelW = m_hasCancel ? buttonWidthForLabel(m_labelCancel) : 0;

    int availableW = m_msgW - 2 * UIConfig::PADDING;
    if (availableW < UIConfig::BUTTON_WIDTH)
        availableW = UIConfig::BUTTON_WIDTH;
    if (!m_hasCancel)
    {
        if (okW > availableW)
            okW = availableW;
        okX = m_msgX + m_msgW / 2;
        cancelX = 0;
        cancelW = 0;
        return;
    }

    int maxButtonW = (availableW - UIConfig::BUTTON_SPACING) / 2;
    if (maxButtonW < UIConfig::BUTTON_WIDTH)
        maxButtonW = UIConfig::BUTTON_WIDTH;
    if (okW > maxButtonW)
        okW = maxButtonW;
    if (cancelW > maxButtonW)
        cancelW = maxButtonW;

    const int btnAreaW = okW + UIConfig::BUTTON_SPACING + cancelW;
    const int btnStartX = m_msgX + m_msgW - UIConfig::PADDING - btnAreaW;
    okX = btnStartX + okW / 2;
    cancelX = btnStartX + okW + UIConfig::BUTTON_SPACING + cancelW / 2;
}

/**
 * @brief Mehrzeiligen Text mit Umbruch zeichnen (wortweise für Performance)
 */
void MessageBox::drawMultilineText(int x, int y, int maxWidth, const char *text)
{
    if (!text || !*text)
        return;

    char lineBuffer[256] = {0};  // Initialisiert für sicheren snprintf-Zugriff
    char wordBuffer[64];
    int lineLen = 0;
    int wordLen = 0;
    int yOffset = 0;
    const int lineHeight = m_tft->fontHeight() + UIConfig::LINE_HEIGHT_FACTOR;
    const char *pos = text;

    while (*pos)
    {
        // Expliziter Zeilenumbruch - auch Leerzeilen müssen die y-Position fortschalten, damit aufeinanderfolgende '\n' als Absatz-Trennung wirken.
        if (*pos == '\n')
        {
            if (lineLen > 0)
            {
                // Aktuelle Zeile enthält Text: flushen, dann y fortschalten.
                lineBuffer[lineLen] = '\0';
                m_tft->drawString(lineBuffer, x, y + yOffset);
                lineLen = 0;
            }
            // y immer fortschalten - auch bei leerer Zeile.
            yOffset += lineHeight;
            pos++;
            continue;
        }

        // Wort sammeln (bis Space oder Ende)
        wordLen = 0;
        while (*pos && *pos != ' ' && *pos != '\n' && wordLen < static_cast<int>(sizeof(wordBuffer)) - 1)
        {
            wordBuffer[wordLen++] = *pos++;
        }
        wordBuffer[wordLen] = '\0';

        // Spaces überspringen
        while (*pos == ' ')
            pos++;

        if (wordLen == 0)
            continue;

        // Prüfen ob Wort noch in aktuelle Zeile passt
        char testBuffer[256];
        if (lineLen > 0)
        {
            snprintf(testBuffer, sizeof(testBuffer), "%s %s", lineBuffer, wordBuffer);
        }
        else
        {
            strncpy(testBuffer, wordBuffer, sizeof(testBuffer) - 1);
            testBuffer[sizeof(testBuffer) - 1] = '\0';
        }

        if (m_tft->textWidth(testBuffer) <= maxWidth)
        {
            // Wort passt - zur Zeile hinzufügen
            if (lineLen > 0)
            {
                lineBuffer[lineLen++] = ' ';
            }
            memcpy(lineBuffer + lineLen, wordBuffer, wordLen);
            lineLen += wordLen;
            lineBuffer[lineLen] = '\0';
        }
        else
        {
            // Wort passt nicht - aktuelle Zeile zeichnen
            if (lineLen > 0)
            {
                m_tft->drawString(lineBuffer, x, y + yOffset);
                yOffset += lineHeight;
            }
            // Wort als neue Zeile beginnen
            memcpy(lineBuffer, wordBuffer, wordLen);
            lineLen = wordLen;
            lineBuffer[lineLen] = '\0';
        }
    }

    // Letzte Zeile zeichnen
    if (lineLen > 0)
    {
        m_tft->drawString(lineBuffer, x, y + yOffset);
    }
}

/**
 * @brief Touch-Input auswerten und Buttons triggern
 */
void MessageBox::processTouchInput()
{
    lgfx::touch_point_t touch;
    const bool currentlyTouching = m_tft->getTouch(&touch);

    if (currentlyTouching)
    {
        const bool onOk = m_btnOk.contains(touch.x, touch.y);
        const bool onCancel = m_hasCancel && m_btnCancel.contains(touch.x, touch.y);

        if (onOk != m_touchState.wasOnOk)
        {
            m_btnOk.drawButton(onOk);
            m_touchState.wasOnOk = onOk;
        }
        if (m_hasCancel && (onCancel != m_touchState.wasOnCancel))
        {
            m_btnCancel.drawButton(onCancel);
            m_touchState.wasOnCancel = onCancel;
        }
        m_touchState.isTouching = true;
    }
    else if (m_touchState.isTouching)
    {
        if (m_touchState.wasOnOk)
        {
            // OK geklickt -> Wechsel in Pending-Zustand für externe Verarbeitung
            m_btnOk.drawButton(false);
            m_state = State::ConfirmationPending;
            m_pendingStartTime = millis();
            drawLoadingState();
            // NICHT finish() aufrufen - wartet auf completeConfirmation()
        }
        else if (m_hasCancel && m_touchState.wasOnCancel)
        {
            // Abbruch -> normal beenden
            m_btnCancel.drawButton(false);
            finish(false);
        }
        m_touchState = TouchState();
    }
}

/**
 * @brief Timeout überwachen
 */
void MessageBox::checkTimeout()
{
    if (m_timeoutMs > 0 && (millis() - m_startTime) > m_timeoutMs)
    {
        finish(false);
    }
}

/**
 * @brief Touch-Input im Pending-Zustand (nur Cancel erlaubt)
 */
void MessageBox::processPendingInput()
{
    if (!m_hasCancel)
        return;

    lgfx::touch_point_t touch;
    const bool currentlyTouching = m_tft->getTouch(&touch);

    if (currentlyTouching)
    {
        const bool onCancel = m_btnCancel.contains(touch.x, touch.y);

        if (onCancel != m_touchState.wasOnCancel)
        {
            m_btnCancel.drawButton(onCancel);
            m_touchState.wasOnCancel = onCancel;
        }
        m_touchState.isTouching = true;
    }
    else if (m_touchState.isTouching)
    {
        if (m_touchState.wasOnCancel)
        {
            // Abbruch im Pending-Zustand
            m_btnCancel.drawButton(false);
            finish(false);
        }
        m_touchState = TouchState();
    }
}

/**
 * @brief Pending-Timeout überwachen
 */
void MessageBox::checkPendingTimeout()
{
    if (m_pendingTimeoutMs > 0 && (millis() - m_pendingStartTime) > m_pendingTimeoutMs)
    {
        LOG_WARNING("MessageBox: Pending-Timeout erreicht, Dialog wird abgebrochen");
        finish(false);
    }
}

/**
 * @brief Abschluss: Restore, Callback, Aufräumen
 */
void MessageBox::finish(bool result)
{
    m_resultConfirmed = result;

    restore();

    if (m_screenBuffer != nullptr)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
        m_bufferSizeBytes = 0;
    }

    m_state = State::Finished;

    if (m_callback)
    {
        auto cb = m_callback;
        m_callback = nullptr;
        cb(m_resultConfirmed);
    }

    // Reentrancy-Schutz: Nur auf Hidden setzen, wenn der Callback nicht bereits einen neuen Dialog gestartet hat (via start())
    if (m_state == State::Finished)
    {
        m_state = State::Hidden;
    }
}

/**
 * @brief Setzt den Text für den Lade-Zustand
 */
void MessageBox::setLoadingText(const char *text)
{
    stringutils::safeCopy(m_loadingText, text ? text : "Bitte warten...");
}

/**
 * @brief Externe Bestätigung - schließt die Box ab
 */
void MessageBox::completeConfirmation(bool success)
{
    if (m_state != State::ConfirmationPending)
        return;

    finish(success);
}

/**
 * @brief Zeichnet den Lade-Zustand (beide Buttons werden durch zentrierten Text ersetzt)
 */
void MessageBox::drawLoadingState()
{
    m_tft->startWrite();

    // Gesamten Button-Bereich clearen (unabhängig von Ein- oder Zwei-Button-Layout)
    const int btnAreaY = m_msgY + m_msgH - UIConfig::PADDING - UIConfig::BUTTON_HEIGHT - 10;
    const int btnAreaH = UIConfig::BUTTON_HEIGHT + 20;
    m_tft->fillRect(m_msgX + UIConfig::PADDING, btnAreaY,
                    m_msgW - 2 * UIConfig::PADDING, btnAreaH, Theme::ColorMainBackGround);

    // Lade-Text zentriert anzeigen
    m_tft->loadFont(ComfortaaBold14);
    m_tft->setTextColor(0xFD20, Theme::ColorMainBackGround); // Orange
    m_tft->setTextDatum(MC_DATUM);
    m_tft->drawString(m_loadingText, m_msgX + m_msgW / 2, btnAreaY + btnAreaH / 2);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->unloadFont();

    m_tft->endWrite();
}

/**
 * @brief Prüft, ob der Aufrufer die Benutzeroberfläche neu zeichnen muss.
 */
bool MessageBox::needsRedraw() const
{
    return m_needsRedraw;
}
