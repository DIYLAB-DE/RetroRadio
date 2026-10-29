#ifndef WORKSHOP_H
#define WORKSHOP_H

#include <stddef.h>
#include <stdint.h>

#include "AppState.h"
#include "BaseModule.h"
#include "Button.h"
#include "Clocks.h"
#include "DS1307_RTC.h"
#include "Gfx.h"
#include "Logger.h"
#include "StringUtils.h"
#include "Strings_workshop.h"
#include "Theme.h"
#include "WorkshopTool.h"

// ============================================================================
// KONFIGURATION
// ============================================================================

/**
 * @brief Layoutkonstanten und Farbpalette.
 */
namespace WorkshopCfg
{
    // Display
    constexpr int SCREEN_W = 800;                   ///< Displaybreite in Pixeln
    constexpr int SCREEN_H = 480;                   ///< Displayhöhe in Pixeln
    constexpr int FOOTER_H = 56;                    ///< Fußzeilenhöhe in Pixeln
    constexpr int VIEWPORT_H = SCREEN_H - FOOTER_H; ///< Nutzbarer Bereich
    constexpr int FOOTER_Y = VIEWPORT_H;            ///< Y-Start der Fußzeile
    constexpr int MENU_SPRITE_H = SCREEN_H;

    // Scrollbar
    constexpr uint16_t C_SCROLLBAR_TRACK = 0x2104;  ///< Scrollbar-Track
    constexpr uint16_t C_SCROLLBAR_DRAG = 0x8410;   ///< Scrollbar-Thumb beim Drag
    constexpr uint16_t C_SCROLLBAR_NORMAL = 0x5ACB; ///< Scrollbar-Thumb normal

    // Linke Seite: Menüliste
    constexpr int MENU_X = 2;                              ///< Linke Kante der Menüliste
    constexpr int MENU_W = 201;                            ///< Breite der Menüliste
    constexpr int SCROLLBAR_X = MENU_X + MENU_W + 4;       ///< Linke Kante der Scrollbar (4 px Abstand nach Menü)
    constexpr int SCROLLBAR_W = 6;                         ///< Breite der Scrollbar
    constexpr int WORK_X = SCROLLBAR_X + SCROLLBAR_W + 10; ///< Linke Kante des Arbeitsbereichs (10 px Abstand nach Scrollbar)
    constexpr int DIVIDER_X = SCROLLBAR_X + SCROLLBAR_W;   ///< Grenze Menübereich Arbeitsbereich
    constexpr int ITEM_ACCENT_W = 4;                       ///< Breite des Kategorie-Farbstreifens
    constexpr int ITEM_TEXT_X = ITEM_ACCENT_W + 6;         ///< Texteinzug nach Akzent-Balken

    // Rechte Seite: Arbeitsbereich
    constexpr int WORK_Y = 5;                      ///< Abstand Display-oben bis Contentbereich-oben
    constexpr int WORK_W = SCREEN_W - WORK_X - 10; ///< Abstand zum rechten Displayrand
    constexpr int WORK_H = VIEWPORT_H - WORK_Y;    ///< Höhe

    // Sprite-Hintergrund
    constexpr uint16_t C_WORK_BG = 0x1082;       ///< Arbeitsbereich-Hintergrund
    constexpr uint16_t C_WORK_TITLE_BG = 0x2104; ///< Titelbox-Hintergrund

    // Eckenradius
    constexpr int WORK_CORNER_R = 8; ///< Eckenradius für alle RoundRects im Arbeitsbereich

    // Titelbox - Margin 10px, Padding 10px
    constexpr int TITLE_MARGIN = 10;                                         ///< Abstand Titelbox zum Sprite-Rand (alle Seiten)
    constexpr int TITLE_PADDING = 10;                                        ///< Texteinzug innerhalb der Titelbox
    constexpr int TITLE_BOX_X = TITLE_MARGIN;                                ///< Titelbox X (sprite-relativ)
    constexpr int TITLE_BOX_Y = TITLE_MARGIN;                                ///< Titelbox Y (sprite-relativ)
    constexpr int TITLE_BOX_H = 69;                                          ///< Titelbox Höhe
    constexpr int TITLE_TEXT_X = TITLE_BOX_X + TITLE_PADDING;                ///< X-Position Haupttitel
    constexpr int TITLE_TEXT_Y = TITLE_BOX_Y + TITLE_PADDING - 5;            ///< Y-Position Haupttitel
    constexpr int SUB_TEXT_Y = TITLE_BOX_Y + TITLE_BOX_H - 26;               ///< Y-Position Untertitel
    constexpr int WORK_CONTENT_Y = TITLE_BOX_Y + TITLE_BOX_H + TITLE_MARGIN; ///< Y-Start Inhaltsbereich
    constexpr int HEADER_QR_BTN_SIZE = 60;                                   ///< Seitenlänge des Website-Buttons im Header
    constexpr int HEADER_QR_BTN_PAD_R = 5;                                   ///< Abstand des Website-Buttons zum rechten Titelbox-Rand
    constexpr int HEADER_QR_BTN_Y = TITLE_BOX_Y + 5;                         ///< Y-Position des Website-Buttons im Header

    // Listenzeilen
    constexpr int SECTION_ROW_H = 24; ///< Zeilenhöhe für Sektionsüberschrift
    constexpr int ITEM_ROW_H = 44;    ///< Zeilenhöhe für einen Werkzeug-Eintrag

    // Fußzeile
    constexpr int FOOTER_BTN_CY = FOOTER_Y + FOOTER_H / 2; ///< Vertikal-Mitte der Buttons

    // Exit-Button
    constexpr int EXIT_BTN_SIZE = 44;                              ///< Seitenlänge des Exit-Buttons
    constexpr int EXIT_BTN_CX = SCREEN_W - EXIT_BTN_SIZE / 2 - 10; ///< X-Mitte Exit-Button (10 px vom rechten Rand)
    constexpr int EXIT_BTN_CY = FOOTER_BTN_CY;

    // Digitaluhr
    constexpr int CLOCK_W = 104;                                                 ///< Uhr-Sprite-Breite
    constexpr int CLOCK_H = 45;                                                  ///< Uhr-Sprite-Höhe
    constexpr int CLOCK_PUSH_X = EXIT_BTN_CX - EXIT_BTN_SIZE / 2 - 14 - CLOCK_W; ///< X-Startpos. Uhr-Sprite (Abstand zur Uhr: 14 px)
    constexpr int CLOCK_PUSH_Y = FOOTER_Y + (FOOTER_H - CLOCK_H) / 2 + 1;        ///< Y-Startpos. Uhr-Sprite

    // Tool-Steuerbereich im Footer (links von Uhr und Exit-Button)
    constexpr int TOOL_FOOTER_GAP = 9;                                       ///< Abstand zum Uhr-Sprite in Pixeln
    constexpr int TOOL_FOOTER_X = WORK_X;                                    ///< Linke Kante (Arbeitsbereich-X)
    constexpr int TOOL_FOOTER_H = 44;                                        ///< Höhe des Steuerbereichs
    constexpr int TOOL_FOOTER_Y = FOOTER_Y + (FOOTER_H - TOOL_FOOTER_H) / 2; ///< Vertikal zentriert im Footer
    constexpr int TOOL_FOOTER_W = CLOCK_PUSH_X - WORK_X - TOOL_FOOTER_GAP;   ///< Verfügbare Breite (396 px)

    // Farben
    constexpr uint16_t C_BG = Theme::ColorMainBackGround; ///< Hintergrundfarbe
    constexpr uint16_t C_TEXT = 0xDEDB;                   ///< Standardtext
    constexpr uint16_t C_TEXT_DIM = 0xAD75;               ///< Abgeschwächter Text
    constexpr uint16_t C_TEXT_BRIGHT = 0xFFFF;            ///< Heller Text für gewählten Eintrag
    constexpr uint16_t C_SEP_LINE = 0x2945;               ///< Trennlinie zwischen Listeneinträgen
    constexpr uint16_t C_SEC_HDR_TXT = 0xB5F7;            ///< Sektionskopf-Text
    constexpr uint16_t C_WORK_SUB = 0xB5F7;               ///< Arbeitsbereich-Untertitel
    constexpr uint16_t C_ACCENT = 0xFD20;                 ///< Akzentfarbe für Highlights

    // Sektionsfarben (eine je Kategorie)
    constexpr uint16_t C_SEC_MISC = 0xFA1F;        ///< Werkzeuge (hot Pink)
    constexpr uint16_t C_SEC_GAMES = 0xFEA0;       ///< Spiele (Gold)
    constexpr uint16_t C_SEC_ELECTRONICS = 0x07E0; ///< Elektronik (reines Grün)
    constexpr uint16_t C_SEC_UNITS = 0x04FF;       ///< Einheiten (Cyan-Blau)
    constexpr uint16_t C_SEC_DIGITAL = 0xF800;     ///< Digital (pures Rot)
    constexpr uint16_t C_SEC_KNITTING = 0xC1AC;    ///< Stricken (Pflaume)
    constexpr uint16_t C_SEC_MUSIC = 0x07F4;       ///< Musik (Petrol)
    constexpr uint16_t C_SEC_INACTIVE = 0x2945;    ///< Inaktiver Block (Mittelgrau)

    // Scroll-Physik
    /// Verbleibender Anteil der Coast-Geschwindigkeit nach 1 Sekunde Reibung. Niedriger = stärkere Reibung, kürzeres Ausrollen.
    constexpr float SCROLL_FRICTION_PER_SEC = 0.04f;

    /// Verbleibender Anteil der Gummiband-Distanz nach 1 Sekunde. Sehr klein, damit das Bouncing schnell und satt einrastet.
    constexpr float SCROLL_BOUNCE_REMAIN_PER_SEC = 0.0001f;

    /// Geschwindigkeitsschwelle, unter der das Coasting endgültig stoppt (in Pixel pro Sekunde, vergleichbar mit der Drag-Velocity).
    constexpr float SCROLL_STOP_VELOCITY = 30.0f;

    /// Maximale dt-Begrenzung pro Update (Sekunden). Schützt vor Sprüngen nach langen draw()-Pausen (z.B. Tool-Wechsel, Modal-Dialog).
    constexpr float SCROLL_DT_MAX = 0.050f;
} // namespace WorkshopCfg

// ============================================================================
// TYPDEFINITIONEN
// ============================================================================

/**
 * @brief Kategorien der Werkzeugsammlung.
 *
 * Reihenfolge entspricht der Anzeige im linken Menü von oben nach unten.
 */
enum class WorkshopCategory : uint8_t
{
    MISC = 0,        ///< Werkzeuge
    GAMES = 1,       ///< Spiele
    ELECTRONICS = 2, ///< Elektronik
    UNITS = 3,       ///< Einheiten
    DIGITAL = 4,     ///< Digital
    KNITTING = 5,    ///< Stricken
    MUSIC = 6,       ///< Musik
    COUNT = 7        ///< Sentinel (kein gültiger Wert)
};

/**
 * @brief Darstellungstyp einer Werkzeugseite im Arbeitsbereich.
 */
enum class WorkshopPageType : uint8_t
{
    CALCULATOR, ///< Interaktiver Rechner (Eingabe + Ergebnis)
    REFERENCE   ///< Interaktive Referenz (Tabelle, Farbcode etc.)
};

/**
 * @brief Eindeutige Bezeichner aller Werkzeugseiten.
 *
 * NONE ist für SECTION_HEADER-Einträge reserviert.
 */
enum class WorkshopPageId : uint8_t
{
    NONE = 0,

    // Werkzeuge
    CALCULATOR,      ///< Allgemeiner Taschenrechner (Dez/Hex/Bin)
    WORLD_CLOCK,     ///< Weltzeit-Uhr (8 Metropolen)
    FLIGHT_RADAR,    ///< OpenSky-Flugradar mit festem Mittelpunkt
    COLOR_CONVERTER, ///< Farb-Konverter (RGB / HEX / RGB565)
    PERIODIC_TABLE,  ///< Interaktives Periodensystem der Elemente
    SCREW_REFERENCE, ///< Schrauben-Referenz M2-M12 (Kernloch, SW, Drehmoment)
    BMI_CALCULATOR,  ///< BMI-Rechner (Body Mass Index)
    GAME_2048,       ///< 2048-Schiebepuzzle (Erholungsspiel)
    MILL_GAME,       ///< Mühle - Brettspiel für die Pause
    LIGHTS_OUT,      ///< Lights Out - Knobelspiel für die Pause
    DICE,            ///< Würfel - Zufallszahlen für externe Spiele
    SHUT_THE_BOX,    ///< Shut the Box - Würfelspiel für die Pause
    LOTTO_NORMAL,    ///< Lotto 6 aus 49 als normaler Tipp
    LOTTO_SYSTEM,    ///< Lotto 6 aus 49 als Vollsystem 007 bis 011

    // Elektronik
    RESISTOR_COLOR_CODE,     ///< Widerstandsfarbcode (4-/5-Band)
    SMD_RESISTOR_CODE,       ///< SMD-Widerstandscode (3/4 Stellen und R-Code)
    OHM_CALCULATOR,          ///< Ohm-Rechner (U / I / R / P)
    VOLTAGE_DIVIDER,         ///< Spannungsteiler (R1 / R2 / U_out)
    LED_RESISTOR_CALCULATOR, ///< LED-Vorwiderstand (R, Normwert, Verlustleistung)
    RC_RL_FILTER_CALCULATOR, ///< RC/RL-Filter 1. Ordnung (Tiefpass/Hochpass, RC/RL)
    LC_FILTER_CALCULATOR,    ///< LC-Filter 2. Ordnung (Tiefpass/Hochpass)
    UKW_RESONATOR,           ///< UKW-Schwingkreis (80...150 MHz, Luftspule)
    LM317_CALCULATOR,        ///< LM317-Spannungsregler (Vref / R1 / R2 / Vout)
    DECIBEL_CALCULATOR,      ///< Dezibel-Rechner (Spannung und Leistung)
    NE555_CALCULATOR,        ///< NE555-Timer-Rechner (Astabil/Monostabil)
    OPAMP_CALCULATOR,        ///< Op-Amp-Rechner (Invertierend/Nicht-invertierend)

    // Einheiten
    INCH_MM_CALCULATOR,  ///< Zoll mm
    TEMP_CALCULATOR,     ///< Temperatur: C F K
    PRESSURE_CALCULATOR, ///< Druck: bar / psi / atm / Pa
    SI_PREFIXES,         ///< SI-Präfixe (Referenztabelle p ... T)

    // Digital
    ASCII_TABLE,    ///< ASCII-Referenztabelle (Zeichen 0-127 / 128-255)
    BIT_EDITOR,     ///< Interaktiver 8/16/32-Bit Binär-Editor
    LOGIC_GATE_SIM, ///< Logikgatter-Simulator (AND/OR/XOR/NAND/NOR/NOT)

    // Stricken
    STITCH_GAUGE,       ///< Maschenprobe (Probe pro 10 cm -> Maschen, Reihen)
    STITCH_DISTRIBUTOR, ///< Maschenverteilung (Zu- und Abnahmen über eine Runde verteilen)
    SLEEVE_TAPER,       ///< Ärmelschräge (Zu- und Abnahmen über mehrere Reihen verteilen)

    // Musik
    CIRCLE_OF_FIFTHS, ///< Quintenzirkel (Tonarten, Vorzeichen, Stufenharmonik)
    METRONOME,        ///< Metronom (Tempo, Taktart, optionaler Klick)

    COUNT ///< Sentinel
};

/**
 * @brief Zeilentyp in der flachen Menüliste.
 */
enum class WorkshopEntryType : uint8_t
{
    SECTION_HEADER, ///< Sektionsüberschrift
    TOOL_ITEM       ///< Werkzeug-Eintrag
};

// ============================================================================
// STRUKTUREN
// ============================================================================

/**
 * @brief Callback-Funktionen für das Workshop-Modul.
 */
struct WorkshopCallbacks
{
    void (*onExitRequested)() = nullptr;             ///< Wird beim Verlassen des Moduls aufgerufen
    void (*onPlaySound)(const char *path) = nullptr; ///< Spielt eine Sounddatei von der SD-Karte ab; nullptr = stumm
};

/**
 * @brief Metadaten einer Werkzeugkategorie.
 */
struct WorkshopCategoryDef
{
    WorkshopCategory id;     ///< Kategorie-ID
    WorkshopKey titleKey;    ///< Schlüssel für den Anzeigenamen der Kategorie
    WorkshopKey summaryKey;  ///< Schlüssel für die Kurzbeschreibung
};

/**
 * @brief Metadaten einer Werkzeugseite.
 */
struct WorkshopPageDef
{
    WorkshopPageId id;         ///< Seiten-ID
    WorkshopCategory category; ///< Zugehörige Kategorie
    WorkshopPageType type;     ///< Darstellungstyp
    WorkshopKey titleKey;      ///< Schlüssel für den Anzeigetitel
    WorkshopKey subtitleKey;   ///< Schlüssel für die Kurzbeschreibung
};

/**
 * @brief Eintrag in der Menüliste.
 *
 * Die Gesamtliste s_flatList[] enthält Sektionsüberschriften und Werkzeug-Einträge in einer einzigen Sequenz ohne Verschachtelung. Sektionsüberschriften sind nicht wählbar und tragen NONE als pageId.
 */
struct WorkshopListEntry
{
    WorkshopEntryType type;    ///< SECTION_HEADER oder TOOL_ITEM
    WorkshopCategory category; ///< Zugehörige Kategorie
    WorkshopPageId pageId;     ///< Zugehörige Seiten-ID (NONE bei SECTION_HEADER)
    WorkshopKey labelKey;      ///< Schlüssel für den Anzeigetext in der Menüliste
};

/**
 * @brief Website-Link einer Werkzeugseite.
 */
struct WorkshopWebLinkDef
{
    WorkshopPageId id; ///< Seiten-ID
    const char *url;   ///< Website-URL; leer blendet den Header-Button aus
};

// ============================================================================
// KLASSE
// ============================================================================

/**
 * @class Workshop
 * @brief Digitaler Werkzeugkasten für RetroRadio.
 *
 * Rein lokales Offline-Modul ohne HTTP-Kommunikation oder Worker-Tasks. Das Modul verwaltet eine scrollbare Menüliste links und einen Inhaltsbereich rechts. Interaktive Werkzeuge werden als WorkshopTool-Objekte über new angelegt und beim Wechsel des aktiven Tools per delete wieder freigegeben. Die Ansteuerung erfolgt ausschließlich über das WorkshopTool-Interface.
 */
class Workshop : public BaseModule
{
public:
    /**
     * @brief Konstruktor
     */
    Workshop();

    /**
     * @brief Destruktor
     */
    ~Workshop();

    Workshop(const Workshop &) = delete;
    Workshop &operator=(const Workshop &) = delete;
    Workshop(Workshop &&) = delete;
    Workshop &operator=(Workshop &&) = delete;

    /**
     * @brief Initialisiert das Workshop-Modul.
     * @param tft      Referenz auf das Display-Objekt
     * @param callbacks Callback-Struktur (onExitRequested, onPlaySound)
     * @return true bei erfolgreicher Sprite-Erstellung und Button-Initialisierung
     *
     * Erstellt alle PSRAM-Sprites, initialisiert Footer-Buttons und setzt den Zustand auf den ersten Eintrag. Das initiale Werkzeug wird sofort über createTool() instantiiert.
     */
    bool init(LGFX &tft, const WorkshopCallbacks &callbacks);

    /**
     * @brief Hauptzeichenfunktion, zyklisch vom MainScreen aufgerufen.
     *
     * Verarbeitet Touch-Events, aktualisiert Scroll-Physik und zeichnet ausschließlich die Sprites neu, deren Dirty-Flag gesetzt ist. Touch-Events im Inhaltsbereich werden an m_activeTool weitergeleitet.
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück.
     * @return AppModule::WORKSHOP
     */
    AppModule type() const override;

    /**
     * @brief Pausiert oder reaktiviert das automatische Neuzeichnen des Werkzeug-Inhaltsbereichs.
     * @param paused true = Inhalt einfrieren, false = wieder zeichnen
     *
     * Wird von Werkzeugen genutzt, die einen modalen Dialog (z. B. eine MessageBox) direkt aufs Display rendern. Solange pausiert ist, unterdrückt der Render-Loop sowohl den periodischen Cursor-Blink-Tick als auch dirty-getriebene Sprite-Pushes. Beim Reaktivieren wird ein vollständiger Inhalts-Redraw angefordert.
     */
    void setWorkContentPaused(bool paused);

    /**
     * @brief Fordert einen vollständigen Redraw des Arbeitsbereichs an.
     *
     * Wird von Werkzeugen genutzt, wenn ein animierter Teilbereich nicht mehr ausreicht, zum Beispiel nach neuen Daten für statische Seitenbereiche.
     */
    void requestWorkRedraw();

    /**
     * @brief Fordert einen Redraw der Fußzeile an.
     *
     * Wird von Werkzeugen genutzt, wenn sich eine Statusanzeige im Footer geändert hat.
     */
    void requestFooterRedraw();

    /**
     * @brief Spielt eine Sounddatei über den hinterlegten Sound-Callback ab.
     * @param path Pfad zur Sounddatei auf der SD-Karte (z.B. "/system/sounds/metronome/click.mp3")
     *
     * Leitet den Aufruf an WorkshopCallbacks::onPlaySound weiter. Ist der Callback nicht gesetzt oder der Pfad nullptr, ist der Aufruf still und ohne Wirkung. Wird typischerweise von Werkzeugen über den owner()-Zeiger aufgerufen.
     */
    void playSound(const char *path);

private:
    static constexpr int SUBTITLE_TEXT_CAP = 96; ///< Größe des internen Untertiteltext-Puffers

    // ========================================================================
    // INNERE STRUKTUREN
    // ========================================================================

    /**
     * @brief Scroll-Zustand der Menüliste.
     *
     * Verwaltet Scroll-Position, Momentum-Scrolling und Gummiband-Effekt am Listenrand.
     */
    struct ScrollState
    {
        int32_t scrollOffset = 0;    ///< Aktuelle Scroll-Position in Pixeln (gerundet)
        float scrollOffsetF = 0.0f;  ///< Sub-Pixel-genauer Akkumulator (Quelle der Wahrheit)
        int32_t maxScroll = 0;       ///< Maximale Scroll-Position
        float velocity = 0.0f;       ///< Momentum-Geschwindigkeit (Pixel pro Sekunde)
        bool isDragging = false;     ///< Aktiver Touch-Drag läuft
        int32_t dragStartY = 0;      ///< Touch-Y bei Drag-Beginn
        int32_t dragStartOffset = 0; ///< Scroll-Offset bei Drag-Beginn
        uint32_t lastTickMs = 0;     ///< Zeitstempel des letzten Physik-Updates (millis())

        /** @brief Setzt den Scroll-Zustand vollständig zurück. */
        void reset()
        {
            scrollOffset = 0;
            scrollOffsetF = 0.0f;
            maxScroll = 0;
            velocity = 0.0f;
            isDragging = false;
            dragStartY = 0;
            dragStartOffset = 0;
            lastTickMs = 0;
        }

        /**
         * @brief Setzt den Offset hart auf einen ganzzahligen Wert und synchronisiert den Float-Akkumulator mit.
         */
        void setOffset(int32_t value)
        {
            scrollOffset = value;
            scrollOffsetF = static_cast<float>(value);
        }

        /**
         * @brief Addiert eine fraktionale Pixel-Distanz auf den Akkumulator und projiziert sie auf den ganzzahligen scrollOffset. So gehen Sub-Pixel-Anteile nicht verloren.
         */
        void addF(float delta)
        {
            scrollOffsetF += delta;
            scrollOffset = static_cast<int32_t>(scrollOffsetF);
        }
    };

    /**
     * @brief Tap-Erkennungs-Zustand.
     *
     * Ein Touch gilt als Tap, wenn die Finger-Bewegung den THRESHOLD nicht überschreitet. Alles darüber wird als Drag behandelt.
     */
    struct TapState
    {
        int32_t startX = 0; ///< X-Position bei Touch-Start
        int32_t startY = 0; ///< Y-Position bei Touch-Start
        bool isTap = false; ///< true: Finger hat sich kaum bewegt

        static constexpr int THRESHOLD = 12; ///< Maximale Finger-Bewegung für Tap

        /** @brief Setzt den Tap-Zustand zurück. */
        void reset()
        {
            startX = 0;
            startY = 0;
            isTap = false;
        }

        /**
         * @brief Prüft ob die Finger-Bewegung den Tap-Schwellwert überschreitet.
         * @param x Aktuelle X-Position
         * @param y Aktuelle Y-Position
         * @return true wenn Bewegung zu groß für einen Tap
         */
        bool exceedsThreshold(int32_t x, int32_t y) const
        {
            return (abs(x - startX) > THRESHOLD || abs(y - startY) > THRESHOLD);
        }
    };

    // ========================================================================
    // INITIALISIERUNG
    // ========================================================================

    bool createSprites(); ///< Erstellt alle PSRAM-Sprites
    void deleteSprites(); ///< Gibt alle Sprites frei
    void initButtons();   ///< Initialisiert Footer-Buttons

    // ========================================================================
    // DIRTY-FLAG-VERWALTUNG
    // ========================================================================

    void invalidateMenu();        ///< Menüliste zum Neuzeichnen markieren
    void invalidateWork();        ///< Gesamten Arbeitsbereich (Frame + Inhalt) neu zeichnen
    void invalidateWorkContent(); ///< Nur Inhaltsbereich des Arbeitsbereichs neu zeichnen
    void invalidateAll();         ///< Alle Bereiche zum Neuzeichnen markieren

    // ========================================================================
    // ZEICHENFUNKTIONEN
    // ========================================================================

    /**
     * @brief Zeichnet das einmalige statische Grundlayout.
     *
     * Löscht den Bildschirm vollständig. Wird nur beim ersten draw()-Aufruf ausgeführt; danach übernehmen die Sprite-Push-Operationen die Darstellung.
     */
    void drawStaticLayout();

    /**
     * @brief Zeichnet die gesamte Menüliste in m_menuSprite.
     *
     * Jeder TOOL_ITEM-Eintrag erhält einen ITEM_ACCENT_W breiten farbigen Balken am linken Rand in der Kategorie-Farbe; dieser scrollt als Teil des Sprites automatisch mit. SECTION_HEADER-Zeilen erhalten die Kategoriefarbe als dunklen Hintergrund-Einfärbung plus vollen Akzent-Balken. Der gewählte Eintrag (m_selectedFlatIdx) wird mit einer dunkleren Einfärbung der Kategoriefarbe hervorgehoben.
     */
    void drawMenuList();

    /**
     * @brief Zeichnet die Scrollbar in m_scrollbarSprite und pusht ihn.
     *
     * Thumb-Höhe und -Position werden proportional zur Listenhöhe berechnet (Basis: SCREEN_H = 480 px). Die Darstellung erfolgt vollständig im Scrollbar-Sprite; anschließend wird dieser per pushSprite() ausgegeben.
     */
    void drawScrollbar();

    /**
     * @brief Zeichnet den Arbeitsbereich in m_workSprite und pusht ihn.
     *
     * Aufbau: dunkelblaugrauer Hintergrund, Titelbox mit Haupttitel und Untertitel, anschließend der Inhaltsbereich aus drawWorkspaceContent().
     */
    void drawWorkspace();

    /**
     * @brief Zeichnet den optionalen Website-Button in der Titelbox.
     * @param titleBoxW Breite der aktuellen Titelbox
     */
    void drawHeaderQrButton(int titleBoxW);

    /**
     * @brief Zeichnet das nicht lesbare QR-Symbol im Website-Button.
     * @param x Sprite-relative X-Position des Buttons
     * @param y Sprite-relative Y-Position des Buttons
     * @param pressed true wenn der Button gerade gedrückt ist
     */
    void drawFakeQrButtonIcon(int x, int y, bool pressed);

    /**
     * @brief Zeichnet nur den Inhaltsbereich im bereits vorhandenen Work-Sprite neu.
     *
     * Der Work-Sprite bleibt erhalten; Titelbox und Rahmen werden nicht neu aufgebaut. Geeignet für schnelle Redraws interaktiver Werkzeuge.
     */
    void drawWorkspaceContentOnly();

    /**
     * @brief Dispatcher für den Seiteninhalt des Arbeitsbereichs.
     * @param pageId Anzuzeigende Seite
     *
     * Leitet an m_activeTool->draw() weiter, sofern ein Werkzeug aktiv ist. Wenn createTool() fehlgeschlagen ist (z.B. OOM), wird stattdessen eine kurze Fehlermeldung im Inhaltsbereich ausgegeben.
     */
    void drawWorkspaceContent(WorkshopPageId pageId);

    /**
     * @brief Zeichnet die Fußzeile und alle Footer-Elemente.
     *
     * Löscht den linken Footer-Bereich bis zur Uhr, zeichnet den Exit-Button direkt auf das TFT und delegiert den werkzeugspezifischen Footer-Inhalt an das aktive Werkzeug.
     */
    void drawFooter();

    /**
     * @brief Aktualisiert die Digitaluhr über den Clock-Sprite.
     * @param x X-Startposition des Uhrbereichs auf dem Display
     * @param y Y-Startposition des Uhrbereichs auf dem Display
     *
     * Die eigentliche Darstellung erfolgt über Clocks::drawDigitalClockPlain().
     */
    void drawDigitalClock(int32_t x, int32_t y);

    // ========================================================================
    // TOUCH-HANDLING
    // ========================================================================

    /**
     * @brief Verarbeitet Touch-Events in der Menüliste.
     * @param tx Absoluter Touch-X
     * @param ty Absoluter Touch-Y
     * @param touched true wenn Display berührt
     *
     * Unterscheidet Drag (Scroll) von Tap (Auswahl). Bei einem Tap auf ein TOOL_ITEM wird selectItem() aufgerufen.
     */
    void handleMenuTouch(int32_t tx, int32_t ty, bool touched);

    /**
     * @brief Aktualisiert den Scroll-Zustand während eines Touch-Drags.
     * @param tx Touch-X (nicht ausgewertet)
     * @param ty Aktueller Touch-Y
     * @param touched true wenn Display berührt
     */
    void handleScrollTouch(int32_t tx, int32_t ty, bool touched);

    /**
     * @brief Verarbeitet Touch-Events der Footer-Buttons.
     * @param tx Absoluter Touch-X
     * @param ty Absoluter Touch-Y
     * @param touched true wenn Display berührt
     */
    void handleButtonTouch(int32_t tx, int32_t ty, bool touched);

    /**
     * @brief Verarbeitet Touch-Events des optionalen Website-Buttons im Header.
     * @param tx Absoluter Touch-X
     * @param ty Absoluter Touch-Y
     * @param touched true wenn Display berührt
     * @return true wenn das Event vom Website-Button verarbeitet wurde
     */
    bool handleHeaderQrButtonTouch(int32_t tx, int32_t ty, bool touched);

    /**
     * @brief Aktualisiert das geöffnete QR-Modal und schließt es bei Tap außerhalb.
     * @param tx Absoluter Touch-X
     * @param ty Absoluter Touch-Y
     * @param touched true wenn Display berührt
     */
    void handleQrModalTouch(int32_t tx, int32_t ty, bool touched);

    /** @brief Führt einen Schritt der Momentum-Scroll-Physik aus (jeden Frame). */
    void updateScrollPhysics();

    /** @brief Begrenzt scrollOffset auf [0, maxScroll] und setzt Velocity auf 0. */
    void constrainScroll();

    // ========================================================================
    // NAVIGATION
    // ========================================================================

    /**
     * @brief Wählt einen TOOL_ITEM-Eintrag aus und wechselt das aktive Werkzeug.
     * @param flatIdx Index in s_flatList[] (wird ignoriert wenn SECTION_HEADER)
     *
     * Beendet das bisherige Werkzeug (onExit, delete), instantiiert das neue via createTool() und ruft onEnter() auf. Markiert Menü und Arbeitsbereich als dirty.
     */
    void selectItem(int flatIdx);

    /**
     * @brief Stellt sicher, dass der gewählte Eintrag vollständig sichtbar ist.
     *
     * Scrollt minimal, damit der markierte Eintrag weder oben noch unten abgeschnitten ist.
     */
    void ensureSelectedVisible();

    // ========================================================================
    // WERKZEUG-VERWALTUNG
    // ========================================================================

    /**
     * @brief Instantiiert das zu einer Seiten-ID passende Werkzeug.
     * @param id Seiten-ID des gewünschten Werkzeugs
     * @return Zeiger auf das neue WorkshopTool-Objekt, oder nullptr wenn keine Implementierung vorliegt
     *
     * Allokation per new, Freigabe per delete in selectItem().
     */
    WorkshopTool *createTool(WorkshopPageId id);

    // ========================================================================
    // HILFSFUNKTIONEN
    // ========================================================================

    /**
     * @brief Berechnet die Y-Position eines Eintrags vor Scroll-Korrektur.
     * @param flatIdx Index in s_flatList[]
     * @return Y-Pixelposition relativ zum Listenanfang (0 = ganz oben)
     */
    int getEntryY(int flatIdx) const;

    /**
     * @brief Ermittelt den Listenindex an einer absoluten Y-Position (vor Scroll).
     * @param absY Absolute Y-Position im Listeninhalt (= touch_y + scrollOffset)
     * @return Index in s_flatList[], oder -1 wenn außerhalb
     */
    int getEntryAtY(int absY) const;

    /** @brief Berechnet die Gesamthöhe des Listeninhalts in Pixeln. */
    int getTotalListHeight() const;

    /**
     * @brief Gibt die Streifenfarbe einer Kategorie zurück.
     * @param cat Ziel-Kategorie
     * @return RGB565-Farbe für den aktiven Sektionsblock
     */
    uint16_t getSectionColor(WorkshopCategory cat) const;

    /**
     * @brief Sucht die PageDef-Struktur zu einer Seiten-ID.
     * @param id Gesuchte Seiten-ID
     * @return Zeiger auf WorkshopPageDef, oder nullptr
     */
    const WorkshopPageDef *getPageDef(WorkshopPageId id) const;

    /**
     * @brief Sucht die Website-URL zu einer Seiten-ID.
     * @param id Gesuchte Seiten-ID
     * @return URL-Zeiger oder leerer String wenn kein Link hinterlegt ist
     */
    const char *getPageWebUrl(WorkshopPageId id) const;

    /** @brief Gibt die Website-URL der aktuell aktiven Seite zurück. */
    const char *getActivePageWebUrl() const;

    /** @brief Prüft ob die aktive Seite einen Website-Link besitzt. */
    bool hasActivePageWebUrl() const;

    /** @brief Berechnet die absolute X-Position des Website-Buttons. */
    int getHeaderQrButtonX() const;

    /** @brief Zeigt das QR-Modal für die aktive Werkzeugseite an. */
    void showQrModal();

    /** @brief Schließt das QR-Modal und stellt den Hintergrund wieder her. */
    void closeQrModal();

    /** @brief Sichert den Hintergrundbereich des QR-Modals. */
    void backupQrModal();

    /** @brief Zeichnet das QR-Modal mit echtem QR-Code und Hinweistext. */
    void drawQrModal();

    /**
     * @brief Zeichnet mehrzeiligen Text im QR-Modal.
     * @param x X-Position
     * @param y Y-Position
     * @param maxWidth Maximale Textbreite
     * @param text Textinhalt
     */
    void drawQrModalText(int x, int y, int maxWidth, const char *text);

    // ========================================================================
    // MEMBER-VARIABLEN
    // ========================================================================

    // Core
    LGFX *m_tft = nullptr;              ///< Display-Objekt
    WorkshopCallbacks m_callbacks = {}; ///< Callback-Funktionen
    bool m_initialized = false;         ///< Modul erfolgreich initialisiert
    bool m_layoutDrawn = false;         ///< Statisches Layout wurde gezeichnet
    int m_lastDrawnSecond = -1;         ///< Letzte gezeichnete Sekunde (Uhranzeige)

    // Zustand
    int m_selectedFlatIdx = 1; ///< Index in s_flatList[] des gewählten Eintrags
    ScrollState m_scroll = {}; ///< Scroll-Physik der Menüliste
    TapState m_tap = {};       ///< Tap-Erkennung

    // Dirty-Flags
    bool m_menuDirty = true;                         ///< Menüliste muss neu gezeichnet werden
    bool m_workFrameDirty = true;                    ///< Arbeitsbereich-Rahmen / Titelbox muss neu gezeichnet werden
    bool m_workContentDirty = true;                  ///< Nur der Werkzeug-Inhalt muss neu gezeichnet werden
    bool m_workContentPaused = false;                ///< Inhalts-Redraw eingefroren (z. B. durch Tool-Dialog)
    bool m_footerDirty = true;                       ///< Fußzeile muss neu gezeichnet werden
    int m_lastScrollOffset = -1;                     ///< Zuletzt gezeichneter Scroll-Offset
    char m_lastSubtitleText[SUBTITLE_TEXT_CAP] = {}; ///< Zuletzt erkannter Tool-Untertiteltext

    // Touch-Tracking
    bool m_touchWasActive = false;        ///< Touch-Zustand im letzten Frame
    bool m_touchStartedInMenu = false;    ///< Touch-Start lag im Menübereich
    int32_t m_lastTouchY = 0;             ///< Letzte Touch-Y (Velocity-Berechnung)
    unsigned long m_lastTouchTime = 0;    ///< Zeitstempel letzter Touch
    bool m_toolTouchActive = false;       ///< Touch-Sequenz läuft im Werkzeug-Bereich
    bool m_footerToolTouchActive = false; ///< Touch-Sequenz läuft im Footer-Steuerbereich
    bool m_headerQrTouchActive = false;   ///< Touch-Sequenz läuft auf dem Website-Button
    bool m_headerQrPressed = false;       ///< Website-Button wird gerade gedrückt dargestellt
    bool m_qrModalVisible = false;        ///< QR-Modal ist sichtbar
    bool m_qrModalTouchActive = false;    ///< Touch-Sequenz läuft während des QR-Modals
    bool m_qrModalTouchOutside = false;   ///< Touch-Sequenz begann außerhalb des QR-Modals
    bool m_qrModalBackupValid = false;    ///< Hintergrundsicherung des QR-Modals ist gültig
    lgfx::rgb565_t *m_qrModalBackup = nullptr; ///< Hintergrundpuffer des QR-Modals
    size_t m_qrModalBackupBytes = 0;      ///< Größe des Hintergrundpuffers in Bytes

    // Sprites
    Gfx_Sprite *m_menuSprite = nullptr;      ///< Menüliste (MENU_W x MENU_SPRITE_H)
    Gfx_Sprite *m_scrollbarSprite = nullptr; ///< Scrollbar (SCROLLBAR_W x SCREEN_H)
    Gfx_Sprite *m_workSprite = nullptr;      ///< Arbeitsbereich (WORK_W x WORK_H)
    Gfx_Sprite *m_clockSprite = nullptr;     ///< Digitaluhr (CLOCK_W x CLOCK_H)

    // Footer-Buttons
    Button m_exitButton; ///< Modul verlassen

    // Aktives Werkzeug
    WorkshopTool *m_activeTool = nullptr; ///< Aktives Werkzeug, oder nullptr wenn createTool() fehlgeschlagen ist

    // Statische Datenbasis (Definitionen in Workshop.cpp)
    static const WorkshopCategoryDef s_categories[]; ///< Kategorie-Metadaten
    static const WorkshopPageDef s_pages[];          ///< Seiten-Metadaten
    static const WorkshopListEntry s_flatList[];     ///< Flache Menüliste
    static const WorkshopWebLinkDef s_webLinks[];    ///< Website-Links pro Werkzeugseite
    static const int FLAT_LIST_COUNT;                ///< Anzahl Einträge in s_flatList[]
    static const int WEB_LINK_COUNT;                 ///< Anzahl Einträge in s_webLinks[]
};

#endif // WORKSHOP_H
