#include "Tools.h"

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <strings.h>

#include "AppState.h"
#include "Logger.h"
#include "SD.h"
#include "StringUtils.h"

// ===================================================================
// Systeminformationen
// ===================================================================

/**
 * @brief Gibt Chip-Modell, Core-Anzahl, Taktfrequenz, Heap-, PSRAM- und Flash-Größen aus
 */
void systemInfo()
{
    uint32_t psrSize = ESP.getPsramSize();
    uint32_t fSize = ESP.getFlashChipSize();
    uint32_t hSize = ESP.getHeapSize();
    uint32_t psrFree = ESP.getFreePsram();
    uint32_t hFree = ESP.getFreeHeap();
    uint32_t maxAllocHeap = ESP.getMaxAllocHeap();
    uint32_t maxAllocPsram = ESP.getMaxAllocPsram();
    uint8_t cores = ESP.getChipCores();
    uint32_t freq = ESP.getCpuFreqMHz();
    const char *model = ESP.getChipModel();

    LOG_LINE_LABEL_COLOR("System", Logger::CYAN, 80);
    LOG_INFO_F("Model: %s, Cores: %u, Frequency: %lu MHz", model, cores, freq);
    LOG_INFO_F("Size: Heap: %lu KB, PSRAM: %lu KB, Flash: %lu KB", hSize / 1024, psrSize / 1024, fSize / 1024);
    LOG_INFO_F("Free: Heap: %lu KB, PSRAM: %lu KB", hFree / 1024, psrFree / 1024);
    LOG_INFO_F("MaxAlloc: Heap: %lu KB, PSRAM: %lu KB", maxAllocHeap / 1024, maxAllocPsram / 1024);
    LOG_LINE_COLOR(Logger::CYAN, 80);
}

// ===================================================================
// SD-Karte
// ===================================================================

/**
 * @brief Gibt Kartentyp, Kartengröße, Gesamtplatz und belegten Platz aus
 */
void sdCardInfo(bool mount)
{
    if (!mount)
    {
        LOG_ERROR("Die SD-Karte konnte nicht eingebunden werden.");
        return;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE)
    {
        LOG_ERROR("Keine SD-Karte gefunden.");
        return;
    }

    const char *typeName = "UNBEKANNT";
    if (cardType == CARD_MMC)
        typeName = "MMC";
    else if (cardType == CARD_SD)
        typeName = "SDSC";
    else if (cardType == CARD_SDHC)
        typeName = "SDHC";

    LOG_LINE_LABEL_COLOR("SD-Karte", Logger::CYAN, 80);
    LOG_INFO_F("Kartentyp: %s", typeName);
    LOG_INFO_F("Kartengröße: %llu MB", SD.cardSize() / (1024ULL * 1024ULL));
    LOG_INFO_F("Gesamtplatz: %llu MB", SD.totalBytes() / (1024ULL * 1024ULL));
    LOG_INFO_F("Belegter Platz: %llu MB", SD.usedBytes() / (1024ULL * 1024ULL));
    LOG_LINE_COLOR(Logger::CYAN, 80);
}

// ===================================================================
// RTC
// ===================================================================

/**
 * @brief Gibt den Initialisierungsstatus der RTC aus
 */
void rtcInfo(bool init)
{
    LOG_LINE_LABEL_COLOR("RTC", Logger::CYAN, 80);

    if (!init)
        LOG_ERROR("Die RTC konnte nicht initialisiert werden.");
    else
        LOG_SUCCESS("RTC bereit");

    LOG_LINE_COLOR(Logger::CYAN, 80);
}

// ===================================================================
// Temporäre Nordlicht-Messsonde
// ===================================================================

namespace auroraprobe
{
namespace
{
    constexpr uint16_t FRAME_COUNT = 128; ///< Vollständige Bilder je Einzelaufnahme
    constexpr uint32_t SETTLE_MS = 2000; ///< Ruhezeit nach dem Startkommando
    constexpr uint32_t TIMEOUT_MS = 20000; ///< Maximale Dauer der eigentlichen Aufnahme
    constexpr int SERIAL_RESERVE = 224; ///< Freier Sendespeicher vor einer kurzen Ausgabezeile
    constexpr uint8_t SECTION_COUNT = static_cast<uint8_t>(LoopSection::Count); ///< Anzahl der Zeitabschnitte
    static constexpr const char *SECTION_NAMES[] = {"Netz", "Audio", "Status", "Statistik", "FTP", "UI", "UI_Zeitfenster", "UI_Touch", "UI_Daten", "UI_Visualizer", "UI_Bedienung", "UI_Uhr", "UI_Kassette"}; ///< Reihenfolge der Zeitabschnitte
    static_assert(sizeof(SECTION_NAMES) / sizeof(SECTION_NAMES[0]) == SECTION_COUNT, "Abschnittsnamen müssen vollständig sein");

    /**
     * @brief Zustände einer ausdrücklich gestarteten Einzelaufnahme
     */
    enum class Phase : uint8_t
    {
        Idle, ///< Kein Messauftrag
        Settling, ///< Warte auf eine stabile Ausgangslage
        Recording, ///< Erfasse Einzelbilder ohne Messausgaben
        Draining ///< Gib die eingefrorene Aufnahme zeilenweise aus
    };

    /**
     * @brief Summe und größte Laufzeit eines Abschnitts
     */
    struct Metric
    {
        uint32_t sum = 0; ///< Summe in Mikrosekunden
        uint32_t maximum = 0; ///< Größte einzelne Laufzeit in Mikrosekunden

        /**
         * @brief Übernimmt eine Abschnittslaufzeit
         */
        void add(uint32_t value)
        {
            sum += value;
            if (value > maximum)
                maximum = value;
        }
    };

    /**
     * @brief Zeiten zwischen den Hauptschleifenenden zweier Bildausgaben
     */
    struct Interval
    {
        Metric sections[SECTION_COUNT]; ///< Abschnittssummen und Maxima; UI enthält seine Unterabschnitte
        uint32_t loops = 0; ///< Vollständige Hauptschleifendurchläufe
        uint32_t loopMaximum = 0; ///< Größte Hauptschleifenlaufzeit in Mikrosekunden
        uint32_t audioGapMaximum = 0; ///< Größter Audio-Aufrufabstand in Mikrosekunden
    };

    /**
     * @brief Einzelbild mit Zeitstempeln und zugehörigem Hauptschleifenintervall
     */
    struct Frame
    {
        uint32_t beginUs = 0; ///< Beginn der Bildberechnung, micros-Zeitstempel
        uint32_t animationMs = 0; ///< Tatsächlich verwendeter Bewegungsschritt in Millisekunden
        uint32_t pushBeginUs = 0; ///< Beginn der Bildkopie, micros-Zeitstempel
        uint32_t pushEndUs = 0; ///< Ende der Bildkopie, kein gemessener Display-Abtastzeitpunkt
        uint32_t computeUs = 0; ///< Summe der Rechenabschnitte in Mikrosekunden
        uint32_t slices = 0; ///< Anzahl der Rechenabschnitte
        uint32_t gapUs = 0; ///< Abstand zum vorigen Ende einer Bildkopie
        uint32_t loopEndUs = 0; ///< Ende des Durchlaufs mit dieser Bildausgabe
        Interval interval; ///< Laufzeiten im zugehörigen Hauptschleifenintervall
    };

    Frame *frames = nullptr; ///< Einmalig beim Start reservierter PSRAM-Messspeicher
    Frame currentFrame; ///< Aktuell berechnetes Bild
    Interval interval; ///< Laufende Abschnittssummen
    Phase phase = Phase::Idle; ///< Zustand der Messsonde
    bool frameOpen = false; ///< Eine Bildberechnung wird erfasst
    bool framePending = false; ///< Fertiges Bild wartet auf das Hauptschleifenende
    bool havePrevious = false; ///< Voriges vollständiges Bild als Intervallbeginn vorhanden
    bool haveAudio = false; ///< Voriger Audio-Aufruf vorhanden
    bool sourcePaused = false; ///< SD-Player meldet ausdrücklich Pause
    bool sourceReady = false; ///< SD-Player ist dialogfrei mit geladenem Titel aktiv
    char sourcePath[256] = {}; ///< Nur zur Erkennung eines Titelwechsels, wird nicht ausgegeben
    char sourceFormat = '?'; ///< M für MP3, F für FLAC, Fragezeichen für andere Endungen
    uint32_t sourceRevision = 0; ///< Ändert sich bei Titel-, Pausen- oder Bereitschaftswechsel
    uint32_t capturedRevision = 0; ///< Quellzustand beim Startkommando
    uint8_t capturedStyle = 0; ///< Farbwelt beim Startkommando
    char requestedMode = 0; ///< Angeforderter Lauf P, M oder F
    char capturedFormat = '?'; ///< Dateityp beim Startkommando
    uint32_t phaseStartMs = 0; ///< Beginn der Ruhezeit oder Aufnahme
    uint32_t previousEndUs = 0; ///< Ende der vorigen Bildkopie
    uint32_t previousLoopEndUs = 0; ///< Hauptschleifenende vor dem ersten gespeicherten Intervall
    uint32_t currentLoopEndUs = 0; ///< Ende des zuletzt erfassten Hauptschleifendurchlaufs
    uint32_t lastAudioUs = 0; ///< Beginn des letzten Audio-Aufrufs
    uint16_t count = 0; ///< Anzahl gespeicherter Bilder
    uint16_t outputFrame = 0; ///< Nächster auszugebender Bilddatensatz
    int outputSection = -2; ///< Minus zwei für Bildzeiten, minus eins für Intervall, danach Abschnittsindex
    bool headerSent = false; ///< Kopf der eingefrorenen Aufnahme wurde ausgegeben
    const char *finishReason = "vollständig"; ///< Abschluss- oder Abbruchgrund der Aufnahme

    /**
     * @brief Friert vollständige Datensätze für die spätere Ausgabe ein
     */
    void finish(const char *reason)
    {
        phase = Phase::Draining;
        finishReason = reason;
        frameOpen = false;
        framePending = false;
        outputFrame = 0;
        outputSection = -2;
        headerSent = false;
    }

    /**
     * @brief Gibt nach der Aufnahme höchstens eine kurze Zeile pro Hauptschleife aus
     */
    void drain()
    {
        if (!Serial || Serial.availableForWrite() < SERIAL_RESERVE)
            return;
        if (!headerSent)
        {
            LOG_INFO_F("NORDLICHT_EINZEL: BEGIN v=1 Modus=%c Dateityp=%c Stil=%u Bilder=%u Vor_Ende_us=%u Vor_Loop_us=%u Grund=%s", requestedMode, capturedFormat, capturedStyle, count, previousEndUs, previousLoopEndUs, finishReason);
            headerSent = true;
        }
        else if (outputFrame < count)
        {
            const Frame &frame = frames[outputFrame];
            if (outputSection == -2)
            {
                LOG_INFO_F("NORDLICHT_EINZEL: B,%u,%u,%u,%u,%u,%u,%u,%u", outputFrame, frame.beginUs, frame.animationMs, frame.pushBeginUs, frame.pushEndUs, frame.computeUs, frame.slices, frame.gapUs);
            }
            else if (outputSection == -1)
            {
                LOG_INFO_F("NORDLICHT_EINZEL: L,%u,%u,%u,%u,%u", outputFrame, frame.loopEndUs, frame.interval.loops, frame.interval.loopMaximum, frame.interval.audioGapMaximum);
            }
            else
            {
                const Metric &metric = frame.interval.sections[outputSection];
                LOG_INFO_F("NORDLICHT_EINZEL: S,%u,%s,%u,%u", outputFrame, SECTION_NAMES[outputSection], metric.sum, metric.maximum);
            }
            if (++outputSection == SECTION_COUNT)
            {
                outputSection = -2;
                ++outputFrame;
            }
        }
        else
        {
            LOG_INFO_F("NORDLICHT_EINZEL: END Modus=%c Bilder=%u Grund=%s", requestedMode, count, finishReason);
            phase = Phase::Idle;
        }
    }
}

/**
 * @brief Reserviert einmalig den Messspeicher vor dem Start der Oberfläche
 */
void init()
{
    if (!frames)
        frames = static_cast<Frame *>(heap_caps_malloc(sizeof(Frame) * FRAME_COUNT, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!frames)
        LOG_WARNING("Nordlicht-Einzelbildmessung inaktiv: Messspeicher fehlt.");
}

/**
 * @brief Übernimmt den SD-Zustand, ohne Dateinamen auszugeben
 */
void setSdSource(const char *path, bool paused, bool ready)
{
    if (strcmp(sourcePath, path) != 0 || paused != sourcePaused || ready != sourceReady)
    {
        stringutils::safeCopy(sourcePath, path);
        sourcePaused = paused;
        sourceReady = ready;
        const char *extension = strrchr(path, '.');
        sourceFormat = extension && strcasecmp(extension, ".mp3") == 0 ? 'M' : extension && strcasecmp(extension, ".flac") == 0 ? 'F' : '?';
        ++sourceRevision;
    }
}

/**
 * @brief Bricht eine laufende Aufnahme bei einer Visualizer-Reinitialisierung ab
 */
void reset()
{
    if (phase == Phase::Settling || phase == Phase::Recording)
        finish("Reinitialisierung");
}

/**
 * @brief Erfasst Beginn und Bewegungsschritt eines Bildes
 */
void beginFrame(uint32_t startUs, uint32_t animationMs)
{
    if (phase != Phase::Recording)
        return;
    currentFrame = {};
    currentFrame.beginUs = startUs;
    currentFrame.animationMs = animationMs;
    frameOpen = true;
}

/**
 * @brief Summiert die Rechenabschnitte des laufenden Bildes
 */
void recordSlice(uint32_t elapsedUs, uint16_t columns)
{
    if (!frameOpen || columns == 0)
        return;
    currentFrame.computeUs += elapsedUs;
    ++currentFrame.slices;
}

/**
 * @brief Merkt ein vollständig ausgegebenes Bild bis zum Hauptschleifenende
 */
void endFrame(uint32_t pushStartUs, uint32_t endUs)
{
    if (!frameOpen)
        return;
    currentFrame.pushBeginUs = pushStartUs;
    currentFrame.pushEndUs = endUs;
    framePending = true;
    frameOpen = false;
}

/**
 * @brief Erfasst Audio-Aufrufabstände während der Aufnahme
 */
void recordAudio(uint32_t startUs)
{
    if (phase != Phase::Recording)
        return;
    const uint32_t gap = startUs - lastAudioUs;
    if (haveAudio && gap > interval.audioGapMaximum)
        interval.audioGapMaximum = gap;
    lastAudioUs = startUs;
    haveAudio = true;
}

/**
 * @brief Erfasst Abschnittslaufzeiten ohne Ausgabe während der Aufnahme
 */
void recordSection(LoopSection section, uint32_t elapsedUs)
{
    if (phase == Phase::Recording && section < LoopSection::Count)
        interval.sections[static_cast<uint8_t>(section)].add(elapsedUs);
}

/**
 * @brief Erfasst vollständige Hauptschleifendurchläufe während der Aufnahme
 */
void recordLoop(uint32_t startUs, uint32_t endUs)
{
    if (phase != Phase::Recording)
        return;
    ++interval.loops;
    const uint32_t elapsed = endUs - startUs;
    if (elapsed > interval.loopMaximum)
        interval.loopMaximum = elapsed;
    currentLoopEndUs = endUs;
}

/**
 * @brief Steuert ausdrücklich gestartete Einzelaufnahmen und ihre nachgelagerte Ausgabe
 */
void report(bool active, bool playing)
{
    if (phase == Phase::Draining)
    {
        drain();
        return;
    }
    const uint32_t now = millis();
    const bool sdActive = active && APP_STATE.getActiveModule() == AppModule::SD_PLAYER && sourceReady;
    if (phase == Phase::Idle)
    {
        if (!Serial || Serial.available() == 0)
            return;
        const int command = Serial.read();
        if (command != 'P' && command != 'M' && command != 'F')
            return;
        const bool matches = command == 'P' ? sourcePaused && !playing : !sourcePaused && playing && sourceFormat == command;
        if (!frames || !sdActive || !matches)
        {
            LOG_WARNING("NORDLICHT_EINZEL: Start abgelehnt; Messspeicher oder angeforderter SD-Zustand fehlt.");
            return;
        }
        requestedMode = static_cast<char>(command);
        capturedFormat = sourceFormat;
        capturedRevision = sourceRevision;
        capturedStyle = APP_STATE.getAuroraStyle();
        count = 0;
        interval = {};
        frameOpen = false;
        framePending = false;
        havePrevious = false;
        haveAudio = false;
        previousEndUs = 0;
        previousLoopEndUs = 0;
        phaseStartMs = now;
        phase = Phase::Settling;
        return;
    }
    if (!sdActive || sourceRevision != capturedRevision || APP_STATE.getAuroraStyle() != capturedStyle || playing == (requestedMode == 'P'))
    {
        finish("Zustandswechsel");
        return;
    }
    if (!Serial)
    {
        finish("USB getrennt");
        return;
    }
    if (phase == Phase::Settling)
    {
        if (static_cast<uint32_t>(now - phaseStartMs) >= SETTLE_MS)
        {
            phaseStartMs = now;
            phase = Phase::Recording;
        }
        return;
    }
    if (static_cast<uint32_t>(now - phaseStartMs) >= TIMEOUT_MS)
    {
        finish("Zeitlimit");
        return;
    }
    if (!framePending)
        return;
    framePending = false;
    if (havePrevious)
    {
        currentFrame.gapUs = currentFrame.pushEndUs - (count ? frames[count - 1].pushEndUs : previousEndUs);
        currentFrame.loopEndUs = currentLoopEndUs;
        currentFrame.interval = interval;
        frames[count++] = currentFrame;
    }
    else
    {
        previousEndUs = currentFrame.pushEndUs;
        previousLoopEndUs = currentLoopEndUs;
        havePrevious = true;
    }
    interval = {};
    if (count == FRAME_COUNT)
        finish("vollständig");
}
}
