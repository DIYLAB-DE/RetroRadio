#include "WorkshopTool_Dice.h"

#include <esp_random.h>

#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold30.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================
namespace
{
    // Animation
    constexpr uint32_t ANIM_DURATION_MS = 1200; ///< Gesamtdauer einer Wurf-Animation in Millisekunden

    // Layout des Würfelbereichs
    constexpr int TOP_GAP = 10;    ///< Vertikaler Abstand oben, symmetrisch zum Sidebar-Top
    constexpr int COLUMN_GAP = 14; ///< Abstand zwischen Würfelbereich und Sidebar

    constexpr int DIE_SIZE_SINGLE = 200; ///< Kantenlänge eines einzelnen Würfels im 1-Würfel-Modus
    constexpr int DIE_SIZE_DOUBLE = 148; ///< Kantenlänge jedes Würfels im 2-Würfel-Modus
    constexpr int DIE_GAP_DOUBLE = 30;   ///< Horizontaler Abstand zwischen den beiden Würfeln

    constexpr int DIE_CORNER_R_SINGLE = 22; ///< Eckenradius des Würfel-Korpus (1-Würfel-Modus)
    constexpr int DIE_CORNER_R_DOUBLE = 18; ///< Eckenradius des Würfel-Korpus (2-Würfel-Modus)

    constexpr int SHADOW_OFFSET_X = 4; ///< Horizontaler Schatten-Versatz nach rechts
    constexpr int SHADOW_OFFSET_Y = 7; ///< Vertikaler Schatten-Versatz nach unten

    constexpr int BORDER_THICKNESS = 3; ///< Stärke des dunklen Würfel-Rahmens (3D-Kante)

    // Layout der Sidebar (rechte Seite)
    constexpr int SIDE_W = 200;        ///< Breite der Sidebar
    constexpr int CARD_R = 8;          ///< Eckenradius der Info-Karten
    constexpr int CARD_GAP = 6;        ///< Vertikaler Abstand zwischen den Info-Karten
    constexpr int CARD_RESULT_H = 110; ///< Höhe der Ergebnis-Karte oben

    // Würfel-Farben
    constexpr uint16_t C_DIE_W_FILL = 0xE6D7;      ///< Würfel-Oberseite: warmes Creme / Elfenbein
    constexpr uint16_t C_DIE_W_OUTLINE = 0x8A65;   ///< Dunkle 3D-Kante: warmes Schokoladenbraun
    constexpr uint16_t C_DIE_W_PIP = 0x3921;       ///< Augen-Farbe: sehr dunkles Warmbraun
    constexpr uint16_t C_DIE_W_HIGHLIGHT = 0xFF5B; ///< Glanzlicht-Streifen oben: sehr helles Elfenbein

    // Würfel-Farben
    constexpr uint16_t C_DIE_R_FILL = 0xE902;      ///< Würfel-Oberseite: warmes Zinnober-Rot
    constexpr uint16_t C_DIE_R_OUTLINE = 0x7800;   ///< Dunkle 3D-Kante des roten Würfels
    constexpr uint16_t C_DIE_R_PIP = 0xFEDF;       ///< Augen-Farbe des roten Würfels (Creme-Weiß)
    constexpr uint16_t C_DIE_R_HIGHLIGHT = 0xF965; ///< Glanzlicht-Streifen oben (helles Rot)

    // Schatten unter dem Würfel
    constexpr uint16_t C_SHADOW = 0x0861; ///< Schatten-Farbe, etwas dunkler als der Tool-Hintergrund

    // Karten-Farben
    constexpr uint16_t C_CARD_BG = 0x2945;        ///< Hintergrund der Info-Karten
    constexpr uint16_t C_CARD_LABEL = 0xAD75;     ///< Label-Farbe in den Karten
    constexpr uint16_t C_CARD_VALUE = 0xFFFF;     ///< Werte-Farbe in den Karten (Weiß)
    constexpr uint16_t C_CARD_VALUE_DIM = 0xDEDB; ///< Gedimmte Werte-Farbe (Historie)
    constexpr uint16_t C_CARD_ACCENT = 0xFEA0;    ///< Akzent-Farbe für das Ergebnis (Gold)
    constexpr uint16_t C_CARD_ROLLING = 0xFD20;   ///< Akzent-Farbe während der Animation (Orange)

    // Toggle-Buttons
    constexpr uint16_t TOGGLE_ACTIVE_FILL = 0x328A;      ///< Füllfarbe aktiver Toggle-Buttons (TealMid)
    constexpr uint16_t TOGGLE_ACTIVE_TEXT = 0xFFFF;      ///< Textfarbe aktiver Toggle-Buttons
    constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;   ///< Umrissfarbe aktiver Toggle-Buttons
    constexpr uint16_t TOGGLE_INACTIVE_FILL = 0x0000;    ///< Füllfarbe inaktiver Toggle-Buttons
    constexpr uint16_t TOGGLE_INACTIVE_TEXT = 0x7BEF;    ///< Textfarbe inaktiver Toggle-Buttons
    constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8; ///< Umrissfarbe inaktiver Toggle-Buttons

    // Footer-Buttons
    constexpr int FOOTER_BTN_H = 44;      ///< Höhe der Footer-Buttons
    constexpr int FOOTER_BTN_R = 6;       ///< Eckenradius der Footer-Buttons
    constexpr int FOOTER_BTN_GAP = 4;     ///< Horizontaler Abstand zwischen Footer-Buttons
    constexpr int FOOTER_BTN_MARGIN = 2;  ///< Abstand des ersten Buttons zum linken Rand
    constexpr int FOOTER_BTN_W_MODE = 88; ///< Breite eines Modus-Toggles ("1 Würfel" / "2 Würfel")
    constexpr int FOOTER_BTN_W_HELP = 52; ///< Breite des Hilfe-Buttons

    // Hilfe-Dialog
    constexpr int DIALOG_HELP_W = 680;                       ///< Breite des Hilfe-Dialogs
    constexpr int DIALOG_HELP_X = (800 - DIALOG_HELP_W) / 2; ///< X-Position des Hilfe-Dialogs (zentriert)

    // Hilfsfunktionen

    /**
     * @brief Klemmt einen Wert ins angegebene Intervall.
     */
    inline int clampi(int v, int lo, int hi)
    {
        if (v < lo)
            return lo;

        if (v > hi)
            return hi;

        return v;
    }

} // namespace

// ===================================================================
// Lifecycle
// ===================================================================

/**
 * @brief Initialisiert Zustand, Footer-Buttons und MessageBox.
 */
void WorkshopTool_Dice::onEnter(LGFX *tft)
{
    m_twoDice = false;
    m_hasResult = false;
    m_valFinal[0] = 1;
    m_valFinal[1] = 1;

    m_animating = false;
    m_animStartMs = 0;
    m_animFakeVal[0] = 1;
    m_animFakeVal[1] = 1;

    m_historyCount = 0;

    m_touchActive = false;
    m_touchLastX = 0;
    m_touchLastY = 0;

    if (!m_msg)
        m_msg.reset(new MessageBox(tft));

    if (!m_buttonsInitialized)
    {
        constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
        constexpr int x0 = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN;
        constexpr int cx1 = x0 + FOOTER_BTN_W_MODE / 2;
        constexpr int cx2 = cx1 + FOOTER_BTN_W_MODE + FOOTER_BTN_GAP;
        constexpr int cx3 = cx2 + FOOTER_BTN_W_MODE / 2 + FOOTER_BTN_GAP + FOOTER_BTN_W_HELP / 2;

        I18n::format(m_btnOneLabel, sizeof(m_btnOneLabel), WorkshopKey::WORKSHOP_DICE_BTN_ONE);
        I18n::format(m_btnTwoLabel, sizeof(m_btnTwoLabel), WorkshopKey::WORKSHOP_DICE_BTN_TWO);

        m_btnOne.initButton(
            tft,                   // Display-Objekt
            cx1,                   // X-Position des Button-Zentrums
            cy,                    // Y-Position des Button-Zentrums
            FOOTER_BTN_W_MODE,     // Breite des Buttons
            FOOTER_BTN_H,          // Höhe des Buttons
            FOOTER_BTN_R,          // Radius der abgerundeten Ecken
            TOGGLE_ACTIVE_OUTLINE, // Umrissfarbe (aktiv, da 1-Würfel-Startmodus)
            TOGGLE_ACTIVE_FILL,    // Füllfarbe (aktiv)
            TOGGLE_ACTIVE_TEXT,    // Textfarbe (aktiv)
            m_btnOneLabel,         // Button-Label
            nullptr                // kein Icon
        );
        m_btnOne.setFont(LeagueGothicCond34);

        m_btnTwo.initButton(
            tft,                     // Display-Objekt
            cx2,                     // X-Position des Button-Zentrums
            cy,                      // Y-Position des Button-Zentrums
            FOOTER_BTN_W_MODE,       // Breite des Buttons
            FOOTER_BTN_H,            // Höhe des Buttons
            FOOTER_BTN_R,            // Radius der abgerundeten Ecken
            TOGGLE_INACTIVE_OUTLINE, // Umrissfarbe (inaktiv)
            TOGGLE_INACTIVE_FILL,    // Füllfarbe (inaktiv)
            TOGGLE_INACTIVE_TEXT,    // Textfarbe (inaktiv)
            m_btnTwoLabel,           // Button-Label
            nullptr                  // kein Icon
        );
        m_btnTwo.setFont(LeagueGothicCond34);

        m_btnHelp.initButton(
            tft,                     // Display-Objekt
            cx3,                     // X-Position des Button-Zentrums
            cy,                      // Y-Position des Button-Zentrums
            FOOTER_BTN_W_HELP,       // Breite des Buttons
            FOOTER_BTN_H,            // Höhe des Buttons
            FOOTER_BTN_R,            // Radius der abgerundeten Ecken
            TOGGLE_INACTIVE_OUTLINE, // Umrissfarbe
            TOGGLE_INACTIVE_FILL,    // Füllfarbe
            TOGGLE_INACTIVE_TEXT,    // Textfarbe
            "?",                     // Button-Label
            nullptr                  // kein Icon
        );
        m_btnHelp.setFont(LeagueGothicCond34);

        m_buttonsInitialized = true;
    }

    updateModeButtons();
}

/**
 * @brief Räumt den Werkzeugzustand auf.
 */
void WorkshopTool_Dice::onExit()
{
    m_msg.reset();
    m_buttonsInitialized = false;

    m_touchActive = false;
    m_touchLastX = 0;
    m_touchLastY = 0;

    m_animating = false;
}

// ===================================================================
// Wurf-Logik
// ===================================================================

/**
 * @brief Liefert eine gleichverteilte Augenzahl 1 bis 6 aus dem Hardware-RNG.
 */
uint8_t WorkshopTool_Dice::rollDie() const
{
    // Rejection-Sampling: Der größte durch 6 teilbare 32-Bit-Bereich liegt unter (limit). Werte ab limit werden verworfen, damit der Modulo-Bias verschwindet. Die Verwerfungsrate ist praktisch null (nur 4 von ca. 4,29 Milliarden Werten).
    constexpr uint32_t RANGE = 0xFFFFFFFFu - (0xFFFFFFFFu % 6u);

    uint32_t v;
    do
    {
        v = esp_random();
    } while (v >= RANGE);

    return static_cast<uint8_t>((v % 6u) + 1u);
}

/**
 * @brief Startet die Wurf-Animation und legt die finalen Augenzahlen fest.
 */
void WorkshopTool_Dice::startRoll()
{
    // Finalwert sofort bestimmen, damit die Animation rein kosmetisch ist und kein weiterer RNG-Call das sichtbare Ergebnis beeinflusst.
    m_valFinal[0] = rollDie();
    m_valFinal[1] = m_twoDice ? rollDie() : 1;

    // Initialer Zwischenwert: einfach der finale Wert. Der erste Animations-Tick überschreibt ihn sofort mit einer zufälligen Fake-Zahl.
    m_animFakeVal[0] = m_valFinal[0];
    m_animFakeVal[1] = m_valFinal[1];

    m_animating = true;
    m_animStartMs = millis();
}

/**
 * @brief Schiebt einen neuen Wurf als neuesten Eintrag in die Historie.
 */
void WorkshopTool_Dice::pushHistory(uint8_t count, uint8_t v0, uint8_t v1)
{
    // Bestehende Einträge um einen Slot nach hinten verschieben, neuester landet in Slot 0.
    for (int i = HISTORY_CAPACITY - 1; i > 0; --i)
        m_history[i] = m_history[i - 1];

    m_history[0].count = count;
    m_history[0].values[0] = v0;
    m_history[0].values[1] = v1;

    if (m_historyCount < HISTORY_CAPACITY)
        m_historyCount++;
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Ereignisse auf dem Würfelbereich.
 */
bool WorkshopTool_Dice::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    // Solange die MessageBox sichtbar ist, schluckt sie alle Brett-Touches und erhält nur den regelmäßigen Tick für ihre Timeout-Logik.
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

    // Ein Tap während einer laufenden Animation wird ignoriert, damit sich die Würfel nicht mitten im Rollen neu bestücken.
    if (m_animating)
        return false;

    if (!hitTestDiceArea(m_touchLastX, m_touchLastY))
        return false;

    startRoll();

    // Sofort-Redraw anfordern, damit das erste Anim-Frame ohne Wartezeit auf den Cursor-Blink gezeichnet wird.
    return true;
}

/**
 * @brief Prüft, ob absolute Touch-Koordinaten im Würfelbereich liegen.
 */
bool WorkshopTool_Dice::hitTestDiceArea(int32_t tx, int32_t ty) const
{
    // In tool-relative Koordinaten umrechnen.
    const int relX = static_cast<int>(tx) - WorkshopCfg::WORK_X - m_originX;
    const int relY = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_originY;

    if (relX < 0 || relY < 0)
        return false;

    // Würfelbereich ist alles links der Sidebar: [0 .. (contentWidth - SIDE_W - COLUMN_GAP)]. Der gesamte Bereich gilt als tap-aktiv, damit der Nutzer nicht exakt den Würfel-Korpus treffen muss.
    constexpr int CONTENT_W = 547;
    constexpr int DICE_AREA_W = CONTENT_W - SIDE_W - COLUMN_GAP;
    constexpr int DICE_AREA_H = 327;

    if (relX >= DICE_AREA_W || relY >= DICE_AREA_H)
        return false;

    return true;
}

// ===================================================================
// Zeichnen
// ===================================================================

/**
 * @brief Zeichnet Würfel und Sidebar in den Work-Sprite.
 */
void WorkshopTool_Dice::draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h)
{
    (void)w;
    (void)h;

    if (!sprite)
        return;

    // Solange die MessageBox sichtbar ist, nicht ins Sprite zeichnen. Der nächste Workshop-Push würde die Box sonst überschreiben.
    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();

        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);

        return;
    }

    m_originX = ox;
    m_originY = oy;

    // Animation tickt innerhalb der draw()-Aufrufe: jeder Redraw während der Animation erzeugt einen neuen Fake-Wert und schreibt den Endwert zurück, sobald die Dauer überschritten ist.
    if (m_animating)
    {
        const uint32_t elapsed = millis() - m_animStartMs;

        if (elapsed >= ANIM_DURATION_MS)
        {
            // Animation abschließen und Wurf in die Historie buchen.
            m_animating = false;
            m_hasResult = true;
            pushHistory(m_twoDice ? 2 : 1, m_valFinal[0], m_valFinal[1]);
        }
        else
        {
            // Während des Rollens zufällige Fake-Werte anzeigen, die sich klar von den Endwerten unterscheiden. Falls die Zufallszahl versehentlich mit dem Finalwert kollidiert, wird sie auf den nächsten Wert verschoben, damit zwischen den drei Zwischenschritten tatsächlich eine sichtbare Bewegung liegt.
            uint8_t f0 = rollDie();
            if (f0 == m_valFinal[0])
                f0 = (f0 % 6u) + 1u;

            m_animFakeVal[0] = f0;

            if (m_twoDice)
            {
                uint8_t f1 = rollDie();
                if (f1 == m_valFinal[1])
                    f1 = (f1 % 6u) + 1u;

                m_animFakeVal[1] = f1;
            }
        }
    }

    drawDiceArea(sprite);
    drawSidebar(sprite);
}

/**
 * @brief Zeichnet den Würfelbereich inklusive Animations-Effekte.
 */
void WorkshopTool_Dice::drawDiceArea(Gfx_Sprite *sprite)
{
    // Verfügbarer Bereich links der Sidebar.
    constexpr int CONTENT_W = 547;
    constexpr int DICE_AREA_W = CONTENT_W - SIDE_W - COLUMN_GAP;

    // Augenzahlen, die gerade gezeichnet werden sollen. Während der Animation sind das die schnell wechselnden Fake-Werte, danach die finalen Augenzahlen.
    const uint8_t v0 = m_animating ? m_animFakeVal[0] : m_valFinal[0];
    const uint8_t v1 = m_animating ? m_animFakeVal[1] : m_valFinal[1];

    // Center-Y einheitlich: Würfel werden vertikal mittig über die Netto-Höhe 327 gesetzt, abzüglich TOP_GAP als oberer Luftanteil.
    const int areaCenterX = m_originX + DICE_AREA_W / 2;
    const int areaCenterY = m_originY + 163; // 10 (TOP_GAP) + 307/2 gerundet

    if (m_twoDice)
    {
        constexpr int CENTER_STRIDE = DIE_SIZE_DOUBLE + DIE_GAP_DOUBLE;
        const int cx0 = areaCenterX - CENTER_STRIDE / 2;
        const int cx1 = areaCenterX + CENTER_STRIDE / 2;

        drawSingleDie(sprite, cx0, areaCenterY, DIE_SIZE_DOUBLE, v0, false); // Würfel links (heller Korpus)
        drawSingleDie(sprite, cx1, areaCenterY, DIE_SIZE_DOUBLE, v1, true);  // Würfel rechts (roter Korpus)
    }
    else
    {
        drawSingleDie(sprite, areaCenterX, areaCenterY, DIE_SIZE_SINGLE, v0, false);
    }
}

/**
 * @brief Zeichnet einen einzelnen Würfel mit Schatten, Hülle, Glanzlicht und Augen.
 */
void WorkshopTool_Dice::drawSingleDie(Gfx_Sprite *sprite, int cx, int cy, int size, uint8_t value, bool redAccent)
{
    const uint16_t colFill = redAccent ? C_DIE_R_FILL : C_DIE_W_FILL;
    const uint16_t colOutline = redAccent ? C_DIE_R_OUTLINE : C_DIE_W_OUTLINE;
    const uint16_t colPip = redAccent ? C_DIE_R_PIP : C_DIE_W_PIP;
    const uint16_t colHighlight = redAccent ? C_DIE_R_HIGHLIGHT : C_DIE_W_HIGHLIGHT;

    const int cornerR = (size >= DIE_SIZE_SINGLE - 4) ? DIE_CORNER_R_SINGLE : DIE_CORNER_R_DOUBLE;

    const int x0 = cx - size / 2;
    const int y0 = cy - size / 2;

    // Schatten (leicht nach unten-rechts, gleiche Form)
    sprite->fillSmoothRoundRect(x0 + SHADOW_OFFSET_X, y0 + SHADOW_OFFSET_Y, size, size, cornerR, C_SHADOW);

    // Dunkle Kante als Unterlage: etwas größer als die Füllfläche, deutet die 3D-Kante des Würfels an.
    sprite->fillSmoothRoundRect(x0, y0, size, size, cornerR, colOutline);

    // Eigentliche Würfel-Fläche (inset um BORDER_THICKNESS)
    const int innerSize = size - 2 * BORDER_THICKNESS;
    const int innerCorner = (cornerR > BORDER_THICKNESS) ? (cornerR - BORDER_THICKNESS) : 2;
    sprite->fillSmoothRoundRect(x0 + BORDER_THICKNESS, y0 + BORDER_THICKNESS, innerSize, innerSize, innerCorner, colFill);

    // Glanzlicht: dünner heller Streifen im oberen Bereich der Oberseite, schafft die Anmutung einer beleuchteten Kante.
    const int hlInset = size / 6;
    const int hlW = size - 2 * hlInset;
    const int hlH = 4;
    const int hlY = y0 + BORDER_THICKNESS + 4;
    sprite->fillSmoothRoundRect(x0 + hlInset, hlY, hlW, hlH, 2, colHighlight);

    // Augen nach klassischem Muster
    drawPips(sprite, cx, cy, size, value, colPip);
}

/**
 * @brief Zeichnet die Augen nach dem klassischen Sechs-Punkte-Schema.
 */
void WorkshopTool_Dice::drawPips(Gfx_Sprite *sprite, int cx, int cy, int size, uint8_t value, uint16_t pipCol)
{
    // Abstand der Rand-Augen zum Würfelrand (vom Würfel-Zentrum aus).
    const int offset = static_cast<int>(size * 0.28f);
    // Radius eines einzelnen Auges.
    const int pipR = clampi(size / 12, 4, 16);

    // Sieben mögliche Positionen, selektiv angesprochen je nach Augenzahl.
    const int xl = cx - offset; // links
    const int xc = cx;          // zentriert
    const int xr = cx + offset; // rechts
    const int yt = cy - offset; // oben
    const int yc = cy;          // mitte
    const int yb = cy + offset; // unten

    auto pip = [&](int x, int y)
    {
        sprite->fillSmoothCircle(x, y, pipR, pipCol);
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

    case 5:
        pip(xl, yt);
        pip(xr, yt);
        pip(xc, yc);
        pip(xl, yb);
        pip(xr, yb);
        break;

    case 6:
        pip(xl, yt);
        pip(xr, yt);
        pip(xl, yc);
        pip(xr, yc);
        pip(xl, yb);
        pip(xr, yb);
        break;

    default:
        // Defensive: Werte außerhalb 1..6 sollten nicht vorkommen. Falls doch, fallback auf 1.
        pip(xc, yc);
        break;
    }
}

/**
 * @brief Zeichnet die rechte Sidebar mit Ergebnis- und Historie-Karte.
 */
void WorkshopTool_Dice::drawSidebar(Gfx_Sprite *sprite)
{
    constexpr int CONTENT_W = 547;
    constexpr int CONTENT_H = 327;

    const int sideX = m_originX + CONTENT_W - SIDE_W;
    const int sideY = m_originY + TOP_GAP;

    const int resultY = sideY;
    const int historyY = resultY + CARD_RESULT_H + CARD_GAP;
    const int historyH = CONTENT_H - TOP_GAP - CARD_RESULT_H - CARD_GAP - TOP_GAP;

    // Karten-Hintergründe
    sprite->fillSmoothRoundRect(sideX, resultY, SIDE_W, CARD_RESULT_H, CARD_R, C_CARD_BG);
    sprite->fillSmoothRoundRect(sideX, historyY, SIDE_W, historyH, CARD_R, C_CARD_BG);

    // Ergebnis-Karte
    // Label oben zentriert
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->setTextDatum(TC_DATUM);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_DICE_LABEL_RESULT), sideX + SIDE_W / 2, resultY + 8);
    sprite->unloadFont();

    // Großer Wert in der Mitte, gold während Ergebnis stabil, orange während Animation. Im 1-Würfel-Modus wird die einzelne Augenzahl gezeigt, im 2-Würfel-Modus die Summe.
    const uint16_t valueCol = m_animating ? C_CARD_ROLLING : C_CARD_ACCENT;

    sprite->loadFont(ComfortaaBold30);
    sprite->setTextColor(valueCol, C_CARD_BG);
    sprite->setTextDatum(MC_DATUM);

    char buf[32];
    const int bigValY = resultY + CARD_RESULT_H / 2 + 2;

    if (!m_hasResult && !m_animating)
    {
        sprite->drawString("-", sideX + SIDE_W / 2, bigValY);
    }
    else
    {
        const uint8_t v0 = m_animating ? m_animFakeVal[0] : m_valFinal[0];
        const uint8_t v1 = m_animating ? m_animFakeVal[1] : m_valFinal[1];
        const unsigned sum = m_twoDice ? static_cast<unsigned>(v0 + v1) : static_cast<unsigned>(v0);

        stringutils::safeFormat(buf, "%u", sum);
        sprite->drawString(buf, sideX + SIDE_W / 2, bigValY);
    }

    sprite->unloadFont();

    // Im 2-Würfel-Modus unter der großen Zahl die Einzelwerte als "a + b" nachführen.
    if (m_twoDice && (m_hasResult || m_animating))
    {
        const uint8_t v0 = m_animating ? m_animFakeVal[0] : m_valFinal[0];
        const uint8_t v1 = m_animating ? m_animFakeVal[1] : m_valFinal[1];

        stringutils::safeFormat(buf, "%u + %u", static_cast<unsigned>(v0), static_cast<unsigned>(v1));

        sprite->loadFont(ComfortaaBold14);
        sprite->setTextColor(C_CARD_VALUE_DIM, C_CARD_BG);
        sprite->setTextDatum(TC_DATUM);
        sprite->drawString(buf, sideX + SIDE_W / 2, resultY + CARD_RESULT_H - 22);
        sprite->unloadFont();
    }

    // Historie-Karte
    // Label oben
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextColor(C_CARD_LABEL, C_CARD_BG);
    sprite->setTextDatum(TC_DATUM);
    sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_DICE_LABEL_HISTORY), sideX + SIDE_W / 2, historyY + 8);
    sprite->unloadFont();

    // Einträge darunter als Textzeilen
    if (m_historyCount == 0)
    {
        sprite->loadFont(ComfortaaBold14);
        sprite->setTextColor(C_CARD_VALUE_DIM, C_CARD_BG);
        sprite->setTextDatum(MC_DATUM);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_DICE_HISTORY_EMPTY), sideX + SIDE_W / 2, historyY + historyH / 2 + 6);
        sprite->unloadFont();
    }
    else
    {
        sprite->loadFont(ComfortaaBold16);
        sprite->setTextDatum(MC_DATUM);

        const int rowY0 = historyY + 30;
        const int rowH = 18;
        const int maxVisible = (historyH - 36) / rowH;
        const int n = (m_historyCount < maxVisible) ? m_historyCount : maxVisible;

        for (int i = 0; i < n; ++i)
        {
            const RollEntry &e = m_history[i];

            // Der jüngste Eintrag (Slot 0) steht oben und wird heller gerendert, ältere dunkler.
            const uint16_t col = (i == 0) ? C_CARD_VALUE : C_CARD_VALUE_DIM;
            sprite->setTextColor(col, C_CARD_BG);

            if (e.count == 1)
                stringutils::safeFormat(buf, "%u", static_cast<unsigned>(e.values[0]));
            else
                stringutils::safeFormat(buf, "%u + %u = %u", static_cast<unsigned>(e.values[0]), static_cast<unsigned>(e.values[1]), static_cast<unsigned>(e.values[0] + e.values[1]));

            sprite->drawString(buf, sideX + SIDE_W / 2, rowY0 + i * rowH + rowH / 2);
        }

        sprite->unloadFont();
    }
}

// ===================================================================
// Footer
// ===================================================================

/**
 * @brief Aktualisiert die Toggle-Farben der beiden Modus-Buttons.
 */
void WorkshopTool_Dice::updateModeButtons()
{
    auto apply = [](Button &b, bool active)
    {
        b.setOutlineColor(active ? TOGGLE_ACTIVE_OUTLINE : TOGGLE_INACTIVE_OUTLINE);
        b.setFillColor(active ? TOGGLE_ACTIVE_FILL : TOGGLE_INACTIVE_FILL);
        b.setTextColor(active ? TOGGLE_ACTIVE_TEXT : TOGGLE_INACTIVE_TEXT);
    };

    apply(m_btnOne, !m_twoDice);
    apply(m_btnTwo, m_twoDice);
}

/**
 * @brief Zeichnet die Footer-Buttons (Modus-Toggle und Hilfe).
 */
void WorkshopTool_Dice::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    m_btnOne.drawButton();
    m_btnTwo.drawButton();
    m_btnHelp.drawButton();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
 */
bool WorkshopTool_Dice::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    if (m_msg && m_msg->isVisible())
        return false;

    const bool onOne = touched && m_btnOne.contains(tx, ty);
    const bool onTwo = touched && m_btnTwo.contains(tx, ty);
    const bool onHelp = touched && m_btnHelp.contains(tx, ty);

    const bool wasOne = m_btnOne.isPressed();
    const bool wasTwo = m_btnTwo.isPressed();
    const bool wasHelp = m_btnHelp.isPressed();

    m_btnOne.press(onOne);
    m_btnTwo.press(onTwo);
    m_btnHelp.press(onHelp);

    // Aktion bei Loslassen
    if (m_btnOne.justReleased())
    {
        if (m_twoDice)
        {
            m_twoDice = false;
            // Laufenden Wurf verwerfen, ein Modus-Wechsel ist ein sauberer Schnitt.
            m_animating = false;
            updateModeButtons();
            return true;
        }
        return false;
    }

    if (m_btnTwo.justReleased())
    {
        if (!m_twoDice)
        {
            m_twoDice = true;
            m_animating = false;
            updateModeButtons();
            return true;
        }
        return false;
    }

    if (m_btnHelp.justReleased())
    {
        showHelpDialog();
        return false;
    }

    // Press-Rückmeldung ohne Aktion
    if (onOne != wasOne)
        m_btnOne.drawButton(onOne);

    if (onTwo != wasTwo)
        m_btnTwo.drawButton(onTwo);

    if (onHelp != wasHelp)
        m_btnHelp.drawButton(onHelp);

    return false;
}

/**
 * @brief Liefert einen modusabhängigen Untertitel.
 */
const char *WorkshopTool_Dice::getSubtitle() const
{
    if (m_animating)
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_DICE_SUB_ROLLING);
    else if (m_twoDice)
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_DICE_SUB_TWO_DICE);
    else
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf), WorkshopKey::WORKSHOP_DICE_SUB_ONE_DIE);

    return m_subtitleBuf;
}

// ===================================================================
// MessageBox-Dialog
// ===================================================================

/**
 * @brief Öffnet die Kurzanleitung in einer modalen MessageBox.
 */
void WorkshopTool_Dice::showHelpDialog()
{
    if (!m_msg)
        return;

    I18n::format(m_helpTitleBuf, sizeof(m_helpTitleBuf), WorkshopKey::WORKSHOP_DICE_HELP_TITLE);
    I18n::format(m_helpTextBuf, sizeof(m_helpTextBuf), WorkshopKey::WORKSHOP_DICE_HELP_TEXT);
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
void WorkshopTool_Dice::onMsgResult(bool confirmed)
{
    (void)confirmed;

    // Footer-Buttons in den Ruhezustand zurückversetzen und neu zeichnen, damit ein während des Dialog-Tippens gehaltener Button-Zustand nicht nachwirkt.
    if (m_buttonsInitialized)
    {
        m_btnOne.press(false);
        m_btnTwo.press(false);
        m_btnHelp.press(false);

        m_btnOne.drawButton();
        m_btnTwo.drawButton();
        m_btnHelp.drawButton();
    }

    if (owner())
        owner()->setWorkContentPaused(false);
}
