#include "WorkshopTool_AsciiTable.h"

#include <cstring>

#include "I18n.h"
#include "Workshop.h"
#include "StringUtils.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Anonymer Namespace - Layout- und Farbkonstanten
// ===================================================================

namespace
{
    // ===================================================================
    // Layout
    // ===================================================================

    constexpr int COLS = 8;       ///< Anzahl Tabellenspalten
    constexpr int ROWS = 16;      ///< Anzahl Tabellenzeilen
    constexpr int LABEL_W = 42;   ///< Breite der Zeilenbeschriftung in Pixeln
    constexpr int CELL_W = 63;    ///< Breite einer Zeichenzelle in Pixeln
    constexpr int HEADER_H = 20;  ///< Höhe der Spalten-Kopfzeile in Pixeln
    constexpr int HEADER_GAP = 2; ///< Abstand zwischen Content-Ursprung und Kopfzeile in Pixeln
    constexpr int ROW_H = 18;     ///< Höhe einer Datenzeile in Pixeln (18px für Akzentzeichen)
    constexpr int INFO_GAP = 0;   ///< Abstand zum Info-Streifen

    // X-Positionen der Felder im Info-Streifen (sprite-relativ)
    constexpr int INFO_DEC_LBL = 0;   ///< X: Label "DEC"
    constexpr int INFO_DEC_VAL = 40;  ///< X: DEC-Wert (max 3 Ziffern)
    constexpr int INFO_HEX_LBL = 90;  ///< X: Label "HEX"
    constexpr int INFO_HEX_VAL = 126; ///< X: HEX-Wert (2 Stellen)
    constexpr int INFO_OCT_LBL = 176; ///< X: Label "OCT"
    constexpr int INFO_OCT_VAL = 212; ///< X: OCT-Wert (3 Stellen)
    constexpr int INFO_BIN_LBL = 264; ///< X: Label "BIN"
    constexpr int INFO_BIN_VAL = 300; ///< X: BIN-Wert (8 Stellen à ~7 px = ~56 px)
    constexpr int INFO_CHAR_X = 374;  ///< X: Glyph, Kürzel + Beschreibung

    // ===================================================================
    // Footer-Toggle-Buttons
    // ===================================================================

    constexpr int FOOTER_BTN_W = 100;    ///< Buttonbreite
    constexpr int FOOTER_BTN_H = 44;     ///< Buttonhöhe
    constexpr int FOOTER_BTN_R = 6;      ///< Eckenradius
    constexpr int FOOTER_BTN_GAP = 8;    ///< Abstand zwischen den beiden Buttons
    constexpr int FOOTER_BTN_MARGIN = 8; ///< Abstand zum linken Rand des Footer-Bereichs

    // Farben für aktiven / inaktiven Toggle-Button
    constexpr uint16_t TOGGLE_ACTIVE_FILL = 0x328A;      ///< Highlight-Füllung
    constexpr uint16_t TOGGLE_ACTIVE_TEXT = 0xFFFF;      ///< Heller Text
    constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;   ///< Rand = Füllung
    constexpr uint16_t TOGGLE_INACTIVE_FILL = 0x0000;    ///< Dunkle Füllung
    constexpr uint16_t TOGGLE_INACTIVE_TEXT = 0x7BEF;    ///< Gedimmter Text
    constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8; ///< Dezenter Rand

    // ===================================================================
    // Steuerzeichen-Kürzel und Langbezeichnungen (Standard: 0-31 + DEL)
    // ===================================================================

    // Index 0-31 = ASCII 0-31; Index 32 = ASCII 127 (DEL)

    const char *const CTRL_NAMES[33] = {
        "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
        "BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI",
        "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
        "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US",
        "DEL"};

    const char *const CTRL_DESCS[33] = {
        "Null", "Start of Heading", "Start of Text", "End of Text",
        "End of Transmit", "Enquiry", "Acknowledge", "Bell",
        "Backspace", "Horizontal Tab", "Line Feed", "Vertical Tab",
        "Form Feed", "Carriage Return", "Shift Out", "Shift In",
        "Data Link Escape", "Device Control 1", "Device Control 2", "Device Control 3",
        "Device Control 4", "Neg. Acknowledge", "Synchronous Idle", "End of Tx Block",
        "Cancel", "End of Medium", "Substitute", "Escape",
        "File Separator", "Group Separator", "Record Separator", "Unit Separator",
        "Delete"};

    // ===================================================================
    // Windows-1252-Zeichenzuordnung für 0x80-0x9F
    // ===================================================================

    // Byte-Wert -> Unicode-Codepoint (0 = Position nicht definiert) Referenz: https://www.torsten-horn.de/techdocs/ascii.htm

    constexpr uint16_t WIN1252_MAP[32] = {
        0x20AC,
        0x0000,
        0x201A,
        0x0192,
        0x201E,
        0x2026,
        0x2020,
        0x2021, // 0x80-0x87  € . ‚ ƒ " ... † ‡
        0x02C6,
        0x2030,
        0x0160,
        0x2039,
        0x0152,
        0x0000,
        0x017D,
        0x0000, // 0x88-0x8F  ˆ ‰ Š ‹ Œ . Ž .
        0x0000,
        0x2018,
        0x2019,
        0x201C,
        0x201D,
        0x2022,
        0x2013,
        0x2014, // 0x90-0x97  . ' ' " " • - -
        0x02DC,
        0x2122,
        0x0161,
        0x203A,
        0x0153,
        0x0000,
        0x017E,
        0x0178, // 0x98-0x9F  ˜ ™ š › œ . ž Ÿ
    };

    /**
     * @brief Wandelt einen Unicode-Codepoint in eine UTF-8-Bytefolge um.
     * @param cp  Unicode-Codepoint (U+0000-U+FFFF)
     * @param buf Ausgabepuffer (mindestens 4 Bytes)
     * @return Anzahl geschriebener Bytes (1-3), 0 bei ungültigem Codepoint
     */
    int unicodeToUtf8(uint16_t cp, char *buf)
    {
        if (cp == 0)
        {
            buf[0] = '\0';
            return 0;
        }

        if (cp <= 0x7F)
        {
            buf[0] = static_cast<char>(cp);
            buf[1] = '\0';
            return 1;
        }

        if (cp <= 0x7FF)
        {
            buf[0] = static_cast<char>(0xC0 | (cp >> 6));
            buf[1] = static_cast<char>(0x80 | (cp & 0x3F));
            buf[2] = '\0';
            return 2;
        }

        // U+0800 - U+FFFF -> 3-Byte-UTF-8
        buf[0] = static_cast<char>(0xE0 | (cp >> 12));
        buf[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        buf[2] = static_cast<char>(0x80 | (cp & 0x3F));
        buf[3] = '\0';

        return 3;
    }

    /**
     * @brief Wandelt einen Wert in eine 8-stellige Binärzeichenkette um.
     * @param out Ausgabepuffer (mindestens 9 Bytes)
     * @param val Eingabewert (0-255)
     */
    void toBin8(char out[9], int val)
    {
        for (int i = 0; i < 8; ++i)
            out[i] = ((val >> (7 - i)) & 1) ? '1' : '0';

        out[8] = '\0';
    }

} // namespace

// ===================================================================
// Konstruktor / Lifecycle
// ===================================================================

/**
 * @brief Konstruktor - übernimmt die Farbpalette.
 */
WorkshopTool_AsciiTable::WorkshopTool_AsciiTable(AsciiTableColors colors)
    : m_colors(colors)
{
}

/**
 * @brief Initialisiert den Werkzeugzustand und die Footer-Buttons.
 */
void WorkshopTool_AsciiTable::onEnter(LGFX *tft)
{
    m_selectedChar = -1;
    m_showExtended = false;
    m_touchWasActive = false;
    m_lastTouchX = 0;
    m_lastTouchY = 0;
    m_ox = m_oy = 0;

    memset(m_subtitleBuf, 0, sizeof(m_subtitleBuf));
    refreshSubtitle();

    initFooterButtons(tft);
}

/**
 * @brief Räumt den Werkzeugzustand auf.
 */
void WorkshopTool_AsciiTable::onExit()
{
    m_selectedChar = -1;
    m_showExtended = false;
    m_buttonsInitialized = false;
}

// ===================================================================
// Footer-Buttons
// ===================================================================

/**
 * @brief Initialisiert die beiden Toggle-Buttons im Footer-Bereich.
 */
void WorkshopTool_AsciiTable::initFooterButtons(LGFX *tft)
{
    // Absolute Zentrumskoordinaten der Buttons
    constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
    constexpr int cx1 = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN + FOOTER_BTN_W / 2;
    constexpr int cx2 = cx1 + FOOTER_BTN_W / 2 + FOOTER_BTN_GAP + FOOTER_BTN_W / 2;

    // Button "0-127" (Standard-Bereich)
    m_btnStandard.initButton(
        tft,
        cx1, cy,
        FOOTER_BTN_W, FOOTER_BTN_H,
        FOOTER_BTN_R,
        TOGGLE_ACTIVE_OUTLINE,
        TOGGLE_ACTIVE_FILL,
        TOGGLE_ACTIVE_TEXT,
        "0-127",
        nullptr);
    m_btnStandard.setFont(LeagueGothicCond34);

    // Button "128-255" (Erweiterter Bereich)
    m_btnExtended.initButton(
        tft,
        cx2, cy,
        FOOTER_BTN_W, FOOTER_BTN_H,
        FOOTER_BTN_R,
        TOGGLE_INACTIVE_OUTLINE,
        TOGGLE_INACTIVE_FILL,
        TOGGLE_INACTIVE_TEXT,
        "128-255",
        nullptr);
    m_btnExtended.setFont(LeagueGothicCond34);

    m_buttonsInitialized = true;
}

/**
 * @brief Setzt Farben der Toggle-Buttons entsprechend m_showExtended.
 */
void WorkshopTool_AsciiTable::updateButtonColors()
{
    if (!m_showExtended)
    {
        // Standard aktiv
        m_btnStandard.setFillColor(TOGGLE_ACTIVE_FILL);
        m_btnStandard.setTextColor(TOGGLE_ACTIVE_TEXT);
        m_btnStandard.setOutlineColor(TOGGLE_ACTIVE_OUTLINE);
        // Extended inaktiv
        m_btnExtended.setFillColor(TOGGLE_INACTIVE_FILL);
        m_btnExtended.setTextColor(TOGGLE_INACTIVE_TEXT);
        m_btnExtended.setOutlineColor(TOGGLE_INACTIVE_OUTLINE);
    }
    else
    {
        // Standard inaktiv
        m_btnStandard.setFillColor(TOGGLE_INACTIVE_FILL);
        m_btnStandard.setTextColor(TOGGLE_INACTIVE_TEXT);
        m_btnStandard.setOutlineColor(TOGGLE_INACTIVE_OUTLINE);
        // Extended aktiv
        m_btnExtended.setFillColor(TOGGLE_ACTIVE_FILL);
        m_btnExtended.setTextColor(TOGGLE_ACTIVE_TEXT);
        m_btnExtended.setOutlineColor(TOGGLE_ACTIVE_OUTLINE);
    }
}

// ===================================================================
// Footer: Zeichnen und Touch
// ===================================================================

/**
 * @brief Zeichnet die Toggle-Buttons in die Fußzeile.
 */
void WorkshopTool_AsciiTable::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    updateButtonColors();
    m_btnStandard.drawButton();
    m_btnExtended.drawButton();
}

/**
 * @brief Verarbeitet Touch-Events auf den Toggle-Buttons.
 */
bool WorkshopTool_AsciiTable::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    const bool onStd = touched && m_btnStandard.contains(tx, ty);
    const bool onExt = touched && m_btnExtended.contains(tx, ty);

    const bool stdWasPressed = m_btnStandard.isPressed();
    const bool extWasPressed = m_btnExtended.isPressed();

    m_btnStandard.press(onStd);
    m_btnExtended.press(onExt);

    // Toggle bei Loslassen auf dem jeweils anderen Button
    if (m_btnStandard.justReleased() && m_showExtended)
    {
        m_showExtended = false;
        m_selectedChar = -1;
        refreshSubtitle();
        return true; // Workshop ruft drawFooter() und invalidateWork() auf
    }
    if (m_btnExtended.justReleased() && !m_showExtended)
    {
        m_showExtended = true;
        m_selectedChar = -1;
        refreshSubtitle();
        return true;
    }

    // Nur visuelle Rückmeldung bei Drücken (ohne Toggle)
    if (onStd != stdWasPressed)
        m_btnStandard.drawButton(onStd);

    if (onExt != extWasPressed)
        m_btnExtended.drawButton(onExt);

    return false;
}

/**
 * @brief Gibt den zum aktiven Bereich passenden Untertitel zurück.
 */
const char *WorkshopTool_AsciiTable::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Befüllt m_subtitleBuf passend zum aktuellen Bereichsmodus.
 */
void WorkshopTool_AsciiTable::refreshSubtitle()
{
    if (m_showExtended)
        I18n::format(m_subtitleBuf, sizeof(m_subtitleBuf),
                     WorkshopKey::WORKSHOP_ASCII_TABLE_SUB_EXTENDED);
    else
        m_subtitleBuf[0] = '\0';
}

// ===================================================================
// Zeichenfunktionen
// ===================================================================

/**
 * @brief Zeichnet die ASCII-Tabelle vollständig in den Work-Sprite.
 */
void WorkshopTool_AsciiTable::draw(Gfx_Sprite *spr, int ox, int oy, int w, int h)
{
    m_ox = ox;
    m_oy = oy;

    drawTable(spr, ox, oy, w, h);
    const int tableH = HEADER_H + ROWS * ROW_H;
    const int infoY = oy + HEADER_GAP + tableH + INFO_GAP;
    const int infoH = h - HEADER_GAP - tableH - INFO_GAP;
    drawInfoStrip(spr, ox, infoY, w, infoH);
}

/**
 * @brief Zeichnet das 8×16-Raster mit Kopfzeile und Zeilenbeschriftungen.
 */
void WorkshopTool_AsciiTable::drawTable(Gfx_Sprite *spr, int ox, int oy, int w, int h)
{
    (void)h;

    const int base = m_showExtended ? 128 : 0;
    const int tableH = HEADER_H + ROWS * ROW_H;
    const int tableTop = oy + HEADER_GAP;
    const int dataTop = tableTop + HEADER_H;

    // Gesamten Bereich (inkl. HEADER_GAP) löschen
    spr->fillRect(ox, oy, w, HEADER_GAP + tableH, m_colors.tableBg);

    // ===================================================================
    // Kopfzeile: "0x00" bis "0x70" beziehungsweise "0x80" bis "0xF0"
    // ===================================================================

    spr->fillRect(ox, tableTop, w, HEADER_H, m_colors.headerBg);
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(MC_DATUM);
    spr->setTextColor(m_colors.headerTx, m_colors.headerBg);

    char hdrBuf[8];
    memset(hdrBuf, 0, sizeof(hdrBuf));

    for (int col = 0; col < COLS; ++col)
    {
        const int cx = ox + LABEL_W + col * CELL_W + CELL_W / 2;
        const int cy = tableTop + HEADER_H / 2;
        stringutils::safeFormat(hdrBuf, "0x%02X", base + col * 16);
        spr->drawString(hdrBuf, cx, cy);
    }

    // ===================================================================
    // Datenzeilen
    // ===================================================================

    char labelBuf[8];
    char cellBuf[8];
    memset(labelBuf, 0, sizeof(labelBuf));
    memset(cellBuf, 0, sizeof(cellBuf));

    for (int row = 0; row < ROWS; ++row)
    {
        const int rowY = dataTop + row * ROW_H;

        // Zeilenbeschriftung "+0" bis "+F"
        spr->fillRect(ox, rowY, LABEL_W - 1, ROW_H, m_colors.headerBg);
        spr->setTextDatum(ML_DATUM);
        spr->setTextColor(m_colors.labelTx, m_colors.headerBg);
        stringutils::safeFormat(labelBuf, "+%X", row);
        spr->drawString(labelBuf, ox, rowY + ROW_H / 2);

        // Zeichenzellen
        for (int col = 0; col < COLS; ++col)
        {
            const int ch = base + col * 16 + row;
            const int cellX = ox + LABEL_W + col * CELL_W;
            const bool sel = (ch == m_selectedChar);
            const int textCx = cellX + CELL_W / 2;
            const int textCy = rowY + ROW_H / 2;

            if (sel)
                spr->fillRect(cellX, rowY, CELL_W, ROW_H, m_colors.selBg);

            spr->setTextDatum(MC_DATUM);

            if (!m_showExtended)
            {
                // Standard-Bereich 0-127
                if (ch == 32)
                {
                    spr->setTextColor(sel ? m_colors.selTx : m_colors.ctrlTx, sel ? m_colors.selBg : m_colors.tableBg);
                    spr->drawString("SPC", textCx, textCy);
                }
                else if (ch < 32 || ch == 127)
                {
                    spr->setTextColor(sel ? m_colors.selTx : m_colors.ctrlTx, sel ? m_colors.selBg : m_colors.tableBg);
                    spr->drawString(ctrlName(ch), textCx, textCy);
                }
                else
                {
                    cellBuf[0] = static_cast<char>(ch);
                    cellBuf[1] = '\0';
                    spr->setTextColor(sel ? m_colors.selTx : m_colors.printTx, sel ? m_colors.selBg : m_colors.tableBg);
                    spr->drawString(cellBuf, textCx, textCy);
                }
            }
            else
            {
                // Erweiterter Bereich 128-255
                if (ch <= 159)
                {
                    // Windows-1252 Zeichen (128-159)
                    const uint16_t cp = WIN1252_MAP[ch - 128];
                    if (cp != 0)
                    {
                        unicodeToUtf8(cp, cellBuf);
                        spr->setTextColor(sel ? m_colors.selTx : m_colors.printTx, sel ? m_colors.selBg : m_colors.tableBg);
                        spr->drawString(cellBuf, textCx, textCy);
                    }
                    // cp == 0: Position nicht definiert -> Zelle bleibt leer
                }
                else if (ch == 160)
                {
                    // Non-Breaking Space: wie Steuerzeichen behandeln
                    spr->setTextColor(sel ? m_colors.selTx : m_colors.ctrlTx, sel ? m_colors.selBg : m_colors.tableBg);
                    spr->drawString("NBS", textCx, textCy);
                }
                else
                {
                    // 161-255: Tatsächliches ISO 8859-1 Zeichen als UTF-8 rendern ISO 8859-1 Codepoints 0x80-0xFF = Unicode U+0080-U+00FF UTF-8: U+0080-U+00BF -> 0xC2 + Byte U+00C0-U+00FF -> 0xC3 + (Byte - 0x40)
                    if (ch <= 0xBF)
                    {
                        cellBuf[0] = static_cast<char>(0xC2);
                        cellBuf[1] = static_cast<char>(ch);
                    }
                    else
                    {
                        cellBuf[0] = static_cast<char>(0xC3);
                        cellBuf[1] = static_cast<char>(ch - 0x40);
                    }
                    cellBuf[2] = '\0';

                    spr->setTextColor(sel ? m_colors.selTx : m_colors.printTx,
                                      sel ? m_colors.selBg : m_colors.tableBg);
                    spr->drawString(cellBuf, textCx, textCy);
                }
            }
        }
    }

    // ===================================================================
    // Trennlinien zeichnen
    // ===================================================================

    // Vertikale Trennlinie Label-Spalte / Datengitter
    spr->drawFastVLine(ox + LABEL_W - 1, dataTop, ROWS * ROW_H, m_colors.sep);

    // Vertikale Trennlinien zwischen den Datenspalten
    for (int col = 1; col < COLS; ++col)
    {
        const int vx = ox + LABEL_W + col * CELL_W;
        spr->drawFastVLine(vx, dataTop, ROWS * ROW_H, m_colors.sep);
    }

    // Horizontale Trennlinien zwischen den Datenzeilen
    for (int row = 0; row < ROWS - 1; ++row)
    {
        const int lineY = dataTop + (row + 1) * ROW_H - 1;
        spr->drawFastHLine(ox, lineY, LABEL_W + COLS * CELL_W, m_colors.sep);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet den Info-Streifen am unteren Rand des Content-Bereichs.
 */
void WorkshopTool_AsciiTable::drawInfoStrip(Gfx_Sprite *spr, int ox, int oy, int w, int h)
{
    constexpr int CLEAR_EXTRA = 4;
    spr->fillRect(ox, oy, w, h + CLEAR_EXTRA, m_colors.infoBg);
    const int cy = oy + h / 2 + 4;

    spr->loadFont(ComfortaaBold14);

    if (m_selectedChar < 0)
    {
        spr->setTextDatum(MC_DATUM);
        spr->setTextColor(m_colors.infoHint, m_colors.infoBg);
        spr->drawString(I18n::str(WorkshopKey::WORKSHOP_ASCII_TABLE_HINT_TAP), ox + w / 2, cy);
        spr->unloadFont();
        return;
    }

    const int ch = m_selectedChar;

    char decBuf[8];
    char hexBuf[8];
    char octBuf[8];
    char binBuf[9];
    char nameBuf[52];

    memset(decBuf, 0, sizeof(decBuf));
    memset(hexBuf, 0, sizeof(hexBuf));
    memset(octBuf, 0, sizeof(octBuf));
    memset(binBuf, 0, sizeof(binBuf));
    memset(nameBuf, 0, sizeof(nameBuf));

    stringutils::safeFormat(decBuf, "%d", ch);
    stringutils::safeFormat(hexBuf, "%02X", ch);
    stringutils::safeFormat(octBuf, "%03o", ch);
    toBin8(binBuf, ch);

    // Name / Beschreibung zusammenbauen
    if (ch < 32 || ch == 127)
    {
        // Standard-Steuerzeichen
        stringutils::safeFormat(nameBuf, "%s  %s", ctrlName(ch), ctrlDesc(ch));
    }
    else if (ch == 32)
    {
        stringutils::safeCopy(nameBuf, "SPC  Space");
    }
    else if (ch >= 33 && ch <= 126)
    {
        // Druckbares Standard-Zeichen
        nameBuf[0] = static_cast<char>(ch);
        nameBuf[1] = '\0';
    }
    else if (ch >= 128 && ch <= 159)
    {
        // Windows-1252 Zeichen
        const uint16_t cp = WIN1252_MAP[ch - 128];
        if (cp != 0)
            unicodeToUtf8(cp, nameBuf);
        else
            I18n::format(nameBuf, sizeof(nameBuf),
                         WorkshopKey::WORKSHOP_ASCII_TABLE_NOT_DEFINED);
    }
    else if (ch == 160)
    {
        stringutils::safeCopy(nameBuf, "NBS  Non-Breaking Space");
    }
    else
    {
        // 161-255: Tatsächliches ISO 8859-1 Zeichen als UTF-8
        if (ch <= 0xBF)
        {
            nameBuf[0] = static_cast<char>(0xC2);
            nameBuf[1] = static_cast<char>(ch);
        }
        else
        {
            nameBuf[0] = static_cast<char>(0xC3);
            nameBuf[1] = static_cast<char>(ch - 0x40);
        }
        nameBuf[2] = '\0';
    }

    // Labels (gedimmt)
    spr->setTextDatum(ML_DATUM);
    spr->setTextColor(m_colors.infoKey, m_colors.infoBg);
    spr->drawString("DEC", ox + INFO_DEC_LBL, cy);
    spr->drawString("HEX", ox + INFO_HEX_LBL, cy);
    spr->drawString("OCT", ox + INFO_OCT_LBL, cy);
    spr->drawString("BIN", ox + INFO_BIN_LBL, cy);

    // Werte (hell)
    spr->setTextColor(m_colors.infoVal, m_colors.infoBg);
    spr->drawString(decBuf, ox + INFO_DEC_VAL, cy);
    spr->drawString(hexBuf, ox + INFO_HEX_VAL, cy);
    spr->drawString(octBuf, ox + INFO_OCT_VAL, cy);
    spr->drawString(binBuf, ox + INFO_BIN_VAL, cy);
    spr->drawString(nameBuf, ox + INFO_CHAR_X, cy);

    spr->unloadFont();
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Events und aktualisiert die Zeichenauswahl.
 */
bool WorkshopTool_AsciiTable::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    if (touched)
    {
        m_lastTouchX = tx;
        m_lastTouchY = ty;
        m_touchWasActive = true;
        return false;
    }

    if (!m_touchWasActive)
        return false;

    m_touchWasActive = false;

    const int ch = hitTest(m_lastTouchX, m_lastTouchY);
    if (ch == m_selectedChar)
        return false;

    m_selectedChar = ch;
    return true;
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Ermittelt den ASCII-Code an einer absoluten Display-Position.
 */
int WorkshopTool_AsciiTable::hitTest(int32_t tx, int32_t ty) const
{
    const int cx = static_cast<int>(tx) - WorkshopCfg::WORK_X - m_ox;
    const int cy = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_oy;

    if (cx < LABEL_W || cx >= LABEL_W + COLS * CELL_W)
        return -1;

    // Datengitter: nach HEADER_GAP + Kopfzeile
    if (cy < HEADER_GAP + HEADER_H || cy >= HEADER_GAP + HEADER_H + ROWS * ROW_H)
        return -1;

    const int col = (cx - LABEL_W) / CELL_W;
    const int row = (cy - HEADER_GAP - HEADER_H) / ROW_H;
    const int base = m_showExtended ? 128 : 0;

    return base + col * 16 + row;
}

/**
 * @brief Gibt das Kürzel eines Steuerzeichens zurück.
 */
const char *WorkshopTool_AsciiTable::ctrlName(int ch)
{
    if (ch >= 0 && ch <= 31)
        return CTRL_NAMES[ch];
    if (ch == 127)
        return CTRL_NAMES[32];
    return "?";
}

/**
 * @brief Gibt die Langbezeichnung eines Steuerzeichens zurück.
 */
const char *WorkshopTool_AsciiTable::ctrlDesc(int ch)
{
    if (ch >= 0 && ch <= 31)
        return CTRL_DESCS[ch];
    if (ch == 127)
        return CTRL_DESCS[32];
    return "";
}
