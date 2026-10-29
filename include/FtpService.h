#ifndef FTP_SERVICE_H
#define FTP_SERVICE_H

#include <Arduino.h>
#include <SimpleFTPServer.h>

/**
 * @brief FTP-Server-Service für SD-Karten-Zugriff über das Netzwerk
 *
 * Ermöglicht den Zugriff auf die SD-Karte über FTP-Clients wie FileZilla, Windows Explorer oder Total Commander. Unterstützt Upload, Download, Umbenennen, Löschen und Verzeichnis-Operationen.
 *
 * Basiert auf der Bibliothek "SimpleFTPServer" von xreef (Renzo Mischianti).
 * https://github.com/xreef/SimpleFTPServer
 *
 * Features:
 * - Passive Mode (PASV) für NAT/Firewall-Kompatibilität
 * - Nur eine gleichzeitige Verbindung (schützt SD-Karte)
 * - Automatisches Stoppen/Starten bei Sleep-Mode
 * - Logging über den globalen Logger (Connect/Disconnect/Transfers)
 *
 * Verwendung:
 * @code
 * // In main.cpp (nach WiFi-Verbindung):
 * FTP_SERVICE.begin();
 *
 * // In loop():
 * FTP_SERVICE.handle();
 *
 * // Verbindung über FTP-Client:
 * // ftp://esp32:esp32@192.168.2.50/
 * @endcode
 *
 * Voraussetzungen:
 * 1. platformio.ini:
 *    lib_deps = xreef/SimpleFTPServer
 *    build_flags = -DDEFAULT_STORAGE_TYPE_ESP32=STORAGE_SD
 *
 * 2. SD-Karte muss vor begin() initialisiert sein (SD.begin() in setup()).
 *
 * @note Nur eine gleichzeitige FTP-Verbindung wird unterstützt.
 *       FileZilla: Bearbeiten -> Einstellungen -> Übertragungen ->
 *       Max. gleichzeitige Übertragungen: 1
 *
 * Ports:
 * - Control-Port: 21 (Standard-FTP)
 * - Data-Port:    50009 (Passive Mode)
 */
class FtpService
{
public:
    // ========================================================================
    // Singleton
    // ========================================================================

    /**
     * @brief Zugriff auf die Singleton-Instanz
     * @return Referenz auf die einzige FtpService-Instanz
     */
    static FtpService &instance();

    // Singleton: Kopieren und Zuweisen verbieten
    FtpService(const FtpService &) = delete;
    FtpService &operator=(const FtpService &) = delete;

    /**
     * @brief Gibt die Ressourcen des FTP-Servers frei
     */
    ~FtpService();

    // ========================================================================
    // Lebenszyklus
    // ========================================================================

    /**
     * @brief Startet den FTP-Server
     *
     * Initialisiert den FTP-Server mit den angegebenen Zugangsdaten und bindet ihn an die SD-Karte.
     *
     * @param user FTP-Benutzername (Standard: "esp32")
     * @param pass FTP-Passwort (Standard: "esp32")
     * @return true bei Erfolg, false wenn SD nicht verfügbar
     */
    bool begin(const char *user = "esp32", const char *pass = "esp32");

    /**
     * @brief Stoppt den FTP-Server und trennt aktive Verbindungen
     *
     * Kann z.B. beim Wechsel in den Sleep-Mode aufgerufen werden, um Ressourcen freizugeben.
     */
    void stop();

    /**
     * @brief Verarbeitet FTP-Kommandos und Datentransfers
     *
     * Muss zyklisch in loop() aufgerufen werden. Kehrt sofort zurück, wenn der Server nicht aktiv ist.
     */
    void handle();

    // ========================================================================
    // Status
    // ========================================================================

    /**
     * @brief Prüft ob der FTP-Server läuft
     * @return true wenn begin() erfolgreich war und stop() nicht aufgerufen wurde
     */
    bool isRunning() const { return m_running; }

private:
    /**
     * @brief Privater Konstruktor (Singleton)
     */
    FtpService();

    FtpServer m_ftpServer; ///< FTP-Server-Instanz
    bool m_running = false; ///< Server aktiv

    // ========================================================================
    // FTP-Callbacks (static, da von der Library als C-Funktionszeiger erwartet)
    // ========================================================================

    /**
     * @brief Callback für FTP-Verbindungs-Events
     * @param ftpOperation Art des Events (Connect, Disconnect, FreeSpaceChange)
     * @param freeSpace Freier Speicherplatz in Bytes
     * @param totalSpace Gesamter Speicherplatz in Bytes
     */
    static void onFtpEvent(FtpOperation ftpOperation, uint32_t freeSpace, uint32_t totalSpace);

    /**
     * @brief Callback für FTP-Transfer-Events
     * @param ftpOperation Art des Transfers (Upload/Download Start/End)
     * @param name Dateiname
     * @param transferredSize Übertragene Bytes
     */
    static void onFtpTransfer(FtpTransferOperation ftpOperation, const char *name, uint32_t transferredSize);
};

/**
 * @brief Makro für kürzeren Zugriff auf den FtpService
 *
 * Verwendung:
 * @code
 * FTP_SERVICE.begin();
 * FTP_SERVICE.handle();
 * @endcode
 */
#define FTP_SERVICE FtpService::instance()

#endif // FTP_SERVICE_H
