#ifndef WORKSHOPTOOL_PERIODICTABLE_H
#define WORKSHOPTOOL_PERIODICTABLE_H

// ============================================================================
// Interaktives Periodensystem der Elemente
// ============================================================================

#include <stdint.h>

#include "Button.h"
#include "Gfx.h"
#include "WorkshopTool.h"

// ============================================================================
// ELEMENTKATEGORIEN
// ============================================================================

/**
 * @brief Klassifikation der chemischen Elemente.
 *
 * Die Reihenfolge entspricht dem Index in den Farb- und Namens-Arrays s_catColors[] und s_catNames[] in der Implementierungsdatei.
 */
enum class ElementCategory : uint8_t
{
    ALKALI_METAL = 0,      ///< Alkalimetalle (Gruppe 1, H ausgenommen)
    ALKALINE_EARTH = 1,    ///< Erdalkalimetalle (Gruppe 2)
    TRANSITION_METAL = 2,  ///< Übergangsmetalle (d-Block)
    POST_TRANSITION = 3,   ///< Post-Übergangsmetalle (p-Block, metallisch)
    METALLOID = 4,         ///< Halbmetalle (Metalloide)
    REACTIVE_NONMETAL = 5, ///< Reaktive Nichtmetalle (inkl. Wasserstoff)
    NOBLE_GAS = 6,         ///< Edelgase (Gruppe 18)
    LANTHANIDE = 7,        ///< Lanthanoide (f-Block, Periode 6)
    ACTINIDE = 8,          ///< Actinoide (f-Block, Periode 7)
    UNKNOWN = 9            ///< Unbekannte / unsichere Eigenschaften
};

// ============================================================================
// ELEMENTDATEN
// ============================================================================

/**
 * @brief Kompakte Datenbeschreibung eines chemischen Elements.
 *
 * Die Struktur ist so angeordnet, dass der float-Wert an einer 4-Byte-Grenze liegt und kein implizites Padding entsteht. Alle Instanzen sind als statisches const-Array in FLASH gespeichert.
 */
struct ElementData
{
    float mass;               ///< Atommasse in atomaren Masseneinheiten (u)
    int16_t year;             ///< Entdeckungsjahr (0 = Antike, -1 = unbekannt)
    uint8_t number;           ///< Ordnungszahl 1-118
    uint8_t col;              ///< Gitterspalte 1-18 (Gruppe)
    uint8_t row;              ///< Gitterzeile 1-7 (Periode), 8 = Lanthanoid, 9 = Actinoid
    ElementCategory category; ///< Elementkategorie
    uint8_t state;            ///< Aggregatzustand bei 25 °C: 0=fest, 1=flüssig, 2=gasförmig
    uint8_t pad;              ///< Padding (nicht verwendet)
    char symbol[4];           ///< Elementsymbol (max. 2 Zeichen + NUL)
    char nameDe[20];          ///< Deutscher Elementname (max. 19 Zeichen + NUL)
    char discoverer[24];      ///< Entdecker (max. 22 Zeichen + NUL, reines ASCII)
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class WorkshopTool_PeriodicTable
 * @brief Interaktives Periodensystem der Elemente für den Workshop.
 *
 * Stellt alle 118 Elemente in einem kompakten Gitterlayout dar. Jedes Element ist antippbar und öffnet ein Detailfenster mit Name, Atommasse, Kategorie, Gruppe/Periode und Entdeckungsjahr.
 *
 * Das Detailfenster wird als Overlay direkt in den Work-Sprite gezeichnet und beim Antippen außerhalb geschlossen, ohne eigene Sprites oder Backup-Puffer. Ein erneutes draw() stellt den ursprünglichen Zustand wieder her.
 *
 * Schriften: ComfortaaBold10 (Ordnungszahl, Detailzeilen),
 *            ComfortaaBold14 (Symbol im Gitter),
 *            ComfortaaBold16 (Elementname im Popup),
 *            LeagueGothicCond34     (großes Symbol im Popup-Kopfbereich).
 *
 * Kategorie-Farben orientieren sich optisch an der Referenz
 * https://artsexperiments.withgoogle.com/periodic-table/
 */
class WorkshopTool_PeriodicTable : public WorkshopTool
{
public:
    /**
     * @brief Initialisiert alle Member mit ihren Standardwerten.
     */
    WorkshopTool_PeriodicTable();

    /**
     * @brief Destruktor (trivial, kein eigener Heap-Besitz).
     */
    ~WorkshopTool_PeriodicTable() override = default;

    WorkshopTool_PeriodicTable(const WorkshopTool_PeriodicTable &) = delete;
    WorkshopTool_PeriodicTable &operator=(const WorkshopTool_PeriodicTable &) = delete;

    /**
     * @brief Wird beim Aktivieren des Tools aufgerufen.
     * @param tft Nicht benötigt (kein eigener Sprite, kein Button)
     *
     * Setzt alle Zustandsvariablen zurück, baut die Lookup-Tabelle auf und erzwingt ein vollständiges Neuzeichnen beim nächsten draw()-Aufruf.
     */
    void onEnter(LGFX *tft) override;

    /**
     * @brief Wird beim Verlassen des Tools aufgerufen.
     *
     * Schließt ein eventuell geöffnetes Detailfenster, damit beim späteren onEnter() ein sauberer Zustand vorliegt.
     */
    void onExit() override;

    /**
     * @brief Zeichnet das Periodensystem (und ggf. das Detailfenster) in den Work-Sprite.
     * @param sprite  Work-Sprite des Workshop-Moduls
     * @param originX X-Ursprung des Inhaltsbereichs (sprite-relativ)
     * @param originY Y-Ursprung des Inhaltsbereichs (sprite-relativ)
     * @param w       Verfügbare Breite des Inhaltsbereichs in Pixeln
     * @param h       Verfügbare Höhe des Inhaltsbereichs in Pixeln
     *
     * Berechnet das Layout beim ersten Aufruf bzw. bei Größenänderung. Zeichnet zuerst die Tabelle, anschließend das Overlay-Popup (wenn aktiv).
     */
    void draw(Gfx_Sprite *sprite, int originX, int originY, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events für Elementauswahl und Popup-Steuerung.
     * @param tx      Absoluter Display-X (bei touched=false vom Workshop immer 0)
     * @param ty      Absoluter Display-Y (bei touched=false vom Workshop immer 0)
     * @param touched true wenn Display berührt
     * @return true wenn ein Neuzeichnen erforderlich ist
     *
     * Popup öffnet beim ersten Touch-down auf einer Zelle. Popup schließt beim Touch-down außerhalb des Popups (sofort) oder beim Touch-up nach einem Touch-down auf dem OK-Button. Da der Workshop bei touched=false stets (0,0) übergibt, wird die letzte bekannte Touch-Position (m_lastTouchX/Y) für den OK-Button-Check auf touch-up verwendet.
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

    // Statische Datenbasis (Definitionen in der .cpp-Datei)
    static const ElementData s_elements[118]; ///< Alle 118 Elemente

private:
    // ========================================================================
    // LAYOUT-KONSTANTEN
    // ========================================================================

    static constexpr int CELL_W = 29;   ///< Zellbreite in Pixeln
    static constexpr int CELL_H = 30;   ///< Zellhöhe in Pixeln
    static constexpr int CELL_GAP = 2;  ///< Abstand zwischen Zellen
    static constexpr int COLS = 18;     ///< Anzahl Spalten (Gruppen 1-18)
    static constexpr int MAIN_ROWS = 7; ///< Haupttabellenzeilen (Perioden 1-7)
    static constexpr int LANT_ROWS = 2; ///< Lanthanoid/Actinoid-Zeilen
    static constexpr int LANT_GAP = 7;  ///< Abstand zwischen Haupttabelle und Lanthanoid-Zeilen

    // Zur Übersetzungszeit berechnete Geometrie
    static constexpr int TABLE_W = COLS * (CELL_W + CELL_GAP) - CELL_GAP;
    static constexpr int TABLE_MAIN_H = MAIN_ROWS * (CELL_H + CELL_GAP) - CELL_GAP;
    static constexpr int LANT_H = LANT_ROWS * (CELL_H + CELL_GAP) - CELL_GAP;
    static constexpr int TOTAL_TABLE_H = TABLE_MAIN_H + LANT_GAP + LANT_H;

    // Popup-Geometrie (MessageBox-konformer Stil)
    static constexpr int POPUP_W = 340;        ///< Popup-Breite
    static constexpr int POPUP_H = 265;        ///< Popup-Gesamthöhe
    static constexpr int POPUP_RADIUS = 4;     ///< Eckenradius (wie MessageBox)
    static constexpr int POPUP_PAD = 16;       ///< Innen-Padding
    static constexpr int POPUP_SEP_Y = 50;     ///< Y-Position der Trennlinie
    static constexpr int POPUP_TXT_Y = 62;     ///< Y-Start Inhaltsbereich
    static constexpr int POPUP_LINE_H = 22;    ///< Zeilenabstand Detailtexte
    static constexpr int POPUP_HDR_SYM = 29;   ///< Icon-Breite = CELL_W
    static constexpr int POPUP_HDR_SYM_H = 30; ///< Icon-Höhe  = CELL_H
    static constexpr int POPUP_BTN_W = 80;     ///< OK-Button-Breite
    static constexpr int POPUP_BTN_H = 45;     ///< OK-Button-Höhe, wie in den übrigen Dialogen
    static constexpr int POPUP_LABEL_W = 120;  ///< Breite der Label-Spalte (für Wert-Ausrichtung)

    // Legende
    static constexpr int LEGEND_TOP_GAP = 11; ///< Abstand zwischen Tabelle und Legende
    static constexpr int LEGEND_SQ = 9;       ///< Seitenlänge des Farbquadrats
    static constexpr int LEGEND_TXT_GAP = 4;  ///< Abstand zwischen Quadrat und Text
    static constexpr int LEGEND_ROW_H = 14;   ///< Höhe einer Legendenzeile
    static constexpr int LEGEND_COLS = 5;     ///< Einträge pro Zeile

    // Farben der Tabelle und des Detailfensters
    static constexpr uint16_t C_POPUP_BG = 0x0000;     ///< Hintergrund (Theme::ColorMainBackGround)
    static constexpr uint16_t C_POPUP_BORDER = 0x24F2; ///< Rahmen (Theme::ColorDialogBorder)
    static constexpr uint16_t C_POPUP_TITLE = 0xEC84;  ///< Titelfarbe (Theme::ColorTitle)
    static constexpr uint16_t C_POPUP_SEP = 0xD206;    ///< Trennlinie (Theme::ColorDialogSeparator)
    static constexpr uint16_t C_POPUP_TXT = 0xBDF7;    ///< Inhaltstext (UIConfig::COLOR_TEXT)
    static constexpr uint16_t C_POPUP_DIM = 0x8410;    ///< Gedimmter Label-Text

    // Tabellen-interne Farben (Magic Colors als benannte Konstanten)
    static constexpr uint16_t C_WORK_BG = 0x1082;    ///< Hintergrundfarbe des Tool-Bereichs (dunkelgrau)
    static constexpr uint16_t C_LEGEND_TXT = 0xC618; ///< Sekundäre Textfarbe: OZ in Zellen, Legendentext (hellgrau)

    // ========================================================================
    // INTERNE METHODEN
    // ========================================================================

    /**
     * @brief Berechnet und cached die Tischposition im Inhaltsbereich.
     * @param w Verfügbare Breite
     * @param h Verfügbare Höhe
     *
     * Wird nur ausgeführt wenn m_layoutValid false ist oder w/h sich geändert haben.
     */
    void computeLayout(int w, int h);

    /**
     * @brief Zeichnet alle Elementzellen in den Sprite (3-Pass: Füllung, OZ, Symbol).
     * @param spr Ziel-Sprite
     *
     * Schriften werden einmal je Zeichendurchlauf geladen und nicht für jede Zelle gewechselt.
     */
    void drawTable(Gfx_Sprite *spr);

    /**
     * @brief Zeichnet die Kategorie-Farblegende unterhalb der Tabelle.
     * @param spr Ziel-Sprite
     */
    void drawLegend(Gfx_Sprite *spr);

    /**
     * @brief Zeichnet das Detail-Popup für ein Element.
     * @param spr Ziel-Sprite
     */
    void drawPopup(Gfx_Sprite *spr);

    /**
     * @brief Berechnet die Sprite-relative Position einer Zelle.
     * @param e    Elementdaten
     * @param outX Ausgabe: Sprite-X der linken oberen Zellenecke
     * @param outY Ausgabe: Sprite-Y der linken oberen Zellenecke
     */
    void getCellSpritePos(const ElementData &e, int &outX, int &outY) const;

    /**
     * @brief O(1)-Treffertest via vorberechneter Lookup-Tabelle.
     * @param cx Content-relatives X (= tx - WORK_X - m_originX)
     * @param cy Content-relatives Y
     * @return Element-Index (0-basiert) oder -1
     */
    int hitTestCell(int cx, int cy) const;

    /**
     * @brief Prüft ob eine content-relative Position innerhalb des Popups liegt.
     */
    bool hitTestPopup(int cx, int cy) const;

    /**
     * @brief Prüft ob eine content-relative Position den OK-Button trifft.
     * @param cx Content-relatives X
     * @param cy Content-relatives Y
     * @return true wenn Position im OK-Button-Rechteck
     */
    bool hitTestOkButton(int cx, int cy) const;

    /**
     * @brief Baut die statische Zuordnung von (row-1, col-1) zu elemIdx auf.
     *
     * Wird einmalig in onEnter() aufgerufen; ermöglicht O(1)-Treffertest.
     */
    void initLookup();

    // ========================================================================
    // MEMBER-VARIABLEN
    // ========================================================================

    // Layout-Cache
    int m_originX = 0;          ///< Gespeichertes originX aus letztem draw()
    int m_originY = 0;          ///< Gespeichertes originY aus letztem draw()
    int m_contentW = 0;         ///< Gespeicherte Breite aus letztem draw()
    int m_contentH = 0;         ///< Gespeicherte Höhe aus letztem draw()
    bool m_layoutValid = false; ///< Layout bereits berechnet und gültig

    // Tabellen-Ursprung (content-relativ, nach computeLayout() gültig)
    int m_tableX = 0; ///< X-Ursprung der Tabelle im Inhaltsbereich
    int m_tableY = 0; ///< Y-Ursprung der Tabelle im Inhaltsbereich

    // Popup-Ursprung (content-relativ, nach computeLayout() gültig)
    int m_popupX = 0; ///< X-Ursprung des Popups im Inhaltsbereich
    int m_popupY = 0; ///< Y-Ursprung des Popups im Inhaltsbereich

    // Interaktions-Zustand
    int m_selectedElem = -1;    ///< Element-Index des offenen Popups (-1 = kein Popup)
    bool m_wasPressed = false;  ///< Touch war aktiv im letzten Frame
    LGFX *m_tft = nullptr;      ///< Display, nur zur Weitergabe an den Knopf
    Button m_okButton;          ///< Knopf des Aufklappers, damit er wie alle anderen aussieht
    bool m_pressedOnOk = false; ///< Touch-down traf den OK-Button (für Touch-up-Check)
    bool m_tableDirty = true;   ///< Tabelle beim nächsten draw() neu zeichnen
    bool m_popupDirty = false;  ///< Popup beim nächsten draw() neu zeichnen
    int32_t m_lastTouchX = 0;   ///< Zuletzt bekannte Touch-X (wird bei touched=true aktualisiert)
    int32_t m_lastTouchY = 0;   ///< Zuletzt bekannte Touch-Y

    // O(1)-Zuordnung von [row-1][col-1] zum Index in s_elements[]; -1 kennzeichnet ein leeres Feld
    static int8_t s_cellLookup[9][18]; ///< Vorberechnete Gitter-Lookup-Tabelle
    static bool s_lookupBuilt;         ///< Lookup einmalig aufgebaut
};

#endif // WORKSHOPTOOL_PERIODICTABLE_H
