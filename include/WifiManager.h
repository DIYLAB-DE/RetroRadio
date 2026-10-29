#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <stdint.h>

#include <atomic>
#include <vector>

/**
 * @class WifiManager
 * @brief Verwaltet die WiFi-Verbindung eines ESP32 mit Multi-Profil-Unterstützung.
 *
 * Diese Klasse basiert auf einer stabilen Architektur (v2) mit gezielten Verbesserungen. Sie wurde für eine flackerarme Logik optimiert, indem harte ON/OFF-Zyklen des Funkmoduls vermieden werden. Die Kernfunktionalität umfasst asynchrones Scannen, um die Benutzeroberfläche nicht zu blockieren, einen Direkt-Verbindungsversuch mit BSSID/Kanal und einen robusten Fallback-Mechanismus, falls dieser fehlschlägt.
 *
 * Optimierungen v3:
 * - APInfo verwendet char[33] statt String (keine Heap-Fragmentierung)
 * - Cache wird wiederverwendet statt clear()/push_back() (stabiler Speicher)
 * - Instanz-Member statt static für RSSI-Timer (Multi-Instanz-sicher)
 */
class WifiManager {
   public:
    // Zeitparameter (anpassbar)
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 8000;   ///< Timeout pro Verbindungsversuch (ms)
    static constexpr uint32_t RECONNECT_INTERVAL_MS = 20000;  ///< Abstand zwischen Reconnect-Versuchen (ms)
    static constexpr uint32_t RETRY_DELAY_MS = 2500;          ///< Pause bei "Rotation" (ms)

    static constexpr uint32_t MIN_SCAN_INTERVAL_MS = 30000;    ///< Untergrenze des Scan-Intervalls (ms)
    static constexpr uint32_t MAX_SCAN_INTERVAL_MS = 300000;   ///< Obergrenze (ms, 5 min)
    static constexpr uint32_t SCAN_RUNNING_TIMEOUT_MS = 7000;  ///< Safety-Timeout für laufenden Scan (ms)

    static constexpr uint8_t MAX_AP_CACHE = 24;  ///< Max. Anzahl gecachter APs

    /**
     * @brief Definiert die möglichen Zustände der Verbindungs-State-Machine.
     */
    enum class ConnectionState {
        IDLE,          ///< Manager ist inaktiv.
        CONNECTING,    ///< Ein Verbindungsaufbau läuft gerade.
        CONNECTED,     ///< Erfolgreich mit einem AP verbunden.
        DISCONNECTED,  ///< Nicht verbunden, wartet auf den nächsten Wiederverbindungsversuch.
        ROTATING       ///< Interner Zustand, um zum nächsten Profil zu wechseln.
    };

    /**
     * @brief Standardkonstruktor.
     * Lässt das WLAN-Modul initial ausgeschaltet. Die Aktivierung erfolgt erst in begin().
     */
    WifiManager();

    /**
     * @brief Initialisiert den WiFi-Stack und startet den Manager.
     * @details Diese Methode muss einmalig beim Programmstart aufgerufen werden. Sie konfiguriert den ESP32 als WLAN-Station, setzt den regulatorischen Ländercode und startet die erste asynchrone Netzwerksuche.
     * @param countryCode ISO 3166-1 alpha-2 Ländercode (z.B. "DE", "US", "JP").
     * @return true, wenn die Initialisierung gestartet wurde, false bei fehlenden Profilen.
     */
    bool begin(const char* countryCode = "DE");

    /**
     * @brief Führt die zyklische Abarbeitung der WiFi-Logik aus.
     * @details Diese nicht blockierende Methode muss regelmäßig aus der Hauptschleife loop() aufgerufen werden. Sie steuert den Zustandsautomaten, prüft Suchergebnisse und stößt bei Bedarf Wiederverbindungsversuche an.
     */
    void update();

    /**
     * @brief Trennt die aktuelle WiFi-Verbindung "weich".
     * @details Das Funkmodul bleibt eingeschaltet. Die Verbindung wird getrennt und der Manager in den Zustand IDLE versetzt.
     */
    void disconnect();

    /**
     * @brief Prüft, ob eine aktive Verbindung zum Access Point besteht.
     * @return true, wenn der Status WL_CONNECTED ist, andernfalls false.
     */
    bool isConnected() const { return m_connected.load(); }

    /**
     * @brief Gibt die aktuell zugewiesene lokale IP-Adresse zurück.
     * @return Ein IPAddress-Objekt. Ist keine Verbindung vorhanden, ist die IP 0.0.0.0.
     */
    IPAddress getIP() const { return WiFi.localIP(); }

    /**
     * @brief Gibt die SSID des aktuell verbundenen Netzwerks zurück.
     * @return Ein String mit der SSID. Ist nicht verbunden, ist der String leer.
     */
    String getCurrentSSID() const { return WiFi.SSID(); }

    /**
     * @brief Gibt die aktuelle Signalstärke (RSSI) in dBm zurück.
     * @return Der RSSI-Wert als int32_t. Typische Werte sind negativ (z.B. -55 dBm).
     */
    int32_t getRSSI() const { return WiFi.RSSI(); }

    /**
     * @brief Gibt den aktuellen internen Zustand des Managers zurück.
     * @return Der aktuelle Wert aus dem ConnectionState Enum.
     */
    ConnectionState getState() const { return m_state; }

    /**
     * @brief Gibt den Index des WiFi-Profils zurück, das aktuell verwendet wird.
     * @return Der Profilindex (0-2) oder -1, wenn kein Profil aktiv ist.
     */
    int8_t getCurrentProfileIndex() const { return m_currentProfile; }

    /**
     * @brief Konvertiert einen RSSI-Wert (dBm) in eine Qualitätsangabe in Prozent.
     * @param rssi Der RSSI-Wert in dBm.
     * @return Die Qualität von 0 bis 100.
     */
    static uint8_t rssiToQuality(int32_t rssi);

    /**
     * @brief Erlaubt oder verbietet Hintergrund-Scans, während eine Verbindung besteht.
     * @details Die Option ist standardmäßig deaktiviert und kann für manuelle Roaming-Szenarien aktiviert werden.
     * @param enable true, um Scans im verbundenen Zustand zu erlauben, sonst false.
     */
    void setScanWhileConnected(bool enable) { m_scanWhileConnected = enable; }

    /**
     * @brief Pausiert alle Scan-Aktivitäten vollständig.
     * Nützlich für einen "Offline-Modus".
     */
    void suspendScanning() { m_scanningSuspended = true; }

    /**
     * @brief Setzt die Scan-Aktivitäten fort, die mit suspendScanning() pausiert wurden.
     */
    void resumeScanning() {
        m_scanningSuspended = false;
        m_scanIntervalMs = MIN_SCAN_INTERVAL_MS;
    }

   private:
    /**
     * @brief Interne Datenstruktur zur Speicherung von Access-Point-Informationen.
     * @details Verwendet char-Array statt String um Heap-Fragmentierung zu vermeiden.
     */
    struct APInfo {
        char ssid[33]{};      ///< SSID als fixes char-Array (max 32 + '\0')
        uint8_t bssid[6]{};   ///< MAC-Adresse des AP
        int32_t channel = 0;  ///< WiFi-Kanal
        int32_t rssi = -127;  ///< Signalstärke in dBm
    };

    // Zustände/Flags
    std::atomic<bool> m_connected{false};
    ConnectionState m_state = ConnectionState::IDLE;

    // Profilverwaltung
    int8_t m_currentProfile = -1;
    uint8_t m_profileCount = 0;

    // Zeitmarken
    uint32_t m_connectStartTime = 0;
    uint32_t m_lastReconnectAttempt = 0;
    uint32_t m_rotateDelayStart = 0;
    uint32_t m_lastRssiUpdate = 0;  ///< Timer für RSSI-Aktualisierung (war static)

    // Debounce für Connection-Status (asymmetrisch)
    uint32_t m_lastStatusChangeTime = 0;
    bool m_pendingConnected = false;
    static constexpr uint32_t CONNECTED_DEBOUNCE_MS = 500;
    static constexpr uint32_t DISCONNECTED_DEBOUNCE_MS = 100;

    // Scan-Status und Cache
    std::vector<APInfo> m_apCache;  ///< Vorab-allozierter Cache (wird wiederverwendet)
    uint8_t m_apCount = 0;          ///< Anzahl gültiger Einträge im Cache
    bool m_scanInProgress = false;
    uint32_t m_scanStartTime = 0;
    uint32_t m_lastScanTime = 0;
    uint32_t m_scanIntervalMs = MIN_SCAN_INTERVAL_MS;
    bool m_scanWhileConnected = false;
    bool m_scanningSuspended = false;
    bool m_lastScanWasPassive = true;  ///< Merker für intelligenten passiv/aktiv-Scan

    // Fallback-Marker für generischen Connect (Kern der V2-Logik)
    int8_t m_lastTriedProfile = -1;
    bool m_genericFallbackTried = false;

    // Private Methoden
    bool connectToBestVisibleProfile();
    void updateConnectionStatus();
    void startAsyncScanIfAllowed(bool force = false);
    void pollScanResults();
    const APInfo* findAPForSSID(const char* ssid) const;
    void relaxScanInterval();
    void tightenScanInterval();
    void setCountryCode(const char* countryCode);
};

#endif  // WIFI_MANAGER_H
