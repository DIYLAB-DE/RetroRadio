#ifndef WORKSHOPTOOL_CALCULATOR_H
#define WORKSHOPTOOL_CALCULATOR_H

#include <stddef.h>

#include "DotMatrix.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_Calculator
 * @brief Taschenrechner mit 4 Grundrechenarten, Klammerung und Dez/Hex/Bin-Anzeige.
 *
 * Zeigt das Ergebnis in drei Zeilen:
 *   - DEZ: immer sichtbar (DotMatrix, klassischer HD44780-Look)
 *   - HEX: nur bei nicht-negativer Ganzzahl ≤ 0xFFFFFFFF
 *   - BIN: nur bei Ganzzahl im Bereich 0..65535 (16 Bit)
 *   Bei Fließkommazahlen bleiben HEX- und BIN-Zeile leer.
 *
 * Tastaturlayout (5×5):
 *   [C]    [⌫]   [(]   [)]   [/]
 *   [7]    [8]   [9]   [√]   [*]
 *   [4]    [5]   [6]   [x²]  [-]
 *   [1]    [2]   [3]   [π]   [+]
 *   [+/-]  [0]   [.]   [%]   [=]
 *
 * Rechenarchitektur: Expression-String-Modell.
 *   Der gesamte Rechenausdruck wird als einfacher Zeichenstring gehalten
 *   (m_expression). Die Auswertung erfolgt über einen eingebetteten
 *   Shunting-Yard-Parser mit korrekter Operatorpräzedenz und beliebiger
 *   Klammertiefe.
 *
 * Sonderzeichen in den Tastenbeschriftungen:
 *   √ (U+221A), x² (U+00B2) und π (U+03C0) sind als UTF-8 im Quelltext
 *   kodiert und müssen im verwendeten Font (ComfortaaBold16) vorhanden sein.
 *   ⌫ (Backspace) wird nicht als Glyphe gerendert, sondern in drawButtons()
 *   über drawSmoothLine()-Aufrufe als Vektorgrafik gezeichnet.
 *
 * Zeichnet ausschließlich in den übergebenen Work-Sprite (kein eigener Sprite).
 */
class WorkshopTool_Calculator : public WorkshopTool
{
public:
    /**
     * @brief Konstruktor. Initialisiert DotMatrix-Instanzen mit LED-Farbschema.
     */
    WorkshopTool_Calculator();

    /** @brief Destruktor. Gibt keine eigenen Ressourcen frei (kein eigener Sprite). */
    ~WorkshopTool_Calculator() override = default;

    // Kopieren/Verschieben verboten (DotMatrix ist nicht kopierbar)
    WorkshopTool_Calculator(const WorkshopTool_Calculator &) = delete;
    WorkshopTool_Calculator &operator=(const WorkshopTool_Calculator &) = delete;

    /**
     * @brief Reserviert DotMatrix-Textpuffer und setzt Rechner zurück.
     * @param tft Display-Objekt (wird hier nicht dauerhaft gespeichert)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Kein Aufräumaufwand beim Verlassen. */
    void onExit() override {}

    /**
     * @brief Zeichnet Anzeige und Tastatur in den Work-Sprite.
     * @param sprite   Work-Sprite des Workshop-Moduls
     * @param originX  Linke Kante des Inhaltsbereichs (sprite-relativ)
     * @param originY  Obere Kante des Inhaltsbereichs (sprite-relativ)
     * @param w        Verfügbare Breite (typisch 547 px)
     * @param h        Verfügbare Höhe (typisch 305 px)
     */
    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events für die Taschenrechnertasten.
     * @param tx      Absoluter Display-X (bei touched=false ignoriert)
     * @param ty      Absoluter Display-Y (bei touched=false ignoriert)
     * @param touched true bei aktivem Touch
     * @return true wenn Neuzeichnung des Arbeitsbereichs erforderlich
     *
     * Drückt eine Taste visuell beim Berühren (Feedback) und führt die Aktion beim Loslassen aus (Touch-up-Semantik).
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    // =========================================================================
    // Innere Typen
    // =========================================================================

    /** @brief Rechteck einer Rechner-Taste (relativ zum Inhaltsursprung). */
    struct BtnRect
    {
        int x; ///< Linke Kante (inhaltsbereich-relativ)
        int y; ///< Obere Kante (inhaltsbereich-relativ)
        int w; ///< Breite
        int h; ///< Höhe
    };

    /** @brief Klassifizierung der Tasten für Farb- und Aktionszuordnung. */
    enum class BtnType : uint8_t
    {
        DIGIT,    ///< Ziffern 0-9
        DOT,      ///< Dezimalpunkt
        OPERATOR, ///< Rechenoperatoren +, -, *, /
        MODIFIER, ///< Modifikatoren: (, ), %, +/-, ⌫
        FUNCTION, ///< Mathematische Funktionen: √, x², π
        CLEAR,    ///< Alles zurücksetzen [C]
        EQUALS    ///< Berechnung auslösen [=]
    };

    /**
     * @brief Statische Beschreibung einer Taste.
     *
     * label[] enthält den Anzeigetext als UTF-8-String (max. 3 Bytes + '\0'). action kodiert die Aktion: Ziffer, Operator oder Sonderzeichen ('C'=Clear, '='=Equals, '~'=Vorzeichen, '%'=Prozent, 'B'=Backspace,
     *  '('=öffnende Klammer, ')'=schließende Klammer,
     *  'R'=Quadratwurzel(√), 'Q'=Quadrat(x²), 'P'=Pi(π)).
     */
    struct KeyEntry
    {
        char    label[6]; ///< Anzeigetext auf der Taste (UTF-8, max. 5 Bytes + '\0')
        char    action;   ///< Aktionszeichen
        BtnType type;     ///< Tastentyp für Farbzuordnung
    };

    /**
     * @brief Token des Shunting-Yard-Parsers.
     *
     * Unterscheidet Zahlen, Operatoren (inkl. unärer '+' als 'P', unärer '-' als 'M') und Klammern.
     */
    struct CalcToken
    {
        enum Type : uint8_t { NUMBER, OPERATOR, PAREN } type = NUMBER;
        char   op     = 0;
        double number = 0.0;
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_BUTTONS = 25; ///< Anzahl Tasten (5×5 Grid)

    static constexpr size_t EXPR_CAP  = 256; ///< Maximale Länge des Ausdrucks-Strings inkl. '\0'
    static constexpr int    TOKEN_CAP = 64;  ///< Maximale Token-Anzahl pro Auswertung

    // Anzeige-Layout
    static constexpr int EXPR_H           = 22;
    static constexpr int DISPLAY_H        = 122;
    static constexpr int DISPLAY_CORNER_R = 8;
    static constexpr int DISPLAY_PAD_TOP  = 6;  ///< Oberer Innenabstand der Anzeige (unten ebenfalls 6px)
    static constexpr int DISPLAY_ROW_GAP  = 4;
    static constexpr int DISPLAY_LBL_X    = 8;

    // DotMatrix Y-Positionen (4×4 Dots, dotSpace=1, charSpace=3)
    static constexpr int DISP_CHAR_H    = 34;
    static constexpr int DISP_DEZ_REL_Y = DISPLAY_PAD_TOP;
    static constexpr int DISP_HEX_REL_Y = DISP_DEZ_REL_Y + DISP_CHAR_H + DISPLAY_ROW_GAP;
    static constexpr int DISP_BIN_REL_Y = DISP_HEX_REL_Y + DISP_CHAR_H + DISPLAY_ROW_GAP;

    static constexpr int DOTS_FIELD_CHARS = 18;
    static constexpr int DOTS_START_X     = 58;

    // Tastatur-Layout
    static constexpr int BTN_DISPLAY_GAP = 5;
    static constexpr int BTN_GAP         = 5;
    static constexpr int BTN_RADIUS      = 6;
    static constexpr int BTN_TEXT_OY     = 2; ///< Vertikaler Korrektur-Offset für Textzentrierung (Font-Metrik-Ausgleich)
    static constexpr int BTN_ROWS        = 5;
    static constexpr int BTN_COLS        = 5; ///< Fünf Spalten für schlanke Tastenproportionen

    // LCD-Anzeige Farbpalette (RGB565)
    static constexpr uint16_t DISP_BG        = 0xD6B4;
    static constexpr uint16_t LCD_ON_COLOR   = 0x2104;
    static constexpr uint16_t LCD_OFF_COLOR  = 0xC652;
    static constexpr uint16_t DISP_LABEL_CLR = 0x2945;
    static constexpr uint16_t EXPR_COLOR     = 0xAD75;

    // Tastatur Farbpalette (RGB565)
    static constexpr uint16_t BTN_NUM_FILL    = 0x2945; ///< Zifferntasten
    static constexpr uint16_t BTN_NUM_TEXT    = 0xDEDB;
    static constexpr uint16_t BTN_MOD_FILL    = 0x3186; ///< (, ), %, +/-, ⌫
    static constexpr uint16_t BTN_MOD_TEXT    = 0xFFFF;
    static constexpr uint16_t BTN_FN_FILL     = 0x2A6B; ///< √, x², π (dunkleres Teal)
    static constexpr uint16_t BTN_FN_TEXT     = 0xFFFF;
    static constexpr uint16_t BTN_OP_FILL     = 0x328A; ///< Operatoren (Petrol)
    static constexpr uint16_t BTN_OP_TEXT     = 0xFFFF;
    static constexpr uint16_t BTN_CLR_FILL    = 0x7800; ///< C-Taste (Dunkelrot)
    static constexpr uint16_t BTN_CLR_TEXT    = 0xFFFF;
    static constexpr uint16_t BTN_BS_FILL     = 0x7812; ///< Violette Füllfarbe der BS-Taste
    static constexpr uint16_t BTN_BS_TEXT     = 0xFFFF;
    static constexpr uint16_t BTN_EQ_FILL     = 0x2C6B; ///< =-Taste (Dunkelgrün)
    static constexpr uint16_t BTN_EQ_TEXT     = 0xFFFF;
    static constexpr uint16_t BTN_PRESSED_FILL = 0x6B6D;
    static constexpr uint16_t BTN_PRESSED_TEXT = 0xFFFF;

    static const KeyEntry KEYS[NUM_BUTTONS];

    // =========================================================================
    // Eingabeverarbeitung
    // =========================================================================

    /**
     * @brief Berechnet alle Tastenpositionen für die gegebene Inhaltsgröße.
     * @param w Verfügbare Breite
     * @param h Verfügbare Höhe
     */
    void computeLayout(int w, int h);

    /**
     * @brief Treffertest für inhaltsbereich-relative Koordinaten.
     * @param cx Inhaltsbereich-X
     * @param cy Inhaltsbereich-Y
     * @return Index in KEYS[]/m_btnRects[], oder -1
     */
    int hitTest(int cx, int cy) const;

    /**
     * @brief Führt die Aktion einer Taste aus.
     * @param btnIdx Index in KEYS[] (0..NUM_BUTTONS-1)
     */
    void executeAction(int btnIdx);

    /**
     * @brief Kernfunktion der Zeicheneingabe.
     * @param value Ziffer, '.', oder Operator (+,-,*,/)
     */
    void appendInput(char value);

    /**
     * @brief Hängt eine öffnende Klammer an.
     *
     * Fügt implizites '*' ein, wenn davor eine Ziffer oder ')' steht.
     */
    void openParen();

    /**
     * @brief Hängt eine schließende Klammer an.
     *
     * Wirkungslos wenn keine offene Klammer vorhanden ist oder das letzte Zeichen kein numerisches Zeichen bzw. kein ')' ist.
     */
    void closeParen();

    /**
     * @brief Löscht das letzte Zeichen (⌫).
     *
     * Nach = wird der gesamte Ausdruck gelöscht (Neustart).
     */
    void backspace();

    /**
     * @brief Negiert den letzten Zahlenwert im Ausdruck (+/-).
     */
    void toggleSign();

    /**
     * @brief Dividiert die letzte Zahl im Ausdruck durch 100 (%).
     *
     * Wirkungslos wenn das letzte Zeichen kein numerisches Zeichen ist.
     */
    void applyPercent();

    /**
     * @brief Ersetzt die letzte Zahl durch ihre Quadratwurzel (√).
     *
     * Berücksichtigt ein vorangehendes unäres Minuszeichen. Wirkungslos wenn der Wert negativ oder kein Zahlenwert am Ende vorhanden ist.
     */
    void applySquareRoot();

    /**
     * @brief Ersetzt die letzte Zahl durch ihr Quadrat (x²).
     *
     * Berücksichtigt ein vorangehendes unäres Minuszeichen, sodass (-3)² = 9 korrekt berechnet wird.
     */
    void applySquare();

    /**
     * @brief Hängt den π-Wert (3.14159265358979) an den Ausdruck an.
     *
     * Fügt implizites '*' ein wenn davor eine Ziffer oder ')' steht.
     */
    void appendPi();

    /**
     * @brief Wertet den Ausdruck aus und speichert das Ergebnis.
     */
    void executeEquals();

    /** @brief Setzt den Rechner vollständig zurück. */
    void clearAll();

    // =========================================================================
    // Anzeige
    // =========================================================================

    /** @brief Berechnet m_displayValue und m_resultText aus dem aktuellen Ausdruck. */
    void updateDisplay();

    /** @brief Baut m_traceStr aus m_expression auf (Leerzeichen um binäre Operatoren). */
    void buildTrace();

    /** @brief Füllt die DotMatrix-Texte aus m_resultText und m_displayValue. */
    void updateDisplayStrings();

    /**
     * @brief Zeichnet den LED-Anzeigebereich (Hintergrund + DotMatrix-Zeilen).
     * @param spr  Work-Sprite
     * @param ox   Inhaltsbereich-Ursprung X (sprite-relativ)
     * @param oy   Inhaltsbereich-Ursprung Y (sprite-relativ)
     * @param w    Verfügbare Breite
     */
    void drawDisplay(Gfx_Sprite *spr, int ox, int oy, int w);

    /**
     * @brief Zeichnet alle Tasten der Rechnertastatur.
     * @param spr  Work-Sprite
     * @param ox   Inhaltsbereich-Ursprung X (sprite-relativ)
     * @param oy   Inhaltsbereich-Ursprung Y (sprite-relativ)
     */
    void drawButtons(Gfx_Sprite *spr, int ox, int oy);

    // =========================================================================
    // Shunting-Yard-Auswertung
    // =========================================================================

    /**
     * @brief Wertet einen mathematischen Ausdrucks-String aus (Shunting-Yard).
     * @param expr    Eingangsstring (kein Leerzeichen, '.' als Dezimaltrenner)
     * @param outVal  Ergebnis bei Erfolg
     * @return true bei erfolgreich berechnetem, endlichem Ergebnis
     */
    static bool tryEvaluate(const char *expr, double &outVal);

    /** @brief Zerlegt den Ausdruck in Token. */
    static bool calcTokenize(const char *expr,
                              CalcToken *tokens, int cap, int &outCount);

    /** @brief Konvertiert Infix-Tokens in Umgekehrte Polnische Notation. */
    static bool calcToRpn(const CalcToken *tokens, int count,
                           CalcToken *outRpn, int cap, int &outCount);

    /** @brief Wertet eine RPN-Tokenfolge aus. */
    static bool calcEvalRpn(const CalcToken *rpn, int count, double &outVal);

    // =========================================================================
    // Zeichenketten-Hilfsfunktionen
    // =========================================================================

    /** @brief Letztes Zeichen von s, oder '\0' wenn s leer ist. */
    static char exprLastChar(const char *s);

    /** @brief true wenn c ein Rechenoperator ist (+,-,*,/). */
    static bool isOperatorChar(char c);

    /** @brief true wenn c eine Ziffer oder '.' ist. */
    static bool isNumericChar(char c);

    /** @brief Löscht das Zeichen an Position pos in buf. */
    static void exprEraseAt(char *buf, size_t pos);

    /** @brief Fügt c an Position pos in buf ein. @return true bei Erfolg. */
    static bool exprInsertAt(char *buf, size_t cap, size_t pos, char c);

    /**
     * @brief Ermittelt die letzte Zahl am Ende von expr inklusive vorangehendem
     *        unären Minus.
     * @param expr       Eingangsstring
     * @param outVal     Wert der gefundenen Zahl (mit Vorzeichen)
     * @param outAbsStart Position des ersten Zeichens (inkl. '-' falls vorhanden)
     * @return true wenn eine Zahl gefunden wurde
     */
    static bool findLastNumber(const char *expr, double &outVal, int &outAbsStart);

    /**
     * @brief Formatiert val als kompakte Dezimalzahl ohne überflüssige Nullen.
     * @param val Zu formatierender Wert; NaN oder Inf ergibt "Fehler"
     * @param buf  Ausgabepuffer
     * @param sz   Puffergröße
     */
    static void formatDecimal(double val, char *buf, size_t sz);

    /** @brief Formatiert val als 16-Bit-Binärstring "0bNNNN...NNNN". */
    static void formatBinary(unsigned int val, char *buf);

    /**
     * @brief Füllt src links mit Leerzeichen auf totalChars Zeichen auf.
     *
     * Ist src länger als totalChars, wird rechtsbündig auf totalChars gekürzt.
     */
    static void padLeft(char *dst, size_t dstSz, const char *src, int totalChars);

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    int  m_originX     = 0;
    int  m_originY     = 0;
    int  m_w           = 0;
    int  m_h           = 0;
    bool m_layoutValid = false;

    BtnRect m_btnRects[NUM_BUTTONS];

    // Rechner-Zustand
    char   m_expression[EXPR_CAP] = ""; ///< Aktueller Rechenausdruck (ohne Leerzeichen)
    char   m_resultText[32]       = "0"; ///< Formatiertes Ergebnis für DEZ-Anzeige
    double m_displayValue         = 0.0; ///< Numerischer Wert für HEX/BIN-Berechnung
    bool   m_justEvaluated        = false; ///< true nach =; nächste Ziffer startet neuen Ausdruck

    char m_traceStr[400] = ""; ///< Formatierter Rechenweg (Leerzeichen um binäre Operatoren)

    // DotMatrix-Strings
    char m_dezStr[20] = "";
    char m_hexStr[20] = "";
    char m_binStr[20] = "";
    bool m_valueDirty = true;

    // Touch-Tracking
    int  m_pressedBtn  = -1;
    bool m_touchWas    = false;
    bool m_touchReady  = false; ///< true sobald ein sauberes touched=false Frame gesehen wurde; verhindert Phantom-Touch-Auslösung beim Booten

    // DotMatrix-Instanzen
    DotMatrix m_dotDez;
    DotMatrix m_dotHex;
    DotMatrix m_dotBin;
};

#endif // WORKSHOPTOOL_CALCULATOR_H
