#ifndef WORKSHOPTOOL_SMDRESISTOR_H
#define WORKSHOPTOOL_SMDRESISTOR_H

#include <stddef.h>
#include <stdint.h>

#include "Button.h"
#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_SmdResistor
 * @brief Decoder für einfache SMD-Widerstandscodes.
 *
 * Unterstützt dreistellige Codes, vierstellige Codes, R-Notation und Null-Ohm-Brücken. EIA-96-Codes werden nicht ausgewertet.
 */
class WorkshopTool_SmdResistor : public WorkshopTool
{
public:
    /**
     * @brief Konstruktor. Setzt den Startcode auf 472.
     */
    WorkshopTool_SmdResistor();

    /** @brief Destruktor. Keine eigenen Ressourcen vorhanden. */
    ~WorkshopTool_SmdResistor() override = default;

    WorkshopTool_SmdResistor(const WorkshopTool_SmdResistor &) = delete;
    WorkshopTool_SmdResistor &operator=(const WorkshopTool_SmdResistor &) = delete;
    WorkshopTool_SmdResistor(WorkshopTool_SmdResistor &&) = delete;
    WorkshopTool_SmdResistor &operator=(WorkshopTool_SmdResistor &&) = delete;

    /**
     * @brief Setzt das Werkzeug beim Betreten zurück.
     * @param tft Display-Objekt, wird nicht gespeichert
     */
    void onEnter(LGFX *tft) override;

    /** @brief Kein Aufräumaufwand beim Verlassen. */
    void onExit() override {}

    /**
     * @brief Zeichnet Auswahlspalten, SMD-Vorschau und Ergebniszeile.
     * @param sprite  Work-Sprite des Workshop-Moduls
     * @param originX Linke Kante des Inhaltsbereichs
     * @param originY Obere Kante des Inhaltsbereichs
     * @param w       Verfügbare Breite
     * @param h       Verfügbare Höhe
     */
    void draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Eingaben auf den Auswahlspalten.
     * @param tx      Absoluter Display-X
     * @param ty      Absoluter Display-Y
     * @param touched true bei aktivem Touch
     * @return true wenn neu gezeichnet werden soll
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    /**
     * @brief Zeichnet die Modus-Buttons in der Footer-Zeile.
     * @param tft Display-Objekt
     * @param x Linke Kante des verfügbaren Footer-Bereichs
     * @param y Obere Kante des verfügbaren Footer-Bereichs
     * @param w Verfügbare Breite
     * @param h Verfügbare Höhe
     */
    void drawFooter(LGFX *tft, int x, int y, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Eingaben auf den Footer-Modus-Buttons.
     * @param tx Absoluter Display-X
     * @param ty Absoluter Display-Y
     * @param touched true bei aktivem Touch
     * @return true wenn Arbeitsbereich und Footer neu gezeichnet werden sollen
     */
    bool handleFooterTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    /** @brief Achsenparalleles Rechteck, content-relativ. */
    struct Rect
    {
        int x; ///< Linke Kante
        int y; ///< Obere Kante
        int w; ///< Breite
        int h; ///< Höhe
    };

    /** @brief Aktiver Eingabemodus. */
    enum class Mode : uint8_t
    {
        Digit3 = 0, ///< 3-stelliger Code
        Digit4 = 1, ///< 4-stelliger Code
        RCode = 2   ///< R-Code
    };

    /** @brief Ergebnisart für die Hinweiszeile. */
    enum class ResultKind : uint8_t
    {
        Invalid,  ///< Code ist nicht auswertbar
        Numeric3, ///< 3-stelliger Zifferncode
        Numeric4, ///< 4-stelliger Zifferncode
        RCode,    ///< R-Notation mit drei oder vier Stellen
        Zero      ///< Null-Ohm-Brücke
    };

    static constexpr int CODE_MAX = 4;      ///< Maximale Code-Länge ohne Nullterminator
    static constexpr int NUM_MODES = 3;     ///< Anzahl der Modi
    static constexpr int MAX_COLS = 4;      ///< Maximale Anzahl Auswahlspalten
    static constexpr int DIGIT_ROWS = 10;   ///< Ziffern 0 bis 9
    static constexpr int R_MARK_ROW = 10;   ///< Zeile für das R-Zeichen
    static constexpr int BLANK_ROW = 11;    ///< Leere Stelle im R-Code-Modus
    static constexpr int R_ROWS = 12;       ///< Ziffern 0 bis 9 plus R und Leerfeld
    static constexpr int MAX_ROWS = R_ROWS; ///< Maximale Zeilenanzahl

    static constexpr int BODY_TOP = 10;      ///< Abstand vom Header zur Hauptfläche
    static constexpr int SELECT_W = 334;     ///< Breite der Auswahlspalten
    static constexpr int COL_GAP = 10;       ///< Abstand zwischen Auswahlspalten
    static constexpr int CELL_GAP = 4;       ///< Abstand zwischen Auswahlzellen
    static constexpr int RESULT_H = 25;      ///< Höhe der Ergebniszeile
    static constexpr int RESULT_GAP = 8;     ///< Abstand zwischen Hauptfläche und Ergebniszeile
    static constexpr int CELL_R = 5;         ///< Eckenradius der Auswahlzellen
    static constexpr int TEXT_OY = 1;        ///< Vertikaler Textkorrekturwert
    static constexpr int PREVIEW_PAD_X = 16; ///< Horizontaler Innenabstand der Vorschau
    static constexpr int CHIP_W = 138;       ///< Breite des gezeichneten SMD-Bauteils
    static constexpr int CHIP_H = 68;        ///< Höhe des gezeichneten SMD-Bauteils
    static constexpr int CHIP_PAD_W = 27;    ///< Breite der metallisierten Anschlüsse
    static constexpr int CHIP_R = 7;         ///< Eckenradius des Bauteilkörpers

    static constexpr uint16_t C_BG = 0x1082;              ///< Hintergrund
    static constexpr uint16_t C_PANEL_ALT = 0x2945;       ///< Ergebnis- und Vorschaufläche
    static constexpr uint16_t C_CELL = 0x1082;            ///< Auswahlzelle
    static constexpr uint16_t C_CELL_BORDER = 0x6B6D;     ///< Rahmen der Auswahlzellen
    static constexpr uint16_t C_CELL_ACTIVE = 0x328A;     ///< Aktive Auswahl
    static constexpr uint16_t C_CELL_PRESS = 0x4B4D;      ///< Gedrückte Auswahl
    static constexpr uint16_t C_TEXT = 0xDEDB;            ///< Standardtext
    static constexpr uint16_t C_TEXT_DIM = 0xAD75;        ///< Gedimmter Text
    static constexpr uint16_t C_TEXT_BRIGHT = 0xFFFF;     ///< Heller Text
    static constexpr uint16_t C_ACCENT = 0x07E0;          ///< Elektronik-Akzent
    static constexpr uint16_t C_ERR = 0xF9E7;             ///< Fehlertext
    static constexpr uint16_t C_CHIP_BODY = 0x0000;       ///< Schwarzer Bauteilkörper
    static constexpr uint16_t C_CHIP_EDGE = 0x6B6D;       ///< Bauteilkontur
    static constexpr uint16_t C_CHIP_PAD = 0xBDF7;        ///< Helle Metallisierung
    static constexpr uint16_t C_CHIP_PAD_LIGHT = 0xE71C;  ///< Lichtkante der Metallisierung
    static constexpr uint16_t C_CHIP_PAD_DARK = 0x738E;   ///< Schattenkante der Metallisierung
    static constexpr uint16_t C_CHIP_PAD_SHADOW = 0x8CB4; ///< Schatten der Metallisierung

    static const WorkshopKey MODE_LABEL_KEYS[NUM_MODES]; ///< I18n-Keys der Modus-Buttons

    /**
     * @brief Setzt Eingabe, Ergebnis und Touch-Zustand auf Standardwerte.
     */
    void clearAll();

    /**
     * @brief Berechnet Modus-, Auswahl- und Ergebnisrechtecke.
     * @param w Verfügbare Breite
     * @param h Verfügbare Höhe
     */
    void computeLayout(int w, int h);

    /**
     * @brief Stellt Standardwerte für den aktiven Modus ein.
     */
    void setDefaultForMode();

    /**
     * @brief Wechselt den aktiven Modus.
     * @param mode Neuer Modus
     */
    void setMode(Mode mode);

    /**
     * @brief Baut m_code aus den aktuell gewählten Spaltenwerten.
     */
    void updateCodeFromSelection();

    /**
     * @brief Wertet den aktuellen Code aus und aktualisiert Ergebnis und Status.
     */
    void decode();

    /**
     * @brief Initialisiert die Footer-Buttons.
     * @param tft Display-Objekt für die Button-Klasse
     */
    void initFooterButtons(LGFX *tft);

    /**
     * @brief Setzt die Farben der Footer-Buttons passend zum aktiven Modus.
     */
    void updateFooterButtonColors();

    /**
     * @brief Zeichnet die Auswahlspalten.
     */
    void drawSelector();

    /**
     * @brief Zeichnet eine einzelne Auswahlzelle.
     * @param col     Spaltenindex
     * @param row     Zeilenindex
     * @param pressed true für Druckzustand
     */
    void drawCell(int col, int row, bool pressed);

    /**
     * @brief Zeichnet die SMD-Bauteilvorschau.
     */
    void drawPreview();

    /**
     * @brief Zeichnet das SMD-Bauteil mit Code-Aufdruck.
     * @param x Linke Kante
     * @param y Obere Kante
     */
    void drawChip(int x, int y);

    /**
     * @brief Zeichnet eine kompakte Tabelle mit gängigen SMD-Baugrößen.
     * @param x Linke Kante
     * @param y Obere Kante
     * @param w Breite
     */
    void drawPackageTable(int x, int y, int w);

    /**
     * @brief Zeichnet die Ergebniszeile.
     */
    void drawResult();

    /**
     * @brief Ermittelt die Auswahlzelle an content-relativen Koordinaten.
     * @param cx     Content-X
     * @param cy     Content-Y
     * @param outCol Ausgabe: Spalte
     * @param outRow Ausgabe: Zeile
     * @return true bei Treffer
     */
    bool hitTestCell(int cx, int cy, int &outCol, int &outRow) const;

    /**
     * @brief Prüft, ob ein Auswahlwert in der Spalte gültig ist.
     * @param col Spaltenindex
     * @param row Zeilenindex
     * @return true wenn die Zelle angezeigt und nutzbar ist
     */
    bool isRowValid(int col, int row) const;

    /**
     * @brief Prüft, ob der aktuelle Code nur aus Ziffern besteht.
     */
    bool isNumericCode() const;

    /**
     * @brief Prüft, ob der aktuelle Code ausschließlich Nullen enthält.
     */
    bool isZeroCode() const;

    /**
     * @brief Dekodiert einen reinen Zifferncode.
     * @param len Code-Länge
     * @param outOhm Ausgabe des Widerstandswerts
     * @return true bei Erfolg
     */
    bool decodeNumeric(size_t len, double &outOhm) const;

    /**
     * @brief Dekodiert einen Code mit R als Dezimaltrennzeichen.
     * @param outOhm Ausgabe des Widerstandswerts
     * @return true bei Erfolg
     */
    bool decodeRCode(double &outOhm) const;

    /**
     * @brief Formatiert einen Widerstandswert.
     * @param ohm Widerstandswert in Ohm
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße
     */
    static void formatResistance(double ohm, char *buf, size_t sz);

    /**
     * @brief Ersetzt Dezimalpunkte durch Kommas.
     * @param buf Zu bearbeitender Puffer
     */
    static void dotToComma(char *buf);

    /**
     * @brief Liefert den passenden Hinweistext für das aktuelle Ergebnis.
     */
    WorkshopKey resultTextKey() const;

    /**
     * @brief Liefert die Anzahl Spalten für den aktiven Modus.
     */
    int activeCols() const;

    /**
     * @brief Liefert die Anzahl Zeilen für den aktiven Modus.
     */
    int activeRows() const;

    Gfx_Sprite *m_sprite = nullptr; ///< Aktueller Work-Sprite
    int m_originX = 0;              ///< Inhaltsursprung X
    int m_originY = 0;              ///< Inhaltsursprung Y
    int m_w = 0;                    ///< Letzte Breite
    int m_h = 0;                    ///< Letzte Höhe
    bool m_layoutValid = false;     ///< true wenn Layout berechnet ist
    bool m_dirty = true;            ///< true wenn neu dekodiert werden muss

    Rect m_cellRects[MAX_COLS][MAX_ROWS] = {}; ///< Auswahlzellen
    Rect m_selectorRect = {};                  ///< Bereich der Auswahlspalten
    Rect m_previewRect = {};                   ///< Bereich der Bauteilvorschau
    Rect m_resultRect = {};                    ///< Ergebniszeile

    Mode m_mode = Mode::Digit3;                                              ///< Aktiver Eingabemodus
    uint8_t m_selectedRows[MAX_COLS] = {};                                   ///< Gewählte Zeile je Spalte
    char m_code[CODE_MAX + 1] = {};                                          ///< Aktueller SMD-Code
    char m_valueStr[32] = {};                                                ///< Formatierter Ergebniswert
    ResultKind m_resultKind = ResultKind::Invalid;                           ///< Ergebnisart
    WorkshopKey m_errorKey = WorkshopKey::WORKSHOP_SMD_RESISTOR_ERR_INVALID; ///< Fehlertext

    int m_pressedCol = -1;     ///< Aktuell gedrückte Auswahlspalte
    int m_pressedRow = -1;     ///< Aktuell gedrückte Auswahlzeile
    bool m_touchWas = false;   ///< Touch war im letzten Frame aktiv
    bool m_touchReady = false; ///< Schutz vor Phantom-Touch

    Button m_modeButtons[NUM_MODES];         ///< Modus-Buttons im Footer
    bool m_footerButtonsInitialized = false; ///< Footer-Buttons wurden initialisiert
    char m_modeLabels[NUM_MODES][16] = {};   ///< Beschriftungen der Footer-Buttons
};

#endif // WORKSHOPTOOL_SMDRESISTOR_H
