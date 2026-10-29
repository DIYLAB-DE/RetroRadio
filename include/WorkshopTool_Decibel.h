#ifndef WORKSHOPTOOL_DECIBEL_H
#define WORKSHOPTOOL_DECIBEL_H

#include <stddef.h>
#include <stdint.h>

#include "Strings_workshop.h"
#include "WorkshopNumpad.h"
#include "WorkshopTool.h"

/**
 * @class WorkshopTool_Decibel
 * @brief Interaktiver Dezibel-Rechner mit zwei unabhängigen Sektionen.
 *
 * Zwei Sektionen sind gleichzeitig sichtbar; ein Umschalten ist nicht erforderlich:
 *
 * Sektion Spannung (Spannungspegel, Faktor 20):
 *   dB = 20 * log10(V2 / V1)
 *   V1 = V2 / 10^(dB / 20)
 *   V2 = V1 * 10^(dB / 20)
 *
 * Sektion Leistung (Leistungspegel, Faktor 10):
 *   dB = 10 * log10(P2 / P1)
 *   P1 = P2 / 10^(dB / 10)
 *   P2 = P1 * 10^(dB / 10)
 *
 * Jede Sektion besitzt drei Felder (X1, X2, dB) mit FIFO-2-Logik: immer zwei Felder sind Eingaben, das dritte wird berechnet. Tippt der Nutzer auf die berechnete Zeile, wird sie zur Eingabe; der älteste Eingabe-Slot wird zur neuen berechneten Größe (FIFO-Verdrängung).
 *
 * Das gemeinsame Nummernpad wirkt stets auf die aktive Sektion. Die aktive Sektion wird durch Tippen auf eines ihrer Felder gesetzt. Ein farbiger Akzentbalken im Sektionskopf zeigt die aktive Sektion an.
 *
 * Layout (547 × 327 px Netto-Inhaltsbereich):
 *   TOP_GAP                 10 px
 *   2 × Sektionskopf         2 × 18 px = 36 px
 *   2 × 3 Eingabezeilen      6 × 25 px = 150 px
 *   Sektionsabstand           6 px
 *   Restabstand zum Numpad    5 px
 *   Nummernpad (REQUIRED_H) 120 px (bottom-anchored)
 *   Summe                   327 px (exakt)
 *
 * Standardwerte beim Öffnen:
 *   Spannung: V1 = 1 V, V2 = 2 V -> dB = 6,02 (berechnet)
 *   Leistung: P1 = 1 W, P2 = 2 W -> dB = 3,01 (berechnet)
 *   Aktive Sektion beim Start: Spannung (Index 0).
 *
 * Einheitenskalierung:
 *   V1, V2: automatisch uV / mV / V / kV (Micro als "u", kein µ-Sonderzeichen)
 *   P1, P2: automatisch uW / mW / W / kW
 *   dB: stets als "%.2f dB", keine Skalierung
 *
 * Cursor blinkt in der fokussierten Zeile der aktiven Sektion (400 ms an / 400 ms aus via millis()).
 *
 * Zeichnet ausschließlich in den übergebenen Work-Sprite (kein eigener Sprite).
 */
class WorkshopTool_Decibel : public WorkshopTool
{
public:
    /**
     * @brief Konstruktor. Setzt alle Member und Standardwerte.
     *
     * Ruft clearAll() auf, da PSRAM nicht vorinitialisiert ist.
     */
    WorkshopTool_Decibel();

    /** @brief Destruktor. Keine eigenen Ressourcen vorhanden. */
    ~WorkshopTool_Decibel() override = default;

    WorkshopTool_Decibel(const WorkshopTool_Decibel &)            = delete;
    WorkshopTool_Decibel &operator=(const WorkshopTool_Decibel &) = delete;

    /**
     * @brief Setzt den Rechner beim Modul-Einstieg in den Standardzustand zurück.
     * @param tft Display-Objekt (nicht dauerhaft gespeichert)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Kein Aufräumaufwand beim Verlassen. */
    void onExit() override {}

    /**
     * @brief Zeichnet beide Sektionen und das Nummernpad vollständig.
     * @param sprite   Work-Sprite des Workshop-Moduls
     * @param originX  Linke Kante des Inhaltsbereichs (sprite-relativ)
     * @param originY  Obere Kante des Inhaltsbereichs (sprite-relativ)
     * @param w        Verfügbare Breite in Pixeln (typisch 547)
     * @param h        Verfügbare Höhe in Pixeln (typisch 327)
     *
     * m_originX/m_originY werden auf originX/originY gesetzt; computeLayout() berechnet alle Rechtecke bei geänderter Geometrie neu. Das Nummernpad wirkt stets auf die fokussierte Größe der aktiven Sektion. Cursor blinkt nur in der aktiven Sektion.
     */
    void draw(Gfx_Sprite *sprite,
              int originX, int originY,
              int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Events für Felder und Nummernpad-Tasten.
     * @param tx      Absoluter Display-X (bei touched=false ignoriert)
     * @param ty      Absoluter Display-Y (bei touched=false ignoriert)
     * @param touched true bei aktivem Touch
     * @return true wenn Neuzeichnung des Arbeitsbereichs erforderlich
     *
     * Touch-up-Semantik: visuelles Feedback beim Drücken, Aktion erst beim Loslassen. Phantom-Touch-Schutz über m_touchReady (ein Flag für beide Sektionen gemeinsam).
     *
     * Feld-Tap:
     *   Eingabefeld -> Sektion aktivieren und Fokus auf dieses Feld setzen.
     *   Berechnetes Feld -> Sektion aktivieren und FIFO-Verdrängung ausführen.
     *                      (ältester Slot wird berechnet, getipptes Feld
     *                      wird zur neuesten Eingabe).
     * Tasten-Tap:
     *   Aktion wirkt auf das fokussierte Feld der aktiven Sektion.
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    // =========================================================================
    // Innere Typen
    // =========================================================================

    /**
     * @brief Zustandsdaten einer Berechnungssektion (Spannung oder Leistung).
     *
     * Kapselt FIFO-Slots, Eingabepuffer, SI-Werte und Berechnungsstatus. Alle Zeichen- und Berechnungsfunktionen erhalten die betroffene Sektion als Parameter. Alle Felder müssen explizit initialisiert werden, da PSRAM nicht vorgenullt ist.
     */
    struct Section
    {
        uint8_t fifoSlots[2]    = {}; ///< Eingabe-Slots: [0] = ältester, [1] = neuester
        uint8_t computedQty     = 0;  ///< Berechnete Größe (0=X1, 1=X2, 2=dB)
        uint8_t focusedQty      = 0;  ///< Fokussiertes Feld (empfängt Tastatureingabe)
        char    inputBuf[3][24] = {}; ///< Eingabepuffer für alle drei Felder (SI-Rohwert)
        double  values[3]       = {}; ///< SI-Werte aller drei Felder (V bzw. W, dB)
        bool    computeOk       = false; ///< true wenn letzte Berechnung gültig war
    };

    /** @brief Achsenparalleles Rechteck, content-relativ zum Inhaltsursprung. */
    struct Rect
    {
        int x; ///< Linke Kante in Pixeln
        int y; ///< Obere Kante in Pixeln
        int w; ///< Breite in Pixeln
        int h; ///< Höhe in Pixeln
    };

    // =========================================================================
    // Konstanten
    // =========================================================================

    static constexpr int NUM_SEC   = 2;  ///< Anzahl Sektionen (Spannung, Leistung)
    static constexpr int NUM_QTY   = 3;  ///< Größen je Sektion (X1, X2, dB)
    static constexpr int NUM_FIFO  = 2;  ///< Eingabe-Slots je Sektion (FIFO-2)
    static constexpr int FIELD_CAP = 24; ///< Eingabepuffer-Kapazität inkl. NUL

    // Feld-Layout
    static constexpr int TOP_GAP          = 10; ///< Abstand von originY bis erste Sektion in Pixeln
    static constexpr int SECTION_HEADER_H = 18; ///< Höhe des Sektionskopfes in Pixeln
    static constexpr int ROW_H            = 25; ///< Höhe einer Größen-Zeile in Pixeln
    static constexpr int SECTION_GAP      =  6; ///< Abstand zwischen den beiden Sektionen in Pixeln

    /// Höhe einer vollständigen Sektion inkl. Sektionskopf
    static constexpr int SEC_ROWS_H = SECTION_HEADER_H + NUM_QTY * ROW_H;

    // Sektionskopf
    static constexpr int HEADER_ACCENT_W = 4; ///< Breite des Akzentbalkens links im Sektionskopf in Pixeln

    // Feld-Detaillierung
    static constexpr int LABEL_W  = 40; ///< Breite der Symbol-Spalte (V1/V2/dB/P1/P2) in Pixeln
    static constexpr int UNIT_W   = 56; ///< Breite der Einheiten-Spalte in Pixeln
    static constexpr int ACCENT_W =  4; ///< Breite des linken Farbakzent-Streifens je Zeile in Pixeln

    // Cursor-Blinken
    static constexpr unsigned long CURSOR_PERIOD_MS = 800; ///< Gesamtdauer eines Blink-Zyklus in ms
    static constexpr unsigned long CURSOR_ON_MS     = 400; ///< Einschaltdauer innerhalb des Zyklus in ms

    // Farbpalette der Bedienoberfläche im RGB565-Format
    static constexpr uint16_t C_BG          = 0x1082; ///< Arbeitsbereich-Hintergrund
    static constexpr uint16_t C_FIELD_INPUT = 0x2945; ///< Nicht-fokussiertes Feld
    static constexpr uint16_t C_FIELD_FOCUS = 0x328A; ///< Fokussiertes Feld (Petrol, = WorkshopCfg::C_SELECTED)
    static constexpr uint16_t C_FIELD_PRESS = 0x4B4D; ///< Tap-Feedback
    static constexpr uint16_t C_SEP         = 0x2104; ///< Trennlinie zwischen Zeilen

    // Textfarben je Feld-Zustand (fokussiert / Eingabe / berechnet)
    static constexpr uint16_t C_LBL_FOCUS  = 0xFFFF; ///< Symbol fokussiert
    static constexpr uint16_t C_LBL_INPUT  = 0xDEDB; ///< Symbol Eingabe
    static constexpr uint16_t C_LBL_COMP   = 0x7BEF; ///< Symbol berechnet
    static constexpr uint16_t C_NAME_FOCUS = 0xB5F7; ///< Langname fokussiert
    static constexpr uint16_t C_NAME_INPUT = 0x8410; ///< Langname Eingabe
    static constexpr uint16_t C_NAME_COMP  = 0x4228; ///< Langname berechnet
    static constexpr uint16_t C_VAL_FOCUS  = 0xFFFF; ///< Wert fokussiert
    static constexpr uint16_t C_VAL_INPUT  = 0xEF7D; ///< Wert Eingabe
    static constexpr uint16_t C_VAL_COMP   = 0xB5F7; ///< Wert berechnet
    static constexpr uint16_t C_UNIT_FOCUS = 0xAD75; ///< Einheit fokussiert
    static constexpr uint16_t C_UNIT_INPUT = 0x8410; ///< Einheit Eingabe
    static constexpr uint16_t C_UNIT_COMP  = 0x4228; ///< Einheit berechnet

    // Sektionskopf-Farben
    static constexpr uint16_t C_SEC_HDR_BG       = 0x2104; ///< Sektionskopf-Hintergrund (beide)
    static constexpr uint16_t C_SEC_HDR_TXT      = 0xAD75; ///< Sektionskopf-Text inaktiv
    static constexpr uint16_t C_SEC_HDR_TXT_ACT  = 0xFFFF; ///< Sektionskopf-Text aktiv
    static constexpr uint16_t C_SEC_ACCENT_V     = 0xFD20; ///< Akzentbalken Spannung aktiv (Orange)
    static constexpr uint16_t C_SEC_ACCENT_P     = 0x04FF; ///< Akzentbalken Leistung aktiv (Cyan)

    // Feld-Akzentfarben (linker Streifen)
    static constexpr uint16_t C_ACCENT_X1 = 0x07E0; ///< Eingang X1 (Grün)
    static constexpr uint16_t C_ACCENT_X2 = 0xFD20; ///< Ausgang X2 (Orange)
    static constexpr uint16_t C_ACCENT_DB = 0xB5F7; ///< dB-Feld (Cyan-Weiß)

    // =========================================================================
    // Statische Datenbasis
    // =========================================================================

    /** @brief Kurzsymbole je Sektion (Spannung: V1, V2, dB). */
    static const char * const QTY_SYMBOLS_V[NUM_QTY];

    /** @brief Kurzsymbole je Sektion (Leistung: P1, P2, dB). */
    static const char * const QTY_SYMBOLS_P[NUM_QTY];

    /** @brief Liefert die lokalisierbaren Langnamen der Größen für beide Sektionen. */
    static const WorkshopKey QTY_NAME_KEYS[NUM_QTY];

    /** @brief Basiseinheiten Spannung: V, V, dB. */
    static const char * const QTY_UNITS_V[NUM_QTY];

    /** @brief Basiseinheiten Leistung: W, W, dB. */
    static const char * const QTY_UNITS_P[NUM_QTY];

    /** @brief Sektionskopf-Titel je Sektion (lokalisierbar): Spannung / Leistung. */
    static const WorkshopKey SECTION_TITLE_KEYS[NUM_SEC];

    // =========================================================================
    // Initialisierung
    // =========================================================================

    /**
     * @brief Setzt alle Member und beide Sektionen auf definierte Ausgangswerte.
     *
     * PSRAM ist nicht vorinitialisiert; alle Puffer werden explizit per memset genullt. Danach werden die Standardwerte gesetzt und compute() aufgerufen.
     *
     * Standardbelegung:
     *   Spannung: V1=1 V (Slot 0, ältester), V2=2 V (Slot 1, neuester, fokussiert),
     *             dB wird berechnet -> 6,02.
     *   Leistung: P1=1 W (Slot 0, ältester), P2=2 W (Slot 1, neuester, fokussiert),
     *             dB wird berechnet -> 3,01.
     *   Aktive Sektion: 0 (Spannung).
     */
    void clearAll();

    /**
     * @brief Berechnet alle Feld-Rechtecke und das Numpad-Layout.
     * @param w Verfügbare Breite in Pixeln
     * @param h Verfügbare Höhe in Pixeln
     *
     * Alle Rechtecke sind content-relativ (Ursprung = originX/originY). Das Nummernpad ist bottom-anchored und belegt die unteren REQUIRED_H Pixel.
     */
    void computeLayout(int w, int h);

    // =========================================================================
    // Berechnung
    // =========================================================================

    /**
     * @brief Parst Eingabepuffer einer Sektion und berechnet die Ausgabegröße.
     * @param sec       Zu berechnende Sektion (in/out: values, computeOk)
     * @param isVoltage true für Spannungssektion (Faktor 20), false für Leistung (10)
     *
     * Liest die zwei Eingabe-Slots aus den Puffern (atof), berechnet die dritte Größe (computedQty) und schreibt das Ergebnis in sec.values[computedQty]. Setzt sec.computeOk = true bei numerisch gültigem Ergebnis.
     *
     * Formeln je computedQty (Faktor f = isVoltage ? 20 : 10):
     *   2 (dB):  dB = f * log10(X2 / X1)  [X1 != 0, X2/X1 > 0]
     *   0 (X1):  X1 = X2 / 10^(dB / f)
     *   1 (X2):  X2 = X1 * 10^(dB / f)
     *
     * sec.computeOk wird zu Beginn zurückgesetzt (vor allen frühen Returns), damit bei ungültigen Eingaben keine veralteten Werte angezeigt bleiben.
     */
    void compute(Section &sec, bool isVoltage);

    /**
     * @brief Formatiert einen SI-Spannungswert als auto-skalierten Anzeigestring.
     * @param val SI-Wert in Volt
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße in Bytes
     *
     * Skalierung: uV / mV / V / kV. Ungültige Werte wie NaN oder Inf ergeben "---". Micro-Präfix als "u" (ASCII, kein µ-Sonderzeichen). Dezimalpunkt wird für die Anzeige durch ein Komma ersetzt.
     */
    static void formatVoltage(double val, char *buf, size_t sz);

    /**
     * @brief Formatiert einen SI-Leistungswert als auto-skalierten Anzeigestring.
     * @param val SI-Wert in Watt
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße in Bytes
     *
     * Skalierung: uW / mW / W / kW. Ungültige Werte ergeben "---". Micro-Präfix als "u" (ASCII). Dezimalpunkt wird durch Komma ersetzt.
     */
    static void formatPower(double val, char *buf, size_t sz);

    /**
     * @brief Formatiert einen dB-Wert als Anzeigestring.
     * @param val dB-Wert
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße in Bytes
     *
     * Format: "%.2f dB". Ungültige Werte wie NaN oder Inf ergeben "---". Keine Einheitenskalierung (dB ist bereits eine relative Größe). Dezimalpunkt wird durch Komma ersetzt.
     */
    static void formatDb(double val, char *buf, size_t sz);

    /**
     * @brief Formatiert den Anzeigewert eines Feldes je nach Sektion und Größen-Index.
     * @param sec       Quell-Sektion
     * @param qty       Größen-Index (0=X1, 1=X2, 2=dB)
     * @param isVoltage true für Spannungssektion (V-Skalierung), false für Leistung (W)
     * @param buf       Ausgabepuffer
     * @param sz        Puffergröße in Bytes
     *
     * Leitet an formatVoltage(), formatPower() oder formatDb() weiter.
     */
    static void formatDisplayValue(const Section &sec, int qty, bool isVoltage,
                                   char *buf, size_t sz);

    /**
     * @brief Formatiert einen SI-Rohwert als editierbaren Dezimalstring (%.6g).
     * @param val SI-Wert
     * @param buf Ausgabepuffer
     * @param sz  Puffergröße in Bytes
     *
     * Wird beim Wechsel einer berechneten Zeile zur Eingabe verwendet. Werte von 0.0 oder nicht endliche Werte ergeben "0". Liefert stets einen Punkt als Dezimaltrenner, da der Puffer per atof() weiterverarbeitet wird.
     */
    static void formatEditValue(double val, char *buf, size_t sz);

    /**
     * @brief Gibt zurück ob eine Größe als Eingabe-Slot aktiv ist.
     * @param sec Sektion
     * @param qty Größen-Index (0-2)
     * @return true wenn qty in sec.fifoSlots[] enthalten ist
     */
    static bool isInput(const Section &sec, int qty);

    /**
     * @brief Gibt die Akzentfarbe des linken Streifens einer Größen-Zeile zurück.
     * @param qty 0 für Grün (Eingang), 1 für Orange (Ausgang), 2 für Cyan-Weiß (dB)
     */
    static uint16_t fieldAccentColor(int qty);

    /**
     * @brief Gibt die Akzentbalken-Farbe des Sektionskopfes zurück.
     * @param secIdx 0 für Orange (Spannung), 1 für Cyan (Leistung)
     */
    static uint16_t sectionAccentColor(int secIdx);

    // =========================================================================
    // Tastatureingabe
    // =========================================================================

    /**
     * @brief Führt eine Tastenaktion auf dem fokussierten Feld der aktiven Sektion aus.
     * @param action '0'-'9' = Ziffer, '.' = Dezimalpunkt, 'S' = Vorzeichen togglen,
     *               'B' = Backspace, 'C' = Clear
     *
     * Modifiziert sec.inputBuf[sec.focusedQty] und ruft anschließend compute() auf.
     */
    void executeKey(char action);

    // =========================================================================
    // Zeichnen
    // =========================================================================

    /**
     * @brief Zeichnet den Sektionskopf einer Sektion.
     * @param secIdx  Sektionsindex (0=Spannung, 1=Leistung)
     * @param headerY Y-Position des Sektionskopfes (content-relativ)
     * @param active  true wenn diese Sektion die aktive ist
     *
     * Aktive Sektion: Akzentbalken links + heller Text. Inaktive Sektion: kein Balken + gedimmter Text. Setzt voraus, dass ComfortaaBold16 bereits geladen ist.
     */
    void drawSectionHeader(int secIdx, int headerY, bool active);

    /**
     * @brief Zeichnet eine einzelne Größen-Zeile.
     * @param sec      Sektion der Zeile
     * @param secIdx   Sektionsindex (0 oder 1, für Symbol- und Einheitenauswahl)
     * @param qty      Größen-Index (0=X1, 1=X2, 2=dB)
     * @param fieldY   Y-Position der Zeile (content-relativ)
     * @param w        Breite des Inhaltsbereichs in Pixeln
     * @param pressed  true für Tap-Feedback (C_FIELD_PRESS)
     * @param cursorOn true wenn Cursor sichtbar sein soll
     *
     * Fokussiert wird eine Zeile nur wenn sie in der aktiven Sektion liegt und das fokussierte Feld dieser Sektion ist. Setzt voraus, dass ComfortaaBold16 bereits geladen ist.
     */
    void drawField(const Section &sec, int secIdx, int qty,
                   int fieldY, int w,
                   bool pressed, bool cursorOn);

    // =========================================================================
    // Treffertests
    // =========================================================================

    /**
     * @brief Treffertest auf alle Felder beider Sektionen (content-relativ).
     * @param cx        Content-relative X-Position
     * @param cy        Content-relative Y-Position
     * @param outSecIdx Sektionsindex des getroffenen Feldes (0 oder 1)
     * @param outQty    Größen-Index des getroffenen Feldes (0, 1 oder 2)
     * @return true wenn ein Feld getroffen wurde
     */
    bool hitTestField(int cx, int cy, int &outSecIdx, int &outQty) const;

    // =========================================================================
    // Member-Variablen
    // =========================================================================

    Gfx_Sprite *m_sprite  = nullptr; ///< Aktueller Work-Sprite (in draw() gesetzt)
    int  m_originX        = 0;       ///< Inhaltsursprung X (sprite-relativ)
    int  m_originY        = 0;       ///< Inhaltsursprung Y (sprite-relativ)
    int  m_w              = 0;       ///< Zuletzt verwendete Netto-Breite
    int  m_h              = 0;       ///< Zuletzt verwendete Netto-Höhe
    bool m_layoutValid    = false;   ///< true wenn Layout-Rects aktuell berechnet sind

    // Sektionsdaten
    Section m_sections[NUM_SEC] = {}; ///< Zustandsdaten beider Sektionen
    int     m_activeSection     = 0;  ///< Aktive Sektion (0=Spannung, 1=Leistung)

    // Layout-Rects (content-relativ, berechnet in computeLayout)
    int  m_headerY[NUM_SEC]              = {}; ///< Y-Positionen der Sektionsköpfe
    Rect m_fieldRects[NUM_SEC][NUM_QTY]  = {}; ///< Feld-Rechtecke [Sektion][Größe]

    WorkshopNumpad m_numpad; ///< Nummernpad-Verwaltung (Layout, Zeichnung, Treffertest)

    // Touch-Tracking
    int  m_pressedSecIdx = -1;    ///< Sektionsindex des gedrückten Feldes (-1 = keines)
    int  m_pressedField  = -1;    ///< Größen-Index des gedrückten Feldes (-1 = keines)
    int  m_pressedKey    = -1;    ///< Index der gedrückten Nummernpad-Taste (-1 = keine)
    bool m_touchWas      = false; ///< true wenn im letzten Frame ein Touch aktiv war
    bool m_touchReady    = false; ///< Phantom-Touch-Schutz: aktiv nach erstem touched=false-Frame
};

#endif // WORKSHOPTOOL_DECIBEL_H
