#ifndef ROUTES_H
#define ROUTES_H

class AsyncWebServer;
class LGFX;
class StationManager;

/**
 * @brief Initialisiert alle HTTP-Routen des Webservers
 *
 * @param server Referenz auf den AsyncWebServer
 * @param tft Referenz auf das LGFX-Objekt für die Anzeige
 * @param stationManager Referenz auf den StationManager für Senderverwaltung
 *
 * @note Der Screenshot-Endpunkt verwendet Chunked Transfer für speichereffizientes Streaming
 */
void setupRoutes(AsyncWebServer &server, LGFX &tft, StationManager &stationManager);

// ============================================================================
// Radio-Fernsteuerung SSE-Funktionen
// ============================================================================

/**
 * @brief Sendet den aktuellen Radio-Status als SSE-Event an alle verbundenen Clients
 * @note Wird automatisch bei Statusänderungen aufgerufen
 */
void sendRadioSSEEvent();

/**
 * @brief Aktualisiert den Stationsnamen und sendet SSE-Event
 * @param stationName Name der aktuellen Station
 */
void updateRadioStation(const char* stationName);

/**
 * @brief Aktualisiert den Stream-Titel und sendet SSE-Event
 * @param streamTitle Aktueller Stream-Titel (Artist - Song)
 */
void updateRadioStreamTitle(const char* streamTitle);

/**
 * @brief Aktualisiert die Bitrate und sendet SSE-Event
 * @param bitrate Bitrate als String (z.B. "128 kbps")
 */
void updateRadioBitrate(const char* bitrate);

/**
 * @brief Aktualisiert alle Metadaten (Station, Titel, Bitrate) und sendet EIN SSE-Event
 * @param stationName Stationsname (oder nullptr zum Beibehalten)
 * @param streamTitle Stream-Titel (oder nullptr zum Beibehalten)
 * @param bitrate Bitrate-String (oder nullptr zum Beibehalten)
 */
void updateRadioMetadata(const char* stationName, const char* streamTitle, const char* bitrate);

/**
 * @brief Benachrichtigt über Senderwechsel: Holt den Stationsnamen aus dem RAM-Bestand der Senderverwaltung, leert die Stream-Metadaten und sendet ein SSE-Event an alle Clients
 * @param stationId Index der neuen Station in der Senderliste
 */
void notifyRadioStationChange(int stationId);

// ============================================================================
// Webinterface-Sperre (Display-Sperre -> Web-Sperre)
// ============================================================================

/**
 * @brief Meldet dem Webinterface den Sperrzustand des Displays
 * @param locked true, wenn der Display-Sperrbildschirm gerade aktiv ist
 *
 * Sendet ein SSE-Event an alle verbundenen Browser. Geht das Display in den gesperrten Zustand, sperrt sich das Webinterface beim Empfang sofort selbst. Der entsperrte Zustand wird ebenfalls gemeldet, damit ein neu verbundener Browser den aktuellen Stand kennt. Der Aufruf ist gefahrlos, bevor der Webserver gestartet ist (er wird dann ignoriert).
 */
void notifyDisplayLockChanged(bool locked);

// ============================================================================
// Stations-Reload-Flag (WebUI -> RetroRadio)
// ============================================================================

/**
 * @brief Prüft ob die Senderliste per WebUI geändert wurde und ein Reload nötig ist
 * @return true wenn Stations-Reload ansteht
 */
bool isStationsReloadPending();

/**
 * @brief Setzt das Stations-Reload-Flag zurück (nach erfolgtem Reload)
 */
void clearStationsReloadPending();

#endif // ROUTES_H
