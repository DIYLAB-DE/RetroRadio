#include "WifiManager.h"

#include <ESPmDNS.h>
#include <esp_wifi.h>

#include <algorithm>
#include <cstring>

#include "AppState.h"
#include "Logger.h"

/**
 * @brief Konstruktor: Schaltet das Funkmodul initial aus.
 */
WifiManager::WifiManager()
{
    // Sicherstellen, dass WiFi initial aus ist und wir die Kontrolle haben.
    WiFi.mode(WIFI_OFF);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(false); // Reconnects werden manuell gesteuert.
}

/**
 * @brief Initialisiert den WiFi-Stack und startet den Manager.
 */
bool WifiManager::begin(const char *countryCode)
{
    m_profileCount = APP_STATE.getActiveWiFiProfileCount();
    if (m_profileCount == 0)
    {
        LOG_WARNING("Keine WiFi-Profile konfiguriert");
        return false;
    }

    LOG_INFO_F("Starte WiFi mit %d Profil(en)", m_profileCount);

    // Grundkonfiguration (nur einmalig durchführen)
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(false);

    // Setzt den Ländercode für korrekte Kanalverwendung (wichtig für 5GHz und Kanäle > 11)
    setCountryCode(countryCode);

    // Konfiguration für das Scannen von Netzwerken
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
    WiFi.setHostname(APP_STATE.getHostname());

    delay(100); // Kurze Pause, damit sich das RF-Subsystem stabilisieren kann.

    // AP-Cache vorab allozieren (verhindert Fragmentierung durch clear()/push_back())
    m_apCache.resize(MAX_AP_CACHE);
    m_apCount = 0;

    // Ein initiales Profil für Anzeigezwecke auswählen
    for (uint8_t i = 0; i < 3; ++i)
    {
        if (APP_STATE.isWiFiProfileEnabled(i))
        {
            m_currentProfile = i;
            break;
        }
    }

    // Den ersten asynchronen Scan anstoßen
    m_scanIntervalMs = MIN_SCAN_INTERVAL_MS;
    startAsyncScanIfAllowed(true);

    // Startzustand setzen
    m_state = ConnectionState::DISCONNECTED;
    m_lastReconnectAttempt = millis();
    return true;
}

/**
 * @brief Führt die zyklische Abarbeitung der WiFi-Logik aus.
 */
void WifiManager::update()
{
    // 1. Prüfen, ob ein laufender Scan abgeschlossen ist
    pollScanResults();

    // 2. Den Verbindungsstatus (CONNECTED/DISCONNECTED) aktualisieren
    updateConnectionStatus();

    // 3. Die Haupt-State-Machine abarbeiten
    switch (m_state)
    {
    case ConnectionState::CONNECTING:
    {
        // Prüfen, ob der Verbindungsversuch zu lange dauert (Timeout)
        if (millis() - m_connectStartTime > CONNECTION_TIMEOUT_MS)
        {
            // Wenn der Direkt-Connect (mit BSSID) scheitert, versuchen wir es einmal generisch.
            if (!m_genericFallbackTried && m_currentProfile >= 0)
            {
                char ssid[65], pwd[65];
                if (APP_STATE.getWiFiProfile(m_currentProfile, ssid, sizeof(ssid), pwd, sizeof(pwd)))
                {
                    LOG_WARNING_F("Timeout bei Profil %d – Fallback ohne BSSID/Channel", m_currentProfile);
                    WiFi.disconnect(false);
                    yield();               // Anderen Tasks Zeit geben
                    delay(3);              // Minimale Pause für WiFi-Stack
                    WiFi.begin(ssid, pwd); // Generischer Connect: Der Stack sucht selbst.
                    m_genericFallbackTried = true;
                    m_connectStartTime = millis(); // Timeout-Timer für den Fallback neu starten
                    break;                         // In diesem Zyklus beenden und auf den Fallback warten.
                }
            }
            // Wenn auch der Fallback scheitert, geben wir für dieses Profil auf.
            LOG_WARNING_F("Timeout bei Profil %d endgültig fehlgeschlagen", m_currentProfile);
            m_state = ConnectionState::ROTATING;
            m_rotateDelayStart = millis();
        }
        break;
    }
    case ConnectionState::ROTATING:
    {
        // Nach einer kurzen Pause den nächsten Verbindungsversuch starten.
        if (millis() - m_rotateDelayStart > RETRY_DELAY_MS)
        {
            if (!connectToBestVisibleProfile())
            {
                // Wenn kein bekanntes Netz sichtbar ist, Scan-Intervall vergrößern.
                relaxScanInterval();
                m_state = ConnectionState::DISCONNECTED;
                m_lastReconnectAttempt = millis();
                startAsyncScanIfAllowed();
            }
        }
        break;
    }
    case ConnectionState::CONNECTED:
    {
        // Überwachen, ob die Verbindung noch besteht.
        if (!m_connected)
        {
            LOG_WARNING("WiFi-Verbindung verloren!");
            m_state = ConnectionState::DISCONNECTED;
            m_lastReconnectAttempt = millis();
            // Bei Verlust sofort wieder reaktiver scannen.
            m_scanIntervalMs = MIN_SCAN_INTERVAL_MS;
            startAsyncScanIfAllowed(true);
            break;
        }
        // Zyklisch die Signalstärke aktualisieren (Member statt static)
        uint32_t now = millis();
        if (now - m_lastRssiUpdate > 1500)
        {
            int32_t rssi = WiFi.RSSI();
            uint8_t q = rssiToQuality(rssi);
            if (q != APP_STATE.getWifiSignalPercent())
            {
                APP_STATE.setWifiSignalPercent(q);
                APP_STATE.setWifiSignalStrength(q / 10);
            }
            m_lastRssiUpdate = now;
        }
        break;
    }
    case ConnectionState::DISCONNECTED:
    {
        uint32_t now = millis();
        // Nur in festgelegten Intervallen einen neuen Versuch starten.
        if ((now - m_lastReconnectAttempt) > RECONNECT_INTERVAL_MS)
        {
            // Nur verbinden, wenn wir durch einen Scan aktuelle Daten haben.
            if (m_apCount > 0)
            {
                if (connectToBestVisibleProfile())
                {
                    // Bei Erfolg das Scan-Intervall wieder verkürzen.
                    tightenScanInterval();
                }
                else
                {
                    // Sonst weiter langsam scannen.
                    relaxScanInterval();
                }
            }
            m_lastReconnectAttempt = now;
        }
        break;
    }
    case ConnectionState::IDLE:
    default:
        // Nichts zu tun.
        break;
    }
}

/**
 * @brief Trennt die aktuelle WiFi-Verbindung "weich".
 */
void WifiManager::disconnect()
{
    LOG_INFO("Trenne WiFi-Verbindung");
    // false = Funk anlassen, nur Verbindung trennen
    WiFi.disconnect(false);
    m_connected.store(false);
    m_state = ConnectionState::IDLE;
    MDNS.end(); // mDNS-Dienst beenden
}

/**
 * @brief Konvertiert RSSI (dBm) in Qualitäts-Prozent.
 */
uint8_t WifiManager::rssiToQuality(int32_t rssi)
{
    if (rssi <= -100)
        return 0;
    if (rssi >= -50)
        return 100;
    return static_cast<uint8_t>(2 * (rssi + 100));
}

/**
 * @brief Verbindet mit dem besten sichtbaren, konfigurierten WiFi-Profil.
 */
bool WifiManager::connectToBestVisibleProfile()
{
    if (m_apCount == 0)
    {
        return false; // Keine Scandaten, kein Versuch.
    }

    int8_t bestIdx = -1;
    const APInfo *bestAp = nullptr;
    int bestRssi = -128; // Sehr schlechter Startwert

    // Alle aktivierten Profile durchgehen
    for (uint8_t i = 0; i < 3; ++i)
    {
        if (!APP_STATE.isWiFiProfileEnabled(i))
            continue;
        char ssid[65], pwd[65];
        if (!APP_STATE.getWiFiProfile(i, ssid, sizeof(ssid), pwd, sizeof(pwd)))
            continue;

        // Im Cache nach dem besten AP für diese SSID suchen
        const APInfo *ap = findAPForSSID(ssid);
        if (ap && ap->rssi > bestRssi)
        {
            bestRssi = ap->rssi;
            bestAp = ap;
            bestIdx = i;
        }
    }

    if (bestIdx < 0 || !bestAp)
    {
        LOG_INFO("Keines der aktivierten Profile ist sichtbar");
        return false;
    }

    // Falls noch eine alte Verbindung besteht, diese sauber trennen.
    if (WiFi.status() != WL_DISCONNECTED)
    {
        WiFi.disconnect(false);
        yield();  // Anderen Tasks Zeit geben
        delay(3); // Minimale Pause für WiFi-Stack
    }

    char ssid[65], pwd[65];
    APP_STATE.getWiFiProfile(bestIdx, ssid, sizeof(ssid), pwd, sizeof(pwd));

    LOG_INFO_F("Verbinde (beste Sichtbarkeit) Profil %d: %s (ch=%d, rssi=%d)",
               bestIdx, ssid, static_cast<int>(bestAp->channel), static_cast<int>(bestAp->rssi));

    // Der schnelle Direkt-Connect mit allen bekannten Daten
    WiFi.begin(ssid, pwd, bestAp->channel, bestAp->bssid, true);

    // Zustand für den Verbindungsversuch setzen
    m_currentProfile = bestIdx;
    m_lastTriedProfile = bestIdx;
    m_genericFallbackTried = false; // Fallback-Logik für diesen Versuch zurücksetzen
    m_connectStartTime = millis();
    m_state = ConnectionState::CONNECTING;
    return true;
}

/**
 * @brief Aktualisiert den Verbindungsstatus mit asymmetrischem Debouncing.
 */
void WifiManager::updateConnectionStatus()
{
    bool currentStatus = (WiFi.status() == WL_CONNECTED);
    bool wasConnected = m_connected.load();
    uint32_t now = millis();

    // Nur bei einer Statusänderung aktiv werden
    if (currentStatus != wasConnected)
    {
        // Beim ersten Erkennen der Änderung, den Zustand nur puffern.
        if (m_pendingConnected != currentStatus)
        {
            m_pendingConnected = currentStatus;
            m_lastStatusChangeTime = now;
            return;
        }

        // Debounce-Zeit je nach Richtung der Änderung wählen (schnell bei Disconnect)
        uint32_t debounceTime = currentStatus ? CONNECTED_DEBOUNCE_MS : DISCONNECTED_DEBOUNCE_MS;

        // Wenn der Zustand für die Debounce-Zeit stabil war, die Änderung übernehmen.
        if (now - m_lastStatusChangeTime >= debounceTime)
        {
            m_connected.store(currentStatus);
            LOG_INFO_F("WiFi-Status-Änderung: %s", currentStatus ? "CONNECTED" : "DISCONNECTED");

            if (currentStatus)
            {
                // Wenn wir im CONNECTING-Zustand waren, ist der Übergang zu CONNECTED erfolgreich.
                if (m_state == ConnectionState::CONNECTING)
                {
                    m_state = ConnectionState::CONNECTED;
                    IPAddress ip = WiFi.localIP();
                    LOG_SUCCESS_F("Verbunden mit %s (IP: %s)", WiFi.SSID().c_str(), ip.toString().c_str());

                    // mDNS starten, damit das Gerät im Netzwerk gefunden wird
                    const char *hostname = WiFi.getHostname();
                    if (hostname && MDNS.begin(hostname))
                    {
                        LOG_SUCCESS_F("mDNS gestartet: %s.local", hostname);
                    }
                    else
                    {
                        LOG_ERROR("mDNS konnte nicht gestartet werden.");
                    }
                    // Fallback-Flags für den nächsten Versuch zurücksetzen.
                    m_lastTriedProfile = -1;
                    m_genericFallbackTried = false;
                }
            }
            else
            {
                // Bei Disconnect immer mDNS beenden.
                MDNS.end();
            }

            // Globalen App-Status aktualisieren
            APP_STATE.setWifiConnected(currentStatus);
            if (currentStatus)
            {
                int32_t rssi = WiFi.RSSI();
                uint8_t q = rssiToQuality(rssi);
                APP_STATE.setWifiSignalPercent(q);
                APP_STATE.setWifiSignalStrength(q / 10);
            }
            else
            {
                APP_STATE.setWifiSignalPercent(0);
                APP_STATE.setWifiSignalStrength(0);
            }
        }
    }
    else
    {
        // Wenn sich nichts geändert hat, den Timer zurücksetzen.
        m_lastStatusChangeTime = now;
        m_pendingConnected = currentStatus;
    }
}

/**
 * @brief Startet einen asynchronen Scan, wenn erlaubt.
 */
void WifiManager::startAsyncScanIfAllowed(bool force)
{
    uint32_t now = millis();
    // Scan nicht starten, wenn bereits einer läuft, er pausiert ist, oder wir verbunden sind und es nicht explizit erlaubt wurde.
    if (m_scanInProgress || m_scanningSuspended)
        return;
    if (m_connected.load() && !m_scanWhileConnected)
        return;
    // Scan nicht starten, wenn das Intervall noch nicht abgelaufen ist (außer bei `force`).
    if (!force && (now - m_lastScanTime < m_scanIntervalMs))
        return;

    WiFi.scanDelete(); // Alte Ergebnisse löschen

    // Intelligente Scan-Methode: Passiv ist schonender, Aktiv ist gründlicher.
    bool usePassive = m_lastScanWasPassive;
    uint32_t scanTimeMs = usePassive ? 200 : 120; // Passiv länger, aktiv kürzer
    int r = WiFi.scanNetworks(true /*async*/, false /*show_hidden*/, usePassive, scanTimeMs);
    (void)r; // `r` wird nicht verwendet, aber der Aufruf startet den Scan.

    m_scanInProgress = true;
    m_scanStartTime = now;
}

/**
 * @brief Pollt das Ergebnis eines asynchronen Scans und verarbeitet es.
 */
void WifiManager::pollScanResults()
{
    // Wenn kein Scan läuft, prüfen ob ein neuer gestartet werden soll.
    if (!m_scanInProgress)
    {
        if (m_state == ConnectionState::DISCONNECTED || m_state == ConnectionState::ROTATING)
        {
            startAsyncScanIfAllowed();
        }
        return;
    }

    int res = WiFi.scanComplete();
    if (res == WIFI_SCAN_RUNNING)
    {
        // Sicherheits-Timeout, falls der Scan hängen bleibt.
        if (millis() - m_scanStartTime > SCAN_RUNNING_TIMEOUT_MS)
        {
            LOG_WARNING("Scan-Laufzeit überschritten – breche ab");
            WiFi.scanDelete();
            m_scanInProgress = false;
        }
        return;
    }

    // Fehlerfall explizit behandeln (negative Werte außer WIFI_SCAN_RUNNING)
    if (res < 0 && res != WIFI_SCAN_RUNNING)
    {
        LOG_WARNING_F("Scan fehlgeschlagen mit Code: %d", res);
        WiFi.scanDelete();
        m_scanInProgress = false;
        relaxScanInterval();
        return;
    }

    // Wenn der (schonende) passive Scan keine Ergebnisse brachte, einen (aggressiveren) aktiven nachschieben.
    if (res == 0 && m_lastScanWasPassive)
    {
        LOG_INFO("Passiver Scan ohne Ergebnis, starte aktiven Fallback-Scan");
        WiFi.scanDelete();
        m_lastScanWasPassive = false;  // Nächster Scan wird aktiv
        startAsyncScanIfAllowed(true); // Sofort starten
        return;
    }

    // Cache wiederverwenden statt clear()/push_back() (verhindert Fragmentierung)
    m_apCount = 0;

    if (res > 0)
    {
        int n = std::min(res, static_cast<int>(MAX_AP_CACHE));
        for (int i = 0; i < n; ++i)
        {
            APInfo &ap = m_apCache[m_apCount];

            // SSID temporär holen und sofort in char-Array kopieren
            String tempSsid = WiFi.SSID(i);
            strncpy(ap.ssid, tempSsid.c_str(), 32);
            ap.ssid[32] = '\0';

            // BSSID kopieren
            const uint8_t *bssidPtr = WiFi.BSSID(i);
            if (bssidPtr)
            {
                memcpy(ap.bssid, bssidPtr, 6);
            }
            else
            {
                memset(ap.bssid, 0, 6);
            }

            ap.channel = WiFi.channel(i);
            ap.rssi = WiFi.RSSI(i);

            ++m_apCount;
        }
    }

    WiFi.scanDelete();
    m_scanInProgress = false;
    m_lastScanTime = millis();
    m_lastScanWasPassive = true; // Für den nächsten Zyklus wieder mit passiv beginnen.

    LOG_INFO_F("Scan abgeschlossen: %d AP(s)", m_apCount);

    // Wenn wir nach einem frischen Scan offline sind, sofort versuchen zu verbinden.
    if (m_state == ConnectionState::DISCONNECTED)
    {
        if (connectToBestVisibleProfile())
        {
            tightenScanInterval();
        }
        else
        {
            relaxScanInterval();
        }
    }
}

/**
 * @brief Findet den AP mit dem stärksten Signal für eine gegebene SSID im Cache.
 */
const WifiManager::APInfo *WifiManager::findAPForSSID(const char *ssid) const
{
    const APInfo *best = nullptr;
    int bestRSSI = -128;

    // Nur bis m_apCount iterieren (nicht über den gesamten Vector)
    for (uint8_t i = 0; i < m_apCount; ++i)
    {
        if (strcmp(m_apCache[i].ssid, ssid) == 0 && m_apCache[i].rssi > bestRSSI)
        {
            best = &m_apCache[i];
            bestRSSI = m_apCache[i].rssi;
        }
    }
    return best;
}

/**
 * @brief Verdoppelt das Scan-Intervall (bis zum Maximum).
 */
void WifiManager::relaxScanInterval()
{
    if (m_scanIntervalMs < MAX_SCAN_INTERVAL_MS)
    {
        m_scanIntervalMs = std::min(m_scanIntervalMs * 2, MAX_SCAN_INTERVAL_MS);
    }
}

/**
 * @brief Setzt das Scan-Intervall auf das Minimum zurück.
 */
void WifiManager::tightenScanInterval()
{
    m_scanIntervalMs = MIN_SCAN_INTERVAL_MS;
}

/**
 * @brief Setzt den Ländercode für die WiFi-Hardware.
 */
void WifiManager::setCountryCode(const char *countryCode)
{
    wifi_country_t country;
    memset(&country, 0, sizeof(country));
    strncpy(country.cc, countryCode, 2);
    country.cc[2] = '\0';

    // Standard-Kanalbereiche für gängige Regionen
    if (strcmp(countryCode, "US") == 0)
    {
        country.schan = 1;
        country.nchan = 11;
    }
    else if (strcmp(countryCode, "JP") == 0)
    {
        country.schan = 1;
        country.nchan = 14;
    }
    else
    {
        country.schan = 1;
        country.nchan = 13; // Default für Europa/DE
    }
    country.policy = WIFI_COUNTRY_POLICY_MANUAL;

    if (esp_wifi_set_country(&country) == ESP_OK)
    {
        LOG_INFO_F("Country-Code gesetzt: %s (Kanäle %d-%d)",
                   countryCode, country.schan, country.schan + country.nchan - 1);
    }
    else
    {
        LOG_ERROR("Fehler beim Setzen des Country-Codes");
    }
}
