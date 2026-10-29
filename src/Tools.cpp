#include "Tools.h"

#include <Arduino.h>
#include <WiFi.h>

#include "AppState.h"
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

// ===================================================================
// FPS und Laufzeit-Metriken
// ===================================================================

/**
 * @brief Gibt FPS, Heap-/PSRAM-Metriken und WiFi-Signalstärke periodisch aus
 */
volatile uint32_t g_visDataCount = 0;
volatile uint32_t g_audioCbCount = 0;

void fps(uint32_t seconds)
{
    static uint32_t lastMillis;
    static uint32_t frameCount;
    uint32_t now = millis();
    frameCount++;

    if (now - lastMillis < seconds * 1000)
        return;

    uint32_t framesPerSecond = frameCount / seconds;

    float heapFreeKb = ESP.getFreeHeap() / 1024.0f;
    float heapMaxBlockKb = ESP.getMaxAllocHeap() / 1024.0f;
    float heapMinFreeKb = ESP.getMinFreeHeap() / 1024.0f;

    float psramFreeMb = ESP.getFreePsram() / (1024.0f * 1024.0f);
    float psramMaxBlockMb = ESP.getMaxAllocPsram() / (1024.0f * 1024.0f);

    int8_t rssi = WiFi.RSSI();

    LOG_LINE_LABEL_COLOR("FPS/RAM/PSRAM", Logger::CYAN, 80);
    LOG_INFO_F("FPS: %u | RSSI: %i dB", framesPerSecond, rssi);
    // Aktive Visualisierung samt Stilvariante, damit jede Messzeile ihren Kontext trägt
    static const char *const visNames[] = {"Analog-VU", "Digital-VU", "Spektrum", "Wellenform", "Goniometer", "Mittiges Spektrum", "Wasserfall", "Nordlicht"};
    const uint8_t visMode = APP_STATE.getVisMode();
    const char *visName = (visMode < sizeof(visNames) / sizeof(visNames[0])) ? visNames[visMode] : "?";
    const char *visStyle = "";
    switch (visMode)
    {
    case 1:
        visStyle = APP_STATE.getDigitalVUStyle() ? " (Regenbogen)" : " (klassisch)";
        break;
    case 2:
        visStyle = APP_STATE.getSpectrumStyle() ? " (Regenbogen)" : " (klassisch)";
        break;
    case 3:
        visStyle = APP_STATE.getWaveformStyle() ? " (gestapelt)" : " (überlagert)";
        break;
    case 4:
        visStyle = APP_STATE.getGonioStyle() ? " (Regenbogen)" : " (klassisch)";
        break;
    case 5:
        visStyle = APP_STATE.getCenteredSpectrumStyle() ? " (Regenbogen)" : " (grün)";
        break;
    case 6:
        visStyle = APP_STATE.getWaterfallStyle() ? " (mehrfarbig)" : " (grün)";
        break;
    case 7:
        visStyle = APP_STATE.getAuroraStyle() ? " (warm)" : " (kühl)";
        break;
    }

    LOG_INFO_F("Visualizer-Daten: %u/s | Audio-Callback: %u/s | Modus: %s%s", static_cast<unsigned>(g_visDataCount / seconds), static_cast<unsigned>(g_audioCbCount / seconds), visName, visStyle);
    LOG_INFO_F("Heap: %.1f KB frei | Max Block: %.1f KB | Watermark: %.1f KB", heapFreeKb, heapMaxBlockKb, heapMinFreeKb);
    LOG_INFO_F("PSRAM: %.2f MB frei | Max Block: %.2f MB", psramFreeMb, psramMaxBlockMb);

    frameCount = 0;
    g_visDataCount = 0;
    g_audioCbCount = 0;
    lastMillis = now;
}
