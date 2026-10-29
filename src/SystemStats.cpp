#include "SystemStats.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>
#include <time.h>
#include <ESPAsyncWebServer.h>

#include "StringUtils.h"
#include "AppState.h"

// ===================================================================
// Singleton-Instanz
// ===================================================================

/**
 * @brief Liefert die Singleton-Instanz der Systemstatistik.
 */
SystemStats& SystemStats::getInstance()
{
    static SystemStats instance;
    return instance;
}

// ===================================================================
// Konstruktor
// ===================================================================

/**
 * @brief Initialisiert die Systemstatistik mit definierten Ausgangswerten.
 */
SystemStats::SystemStats()
    : m_wifiRSSI(0)
    , m_wifiQuality(0)
    , m_wifiConnected(false)
    , m_wifiReconnects(0)
    , m_ntpSynced(false)
    , m_dstActive(false)
    , m_ntpSyncCount(0)
    , m_lastNtpSyncTimestamp(0)
    , m_heapFree(0)
    , m_heapTotal(0)
    , m_psramFree(0)
    , m_psramTotal(0)
    , m_uptimeSeconds(0)
    , m_eventSource(nullptr)
    , m_lastUpdateFast(0)
    , m_initialized(false)
    , m_lastWifiQuality(0)
    , m_lastHeapFree(0)
{
    // Alle Puffer leeren
    m_wifiSSID[0] = '\0';
    m_wifiHostname[0] = '\0';
    m_wifiIP[0] = '\0';
    m_wifiMAC[0] = '\0';
    m_wifiChannel[0] = '\0';
    m_wifiSignal[0] = '\0';
    m_wifiOnline[0] = '\0';
    m_ntpServer[0] = '\0';
    m_timezoneRegion[0] = '\0';
    m_timezoneStr[0] = '\0';
    m_dstStatus[0] = '\0';
    m_dayOfYearKW[0] = '\0';
    m_lastSync[0] = '\0';
    m_syncInterval[0] = '\0';
    m_localTime[0] = '\0';
    m_uptime[0] = '\0';
    m_mcu[0] = '\0';
    m_ram[0] = '\0';
    m_software[0] = '\0';
    m_heap[0] = '\0';
    m_psramInfo[0] = '\0';
    m_firmware[0] = '\0';
    m_uart[0] = '\0';
    m_lastLocalTime[0] = '\0';
}

// ===================================================================
// Initialisierung
// ===================================================================

/**
 * @brief Initialisiert die Statistikerfassung und die Ereignisausgabe.
 */
bool SystemStats::begin(AsyncEventSource* eventSource)
{
    m_eventSource = eventSource;
    
    // Statische Werte einmalig sammeln
    updateStaticStats();
    
    // Alle anderen Werte initial sammeln
    forceUpdate();
    
    m_initialized = true;
    return true;
}

// ===================================================================
// Aktualisierungsmethoden
// ===================================================================

/**
 * @brief Aktualisiert die fälligen Laufzeitstatistiken.
 */
void SystemStats::update()
{
    if (!m_initialized) return;
    
    // Nur arbeiten, wenn mindestens ein SSE-Client verbunden ist
    if (m_eventSource == nullptr || m_eventSource->count() == 0) return;
    
    uint32_t now = millis();
    
    // Updates alle 3 Sekunden (ausreichend für Statistik-Anzeigen)
    if (now - m_lastUpdateFast >= UPDATE_INTERVAL_FAST_MS)
    {
        m_lastUpdateFast = now;
        
        updateWifiStats();
        updateTimeStats();
        updateSystemStats();
        sendSSEUpdate();
    }
}

/**
 * @brief Erzwingt eine vollständige Aktualisierung der Laufzeitstatistiken.
 */
void SystemStats::forceUpdate()
{
    updateWifiStats();
    updateTimeStats();
    updateSystemStats();
    
    m_lastUpdateFast = millis();
}

// ===================================================================
// Zähler-Methoden
// ===================================================================

/**
 * @brief Erhöht den Zähler der WLAN-Neuverbindungen.
 */
void SystemStats::incrementWifiReconnects()
{
    m_wifiReconnects++;
}

/**
 * @brief Speichert den Zeitpunkt der letzten NTP-Synchronisation.
 */
void SystemStats::setLastNtpSyncTime()
{
    // Unix-Timestamp der aktuellen Zeit speichern
    time_t now;
    time(&now);
    m_lastNtpSyncTimestamp = now;
    m_ntpSyncCount++;
    formatLastSync();
}

/**
 * @brief Formatiert den letzten NTP-Zeitabgleich für Anzeigen.
 */
void SystemStats::formatLastSync()
{
    if (m_lastNtpSyncTimestamp > 0)
    {
        struct tm syncTm;
        localtime_r(&m_lastNtpSyncTimestamp, &syncTm);
        stringutils::safeFormat(m_lastSync, "%02d.%02d.%04d %02d:%02d Uhr",
            syncTm.tm_mday,
            syncTm.tm_mon + 1,
            syncTm.tm_year + 1900,
            syncTm.tm_hour,
            syncTm.tm_min);
    }
    else
    {
        stringutils::safeCopy(m_lastSync, "---");
    }
}

// ===================================================================
// WLAN-Aktualisierungen
// ===================================================================

/**
 * @brief Aktualisiert die WLAN-Statistiken.
 */
void SystemStats::updateWifiStats()
{
    m_wifiConnected = (WiFi.status() == WL_CONNECTED);
    
    if (m_wifiConnected)
    {
        // SSID
        stringutils::safeCopy(m_wifiSSID, WiFi.SSID().c_str());
        
        // Hostname
        stringutils::safeCopy(m_wifiHostname, WiFi.getHostname());
        
        // IP-Adresse
        IPAddress ip = WiFi.localIP();
        stringutils::safeFormat(m_wifiIP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        
        // Kanal mit Frequenzband
        uint8_t channel = WiFi.channel();
        const char* band = (channel <= 14) ? "2,4GHz" : "5GHz";
        stringutils::safeFormat(m_wifiChannel, "%d (%s)", channel, band);
        
        // RSSI und Qualität
        m_wifiRSSI = WiFi.RSSI();
        m_wifiQuality = rssiToQuality(m_wifiRSSI);
        stringutils::safeFormat(m_wifiSignal, "%ddB (%d%%)", m_wifiRSSI, m_wifiQuality);
        
        // Online-Zeit mit Reconnects: "0T, 23Std, 0Min (7x)"
        formatUptimeWithReconnects(m_uptimeSeconds, m_wifiReconnects, m_wifiOnline, sizeof(m_wifiOnline));
    }
    else
    {
        stringutils::safeCopy(m_wifiSSID, "---");
        stringutils::safeCopy(m_wifiIP, "---");
        stringutils::safeCopy(m_wifiChannel, "---");
        stringutils::safeCopy(m_wifiSignal, "---");
        stringutils::safeCopy(m_wifiOnline, "---");
        m_wifiRSSI = 0;
        m_wifiQuality = 0;
    }
}

// ===================================================================
// Zeitaktualisierungen
// ===================================================================

/**
 * @brief Aktualisiert die Zeitstatistiken.
 */
void SystemStats::updateTimeStats()
{
    struct tm timeinfo;
    if (::getLocalTime(&timeinfo, 10))
    {
        // Lokalzeit formatieren
        stringutils::safeFormat(m_localTime, "%02d.%02d.%04d, %02d:%02d:%02d",
            timeinfo.tm_mday,
            timeinfo.tm_mon + 1,
            timeinfo.tm_year + 1900,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
        
        // Sommerzeit-Status erkennen
        m_dstActive = (timeinfo.tm_isdst > 0);

        // Zonenkürzel und Versatz kommen aus der eingestellten Zeitzone: "inaktiv, CET +0100" oder "aktiv, CEST +0200"
        char zoneName[8];
        char zoneOffset[8];
        strftime(zoneName, sizeof(zoneName), "%Z", &timeinfo);
        strftime(zoneOffset, sizeof(zoneOffset), "%z", &timeinfo);
        stringutils::safeFormat(m_dstStatus, "%s, %s %s", m_dstActive ? "aktiv" : "inaktiv", zoneName, zoneOffset);
        
        // Tag im Jahr und Kalenderwoche: "021.Tag im Jahr, KW04"
        int dayOfYear = timeinfo.tm_yday + 1;  // tm_yday ist 0-basiert
        int weekNum = getWeekNumber(
            timeinfo.tm_year + 1900, 
            timeinfo.tm_mon + 1, 
            timeinfo.tm_mday
        );
        stringutils::safeFormat(m_dayOfYearKW, "%03d.Tag im Jahr, KW%02d", dayOfYear, weekNum);
    }
    else
    {
        stringutils::safeCopy(m_localTime, "---");
        stringutils::safeCopy(m_dstStatus, "---");
        stringutils::safeCopy(m_dayOfYearKW, "---");
        m_dstActive = false;
    }
    
    // Uptime berechnen
    m_uptimeSeconds = millis() / 1000;
    formatUptime(m_uptimeSeconds, m_uptime, sizeof(m_uptime));
    
    // NTP-Status aus AppState
    m_ntpSynced = APP_STATE.getNtpSynced();
    
    formatLastSync();
    
    // Sync-Intervall aus AppState formatieren: "60m (Sync: 312x)"
    unsigned long intervalMs = APP_STATE.getNtpSyncInterval();
    unsigned long intervalMin = intervalMs / 60000;
    stringutils::safeFormat(m_syncInterval, "%lum (Sync: %lux)", intervalMin, (unsigned long)m_ntpSyncCount);
}

// ===================================================================
// Systemaktualisierungen
// ===================================================================

/**
 * @brief Aktualisiert die dynamischen Speicher- und Laufzeitstatistiken.
 */
void SystemStats::updateSystemStats()
{
    // Heap: "105256 Bytes frei"
    m_heapFree = ESP.getFreeHeap();
    m_heapTotal = ESP.getHeapSize();
    stringutils::safeFormat(m_heap, "%lu Bytes frei", (unsigned long)m_heapFree);
    
    // PSRAM
    if (psramFound())
    {
        m_psramFree = ESP.getFreePsram();
        m_psramTotal = ESP.getPsramSize();
        uint8_t freePercent = (m_psramFree * 100) / m_psramTotal;
        stringutils::safeFormat(m_psramInfo, "%lu Bytes frei (%d%%)", 
            (unsigned long)m_psramFree, freePercent);
    }
    else
    {
        m_psramFree = 0;
        m_psramTotal = 0;
        stringutils::safeCopy(m_psramInfo, "nicht verfügbar");
    }
}

// ===================================================================
// Statische Werte (einmalig)
// ===================================================================

/**
 * @brief Erfasst die statischen Systeminformationen.
 */
void SystemStats::updateStaticStats()
{
    // MAC-Adresse
    uint8_t mac[6];
    WiFi.macAddress(mac);
    stringutils::safeFormat(m_wifiMAC, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Hostname (initial)
    stringutils::safeCopy(m_wifiHostname, WiFi.getHostname());
    
    // MCU-Info: "ESP32-S3, 240MHz"
    stringutils::safeFormat(m_mcu, "%s, %dMHz", 
        ESP.getChipModel(), 
        ESP.getCpuFreqMHz());
    
    // RAM: "Flash: 8MB, PS: 8MB"
    uint32_t flashSize = ESP.getFlashChipSize();
    uint32_t psramSize = psramFound() ? ESP.getPsramSize() : 0;
    
    stringutils::safeFormat(m_ram, "Flash: %dMB, PS: %dMB", 
        flashSize / (1024 * 1024),
        psramSize / (1024 * 1024));
    
    // Firmware-Version
#ifdef FW_VERSION
    stringutils::safeCopy(m_firmware, FW_VERSION);
#else
    stringutils::safeCopy(m_firmware, "1.0.0");
#endif
    
    // UART-Info
    stringutils::safeFormat(m_uart, "%u bit/s", static_cast<unsigned>(Serial.baudRate()));
    
    // Firmware/Sketch-Größe: "3557808 Byte (57%)" - statisch, ändert sich nie
    size_t sketchSize = ESP.getSketchSize();
    const esp_partition_t* partition = esp_ota_get_running_partition();
    size_t partitionSize = partition ? partition->size : 0;
    
    if (partitionSize > 0)
    {
        uint8_t usedPercent = (sketchSize * 100) / partitionSize;
        stringutils::safeFormat(m_software, "%lu Byte (%d%%)", (unsigned long)sketchSize, usedPercent);
    }
    else
    {
        stringutils::safeFormat(m_software, "%lu Byte", (unsigned long)sketchSize);
    }
    
    // NTP-Server aus AppState
    if (APP_STATE.hasNtpServer())
    {
        stringutils::safeCopy(m_ntpServer, APP_STATE.getNtpServer());
    }
    else
    {
        stringutils::safeCopy(m_ntpServer, "pool.ntp.org");
    }
    
    // Zeitzone Region und POSIX-String (Zone)
    if (APP_STATE.hasTimezone())
    {
        stringutils::safeCopy(m_timezoneRegion, APP_STATE.getTimezoneRegion());
        stringutils::safeCopy(m_timezoneStr, APP_STATE.getTimezoneStr());
    }
    else
    {
        stringutils::safeCopy(m_timezoneRegion, "Europe/Berlin");
        stringutils::safeCopy(m_timezoneStr, "CET-1CEST,M3.5.0,M10.5.0/3");
    }
}

// ===================================================================
// JSON-Export
// ===================================================================

/**
 * @brief Schreibt alle Systemstatistiken als JSON in den Zielpuffer.
 */
size_t SystemStats::toJson(char* buffer, size_t bufferSize) const
{
    if (buffer == nullptr || bufferSize == 0) return 0;
    
    return stringutils::safeFormatReturn(buffer, bufferSize,
        "{"
        "\"wifi\":{"
            "\"ssid\":\"%s\","
            "\"hostname\":\"%s\","
            "\"ip\":\"%s\","
            "\"mac\":\"%s\","
            "\"channel\":\"%s\","
            "\"signal\":\"%s\","
            "\"online\":\"%s\","
            "\"rssi\":%d,"
            "\"quality\":%d,"
            "\"reconnects\":%u,"
            "\"connected\":%s"
        "},"
        "\"ntp\":{"
            "\"server\":\"%s\","
            "\"region\":\"%s\","
            "\"zone\":\"%s\","
            "\"dst\":\"%s\","
            "\"dayKW\":\"%s\","
            "\"lastSync\":\"%s\","
            "\"interval\":\"%s\","
            "\"localtime\":\"%s\","
            "\"uptime\":\"%s\","
            "\"synced\":%s,"
            "\"syncCount\":%lu"
        "},"
        "\"system\":{"
            "\"mcu\":\"%s\","
            "\"ram\":\"%s\","
            "\"software\":\"%s\","
            "\"heap\":\"%s\","
            "\"heapFree\":%lu,"
            "\"heapTotal\":%lu,"
            "\"psramFree\":%lu,"
            "\"psramTotal\":%lu,"
            "\"uptimeSeconds\":%lu,"
            "\"firmware\":\"%s\","
            "\"psram\":\"%s\","
            "\"uart\":\"%s\""
        "}"
        "}",
        // WiFi
        m_wifiSSID,
        m_wifiHostname,
        m_wifiIP,
        m_wifiMAC,
        m_wifiChannel,
        m_wifiSignal,
        m_wifiOnline,
        m_wifiRSSI,
        m_wifiQuality,
        m_wifiReconnects,
        m_wifiConnected ? "true" : "false",
        // NTP
        m_ntpServer,
        m_timezoneRegion,
        m_timezoneStr,
        m_dstStatus,
        m_dayOfYearKW,
        m_lastSync,
        m_syncInterval,
        m_localTime,
        m_uptime,
        m_ntpSynced ? "true" : "false",
        (unsigned long)m_ntpSyncCount,
        // System
        m_mcu,
        m_ram,
        m_software,
        m_heap,
        (unsigned long)m_heapFree,
        (unsigned long)m_heapTotal,
        (unsigned long)m_psramFree,
        (unsigned long)m_psramTotal,
        (unsigned long)m_uptimeSeconds,
        m_firmware,
        m_psramInfo,
        m_uart
    );
}

/**
 * @brief Schreibt geänderte Systemstatistiken als JSON in den Zielpuffer.
 */
size_t SystemStats::toJsonDelta(char* buffer, size_t bufferSize)
{
    if (buffer == nullptr || bufferSize == 0) return 0;
    
    // Delta-JSON mit allen sich häufig ändernden Werten
    return stringutils::safeFormatReturn(buffer, bufferSize,
        "{"
        "\"wifi\":{"
            "\"signal\":\"%s\","
            "\"online\":\"%s\","
            "\"quality\":%d,"
            "\"reconnects\":%u,"
            "\"connected\":%s"
        "},"
        "\"ntp\":{"
            "\"dst\":\"%s\","
            "\"dayKW\":\"%s\","
            "\"lastSync\":\"%s\","
            "\"interval\":\"%s\","
            "\"localtime\":\"%s\","
            "\"uptime\":\"%s\","
            "\"synced\":%s,"
            "\"syncCount\":%lu"
        "},"
        "\"system\":{"
            "\"software\":\"%s\","
            "\"heap\":\"%s\","
            "\"heapFree\":%lu,"
            "\"psramFree\":%lu,"
            "\"psramTotal\":%lu,"
            "\"uptimeSeconds\":%lu,"
            "\"psram\":\"%s\""
        "}"
        "}",
        m_wifiSignal,
        m_wifiOnline,
        m_wifiQuality,
        m_wifiReconnects,
        m_wifiConnected ? "true" : "false",
        m_dstStatus,
        m_dayOfYearKW,
        m_lastSync,
        m_syncInterval,
        m_localTime,
        m_uptime,
        m_ntpSynced ? "true" : "false",
        (unsigned long)m_ntpSyncCount,
        m_software,
        m_heap,
        (unsigned long)m_heapFree,
        (unsigned long)m_psramFree,
        (unsigned long)m_psramTotal,
        (unsigned long)m_uptimeSeconds,
        m_psramInfo
    );
}

// ===================================================================
// SSE-Methoden
// ===================================================================

/**
 * @brief Setzt die Ereignisquelle für Statistikaktualisierungen.
 */
void SystemStats::setEventSource(AsyncEventSource* eventSource)
{
    m_eventSource = eventSource;
}

/**
 * @brief Sendet die geänderten Statistiken als Server-Sent Event.
 */
void SystemStats::sendSSEUpdate()
{
    if (m_eventSource == nullptr) return;
    
    // Nur senden wenn sich etwas geändert hat
    bool hasChanges = (m_wifiQuality != m_lastWifiQuality) ||
                      (m_heapFree != m_lastHeapFree) ||
                      (strcmp(m_localTime, m_lastLocalTime) != 0);
    
    if (!hasChanges) return;
    
    // Delta-JSON erstellen (größerer Puffer wegen mehr Felder)
    char jsonBuffer[800];
    size_t len = toJsonDelta(jsonBuffer, sizeof(jsonBuffer));
    
    if (len > 0)
    {
        m_eventSource->send(jsonBuffer, "stats", millis());
        
        // Letzte Werte merken
        m_lastWifiQuality = m_wifiQuality;
        m_lastHeapFree = m_heapFree;
        stringutils::safeCopy(m_lastLocalTime, m_localTime);
    }
}

// ===================================================================
// Hilfsmethoden
// ===================================================================

/**
 * @brief Konvertiert einen RSSI-Wert in eine Qualitätsangabe in Prozent.
 */
uint8_t SystemStats::rssiToQuality(int8_t rssi)
{
    if (rssi <= -100) return 0;
    if (rssi >= -50) return 100;
    return 2 * (rssi + 100);
}

/**
 * @brief Formatiert eine Laufzeit als lesbaren Text.
 */
void SystemStats::formatUptime(uint32_t seconds, char* buffer, size_t bufferSize)
{
    uint32_t days = seconds / 86400;
    seconds %= 86400;
    uint32_t hours = seconds / 3600;
    seconds %= 3600;
    uint32_t minutes = seconds / 60;
    
    if (days > 0)
    {
        stringutils::safeFormat(buffer, bufferSize, "%luT, %luStd, %luMin", 
            (unsigned long)days, (unsigned long)hours, (unsigned long)minutes);
    }
    else if (hours > 0)
    {
        stringutils::safeFormat(buffer, bufferSize, "%luStd, %luMin", 
            (unsigned long)hours, (unsigned long)minutes);
    }
    else
    {
        stringutils::safeFormat(buffer, bufferSize, "%luMin", (unsigned long)minutes);
    }
}

/**
 * @brief Formatiert Laufzeit und Anzahl der WLAN-Neuverbindungen.
 */
void SystemStats::formatUptimeWithReconnects(uint32_t seconds, uint16_t reconnects, char* buffer, size_t bufferSize)
{
    uint32_t days = seconds / 86400;
    seconds %= 86400;
    uint32_t hours = seconds / 3600;
    seconds %= 3600;
    uint32_t minutes = seconds / 60;
    
    // Format: "0T, 23Std, 0Min (7x)"
    stringutils::safeFormat(buffer, bufferSize, "%luT, %luStd, %luMin (%ux)", 
        (unsigned long)days, (unsigned long)hours, (unsigned long)minutes, reconnects);
}

/**
 * @brief Berechnet die ISO-8601 Kalenderwoche
 */
int SystemStats::getWeekNumber(int year, int month, int day)
{
    // Einfache Berechnung nach ISO 8601
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    mktime(&timeinfo);
    
    // Wochentag (0 = Sonntag, 1 = Montag, ...)
    int wday = timeinfo.tm_wday;
    if (wday == 0) wday = 7;  // ISO: Sonntag = 7
    
    // Tag des Jahres
    int yday = timeinfo.tm_yday;
    
    // Berechne Kalenderwoche
    int week = (yday - wday + 10) / 7;
    
    // Korrektur für Jahreswechsel
    if (week < 1)
    {
        // Letzte Woche des Vorjahres
        week = getWeekNumber(year - 1, 12, 31);
    }
    else if (week > 52)
    {
        // Prüfe ob es Woche 1 des nächsten Jahres ist
        struct tm jan1 = {0};
        jan1.tm_year = year - 1900 + 1;
        jan1.tm_mon = 0;
        jan1.tm_mday = 1;
        mktime(&jan1);
        int jan1Wday = jan1.tm_wday;
        if (jan1Wday == 0) jan1Wday = 7;
        
        if (jan1Wday <= 4)
        {
            week = 1;
        }
    }
    
    return week;
}
