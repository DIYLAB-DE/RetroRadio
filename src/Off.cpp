#include "Off.h"

#include "AppState.h"    // für globale Zustände und Einstellungen
#include "I18n.h"        // für Lokalisierung und Textformatierung
#include "Logger.h"      // für Log-Ausgaben (Debug, Info, Fehler)
#include "Strings_off.h" // für lokalisierte Strings des Off-Moduls
#include "StringUtils.h" // für sichere String-Operationen (safeCopy, safeAppend)
#include "Theme.h"       // für Farbkonstanten und Theme-Definitionen

// Fonts
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold30.h"
#include "fonts/LeagueGothicCond34.h"

// QR-Code-Renderer mit PSRAM-Allocator (Makros vor dem Include setzen)
#define QR_CODE_RENDERER_MALLOC(sz) heap_caps_malloc((sz), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define QR_CODE_RENDERER_FREE(ptr) free((ptr))
#include "QRCodeRenderer.h"

// ===================================================================
// Layout- und Farb-Konstanten
// ===================================================================

namespace
{
    constexpr int DISPLAY_W = 800; ///< Display-Breite

    // QR-Code (linke Bildschirmhälfte)
    constexpr int QR_BOX = 300; ///< Kantenlänge des QR-Bereichs
    constexpr int QR_X = 50;    ///< X-Position des QR-Bereichs
    constexpr int QR_Y = 70;    ///< Y-Position des QR-Bereichs

    // Textspalte (rechte Bildschirmhälfte)
    constexpr int TEXT_X = 400;                     ///< Linke Kante der Textspalte
    constexpr int TEXT_W = DISPLAY_W - TEXT_X - 32; ///< Nutzbare Textbreite für den Umbruch
    constexpr int TITLE_Y = 84;                     ///< Y-Position des Titels
    constexpr int TITLE_SEP_Y = 124;                ///< Y-Position des Trenners unter dem Titel
    constexpr int TITLE_SEP_H = 2;                  ///< Höhe des Trenners in Pixeln
    constexpr int HINT_Y = 150;                     ///< Y-Position der ersten Hinweiszeile
    constexpr int HINT_LINE_H = 26;                 ///< Zeilenhöhe von Hinweistext und Link-Block
    constexpr int LINK_GAP = 20;                    ///< Abstand Hinweistext zum Link-Block
    constexpr int URL_GAP = 4;                      ///< Zusätzlicher Abstand Link-Label zur Adresse

    // Buttons
    constexpr int BTN_W = 220;                          ///< Button-Breite
    constexpr int BTN_H = 56;                           ///< Button-Höhe
    constexpr int BTN_RADIUS = 6;                       ///< Eckenradius der Buttons
    constexpr int BTN_CY = 430;                         ///< Y-Mittelpunkt beider Buttons
    constexpr int BTN_BACK_CX = 160;                    ///< X-Mittelpunkt des Zurück-Buttons
    constexpr int BTN_OFF_CX = DISPLAY_W - BTN_BACK_CX; ///< X-Mittelpunkt des Ausschalt-Buttons, gespiegelt zum Zurück-Button

    // Farben
    constexpr uint16_t C_BG = 0x0000;          ///< Hintergrund schwarz
    constexpr uint16_t C_TEXT = 0xC618;        ///< Hinweistext in Silber
    constexpr uint16_t C_LABEL = 0x8C71;       ///< Link-Label gedämpft grau
    constexpr uint16_t C_URL = 0x5D9F;         ///< Adresse in Hellblau
    constexpr uint16_t C_BTN_FILL = 0x0000;    ///< Button-Füllung schwarz
    constexpr uint16_t C_BACK_BORDER = 0x52AA; ///< Rahmen des Zurück-Buttons
    constexpr uint16_t C_BACK_TEXT = 0xC618;   ///< Text des Zurück-Buttons
    constexpr uint16_t C_OFF_BORDER = 0xF800;  ///< Rahmen des Ausschalt-Buttons in Rot
    constexpr uint16_t C_OFF_TEXT = 0xF800;    ///< Text des Ausschalt-Buttons in Rot

    // Adresse: im QR-Code kodierte Form und sichtbare Kurzform
    constexpr const char *QR_URL = "https://www.burda-audio.de/retroradio";
    constexpr const char *URL_DISPLAY = "burda-audio.de/retroradio";
    constexpr const char *PROJECT_TITLE = "RetroRadio";
}

// ===================================================================
// Konstruktor / Destruktor
// ===================================================================

/**
 * @brief Konstruktor
 */
Off::Off() {}

/**
 * @brief Destruktor
 */
Off::~Off() {}

// ===================================================================
// Modul-Schnittstelle
// ===================================================================

/**
 * @brief Gibt den Modultyp zurück
 */
AppModule Off::type() const
{
    return AppModule::OFF;
}

/**
 * @brief Initialisiert das Modul und zeichnet den Bildschirm
 */
bool Off::init(LGFX &tft, const OffCallbacks &callbacks)
{
    m_tft = &tft;
    m_callbacks = callbacks;

    initButtons();
    drawScreen();

    m_initialized = true;
    LOG_SUCCESS("Off-Modul erfolgreich initialisiert.");
    return true;
}

// ===================================================================
// Initialisierung
// ===================================================================

/**
 * @brief Erstellt die beiden Buttons mit lokalisierter Beschriftung
 */
void Off::initButtons()
{
    char backLabel[24];
    char offLabel[24];
    I18n::format(backLabel, sizeof(backLabel), OffKey::OFF_BTN_BACK);
    I18n::format(offLabel, sizeof(offLabel), OffKey::OFF_BTN_SHUTDOWN);

    m_backButton.initButton(
        m_tft,         // Display-Objekt
        BTN_BACK_CX,   // X-Position des Button-Zentrums
        BTN_CY,        // Y-Position des Button-Zentrums
        BTN_W,         // Breite des Buttons
        BTN_H,         // Höhe des Buttons
        BTN_RADIUS,    // Radius für abgerundete Ecken
        C_BACK_BORDER, // Farbe des Rahmens
        C_BTN_FILL,    // Füllfarbe des Buttons
        C_BACK_TEXT,   // Textfarbe
        backLabel,     // Beschriftung des Buttons
        nullptr        // Kein Icon
    );

    m_backButton.setFont(LeagueGothicCond34);

    m_offButton.initButton(
        m_tft,        // Display-Objekt
        BTN_OFF_CX,   // X-Position des Button-Zentrums
        BTN_CY,       // Y-Position des Button-Zentrums
        BTN_W,        // Breite des Buttons
        BTN_H,        // Höhe des Buttons
        BTN_RADIUS,   // Radius für abgerundete Ecken
        C_OFF_BORDER, // Farbe des Rahmens
        C_BTN_FILL,   // Füllfarbe des Buttons
        C_OFF_TEXT,   // Textfarbe
        offLabel,     // Beschriftung des Buttons
        nullptr       // Kein Icon
    );

    m_offButton.setFont(LeagueGothicCond34);
}

// ===================================================================
// Zeichenfunktionen
// ===================================================================

/**
 * @brief Zeichnet den kompletten Bildschirm einmalig
 */
void Off::drawScreen()
{
    m_tft->clear(C_BG);
    drawQrCode();
    drawInfoText();
    m_backButton.drawButton(false);
    m_offButton.drawButton(false);
}

/**
 * @brief Zeichnet den QR-Code zur Projektseite
 */
void Off::drawQrCode()
{
    QRCodeRenderer::Options opt;
    opt.ecc = QRCodeRenderer::Ecc::Medium;
    opt.borderModules = 4;
    opt.fgColor = 0x0000;
    opt.bgColor = 0xFFFF;

    if (!QRCodeRenderer::drawCentered(*m_tft, QR_X, QR_Y, QR_BOX, QR_BOX, QR_URL, opt))
        LOG_ERROR("QR-Code konnte nicht erzeugt werden!");
}

/**
 * @brief Zeichnet Titel, Hinweistext und Adresse in die rechte Spalte
 */
void Off::drawInfoText()
{
    m_tft->setTextDatum(TL_DATUM);

    // Titel
    m_tft->loadFont(ComfortaaBold30);
    m_tft->setTextColor(Theme::ColorTitle, C_BG);
    m_tft->drawString(PROJECT_TITLE, TEXT_X, TITLE_Y);
    m_tft->unloadFont();

    // Trenner unter dem Titel
    m_tft->fillRect(TEXT_X, TITLE_SEP_Y, TEXT_W, TITLE_SEP_H, Theme::ColorDialogSeparator);

    // Hinweistext und Link-Block teilen sich eine Schrift
    char hint[160];
    char linkLabel[40];
    I18n::format(hint, sizeof(hint), OffKey::OFF_HINT);
    I18n::format(linkLabel, sizeof(linkLabel), OffKey::OFF_LINK_LABEL);

    m_tft->loadFont(ComfortaaBold16);

    const int afterHintY = drawWrappedText(hint, TEXT_X, HINT_Y, TEXT_W, HINT_LINE_H, C_TEXT);

    const int labelY = afterHintY + LINK_GAP;
    m_tft->setTextColor(C_LABEL, C_BG);
    m_tft->drawString(linkLabel, TEXT_X, labelY);

    const int urlY = labelY + HINT_LINE_H + URL_GAP;
    m_tft->setTextColor(C_URL, C_BG);
    m_tft->drawString(URL_DISPLAY, TEXT_X, urlY);

    m_tft->unloadFont();
}

/**
 * @brief Zeichnet einen Text mit Wortumbruch in eine feste Breite
 */
int Off::drawWrappedText(const char *text, int x, int y, int maxWidth, int lineHeight, uint16_t color)
{
    if (!text)
        return y;

    m_tft->setTextColor(color, C_BG);

    char line[160];
    char word[80];
    char test[160];
    line[0] = '\0';
    int curY = y;

    const char *p = text;
    while (*p)
    {
        // Nächstes Wort bis zum Leerzeichen einsammeln
        size_t wlen = 0;
        while (*p && *p != ' ' && wlen + 1 < sizeof(word))
            word[wlen++] = *p++;
        word[wlen] = '\0';

        // Folgende Leerzeichen überspringen
        while (*p == ' ')
            ++p;

        // Testzeile aus bisheriger Zeile und neuem Wort bilden
        if (line[0] == '\0')
        {
            stringutils::safeCopy(test, word);
        }
        else
        {
            stringutils::safeCopy(test, line);
            stringutils::safeAppend(test, " ");
            stringutils::safeAppend(test, word);
        }

        // Passt die Testzeile, wird sie übernommen. Ein einzelnes zu breites Wort wird zwangsweise gesetzt, sonst käme es zu einer Endlosschleife.
        if (line[0] == '\0' || m_tft->textWidth(test) <= maxWidth)
        {
            stringutils::safeCopy(line, test);
        }
        else
        {
            m_tft->drawString(line, x, curY);
            curY += lineHeight;
            stringutils::safeCopy(line, word);
        }
    }

    if (line[0] != '\0')
    {
        m_tft->drawString(line, x, curY);
        curY += lineHeight;
    }

    return curY;
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet pro Frame die Touch-Eingaben der beiden Buttons
 */
void Off::draw()
{
    if (!m_initialized)
        return;

    // Bei aktiver Grafik-Sperre nicht zeichnen
    if (APP_STATE.getBlockGFX())
        return;

    int32_t tx = 0;
    int32_t ty = 0;
    const bool touched = m_tft->getTouch(&tx, &ty);

    handleButton(m_backButton, touched, tx, ty, m_callbacks.onExitRequested);
    handleButton(m_offButton, touched, tx, ty, m_callbacks.onPowerOffRequested);
}

/**
 * @brief Wertet einen Button aus und löst beim Loslassen den Callback aus
 */
void Off::handleButton(Button &btn, bool touched, int32_t tx, int32_t ty, void (*callback)())
{
    if (touched)
    {
        if (!btn.isPressed() && btn.contains(tx, ty))
        {
            btn.press(true);
            btn.drawButton(true);
        }
    }
    else if (btn.isPressed())
    {
        btn.press(false);
        btn.drawButton(false);

        if (callback)
            callback();
    }
}
