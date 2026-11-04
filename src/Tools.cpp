#include "Tools.h"

#include <Arduino.h>

#include "Logger.h"
#include "SD.h"

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
