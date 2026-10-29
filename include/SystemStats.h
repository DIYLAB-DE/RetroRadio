#ifndef SYSTEM_STATS_H
#define SYSTEM_STATS_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>

// Vorwärtsdeklarationen
class AsyncEventSource;

/**
 * @brief Zentrale Klasse für Systemstatistiken (Singleton)
 * 
 * Sammelt und verwaltet alle Systemdaten für Display und Webinterface:
 * - WiFi-Informationen (SSID, IP, Signal, Online-Zeit, Reconnects)
 * - NTP/Zeit-Informationen (Server, Region, Zone, DST, KW, Sync-Status)
 * - Hardware-Informationen (MCU, RAM, Software, Heap, Firmware, UART)
 * 
 * Die Klasse verwendet feste Puffer statt String-Objekte für optimale Heap-Stabilität auf ESP32.
 */
class SystemStats
{
public:
    // ========================================================================
    // Puffergrößen
    // ========================================================================
    
    static constexpr size_t SSID_SIZE      = 33;   ///< Max. SSID-Länge + Nullterminator
    static constexpr size_t HOSTNAME_SIZE  = 33;   ///< Max. Hostname-Länge
    static constexpr size_t IP_SIZE        = 16;   ///< IPv4: "255.255.255.255"
    static constexpr size_t MAC_SIZE       = 18;   ///< MAC: "XX:XX:XX:XX:XX:XX"
    static constexpr size_t CHANNEL_SIZE   = 12;   ///< Kanal: "11 (2,4GHz)"
    static constexpr size_t SIGNAL_SIZE    = 16;   ///< Signal: "-64dB (72%)"
    static constexpr size_t TIME_SIZE      = 24;   ///< Zeit: "27.01.2026, 08:28:45"
    static constexpr size_t UPTIME_SIZE    = 40;   ///< Uptime: "5T, 12Std, 30Min (7x)"
    static constexpr size_t SERVER_SIZE    = 64;   ///< NTP-Server URL
    static constexpr size_t TIMEZONE_SIZE  = 48;   ///< Zeitzone: "CET-1CEST,M3.5.0/3"
    static constexpr size_t DST_SIZE       = 24;   ///< DST: "inaktiv, CET +0100"
    static constexpr size_t DAYWEEK_SIZE   = 32;   ///< "021.Tag im Jahr, KW04"
    static constexpr size_t LASTSYNC_SIZE  = 24;   ///< "27.01.2026 08:20 Uhr"
    static constexpr size_t INTERVAL_SIZE  = 24;   ///< "60m (Sync: 312x)"
    static constexpr size_t MCU_SIZE       = 32;   ///< MCU: "ESP32-S3, 240MHz"
    static constexpr size_t MEMORY_SIZE    = 32;   ///< Speicher: "Flash: 8MB, PS: 8MB"
    static constexpr size_t SOFTWARE_SIZE  = 32;   ///< "1355136 Byte (43%)"
    static constexpr size_t HEAP_SIZE      = 24;   ///< "105256 Bytes frei"
    static constexpr size_t VERSION_SIZE   = 32;   ///< "v24.8.1"
    static constexpr size_t UART_SIZE      = 24;   ///< "115200 bit/s"

    // ========================================================================
    // Singleton-Zugriff
    // ========================================================================

    static SystemStats& getInstance();

    // Kopieren/Verschieben verhindern
    SystemStats(const SystemStats&) = delete;
    SystemStats& operator=(const SystemStats&) = delete;

    // ========================================================================
    // Initialisierung
    // ========================================================================

    bool begin(AsyncEventSource* eventSource = nullptr);

    // ========================================================================
    // Update-Methoden
    // ========================================================================

    void update();
    void forceUpdate();

    // ========================================================================
    // WiFi-Getter
    // ========================================================================

    const char* getWifiSSID() const      { return m_wifiSSID; }
    const char* getWifiHostname() const  { return m_wifiHostname; }
    const char* getWifiIP() const        { return m_wifiIP; }
    const char* getWifiMAC() const       { return m_wifiMAC; }
    const char* getWifiChannel() const   { return m_wifiChannel; }
    const char* getWifiSignal() const    { return m_wifiSignal; }
    const char* getWifiOnline() const    { return m_wifiOnline; }  ///< Online-Zeit mit Reconnects
    int8_t      getWifiRSSI() const      { return m_wifiRSSI; }
    uint8_t     getWifiQuality() const   { return m_wifiQuality; }
    bool        isWifiConnected() const  { return m_wifiConnected; }
    uint16_t    getWifiReconnects() const { return m_wifiReconnects; }

    // ========================================================================
    // NTP/Zeit-Getter
    // ========================================================================

    const char* getNtpServer() const     { return m_ntpServer; }
    const char* getTimezoneRegion() const { return m_timezoneRegion; }  ///< Region (Europe/Berlin)
    const char* getTimezoneStr() const   { return m_timezoneStr; }      ///< POSIX-String (Zone)
    const char* getDstStatus() const     { return m_dstStatus; }        ///< "inaktiv, CET +0100"
    const char* getDayOfYearKW() const   { return m_dayOfYearKW; }      ///< "021.Tag im Jahr, KW04"
    const char* getLastSync() const      { return m_lastSync; }         ///< "27.01.2026 08:20 Uhr"
    const char* getSyncInterval() const  { return m_syncInterval; }     ///< "60m (Sync: 312x)"
    const char* getLocalTime() const     { return m_localTime; }
    const char* getUptime() const        { return m_uptime; }
    bool        isNtpSynced() const      { return m_ntpSynced; }
    bool        isDstActive() const      { return m_dstActive; }
    uint32_t    getNtpSyncCount() const  { return m_ntpSyncCount; }

    // ========================================================================
    // System-Getter
    // ========================================================================

    const char* getMCU() const           { return m_mcu; }
    const char* getRAM() const           { return m_ram; }             ///< "Flash: 8MB, PS: 8MB"
    const char* getSoftware() const      { return m_software; }        ///< "3557808 Byte (57%)"
    const char* getHeap() const          { return m_heap; }            ///< "105256 Bytes frei"
    const char* getFirmware() const      { return m_firmware; }
    const char* getPsramInfo() const     { return m_psramInfo; }       ///< "6543210 Bytes frei (78%)"
    const char* getUART() const          { return m_uart; }            ///< "115200 bit/s"
    
    uint32_t    getHeapFree() const      { return m_heapFree; }
    uint32_t    getHeapTotal() const     { return m_heapTotal; }
    uint32_t    getPsramFree() const     { return m_psramFree; }
    uint32_t    getPsramTotal() const    { return m_psramTotal; }

    // ========================================================================
    // Zähler-Methoden (extern aufzurufen)
    // ========================================================================

    void incrementWifiReconnects();      ///< WiFi-Reconnect zählen
    void setLastNtpSyncTime();           ///< Letzte Sync-Zeit setzen und Zähler erhöhen

    // ========================================================================
    // JSON-Export
    // ========================================================================

    size_t toJson(char* buffer, size_t bufferSize) const;
    size_t toJsonDelta(char* buffer, size_t bufferSize);

    // ========================================================================
    // SSE-Steuerung
    // ========================================================================

    void setEventSource(AsyncEventSource* eventSource);
    void sendSSEUpdate();

private:
    // ========================================================================
    // Konstruktor (privat für Singleton)
    // ========================================================================
    SystemStats();
    ~SystemStats() = default;

    // ========================================================================
    // Interne Update-Methoden
    // ========================================================================
    
    void updateWifiStats();
    void updateTimeStats();
    void updateSystemStats();
    void updateStaticStats();
    void formatLastSync();                    ///< Letzte Sync-Zeit formatieren

    // ========================================================================
    // Hilfsmethoden
    // ========================================================================

    static uint8_t rssiToQuality(int8_t rssi);
    static void formatUptime(uint32_t seconds, char* buffer, size_t bufferSize);
    static void formatUptimeWithReconnects(uint32_t seconds, uint16_t reconnects, char* buffer, size_t bufferSize);
    static int getWeekNumber(int year, int month, int day);

    // ========================================================================
    // Datenpuffer - WiFi
    // ========================================================================
    
    char m_wifiSSID[SSID_SIZE];
    char m_wifiHostname[HOSTNAME_SIZE];
    char m_wifiIP[IP_SIZE];
    char m_wifiMAC[MAC_SIZE];
    char m_wifiChannel[CHANNEL_SIZE];
    char m_wifiSignal[SIGNAL_SIZE];
    char m_wifiOnline[UPTIME_SIZE];           ///< Online-Zeit mit Reconnects
    int8_t m_wifiRSSI;
    uint8_t m_wifiQuality;
    bool m_wifiConnected;
    uint16_t m_wifiReconnects;                ///< Anzahl WiFi-Reconnects

    // ========================================================================
    // Datenpuffer - NTP/Zeit
    // ========================================================================
    
    char m_ntpServer[SERVER_SIZE];
    char m_timezoneRegion[TIMEZONE_SIZE];     ///< Region (Europe/Berlin)
    char m_timezoneStr[TIMEZONE_SIZE];        ///< POSIX-String (Zone)
    char m_dstStatus[DST_SIZE];               ///< "inaktiv, CET +0100"
    char m_dayOfYearKW[DAYWEEK_SIZE];         ///< "021.Tag im Jahr, KW04"
    char m_lastSync[LASTSYNC_SIZE];           ///< "27.01.2026 08:20 Uhr"
    char m_syncInterval[INTERVAL_SIZE];       ///< "60m (Sync: 312x)"
    char m_localTime[TIME_SIZE];
    char m_uptime[UPTIME_SIZE];
    bool m_ntpSynced;
    bool m_dstActive;
    uint32_t m_ntpSyncCount;                  ///< Anzahl NTP-Syncs
    time_t m_lastNtpSyncTimestamp;            ///< Unix-Timestamp der letzten Sync

    // ========================================================================
    // Datenpuffer - System
    // ========================================================================
    
    char m_mcu[MCU_SIZE];
    char m_ram[MEMORY_SIZE];                  ///< "Flash: 8MB, PS: 8MB"
    char m_software[SOFTWARE_SIZE];           ///< "3557808 Byte (57%)"
    char m_heap[HEAP_SIZE];                   ///< "105256 Bytes frei"
    char m_psramInfo[MEMORY_SIZE];            ///< "6543210 Bytes frei (78%)"
    char m_firmware[VERSION_SIZE];
    char m_uart[UART_SIZE];                   ///< "115200 bit/s"
    
    uint32_t m_heapFree;
    uint32_t m_heapTotal;
    uint32_t m_psramFree;
    uint32_t m_psramTotal;
    uint32_t m_uptimeSeconds;

    // ========================================================================
    // Interne Steuerung
    // ========================================================================
    
    AsyncEventSource* m_eventSource;
    uint32_t m_lastUpdateFast;
    bool m_initialized;

    // Change-Tracking für Delta-Updates
    uint8_t m_lastWifiQuality;
    uint32_t m_lastHeapFree;
    char m_lastLocalTime[TIME_SIZE];

    // ========================================================================
    // Konstanten
    // ========================================================================
    
    static constexpr uint32_t UPDATE_INTERVAL_FAST_MS = 3000;   ///< Nur wenn Client verbunden
};

#define SYSTEM_STATS SystemStats::getInstance()

#endif // SYSTEM_STATS_H
