#ifndef STATION_PICKER_H
#define STATION_PICKER_H

#include <functional>

#include "Button.h"
#include "Gfx.h"

struct Station; ///< Forward Declaration (StationManager.h)

/**
 * @brief Modaler Dialog zur Sender-Direktwahl
 *
 * Zeigt alle Sender als 10x7-Gitter mit Index + Kurzname. Der aktuell spielende Sender wird farblich hervorgehoben. Auswahl per Touch -> Callback mit Station-Index.
 *
 * Kein Button-Array für 70 Zellen - stattdessen wird die Zelle per Grid-Mathematik aus der Touch-Position berechnet (O(1)).
 *
 * Farben und Layout-Werte sind über StationPickerCfg konfigurierbar.
 */

// =============================================================================
// Konfiguration (Farben + Layout, zentral änderbar)
// =============================================================================
namespace StationPickerCfg
{
    namespace Colors
    {
        // Dialog
        constexpr uint16_t BORDER = 0x24F2; ///< Dialog-Rahmen
        constexpr uint16_t BG = 0x0000;     ///< Dialog-Hintergrund

        // Zellen
        constexpr uint16_t CELL_BG = 0x1082;     ///< Zelle normal (dunkles Grau)
        constexpr uint16_t CELL_BORDER = 0x2104; ///< Zelle Rahmen (subtil)
        constexpr uint16_t CELL_NUM = 0x04FF;    ///< Sendernummer (Cyan-Akzent)
        constexpr uint16_t CELL_NAME = 0xAD75;   ///< Sendername (helles Grau)

        // Aktiver Sender
        constexpr uint16_t ACTIVE_BG = 0x0320;     ///< Dunkelgrüner Hintergrund
        constexpr uint16_t ACTIVE_BORDER = 0x07E0; ///< Grüner Rahmen
        constexpr uint16_t ACTIVE_NUM = 0x07E0;    ///< Grüne Nummer
        constexpr uint16_t ACTIVE_NAME = 0xFFFF;   ///< Weißer Name

        // Preset-Sender
        constexpr uint16_t PRESET_DOT = 0xFC60;  ///< Orange Preset-Punkt
        constexpr uint16_t PRESET_NAME = 0x6C64; ///< Olivgrün für Favoriten-Name

        // Touch-Highlight
        constexpr uint16_t TOUCH_BG = 0x2945;     ///< Aufgehellter Hintergrund
        constexpr uint16_t TOUCH_BORDER = 0x04FF; ///< Cyan-Rahmen

        // Cancel-Button
        constexpr uint16_t CANCEL_BORDER = 0x39E8; ///< Abbrechen Rahmen
        constexpr uint16_t CANCEL_TEXT = 0xD69A;   ///< Abbrechen Text
    }

    namespace Layout
    {
        // Dialog-Position
        constexpr int DLG_W = 784; ///< Dialog-Breite
        constexpr int DLG_H = 446; ///< Dialog-Höhe
        constexpr int DLG_X = (800 - DLG_W) / 2; ///< X-Mitte des Displays minus Hälfte der Dialog-Breite
        constexpr int DLG_Y = (480 - DLG_H) / 2; ///< Y-Mitte des Displays minus Hälfte der Dialog-Höhe

        // Innenabstände
        constexpr int PADDING = 12; ///< Abstand Dialog-Rand -> Inhalt
        constexpr int BORDER_RADIUS = 4; ///< Eckenradius für Dialog und Zellen

        // Titel + Trennlinie
        constexpr int TITLE_Y = 18; ///< Y-Position des Titels
        constexpr int SEP_Y = 48;   ///< Y-Position der Trennlinie
        constexpr int SEP_H = 2;    ///< Höhe der Trennlinie

        // Grid
        constexpr int GRID_TOP = 58; ///< Abstand von Dialog-Top bis Grid-Start
        constexpr int COLS = 10; ///< Anzahl Spalten im Grid
        constexpr int ROWS = 7; ///< Anzahl Zeilen im Grid
        constexpr int CELL_GAP = 2; ///< Abstand zwischen Zellen

        // Cancel-Button
        constexpr int CANCEL_H = 36; ///< Höhe des Abbrechen-Buttons
        constexpr int CANCEL_PAD = 6; ///< Abstand von Button zu Dialog-Bottom
    }
}

// Kurzalias
namespace SPColors = StationPickerCfg::Colors;
namespace SPLayout = StationPickerCfg::Layout;

class StationPicker
{
public:
    /**
     * @brief Konstruktor
     * @param gfx Pointer auf das Display-Objekt
     */
    explicit StationPicker(LGFX *gfx);

    /**
     * @brief Destruktor - gibt Backup-Buffer frei
     */
    ~StationPicker();

    // Copy/Move verhindern
    StationPicker(const StationPicker &) = delete;
    StationPicker &operator=(const StationPicker &) = delete;

    /**
     * @brief Registriert Callback für Auswahlereignisse
     * @param cb Funktion mit Parameter int (stationIndex oder -1 für Abbruch)
     */
    void setCallback(std::function<void(int)> cb);

    /**
     * @brief Setzt den Dialog-Titel
     * @param title Titeltext (Pointer muss für Dialogdauer gültig bleiben)
     *
     * Bei nullptr wird der Default-Titel verwendet. Der String wird nur als Pointer gespeichert, nicht kopiert. PROGMEM-Pointer aus `STR(...)` sind dauerhaft gültig und damit sicher.
     */
    void setTitle(const char *title);

    /**
     * @brief Setzt das Label des Abbrechen-Buttons
     * @param label Button-Text (wird beim Zeichnen in den Button kopiert)
     *
     * Bei nullptr wird das Default-Label verwendet. Der String wird in drawCancelButton() an Button::initButton() weitergegeben, der ihn intern kopiert. Damit ist die Lifetime des Pointers nur bis zum Aufruf von start() relevant.
     */
    void setCancelLabel(const char *label);

    /**
     * @brief Aktiviert automatisches Timeout
     * @param ms Timeout in Millisekunden (0 = deaktiviert)
     */
    void setTimeout(uint32_t ms);

    /**
     * @brief Startet den Dialog
     * @param stations Array aller verfügbaren Stationen
     * @param count Anzahl der Stationen im Array
     * @param activeStation Index des aktuell spielenden Senders (-1 = keiner)
     *
     * @note Die Station-Daten werden nur als Pointer referenziert, nicht kopiert. Sie müssen für die Dialogdauer gültig bleiben.
     */
    void start(const Station *stations, int count, int activeStation);

    /**
     * @brief Aktualisiert Dialog-Logik und Touch-Verarbeitung
     * @note Muss in der Hauptschleife aufgerufen werden
     */
    void update();

    /**
     * @brief Prüft ob der Dialog aktiv ist
     * @return true wenn aktiv, false sonst
     */
    bool isActive() const;

private:
    // =================================================================
    // Methoden
    // =================================================================

    void backup();                              ///< Bildschirminhalt sichern
    void restore();                             ///< Bildschirminhalt wiederherstellen
    void draw();                                ///< Kompletten Dialog zeichnen
    void drawFrame();                           ///< Rahmen + Hintergrund
    void drawHeader();                          ///< Titel
    void drawSeparator();                       ///< Trennlinie
    void drawGrid();                            ///< Alle Zellen zeichnen
    void drawCell(int index, bool highlighted); ///< Einzelne Zelle zeichnen
    void drawCancelButton();                    ///< Abbrechen-Button
    void processTouchInput();                   ///< Touch-Handling
    void checkTimeout();                        ///< Timeout prüfen
    int cellAtPosition(int tx, int ty) const;   ///< Zelle an Touch-Position (O(1))
    void finish(int result);                    ///< Dialog beenden

    // =================================================================
    // Member-Variablen
    // =================================================================

    LGFX *m_tft; ///< Zeiger auf das Display-Objekt

    // Anwender-sichtbare Texte (nur Pointer, nicht kopiert)
    const char *m_titleText = "SENDER";   ///< Header-Titel
    const char *m_cancelLabel = "Abbruch"; ///< Cancel-Button-Beschriftung

    // Stationsdaten (nur Referenz, nicht kopiert!)
    const Station *m_stations = nullptr;
    int m_stationCount = 0;
    int m_activeStation = -1; ///< Aktuell spielender Sender

    // Berechnetes Grid-Layout (in start() initialisiert)
    int m_gridX = 0; ///< Grid linke Kante (absolut)
    int m_gridY = 0; ///< Grid obere Kante (absolut)
    int m_cellW = 0; ///< Zellenbreite inkl. Gap
    int m_cellH = 0; ///< Zellenhöhe inkl. Gap
    int m_rows = 0;  ///< Tatsächliche Zeilenanzahl

    // Zustand
    bool m_active = false;
    int m_touchedCell = -1; ///< Aktuell berührte Zelle (-1 = keine, -2 = Abbrechen-Button)
    bool m_isTouching = false;
    std::function<void(int)> m_callback;
    uint32_t m_timeoutMs = 0;
    uint32_t m_startTime = 0;

    // Cancel-Button
    Button m_cancelBtn;

    // Bildschirm-Backup (PSRAM)
    lgfx::rgb565_t *m_screenBuffer = nullptr;
    bool m_backupValid = false;
};

#endif // STATION_PICKER_H
