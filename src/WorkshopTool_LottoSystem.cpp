#include "WorkshopTool_LottoSystem.h"

#include <esp_random.h>

#include "AppState.h"
#include "I18n.h"
#include "StringUtils.h"
#include "Theme.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/ComfortaaBold30.h"
#include "fonts/LeagueGothicCond34.h"

namespace
{
    constexpr int CONTENT_W = 547; ///< Netto-Breite des Werkzeugbereichs
    constexpr int CONTENT_H = 327; ///< Netto-Höhe des Werkzeugbereichs
    constexpr int TOP_GAP = 6;     ///< Oberer Abstand im Werkzeugbereich

    constexpr int GRID_X = 0;       ///< X-Position der Zahlenmatrix
    constexpr int GRID_Y = TOP_GAP; ///< Y-Position der Zahlenmatrix
    constexpr int CELL_W = 41;      ///< Breite einer Lottozahl
    constexpr int CELL_H = 32;      ///< Höhe einer Lottozahl
    constexpr int CELL_GAP = 3;     ///< Abstand zwischen Lottozahlen
    constexpr int CELL_R = 7;       ///< Eckenradius einer Lottozahl
    constexpr int GRID_COLS = 7;    ///< Spalten der Zahlenmatrix
    constexpr int GRID_ROWS = 7;    ///< Zeilen der Zahlenmatrix
    constexpr int GRID_W = GRID_COLS * CELL_W + (GRID_COLS - 1) * CELL_GAP;
    constexpr int GRID_H = GRID_ROWS * CELL_H + (GRID_ROWS - 1) * CELL_GAP;

    constexpr int SUMMARY_Y = GRID_Y + GRID_H + 8; ///< Y-Position der Auswahlübersicht
    constexpr int SUMMARY_W = GRID_W;              ///< Breite der Auswahlübersicht
    constexpr int SUMMARY_H = CONTENT_H - SUMMARY_Y;

    constexpr int SIDE_W = 224;                 ///< Breite der rechten Statusspalte
    constexpr int SIDE_X = CONTENT_W - SIDE_W;  ///< X-Position der Statusspalte
    constexpr int CARD_R = 8;                   ///< Eckenradius der Statuskarten
    constexpr int CARD_GAP = 5;                 ///< Abstand zwischen Statuskarten
    constexpr int CARD_SYSTEM_H = 70;           ///< Höhe der Systemkarte
    constexpr int CARD_DRAW_H = 58;             ///< Höhe der Ziehungskarte
    constexpr int CARD_WINS_H = CONTENT_H - TOP_GAP - CARD_SYSTEM_H - CARD_DRAW_H - 2 * CARD_GAP;
    constexpr int SUPER_SUMMARY_Y = 32;         ///< Y-Offset der kompakten Superzahl-Zeile
    constexpr int SUPER_SUMMARY_BTN_Y = 30;     ///< Y-Offset der Superzahl-Buttons
    constexpr int SUPER_SUMMARY_BTN_PAD_X = 7;  ///< Horizontaler Innenabstand der Superzahl-Buttons
    constexpr int SUPER_SUMMARY_BTN_H = 20;     ///< Höhe eines Superzahl-Buttons
    constexpr int SUPER_SUMMARY_BTN_GAP = 4;    ///< Abstand zwischen Superzahl-Buttons

    constexpr int SUPER_DIALOG_W = 500;         ///< Breite des Superzahl-Dialogs
    constexpr int SUPER_DIALOG_H = 285;         ///< Höhe des Superzahl-Dialogs
    constexpr int SUPER_DIALOG_X = (CONTENT_W - SUPER_DIALOG_W) / 2; ///< X-Position des Superzahl-Dialogs
    constexpr int SUPER_DIALOG_Y = (CONTENT_H - SUPER_DIALOG_H) / 2; ///< Y-Position des Superzahl-Dialogs
    constexpr int SUPER_DIALOG_PADDING = 20;    ///< Innenabstand des Superzahl-Dialogs
    constexpr int SUPER_DIALOG_R = 4;           ///< Eckenradius des Superzahl-Dialogs
    constexpr int SUPER_DIALOG_SEP_Y = 48;      ///< Y-Offset der Trennerlinie
    constexpr int SUPER_DIALOG_ROW_TICKET_Y = 96; ///< Y-Offset der Tipp-Superzahl-Zeile
    constexpr int SUPER_DIALOG_ROW_DRAW_Y = 168;  ///< Y-Offset der Ziehungs-Superzahl-Zeile
    constexpr int SUPER_DIALOG_LABEL_OFFSET = 26; ///< Abstand der Zeilenbeschriftung zur Zahlenreihe
    constexpr int SUPER_DIALOG_DIGIT_W = 38;    ///< Breite eines Superzahl-Buttons
    constexpr int SUPER_DIALOG_DIGIT_H = 40;    ///< Höhe eines Superzahl-Buttons
    constexpr int SUPER_DIALOG_DIGIT_GAP = 7;   ///< Abstand zwischen Superzahl-Buttons
    constexpr int SUPER_DIALOG_CLOSE_W = 90;    ///< Breite des Schließen-Buttons
    constexpr int SUPER_DIALOG_CLOSE_H = 38;    ///< Höhe des Schließen-Buttons

    constexpr int FOOTER_BTN_H = 44;     ///< Höhe der Footer-Buttons
    constexpr int FOOTER_BTN_R = 6;      ///< Eckenradius der Footer-Buttons
    constexpr int FOOTER_BTN_GAP = 4;    ///< Abstand zwischen Footer-Buttons
    constexpr int FOOTER_BTN_MARGIN = 2; ///< Linker Rand im Footer
    constexpr int BTN_W_SYSTEM = 34;     ///< Breite eines Systemwahl-Buttons
    constexpr int BTN_W_RANDOM = 44;     ///< Breite des Zufalls-Buttons
    constexpr int BTN_W_CLEAR = 44;      ///< Breite des Lösch-Buttons
    constexpr int BTN_W_MODE = 70;       ///< Breite des Modus-Buttons
    constexpr int BTN_W_HELP = 34;       ///< Breite des Hilfe-Buttons
    constexpr int DIE_ICON_Y_OFFSET = -1;   ///< Vertikaler Versatz des Würfel-Icons
    constexpr int TRASH_ICON_Y_OFFSET = 2;  ///< Vertikaler Versatz des Mülleimer-Icons

    constexpr uint16_t C_CARD_BG = 0x2945;       ///< Hintergrund der Statuskarten
    constexpr uint16_t C_CARD_LABEL = 0xAD75;    ///< Label-Farbe
    constexpr uint16_t C_CARD_VALUE = 0xFFFF;    ///< Helle Werte-Farbe
    constexpr uint16_t C_CARD_DIM = 0xB5B6;      ///< Gedimmte Werte-Farbe
    constexpr uint16_t C_LOTTO_YELLOW = 0xFEA0;  ///< Lotto-Gelb
    constexpr uint16_t C_LOTTO_RED = 0xE800;     ///< Lotto-Rot
    constexpr uint16_t C_LOTTO_GREEN = 0x07E0;   ///< Lotto-Grün
    constexpr uint16_t C_NUMBER_IDLE = 0x18C3;   ///< Ruhende Zahl
    constexpr uint16_t C_NUMBER_EDGE = 0x5AEB;   ///< Rand ruhender Zahlen
    constexpr uint16_t C_NUMBER_TEXT = 0xDEDB;   ///< Text ruhender Zahlen
    constexpr uint16_t C_NUMBER_TEXT_DIM = 0x94B2; ///< Gedämpfter Text ruhender Zahlen
    constexpr uint16_t C_NUMBER_TICKET = C_LOTTO_YELLOW; ///< Tippzahl
    constexpr uint16_t C_NUMBER_DRAW = C_LOTTO_GREEN;    ///< Gewinnzahl
    constexpr uint16_t C_NUMBER_HIT = C_LOTTO_RED;       ///< Treffer-Markierung
    constexpr uint16_t C_BUTTON_ACTIVE_FILL = 0x328A;    ///< Aktiver Button-Hintergrund
    constexpr uint16_t C_BUTTON_ACTIVE_TEXT = 0xFFFF;    ///< Aktiver Button-Text
    constexpr uint16_t C_BUTTON_ACTIVE_OUTLINE = 0x328A; ///< Aktiver Button-Rand
    constexpr uint16_t C_BUTTON_INACTIVE_FILL = 0x0000;  ///< Inaktiver Button-Hintergrund
    constexpr uint16_t C_BUTTON_INACTIVE_TEXT = 0x7BEF;  ///< Inaktiver Button-Text
    constexpr uint16_t C_BUTTON_INACTIVE_OUTLINE = 0x39E8; ///< Inaktiver Button-Rand
    constexpr uint16_t C_DIE_FILL = 0xE6D7;   ///< Würfel-Fläche
    constexpr uint16_t C_DIE_EDGE = 0x8A65;   ///< Würfel-Rand
    constexpr uint16_t C_DIE_PIP = 0x3921;    ///< Würfelaugen
    constexpr uint16_t C_TRASH_BODY = 0xE800; ///< Mülleimer-Körper
    constexpr uint16_t C_TRASH_LID = 0xFEA0;  ///< Mülleimer-Deckel

    constexpr int DIALOG_HELP_W = 700;                       ///< Breite des Hilfe-Dialogs
    constexpr int DIALOG_HELP_X = (800 - DIALOG_HELP_W) / 2; ///< X-Position des Hilfe-Dialogs

    constexpr uint8_t LOTTO_MAX_NUMBER = 49; ///< Höchste Lottozahl
    constexpr uint8_t DRAW_NUMBER_COUNT = 6; ///< Anzahl Gewinnzahlen
    constexpr uint8_t SUPER_DIGIT_COUNT = 10; ///< Anzahl möglicher Superzahlen
    constexpr uint8_t SUPER_UNSET = 10;      ///< Noch nicht gesetzte Superzahl

    /**
     * @brief Liefert das Bit zur Lottozahl.
     */
    inline uint64_t numberBit(uint8_t n)
    {
        return 1ULL << n;
    }

    /**
     * @brief Zeichnet Augen eines kleinen Würfels.
     */
    void drawDiePips(LGFX *tft, int cx, int cy, int size, uint8_t value, uint16_t color)
    {
        const int innerSize = size - 4;
        const int off = innerSize / 4;
        const int pipR = size / 12;
        const int xl = cx - off;
        const int xc = cx;
        const int xr = cx + off;
        const int yt = cy - off;
        const int yc = cy;
        const int yb = cy + off;

        auto pip = [&](int x, int y)
        {
            tft->fillSmoothCircle(x, y, pipR, color);
        };

        switch (value)
        {
        case 1:
            pip(xc, yc);
            break;
        case 2:
            pip(xl, yt);
            pip(xr, yb);
            break;
        case 3:
            pip(xl, yt);
            pip(xc, yc);
            pip(xr, yb);
            break;
        case 4:
            pip(xl, yt);
            pip(xr, yt);
            pip(xl, yb);
            pip(xr, yb);
            break;
        default:
            pip(xl, yt);
            pip(xr, yt);
            pip(xc, yc);
            pip(xl, yb);
            pip(xr, yb);
            break;
        }
    }

    /**
     * @brief Zeichnet einen kleinen Würfel.
     */
    void drawSmallDie(LGFX *tft, int cx, int cy, int size, uint8_t value)
    {
        const int x = cx - size / 2;
        const int y = cy - size / 2;
        const int r = size / 6;

        tft->fillSmoothRoundRect(x, y, size, size, r, C_DIE_EDGE);
        tft->fillSmoothRoundRect(x + 2, y + 2, size - 4, size - 4, r - 1, C_DIE_FILL);
        drawDiePips(tft, cx, cy, size, value, C_DIE_PIP);
    }

    /**
     * @brief Zeichnet den Zufalls-Button mit einem kleinen Würfel.
     */
    void drawRandomButton(LGFX *tft, int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, bool invert, const char *longName)
    {
        (void)longName;

        const int32_t x0 = x - w / 2;
        const int32_t y0 = y - h / 2;
        const uint16_t outline = invert ? C_BUTTON_INACTIVE_TEXT : C_BUTTON_INACTIVE_OUTLINE;
        tft->fillSmoothRoundRect(x0, y0, w, h, r, outline);
        tft->fillSmoothRoundRect(x0 + 1, y0 + 1, w - 2, h - 2, r, C_BUTTON_INACTIVE_FILL);
        drawSmallDie(tft, x, y + DIE_ICON_Y_OFFSET, 27, 5);
    }

    /**
     * @brief Zeichnet den Lösch-Button als kleinen Mülleimer.
     */
    void drawTrashButton(LGFX *tft, int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, bool invert, const char *longName)
    {
        (void)longName;

        const int32_t x0 = x - w / 2;
        const int32_t y0 = y - h / 2;
        const uint16_t outline = invert ? C_BUTTON_INACTIVE_TEXT : C_BUTTON_INACTIVE_OUTLINE;
        const int cx = x;
        const int cy = y + TRASH_ICON_Y_OFFSET;

        tft->fillSmoothRoundRect(x0, y0, w, h, r, outline);
        tft->fillSmoothRoundRect(x0 + 1, y0 + 1, w - 2, h - 2, r, C_BUTTON_INACTIVE_FILL);
        tft->drawWideLine(cx - 8, cy - 9, cx + 8, cy - 9, 1.4f, C_TRASH_LID);
        tft->drawWideLine(cx - 4, cy - 13, cx + 4, cy - 13, 1.4f, C_TRASH_LID);
        tft->drawWideLine(cx - 2, cy - 15, cx + 2, cy - 15, 1.4f, C_TRASH_LID);
        tft->drawWideLine(cx - 7, cy - 6, cx - 5, cy + 9, 1.4f, C_TRASH_BODY);
        tft->drawWideLine(cx + 7, cy - 6, cx + 5, cy + 9, 1.4f, C_TRASH_BODY);
        tft->drawWideLine(cx - 5, cy + 9, cx + 5, cy + 9, 1.4f, C_TRASH_BODY);
        tft->drawWideLine(cx - 2, cy - 4, cx - 2, cy + 6, 1.1f, C_TRASH_BODY);
        tft->drawWideLine(cx + 2, cy - 4, cx + 2, cy + 6, 1.1f, C_TRASH_BODY);
    }

    /**
     * @brief Liefert die lokalisierte Bezeichnung der Gewinnklasse.
     */
    const __FlashStringHelper *prizeClassText(uint8_t index)
    {
        static const WorkshopKey KEYS[9] = {
            WorkshopKey::WORKSHOP_LOTTO_PRIZE_6_SUPER,
            WorkshopKey::WORKSHOP_LOTTO_PRIZE_6,
            WorkshopKey::WORKSHOP_LOTTO_PRIZE_5_SUPER,
            WorkshopKey::WORKSHOP_LOTTO_PRIZE_5,
            WorkshopKey::WORKSHOP_LOTTO_PRIZE_4_SUPER,
            WorkshopKey::WORKSHOP_LOTTO_PRIZE_4,
            WorkshopKey::WORKSHOP_LOTTO_PRIZE_3_SUPER,
            WorkshopKey::WORKSHOP_LOTTO_PRIZE_3,
            WorkshopKey::WORKSHOP_LOTTO_PRIZE_2_SUPER,
        };
        if (index >= 9)
            index = 8;
        return I18n::str(KEYS[index]);
    }
}

const WorkshopTool_LottoSystem::SystemDef WorkshopTool_LottoSystem::s_systems[5] = {
    {7, 7, 840},
    {8, 28, 3360},
    {9, 84, 10080},
    {10, 210, 25200},
    {11, 462, 55440},
};

/**
 * @brief Initialisiert Zustand, Footer-Buttons und Hilfe-Dialog.
 */
void WorkshopTool_LottoSystem::onEnter(LGFX *tft)
{
    loadState();
    initFooterButtons(tft);

    if (!m_msg)
        m_msg.reset(new MessageBox(tft));
}

/**
 * @brief Räumt Dialog und Touch-Zustand auf.
 */
void WorkshopTool_LottoSystem::onExit()
{
    if (owner())
        owner()->setWorkContentPaused(false);

    m_msg.reset();
    m_buttonsInitialized = false;
    m_touchActive = false;
    m_superDialogVisible = false;
}

/**
 * @brief Löscht Tipp, Ziehung und Auswertung.
 */
void WorkshopTool_LottoSystem::resetAll()
{
    m_systemIndex = 0;
    m_mode = InputMode::Ticket;
    m_ticketMask = 0;
    m_drawMask = 0;
    m_ticketSuperNumber = SUPER_UNSET;
    m_drawSuperNumber = SUPER_UNSET;
    m_superHit = false;
    m_touchActive = false;
    m_superDialogVisible = false;
    clearEvaluation();
}

/**
 * @brief Lädt den gespeicherten Systemschein.
 */
void WorkshopTool_LottoSystem::loadState()
{
    uint8_t systemIndex = 0;
    uint8_t mode = 0;
    uint64_t ticketMask = 0;
    uint64_t drawMask = 0;
    uint8_t ticketSuper = SUPER_UNSET;
    uint8_t drawSuper = SUPER_UNSET;

    APP_STATE.getWorkshopLottoState(systemIndex, mode, ticketMask, drawMask, ticketSuper, drawSuper);

    resetAll();
    m_systemIndex = (systemIndex < 5) ? systemIndex : 0;
    m_mode = (mode == 1) ? InputMode::Draw : InputMode::Ticket;
    m_ticketMask = validNumberMask(ticketMask);
    m_drawMask = validNumberMask(drawMask);
    m_ticketSuperNumber = validSuperNumber(ticketSuper);
    m_drawSuperNumber = validSuperNumber(drawSuper);

    trimMaskToLimit(m_ticketMask, activeSystemSize());
    trimMaskToLimit(m_drawMask, DRAW_NUMBER_COUNT);
    evaluate();
}

/**
 * @brief Speichert den aktuellen Systemschein.
 */
void WorkshopTool_LottoSystem::saveState() const
{
    APP_STATE.setWorkshopLottoState(m_systemIndex, static_cast<uint8_t>(m_mode), m_ticketMask, m_drawMask, m_ticketSuperNumber, m_drawSuperNumber);
}

/**
 * @brief Wechselt das Vollsystem.
 */
void WorkshopTool_LottoSystem::setSystemIndex(uint8_t index)
{
    if (index >= 5 || index == m_systemIndex)
        return;

    m_systemIndex = index;
    m_mode = InputMode::Ticket;
    m_ticketMask = 0;
    m_drawMask = 0;
    m_ticketSuperNumber = SUPER_UNSET;
    m_drawSuperNumber = SUPER_UNSET;
    m_superHit = false;
    clearEvaluation();
    updateFooterButtonColors();
    saveState();
}

/**
 * @brief Wählt eine Zahl im aktuellen Modus an oder ab.
 */
void WorkshopTool_LottoSystem::toggleNumber(uint8_t n)
{
    if (n < 1 || n > LOTTO_MAX_NUMBER)
        return;

    const uint64_t bit = numberBit(n);
    bool changed = false;
    if (m_mode == InputMode::Ticket)
    {
        if ((m_ticketMask & bit) != 0)
        {
            m_ticketMask &= ~bit;
            changed = true;
        }
        else if (ticketCount() < activeSystemSize())
        {
            m_ticketMask |= bit;
            changed = true;
        }
    }
    else
    {
        if ((m_drawMask & bit) != 0)
        {
            m_drawMask &= ~bit;
            changed = true;
        }
        else if (drawCount() < DRAW_NUMBER_COUNT)
        {
            m_drawMask |= bit;
            changed = true;
        }
    }

    if (!changed)
        return;

    evaluate();
    saveState();
}

/**
 * @brief Füllt den aktuellen Modus per Zufall.
 */
void WorkshopTool_LottoSystem::fillRandom()
{
    const uint8_t target = (m_mode == InputMode::Ticket) ? activeSystemSize() : DRAW_NUMBER_COUNT;
    uint64_t mask = 0;

    while (countMask(mask) < target)
    {
        const uint8_t n = randomNumber(LOTTO_MAX_NUMBER);
        mask |= numberBit(n);
    }

    if (m_mode == InputMode::Ticket)
    {
        m_ticketMask = mask;
        m_ticketSuperNumber = static_cast<uint8_t>(randomNumber(SUPER_DIGIT_COUNT) - 1);
    }
    else
    {
        m_drawMask = mask;
        m_drawSuperNumber = static_cast<uint8_t>(randomNumber(SUPER_DIGIT_COUNT) - 1);
    }

    evaluate();
    saveState();
}

/**
 * @brief Löscht Tipp oder Ziehung.
 */
void WorkshopTool_LottoSystem::clearCurrentMode()
{
    if (m_mode == InputMode::Ticket)
    {
        m_ticketMask = 0;
        m_ticketSuperNumber = SUPER_UNSET;
    }
    else
    {
        m_drawMask = 0;
        m_drawSuperNumber = SUPER_UNSET;
    }

    evaluate();
    saveState();
}

/**
 * @brief Berechnet Gewinnklassen aus Tipp und Ziehung.
 */
void WorkshopTool_LottoSystem::evaluate()
{
    clearEvaluation();
    updateSuperHit();

    if (!isTicketComplete() || !isDrawComplete() || !isSuperComplete())
        return;

    uint8_t nums[11] = {};
    uint8_t count = 0;
    for (uint8_t n = 1; n <= LOTTO_MAX_NUMBER; ++n)
    {
        if (hasNumber(m_ticketMask, n) && count < sizeof(nums))
            nums[count++] = n;
    }

    for (uint8_t a = 0; a + 5 < count; ++a)
    {
        for (uint8_t b = a + 1; b + 4 < count; ++b)
        {
            for (uint8_t c = b + 1; c + 3 < count; ++c)
            {
                for (uint8_t d = c + 1; d + 2 < count; ++d)
                {
                    for (uint8_t e = d + 1; e + 1 < count; ++e)
                    {
                        for (uint8_t f = e + 1; f < count; ++f)
                        {
                            uint8_t hits = 0;
                            hits = static_cast<uint8_t>(hits + (hasNumber(m_drawMask, nums[a]) ? 1 : 0));
                            hits = static_cast<uint8_t>(hits + (hasNumber(m_drawMask, nums[b]) ? 1 : 0));
                            hits = static_cast<uint8_t>(hits + (hasNumber(m_drawMask, nums[c]) ? 1 : 0));
                            hits = static_cast<uint8_t>(hits + (hasNumber(m_drawMask, nums[d]) ? 1 : 0));
                            hits = static_cast<uint8_t>(hits + (hasNumber(m_drawMask, nums[e]) ? 1 : 0));
                            hits = static_cast<uint8_t>(hits + (hasNumber(m_drawMask, nums[f]) ? 1 : 0));

                            if (hits == 6)
                                m_winCounts[m_superHit ? 0 : 1]++;
                            else if (hits == 5)
                                m_winCounts[m_superHit ? 2 : 3]++;
                            else if (hits == 4)
                                m_winCounts[m_superHit ? 4 : 5]++;
                            else if (hits == 3)
                                m_winCounts[m_superHit ? 6 : 7]++;
                            else if (hits == 2 && m_superHit)
                                m_winCounts[8]++;
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief Setzt die Gewinnzähler zurück.
 */
void WorkshopTool_LottoSystem::clearEvaluation()
{
    for (uint8_t i = 0; i < 9; ++i)
        m_winCounts[i] = 0;
}

/**
 * @brief Liefert die aktive Systemgröße.
 */
uint8_t WorkshopTool_LottoSystem::activeSystemSize() const
{
    return s_systems[m_systemIndex].numberCount;
}

/**
 * @brief Liefert die Anzahl der Normalfelder.
 */
uint16_t WorkshopTool_LottoSystem::activeFieldCount() const
{
    return s_systems[m_systemIndex].fieldCount;
}

/**
 * @brief Liefert den Spieleinsatz in Cent.
 */
uint32_t WorkshopTool_LottoSystem::activePriceCents() const
{
    return s_systems[m_systemIndex].priceCents;
}

/**
 * @brief Zählt Tippzahlen.
 */
uint8_t WorkshopTool_LottoSystem::ticketCount() const
{
    return countMask(m_ticketMask);
}

/**
 * @brief Zählt Gewinnzahlen.
 */
uint8_t WorkshopTool_LottoSystem::drawCount() const
{
    return countMask(m_drawMask);
}

/**
 * @brief Prüft, ob der Tipp vollständig ist.
 */
bool WorkshopTool_LottoSystem::isTicketComplete() const
{
    return ticketCount() == activeSystemSize();
}

/**
 * @brief Prüft, ob die Ziehung vollständig ist.
 */
bool WorkshopTool_LottoSystem::isDrawComplete() const
{
    return drawCount() == DRAW_NUMBER_COUNT;
}

/**
 * @brief Prüft, ob beide Superzahlen gesetzt sind.
 */
bool WorkshopTool_LottoSystem::isSuperComplete() const
{
    return m_ticketSuperNumber < SUPER_UNSET && m_drawSuperNumber < SUPER_UNSET;
}

/**
 * @brief Prüft ein Zahlen-Bit.
 */
bool WorkshopTool_LottoSystem::hasNumber(uint64_t mask, uint8_t n) const
{
    return (mask & numberBit(n)) != 0;
}

/**
 * @brief Zählt gesetzte Zahlenbits.
 */
uint8_t WorkshopTool_LottoSystem::countMask(uint64_t mask) const
{
    uint8_t count = 0;
    for (uint8_t n = 1; n <= LOTTO_MAX_NUMBER; ++n)
    {
        if ((mask & numberBit(n)) != 0)
            count++;
    }
    return count;
}

/**
 * @brief Entfernt ungültige Zahlenbits aus einer Maske.
 */
uint64_t WorkshopTool_LottoSystem::validNumberMask(uint64_t mask) const
{
    uint64_t clean = 0;
    for (uint8_t n = 1; n <= LOTTO_MAX_NUMBER; ++n)
    {
        if ((mask & numberBit(n)) != 0)
            clean |= numberBit(n);
    }
    return clean;
}

/**
 * @brief Prüft eine Superzahl.
 */
uint8_t WorkshopTool_LottoSystem::validSuperNumber(uint8_t n) const
{
    return (n < SUPER_DIGIT_COUNT) ? n : SUPER_UNSET;
}

/**
 * @brief Kürzt eine Zahlenmaske auf ein Maximum.
 */
void WorkshopTool_LottoSystem::trimMaskToLimit(uint64_t &mask, uint8_t maxCount) const
{
    while (countMask(mask) > maxCount)
    {
        for (int n = LOTTO_MAX_NUMBER; n >= 1; --n)
        {
            const uint64_t bit = numberBit(static_cast<uint8_t>(n));
            if ((mask & bit) != 0)
            {
                mask &= ~bit;
                break;
            }
        }
    }
}

/**
 * @brief Aktualisiert den Superzahl-Treffer.
 */
void WorkshopTool_LottoSystem::updateSuperHit()
{
    m_superHit = isSuperComplete() && m_ticketSuperNumber == m_drawSuperNumber;
}

/**
 * @brief Liefert eine gleichverteilte Zufallszahl von 1 bis maxInclusive.
 */
uint8_t WorkshopTool_LottoSystem::randomNumber(uint8_t maxInclusive) const
{
    const uint32_t range = static_cast<uint32_t>(maxInclusive);
    const uint32_t limit = 0xFFFFFFFFu - (0xFFFFFFFFu % range);

    uint32_t v;
    do
    {
        v = esp_random();
    } while (v >= limit);

    return static_cast<uint8_t>((v % range) + 1u);
}

/**
 * @brief Zeichnet Zahlenfeld, Tippübersicht und Auswertung.
 */
void WorkshopTool_LottoSystem::draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h)
{
    (void)w;
    (void)h;

    if (!sprite)
        return;

    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);
        return;
    }

    m_originX = ox;
    m_originY = oy;

    drawNumberGrid(sprite);
    drawSelectionSummary(sprite);
    drawSidebar(sprite);
    if (m_superDialogVisible)
        drawSuperDialog(sprite);
}

/**
 * @brief Zeichnet die 49 Lottozahlen.
 */
void WorkshopTool_LottoSystem::drawNumberGrid(Gfx_Sprite *sprite)
{
    for (uint8_t n = 1; n <= LOTTO_MAX_NUMBER; ++n)
    {
        const uint8_t idx = static_cast<uint8_t>(n - 1);
        const int col = idx % GRID_COLS;
        const int row = idx / GRID_COLS;
        const int x = m_originX + GRID_X + col * (CELL_W + CELL_GAP);
        const int y = m_originY + GRID_Y + row * (CELL_H + CELL_GAP);
        const bool ticket = hasNumber(m_ticketMask, n);
        const bool drawn = hasNumber(m_drawMask, n);
        drawNumberCell(sprite, n, x, y, ticket, drawn);
    }
}

/**
 * @brief Zeichnet eine Lottozahl.
 */
void WorkshopTool_LottoSystem::drawNumberCell(Gfx_Sprite *sprite, uint8_t n, int x, int y, bool ticket, bool drawn) const
{
    uint16_t fill = C_NUMBER_IDLE;
    uint16_t edge = C_NUMBER_EDGE;
    uint16_t text = C_NUMBER_TEXT;

    if (m_mode == InputMode::Ticket)
    {
        if (ticket)
        {
            fill = C_NUMBER_TICKET;
            edge = C_LOTTO_RED;
            text = 0x2104;
        }
        else if (drawn)
        {
            edge = C_LOTTO_GREEN;
        }
    }
    else
    {
        if (drawn)
        {
            fill = C_NUMBER_DRAW;
            edge = ticket ? C_NUMBER_HIT : C_LOTTO_GREEN;
            text = 0x0000;
        }
        else if (ticket)
        {
            edge = C_NUMBER_TICKET;
        }
    }

    sprite->fillSmoothRoundRect(x, y, CELL_W, CELL_H, CELL_R, edge);
    sprite->fillSmoothRoundRect(x + 2, y + 2, CELL_W - 4, CELL_H - 4, CELL_R - 2, fill);

    char buf[4];
    stringutils::safeFormat(buf, sizeof(buf), "%u", static_cast<unsigned>(n));
    sprite->loadFont(ComfortaaBold16);
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(text, fill);
    sprite->drawString(buf, x + CELL_W / 2, y + CELL_H / 2 + 1);
    sprite->unloadFont();
}

/**
 * @brief Zeichnet die gewählten Zahlen.
 */
void WorkshopTool_LottoSystem::drawSelectionSummary(Gfx_Sprite *sprite)
{
    const int x = m_originX + GRID_X;
    const int y = m_originY + SUMMARY_Y;
    constexpr int LINE_GAP = 42;
    constexpr int TEXT_BLOCK_H = LINE_GAP + 14;
    const int rowTicketY = y + (SUMMARY_H - TEXT_BLOCK_H) / 2;
    const int rowDrawY = rowTicketY + LINE_GAP;

    sprite->fillSmoothRoundRect(x, y, SUMMARY_W, SUMMARY_H, CARD_R, C_CARD_BG);

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TL_DATUM);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LOTTO_LABEL_TICKET), x + 10, rowTicketY);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LOTTO_LABEL_DRAW), x + 10, rowDrawY);
    sprite->unloadFont();

    drawNumberList(sprite, m_ticketMask, x + 92, rowTicketY, SUMMARY_W - 102, WorkshopKey::WORKSHOP_LOTTO_EMPTY_TICKET, C_LOTTO_YELLOW);
    drawNumberList(sprite, m_drawMask, x + 92, rowDrawY, SUMMARY_W - 102, WorkshopKey::WORKSHOP_LOTTO_EMPTY_DRAW, C_LOTTO_GREEN);
}

/**
 * @brief Zeichnet eine kompakte Zahlenliste.
 */
void WorkshopTool_LottoSystem::drawNumberList(Gfx_Sprite *sprite, uint64_t mask, int x, int y, int w, WorkshopKey emptyKey, uint16_t color) const
{
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TL_DATUM);

    if (mask == 0)
    {
        sprite->setTextColor(C_CARD_DIM, C_CARD_BG);
        sprite->drawString(I18n::str(emptyKey), x, y);
        sprite->unloadFont();
        return;
    }

    char buf[8];
    constexpr int NUMBER_STEP_X = 26;
    constexpr int NUMBER_STEP_Y = 16;
    const int numbersPerLine = (w > NUMBER_STEP_X) ? (w / NUMBER_STEP_X) : 1;
    int index = 0;
    for (uint8_t n = 1; n <= LOTTO_MAX_NUMBER; ++n)
    {
        if (!hasNumber(mask, n))
            continue;

        stringutils::safeFormat(buf, sizeof(buf), "%02u", static_cast<unsigned>(n));
        sprite->setTextColor(color, C_CARD_BG);
        sprite->drawString(buf, x + (index % numbersPerLine) * NUMBER_STEP_X, y + (index / numbersPerLine) * NUMBER_STEP_Y);
        index++;
    }

    sprite->unloadFont();
}

/**
 * @brief Zeichnet die kompakte Superzahl-Anzeige.
 */
void WorkshopTool_LottoSystem::drawSuperSummary(Gfx_Sprite *sprite, int sideX, int drawY) const
{
    char ticketBuf[2] = "-";
    char drawBuf[2] = "-";
    if (m_ticketSuperNumber < SUPER_UNSET)
        stringutils::safeFormat(ticketBuf, sizeof(ticketBuf), "%u", static_cast<unsigned>(m_ticketSuperNumber));
    if (m_drawSuperNumber < SUPER_UNSET)
        stringutils::safeFormat(drawBuf, sizeof(drawBuf), "%u", static_cast<unsigned>(m_drawSuperNumber));

    char ticketText[18] = {};
    char drawText[18] = {};
    I18n::format(ticketText, sizeof(ticketText), WorkshopKey::WORKSHOP_LOTTO_FMT_SUMMARY_TICKET_SUPER, ticketBuf);
    I18n::format(drawText, sizeof(drawText), WorkshopKey::WORKSHOP_LOTTO_FMT_SUMMARY_DRAW_SUPER, drawBuf);

    const int y = drawY + SUPER_SUMMARY_Y;
    const int btnY = drawY + SUPER_SUMMARY_BTN_Y;

    sprite->loadFont(ComfortaaBold14);
    const int ticketBtnW = sprite->textWidth(ticketText) + 2 * SUPER_SUMMARY_BTN_PAD_X;
    const int drawBtnW = sprite->textWidth(drawText) + 2 * SUPER_SUMMARY_BTN_PAD_X;
    const int drawBtnX = sideX + SIDE_W - 12 - drawBtnW;
    const int ticketBtnX = drawBtnX - SUPER_SUMMARY_BTN_GAP - ticketBtnW;

    sprite->setTextDatum(TL_DATUM);
    sprite->setTextColor(Theme::ColorCyan, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LOTTO_LABEL_SUPER), sideX + 12, y - 1);

    auto drawButton = [&](int bx, int bw, const char *text, bool active, uint16_t activeFill)
    {
        const int chipY = btnY + SUPER_SUMMARY_BTN_H / 2 - 1;
        const uint16_t fill = active ? activeFill : C_BUTTON_INACTIVE_FILL;
        const uint16_t textColor = active ? 0x0000 : C_CARD_DIM;

        sprite->fillRect(bx, btnY, bw, SUPER_SUMMARY_BTN_H, fill);
        sprite->setTextDatum(MC_DATUM);
        sprite->setTextColor(textColor, fill);
        sprite->drawString(text, bx + bw / 2, chipY);
    };

    drawButton(ticketBtnX, ticketBtnW, ticketText, m_ticketSuperNumber < SUPER_UNSET, Theme::ColorAmber);
    drawButton(drawBtnX, drawBtnW, drawText, m_drawSuperNumber < SUPER_UNSET, Theme::ColorGreen);
    sprite->unloadFont();
}

/**
 * @brief Zeichnet die Superzahl-Auswahl.
 */
void WorkshopTool_LottoSystem::drawSuperDialog(Gfx_Sprite *sprite) const
{
    const int x = m_originX + SUPER_DIALOG_X;
    const int y = m_originY + SUPER_DIALOG_Y;

    sprite->fillSmoothRoundRect(x, y, SUPER_DIALOG_W, SUPER_DIALOG_H, SUPER_DIALOG_R, Theme::ColorDialogBorder);
    sprite->fillSmoothRoundRect(x + 1, y + 1, SUPER_DIALOG_W - 2, SUPER_DIALOG_H - 2, SUPER_DIALOG_R, Theme::ColorMainBackGround);

    sprite->loadFont(ComfortaaBold18);
    sprite->setTextDatum(TL_DATUM);
    sprite->setTextColor(Theme::ColorTitle, Theme::ColorMainBackGround);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LOTTO_DIALOG_SUPER), x + SUPER_DIALOG_PADDING, y + 18);
    sprite->unloadFont();

    sprite->fillRect(x + SUPER_DIALOG_PADDING, y + SUPER_DIALOG_SEP_Y, SUPER_DIALOG_W - 2 * SUPER_DIALOG_PADDING, 2, Theme::ColorDialogSeparator);

    drawSuperDialogRow(sprite, y + SUPER_DIALOG_ROW_TICKET_Y, WorkshopKey::WORKSHOP_LOTTO_DIALOG_TICKET_SUPER, m_ticketSuperNumber, Theme::ColorAmber);
    drawSuperDialogRow(sprite, y + SUPER_DIALOG_ROW_DRAW_Y, WorkshopKey::WORKSHOP_LOTTO_DIALOG_DRAW_SUPER, m_drawSuperNumber, Theme::ColorGreen);

    const int btnX = x + SUPER_DIALOG_W - SUPER_DIALOG_PADDING - SUPER_DIALOG_CLOSE_W;
    const int btnY = y + SUPER_DIALOG_H - SUPER_DIALOG_PADDING - SUPER_DIALOG_CLOSE_H;
    sprite->fillSmoothRoundRect(btnX, btnY, SUPER_DIALOG_CLOSE_W, SUPER_DIALOG_CLOSE_H, SUPER_DIALOG_R, Theme::ColorButtonBorder);
    sprite->fillSmoothRoundRect(btnX + 1, btnY + 1, SUPER_DIALOG_CLOSE_W - 2, SUPER_DIALOG_CLOSE_H - 2, SUPER_DIALOG_R, Theme::ColorMainBackGround);
    sprite->loadFont(LeagueGothicCond34);
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(Theme::ColorButtonTextOk, Theme::ColorMainBackGround);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_DIALOG_CLOSE), btnX + SUPER_DIALOG_CLOSE_W / 2, btnY + SUPER_DIALOG_CLOSE_H / 2 + 1);
    sprite->unloadFont();
}

/**
 * @brief Zeichnet eine Superzahl-Zeile im Dialog.
 */
void WorkshopTool_LottoSystem::drawSuperDialogRow(Gfx_Sprite *sprite, int rowY, WorkshopKey labelKey, uint8_t value, uint16_t activeFill) const
{
    const int dialogX = m_originX + SUPER_DIALOG_X;
    const int rowX = dialogX + (SUPER_DIALOG_W - (SUPER_DIGIT_COUNT * SUPER_DIALOG_DIGIT_W + (SUPER_DIGIT_COUNT - 1) * SUPER_DIALOG_DIGIT_GAP)) / 2;

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TL_DATUM);
    sprite->setTextColor(activeFill, Theme::ColorMainBackGround);
    sprite->drawString(I18n::str(labelKey), dialogX + SUPER_DIALOG_PADDING, rowY - SUPER_DIALOG_LABEL_OFFSET);

    sprite->setTextDatum(MC_DATUM);
    char buf[2] = {};
    for (uint8_t n = 0; n < SUPER_DIGIT_COUNT; ++n)
    {
        const int bx = rowX + n * (SUPER_DIALOG_DIGIT_W + SUPER_DIALOG_DIGIT_GAP);
        const bool active = value == n;
        const uint16_t fill = active ? activeFill : Theme::ColorButtonFill;
        const uint16_t edge = active ? activeFill : Theme::ColorButtonBorder;
        const uint16_t text = active ? 0x0000 : Theme::ColorButtonText;

        sprite->fillSmoothRoundRect(bx, rowY, SUPER_DIALOG_DIGIT_W, SUPER_DIALOG_DIGIT_H, SUPER_DIALOG_R, edge);
        sprite->fillSmoothRoundRect(bx + 1, rowY + 1, SUPER_DIALOG_DIGIT_W - 2, SUPER_DIALOG_DIGIT_H - 2, SUPER_DIALOG_R, fill);
        stringutils::safeFormat(buf, sizeof(buf), "%u", static_cast<unsigned>(n));
        sprite->setTextColor(text, fill);
        sprite->drawString(buf, bx + SUPER_DIALOG_DIGIT_W / 2, rowY + SUPER_DIALOG_DIGIT_H / 2 + 1);
    }
    sprite->unloadFont();
}

/**
 * @brief Zeichnet Systemdaten und Gewinnklassen.
 */
void WorkshopTool_LottoSystem::drawSidebar(Gfx_Sprite *sprite)
{
    const int sideX = m_originX + SIDE_X;
    int y = m_originY + TOP_GAP;

    const int systemY = y;
    y += CARD_SYSTEM_H + CARD_GAP;
    const int drawY = y;
    y += CARD_DRAW_H + CARD_GAP;
    const int winsY = y;

    sprite->fillSmoothRoundRect(sideX, systemY, SIDE_W, CARD_SYSTEM_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, drawY, SIDE_W, CARD_DRAW_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, winsY, SIDE_W, CARD_WINS_H, CARD_R, C_CARD_BG);

    char buf[40];
    stringutils::safeFormat(buf, sizeof(buf), "Vollsystem %03u", static_cast<unsigned>(activeSystemSize()));

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TL_DATUM);
    sprite->setTextColor(Theme::ColorTitle, C_CARD_BG);
    sprite->drawString(buf, sideX + 12, systemY + 8);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextColor(Theme::ColorCyan, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LOTTO_LABEL_FIELDS), sideX + 13, systemY + 31);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LOTTO_LABEL_PRICE), sideX + 12, systemY + 50);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TR_DATUM);
    sprite->setTextColor(Theme::ColorMainText, C_CARD_BG);
    stringutils::safeFormat(buf, sizeof(buf), "%u", static_cast<unsigned>(activeFieldCount()));
    sprite->drawString(buf, sideX + SIDE_W - 12, systemY + 31);
    stringutils::safeFormat(buf, sizeof(buf), "%lu,%02lu EUR", static_cast<unsigned long>(activePriceCents() / 100), static_cast<unsigned long>(activePriceCents() % 100));
    sprite->drawString(buf, sideX + SIDE_W - 12, systemY + 50);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TL_DATUM);
    sprite->setTextColor(Theme::ColorTitle, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LOTTO_LABEL_DRAW), sideX + 12, drawY + 8);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TR_DATUM);
    sprite->setTextColor(isDrawComplete() ? C_LOTTO_GREEN : C_CARD_DIM, C_CARD_BG);
    stringutils::safeFormat(buf, sizeof(buf), "%u/6", static_cast<unsigned>(drawCount()));
    sprite->drawString(buf, sideX + SIDE_W - 12, drawY + 8);
    sprite->unloadFont();

    drawSuperSummary(sprite, sideX, drawY);

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TL_DATUM);
    sprite->setTextColor(Theme::ColorTitle, C_CARD_BG);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LOTTO_LABEL_WINS), sideX + 12, winsY + 8);
    sprite->unloadFont();

    const bool ready = isTicketComplete() && isDrawComplete() && isSuperComplete();
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(TL_DATUM);

    if (!ready)
    {
        sprite->setTextColor(C_CARD_DIM, C_CARD_BG);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_LOTTO_HINT_INCOMPLETE), sideX + 12, winsY + 40);
        sprite->unloadFont();
        return;
    }

    for (uint8_t i = 0; i < 9; ++i)
    {
        const int rowY = winsY + 30 + i * 16;
        const bool hasWin = m_winCounts[i] > 0;
        const bool oddRow = (i % 2) != 0;
        const uint16_t classColor = oddRow ? C_NUMBER_TEXT_DIM : C_NUMBER_TEXT;
        const uint16_t textColor = hasWin ? Theme::ColorMainText : classColor;
        sprite->setTextColor(classColor, C_CARD_BG);
        stringutils::safeFormat(buf, sizeof(buf), "%u", static_cast<unsigned>(i + 1));
        sprite->drawString(buf, sideX + 18, rowY);
        sprite->setTextColor(textColor, C_CARD_BG);
        sprite->drawString(prizeClassText(i), sideX + 42, rowY);

        sprite->setTextDatum(TR_DATUM);
        sprite->setTextColor(hasWin ? C_LOTTO_YELLOW : C_CARD_DIM, C_CARD_BG);
        stringutils::safeFormat(buf, sizeof(buf), "%u", static_cast<unsigned>(m_winCounts[i]));
        sprite->drawString(buf, sideX + SIDE_W - 12, rowY);
        sprite->setTextDatum(TL_DATUM);
    }

    sprite->unloadFont();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf Zahlenfeld und Superzahl-Eingabe.
 */
bool WorkshopTool_LottoSystem::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);

        m_touchActive = false;
        return false;
    }

    if (touched)
    {
        m_touchActive = true;
        m_touchLastX = tx;
        m_touchLastY = ty;
        return false;
    }

    if (!m_touchActive)
        return false;

    m_touchActive = false;

    if (m_superDialogVisible)
    {
        uint8_t superNumber = SUPER_UNSET;
        bool superDraw = false;
        bool closeDialog = false;
        if (hitTestSuperDialog(m_touchLastX, m_touchLastY, superNumber, superDraw, closeDialog))
        {
            if (closeDialog)
            {
                m_superDialogVisible = false;
                return true;
            }

            if (superDraw)
                m_drawSuperNumber = superNumber;
            else
                m_ticketSuperNumber = superNumber;

            evaluate();
            saveState();
            return true;
        }

        return false;
    }

    uint8_t number = 0;
    if (hitTestNumber(m_touchLastX, m_touchLastY, number))
    {
        toggleNumber(number);
        return true;
    }

    if (hitTestSuperSummary(m_touchLastX, m_touchLastY))
    {
        m_superDialogVisible = true;
        return true;
    }

    return false;
}

/**
 * @brief Ermittelt eine getippte Zahl.
 */
bool WorkshopTool_LottoSystem::hitTestNumber(int32_t tx, int32_t ty, uint8_t &outNumber) const
{
    const int relX = static_cast<int>(tx) - WorkshopCfg::WORK_X - m_originX - GRID_X;
    const int relY = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_originY - GRID_Y;

    if (relX < 0 || relY < 0 || relX >= GRID_W || relY >= GRID_H)
        return false;

    const int col = relX / (CELL_W + CELL_GAP);
    const int row = relY / (CELL_H + CELL_GAP);
    const int inX = relX - col * (CELL_W + CELL_GAP);
    const int inY = relY - row * (CELL_H + CELL_GAP);

    if (col < 0 || col >= GRID_COLS || row < 0 || row >= GRID_ROWS)
        return false;

    if (inX >= CELL_W || inY >= CELL_H)
        return false;

    outNumber = static_cast<uint8_t>(row * GRID_COLS + col + 1);
    return outNumber >= 1 && outNumber <= LOTTO_MAX_NUMBER;
}

/**
 * @brief Prüft die kompakte Superzahl-Anzeige.
 */
bool WorkshopTool_LottoSystem::hitTestSuperSummary(int32_t tx, int32_t ty) const
{
    const int relX = static_cast<int>(tx) - WorkshopCfg::WORK_X - m_originX;
    const int relY = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_originY;
    const int drawY = TOP_GAP + CARD_SYSTEM_H + CARD_GAP;

    return relX >= SIDE_X && relX < SIDE_X + SIDE_W && relY >= drawY + 27 && relY < drawY + CARD_DRAW_H;
}

/**
 * @brief Ermittelt eine Superzahl im Dialog.
 */
bool WorkshopTool_LottoSystem::hitTestSuperDialog(int32_t tx, int32_t ty, uint8_t &outNumber, bool &outDraw, bool &outClose) const
{
    outNumber = SUPER_UNSET;
    outDraw = false;
    outClose = false;

    const int relX = static_cast<int>(tx) - WorkshopCfg::WORK_X - m_originX;
    const int relY = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_originY;
    const int dialogX = SUPER_DIALOG_X;
    const int dialogY = SUPER_DIALOG_Y;

    const int closeX = dialogX + SUPER_DIALOG_W - SUPER_DIALOG_PADDING - SUPER_DIALOG_CLOSE_W;
    const int closeY = dialogY + SUPER_DIALOG_H - SUPER_DIALOG_PADDING - SUPER_DIALOG_CLOSE_H;
    if (relX >= closeX && relX < closeX + SUPER_DIALOG_CLOSE_W && relY >= closeY && relY < closeY + SUPER_DIALOG_CLOSE_H)
    {
        outClose = true;
        return true;
    }

    const int rowW = SUPER_DIGIT_COUNT * SUPER_DIALOG_DIGIT_W + (SUPER_DIGIT_COUNT - 1) * SUPER_DIALOG_DIGIT_GAP;
    const int rowX = dialogX + (SUPER_DIALOG_W - rowW) / 2;
    if (relX < rowX || relX >= rowX + rowW)
        return false;

    const int col = (relX - rowX) / (SUPER_DIALOG_DIGIT_W + SUPER_DIALOG_DIGIT_GAP);
    const int inX = (relX - rowX) - col * (SUPER_DIALOG_DIGIT_W + SUPER_DIALOG_DIGIT_GAP);
    if (col < 0 || col >= SUPER_DIGIT_COUNT || inX >= SUPER_DIALOG_DIGIT_W)
        return false;

    const int ticketY = dialogY + SUPER_DIALOG_ROW_TICKET_Y;
    const int drawY = dialogY + SUPER_DIALOG_ROW_DRAW_Y;
    if (relY >= ticketY && relY < ticketY + SUPER_DIALOG_DIGIT_H)
    {
        outNumber = static_cast<uint8_t>(col);
        outDraw = false;
        return true;
    }

    if (relY >= drawY && relY < drawY + SUPER_DIALOG_DIGIT_H)
    {
        outNumber = static_cast<uint8_t>(col);
        outDraw = true;
        return true;
    }

    return false;
}

/**
 * @brief Initialisiert Footer-Buttons.
 */
void WorkshopTool_LottoSystem::initFooterButtons(LGFX *tft)
{
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int x0 = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN;

    int cx = x0 + BTN_W_SYSTEM / 2;
    for (uint8_t i = 0; i < 5; ++i)
    {
        char label[4];
        stringutils::safeFormat(label, sizeof(label), "%03u", static_cast<unsigned>(i + 7));
        m_btnSystem[i].initButton(tft, cx, cy, BTN_W_SYSTEM, FOOTER_BTN_H, FOOTER_BTN_R, C_BUTTON_INACTIVE_OUTLINE, C_BUTTON_INACTIVE_FILL, C_BUTTON_INACTIVE_TEXT, label, nullptr);
        m_btnSystem[i].setFont(LeagueGothicCond34);
        cx += BTN_W_SYSTEM + FOOTER_BTN_GAP;
    }
    cx -= BTN_W_SYSTEM / 2;

    I18n::format(m_btnModeTicketLabel, sizeof(m_btnModeTicketLabel), WorkshopKey::WORKSHOP_LOTTO_BTN_TICKET);
    I18n::format(m_btnModeDrawLabel, sizeof(m_btnModeDrawLabel), WorkshopKey::WORKSHOP_LOTTO_BTN_DRAW);

    m_btnRandom.initButton(tft, cx + BTN_W_RANDOM / 2, cy, BTN_W_RANDOM, FOOTER_BTN_H, FOOTER_BTN_R, C_BUTTON_INACTIVE_OUTLINE, C_BUTTON_INACTIVE_FILL, C_BUTTON_INACTIVE_TEXT, "", nullptr);
    m_btnRandom.setFont(LeagueGothicCond34);
    m_btnRandom.setDrawCb(drawRandomButton);
    cx += BTN_W_RANDOM + FOOTER_BTN_GAP;

    m_btnClear.initButton(tft, cx + BTN_W_CLEAR / 2, cy, BTN_W_CLEAR, FOOTER_BTN_H, FOOTER_BTN_R, C_BUTTON_INACTIVE_OUTLINE, C_BUTTON_INACTIVE_FILL, C_BUTTON_INACTIVE_TEXT, "", nullptr);
    m_btnClear.setFont(LeagueGothicCond34);
    m_btnClear.setDrawCb(drawTrashButton);
    cx += BTN_W_CLEAR + FOOTER_BTN_GAP;

    m_btnMode.initButton(tft, cx + BTN_W_MODE / 2, cy, BTN_W_MODE, FOOTER_BTN_H, FOOTER_BTN_R, C_BUTTON_INACTIVE_OUTLINE, C_BUTTON_INACTIVE_FILL, C_BUTTON_INACTIVE_TEXT, m_btnModeDrawLabel, nullptr);
    m_btnMode.setFont(LeagueGothicCond34);
    cx += BTN_W_MODE + FOOTER_BTN_GAP;

    m_btnHelp.initButton(tft, cx + BTN_W_HELP / 2, cy, BTN_W_HELP, FOOTER_BTN_H, FOOTER_BTN_R, C_BUTTON_INACTIVE_OUTLINE, C_BUTTON_INACTIVE_FILL, C_BUTTON_INACTIVE_TEXT, "?", nullptr);
    m_btnHelp.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
    updateFooterButtonColors();
}

/**
 * @brief Aktualisiert Buttonfarben.
 */
void WorkshopTool_LottoSystem::updateFooterButtonColors()
{
    if (!m_buttonsInitialized)
        return;

    auto apply = [](Button &b, bool active)
    {
        b.setOutlineColor(active ? C_BUTTON_ACTIVE_OUTLINE : C_BUTTON_INACTIVE_OUTLINE);
        b.setFillColor(active ? C_BUTTON_ACTIVE_FILL : C_BUTTON_INACTIVE_FILL);
        b.setTextColor(active ? C_BUTTON_ACTIVE_TEXT : C_BUTTON_INACTIVE_TEXT);
    };

    for (uint8_t i = 0; i < 5; ++i)
        apply(m_btnSystem[i], i == m_systemIndex);

    apply(m_btnRandom, false);
    apply(m_btnClear, false);
    apply(m_btnMode, m_mode == InputMode::Draw);
    apply(m_btnHelp, false);

    m_btnMode.setLabelText((m_mode == InputMode::Ticket) ? m_btnModeDrawLabel : m_btnModeTicketLabel);
}

/**
 * @brief Zeichnet die Footer-Buttons.
 */
void WorkshopTool_LottoSystem::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    updateFooterButtonColors();
    for (uint8_t i = 0; i < 5; ++i)
        m_btnSystem[i].drawButton();

    m_btnRandom.drawButton();
    m_btnClear.drawButton();
    m_btnMode.drawButton();
    m_btnHelp.drawButton();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
 */
bool WorkshopTool_LottoSystem::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    if (m_superDialogVisible)
        return false;

    if (m_msg && m_msg->isVisible())
        return false;

    bool onSystem[5] = {};
    for (uint8_t i = 0; i < 5; ++i)
        onSystem[i] = touched && m_btnSystem[i].contains(tx, ty);

    const bool onRandom = touched && m_btnRandom.contains(tx, ty);
    const bool onClear = touched && m_btnClear.contains(tx, ty);
    const bool onMode = touched && m_btnMode.contains(tx, ty);
    const bool onHelp = touched && m_btnHelp.contains(tx, ty);

    bool wasSystem[5] = {};
    for (uint8_t i = 0; i < 5; ++i)
        wasSystem[i] = m_btnSystem[i].isPressed();

    const bool wasRandom = m_btnRandom.isPressed();
    const bool wasClear = m_btnClear.isPressed();
    const bool wasMode = m_btnMode.isPressed();
    const bool wasHelp = m_btnHelp.isPressed();

    for (uint8_t i = 0; i < 5; ++i)
        m_btnSystem[i].press(onSystem[i]);

    m_btnRandom.press(onRandom);
    m_btnClear.press(onClear);
    m_btnMode.press(onMode);
    m_btnHelp.press(onHelp);

    for (uint8_t i = 0; i < 5; ++i)
    {
        if (m_btnSystem[i].justReleased())
        {
            setSystemIndex(i);
            return true;
        }
    }

    if (m_btnRandom.justReleased())
    {
        fillRandom();
        return true;
    }

    if (m_btnClear.justReleased())
    {
        clearCurrentMode();
        return true;
    }

    if (m_btnMode.justReleased())
    {
        m_mode = (m_mode == InputMode::Ticket) ? InputMode::Draw : InputMode::Ticket;
        updateFooterButtonColors();
        saveState();
        return true;
    }

    if (m_btnHelp.justReleased())
    {
        showHelpDialog();
        return false;
    }

    for (uint8_t i = 0; i < 5; ++i)
    {
        if (onSystem[i] != wasSystem[i])
            m_btnSystem[i].drawButton(onSystem[i]);
    }

    if (onRandom != wasRandom)
        m_btnRandom.drawButton(onRandom);
    if (onClear != wasClear)
        m_btnClear.drawButton(onClear);
    if (onMode != wasMode)
        m_btnMode.drawButton(onMode);
    if (onHelp != wasHelp)
        m_btnHelp.drawButton(onHelp);

    return false;
}

/**
 * @brief Liefert einen zustandsabhängigen Untertitel.
 */
const char *WorkshopTool_LottoSystem::getSubtitle() const
{
    if (isTicketComplete() && isDrawComplete() && isSuperComplete())
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_LOTTO_SUB_READY);
    else if (m_mode == InputMode::Draw)
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_LOTTO_FMT_SUB_DRAW, static_cast<unsigned>(drawCount()));
    else
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_LOTTO_FMT_SUB_TICKET, static_cast<unsigned>(ticketCount()), static_cast<unsigned>(activeSystemSize()));

    return m_subtitleBuf;
}

/**
 * @brief Öffnet die Anleitung.
 */
void WorkshopTool_LottoSystem::showHelpDialog()
{
    if (!m_msg)
        return;

    I18n::format(m_helpTitleBuf, sizeof(m_helpTitleBuf), WorkshopKey::WORKSHOP_LOTTO_HELP_TITLE);
    I18n::format(m_helpTextBuf, sizeof(m_helpTextBuf), WorkshopKey::WORKSHOP_LOTTO_HELP_TEXT);
    I18n::format(m_dialogCloseBuf, sizeof(m_dialogCloseBuf), WorkshopKey::WORKSHOP_DIALOG_CLOSE);

    m_msg->setTitle(m_helpTitleBuf);
    m_msg->setMessage(m_helpTextBuf);
    m_msg->setButtonLabel(m_dialogCloseBuf);
    m_msg->setTimeout(0);
    m_msg->setAutoBounds(DIALOG_HELP_X, 0, DIALOG_HELP_W, 480);
    m_msg->setCallback([this](bool confirmed)
                       { onMsgResult(confirmed); });

    if (owner())
        owner()->setWorkContentPaused(true);

    m_msg->start();
}

/**
 * @brief Callback der MessageBox.
 */
void WorkshopTool_LottoSystem::onMsgResult(bool confirmed)
{
    (void)confirmed;

    if (m_buttonsInitialized)
    {
        for (uint8_t i = 0; i < 5; ++i)
            m_btnSystem[i].press(false);

        m_btnRandom.press(false);
        m_btnClear.press(false);
        m_btnMode.press(false);
        m_btnHelp.press(false);

        updateFooterButtonColors();
        for (uint8_t i = 0; i < 5; ++i)
            m_btnSystem[i].drawButton();

        m_btnRandom.drawButton();
        m_btnClear.drawButton();
        m_btnMode.drawButton();
        m_btnHelp.drawButton();
    }

    if (owner())
        owner()->setWorkContentPaused(false);
}
