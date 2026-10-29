#include "FtpService.h"

#include <SD.h>
#include <inttypes.h>

#include "Logger.h"

// ===================================================================
// Singleton
// ===================================================================

/**
 * @brief Zugriff auf die Singleton-Instanz
 */
FtpService &FtpService::instance()
{
    static FtpService instance;
    return instance;
}

FtpService::FtpService() = default;

/**
 * @brief Gibt die Ressourcen des FTP-Servers frei
 */
FtpService::~FtpService()
{
    stop();
}

// ===================================================================
// Lebenszyklus
// ===================================================================

/**
 * @brief Startet den FTP-Server
 */
bool FtpService::begin(const char *user, const char *pass)
{
    if (m_running)
    {
        LOG_WARNING("FTP-Server läuft bereits");
        return true;
    }

    // SD-Karte prüfen
    if (SD.cardType() == CARD_NONE)
    {
        LOG_ERROR("FTP-Server: Keine SD-Karte gefunden!");
        return false;
    }

    // Callbacks registrieren (vor begin!)
    m_ftpServer.setCallback(onFtpEvent);
    m_ftpServer.setTransferCallback(onFtpTransfer);

    // FTP-Server starten
    m_ftpServer.begin(user, pass);
    m_running = true;

    LOG_SUCCESS_F("FTP-Server gestartet (User: %s, Ports: 21/50009)", user);
    return true;
}

/**
 * @brief Stoppt den FTP-Server und trennt aktive Verbindungen
 */
void FtpService::stop()
{
    if (m_running)
    {
        m_ftpServer.end();
        m_running = false;
        LOG_INFO("FTP-Server gestoppt");
    }
}

/**
 * @brief Verarbeitet FTP-Kommandos und Datentransfers
 */
void FtpService::handle()
{
    if (m_running)
        m_ftpServer.handleFTP();
}

// ===================================================================
// FTP-Callbacks
// ===================================================================

/**
 * @brief Callback für FTP-Verbindungs-Events
 */
void FtpService::onFtpEvent(FtpOperation ftpOperation, uint32_t freeSpace, uint32_t totalSpace)
{
    switch (ftpOperation)
    {
    case FTP_CONNECT:
        LOG_INFO("FTP: Client verbunden");
        break;

    case FTP_DISCONNECT:
        LOG_INFO("FTP: Client getrennt");
        break;

    case FTP_FREE_SPACE_CHANGE:
        LOG_INFO_F("FTP: Speicher geändert – %" PRIu32 " von %" PRIu32 " KB frei",
                   freeSpace / 1024, totalSpace / 1024);
        break;

    default:
        break;
    }
}

/**
 * @brief Callback für FTP-Transfer-Events
 */
void FtpService::onFtpTransfer(FtpTransferOperation ftpOperation, const char *name, uint32_t transferredSize)
{
    switch (ftpOperation)
    {
    case FTP_UPLOAD_START:
        LOG_INFO_F("FTP: Upload gestartet → %s", name);
        break;

    case FTP_UPLOAD:
        LOG_INFO_F("FTP: Upload abgeschlossen → %s (%" PRIu32 " Bytes)", name, transferredSize);
        break;

    case FTP_DOWNLOAD_START:
        LOG_INFO_F("FTP: Download gestartet → %s", name);
        break;

    case FTP_DOWNLOAD:
        LOG_INFO_F("FTP: Download abgeschlossen → %s (%" PRIu32 " Bytes)", name, transferredSize);
        break;

    default:
        break;
    }
}
