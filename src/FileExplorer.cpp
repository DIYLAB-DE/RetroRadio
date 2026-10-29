#include "FileExplorer.h"

#include <SD.h>
#include <esp_heap_caps.h>

#include <algorithm>
#include <cmath>

// Theme
#include "Theme.h"

// i18n
#include "I18n.h"
#include "Strings_sdplayer.h"

// Fonts
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

// Icons
#include "IconShapes.h"

// ===================================================================
// Konstanten
// ===================================================================

namespace
{
    // Farben der gezeichneten Ordnersymbole, an den früheren Bitmaps abgelesen
    constexpr uint16_t ICON_FOLDER_COLOR = 0x8DFA;      ///< Strichfarbe der Ordnersymbole
    constexpr uint16_t ICON_FOLDER_BACKGROUND = 0x0000; ///< Freie Fläche der Ordnersymbole

    // Datei-Limits
    constexpr int MAX_DIRECTORY_DEPTH = 20;      ///< Maximale Verzeichnistiefe bei rekursiver Suche
    constexpr int MAX_FILES_TOTAL = 300;         ///< Maximale Gesamtanzahl der Dateien
    constexpr int MAX_ITERATIONS = 2000;         ///< Maximale Anzahl der Verzeichnis-Iterationen
    constexpr int MAX_FILES_PER_DIRECTORY = 300; ///< Maximale Anzahl der Dateien pro Verzeichnis
    constexpr int MAX_STACK_SIZE = 100;          ///< Maximale Stapelgröße für die Verzeichnistiefe
    constexpr int TYPICAL_STACK_DEPTH = 32;      ///< Typische Stapeltiefe für Reservierung

    // Touch-Handling
    constexpr unsigned long SHORT_TAP_THRESHOLD_MS = 400;  ///< Maximale Dauer für kurzen Tap
    constexpr unsigned long LONG_PRESS_THRESHOLD_MS = 500; ///< Dauer für Long-Press Erkennung
    constexpr int DRAG_THRESHOLD_PX = 10;                  ///< Mindestbewegung für Drag-Erkennung

    // Scroll-Physik
    constexpr float DRAG_SCROLL_MULTIPLIER = 1.3f; ///< Multiplikator für Scroll-Geschwindigkeit beim Draggen
    constexpr float SOFT_BRAKE_FACTOR = 0.98f;     ///< Faktor für sanftes Bremsen bei kleinen Geschwindigkeiten
    constexpr float SOFT_BRAKE_THRESHOLD = 50.0f;  ///< Schwelle für sanftes Bremsen
    constexpr float SPRING_RATE = 10.0f;           ///< Federkonstante für Gummiband-Effekt
    constexpr float FRICTION_DECAY = 2.0f;         ///< Reibungskoeffizient (Dämpfung pro Sekunde)
    constexpr float VELOCITY_EPSILON = 10.0f;      ///< Minimale Geschwindigkeit bevor auf 0 gesetzt wird (Pixel/Sekunde)
    constexpr float SNAP_THRESHOLD = 1.0f;         ///< Schwelle für Scroll-Position beim Einrasten (Pixel)
    constexpr float MAX_VELOCITY = 3000.0f;        ///< Maximale Geschwindigkeit (Pixel/Sekunde)
    constexpr float VELOCITY_BOOST = 1.5f;         ///< Faktor für Velocity-Berechnung beim Loslassen
    constexpr float VELOCITY_SMOOTH_NEW = 0.5f;    ///< Glättungsfaktor für neue Velocity
    constexpr float VELOCITY_SMOOTH_OLD = 0.5f;    ///< Glättungsfaktor für alte Velocity
    constexpr float MAX_DELTA_TIME = 0.1f;         ///< Maximales Delta-Time in Sekunden (100ms = 10 FPS Minimum)
    constexpr int RUBBER_BAND_LIMIT = 80;          ///< Gummiband-Grenze beim Überziehen (Pixel)
    constexpr float RUBBER_BAND_RESISTANCE = 0.4f; ///< Dämpfungsfaktor beim Überziehen

    // Dateigröße-Formatierung
    constexpr double BYTES_PER_KB = 1024.0; ///< Anzahl Bytes pro KB
    constexpr int MAX_SIZE_UNIT_INDEX = 3;  ///< Maximale Index für Größen-Einheiten (B, KB, MB, GB)

    // UI-Layout
    constexpr int HEADER_PATH_LINE_SPACING = 20; ///< Zeilenabstand im Header
    constexpr int ICON_MARGIN = 5;               ///< Abstand nach Icons
    constexpr int NAME_TYPE_GAP = 10;            ///< Abstand zwischen Name und Typ-Spalte

    // Scrollbar-Farben
    constexpr uint16_t SCROLLBAR_TRACK_COLOR = 0x2104; ///< Hintergrund der Scrollbar
    constexpr uint16_t SCROLLBAR_THUMB_COLOR = 0x5ACB; ///< Scrollbar-Griff (normal)
    constexpr uint16_t SCROLLBAR_THUMB_DRAG = 0x8410;  ///< Scrollbar-Griff (beim Ziehen)
}

// ===================================================================
// Konstruktor & Destruktor
// ===================================================================

/**
 * @brief Konstruktor
 */
FileExplorer::FileExplorer(LGFX *gfx) : m_tft(gfx)
{
    m_currentPath[0] = '\0';
}

/**
 * @brief Destruktor
 */
FileExplorer::~FileExplorer()
{
    cleanupResources();

    if (m_screenBuffer != nullptr)
    {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
        m_screenBufferSize = 0;
    }

    // Ordnersymbole des Auswahlbalkens freigeben
    if (m_folderIconSelected)
    {
        free(m_folderIconSelected);
        m_folderIconSelected = nullptr;
    }
    if (m_folderUpIconSelected)
    {
        free(m_folderUpIconSelected);
        m_folderUpIconSelected = nullptr;
    }
}

/**
 * @brief Gibt alle Sprite-Ressourcen sicher frei
 */
void FileExplorer::cleanupResources()
{
    if (m_viewSprite)
        m_viewSprite->unloadFont();

    if (m_statusSprite)
        m_statusSprite->unloadFont();

    m_tft->unloadFont();

    m_tft->safeDeleteSprite(m_viewSprite);
    m_tft->safeDeleteSprite(m_scrollbarSprite);
    m_tft->safeDeleteSprite(m_statusSprite);
}

// ===================================================================
// Öffentliche Methoden
// ===================================================================

/**
 * @brief Setzt die Größe und Position des Dialogs
 */
void FileExplorer::setBounds(int x, int y, int w, int h)
{
    m_dialogX = x;
    m_dialogY = y;
    m_dialogW = std::max(w, UIConfig::MIN_DIALOG_WIDTH);
    m_dialogH = std::max(h, UIConfig::MIN_DIALOG_HEIGHT);
}

/**
 * @brief Startet den Explorer
 */
void FileExplorer::start(const FileExplorerConfig &config, std::function<void(const std::vector<FileResult> &)> callback)
{
    if (m_state != State::Hidden)
        return;

    // Standard-Bounds setzen falls nicht konfiguriert
    if (m_dialogW == 0 || m_dialogH == 0)
        setBounds((m_tft->width() - DEFAULT_DIALOG_W) / 2, (m_tft->height() - DEFAULT_DIALOG_H) / 2, DEFAULT_DIALOG_W, DEFAULT_DIALOG_H);

    m_config = config;
    m_callback = std::move(callback);
    m_state = State::Active;

    // Zustand zurücksetzen
    m_needsRedraw = true;
    m_needsStatusRedraw = true;
    m_fileList.clear();
    m_activeButton = nullptr;
    m_buttonPressValid = false;
    m_wasTouching = false;
    m_selectedDirectoryIndex = -1;
    m_lastScrollOffsetPx = -1;

    // Scroll-Zustand initialisieren
    m_scrollState = ScrollState{};
    m_scrollState.lastUpdateTime = millis();

    // Viewport-Höhe berechnen (4 Pixel für die zwei Trennlinien, 20 für zusätzlichen Abstand)
    m_scrollState.viewportHeight = m_dialogH - UIConfig::HEADER_HEIGHT - UIConfig::COLUMN_HEADER_HEIGHT - 4 - UIConfig::FOOTER_HEIGHT - 20;

    // Status-Sprite Dimensionen berechnen
    const int statusSpriteWidth = m_dialogW - (2 * UIConfig::BTN_WIDTH) - (4 * UIConfig::PADDING);
    const int statusSpriteHeight = UIConfig::BTN_HEIGHT;

    // Sprites anlegen
    m_tft->safeCreateSprite(m_viewSprite, m_dialogW - UIConfig::SCROLLBAR_WIDTH - 7, m_scrollState.viewportHeight);
    m_tft->safeCreateSprite(m_scrollbarSprite, UIConfig::SCROLLBAR_WIDTH, m_scrollState.viewportHeight);
    m_tft->safeCreateSprite(m_statusSprite, statusSpriteWidth, statusSpriteHeight);

    if (!m_viewSprite || !m_scrollbarSprite || !m_statusSprite)
    {
        LOG_ERROR("FileExplorer: Sprites konnten nicht angelegt werden");
        cleanupResources();
        m_state = State::Hidden;
        return;
    }

    m_viewSprite->loadFont(ComfortaaBold18);
    m_statusSprite->loadFont(ComfortaaBold14);

    // Buttons dynamisch positionieren
    const int btnY = m_dialogY + m_dialogH - UIConfig::PADDING - (UIConfig::BTN_HEIGHT / 2);
    const int btnCancelX = m_dialogX + m_dialogW - UIConfig::PADDING - (UIConfig::BTN_WIDTH / 2);
    const int btnOkX = btnCancelX - (UIConfig::BTN_WIDTH / 2) - UIConfig::PADDING - (UIConfig::BTN_WIDTH / 2);

    m_okButton.initButton(
        m_tft,                    // Display-Objekt
        btnOkX,                   // X-Position in Button Mitte
        btnY,                     // Y-Position in Button Mitte
        UIConfig::BTN_WIDTH,      // Breite
        UIConfig::BTN_HEIGHT,     // Höhe
        UIConfig::BTN_RADIUS,     // Eckenradius
        Theme::ColorButtonBorder, // Rahmenfarbe
        Theme::ColorButtonFill,   // Füllfarbe
        Theme::ColorButtonTextOk, // Textfarbe
        "ok",                     // Beschriftung
        nullptr                   // Kein Icon
    );

    m_okButton.setFont(LeagueGothicCond34);

    m_cancelButton.initButton(
        m_tft,                        // Display-Objekt
        btnCancelX,                   // X-Position in Button Mitte
        btnY,                         // Y-Position in Button Mitte
        UIConfig::BTN_WIDTH,          // Breite
        UIConfig::BTN_HEIGHT,         // Höhe
        UIConfig::BTN_RADIUS,         // Eckenradius
        Theme::ColorButtonBorder,     // Rahmenfarbe
        Theme::ColorButtonFill,       // Füllfarbe
        Theme::ColorButtonTextCancel, // Textfarbe
        reinterpret_cast<PGM_P>(STR(CommonKey::COMMON_BTN_CANCEL)), // Beschriftung
        nullptr                       // Kein Icon
    );

    m_cancelButton.setFont(LeagueGothicCond34);

    stringutils::safeCopy(m_currentPath, sizeof(m_currentPath), "/");

    backupScreen();
    scanDirectory(m_currentPath);
    draw();
}

/**
 * @brief Aktualisiert den Zustand
 */
void FileExplorer::update()
{
    if (!isActive())
        return;

    int32_t tx = 0, ty = 0;
    const bool touched = m_tft->getTouch(&tx, &ty);

    handleTouch(tx, ty, touched);
    updateScrollPhysics();

    const bool scrollChanged = (m_scrollState.scrollOffsetPx != m_lastScrollOffsetPx);
    const bool hasVelocity = (std::abs(m_scrollState.scrollVelocity) > VELOCITY_EPSILON);

    if (m_needsRedraw || m_scrollState.isDragging || hasVelocity || scrollChanged)
    {
        drawFileList();
        drawScrollbar();
        m_lastScrollOffsetPx = m_scrollState.scrollOffsetPx;
        m_needsRedraw = false;
    }

    if (m_needsStatusRedraw)
    {
        drawStatusContent();
        m_needsStatusRedraw = false;
    }
}

/**
 * @brief Prüft, ob der Dialog aktiv ist
 */
bool FileExplorer::isActive() const
{
    return m_state == State::Active;
}

// ===================================================================
// Bildschirm-Backup
// ===================================================================

/**
 * @brief Sichert den Bildschirmbereich hinter dem Dialog
 */
void FileExplorer::backupScreen()
{
    m_backupValid = false;
    const size_t requiredSize = static_cast<size_t>(m_dialogW) * m_dialogH * sizeof(lgfx::rgb565_t);

    if (m_screenBuffer == nullptr || m_screenBufferSize < requiredSize)
    {
        if (m_screenBuffer != nullptr)
        {
            heap_caps_free(m_screenBuffer);
            m_screenBuffer = nullptr;
            m_screenBufferSize = 0;
        }

        m_screenBuffer = static_cast<lgfx::rgb565_t *>(heap_caps_malloc(requiredSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

        if (m_screenBuffer != nullptr)
        {
            m_screenBufferSize = requiredSize;
        }
        else
        {
            LOG_ERROR("FileExplorer: Konnte Backup-Buffer nicht anlegen");
            return;
        }
    }

    m_tft->readRect(m_dialogX, m_dialogY, m_dialogW, m_dialogH, m_screenBuffer);
    m_backupValid = true;
}

/**
 * @brief Stellt den gesicherten Bildschirmbereich wieder her
 */
void FileExplorer::restoreScreen()
{
    if (m_backupValid && m_screenBuffer != nullptr)
    {
        m_tft->pushImage(m_dialogX, m_dialogY, m_dialogW, m_dialogH, m_screenBuffer);
        m_backupValid = false;
    }
    else
    {
        LOG_WARNING("Restore abgebrochen: Backup ungültig oder Buffer null.");
    }
}

// ===================================================================
// Dialog-Beendigung
// ===================================================================

/**
 * @brief Beendet den Dialog und ruft den Callback auf
 */
void FileExplorer::finish(bool confirmed)
{
    std::vector<FileResult> results;

    if (confirmed)
    {
        const size_t estimatedCount = (m_dirStats.selectedDirs > 0)
                                          ? 64
                                          : static_cast<size_t>(m_dirStats.selectedFiles);
        results.reserve(estimatedCount);

        bool hasSelectedDirectory = false;

        for (const auto &entry : m_fileList)
        {
            if (entry.isSelected && entry.isDirectory && strcmp(entry.name, "..") != 0)
            {
                hasSelectedDirectory = true;

                char fullPath[256];
                if (strcmp(m_currentPath, "/") == 0)
                    stringutils::safeFormat(fullPath, sizeof(fullPath), "/%s", entry.name);
                else
                    stringutils::safeFormat(fullPath, sizeof(fullPath), "%s/%s", m_currentPath, entry.name);

                collectFilesFromDirectory(fullPath, results);
                break;
            }
        }

        if (!hasSelectedDirectory)
        {
            for (const auto &entry : m_fileList)
            {
                if (entry.isSelected && !entry.isDirectory)
                {
                    FileResult result{};

                    if (strcmp(m_currentPath, "/") == 0)
                        stringutils::safeFormat(result.path, sizeof(result.path), "/%s", entry.name);
                    else
                        stringutils::safeFormat(result.path, sizeof(result.path), "%s/%s", m_currentPath, entry.name);

                    results.push_back(result);
                }
            }
        }
    }

    restoreScreen();

    // Buffer wird nicht freigegeben, nur im Destruktor - wird bei erneutem start() wiederverwendet
    m_backupValid = false;

    cleanupResources();

    auto callback = std::move(m_callback);
    m_callback = nullptr;

    m_state = State::Hidden;

    if (callback)
        callback(results);
}

// ===================================================================
// Datei-Sammlung (rekursiv)
// ===================================================================

/**
 * @brief Sammelt alle Dateien aus einem Verzeichnis rekursiv
 */
void FileExplorer::collectFilesFromDirectory(const char *rootPath, std::vector<FileResult> &results) const
{
    struct PathBuf
    {
        char path[256];
        int depth;
    };

    std::vector<PathBuf> stack;
    stack.reserve(TYPICAL_STACK_DEPTH);

    {
        PathBuf pb{};
        stringutils::safeCopy(pb.path, sizeof(pb.path), (rootPath && rootPath[0]) ? rootPath : "/");
        pb.depth = 0;
        stack.push_back(pb);
    }

    int iterations = 0;

    while (!stack.empty())
    {
        if (++iterations > MAX_ITERATIONS)
        {
            LOG_ERROR_F("FileExplorer: Abbruch nach %d Iterationen", MAX_ITERATIONS);
            break;
        }

        if (results.size() >= static_cast<size_t>(MAX_FILES_TOTAL))
        {
            LOG_WARNING_F("FileExplorer: Dateilimit erreicht (%d)", MAX_FILES_TOTAL);
            break;
        }

        PathBuf current = stack.back();
        stack.pop_back();

        if (current.depth >= MAX_DIRECTORY_DEPTH)
        {
            LOG_WARNING_F("FileExplorer: Max. Tiefe bei: %s", current.path);
            continue;
        }

        File dir = SD.open(current.path);
        if (!dir || !dir.isDirectory())
        {
            LOG_ERROR_F("FileExplorer: Kann nicht oeffnen: %s", current.path);
            continue;
        }

        int filesInDir = 0;

        for (;;)
        {
            File file = dir.openNextFile();
            if (!file)
                break;

            if (++filesInDir > MAX_FILES_PER_DIRECTORY)
            {
                LOG_WARNING_F("FileExplorer: >%d Dateien in %s",
                              MAX_FILES_PER_DIRECTORY, current.path);
                file.close();
                break;
            }

            const char *filename = file.name();
            if (!filename || !filename[0])
            {
                file.close();
                continue;
            }

            if (!m_config.showHiddenFiles && filename[0] == '.')
            {
                file.close();
                continue;
            }

            if (isSystemDirectory(filename))
            {
                file.close();
                continue;
            }

            if (file.isDirectory())
            {
                PathBuf sub{};
                if (strcmp(current.path, "/") == 0)
                    stringutils::safeFormat(sub.path, sizeof(sub.path), "/%s", filename);
                else
                    stringutils::safeFormat(sub.path, sizeof(sub.path), "%s/%s", current.path, filename);

                sub.depth = current.depth + 1;

                if (stack.size() < static_cast<size_t>(MAX_STACK_SIZE))
                    stack.push_back(sub);
                else
                    LOG_WARNING("FileExplorer: Stack-Limit erreicht");
            }
            else
            {
                if (isExtensionAllowed(filename))
                {
                    FileResult fr{};
                    char fullPath[256];

                    if (strcmp(current.path, "/") == 0)
                        stringutils::safeFormat(fullPath, sizeof(fullPath), "/%s", filename);
                    else
                        stringutils::safeFormat(fullPath, sizeof(fullPath), "%s/%s", current.path, filename);

                    stringutils::safeCopy(fr.path, sizeof(fr.path), fullPath);
                    results.push_back(fr);
                }
            }

            file.close();
        }

        dir.close();
    }
}

/**
 * @brief Prüft, ob es sich um ein Systemverzeichnis handelt
 */
bool FileExplorer::isSystemDirectory(const char *name) const
{
    if (!name || !name[0])
        return false;

    static const char *const kSystemEntries[] = {
        "System Volume Information",
        "RECYCLER",
        "$Recycle.Bin",
        "Thumbs.db",
        "desktop.ini"};

    for (const char *sys : kSystemEntries)
    {
        if (strcasecmp(name, sys) == 0)
            return true;
    }

    return false;
}

// ===================================================================
// Spalten-Berechnung
// ===================================================================

/**
 * @brief Berechnet die Spalten-Positionen
 */
auto FileExplorer::computeColumns() const -> Columns
{
    Columns c{};
    c.listLeft = m_dialogX + 3;

    const int fallbackW = m_dialogW - UIConfig::SCROLLBAR_WIDTH - 6;
    c.listRight = c.listLeft + (m_viewSprite ? m_viewSprite->width() : fallbackW);

    c.nameX = c.listLeft + UIConfig::PADDING;
    c.sizeX = c.listRight - UIConfig::PADDING;
    c.typeCenterX = c.sizeX - SIZE_COL_RESERVE - (TYPE_COL_WIDTH / 2);

    return c;
}

// ===================================================================
// Zeichenmethoden
// ===================================================================

/**
 * @brief Zeichnet den gesamten Dialog
 */
void FileExplorer::draw()
{
    m_tft->startWrite();
    drawFrame();
    drawHeader();
    drawColumnHeader();
    drawStatusbar();
    drawButtons();
    m_needsRedraw = true;
    m_tft->endWrite();
}

/**
 * @brief Zeichnet den Rahmen des Dialogs
 */
void FileExplorer::drawFrame()
{
    // Rahmen mit abgerundeten Ecken zeichnen
    m_tft->fillSmoothRoundRect(m_dialogX, m_dialogY, m_dialogW, m_dialogH, UIConfig::BORDER_RADIUS, Theme::ColorDialogBorder);
    m_tft->fillSmoothRoundRect(m_dialogX + 1, m_dialogY + 1, m_dialogW - 2, m_dialogH - 2, UIConfig::BORDER_RADIUS, Theme::ColorMainBackGround);
}

/**
 * @brief Zeichnet die Kopfzeile mit Titel und Pfad
 */
void FileExplorer::drawHeader()
{
    const int titleY = m_dialogY + UIConfig::PADDING;
    m_tft->loadFont(ComfortaaBold16);
    m_tft->setTextColor(Theme::ColorTitle, Theme::ColorMainBackGround);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->drawString(m_config.title, m_dialogX + UIConfig::PADDING, titleY);

    const int pathY = titleY + HEADER_PATH_LINE_SPACING;
    m_tft->setTextColor(Theme::ColorMainText, Theme::ColorMainBackGround);

    const int maxPathWidth = m_dialogW - (2 * UIConfig::PADDING);

    char displayPath[256];
    stringutils::safeCopy(displayPath, sizeof(displayPath), m_currentPath);

    if (m_tft->textWidth(displayPath) > maxPathWidth)
    {
        const char *lastSlash = strrchr(m_currentPath, '/');

        if (lastSlash && lastSlash != m_currentPath)
        {
            char shortPath[256];
            stringutils::safeFormat(shortPath, sizeof(shortPath), "...%s", lastSlash);

            if (m_tft->textWidth(shortPath) <= maxPathWidth)
            {
                stringutils::safeCopy(displayPath, sizeof(displayPath), shortPath);
            }
            else
            {
                const char *p = m_currentPath;
                const char *ellipsis = "...";
                const int ellipsisWidth = m_tft->textWidth(ellipsis);

                while (*p && m_tft->textWidth(p) > maxPathWidth - ellipsisWidth)
                    p++;

                stringutils::safeFormat(displayPath, sizeof(displayPath), "%s%s", ellipsis, p);
            }
        }
        else if (strcmp(m_currentPath, "/") != 0)
        {
            stringutils::safeCopy(displayPath, sizeof(displayPath), "...");
        }
    }

    m_tft->drawString(displayPath, m_dialogX + UIConfig::PADDING, pathY);
    m_tft->unloadFont();
}

/**
 * @brief Zeichnet die Spaltenüberschriften
 */
void FileExplorer::drawColumnHeader()
{
    int y = m_dialogY + UIConfig::HEADER_HEIGHT;
    const int x = m_dialogX;

    // Obere Trennlinie
    m_tft->drawFastHLine(x + UIConfig::PADDING, y, m_dialogW - (2 * UIConfig::PADDING), Theme::ColorDialogSeparator);
    y += 2;

    m_tft->loadFont(ComfortaaBold16);
    m_tft->setTextColor(Theme::ColorAmber, Theme::ColorMainBackGround);

    const Columns col = computeColumns();
    const int textY = y + (UIConfig::COLUMN_HEADER_HEIGHT / 2);

    m_tft->setTextDatum(ML_DATUM);
    m_tft->drawString(STR(SDPlayerKey::FILEEXPLORER_COL_NAME), col.nameX, textY);

    m_tft->setTextDatum(MC_DATUM);
    m_tft->drawString(STR(SDPlayerKey::FILEEXPLORER_COL_TYPE), col.typeCenterX, textY);

    m_tft->setTextDatum(MR_DATUM);
    m_tft->drawString(STR(SDPlayerKey::FILEEXPLORER_COL_SIZE), col.sizeX, textY);
    m_tft->unloadFont();

    // Untere Trennlinie
    y = m_dialogY + UIConfig::HEADER_HEIGHT + UIConfig::COLUMN_HEADER_HEIGHT + 2;
    m_tft->drawFastHLine(x + UIConfig::PADDING, y, m_dialogW - (2 * UIConfig::PADDING), Theme::ColorDialogSeparator);
}

/**
 * @brief Aktualisiert die Verzeichnis-Statistiken
 */
void FileExplorer::updateDirectoryStats()
{
    m_dirStats = DirectoryStats{};

    for (const auto &entry : m_fileList)
    {
        if (strcmp(entry.name, "..") == 0)
            continue;

        if (entry.isDirectory)
        {
            m_dirStats.dirCount++;
            if (entry.isSelected)
                m_dirStats.selectedDirs++;
        }
        else
        {
            m_dirStats.fileCount++;
            if (entry.isSelected)
            {
                m_dirStats.selectedFiles++;
                m_dirStats.selectedSize += entry.size;
            }
        }
    }

    m_needsStatusRedraw = true;
}

/**
 * @brief Zeichnet den Inhalt des Status-Sprites
 */
void FileExplorer::drawStatusContent()
{
    if (!m_statusSprite)
        return;

    m_statusSprite->clear(Theme::ColorMainBackGround);
    m_statusSprite->setTextColor(Theme::ColorMainText, Theme::ColorMainBackGround);
    m_statusSprite->setTextDatum(TL_DATUM);

    // Erste Zeile: Inhalt des aktuellen Verzeichnisses
    char line1[128];
    STRF(line1, SDPlayerKey::FILEEXPLORER_STATUS_OVERVIEW, m_dirStats.fileCount, m_dirStats.dirCount);
    m_statusSprite->drawString(line1, 0, 2);

    // Zweite Zeile: Auswahl-Information
    char line2[128];
    if (m_dirStats.selectedDirs > 0)
    {
        STRF(line2, SDPlayerKey::FILEEXPLORER_STATUS_FOLDERS_SELECTED, m_dirStats.selectedDirs);
        m_statusSprite->drawString(line2, 0, 20);
    }
    else if (m_dirStats.selectedFiles > 0)
    {
        char sizeStr[20];
        formatSize(static_cast<uint32_t>(m_dirStats.selectedSize), sizeStr, sizeof(sizeStr));
        STRF(line2, SDPlayerKey::FILEEXPLORER_STATUS_FILES_SELECTED, sizeStr, m_dirStats.selectedFiles);
        m_statusSprite->drawString(line2, 0, 20);
    }
    else
    {
        m_statusSprite->drawString(STR(SDPlayerKey::FILEEXPLORER_STATUS_NOTHING_SELECTED), 0, 20);
    }

    const int statusX = m_dialogX + UIConfig::PADDING;
    const int statusY = m_dialogY + m_dialogH - UIConfig::PADDING - UIConfig::BTN_HEIGHT;
    m_statusSprite->pushSprite(statusX, statusY);
}

/**
 * @brief Zeichnet die Statusleiste
 */
void FileExplorer::drawStatusbar()
{
    drawStatusContent();
}

/**
 * @brief Zeichnet die Buttons
 */
void FileExplorer::drawButtons()
{
    m_okButton.drawButton();
    m_cancelButton.drawButton();
}

/**
 * @brief Zeichnet die Dateiliste in das Sprite
 */
void FileExplorer::drawFileList()
{
    if (!m_viewSprite)
        return;

    m_viewSprite->clear(Theme::ColorMainBackGround);

    if (m_fileList.empty())
    {
        m_viewSprite->setTextColor(Theme::ColorMainText, Theme::ColorMainBackGround);
        m_viewSprite->setTextDatum(MC_DATUM);
        m_viewSprite->drawString(STR(SDPlayerKey::FILEEXPLORER_EMPTY), m_viewSprite->width() / 2, m_viewSprite->height() / 2);
    }
    else
    {
        const int itemHeight = UIConfig::ITEM_HEIGHT;
        const int scrollOffset = m_scrollState.scrollOffsetPx;
        const int viewportH = m_scrollState.viewportHeight;

        const Columns col = computeColumns();

        int startIndex = scrollOffset / itemHeight;
        const int visibleCount = (viewportH / itemHeight) + 2;
        int endIndex = startIndex + visibleCount;

        startIndex = std::max(0, startIndex);
        endIndex = std::min(endIndex, static_cast<int>(m_fileList.size()));

        for (int i = startIndex; i < endIndex; ++i)
        {
            const int yPos = (i * itemHeight) - scrollOffset;

            if (yPos > -itemHeight && yPos < viewportH)
                drawFileEntry(i, yPos, col);
        }
    }

    m_viewSprite->pushSprite(m_dialogX + 3, m_dialogY + UIConfig::HEADER_HEIGHT + UIConfig::COLUMN_HEADER_HEIGHT + 4);
}

/**
 * @brief Liefert das Ordnersymbol passend zum Zeilengrund; auf dem Auswahlbalken eine eigene, mit dessen Farbe als Grund gezeichnete Fassung, damit die Kachel nicht schwarz durchscheint
 */
const uint16_t *FileExplorer::folderIcon(bool up, bool selected)
{
    const IconId id = up ? IconId::FolderUp : IconId::Folder;
    if (!selected)
        return IconShapes::cached(id, ICON_FOLDER_COLOR, ICON_FOLDER_BACKGROUND);

    uint16_t *&buffer = up ? m_folderUpIconSelected : m_folderIconSelected;
    if (!buffer)
        buffer = IconShapes::draw(id, ICON_FOLDER_COLOR, Theme::ColorSelectedRow);
    return buffer;
}

/**
 * @brief Zeichnet einen einzelnen Eintrag in der Dateiliste
 */
void FileExplorer::drawFileEntry(int index, int yPos, const Columns &col)
{
    if (!m_viewSprite || index < 0 || index >= static_cast<int>(m_fileList.size()))
        return;

    const auto &entry = m_fileList[index];

    uint16_t bgColor = Theme::ColorMainBackGround;
    if (entry.isSelected)
    {
        bgColor = Theme::ColorSelectedRow;
        m_viewSprite->fillRect(0, yPos, m_viewSprite->width(), UIConfig::ITEM_HEIGHT, bgColor);
    }

    uint16_t textColor = entry.isDirectory ? Theme::ColorCyan : Theme::ColorMainText;
    if (entry.isSelected)
        textColor = TFT_WHITE;

    m_viewSprite->setTextColor(textColor, bgColor);

    const int spriteOffsetX = col.listLeft;
    int nameX_sprite = col.nameX - spriteOffsetX;
    const int sizeX_sprite = col.sizeX - spriteOffsetX;
    const int typeCenterX_sprite = col.typeCenterX - spriteOffsetX;

    // Icons zeichnen
    if (entry.isDirectory)
    {
        const int iconY = yPos + (UIConfig::ITEM_HEIGHT - 16) / 2;
        const bool up = (strcmp(entry.name, "..") == 0);
        const uint16_t *icon = folderIcon(up, entry.isSelected);
        if (icon)
        {
            if (up)
                m_viewSprite->pushImage(nameX_sprite, iconY + 2, 17, 16, icon);
            else
                m_viewSprite->pushImage(nameX_sprite, iconY - 1, 19, 16, icon);
        }

        nameX_sprite += 19 + ICON_MARGIN;
    }

    const int maxNameWidth = typeCenterX_sprite - nameX_sprite - (TYPE_COL_WIDTH / 2) - NAME_TYPE_GAP;

    // Name ohne Extension vorbereiten
    const char *displayName = entry.name;
    char tempNameBuf[128];

    if (!entry.isDirectory)
    {
        stringutils::safeCopy(tempNameBuf, sizeof(tempNameBuf), entry.name);
        char *dot = strrchr(tempNameBuf, '.');
        if (dot)
            *dot = '\0';

        displayName = tempNameBuf;
    }

    // Text kürzen falls nötig
    char truncatedName[128];
    truncateTextToWidth(displayName, maxNameWidth, truncatedName, sizeof(truncatedName));

    m_viewSprite->setTextDatum(ML_DATUM);
    m_viewSprite->drawString(truncatedName, nameX_sprite, yPos + UIConfig::ITEM_HEIGHT / 2);

    // Größe (rechts)
    if (!entry.isDirectory && entry.size > 0)
    {
        char sizeStr[20];
        formatSize(entry.size, sizeStr, sizeof(sizeStr));
        m_viewSprite->setTextDatum(MR_DATUM);
        m_viewSprite->drawString(sizeStr, sizeX_sprite, yPos + UIConfig::ITEM_HEIGHT / 2);
    }

    // Typ (zentriert)
    m_viewSprite->setTextDatum(MC_DATUM);
    if (entry.isDirectory)
    {
        m_viewSprite->drawString("<DIR>", typeCenterX_sprite, yPos + UIConfig::ITEM_HEIGHT / 2);
    }
    else
    {
        const char *extension = (entry.extension[0] == '.') ? entry.extension + 1 : entry.extension;

        uint16_t typeColor = textColor;
        if (strcasecmp(extension, "mp3") == 0)
            typeColor = Theme::ColorGreen;
        else if (strcasecmp(extension, "flac") == 0)
            typeColor = TFT_YELLOW;

        if (typeColor != textColor)
            m_viewSprite->setTextColor(typeColor, bgColor);

        m_viewSprite->drawString(extension, typeCenterX_sprite, yPos + UIConfig::ITEM_HEIGHT / 2);

        if (typeColor != textColor)
            m_viewSprite->setTextColor(textColor, bgColor);
    }
}

/**
 * @brief Zeichnet die Scrollbar
 */
void FileExplorer::drawScrollbar()
{
    if (!m_scrollbarSprite)
        return;

    const int x = m_dialogX + m_dialogW - UIConfig::SCROLLBAR_WIDTH - 3;
    const int y = m_dialogY + UIConfig::HEADER_HEIGHT + UIConfig::COLUMN_HEADER_HEIGHT + 4;

    m_scrollbarSprite->clear(Theme::ColorMainBackGround);

    if (m_scrollState.maxScroll > 0)
    {
        const float visibleRatio = static_cast<float>(m_scrollState.viewportHeight) / static_cast<float>(m_scrollState.contentHeight);
        const int barHeight = std::max(20, static_cast<int>(std::lround(m_scrollState.viewportHeight * visibleRatio)));
        const int clampedOffset = std::max(0, std::min(m_scrollState.scrollOffsetPx, m_scrollState.maxScroll));
        const float scrollRatio = static_cast<float>(clampedOffset) / static_cast<float>(m_scrollState.maxScroll);
        const int barY = static_cast<int>(std::lround(scrollRatio * (m_scrollState.viewportHeight - barHeight)));

        m_scrollbarSprite->fillRect(0, 0, UIConfig::SCROLLBAR_WIDTH, m_scrollState.viewportHeight, SCROLLBAR_TRACK_COLOR);

        const uint16_t barColor = m_scrollState.isDragging ? SCROLLBAR_THUMB_DRAG : SCROLLBAR_THUMB_COLOR;
        m_scrollbarSprite->fillRect(0, barY, UIConfig::SCROLLBAR_WIDTH, barHeight, barColor);
    }

    m_scrollbarSprite->pushSprite(x, y);
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Eingaben
 */
void FileExplorer::handleTouch(int32_t x, int32_t y, bool is_touched)
{
    bool button_event = false;

    // Button losgelassen
    if (!is_touched && m_activeButton)
    {
        Button *released = m_activeButton;
        released->press(false);
        released->drawButton(false);

        const bool shouldConfirm = m_buttonPressValid;

        m_activeButton = nullptr;
        m_buttonPressValid = false;

        if (shouldConfirm)
        {
            const bool confirmed = (released == &m_okButton);
            finish(confirmed);
        }
        return;
    }

    // Button neu gedrückt
    if (is_touched && !m_wasTouching && !m_activeButton)
    {
        if (m_okButton.contains(x, y))
        {
            m_activeButton = &m_okButton;
            m_buttonPressValid = true;
            button_event = true;
        }
        else if (m_cancelButton.contains(x, y))
        {
            m_activeButton = &m_cancelButton;
            m_buttonPressValid = true;
            button_event = true;
        }

        if (button_event)
        {
            m_activeButton->press(true);
            m_activeButton->drawButton(true);
        }
    }

    // Button wird gehalten
    if (is_touched && m_activeButton)
    {
        button_event = true;

        const bool stillOnButton = m_activeButton->contains(x, y);

        if (stillOnButton != m_buttonPressValid)
        {
            m_buttonPressValid = stillOnButton;
            m_activeButton->press(stillOnButton);
            m_activeButton->drawButton(stillOnButton);
        }
    }

    // Scroll-Logik nur wenn kein Button-Event
    if (!button_event)
    {
        const Columns col = computeColumns();
        const int viewY = m_dialogY + UIConfig::HEADER_HEIGHT + UIConfig::COLUMN_HEADER_HEIGHT + 4;
        const bool inScrollView = (x >= col.listLeft && x < col.listRight && y >= viewY && y < viewY + m_scrollState.viewportHeight);
        const unsigned long currentTime = millis();

        // Touch beginnt (steigende Flanke)
        if (is_touched && !m_wasTouching)
        {
            if (inScrollView)
            {
                m_scrollState.isDragging = false;
                m_scrollState.longPressHandled = false;
                m_scrollState.scrollVelocity = 0.0f;
                m_scrollState.touchStartTime = currentTime;
                m_scrollState.pressStartX = x;
                m_scrollState.pressStartY = y;
                m_scrollState.tapCandidateIndex = (m_scrollState.scrollOffsetPx + (y - viewY)) / UIConfig::ITEM_HEIGHT;
            }
        }

        // Touch wird gehalten
        else if (is_touched && m_wasTouching)
        {
            if (m_scrollState.tapCandidateIndex != -1 && !m_scrollState.longPressHandled)
            {
                const int dx = std::abs(x - m_scrollState.pressStartX);
                const int dy = std::abs(y - m_scrollState.pressStartY);

                if (dx > DRAG_THRESHOLD_PX || dy > DRAG_THRESHOLD_PX)
                {
                    m_scrollState.tapCandidateIndex = -1;
                    m_scrollState.isDragging = true;
                    m_scrollState.dragStartY = y;
                    m_scrollState.dragStartPos = m_scrollState.scrollPos;
                }
                else if (currentTime - m_scrollState.touchStartTime > LONG_PRESS_THRESHOLD_MS)
                {
                    toggleItemSelection(m_scrollState.tapCandidateIndex);
                    m_scrollState.longPressHandled = true;
                    m_scrollState.tapCandidateIndex = -1;
                }
            }

            if (m_scrollState.isDragging)
            {
                const int deltaY = y - m_scrollState.dragStartY;
                m_scrollState.scrollPos = m_scrollState.dragStartPos - (deltaY * DRAG_SCROLL_MULTIPLIER);
                m_scrollState.scrollOffsetPx = static_cast<int>(lroundf(m_scrollState.scrollPos));
                m_needsRedraw = true;

                // Velocity-Berechnung
                const unsigned long dt_ms = currentTime - m_scrollState.lastTouchTime;
                if (dt_ms > 0 && dt_ms < 100)
                {
                    float newVelocity = ((m_scrollState.lastTouchY - y) * 1000.0f) / dt_ms;
                    m_scrollState.scrollVelocity = (newVelocity * VELOCITY_SMOOTH_NEW) + (m_scrollState.scrollVelocity * VELOCITY_SMOOTH_OLD);
                    m_scrollState.scrollVelocity = std::clamp(m_scrollState.scrollVelocity * VELOCITY_BOOST, -MAX_VELOCITY, MAX_VELOCITY);
                }

                // Gummiband-Effekt beim Überziehen
                if (m_scrollState.scrollPos < -RUBBER_BAND_LIMIT)
                {
                    const float excess = m_scrollState.scrollPos + RUBBER_BAND_LIMIT;
                    m_scrollState.scrollPos = -RUBBER_BAND_LIMIT + (excess * RUBBER_BAND_RESISTANCE);
                    m_scrollState.scrollOffsetPx = static_cast<int>(lroundf(m_scrollState.scrollPos));
                }
                else if (m_scrollState.scrollPos > m_scrollState.maxScroll + RUBBER_BAND_LIMIT)
                {
                    const float excess = m_scrollState.scrollPos - m_scrollState.maxScroll - RUBBER_BAND_LIMIT;
                    m_scrollState.scrollPos = m_scrollState.maxScroll + RUBBER_BAND_LIMIT + (excess * RUBBER_BAND_RESISTANCE);
                    m_scrollState.scrollOffsetPx = static_cast<int>(lroundf(m_scrollState.scrollPos));
                }
            }

            m_scrollState.lastTouchY = y;
            m_scrollState.lastTouchTime = currentTime;
        }

        // Touch losgelassen
        else if (!is_touched && m_wasTouching)
        {
            if (m_scrollState.tapCandidateIndex != -1)
            {
                if (currentTime - m_scrollState.touchStartTime < SHORT_TAP_THRESHOLD_MS)
                {
                    const int idx = m_scrollState.tapCandidateIndex;
                    if (idx >= 0 && idx < static_cast<int>(m_fileList.size()) && m_fileList[idx].isDirectory)
                        selectItem(idx);
                }
            }

            m_scrollState.scrollVelocity = std::max(-MAX_VELOCITY, std::min(m_scrollState.scrollVelocity, MAX_VELOCITY));

            m_scrollState.isDragging = false;
            m_scrollState.tapCandidateIndex = -1;
        }
    }

    m_wasTouching = is_touched;
}

// ===================================================================
// Zeitbasierte Scroll-Physik
// ===================================================================

/**
 * @brief Aktualisiert die Scroll-Physik zeitbasiert
 */
void FileExplorer::updateScrollPhysics()
{
    if (m_scrollState.isDragging)
        return;

    unsigned long currentTime = millis();
    float dt = std::min(static_cast<float>(currentTime - m_scrollState.lastUpdateTime) / 1000.0f, MAX_DELTA_TIME);
    m_scrollState.lastUpdateTime = currentTime;

    if (dt <= 0.0f)
        return;

    bool positionChanged = false;

    // Gummiband-Zurückschnappen am oberen Ende
    if (m_scrollState.scrollPos < 0.0f)
    {
        float displacement = -m_scrollState.scrollPos;
        float springForce = displacement * SPRING_RATE;
        m_scrollState.scrollPos += springForce * dt;
        m_scrollState.scrollVelocity = 0.0f;

        if (std::abs(m_scrollState.scrollPos) < SNAP_THRESHOLD)
            m_scrollState.scrollPos = 0.0f;

        positionChanged = true;
    }
    // Gummiband-Zurückschnappen am unteren Ende
    else if (m_scrollState.scrollPos > static_cast<float>(m_scrollState.maxScroll))
    {
        float displacement = static_cast<float>(m_scrollState.maxScroll) - m_scrollState.scrollPos;
        float springForce = displacement * SPRING_RATE;
        m_scrollState.scrollPos += springForce * dt;
        m_scrollState.scrollVelocity = 0.0f;

        if (std::abs(m_scrollState.scrollPos - static_cast<float>(m_scrollState.maxScroll)) < SNAP_THRESHOLD)
            m_scrollState.scrollPos = static_cast<float>(m_scrollState.maxScroll);

        positionChanged = true;
    }
    // Momentum-Scrolling innerhalb der Grenzen
    else if (std::abs(m_scrollState.scrollVelocity) > VELOCITY_EPSILON)
    {
        m_scrollState.scrollPos += m_scrollState.scrollVelocity * dt;

        // Reibung: exponentieller Abfall, unabhängig von der Frame-Rate
        m_scrollState.scrollVelocity *= std::exp(-FRICTION_DECAY * dt);

        if (std::abs(m_scrollState.scrollVelocity) < SOFT_BRAKE_THRESHOLD && std::abs(m_scrollState.scrollVelocity) > VELOCITY_EPSILON)
            m_scrollState.scrollVelocity *= SOFT_BRAKE_FACTOR;

        if (std::abs(m_scrollState.scrollVelocity) < VELOCITY_EPSILON)
            m_scrollState.scrollVelocity = 0.0f;

        if (m_scrollState.scrollPos < 0.0f && std::abs(m_scrollState.scrollVelocity) < VELOCITY_EPSILON)
            m_scrollState.scrollPos = 0.0f;
        else if (m_scrollState.scrollPos > static_cast<float>(m_scrollState.maxScroll) && std::abs(m_scrollState.scrollVelocity) < VELOCITY_EPSILON)
            m_scrollState.scrollPos = static_cast<float>(m_scrollState.maxScroll);

        positionChanged = true;
    }
    else
    {
        m_scrollState.scrollVelocity = 0.0f;
    }

    if (positionChanged)
    {
        m_scrollState.scrollOffsetPx = static_cast<int>(lroundf(m_scrollState.scrollPos));
        m_needsRedraw = true;
    }
}

// ===================================================================
// Verzeichnis-Navigation
// ===================================================================

/**
 * @brief Scannt ein Verzeichnis und füllt die Dateiliste
 */
void FileExplorer::scanDirectory(const char *path)
{
    m_fileList.clear();
    m_fileList.reserve(64);
    m_selectedDirectoryIndex = -1;
    stringutils::safeCopy(m_currentPath, sizeof(m_currentPath), path);

    File root = SD.open(path);
    if (!root || !root.isDirectory())
    {
        LOG_ERROR_F("FileExplorer: Verzeichnis nicht oeffenbar: %s", path);
        return;
    }

    // ".." Eintrag für nicht-Root Verzeichnisse
    if (strcmp(path, "/") != 0)
    {
        FileEntry entry{};
        stringutils::safeCopy(entry.name, sizeof(entry.name), "..");
        entry.isDirectory = true;
        m_fileList.push_back(entry);
    }

    File file = root.openNextFile();
    while (file)
    {
        const char *filename = file.name();
        const char *baseName = strrchr(filename, '/');
        baseName = baseName ? baseName + 1 : filename;

        bool shouldProcess = true;

        if (!m_config.showHiddenFiles && baseName[0] == '.')
            shouldProcess = false;

        if (shouldProcess && isSystemDirectory(baseName))
            shouldProcess = false;

        if (shouldProcess)
        {
            FileEntry entry{};
            entry.isDirectory = file.isDirectory();

            if (entry.isDirectory || isExtensionAllowed(baseName))
            {
                stringutils::safeCopy(entry.name, sizeof(entry.name), baseName);
                entry.size = file.size();
                entry.isSelected = false;

                if (!entry.isDirectory)
                {
                    const char *ext = strrchr(baseName, '.');
                    if (ext)
                        stringutils::safeCopy(entry.extension, sizeof(entry.extension), ext);
                }
                m_fileList.push_back(entry);
            }
        }

        file.close();
        file = root.openNextFile();
    }
    root.close();

    // Sortierung: ".." zuerst, dann Verzeichnisse, dann Dateien (jeweils alphabetisch)
    std::sort(m_fileList.begin(), m_fileList.end(),
              [](const FileEntry &a, const FileEntry &b)
              {
                  if (strcmp(a.name, "..") == 0)
                      return true;

                  if (strcmp(b.name, "..") == 0)
                      return false;

                  if (a.isDirectory != b.isDirectory)
                      return a.isDirectory;

                  return strcasecmp(a.name, b.name) < 0;
              });

    // Scroll-Zustand zurücksetzen
    m_scrollState.scrollPos = 0.0f;
    m_scrollState.scrollOffsetPx = 0;
    m_scrollState.scrollVelocity = 0.0f;
    m_scrollState.contentHeight = static_cast<int>(m_fileList.size()) * UIConfig::ITEM_HEIGHT;
    m_scrollState.maxScroll = std::max(0, m_scrollState.contentHeight - m_scrollState.viewportHeight);
    m_scrollState.lastUpdateTime = millis();

    m_needsRedraw = true;
    drawHeader();
    updateDirectoryStats();
}

/**
 * @brief Navigiert zum übergeordneten Verzeichnis
 */
void FileExplorer::navigateToParent()
{
    char tempPath[256];
    stringutils::safeCopy(tempPath, sizeof(tempPath), m_currentPath);
    char *lastSlash = strrchr(tempPath, '/');

    if (lastSlash > tempPath)
        *lastSlash = '\0';
    else if (lastSlash == tempPath && strlen(tempPath) > 1)
        tempPath[1] = '\0';

    scanDirectory(tempPath);
}

/**
 * @brief Navigiert in ein Unterverzeichnis
 */
void FileExplorer::navigateToChild(const char *childName)
{
    if (!childName || !childName[0])
        return;

    char newPath[256];
    if (strcmp(m_currentPath, "/") == 0)
        stringutils::safeFormat(newPath, sizeof(newPath), "/%s", childName);
    else
        stringutils::safeFormat(newPath, sizeof(newPath), "%s/%s", m_currentPath, childName);

    scanDirectory(newPath);
}

/**
 * @brief Behandelt einen Tap auf einen Eintrag
 */
void FileExplorer::selectItem(int index)
{
    if (index < 0 || index >= static_cast<int>(m_fileList.size()))
        return;

    if (m_fileList[index].isDirectory)
    {
        if (strcmp(m_fileList[index].name, "..") == 0)
            navigateToParent();
        else
            navigateToChild(m_fileList[index].name);
    }
    else
    {
        toggleItemSelection(index);
    }
}

/**
 * @brief Schaltet die Auswahl eines Eintrags um
 */
void FileExplorer::toggleItemSelection(int index)
{
    if (index < 0 || index >= static_cast<int>(m_fileList.size()))
        return;

    if (strcmp(m_fileList[index].name, "..") == 0)
        return;

    if (m_fileList[index].isDirectory)
    {
        if (m_selectedDirectoryIndex != -1 && m_selectedDirectoryIndex != index)
            m_fileList[m_selectedDirectoryIndex].isSelected = false;

        m_fileList[index].isSelected = !m_fileList[index].isSelected;

        if (m_fileList[index].isSelected)
        {
            // Alle Dateien abwählen wenn ein Verzeichnis ausgewählt wird
            for (auto &entry : m_fileList)
            {
                if (!entry.isDirectory)
                    entry.isSelected = false;
            }
            m_selectedDirectoryIndex = index;
        }
        else
        {
            m_selectedDirectoryIndex = -1;
        }
    }
    else
    {
        if (m_selectedDirectoryIndex == -1)
            m_fileList[index].isSelected = !m_fileList[index].isSelected;
    }

    m_needsRedraw = true;
    updateDirectoryStats();
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Formatiert eine Dateigröße in lesbare Einheiten
 */
void FileExplorer::formatSize(uint32_t bytes, char *buf, size_t len) const
{
    if (!buf || len == 0)
        return;

    static const char *const suffix[] = {"B", "KB", "MB", "GB"};

    if (bytes == 0)
    {
        stringutils::safeFormat(buf, len, "0 B");
        return;
    }

    double size = static_cast<double>(bytes);
    int unitIndex = 0;

    while (size >= BYTES_PER_KB && unitIndex < MAX_SIZE_UNIT_INDEX)
    {
        size /= BYTES_PER_KB;
        unitIndex++;
    }

    stringutils::safeFormat(buf, len, "%.2f %s", size, suffix[unitIndex]);

    // Im deutschen Sprachmodus den Punkt durch ein Komma ersetzen
    if (I18n::currentLang() == LangIndex::DE)
    {
        for (size_t k = 0; buf[k] != '\0' && k < len; ++k)
        {
            if (buf[k] == '.')
            {
                buf[k] = ',';
                break;
            }
        }
    }
}

/**
 * @brief Prüft, ob die Dateiendung erlaubt ist
 */
bool FileExplorer::isExtensionAllowed(const char *filename) const
{
    if (m_config.extensionCount == 0)
        return true;

    if (!filename)
        return false;

    const char *ext = strrchr(filename, '.');
    if (!ext)
        return false;

    for (size_t i = 0; i < m_config.extensionCount; ++i)
    {
        if (m_config.allowedExtensions[i] && strcasecmp(ext, m_config.allowedExtensions[i]) == 0)
            return true;
    }

    return false;
}

/**
 * @brief Kürzt Text auf maximale Pixelbreite mit Ellipse
 */
void FileExplorer::truncateTextToWidth(const char *text, int maxWidth, char *output, size_t outputSize) const
{
    if (!text || !m_viewSprite || !output || outputSize == 0)
    {
        if (output && outputSize > 0)
            output[0] = '\0';
        return;
    }

    // Schneller Pfad: Text passt komplett
    if (m_viewSprite->textWidth(text) <= maxWidth)
    {
        stringutils::safeCopy(output, outputSize, text);
        return;
    }

    const int ellipsisWidth = m_viewSprite->textWidth("...");
    const int targetWidth = maxWidth - ellipsisWidth;

    if (targetWidth <= 0)
    {
        stringutils::safeCopy(output, outputSize, "...");
        return;
    }

    const size_t len = strlen(text);
    const size_t maxLen = std::min<size_t>(len, outputSize - 4);

    auto isUtf8Continuation = [](char c)
    {
        return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
    };

    // textWidth() ist monoton steigend mit der Textlänge
    char tempBuffer[128];
    size_t low = 0;
    size_t high = std::min<size_t>(maxLen, sizeof(tempBuffer) - 1);
    size_t bestFit = 0;

    while (low <= high)
    {
        const size_t mid = low + (high - low) / 2;

        memcpy(tempBuffer, text, mid);
        tempBuffer[mid] = '\0';

        if (m_viewSprite->textWidth(tempBuffer) <= targetWidth)
        {
            bestFit = mid;
            low = mid + 1;
        }
        else
        {
            if (mid == 0)
                break;
            high = mid - 1;
        }
    }

    // UTF-8 Korrektur: zurückgehen bis zum Start des Codepoints
    while (bestFit > 0 && isUtf8Continuation(text[bestFit]))
        --bestFit;

    if (bestFit > 0)
    {
        memcpy(tempBuffer, text, bestFit);
        tempBuffer[bestFit] = '\0';
        stringutils::safeCopy(output, outputSize, tempBuffer);
        stringutils::safeAppend(output, outputSize, "...");
    }
    else
    {
        stringutils::safeCopy(output, outputSize, "...");
    }
}
