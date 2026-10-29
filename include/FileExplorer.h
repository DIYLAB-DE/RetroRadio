#ifndef FILE_EXPLORER_H
#define FILE_EXPLORER_H

#include <functional>
#include <initializer_list>
#include <vector>

#include "Button.h"
#include "Gfx.h"
#include "Logger.h"
#include "StringUtils.h"

// ============================================================================
// Konfigurationsstrukturen
// ============================================================================

/**
 * @brief Maximale Anzahl erlaubter Dateiendungen.
 */
constexpr size_t MAX_ALLOWED_EXTENSIONS = 16;

/**
 * @brief Konfigurationsstruktur für den FileExplorer.
 */
struct FileExplorerConfig
{
    const char *allowedExtensions[MAX_ALLOWED_EXTENSIONS] = {".mp3", ".aac", ".flac", ".m4a", ".ogg"}; ///< Erlaubte Dateiendungen
    size_t extensionCount = 5;                                                                         ///< Anzahl der Einträge in allowedExtensions (0 = alle erlaubt)
    bool showHiddenFiles = false;                                                                      ///< Versteckte Dateien anzeigen (beginnen mit '.')
    const char *title = "Musikdatei(en) auswählen:";                                                   ///< Dialogtitel

    /**
     * @brief Setzt die erlaubten Dateiendungen.
     * @param extensions Array von Dateiendungen (z.B. {".mp3", ".aac", ".flac", ".m4a", ".ogg"})
     * @param count Anzahl der Einträge im Array
     *
     * @note Maximal MAX_ALLOWED_EXTENSIONS Einträge werden übernommen.
     */
    void setExtensions(const char *const *extensions, size_t count)
    {
        extensionCount = (count > MAX_ALLOWED_EXTENSIONS) ? MAX_ALLOWED_EXTENSIONS : count;
        for (size_t i = 0; i < extensionCount; ++i)
            allowedExtensions[i] = extensions[i];
    }

    /**
     * @brief Setzt die erlaubten Dateiendungen per Initialisierungsliste.
     * @param extensions Initialisierungsliste (z.B. {".mp3", ".aac", ".flac", ".m4a", ".ogg"})
     *
     * @example
     * @code
     * config.setExtensions({".mp3", ".aac", ".flac", ".m4a", ".ogg"});
     * @endcode
     */
    void setExtensions(std::initializer_list<const char *> extensions)
    {
        extensionCount = (extensions.size() > MAX_ALLOWED_EXTENSIONS) ? MAX_ALLOWED_EXTENSIONS : extensions.size();
        size_t i = 0;
        for (const char *ext : extensions)
        {
            if (i >= extensionCount)
                break;
            allowedExtensions[i++] = ext;
        }
    }

    /**
     * @brief Erlaubt alle Dateiendungen (kein Filter).
     */
    void allowAllExtensions()
    {
        extensionCount = 0;
    }
};

/**
 * @brief Struktur für die Rückgabe von ausgewählten Dateien.
 *
 * Enthält den vollständigen Pfad zur ausgewählten Datei. Die maximale Pfadlänge beträgt 255 Zeichen + Null-Terminator.
 */
struct FileResult
{
    char path[256]; ///< Vollständiger Pfad zur Datei
};

// ============================================================================
// FileExplorer Klasse
// ============================================================================

/**
 * @class FileExplorer
 * @brief Ein nicht-blockierender, modaler Dateimanager für SD-Karten.
 *
 * Der FileExplorer ermöglicht die Navigation durch das Dateisystem einer SD-Karte und die Auswahl von Dateien oder Verzeichnissen. Er unterstützt:
 *
 * - Touch-basierte Navigation mit Momentum-Scrolling
 * - Zeitbasierte und damit bildratenunabhängige Scroll-Physik
 * - Filterung nach Dateiendungen
 * - Auswahl einzelner Dateien oder ganzer Verzeichnisse
 * - Gummiband-Effekt beim Überscollen
 *
 * @note Der Explorer ist nicht-blockierend und muss regelmäßig über update() aktualisiert werden.
 *
 * @example
 * @code
 * FileExplorer explorer(tft);
 * FileExplorerConfig config;
 * config.allowedExtensions = {".mp3", ".aac", ".flac", ".m4a", ".ogg"};
 *
 * explorer.start(config, [](const std::vector<FileResult>& results) {
 *     for (const auto& file : results) {
 *         Serial.println(file.path);
 *     }
 * });
 *
 * // In der Hauptschleife:
 * while (explorer.isActive()) {
 *     explorer.update();
 * }
 * @endcode
 */
class FileExplorer
{
public:
    /**
     * @brief Konstruktor.
     * @param gfx Referenz auf das LGFX Display-Objekt
     */
    explicit FileExplorer(LGFX *gfx);

    /**
     * @brief Destruktor - gibt alle Ressourcen frei.
     */
    ~FileExplorer();

    // Kopieren und Verschieben verbieten
    FileExplorer(const FileExplorer &) = delete;
    FileExplorer &operator=(const FileExplorer &) = delete;
    FileExplorer(FileExplorer &&) = delete;
    FileExplorer &operator=(FileExplorer &&) = delete;

    /**
     * @brief Setzt die Größe und Position des Dialogs.
     * @param x X-Position (linke Kante)
     * @param y Y-Position (obere Kante)
     * @param w Breite in Pixel
     * @param h Höhe in Pixel
     *
     * @note Muss vor start() aufgerufen werden. Werte werden auf Mindestgröße (MIN_DIALOG_WIDTH x MIN_DIALOG_HEIGHT) begrenzt.
     */
    void setBounds(int x, int y, int w, int h);

    /**
     * @brief Startet den Explorer mit der gegebenen Konfiguration.
     * @param config Konfiguration für Dateifilter und Anzeige
     * @param callback Callback-Funktion, die bei Beendigung aufgerufen wird
     *
     * Der Callback erhält einen Vektor mit allen ausgewählten Dateipfaden. Bei Abbruch ist der Vektor leer.
     *
     * @note Falls setBounds() nicht aufgerufen wurde, wird eine Standardgröße zentriert verwendet.
     */
    void start(const FileExplorerConfig &config, std::function<void(const std::vector<FileResult> &)> callback);

    /**
     * @brief Aktualisiert den Zustand des Explorers.
     *
     * Muss in der Hauptschleife aufgerufen werden, solange isActive() true ist. Verarbeitet Touch-Eingaben, Scroll-Physik und Zeichenoperationen.
     */
    void update();

    /**
     * @brief Prüft, ob der Dialog aktiv ist.
     * @return true wenn der Dialog angezeigt wird
     */
    bool isActive() const;

private:
    // ========================================================================
    // Interne Typen und Strukturen
    // ========================================================================

    /**
     * @brief Zustandsautomat für den Dialog.
     */
    enum class State
    {
        Hidden, ///< Dialog ist nicht sichtbar
        Active  ///< Dialog ist aktiv und verarbeitet Eingaben
    };

    /**
     * @brief Repräsentiert einen Eintrag in der Dateiliste.
     */
    struct FileEntry
    {
        char name[128];     ///< Datei-/Verzeichnisname
        char extension[10]; ///< Dateiendung (mit Punkt)
        uint32_t size;      ///< Dateigröße in Bytes
        bool isDirectory;   ///< true wenn Verzeichnis
        bool isSelected;    ///< true wenn ausgewählt
    };

    /**
     * @brief Zustand für Scrolling und Touch-Handling.
     *
     * Enthält alle Variablen für die zeitbasierte Scroll-Physik und Touch-Erkennung (Tap, Long-Press, Drag).
     *
     * @note scrollPos ist die interne Float-Position für präzise Physik-Berechnung. scrollOffsetPx ist die abgerundete Integer-Position fürs Zeichnen. Diese Trennung verhindert, dass kleine Bewegungen < 1px verloren gehen.
     */
    struct ScrollState
    {
        // Scroll-Position und -Grenzen
        float scrollPos = 0.0f; ///< Interne Scroll-Position (Float für präzise Physik)
        int scrollOffsetPx = 0; ///< Abgeleitete Pixel-Position fürs Zeichnen
        int contentHeight = 0;  ///< Gesamthöhe des Inhalts
        int viewportHeight = 0; ///< Sichtbare Höhe
        int maxScroll = 0;      ///< Maximaler Scroll-Offset

        // Physik-Parameter
        float scrollVelocity = 0.0f; ///< Aktuelle Geschwindigkeit (Pixel/Sekunde)

        // Drag-Zustand
        bool isDragging = false;  ///< true während eines Drag-Vorgangs
        int dragStartY = 0;       ///< Y-Position bei Drag-Beginn
        float dragStartPos = 0.0f; ///< Scroll-Position bei Drag-Beginn (Float)

        // Zeitstempel für Delta-Time und Velocity-Berechnung
        unsigned long lastUpdateTime = 0; ///< Zeitstempel des letzten Updates (ms)
        unsigned long lastTouchTime = 0;  ///< Zeitstempel der letzten Touch-Position
        int lastTouchY = 0;               ///< Y-Position der letzten Touch-Position

        // Tap/Long-Press Erkennung
        unsigned long touchStartTime = 0; ///< Zeitstempel des Touch-Beginns
        bool longPressHandled = false;    ///< true wenn Long-Press bereits verarbeitet
        int tapCandidateIndex = -1;       ///< Index des potentiellen Tap-Ziels (-1 = keins)
        int pressStartX = 0;              ///< X-Position bei Touch-Beginn
        int pressStartY = 0;              ///< Y-Position bei Touch-Beginn
    };

    /**
     * @brief Spalten-Positionen für die Listenansicht.
     */
    struct Columns
    {
        int listLeft;    ///< Linke Kante der Listenfläche (Sprite)
        int listRight;   ///< Rechte Kante der Listenfläche (Sprite)
        int nameX;       ///< X-Position für Dateinamen (linksbündig)
        int sizeX;       ///< X-Position für Größe (rechtsbündig)
        int typeCenterX; ///< X-Position für Typ (zentriert)
    };

    /**
     * @brief Statistiken für das aktuelle Verzeichnis.
     */
    struct DirectoryStats
    {
        int fileCount = 0;         ///< Anzahl Dateien (ohne ..)
        int dirCount = 0;          ///< Anzahl Unterverzeichnisse
        int selectedFiles = 0;     ///< Anzahl ausgewählter Dateien
        int selectedDirs = 0;      ///< Anzahl ausgewählter Verzeichnisse
        uint64_t selectedSize = 0; ///< Gesamtgröße der Auswahl in Bytes
    };

    // ========================================================================
    // Konstanten
    // ========================================================================

    /// Reservierter Platz für die Größen-Spalte (rechts neben Typ)
    static constexpr int SIZE_COL_RESERVE = 100;
    /// Breite der Typ-Spalte für Zentrierung
    static constexpr int TYPE_COL_WIDTH = 80;
    /// Standard-Dialogbreite wenn setBounds() nicht aufgerufen wurde
    static constexpr int DEFAULT_DIALOG_W = 720;
    /// Standard-Dialoghöhe wenn setBounds() nicht aufgerufen wurde
    static constexpr int DEFAULT_DIALOG_H = 400;

    /**
     * @brief UI-Layout Konstanten.
     */
    struct UIConfig
    {
        // Dialog-Layout
        static constexpr int HEADER_HEIGHT = 60;        ///< Höhe des Header-Bereichs
        static constexpr int COLUMN_HEADER_HEIGHT = 30; ///< Höhe der Spaltenüberschriften
        static constexpr int FOOTER_HEIGHT = 45;        ///< Höhe des Footer-Bereichs
        static constexpr int ITEM_HEIGHT = 31;          ///< Höhe eines Listeneintrags
        static constexpr int SCROLLBAR_WIDTH = 5;       ///< Breite der Scrollbar
        static constexpr int PADDING = 15;              ///< Innenabstand
        static constexpr int BORDER_RADIUS = 4;         ///< Eckenradius

        // Buttons
        static constexpr int BTN_WIDTH = 90;  ///< Button-Breite
        static constexpr int BTN_HEIGHT = 45; ///< Button-Höhe, gleich der Höhe des Fußbereichs
        static constexpr int BTN_RADIUS = 4;  ///< Button-Eckenradius

        // Mindestgröße für den Dialog
        static constexpr int MIN_DIALOG_WIDTH = 480;  ///< Minimale Dialogbreite
        static constexpr int MIN_DIALOG_HEIGHT = 320; ///< Minimale Dialoghöhe
    };

    // ========================================================================
    // Member-Variablen
    // ========================================================================

    // Kernkomponenten
    LGFX *m_tft;                                                     ///< Pointer auf das Displayobjekt
    State m_state = State::Hidden;                                   ///< Aktueller Dialog-Zustand
    FileExplorerConfig m_config;                                     ///< Aktive Konfiguration
    std::function<void(const std::vector<FileResult> &)> m_callback; ///< Beendigungs-Callback

    // UI-Elemente und Sprites
    Gfx_Sprite *m_viewSprite = nullptr;      ///< Sprite für die Dateiliste
    Gfx_Sprite *m_scrollbarSprite = nullptr; ///< Sprite für die Scrollbar
    Gfx_Sprite *m_statusSprite = nullptr;    ///< Sprite für die Statuszeile
    uint16_t *m_folderIconSelected = nullptr;   ///< Ordnersymbol auf dem Grund des Auswahlbalkens, im PSRAM
    uint16_t *m_folderUpIconSelected = nullptr; ///< Symbol des übergeordneten Ordners auf dem Grund des Auswahlbalkens, im PSRAM
    Button m_okButton;                       ///< OK-Button
    Button m_cancelButton;                   ///< Abbrechen-Button
    Button *m_activeButton = nullptr;        ///< Aktuell gedrückter Button (oder nullptr)
    bool m_buttonPressValid = false;         ///< true wenn Finger noch auf aktivem Button ist

    // Dialog-Dimensionen
    int m_dialogX = 0; ///< X-Position des Dialogs
    int m_dialogY = 0; ///< Y-Position des Dialogs
    int m_dialogW = 0; ///< Breite des Dialogs
    int m_dialogH = 0; ///< Höhe des Dialogs

    // Display-Backup
    lgfx::rgb565_t *m_screenBuffer = nullptr; ///< Backup des Hintergrunds
    size_t m_screenBufferSize = 0;            ///< Aktuelle Buffer-Größe in Bytes
    bool m_backupValid = false;               ///< true wenn Backup gültig ist

    // Dateiliste und Navigation
    std::vector<FileEntry> m_fileList; ///< Einträge im aktuellen Verzeichnis
    char m_currentPath[256];           ///< Aktueller Verzeichnispfad
    int m_selectedDirectoryIndex = -1; ///< Index des ausgewählten Verzeichnisses (-1 = keins)
    DirectoryStats m_dirStats;         ///< Statistiken für aktuelles Verzeichnis

    // Zustandsvariablen
    ScrollState m_scrollState;         ///< Scroll- und Touch-Zustand
    bool m_needsRedraw = true;         ///< true wenn Dateiliste neu gezeichnet werden muss
    bool m_needsStatusRedraw = true;   ///< true wenn Status neu gezeichnet werden muss
    bool m_wasTouching = false;        ///< Touch-Zustand im vorherigen Frame
    int m_lastScrollOffsetPx = -1;     ///< Scroll-Offset (Pixel) im vorherigen Frame

    // ========================================================================
    // Private Methoden
    // ========================================================================

    /**
     * @brief Berechnet die Spalten-Positionen basierend auf der Dialog-Breite.
     * @return Columns-Struktur mit berechneten Positionen
     */
    Columns computeColumns() const;

    // Lifecycle
    void backupScreen();         ///< Sichert den Bildschirmbereich
    void restoreScreen();        ///< Stellt den gesicherten Bereich wieder her
    void finish(bool confirmed); ///< Beendet den Dialog
    void cleanupResources();     ///< Gibt alle Sprite-Ressourcen frei

    // Dateioperationen
    void collectFilesFromDirectory(const char *dirPath, std::vector<FileResult> &results) const;
    bool isSystemDirectory(const char *dirname) const;
    void updateDirectoryStats();

    // Zeichenmethoden
    void draw();                                            ///< Zeichnet den gesamten Dialog
    void drawFrame();                                       ///< Zeichnet den Rahmen
    void drawHeader();                                      ///< Zeichnet den Header mit Titel und Pfad
    void drawFileList();                                    ///< Zeichnet die Dateiliste
    void drawColumnHeader();                                ///< Zeichnet die Spaltenüberschriften
    void drawFileEntry(int index, int yPos, const Columns &col); ///< Zeichnet einen einzelnen Eintrag
    const uint16_t *folderIcon(bool up, bool selected);         ///< Liefert das Ordnersymbol passend zum Zeilengrund, auf dem Auswahlbalken eine eigene Fassung
    void drawStatusbar();                                   ///< Zeichnet die Statusleiste
    void drawScrollbar();                                   ///< Zeichnet die Scrollbar
    void drawButtons();                                     ///< Zeichnet die Buttons
    void drawStatusContent();                               ///< Zeichnet den Status-Inhalt

    // Logik
    void handleTouch(int32_t x, int32_t y, bool is_touched);
    void updateScrollPhysics(); ///< Zeitbasierte Scroll-Physik

    // Navigation
    void scanDirectory(const char *path);
    void navigateToParent();
    void navigateToChild(const char *childName);
    void selectItem(int index);
    void toggleItemSelection(int index);

    // Helfer
    void formatSize(uint32_t bytes, char *buf, size_t len) const;
    bool isExtensionAllowed(const char *filename) const;
    void truncateTextToWidth(const char *text, int maxWidth, char *output, size_t outputSize) const;
};

#endif // FILE_EXPLORER_H
