#include "WorkshopTool_SiPrefixes.h"
#include "I18n.h"
#include "Workshop.h"
#include "StringUtils.h"

#include "fonts/ComfortaaBold16.h"

// ===================================================================
// STATISCHE DATENBASIS
// ===================================================================

const WorkshopTool_SiPrefixes::SiPrefix
WorkshopTool_SiPrefixes::s_prefixes[] = {
    {WorkshopKey::WORKSHOP_SI_PREFIXES_NAME_TERA,  "T",  12, "1 THz, 1 TB"},
    {WorkshopKey::WORKSHOP_SI_PREFIXES_NAME_GIGA,  "G",   9, "1 GHz, 1 GB"},
    {WorkshopKey::WORKSHOP_SI_PREFIXES_NAME_MEGA,  "M",   6, "1 MHz, 1 M\xCE\xA9"},
    {WorkshopKey::WORKSHOP_SI_PREFIXES_NAME_KILO,  "k",   3, "1 k\xCE\xA9, 1 kHz"},
    {WorkshopKey::WORKSHOP_SI_PREFIXES_NAME_BASE,  "",    0, "1 \xCE\xA9, 1 Hz, 1 V"},
    {WorkshopKey::WORKSHOP_SI_PREFIXES_NAME_MILLI, "m",  -3, "1 mA, 1 mV"},
    {WorkshopKey::WORKSHOP_SI_PREFIXES_NAME_MICRO, "\xC2\xB5", -6, "1 \xC2\xB5\x46, 1 \xC2\xB5\x48"},
    {WorkshopKey::WORKSHOP_SI_PREFIXES_NAME_NANO,  "n",  -9, "1 nF, 1 nH"},
    {WorkshopKey::WORKSHOP_SI_PREFIXES_NAME_PICO,  "p", -12, "1 pF, 1 ps"},
};

const int WorkshopTool_SiPrefixes::PREFIX_COUNT =
    static_cast<int>(sizeof(WorkshopTool_SiPrefixes::s_prefixes) /
                     sizeof(WorkshopTool_SiPrefixes::s_prefixes[0]));

// ===================================================================
// Anonymer Namespace: Layoutkonstanten und Farben
// ===================================================================

namespace
{
    // ===================================================================
    // Höhen
    // ===================================================================

    // Verfügbare Gesamthöhe h = 327 px:
    //   TOP_PAD(10) + HEADER_H(28) + PREFIX_COUNT(9)×ROW_H(28)
    //               + SEP(1) + INFO_H(36) = 327
    constexpr int TOP_PAD  = 10; ///< Oberer Abstand vor der Kopfzeile
    constexpr int HEADER_H = 28; ///< Höhe der Kopfzeile (Spaltentitel)
    constexpr int ROW_H    = 28; ///< Höhe einer Datenzeile
    constexpr int INFO_H   = 36; ///< Höhe des Info-Balkens
    constexpr int ACCENT_W =  3; ///< Breite des Selektions-Akzentbalkens

    // ===================================================================
    // Spaltenbreiten und X-Offsets (alle relativ zu ox)
    // ===================================================================

    // Gesamtbreite w = 547 px:
    //   INNER_PAD(6) + COL0(110) + SEP(1) + COL1(64) + SEP(1)
    //               + COL2(108) + SEP(1) + COL3(250) + INNER_PAD(6) = 547
    constexpr int INNER_PAD = 6;   ///< Linker und rechter Innenrand
    constexpr int COL0_W    = 110; ///< Spaltenbreite: Präfix-Name
    constexpr int COL1_W    = 64;  ///< Spaltenbreite: Symbol
    constexpr int COL2_W    = 108; ///< Spaltenbreite: Zehnerpotenz
    // COL3_W = 547 - 6 - 110 - 1 - 64 - 1 - 108 - 1 - 6 = 250 px

    constexpr int SEP1_OX = INNER_PAD + COL0_W;           ///< Trennlinie 1 bei ox + 116
    constexpr int SEP2_OX = SEP1_OX + 1 + COL1_W;         ///< Trennlinie 2 bei ox + 181
    constexpr int SEP3_OX = SEP2_OX + 1 + COL2_W;         ///< Trennlinie 3 bei ox + 290

    constexpr int NAME_TEXT_OX  = INNER_PAD + 5;           ///< ML_DATUM: ox + 11
    constexpr int SYM_CENTER_OX = SEP1_OX + 1 + COL1_W/2; ///< MC_DATUM: ox + 149
    constexpr int POT_CENTER_OX = SEP2_OX + 1 + COL2_W/2; ///< MC_DATUM: ox + 236
    constexpr int EX_TEXT_OX    = SEP3_OX + 1 + 5;        ///< ML_DATUM: ox + 296

    // ===================================================================
    // Farben
    // ===================================================================
    constexpr uint16_t C_HDR_BG   = 0x2945; ///< Kopfzeilen-Hintergrund   (= C_SEP_LINE)
    constexpr uint16_t C_HDR_FG   = 0xB5F7; ///< Kopfzeilen-Text          (= C_WORK_SUB)
    constexpr uint16_t C_ROW_A    = 0x1082; ///< Gerade Datenzeile         (= C_WORK_BG)
    constexpr uint16_t C_ROW_B    = 0x2104; ///< Ungerade Datenzeile       (= C_WORK_TITLE_BG)
    constexpr uint16_t C_BASE_BG  = 0x2965; ///< Basis-Zeile (Exponent 0)
    constexpr uint16_t C_SEL_BG   = 0x034A; ///< Ausgewählte Zeile (dunkles Cyan)
    constexpr uint16_t C_ACCENT   = 0x04FF; ///< Akzentbalken              (= C_SEC_UNITS)
    constexpr uint16_t C_FG       = 0xFFFF; ///< Normaler Text (weiß)
    constexpr uint16_t C_FG_POS   = 0x47E4; ///< Positiver Exponent (Grün)
    constexpr uint16_t C_FG_NEG   = 0x3DBF; ///< Negativer Exponent (Hellblau)
    constexpr uint16_t C_FG_BASE  = 0xC618; ///< Basis-Exponent (Hellgrau)
    constexpr uint16_t C_FG_EX    = 0x8C51; ///< Beispiel-Text (Mittelgrau)
    constexpr uint16_t C_SEP      = 0x2945; ///< Trennlinien               (= C_SEP_LINE)
    constexpr uint16_t C_INFO_BG  = 0x10A3; ///< Info-Balken-Hintergrund
    constexpr uint16_t C_INFO_FG  = 0xFFFF; ///< Info-Text (weiß)
    constexpr uint16_t C_INFO_DIM = 0x5AEB; ///< Separator-Mittelpunkt (gedimmt)
    constexpr uint16_t C_INFO_HINT= 0x8C51; ///< Hinweistext ohne Selektion

} // namespace

// ===================================================================
// LIFECYCLE
// ===================================================================

/**
 * @brief Setzt den internen Zustand zurück.
 */
void WorkshopTool_SiPrefixes::onEnter(LGFX *tft)
{
    (void)tft;
    m_selectedRow = -1;
    m_pressedRow  = -1;
    m_touchActive = false;
    m_ox = m_oy = m_w = m_h = 0;
}

/**
 * @brief Ressourcenfreigabe (keine dynamischen Ressourcen vorhanden).
 */
void WorkshopTool_SiPrefixes::onExit()
{
    // keine dynamischen Ressourcen
}

// ===================================================================
// ZEICHNEN
// ===================================================================

/**
 * @brief Zeichnet Tabelle und Info-Balken in den Work-Sprite.
 */
void WorkshopTool_SiPrefixes::draw(Gfx_Sprite *spr, int ox, int oy, int w, int h)
{
    m_ox = ox;
    m_oy = oy;
    m_w  = w;
    m_h  = h;

    // Tabelle beginnt TOP_PAD px unterhalb des Werkzeug-Ursprungs
    const int tabY   = oy + TOP_PAD;
    const int tableH = HEADER_H + PREFIX_COUNT * ROW_H;
    const int infoY  = tabY + tableH + 1; // +1 für Trennlinie

    drawTable(spr, ox, tabY, w);
    spr->drawFastHLine(ox, tabY + tableH, w, C_SEP);
    drawInfoBar(spr, ox, infoY, w, INFO_H);
}

/**
 * @brief Zeichnet Hintergründe, Trennlinien und Text der Präfix-Tabelle.
 */
void WorkshopTool_SiPrefixes::drawTable(Gfx_Sprite *spr, int ox, int tabY, int w) const
{
    // ===================================================================
    // 1. Zeilenhintergründe
    // ===================================================================

    spr->fillRect(ox, tabY, w, HEADER_H, C_HDR_BG);

    for (int i = 0; i < PREFIX_COUNT; ++i)
    {
        const int ry = tabY + HEADER_H + i * ROW_H;

        uint16_t bg;
        if (i == static_cast<int>(m_selectedRow))
            bg = C_SEL_BG;
        else if (s_prefixes[i].exponent == 0)
            bg = C_BASE_BG;
        else
            bg = (i % 2 == 0) ? C_ROW_A : C_ROW_B;

        spr->fillRect(ox, ry, w, ROW_H, bg);

        if (i == static_cast<int>(m_selectedRow))
            spr->fillRect(ox, ry, ACCENT_W, ROW_H, C_ACCENT);
    }

    // ===================================================================
    // 2. Trennlinien
    // ===================================================================

    const int tableH = HEADER_H + PREFIX_COUNT * ROW_H;

    spr->drawFastHLine(ox,           tabY + HEADER_H, w,      C_SEP);
    spr->drawFastVLine(ox + SEP1_OX, tabY,            tableH, C_SEP);
    spr->drawFastVLine(ox + SEP2_OX, tabY,            tableH, C_SEP);
    spr->drawFastVLine(ox + SEP3_OX, tabY,            tableH, C_SEP);

    // ===================================================================
    // 3. Text (ein Font-Load für die gesamte Tabelle)
    // ===================================================================

    spr->loadFont(ComfortaaBold16);

    // Kopfzeilen-Beschriftungen
    {
        const int hdrCy = tabY + HEADER_H / 2;
        spr->setTextColor(C_HDR_FG, C_HDR_BG);

        spr->setTextDatum(ML_DATUM);
        spr->drawString(I18n::str(WorkshopKey::WORKSHOP_SI_PREFIXES_HDR_PREFIX),   ox + NAME_TEXT_OX,  hdrCy);

        spr->setTextDatum(MC_DATUM);
        spr->drawString(I18n::str(WorkshopKey::WORKSHOP_SI_PREFIXES_HDR_SYMBOL),   ox + SYM_CENTER_OX, hdrCy);
        spr->drawString(I18n::str(WorkshopKey::WORKSHOP_SI_PREFIXES_HDR_POWER),    ox + POT_CENTER_OX, hdrCy);

        spr->setTextDatum(ML_DATUM);
        spr->drawString(I18n::str(WorkshopKey::WORKSHOP_SI_PREFIXES_HDR_EXAMPLE), ox + EX_TEXT_OX,    hdrCy);
    }

    // Datenzeilen
    for (int i = 0; i < PREFIX_COUNT; ++i)
    {
        const int rowCy = tabY + HEADER_H + i * ROW_H + ROW_H / 2;
        const SiPrefix &p = s_prefixes[i];

        uint16_t bg;
        if (i == static_cast<int>(m_selectedRow))
            bg = C_SEL_BG;
        else if (p.exponent == 0)
            bg = C_BASE_BG;
        else
            bg = (i % 2 == 0) ? C_ROW_A : C_ROW_B;

        const uint16_t fgName = (p.exponent == 0) ? C_FG_BASE : C_FG;
        const uint16_t fgPot  = (p.exponent > 0)  ? C_FG_POS
                              : (p.exponent < 0)   ? C_FG_NEG
                              :                      C_FG_BASE;

        // Spalte 0: Präfix-Name (linksbündig)
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(fgName, bg);
        spr->drawString(I18n::str(p.nameKey), ox + NAME_TEXT_OX, rowCy);

        // Spalte 1: Symbol (zentriert)
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_FG, bg);
        spr->drawString(p.symbol, ox + SYM_CENTER_OX, rowCy);

        // Spalte 2: Zehnerpotenz "10^X" (zentriert, farbig nach Vorzeichen)
        char potStr[8];
        stringutils::safeFormat(potStr, "10^%d", static_cast<int>(p.exponent));
        spr->setTextColor(fgPot, bg);
        spr->drawString(potStr, ox + POT_CENTER_OX, rowCy);

        // Spalte 3: Beispiel (linksbündig, gedimmt)
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(C_FG_EX, bg);
        spr->drawString(p.example, ox + EX_TEXT_OX, rowCy);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet den Info-Balken mit Umrechnungsrelationen oder Hinweistext.
 */
void WorkshopTool_SiPrefixes::drawInfoBar(Gfx_Sprite *spr, int ox, int infoY,
                                           int w, int h) const
{
    spr->fillRect(ox, infoY, w, h, C_INFO_BG);

    spr->loadFont(ComfortaaBold16);

    const int cy = infoY + h / 2;

    if (m_selectedRow < 0)
    {
        // Kein Präfix gewählt: Hinweistext anzeigen
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_INFO_HINT, C_INFO_BG);
        spr->drawString(I18n::str(WorkshopKey::WORKSHOP_SI_PREFIXES_HINT_TAP), ox + w / 2, cy);
        spr->unloadFont();
        return;
    }

    const SiPrefix &p = s_prefixes[m_selectedRow];

    // Linke Seite: Relation nach unten (gewählter Präfix -> nächst-kleinerer) Format: "1 M = 1000 k"
    char leftStr[32] = "";
    if (m_selectedRow < PREFIX_COUNT - 1)
    {
        const SiPrefix &below = s_prefixes[m_selectedRow + 1];
        stringutils::safeFormat(leftStr, "1 %s = 1000 %s", p.symbol, below.symbol);
    }

    // Rechte Seite: Relation nach oben (nächst-größerer -> gewählter Präfix) Format: "1 G = 1000 M"
    char rightStr[32] = "";
    if (m_selectedRow > 0)
    {
        const SiPrefix &above = s_prefixes[m_selectedRow - 1];
        stringutils::safeFormat(rightStr, "1 %s = 1000 %s", above.symbol, p.symbol);
    }

    spr->setTextColor(C_INFO_FG, C_INFO_BG);

    if (leftStr[0] != '\0' && rightStr[0] != '\0')
    {
        // Beide Nachbarn: "1 M = 1000 k   ·   1 G = 1000 M"
        const int cx = ox + w / 2;

        spr->setTextDatum(MR_DATUM);
        spr->drawString(leftStr, cx - 14, cy);

        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(C_INFO_DIM, C_INFO_BG);
        spr->drawString("\xC2\xB7", cx, cy); // · (U+00B7, Mittelpunkt)

        spr->setTextColor(C_INFO_FG, C_INFO_BG);
        spr->setTextDatum(ML_DATUM);
        spr->drawString(rightStr, cx + 14, cy);
    }
    else
    {
        // Nur ein Nachbar (Tera oder Piko): zentrieren
        spr->setTextDatum(MC_DATUM);
        spr->drawString(leftStr[0] != '\0' ? leftStr : rightStr, ox + w / 2, cy);
    }

    spr->unloadFont();
}

// ===================================================================
// TOUCH-HANDLING
// ===================================================================

/**
 * @brief Treffertest: gibt den Datenzeilen-Index für eine Display-Y-Koordinate zurück.
 */
int WorkshopTool_SiPrefixes::hitTestRow(int32_t ty) const
{
    if (m_h == 0)
        return -1;

    // Display-Koordinaten -> lokal zum Werkzeugbereich (Sprite-Y minus Display-Offset)
    const int localY = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_oy;

    // TOP_PAD und Kopfzeile überspringen
    const int dataStart = TOP_PAD + HEADER_H;
    if (localY < dataStart)
        return -1;

    // Nur Tabellenzeilen (nicht den Info-Balken) sind trefferbar
    const int tableEnd = TOP_PAD + HEADER_H + PREFIX_COUNT * ROW_H;
    if (localY >= tableEnd)
        return -1;

    const int row = (localY - dataStart) / ROW_H;
    return (row >= 0 && row < PREFIX_COUNT) ? row : -1;
}

/**
 * @brief Verarbeitet Touch-Ereignisse; Selektion wird beim Release ausgelöst.
 */
bool WorkshopTool_SiPrefixes::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    (void)tx;

    if (touched)
    {
        // Nur beim ersten Frame des Touches die Zielzeile merken
        if (!m_touchActive)
        {
            m_touchActive = true;
            m_pressedRow  = static_cast<int8_t>(hitTestRow(ty));
        }
        return false; // Noch nicht auslösen - kein Redraw
    }

    // Touch-Release: Selektion einmalig auslösen
    m_touchActive = false;
    const int row = static_cast<int>(m_pressedRow);
    m_pressedRow  = -1;

    if (row < 0)
        return false;

    // Gleiche Zeile: Markierung aufheben; andere Zeile: neue Markierung
    if (row == static_cast<int>(m_selectedRow))
        m_selectedRow = -1;
    else
        m_selectedRow = static_cast<int8_t>(row);

    return true; // Redraw anfordern
}
